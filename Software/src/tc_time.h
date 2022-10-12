/*
 * -------------------------------------------------------------------
 * CircuitSetup.us Time Circuits Display
 * (C) 2021-2022 John deGlavina https://circuitsetup.us
 * (C) 2022 Thomas Winischhofer (A10001986)
 *
 * Time and Main Controller
 *
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
 */

#ifndef _TC_TIME_H
#define _TC_TIME_H

#include <Arduino.h>
#include <time.h>
#include <WiFi.h>
#include <Wire.h>
#ifdef FAKE_POWER_ON
#include <OneButton.h>
#endif

#include "tc_global.h"
#include "rtc.h"
#include "clockdisplay.h"
#ifdef TC_HAVESPEEDO
#include "speeddisplay.h"
#ifdef TC_HAVEGPS
#include "gps.h"
#endif
#ifdef TC_HAVETEMP
#include "tempsensor.h"
#endif
#endif
#include "tc_keypad.h"
#include "tc_menus.h"
#include "tc_audio.h"
#include "tc_wifi.h"
#include "time.h"
#include "tc_settings.h"

#define DEST_TIME_ADDR 0x71 // i2C address of TC displays
#define PRES_TIME_ADDR 0x72
#define DEPT_TIME_ADDR 0x74

#define SPEEDO_ADDR    0x70 // i2C address of speedo display
#define GPS_ADDR       0x10 // i2C address of GPS receiver
#define TEMP_ADDR      0x18 // i2C address of temperature sensor

#define DS3231_ADDR    0x68 // i2C address of DS3231 RTC
#define PCF2129_ADDR   0x51 // i2C address of PCF2129 RTC

// The time between reentry sound being started and the display coming on
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
// ACTUAL POINT OF TIME TRAVEL:
#define TT_P1_POINT88   1000    // ms into "starttravel" sample, when 88mph is reached.
#define TT_SNDLAT        400    // DO NOT CHANGE

// Preprocessor config for External Time Travel Output (ETTO):
// Lead time from trigger (LOW->HIGH) to actual tt (ie when 88mph is reached)
// The external prop has ETTO_LEAD_TIME ms to play its pre-tt sequence. After
// ETTO_LEAD_TIME ms, 88mph is reached, and the actual tt takes place.
#define ETTO_LEAD_TIME      5000
// Trigger mode:
// true:  Pulse for ETTO_PULSE_DURATION ms on tt start minus leadTime
// false: LOW->HIGH on tt start minus leadTime, HIGH->LOW on start of reentry
#define ETTO_USE_PULSE      false
// If ETTO_USE_PULSE is true, pulse for approx. ETTO_PULSE_DURATION ms
#define ETTO_PULSE_DURATION 1000
// End of ETTO config

extern uint8_t        autoInterval;
extern const uint8_t  autoTimeIntervals[6];

extern bool           alarmOnOff;
extern uint8_t        alarmHour;
extern uint8_t        alarmMinute;

extern uint16_t       lastYear;

extern clockDisplay destinationTime;
extern clockDisplay presentTime;
extern clockDisplay departedTime;
#ifdef TC_HAVESPEEDO
extern bool useSpeedo;
extern speedDisplay speedo;
#ifdef TC_HAVETEMP
extern bool useTemp;
#endif
#endif

extern tcRTC rtc;

#define NUM_AUTOTIMES 11
extern dateStruct destinationTimes[NUM_AUTOTIMES];
extern dateStruct departedTimes[NUM_AUTOTIMES];
extern int8_t     autoTime;

extern void time_boot();
extern void time_setup();
extern void time_loop();
extern void timeTravel(bool doComplete, bool withSpeedo = false);
extern void resetPresentTime();
extern void pauseAuto();
extern bool checkIfAutoPaused();
bool getNTPOrGPSTime();
bool getNTPTime();
extern bool checkTimeOut();
extern bool isLeapYear(int year);
extern int  daysInMonth(int month, int year);
extern DateTime myrtcnow();
extern uint64_t dateToMins(int year, int month, int day, int hour, int minute);
extern void minsToDate(uint64_t total, int& year, int& month, int& day, int& hour, int& minute);
extern uint32_t getHrs1KYrs(int index);
uint8_t dayOfWeek(int d, int m, int y);
extern void correctYr4RTC(uint16_t& year, int16_t& offs);

#ifdef FAKE_POWER_ON
void fpbKeyPressed();
void fpbKeyLongPressStop();
#endif

void my2delay(unsigned int mydel);
void waitAudioDoneIntro();

extern void pwrNeedFullNow(bool force = false);

#ifdef TC_HAVESPEEDO
#ifdef TC_HAVEGPS
bool getGPStime();
bool setGPStime();
void dispGPSSpeed(bool force = false);
#endif
#ifdef TC_HAVETEMP
void dispTemperature(bool force = false);
#endif
#endif

// These block various events
extern bool FPBUnitIsOn;
extern bool startup;
extern bool timeTraveled;
extern int  timeTravelP0;
extern int  timeTravelP1;
extern int  timeTravelP2;
extern int  specDisp;

// Our generic timeout when waiting for buttons, in seconds. max 255.
#define maxTime 240
extern uint8_t timeout;

extern uint64_t timeDifference;
extern bool     timeDiffUp;

#ifdef FAKE_POWER_ON
extern bool waitForFakePowerButton;
#endif

#endif
