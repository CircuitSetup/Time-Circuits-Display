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
 * License: Modified MIT NON-AI
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
 * Links inside the Software pointing to the original source must not 
 * be changed or removed.
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
#define ARDUINOJSON_ENABLE_ARDUINO_PRINT 0
#define ARDUINOJSON_DECODE_UNICODE 0
#define ARDUINOJSON_ENABLE_STD_STREAM 0
#define ARDUINOJSON_ENABLE_STD_STRING 0
#define ARDUINOJSON_ENABLE_NAN 0
#define ARDUINOJSON_ENABLE_INFINITY 0
#include <ArduinoJson.h>  // https://github.com/bblanchon/ArduinoJson
#include "src/SD/SD.h"
#include <SPI.h>
#include <FS.h>
#define MYNVS LittleFS
#include <LittleFS.h>
#include <Update.h>

#include "tc_settings.h"
#include "tc_audio.h"
#include "tc_main.h"
#include "tc_wifi.h"

// SPI speed for SD. We used 16000000 in the past,
// but since we have short traces and likely no
// extender, we go a bit higher now.
// 25000000 is max for SD, 20000000 is max for MMC
// SD-code automatically limits according to card type
#define SD_SPI_FREQ 20000000

// Settings upgrade, stage 2: Assume new settings
//  are present, but still delete obsolete files.
#define SETTINGS_TRANSITION_2

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

#define NUM_AUDIOFILES 26
#define AC_FMTV 2
#define AC_OHSZ (14 + ((NUM_AUDIOFILES+1)*(32+4)))
#ifdef CS_EDITION
#define SND_REQ_VERSION "CS09"
#define AC_TS 17687967
#define SND_NON_ALIEN "TW"
#else
#define SND_REQ_VERSION "TW09"
#define AC_TS 17682952
#define SND_NON_ALIEN "CS"
#endif

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
    uint8_t remMonth        = 0;
    uint8_t remDay          = 0;
    uint8_t remHour         = 0;
    uint8_t remMin          = 0;
    uint8_t carMode         = 0;
    uint8_t useLineOut      = 0;
    uint8_t remoteAllowed   = 0;
    uint8_t remoteKPAllowed = 0;
    int8_t  scorr           = 0;
    int8_t  tcorr           = 0;
    uint8_t exhOnOff        = 0;
    uint8_t showUpdAvail    = 1;
    dateStruct exhDates[2]; // initialized to default in main_boot()
    uint8_t updateV         = 0;
    uint8_t updateR         = 0;
    uint8_t beepLvlIdx      = 2;
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
    uint8_t    unused1        = 0;
    dateStruct dUDate         = { 0, 0, 0, 0, 0 };
    dateStruct lUDate         = { 0, 0, 0, 0, 0 };
} clockData;

static int      clkValidBytes  = 0;
static uint32_t clockHash      = 0;
static bool     haveClockState = false;

static uint32_t mainConfigHash = 0;
static uint32_t ipHash = 0;

static const char *CONFN  = "/TCDA.bin";
static const char *CONFND = "/TCDA.old";
static const char *CONID  = "TCDA";
const  char       rspv[]  = SND_REQ_VERSION;
static uint32_t   soa = AC_TS;
static bool       ic = false;
static uint8_t*   f(uint8_t *d, uint32_t m, int y) { return d; }
static char       *uploadFileNames[MAX_SIM_UPLOADS] = { NULL };
static char       *uploadRealFileNames[MAX_SIM_UPLOADS] = { NULL };

static const char *cfgName     = "/config.json"; // Main config (flash)
static const char *ipCfgName   = "/tcdipcfg";    // IP config (flash)
static const char *clkSCfgName = "/tcdcscfg";    // Clock state (flash)
static const char *clkCfgName  = "/tcdckcfg";    // Clock data (flash/SD)
static const char *secCfgName  = "/tcd2cfg";     // Secondary settings (flash/SD)
static const char *terCfgName  = "/tcd3cfg";     // Tertiary settings (SD)

#ifdef SETTINGS_TRANSITION_2
static const char *obsFiles[] = {
    "/ipconfig.json",   // Skipped for SD iteration
    "/beep.mp3",        // Skipped for SD iteration
    "/tcdly", "/tcddt", "/tcdpt", "/tcdlt",
    "/tcdbricfg.json", "/tcdaicfg.json", "/tcdvolcfg.json", "/tcdalmcfg.json", 
    "/tcdremcfg.json", "/stconfig",      "/scconfig.json",  "/cmconfig.json",
    "/loconfig,json",  "/raconfig.json", "/tcdmcfg.json",
    NULL
};
#endif

static const char fwfn[]      = "/tcdfw.bin";
static const char fwfnold[]   = "/tcdfw.old";

static const char *fsNoAvail     = "File System not available";
static const char *failFileWrite = "Failed to open file for writing";
#ifdef TC_DBG_BOOT
static const char *badConfig     = "Settings bad/missing/incomplete; writing new file";
#endif

#ifdef HAVE_MQTT
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
unsigned int musFolderNum = 0;

int sspeedopin = 0;
int stachopin = 0;

static void loadBrightness();
static void loadBeepAutoInterval();
static void loadCarMode();
#ifdef HAVE_REMOTE
static void loadRemoteAllowed();
#endif
static void loadUpdAvail();

uint16_t    loadClockState(int16_t& yoffs);
bool        saveClockState(uint16_t curYear, int16_t yearoffset);
dateStruct *getClockDataDL(unsigned int did, int slot = 0);
void        getClockDataP(uint64_t& timeDifference, bool &timeDiffUp);
void        updateClockDataDL(unsigned int did, int slot, dateStruct *givenDate);
bool        saveClockDataDL(bool force, unsigned int did, dateStruct *givenDate);
void        updateClockDataP();
bool        saveClockDataP(bool force);
static void loadAllClockData();
static uint8_t* (*r)(uint8_t *, uint32_t, int);

static bool audio_files_present(int& alienVER);

static void firmware_update();

// Helpers from tc_keypad
extern void start_file_copy();
extern void file_copy_progress(uint32_t ts, uint32_t tw);
extern void file_copy_done(int err);

#ifdef HAVE_MQTT
static void preAllocMQTTTopMsg()
{
    for(int i = 0; i < 10; i++) {
        if(settings.mqmt[i]) free(settings.mqmt[i]);
        if((settings.mqmt[i] = (char *)malloc(128))) {
            memset(settings.mqmt[i], 0, 128);
        }
        if(settings.mqmm[i]) free(settings.mqmm[i]);
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
                settings.mqmt[i] = NULL;
                #ifdef TC_DBG_BOOT
                Serial.printf("MQTT: Freeing topic %d\n", i);
                #endif
            }
        }
        if(settings.mqmm[i]) {
            if(!*settings.mqmm[i]) {
                free(settings.mqmm[i]);
                settings.mqmm[i] = NULL;
                #ifdef TC_DBG_BOOT
                Serial.printf("MQTT: Freeing msg %d\n", i);
                #endif
            }
        }
    }
}
#endif

/*
 * Format Flash FS
 */

static bool formatFlashFS(bool userSignal)
{
    bool ret = false;

    if(userSignal) {
        // Show the user some action
        destinationTime.showTextDirect("WAIT");
    } else {
        #ifdef TC_DBG_BOOT
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
 * Unmount filesystems
 */

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


/*
 * Generic file readers/writers
 */

void deleteFileFromSD(const char *fn)
{
    if(haveSD) {
        SD.remove(fn);
    }
}

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
        if((len = myFile.size())) {
            buf = (uint8_t *)malloc(len+1);
            if(buf) {
                buf[len] = 0;
                return readFile(myFile, buf, len);
            }
        }
        myFile.close();
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
bool readFileFromSD(const char *fn, uint8_t *buf, int len)
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
bool writeFileToSD(const char *fn, uint8_t *buf, int len)
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

/*
 * Config file handling
 */

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

static bool loadConfigFile(const char *fn, uint8_t *buf, int len, int& validBytes, int forcefs = 0)
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
    if(haveConfigFile && (fl < 2)) haveConfigFile = false;
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

static bool saveConfigFile(const char *fn, uint8_t *buf, int len, int forcefs = 0)
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

/*
 * Helpers for JSON config files
 */

static DeserializationError readJSONCfgFile(JsonDocument& json, File& configFile, uint32_t *readHash = NULL)
{
    const char *buf = NULL;   // const to avoid ArduinoJSON's "zero-copy mode".
    size_t bufSize = configFile.size();
    DeserializationError ret;

    if(!bufSize)
        return DeserializationError::InvalidInput;

    if(!(buf = (const char *)malloc(bufSize + 1))) {
        #ifdef TC_DBG_BOOT
        Serial.printf("rJSON: malloc failed (%d)\n", bufSize);
        #endif
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

static bool writeJSONCfgFile(const JsonDocument& json, const char *fn, bool useSD, uint32_t oldHash = 0, uint32_t *newHash = NULL)
{
    char *buf;
    size_t bufSize = measureJson(json);
    bool success = false;

    if(!(buf = (char *)malloc(bufSize + 1))) {
        #ifdef TC_DBG_BOOT
        Serial.printf("wJSON: malloc failed (%d) (%s)\n", bufSize, fn);
        #endif
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

    #ifdef TC_DBG_BOOT
    if(!success) {
        Serial.printf("wJSON: %s - %s\n", fn, failFileWrite);
    }
    #endif

    return success;
}

/*
 *  Helpers for parm copying & checking
 */

static bool CopyTextParm(const char *json, char *setting, int setSize)
{
    if(!json) return true;
    
    memset(setting, 0, setSize);
    strncpy(setting, json, setSize - 1);
    return false;
}

static bool CopyCBParm(const char *json, char *text, int setDefault)
{
    text[1] = 0;
    
    if(json && (*json == '0' || *json == '1')) {
        *text = *json;
        return false;
    }

    *text = setDefault ? '1' : '0';

    return true;
}

static bool CopyCheckValidNumParm(const char *json, char *text, int lowerLim, int upperLim, int setDefault)
{
    bool ret = true;
    int t = setDefault;

    if(json) {
        int u = atoi(json);    
        if(u >= lowerLim && u <= upperLim) {
            t = u;
            ret = false;
        }
    }

    // Re-do to get rid of formatting errors (eg "000")
    sprintf(text, "%d", t);

    return ret;
}

static bool CopyCheckValidNumParmF(const char *json, char *text, float lowerLim, float upperLim, float setDefault)
{
    bool ret = true;
    float t = setDefault;

    if(json) {
        float u = strtof(json, NULL);
        if(u >= lowerLim && u <= upperLim) {
            t = u;
            ret = false;
        }
    }

    // Re-do to get rid of formatting errors (eg "0.")
    sprintf(text, "%.1f", t);

    return ret;
}

bool evalBool(char *s)
{
    if(*s == '0') return false;
    return true;
}

bool evalBoolSetClear(char *s, uint32_t& ff, uint32_t fl)
{
    if(*s == '0') {
        ff &= ~fl;
        return false;
    }
    ff |= fl;
    return true;
}

void clearWiFiCredentials()
{
    memset(settings.ssid, 0, sizeof(settings.ssid));
    memset(settings.pass, 0, sizeof(settings.pass));
    memset(settings.bssid, 0, sizeof(settings.bssid));
}

static bool read_settings(File configFile, int cfgReadCount)
{
    #ifdef TC_DBG_BOOT
    const char *funcName = "read_settings";
    #endif
    bool wd = false;
    size_t jsonSize = 0;
    
    DECLARE_D_JSON(JSON_SIZE,json);

    if(!readJSONCfgFile(json, configFile, &mainConfigHash)) {

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

        // WiFi Configuration

        if(!cfgReadCount) {
            clearWiFiCredentials();
        }

        if(json["ssid"]) {
            clearWiFiCredentials();
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

        wd |= CopyTextParm(json["hn"], settings.hostName, sizeof(settings.hostName));
        
        wd |= CopyCheckValidNumParm(json["wCR"], settings.wifiConRetries, 1, 10, DEF_WIFI_RETRY);
        wd |= CopyCBParm(json["wPR"], settings.wifiPRetry, DEF_WIFI_PRETRY);
        wd |= CopyCheckValidNumParm(json["wOD"], settings.wifiOffDelay, 0, 99, DEF_WIFI_OFFDELAY);

        wd |= CopyTextParm(json["sID"], settings.systemID, sizeof(settings.systemID));
        wd |= CopyTextParm(json["appw"], settings.appw, sizeof(settings.appw));
        wd |= CopyCheckValidNumParm(json["apch"], settings.apChnl, 0, 11, DEF_AP_CHANNEL);
        wd |= CopyCheckValidNumParm(json["wAOD"], settings.wifiAPOffDelay, 0, 99, DEF_WIFI_APOFFDELAY);

        // Settings
        
        wd |= CopyCBParm(json["pI"], settings.playIntro, DEF_PLAY_INTRO);
        // Beep, Time cycling saved in separate file
        wd |= CopyCBParm(json["sARA"], settings.autoRotAnim, 1);
        wd |= CopyCBParm(json["skpTTA"], settings.skipTTAnim, DEF_SKIP_TTANIM);
        #ifndef ACAR_DISPLAY
        wd |= CopyCBParm(json["p3an"], settings.p3anim, DEF_P3ANIM);
        #endif
        wd |= CopyCBParm(json["pTTs"], settings.playTTsnds, DEF_PLAY_TT_SND);
        wd |= CopyCBParm(json["pTOTH"], settings.sayTOTH, DEF_PLAY_TOTH); 
        wd |= CopyCBParm(json["alRTC"], settings.alarmRTC, DEF_ALARM_RTC);
        wd |= CopyCBParm(json["md24"], settings.mode24, DEF_MODE24);
        
        wd |= CopyTextParm(json["tZ"], settings.timeZone, sizeof(settings.timeZone));
        wd |= CopyTextParm(json["ntpS"], settings.ntpServer, sizeof(settings.ntpServer));
        #ifdef HAVE_GPS
        wd |= CopyCBParm(json["gTme"], settings.useGPSTime, DEF_USE_GPS_TIME);
        #endif

        wd |= CopyTextParm(json["tZDest"], settings.timeZoneDest, sizeof(settings.timeZoneDest));
        wd |= CopyTextParm(json["tZDep"], settings.timeZoneDep, sizeof(settings.timeZoneDep));
        wd |= CopyTextParm(json["tZNDest"], settings.timeZoneNDest, sizeof(settings.timeZoneNDest));
        wd |= CopyTextParm(json["tZNDep"], settings.timeZoneNDep, sizeof(settings.timeZoneNDep));
        wd |= CopyCBParm(json["WCNP"], settings.WCNamePerm, DEF_WCSHOWNAME);

        wd |= CopyCheckValidNumParm(json["almT"], settings.alarmType, 0, 1, DEF_ALARM_TYPE);
        wd |= CopyCBParm(json["aSz"], settings.doSnooze, DEF_SNOOZE);
        wd |= CopyCheckValidNumParm(json["aSzT"], settings.snoozeTime, 1, 15, DEF_SNOOZE_TIME);
        wd |= CopyCBParm(json["aASz"], settings.autoSnooze, DEF_ASNOOZE);
        wd |= CopyCBParm(json["aLU"], settings.almLoopUserSnd, DEF_LOOP_USER_SND);
        
        wd |= CopyCBParm(json["dtNmOff"], settings.dtNmOff, DEF_DT_OFF);
        wd |= CopyCBParm(json["ptNmOff"], settings.ptNmOff, DEF_PT_OFF);
        wd |= CopyCBParm(json["ltNmOff"], settings.ltNmOff, DEF_LT_OFF);
        wd |= CopyCheckValidNumParm(json["aNMPre"], settings.autoNMPreset, 0, 10, DEF_AUTONM_PRESET);
        wd |= CopyCheckValidNumParm(json["aNMOn"], settings.autoNMOn, 0, 23, DEF_AUTONM_ON);
        wd |= CopyCheckValidNumParm(json["aNMOff"], settings.autoNMOff, 0, 23, DEF_AUTONM_OFF);
        #ifdef HAVE_LIGHT
        wd |= CopyCBParm(json["uLgt"], settings.useLight, DEF_USE_LIGHT);
        wd |= CopyCheckValidNumParm(json["lxLim"], settings.luxLimit, 0, 50000, DEF_LUX_LIMIT);
        #endif

        wd |= CopyCBParm(json["CoSD"], settings.CfgOnSD, DEF_CFG_ON_SD);
        wd |= CopyCBParm(json["ttps"], settings.timesPers, DEF_TIMES_PERS);

        #ifdef ACAR_DISPLAY
        wd |= CopyCBParm(json["swapDL"],settings.swapDL, DEF_SWPDL);
        #endif
        wd |= CopyCBParm(json["rAPM"], settings.revAmPm, DEF_REVAMPM);

        wd |= CopyCBParm(json["fPwr"], settings.fakePwrOn, DEF_FAKE_PWR);

        #ifdef HAVE_TEMP
        wd |= CopyCBParm(json["tmpU"], settings.tempUnit, DEF_TEMP_UNIT);
        wd |= CopyCheckValidNumParmF(json["tmpOf"], settings.tempOffs, -3.0f, 3.0f, DEF_TEMP_OFFS);
        #endif

        wd |= CopyCheckValidNumParm(json["spT"], settings.speedoType, 0, 99, DEF_SPEEDO_TYPE);
        wd |= CopyCheckValidNumParm(json["spB"], settings.speedoBright, 0, 15, DEF_BRIGHT_SPEEDO);
        wd |= CopyCBParm(json["spAO"], settings.speedoAO, DEF_SPEEDO_AO);
        wd |= CopyCBParm(json["spAF"], settings.speedoAF, DEF_SPEEDO_ACCELFIG);
        wd |= CopyCheckValidNumParmF(json["spFc"], settings.speedoFact, 0.5f, 5.0f, DEF_SPEEDO_FACT);
        wd |= CopyCBParm(json["spP3"], settings.speedoP3, DEF_SPEEDO_P3);
        wd |= CopyCBParm(json["spP3R"], settings.speedoP3R, 0);
        wd |= CopyCBParm(json["spd3rd"], settings.speedo3rdD, DEF_SPEEDO_3RDD);
        #ifdef HAVE_GPS
        wd |= CopyCBParm(json["uGPSS"], settings.dispGPSSpeed, DEF_USE_GPS_SPEED);
        wd |= CopyCheckValidNumParm(json["spUR"], settings.spdUpdRate, 0, 3, DEF_SPD_UPD_RATE);
        #endif
        #ifdef HAVE_TEMP
        wd |= CopyCBParm(json["dTmp"], settings.dispTemp, DEF_DISP_TEMP);
        wd |= CopyCheckValidNumParm(json["tmpB"], settings.tempBright, 0, 15, DEF_TEMP_BRIGHT);
        wd |= CopyCBParm(json["tmpONM"], settings.tempOffNM, DEF_TEMP_OFF_NM);
        #endif

        wd |= CopyCheckValidNumParm(json["ettDl"], settings.ettDelay, 0, ETT_MAX_DEL, DEF_ETT_DELAY);
        
        wd |= CopyCBParm(json["ETTOc"], settings.ETTOcmd, DEF_ETTO_CMD);
        wd |= CopyCBParm(json["ETTOPU"], settings.ETTOpus, DEF_ETTO_PUS);
        wd |= CopyCBParm(json["uETTO"], settings.useETTO, DEF_USE_ETTO);
        wd |= CopyCBParm(json["nETTOL"], settings.noETTOLead, DEF_NO_ETTO_LEAD);
        wd |= CopyCBParm(json["ETTOa"], settings.ETTOalm, DEF_ETTO_ALM);
        wd |= CopyCheckValidNumParm(json["ETTOAD"], settings.ETTOAD, 3, 99, DEF_ETTO_ALM_D);

        #ifdef SERVOSPEEDO
        wd |= CopyCheckValidNumParm(json["tin"], settings.ttinpin, 0, 2, 0);
        wd |= CopyCheckValidNumParm(json["tout"], settings.ttoutpin, 0, 2, 0);
        #endif

        #ifdef HAVE_GPS
        wd |= CopyCBParm(json["qGPS"], settings.provGPS2BTTFN, DEF_GPS4BTTFN);
        #endif
        
        #ifdef HAVE_MQTT   
        wd |= CopyCBParm(json["uMQTT"], settings.useMQTT, 0);
        wd |= CopyTextParm(json["mqttS"], settings.mqttServer, sizeof(settings.mqttServer));
        wd |= CopyCheckValidNumParm(json["mqttV"], settings.mqttVers, 0, 1, 0);
        wd |= CopyTextParm(json["mqttU"], settings.mqttUser, sizeof(settings.mqttUser));
        wd |= CopyTextParm(json["mqttT"], settings.mqttTopic, sizeof(settings.mqttTopic));
        wd |= CopyTextParm(json["mqttTP"], settings.mqttTopicP, sizeof(settings.mqttTopicP));
        wd |= CopyTextParm(json["mqttTL"], settings.mqttTopicL, sizeof(settings.mqttTopicL));
        wd |= CopyCBParm(json["pMQTT"], settings.pubMQTT, 0);
        wd |= CopyCBParm(json["vMQTT"], settings.MQTTvarLead, DEF_MQTT_VTT);
        wd |= CopyCBParm(json["aMQTT"], settings.pubMQTTAl, 0);
        wd |= CopyCBParm(json["mqP"], settings.mqttPwr, 0);
        wd |= CopyCBParm(json["mqPO"], settings.mqttPwrOn, 0);
        wd |= CopyCBParm(json["pMP"], settings.pubMP, 0);

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
    #ifndef ACAR_DISPLAY
    json["p3an"] = (const char *)settings.p3anim;
    #endif
    json["pTTs"] = (const char *)settings.playTTsnds;
    json["pTOTH"] = (const char *)settings.sayTOTH;
    json["alRTC"] = (const char *)settings.alarmRTC;
    json["md24"] = (const char *)settings.mode24;

    json["tZ"] = (const char *)settings.timeZone;
    json["ntpS"] = (const char *)settings.ntpServer;
    #ifdef HAVE_GPS
    json["gTme"] = (const char *)settings.useGPSTime;
    #endif

    json["tZDest"] = (const char *)settings.timeZoneDest;
    json["tZDep"] = (const char *)settings.timeZoneDep;
    json["tZNDest"] = (const char *)settings.timeZoneNDest;
    json["tZNDep"] = (const char *)settings.timeZoneNDep;
    json["WCNP"] = (const char *)settings.WCNamePerm;

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
    #ifdef HAVE_LIGHT
    json["uLgt"] = (const char *)settings.useLight;
    json["lxLim"] = (const char *)settings.luxLimit;
    #endif

    json["CoSD"] = (const char *)settings.CfgOnSD;
    json["ttps"] = (const char *)settings.timesPers;

    #ifdef ACAR_DISPLAY
    json["swapDL"] = (const char *)settings.swapDL;
    #endif
    json["rAPM"] = (const char *)settings.revAmPm;

    json["fPwr"] = (const char *)settings.fakePwrOn;

    json["spT"] = (const char *)settings.speedoType;
    json["spB"] = (const char *)settings.speedoBright;
    json["spAO"] = (const char *)settings.speedoAO;
    json["spAF"] = (const char *)settings.speedoAF;
    json["spFc"] = (const char *)settings.speedoFact;
    json["spP3"] = (const char *)settings.speedoP3;
    json["spP3R"] = (const char *)settings.speedoP3R;
    json["spd3rd"] = (const char *)settings.speedo3rdD;
    #ifdef HAVE_GPS
    json["uGPSS"] = (const char *)settings.dispGPSSpeed;
    json["spUR"] = (const char *)settings.spdUpdRate;
    #endif
    #ifdef HAVE_TEMP
    json["dTmp"] = (const char *)settings.dispTemp;
    json["tmpB"] = (const char *)settings.tempBright;
    json["tmpONM"] = (const char *)settings.tempOffNM;
    #endif

    #ifdef HAVE_TEMP
    json["tmpU"] = (const char *)settings.tempUnit;
    json["tmpOf"] = (const char *)settings.tempOffs;
    #endif

    json["ettDl"] = (const char *)settings.ettDelay;
    
    #ifdef HAVE_GPS
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

    #ifdef HAVE_MQTT
    json["uMQTT"] = (const char *)settings.useMQTT;
    json["mqttS"] = (const char *)settings.mqttServer;
    json["mqttV"] = (const char *)settings.mqttVers;
    json["mqttU"] = (const char *)settings.mqttUser;
    json["mqttT"] = (const char *)settings.mqttTopic;
    json["mqttTP"] = (const char *)settings.mqttTopicP;
    json["mqttTL"] = (const char *)settings.mqttTopicL;
    json["pMQTT"] = (const char *)settings.pubMQTT;
    json["vMQTT"] = (const char *)settings.MQTTvarLead;
    json["aMQTT"] = (const char *)settings.pubMQTTAl;
    json["mqP"] = (const char *)settings.mqttPwr;
    json["mqPO"] = (const char *)settings.mqttPwrOn;
    json["pMP"] = (const char *)settings.pubMP;
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

static void removeObsFiles()
{
    // Remove files that no longer should exist
    char dtmfBuf1[] = "/Dtmf-0.mp3";
    char dtmfBuf2[] = "/Dtmf-0.wav";
    if(MYNVS.exists(dtmfBuf1) || MYNVS.exists(dtmfBuf2)) {
        for(int i = 0; i < 10; i++) {
            dtmfBuf1[6] = dtmfBuf2[6] = i + '0';
            MYNVS.remove(dtmfBuf1);
            MYNVS.remove(dtmfBuf2);
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
}

/*
 * settings_setup()
 * 
 * Mount LittleFS and SD (if available).
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

    #ifdef HAVE_MQTT
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
        
        removeObsFiles();
        
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

        #ifdef TC_DBG_BOOT
        Serial.println("failed.");
        #endif
        Serial.println("*** Mounting flash FS failed. Using SD (if available)");

    }
    
    // Set up SD card
    pinMode(SD_CS_PIN, OUTPUT);
    digitalWrite(SD_CS_PIN, HIGH);
    SPI.begin(SPI_SCK_PIN, SPI_MISO_PIN, SPI_MOSI_PIN);
    delay(20);

    #ifdef TC_DBG_BOOT
    Serial.printf("%s: Mounting SD... ", funcName);
    #endif

    // Two attemps. Not really a necessity after
    // the SD init changes (jul 2026), but why not.
    if(!(haveSD = SD.begin(SD_CS_PIN, SPI, SD_SPI_FREQ))) {
        delay(20);
        haveSD = SD.begin(SD_CS_PIN, SPI, SD_SPI_FREQ);
    }

    if(haveSD) {
        uint8_t cardType = SD.cardType();
       
        #ifdef TC_DBG_BOOT
        const char *sdTypes[5] = { "No card", "MMC", "SD", "SDHC", "unknown (unusable)" };
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
            #ifdef HAVE_MQTT
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
        #ifdef TC_DBG_BOOT
        Serial.println("No SD card found");
        #endif
    }

    #ifdef HAVE_MQTT
    freeUnusedMQTTTopMsg();
    #endif

    // Check if (current) audio data is installed
    if((haveAudioFiles = audio_files_present(alienVER))) {
        // Check for TCC, result required before wifi_setup()
        checkForTCC();
    }

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
        for(int i = 2; ; i++) {
            if(!obsFiles[i]) break;
            deleteFileFromSD(obsFiles[i]);
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

    #ifdef HAVE_REMOTE
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

/*
 *  Load/save display brightness
 */

static void loadBrightness()
{
    if(haveSecSettings) {
        #ifdef TC_DBG_BOOT
        Serial.println("loadBrightness: extracting from secSettings");
        #endif
        settings.destTimeBright = secSettings.brightness[0];
        settings.presTimeBright = secSettings.brightness[1];
        settings.lastTimeBright = secSettings.brightness[2];
    }
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
        if(secSettings.curVolume == 255 || secSettings.curVolume < VOL_LEVELS) {
            aud_state.curVolume = secSettings.curVolume;
        }
        if(secSettings.beepLvlIdx <= 3) {
            beepLvlIdx = secSettings.beepLvlIdx;
        }
    }
}

void storeCurVolume()
{
    // Used to keep secSettings up-to-date in case
    // of delayed save
    secSettings.curVolume = aud_state.curVolume;
    secSettings.beepLvlIdx = beepLvlIdx;
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
            if(alarmWeekday > 9 && (!(alarmWeekday & 0x80))) alarmWeekday = 0;
        }
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
    }
}

void saveReminder()
{
    secSettings.remDay = remDay;
    if(!remDay) {
        secSettings.remMonth = 0;
        secSettings.remHour = 0;
        secSettings.remMin = 0;
    } else {
        secSettings.remMonth = remMonth;
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

#ifdef HAVE_REMOTE
static void loadRemoteAllowed()
{
    if(haveSecSettings) {
        #ifdef TC_DBG_BOOT
        Serial.println("loadRemoteAllowed: extracting from secSettings");
        #endif
        csf &= ~(CSF_REMALLOW|CSF_REMKPALLOW);
        if(secSettings.remoteAllowed)   csf |= CSF_REMALLOW;
        if(secSettings.remoteKPAllowed) csf |= CSF_REMKPALLOW;
    }
}

void saveRemoteAllowed()
{
    secSettings.remoteAllowed   = (csf & CSF_REMALLOW)   ? 1 : 0;
    secSettings.remoteKPAllowed = (csf & CSF_REMKPALLOW) ? 1 : 0;
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
    if(haveTerSettings) {
        #ifdef TC_DBG_BOOT
        Serial.println("loadMusFoldNum: extracting from terSettings");
        #endif
        if(terSettings.musFolderNum <= 9) {
            musFolderNum = terSettings.musFolderNum;
        }
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
    if(haveTerSettings) {
        aud_state.mpShuffle = terSettings.mpShuffle;
    }
}

void saveShuffle()
{
    terSettings.mpShuffle = aud_state.mpShuffle;
    saveTerSettings(true);
}

/*
 * Load/save boot display mode (RC, WC, Nav, Mini)
 */

uint8_t loadBootMode()
{
    if(haveTerSettings) {
        return terSettings.bootMode;
    }

    return 0;
}

void storeBootMode()
{
    uint8_t t = 0;
    if(isRcMode())   t |= 0x01;
    if(isWcMode())   t |= 0x02;
    #ifdef HAVE_GPS
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
        deleteFileFromSD(ipCfgName);
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

static dateStruct *getClockDataPtr(unsigned int did, int slot)
{
    switch(did) {
    case DISP_DEST:
        return slot ? &clockData.dUDate : &clockData.dDate;
    case DISP_LAST:
        return slot ? &clockData.lUDate : &clockData.lDate;
    default:
        return NULL;
    }
}

dateStruct *getClockDataDL(unsigned int did, int slot)
{    
    dateStruct *myDate = getClockDataPtr(did, slot);

    // See if "virgin"
    if(myDate && (myDate->month > 0) && (myDate->day > 0))
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

void updateClockDataDL(unsigned int did, int slot, dateStruct *givenDate)
{
    dateStruct *myDate = getClockDataPtr(did, slot);

    if(myDate) {
        memcpy((void *)myDate, (void *)givenDate, sizeof(dateStruct));
    }
}

bool saveClockDataDL(bool force, unsigned int did, dateStruct *givenDate)
{
    updateClockDataDL(did, 0, givenDate);
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
    }

    // Load display-specific clock data
    memset((void *)&clockData, 0, sizeof(clockData));
    if(loadConfigFile(clkCfgName, (uint8_t *)&clockData, sizeof(clockData), clkValidBytes)) {
        clockHash = calcHash((uint8_t *)&clockData, sizeof(clockData));
        #ifdef TC_DBG_BOOT
        Serial.printf("loadClockData: Loaded clockdata from %s\n", clkCfgName);
        #endif
    }
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
        return;
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
        deleteFileFromSD(clkCfgName);
        deleteFileFromSD(secCfgName);
    } else {
        MYNVS.remove(clkCfgName);
        MYNVS.remove(secCfgName);
    }
}


/*
 * Sound pack installer
 *
 */

static bool audio_files_present(int& alienVER)
{
    File file;
    uint8_t buf[4];
    static const char *fn = "/VER";

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

static uint32_t getuint32(uint8_t *buf)
{
    uint32_t t = 0;
    for(int i = 3; i >= 0; i--) {
        t <<= 8;
        t += buf[i];
    }
    return t;
}

static bool dfile_open(File& file, bool tSD, const char *fn, const char *md)
{
    if(tSD || FlashROMode) {
        return (file = SD.open(fn, md));
    }
    return (file = MYNVS.open(fn, md));
}

static void cfc(File& sfile, int& haveErr, int& haveWriteErr, uint32_t& tw, uint32_t ts)
{
    #ifdef TC_DBG_BOOT
    const char *funcName = "cfc";
    #endif
    uint8_t buf1[1+32+4];
    uint8_t buf2[1024];
    uint32_t s;
    bool tSD = false;
    File dfile;

    buf1[0] = '/';
    sfile.read(buf1 + 1, 32+4);
    s = getuint32((*r)(buf1 + 1, soa, 32) + 32);
    if(buf1[1] == '_') {
        tSD = true;
        strcpy((char *)&buf1[1], (char *)&buf1[2]);
    }
    if((dfile_open(dfile, tSD, (const char *)buf1, FILE_WRITE))) {
        uint32_t t = 1024;
        #ifdef TC_DBG_BOOT
        Serial.printf("%s: Opened destination file: %s, length %d\n", funcName, (const char *)buf1, s);
        #endif
        while(s > 0) {
            t = (s < t) ? s : t;
            if(sfile.read(buf2, t) != t) {
                haveErr++;
                break;
            }
            tw += t;
            if(dfile.write((*r)(buf2, soa, t), t) != t) {
                haveErr++;
                haveWriteErr++;
                break;
            }
            s -= t;
            file_copy_progress(ts, tw);
        }
    } else {
        haveErr++;
        haveWriteErr++;
        Serial.printf("Error opening destination file: %s\n", buf1);
    }
}

bool check_allow_CPA()
{
    return allowCPA;
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
            if((!memcmp(dbuf, CONID, 4))         && 
               ((*(dbuf+4) & 0x7f) == AC_FMTV)   &&
               (!memcmp(dbuf+5, rspv, 4))        &&
               (*(dbuf+9) == (NUM_AUDIOFILES+1)) &&
               (getuint32(dbuf+10) == soa)       &&
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
// Sets delIDfile to true in any case (to avoid repeated flash-hurting attempts)
bool copy_audio_files(bool& delIDfile)
{
    int i, haveErr = 0, haveWriteErr = 0;
    uint32_t tw = 0, ts;

    if(!allowCPA) {
        delIDfile = false;
        return true;
    }

    start_file_copy();

    if(ic) {
        File sfile;
        if(sfile = SD.open(CONFN, FILE_READ)) {
            ts = sfile.size();
            sfile.seek(14);
            for(i = 0; i < NUM_AUDIOFILES+1; i++) {
               cfc(sfile, haveErr, haveWriteErr, tw, ts);
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

    delIDfile = true;   // (haveErr == 0);

    return (haveWriteErr == 0);
}

void delete_ID_file()
{
    if(haveSD && ic) {
        deleteFileFromSD(CONFND);
        SD.rename(CONFN, CONFND);
    }
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

                #ifdef TC_DBG_BOOT
                char t = uploadFileName[8];
                #endif
                
                uploadFileName[8] = '/';
                
                #ifdef TC_DBG_BOOT
                Serial.printf("openUploadFile: Deleting %s\n", uploadFileName+8);
                #endif
                
                deleteFileFromSD(uploadFileName+8);
                
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
            deleteFileFromSD(uploadRealFileNames[idx]);
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
        
        deleteFileFromSD(t);
        
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
    static const char *upderr = "Firmware update error %d\n";
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
    deleteFileFromSD(fwfnold);
    SD.rename(fwfn, fwfnold);
    unmount_fs();
    delay(1000);
    fw_error_blink(0);
    esp_restart();
}    
