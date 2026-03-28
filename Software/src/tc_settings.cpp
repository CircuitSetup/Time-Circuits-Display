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
#define MYNVS SPIFFS
#include <SPIFFS.h>
#else
#define MYNVS LittleFS
#include <LittleFS.h>
#endif
#include <Update.h>

#include "tc_settings.h"
#include "tc_menus.h"
#include "tc_audio.h"
#include "tc_time.h"
#include "tc_wifi.h"

// If defined, old settings files will be used
// and converted if no new settings file is found.
#define SETTINGS_TRANSITION
// Stage 2: Assume new settings are present, but
// still delete obsolete files.
//#define SETTINGS_TRANSITION_2

#ifdef SETTINGS_TRANSITION
#undef SETTINGS_TRANSITION_2
#endif

// Size of main config JSON
// Needs to be adapted when config grows
#define JSON_SIZE 5000
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
#ifdef V_A10001986
#define SND_REQ_VERSION "TW06"
#define AC_TS 1174312
#define SND_NON_ALIEN "CS"
#else
#define SND_REQ_VERSION "CS06"
#define AC_TS 1179025
#define SND_NON_ALIEN "TW"
#endif

static const char *CONFN  = "/TCDA.bin";
static const char *CONFND = "/TCDA.old";
static const char *CONID  = "TCDA";
const char        rspv[]  = SND_REQ_VERSION;
static uint32_t   soa = AC_TS;
static bool       ic = false;
static uint8_t*   f(uint8_t *d, uint32_t m, int y) { return d; }
static char       *uploadFileNames[MAX_SIM_UPLOADS] = { NULL };
static char       *uploadRealFileNames[MAX_SIM_UPLOADS] = { NULL };

// Secondary settings
// Do not change or insert new values, this
// struct is saved as such. Append new stuff.
static struct [[gnu::packed]] {
    uint8_t brightness[3]   = { DEF_BRIGHT_DEST, DEF_BRIGHT_PRES, DEF_BRIGHT_DEPA };
    uint8_t autoInterval    = DEF_AUTOROTTIMES;
    uint8_t beepMode        = DEF_BEEP;
    uint8_t curVolume       = DEFAULT_VOLUME;
    uint8_t alarmOnOff      = DEF_ALARM_ONOFF;
    uint8_t alarmHour       = DEF_ALARM_HOUR;
    uint8_t alarmMinute     = DEF_ALARM_MINUTE;
    uint8_t alarmWeekday    = DEF_ALARM_WD;
    uint8_t remMonth        = DEF_REM_MONTH;
    uint8_t remDay          = DEF_REM_DAY;
    uint8_t remHour         = DEF_REM_HOUR;
    uint8_t remMin          = DEF_REM_MIN;
    uint8_t carMode         = 0;
    uint8_t useLineOut      = 0;
    uint8_t remoteAllowed   = 0;
    uint8_t remoteKPAllowed = 0;
    int8_t  scorr           = 0;
    int8_t  tcorr           = 0;
    uint8_t exhOnOff        = 0;
    uint8_t showUpdAvail    = 1;
    dateStruct exhDates[2]; // initialized to default in time_boot()
    uint8_t updateV         = 0;
    uint8_t updateR         = 0;
} secSettings;

// Tertiary settings (SD only)
// Do not change or insert new values, this
// struct is saved as such. Append new stuff.
static struct [[gnu::packed]] {
    uint8_t musFolderNum    = 0;
    uint8_t mpShuffle       = 0;
    uint8_t bootMode        = 0;
} terSettings;

static int      secSetValidBytes = 0;
static uint32_t secSettingsHash  = 0;
static bool     haveSecSettings  = false;
static int      terSetValidBytes = 0;
static uint32_t terSettingsHash  = 0;
static bool     haveTerSettings  = false;

// ClockState
// Do not change or insert new values, this
// struct is saved as such. Append new stuff.
static struct {
    uint16_t  lastYear = 0;
    int16_t   yoffs    = 0;
} clockState;

static int clkSValidBytes = 0;

// ClockData
// Do not change or insert new values, this
// struct is saved as such. Append new stuff.
static struct [[gnu::packed]] {
    uint64_t   timeDifference = 0;
    dateStruct dDate          = { 0, 0, 0, 0, 0 };
    dateStruct lDate          = { 0, 0, 0, 0, 0 };
    uint8_t    timeDiffUp     = 0;
} clockData;

static int      clkValidBytes  = 0;
static uint32_t clockHash      = 0;
static bool     haveClockState = false;

uint32_t        mainConfigHash = 0;
static uint32_t ipHash = 0;

static const char *cfgName     = "/config.json"; // Main config (flash)
static const char *ipCfgName   = "/tcdipcfg";    // IP config (flash)
static const char *clkSCfgName = "/tcdcscfg";    // Clock state (flash)
static const char *clkCfgName  = "/tcdckcfg";    // Clock data (flash/SD)
static const char *secCfgName  = "/tcd2cfg";     // Secondary settings (flash/SD)
static const char *terCfgName  = "/tcd3cfg";     // Tertiary settings (SD)

#ifdef SETTINGS_TRANSITION
static const char *ipCfgNameO = "/ipconfig.json";   // IP config (old)
static const char *briCfgName = "/tcdbricfg.json";  // Brightness (old)
static const char *ainCfgName = "/tcdaicfg.json";   // Beep+Auto-interval (old)
static const char *volCfgName = "/tcdvolcfg.json";  // Volume config (old)
static const char *almCfgName = "/tcdalmcfg.json";  // Alarm config (old)
static const char *remCfgName = "/tcdremcfg.json";  // Reminder config (old)
static const char *stCfgName  = "/stconfig";        // Exhib Mode (old)
static const char *cmCfgName  = "/cmconfig.json";   // Carmode (old)
static const char *loCfgName  = "/loconfig,json";   // LineOut (old)
#ifdef TC_HAVE_REMOTE
static const char *raCfgName  = "/raconfig.json";   // RemoteAllowed (old)
#endif
#ifdef SERVOSPEEDO
static const char *scCfgName  = "/scconfig.json";   // Servo correction (old)
#endif
static const char *musCfgName = "/tcdmcfg.json";    // Music Folder
#endif
#ifdef SETTINGS_TRANSITION_2
static const char *obsFiles[] = {
    "/ipconfig.json",   // Skipped for SD iteration
    "/tcdly", "/tcddt", "/tcdpt", "/tcdlt",
    "/tcdbricfg.json", "/tcdaicfg.json", "/tcdvolcfg.json", "/tcdalmcfg.json", 
    "/tcdremcfg.json", "/stconfig",      "/scconfig.json",  "/cmconfig.json",
    "/loconfig,json",  "/raconfig.json", "/tcdmcfg.json",   "/_installing.mp3",
    "/beep.mp3",
    NULL
};
#endif

static const char fwfn[]      = "/tcdfw.bin";
static const char fwfnold[]   = "/tcdfw.old";

static const char *fsNoAvail = "File System not available";
static const char *failFileWrite = "Failed to open file for writing";
#ifdef TC_DBG_BOOT
static const char *badConfig = "Settings bad/missing/incomplete; writing new file";
#endif

#ifdef TC_HAVEMQTT
static char mqm[] = "mqxx";
#endif

// If LittleFS/SPIFFS is mounted
bool haveFS = false;

// If a SD card is found
bool haveSD = false;

// Save sedondary settings on SD?
bool configOnSD = false;

// Paranoia: No writes Flash-FS
bool FlashROMode = false;

// If SD contains default audio files
static bool allowCPA = false;

// If current audio data is installed
bool haveAudioFiles = false;

// Music Folder Number
uint8_t musFolderNum = 0;

int sspeedopin = 0;
int stachopin = 0;

static uint8_t* (*r)(uint8_t *, uint32_t, int);
static bool read_settings(File configFile, int cfgReadCount);

#ifdef SETTINGS_TRANSITION
static bool CopyTextParm(const char *json, const char *json2, char *setting, int setSize);
static bool CopyCheckValidNumParm(const char *json, const char *json2, char *text, int psize, int lowerLim, int upperLim, int setDefault);
static bool CopyCheckValidNumParmF(const char *json, const char *json2, char *text, int psize, float lowerLim, float upperLim, float setDefault);
#else
static bool CopyTextParm(const char *json, char *setting, int setSize);
static bool CopyCheckValidNumParm(const char *json, char *text, int psize, int lowerLim, int upperLim, int setDefault);
static bool CopyCheckValidNumParmF(const char *json, char *text, int psize, float lowerLim, float upperLim, float setDefault);
#endif
static bool checkValidNumParm(char *text, int lowerLim, int upperLim, int setDefault);
static bool checkValidNumParmF(char *text, float lowerLim, float upperLim, float setDefault);

static bool loadBrightness();
static void loadBeepAutoInterval();
static void loadCarMode();
#ifdef TC_HAVE_REMOTE
static void loadRemoteAllowed();
#endif
static void loadUpdAvail();
uint16_t    loadClockState(int16_t& yoffs);
bool        saveClockState(uint16_t curYear, int16_t yearoffset);
dateStruct *getClockDataDL(uint8_t did);
void        getClockDataP(uint64_t& timeDifference, bool &timeDiffUp);
void        updateClockDataDL(uint8_t did, uint16_t year, uint8_t month, uint8_t day, uint8_t hour, uint8_t minute);
bool        saveClockDataDL(bool force, uint8_t did, uint16_t year, uint8_t month, uint8_t day, uint8_t hour, uint8_t minute);
void        updateClockDataP();
bool        saveClockDataP(bool force);
static void loadAllClockData();

static void cfc(File& sfile, int& haveErr, int& haveWriteErr);
static bool audio_files_present(int& alienVER);

static DeserializationError readJSONCfgFile(JsonDocument& json, File& configFile, uint32_t *newHash = NULL);
static bool writeJSONCfgFile(const JsonDocument& json, const char *fn, bool useSD, uint32_t oldHash = 0, uint32_t *newHash = NULL);

static bool readFileFromSD(const char *fn, uint8_t *buf, int len);
static bool writeFileToSD(const char *fn, uint8_t *buf, int len);
static bool readFileFromFS(const char *fn, uint8_t *buf, int len);
static bool writeFileToFS(const char *fn, uint8_t *buf, int len);

static bool loadConfigFile(const char *fn, uint8_t *buf, int len, int& validBytes, int forcefs = 0);
static bool saveConfigFile(const char *fn, uint8_t *buf, int len, int forcefs = 0);
static uint32_t calcHash(uint8_t *buf, int len);
static bool saveSecSettings(bool useCache);
static bool saveTerSettings(bool useCache);
#ifdef SETTINGS_TRANSITION
static void removeOldFiles(const char *oldfn);
#endif

static bool formatFlashFS(bool userSignal);

extern void start_file_copy();
extern void file_copy_progress();
extern void file_copy_done(int err);

static void firmware_update();

#ifdef TC_HAVEMQTT
static void preAllocMQTTTopMsg()
{
    for(int i = 0; i < 10; i++) {
        if(settings.mqmt[i]) free(settings.mqmt[i]);
        if(settings.mqmm[i]) free(settings.mqmm[i]);
        if((settings.mqmt[i] = (char *)malloc(128))) {
            memset(settings.mqmt[i], 0, 128);
        }
        if((settings.mqmm[i] = (char *)malloc(64))) {
            memset(settings.mqmm[i], 0, 64);
        }
    }
}

static void freeUnusedMQTTTopMsg()
{
    for(int i = 0; i < 10; i++) {
        if(settings.mqmt[i]) {
            if(!*settings.mqmt[i]) {
                free(settings.mqmt[i]);
                #ifdef TC_DBG_BOOT
                Serial.printf("MQTT: Freeing topic %d\n", i);
                #endif
            }
        }
        if(settings.mqmm[i]) {
            if(!*settings.mqmm[i]) {
                free(settings.mqmm[i]);
                #ifdef TC_DBG_BOOT
                Serial.printf("MQTT: Freeing msg %d\n", i);
                #endif
            }
        }
    }
}
#endif

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
    #ifdef TC_DBG_BOOT
    const char *funcName = "settings_setup";
    #endif
    bool writedefault = false;
    bool freshFS = false;
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
                #ifdef TC_DBG_BOOT
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

    #ifdef TC_HAVEMQTT
    for(int i = 0; i < 10; i++) {
        settings.mqmt[i] = settings.mqmm[i] = NULL;
    }
    // Pre-allocate MQTT user topics/messages here to avoid
    // memory fragmentation (if done after loading the JSON,
    // the strings end up above the JSON buffer, which is freed
    // and we end up with a hole in the heap)
    preAllocMQTTTopMsg();
    #endif

    #ifdef TC_DBG_BOOT
    Serial.printf("%s: Mounting flash FS... ", funcName);
    #endif

    if(MYNVS.begin()) {

        haveFS = true;

    } else {

        #ifdef TC_DBG_BOOT
        Serial.print("failed, formatting... ");
        #endif

        haveFS = formatFlashFS(true);
        freshFS = true;

    }

    if(haveFS) {

        #ifdef TC_DBG_BOOT
        Serial.printf("ok.\nFlashFS: %d total, %d used, %d free\n", MYNVS.totalBytes(), MYNVS.usedBytes(), MYNVS.totalBytes() - MYNVS.usedBytes());
        #endif
        
        // Remove files that no longer should exist
        char dtmfBuf[] = "/Dtmf-0.mp3";
        if(MYNVS.exists(dtmfBuf)) {
            #ifdef TC_DBG_BOOT
            Serial.println("Removing old audio files");
            #endif
            for(int i = 0; i < 10; i++) {
                dtmfBuf[6] = i + '0';
                MYNVS.remove(dtmfBuf);
            }
        }
        #ifdef SETTINGS_TRANSITION_2
        for(int i = 0; ; i++) {
            if(!obsFiles[i]) break;
            MYNVS.remove(obsFiles[i]);
        }
        #else
        MYNVS.remove("/beep.mp3");
        #endif
        
        if(MYNVS.exists(cfgName)) {
            File configFile = MYNVS.open(cfgName, "r");
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
    #ifdef TC_DBG_BOOT
    Serial.printf("%s: SD/SPI frequency %dMHz\n", funcName, sdfreq / 1000000);
    #endif

    #ifdef TC_DBG_BOOT
    Serial.printf("%s: Mounting SD... ", funcName);
    #endif

    if(!(SDres = SD.begin(SD_CS_PIN, SPI, sdfreq))) {
        #ifdef TC_DBG_BOOT
        Serial.printf("Retrying at 25Mhz... ");
        #endif
        SDres = SD.begin(SD_CS_PIN, SPI, 25000000);
    }

    if(SDres) {

        #ifdef TC_DBG_BOOT
        Serial.println("ok");
        #endif

        uint8_t cardType = SD.cardType();
       
        #ifdef TC_DBG_BOOT
        const char *sdTypes[5] = { "No card", "MMC", "SD", "SDHC", "unknown (SD not usable)" };
        Serial.printf("SD card type: %s\n", sdTypes[cardType > 4 ? 4 : cardType]);
        #endif

        haveSD = ((cardType != CARD_NONE) && (cardType != CARD_UNKNOWN));

    }

    if(haveSD) {

        firmware_update();

        if(SD.exists("/TCD_FLASH_RO") || !haveFS) {
            bool writedefault2 = true;
            FlashROMode = true;
            Serial.println("Flash-RO mode: Using SD only.");
            #ifdef TC_HAVEMQTT
            preAllocMQTTTopMsg();
            #endif
            if(SD.exists(cfgName)) {
                File configFile = SD.open(cfgName, "r");
                if(configFile) {
                    writedefault2 = read_settings(configFile, cfgReadCount);
                    configFile.close();
                }
            }
            if(writedefault2) {
                #ifdef TC_DBG_BOOT
                Serial.printf("%s: %s\n", funcName, badConfig);
                #endif
                mainConfigHash = 0;
                write_settings();
            }
        }

    } else {
        Serial.println("No SD card found");
    }

    #ifdef TC_HAVEMQTT
    freeUnusedMQTTTopMsg();
    #endif

    // Check if (current) audio data is installed
    haveAudioFiles = audio_files_present(alienVER);

    // Re-format flash FS if either alien VER found, or
    // neither VER nor our config file exist.
    // (Note: LittleFS crashes when flash FS is full.)
    if(!haveAudioFiles && haveFS && !FlashROMode) {
        if((alienVER > 0) || 
           (alienVER < 0 && !freshFS && !cfgReadCount)) {
            #ifdef TCD_DBG_BOOT
            Serial.printf("Reformatting. Alien VER: %d, used space %d", alienVER, MYNVS.usedBytes());
            #endif
            writedefault = true;
            formatFlashFS(true);
        }
    }

    // Now write new config to flash FS if old one somehow bad
    // Only write this file if FlashROMode is off
    if(haveFS && writedefault && !FlashROMode) {
        #ifdef TC_DBG_BOOT
        Serial.printf("%s: %s\n", funcName, badConfig);
        #endif
        mainConfigHash = 0;
        write_settings();
    }

    #ifdef SETTINGS_TRANSITION_2
    if(haveSD) {
        for(int i = 1; ; i++) {
            if(!obsFiles[i]) break;
            SD.remove(obsFiles[i]);
        }
    }
    #endif

    // Determine if secondary settings are to be stored on SD
    configOnSD = (haveSD && (evalBool(settings.CfgOnSD) || FlashROMode));

    // Load secondary config file
    if(loadConfigFile(secCfgName, (uint8_t *)&secSettings, sizeof(secSettings), secSetValidBytes)) {
        secSettingsHash = calcHash((uint8_t *)&secSettings, sizeof(secSettings));
        haveSecSettings = true;
    }

    // Load tertiary config file (SD only)
    if(haveSD) {
        if(loadConfigFile(terCfgName, (uint8_t *)&terSettings, sizeof(terSettings), terSetValidBytes, 1)) {
            terSettingsHash = calcHash((uint8_t *)&terSettings, sizeof(terSettings));
            haveTerSettings = true;
        }
    }

    // Load clock state & data
    loadAllClockData();

    loadBrightness();

    loadBeepAutoInterval();

    loadCarMode();

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

    loadUpdAvail();
    
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
        MYNVS.end();
        #ifdef TC_DBG_GEN
        Serial.println("Unmounted Flash FS");
        #endif
        haveFS = false;
    }
    if(haveSD) {
        SD.end();
        #ifdef TC_DBG_GEN
        Serial.println("Unmounted SD card");
        #endif
        haveSD = false;
    }
}

static bool read_settings(File configFile, int cfgReadCount)
{
    #ifdef TC_DBG_BOOT
    const char *funcName = "read_settings";
    #endif
    bool wd = false;
    size_t jsonSize = 0;
    
    DECLARE_D_JSON(JSON_SIZE,json);

    DeserializationError error = readJSONCfgFile(json, configFile, &mainConfigHash);

    #if ARDUINOJSON_VERSION_MAJOR < 7
    jsonSize = json.memoryUsage();
    if(jsonSize > JSON_SIZE) {
        Serial.printf("ERROR: Config too large (%d vs %d)\n", jsonSize, JSON_SIZE);
    }
    
    #ifdef TC_DBG_BOOT
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
            memset(settings.bssid, 0, sizeof(settings.bssid));
        }

        if(json["ssid"]) {
            memset(settings.ssid, 0, sizeof(settings.ssid));
            memset(settings.pass, 0, sizeof(settings.pass));
            memset(settings.bssid, 0, sizeof(settings.bssid));
            strncpy(settings.ssid, json["ssid"], sizeof(settings.ssid) - 1);
            if(json["pass"]) {
                strncpy(settings.pass, json["pass"], sizeof(settings.pass) - 1);
            }
            if(json["bssid"]) {
                strncpy(settings.bssid, json["bssid"], sizeof(settings.bssid) - 1);
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

        #ifdef SETTINGS_TRANSITION

        wd |= CopyTextParm(json["hn"], json["hostName"], settings.hostName, sizeof(settings.hostName));
        
        wd |= CopyCheckValidNumParm(json["wCR"], json["wifiConRetries"], settings.wifiConRetries, sizeof(settings.wifiConRetries), 1, 10, DEF_WIFI_RETRY);
        wd |= CopyCheckValidNumParm(json["wPR"], json["wifiPRetry"], settings.wifiPRetry, sizeof(settings.wifiPRetry), 0, 1, DEF_WIFI_PRETRY);
        wd |= CopyCheckValidNumParm(json["wOD"], json["wifiOffDelay"], settings.wifiOffDelay, sizeof(settings.wifiOffDelay), 0, 99, DEF_WIFI_OFFDELAY);

        wd |= CopyTextParm(json["sID"], json["systemID"], settings.systemID, sizeof(settings.systemID));
        wd |= CopyTextParm(json["appw"], NULL, settings.appw, sizeof(settings.appw));
        wd |= CopyCheckValidNumParm(json["apch"], NULL, settings.apChnl, sizeof(settings.apChnl), 0, 11, DEF_AP_CHANNEL);
        wd |= CopyCheckValidNumParm(json["wAOD"], json["wifiAPOffDelay"], settings.wifiAPOffDelay, sizeof(settings.wifiAPOffDelay), 0, 99, DEF_WIFI_APOFFDELAY);

        #else

        wd |= CopyTextParm(json["hn"], settings.hostName, sizeof(settings.hostName));
        
        wd |= CopyCheckValidNumParm(json["wCR"], settings.wifiConRetries, sizeof(settings.wifiConRetries), 1, 10, DEF_WIFI_RETRY);
        wd |= CopyCheckValidNumParm(json["wPR"], settings.wifiPRetry, sizeof(settings.wifiPRetry), 0, 1, DEF_WIFI_PRETRY);
        wd |= CopyCheckValidNumParm(json["wOD"], settings.wifiOffDelay, sizeof(settings.wifiOffDelay), 0, 99, DEF_WIFI_OFFDELAY);

        wd |= CopyTextParm(json["sID"], settings.systemID, sizeof(settings.systemID));
        wd |= CopyTextParm(json["appw"], settings.appw, sizeof(settings.appw));
        wd |= CopyCheckValidNumParm(json["apch"], settings.apChnl, sizeof(settings.apChnl), 0, 11, DEF_AP_CHANNEL);
        wd |= CopyCheckValidNumParm(json["wAOD"], settings.wifiAPOffDelay, sizeof(settings.wifiAPOffDelay), 0, 99, DEF_WIFI_APOFFDELAY);

        #endif

        // Settings

        #ifdef SETTINGS_TRANSITION // --------------------------------------------

        wd |= CopyCheckValidNumParm(json["pI"], json["playIntro"], settings.playIntro, sizeof(settings.playIntro), 0, 1, DEF_PLAY_INTRO);
        // Beep now in separate file, so it missing is ok
        CopyCheckValidNumParm(json["bep"], json["beep"], settings.beep, sizeof(settings.beep), 0, 3, DEF_BEEP);
        // Time cycling saved in separate file
        wd |= CopyCheckValidNumParm(json["sARA"], NULL, settings.autoRotAnim, sizeof(settings.autoRotAnim), 0, 1, 1);
        wd |= CopyCheckValidNumParm(json["skpTTA"], NULL, settings.skipTTAnim, sizeof(settings.skipTTAnim), 0, 1, DEF_SKIP_TTANIM);
        #ifndef IS_ACAR_DISPLAY
        wd |= CopyCheckValidNumParm(json["p3an"], NULL, settings.p3anim, sizeof(settings.p3anim), 0, 1, DEF_P3ANIM);
        #endif
        #ifdef IS_ACAR_DISPLAY
        wd |= CopyCheckValidNumParm(json["swapDL"], NULL, settings.swapDL, sizeof(settings.swapDL), 0, 1, DEF_SWPDL);
        #endif
        wd |= CopyCheckValidNumParm(json["pTTs"], json["playTTsnds"], settings.playTTsnds, sizeof(settings.playTTsnds), 0, 1, DEF_PLAY_TT_SND);
        wd |= CopyCheckValidNumParm(json["alRTC"], json["alarmRTC"], settings.alarmRTC, sizeof(settings.alarmRTC), 0, 1, DEF_ALARM_RTC);
        wd |= CopyCheckValidNumParm(json["md24"], json["mode24"], settings.mode24, sizeof(settings.mode24), 0, 1, DEF_MODE24);
        
        wd |= CopyTextParm(json["tZ"], json["timeZone"], settings.timeZone, sizeof(settings.timeZone));
        wd |= CopyTextParm(json["ntpS"], json["ntpServer"], settings.ntpServer, sizeof(settings.ntpServer));
        #ifdef TC_HAVEGPS
        wd |= CopyCheckValidNumParm(json["gTme"], NULL, settings.useGPSTime, sizeof(settings.useGPSTime), 0, 1, DEF_USE_GPS_TIME);
        #endif

        wd |= CopyTextParm(json["tZDest"], json["timeZoneDest"], settings.timeZoneDest, sizeof(settings.timeZoneDest));
        wd |= CopyTextParm(json["tZDep"], json["timeZoneDep"], settings.timeZoneDep, sizeof(settings.timeZoneDep));
        wd |= CopyTextParm(json["tZNDest"], json["timeZoneNDest"], settings.timeZoneNDest, sizeof(settings.timeZoneNDest));
        wd |= CopyTextParm(json["tZNDep"], json["timeZoneNDep"], settings.timeZoneNDep, sizeof(settings.timeZoneNDep));

        wd |= CopyCheckValidNumParm(json["almT"], NULL, settings.alarmType, sizeof(settings.alarmType), 0, 1, DEF_ALARM_TYPE);
        wd |= CopyCheckValidNumParm(json["aSz"], NULL, settings.doSnooze, sizeof(settings.doSnooze), 0, 1, DEF_SNOOZE);
        wd |= CopyCheckValidNumParm(json["aSzT"], NULL, settings.snoozeTime, sizeof(settings.snoozeTime), 1, 15, DEF_SNOOZE_TIME);
        wd |= CopyCheckValidNumParm(json["aASz"], NULL, settings.autoSnooze, sizeof(settings.autoSnooze), 0, 1, DEF_ASNOOZE);
        wd |= CopyCheckValidNumParm(json["aLU"], NULL, settings.almLoopUserSnd, sizeof(settings.almLoopUserSnd), 0, 1, DEF_LOOP_USER_SND);
        
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

        wd |= CopyCheckValidNumParm(json["CoSD"], json["CfgOnSD"], settings.CfgOnSD, sizeof(settings.CfgOnSD), 0, 1, DEF_CFG_ON_SD);
        //wd |= CopyCheckValidNumParm(json["sdFreq"], NULL, settings.sdFreq, sizeof(settings.sdFreq), 0, 1, DEF_SD_FREQ);
        wd |= CopyCheckValidNumParm(json["ttps"], json["timeTrPers"], settings.timesPers, sizeof(settings.timesPers), 0, 1, DEF_TIMES_PERS);

        wd |= CopyCheckValidNumParm(json["rAPM"], NULL, settings.revAmPm, sizeof(settings.revAmPm), 0, 1, DEF_REVAMPM);

        wd |= CopyCheckValidNumParm(json["fPwr"], json["fakePwrOn"], settings.fakePwrOn, sizeof(settings.fakePwrOn), 0, 1, DEF_FAKE_PWR);

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

        #ifdef TC_HAVETEMP
        wd |= CopyCheckValidNumParm(json["tmpU"], json["tempUnit"], settings.tempUnit, sizeof(settings.tempUnit), 0, 1, DEF_TEMP_UNIT);
        wd |= CopyCheckValidNumParmF(json["tmpOf"], json["tempOffs"], settings.tempOffs, sizeof(settings.tempOffs), -3.0f, 3.0f, DEF_TEMP_OFFS);
        #endif

        wd |= CopyCheckValidNumParm(json["ettDl"], json["ettDelay"], settings.ettDelay, sizeof(settings.ettDelay), 0, ETT_MAX_DEL, DEF_ETT_DELAY);
        //wd |= CopyCheckValidNumParm(json["ettLong"], NULL, settings.ettLong, sizeof(settings.ettLong), 0, 1, DEF_ETT_LONG);
        
        wd |= CopyCheckValidNumParm(json["ETTOc"], NULL, settings.ETTOcmd, sizeof(settings.ETTOcmd), 0, 1, DEF_ETTO_CMD);
        wd |= CopyCheckValidNumParm(json["ETTOPU"], NULL, settings.ETTOpus, sizeof(settings.ETTOpus), 0, 1, DEF_ETTO_PUS);
        wd |= CopyCheckValidNumParm(json["uETTO"], json["useETTO"], settings.useETTO, sizeof(settings.useETTO), 0, 1, DEF_USE_ETTO);
        wd |= CopyCheckValidNumParm(json["nETTOL"], json["noETTOLead"], settings.noETTOLead, sizeof(settings.noETTOLead), 0, 1, DEF_NO_ETTO_LEAD);
        wd |= CopyCheckValidNumParm(json["ETTOa"], NULL, settings.ETTOalm, sizeof(settings.ETTOalm), 0, 1, DEF_ETTO_ALM);
        wd |= CopyCheckValidNumParm(json["ETTOAD"], NULL, settings.ETTOAD, sizeof(settings.ETTOAD), 3, 99, DEF_ETTO_ALM_D);
        
        
        #ifdef SERVOSPEEDO
        wd |= CopyCheckValidNumParm(json["tin"], NULL, settings.ttinpin, sizeof(settings.ttinpin), 0, 2, 0);
        wd |= CopyCheckValidNumParm(json["tout"], NULL, settings.ttoutpin, sizeof(settings.ttoutpin), 0, 2, 0);
        #endif

        #ifdef TC_HAVEGPS
        wd |= CopyCheckValidNumParm(json["qGPS"], json["quickGPS"], settings.provGPS2BTTFN, sizeof(settings.provGPS2BTTFN), 0, 1, DEF_GPS4BTTFN);
        #endif

        #else // SETTINGS_TRANSITION  --------------------------------------------
        
        wd |= CopyCheckValidNumParm(json["pI"], settings.playIntro, sizeof(settings.playIntro), 0, 1, DEF_PLAY_INTRO);
        // Beep now in separate file, so it missing is ok
        CopyCheckValidNumParm(json["bep"], settings.beep, sizeof(settings.beep), 0, 3, DEF_BEEP);
        // Time cycling saved in separate file
        wd |= CopyCheckValidNumParm(json["sARA"], settings.autoRotAnim, sizeof(settings.autoRotAnim), 0, 1, 1);
        wd |= CopyCheckValidNumParm(json["skpTTA"], settings.skipTTAnim, sizeof(settings.skipTTAnim), 0, 1, DEF_SKIP_TTANIM);
        #ifndef IS_ACAR_DISPLAY
        wd |= CopyCheckValidNumParm(json["p3an"], settings.p3anim, sizeof(settings.p3anim), 0, 1, DEF_P3ANIM);
        #endif
        #ifdef IS_ACAR_DISPLAY
        wd |= CopyCheckValidNumParm(json["swapDL"],settings.swapDL, sizeof(settings.swapDL), 0, 1, DEF_SWPDL);
        #endif
        wd |= CopyCheckValidNumParm(json["pTTs"], settings.playTTsnds, sizeof(settings.playTTsnds), 0, 1, DEF_PLAY_TT_SND);
        wd |= CopyCheckValidNumParm(json["alRTC"], settings.alarmRTC, sizeof(settings.alarmRTC), 0, 1, DEF_ALARM_RTC);
        wd |= CopyCheckValidNumParm(json["md24"], settings.mode24, sizeof(settings.mode24), 0, 1, DEF_MODE24);
        
        wd |= CopyTextParm(json["tZ"], settings.timeZone, sizeof(settings.timeZone));
        wd |= CopyTextParm(json["ntpS"], settings.ntpServer, sizeof(settings.ntpServer));
        #ifdef TC_HAVEGPS
        wd |= CopyCheckValidNumParm(json["gTme"], settings.useGPSTime, sizeof(settings.useGPSTime), 0, 1, DEF_USE_GPS_TIME);
        #endif

        wd |= CopyTextParm(json["tZDest"], settings.timeZoneDest, sizeof(settings.timeZoneDest));
        wd |= CopyTextParm(json["tZDep"], settings.timeZoneDep, sizeof(settings.timeZoneDep));
        wd |= CopyTextParm(json["tZNDest"], settings.timeZoneNDest, sizeof(settings.timeZoneNDest));
        wd |= CopyTextParm(json["tZNDep"], settings.timeZoneNDep, sizeof(settings.timeZoneNDep));

        wd |= CopyCheckValidNumParm(json["almT"], settings.alarmType, sizeof(settings.alarmType), 0, 1, DEF_ALARM_TYPE);
        wd |= CopyCheckValidNumParm(json["aSz"], settings.doSnooze, sizeof(settings.doSnooze), 0, 1, DEF_SNOOZE);
        wd |= CopyCheckValidNumParm(json["aSzT"], settings.snoozeTime, sizeof(settings.snoozeTime), 1, 15, DEF_SNOOZE_TIME);
        wd |= CopyCheckValidNumParm(json["aASz"], settings.autoSnooze, sizeof(settings.autoSnooze), 0, 1, DEF_ASNOOZE);
        wd |= CopyCheckValidNumParm(json["aLU"], settings.almLoopUserSnd, sizeof(settings.almLoopUserSnd), 0, 1, DEF_LOOP_USER_SND);
        
        wd |= CopyCheckValidNumParm(json["dtNmOff"], settings.dtNmOff, sizeof(settings.dtNmOff), 0, 1, DEF_DT_OFF);
        wd |= CopyCheckValidNumParm(json["ptNmOff"], settings.ptNmOff, sizeof(settings.ptNmOff), 0, 1, DEF_PT_OFF);
        wd |= CopyCheckValidNumParm(json["ltNmOff"], settings.ltNmOff, sizeof(settings.ltNmOff), 0, 1, DEF_LT_OFF);
        wd |= CopyCheckValidNumParm(json["aNMPre"], settings.autoNMPreset, sizeof(settings.autoNMPreset), 0, 10, DEF_AUTONM_PRESET);
        wd |= CopyCheckValidNumParm(json["aNMOn"], settings.autoNMOn, sizeof(settings.autoNMOn), 0, 23, DEF_AUTONM_ON);
        wd |= CopyCheckValidNumParm(json["aNMOff"], settings.autoNMOff, sizeof(settings.autoNMOff), 0, 23, DEF_AUTONM_OFF);
        #ifdef TC_HAVELIGHT
        wd |= CopyCheckValidNumParm(json["uLgt"], settings.useLight, sizeof(settings.useLight), 0, 1, DEF_USE_LIGHT);
        wd |= CopyCheckValidNumParm(json["lxLim"], settings.luxLimit, sizeof(settings.luxLimit), 0, 50000, DEF_LUX_LIMIT);
        #endif

        wd |= CopyCheckValidNumParm(json["CoSD"], settings.CfgOnSD, sizeof(settings.CfgOnSD), 0, 1, DEF_CFG_ON_SD);
        //wd |= CopyCheckValidNumParm(json["sdFreq"], settings.sdFreq, sizeof(settings.sdFreq), 0, 1, DEF_SD_FREQ);
        wd |= CopyCheckValidNumParm(json["ttps"], settings.timesPers, sizeof(settings.timesPers), 0, 1, DEF_TIMES_PERS);

        wd |= CopyCheckValidNumParm(json["rAPM"], settings.revAmPm, sizeof(settings.revAmPm), 0, 1, DEF_REVAMPM);

        wd |= CopyCheckValidNumParm(json["fPwr"], settings.fakePwrOn, sizeof(settings.fakePwrOn), 0, 1, DEF_FAKE_PWR);

        #ifdef TC_HAVETEMP
        wd |= CopyCheckValidNumParm(json["tmpU"], settings.tempUnit, sizeof(settings.tempUnit), 0, 1, DEF_TEMP_UNIT);
        wd |= CopyCheckValidNumParmF(json["tmpOf"], settings.tempOffs, sizeof(settings.tempOffs), -3.0f, 3.0f, DEF_TEMP_OFFS);
        #endif

        wd |= CopyCheckValidNumParm(json["spT"], settings.speedoType, sizeof(settings.speedoType), 0, 99, DEF_SPEEDO_TYPE);
        wd |= CopyCheckValidNumParm(json["spB"], settings.speedoBright, sizeof(settings.speedoBright), 0, 15, DEF_BRIGHT_SPEEDO);
        wd |= CopyCheckValidNumParm(json["spAO"], settings.speedoAO, sizeof(settings.speedoAO), 0, 1, DEF_SPEEDO_AO);
        wd |= CopyCheckValidNumParm(json["spAF"], settings.speedoAF, sizeof(settings.speedoAF), 0, 1, DEF_SPEEDO_ACCELFIG);
        wd |= CopyCheckValidNumParmF(json["spFc"], settings.speedoFact, sizeof(settings.speedoFact), 0.5f, 5.0f, DEF_SPEEDO_FACT);
        wd |= CopyCheckValidNumParm(json["spP3"], settings.speedoP3, sizeof(settings.speedoP3), 0, 1, DEF_SPEEDO_P3);
        wd |= CopyCheckValidNumParm(json["spd3rd"], settings.speedo3rdD, sizeof(settings.speedo3rdD), 0, 1, DEF_SPEEDO_3RDD);
        #ifdef TC_HAVEGPS
        wd |= CopyCheckValidNumParm(json["uGPSS"], settings.dispGPSSpeed, sizeof(settings.dispGPSSpeed), 0, 1, DEF_USE_GPS_SPEED);
        wd |= CopyCheckValidNumParm(json["spUR"], settings.spdUpdRate, sizeof(settings.spdUpdRate), 0, 3, DEF_SPD_UPD_RATE);
        #endif
        #ifdef TC_HAVETEMP
        wd |= CopyCheckValidNumParm(json["dTmp"], settings.dispTemp, sizeof(settings.dispTemp), 0, 1, DEF_DISP_TEMP);
        wd |= CopyCheckValidNumParm(json["tmpB"], settings.tempBright, sizeof(settings.tempBright), 0, 15, DEF_TEMP_BRIGHT);
        wd |= CopyCheckValidNumParm(json["tmpONM"], settings.tempOffNM, sizeof(settings.tempOffNM), 0, 1, DEF_TEMP_OFF_NM);
        #endif

        wd |= CopyCheckValidNumParm(json["ettDl"], settings.ettDelay, sizeof(settings.ettDelay), 0, ETT_MAX_DEL, DEF_ETT_DELAY);
        //wd |= CopyCheckValidNumParm(json["ettLong"], settings.ettLong, sizeof(settings.ettLong), 0, 1, DEF_ETT_LONG);
        
        wd |= CopyCheckValidNumParm(json["ETTOc"], settings.ETTOcmd, sizeof(settings.ETTOcmd), 0, 1, DEF_ETTO_CMD);
        wd |= CopyCheckValidNumParm(json["ETTOPU"], settings.ETTOpus, sizeof(settings.ETTOpus), 0, 1, DEF_ETTO_PUS);
        wd |= CopyCheckValidNumParm(json["uETTO"], settings.useETTO, sizeof(settings.useETTO), 0, 1, DEF_USE_ETTO);
        wd |= CopyCheckValidNumParm(json["nETTOL"], settings.noETTOLead, sizeof(settings.noETTOLead), 0, 1, DEF_NO_ETTO_LEAD);
        wd |= CopyCheckValidNumParm(json["ETTOa"], settings.ETTOalm, sizeof(settings.ETTOalm), 0, 1, DEF_ETTO_ALM);
        wd |= CopyCheckValidNumParm(json["ETTOAD"], settings.ETTOAD, sizeof(settings.ETTOAD), 3, 99, DEF_ETTO_ALM_D);

        #ifdef SERVOSPEEDO
        wd |= CopyCheckValidNumParm(json["tin"], settings.ttinpin, sizeof(settings.ttinpin), 0, 2, 0);
        wd |= CopyCheckValidNumParm(json["tout"], settings.ttoutpin, sizeof(settings.ttoutpin), 0, 2, 0);
        #endif

        #ifdef TC_HAVEGPS
        wd |= CopyCheckValidNumParm(json["qGPS"], settings.provGPS2BTTFN, sizeof(settings.provGPS2BTTFN), 0, 1, DEF_GPS4BTTFN);
        #endif
        
        #endif  // SETTINGS_TRANSITION  --------------------------------------------

        #ifdef TC_HAVEMQTT
        #ifdef SETTINGS_TRANSITION
        wd |= CopyCheckValidNumParm(json["uMQTT"], json["useMQTT"], settings.useMQTT, sizeof(settings.useMQTT), 0, 1, 0);
        wd |= CopyTextParm(json["mqttS"], json["mqttServer"], settings.mqttServer, sizeof(settings.mqttServer));
        wd |= CopyCheckValidNumParm(json["mqttV"], NULL, settings.mqttVers, sizeof(settings.mqttVers), 0, 1, 0);
        wd |= CopyTextParm(json["mqttU"], json["mqttUser"], settings.mqttUser, sizeof(settings.mqttUser));
        wd |= CopyTextParm(json["mqttT"], json["mqttTopic"], settings.mqttTopic, sizeof(settings.mqttTopic));
        wd |= CopyCheckValidNumParm(json["pMQTT"], json["pubMQTT"], settings.pubMQTT, sizeof(settings.pubMQTT), 0, 1, 0);
        wd |= CopyCheckValidNumParm(json["vMQTT"], NULL, settings.MQTTvarLead, sizeof(settings.MQTTvarLead), 0, 1, DEF_MQTT_VTT);
        wd |= CopyCheckValidNumParm(json["mqP"], NULL, settings.mqttPwr, sizeof(settings.mqttPwr), 0, 1, 0);
        wd |= CopyCheckValidNumParm(json["mqPO"], NULL, settings.mqttPwrOn, sizeof(settings.mqttPwrOn), 0, 1, 0);
        #else
        wd |= CopyCheckValidNumParm(json["uMQTT"], settings.useMQTT, sizeof(settings.useMQTT), 0, 1, 0);
        wd |= CopyTextParm(json["mqttS"], settings.mqttServer, sizeof(settings.mqttServer));
        wd |= CopyCheckValidNumParm(json["mqttV"], settings.mqttVers, sizeof(settings.mqttVers), 0, 1, 0);
        wd |= CopyTextParm(json["mqttU"], settings.mqttUser, sizeof(settings.mqttUser));
        wd |= CopyTextParm(json["mqttT"], settings.mqttTopic, sizeof(settings.mqttTopic));
        wd |= CopyCheckValidNumParm(json["pMQTT"], settings.pubMQTT, sizeof(settings.pubMQTT), 0, 1, 0);
        wd |= CopyCheckValidNumParm(json["vMQTT"], settings.MQTTvarLead, sizeof(settings.MQTTvarLead), 0, 1, DEF_MQTT_VTT);
        wd |= CopyCheckValidNumParm(json["mqP"], settings.mqttPwr, sizeof(settings.mqttPwr), 0, 1, 0);
        wd |= CopyCheckValidNumParm(json["mqPO"], settings.mqttPwrOn, sizeof(settings.mqttPwrOn), 0, 1, 0);
        #endif

        for(int i = 0; i < 10; i++) {
            mqm[2] = i + '0';
            mqm[3] = 't';
            if(settings.mqmt[i]) {
                free((void *)settings.mqmt[i]);
                settings.mqmt[i] = NULL;
            }
            if(settings.mqmm[i]) {
                free((void *)settings.mqmm[i]);
                settings.mqmm[i] = NULL;
            }
            if(json[mqm]) {
                settings.mqmt[i] = (char *)malloc(strlen(json[mqm]) + 1);
                strcpy(settings.mqmt[i], json[mqm]);
                #ifdef TC_DBG_BOOT
                Serial.printf("MQTT msg %d topic: %s\n", i, settings.mqmt[i]);
                #endif
            }
            mqm[3] = 'm';
            if(json[mqm]) {
                settings.mqmm[i] = (char *)malloc(strlen(json[mqm]) + 1);
                strcpy(settings.mqmm[i], json[mqm]);
                #ifdef TC_DBG_BOOT
                Serial.printf("MQTT msg %d message: %s\n", i, settings.mqmm[i]);
                #endif
            }
        }
        #endif
        
    } else {

        wd = true;

    }

    return wd;
}

void write_settings()
{
    #ifdef TC_DBG_BOOT
    const char *funcName = "write_settings";
    #endif
    DECLARE_D_JSON(JSON_SIZE,json);

    if(!haveFS && !FlashROMode) {
        Serial.printf("%s\n", fsNoAvail);
        return;
    }

    #ifdef TC_DBG_BOOT
    Serial.printf("%s: Writing config file\n", funcName);
    #endif

    // Write this only if either set, or also present in file read earlier
    if(settings.ssid[0] || settings.ssid[1] != 'X') {
        json["ssid"] = (const char *)settings.ssid;
        json["pass"] = (const char *)settings.pass;
        json["bssid"] = (const char *)settings.bssid;
    }

    json["hn"] = (const char *)settings.hostName;
    json["wCR"] = (const char *)settings.wifiConRetries;
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
    #ifdef IS_ACAR_DISPLAY
    json["swapDL"] = (const char *)settings.swapDL;
    #endif
    json["pTTs"] = (const char *)settings.playTTsnds;
    json["alRTC"] = (const char *)settings.alarmRTC;
    json["md24"] = (const char *)settings.mode24;

    json["tZ"] = (const char *)settings.timeZone;
    json["ntpS"] = (const char *)settings.ntpServer;
    #ifdef TC_HAVEGPS
    json["gTme"] = (const char *)settings.useGPSTime;
    #endif

    json["tZDest"] = (const char *)settings.timeZoneDest;
    json["tZDep"] = (const char *)settings.timeZoneDep;
    json["tZNDest"] = (const char *)settings.timeZoneNDest;
    json["tZNDep"] = (const char *)settings.timeZoneNDep;

    json["almT"] = (const char *)settings.alarmType;
    json["aSz"] = (const char *)settings.doSnooze;
    json["aSzT"] = (const char *)settings.snoozeTime;
    json["aASz"] = (const char *)settings.autoSnooze;
    json["aLU"] = (const char *)settings.almLoopUserSnd;
    
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

    json["CoSD"] = (const char *)settings.CfgOnSD;
    //json["sdFreq"] = (const char *)settings.sdFreq;
    json["ttps"] = (const char *)settings.timesPers;

    json["rAPM"] = (const char *)settings.revAmPm;

    json["fPwr"] = (const char *)settings.fakePwrOn;

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

    #ifdef TC_HAVETEMP
    json["tmpU"] = (const char *)settings.tempUnit;
    json["tmpOf"] = (const char *)settings.tempOffs;
    #endif

    json["ettDl"] = (const char *)settings.ettDelay;
    //json["ettLong"] = (const char *)settings.ettLong;
    
    #ifdef TC_HAVEGPS
    json["qGPS"] = (const char *)settings.provGPS2BTTFN;
    #endif

    json["ETTOc"] = (const char *)settings.ETTOcmd;
    json["ETTOPU"] = (const char *)settings.ETTOpus;
    json["uETTO"] = (const char *)settings.useETTO;
    json["nETTOL"] = (const char *)settings.noETTOLead;
    json["ETTOa"] = (const char *)settings.ETTOalm;
    json["ETTOAD"] = (const char *)settings.ETTOAD;
            
    #ifdef SERVOSPEEDO
    json["tin"] = (const char *)settings.ttinpin;
    json["tout"] = (const char *)settings.ttoutpin;
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
    for(int i = 0; i < 10; i++) {
        mqm[2] = i + '0';
        mqm[3] = 't';
        if(settings.mqmt[i]) {
            json[mqm] = (const char *)settings.mqmt[i];
            #ifdef TC_DBG_BOOT
            Serial.printf("Saving MQTT msg %d topic: %s\n", i, settings.mqmt[i]);
            #endif
        }
        mqm[3] = 'm';
        if(settings.mqmm[i]) {
            json[mqm] = (const char *)settings.mqmm[i];
            #ifdef TC_DBG_BOOT
            Serial.printf("Saving MQTT msg %d message: %s\n", i, settings.mqmm[i]);
            #endif
        }
    }
    #endif

    writeJSONCfgFile(json, cfgName, FlashROMode, mainConfigHash, &mainConfigHash);
}

bool checkConfigExists()
{
    return FlashROMode ? SD.exists(cfgName) : (haveFS && MYNVS.exists(cfgName));
}

/*
 *  Helpers for parm copying & checking
 */

#ifdef SETTINGS_TRANSITION
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

static bool CopyCheckValidNumParm(const char *json, const char *json2, char *text, int psize, int lowerLim, int upperLim, int setDefault)
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

static bool CopyCheckValidNumParmF(const char *json, const char *json2, char *text, int psize, float lowerLim, float upperLim, float setDefault)
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
#else
static bool CopyTextParm(const char *json, char *setting, int setSize)
{
    if(!json) return true;
    
    memset(setting, 0, setSize);
    strncpy(setting, json, setSize - 1);
    return false;
}

static bool CopyCheckValidNumParm(const char *json, char *text, int psize, int lowerLim, int upperLim, int setDefault)
{
    if(!json) return true;

    memset(text, 0, psize);
    strncpy(text, json, psize-1);
    return checkValidNumParm(text, lowerLim, upperLim, setDefault);
}

static bool CopyCheckValidNumParmF(const char *json, char *text, int psize, float lowerLim, float upperLim, float setDefault)
{
    if(!json) return true;

    memset(text, 0, psize);
    strncpy(text, json, psize-1);
    return checkValidNumParmF(text, lowerLim, upperLim, setDefault);
}
#endif

static bool checkValidNumParm(char *text, int lowerLim, int upperLim, int setDefault)
{
    int i, len = strlen(text);
    bool ret = false;

    if(!len) {
        i = setDefault;
        ret = true;
    } else {
        for(int j = 0; j < len; j++) {
            if(text[j] < '0' || text[j] > '9') {
                i = setDefault;
                ret = true;
                break;
            }
        }
        if(!ret) {
            i = atoi(text);   
            if(i < lowerLim) {
                i = lowerLim;
                ret = true;
            } else if(i > upperLim) {
                i = upperLim;
                ret = true;
            }
        }
    }

    // Re-do to get rid of formatting errors (eg "000")
    sprintf(text, "%d", i);

    return ret;
}

static bool checkValidNumParmF(char *text, float lowerLim, float upperLim, float setDefault)
{
    int i, len = strlen(text);
    bool ret = false;
    float f;

    if(!len) {
        f = setDefault;
        ret = true;
    } else {
        for(i = 0; i < len; i++) {
            if(text[i] != '.' && text[i] != '-' && (text[i] < '0' || text[i] > '9')) {
                f = setDefault;
                ret = true;
                break;
            }
        }
        if(!ret) {
            f = strtof(text, NULL);
            if(f < lowerLim) {
                f = lowerLim;
                ret = true;
            } else if(f > upperLim) {
                f = upperLim;
                ret = true;
            }
        }
    }
    // Re-do to get rid of formatting errors (eg "0.")
    sprintf(text, "%.1f", f);

    return ret;
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
        if(MYNVS.exists(fn)) {
            haveConfigFile = (f = MYNVS.open(fn, "r"));
        }
    }

    return haveConfigFile;
}

/*
 *  Load/save display brightness
 */

static bool loadBrightness()
{
    if(haveSecSettings) {
        #ifdef TC_DBG_BOOT
        Serial.println("loadBrightness: extracting from secSettings");
        #endif
        settings.destTimeBright = secSettings.brightness[0];
        settings.presTimeBright = secSettings.brightness[1];
        settings.lastTimeBright = secSettings.brightness[2];
    } else {
        #ifdef SETTINGS_TRANSITION
        File configFile;
        if(openCfgFileRead(briCfgName, configFile)) {
            DECLARE_S_JSON(512,json);
            char destTimeBright[4];
            char presTimeBright[4];
            char lastTimeBright[4];
            if(!readJSONCfgFile(json, configFile)) {
                CopyCheckValidNumParm(json["dBri"], NULL, destTimeBright, sizeof(destTimeBright), 0, 15, DEF_BRIGHT_DEST);
                CopyCheckValidNumParm(json["pBri"], NULL, presTimeBright, sizeof(presTimeBright), 0, 15, DEF_BRIGHT_PRES);
                CopyCheckValidNumParm(json["lBri"], NULL, lastTimeBright, sizeof(lastTimeBright), 0, 15, DEF_BRIGHT_DEPA);
                settings.destTimeBright = atoi(destTimeBright);
                settings.presTimeBright = atoi(presTimeBright);
                settings.lastTimeBright = atoi(lastTimeBright);
            }
            configFile.close();
            saveBrightness();
        }
        removeOldFiles(briCfgName);
        #endif
    }

    return true;
}

void saveBrightness()
{
    secSettings.brightness[0] = settings.destTimeBright;
    secSettings.brightness[1] = settings.presTimeBright;
    secSettings.brightness[2] = settings.lastTimeBright;
    saveSecSettings(true);
}

/*
 *  Load/save BeepMode & autoInterval
 *
 */

static void updateAIntBeepCP()
{
    sprintf(settings.autoRotateTimes, "%d", autoInterval);
    sprintf(settings.beep, "%d", beepMode);
}
  
static void loadBeepAutoInterval()
{
    if(haveSecSettings) {
        #ifdef TC_DBG_BOOT
        Serial.println("loadBeepAutoInterval: extracting from secSettings");
        #endif
        if(secSettings.autoInterval <= 5) {
            autoInterval = secSettings.autoInterval;
        }
        if(secSettings.beepMode <= 3) {
            beepMode = secSettings.beepMode;
        }
        updateAIntBeepCP();
    } else {
        #ifdef SETTINGS_TRANSITION
        File configFile;
        if(openCfgFileRead(ainCfgName, configFile)) {
            DECLARE_S_JSON(512,json);
            if(!readJSONCfgFile(json, configFile)) {
                CopyCheckValidNumParm(json["ai"], NULL, settings.autoRotateTimes, sizeof(settings.autoRotateTimes), 0, 5, DEF_AUTOROTTIMES);
                CopyCheckValidNumParm(json["beep"], NULL, settings.beep, sizeof(settings.beep), 0, 3, atoi(settings.beep));
            }
            configFile.close();
        }
        removeOldFiles(ainCfgName);
        autoInterval = (uint8_t)atoi(settings.autoRotateTimes);
        beepMode = (uint8_t)atoi(settings.beep);
        saveBeepAutoInterval();
        #endif
    }
}

void saveBeepAutoInterval()
{
    secSettings.autoInterval = autoInterval;
    secSettings.beepMode = beepMode;
    saveSecSettings(true);
    updateAIntBeepCP();
}

/*
 *  Load/save audio volume
 */

void loadCurVolume()
{
    if(haveSecSettings) {
        #ifdef TC_DBG_BOOT
        Serial.println("loadCurVolume: extracting from secSettings");
        #endif
        if(secSettings.curVolume == 255 || secSettings.curVolume <= 19) {
            curVolume = secSettings.curVolume;
        }
    } else {
        #ifdef SETTINGS_TRANSITION
        char temp[6];
        File configFile;
        if(openCfgFileRead(volCfgName, configFile)) {
            DECLARE_S_JSON(512,json);
            if(!readJSONCfgFile(json, configFile)) {
                if(!CopyCheckValidNumParm(json["volume"], NULL, temp, sizeof(temp), 0, 255, 255)) {
                    int ncv = atoi(temp);
                    if((ncv >= 0 && ncv <= 19) || ncv == 255) curVolume = ncv;
                }
            } 
            configFile.close();
            saveCurVolume();
        }
        removeOldFiles(volCfgName);
        #endif
    }
}

void storeCurVolume()
{
    // Used to keep secSettings up-to-date in case
    // of delayed save
    secSettings.curVolume = curVolume;
}

void saveCurVolume()
{
    storeCurVolume();
    saveSecSettings(true);
}

/*
 *  Load/save the Alarm settings
 */

void loadAlarm()
{
    if(haveSecSettings) {
        #ifdef TC_DBG_BOOT
        Serial.println("loadAlarm: extracting from secSettings");
        #endif
        if(((secSettings.alarmHour   == 255) || (secSettings.alarmHour   <= 23)) &&
           ((secSettings.alarmMinute == 255) || (secSettings.alarmMinute <= 59))) {
            alarmHour = secSettings.alarmHour;
            alarmMinute = secSettings.alarmMinute;
            alarmOnOff = !!secSettings.alarmOnOff;
            alarmWeekday = secSettings.alarmWeekday;
            if(alarmWeekday > 9) alarmWeekday = 0;
        }
    } else {
        #ifdef SETTINGS_TRANSITION
        bool writedefault = true;
        File configFile;
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
        removeOldFiles(almCfgName);
        if(writedefault) {
            alarmHour = DEF_ALARM_HOUR;
            alarmMinute = DEF_ALARM_MINUTE;
            alarmOnOff = DEF_ALARM_ONOFF;
            alarmWeekday = DEF_ALARM_WD;
        } else {
            saveAlarm();
        }
        #endif
    }
}

void saveAlarm()
{
    secSettings.alarmHour = alarmHour;
    secSettings.alarmMinute = alarmMinute;
    secSettings.alarmOnOff = alarmOnOff ? 1 : 0;
    secSettings.alarmWeekday = alarmWeekday;
    saveSecSettings(true);
}

/*
 *  Load/save the Yearly/Monthly Reminder settings
 */

void loadReminder()
{
    if(haveSecSettings) {
        #ifdef TC_DBG_BOOT
        Serial.println("loadReminder: extracting from secSettings");
        #endif
        if(secSettings.remMonth <= 12 && secSettings.remDay <= 31 && 
           secSettings.remHour <= 23 && secSettings.remMin <= 59) {
            remMonth = secSettings.remMonth;
            remDay  = secSettings.remDay;
            remHour = secSettings.remHour;
            remMin  = secSettings.remMin;
        }
    } else {
        #ifdef SETTINGS_TRANSITION
        bool writedefault = false;
        File configFile;
        if(!haveFS && !configOnSD) return;
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
                remMonth = DEF_REM_MONTH;
                remDay = DEF_REM_DAY;
                remHour = DEF_REM_HOUR;
                remMin = DEF_REM_MIN;
            } else {
                saveReminder();
            }
        }
        removeOldFiles(remCfgName);
        #endif
    }
}

void saveReminder()
{
    secSettings.remMonth = remMonth;
    secSettings.remDay = remDay;
    if(!remMonth && !remDay) {
        secSettings.remHour = 0;
        secSettings.remMin = 0;
    } else {
        secSettings.remHour = remHour;
        secSettings.remMin = remMin;
    }
    saveSecSettings(true);
}

/*
 *  Load/save carMode
 */

static void loadCarMode()
{
    if(haveSecSettings) {
        #ifdef TC_DBG_BOOT
        Serial.println("loadCarMode: extracting from secSettings");
        #endif
        carMode = !!secSettings.carMode;
    } else {
        #ifdef SETTINGS_TRANSITION
        File configFile;
        if(!haveFS && !configOnSD) return;
        if(openCfgFileRead(cmCfgName, configFile)) {
            DECLARE_S_JSON(512,json);
            if(!readJSONCfgFile(json, configFile)) {
                if(json["CarMode"]) {
                    carMode = (atoi(json["CarMode"]) > 0);
                }
            } 
            configFile.close();
            saveCarMode();
        }
        removeOldFiles(cmCfgName);
        #endif
    }
}

void saveCarMode()
{
    secSettings.carMode = carMode ? 1 : 0;
    saveSecSettings(true);
}

/*
 * Save/load stale present time
 */

void loadStaleTime(void *target, bool& currentOn)
{
    if(haveSecSettings) {
        #ifdef TC_DBG_BOOT
        Serial.println("loadStaleTime: extracting from secSettings");
        #endif
        currentOn = !!secSettings.exhOnOff;
        memcpy(target, (void *)&secSettings.exhDates[0], 2*sizeof(dateStruct));
    } else {
        #ifdef SETTINGS_TRANSITION
        bool haveConfigFile = false;
        uint8_t loadBuf[1+(2*sizeof(dateStruct))+1];
        uint16_t sum = 0;
        if(!haveFS && !configOnSD) return;
        if(configOnSD) {
            haveConfigFile = readFileFromSD(stCfgName, loadBuf, sizeof(loadBuf));
        }
        if(!haveConfigFile && haveFS) {
            haveConfigFile = readFileFromFS(stCfgName, loadBuf, sizeof(loadBuf));
        }
        removeOldFiles(stCfgName);
        if(!haveConfigFile) return;
        for(uint8_t i = 0; i < 1+(2*sizeof(dateStruct)); i++) {
            sum += (loadBuf[i] ^ 0x55);
        }
        if(((sum & 0xff) == loadBuf[1+(2*sizeof(dateStruct))])) {
            currentOn = loadBuf[0] ? true : false;
            memcpy(target, (void *)&loadBuf[1], 2*sizeof(dateStruct));
            saveStaleTime(target, currentOn);
        }
        #endif
    }
}

void saveStaleTime(void *source, bool currentOn)
{
    secSettings.exhOnOff = currentOn ? 1 : 0;
    memcpy((void *)&secSettings.exhDates[0], source, 2*sizeof(dateStruct));
    saveSecSettings(true);
}

void initDefaultStaleTime(void *src)
{
    // Copy default times into secSettings to have a default there, too
    memcpy((void *)&secSettings.exhDates[0], src, 2*sizeof(dateStruct));
}

/*
 *  Load/save lineOut
 */

void loadLineOut()
{
    if(!haveLineOut)
        return;

    if(haveSecSettings) {
        #ifdef TC_DBG_BOOT
        Serial.println("loadLineOut: extracting from secSettings");
        #endif
        useLineOut = !!secSettings.useLineOut;
    } else {
        #ifdef SETTINGS_TRANSITION
        File configFile;
        if(!haveFS && !configOnSD) return;
        if(openCfgFileRead(loCfgName, configFile)) {
            DECLARE_S_JSON(512,json);
            if(!readJSONCfgFile(json, configFile)) {
                if(json["LineOut"]) {
                    useLineOut = (atoi(json["LineOut"]) > 0);
                }
            }
            configFile.close();
            saveLineOut();
        }
        removeOldFiles(loCfgName);
        #endif
    }
}

void saveLineOut()
{
    if(!haveLineOut)
        return;

    secSettings.useLineOut = useLineOut ? 1 : 0;
    saveSecSettings(true);
}

/*
 *  Load/save remoteAllowed
 */

#ifdef TC_HAVE_REMOTE
static void loadRemoteAllowed()
{
    if(haveSecSettings) {
        #ifdef TC_DBG_BOOT
        Serial.println("loadRemoteAllowed: extracting from secSettings");
        #endif
        remoteAllowed = !!secSettings.remoteAllowed;
        remoteKPAllowed = !!secSettings.remoteKPAllowed;
    } else {
        #ifdef SETTINGS_TRANSITION
        File configFile;
        if(!haveFS && !configOnSD) return;
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
            saveRemoteAllowed();
        }
        removeOldFiles(raCfgName);
        #endif
    }
}

void saveRemoteAllowed()
{
    secSettings.remoteAllowed = remoteAllowed ? 1 : 0;
    secSettings.remoteKPAllowed = remoteKPAllowed ? 1 : 0;
    saveSecSettings(true);
}
#endif

#ifdef SERVOSPEEDO
void loadServoCorr(int& scorr, int& tcorr)
{
    if(haveSecSettings) {
        #ifdef TC_DBG_BOOT
        Serial.println("loadServoCorr: extracting from secSettings");
        #endif
        scorr = secSettings.scorr;
        tcorr = secSettings.tcorr;
    } else {
        #ifdef SETTINGS_TRANSITION
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
                saveServoCorr(scorr, tcorr);
            }
        }
        removeOldFiles(scCfgName);
        #endif
    }
}

void saveServoCorr(int scorr, int tcorr)
{
    secSettings.scorr = scorr;
    secSettings.tcorr = tcorr;
    saveSecSettings(true);
}
#endif

/*
 *  Load/save "show update notification at boot"
 */

static void loadUpdAvail()
{
    if(haveSecSettings) {
        showUpdAvail = !!secSettings.showUpdAvail;
    }
}

void saveUpdAvail()
{
    secSettings.showUpdAvail = showUpdAvail ? 1 : 0;
    saveSecSettings(true);
}

/*
 *  Load/save curr version
 */

void loadUpdVers(int &v, int& r)
{
    if(haveSecSettings) {
        v = secSettings.updateV;
        r = secSettings.updateR;
    } else {
        v = r = 0;
    }
}

void saveUpdVers(int v, int r)
{
    secSettings.updateV = v;
    secSettings.updateR = r;
    saveSecSettings(true);
}

/*
 * Load/save Music Folder Number
 */

void loadMusFoldNum()
{
    if(!haveSD)
        return;

    if(haveTerSettings) {
        #ifdef TC_DBG_BOOT
        Serial.println("loadMusFoldNum: extracting from terSettings");
        #endif
        if(terSettings.musFolderNum <= 9) {
            musFolderNum = terSettings.musFolderNum;
        }
    } else {
        #ifdef SETTINGS_TRANSITION
        char temp[4];
        musFolderNum = 0;
        if(SD.exists(musCfgName)) {
            File configFile = SD.open(musCfgName, "r");
            if(configFile) {
                DECLARE_S_JSON(512,json);
                if(!readJSONCfgFile(json, configFile)) {
                    if(!CopyCheckValidNumParm(json["folder"], NULL, temp, sizeof(temp), 0, 9, 0)) {
                        musFolderNum = atoi(temp);
                    }
                }
                configFile.close();
                saveMusFoldNum();
            }
        }
        removeOldFiles(musCfgName);
        #endif
        
    }
}

void saveMusFoldNum()
{
    terSettings.musFolderNum = musFolderNum;
    saveTerSettings(true);
}

/*
 * Load/save shuffle setting
 */

void loadShuffle()
{
    if(haveSD && haveTerSettings) {
        mpShuffle = !!terSettings.mpShuffle;
    }
}

void saveShuffle()
{
    terSettings.mpShuffle = mpShuffle;
    saveTerSettings(true);
}

/*
 * Load/save boot display mode (RC, WC, Nav, Mini)
 */

uint8_t loadBootMode()
{
    if(haveSD && haveTerSettings) {
        return terSettings.bootMode;
    }

    return 0;
}

void storeBootMode()
{
    uint8_t t = 0;
    if(isRcMode())   t |= 0x01;
    if(isWcMode())   t |= 0x02;
    #ifdef TC_HAVEGPS
    if(isNavMode())  t |= 0x04;
    #endif
    if(isMiniMode()) t |= 0x08;
    terSettings.bootMode = t;
}

void saveBootMode()
{
    storeBootMode();
    saveTerSettings(true);
}

/*
 * Load/save/delete settings for static IP configuration
 */

#ifdef SETTINGS_TRANSITION
static bool CopyIPParm(const char *json, char *text, uint8_t psize)
{
    if(!json) return true;

    if(strlen(json) == 0)
        return true;

    memset(text, 0, psize);
    strncpy(text, json, psize-1);
    return false;
}
#endif

bool loadIpSettings()
{
    memset((void *)&ipsettings, 0, sizeof(ipsettings));

    if(!haveFS && !FlashROMode)
        return false;

    int vb = 0;
    if(loadConfigFile(ipCfgName, (uint8_t *)&ipsettings, sizeof(ipsettings), vb, -1)) {
        if(*ipsettings.ip) {
            if(checkIPConfig()) {
                ipHash = calcHash((uint8_t *)&ipsettings, sizeof(ipsettings));
                return true;
            } else {
                #ifdef TC_DBG_BOOT
                Serial.println("loadIpSettings: IP settings invalid; deleting file");
                #endif
                memset((void *)&ipsettings, 0, sizeof(ipsettings));
                deleteIpSettings();
            }
        }
    } else {
        #ifdef SETTINGS_TRANSITION
        bool invalid = false;
        bool haveConfig = false;
        if( (!FlashROMode && MYNVS.exists(ipCfgNameO)) ||
            (FlashROMode && SD.exists(ipCfgNameO)) ) {
            File configFile = FlashROMode ? SD.open(ipCfgNameO, "r") : MYNVS.open(ipCfgNameO, "r");
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
            removeOldFiles(ipCfgNameO);
        }
        if(invalid) {
            memset((void *)&ipsettings, 0, sizeof(ipsettings));
        } else {
            writeIpSettings();
        }
        return haveConfig;
        #endif
    }

    ipHash = 0;
    return false;
}

void writeIpSettings()
{
    if(!haveFS && !FlashROMode)
        return;

    if(!*ipsettings.ip)
        return;

    uint32_t nh = calcHash((uint8_t *)&ipsettings, sizeof(ipsettings));
    
    if(ipHash) {
        if(nh == ipHash) {
            #ifdef TC_DBG_BOOT
            Serial.printf("writeIpSettings: Not writing, hash identical (%x)\n", ipHash);
            #endif
            return;
        }
    }

    ipHash = nh;
    
    saveConfigFile(ipCfgName, (uint8_t *)&ipsettings, sizeof(ipsettings), -1);
}

void deleteIpSettings()
{
    #ifdef TC_DBG_BOOT
    Serial.println("deleteIpSettings: Deleting ip config");
    #endif

    ipHash = 0;

    if(FlashROMode) {
        SD.remove(ipCfgName);
    } else if(haveFS) {
        MYNVS.remove(ipCfgName);
    }
}

/*
 * Clock state & data
 */

uint16_t loadClockState(int16_t& yoffs)
{
    yoffs = clockState.yoffs;
    return clockState.lastYear;
}

bool saveClockState(uint16_t curYear, int16_t yearoffset)
{
    if(haveClockState &&
       (clockState.lastYear == curYear) &&
       (clockState.yoffs == yearoffset)) {
        return false; // no fs access
    }
    
    clockState.lastYear = curYear;
    clockState.yoffs    = yearoffset;
    haveClockState      = true;

    #ifdef TC_DBG_BOOT
    Serial.printf("saveClockState: Writing new data (%d %d)\n", curYear, yearoffset);
    #endif
    
    saveConfigFile(clkSCfgName, (uint8_t *)&clockState, sizeof(clockState), -1);
    return true;  // fs access
}

dateStruct *getClockDataDL(uint8_t did)
{    
    dateStruct *myDate;

    switch(did) {
    case DISP_DEST:
        myDate = &clockData.dDate;
        break;
    case DISP_LAST:
        myDate = &clockData.lDate;
        break;
    default:
        return NULL;
    }

    // See if "virgin"
    if(myDate->month > 0 && myDate->day > 0)
        return myDate;

    return NULL;
}

void getClockDataP(uint64_t& myTimeDifference, bool& myTimeDiffUp)
{
    myTimeDifference = clockData.timeDifference;
    myTimeDiffUp = !!clockData.timeDiffUp;
}

static bool saveClockData(bool force)
{
    uint32_t oldHash = clockHash;
    
    clockHash = calcHash((uint8_t *)&clockData, sizeof(clockData));
    
    if(!force && (oldHash == clockHash)) {
        #ifdef TC_DBG_BOOT
        Serial.printf("saveClockData: Data up to date, not writing (%x)\n", clockHash);
        #endif
        return false; // no fs access
    }
    
    saveConfigFile(clkCfgName, (uint8_t *)&clockData, sizeof(clockData));
    return true;  // fs access
}

void updateClockDataDL(uint8_t did, uint16_t year, uint8_t month, uint8_t day, uint8_t hour, uint8_t minute)
{
    dateStruct *myDate = (did == DISP_DEST) ? &clockData.dDate : &clockData.lDate;
    myDate->year = year;
    myDate->month = month;
    myDate->day = day;
    myDate->hour = hour;
    myDate->minute = minute;
}

bool saveClockDataDL(bool force, uint8_t did, uint16_t year, uint8_t month, uint8_t day, uint8_t hour, uint8_t minute)
{
    updateClockDataDL(did, year, month, day, hour, minute);
    return saveClockData(force);
}

void updateClockDataP()
{
    clockData.timeDifference = timeDifference;
    clockData.timeDiffUp = timeDiffUp ? 1 : 0;
}

bool saveClockDataP(bool force)
{
    updateClockDataP();
    return saveClockData(force);
}

static void loadAllClockData()
{
    // Load clock state (lastYear, yearOffset)
    if(loadConfigFile(clkSCfgName, (uint8_t *)&clockState, sizeof(clockState), clkSValidBytes, -1)) {
        haveClockState = true;
        #ifdef TC_DBG_BOOT
        Serial.printf("loadClockData: Loaded clock state from %s (%d %d)\n", clkSCfgName, clockState.lastYear, clockState.yoffs);
        #endif
    } else {
        #ifdef SETTINGS_TRANSITION
        const char *fnLastYear   = "/tcdly";
        const char *fnLastYearSD = "/tcdly.bin";
        uint8_t loadBuf[8];
        if(FlashROMode) {
            readFileFromSD(fnLastYearSD, loadBuf, 8);
        } else {
            readFileFromFS(fnLastYear, loadBuf, 8);
        }
        if(haveFS) MYNVS.remove(fnLastYear);
        if(haveSD) SD.remove(fnLastYearSD);
        uint16_t curYear = 0; 
        int16_t yearoffset = 0;
        if( (loadBuf[0] == (loadBuf[2] ^ 0xff)) &&
            (loadBuf[1] == (loadBuf[3] ^ 0xff)) ) {
            if( (loadBuf[4] == (loadBuf[6] ^ 0x55)) &&
                (loadBuf[5] == (loadBuf[7] ^ 0x55)) ) {
                yearoffset = (int16_t)((loadBuf[5] << 8) | loadBuf[4]);
            }
            curYear = (int16_t)((loadBuf[1] << 8) | loadBuf[0]);
            #ifdef TC_DBG_BOOT
            Serial.printf("loadClockData: Loading old state (%d %d)\n", curYear, yearoffset);
            #endif
        }
        saveClockState(curYear, yearoffset);
        #endif // SETTINGS_TRANSITION
    }

    // Load display-specific clock data
    memset((void *)&clockData, 0, sizeof(clockData));
    if(loadConfigFile(clkCfgName, (uint8_t *)&clockData, sizeof(clockData), clkValidBytes)) {
        clockHash = calcHash((uint8_t *)&clockData, sizeof(clockData));
        #ifdef TC_DBG_BOOT
        Serial.printf("loadClockData: Loaded clockdata from %s\n", clkCfgName);
        #endif
    } else {
        #ifdef SETTINGS_TRANSITION
        const char *fnEEPROM[3] = { "/tcddt",     "/tcdpt",     "/tcdlt" };
        const char *fnSD[3] =     { "/tcddt.bin", "/tcdpt.bin", "/tcdlt.bin" };
        uint8_t loadBuf[10];
        for(int i = 0; i < 3; i++) {
            memset((void *)loadBuf, 0, sizeof(loadBuf));
            if(configOnSD) {
                readFileFromSD(fnSD[i], loadBuf, 10);
            } else {
                readFileFromFS(fnEEPROM[i], loadBuf, 10);
            }
            #ifdef TC_DBG_BOOT
            Serial.printf("loadClockData: Deleting %s[.bin]\n", fnSD[i]);
            #endif
            if(haveSD) SD.remove(fnSD[i]);
            if(haveFS) MYNVS.remove(fnEEPROM[i]);
            uint16_t sum = 0;
            for(int j = 0; j < 9; j++) {
                sum += (loadBuf[j] ^ 0x55);
            }
            if((sum & 0xff) == loadBuf[9]) {
                switch(i) {
                case DISP_DEST:
                case DISP_LAST:
                    {
                        dateStruct *myDate = (i == DISP_DEST) ? &clockData.dDate : &clockData.lDate;
                        myDate->year   = (loadBuf[1] << 8) | loadBuf[0];
                        myDate->month  = loadBuf[4];
                        myDate->day    = loadBuf[5];
                        myDate->hour   = loadBuf[6];
                        myDate->minute = loadBuf[7];
                    }
                    break;
                case DISP_PRES:
                    clockData.timeDifference = 
                                     ((uint64_t)loadBuf[3] << 32) |  // Dumb casts for
                                     ((uint64_t)loadBuf[4] << 24) |  // silencing compiler
                                     ((uint64_t)loadBuf[5] << 16) |
                                     ((uint64_t)loadBuf[6] <<  8) |
                                      (uint64_t)loadBuf[7];
                    clockData.timeDiffUp = loadBuf[8] ? 1 : 0;
                    break;
                }
            } else {
                #ifdef TC_DBG_BOOT
                Serial.printf("loadClockData: old data - invalid checksum disp %d: %d <> %d\n", i, sum & 0xff, loadBuf[9]);
                #endif
            }
        }
        saveClockData(true);
        #endif // SETTINGS_TRANSITION
    }
}

/*
 * Sound pack installer
 *
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
               (!memcmp(dbuf+5, rspv, 4))            &&
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

    if(!allowCPA) {
        delIDfile = false;
        return true;
    }

    start_file_copy();

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

static bool dfile_open(File& file, const char *fn, const char *md)
{
    if(FlashROMode) {
        return (file = SD.open(fn, md));
    }
    return (file = MYNVS.open(fn, md));
}

static void cfc(File& sfile, int& haveErr, int& haveWriteErr)
{
    #ifdef TC_DBG_BOOT
    const char *funcName = "cfc";
    #endif
    uint8_t buf1[1+32+4];
    uint8_t buf2[1024];
    File dfile;

    buf1[0] = '/';
    sfile.read(buf1 + 1, 32+4);   
    if((dfile_open(dfile, (const char *)(*r)(buf1 + 1, soa, 32) - 1, FILE_WRITE))) {
        uint32_t s = getuint32(buf1 + 1 + 32), t = 1024;
        #ifdef TC_DBG_BOOT
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
        if(!MYNVS.exists(fn))
            return false;
        if(!(file = MYNVS.open(fn, FILE_READ)))
            return false;
    }

    file.read(buf, 4);
    file.close();

    if(!FlashROMode) {
        alienVER = (memcmp(buf, rspv, 2) && memcmp(buf, SND_NON_ALIEN, 2)) ? 1 : 0;
    }

    return (!memcmp(buf, rspv, 4));
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

static bool formatFlashFS(bool userSignal)
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

    MYNVS.format();
    if(MYNVS.begin()) ret = true;

    if(userSignal) {
        destinationTime.showTextDirect("");
    }

    return ret;
}

/*
 * Re-format flash FS and write back all settings.
 * Used during audio file installation when flash FS needs
 * to be re-formatted.
 * Is never called in FlashROmode.
 * Needs a reboot afterwards!
 */
void reInstallFlashFS()
{
    // Re-load clockdata from NVS
    if(!configOnSD) {
        memset((void *)&clockData, 0, sizeof(clockData));
        loadConfigFile(clkCfgName, (uint8_t *)&clockData, sizeof(clockData), clkValidBytes);
    }

    // Format partition
    formatFlashFS(false);

    // Rewrite all settings residing in NVS
    #ifdef TC_DBG_BOOT
    Serial.println("Re-writing main, ip settings and clockstate");
    #endif
    haveClockState = false;
    presentTime.saveClockStateData(lastYear);
    
    mainConfigHash = 0;
    write_settings();

    ipHash = 0;
    writeIpSettings();

    if(!configOnSD) {
        #ifdef TC_DBG_BOOT
        Serial.println("Re-writing clockdata and secondary settings");
        #endif
        saveConfigFile(clkCfgName, (uint8_t *)&clockData, sizeof(clockData));
        saveSecSettings(false);
    }
}

/* 
 * Move settings from/to SD if user changed "save to SD"-option in CP
 * Needs a reboot afterwards!
 */
void moveSettings()
{       
    if(!haveSD || !haveFS) 
        return;

    if(configOnSD && FlashROMode) {
        #ifdef TC_DBG_BOOT
        Serial.println("moveSettings: Writing to flash prohibted (FlashROMode), aborting.");
        #endif
    }

    // Flush pending saves
    flushDelayedSave();

    // Re-load genuine clockdata
    memset((void *)&clockData, 0, sizeof(clockData));
    loadConfigFile(clkCfgName, (uint8_t *)&clockData, sizeof(clockData), clkValidBytes);
    
    configOnSD = !configOnSD;
    
    #ifdef TC_DBG_BOOT
    Serial.printf("moveSettings: Storing secondary settings %s\n", configOnSD ? "on SD" : "in Flash FS");
    #endif
    saveConfigFile(clkCfgName, (uint8_t *)&clockData, sizeof(clockData));
    saveSecSettings(false);

    configOnSD = !configOnSD;

    if(configOnSD) {
        SD.remove(clkCfgName);
        SD.remove(secCfgName);
    } else {
        MYNVS.remove(clkCfgName);
        MYNVS.remove(secCfgName);
    }
}

/*
 * Helpers for JSON config files
 */
static DeserializationError readJSONCfgFile(JsonDocument& json, File& configFile, uint32_t *readHash)
{
    const char *buf = NULL;   // const to avoid ArduinoJSON's "zero-copy mode".
    size_t bufSize = configFile.size();
    DeserializationError ret;

    if(!(buf = (const char *)malloc(bufSize + 1))) {
        Serial.printf("rJSON: malloc failed (%d)\n", bufSize);
        return DeserializationError::NoMemory;
    }

    memset((void *)buf, 0, bufSize + 1);

    configFile.read((uint8_t *)buf, bufSize);

    #ifdef TC_DBG_BOOT
    Serial.println(buf);
    #endif

    if(readHash) {
        *readHash = calcHash((uint8_t *)buf, bufSize);
    }
    
    ret = deserializeJson(json, buf);

    free((void *)buf);

    return ret;
}

static bool writeJSONCfgFile(const JsonDocument& json, const char *fn, bool useSD, uint32_t oldHash, uint32_t *newHash)
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

    #ifdef TC_DBG_BOOT
    Serial.printf("Writing %s to %s, %d bytes\n", fn, useSD ? "SD" : "FS", bufSize);
    Serial.println((const char *)buf);
    #endif

    if(oldHash || newHash) {
        uint32_t newH = calcHash((uint8_t *)buf, bufSize);
        
        if(newHash) *newHash = newH;
    
        if(oldHash) {
            if(oldHash == newH) {
                #ifdef TC_DBG_BOOT
                Serial.printf("Not writing %s, hash identical (%x)\n", fn, oldHash);
                #endif
                free(buf);
                return true;
            }
        }
    }

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
 * Generic file readers/writers
 */

static bool readFile(File& myFile, uint8_t *buf, int len)
{
    if(myFile) {
        size_t bytesr = myFile.read(buf, len);
        myFile.close();
        return (bytesr == len);
    } else
        return false;
}

static bool readFileU(File& myFile, uint8_t*& buf, int& len)
{
    if(myFile) {
        len = myFile.size();
        buf = (uint8_t *)malloc(len+1);
        if(buf) {
            buf[len] = 0;
            return readFile(myFile, buf, len);
        } else {
            myFile.close();
        }
    }
    return false;
}

// Read file of unknown size from SD
static bool readFileFromSDU(const char *fn, uint8_t*& buf, int& len)
{   
    if(!haveSD)
        return false;

    File myFile = SD.open(fn, FILE_READ);
    return readFileU(myFile, buf, len);
}

// Read file of unknown size from NVS
static bool readFileFromFSU(const char *fn, uint8_t*& buf, int& len)
{   
    if(!haveFS || !MYNVS.exists(fn))
        return false;

    File myFile = MYNVS.open(fn, FILE_READ);
    return readFileU(myFile, buf, len);
}

// Read file of known size from SD
static bool readFileFromSD(const char *fn, uint8_t *buf, int len)
{   
    if(!haveSD)
        return false;

    File myFile = SD.open(fn, FILE_READ);
    return readFile(myFile, buf, len);
}

// Read file of known size from NVS
static bool readFileFromFS(const char *fn, uint8_t *buf, int len)
{
    if(!haveFS || !MYNVS.exists(fn))
        return false;

    File myFile = MYNVS.open(fn, FILE_READ);
    return readFile(myFile, buf, len);
}

static bool writeFile(File& myFile, uint8_t *buf, int len)
{
    if(myFile) {
        size_t bytesw = myFile.write(buf, len);
        myFile.close();
        return (bytesw == len);
    } else
        return false;
}

// Write file to SD
static bool writeFileToSD(const char *fn, uint8_t *buf, int len)
{
    if(!haveSD)
        return false;

    File myFile = SD.open(fn, FILE_WRITE);
    return writeFile(myFile, buf, len);
}

// Write file to NVS
static bool writeFileToFS(const char *fn, uint8_t *buf, int len)
{
    if(!haveFS)
        return false;

    File myFile = MYNVS.open(fn, FILE_WRITE);
    return writeFile(myFile, buf, len);
}

static uint8_t cfChkSum(const uint8_t *buf, int len)
{
    uint16_t s = 0;
    while(len--) {
        s += *buf++;
    }
    s = (s >> 8) + (s & 0xff);
    s += (s >> 8);
    return (uint8_t)(~s);
}

static bool loadConfigFile(const char *fn, uint8_t *buf, int len, int& validBytes, int forcefs)
{
    bool haveConfigFile = false;
    int fl;
    uint8_t *bbuf = NULL;

    // forcefs: > 0: SD only; = 0 either (configOnSD); < 0: Flash if !FlashROMode, SD if FlashROMode

    if(haveSD && ((!forcefs && configOnSD) || forcefs > 0 || (forcefs < 0 && FlashROMode))) {
        haveConfigFile = readFileFromSDU(fn, bbuf, fl);
    }
    if(!haveConfigFile && haveFS && (!forcefs || (forcefs < 0 && !FlashROMode))) {
        haveConfigFile = readFileFromFSU(fn, bbuf, fl);
    }
    if(haveConfigFile) {
        uint8_t chksum = cfChkSum(bbuf, fl - 1);
        if((haveConfigFile = (bbuf[fl - 1] == chksum))) {
            validBytes = bbuf[0] | (bbuf[1] << 8);
            memcpy(buf, bbuf + 2, min(len, validBytes));
            haveConfigFile = true; //(len <= validBytes);
            #ifdef TC_DBG_BOOT
            Serial.printf("loadConfigFile: loaded %s: need %d, got %d bytes: ", fn, len, validBytes);
            for(int k = 0; k < len; k++) Serial.printf("%02x ", buf[k]);
            Serial.printf("chksum %02x\n", chksum);
            #endif
        } else {
            #ifdef TC_DBG_BOOT
            Serial.printf("loadConfigFile: Bad checksum %02x %02x\n", chksum, bbuf[fl - 1]);
            #endif
        }
    }

    if(bbuf) free(bbuf);

    return haveConfigFile;
}

static bool saveConfigFile(const char *fn, uint8_t *buf, int len, int forcefs)
{
    uint8_t *bbuf;
    bool ret = false;

    if(!(bbuf = (uint8_t *)malloc(len + 3)))
        return false;

    bbuf[0] = len & 0xff;
    bbuf[1] = len >> 8;
    memcpy(bbuf + 2, buf, len);
    bbuf[len + 2] = cfChkSum(bbuf, len + 2);
    
    #ifdef TC_DBG_BOOT
    Serial.printf("saveConfigFile: %s: ", fn);
    for(int k = 0; k < len + 3; k++) Serial.printf("0x%02x ", bbuf[k]);
    Serial.println("");
    #endif

    if((!forcefs && configOnSD) || forcefs > 0 || (forcefs < 0 && FlashROMode)) {
        ret = writeFileToSD(fn, bbuf, len + 3);
    } else if(haveFS) {
        ret = writeFileToFS(fn, bbuf, len + 3);
    }

    free(bbuf);

    return ret;
}

static uint32_t calcHash(uint8_t *buf, int len)
{
    uint32_t hash = 2166136261UL;
    for(int i = 0; i < len; i++) {
        hash = (hash ^ buf[i]) * 16777619;
    }
    return hash;
}

static bool saveSecSettings(bool useCache)
{
    uint32_t oldHash = secSettingsHash;

    secSettingsHash = calcHash((uint8_t *)&secSettings, sizeof(secSettings));
    
    if(useCache) {
        if(oldHash == secSettingsHash) {
            #ifdef TC_DBG_BOOT
            Serial.printf("saveSecSettings: Data up to date, not writing (%x)\n", secSettingsHash);
            #endif
            return true;
        }
    }
    
    return saveConfigFile(secCfgName, (uint8_t *)&secSettings, sizeof(secSettings), 0);
}

static bool saveTerSettings(bool useCache)
{
    if(!haveSD)
        return false;

    uint32_t oldHash = terSettingsHash;
    
    terSettingsHash = calcHash((uint8_t *)&terSettings, sizeof(terSettings));
    
    if(useCache) {
        if(oldHash == terSettingsHash) {
            #ifdef TC_DBG_BOOT
            Serial.printf("saveTerSettings: Data up to date, not writing (%x)\n", terSettingsHash);
            #endif
            return true;
        }
    }
    
    return saveConfigFile(terCfgName, (uint8_t *)&terSettings, sizeof(terSettings), 1);
}

#ifdef SETTINGS_TRANSITION
static void removeOldFiles(const char *oldfn)
{
    if(haveSD) SD.remove(oldfn);
    if(haveFS) MYNVS.remove(oldfn);
    #ifdef TC_DBG_BOOT
    Serial.printf("removeOldFiles: Removing %s\n", oldfn);
    #endif
}
#endif

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

                #ifdef TC_DBG_BOOT
                char t = uploadFileName[8];
                #endif
                
                uploadFileName[8] = '/';
                
                #ifdef TC_DBG_BOOT
                Serial.printf("openUploadFile: Deleting %s\n", uploadFileName+8);
                #endif
                
                SD.remove(uploadFileName+8);
                
                #ifdef TC_DBG_BOOT
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

        #ifdef TC_DBG_BOOT
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
            #ifdef TC_DBG_BOOT
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
        
        #ifdef TC_DBG_BOOT
        Serial.printf("renameUploadFile [1]: Deleting %s\n", t);
        #endif
        
        SD.remove(t);
        
        #ifdef TC_DBG_BOOT
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
