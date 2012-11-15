/********************************************************************
author:		Aurelio Reis
*********************************************************************/

#include <LiquidCrystal.h>
#include <avr/pgmspace.h>
#include <EEPROM.h>
#include <avr/sleep.h>
#include "Utils.h"
#include "SaveLoad.h"

//#define DISABLE_SOFTSTARTSTOP
#define DISABLE_RPM


const int minTimeMinutes = 1;
const int maxTimeMinutes = 360;

// NOTE: these numbers are determined by the motor limits but likely 
// should be exposed for modification and storage on EEPROM
const int minRpm = 20;
const int maxRpm = 40;

// in reality the voltage drop is non-linear so I'm remapping
// to what I've observed through testing
const int realMinRpm = 30;
const int realMaxRpm = 40;

// same as above but scaled for analogWrite() 8-bit range
const int maxRpm255 = 255;
const int minRpm255 = int( ( minRpm / (float)maxRpm ) * 255.0f );

bool isStartSwitchOn = false;
bool wasStartSwitchOn = false;
uint32 lastRotTimeMs = millis();
bool startSwitchStateBlocker = isButtonOnPu( digiInStartSwitchPin );
uint32 runStartTime = 0;

uint32 softStartRampupTime = 1500; // 1.5 seconds
float invSoftStartRampupTime = 1.0f / (float)softStartRampupTime;
uint32 softStopTimeMs = 1000;  // 1 seconds
uint32 timeElapsedSinceStart = 0;

const int MAX_IDLE_TIME_FOR_SLEEP_MS = 10 * 60 * 1000;	// 10 minutes
static uint32 sleepIdleStartTime = millis();

LiquidCrystal lcd( LCD_RS, LCD_E, LCD_D4, LCD_D5, LCD_D6, LCD_D7 );

enum ELcdMode { LCDM_ENTRY, LCDM_RUNNING, NUM_LCDMODES } lcdScreenMode = LCDM_ENTRY;

prog_char string_0[] PROGMEM = "   smartCaster";
prog_char string_1[] PROGMEM = "RUNNING TIME:";
prog_char string_2[] PROGMEM = "String 2";
prog_char string_3[] PROGMEM = "String 3";
prog_char string_4[] PROGMEM = "String 4";
prog_char string_5[] PROGMEM = "String 5";

PROGMEM const char *staticStringPool[] = {   
	string_0,
	string_1,
	string_2,
	string_3,
	string_4,
	string_5
};

enum { SP_SMARTCASTER = 0, SP_RUNNING, SP_2, SP_3, SP_4, SP_5 };

static char _tempStringBuffer[ 30 ];	// make sure this is large enough for the largest string it must hold

//////////////////////////////////////////////////////////////////////////
// getStaticString
//////////////////////////////////////////////////////////////////////////

const char *getStaticString( int StringVal ) {
	// Necessary casts and dereferencing, just copy. 
	strcpy_P( _tempStringBuffer, (char*)pgm_read_word( &( staticStringPool[ StringVal ] ) ) );

	return _tempStringBuffer;
}

//////////////////////////////////////////////////////////////////////////
// initState
//////////////////////////////////////////////////////////////////////////

void initState() {
	memset( &state, 0, sizeof( state ) );
}

//////////////////////////////////////////////////////////////////////////
// resetPagesToDefault
//////////////////////////////////////////////////////////////////////////

void resetPagesToDefault() {
	for ( int i = 0; i < NUM_PAGES; ++i ) {
		pages[ i ].userRpm = 20;
		pages[ i ].userRunTimeMinutes = 30;
	}
}

//////////////////////////////////////////////////////////////////////////
// curPage
//////////////////////////////////////////////////////////////////////////

struct presetPage *curPage() { return &pages[ currentPage ]; }

//////////////////////////////////////////////////////////////////////////
// nextPage
//////////////////////////////////////////////////////////////////////////

void nextPage() {
	currentPage = ( currentPage + 1 ) % NUM_PAGES;
}

//////////////////////////////////////////////////////////////////////////
// prepareRun
//////////////////////////////////////////////////////////////////////////

void prepareRun() {
	saveSettings();

	timeElapsedSinceStart = 0;
	runStartTime = millis();
	lastRotTimeMs = millis();

	initState();
	state.prevRpm = 99999; // force update
	state.endTime = millis() + minutesToMilliseconds( curPage()->userRunTimeMinutes );	
	state.desiredMotorSpeed255 = map( curPage()->userRpm, minRpm, maxRpm, minRpm255, maxRpm255 );
	state.observedRpm = curPage()->userRpm;

	// whenever the interrupt pin goes high the interrupt is triggered
#ifndef DISABLE_RPM
	attachInterrupt( digiInRpmMonitorInterruptPin, interrupt_rotationDetected, LOW );
#endif
}

//////////////////////////////////////////////////////////////////////////
// lowBatteryCheck
//////////////////////////////////////////////////////////////////////////

void lowBatteryCheck() {
	int batLevel = calculateBatteryLevel( analogRead( anagInBatteryLevelPin ) );

	if ( batLevel < 15 ) {
		lcd.clear();
		lcd.setCursor( 0, 0 );
		lcd.print( "   *WARNING*" );
		lcd.setCursor( 0, 1 );
		lcd.print( " *BATTERY LOW*" );
		lowBatteryAlarm( 2 );
		delay( 1500 );
	}
}

//////////////////////////////////////////////////////////////////////////
// stopMotor
//////////////////////////////////////////////////////////////////////////

void stopMotor() {
	if ( settings.isSoftStopEnabled ) {
		uint32 stopTimeMs = millis();
		float motorSpeedDelta = 1.0f;
		int motorSpeed = 0;

		do {
			float stopTimeDifMs = float( millis() - stopTimeMs );
			float motorSpeedDelta = max( 0.0f, min( 1.0f, stopTimeDifMs / (float)softStopTimeMs ) );
			motorSpeed = (int)smoothstep( state.desiredMotorSpeed255, minRpm255, motorSpeedDelta );
			analogWrite( pwmOutMotorPin, motorSpeed );

			if ( verbose ) {
				Serial.print( "motor speed: " );
				Serial.print( motorSpeed );
				Serial.print( "\n" );
			}
		} while ( motorSpeed > minRpm255 );
	}

	analogWrite( pwmOutMotorPin, 0 );
}

//////////////////////////////////////////////////////////////////////////
// beginRun
//////////////////////////////////////////////////////////////////////////

void beginRun() {
	lowBatteryCheck();

	lcd.clear();
	lcd.setCursor( 0, 0 );
	lcd.print( "**RUN STARTING**" );

	startAlarm( lcd );

	lcdScreenMode = LCDM_RUNNING;
	lcd.clear();

	prepareRun();
}

//////////////////////////////////////////////////////////////////////////
// finalizeRun
//////////////////////////////////////////////////////////////////////////

void finalizeRun() {
	detachInterrupt( digiInRpmMonitorInterruptPin );
	isStartSwitchOn = false;
	lcdScreenMode = LCDM_ENTRY;
	startSwitchStateBlocker = isButtonOnPu( digiInStartSwitchPin );	// ensure start switch is turned off by user
	resetSleepTimer();
}

//////////////////////////////////////////////////////////////////////////
// endRun
//////////////////////////////////////////////////////////////////////////

void endRun() { 
	if ( lcdScreenMode != LCDM_RUNNING ) {
		return;
	}

	lcd.clear();
	lcd.setCursor( 0, 0 );
	lcd.print( "*RUN COMPLETED*" );

	doneAlarm( 4 );

	stopMotor();

	finalizeRun();

	lcd.clear();
}

//////////////////////////////////////////////////////////////////////////
// emergencyShutdown
//////////////////////////////////////////////////////////////////////////

void emergencyShutdown() {        
	lcd.clear();
	lcd.setCursor( 0, 0 );
	lcd.print( "*EMERGENCY STOP*" );
	lcd.setCursor( 0, 1 );
	lcd.print( "  LOW RPM SPEED" );

	emergencyAlarm( 4 );

	stopMotor();

	finalizeRun();

	lcd.clear();
}

//////////////////////////////////////////////////////////////////////////
// resetSettings
//////////////////////////////////////////////////////////////////////////

void resetSettings() {
	memset( &settings, 0xFF, sizeof( settings ) );	// NOTE: all enabled (1)  

#ifdef DISABLE_SOFTSTARTSTOP
	settings.isSoftStartEnabled = false;
	settings.isSoftStopEnabled = false;
#endif
}

//////////////////////////////////////////////////////////////////////////
// factoryReset
//////////////////////////////////////////////////////////////////////////

void factoryReset() {
	resetPagesToDefault();
	resetSettings();
	saveSettings();
}

//////////////////////////////////////////////////////////////////////////
// interrupt_rotationDetected
//////////////////////////////////////////////////////////////////////////

bool rotationDetected = false;

	// NOTE: this function is time sensitive -- need to ensure this is fast enough,
	// perhaps by removing fp math or moving logic to main loop
void interrupt_rotationDetected() {
	if ( LCDM_RUNNING != lcdScreenMode ) {
		return;
	}

	rotationDetected = true;
}

//////////////////////////////////////////////////////////////////////////
// resetSleepTimer
//////////////////////////////////////////////////////////////////////////

void resetSleepTimer() {
	sleepIdleStartTime = millis();
}

//////////////////////////////////////////////////////////////////////////
// interrupt_wakeUp
//////////////////////////////////////////////////////////////////////////

void interrupt_wakeUp() {

}

//////////////////////////////////////////////////////////////////////////
// enterSleep
//////////////////////////////////////////////////////////////////////////

void enterSleep() {
	if ( verbose ) {
		Serial.println( "Entering Sleep..." );
	}

	// enter sleep
	digitalWrite( digiOutPowerSaveSupply, LOW );
	lcd.noDisplay();
	set_sleep_mode( SLEEP_MODE_PWR_DOWN );
	sleep_enable();
	attachInterrupt( digiInSleepInterruptPin, interrupt_wakeUp, CHANGE );
	sleep_mode(); 

	// exit sleep
	sleep_disable();
	detachInterrupt( digiInSleepInterruptPin );
	digitalWrite( digiOutPowerSaveSupply, HIGH );
	lcd.display();
	resetSleepTimer();

	if ( verbose ) {
		Serial.println( "Exiting Sleep..." );
	}
}

//////////////////////////////////////////////////////////////////////////
// setup
//////////////////////////////////////////////////////////////////////////

void setup() {
	lcd.begin( 16, 2 );
	lcd.clear();

	pinMode( digiInStartSwitchPin, INPUT );
	digitalWrite( digiInStartSwitchPin, HIGH );  // enable internal pull-up resistor

	pinMode( digiInUpButtonPin, INPUT );
	digitalWrite( digiInUpButtonPin, HIGH );  // enable internal pull-up resistor

	pinMode( digiInDownButtonPin, INPUT );
	digitalWrite( digiInDownButtonPin, HIGH );  // enable internal pull-up resistor

	pinMode( digiInPageButtonPin, INPUT );
	digitalWrite( digiInPageButtonPin, HIGH );  // enable internal pull-up resistor

	pinMode( digiOutPowerSaveSupply, OUTPUT );

	pinMode( pwmOutMotorPin, OUTPUT );
	pinMode( pwmOutSpeakerPin, OUTPUT );
	
	// THIS IS A TEST:
	pinMode( digiInSleepInterruptPin, INPUT );

	Serial.begin( 9600 );

	// reset to blank state then load from EEPROM
	resetPagesToDefault();
	resetSettings();
	// FIXME:
	//loadSettings();

	// make sure power is off to motor
	analogWrite( pwmOutMotorPin, 0 );
}

//////////////////////////////////////////////////////////////////////////
// blockingSwitchNagScreen
//////////////////////////////////////////////////////////////////////////

// make sure they don't power-up the machine with the start switch on
void blockingSwitchNagScreen() {
	if ( true == startSwitchStateBlocker ) {
		lcd.clear();
		lcd.setCursor( 0, 0 );
		lcd.print( "  *SWITCH OFF*" );
		lcd.setCursor( 0, 1 );
		lcd.print( " *TO CONTINUE*" );

		while ( isButtonOnPu( digiInStartSwitchPin ) ) {
			// make sure power is off to motor
			analogWrite( pwmOutMotorPin, 0 );

			delay( 500 );
		}

		startSwitchStateBlocker = false;
		resetSleepTimer();
	}
}

//////////////////////////////////////////////////////////////////////////
// updateLcdInput
//////////////////////////////////////////////////////////////////////////

static bool modeButtonDebounce = false;
static uint32 upButtonDebounceTimerMs = 0;
static uint32 downButtonDebounceTimerMs = 0;
const uint32 DEBOUNCE_TIMER_STEP = 100;

void updateLcdInput() {
	switch( lcdScreenMode ) {
		case LCDM_ENTRY:
		{
			if ( isButtonOnPu( digiInPageButtonPin ) ) {
				if ( false == modeButtonDebounce ) {
					modeButtonDebounce = true;
					nextPage();
					
					if ( verbose ) {
						Serial.println( "PAGE" );
					}					
				}
			} else {
				if ( true == modeButtonDebounce ) {
					//Serial.println( "OFF" );
				}
				modeButtonDebounce = false;
			}

			if ( isButtonOnPu( digiInUpButtonPin ) && upButtonDebounceTimerMs < millis() ) {
				curPage()->userRunTimeMinutes++;
				upButtonDebounceTimerMs = millis() + DEBOUNCE_TIMER_STEP;
				
				if ( verbose ) {
					Serial.println( "UP" );
				}
			}

			if ( isButtonOnPu( digiInDownButtonPin ) && downButtonDebounceTimerMs < millis() ) {
				curPage()->userRunTimeMinutes--;
				downButtonDebounceTimerMs = millis() + DEBOUNCE_TIMER_STEP;

				if ( verbose ) {
					Serial.println( "DOWN" );
				}
			}
			
			// THIS IS A TEST
			static int prevstate = digitalRead( digiInSleepInterruptPin );
			if ( prevstate != digitalRead( digiInSleepInterruptPin ) ) {
				prevstate = digitalRead( digiInSleepInterruptPin );
				//if ( verbose ) {
					Serial.println( "STATE CHANGED!" );
				//}
			}

			curPage()->userRunTimeMinutes = constrain( curPage()->userRunTimeMinutes, minTimeMinutes, maxTimeMinutes );

			curPage()->userRpm = constrain( curPage()->userRpm, minRpm, maxRpm );

			// Fix for imprecision with certain sliders at extremes
			if ( curPage()->userRpm > ( maxRpm - 2 ) ) {
				curPage()->userRpm = maxRpm;
			}

			// set rpm speed
			int sliderValueAnag = analogRead( anagInSliderPin );
			curPage()->userRpm = map( sliderValueAnag, 0, 1023, minRpm, maxRpm );
			
			if ( verbose ) {
				static int prevSliderValue = analogRead( anagInSliderPin );
				
				if ( prevSliderValue != sliderValueAnag ) {
					Serial.print( "Slider Value: " );
					Serial.print( sliderValueAnag );  
					Serial.print( "=>" );
					Serial.println( curPage()->userRpm );
				}
			}
		}
		break;
	}
}

//////////////////////////////////////////////////////////////////////////
// updateLcd
//////////////////////////////////////////////////////////////////////////

void updateLcd() {
	switch( lcdScreenMode ) {
		default:
			lcd.print( "*SYSTEM ERROR*" );
		break;

		case LCDM_ENTRY:
		{
			lcd.setCursor( 0, 0 );
			lcd.print( "PG#" );
			lcd.print( currentPage + 1 );

			lcd.print( "   TIME:" );
			lcd.print( curPage()->userRunTimeMinutes );
			lcd.print( "   " );  // clear margin

			int realRpm = map( curPage()->userRpm, minRpm, maxRpm, realMinRpm, realMaxRpm );
			
			lcd.setCursor( 0, 1 );
			lcd.print( "RPM:" );
			lcd.print( realRpm );

			lcd.print( " BAT:" );
			int batLevel = calculateBatteryLevel( analogRead( anagInBatteryLevelPin ) );

			lcd.print( batLevel );
			lcd.print( "   " );
		}
		break;

		case LCDM_RUNNING:
		{
			lcd.setCursor( 0, 0 );
			lcd.print( getStaticString( SP_RUNNING ) );

			if ( state.prevRpm != state.observedRpm ) {
				state.prevRpm = state.observedRpm;

				uint32 timeRemainingMins = msToMinutes( max( 0.0f, state.endTime - millis() ) );
				// +1 because we are always under the specified amount but it looks odd to be
				// at 0 minutes and not be done
				lcd.print( max( 1, timeRemainingMins ) );

				int realRpm = map( state.observedRpm, minRpm, maxRpm, realMinRpm, realMaxRpm );
				
				lcd.setCursor( 0, 1 );
				lcd.print( " RPM:" );
				lcd.print( realRpm );

				lcd.print( "  BAT:" );
				int batLevel = calculateBatteryLevel( analogRead( anagInBatteryLevelPin ) );
				lcd.print( batLevel );
				lcd.print( "   " );
			}
		}
		break;
	}

	// TODO: only update infrequently but don't block rest of program
	//delay( 100 );
}

//////////////////////////////////////////////////////////////////////////
// drawLettersDelayed
//////////////////////////////////////////////////////////////////////////

#define LETTER_AND_DELAY( _letter, _delay )    lcd.print( _letter );    delay( _delay );

void drawLettersDelayed( const char *text, int delayTime ) {
	int len = strlen( text );

	for ( int i = 0; i < len; ++i ) {
		lcd.print( text[ i ] );
		delay( delayTime );
	}
}

//////////////////////////////////////////////////////////////////////////
// blockingRunIntro
//////////////////////////////////////////////////////////////////////////

void blockingRunIntro() {
	static bool introCompleted = false;

	if ( false == introCompleted ) {
		lcd.clear();
		lcd.setCursor( 0, 0 );

		drawLettersDelayed( getStaticString( SP_SMARTCASTER ), 150 );
		delay( 500 );

		// check for factory reset bootup option
		if (	isButtonOnPu( digiInPageButtonPin ) &&
			isButtonOnPu( digiInUpButtonPin ) &&
			isButtonOnPu( digiInDownButtonPin ) ) {
				factoryReset();

				lcd.clear();
				lcd.setCursor( 0, 0 );
				lcd.print( "*FACTORY RESET*" );
				delay( 2000 );
				// disable safety checks?
		} else 	if (	isButtonOnPu( digiInPageButtonPin ) &&
			isButtonOnPu( digiInDownButtonPin ) ) {
				settings.isEmergencyShutdownEnabled = false;

				lcd.clear();
				lcd.setCursor( 0, 0 );
				lcd.print( "*SAFE-SHUTDOWN*" );
				lcd.setCursor( 0, 1 );
				lcd.print( "  *DISABLED*" );
				delay( 2000 );
				// disable sleep?
		} else 	if (	isButtonOnPu( digiInPageButtonPin ) &&
			isButtonOnPu( digiInUpButtonPin ) ) {
				settings.isSleepModeEnabled = false;

				lcd.clear();
				lcd.setCursor( 0, 0 );
				lcd.print( "   *SLEEP*" );
				lcd.setCursor( 0, 1 );
				lcd.print( "  *DISABLED*" );
				delay( 2000 );		
				// export settings
		} else 	if (	isButtonOnPu( digiInPageButtonPin ) ) {
			lcd.clear();
			lcd.setCursor( 0, 0 );
			lcd.print( "   *EXPORTING*" );
			lcd.setCursor( 0, 1 );
			lcd.print( "   *SETTINGS*" );
			delay( 2000 );		

			exportSettingsToSerial();

			lcd.clear();
			lcd.setCursor( 0, 0 );
			lcd.print( "   *SUCCESS!*" );
			// import settings
		} else 	if (	isButtonOnPu( digiInUpButtonPin ) ) {
			lcd.clear();
			lcd.setCursor( 0, 0 );
			lcd.print( "   *IMPORTING*" );
			lcd.setCursor( 0, 1 );
			lcd.print( "   *SETTINGS*" );
			delay( 2000 );		

			lcd.clear();
			lcd.setCursor( 0, 0 );
			lcd.print( "*WAITING FOR*" );
			lcd.setCursor( 0, 1 );
			lcd.print( "    *DATA*" );
			delay( 2000 );		

			if ( true == importSettingsFromSerial() ) {
				lcd.clear();
				lcd.setCursor( 0, 0 );
				lcd.print( "   *SUCCESS!*" );
			} else {
				lcd.clear();
				lcd.setCursor( 0, 0 );
				lcd.print( "   *FAILURE!*" );
			}
		}

		introCompleted = true;
		resetSleepTimer();
	}
}

//////////////////////////////////////////////////////////////////////////
// updateMotor
//////////////////////////////////////////////////////////////////////////

void updateMotor() {
	if ( settings.isSoftStartEnabled ) {
		timeElapsedSinceStart = millis() - runStartTime;
	} else {
		timeElapsedSinceStart = softStartRampupTime;
	}

	int motorSpeed = state.desiredMotorSpeed255;

	if ( timeElapsedSinceStart < softStartRampupTime ) {
		float softStartPerc = min( 1.0f, timeElapsedSinceStart * invSoftStartRampupTime );
		motorSpeed = (int)smoothstep( minRpm255, motorSpeed, softStartPerc );
	}

	if ( verbose ) {
		Serial.print( "motor speed: " );
		Serial.print( motorSpeed );
		Serial.print( "\n" );
	}

	analogWrite( pwmOutMotorPin, motorSpeed );
}

void updateRpm() {
	uint32 timeMs = millis();

	if ( !rotationDetected || timeMs == lastRotTimeMs ) {
		return;
	}

	rotationDetected = false;
	Serial.println( timeMs );
	static const uint32 DBZ_EPSILON = 1;
	uint32 timeDifMs = ( timeMs - lastRotTimeMs ) + DBZ_EPSILON;

	static uint32 maxTimePerRotation = uint32( ( (float)maxRpm / 60.0f ) * 1000.0f );

	// cheap software debounce -- ideal circuit would use an rc filter
	if ( timeDifMs < maxTimePerRotation ) {
		return;
	}

	lastRotTimeMs = timeMs;
	
	float rps = 1000.0f / (float)timeDifMs;
	state.observedRpm = max( 0, min( 999, (int)( rps * 60.0f ) ) );

	// if the rpm is above/below a certain threshold, emergency shut down (if option is enabled)
	// FIXME: currently disabled
	if ( 0 && settings.isEmergencyShutdownEnabled ) {
		// Soft start requires a grace period so skip check
		if ( timeElapsedSinceStart < softStartRampupTime ) {
			return;
		}

		static const float rpmThreshold = 0.25f;
		float rpmPerc = state.observedRpm / (float)curPage()->userRpm;
		if ( abs( rpmPerc - 1.0f ) > rpmThreshold ) {
			emergencyShutdown();
		}
	}
}

//////////////////////////////////////////////////////////////////////////
// loop
//////////////////////////////////////////////////////////////////////////

void loop() {
	blockingRunIntro();
	blockingSwitchNagScreen();

	isStartSwitchOn = isButtonOnPu( digiInStartSwitchPin );

	if ( isStartSwitchOn ) {
		if ( isStartSwitchOn != wasStartSwitchOn ) {
			beginRun();
		}

		updateMotor();
		updateRpm();

		// has the run timer completed? if so end run
		if ( millis() >= state.endTime ) {
			endRun();
		}
	} else {
		if ( isStartSwitchOn != wasStartSwitchOn ) {
			endRun();
		}
	}

	updateLcdInput();
	updateLcd();

	wasStartSwitchOn = isStartSwitchOn;

	// if we are not running check to see if its time to enter standby mode to conserve power
	if ( settings.isSleepModeEnabled && LCDM_RUNNING != lcdScreenMode ) {
		int timeIdle = millis() - sleepIdleStartTime;
		if ( timeIdle >= MAX_IDLE_TIME_FOR_SLEEP_MS ) {
			//enterSleep();
		}
	}

	// TODO: slight delay to reduce update rate and conserve power,
	// OR, separate slower update for non-criticals like sleep timer
}
