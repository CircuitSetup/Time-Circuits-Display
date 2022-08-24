/*
 * -------------------------------------------------------------------
 * CircuitSetup.us Time Circuits Display
 * 
 * Code based on Marmoset Electronics 
 * https://www.marmosetelectronics.com/time-circuits-clock
 * by John Monaco
 *
 * Enhanced/modified/written in 2022 by Thomas Winischhofer (A10001986)
 * -------------------------------------------------------------------
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 * 
 */

#ifndef _TC_TIME_H
#define _TC_TIME_H

#include <Arduino.h>
#include <time.h>
#include <RTClib.h>
#include <WiFi.h>
#include <Wire.h>
#include <EEPROM.h>
#ifdef FAKE_POWER_ON
#include <OneButton.h>
#endif

#include "tc_global.h"
#include "clockdisplay.h"
#include "tc_keypad.h"
#include "tc_menus.h"
#include "tc_audio.h"
#include "tc_wifi.h"
#include "time.h"
#include "tc_settings.h"

#define DEST_TIME_ADDR 0x71 // i2C address of displays
#define PRES_TIME_ADDR 0x72
#define DEPT_TIME_ADDR 0x74

// The time between sound being started and the display coming on
// Must be sync'd to the sound file used! (startup.mp3/timetravel.mp3)
#ifdef TWSOUND
#define STARTUP_DELAY 900
#else
#define STARTUP_DELAY 1050
#endif
#define TIMETRAVEL_DELAY 1500

// Time between the phases of (long) time travel
// Sum of all must be 8000
#define TT_P1_DELAY_P1  1400                                                                    // Normal
#define TT_P1_DELAY_P2  (4200-TT_P1_DELAY_P1)                                                   // Light flicker
#define TT_P1_DELAY_P3  (5800-(TT_P1_DELAY_P2+TT_P1_DELAY_P1))                                  // Off
#define TT_P1_DELAY_P4  (6800-(TT_P1_DELAY_P3+TT_P1_DELAY_P2+TT_P1_DELAY_P1))                   // Random display I
#define TT_P1_DELAY_P5  (8000-(TT_P1_DELAY_P4+TT_P1_DELAY_P3+TT_P1_DELAY_P2+TT_P1_DELAY_P1))    // Random display II

extern uint8_t        autoInterval;
extern const uint8_t  autoTimeIntervals[6];

extern bool           alarmOnOff;
extern uint8_t        alarmHour;
extern uint8_t        alarmMinute;

extern clockDisplay destinationTime;
extern clockDisplay presentTime;
extern clockDisplay departedTime;

extern RTC_DS3231 rtc;

extern dateStruct destinationTimes[8];
extern dateStruct departedTimes[8];
extern int8_t     autoTime;

extern void time_boot();
extern void time_setup();
extern void time_loop();
extern void timeTravel(bool makeLong);
extern void resetPresentTime();
extern void pauseAuto();
extern bool checkIfAutoPaused();
extern bool getNTPTime();
extern bool checkTimeOut();
extern void RTCClockOutEnable();
extern bool isLeapYear(int year);
extern int  daysInMonth(int month, int year);
extern DateTime myrtcnow();

extern uint64_t dateToMins(int year, int month, int day, int hour, int minute);
extern void minsToDate(uint64_t total, int& year, int& month, int& day, int& hour, int& minute);

#ifdef FAKE_POWER_ON
void fpbKeyPressed(); 
void fpbKeyLongPressStop();  
#endif

void my2delay(unsigned long mydel);
void waitAudioDoneIntro();

// These block various events
extern bool FPBUnitIsOn;
extern bool startup;
extern bool timeTraveled;
extern int  timeTravelP1;

// Our generic timeout when waiting for buttons, in seconds. max 255.
#define maxTime 240            
extern uint8_t timeout;

extern uint64_t timeDifference;
extern bool     timeDiffUp;

#ifdef FAKE_POWER_ON
extern bool waitForFakePowerButton;
#endif

#endif
