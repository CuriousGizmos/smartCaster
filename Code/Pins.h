#ifndef __PINS__H__
#define __PINS__H__


// NOTE: pin0 (rx), pin1 (tx) should remain unused

const int digiInRpmMonitorInterruptPin = 0;	// NOTE: interrupt 0 is pin 2
const int digiInSleepInterruptPin = 1;		// NOTE: interrupt 1 is pin 3
const int pwmOutSpeakerPin = 10;
const int pwmOutMotorPin = 11;
const int digiInStartSwitchPin = 12;
const int digiInDownButtonPin = 13;
const int UNUSEDPIN14 = 14;//A0
const int digiOutPowerSaveSupply = 15;//A1
const int anagInSliderPin = 16;//A2;
const int digiInUpButtonPin = 17;//A3;
const int digiInPageButtonPin = 18;//A4;
const int anagInBatteryLevelPin = 19;//A5;

/*
INTERFACE CONNECTOR:
1   vcc
2   common ground
3   b2    a4(18)
4   b1    a3(17)
5   pot0  a2(16)
6   b0	  13
7   s0	  12
*/

/*
LCD CONNECTOR
14 13 12 11 10  9  8  7  6  5  4  3  2  1 -L +L
 9  8  7  6  x  x  x  x  5  -  4  -  +  -  -  +
D7 D6 D5 D4              E RW RS VO DD SS  K  A
*/

const int LCD_D4 = 6;
const int LCD_D5 = 7;
const int LCD_D6 = 8;
const int LCD_D7 = 9;
const int LCD_RS = 4;
const int LCD_E = 5; 




#endif // __PINS__H__
