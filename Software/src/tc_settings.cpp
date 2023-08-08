/*
 * -------------------------------------------------------------------
 * CircuitSetup.us Time Circuits Display
 * (C) 2021-2022 John deGlavina https://circuitsetup.us
 * (C) 2022-2023 Thomas Winischhofer (A10001986)
 * https://github.com/realA10001986/Time-Circuits-Display
 *
 * Settings & file handling
 * 
 * Main and IP settings are stored on flash FS only
 * Alarm, Reminder and volume either on SD or on flash FS
 * Music Folder number always on SD
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

#include "tc_settings.h"
#include "tc_menus.h"
#include "tc_audio.h"
#include "tc_time.h"

// Size of main config JSON
// Needs to be adapted when config grows
#define JSON_SIZE 2048

/* If SPIFFS/LittleFS is mounted */
static bool haveFS = false;

/* If a SD card is found */
bool haveSD = false;

/* If SD contains default audio files */
static bool allowCPA = false;

/* Music Folder Number */
uint8_t musFolderNum = 0;

/* Save alarm/volume on SD? */
static bool configOnSD = false;

/* Paranoia: No writes Flash-FS  */
bool FlashROMode = false;

#define NUM_AUDIOFILES 19
#define SND_ENTER_IDX  8
#ifndef TWSOUND
#define SND_ENTER_LEN   13374
#define SND_STARTUP_LEN 21907
#else
#define SND_ENTER_LEN   12149
#define SND_STARTUP_LEN 18419
#endif
static const char *audioFiles[NUM_AUDIOFILES] = {
      "/alarm.mp3",
      "/alarmoff.mp3",
      "/alarmon.mp3",
      "/baddate.mp3",
      "/ee1.mp3",
      "/ee2.mp3",
      "/ee3.mp3",
      "/ee4.mp3",
      "/enter.mp3",
      "/intro.mp3",
      "/nmoff.mp3",
      "/nmon.mp3",
      "/ping.mp3",
      "/reminder.mp3",
      "/shutdown.mp3",
      "/startup.mp3",
      "/timer.mp3",
      "/timetravel.mp3",
      "/travelstart.mp3"
};
static const char *IDFN = "/TCD_def_snd.txt";

static const char *cfgName    = "/config.json";     // Main config (flash)
static const char *almCfgName = "/tcdalmcfg.json";  // Alarm config (flash/SD)
static const char *remCfgName = "/tcdremcfg.json";  // Reminder config (flash/SD)
static const char *volCfgName = "/tcdvolcfg.json";  // Volume config (flash/SD)
static const char *musCfgName = "/tcdmcfg.json";    // Music config (SD)
static const char *ipCfgName  = "/ipconfig.json";   // IP config (flash)

static const char *fsNoAvail = "File System not available";
static const char *badConfig = "Settings bad/missing/incomplete; writing new file";
static const char *failFileWrite = "Failed to open file for writing";

static bool read_settings(File configFile);

static void deleteReminder();

static bool CopyCheckValidNumParm(const char *json, char *text, uint8_t psize, int lowerLim, int upperLim, int setDefault);
static bool CopyCheckValidNumParmF(const char *json, char *text, uint8_t psize, float lowerLim, float upperLim, float setDefault);
static bool checkValidNumParm(char *text, int lowerLim, int upperLim, int setDefault);
static bool checkValidNumParmF(char *text, float lowerLim, float upperLim, float setDefault);

static void open_and_copy(const char *fn, int& haveErr);
static bool filecopy(File source, File dest);
static bool check_if_default_audio_present();

static bool CopyIPParm(const char *json, char *text, uint8_t psize);

extern void start_file_copy();
extern void file_copy_progress();
extern void file_copy_done();
extern void file_copy_error();

/*
 * settings_setup()
 * 
 * Mount SPIFFS/LittleFS and SD (if available).
 * Read configuration from JSON config file
 * If config file not found, create one with default settings
 *
 * If the device is powered on or reset while ENTER is held down, 
 * the IP settings file will be deleted and the device will use DHCP.
 */
void settings_setup()
{
    const char *funcName = "settings_setup";
    bool writedefault = false;

    // Pre-maturely use ENTER button (initialized again in keypad_setup())
    pinMode(ENTER_BUTTON_PIN, INPUT_PULLUP);
    delay(20);

    #ifdef TC_DBG
    Serial.printf("%s: Mounting flash FS... ", funcName);
    #endif

    if(SPIFFS.begin()) {

        haveFS = true;

    } else {

        #ifdef TC_DBG
        Serial.print(F("failed, formatting... "));
        #endif

        destinationTime.showTextDirect("WAIT");

        SPIFFS.format();
        if(SPIFFS.begin()) haveFS = true;

        destinationTime.showTextDirect("");

    }

    if(haveFS) {
      
        #ifdef TC_DBG
        Serial.println(F("ok, loading settings"));
        #endif
        
        if(SPIFFS.exists(cfgName)) {
            File configFile = SPIFFS.open(cfgName, "r");
            if(configFile) {
                writedefault = read_settings(configFile);
                configFile.close();
            } else {
                writedefault = true;
            }
        } else {
            writedefault = true;
        }

        // Write new config file after mounting SD and determining FlashROMode

    } else {

        Serial.println(F("failed.\nThis is very likely a hardware problem."));
        Serial.println(F("*** All settings/states will be stored on the SD card (if available)"));

    }
    
    // Set up SD card
    SPI.begin(SPI_SCK_PIN, SPI_MISO_PIN, SPI_MOSI_PIN);

    haveSD = false;

    uint32_t sdfreq = (settings.sdFreq[0] == '0') ? 16000000 : 4000000;
    #ifdef TC_DBG
    Serial.printf("%s: SD/SPI frequency %dMHz\n", funcName, sdfreq / 1000000);
    #endif

    #ifdef TC_DBG
    Serial.printf("%s: Mounting SD... ", funcName);
    #endif

    if(!SD.begin(SD_CS_PIN, SPI, sdfreq)) {

        Serial.println(F("no SD card found"));

    } else {

        #ifdef TC_DBG
        Serial.println(F("ok"));
        #endif

        uint8_t cardType = SD.cardType();
       
        #ifdef TC_DBG
        const char *sdTypes[5] = { "No card", "MMC", "SD", "SDHC", "unknown (SD not usable)" };
        Serial.printf("SD card type: %s\n", sdTypes[cardType > 4 ? 4 : cardType]);
        #endif

        haveSD = ((cardType != CARD_NONE) && (cardType != CARD_UNKNOWN));

    }

    if(haveSD) {
        if(SD.exists("/TCD_FLASH_RO") || !haveFS) {
            bool writedefault2 = false;
            FlashROMode = true;
            Serial.println(F("Flash-RO mode: All settings/states stored on SD. Reloading settings."));
            if(SD.exists(cfgName)) {
                File configFile = SD.open(cfgName, "r");
                if(configFile) {
                    writedefault2 = read_settings(configFile);
                    configFile.close();
                } else {
                    writedefault2 = true;
                }
            } else {
                writedefault2 = true;
            }
            if(writedefault2) {
                Serial.printf("%s: %s\n", funcName, badConfig);
                write_settings();
            }
        }
    }

    // Now write new config to flash FS if old one somehow bad
    // Only write this file if FlashROMode is off
    if(haveFS && writedefault && !FlashROMode) {
        Serial.printf("%s: %s\n", funcName, badConfig);
        write_settings();
    }

    // Determine if alarm/reminder/volume settings are to be stored on SD
    configOnSD = (haveSD && ((settings.CfgOnSD[0] != '0') || FlashROMode));

    // Check if SD contains our default sound files
    // We don't do that if in FlashROMode mode, of course
    if(haveFS && haveSD && !FlashROMode) {
        allowCPA = check_if_default_audio_present();
    }

    // Allow user to delete static IP data by holding ENTER
    // while booting
    // (10 secs timeout to wait for button-release to allow
    // to run fw without control board attached)
    if(digitalRead(ENTER_BUTTON_PIN)) {

        unsigned long mnow = millis();

        Serial.printf("%s: Deleting ip config\n", funcName);

        deleteIpSettings();

        // Pre-maturely use white led (initialized again in keypad_setup())
        pinMode(WHITE_LED_PIN, OUTPUT);
        digitalWrite(WHITE_LED_PIN, HIGH);
        while(digitalRead(ENTER_BUTTON_PIN)) {
            if(millis() - mnow > 10*1000) break;
        }
        digitalWrite(WHITE_LED_PIN, LOW);
    }
}

static bool read_settings(File configFile)
{
    const char *funcName = "read_settings";
    bool wd = false;
    size_t jsonSize = 0;
    //StaticJsonDocument<JSON_SIZE> json;
    DynamicJsonDocument json(JSON_SIZE);
    
    DeserializationError error = deserializeJson(json, configFile);

    jsonSize = json.memoryUsage();
    if(jsonSize > JSON_SIZE) {
        Serial.printf("%s: ERROR: Config too large (%d vs %d), memory corrupted.\n", funcName, jsonSize, JSON_SIZE);
    }
    
    #ifdef TC_DBG
    if(jsonSize > JSON_SIZE - 256) {
          Serial.printf("%s: WARNING: JSON_SIZE needs to be adapted **************\n", funcName);
    }
    Serial.printf("%s: Size of document: %d (JSON_SIZE %d)\n", funcName, jsonSize, JSON_SIZE);
    serializeJson(json, Serial);
    Serial.println(F(" "));
    #endif

    if(!error) {

        wd |= CopyCheckValidNumParm(json["timeTrPers"], settings.timesPers, sizeof(settings.timesPers), 0, 1, DEF_TIMES_PERS);
        wd |= CopyCheckValidNumParm(json["alarmRTC"], settings.alarmRTC, sizeof(settings.alarmRTC), 0, 1, DEF_ALARM_RTC);
        wd |= CopyCheckValidNumParm(json["playIntro"], settings.playIntro, sizeof(settings.playIntro), 0, 1, DEF_PLAY_INTRO);
        wd |= CopyCheckValidNumParm(json["mode24"], settings.mode24, sizeof(settings.mode24), 0, 1, DEF_MODE24);
        wd |= CopyCheckValidNumParm(json["beep"], settings.beep, sizeof(settings.beep), 0, 3, DEF_BEEP);
        wd |= CopyCheckValidNumParm(json["autoRotateTimes"], settings.autoRotateTimes, sizeof(settings.autoRotateTimes), 0, 5, DEF_AUTOROTTIMES);

        if(json["hostName"]) {
            memset(settings.hostName, 0, sizeof(settings.hostName));
            strncpy(settings.hostName, json["hostName"], sizeof(settings.hostName) - 1);
        } else wd = true;
        wd |= CopyCheckValidNumParm(json["wifiConRetries"], settings.wifiConRetries, sizeof(settings.wifiConRetries), 1, 15, DEF_WIFI_RETRY);
        wd |= CopyCheckValidNumParm(json["wifiConTimeout"], settings.wifiConTimeout, sizeof(settings.wifiConTimeout), 7, 25, DEF_WIFI_TIMEOUT);
        wd |= CopyCheckValidNumParm(json["wifiOffDelay"], settings.wifiOffDelay, sizeof(settings.wifiOffDelay), 0, 99, DEF_WIFI_OFFDELAY);
        wd |= CopyCheckValidNumParm(json["wifiAPOffDelay"], settings.wifiAPOffDelay, sizeof(settings.wifiAPOffDelay), 0, 99, DEF_WIFI_APOFFDELAY);
        wd |= CopyCheckValidNumParm(json["wifiPRetry"], settings.wifiPRetry, sizeof(settings.wifiPRetry), 0, 1, DEF_WIFI_PRETRY);
        
        if(json["timeZone"]) {
            memset(settings.timeZone, 0, sizeof(settings.timeZone));
            strncpy(settings.timeZone, json["timeZone"], sizeof(settings.timeZone) - 1);
        } else wd = true;
        if(json["ntpServer"]) {
            memset(settings.ntpServer, 0, sizeof(settings.ntpServer));
            strncpy(settings.ntpServer, json["ntpServer"], sizeof(settings.ntpServer) - 1);
        } else wd = true;

        if(json["timeZoneDest"]) {
            memset(settings.timeZoneDest, 0, sizeof(settings.timeZoneDest));
            strncpy(settings.timeZoneDest, json["timeZoneDest"], sizeof(settings.timeZoneDest) - 1);
        } else wd = true;
        if(json["timeZoneDep"]) {
            memset(settings.timeZoneDep, 0, sizeof(settings.timeZoneDep));
            strncpy(settings.timeZoneDep, json["timeZoneDep"], sizeof(settings.timeZoneDep) - 1);
        } else wd = true;
        if(json["timeZoneNDest"]) {
            memset(settings.timeZoneNDest, 0, sizeof(settings.timeZoneNDest));
            strncpy(settings.timeZoneNDest, json["timeZoneNDest"], sizeof(settings.timeZoneNDest) - 1);
        } else wd = true;
        if(json["timeZoneNDep"]) {
            memset(settings.timeZoneNDep, 0, sizeof(settings.timeZoneNDep));
            strncpy(settings.timeZoneNDep, json["timeZoneNDep"], sizeof(settings.timeZoneNDep) - 1);
        } else wd = true;

        wd |= CopyCheckValidNumParm(json["destTimeBright"], settings.destTimeBright, sizeof(settings.destTimeBright), 0, 15, DEF_BRIGHT_DEST);
        wd |= CopyCheckValidNumParm(json["presTimeBright"], settings.presTimeBright, sizeof(settings.presTimeBright), 0, 15, DEF_BRIGHT_PRES);
        wd |= CopyCheckValidNumParm(json["lastTimeBright"], settings.lastTimeBright, sizeof(settings.lastTimeBright), 0, 15, DEF_BRIGHT_DEPA);

        wd |= CopyCheckValidNumParm(json["dtNmOff"], settings.dtNmOff, sizeof(settings.dtNmOff), 0, 1, DEF_DT_OFF);
        wd |= CopyCheckValidNumParm(json["ptNmOff"], settings.ptNmOff, sizeof(settings.ptNmOff), 0, 1, DEF_PT_OFF);
        wd |= CopyCheckValidNumParm(json["ltNmOff"], settings.ltNmOff, sizeof(settings.ltNmOff), 0, 1, DEF_LT_OFF);

        wd |= CopyCheckValidNumParm(json["autoNMPreset"], settings.autoNMPreset, sizeof(settings.autoNMPreset), 0, 10, DEF_AUTONM_PRESET);
        if(json["autoNM"]) {    // transition old boolean "autoNM" setting
            char temp[4] = { '0', 0, 0, 0 };
            CopyCheckValidNumParm(json["autoNM"], temp, sizeof(temp), 0, 1, 0);
            if(temp[0] == '0') strcpy(settings.autoNMPreset, "10");
        }
        
        wd |= CopyCheckValidNumParm(json["autoNMOn"], settings.autoNMOn, sizeof(settings.autoNMOn), 0, 23, DEF_AUTONM_ON);
        wd |= CopyCheckValidNumParm(json["autoNMOff"], settings.autoNMOff, sizeof(settings.autoNMOff), 0, 23, DEF_AUTONM_OFF);
        #ifdef TC_HAVELIGHT
        wd |= CopyCheckValidNumParm(json["useLight"], settings.useLight, sizeof(settings.useLight), 0, 1, DEF_USE_LIGHT);
        wd |= CopyCheckValidNumParm(json["luxLimit"], settings.luxLimit, sizeof(settings.luxLimit), 0, 50000, DEF_LUX_LIMIT);
        #endif

        #ifdef TC_HAVETEMP
        wd |= CopyCheckValidNumParm(json["tempUnit"], settings.tempUnit, sizeof(settings.tempUnit), 0, 1, DEF_TEMP_UNIT);
        wd |= CopyCheckValidNumParmF(json["tempOffs"], settings.tempOffs, sizeof(settings.tempOffs), -3.0, 3.0, DEF_TEMP_OFFS);
        #endif

        #ifdef TC_HAVESPEEDO
        wd |= CopyCheckValidNumParm(json["speedoType"], settings.speedoType, sizeof(settings.speedoType), SP_MIN_TYPE, 99, DEF_SPEEDO_TYPE);
        if(json["useSpeedo"]) {    // transition old boolean "useSpeedo" setting
            char temp[4] = { '0', 0, 0, 0 };
            CopyCheckValidNumParm(json["useSpeedo"], temp, sizeof(temp), 0, 1, 0);
            if(temp[0] == '0') strcpy(settings.speedoType, "99");
        }
        
        wd |= CopyCheckValidNumParm(json["speedoBright"], settings.speedoBright, sizeof(settings.speedoBright), 0, 15, DEF_BRIGHT_SPEEDO);
        wd |= CopyCheckValidNumParmF(json["speedoFact"], settings.speedoFact, sizeof(settings.speedoFact), 0.5, 5.0, DEF_SPEEDO_FACT);
        #ifdef TC_HAVEGPS
        wd |= CopyCheckValidNumParm(json["useGPSSpeed"], settings.useGPSSpeed, sizeof(settings.useGPSSpeed), 0, 1, DEF_USE_GPS_SPEED);
        #endif
        #ifdef TC_HAVETEMP
        wd |= CopyCheckValidNumParm(json["dispTemp"], settings.dispTemp, sizeof(settings.dispTemp), 0, 1, DEF_DISP_TEMP);
        wd |= CopyCheckValidNumParm(json["tempBright"], settings.tempBright, sizeof(settings.tempBright), 0, 15, DEF_TEMP_BRIGHT);
        wd |= CopyCheckValidNumParm(json["tempOffNM"], settings.tempOffNM, sizeof(settings.tempOffNM), 0, 1, DEF_TEMP_OFF_NM);
        #endif
        #endif // HAVESPEEDO
        
        #ifdef FAKE_POWER_ON
        wd |= CopyCheckValidNumParm(json["fakePwrOn"], settings.fakePwrOn, sizeof(settings.fakePwrOn), 0, 1, DEF_FAKE_PWR);
        #endif

        #ifdef EXTERNAL_TIMETRAVEL_IN
        wd |= CopyCheckValidNumParm(json["ettDelay"], settings.ettDelay, sizeof(settings.ettDelay), 0, ETT_MAX_DEL, DEF_ETT_DELAY);
        //wd |= CopyCheckValidNumParm(json["ettLong"], settings.ettLong, sizeof(settings.ettLong), 0, 1, DEF_ETT_LONG);
        #endif
        
        #ifdef EXTERNAL_TIMETRAVEL_OUT
        wd |= CopyCheckValidNumParm(json["useETTO"], settings.useETTO, sizeof(settings.useETTO), 0, 1, DEF_USE_ETTO);
        #endif
        #ifdef TC_HAVEGPS
        wd |= CopyCheckValidNumParm(json["quickGPS"], settings.quickGPS, sizeof(settings.quickGPS), 0, 1, DEF_QUICK_GPS);
        #endif
        wd |= CopyCheckValidNumParm(json["playTTsnds"], settings.playTTsnds, sizeof(settings.playTTsnds), 0, 1, DEF_PLAY_TT_SND);

        #ifdef TC_HAVEMQTT
        wd |= CopyCheckValidNumParm(json["useMQTT"], settings.useMQTT, sizeof(settings.useMQTT), 0, 1, 0);
        if(json["mqttServer"]) {
            memset(settings.mqttServer, 0, sizeof(settings.mqttServer));
            strncpy(settings.mqttServer, json["mqttServer"], sizeof(settings.mqttServer) - 1);
        } else wd = true;
        if(json["mqttUser"]) {
            memset(settings.mqttUser, 0, sizeof(settings.mqttUser));
            strncpy(settings.mqttUser, json["mqttUser"], sizeof(settings.mqttUser) - 1);
        } else wd = true;
        if(json["mqttTopic"]) {
            memset(settings.mqttTopic, 0, sizeof(settings.mqttTopic));
            strncpy(settings.mqttTopic, json["mqttTopic"], sizeof(settings.mqttTopic) - 1);
        } else wd = true;
        #ifdef EXTERNAL_TIMETRAVEL_OUT
        wd |= CopyCheckValidNumParm(json["pubMQTT"], settings.pubMQTT, sizeof(settings.pubMQTT), 0, 1, 0);
        #endif
        #endif

        wd |= CopyCheckValidNumParm(json["shuffle"], settings.shuffle, sizeof(settings.shuffle), 0, 1, DEF_SHUFFLE);

        wd |= CopyCheckValidNumParm(json["CfgOnSD"], settings.CfgOnSD, sizeof(settings.CfgOnSD), 0, 1, DEF_CFG_ON_SD);
        //wd |= CopyCheckValidNumParm(json["sdFreq"], settings.sdFreq, sizeof(settings.sdFreq), 0, 1, DEF_SD_FREQ);

    } else {

        wd = true;

    }

    return wd;
}

void write_settings()
{
    const char *funcName = "write_settings";
    DynamicJsonDocument json(JSON_SIZE);
    //StaticJsonDocument<JSON_SIZE> json;

    if(!haveFS && !FlashROMode) {
        Serial.printf("%s: %s\n", funcName, fsNoAvail);
        return;
    }

    #ifdef TC_DBG
    Serial.printf("%s: Writing config file\n", funcName);
    #endif
    
    json["timeTrPers"] = settings.timesPers;
    json["alarmRTC"] = settings.alarmRTC;
    json["mode24"] = settings.mode24;
    json["beep"] = settings.beep;
    json["playIntro"] = settings.playIntro;
    json["autoRotateTimes"] = settings.autoRotateTimes;

    json["hostName"] = settings.hostName;
    json["wifiConRetries"] = settings.wifiConRetries;
    json["wifiConTimeout"] = settings.wifiConTimeout;
    json["wifiOffDelay"] = settings.wifiOffDelay;
    json["wifiAPOffDelay"] = settings.wifiAPOffDelay;
    json["wifiPRetry"] = settings.wifiPRetry;
    
    json["timeZone"] = settings.timeZone;
    json["ntpServer"] = settings.ntpServer;

    json["timeZoneDest"] = settings.timeZoneDest;
    json["timeZoneDep"] = settings.timeZoneDep;
    json["timeZoneNDest"] = settings.timeZoneNDest;
    json["timeZoneNDep"] = settings.timeZoneNDep;
    
    json["destTimeBright"] = settings.destTimeBright;
    json["presTimeBright"] = settings.presTimeBright;
    json["lastTimeBright"] = settings.lastTimeBright;

    json["dtNmOff"] = settings.dtNmOff;
    json["ptNmOff"] = settings.ptNmOff;
    json["ltNmOff"] = settings.ltNmOff;
    
    json["autoNMPreset"] = settings.autoNMPreset;
    json["autoNMOn"] = settings.autoNMOn;
    json["autoNMOff"] = settings.autoNMOff;
    #ifdef TC_HAVELIGHT
    json["useLight"] = settings.useLight;
    json["luxLimit"] = settings.luxLimit;
    #endif

    #ifdef TC_HAVETEMP
    json["tempUnit"] = settings.tempUnit;
    json["tempOffs"] = settings.tempOffs;
    #endif

    #ifdef TC_HAVESPEEDO    
    json["speedoType"] = settings.speedoType;
    json["speedoBright"] = settings.speedoBright;
    json["speedoFact"] = settings.speedoFact;
    #ifdef TC_HAVEGPS
    json["useGPSSpeed"] = settings.useGPSSpeed;
    #endif
    #ifdef TC_HAVETEMP
    json["dispTemp"] = settings.dispTemp;
    json["tempBright"] = settings.tempBright;
    json["tempOffNM"] = settings.tempOffNM;
    #endif
    #endif // HAVESPEEDO
    
    #ifdef FAKE_POWER_ON
    json["fakePwrOn"] = settings.fakePwrOn;
    #endif

    #ifdef EXTERNAL_TIMETRAVEL_IN
    json["ettDelay"] = settings.ettDelay;
    //json["ettLong"] = settings.ettLong;
    #endif

    #ifdef EXTERNAL_TIMETRAVEL_OUT
    json["useETTO"] = settings.useETTO;
    #endif
    #ifdef TC_HAVEGPS
    json["quickGPS"] = settings.quickGPS;
    #endif
    json["playTTsnds"] = settings.playTTsnds;

    #ifdef TC_HAVEMQTT
    json["useMQTT"] = settings.useMQTT;
    json["mqttServer"] = settings.mqttServer;
    json["mqttUser"] = settings.mqttUser;
    json["mqttTopic"] = settings.mqttTopic;
    #ifdef EXTERNAL_TIMETRAVEL_OUT
    json["pubMQTT"] = settings.pubMQTT;
    #endif
    #endif

    json["shuffle"] = settings.shuffle;

    json["CfgOnSD"] = settings.CfgOnSD;
    //json["sdFreq"] = settings.sdFreq;

    File configFile = FlashROMode ? SD.open(cfgName, FILE_WRITE) : SPIFFS.open(cfgName, FILE_WRITE);

    if(configFile) {

        #ifdef TC_DBG
        serializeJson(json, Serial);
        Serial.println(F(" "));
        #endif
        
        serializeJson(json, configFile);
        configFile.close();

    } else {

        Serial.printf("%s: %s\n", funcName, failFileWrite);

    }
}

bool checkConfigExists()
{
    return FlashROMode ? SD.exists(cfgName) : SPIFFS.exists(cfgName);
}


/*
 *  Helpers for parm copying & checking
 */

static bool CopyCheckValidNumParm(const char *json, char *text, uint8_t psize, int lowerLim, int upperLim, int setDefault)
{
    if(!json) return true;

    memset(text, 0, psize);
    strncpy(text, json, psize-1);
    return checkValidNumParm(text, lowerLim, upperLim, setDefault);
}

static bool CopyCheckValidNumParmF(const char *json, char *text, uint8_t psize, float lowerLim, float upperLim, float setDefault)
{
    if(!json) return true;

    memset(text, 0, psize);
    strncpy(text, json, psize-1);
    return checkValidNumParmF(text, lowerLim, upperLim, setDefault);
}

static bool checkValidNumParm(char *text, int lowerLim, int upperLim, int setDefault)
{
    int i, len = strlen(text);

    if(len == 0) {
        sprintf(text, "%d", setDefault);
        return true;
    }

    for(i = 0; i < len; i++) {
        if(text[i] < '0' || text[i] > '9') {
            sprintf(text, "%d", setDefault);
            return true;
        }
    }

    i = (int)(atoi(text));

    if(i < lowerLim) {
        sprintf(text, "%d", lowerLim);
        return true;
    }
    if(i > upperLim) {
        sprintf(text, "%d", upperLim);
        return true;
    }

    // Re-do to get rid of formatting errors (eg "000")
    sprintf(text, "%d", i);

    return false;
}

static bool checkValidNumParmF(char *text, float lowerLim, float upperLim, float setDefault)
{
    int i, len = strlen(text);
    float f;

    if(len == 0) {
        sprintf(text, "%1.1f", setDefault);
        return true;
    }

    for(i = 0; i < len; i++) {
        if(text[i] != '.' && text[i] != '-' && (text[i] < '0' || text[i] > '9')) {
            sprintf(text, "%1.1f", setDefault);
            return true;
        }
    }

    f = atof(text);

    if(f < lowerLim) {
        sprintf(text, "%1.1f", lowerLim);
        return true;
    }
    if(f > upperLim) {
        sprintf(text, "%1.1f", upperLim);
        return true;
    }

    // Re-do to get rid of formatting errors (eg "0.")
    sprintf(text, "%1.1f", f);

    return false;
}

/*
 *  Load/save the Alarm settings
 */

bool loadAlarm()
{
    const char *funcName = "loadAlarm";
    bool writedefault = true;
    bool haveConfigFile = false;
    File configFile;

    if(!haveFS && !configOnSD) {
        Serial.printf("%s: %s\n", funcName, fsNoAvail);
        return false;
    }

    #ifdef TC_DBG
    Serial.printf("%s: Loading from %s\n", funcName, configOnSD ? "SD" : "flash FS");
    #endif

    if(configOnSD) {
        if(SD.exists(almCfgName)) {
            haveConfigFile = (configFile = SD.open(almCfgName, "r"));
        }
    } else {
        // Transition to new file name
        if(SPIFFS.exists("/alarmconfig.json")) {
            SPIFFS.rename("/alarmconfig.json", almCfgName);
        }
        if(SPIFFS.exists(almCfgName)) {
            haveConfigFile = (configFile = SPIFFS.open(almCfgName, "r"));
        }
    }

    if(haveConfigFile) {
        StaticJsonDocument<512> json;
        
        if(!deserializeJson(json, configFile)) {
            if(json["alarmonoff"] && json["alarmhour"] && json["alarmmin"]) {
                int aoo = atoi(json["alarmonoff"]);
                alarmHour = atoi(json["alarmhour"]);
                alarmMinute = atoi(json["alarmmin"]);
                alarmOnOff = ((aoo & 0x0f) != 0);
                alarmWeekday = (aoo & 0xf0) >> 4;
                if(alarmWeekday > 9) alarmWeekday = 0;
                if(((alarmHour   == 255) || (alarmHour   >= 0 && alarmHour   <= 23)) &&
                   ((alarmMinute == 255) || (alarmMinute >= 0 && alarmMinute <= 59))) {
                    writedefault = false;
                }
            }
        } 
        configFile.close();
    }

    if(writedefault) {
        Serial.printf("%s: %s\n", funcName, badConfig);
        alarmHour = alarmMinute = 255;
        alarmOnOff = false;
        alarmWeekday = 0;
        saveAlarm();
    }

    return true;
}

void saveAlarm()
{
    const char *funcName = "saveAlarm";
    char aooBuf[8];
    char hourBuf[8];
    char minBuf[8];
    bool haveConfigFile = false;
    File configFile;
    StaticJsonDocument<512> json;

    if(!haveFS && !configOnSD) {
        Serial.printf("%s: %s\n", funcName, fsNoAvail);
        return;
    }

    #ifdef TC_DBG
    Serial.printf("%s: Writing to %s\n", funcName, configOnSD ? "SD" : "flash FS");
    #endif

    sprintf(aooBuf, "%d", (alarmWeekday * 16) + (alarmOnOff ? 1 : 0));
    json["alarmonoff"] = (char *)aooBuf;

    sprintf(hourBuf, "%d", alarmHour);
    sprintf(minBuf, "%d", alarmMinute);

    json["alarmhour"] = (char *)hourBuf;
    json["alarmmin"] = (char *)minBuf;

    if(configOnSD) {
        haveConfigFile = (configFile = SD.open(almCfgName, FILE_WRITE));
    } else {
        haveConfigFile = (configFile = SPIFFS.open(almCfgName, FILE_WRITE));
    }

    if(haveConfigFile) {
        serializeJson(json, configFile);
        configFile.close();
    } else {
        Serial.printf("%s: %s\n", funcName, failFileWrite);
    }
}

/*
 *  Load/save the Yearly/Monthly Reminder settings
 */

bool loadReminder()
{
    const char *funcName = "loadReminder";
    bool writedefault = false;
    bool haveConfigFile = false;
    File configFile;

    if(!haveFS && !configOnSD) {
        Serial.printf("%s: %s\n", funcName, fsNoAvail);
        return false;
    }

    #ifdef TC_DBG
    Serial.printf("%s: Loading from %s\n", funcName, configOnSD ? "SD" : "flash FS");
    #endif

    if(configOnSD) {
        if(SD.exists(remCfgName)) {
            haveConfigFile = (configFile = SD.open(remCfgName, "r"));
        }
    } else {
        if(SPIFFS.exists(remCfgName)) {
            haveConfigFile = (configFile = SPIFFS.open(remCfgName, "r"));
        }
    }

    if(haveConfigFile) {
        StaticJsonDocument<512> json;
        
        if(!deserializeJson(json, configFile)) {
            if(json["month"] && json["hour"] && json["min"]) {
                remMonth  = atoi(json["month"]);
                remDay  = atoi(json["day"]);
                remHour = atoi(json["hour"]);
                remMin  = atoi(json["min"]);
                if(remMonth > 12 ||               // month can be zero (=monthly reminder)
                   remDay   > 31 || remDay < 1 ||
                   remHour  > 23 || 
                   remMin   > 59)
                    writedefault = true;
            }
        } 
        configFile.close();

        if(writedefault) {
            Serial.printf("%s: %s\n", funcName, badConfig);
            remMonth = remDay = remHour = remMin = 0;
            deleteReminder();
        }
        
    }

    return true;
}

void saveReminder()
{
    const char *funcName = "saveReminder";
    char monBuf[8];
    char dayBuf[8];
    char hourBuf[8];
    char minBuf[8];
    bool haveConfigFile = false;
    File configFile;
    StaticJsonDocument<512> json;

    if(!haveFS && !configOnSD) {
        Serial.printf("%s: %s\n", funcName, fsNoAvail);
        return;
    }

    if(!remMonth && !remDay) {
        deleteReminder();
        return;
    }

    #ifdef TC_DBG
    Serial.printf("%s: Writing to %s\n", funcName, configOnSD ? "SD" : "flash FS");
    #endif

    sprintf(monBuf, "%d", remMonth);
    sprintf(dayBuf, "%d", remDay);
    sprintf(hourBuf, "%d", remHour);
    sprintf(minBuf, "%d", remMin);
    
    json["month"] = (char *)monBuf;
    json["day"] = (char *)dayBuf;
    json["hour"] = (char *)hourBuf;
    json["min"] = (char *)minBuf;

    if(configOnSD) {
        haveConfigFile = (configFile = SD.open(remCfgName, FILE_WRITE));
    } else {
        haveConfigFile = (configFile = SPIFFS.open(remCfgName, FILE_WRITE));
    }

    if(haveConfigFile) {
        serializeJson(json, configFile);
        configFile.close();
    } else {
        Serial.printf("%s: %s\n", funcName, failFileWrite);
    }
}

static void deleteReminder()
{
    if(!haveFS && !configOnSD)
        return;

    if(configOnSD) {
        SD.remove(remCfgName);
    } else {
        SPIFFS.remove(remCfgName);
    }
}

/*
 *  Load/save the Volume
 */

bool loadCurVolume()
{
    const char *funcName = "loadCurVolume";
    char temp[6];
    bool writedefault = true;
    bool haveConfigFile = false;
    File configFile;

    curVolume = DEFAULT_VOLUME;

    if(!haveFS && !configOnSD) {
        Serial.printf("%s: %s\n", funcName, fsNoAvail);
        return false;
    }

    #ifdef TC_DBG
    Serial.printf("%s: Loading from %s\n", funcName, configOnSD ? "SD" : "flash FS");
    #endif

    if(configOnSD) {
        if(SD.exists(volCfgName)) {
            haveConfigFile = (configFile = SD.open(volCfgName, "r"));
        }
    } else {
        if(SPIFFS.exists(volCfgName)) {
            haveConfigFile = (configFile = SPIFFS.open(volCfgName, "r"));
        }
    }

    if(haveConfigFile) {
        StaticJsonDocument<512> json;
        if(!deserializeJson(json, configFile)) {
            if(!CopyCheckValidNumParm(json["volume"], temp, sizeof(temp), 0, 255, 255)) {
                uint8_t ncv = atoi(temp);
                if((ncv >= 0 && ncv <= 19) || ncv == 255) {
                    curVolume = ncv;
                    writedefault = false;
                } 
            }
        } 
        configFile.close();
    }

    if(writedefault) {
        Serial.printf("%s: %s\n", funcName, badConfig);
        saveCurVolume();
    }

    return true;
}

void saveCurVolume()
{
    const char *funcName = "saveCurVolume";
    char buf[6];
    bool haveConfigFile = false;
    File configFile;
    StaticJsonDocument<512> json;

    if(!haveFS && !configOnSD) {
        Serial.printf("%s: %s\n", funcName, fsNoAvail);
        return;
    }

    #ifdef TC_DBG
    Serial.printf("%s: Writing to %s\n", funcName, configOnSD ? "SD" : "flash FS");
    #endif

    sprintf(buf, "%d", curVolume);
    json["volume"] = (char *)buf;

    if(configOnSD) {
        haveConfigFile = (configFile = SD.open(volCfgName, FILE_WRITE));
    } else {
        haveConfigFile = (configFile = SPIFFS.open(volCfgName, FILE_WRITE));
    }

    #ifdef TC_DBG
    serializeJson(json, Serial);
    Serial.println(F(" "));
    #endif

    if(haveConfigFile) {
        serializeJson(json, configFile);
        configFile.close();
    } else {
        Serial.printf("%s: %s\n", funcName, failFileWrite);
    }
}

/* Copy alarm/volume settings from/to SD if user
 * changed "save to SD"-option in CP
 */

void copySettings()
{   
    if(!haveSD || !haveFS) 
        return;

    configOnSD = !configOnSD;

    if(configOnSD || !FlashROMode) {
        #ifdef TC_DBG
        Serial.println(F("copySettings: Copying alarm/vol settings to other medium"));
        #endif
        saveCurVolume();
        saveAlarm();
        saveReminder();
    }

    configOnSD = !configOnSD;
}

/*
 * Load/save Music Folder Number
 */

bool loadMusFoldNum()
{
    bool writedefault = true;
    char temp[4];

    if(!haveSD)
        return false;

    if(SD.exists(musCfgName)) {

        File configFile = SD.open(musCfgName, "r");
        if(configFile) {
            StaticJsonDocument<512> json;
            if(!deserializeJson(json, configFile)) {
                if(!CopyCheckValidNumParm(json["folder"], temp, sizeof(temp), 0, 9, 0)) {
                    musFolderNum = atoi(temp);
                    writedefault = false;
                }
            } 
            configFile.close();
        }

    }

    if(writedefault) {
        musFolderNum = 0;
        saveMusFoldNum();
    }

    return true;
}

void saveMusFoldNum()
{
    const char *funcName = "saveMusFoldNum";
    StaticJsonDocument<512> json;
    char buf[4];

    if(!haveSD)
        return;

    sprintf(buf, "%1d", musFolderNum);
    json["folder"] = buf;
    
    File configFile = SD.open(musCfgName, FILE_WRITE);

    if(configFile) {
        serializeJson(json, configFile);
        configFile.close();
    } else {
        Serial.printf("%s: %s\n", funcName, failFileWrite);
    }
}

/*
 * Load/save/delete settings for static IP configuration
 */

bool loadIpSettings()
{
    bool invalid = false;
    bool haveConfig = false;

    if(!haveFS && !FlashROMode)
        return false;

    if( (!FlashROMode && SPIFFS.exists(ipCfgName)) ||
        (FlashROMode && SD.exists(ipCfgName)) ) {

        File configFile = FlashROMode ? SD.open(ipCfgName, "r") : SPIFFS.open(ipCfgName, "r");

        if(configFile) {

            StaticJsonDocument<512> json;
            DeserializationError error = deserializeJson(json, configFile);

            #ifdef TC_DBG
            serializeJson(json, Serial);
            Serial.println(F(" "));
            #endif

            if(!error) {

                invalid |= CopyIPParm(json["IpAddress"], ipsettings.ip, sizeof(ipsettings.ip));
                invalid |= CopyIPParm(json["Gateway"], ipsettings.gateway, sizeof(ipsettings.gateway));
                invalid |= CopyIPParm(json["Netmask"], ipsettings.netmask, sizeof(ipsettings.netmask));
                invalid |= CopyIPParm(json["DNS"], ipsettings.dns, sizeof(ipsettings.dns));
                
                haveConfig = !invalid;

            } else {

                invalid = true;

            }

            configFile.close();

        }

    }

    if(invalid) {

        // config file is invalid - delete it

        Serial.println(F("loadIpSettings: IP settings invalid; deleting file"));

        deleteIpSettings();

        memset(ipsettings.ip, 0, sizeof(ipsettings.ip));
        memset(ipsettings.gateway, 0, sizeof(ipsettings.gateway));
        memset(ipsettings.netmask, 0, sizeof(ipsettings.netmask));
        memset(ipsettings.dns, 0, sizeof(ipsettings.dns));

    }

    return haveConfig;
}

static bool CopyIPParm(const char *json, char *text, uint8_t psize)
{
    if(!json) return true;

    if(strlen(json) == 0) 
        return true;

    memset(text, 0, psize);
    strncpy(text, json, psize-1);
    return false;
}

void writeIpSettings()
{
    StaticJsonDocument<512> json;

    if(!haveFS && !FlashROMode)
        return;

    if(strlen(ipsettings.ip) == 0)
        return;

    #ifdef TC_DBG
    Serial.println(F("writeIpSettings: Writing file"));
    #endif
    
    json["IpAddress"] = ipsettings.ip;
    json["Gateway"] = ipsettings.gateway;
    json["Netmask"] = ipsettings.netmask;
    json["DNS"] = ipsettings.dns;

    File configFile = FlashROMode ? SD.open(ipCfgName, FILE_WRITE) : SPIFFS.open(ipCfgName, FILE_WRITE);

    #ifdef TC_DBG
    serializeJson(json, Serial);
    Serial.println(F(" "));
    #endif

    if(configFile) {
        serializeJson(json, configFile);
        configFile.close();
    } else {
        Serial.printf("writeIpSettings: %s\n", failFileWrite);
    }
}

void deleteIpSettings()
{
    if(!haveFS && !FlashROMode)
        return;

    #ifdef TC_DBG
    Serial.println(F("deleteIpSettings: Deleting ip config"));
    #endif

    if(FlashROMode) {
        SD.remove(ipCfgName);
    } else {
        SPIFFS.remove(ipCfgName);
    }
}

/*
 * Audio file installer
 *
 * Copies our default audio files from SD to flash FS.
 * The is restricted to the original default audio
 * files that came with the software. If you want to
 * customize your sounds, put them on a FAT32 formatted
 * SD card and leave this SD card in the slot.
 */

bool check_allow_CPA()
{
    return allowCPA;
}

static bool check_if_default_audio_present()
{
    File file;
    size_t ts;
    int i, idx = 0;
    char dtmf_buf[16] = "/Dtmf-0.mp3\0";
    size_t sizes[10+NUM_AUDIOFILES] = {
      4178, 4178, 4178, 4178, 4178, 4178, 3760, 3760, 4596, 3760, // DTMF
      65230, 71500, 60633, 10478,           // alarm, alarmoff, alarmon, baddate
      15184, 22983, 33364, 51701,           // ee1, ee2, ee3, ee4
      SND_ENTER_LEN, 125804, 33853, 47228,  // enter, intro, nmoff, nmon
      16747, 151719, 3790, SND_STARTUP_LEN, // ping, reminder, shutdown, startup, 
      84894, 38899, 135447                  // timer, timetravel, travelstart
    };

    if(!haveSD)
        return false;

    // If identifier missing, quit now
    if(!(SD.exists(IDFN))) {
        #ifdef TC_DBG
        Serial.println("SD: ID file not present");
        #endif
        return false;
    }

    for(i = 0; i < 10; i++) {
        dtmf_buf[6] = i + '0';
        if(!(SD.exists(dtmf_buf))) {
            #ifdef TC_DBG
            Serial.printf("missing: %s\n", dtmf_buf);
            #endif
            return false;
        }
        if(!(file = SD.open(dtmf_buf)))
            return false;
        ts = file.size();
        file.close();
        #ifdef TC_DBG
        sizes[idx++] = ts;
        #else
        if(sizes[idx++] != ts)
            return false;
        #endif
    }

    for(i = 0; i < NUM_AUDIOFILES; i++) {
        if(!SD.exists(audioFiles[i])) {
            #ifdef TC_DBG
            Serial.printf("missing: %s\n", audioFiles[i]);
            #endif
            return false;
        }
        if(!(file = SD.open(audioFiles[i])))
            return false;
        ts = file.size();
        file.close();
        #ifdef TC_DBG
        sizes[idx++] = ts;
        #else
        if(sizes[idx++] != ts)
            return false;
        #endif
    }

    return true;
}

bool copy_audio_files()
{
    char dtmf_buf[16] = "/Dtmf-0.mp3\0";
    int i, haveErr = 0;

    if(!allowCPA) {
        return false;
    }

    start_file_copy();

    /* Copy DTMF files */
    for(i = 0; i < 10; i++) {
        dtmf_buf[6] = i + '0';
        open_and_copy(dtmf_buf, haveErr);
    }

    for(i = 0; i < NUM_AUDIOFILES; i++) {
        open_and_copy(audioFiles[i], haveErr);
    }

    if(haveErr) {
        file_copy_error();
    } else {
        file_copy_done();
    }

    return (haveErr == 0);
}

static void open_and_copy(const char *fn, int& haveErr)
{
    const char *funcName = "copy_audio_files";
    File sFile, dFile;

    if((sFile = SD.open(fn, FILE_READ))) {
        #ifdef TC_DBG
        Serial.printf("%s: Opened source file: %s\n", funcName, fn);
        #endif
        if((dFile = SPIFFS.open(fn, FILE_WRITE))) {
            #ifdef TC_DBG
            Serial.printf("%s: Opened destination file: %s\n", funcName, fn);
            #endif
            if(!filecopy(sFile, dFile)) {
                haveErr++;
            }
            dFile.close();
        } else {
            Serial.printf("%s: Error opening destination file: %s\n", funcName, fn);
            haveErr++;
        }
        sFile.close();
    } else {
        Serial.printf("%s: Error opening source file: %s\n", funcName, fn);
        haveErr++;
    }
}

static bool filecopy(File source, File dest)
{
    uint8_t buffer[1024];
    size_t bytesr, bytesw;

    while((bytesr = source.read(buffer, 1024))) {
        if((bytesw = dest.write(buffer, bytesr)) != bytesr) {
            Serial.println(F("filecopy: Error writing data"));
            return false;
        }
        file_copy_progress();
    }

    return true;
}

bool audio_files_present()
{
    File file;
    size_t ts;
    
    if(FlashROMode)
        return true;

    if(!SPIFFS.exists(audioFiles[SND_ENTER_IDX]))
        return false;
      
    if(!(file = SPIFFS.open(audioFiles[SND_ENTER_IDX])))
        return false;
      
    ts = file.size();
    file.close();

    if(ts != SND_ENTER_LEN)
        return false;

    return true;
}

void delete_ID_file()
{
    if(!haveSD)
        return;

    #ifdef TC_DBG
    Serial.printf("Deleting ID file %s\n", IDFN);
    #endif
        
    if(SD.exists(IDFN)) {
        SD.remove(IDFN);
    }
}

/*
 * Various helpers
 */

void formatFlashFS()
{
    #ifdef TC_DBG
    Serial.println(F("Formatting flash FS"));
    #endif
    SPIFFS.format();
}

// Re-write IP/alarm/reminder/vol settings
// Used during audio file installation when flash FS needs
// to be re-formatted.
void rewriteSecondarySettings()
{
    bool oldconfigOnSD = configOnSD;
    
    #ifdef TC_DBG
    Serial.println("Re-writing seconday settings");
    #endif
    
    writeIpSettings();

    // Create current alarm/volume settings on flash FS
    // regardless of SD-option
    configOnSD = false;
    
    saveAlarm();

    saveReminder();
    
    saveCurVolume();
    
    configOnSD = oldconfigOnSD;

    // Music Folder Number is always on SD only
}

bool readFileFromSD(const char *fn, uint8_t *buf, int len)
{
    size_t bytesr;
    
    if(!haveSD)
        return false;

    File myFile = SD.open(fn, FILE_READ);
    if(myFile) {
        bytesr = myFile.read(buf, len);
        myFile.close();
        return (bytesr == len);
    } else
        return false;
}

bool writeFileToSD(const char *fn, uint8_t *buf, int len)
{
    size_t bytesw;
    
    if(!haveSD)
        return false;

    File myFile = SD.open(fn, FILE_WRITE);
    if(myFile) {
        bytesw = myFile.write(buf, len);
        myFile.close();
        return (bytesw == len);
    } else
        return false;
}

bool readFileFromFS(const char *fn, uint8_t *buf, int len)
{
    size_t bytesr;
    
    if(!haveFS)
        return false;

    if(!SPIFFS.exists(fn))
        return false;

    File myFile = SPIFFS.open(fn, FILE_READ);
    if(myFile) {
        bytesr = myFile.read(buf, len);
        myFile.close();
        return (bytesr == len);
    } else
        return false;
}

bool writeFileToFS(const char *fn, uint8_t *buf, int len)
{
    size_t bytesw;
    
    if(!haveFS)
        return false;

    File myFile = SPIFFS.open(fn, FILE_WRITE);
    if(myFile) {
        bytesw = myFile.write(buf, len);
        myFile.close();
        return (bytesw == len);
    } else
        return false;
}
