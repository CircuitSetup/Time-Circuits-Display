/*
 * -------------------------------------------------------------------
 * CircuitSetup.us Time Circuits Display
 * (C) 2021-2022 John deGlavina https://circuitsetup.us
 * (C) 2022-2024 Thomas Winischhofer (A10001986)
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

#include "tc_global.h"

#define ARDUINOJSON_USE_LONG_LONG 0
#define ARDUINOJSON_USE_DOUBLE 0
#define ARDUINOJSON_ENABLE_ARDUINO_STRING 0
#define ARDUINOJSON_ENABLE_ARDUINO_STREAM 0
#define ARDUINOJSON_ENABLE_STD_STREAM 0
#define ARDUINOJSON_ENABLE_STD_STRING 0
#define ARDUINOJSON_ENABLE_NAN 0
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
#include "tc_wifi.h"
#ifdef HAVE_STALE_PRESENT
#include "clockdisplay.h"
#endif

// Size of main config JSON
// Needs to be adapted when config grows
#define JSON_SIZE 2500
#if ARDUINOJSON_VERSION_MAJOR >= 7
#define DECLARE_S_JSON(x,n) JsonDocument n;
#define DECLARE_D_JSON(x,n) JsonDocument n;
#else
#define DECLARE_S_JSON(x,n) StaticJsonDocument<x> n;
#define DECLARE_D_JSON(x,n) DynamicJsonDocument n(x);
#endif 

#define NUM_AUDIOFILES 20
#define AC_FMTV 2
#define AC_OHSZ (14 + ((10+NUM_AUDIOFILES+1)*(32+4)))
#ifndef TWSOUND
#define SND_REQ_VERSION "CS03"
#define AC_TS 1236252
#else
#define SND_REQ_VERSION "TW03"
#define AC_TS 1231539
#endif

static const char *CONFN  = "/TCDA.bin";
static const char *CONFND = "/TCDA.old";
static const char *CONID  = "TCDA";
static uint32_t   soa = AC_TS;
static bool       ic = false;
static uint8_t*   f(uint8_t *d, uint32_t m, int y) { return d; }

static const char *cfgName    = "/config.json";     // Main config (flash)
static const char *ipCfgName  = "/ipconfig.json";   // IP config (flash)
static const char *briCfgName = "/tcdbricfg.json";  // Brightness (flash/SD)
static const char *ainCfgName = "/tcdaicfg.json";   // Auto-interval (flash/SD)
static const char *volCfgName = "/tcdvolcfg.json";  // Volume config (flash/SD)
static const char *almCfgName = "/tcdalmcfg.json";  // Alarm config (flash/SD)
static const char *remCfgName = "/tcdremcfg.json";  // Reminder config (flash/SD)
#ifdef HAVE_STALE_PRESENT
static const char *stCfgName  = "/stconfig";        // Exhib Mode (flash/SD)
#endif
static const char *musCfgName = "/tcdmcfg.json";    // Music config (SD)
static const char *cmCfgName  = "/cmconfig.json";   // Carmode (flash/SD)
#ifdef TC_HAVELINEOUT
static const char *loCfgName  = "/loconfig.json";   // LineOut (flash/SD)
#endif
#ifdef TC_HAVE_REMOTE
static const char *raCfgName  = "/raconfig.json";   // RemoteAllowed (flash/SD)
#endif


static const char *fsNoAvail = "File System not available";
static const char *failFileWrite = "Failed to open file for writing";
#ifdef TC_DBG
static const char *badConfig = "Settings bad/missing/incomplete; writing new file";
#endif

/* If SPIFFS/LittleFS is mounted */
bool haveFS = false;

/* If a SD card is found */
bool haveSD = false;

/* Save sedondary settings on SD? */
static bool configOnSD = false;

/* Paranoia: No writes Flash-FS  */
bool FlashROMode = false;

/* If SD contains default audio files */
static bool allowCPA = false;

/* If current audio data is installed */
bool haveAudioFiles = false;

/* Music Folder Number */
uint8_t musFolderNum = 0;

/* Cache */
static uint8_t  preSavedDBri = 20;
static uint8_t  preSavedPBri = 20;
static uint8_t  preSavedLBri = 20;
static uint8_t  preSavedAI   = 99;
static uint8_t  prevSavedVol = 254;
static uint8_t* (*r)(uint8_t *, uint32_t, int);

static bool read_settings(File configFile);

static void CopyTextParm(char *setting, const char *json, int setSize);
static bool CopyCheckValidNumParm(const char *json, char *text, uint8_t psize, int lowerLim, int upperLim, int setDefault);
static bool CopyCheckValidNumParmF(const char *json, char *text, uint8_t psize, float lowerLim, float upperLim, float setDefault);
static bool checkValidNumParm(char *text, int lowerLim, int upperLim, int setDefault);
static bool checkValidNumParmF(char *text, float lowerLim, float upperLim, float setDefault);

static bool loadBrightness();
static bool loadAutoInterval();

static void deleteReminder();
static void loadCarMode();

#ifdef TC_HAVE_REMOTE
static void loadRemoteAllowed();
#endif

static void setupDisplayConfigOnSD();
static void saveDisplayData();

static void cfc(File& sfile, int& haveErr, int& haveWriteErr);
static bool audio_files_present();

static bool CopyIPParm(const char *json, char *text, uint8_t psize);

static DeserializationError readJSONCfgFile(JsonDocument& json, File& configFile, const char *funcName);
static bool writeJSONCfgFile(const JsonDocument& json, const char *fn, bool useSD, const char *funcName);

extern void start_file_copy();
extern void file_copy_progress();
extern void file_copy_done(int err);

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
    bool SDres = false;

    // Pre-maturely use ENTER button (initialized again in keypad_setup())
    // Pin pulled-down on control board
    pinMode(ENTER_BUTTON_PIN, INPUT);

    // Detect switchable Line-Out capability (Control Board version >=1.4.5)
    pinMode(MUTE_LINEOUT_PIN, OUTPUT);
    digitalWrite(MUTE_LINEOUT_PIN, LOW);
    pinMode(MLO_MIRROR, INPUT);
    
    delay(20);

    haveLineOut = false;
    
    if(!digitalRead(MLO_MIRROR)) {
        digitalWrite(MUTE_LINEOUT_PIN, HIGH);
        delay(20);
        if(digitalRead(MLO_MIRROR)) {
            digitalWrite(MUTE_LINEOUT_PIN, LOW);
            delay(20);
            if(!digitalRead(MLO_MIRROR)) {
                haveLineOut = true;
                #ifdef TC_DBG
                Serial.println("Switchable line-out detected");
                #endif
            }
        }
    }

    if(haveLineOut) {
        volumePin = VOLUME_PIN_NEW;
    } else {
        volumePin = VOLUME_PIN;
        digitalWrite(MUTE_LINEOUT_PIN, LOW);  // Disable "status led" on CB < 1.4.5
    }

    pinMode(volumePin, INPUT);

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
        int tBytes = SPIFFS.totalBytes(); int uBytes = SPIFFS.usedBytes();
        Serial-printf("FlashFS: %d total, %d used\n", tBytes, uBytes);
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

        Serial.println(F("\n***Mounting flash FS failed."));
        Serial.println(F("*** All settings will be stored on the SD card (if available)"));

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

    if(!(SDres = SD.begin(SD_CS_PIN, SPI, sdfreq))) {
        #ifdef TC_DBG
        Serial.printf("Retrying at 25Mhz... ");
        #endif
        SDres = SD.begin(SD_CS_PIN, SPI, 25000000);
    }

    if(SDres) {

        #ifdef TC_DBG
        Serial.println(F("ok"));
        #endif

        uint8_t cardType = SD.cardType();
       
        #ifdef TC_DBG
        const char *sdTypes[5] = { "No card", "MMC", "SD", "SDHC", "unknown (SD not usable)" };
        Serial.printf("SD card type: %s\n", sdTypes[cardType > 4 ? 4 : cardType]);
        #endif

        haveSD = ((cardType != CARD_NONE) && (cardType != CARD_UNKNOWN));

    } else {

        Serial.println(F("No SD card found"));

    }

    if(haveSD) {
        if(SD.exists("/TCD_FLASH_RO") || !haveFS) {
            bool writedefault2 = false;
            FlashROMode = true;
            Serial.println(F("Flash-RO mode: All settings stored on SD. Reloading settings."));
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
                #ifdef TC_DBG
                Serial.printf("%s: %s\n", funcName, badConfig);
                #endif
                write_settings();
            }
        }
    }

    // Now write new config to flash FS if old one somehow bad
    // Only write this file if FlashROMode is off
    if(haveFS && writedefault && !FlashROMode) {
        #ifdef TC_DBG
        Serial.printf("%s: %s\n", funcName, badConfig);
        #endif
        write_settings();
    }

    // Determine if secondary settings are to be stored on SD
    configOnSD = (haveSD && ((settings.CfgOnSD[0] != '0') || FlashROMode));

    // Setup save target for display objects
    setupDisplayConfigOnSD();

    // Load display brightness config
    loadBrightness();

    // Load (copy) autoInterval ("time cycling interval")
    loadAutoInterval();

    // Load carMode setting
    loadCarMode();

    // Load RemoveAllowed setting
    loadRemoteAllowed();

    // Check if (current) audio data is installed
    haveAudioFiles = audio_files_present();
    
    // Check if SD contains the default sound files
    if((r = e) && haveSD && (FlashROMode || haveFS)) {
        allowCPA = check_if_default_audio_present();
    }
    
    // Allow user to delete static IP data by holding ENTER
    // while booting
    // (10 secs timeout to wait for button-release to allow
    // to run fw without control board attached)
    if(digitalRead(ENTER_BUTTON_PIN)) {

        unsigned long mnow = millis();

        Serial.printf("%s: Deleting ip config; temporarily clearing AP mode WiFi password\n", funcName);

        deleteIpSettings();

        // Set AP mode password to empty (not written, only until reboot!)
        settings.appw[0] = 0;

        // Pre-maturely use white led (initialized again in keypad_setup())
        pinMode(WHITE_LED_PIN, OUTPUT);
        digitalWrite(WHITE_LED_PIN, HIGH);
        while(digitalRead(ENTER_BUTTON_PIN)) {
            if(millis() - mnow > 10*1000) break;
        }
        digitalWrite(WHITE_LED_PIN, LOW);
    }
}

void unmount_fs()
{
    if(haveFS) {
        SPIFFS.end();
        #ifdef TC_DBG
        Serial.println(F("Unmounted Flash FS"));
        #endif
        haveFS = false;
    }
    if(haveSD) {
        SD.end();
        #ifdef TC_DBG
        Serial.println(F("Unmounted SD card"));
        #endif
        haveSD = false;
    }
}

static bool read_settings(File configFile)
{
    const char *funcName = "read_settings";
    bool wd = false;
    size_t jsonSize = 0;
    DECLARE_D_JSON(JSON_SIZE,json);
    /*
    //StaticJsonDocument<JSON_SIZE> json;
    DynamicJsonDocument json(JSON_SIZE);
    */

    DeserializationError error = readJSONCfgFile(json, configFile, funcName);

    #if ARDUINOJSON_VERSION_MAJOR < 7
    jsonSize = json.memoryUsage();
    if(jsonSize > JSON_SIZE) {
        Serial.printf("%s: ERROR: Config too large (%d vs %d), memory corrupted.\n", funcName, jsonSize, JSON_SIZE);
    }
    
    #ifdef TC_DBG
    if(jsonSize > JSON_SIZE - 256) {
          Serial.printf("%s: WARNING: JSON_SIZE needs to be adapted **************\n", funcName);
    }
    Serial.printf("%s: Size of document: %d (JSON_SIZE %d)\n", funcName, jsonSize, JSON_SIZE);
    #endif
    #endif

    if(!error) {

        wd |= CopyCheckValidNumParm(json["timeTrPers"], settings.timesPers, sizeof(settings.timesPers), 0, 1, DEF_TIMES_PERS);
        wd |= CopyCheckValidNumParm(json["alarmRTC"], settings.alarmRTC, sizeof(settings.alarmRTC), 0, 1, DEF_ALARM_RTC);
        wd |= CopyCheckValidNumParm(json["playIntro"], settings.playIntro, sizeof(settings.playIntro), 0, 1, DEF_PLAY_INTRO);
        wd |= CopyCheckValidNumParm(json["mode24"], settings.mode24, sizeof(settings.mode24), 0, 1, DEF_MODE24);
        wd |= CopyCheckValidNumParm(json["beep"], settings.beep, sizeof(settings.beep), 0, 3, DEF_BEEP);
        
        // Transition - now in separate file
        //CopyCheckValidNumParm(json["autoRotateTimes"], settings.autoRotateTimes, sizeof(settings.autoRotateTimes), 0, 5, DEF_AUTOROTTIMES);

        if(json["hostName"]) {
            CopyTextParm(settings.hostName, json["hostName"], sizeof(settings.hostName));
        } else wd = true;

        if(json["systemID"]) {
            CopyTextParm(settings.systemID, json["systemID"], sizeof(settings.systemID));
        } else wd = true;

        if(json["appw"]) {
            CopyTextParm(settings.appw, json["appw"], sizeof(settings.appw));
        } else wd = true;
        
        wd |= CopyCheckValidNumParm(json["wifiConRetries"], settings.wifiConRetries, sizeof(settings.wifiConRetries), 1, 10, DEF_WIFI_RETRY);
        wd |= CopyCheckValidNumParm(json["wifiConTimeout"], settings.wifiConTimeout, sizeof(settings.wifiConTimeout), 7, 25, DEF_WIFI_TIMEOUT);
        wd |= CopyCheckValidNumParm(json["wifiOffDelay"], settings.wifiOffDelay, sizeof(settings.wifiOffDelay), 0, 99, DEF_WIFI_OFFDELAY);
        wd |= CopyCheckValidNumParm(json["wifiAPOffDelay"], settings.wifiAPOffDelay, sizeof(settings.wifiAPOffDelay), 0, 99, DEF_WIFI_APOFFDELAY);
        wd |= CopyCheckValidNumParm(json["wifiPRetry"], settings.wifiPRetry, sizeof(settings.wifiPRetry), 0, 1, DEF_WIFI_PRETRY);
        
        if(json["timeZone"]) {
            CopyTextParm(settings.timeZone, json["timeZone"], sizeof(settings.timeZone));
        } else wd = true;
        if(json["ntpServer"]) {
            CopyTextParm(settings.ntpServer, json["ntpServer"], sizeof(settings.ntpServer));
        } else wd = true;

        if(json["timeZoneDest"]) {
            CopyTextParm(settings.timeZoneDest, json["timeZoneDest"], sizeof(settings.timeZoneDest));
        } else wd = true;
        if(json["timeZoneDep"]) {
            CopyTextParm(settings.timeZoneDep, json["timeZoneDep"], sizeof(settings.timeZoneDep));
        } else wd = true;
        if(json["timeZoneNDest"]) {
            CopyTextParm(settings.timeZoneNDest, json["timeZoneNDest"], sizeof(settings.timeZoneNDest));
        } else wd = true;
        if(json["timeZoneNDep"]) {
            CopyTextParm(settings.timeZoneNDep, json["timeZoneNDep"], sizeof(settings.timeZoneNDep));
        } else wd = true;

        // Transition - now separate config file
        //CopyCheckValidNumParm(json["destTimeBright"], settings.destTimeBright, sizeof(settings.destTimeBright), 0, 15, DEF_BRIGHT_DEST);
        //CopyCheckValidNumParm(json["presTimeBright"], settings.presTimeBright, sizeof(settings.presTimeBright), 0, 15, DEF_BRIGHT_PRES);
        //CopyCheckValidNumParm(json["lastTimeBright"], settings.lastTimeBright, sizeof(settings.lastTimeBright), 0, 15, DEF_BRIGHT_DEPA);

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
        wd |= CopyCheckValidNumParm(json["speedoAF"], settings.speedoAF, sizeof(settings.speedoAF), 0, 1, DEF_SPEEDO_ACCELFIG);
        wd |= CopyCheckValidNumParmF(json["speedoFact"], settings.speedoFact, sizeof(settings.speedoFact), 0.5, 5.0, DEF_SPEEDO_FACT);
        #ifdef TC_HAVEGPS
        wd |= CopyCheckValidNumParm(json["useGPSSpeed"], settings.useGPSSpeed, sizeof(settings.useGPSSpeed), 0, 1, DEF_USE_GPS_SPEED);
        wd |= CopyCheckValidNumParm(json["spdUpdRate"], settings.spdUpdRate, sizeof(settings.spdUpdRate), 0, 3, DEF_SPD_UPD_RATE);
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
        wd |= CopyCheckValidNumParm(json["noETTOLead"], settings.noETTOLead, sizeof(settings.noETTOLead), 0, 1, DEF_NO_ETTO_LEAD);
        #endif
        #ifdef TC_HAVEGPS
        wd |= CopyCheckValidNumParm(json["quickGPS"], settings.quickGPS, sizeof(settings.quickGPS), 0, 1, DEF_QUICK_GPS);
        #endif
        wd |= CopyCheckValidNumParm(json["playTTsnds"], settings.playTTsnds, sizeof(settings.playTTsnds), 0, 1, DEF_PLAY_TT_SND);

        #ifdef TC_HAVEMQTT
        wd |= CopyCheckValidNumParm(json["useMQTT"], settings.useMQTT, sizeof(settings.useMQTT), 0, 1, 0);
        if(json["mqttServer"]) {
            CopyTextParm(settings.mqttServer, json["mqttServer"], sizeof(settings.mqttServer));
        } else wd = true;
        if(json["mqttUser"]) {
            CopyTextParm(settings.mqttUser, json["mqttUser"], sizeof(settings.mqttUser));
        } else wd = true;
        if(json["mqttTopic"]) {
            CopyTextParm(settings.mqttTopic, json["mqttTopic"], sizeof(settings.mqttTopic));
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
    DECLARE_D_JSON(JSON_SIZE,json);
    /*
    DynamicJsonDocument json(JSON_SIZE);
    //StaticJsonDocument<JSON_SIZE> json;
    */

    if(!haveFS && !FlashROMode) {
        Serial.printf("%s: %s\n", funcName, fsNoAvail);
        return;
    }

    #ifdef TC_DBG
    Serial.printf("%s: Writing config file\n", funcName);
    #endif
    
    json["timeTrPers"] = (const char *)settings.timesPers;
    json["alarmRTC"] = (const char *)settings.alarmRTC;
    json["mode24"] = (const char *)settings.mode24;
    json["beep"] = (const char *)settings.beep;
    json["playIntro"] = (const char *)settings.playIntro;

    json["hostName"] = (const char *)settings.hostName;
    json["systemID"] = (const char *)settings.systemID;
    json["appw"] = (const char *)settings.appw;
    json["wifiConRetries"] = (const char *)settings.wifiConRetries;
    json["wifiConTimeout"] = (const char *)settings.wifiConTimeout;
    json["wifiOffDelay"] = (const char *)settings.wifiOffDelay;
    json["wifiAPOffDelay"] = (const char *)settings.wifiAPOffDelay;
    json["wifiPRetry"] = (const char *)settings.wifiPRetry;
    
    json["timeZone"] = (const char *)settings.timeZone;
    json["ntpServer"] = (const char *)settings.ntpServer;

    json["timeZoneDest"] = (const char *)settings.timeZoneDest;
    json["timeZoneDep"] = (const char *)settings.timeZoneDep;
    json["timeZoneNDest"] = (const char *)settings.timeZoneNDest;
    json["timeZoneNDep"] = (const char *)settings.timeZoneNDep;

    json["dtNmOff"] = (const char *)settings.dtNmOff;
    json["ptNmOff"] = (const char *)settings.ptNmOff;
    json["ltNmOff"] = (const char *)settings.ltNmOff;
    
    json["autoNMPreset"] = (const char *)settings.autoNMPreset;
    json["autoNMOn"] = (const char *)settings.autoNMOn;
    json["autoNMOff"] = (const char *)settings.autoNMOff;
    #ifdef TC_HAVELIGHT
    json["useLight"] = (const char *)settings.useLight;
    json["luxLimit"] = (const char *)settings.luxLimit;
    #endif

    #ifdef TC_HAVETEMP
    json["tempUnit"] = (const char *)settings.tempUnit;
    json["tempOffs"] = (const char *)settings.tempOffs;
    #endif

    #ifdef TC_HAVESPEEDO    
    json["speedoType"] = (const char *)settings.speedoType;
    json["speedoBright"] = (const char *)settings.speedoBright;
    json["speedoAF"] = (const char *)settings.speedoAF;
    json["speedoFact"] = (const char *)settings.speedoFact;
    #ifdef TC_HAVEGPS
    json["useGPSSpeed"] = (const char *)settings.useGPSSpeed;
    json["spdUpdRate"] = (const char *)settings.spdUpdRate;
    #endif
    #ifdef TC_HAVETEMP
    json["dispTemp"] = (const char *)settings.dispTemp;
    json["tempBright"] = (const char *)settings.tempBright;
    json["tempOffNM"] = (const char *)settings.tempOffNM;
    #endif
    #endif // HAVESPEEDO
    
    #ifdef FAKE_POWER_ON
    json["fakePwrOn"] = (const char *)settings.fakePwrOn;
    #endif

    #ifdef EXTERNAL_TIMETRAVEL_IN
    json["ettDelay"] = (const char *)settings.ettDelay;
    //json["ettLong"] = (const char *)settings.ettLong;
    #endif

    #ifdef EXTERNAL_TIMETRAVEL_OUT
    json["useETTO"] = (const char *)settings.useETTO;
    json["noETTOLead"] = (const char *)settings.noETTOLead;
    #endif
    #ifdef TC_HAVEGPS
    json["quickGPS"] = (const char *)settings.quickGPS;
    #endif
    json["playTTsnds"] = (const char *)settings.playTTsnds;

    #ifdef TC_HAVEMQTT
    json["useMQTT"] = (const char *)settings.useMQTT;
    json["mqttServer"] = (const char *)settings.mqttServer;
    json["mqttUser"] = (const char *)settings.mqttUser;
    json["mqttTopic"] = (const char *)settings.mqttTopic;
    #ifdef EXTERNAL_TIMETRAVEL_OUT
    json["pubMQTT"] = (const char *)settings.pubMQTT;
    #endif
    #endif

    json["shuffle"] = (const char *)settings.shuffle;

    json["CfgOnSD"] = (const char *)settings.CfgOnSD;
    //json["sdFreq"] = (const char *)settings.sdFreq;

    writeJSONCfgFile(json, cfgName, FlashROMode, funcName);
}

bool checkConfigExists()
{
    return FlashROMode ? SD.exists(cfgName) : (haveFS && SPIFFS.exists(cfgName));
}

/*
 *  Helpers for parm copying & checking
 */

static void CopyTextParm(char *setting, const char *json, int setSize)
{
    memset(setting, 0, setSize);
    strncpy(setting, json, setSize - 1);
}

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

    i = atoi(text);

    if(i < lowerLim) {
        sprintf(text, "%d", lowerLim);
        return true;
    } else if(i > upperLim) {
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
        sprintf(text, "%.1f", setDefault);
        return true;
    }

    for(i = 0; i < len; i++) {
        if(text[i] != '.' && text[i] != '-' && (text[i] < '0' || text[i] > '9')) {
            sprintf(text, "%.1f", setDefault);
            return true;
        }
    }

    f = strtof(text, NULL);

    if(f < lowerLim) {
        sprintf(text, "%.1f", lowerLim);
        return true;
    } else if(f > upperLim) {
        sprintf(text, "%.1f", upperLim);
        return true;
    }

    // Re-do to get rid of formatting errors (eg "0.")
    sprintf(text, "%.1f", f);

    return false;
}

static bool openCfgFileRead(const char *fn, File& f)
{
    bool haveConfigFile = false;
    
    if(configOnSD) {
        if(SD.exists(fn)) {
            haveConfigFile = (f = SD.open(fn, "r"));
        }
    }

    if(!haveConfigFile && haveFS) {
        if(SPIFFS.exists(fn)) {
            haveConfigFile = (f = SPIFFS.open(fn, "r"));
        }
    }

    return haveConfigFile;
}

/*
 *  Load/save Brightness settings
 */

static void updateBriCache()
{
    preSavedDBri = atoi(settings.destTimeBright);
    preSavedPBri = atoi(settings.presTimeBright);
    preSavedLBri = atoi(settings.lastTimeBright); 
}

static bool loadBrightness()
{
    const char *funcName = "loadBrightness";
    bool wd = true;
    File configFile;

    if(!haveFS && !configOnSD) {
        Serial.printf("%s: %s\n", funcName, fsNoAvail);
        return false;
    }

    #ifdef TC_DBG
    Serial.printf("%s: Loading from %s\n", funcName, configOnSD ? "SD" : "flash FS");
    #endif

    if(openCfgFileRead(briCfgName, configFile)) {

        DECLARE_S_JSON(512,json);
        //StaticJsonDocument<512> json;

        if(!readJSONCfgFile(json, configFile, funcName)) {
            wd =  CopyCheckValidNumParm(json["dBri"], settings.destTimeBright, sizeof(settings.destTimeBright), 0, 15, DEF_BRIGHT_DEST);
            wd |= CopyCheckValidNumParm(json["pBri"], settings.presTimeBright, sizeof(settings.presTimeBright), 0, 15, DEF_BRIGHT_PRES);
            wd |= CopyCheckValidNumParm(json["lBri"], settings.lastTimeBright, sizeof(settings.lastTimeBright), 0, 15, DEF_BRIGHT_DEPA);
        } 
        configFile.close();
    }

    if(wd) {
        #ifdef TC_DBG
        Serial.printf("%s: %s\n", funcName, badConfig);
        #endif
        saveBrightness();
    } else {
        updateBriCache();
    }

    return true;
}

void saveBrightness(bool useCache)
{
    const char *funcName = "saveBrightness";
    
    DECLARE_S_JSON(512,json);
    //StaticJsonDocument<512> json;

    if(useCache && 
       (preSavedDBri == atoi(settings.destTimeBright)) &&
       (preSavedPBri == atoi(settings.presTimeBright)) &&
       (preSavedLBri == atoi(settings.presTimeBright))) {
        return;
    }

    if(!haveFS && !configOnSD) {
        Serial.printf("%s: %s\n", funcName, fsNoAvail);
        return;
    }

    json["dBri"] = (const char *)settings.destTimeBright;
    json["pBri"] = (const char *)settings.presTimeBright;
    json["lBri"] = (const char *)settings.lastTimeBright;

    writeJSONCfgFile(json, briCfgName, configOnSD, funcName);

    updateBriCache();
}

/*
 *  autoInterval
 *
 */
static bool loadAutoInterval()
{
    const char *funcName = "loadAutoInterval";
    bool wd = true;
    File configFile;

    if(!haveFS && !configOnSD) {
        Serial.printf("%s: %s\n", funcName, fsNoAvail);
        return false;
    }

    #ifdef TC_DBG
    Serial.printf("%s: Loading from %s\n", funcName, configOnSD ? "SD" : "flash FS");
    #endif

    if(openCfgFileRead(ainCfgName, configFile)) {

        DECLARE_S_JSON(512,json);
        //StaticJsonDocument<512> json;

        if(!readJSONCfgFile(json, configFile, funcName)) {
            wd = CopyCheckValidNumParm(json["ai"], settings.autoRotateTimes, sizeof(settings.autoRotateTimes), 0, 5, DEF_AUTOROTTIMES);
        }
        configFile.close();
    }

    if(wd) {
        #ifdef TC_DBG
        Serial.printf("%s: %s\n", funcName, badConfig);
        #endif
        saveAutoInterval();
    } else {
        autoInterval = (uint8_t)atoi(settings.autoRotateTimes);
        preSavedAI = autoInterval;
    }

    return true;
}

void saveAutoInterval(bool useCache)
{
    const char *funcName = "saveAutoInterval";
    
    DECLARE_S_JSON(512,json);
    //StaticJsonDocument<512> json;

    if(useCache && (preSavedAI == autoInterval)) {
        return;
    }

    if(!haveFS && !configOnSD) {
        Serial.printf("%s: %s\n", funcName, fsNoAvail);
        return;
    }

    sprintf(settings.autoRotateTimes, "%d", autoInterval);

    json["ai"] = (const char *)settings.autoRotateTimes;
    
    writeJSONCfgFile(json, ainCfgName, configOnSD, funcName);

    preSavedAI = autoInterval;
}


/*
 *  Load/save the Volume
 */

bool loadCurVolume()
{
    const char *funcName = "loadCurVolume";
    char temp[6];
    bool writedefault = true;
    File configFile;

    curVolume = DEFAULT_VOLUME;

    if(!haveFS && !configOnSD) {
        Serial.printf("%s: %s\n", funcName, fsNoAvail);
        return false;
    }

    #ifdef TC_DBG
    Serial.printf("%s: Loading from %s\n", funcName, configOnSD ? "SD" : "flash FS");
    #endif

    if(openCfgFileRead(volCfgName, configFile)) {
        DECLARE_S_JSON(512,json);
        //StaticJsonDocument<512> json;
        if(!readJSONCfgFile(json, configFile, funcName)) {
            if(!CopyCheckValidNumParm(json["volume"], temp, sizeof(temp), 0, 255, 255)) {
                int ncv = atoi(temp);
                if((ncv >= 0 && ncv <= 19) || ncv == 255) {
                    curVolume = ncv;
                    writedefault = false;
                }
            }
        } 
        configFile.close();
    }

    if(writedefault) {
        #ifdef TC_DBG
        Serial.printf("%s: %s\n", funcName, badConfig);
        #endif
        saveCurVolume(false);
    } else {
        prevSavedVol = curVolume;
    }

    return true;
}

void saveCurVolume(bool useCache)
{
    const char *funcName = "saveCurVolume";
    char buf[6];
    DECLARE_S_JSON(512,json);
    //StaticJsonDocument<512> json;

    if(useCache && (prevSavedVol == curVolume)) {
        return;
    }

    if(!haveFS && !configOnSD) {
        Serial.printf("%s: %s\n", funcName, fsNoAvail);
        return;
    }

    sprintf(buf, "%d", curVolume);
    json["volume"] = (const char *)buf;

    if(writeJSONCfgFile(json, volCfgName, configOnSD, funcName)) {
        prevSavedVol = curVolume;
    }
}

/*
 *  Load/save the Alarm settings
 */

bool loadAlarm()
{
    const char *funcName = "lAl";
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

    if(openCfgFileRead(almCfgName, configFile)) {

        DECLARE_S_JSON(512,json);
        //StaticJsonDocument<512> json;
        
        if(!readJSONCfgFile(json, configFile, funcName)) {
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
        #ifdef TC_DBG
        Serial.printf("%s: %s\n", funcName, badConfig);
        #endif
        alarmHour = alarmMinute = 255;
        alarmOnOff = false;
        alarmWeekday = 0;
        saveAlarm();
    }

    return true;
}

void saveAlarm()
{
    const char *funcName = "sAl";
    char aooBuf[8];
    char hourBuf[8];
    char minBuf[8];
    DECLARE_S_JSON(512,json);
    //StaticJsonDocument<512> json;

    if(!haveFS && !configOnSD) {
        Serial.printf("%s: %s\n", funcName, fsNoAvail);
        return;
    }

    sprintf(aooBuf, "%d", (alarmWeekday * 16) + (alarmOnOff ? 1 : 0));
    json["alarmonoff"] = (const char *)aooBuf;

    sprintf(hourBuf, "%d", alarmHour);
    sprintf(minBuf, "%d", alarmMinute);

    json["alarmhour"] = (const char *)hourBuf;
    json["alarmmin"] = (const char *)minBuf;

    writeJSONCfgFile(json, almCfgName, configOnSD, funcName);
}

/*
 *  Load/save the Yearly/Monthly Reminder settings
 */

bool loadReminder()
{
    const char *funcName = "lRem";
    bool writedefault = false;
    File configFile;

    if(!haveFS && !configOnSD) {
        Serial.printf("%s: %s\n", funcName, fsNoAvail);
        return false;
    }

    #ifdef TC_DBG
    Serial.printf("%s: Loading from %s\n", funcName, configOnSD ? "SD" : "flash FS");
    #endif

    if(openCfgFileRead(remCfgName, configFile)) {

        DECLARE_S_JSON(512,json);
        //StaticJsonDocument<512> json;
        
        if(!readJSONCfgFile(json, configFile, funcName)) {
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
            #ifdef TC_DBG
            Serial.printf("%s: %s\n", funcName, badConfig);
            #endif
            remMonth = remDay = remHour = remMin = 0;
            deleteReminder();
        }
        
    }

    return true;
}

void saveReminder()
{
    const char *funcName = "sRem";
    char monBuf[8];
    char dayBuf[8];
    char hourBuf[8];
    char minBuf[8];
    DECLARE_S_JSON(512,json);
    //StaticJsonDocument<512> json;

    if(!haveFS && !configOnSD) {
        Serial.printf("%s: %s\n", funcName, fsNoAvail);
        return;
    }

    if(!remMonth && !remDay) {
        deleteReminder();
        return;
    }

    sprintf(monBuf, "%d", remMonth);
    sprintf(dayBuf, "%d", remDay);
    sprintf(hourBuf, "%d", remHour);
    sprintf(minBuf, "%d", remMin);
    
    json["month"] = (const char *)monBuf;
    json["day"] = (const char *)dayBuf;
    json["hour"] = (const char *)hourBuf;
    json["min"] = (const char *)minBuf;

    writeJSONCfgFile(json, remCfgName, configOnSD, funcName);
}

static void deleteReminder()
{
    if(configOnSD) {
        SD.remove(remCfgName);
    } else if(haveFS) {
        SPIFFS.remove(remCfgName);
    }
}

/*
 *  Load/save carMode
 */

static void loadCarMode()
{
    bool haveConfigFile = false;
    File configFile;

    if(!haveFS && !configOnSD)
        return;

    if(openCfgFileRead(cmCfgName, configFile)) {
        DECLARE_S_JSON(512,json);
        //StaticJsonDocument<512> json;
        if(!readJSONCfgFile(json, configFile, "lCM")) {
            if(json["CarMode"]) {
                carMode = (atoi(json["CarMode"]) > 0);
            }
        } 
        configFile.close();
    }
}

void saveCarMode()
{
    char buf[2];
    DECLARE_S_JSON(512,json);
    //StaticJsonDocument<512> json;

    if(!haveFS && !configOnSD)
        return;

    buf[0] = carMode ? '1' : '0';
    buf[1] = 0;
    json["CarMode"] = (const char *)buf;

    writeJSONCfgFile(json, cmCfgName, configOnSD, "sCM");
}

/*
 * Save/load stale present time
 */

#ifdef HAVE_STALE_PRESENT
void loadStaleTime(void *target, bool& currentOn)
{
    bool haveConfigFile = false;
    uint8_t loadBuf[1+(2*sizeof(dateStruct))+1];
    uint16_t sum = 0;
    
    if(!haveFS && !configOnSD)
        return;

    if(configOnSD) {
        haveConfigFile = readFileFromSD(stCfgName, loadBuf, sizeof(loadBuf));
    }
    if(!haveConfigFile && haveFS) {
        haveConfigFile = readFileFromFS(stCfgName, loadBuf, sizeof(loadBuf));
    }

    if(!haveConfigFile) 
        return;

    for(uint8_t i = 0; i < 1+(2*sizeof(dateStruct)); i++) {
        sum += (loadBuf[i] ^ 0x55);
    }
    if(((sum & 0xff) == loadBuf[1+(2*sizeof(dateStruct))])) {
        currentOn = loadBuf[0] ? true : false;
        memcpy(target, (void *)&loadBuf[1], 2*sizeof(dateStruct));
    }

    return;
}

void saveStaleTime(void *source, bool currentOn)
{
    uint8_t savBuf[1+(2*sizeof(dateStruct))+1];
    uint16_t sum = 0;

    savBuf[0] = currentOn;
    memcpy((void *)&savBuf[1], source, 2*sizeof(dateStruct));
    for(uint8_t i = 0; i < 1+(2*sizeof(dateStruct)); i++) {
        sum += (savBuf[i] ^ 0x55);
    }
    savBuf[1+(2*sizeof(dateStruct))] = sum & 0xff;
    
    if(configOnSD) {
        writeFileToSD(stCfgName, savBuf, sizeof(savBuf));
    } else if(haveFS) {
        writeFileToFS(stCfgName, savBuf, sizeof(savBuf));
    }
}
#endif

/*
 *  Load/save lineOut
 */

#ifdef TC_HAVELINEOUT
void loadLineOut()
{
    bool haveConfigFile = false;
    File configFile;

    if(!haveFS && !configOnSD)
        return;

    if(!haveLineOut)
        return;

    if(openCfgFileRead(loCfgName, configFile)) {
        DECLARE_S_JSON(512,json);
        //StaticJsonDocument<512> json;
        if(!readJSONCfgFile(json, configFile, "lLO")) {
            if(json["LineOut"]) {
                useLineOut = (atoi(json["LineOut"]) > 0);
            }
        } 
        configFile.close();
    }
}

void saveLineOut()
{
    char buf[2];
    DECLARE_S_JSON(512,json);
    //StaticJsonDocument<512> json;

    if(!haveFS && !configOnSD)
        return;

    if(!haveLineOut)
        return;

    buf[0] = useLineOut ? '1' : '0';
    buf[1] = 0;
    json["LineOut"] = (const char *)buf;

    writeJSONCfgFile(json, loCfgName, configOnSD, "sLO");
}
#endif

/*
 *  Load/save remoteAllowed
 */

#ifdef TC_HAVE_REMOTE
static void loadRemoteAllowed()
{
    bool haveConfigFile = false;
    File configFile;

    if(!haveFS && !configOnSD)
        return;

    if(openCfgFileRead(raCfgName, configFile)) {
        DECLARE_S_JSON(512,json);
        //StaticJsonDocument<512> json;
        if(!readJSONCfgFile(json, configFile, "lRA")) {
            if(json["Remote"]) {
                remoteAllowed = (atoi(json["Remote"]) > 0);
            }
        } 
        configFile.close();
    }
}

void saveRemoteAllowed()
{
    char buf[2];
    DECLARE_S_JSON(512,json);
    //StaticJsonDocument<512> json;

    if(!haveFS && !configOnSD)
        return;

    buf[0] = remoteAllowed ? '1' : '0';
    buf[1] = 0;
    json["Remote"] = (const char *)buf;

    writeJSONCfgFile(json, raCfgName, configOnSD, "sRA");
}
#endif

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
            DECLARE_S_JSON(512,json);
            //StaticJsonDocument<512> json;
            if(!readJSONCfgFile(json, configFile, "lMF")) {
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
    const char *funcName = "sMF";
    DECLARE_S_JSON(512,json);
    //StaticJsonDocument<512> json;
    char buf[4];

    if(!haveSD)
        return;

    sprintf(buf, "%1d", musFolderNum);
    json["folder"] = (const char *)buf;

    writeJSONCfgFile(json, musCfgName, true, funcName);
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

            DECLARE_S_JSON(512,json);
            //StaticJsonDocument<512> json;
            DeserializationError error = readJSONCfgFile(json, configFile, "lIp");

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

        #ifdef TC_DBG
        Serial.println(F("loadIpSettings: IP settings invalid; deleting file"));
        #endif
        
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
    DECLARE_S_JSON(512,json);
    //StaticJsonDocument<512> json;

    if(!haveFS && !FlashROMode)
        return;

    if(strlen(ipsettings.ip) == 0)
        return;
    
    json["IpAddress"] = (const char *)ipsettings.ip;
    json["Gateway"]   = (const char *)ipsettings.gateway;
    json["Netmask"]   = (const char *)ipsettings.netmask;
    json["DNS"]       = (const char *)ipsettings.dns;

    writeJSONCfgFile(json, ipCfgName, FlashROMode, "wIp");
}

void deleteIpSettings()
{
    #ifdef TC_DBG
    Serial.println(F("deleteIpSettings: Deleting ip config"));
    #endif

    if(FlashROMode) {
        SD.remove(ipCfgName);
    } else if(haveFS) {
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

static uint32_t getuint32(uint8_t *buf)
{
    uint32_t t = 0;
    for(int i = 3; i >= 0; i--) {
      t <<= 8;
      t += buf[i];
    }
    return t;
}

bool check_if_default_audio_present()
{
    uint8_t dbuf[16]; 
    File file;
    size_t ts;
    int i;

    ic = false;

    if(!haveSD)
        return false;

    if(SD.exists(CONFN)) {
        if(file = SD.open(CONFN, FILE_READ)) {
            ts = file.size();
            file.read(dbuf, 14);
            file.close();
            if((!memcmp(dbuf, CONID, 4))             && 
               ((*(dbuf+4) & 0x7f) == AC_FMTV)       &&
               (!memcmp(dbuf+5, SND_REQ_VERSION, 4)) &&
               (*(dbuf+9) == (10+NUM_AUDIOFILES+1))  &&
               (getuint32(dbuf+10) == soa)           &&
               (ts > soa + AC_OHSZ)) {
                ic = true;
                if(!(*(dbuf+4) & 0x80)) r=f;
            }
        }
    }

    return ic;
}

// Returns false if copy failed because of a write error (which 
//    might be cured by a reformat of the FlashFS)
// Returns true if ok or source error (file missing, read error)
// Sets delIDfile to true if copy fully succeeded
bool copy_audio_files(bool& delIDfile)
{
    int i, haveErr = 0, haveWriteErr = 0;
    char dtmfBuf[] = "/Dtmf-0.mp3\0";

    if(!allowCPA) {
        delIDfile = false;
        return true;
    }

    start_file_copy();

    for(i = 0; i < 10; i++) {
        if(SPIFFS.exists(dtmfBuf)) {
            SPIFFS.remove(dtmfBuf);
            #ifdef TC_DBG
            Serial.printf("Removed %s\n", dtmfBuf);
            #endif
            dtmfBuf[6]++;
        } else {
            break;
        }
    }

    if(ic) {
        File sfile;
        if(sfile = SD.open(CONFN, FILE_READ)) {
            sfile.seek(14);
            for(i = 0; i < NUM_AUDIOFILES+10+1; i++) {
               cfc(sfile, haveErr, haveWriteErr);
               if(haveErr) break;
            }
            sfile.close();
        } else {
            haveErr++;
        }
    } else {
        haveErr++;
    }

    file_copy_done(haveErr);

    delIDfile = (haveErr == 0);

    return (haveWriteErr == 0);
}

static bool dfile_open(File &file, const char *fn, const char *md)
{
    if(FlashROMode) {
        return (file = SD.open(fn, md));
    }
    return (file = SPIFFS.open(fn, md));
}

static void cfc(File& sfile, int& haveErr, int& haveWriteErr)
{
    const char *funcName = "cfc";
    uint8_t buf1[1+32+4];
    uint8_t buf2[1024];
    File dfile;

    buf1[0] = '/';
    sfile.read(buf1 + 1, 32+4);   
    if((dfile_open(dfile, (const char *)(*r)(buf1 + 1, soa, 32) - 1, FILE_WRITE))) {
        uint32_t s = getuint32(buf1 + 1 + 32), t = 1024;
        #ifdef TC_DBG
        Serial.printf("%s: Opened destination file: %s, length %d\n", funcName, (const char *)buf1, s);
        #endif
        while(s > 0) {
            t = (s < t) ? s : t;
            if(sfile.read(buf2, t) != t) {
                haveErr++;
                break;
            }
            if(dfile.write((*r)(buf2, soa, t), t) != t) {
                haveErr++;
                haveWriteErr++;
                break;
            }
            s -= t;
            file_copy_progress();
        }
    } else {
        haveErr++;
        haveWriteErr++;
        Serial.printf("%s: Error opening destination file: %s\n", funcName, buf1);
    }
}

static bool audio_files_present()
{
    File file;
    uint8_t buf[4];
    const char *fn = "/VER";

    if(FlashROMode) {
        if(!(file = SD.open(fn, FILE_READ)))
            return false;
    } else {
        // No SD, no FS - don't even bother....
        if(!haveFS)
            return true;
        if(!SPIFFS.exists(fn))
            return false;
        if(!(file = SPIFFS.open(fn, FILE_READ)))
            return false;
    }

    file.read(buf, 4);
    file.close();

    return (!memcmp(buf, SND_REQ_VERSION, 4));
}

void delete_ID_file()
{
    if(haveSD && ic) {
        SD.remove(CONFND);
        SD.rename(CONFN, CONFND);
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

/* Copy secondary settings from/to SD if user
 * changed "save to SD"-option in CP
 * Esp is rebooted afterwards!
 */
void copySettings()
{       
    if(!haveSD || !haveFS) 
        return;
    
    configOnSD = !configOnSD;

    if(configOnSD || !FlashROMode) {
        #ifdef TC_DBG
        Serial.println(F("copySettings: Copying secondary settings to other medium"));
        #endif
        saveBrightness(false);
        saveAutoInterval(false);
        saveCurVolume(false);
        saveAlarm();
        saveReminder();
        saveCarMode();
        #ifdef HAVE_STALE_PRESENT
        if(stalePresent) {
            saveStaleTime((void *)&stalePresentTime[0], stalePresent);
        }
        #endif
        saveDisplayData();
    }

    configOnSD = !configOnSD;
    setupDisplayConfigOnSD();
}

/*
 * Re-write secondary settings
 * Used during audio file installation when flash FS needs
 * to be re-formatted.
 * Is never called in FlashROmode.
 * Esp is rebooted afterwards!
 */
void rewriteSecondarySettings()
{
    bool oldconfigOnSD = configOnSD;
    
    #ifdef TC_DBG
    Serial.println("Re-writing secondary settings");
    #endif
    
    writeIpSettings();

    // Create current secondary settings on flash FS
    // regardless of SD-option
    configOnSD = false;

    saveBrightness(false);
    saveAutoInterval(false);
    saveCurVolume(false);
    saveAlarm();
    saveReminder();
    saveCarMode();
    #ifdef HAVE_STALE_PRESENT
    if(stalePresent) {
        saveStaleTime((void *)&stalePresentTime[0], stalePresent);
    }
    #endif
    saveDisplayData();
    
    configOnSD = oldconfigOnSD;
    setupDisplayConfigOnSD();

    // Music Folder Number is always on SD only
}

static void setupDisplayConfigOnSD()
{
    destinationTime._configOnSD = configOnSD;
    presentTime._configOnSD = configOnSD;
    departedTime._configOnSD = configOnSD;
}

static void saveDisplayData()
{
    if(timetravelPersistent) {
        setupDisplayConfigOnSD();
        // Save with new _configOnSD setting
        presentTime.save(true);
    } else {
        // Load with old _configOnSD setting
        destinationTime.load();
        departedTime.load();
        // Switch _configOnSD setting
        setupDisplayConfigOnSD();
        // Save with new _configOnSD setting
    }
    destinationTime.save(true);
    departedTime.save(true);
}

/*
 * Helpers for JSON config files
 */
static DeserializationError readJSONCfgFile(JsonDocument& json, File& configFile, const char *funcName)
{
    const char *buf = NULL;
    size_t bufSize = configFile.size();
    DeserializationError ret;

    if(!(buf = (const char *)malloc(bufSize + 1))) {
        Serial.printf("%s: Buffer allocation failed (%d)\n", funcName, bufSize);
        return DeserializationError::NoMemory;
    }

    memset((void *)buf, 0, bufSize + 1);

    configFile.read((uint8_t *)buf, bufSize);

    #ifdef TC_DBG
    Serial.println(buf);
    #endif
    
    ret = deserializeJson(json, buf);

    free((void *)buf);

    return ret;
}

static bool writeJSONCfgFile(const JsonDocument& json, const char *fn, bool useSD, const char *funcName)
{
    char *buf;
    size_t bufSize = measureJson(json);
    bool success = false;

    if(!(buf = (char *)malloc(bufSize + 1))) {
        Serial.printf("%s: Buffer allocation failed (%d)\n", funcName, bufSize);
        return false;
    }

    memset(buf, 0, bufSize + 1);
    serializeJson(json, buf, bufSize);

    #ifdef TC_DBG
    Serial.printf("Writing %s to %s, %d bytes\n", fn, useSD ? "SD" : "FS", bufSize);
    Serial.println((const char *)buf);
    #endif

    if(useSD) {
        success = writeFileToSD(fn, (uint8_t *)buf, (int)bufSize);
    } else {
        success = writeFileToFS(fn, (uint8_t *)buf, (int)bufSize);
    }

    free(buf);

    if(!success) {
        Serial.printf("%s: %s\n", funcName, failFileWrite);
    }

    return success;
}

/*
 * Generic file readers/writes for external
 */
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

bool openACFile(File& file)
{
    if(haveSD) {
        if(file = SD.open(CONFN, FILE_WRITE)) {
            return true;
        }
    }

    return false;
}

size_t writeACFile(File& file, uint8_t *buf, size_t len)
{
    return file.write(buf, len);
}

void closeACFile(File& file)
{
    file.close();
}

void removeACFile()
{
    if(haveSD) {
        SD.remove(CONFN);
    }
}
