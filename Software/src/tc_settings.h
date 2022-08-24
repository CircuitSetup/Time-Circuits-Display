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

#ifndef _TC_SETTINGS_H
#define _TC_SETTINGS_H

#include <ArduinoJson.h>  // https://github.com/bblanchon/ArduinoJson
#include <SD.h>
#include <SPI.h>
#include <FS.h>
#include <SPIFFS.h>

#include <EEPROM.h>

#include "tc_global.h"

extern void settings_setup();
extern void write_settings();
bool checkValidNumParm(char *text, int lowerLim, int upperLim, int setDefault);
extern bool loadAlarm();
extern void saveAlarm();
bool loadAlarmEEPROM();
void saveAlarmEEPROM();
extern bool loadIpSettings();
extern void writeIpSettings();
extern void deleteIpSettings();

extern bool    alarmOnOff;
extern uint8_t alarmHour;
extern uint8_t alarmMinute;

extern bool    timetravelPersistent;

extern bool    haveSD;

#define MS(s) XMS(s)
#define XMS(s) #s

// Default settings - change settings in the web interface 192.168.4.1
// For list of time zones, see https://github.com/nayarsystems/posix_tz_db/blob/master/zones.csv

#define DEF_NTP_SERVER      "pool.ntp.org"
#define DEF_TIMEZONE        "CST6CDT,M3.2.0,M11.1.0"    // Posix format
#define DEF_AUTOROTTIMES    0     // 0-5;  Default: Auto-rotate off, use device like in movie
#define DEF_BRIGHT_DEST     15    // 1-15; Default: max brightness
#define DEF_BRIGHT_PRES     15
#define DEF_BRIGHT_DEPA     15
#define DEF_WIFI_RETRY      3     // 1-15; Default: 3 retries
#define DEF_WIFI_TIMEOUT    7     // 1-15; Default: 7 seconds time-out
#define DEF_MODE24          0     // 0-1;  Default: 0=12-hour-mode, 1=24-hour-mode
#define DEF_TIMES_PERS      1     // 0-1;  Default: 1 = TimeTravel persistent
#define DEF_FAKE_PWR        0     // 0-1;  Default: 0 = Do not use external fake "power" switch
#define DEF_ALARM_RTC       1     // 0-1;  Default: 1 = Alarm is RTC-based (otherwise 0 = presentTime-based)
#define DEF_PLAY_INTRO      1     // 0-1;  Default: 1 = Play intro

struct Settings {
    char ntpServer[64]      = DEF_NTP_SERVER;
    char timeZone[64]       = DEF_TIMEZONE;     
    char autoRotateTimes[4] = MS(DEF_AUTOROTTIMES);
    char destTimeBright[4]  = MS(DEF_BRIGHT_DEST);
    char presTimeBright[4]  = MS(DEF_BRIGHT_PRES);
    char lastTimeBright[4]  = MS(DEF_BRIGHT_DEPA);
    char wifiConRetries[4]  = MS(DEF_WIFI_RETRY);
    char wifiConTimeout[4]  = MS(DEF_WIFI_TIMEOUT);
    char mode24[4]          = MS(DEF_MODE24);
    char timesPers[4]       = MS(DEF_TIMES_PERS);
#ifdef FAKE_POWER_ON 
    char fakePwrOn[4]       = MS(DEF_FAKE_PWR);
#endif
    char alarmRTC[4]        = MS(DEF_ALARM_RTC);
    char playIntro[4]       = MS(DEF_PLAY_INTRO);
};

struct IPSettings {
    char ip[20]       = "";
    char gateway[20]  = "";     
    char netmask[20]  = "";
    char dns[20]      = "";
};

extern struct Settings settings;
extern struct IPSettings ipsettings;

#endif
