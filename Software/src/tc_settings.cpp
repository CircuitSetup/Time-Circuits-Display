/*
 * -------------------------------------------------------------------
 * CircuitSetup.us Time Circuits Display
 * (C) 2021-2022 John deGlavina https://circuitsetup.us
 * (C) 2022-2026 Thomas Winischhofer (A10001986)
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

#include <Update.h>

#include "tc_settings.h"
#include "tc_menus.h"
#include "tc_audio.h"
#include "tc_time.h"
#include "tc_wifi.h"
#include "clockdisplay.h"

// Size of main config JSON
// Needs to be adapted when config grows
#define JSON_SIZE 2600
#if ARDUINOJSON_VERSION_MAJOR >= 7
#define DECLARE_S_JSON(x,n) JsonDocument n;
#define DECLARE_D_JSON(x,n) JsonDocument n;
#else
#define DECLARE_S_JSON(x,n) StaticJsonDocument<x> n;
#define DECLARE_D_JSON(x,n) DynamicJsonDocument n(x);
#endif 

#define NUM_AUDIOFILES 24
#define AC_FMTV 2
#define AC_OHSZ (14 + ((10+NUM_AUDIOFILES+1)*(32+4)))
#ifdef TWSOUND
#define SND_NON_ALIEN "CS"
#define SND_REQ_VERSION "TW05"
#define AC_TS 1210586
#else
#define SND_NON_ALIEN "TW"
#define SND_REQ_VERSION "CS05"
#define AC_TS 1215299
#endif

static const char *CONFN  = "/TCDA.bin";
static const char *CONFND = "/TCDA.old";
static const char *CONID  = "TCDA";
static uint32_t   soa = AC_TS;
static bool       ic = false;
static uint8_t*   f(uint8_t *d, uint32_t m, int y) { return d; }
static char       *uploadFileNames[MAX_SIM_UPLOADS] = { NULL };
static char       *uploadRealFileNames[MAX_SIM_UPLOADS] = { NULL };

static const char *cfgName    = "/config.json";     // Main config (flash)
static const char *ipCfgName  = "/ipconfig.json";   // IP config (flash)
static const char *briCfgName = "/tcdbricfg.json";  // Brightness (flash/SD)
static const char *ainCfgName = "/tcdaicfg.json";   // Beep+Auto-interval (flash/SD)
static const char *volCfgName = "/tcdvolcfg.json";  // Volume config (flash/SD)
static const char *almCfgName = "/tcdalmcfg.json";  // Alarm config (flash/SD)
static const char *remCfgName = "/tcdremcfg.json";  // Reminder config (flash/SD)
static const char *stCfgName  = "/stconfig";        // Exhib Mode (flash/SD)
static const char *musCfgName = "/tcdmcfg.json";    // Music config (SD)
static const char *cmCfgName  = "/cmconfig.json";   // Carmode (flash/SD)
static const char *loCfgName  = "/loconfig.json";   // LineOut (flash/SD)
#ifdef TC_HAVE_REMOTE
static const char *raCfgName  = "/raconfig.json";   // RemoteAllowed (flash/SD)
#endif
#ifdef SERVOSPEEDO
static const char *scCfgName  = "/scconfig.json";   // Servo correction
#endif
static const char fwfn[]      = "/tcdfw.bin";
static const char fwfnold[]   = "/tcdfw.old";

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

int sspeedopin = 0;
int stachopin = 0;

/* Cache */
static uint8_t  preSavedDBri = 20;
static uint8_t  preSavedPBri = 20;
static uint8_t  preSavedLBri = 20;
static uint8_t  preSavedAI   = 99;
static uint8_t  preSavedBeep = 99;
static uint8_t  prevSavedVol = 254;
static uint8_t* (*r)(uint8_t *, uint32_t, int);

static bool read_settings(File configFile, int cfgReadCount);

static bool CopyTextParm(const char *json, const char *json2, char *setting, int setSize);
static bool CopyCheckValidNumParm(const char *json, const char *json2, char *text, uint8_t psize, int lowerLim, int upperLim, int setDefault);
static bool CopyCheckValidNumParmF(const char *json, const char *json2, char *text, uint8_t psize, float lowerLim, float upperLim, float setDefault);
static bool checkValidNumParm(char *text, int lowerLim, int upperLim, int setDefault);
static bool checkValidNumParmF(char *text, float lowerLim, float upperLim, float setDefault);

static bool loadBrightness();
static bool loadBeepAutoInterval();

static void deleteReminder();
static void loadCarMode();

#ifdef TC_HAVE_REMOTE
static void loadRemoteAllowed();
#endif

static void setupDisplayConfigOnSD();
static void saveDisplayData();

static void cfc(File& sfile, int& haveErr, int& haveWriteErr);
static bool audio_files_present(int& alienVER);

static bool CopyIPParm(const char *json, char *text, uint8_t psize);

static DeserializationError readJSONCfgFile(JsonDocument& json, File& configFile);
static bool writeJSONCfgFile(const JsonDocument& json, const char *fn, bool useSD);

extern void start_file_copy();
extern void file_copy_progress();
extern void file_copy_done(int err);

static void firmware_update();

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
    #ifdef TC_DBG
    const char *funcName = "settings_setup";
    #endif
    bool writedefault = false;
    bool SDres = false;
    int alienVER = -1;
    int cfgReadCount = 0;

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
        Serial.print("failed, formatting... ");
        #endif

        haveFS = formatFlashFS(true);

    }

    if(haveFS) {

        // Remove sound files that no longer should exist
        char dtmfBuf[] = "/Dtmf-0.mp3";
        SPIFFS.remove("/beep.mp3");
        if(SPIFFS.exists(dtmfBuf)) {
            #ifdef TC_DBG
            Serial.println("Removing old audio files");
            #endif
            for(int i = 0; i < 10; i++) {
                dtmfBuf[6] = i + '0';
                SPIFFS.remove(dtmfBuf);
            }
        }
      
        #ifdef TC_DBG
        Serial.println("ok, loading settings");
        Serial.printf("FlashFS: %d total, %d used\n", SPIFFS.totalBytes(), SPIFFS.usedBytes());
        #endif
        
        if(SPIFFS.exists(cfgName)) {
            File configFile = SPIFFS.open(cfgName, "r");
            if(configFile) {
                writedefault = read_settings(configFile, cfgReadCount);
                cfgReadCount++;
                configFile.close();
            } else {
                writedefault = true;
            }
        } else {
            writedefault = true;
        }

        // Write new config file after mounting SD and determining FlashROMode

    } else {

        Serial.println("failed.\n*** Mounting flash FS failed. Using SD (if available)");

    }
    
    // Set up SD card
    SPI.begin(SPI_SCK_PIN, SPI_MISO_PIN, SPI_MOSI_PIN);

    haveSD = false;

    //uint32_t sdfreq = (settings.sdFreq[0] == '0') ? 16000000 : 4000000;
    uint32_t sdfreq = 16000000;
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
        Serial.println("ok");
        #endif

        uint8_t cardType = SD.cardType();
       
        #ifdef TC_DBG
        const char *sdTypes[5] = { "No card", "MMC", "SD", "SDHC", "unknown (SD not usable)" };
        Serial.printf("SD card type: %s\n", sdTypes[cardType > 4 ? 4 : cardType]);
        #endif

        haveSD = ((cardType != CARD_NONE) && (cardType != CARD_UNKNOWN));

    } else {

        Serial.println("No SD card found");

    }

    if(haveSD) {

        firmware_update();

        if(SD.exists("/TCD_FLASH_RO") || !haveFS) {
            bool writedefault2 = false;
            FlashROMode = true;
            Serial.println("Flash-RO mode: Using SD only.");
            if(SD.exists(cfgName)) {
                File configFile = SD.open(cfgName, "r");
                if(configFile) {
                    writedefault2 = read_settings(configFile, cfgReadCount);
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

    // Check if (current) audio data is installed
    haveAudioFiles = audio_files_present(alienVER);

    // Re-format flash FS if either alien VER found, or no VER exists
    // and remaining storage is not enough for our audio files.
    // (Reason: LittleFS crashes when flash FS is full.)
    if(!haveAudioFiles && haveFS && !FlashROMode) {
        if((alienVER > 0) || (alienVER < 0 && (SPIFFS.totalBytes() - SPIFFS.usedBytes() < soa + 16384))) {
            #ifdef TCD_DBG
            Serial.printf("Reformatting. Alien VER: %d, space %d", alienVER, SPIFFS.totalBytes() - SPIFFS.usedBytes());
            #endif
            writedefault = true;
            formatFlashFS(true);
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
    configOnSD = (haveSD && (evalBool(settings.CfgOnSD) || FlashROMode));

    // Setup save target for display objects
    setupDisplayConfigOnSD();

    // Load display brightness config
    loadBrightness();

    // Load (copy) autoInterval ("time cycling interval")
    loadBeepAutoInterval();

    // Load carMode setting
    loadCarMode();

    // Load RemoteAllowed/RemoteKPAllowed settings
    #ifdef TC_HAVE_REMOTE
    loadRemoteAllowed();
    #endif

    #ifdef SERVOSPEEDO
    ttinpin = atoi(settings.ttinpin);
    ttoutpin = atoi(settings.ttoutpin);
    if(ttinpin == 1) {
        sspeedopin = EXTERNAL_TIMETRAVEL_IN_PIN;
    } else if(ttinpin == 2) {
        stachopin = EXTERNAL_TIMETRAVEL_IN_PIN;
    } else {
        ttinpin = 0;
    }
    if(ttoutpin == 1 && !sspeedopin) {
        sspeedopin = EXTERNAL_TIMETRAVEL_OUT_PIN;
    } else if(ttoutpin == 2 && !stachopin) {
        stachopin = EXTERNAL_TIMETRAVEL_OUT_PIN;
    } else {
        ttoutpin = 0;
    }
    #endif
    
    // Check if SD contains the default sound files
    if((r = e) && haveSD && (FlashROMode || haveFS)) {
        allowCPA = check_if_default_audio_present();
    }

    for(int i = 0; i < MAX_SIM_UPLOADS; i++) {
        uploadFileNames[i] = uploadRealFileNames[i] = NULL;
    }
    
    // Allow user to delete static IP data and temporarily clear
    // AP password by holding ENTER while booting
    // (10 secs timeout to wait for button-release to allow
    // to run fw without control board attached)
    if(digitalRead(ENTER_BUTTON_PIN)) {

        unsigned long mnow = millis();

        Serial.println("Deleting ip config; temporarily clearing AP mode WiFi password");

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
        Serial.println("Unmounted Flash FS");
        #endif
        haveFS = false;
    }
    if(haveSD) {
        SD.end();
        #ifdef TC_DBG
        Serial.println("Unmounted SD card");
        #endif
        haveSD = false;
    }
}

static bool read_settings(File configFile, int cfgReadCount)
{
    #ifdef TC_DBG
    const char *funcName = "read_settings";
    #endif
    bool wd = false;
    size_t jsonSize = 0;
    DECLARE_D_JSON(JSON_SIZE,json);

    DeserializationError error = readJSONCfgFile(json, configFile);

    #if ARDUINOJSON_VERSION_MAJOR < 7
    jsonSize = json.memoryUsage();
    if(jsonSize > JSON_SIZE) {
        Serial.printf("ERROR: Config too large (%d vs %d), memory corrupted.\n", jsonSize, JSON_SIZE);
    }
    
    #ifdef TC_DBG
    if(jsonSize > JSON_SIZE - 256) {
          Serial.printf("%s: WARNING: JSON_SIZE needs to be adapted **************\n", funcName);
    }
    Serial.printf("%s: Size of document: %d (JSON_SIZE %d)\n", funcName, jsonSize, JSON_SIZE);
    #endif
    #endif

    if(!error) {

        // WiFi Configuration

        if(!cfgReadCount) {
            memset(settings.ssid, 0, sizeof(settings.ssid));
            memset(settings.pass, 0, sizeof(settings.pass));
        }

        if(json["ssid"]) {
            memset(settings.ssid, 0, sizeof(settings.ssid));
            memset(settings.pass, 0, sizeof(settings.pass));
            strncpy(settings.ssid, json["ssid"], sizeof(settings.ssid) - 1);
            if(json["pass"]) {
                strncpy(settings.pass, json["pass"], sizeof(settings.pass) - 1);
            }
        } else {
            if(!cfgReadCount) {
                // Set a marker for "no ssid tag in config file", ie read from NVS.
                settings.ssid[1] = 'X';
            } else if(settings.ssid[0] || settings.ssid[1] != 'X') {
                // FlashRO: If flash-config didn't set the marker, write new file 
                // with ssid/pass from flash-config
                wd = true;
            }
        }

        wd |= CopyTextParm(json["hn"], json["hostName"], settings.hostName, sizeof(settings.hostName));
        
        wd |= CopyCheckValidNumParm(json["wCR"], json["wifiConRetries"], settings.wifiConRetries, sizeof(settings.wifiConRetries), 1, 10, DEF_WIFI_RETRY);
        wd |= CopyCheckValidNumParm(json["wCT"], json["wifiConTimeout"], settings.wifiConTimeout, sizeof(settings.wifiConTimeout), 7, 25, DEF_WIFI_TIMEOUT);
        wd |= CopyCheckValidNumParm(json["wPR"], json["wifiPRetry"], settings.wifiPRetry, sizeof(settings.wifiPRetry), 0, 1, DEF_WIFI_PRETRY);
        wd |= CopyCheckValidNumParm(json["wOD"], json["wifiOffDelay"], settings.wifiOffDelay, sizeof(settings.wifiOffDelay), 0, 99, DEF_WIFI_OFFDELAY);

        wd |= CopyTextParm(json["sID"], json["systemID"], settings.systemID, sizeof(settings.systemID));
        wd |= CopyTextParm(json["appw"], NULL, settings.appw, sizeof(settings.appw));
        wd |= CopyCheckValidNumParm(json["apch"], NULL, settings.apChnl, sizeof(settings.apChnl), 0, 11, DEF_AP_CHANNEL);
        wd |= CopyCheckValidNumParm(json["wAOD"], json["wifiAPOffDelay"], settings.wifiAPOffDelay, sizeof(settings.wifiAPOffDelay), 0, 99, DEF_WIFI_APOFFDELAY);

        // Settings

        wd |= CopyCheckValidNumParm(json["pI"], json["playIntro"], settings.playIntro, sizeof(settings.playIntro), 0, 1, DEF_PLAY_INTRO);
        // Beep now in separate file, so it missing is ok
        CopyCheckValidNumParm(json["bep"], json["beep"], settings.beep, sizeof(settings.beep), 0, 3, DEF_BEEP);
        // Time cycling saved in separate file
        wd |= CopyCheckValidNumParm(json["sARA"], NULL, settings.autoRotAnim, sizeof(settings.autoRotAnim), 0, 1, 1);
        wd |= CopyCheckValidNumParm(json["skpTTA"], NULL, settings.skipTTAnim, sizeof(settings.skipTTAnim), 0, 1, DEF_SKIP_TTANIM);
        #ifndef IS_ACAR_DISPLAY
        wd |= CopyCheckValidNumParm(json["p3an"], NULL, settings.p3anim, sizeof(settings.p3anim), 0, 1, DEF_P3ANIM);
        #endif
        wd |= CopyCheckValidNumParm(json["pTTs"], json["playTTsnds"], settings.playTTsnds, sizeof(settings.playTTsnds), 0, 1, DEF_PLAY_TT_SND);
        wd |= CopyCheckValidNumParm(json["alRTC"], json["alarmRTC"], settings.alarmRTC, sizeof(settings.alarmRTC), 0, 1, DEF_ALARM_RTC);
        wd |= CopyCheckValidNumParm(json["md24"], json["mode24"], settings.mode24, sizeof(settings.mode24), 0, 1, DEF_MODE24);
        
        wd |= CopyTextParm(json["tZ"], json["timeZone"], settings.timeZone, sizeof(settings.timeZone));
        wd |= CopyTextParm(json["ntpS"], json["ntpServer"], settings.ntpServer, sizeof(settings.ntpServer));

        wd |= CopyTextParm(json["tZDest"], json["timeZoneDest"], settings.timeZoneDest, sizeof(settings.timeZoneDest));
        wd |= CopyTextParm(json["tZDep"], json["timeZoneDep"], settings.timeZoneDep, sizeof(settings.timeZoneDep));
        wd |= CopyTextParm(json["tZNDest"], json["timeZoneNDest"], settings.timeZoneNDest, sizeof(settings.timeZoneNDest));
        wd |= CopyTextParm(json["tZNDep"], json["timeZoneNDep"], settings.timeZoneNDep, sizeof(settings.timeZoneNDep));
        
        wd |= CopyCheckValidNumParm(json["shuf"], json["shuffle"], settings.shuffle, sizeof(settings.shuffle), 0, 1, DEF_SHUFFLE);

        wd |= CopyCheckValidNumParm(json["dtNmOff"], NULL, settings.dtNmOff, sizeof(settings.dtNmOff), 0, 1, DEF_DT_OFF);
        wd |= CopyCheckValidNumParm(json["ptNmOff"], NULL, settings.ptNmOff, sizeof(settings.ptNmOff), 0, 1, DEF_PT_OFF);
        wd |= CopyCheckValidNumParm(json["ltNmOff"], NULL, settings.ltNmOff, sizeof(settings.ltNmOff), 0, 1, DEF_LT_OFF);
        wd |= CopyCheckValidNumParm(json["aNMPre"], json["autoNMPreset"], settings.autoNMPreset, sizeof(settings.autoNMPreset), 0, 10, DEF_AUTONM_PRESET);
        wd |= CopyCheckValidNumParm(json["aNMOn"], json["autoNMOn"], settings.autoNMOn, sizeof(settings.autoNMOn), 0, 23, DEF_AUTONM_ON);
        wd |= CopyCheckValidNumParm(json["aNMOff"], json["autoNMOff"], settings.autoNMOff, sizeof(settings.autoNMOff), 0, 23, DEF_AUTONM_OFF);
        #ifdef TC_HAVELIGHT
        wd |= CopyCheckValidNumParm(json["uLgt"], json["useLight"], settings.useLight, sizeof(settings.useLight), 0, 1, DEF_USE_LIGHT);
        wd |= CopyCheckValidNumParm(json["lxLim"], json["luxLimit"], settings.luxLimit, sizeof(settings.luxLimit), 0, 50000, DEF_LUX_LIMIT);
        #endif

        #ifdef TC_HAVETEMP
        wd |= CopyCheckValidNumParm(json["tmpU"], json["tempUnit"], settings.tempUnit, sizeof(settings.tempUnit), 0, 1, DEF_TEMP_UNIT);
        wd |= CopyCheckValidNumParmF(json["tmpOf"], json["tempOffs"], settings.tempOffs, sizeof(settings.tempOffs), -3.0f, 3.0f, DEF_TEMP_OFFS);
        #endif

        wd |= CopyCheckValidNumParm(json["spT"], json["speedoType"], settings.speedoType, sizeof(settings.speedoType), 0, 99, DEF_SPEEDO_TYPE);
        wd |= CopyCheckValidNumParm(json["spB"], json["speedoBright"], settings.speedoBright, sizeof(settings.speedoBright), 0, 15, DEF_BRIGHT_SPEEDO);
        wd |= CopyCheckValidNumParm(json["spAO"], NULL, settings.speedoAO, sizeof(settings.speedoAO), 0, 1, DEF_SPEEDO_AO);
        wd |= CopyCheckValidNumParm(json["spAF"], json["speedoAF"], settings.speedoAF, sizeof(settings.speedoAF), 0, 1, DEF_SPEEDO_ACCELFIG);
        wd |= CopyCheckValidNumParmF(json["spFc"], json["speedoFact"], settings.speedoFact, sizeof(settings.speedoFact), 0.5f, 5.0f, DEF_SPEEDO_FACT);
        wd |= CopyCheckValidNumParm(json["spP3"], json["speedoL0Spd"], settings.speedoP3, sizeof(settings.speedoP3), 0, 1, DEF_SPEEDO_P3);
        wd |= CopyCheckValidNumParm(json["spd3rd"], NULL, settings.speedo3rdD, sizeof(settings.speedo3rdD), 0, 1, DEF_SPEEDO_3RDD);
        #ifdef TC_HAVEGPS
        wd |= CopyCheckValidNumParm(json["uGPSS"], json["useGPSSpeed"], settings.dispGPSSpeed, sizeof(settings.dispGPSSpeed), 0, 1, DEF_USE_GPS_SPEED);
        wd |= CopyCheckValidNumParm(json["spUR"], json["spdUpdRate"], settings.spdUpdRate, sizeof(settings.spdUpdRate), 0, 3, DEF_SPD_UPD_RATE);
        #endif
        #ifdef TC_HAVETEMP
        wd |= CopyCheckValidNumParm(json["dTmp"], json["dispTemp"], settings.dispTemp, sizeof(settings.dispTemp), 0, 1, DEF_DISP_TEMP);
        wd |= CopyCheckValidNumParm(json["tmpB"], json["tempBright"], settings.tempBright, sizeof(settings.tempBright), 0, 15, DEF_TEMP_BRIGHT);
        wd |= CopyCheckValidNumParm(json["tmpONM"], json["tempOffNM"], settings.tempOffNM, sizeof(settings.tempOffNM), 0, 1, DEF_TEMP_OFF_NM);
        #endif

        wd |= CopyCheckValidNumParm(json["fPwr"], json["fakePwrOn"], settings.fakePwrOn, sizeof(settings.fakePwrOn), 0, 1, DEF_FAKE_PWR);

        wd |= CopyCheckValidNumParm(json["ettDl"], json["ettDelay"], settings.ettDelay, sizeof(settings.ettDelay), 0, ETT_MAX_DEL, DEF_ETT_DELAY);
        //wd |= CopyCheckValidNumParm(json["ettLong"], NULL, settings.ettLong, sizeof(settings.ettLong), 0, 1, DEF_ETT_LONG);
        
        #ifdef TC_HAVEGPS
        wd |= CopyCheckValidNumParm(json["qGPS"], json["quickGPS"], settings.quickGPS, sizeof(settings.quickGPS), 0, 1, DEF_QUICK_GPS);
        #endif
        
        #ifdef TC_HAVEMQTT
        wd |= CopyCheckValidNumParm(json["uMQTT"], json["useMQTT"], settings.useMQTT, sizeof(settings.useMQTT), 0, 1, 0);
        wd |= CopyTextParm(json["mqttS"], json["mqttServer"], settings.mqttServer, sizeof(settings.mqttServer));
        wd |= CopyCheckValidNumParm(json["mqttV"], NULL, settings.mqttVers, sizeof(settings.mqttVers), 0, 1, 0);
        wd |= CopyTextParm(json["mqttU"], json["mqttUser"], settings.mqttUser, sizeof(settings.mqttUser));
        wd |= CopyTextParm(json["mqttT"], json["mqttTopic"], settings.mqttTopic, sizeof(settings.mqttTopic));
        wd |= CopyCheckValidNumParm(json["pMQTT"], json["pubMQTT"], settings.pubMQTT, sizeof(settings.pubMQTT), 0, 1, 0);
        wd |= CopyCheckValidNumParm(json["vMQTT"], NULL, settings.MQTTvarLead, sizeof(settings.MQTTvarLead), 0, 1, DEF_MQTT_VTT);
        wd |= CopyCheckValidNumParm(json["mqP"], NULL, settings.mqttPwr, sizeof(settings.mqttPwr), 0, 1, 0);
        wd |= CopyCheckValidNumParm(json["mqPO"], NULL, settings.mqttPwrOn, sizeof(settings.mqttPwrOn), 0, 1, 0);
        #endif

        wd |= CopyCheckValidNumParm(json["ETTOc"], NULL, settings.ETTOcmd, sizeof(settings.ETTOcmd), 0, 1, DEF_ETTO_CMD);
        wd |= CopyCheckValidNumParm(json["ETTOPU"], NULL, settings.ETTOpus, sizeof(settings.ETTOpus), 0, 1, DEF_ETTO_PUS);
        wd |= CopyCheckValidNumParm(json["uETTO"], json["useETTO"], settings.useETTO, sizeof(settings.useETTO), 0, 1, DEF_USE_ETTO);
        wd |= CopyCheckValidNumParm(json["nETTOL"], json["noETTOLead"], settings.noETTOLead, sizeof(settings.noETTOLead), 0, 1, DEF_NO_ETTO_LEAD);
        wd |= CopyCheckValidNumParm(json["ETTOa"], NULL, settings.ETTOalm, sizeof(settings.ETTOalm), 0, 1, DEF_ETTO_ALM);
        wd |= CopyCheckValidNumParm(json["ETTOAD"], NULL, settings.ETTOAD, sizeof(settings.ETTOAD), 3, 99, DEF_ETTO_ALM_D);
        
        wd |= CopyCheckValidNumParm(json["CoSD"], json["CfgOnSD"], settings.CfgOnSD, sizeof(settings.CfgOnSD), 0, 1, DEF_CFG_ON_SD);
        //wd |= CopyCheckValidNumParm(json["sdFreq"], NULL, settings.sdFreq, sizeof(settings.sdFreq), 0, 1, DEF_SD_FREQ);
        wd |= CopyCheckValidNumParm(json["ttps"], json["timeTrPers"], settings.timesPers, sizeof(settings.timesPers), 0, 1, DEF_TIMES_PERS);

        #ifdef SERVOSPEEDO
        wd |= CopyCheckValidNumParm(json["tin"], NULL, settings.ttinpin, sizeof(settings.ttinpin), 0, 2, 0);
        wd |= CopyCheckValidNumParm(json["tout"], NULL, settings.ttoutpin, sizeof(settings.ttoutpin), 0, 2, 0);
        #endif
        
        wd |= CopyCheckValidNumParm(json["rAPM"], NULL, settings.revAmPm, sizeof(settings.revAmPm), 0, 1, DEF_REVAMPM);
        
    } else {

        wd = true;

    }

    return wd;
}

void write_settings()
{
    #ifdef TC_DBG
    const char *funcName = "write_settings";
    #endif
    DECLARE_D_JSON(JSON_SIZE,json);

    if(!haveFS && !FlashROMode) {
        Serial.printf("%s\n", fsNoAvail);
        return;
    }

    #ifdef TC_DBG
    Serial.printf("%s: Writing config file\n", funcName);
    #endif

    // Write this only if either set, or also present in file read earlier
    if(settings.ssid[0] || settings.ssid[1] != 'X') {
        json["ssid"] = (const char *)settings.ssid;
        json["pass"] = (const char *)settings.pass;
    }

    json["hn"] = (const char *)settings.hostName;
    json["wCR"] = (const char *)settings.wifiConRetries;
    json["wCT"] = (const char *)settings.wifiConTimeout;
    json["wPR"] = (const char *)settings.wifiPRetry;
    json["wOD"] = (const char *)settings.wifiOffDelay;

    json["sID"] = (const char *)settings.systemID;
    json["appw"] = (const char *)settings.appw;
    json["apch"] = (const char *)settings.apChnl;
    json["wAOD"] = (const char *)settings.wifiAPOffDelay;

    json["pI"] = (const char *)settings.playIntro;
    json["sARA"] = (const char *)settings.autoRotAnim;
    json["skpTTA"] = (const char *)settings.skipTTAnim;
    #ifndef IS_ACAR_DISPLAY
    json["p3an"] = (const char *)settings.p3anim;
    #endif
    json["pTTs"] = (const char *)settings.playTTsnds;
    json["alRTC"] = (const char *)settings.alarmRTC;
    json["md24"] = (const char *)settings.mode24;

    json["tZ"] = (const char *)settings.timeZone;
    json["ntpS"] = (const char *)settings.ntpServer;

    json["tZDest"] = (const char *)settings.timeZoneDest;
    json["tZDep"] = (const char *)settings.timeZoneDep;
    json["tZNDest"] = (const char *)settings.timeZoneNDest;
    json["tZNDep"] = (const char *)settings.timeZoneNDep;
    
    json["shuf"] = (const char *)settings.shuffle;

    json["dtNmOff"] = (const char *)settings.dtNmOff;
    json["ptNmOff"] = (const char *)settings.ptNmOff;
    json["ltNmOff"] = (const char *)settings.ltNmOff;
    json["aNMPre"] = (const char *)settings.autoNMPreset;
    json["aNMOn"] = (const char *)settings.autoNMOn;
    json["aNMOff"] = (const char *)settings.autoNMOff;
    #ifdef TC_HAVELIGHT
    json["uLgt"] = (const char *)settings.useLight;
    json["lxLim"] = (const char *)settings.luxLimit;
    #endif
    
    #ifdef TC_HAVETEMP
    json["tmpU"] = (const char *)settings.tempUnit;
    json["tmpOf"] = (const char *)settings.tempOffs;
    #endif

    json["spT"] = (const char *)settings.speedoType;
    json["spB"] = (const char *)settings.speedoBright;
    json["spAO"] = (const char *)settings.speedoAO;
    json["spAF"] = (const char *)settings.speedoAF;
    json["spFc"] = (const char *)settings.speedoFact;
    json["spP3"] = (const char *)settings.speedoP3;
    json["spd3rd"] = (const char *)settings.speedo3rdD;
    #ifdef TC_HAVEGPS
    json["uGPSS"] = (const char *)settings.dispGPSSpeed;
    json["spUR"] = (const char *)settings.spdUpdRate;
    #endif
    #ifdef TC_HAVETEMP
    json["dTmp"] = (const char *)settings.dispTemp;
    json["tmpB"] = (const char *)settings.tempBright;
    json["tmpONM"] = (const char *)settings.tempOffNM;
    #endif   

    json["fPwr"] = (const char *)settings.fakePwrOn;

    json["ettDl"] = (const char *)settings.ettDelay;
    //json["ettLong"] = (const char *)settings.ettLong;
    
    #ifdef TC_HAVEGPS
    json["qGPS"] = (const char *)settings.quickGPS;
    #endif

    #ifdef TC_HAVEMQTT
    json["uMQTT"] = (const char *)settings.useMQTT;
    json["mqttS"] = (const char *)settings.mqttServer;
    json["mqttV"] = (const char *)settings.mqttVers;
    json["mqttU"] = (const char *)settings.mqttUser;
    json["mqttT"] = (const char *)settings.mqttTopic;
    json["pMQTT"] = (const char *)settings.pubMQTT;
    json["vMQTT"] = (const char *)settings.MQTTvarLead;
    json["mqP"] = (const char *)settings.mqttPwr;
    json["mqPO"] = (const char *)settings.mqttPwrOn;
    #endif

    json["ETTOc"] = (const char *)settings.ETTOcmd;
    json["ETTOPU"] = (const char *)settings.ETTOpus;
    json["uETTO"] = (const char *)settings.useETTO;
    json["nETTOL"] = (const char *)settings.noETTOLead;
    json["ETTOa"] = (const char *)settings.ETTOalm;
    json["ETTOAD"] = (const char *)settings.ETTOAD;
            
    json["CoSD"] = (const char *)settings.CfgOnSD;
    //json["sdFreq"] = (const char *)settings.sdFreq;
    json["ttps"] = (const char *)settings.timesPers;

    #ifdef SERVOSPEEDO
    json["tin"] = (const char *)settings.ttinpin;
    json["tout"] = (const char *)settings.ttoutpin;
    #endif
    
    json["rAPM"] = (const char *)settings.revAmPm;

    writeJSONCfgFile(json, cfgName, FlashROMode);
}

bool checkConfigExists()
{
    return FlashROMode ? SD.exists(cfgName) : (haveFS && SPIFFS.exists(cfgName));
}

/*
 *  Helpers for parm copying & checking
 */

static bool CopyTextParm(const char *json, const char *json2, char *setting, int setSize)
{
    if(json || json2) {
        memset(setting, 0, setSize);
        if(json) {
            strncpy(setting, json, setSize - 1);
            return false; // new tag used, ok
        } else {
            strncpy(setting, json2, setSize - 1);
        }
    }

    return true;  // old tag, or not found
}

static bool CopyCheckValidNumParm(const char *json, const char *json2, char *text, uint8_t psize, int lowerLim, int upperLim, int setDefault)
{
    if(json || json2) {
        memset(text, 0, psize);
        if(json) {
            strncpy(text, json, psize-1);
        } else {
            strncpy(text, json2, psize-1);
        }

        return checkValidNumParm(text, lowerLim, upperLim, setDefault);
    } 
       
    return true;
}

static bool CopyCheckValidNumParmF(const char *json, const char *json2, char *text, uint8_t psize, float lowerLim, float upperLim, float setDefault)
{
    if(json || json2) {
        memset(text, 0, psize);
        if(json) {
            strncpy(text, json, psize-1);
        } else {
            strncpy(text, json2, psize-1);
        }

        return checkValidNumParmF(text, lowerLim, upperLim, setDefault);
    } 
       
    return true;
}

static bool checkValidNumParm(char *text, int lowerLim, int upperLim, int setDefault)
{
    int i, len = strlen(text);

    if(!len) {
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

    if(!len) {
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

static inline void setBoolText(char *buf, const bool& myBool)
{
    buf[0] = myBool ? '1' : '0';
    buf[1] = 0;
}

bool evalBool(char *s)
{
    if(*s == '0') return false;
    return true;
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
    #ifdef TC_DBG
    const char *funcName = "loadBrightness";
    #endif
    bool wd = true;
    File configFile;

    if(!haveFS && !configOnSD)
        return false;

    #ifdef TC_DBG
    Serial.printf("%s: Loading from %s\n", funcName, configOnSD ? "SD" : "flash FS");
    #endif

    if(openCfgFileRead(briCfgName, configFile)) {

        DECLARE_S_JSON(512,json);

        if(!readJSONCfgFile(json, configFile)) {
            wd =  CopyCheckValidNumParm(json["dBri"], NULL, settings.destTimeBright, sizeof(settings.destTimeBright), 0, 15, DEF_BRIGHT_DEST);
            wd |= CopyCheckValidNumParm(json["pBri"], NULL, settings.presTimeBright, sizeof(settings.presTimeBright), 0, 15, DEF_BRIGHT_PRES);
            wd |= CopyCheckValidNumParm(json["lBri"], NULL, settings.lastTimeBright, sizeof(settings.lastTimeBright), 0, 15, DEF_BRIGHT_DEPA);
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
    DECLARE_S_JSON(512,json);

    if(useCache && 
       (preSavedDBri == atoi(settings.destTimeBright)) &&
       (preSavedPBri == atoi(settings.presTimeBright)) &&
       (preSavedLBri == atoi(settings.presTimeBright))) {
        return;
    }

    if(!haveFS && !configOnSD)
        return;

    json["dBri"] = (const char *)settings.destTimeBright;
    json["pBri"] = (const char *)settings.presTimeBright;
    json["lBri"] = (const char *)settings.lastTimeBright;

    writeJSONCfgFile(json, briCfgName, configOnSD);

    updateBriCache();
}

/*
 *  Beep & autoInterval
 *
 */
static bool loadBeepAutoInterval()
{
    #ifdef TC_DBG
    const char *funcName = "loadBeepAutoInterval";
    #endif
    bool wd = true;
    File configFile;

    if(!haveFS && !configOnSD)
        return false;

    #ifdef TC_DBG
    Serial.printf("%s: Loading from %s\n", funcName, configOnSD ? "SD" : "flash FS");
    #endif

    if(openCfgFileRead(ainCfgName, configFile)) {

        DECLARE_S_JSON(512,json);

        if(!readJSONCfgFile(json, configFile)) {
            wd = false;
            wd |= CopyCheckValidNumParm(json["ai"], NULL, settings.autoRotateTimes, sizeof(settings.autoRotateTimes), 0, 5, DEF_AUTOROTTIMES);
            wd |= CopyCheckValidNumParm(json["beep"], NULL, settings.beep, sizeof(settings.beep), 0, 3, atoi(settings.beep));
        }
        configFile.close();
    }

    if(wd) {
        #ifdef TC_DBG
        Serial.printf("%s: %s\n", funcName, badConfig);
        #endif
        saveBeepAutoInterval();
    } else {
        autoInterval = (uint8_t)atoi(settings.autoRotateTimes);
        beepMode = (uint8_t)atoi(settings.beep);
        preSavedAI = autoInterval;
        preSavedBeep = beepMode;
    }

    return true;
}

void saveBeepAutoInterval(bool useCache)
{
    DECLARE_S_JSON(512,json);

    if(useCache && 
       (preSavedAI == autoInterval) && 
       (preSavedBeep == beepMode))
        return;

    if(!haveFS && !configOnSD)
        return;

    sprintf(settings.autoRotateTimes, "%d", autoInterval);
    sprintf(settings.beep, "%d", beepMode);

    json["ai"] = (const char *)settings.autoRotateTimes;
    json["beep"] = (const char *)settings.beep;
    
    writeJSONCfgFile(json, ainCfgName, configOnSD);

    preSavedAI = autoInterval;
    preSavedBeep = beepMode;
}

/*
 *  Load/save the Volume
 */

bool loadCurVolume()
{
    #ifdef TC_DBG
    const char *funcName = "loadCurVolume";
    #endif
    char temp[6];
    bool writedefault = true;
    File configFile;

    curVolume = DEFAULT_VOLUME;

    if(!haveFS && !configOnSD)
        return false;

    #ifdef TC_DBG
    Serial.printf("%s: Loading from %s\n", funcName, configOnSD ? "SD" : "flash FS");
    #endif

    if(openCfgFileRead(volCfgName, configFile)) {
        DECLARE_S_JSON(512,json);
        if(!readJSONCfgFile(json, configFile)) {
            if(!CopyCheckValidNumParm(json["volume"], NULL, temp, sizeof(temp), 0, 255, 255)) {
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
    char buf[6];
    DECLARE_S_JSON(512,json);

    if(useCache && (prevSavedVol == curVolume))
        return;

    if(!haveFS && !configOnSD)
        return;

    sprintf(buf, "%d", curVolume);
    json["volume"] = (const char *)buf;

    if(writeJSONCfgFile(json, volCfgName, configOnSD)) {
        prevSavedVol = curVolume;
    }
}

/*
 *  Load/save the Alarm settings
 */

bool loadAlarm()
{
    #ifdef TC_DBG
    const char *funcName = "lAl";
    #endif
    bool writedefault = true;
    File configFile;

    if(!haveFS && !configOnSD)
        return false;

    #ifdef TC_DBG
    Serial.printf("%s: Loading from %s\n", funcName, configOnSD ? "SD" : "flash FS");
    #endif

    if(openCfgFileRead(almCfgName, configFile)) {

        DECLARE_S_JSON(512,json);
        
        if(!readJSONCfgFile(json, configFile)) {
            if(json["alarmonoff"] && json["alarmhour"] && json["alarmmin"]) {
                int aoo = atoi(json["alarmonoff"]);
                alarmHour = atoi(json["alarmhour"]);
                alarmMinute = atoi(json["alarmmin"]);
                alarmOnOff = ((aoo & 0x0f) != 0);
                alarmWeekday = (aoo & 0xf0) >> 4;
                if(alarmWeekday > 9) alarmWeekday = 0;
                if(((alarmHour   == 255) || (alarmHour   <= 23)) &&
                   ((alarmMinute == 255) || (alarmMinute <= 59))) {
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
    char aooBuf[8];
    char hourBuf[8];
    char minBuf[8];
    DECLARE_S_JSON(512,json);

    if(!haveFS && !configOnSD)
        return;

    sprintf(aooBuf, "%d", (alarmWeekday * 16) + (alarmOnOff ? 1 : 0));
    json["alarmonoff"] = (const char *)aooBuf;

    sprintf(hourBuf, "%d", alarmHour);
    sprintf(minBuf, "%d", alarmMinute);

    json["alarmhour"] = (const char *)hourBuf;
    json["alarmmin"] = (const char *)minBuf;

    writeJSONCfgFile(json, almCfgName, configOnSD);
}

/*
 *  Load/save the Yearly/Monthly Reminder settings
 */

bool loadReminder()
{
    #ifdef TC_DBG
    const char *funcName = "lRem";
    #endif
    bool writedefault = false;
    File configFile;

    if(!haveFS && !configOnSD)
        return false;

    #ifdef TC_DBG
    Serial.printf("%s: Loading from %s\n", funcName, configOnSD ? "SD" : "flash FS");
    #endif

    if(openCfgFileRead(remCfgName, configFile)) {

        DECLARE_S_JSON(512,json);
        
        if(!readJSONCfgFile(json, configFile)) {
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
    char monBuf[8];
    char dayBuf[8];
    char hourBuf[8];
    char minBuf[8];
    DECLARE_S_JSON(512,json);

    if(!haveFS && !configOnSD)
        return;

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

    writeJSONCfgFile(json, remCfgName, configOnSD);
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
    File configFile;

    if(!haveFS && !configOnSD)
        return;

    if(openCfgFileRead(cmCfgName, configFile)) {
        DECLARE_S_JSON(512,json);
        if(!readJSONCfgFile(json, configFile)) {
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

    if(!haveFS && !configOnSD)
        return;

    setBoolText(buf, carMode);

    json["CarMode"] = (const char *)buf;

    writeJSONCfgFile(json, cmCfgName, configOnSD);
}

/*
 * Save/load stale present time
 */

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

/*
 *  Load/save lineOut
 */

void loadLineOut()
{
    File configFile;

    if(!haveFS && !configOnSD)
        return;

    if(!haveLineOut)
        return;

    if(openCfgFileRead(loCfgName, configFile)) {
        DECLARE_S_JSON(512,json);
        if(!readJSONCfgFile(json, configFile)) {
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

    if(!haveFS && !configOnSD)
        return;

    if(!haveLineOut)
        return;

    setBoolText(buf, useLineOut);

    json["LineOut"] = (const char *)buf;

    writeJSONCfgFile(json, loCfgName, configOnSD);
}

/*
 *  Load/save remoteAllowed
 */

#ifdef TC_HAVE_REMOTE
static void loadRemoteAllowed()
{
    File configFile;

    if(!haveFS && !configOnSD)
        return;

    if(openCfgFileRead(raCfgName, configFile)) {
        DECLARE_S_JSON(512,json);
        if(!readJSONCfgFile(json, configFile)) {
            if(json["Remote"]) {
                remoteAllowed = (atoi(json["Remote"]) > 0);
            }
            if(json["RemoteKP"]) {
                remoteKPAllowed = (atoi(json["RemoteKP"]) > 0);
            }
        } 
        configFile.close();
    }
}

void saveRemoteAllowed()
{
    char buf[2];
    char buf2[2];
    DECLARE_S_JSON(512,json);

    if(!haveFS && !configOnSD)
        return;

    setBoolText(buf, remoteAllowed);
    setBoolText(buf2, remoteKPAllowed);
    
    json["Remote"] = (const char *)buf;
    json["RemoteKP"] = (const char *)buf2;

    writeJSONCfgFile(json, raCfgName, configOnSD);
}
#endif

#ifdef SERVOSPEEDO
void loadServoCorr(int& scorr, int& tcorr)
{
    bool haveConfigFile = false;
    int8_t loadBuf[3];

    if(configOnSD) {
        haveConfigFile = readFileFromSD(scCfgName, (uint8_t *)loadBuf, sizeof(loadBuf));
    }
    if(!haveConfigFile && haveFS) {
        haveConfigFile = readFileFromFS(scCfgName, (uint8_t *)loadBuf, sizeof(loadBuf));
    }

    if(haveConfigFile) {
        if((loadBuf[0] ^ 0x55) == loadBuf[2]) {
            scorr = loadBuf[0];
            tcorr = loadBuf[1];
        }
    }
}

void saveServoCorr(int scorr, int tcorr)
{
    int8_t savBuf[3];

    savBuf[0] = scorr;
    savBuf[1] = tcorr;
    savBuf[2] = savBuf[0] ^ 0x55;
    
    if(configOnSD) {
        writeFileToSD(scCfgName, (uint8_t *)savBuf, sizeof(savBuf));
    } else if(haveFS) {
        writeFileToFS(scCfgName, (uint8_t *)savBuf, sizeof(savBuf));
    }
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
            if(!readJSONCfgFile(json, configFile)) {
                if(!CopyCheckValidNumParm(json["folder"], NULL, temp, sizeof(temp), 0, 9, 0)) {
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
    DECLARE_S_JSON(512,json);
    char buf[4];

    if(!haveSD)
        return;

    sprintf(buf, "%1d", musFolderNum);
    json["folder"] = (const char *)buf;

    writeJSONCfgFile(json, musCfgName, true);
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

            DeserializationError error = readJSONCfgFile(json, configFile);

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
        Serial.println("loadIpSettings: IP settings invalid; deleting file");
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

    if(!haveFS && !FlashROMode)
        return;

    if(strlen(ipsettings.ip) == 0)
        return;
    
    json["IpAddress"] = (const char *)ipsettings.ip;
    json["Gateway"]   = (const char *)ipsettings.gateway;
    json["Netmask"]   = (const char *)ipsettings.netmask;
    json["DNS"]       = (const char *)ipsettings.dns;

    writeJSONCfgFile(json, ipCfgName, FlashROMode);
}

void deleteIpSettings()
{
    #ifdef TC_DBG
    Serial.println("deleteIpSettings: Deleting ip config");
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
    #ifdef TC_DBG
    const char *funcName = "cfc";
    #endif
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
        Serial.printf("Error opening destination file: %s\n", buf1);
    }
}

static bool audio_files_present(int& alienVER)
{
    File file;
    uint8_t buf[4];
    const char *fn = "/VER";

    // alienVER is -1 if no VER found,
    //              0 if our VER-type found,
    //              1 if alien VER-type found
    alienVER = -1;

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

    if(!FlashROMode) {
        alienVER = (memcmp(buf, SND_REQ_VERSION, 2) && memcmp(buf, SND_NON_ALIEN, 2)) ? 1 : 0;
    }

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

bool formatFlashFS(bool userSignal)
{
    bool ret = false;

    if(userSignal) {
        // Show the user some action
        destinationTime.showTextDirect("WAIT");
    } else {
        #ifdef TCD_DBG
        Serial.println("Formatting flash FS");
        #endif
    }

    SPIFFS.format();
    if(SPIFFS.begin()) ret = true;

    if(userSignal) {
        destinationTime.showTextDirect("");
    }

    return ret;
}

/* Copy secondary settings from/to SD if user
 * changed "save to SD"-option in CP
 * Esp is rebooted afterwards!
 */

static void writeAllSecSettings()
{
    saveBrightness(false);
    saveBeepAutoInterval(false);
    saveCurVolume(false);
    saveAlarm();
    saveReminder();
    saveCarMode();
    if(stalePresent) {
        saveStaleTime((void *)&stalePresentTime[0], stalePresent);
    }
    saveLineOut();
    #ifdef TC_HAVE_REMOTE
    saveRemoteAllowed();
    #endif
    saveDisplayData();
}

void copySettings()
{       
    if(!haveSD || !haveFS) 
        return;
    
    configOnSD = !configOnSD;

    if(configOnSD || !FlashROMode) {
        #ifdef TC_DBG
        Serial.println("copySettings: Copying secondary settings to other medium");
        #endif
        writeAllSecSettings();
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

    writeAllSecSettings();

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
static DeserializationError readJSONCfgFile(JsonDocument& json, File& configFile)
{
    const char *buf = NULL;
    size_t bufSize = configFile.size();
    DeserializationError ret;

    if(!(buf = (const char *)malloc(bufSize + 1))) {
        Serial.printf("rJSON: malloc failed (%d)\n", bufSize);
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

static bool writeJSONCfgFile(const JsonDocument& json, const char *fn, bool useSD)
{
    char *buf;
    size_t bufSize = measureJson(json);
    bool success = false;

    if(!(buf = (char *)malloc(bufSize + 1))) {
        Serial.printf("wJSON: malloc failed (%d) (%s)\n", bufSize, fn);
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
        Serial.printf("wJSON: %s - %s\n", fn, failFileWrite);
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

/*
 * File upload
 */

static char *allocateUploadFileName(const char *fn, int idx)
{
    if(uploadFileNames[idx]) {
        free(uploadFileNames[idx]);
    }
    if(uploadRealFileNames[idx]) {
        free(uploadRealFileNames[idx]);
    }
    uploadFileNames[idx] = uploadRealFileNames[idx] = NULL;

    if(!strlen(fn))
        return NULL;
  
    if(!(uploadFileNames[idx] = (char *)malloc(strlen(fn)+4)))
        return NULL;

    if(!(uploadRealFileNames[idx] = (char *)malloc(strlen(fn)+4))) {
        free(uploadFileNames[idx]);
        uploadFileNames[idx] = NULL;
        return NULL;
    }

    return uploadRealFileNames[idx];
}

bool openUploadFile(String& fn, File& file, int idx, bool haveAC, int& opType, int& errNo)
{
    char *uploadFileName = NULL;
    bool ret = false;
    
    if(haveSD) {

        errNo = 0;
        opType = 0;  // 0=normal, 1=AC, -1=deletion

        if(!(uploadFileName = allocateUploadFileName(fn.c_str(), idx))) {
            errNo = UPL_MEMERR;
            return false;
        }
        strcpy(uploadFileNames[idx], fn.c_str());
        
        uploadFileName[0] = '/';
        uploadFileName[1] = '-';
        uploadFileName[2] = 0;

        if(fn.length() > 4 && fn.endsWith(".mp3")) {

            strcat(uploadFileName, fn.c_str());

            if((strlen(uploadFileName) > 9) &&
               (strstr(uploadFileName, "/-delete-") == uploadFileName)) {

                #ifdef TC_DBG
                char t = uploadFileName[8];
                #endif
                
                uploadFileName[8] = '/';
                
                #ifdef TC_DBG
                Serial.printf("openUploadFile: Deleting %s\n", uploadFileName+8);
                #endif
                
                SD.remove(uploadFileName+8);
                
                #ifdef TC_DBG
                uploadFileName[8] = t;
                #endif
                
                opType = -1;
            }

        } else if(fn.endsWith(".bin")) {

            if(!haveAC) {
                strcat(uploadFileName, CONFN+1);  // Skip '/', already there
                opType = 1;
            } else {
                errNo = UPL_DPLBIN;
                opType = -1;
            }

        } else {

            errNo = UPL_UNKNOWN;
            opType = -1;
            // ret must be false!

        }

        #ifdef TC_DBG
        Serial.printf("openUploadFile: uploadFilename: %s opType %d\n", uploadFileName, opType);
        #endif

        if(opType >= 0) {
            if((file = SD.open(uploadFileName, FILE_WRITE))) {
                ret = true;
            } else {
                errNo = UPL_OPENERR;
            }
        }

    } else {
      
        errNo = UPL_NOSDERR;
        
    }

    return ret;
}

size_t writeACFile(File& file, uint8_t *buf, size_t len)
{
    return file.write(buf, len);
}

void closeACFile(File& file)
{
    file.close();
}

void removeACFile(int idx)
{
    if(haveSD) {
        if(uploadRealFileNames[idx]) {
            #ifdef TC_DBG
            Serial.printf("removeACFile: Deleting %s\n", uploadRealFileNames[idx]);
            #endif
            SD.remove(uploadRealFileNames[idx]);
        }
    }
}

int getUploadFileNameLen(int idx)
{
    if(idx >= MAX_SIM_UPLOADS) return 0; 
    if(!uploadFileNames[idx]) return 0;
    return strlen(uploadFileNames[idx]);
}

char *getUploadFileName(int idx)
{
    if(idx >= MAX_SIM_UPLOADS) return NULL; 
    return uploadFileNames[idx];
}

void freeUploadFileNames()
{
    for(int i = 0; i < MAX_SIM_UPLOADS; i++) {
        if(uploadFileNames[i]) {
            free(uploadFileNames[i]);
            uploadFileNames[i] = NULL;
        }
        if(uploadRealFileNames[i]) {
            free(uploadRealFileNames[i]);
            uploadRealFileNames[i] = NULL;
        }
    }
}

void renameUploadFile(int idx)
{
    char *uploadFileName = uploadRealFileNames[idx];
    
    if(haveSD && uploadFileName) {

        char *t = (char *)malloc(strlen(uploadFileName)+4);
        t[0] = uploadFileName[0];
        t[1] = 0;
        strcat(t, uploadFileName+2);
        
        #ifdef TC_DBG
        Serial.printf("renameUploadFile [1]: Deleting %s\n", t);
        #endif
        
        SD.remove(t);
        
        #ifdef TC_DBG
        Serial.printf("renameUploadFile [2]: Renaming %s to %s\n", uploadFileName, t);
        #endif
        
        SD.rename(uploadFileName, t);

        // Real name is now changed
        strcpy(uploadFileName, t);
        
        free(t);
    }
}

// Emergency firmware update from SD card
static void fw_error_blink(int n)
{
    bool leds = false;

    for(int i = 0; i < n; i++) {
        leds = !leds;
        digitalWrite(WHITE_LED_PIN, leds ? HIGH : LOW);
        digitalWrite(LEDS_PIN, leds ? LOW : HIGH);
        delay(500);
    }
    digitalWrite(WHITE_LED_PIN, LOW);
    digitalWrite(LEDS_PIN, LOW);
}

static void firmware_update()
{
    const char *upderr = "Firmware update error %d\n";
    uint8_t  buf[1024];
    unsigned int lastMillis = millis();
    bool     leds = false;
    size_t   s;

    if(!SD.exists(fwfn))
        return;
    
    File myFile = SD.open(fwfn, FILE_READ);
    
    if(!myFile)
        return;

    pinMode(LEDS_PIN, OUTPUT);
    pinMode(WHITE_LED_PIN, OUTPUT);
    
    if(!Update.begin(UPDATE_SIZE_UNKNOWN)) {
        Serial.printf(upderr, Update.getError());
        fw_error_blink(5);
        return;
    }

    while((s = myFile.read(buf, 1024))) {
        if(Update.write(buf, s) != s) {
            break;
        }
        if(millis() - lastMillis > 1000) {
            leds = !leds;
            digitalWrite(LEDS_PIN, leds ? HIGH : LOW);
            digitalWrite(WHITE_LED_PIN, leds ? HIGH : LOW);
            lastMillis = millis();
        }
    }
    
    if(Update.hasError() || !Update.end(true)) {
        Serial.printf(upderr, Update.getError());
        fw_error_blink(5);
    } 
    myFile.close();
    // Rename/remove in any case, we don't
    // want an update loop hammer our flash
    SD.remove(fwfnold);
    SD.rename(fwfn, fwfnold);
    unmount_fs();
    delay(1000);
    fw_error_blink(0);
    esp_restart();
}    
