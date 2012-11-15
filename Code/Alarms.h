#ifndef __ALARMS__H__
#define __ALARMS__H__

// arduino required includes:
#include "Pins.h"
#include <LiquidCrystal.h>


void startAlarm( LiquidCrystal &lcd ) {
  	lcd.setCursor( 0, 1 );
	lcd.print( "3... " );

	for ( int i = 0; i < 500; ++i ) {
		digitalWrite(pwmOutSpeakerPin, HIGH);
		delayMicroseconds( 500 );
		digitalWrite(pwmOutSpeakerPin, LOW);
		delayMicroseconds( 500 );
	}

	delay( 500 );

	lcd.print( "2... " );

	for ( int i = 0; i < 500; ++i ) {
		digitalWrite(pwmOutSpeakerPin, HIGH);
		delayMicroseconds( 500 );
		digitalWrite(pwmOutSpeakerPin, LOW);
		delayMicroseconds( 500 );
	}

	delay( 500 );

	lcd.print( "1... " );

	for ( int i = 0; i < 1000; ++i ) {
		digitalWrite(pwmOutSpeakerPin, HIGH);
		delayMicroseconds( 500 );
		digitalWrite(pwmOutSpeakerPin, LOW);
		delayMicroseconds( 500 );
	}
}

void doneAlarm( int times ) {
	for ( int i = 0; i < times; ++i ) {
		for ( int j = 0; j < 500; ++j ) {
			digitalWrite(pwmOutSpeakerPin, HIGH);
			delayMicroseconds( 500 - j );
			digitalWrite(pwmOutSpeakerPin, LOW);
			delayMicroseconds( 500 - j );
		}
		for ( int j = 0; j < 500; ++j ) {
			digitalWrite(pwmOutSpeakerPin, HIGH);
			delayMicroseconds( j );
			digitalWrite(pwmOutSpeakerPin, LOW);
			delayMicroseconds( j );
		}
	}
}

void emergencyAlarm( int times ) {
	for ( int i = 0; i < times; ++i ) {
		for ( int j = 0; j < 600; ++j ) {
			digitalWrite(pwmOutSpeakerPin, HIGH);
			delayMicroseconds( 600 - j );
			digitalWrite(pwmOutSpeakerPin, LOW);
			delayMicroseconds( 600 - j );
		}
		for ( int j = 0; j < 600; ++j ) {
			digitalWrite(pwmOutSpeakerPin, HIGH);
			delayMicroseconds( j );
			digitalWrite(pwmOutSpeakerPin, LOW);
			delayMicroseconds( j );
		}
	}
}

void lowBatteryAlarm( int times ) {
	for ( int i = 0; i < times; ++i ) {
		for ( int j = 0; j < 600; ++j ) {
			digitalWrite(pwmOutSpeakerPin, HIGH);
			delayMicroseconds( 700 );
			digitalWrite(pwmOutSpeakerPin, LOW);
			delayMicroseconds( 700 );
		}
	}
}


#endif
