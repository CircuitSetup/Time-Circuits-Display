/*
 * -------------------------------------------------------------------
 * CircuitSetup.us Time Circuits Display
 * Code adapted from Marmoset Electronics 
 * https://www.marmosetelectronics.com/time-circuits-clock
 * by John Monaco
 * Enhanced/modified in 2022 by Thomas Winischhofer (A10001986)
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
#include <RTClib.h>
#include <WiFi.h>
#include <Wire.h>
#include <EEPROM.h>

#include "tc_global.h"
#include "clockdisplay.h"
#include "tc_keypad.h"
#include "tc_menus.h"
#include "tc_audio.h"
#include "tc_wifi.h"
#include "time.h"
#include "tc_settings.h"

#define SECONDS_IN 15       // SQW Monitor 1Hz from the DS3231

#define STATUS_LED 2        // Status LED (on ESP)

#define DEST_TIME_ADDR 0x71 // i2C address of displays
#define PRES_TIME_ADDR 0x72
#define DEPT_TIME_ADDR 0x74

// EEPROM map
// We use 1(padded to 8) + 10*3 bytes of EEPROM space at 0x0. Let's hope no one is using this already.
// Defined in clockdisplay.h
//#define DEST_TIME_PREF    ... 
//#define PRES_TIME_PREF    ...
//#define DEPT_TIME_PREF    ...
#define   AUTOINTERVAL_PREF 0x00  // autoInterval save location (1 byte)

extern uint8_t        autoInterval;
extern const uint8_t  autoTimeIntervals[5];

extern clockDisplay destinationTime;
extern clockDisplay presentTime;
extern clockDisplay departedTime;

extern RTC_DS3231 rtc;

extern dateStruct destinationTimes[8];
extern dateStruct departedTimes[8];
extern int8_t     autoTime;

extern void time_setup();
extern void time_loop();
extern void timeTravel();
extern void resetPresentTime();
extern bool getNTPTime();
extern bool checkTimeOut();
extern void RTCClockOutEnable();
extern bool isLeapYear(int year);
extern int  daysInMonth(int month, int year);

// Our generic timeout when waiting for buttons, in seconds. max 255.
#define maxTime 240            
extern uint8_t timeout;

extern bool presentTimeBogus;

#endif