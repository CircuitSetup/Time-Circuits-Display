/*
 * -------------------------------------------------------------------
 * CircuitSetup.us Time Circuits Display
 * (C) 2021-2022 John deGlavina https://circuitsetup.us
 * (C) 2022-2025 Thomas Winischhofer (A10001986)
 * https://github.com/realA10001986/Time-Circuits-Display
 * https://tcd.out-a-ti.me
 *
 * Settings & file handling
 *
 * -------------------------------------------------------------------
 * License: MIT NON-AI
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
 * In addition, the following restrictions apply:
 * 
 * 1. The Software and any modifications made to it may not be used 
 * for the purpose of training or improving machine learning algorithms, 
 * including but not limited to artificial intelligence, natural 
 * language processing, or data mining. This condition applies to any 
 * derivatives, modifications, or updates based on the Software code. 
 * Any usage of the Software in an AI-training dataset is considered a 
 * breach of this License.
 *
 * 2. The Software may not be included in any dataset used for 
 * training or improving machine learning algorithms, including but 
 * not limited to artificial intelligence, natural language processing, 
 * or data mining.
 *
 * 3. Any person or organization found to be in violation of these 
 * restrictions will be subject to legal action and may be held liable 
 * for any damages resulting from such use.
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

extern bool haveAudioFiles;

extern uint8_t musFolderNum;

#define MS(s) XMS(s)
#define XMS(s) #s

// Default settings
#define DEF_HOSTNAME        "timecircuits"
#define DEF_WIFI_RETRY      3     // 1-10; Default: 3 retries
#define DEF_WIFI_TIMEOUT    7     // 7-25; Default: 7 seconds
#define DEF_WIFI_PRETRY     1     // 1: Nightly, periodic WiFi reconnection attempts for time sync; 0: off
#define DEF_WIFI_OFFDELAY   0     // 0/10-99; Default 0 = Never power down WiFi in STA-mode
#define DEF_AP_CHANNEL      1     // 1-13; 0 = random(1-13)
#define DEF_WIFI_APOFFDELAY 0     // 0/10-99; Default 0 = Never power down WiFi in AP-mode

#define DEF_PLAY_INTRO      1     // 0: Skip intro; 1: Play intro
#define DEF_BEEP            0     // 0: annoying beep(tm) off, 1: on, 2: on (30 secs), 3: on (60 secs)
#define DEF_AUTOROTTIMES    1     // Time cycling interval. 0: off, 1: 5min, 2: 10min, 3: 15min, 4: 30min, 5: 60min
#define DEF_SKIP_TTANIM     0     // Default: 0: Display disruption during TT; 1: No disruption
#define DEF_P3ANIM          0     // Default: 0: P1/P2 anim, 1: P3 anim on destination date entry
#define DEF_PLAY_TT_SND     1     // Default: 1: Play time travel sounds (0: Do not; for use with external equipment)
#define DEF_ALARM_RTC       1     // 0: Alarm is presentTime-based; 1: Alarm is RTC-based
#define DEF_MODE24          0     // 0: 12 hour clock; 1: 24 hour clock
#define DEF_TIMEZONE        ""    // Default: UTC; Posix format
#define DEF_NTP_SERVER      "pool.ntp.org"
#define DEF_SHUFFLE         0     // Music Player: 0: Do not shuffle by default, 1: Do
#define DEF_BRIGHT_DEST     10    // 1-15; Default: medium brightness
#define DEF_BRIGHT_PRES     10
#define DEF_BRIGHT_DEPA     10
#define DEF_DT_OFF          1     // 1: Dest. time off in night mode; 0: dimmed
#define DEF_PT_OFF          0     // 1: Present time off in night mode; 0: dimmed
#define DEF_LT_OFF          1     // 1: Last dep. time off in night mode; 0: dimmed
#define DEF_AUTONM_PRESET   10    // Default: AutoNM disabled
#define DEF_AUTONM_ON       0     // Default: Both 0
#define DEF_AUTONM_OFF      0
#define DEF_USE_LIGHT       0     // Default: No i2c light sensor
#define DEF_LUX_LIMIT       3     // Default Lux threshold for night mode
#define DEF_TEMP_UNIT       0     // Default: 0: temperature unit Fahrenheit; 1: Celsius
#define DEF_TEMP_OFFS       0.0   // Default: temperature offset 0.0
#define DEF_SPEEDO_TYPE     99    // Default display type: None
#define DEF_BRIGHT_SPEEDO   15    // Default: Max. brightness for speed
#ifdef SP_ALWAYS_ON
#define DEF_SPEEDO_AO       0     // Speedo: 0: Keep on when idle, do not switch off, 1: switch off when idle (=old behavior)
#else
#define DEF_SPEEDO_AO       1     // Speedo: 0: Keep on when idle, do not switch off, 1: switch off when idle (=old behavior)
#endif
#define DEF_SPEEDO_ACCELFIG 0     // Accel figures: 0: Movie (approximated), 1: Real-life
#define DEF_SPEEDO_FACT     2.0   // Real-life acceleration factor (1.0 actual DeLorean figures; >1.0 faster, <1.0 slower)
#define DEF_SPEEDO_P3       0     // Speedo: 1: Like part 3 (leading 0 < 10, no dot) / 0: like parts 1/2 (single digit < 10, dot)
#define DEF_SPEEDO_3RDD     0     // Speedo: 1: Enable third digit (always 0) on CircuitSetup speedo, 0: Do not
#define DEF_USE_GPS_SPEED   0     // 0: Do not show GPS speed on speedo display; 1: Do
#define DEF_SPD_UPD_RATE    1     // 0: 1Hz, 1: 2Hz (default), 2: 4Hz, 3: 5Hz
#define DEF_DISP_TEMP       1     // 1: Display temperature (if available) on speedo; 0: do not (speedo off)
#define DEF_TEMP_BRIGHT     3     // Default temperature brightness
#define DEF_TEMP_OFF_NM     1     // Default: 1: temperature off in night mode; 0: dimmed
#define DEF_FAKE_PWR        0     // 0: Do not use external fake "power" switch, 1: Do
#define DEF_ETT_DELAY       0     // in ms; Default 0: ETT immediately
#define DEF_ETT_LONG        1     // [removed] 0: Ext. TT short (reentry), 1: long
#define DEF_QUICK_GPS       0     // Default: 0: Do not provide GPS speed for BTTF props; 1: Do
#define DEF_ETTO_CMD        0     // Default: 0: not controllable by 990/991 commands, 1: controllable by commands
#define DEF_USE_ETTO        0     // Default: 0: No signal on TT; 1: TT_OUT signals time travel
#define DEF_ETTO_ALM        0     // Default: 0: No signal on alarm; 1: TT_OUT signals alarm
#define DEF_ETTO_PUS        0     // Default: 0: TT_OUT by command: 0: Power up state is low; 1: high
#define DEF_NO_ETTO_LEAD    0     // Default: 0: ETTO with ETTO_LEAD lead time; 1: without
#define DEF_ETTO_ALM_D      5     // Default: TT_OUT signals alarm for 5 seconds
#define DEF_CFG_ON_SD       1     // Default: 1: Save secondary settings on SD, 0: Do not (use internal Flash)
#define DEF_TIMES_PERS      0     // Default: 0: Timetravels not persistent; 1: TT persistent
#define DEF_SD_FREQ         0     // SD/SPI frequency: Default 16MHz
#define DEF_REVAMPM         0     // Default: 0: AM/PM like in P1, 1: AM/PM like in P2/P3

#define DEF_MQTT_VTT        1     // 0: MQTT sends TIMETRAVEL, 5 sec lead. 1: sends TIMETRAVEL_xxxx_yyyy with variable lead

struct Settings {
    char ssid[34]           = "";
    char pass[66]           = "";

    char hostName[34]       = DEF_HOSTNAME;
    char wifiConRetries[4]  = MS(DEF_WIFI_RETRY);
    char wifiConTimeout[4]  = MS(DEF_WIFI_TIMEOUT);
    char wifiPRetry[4]      = MS(DEF_WIFI_PRETRY);
    char wifiOffDelay[4]    = MS(DEF_WIFI_OFFDELAY);
    char systemID[8]        = "";
    char appw[10]           = "";
    char apChnl[4]          = MS(DEF_AP_CHANNEL);
    char wifiAPOffDelay[4]  = MS(DEF_WIFI_APOFFDELAY);
    
    char playIntro[4]       = MS(DEF_PLAY_INTRO);
    char beep[4]            = MS(DEF_BEEP);
    char autoRotateTimes[4] = MS(DEF_AUTOROTTIMES);
    char autoRotAnim[4]     = "1";
    char skipTTAnim[4]      = MS(DEF_SKIP_TTANIM);
#ifndef IS_ACAR_DISPLAY    
    char p3anim[4]          = MS(DEF_P3ANIM);
#endif    
    char playTTsnds[4]      = MS(DEF_PLAY_TT_SND);
    char alarmRTC[4]        = MS(DEF_ALARM_RTC);
    char mode24[4]          = MS(DEF_MODE24);
    char timeZone[64]       = DEF_TIMEZONE;
    char ntpServer[64]      = DEF_NTP_SERVER;
    char timeZoneDest[64]   = "";
    char timeZoneDep[64]    = "";
    char timeZoneNDest[16]  = "";
    char timeZoneNDep[16]   = "";
    char shuffle[4]         = MS(DEF_SHUFFLE);
    char destTimeBright[4]  = MS(DEF_BRIGHT_DEST);
    char presTimeBright[4]  = MS(DEF_BRIGHT_PRES);
    char lastTimeBright[4]  = MS(DEF_BRIGHT_DEPA);
    char dtNmOff[4]         = MS(DEF_DT_OFF);
    char ptNmOff[4]         = MS(DEF_PT_OFF);
    char ltNmOff[4]         = MS(DEF_LT_OFF);
    char autoNMPreset[4]    = MS(DEF_AUTONM_PRESET);
    char autoNMOn[4]        = MS(DEF_AUTONM_ON);
    char autoNMOff[4]       = MS(DEF_AUTONM_OFF);
#ifdef TC_HAVELIGHT
    char useLight[4]        = MS(DEF_USE_LIGHT);
    char luxLimit[8]        = MS(DEF_LUX_LIMIT);
#endif    
#ifdef TC_HAVETEMP
    char tempUnit[4]        = MS(DEF_TEMP_UNIT);
    char tempOffs[6]        = MS(DEF_TEMP_OFFS);
#endif
    char speedoType[4]      = MS(DEF_SPEEDO_TYPE);
    char speedoBright[4]    = MS(DEF_BRIGHT_SPEEDO);
    char speedoAO[4]        = MS(DEF_SPEEDO_AO);
    char speedoAF[4]        = MS(DEF_SPEEDO_ACCELFIG);
    char speedoFact[6]      = MS(DEF_SPEEDO_FACT);
    char speedoP3[4]        = MS(DEF_SPEEDO_P3);
    char speedo3rdD[4]      = MS(DEF_SPEEDO_3RDD);
#ifdef TC_HAVEGPS
    char dispGPSSpeed[4]    = MS(DEF_USE_GPS_SPEED);
    char spdUpdRate[4]      = MS(DEF_SPD_UPD_RATE);
#endif
#ifdef TC_HAVETEMP
    char dispTemp[4]        = MS(DEF_DISP_TEMP);
    char tempBright[4]      = MS(DEF_TEMP_BRIGHT);
    char tempOffNM[4]       = MS(DEF_TEMP_OFF_NM);
#endif
    char fakePwrOn[4]       = MS(DEF_FAKE_PWR);
    char ettDelay[8]        = MS(DEF_ETT_DELAY);
    char ettLong[4]         = MS(DEF_ETT_LONG);
#ifdef TC_HAVEGPS
    char quickGPS[4]        = MS(DEF_QUICK_GPS);
#endif
#ifdef TC_HAVEMQTT  
    char useMQTT[4]         = "0";
    char mqttVers[4]        = "0"; // 0 = 3.1.1, 1 = 5.0
    char mqttServer[80]     = "";  // ip or domain [:port]  
    char mqttUser[64]       = "";  // user[:pass] (UTF8) [limited to 63 bytes through WM]
    char mqttTopic[128]     = "";  // topic (UTF8)       [limited to 127 bytes through WM]
    char pubMQTT[4]         = "0";              // publish to broker (timetravel)
    char MQTTvarLead[4]     = MS(DEF_MQTT_VTT); // publish TIMETRAVEL with lead and P1 duration appended (default to on)
    char mqttPwr[4]         = "0"; // Do not start with MQTT having control over fake-power
    char mqttPwrOn[4]       = "0"; // Do not wait for POWER_ON at startup
#endif // TC_HAVEMQTT
    char ETTOcmd[4]         = MS(DEF_ETTO_CMD);
    char useETTO[4]         = MS(DEF_USE_ETTO);
    char ETTOalm[4]         = MS(DEF_ETTO_ALM);
    char ETTOpus[4]         = MS(DEF_ETTO_PUS);
    char noETTOLead[4]      = MS(DEF_NO_ETTO_LEAD);
    char ETTOAD[6]          = MS(DEF_ETTO_ALM_D);
    char CfgOnSD[4]         = MS(DEF_CFG_ON_SD);
    char timesPers[4]       = MS(DEF_TIMES_PERS);
    //char sdFreq[4]          = MS(DEF_SD_FREQ);
    char revAmPm[4]         = MS(DEF_REVAMPM);    
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

void saveBrightness(bool useCache = true);

void saveBeepAutoInterval(bool useCache = true);

bool loadAlarm();
void saveAlarm();

bool loadReminder();
void saveReminder();

void saveCarMode();

bool loadCurVolume();
void saveCurVolume(bool useCache = true);

bool loadMusFoldNum();
void saveMusFoldNum();

void loadStaleTime(void *target, bool& currentOn);
void saveStaleTime(void *source, bool currentOn);

void loadLineOut();
void saveLineOut();

#ifdef TC_HAVE_REMOTE
void saveRemoteAllowed();
#endif

bool loadIpSettings();
void writeIpSettings();
void deleteIpSettings();

void copySettings();

bool check_if_default_audio_present();
void doCopyAudioFiles();
bool copy_audio_files(bool& delIDfile);

bool check_allow_CPA();
void delete_ID_file();

void waitForEnterRelease();

bool formatFlashFS(bool userSignal);

void rewriteSecondarySettings();

bool readFileFromSD(const char *fn, uint8_t *buf, int len);
bool writeFileToSD(const char *fn, uint8_t *buf, int len);
bool readFileFromFS(const char *fn, uint8_t *buf, int len);
bool writeFileToFS(const char *fn, uint8_t *buf, int len);

#define MAX_SIM_UPLOADS 16
#define UPL_OPENERR 1
#define UPL_NOSDERR 2
#define UPL_WRERR   3
#define UPL_BADERR  4
#define UPL_MEMERR  5
#define UPL_UNKNOWN 6
#define UPL_DPLBIN  7
#include <FS.h>
bool   openUploadFile(String& fn, File& file, int idx, bool haveAC, int& opType, int& errNo);
size_t writeACFile(File& file, uint8_t *buf, size_t len);
void   closeACFile(File& file);
void   removeACFile(int idx);
void   renameUploadFile(int idx);
char   *getUploadFileName(int idx);
int    getUploadFileNameLen(int idx);
void   freeUploadFileNames();

#endif
