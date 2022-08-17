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

#ifndef _TC_SETTINGS_H
#define _TC_SETTINGS_H

#include <ArduinoJson.h>  // https://github.com/bblanchon/ArduinoJson
#include <SD.h>
#include <SPI.h>
#include <FS.h>
#include <SPIFFS.h>

#include <EEPROM.h>

#include "tc_global.h"

// SD Card pins
#define SD_CS     5
#define SPI_MOSI  23
#define SPI_MISO  19
#define SPI_SCK   18

extern void settings_setup();
extern void write_settings();

extern bool loadAlarm();
extern void saveAlarm();
bool loadAlarmEEPROM();
void saveAlarmEEPROM();

extern bool    alarmOnOff;
extern uint8_t alarmHour;
extern uint8_t alarmMinute;

extern bool    timetravelPersistent;

extern bool    haveSD;

// Default settings - change settings in the web interface 192.168.4.1

// For list of time zones, see https://github.com/nayarsystems/posix_tz_db/blob/master/zones.csv

struct Settings {
    char ntpServer[64]      = "pool.ntp.org";
    char timeZone[64]       = "CST6CDT,M3.2.0,M11.1.0";     
    char autoRotateTimes[4] = "0";    // Default: Off, use time circuits like in movie
    char destTimeBright[4]  = "15";
    char presTimeBright[4]  = "15";
    char lastTimeBright[4]  = "15";
    //char beepSound[3]     = "0";
    char wifiConRetries[4]  = "3";    // Default: 3 retries
    char wifiConTimeout[4]  = "7";    // Default: 7 seconds time-out
    char mode24[4]          = "0";    // Default: 0 = 12-hour-mode    
    char timesPers[4]       = "1";    // Default: TimeTravel persistent 
#ifdef FAKE_POWER_ON 
    char fakePwrOn[4]       = "0";       
#endif
    char alarmRTC[4]        = "1";    // Default: Alarm is RTC-based (otherwise presentTime-based = 0)
};

extern struct Settings settings;

#endif
