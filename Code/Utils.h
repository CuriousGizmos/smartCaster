#ifndef __UTILS__H__
#define __UTILS__H__

// arduino required includes:
#include "Arduino.h"

#include "ByteStream.h"
#include "Alarms.h"


// enable for verbose serial debugging prints
static const int verbose = false; // FIXME:

typedef unsigned long uint32;


//////////////////////////////////////////////////////////////////////////
// calculateBatteryLevel
//////////////////////////////////////////////////////////////////////////

// 0-5v is mapped to 0-1024 (10bits), so 5 volts / 1024 units or, .0049  volts.

// calculate the battery level (0-100) from the input voltage (expressed in analog 10-bit, 0-1023)
int calculateBatteryLevel( int inputVoltAnag ) {
	// NOTE: charge is not linear so this is merely an approximation
	static const float max12v = 12.7;				// full battery charge
	static const float min12v = 11.9f;				// battery dischargeed
	static const float vRange = max12v - min12v;

	// expected voltage from anagInBatteryLevelPin is 3v, which is remapped to the 5v expected.	
	const float ratio3to5v = ( 3.0f / 5.0f ) * ( max12v / 12.0f );
	const float maxVoltAnag = 1023.0f * ratio3to5v;

	// map the input voltage (which is lower than standard 5v to the battery max)
	float inputVolt = constrain( (float)inputVoltAnag / maxVoltAnag, 0.0f, 1.0f ) * max12v;
	
	// remap the [0,max] range to [min,max] then [0,100]
	return (int)( ( max( 0.0f, inputVolt - min12v ) / vRange ) * 100.0f );
}

//////////////////////////////////////////////////////////////////////////
// lerp
//////////////////////////////////////////////////////////////////////////

float lerp( float a, float b, float l ) {
	return a + ( b - a ) * l;
}

//////////////////////////////////////////////////////////////////////////
// smoothstep
//////////////////////////////////////////////////////////////////////////

#define SMOOTHSTEP(x) ((x) * (x) * (3 - 2 * (x)))

float smoothstep( float a, float b, float l ) {
	return lerp( a, b, SMOOTHSTEP( l ) );
}

//////////////////////////////////////////////////////////////////////////
// isButtonOnPu
//////////////////////////////////////////////////////////////////////////

#define BUTTON_ON_PU		LOW
#define BUTTON_LOW_PU		HIGH

bool isButtonOnPu( int input ) {
	// pulled-up buttons are on when low
	return digitalRead( input ) == BUTTON_ON_PU;
}

//////////////////////////////////////////////////////////////////////////
// isButtonOn
//////////////////////////////////////////////////////////////////////////

bool isButtonOn( int input ) {
	return digitalRead( input ) == HIGH;
}

//////////////////////////////////////////////////////////////////////////
// minutesToMilliseconds
//////////////////////////////////////////////////////////////////////////

uint32 minutesToMilliseconds( uint32 minutes ) { return minutes * 60000; }	// 60secs in a minute, 1000ms in a sec

//////////////////////////////////////////////////////////////////////////
// msToMinutes
//////////////////////////////////////////////////////////////////////////

uint32 msToMinutes( uint32 ms ) { return uint32( ( ms / 60000.0f ) + 0.5f ); }	// 1000ms in a second, 60secs in a minute, round up0


#define assert( _CND ) { if ( (bool)( _CND ) != true ) { Serial.println( "ASSERT TRIGGERED: " ##_CND ); } }


#endif // __UTILS__H__
