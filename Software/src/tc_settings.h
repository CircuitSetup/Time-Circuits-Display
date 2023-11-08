/*
 * -------------------------------------------------------------------
 * CircuitSetup.us Time Circuits Display
 * (C) 2021-2022 John deGlavina https://circuitsetup.us
 * (C) 2022-2023 Thomas Winischhofer (A10001986)
 * https://github.com/realA10001986/Time-Circuits-Display
 * https://tcd.backtothefutu.re
 *
 * Settings handling
 *
 * -------------------------------------------------------------------
 * License: MIT
 * 
 * Permission is hereby granted, free of charge, to any person 
 * obtaining a copy of this software and associated documentation 
 * files (the "Software"), to deal in the Software without restriction, 
 * including without limitation the rights to use, copy, modify, 
 * merge, publish, distribute, sublicense, and/or sell copies of the 
 * Software, and to permit persons to whom the Software is furnished to 
 * do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be 
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, 
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF 
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. 
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY 
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, 
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE 
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef _TC_SETTINGS_H
#define _TC_SETTINGS_H

extern bool haveFS;
extern bool haveSD;
extern bool FlashROMode;

extern uint8_t musFolderNum;

#define MS(s) XMS(s)
#define XMS(s) #s

// Default settings - change settings in the web interface 192.168.4.1
// For list of time zones, see https://github.com/nayarsystems/posix_tz_db/blob/master/zones.csv

#define DEF_TIMES_PERS      0     // 0-1;  Default: 0 = TimeTravel not persistent
#define DEF_ALARM_RTC       1     // 0-1;  Default: 1 = Alarm is RTC-based (otherwise 0 = presentTime-based)
#define DEF_PLAY_INTRO      1     // 0-1;  Default: 1 = Play intro
#define DEF_MODE24          0     // 0-1;  Default: 0 = 12-hour-mode
#define DEF_BEEP            0     // 0-1:  Default: 0 = annoying beep(tm) off
#define DEF_AUTOROTTIMES    1     // 0-5;  Default: Auto-rotate every 5th minute
#define DEF_HOSTNAME        "timecircuits"
#define DEF_WIFI_RETRY      3     // 1-10; Default: 3 retries
#define DEF_WIFI_TIMEOUT    7     // 7-25; Default: 7 seconds
#define DEF_WIFI_OFFDELAY   0     // 0/10-99; Default 0 = Never power down WiFi in STA-mode
#define DEF_WIFI_APOFFDELAY 0     // 0/10-99; Default 0 = Never power down WiFi in AP-mode
#define DEF_WIFI_PRETRY     1     // Default: Nightly, periodic WiFi reconnection attempts for time sync
#define DEF_NTP_SERVER      "pool.ntp.org"
#define DEF_TIMEZONE        "UTC0"// Default: UTC; Posix format
#define DEF_BRIGHT_DEST     10    // 1-15; Default: medium brightness
#define DEF_BRIGHT_PRES     10
#define DEF_BRIGHT_DEPA     10
#define DEF_AUTONM_PRESET   10    // Default: AutoNM disabled
#define DEF_AUTONM_ON       0     // Default: Both 0
#define DEF_AUTONM_OFF      0
#define DEF_DT_OFF          1     // Default: Dest. time off in night mode
#define DEF_PT_OFF          0     // Default: Present time dimmed in night mode
#define DEF_LT_OFF          1     // Default: Last dep. time off in night mode
#define DEF_FAKE_PWR        0     // 0-1;  Default: 0 = Do not use external fake "power" switch
#define DEF_ETT_DELAY       0     // in ms; Default 0: ETT immediately
#define DEF_ETT_LONG        1     // [removed] 0: Ext. TT short (reentry), 1: long
#define DEF_SPEEDO_TYPE     99    // Default display type: None
#define DEF_SPEEDO_FACT     2.0   // Speedo factor (1.0 actual DeLorean figures; >1.0 faster, <1.0 slower)
#define DEF_BRIGHT_SPEEDO   15    // Default: Max. brightness for speed
#define DEF_USE_GPS_SPEED   0     // 0: Do not use GPS speed on speedo display
#define DEF_DISP_TEMP       1     // 1: Display temperature (if available) on speedo
#define DEF_TEMP_BRIGHT     3     // Default temperature brightness
#define DEF_TEMP_OFF_NM     1     // Default: temperature off in night mode
#define DEF_TEMP_UNIT       0     // Default: temperature unit Fahrenheit
#define DEF_TEMP_OFFS       0.0   // Default: temperature offset 0.0
#define DEF_USE_LIGHT       0     // Default: No i2c light sensor
#define DEF_LUX_LIMIT       3     // Default Lux for night mode
#define DEF_USE_ETTO        0     // Default: 0: No external props
#define DEF_NO_ETTO_LEAD    0     // Default: 0: ETTO with ETTO_LEAD lead time; 1 without
#define DEF_QUICK_GPS       0     // Default: Slow GPS updates (unless speed is being displayed)
#define DEF_PLAY_TT_SND     1     // Default 1: Play time travel sounds (0: Do not; for use with external equipment)
#define DEF_SHUFFLE         0     // Music Player: Do not shuffle by default
#define DEF_CFG_ON_SD       1     // Default: Save secondary settings on SD
#define DEF_SD_FREQ         0     // SD/SPI frequency: Default 16MHz

struct Settings {
    char timesPers[4]       = MS(DEF_TIMES_PERS);
    char alarmRTC[4]        = MS(DEF_ALARM_RTC);
    char playIntro[4]       = MS(DEF_PLAY_INTRO);
    char mode24[4]          = MS(DEF_MODE24);
    char beep[4]            = MS(DEF_BEEP);
    char autoRotateTimes[4] = MS(DEF_AUTOROTTIMES);   
    char hostName[32]       = DEF_HOSTNAME;
    char systemID[8]        = "";
    char appw[10]           = "";
    char wifiConRetries[4]  = MS(DEF_WIFI_RETRY);
    char wifiConTimeout[4]  = MS(DEF_WIFI_TIMEOUT);
    char wifiOffDelay[4]    = MS(DEF_WIFI_OFFDELAY);
    char wifiAPOffDelay[4]  = MS(DEF_WIFI_APOFFDELAY);
    char wifiPRetry[4]      = MS(DEF_WIFI_PRETRY);
    char ntpServer[64]      = DEF_NTP_SERVER;
    char timeZone[64]       = DEF_TIMEZONE;
    char timeZoneDest[64]   = "";
    char timeZoneDep[64]    = "";
    char timeZoneNDest[16]  = "";
    char timeZoneNDep[16]   = "";
    char destTimeBright[4]  = MS(DEF_BRIGHT_DEST);
    char presTimeBright[4]  = MS(DEF_BRIGHT_PRES);
    char lastTimeBright[4]  = MS(DEF_BRIGHT_DEPA);
    char autoNMPreset[4]    = MS(DEF_AUTONM_PRESET);
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
#ifdef TC_HAVETEMP
    char tempUnit[4]        = MS(DEF_TEMP_UNIT);
    char tempOffs[6]        = MS(DEF_TEMP_OFFS);
#endif
#ifdef TC_HAVELIGHT
    char useLight[4]        = MS(DEF_USE_LIGHT);
    char luxLimit[8]        = MS(DEF_LUX_LIMIT);
#endif
#ifdef TC_HAVESPEEDO
    char speedoType[4]      = MS(DEF_SPEEDO_TYPE);
    char speedoBright[4]    = MS(DEF_BRIGHT_SPEEDO);
    char speedoFact[6]      = MS(DEF_SPEEDO_FACT);
#ifdef TC_HAVEGPS
    char useGPSSpeed[4]     = MS(DEF_USE_GPS_SPEED);
#endif
#ifdef TC_HAVETEMP
    char dispTemp[4]        = MS(DEF_DISP_TEMP);
    char tempBright[4]      = MS(DEF_TEMP_BRIGHT);
    char tempOffNM[4]       = MS(DEF_TEMP_OFF_NM);
#endif
#endif // HAVESPEEDO
#ifdef EXTERNAL_TIMETRAVEL_OUT
    char useETTO[4]         = MS(DEF_USE_ETTO);
    char noETTOLead[4]      = MS(DEF_NO_ETTO_LEAD);
#endif
#ifdef TC_HAVEGPS
    char quickGPS[4]        = MS(DEF_QUICK_GPS);
#endif
    char playTTsnds[4]      = MS(DEF_PLAY_TT_SND);
    char shuffle[4]         = MS(DEF_SHUFFLE);
    char CfgOnSD[4]         = MS(DEF_CFG_ON_SD);
    char sdFreq[4]          = MS(DEF_SD_FREQ);
#ifdef TC_HAVEMQTT  
    char useMQTT[4]         = "0";
    char mqttServer[80]     = "";  // ip or domain [:port]  
    char mqttUser[128]      = "";  // user[:pass] (UTF8)
    char mqttTopic[512]     = "";  // topic (UTF8)
    char pubMQTT[4]         = "0"; // publish to broker (timetravel)
#endif    
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

void settings_setup();

void unmount_fs();

void write_settings();
bool checkConfigExists();

bool loadAlarm();
void saveAlarm();

bool loadReminder();
void saveReminder();

void saveCarMode();

bool loadCurVolume();
void saveCurVolume();

bool loadMusFoldNum();
void saveMusFoldNum();

#ifdef HAVE_STALE_PRESENT
void loadStaleTime(void *target, bool& currentOn);
void saveStaleTime(void *source, bool currentOn);
#endif

void copySettings();

bool loadIpSettings();
void writeIpSettings();
void deleteIpSettings();

void doCopyAudioFiles();
bool copy_audio_files();

bool check_allow_CPA();
void delete_ID_file();

bool audio_files_present();

void waitForEnterRelease();

void formatFlashFS();

void rewriteSecondarySettings();

bool readFileFromSD(const char *fn, uint8_t *buf, int len);
bool writeFileToSD(const char *fn, uint8_t *buf, int len);
bool readFileFromFS(const char *fn, uint8_t *buf, int len);
bool writeFileToFS(const char *fn, uint8_t *buf, int len);


#endif
