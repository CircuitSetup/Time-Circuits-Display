/*
 * -------------------------------------------------------------------
 * CircuitSetup.us Time Circuits Display
 * Code adapted from Marmoset Electronics 
 * https://www.marmosetelectronics.com/time-circuits-clock
 * by John Monaco
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
#include <RTClib.h>
#include <WiFi.h>
#include <Wire.h>
#include <Preferences.h>

#include "clockdisplay.h"
#include "tc_keypad.h"
#include "tc_menus.h"
#include "tc_audio.h"
#include "tc_wifi.h"
#include "time.h"

#define SECONDS_IN 15   // SQW Monitor 1Hz from the DS3231
#define STATUS_LED 2  // Status LED

#define DEST_TIME_ADDR 0x71
#define DEST_TIME_EEPROM "dest_time_pref" 
#define PRES_TIME_ADDR 0x72
#define PRES_TIME_EEPROM "pres_time_pref" 
#define DEPT_TIME_ADDR 0x74
#define DEPT_TIME_EEPROM "dept_time_pref" 

#define AUTOINTERVAL_ADDR "autoint_pref" // autoInterval save location
extern uint8_t autoInterval;
extern const uint8_t autoTimeIntervals[5];

extern clockDisplay destinationTime;
extern clockDisplay presentTime;
extern clockDisplay departedTime;

extern RTC_DS3231 rtc;

extern dateStruct destinationTimes[8];
extern dateStruct departedTimes[8];
extern int8_t autoTime;

extern void time_setup();
extern void time_loop();
extern void timeTravel();
extern bool getNTPTime();
extern void doGetAutoTimes();
extern bool checkTimeOut();
extern void RTCClockOutEnable();
extern bool isLeapYear(int year);
extern int daysInMonth(int month, int year);

#endif