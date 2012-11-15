#ifndef __CG__COMMON__H
#define __CG__COMMON__H

#include "Utils.h"


#if 0
	#define CG_PACKED __attribute__((packed))
#else
	#define CG_PACKED 
#endif

//////////////////////////////////////////////////////////////////////////
//
// s_header
//
//////////////////////////////////////////////////////////////////////////

struct s_header {
	short version;
	short size;
} CG_PACKED;

//////////////////////////////////////////////////////////////////////////
//
// globalSettings
//
//////////////////////////////////////////////////////////////////////////

struct globalSettings {
	bool isEmergencyShutdownEnabled : 1;
	bool isSleepModeEnabled : 1;
	bool isSoftStartEnabled : 1;
	bool isSoftStopEnabled : 1;
	// TODO: bool isMuteSound : 1;
} CG_PACKED settings;

//////////////////////////////////////////////////////////////////////////
//
// presetPage
//
/////////////////////////////////////////////////////////////////////////

struct presetPage {
	short userRunTimeMinutes;	// user run time is specified in minutes
	short userRpm;
} CG_PACKED;

//////////////////////////////////////////////////////////////////////////
//
// runningState
//
//////////////////////////////////////////////////////////////////////////

struct runningState {
	int desiredMotorSpeed255;		// the desired motor speed (0-255)
	uint32 endTime;					// time at which the run should end
	volatile int observedRpm;		// updated by an interrupt
	int prevRpm;					// used to determine when the LCD should be updated with the new RPM
} CG_PACKED state;


// NOTE: be careful with this. the atmega328 only has 2kb of sram available, even less on the 168 (1kb)
const int MAX_BUFFER_SIZE = 512;

const int NUM_PAGES = 5;
presetPage pages[ NUM_PAGES ];
int currentPage = 0;

// NOTE: update this whenever any of the structures below change
static const short SC_VERSION = 6;
static const short SETTINGS_SIZE = sizeof( s_header ) + sizeof( globalSettings ) + sizeof( presetPage ) * NUM_PAGES;

void factoryReset();


#endif // __CG__COMMON__H
