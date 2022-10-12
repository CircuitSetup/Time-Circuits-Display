/*
 * -------------------------------------------------------------------
 * CircuitSetup.us Time Circuits Display
 * (C) 2021-2022 John deGlavina https://circuitsetup.us
 * (C) 2022 Thomas Winischhofer (A10001986)
 *
 * Settings handling
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

#ifndef _TC_SETTINGS_H
#define _TC_SETTINGS_H

#include "tc_global.h"

#include <ArduinoJson.h>  // https://github.com/bblanchon/ArduinoJson
#include <SD.h>
#include <SPI.h>
#include <FS.h>
#ifdef USE_SPIFFS
#include <SPIFFS.h>
#else
#define SPIFFS LittleFS
#include <LittleFS.h>
#endif
#include <EEPROM.h>

extern void settings_setup();
extern void write_settings();
bool checkValidNumParm(char *text, int lowerLim, int upperLim, int setDefault);
bool checkValidNumParmF(char *text, double lowerLim, double upperLim, double setDefault);
extern bool loadAlarm();
extern void saveAlarm();
bool loadAlarmEEPROM();
void saveAlarmEEPROM();
extern bool loadIpSettings();
extern void writeIpSettings();
extern void deleteIpSettings();

extern bool copy_audio_files();
void open_and_copy(const char *fn, int& haveErr);
bool filecopy(File source, File dest);

extern bool check_allow_CPA();
bool check_if_default_audio_present();
extern void start_file_copy();
extern void file_copy_progress();
extern void file_copy_done();
extern void file_copy_error();

extern void formatFlashFS();

extern bool    alarmOnOff;
extern uint8_t alarmHour;
extern uint8_t alarmMinute;

extern bool    timetravelPersistent;

extern bool    haveSD;

#define MS(s) XMS(s)
#define XMS(s) #s

// Default settings - change settings in the web interface 192.168.4.1
// For list of time zones, see https://github.com/nayarsystems/posix_tz_db/blob/master/zones.csv

#define DEF_TIMES_PERS      1     // 0-1;  Default: 1 = TimeTravel persistent
#define DEF_ALARM_RTC       1     // 0-1;  Default: 1 = Alarm is RTC-based (otherwise 0 = presentTime-based)
#define DEF_PLAY_INTRO      1     // 0-1;  Default: 1 = Play intro
#define DEF_MODE24          0     // 0-1;  Default: 0=12-hour-mode, 1=24-hour-mode
#define DEF_AUTOROTTIMES    1     // 0-5;  Default: Auto-rotate every 5th minute
#define DEF_WIFI_RETRY      3     // 1-15; Default: 3 retries
#define DEF_WIFI_TIMEOUT    7     // 1-15; Default: 7 seconds time-out
#define DEF_WIFI_OFFDELAY   0     // 0/10-99: Default 0 = Never power down WiFi in STA-mode
#define DEF_WIFI_APOFFDELAY 0     // 0/10-99: Default 0 = Never power down WiFi in AP-mode
#define DEF_NTP_SERVER      "pool.ntp.org"
#define DEF_TIMEZONE        "CST6CDT,M3.2.0,M11.1.0"    // Posix format
#define DEF_BRIGHT_DEST     15    // 1-15; Default: max brightness
#define DEF_BRIGHT_PRES     15
#define DEF_BRIGHT_DEPA     15
#define DEF_AUTONM_ON       0     // Default: Both 0, Auto-Night-Mode disabled
#define DEF_AUTONM_OFF      0
#define DEF_DT_OFF          1     // Default: Dest. time off in night mode
#define DEF_PT_OFF          0     // Default: Present time dimmed in night mode
#define DEF_LT_OFF          1     // Default: Last dep. time off in night mode
#define DEF_FAKE_PWR        0     // 0-1;  Default: 0 = Do not use external fake "power" switch
#define DEF_ETT_DELAY       0     // in ms; Default 0: ETT immediately
#define DEF_ETT_LONG        0     // 0: Ext. TT short (reentry), 1: long
#define DEF_USE_SPEEDO      0     // 0: Don't use speedo part of time travel sequence
#define DEF_SPEEDO_TYPE     SP_MIN_TYPE  // Default display type
#define DEF_SPEEDO_FACT     2.0   // Speedo factor (1.0 actual DeLorean figures; >1.0 faster, <1.0 slower)
#define DEF_BRIGHT_SPEEDO   15    // Default: Max. brightness
#define DEF_USE_GPS         0     // 0: No i2c GPS module
#define DEF_USE_TEMP        0     // 0: No i2c thermometer
#define DEF_TEMP_BRIGHT     3     // Default temp brightness
#define DEF_TEMP_UNIT       0     // Default: temp unit Fahrenheit
#define DEF_USE_ETTO        0     // 0: No external props

struct Settings {
    char timesPers[4]       = MS(DEF_TIMES_PERS);
    char alarmRTC[4]        = MS(DEF_ALARM_RTC);
    char playIntro[4]       = MS(DEF_PLAY_INTRO);
    char mode24[4]          = MS(DEF_MODE24);
    char autoRotateTimes[4] = MS(DEF_AUTOROTTIMES);
    char wifiConRetries[4]  = MS(DEF_WIFI_RETRY);
    char wifiConTimeout[4]  = MS(DEF_WIFI_TIMEOUT);
    char wifiOffDelay[4]    = MS(DEF_WIFI_OFFDELAY);
    char wifiAPOffDelay[4]  = MS(DEF_WIFI_APOFFDELAY);
    char ntpServer[64]      = DEF_NTP_SERVER;
    char timeZone[64]       = DEF_TIMEZONE;
    char destTimeBright[4]  = MS(DEF_BRIGHT_DEST);
    char presTimeBright[4]  = MS(DEF_BRIGHT_PRES);
    char lastTimeBright[4]  = MS(DEF_BRIGHT_DEPA);
    char autoNMOn[4]        = MS(DEF_AUTONM_ON);
    char autoNMOff[4]       = MS(DEF_AUTONM_OFF);
    char dtNmOff[4]         = MS(DEF_DT_OFF);
    char ptNmOff[4]         = MS(DEF_PT_OFF);
    char ltNmOff[4]         = MS(DEF_LT_OFF);
#ifdef FAKE_POWER_ON
    char fakePwrOn[4]       = MS(DEF_FAKE_PWR);
#endif
#ifdef EXTERNAL_TIMETRAVEL_IN
    char ettDelay[8]        = MS(DEF_ETT_DELAY);
    char ettLong[4]         = MS(DEF_ETT_LONG);
#endif
#ifdef TC_HAVESPEEDO
    char useSpeedo[4]       = MS(DEF_USE_SPEEDO);
    char speedoType[4]      = MS(DEF_SPEEDO_TYPE);
    char speedoBright[4]    = MS(DEF_BRIGHT_SPEEDO);
    char speedoFact[6]      = MS(DEF_SPEEDO_FACT);
#ifdef TC_HAVEGPS
    char useGPS[4]          = MS(DEF_USE_GPS);
#endif
#ifdef TC_HAVETEMP
    char useTemp[4]         = MS(DEF_USE_TEMP);
    char tempBright[4]      = MS(DEF_TEMP_BRIGHT);
    char tempUnit[4]        = MS(DEF_TEMP_UNIT);
#endif
#endif
#ifdef EXTERNAL_TIMETRAVEL_OUT
    char useETTO[4]         = MS(DEF_USE_ETTO);
#endif
    char copyAudio[12]      = "";   // never loaded or saved!
};

// Maximum delay for incoming tt trigger
#define ETT_MAX_DEL 60000

struct IPSettings {
    char ip[20]       = "";
    char gateway[20]  = "";
    char netmask[20]  = "";
    char dns[20]      = "";
};

extern struct Settings settings;
extern struct IPSettings ipsettings;

#endif
