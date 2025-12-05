/*
 * -------------------------------------------------------------------
 * CircuitSetup.us Time Circuits Display
 * (C) 2021-2022 John deGlavina https://circuitsetup.us
 * (C) 2022-2025 Thomas Winischhofer (A10001986)
 * https://github.com/realA10001986/Time-Circuits-Display
 * https://tcd.out-a-ti.me
 *
 * Time and Main Controller
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

#include <Arduino.h>
#include <time.h>
#include <WiFi.h>
#include <Wire.h>
#include <Udp.h>
#include <WiFiUdp.h>

#include "tc_keypad.h"
#include "tc_menus.h"
#include "tc_audio.h"
#include "tc_wifi.h"
#include "tc_settings.h"
#if defined(TC_HAVE_RE) || defined(TC_HAVE_REMOTE)
#include "input.h"
#endif

#include "tc_time.h"

// i2c slave addresses

#define DEST_TIME_ADDR 0x71 // TC displays
#define PRES_TIME_ADDR 0x72
#define DEPT_TIME_ADDR 0x74

#define KEYPAD_ADDR    0x20 // PCF8574 port expander (keypad) (redefined in keypad.cpp, here only for completeness)

#define DS3231_ADDR    0x68 // DS3231 RTC
#define PCF2129_ADDR   0x51 // PCF2129 RTC

#define SPEEDO_ADDR    0x70 // speedo display

#define GPS_ADDR       0x10 // GPS receiver

                            // temperature sensors
#define MCP9808_ADDR   0x18 // [default]
#define BMx280_ADDR    0x77 // [default]
#define SHT40_ADDR     0x44 // Only SHT4x-Axxx, not SHT4x-Bxxx
#define SI7021_ADDR    0x40 //
#define TMP117_ADDR    0x49 // [non-default]
#define AHT20_ADDR     0x38 //
#define HTU31_ADDR     0x41 // [non-default]
#define MS8607_ADDR    0x76 // +0x40
#define HDC302X_ADDR   0x45 // [non-default]

                            // light sensors 
#define LTR3xx_ADDR    0x29 // [default]                            
#define TSL2561_ADDR   0x29 // [default]
#define TSL2591_ADDR   0x29 // [default]
#define BH1750_ADDR    0x23 // [default]
#define VEML6030_ADDR  0x48 // [non-default] (can also be 0x10 but then conflicts with GPS)
#define VEML7700_ADDR  0x10 // [default] (conflicts with GPS!)

                            // rotary encoders for speed
#define ADDA4991_ADDR  0x36 // [default]
#define DUPPAV2_ADDR   0x01 // [user configured: A0 closed]
#define DFRGR360_ADDR  0x54 // [default; SW1=0, SW2=0]

                            // rotary encoders for volume
#define ADDA4991V_ADDR 0x37 // [non-default: A0 closed]
#define DUPPAV2V_ADDR  0x03 // [user configured: A0+A1 closed]
#define DFRGR360V_ADDR 0x55 // [SW1=0, SW2=1]

// The time between reentry sound being started and the display coming on
// Must be sync'd to the sound file used! (startup.mp3/timetravel.mp3)
#ifdef TWSOUND
#define STARTUP_DELAY 900
#else
#define STARTUP_DELAY 1050
#endif
#define TIMETRAVEL_DELAY 1500

// Volume-factor for "travelstart" sounds
#define TT_SOUND_FACT 1.2f

// Time between the phases of (long) time travel
// Sum of all must be 8000
#define TT_P1_TOTAL     8000
#define TT_P1_DELAY_P1  1400                                                                        // Normal
#define TT_P1_DELAY_P2  (4200-TT_P1_DELAY_P1)                                                       // Light flicker
#define TT_P1_DELAY_P3  (5800-(TT_P1_DELAY_P2+TT_P1_DELAY_P1))                                      // Off
#define TT_P1_DELAY_P4  (6800-(TT_P1_DELAY_P3+TT_P1_DELAY_P2+TT_P1_DELAY_P1))                       // Random display I
#define TT_P1_DELAY_P5  (TT_P1_TOTAL-(TT_P1_DELAY_P4+TT_P1_DELAY_P3+TT_P1_DELAY_P2+TT_P1_DELAY_P1)) // Random display II
// ACTUAL POINT OF TIME TRAVEL:
#define TT_P1_POINT88   1400    // ms into "starttravel" sample, when 88mph is reached.
#define TT_P1_EXT       TT_P1_TOTAL - TT_P1_DELAY_P1   // part of P1 between P0 and P2

// Speedo display status
#define SPST_NIL  0
#define SPST_GPS  1
#define SPST_RE   2
#define SPST_TEMP 3
#define SPST_ZERO 4
#define SPST_REM  5

// Preprocessor config for External Time Travel Output (ETTO):
// Lead time from trigger (LOW->HIGH) to actual tt (ie when 88mph is reached)
// The external prop has ETTO_LEAD_TIME ms to play its pre-tt sequence. After
// ETTO_LEAD_TIME ms, 88mph is reached, and the actual tt takes place.
#define ETTO_LEAD_TIME      5000
#define ETTO_LAT              50  // DO NOT CHANGE
// End of ETTO config

// Native NTP
#define NTP_PACKET_SIZE 48
#define NTP_DEFAULT_LOCAL_PORT 1337

#define SECS1900_1970 2208988800ULL

// Temperature update intervals
#define TEMP_UPD_INT_L (2*60*1000)
#define TEMP_UPD_INT_S (30*1000)

static bool          bootStrap = true;

// millis64
static unsigned long lastMillis64 = 0;
static uint64_t      millisEpoch = 0;

static bool          couldHaveAuthTime = false;
static bool          haveAuthTime = false;
uint16_t             lastYear = 0;
static uint16_t      lastHour = 23;
static uint8_t       resyncInt = 5;
bool                 syncTrigger = false;
bool                 doAPretry = true;
static bool          deferredCP = false;
static unsigned long deferredCPNow = 0;

// For tracking second changes
static bool          x = false;  
static bool          y = false;

// For beep-auto-modes
uint8_t              beepMode = DEF_BEEP;
bool                 beepTimer = false;
unsigned long        beepTimeout = 30*1000;
unsigned long        beepTimerNow = 0;

// Persistent time travels:
// This controls the firmware's behavior as regards saving times to NVM.
// If this is true, times are saved to NVM, whenever
//  - the user enters a destination time for time travel and presses ENTER
//  - the user activates time travel (hold "0")
//  - the user returns from a time travel (hold "9")
// True means that playing around with time travel is persistent, and even
// present time is kept over a power loss (if the battery backed RTC keeps
// the time). Downside is that the user's custom destination and last
// departure times are overwritten during a time travel.
// False means that time travel games are non-persistent, and the user's
// custom times (as set up in the keypad menu) are never overwritten.
// Also, "false" reduces flash wear considerably.
bool                 timetravelPersistent = false;

// Alarm/HourlySound based on RTC
static bool          alarmRTC = true;

bool                 ETTOcommands = false;
static bool          ETTOalarm = false;
static unsigned long ETTOAlmDur = DEF_ETTO_ALM_D * 1000;
static unsigned long ETTOAlmNow = 0;
static bool          ERROSigAlarm = false;

bool                 useGPS      = false;  // leave this false, will be true after init
bool                 dispGPSSpeed = false;
static bool          provGPS2BTTFN = false;

static bool          useRotEnc = false;
static bool          dispRotEnc = false;
static bool          useRotEncVol = false;

bool                 useSpeedo = false;
bool                 useSpeedoDisplay = false;
static bool          bttfnSpeedoFallback = false;
static uint8_t       speedoStatus = SPST_NIL;
static unsigned long timetravelP0Now = 0;
static unsigned long timetravelP0Delay = 0;
static unsigned long ttP0Now = 0;
static uint8_t       timeTravelP0Speed = 0;
static long          pointOfP1 = 0;
static long          pointOfP1NoLead = 0;
static bool          didTriggerP1 = false;
static float         ttP0TimeFactor = 1.0;
static bool          havePreTTSound = false;
static const char    *preTTSound = "/ttaccel.mp3";
static bool          haveAbortTTSound = false;
static const char    *abortTTSound = "/ttcancel.mp3";

#ifdef TC_HAVEGPS
static unsigned long dispGPSnow = 0;
#endif
static bool          speedoAlwaysOn = false;
static unsigned long dispIdlenow = 0;
#ifdef TC_HAVETEMP
static int           tempBrightness = DEF_TEMP_BRIGHT;
static bool          tempOldNM = false;
#endif
#if (defined(TC_HAVEGPS) && defined(NOT_MY_RESPONSIBILITY)) || defined(TC_HAVE_RE)
static bool          GPSabove88 = false;
#endif
static bool          bttfnRemoteSpeedMaster = false;
static bool          bttfnHaveRemote = false;
#ifdef TC_HAVE_REMOTE
static bool          bttfnOldRemoteSpeedMaster = false;
static bool          bttfnRemStop     = false;  // Status of "Stop" switch/light
static uint8_t       bttfnRemoteSpeed = 0;      // What is displayed on remote
static uint8_t       bttfnRemCurSpd   = 0;      // What is displayed on speedo
static bool          bttfnRemOffSpd   = false;  // Remote wants speed also while fake-off (used for TT)
static uint8_t       bttfnOldRemCurSpd = 255;
static uint8_t       bttfnRemCurSpdOld = 255;
static unsigned long lastRemSpdUpd = 0, remSpdCatchUpDelay = 80;
bool                 remoteAllowed = false;
bool                 remoteKPAllowed = false;
static int           remoteWasMaster = 0;
static const char    *remoteOnSound = "/remoteon.mp3";
static const char    *remoteOffSound = "/remoteoff.mp3";
static bool          oldptnmrem = false;
#endif
static bool          bttfnRemPwrMaster = false;
#ifdef TC_HAVE_RE
static int16_t       fakeSpeed = 0;
static int16_t       oldFSpd = 0;
static int16_t       oldFakeSpeed = -1;
static bool          tempLock = false;
static unsigned long tempLockNow = 0;
static bool          spdreOldNM = false;
#endif
static bool          volChanged = false;
static unsigned long volChangedNow = 0;

bool                 useTemp = false;
bool                 dispTemp = true;
bool                 tempOffNM = true;
#ifdef TC_HAVETEMP
bool                 tempUnit = DEF_TEMP_UNIT;
static unsigned long tempReadNow = 0;
static unsigned long tempDispNow = 0;
static unsigned long tempUpdInt = TEMP_UPD_INT_L;
static bool          tempLastNan = true;
#endif

// The startup sequence
bool                 startup      = false;
static bool          startupSound = false;
static unsigned long startupNow   = 0;

// Timetravel sequence: Timers, triggers, state
static bool          playTTsounds = true;
static bool          skipTTAnim = false;
int                  timeTravelP0 = 0;
static uint16_t      timeTravelP0stalled = 0;
int                  timeTravelP2 = 0;
static unsigned long timetravelP1Now = 0;
static unsigned long timetravelP1Delay = 0;
int                  timeTravelP1 = 0;
static bool          triggerP1 = false;
static bool          triggerP1NoLead = false;
static unsigned long triggerP1Now = 0;
static long          triggerP1LeadTime = 0;
static bool          ETTWithFixedLead = false;
static bool          useETTOWired = false;
static bool          useETTOWiredNoLead = false;
static const long    ettoLeadTime = ETTO_LEAD_TIME - ETTO_LAT;
static bool          triggerETTO = false;
static long          triggerETTOLeadTime = 0;
static unsigned long triggerETTONow = 0;
static long          ettoLeadPoint = 0;
static long          origEttoLeadPoint = 0;
static uint16_t      bttfnTTLeadTime = 0;
static long          ettoBase;
static bool          pubMQTTVL = false;
#ifdef TC_HAVE_REMOTE
static bool          remoteInducedTT = false;
#endif
#ifdef IS_ACAR_DISPLAY
#define P1JAN011885 "010118851200"
#define P1JAN011801 "010118011000"
#define P1ERR900    "  00900 9000"
#define P1ERR99     "  0099  0000"
#else
#define P1JAN011885 "JAN0118851200"
#define P1JAN011801 "JAN0118011000"
#define P1ERR900    "   00900 9000"
#define P1ERR99     "   0099  0000"
#endif
static const char *p1errStrs[4] = {
    P1JAN011885, P1JAN011801, P1ERR900, P1ERR99
};

// Re-entry sequence
static unsigned long timetravelNow = 0;
bool                 timeTravelRE = false;

// CPU power management
static bool          pwrLow = false;
static unsigned long pwrFullNow = 0;

// State flags & co
static bool postSecChange = false;
static bool postHSecChange = false;
static unsigned long pscsnow = 0;
static bool postHSecChangeBusy = false;
static bool DSTcheckDone = false;
static bool autoIntDone = false;
int         autoIntAnimRunning = 0;
static bool autoReadjust = false;
static unsigned long lastAuthTime = 0;
uint64_t    lastAuthTime64 = 0;
static bool authTimeExpired = false;
static bool alarmDone = false;
static bool remDone = false;
static bool hourlySoundDone = false;
static bool autoNMDone = false;
static uint8_t* (*r)(uint8_t *, uint32_t, int);
int         specDisp = 0;

// Used in time loop
static DateTime gdtu, gdtl;

// For displaying times off the real time
uint64_t    timeDifference = 0;
bool        timeDiffUp = false;  // true = add difference, false = subtract difference

// TZ/DST status & data
bool        couldDST[3]     = { false, false, false };   // Could use own DST management (and DST is defined in TZ)
static int  tzForYear[3]    = { 0, 0, 0 };               // Parsing done for this very year
static int8_t tzIsValid[3]  = { -1, -1, -1 };
static int8_t tzHasDST[3]   = { -1, -1, -1 };
static char *tzDSTpart[3]   = { NULL, NULL, NULL };
static int  tzDiffGMT[3]    = { 0, 0, 0 };               // Difference to UTC in nonDST time
static int  tzDiffGMTDST[3] = { 0, 0, 0 };               // Difference to UTC in DST time
static int  tzDiff[3]       = { 0, 0, 0 };               // difference between DST and non-DST in minutes
static int  DSTonMins[3]    = { -1, -1, -1 };            // DST-on date/time in minutes since 1/1 00:00 (in non-DST time)
static int  DSToffMins[3]   = { 600000, 600000, 600000}; // DST-off date/time in minutes since 1/1 00:00 (in DST time)
#ifdef TC_DBG
static const char *badTZ = "Failed to parse TZ\n";
#endif

// "Room condition" mode
static bool rcMode = false;
bool        haveRcMode = false;

// WC stuff
bool        WcHaveTZ1   = false;
bool        WcHaveTZ2   = false;
bool        haveWcMode  = false;
static bool wcMode      = false;
static int  wcLastMin   = -1;
bool        triggerWC   = false;
static bool haveTZName1 = false;
static bool haveTZName2 = false;
uint8_t     destShowAlt = 0, depShowAlt = 0;

uint8_t     mqttDisp = 0;
#ifdef TC_HAVEMQTT
uint8_t     mqttOldDisp = 0;
char        *mqttMsg = NULL;
uint16_t    mqttIdx = 0;
int16_t     mqttMaxIdx = 0;
bool        mqttST = false;
static unsigned long mqttStartNow = 0;
#endif

// For NTP/GPS
static struct        tm _timeinfo;
static WiFiUDP       ntpUDP;
static UDP*          myUDP;
static byte          NTPUDPBuf[NTP_PACKET_SIZE];
static unsigned long NTPUpdateNow = 0;
static unsigned long NTPTSAge = 0;
static unsigned long NTPTSRQAge = 0;
static uint32_t      NTPsecsSinceTCepoch = 0;
static uint32_t      NTPmsSinceSecond = 0;
static bool          NTPPacketDue = false;
static bool          NTPWiFiUp = false;
static uint8_t       NTPfailCount = 0;
static const uint8_t NTPUDPHD[4] = { 'T', 'C', 'D', '1' };
static uint32_t      NTPUDPID    = 0;
 
// The RTC object
static const uint8_t rtcAddr[2*2] = {
    PCF2129_ADDR, RTCT_PCF2129, 
    DS3231_ADDR,  RTCT_DS3231
};
tcRTC rtc(2, rtcAddr);
#ifdef HAVE_PCF2129
static unsigned long OTPRDoneNow = 0;
static bool          RTCNeedsOTPR = false;
static bool          OTPRinProgress = false;
static unsigned long OTPRStarted = 0;
#endif

// The GPS object
#ifdef TC_HAVEGPS
tcGPS myGPS(GPS_ADDR);
static uint64_t      lastLoopGPS   = 0;
static unsigned long GPSupdateFreq = 1000;
#endif

// The TC display objects
clockDisplay destinationTime(DISP_DEST, DEST_TIME_ADDR);
clockDisplay presentTime(DISP_PRES, PRES_TIME_ADDR);
clockDisplay departedTime(DISP_LAST, DEPT_TIME_ADDR);

// The speedo and temp.sensor objects
speedDisplay speedo(SPEEDO_ADDR);
#ifdef TC_HAVE_RE
static const uint8_t rotEncAddr[3*2] = { 
    ADDA4991_ADDR, TC_RE_TYPE_ADA4991,
    DUPPAV2_ADDR,  TC_RE_TYPE_DUPPAV2,
    DFRGR360_ADDR, TC_RE_TYPE_DFRGR360
};
static const uint8_t rotEncVAddr[3*2] = { 
    ADDA4991V_ADDR, TC_RE_TYPE_ADA4991,
    DUPPAV2V_ADDR,  TC_RE_TYPE_DUPPAV2,
    DFRGR360V_ADDR, TC_RE_TYPE_DFRGR360
};
static TCRotEnc rotEnc(3, rotEncAddr);
static TCRotEnc rotEncV(3, rotEncVAddr);
static TCRotEnc *rotEncVol;
#endif
#ifdef TC_HAVETEMP
static const uint8_t tempSensAddr[9*2] = { 
    MCP9808_ADDR, MCP9808,
    BMx280_ADDR,  BMx280,
    SHT40_ADDR,   SHT40,
    MS8607_ADDR,  MS8607,   // before Si7021
    SI7021_ADDR,  SI7021,
    TMP117_ADDR,  TMP117,
    AHT20_ADDR,   AHT20,
    HTU31_ADDR,   HTU31,
    HDC302X_ADDR, HDC302X
};
tempSensor tempSens(9, tempSensAddr);
#endif
#ifdef TC_HAVELIGHT
static const uint8_t lightSensAddr[6*2] = { 
    LTR3xx_ADDR,  LST_LTR3xx,   // must be before TSL26x1
    TSL2591_ADDR, LST_TSL2591,  // must be after LTR3xx
    TSL2561_ADDR, LST_TSL2561,  // must be after LTR3xx
    BH1750_ADDR,  LST_BH1750,
    VEML6030_ADDR,LST_VEML7700,
    VEML7700_ADDR,LST_VEML7700  // must be last
};
lightSensor lightSens(6, lightSensAddr); 
#endif

bool       stalePresent = false;
dateStruct stalePresentTime[2] = {
    {1985, 10, 26,  1, 22},         // original for backup
    {1985, 10, 26,  1, 22}          // changed during use
};

// Time cycling

static bool    autoRotAnim = true;
static uint8_t autoIntSec  = 59;
int8_t         autoTime    = 0;
// Pause auto-time-cycling if user played with time travel
static bool          autoPaused = false;
static unsigned long pauseNow = 0;
static unsigned long pauseDelay = 30*60*1000;  // Pause for 30 minutes

#ifndef TWPRIVATE //  ----------------- OFFICIAL

const dateStruct destinationTimes[NUM_AUTOTIMES] = {
    {1985, 10, 26,  1, 21},   // Einstein 1:20 -> 1:21
    {1955, 11,  5,  6,  0},   // Marty -> 1955
    {1985, 10, 26,  1, 24},   // Marty -> 1985
    {2015, 10, 21, 16, 29},   // Marty, Doc, Jennifer -> 2015
    {1955, 11, 12,  8,  0},   // Old Biff -> 1955 (time unknown)
    {2015, 10, 21, 19, 25},   // Old Biff -> 2015 (time assumed from run of scene)
    {1985, 10, 26, 21,  0},   // Marty, Doc, Jennifer -> 1985
    {1955, 11, 12,  6,  0},   // Marty, Doc -> 1955
    {1885,  1,  1,  0,  0},   // Doc in thunderstorm -> 1885 (date/time assumed from letter)
    {1885,  9,  2,  8,  0},   // Marty -> 1885
    {1985, 10, 27, 11,  0}    // Marty -> 1985
};
const dateStruct departedTimes[NUM_AUTOTIMES] = {
    {1985, 10, 26,  1, 20},   // Einstein 1:20 -> 1:21
    {1985, 10, 26,  1, 29},   // Marty -> 1955 (time assumed)
    {1955, 11, 12, 22,  4},   // Marty -> 1985
    {1985, 10, 26, 11, 35},   // Marty, Doc, Jennifer -> 2015 (time random)
    {2015, 10, 21, 19, 15},   // Old Biff -> 1955  (time assumed from run of scene)
    {1955, 11, 12, 18, 38},   // Old Biff -> 2015
    {2015, 10, 21, 19, 28},   // Marty, Doc, Jennifer -> 1985
    {1985, 10, 27,  2, 42},   // Marty, Doc -> 1955
    {1955, 11, 12, 21, 33},   // Doc in thunderstorm -> 1885 (time assumed from following events)
    {1955, 11, 15, 11, 11},   // Marty -> 1885 (Date shown in movie 11/5 - wrong! Must be after 11/12! Date&time guessed)
    {1885,  9,  7,  8, 22}    // Marty -> 1985 (time guessed/assumed)
};

#else //  --------------------------- TWPRIVATE

#include "z_twdates.h"

#endif  // ------------------------------------

// Alarm weekday masks
static const uint8_t alarmWDmasks[10] = { 
    0b01111111, 0b00111110, 0b01000001, 0b00000010, 0b00000100,
    0b00001000, 0b00010000, 0b00100000, 0b01000000, 0b00000001
};
                
// Night mode
bool           forceReEvalANM = false;
static bool    autoNightMode = false;
static uint8_t autoNightModeMode = 0;   // 0 = daily hours, 1-4 presets
static uint8_t autoNMOnHour  = 0;
static uint8_t autoNMOffHour = 0;
static int8_t  timedNightMode = -1;
static int8_t  sensorNightMode = -1;
int8_t         manualNightMode = -1;
unsigned long  manualNMNow = 0;
int32_t        luxLimit = 3;
static const uint32_t autoNMhomePreset[7] = {     // Mo-Th 5pm-11pm, Fr 1pm-1am, Sa 9am-1am, Su 9am-11pm
        0b011111111000000000000001,   //Sun
        0b111111111111111110000001,   //Mon
        0b111111111111111110000001,   //Tue
        0b111111111111111110000001,   //Wed
        0b111111111111111110000001,   //Thu
        0b111111111111100000000000,   //Fri
        0b011111111000000000000000    //Sat
};
static const uint32_t autoNMofficePreset[7] = {   // Mo-Fr 9am-5pm
        0b111111111111111111111111,   //Sun
        0b111111111000000001111111,   //Mon
        0b111111111000000001111111,   //Tue
        0b111111111000000001111111,   //Wed
        0b111111111000000001111111,   //Thu
        0b111111111000000001111111,   //Fri
        0b111111111111111111111111    //Sat
};
static const uint32_t autoNMoffice2Preset[7] = {  // Mo-Th 7am-5pm, Fr 7am-2pm
        0b111111111111111111111111,   //Sun
        0b111111100000000001111111,   //Mon
        0b111111100000000001111111,   //Tue
        0b111111100000000001111111,   //Wed
        0b111111100000000001111111,   //Thu
        0b111111100000001111111111,   //Fri
        0b111111111111111111111111    //Sat
};
static const uint32_t autoNMshopPreset[7] = {     // Mo-We 8am-8pm, Th-Fr 8am-9pm, Sa 8am-5pm
        0b111111111111111111111111,   //Sun
        0b111111110000000000001111,   //Mon
        0b111111110000000000001111,   //Tue
        0b111111110000000000001111,   //Wed
        0b111111110000000000000111,   //Thu
        0b111111110000000000000111,   //Fri
        0b111111110000000001111111    //Sat
};
static uint32_t autoNMdailyPreset = 0;
const uint32_t *autoNMPresets[AUTONM_NUM_PRESETS] = {
    autoNMhomePreset, autoNMofficePreset, 
    autoNMoffice2Preset, autoNMshopPreset
};
bool useLight = false;
#ifdef TC_HAVELIGHT
static unsigned long lastLoopLight = 0;
#endif

// Count-down timer
unsigned long ctDown = 0;
unsigned long ctDownNow = 0;

// Fake "power" feature (switch, MQTT, remote)
bool FPBUnitIsOn = true;
static TCButton fakePowerOnKey = TCButton(FAKE_POWER_BUTTON_PIN,
    true,    // Button is active LOW
    true     // Enable internal pull-up resistor
);
static bool waitForFakePowerButton = false;
static bool restoreFakePwr = false;
bool        MQTTPwrMaster = false;
bool        MQTTWaitForOn = false;
static bool isFPBKeyChange = false;
static bool isFPBKeyPressed = false;
#if defined(TC_HAVE_REMOTE) || defined(TC_HAVEMQTT)
static bool shadowFPBKeyPressed = false;
#endif
#ifdef TC_HAVEMQTT
static bool shadowMQTTFPBKeyPressed = false;
#endif

// Date & time stuff
static const uint8_t monthDays[] =
{
    31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31
};
static const unsigned int mon_yday[2][13] =
{
    { 0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334, 365 },
    { 0, 31, 60, 91, 121, 152, 182, 213, 244, 274, 305, 335, 366 }
};
static const unsigned int mon_ydayt24t60[2][13] =
{
    { 0, 31*24*60,  59*24*60,  90*24*60, 120*24*60, 151*24*60, 181*24*60, 
        212*24*60, 243*24*60, 273*24*60, 304*24*60, 334*24*60, 365*24*60 },
    { 0, 31*24*60,  60*24*60,  91*24*60, 121*24*60, 152*24*60, 182*24*60, 
        213*24*60, 244*24*60, 274*24*60, 305*24*60, 335*24*60, 366*24*60 }
};
static const uint64_t mins1kYears[] =
{
#ifndef TC_JULIAN_CAL  
             0,  262975680,  525949920,  788924160, 1051898400,
    1314874080, 1577848320, 1840822560, 2103796800, 2366772480,
    2629746720, 2892720960, 3155695200, 3418670880, 3681645120,
    3944619360, 4207593600, 4470569280, 4733543520, 4996517760,
    5259492000, 5522467680
#elif !defined(JSWITCH_1582)
             0,   52596000,  105192000,  157788000,  210384000, 
     262980000,  315576000,  368172000,  420768000,  473364000, 
     525960000,  578556000,  631152000,  683748000,  736344000, 
     788940000,  841536000,  894132000,  946712160,  999306720, 
    1051901280, 1104497280, 1157091840, 1209686400, 1262280960, 
    1314876960, 1367471520, 1420066080, 1472660640, 1525256640, 
    1577851200, 1630445760, 1683040320, 1735636320, 1788230880, 
    1840825440, 1893420000, 1946016000, 1998610560, 2051205120, 
    2103799680, 2156395680, 2208990240, 2261584800, 2314179360, 
    2366775360, 2419369920, 2471964480, 2524559040, 2577155040, 
    2629749600, 2682344160, 2734938720, 2787534720, 2840129280, 
    2892723840, 2945318400, 2997914400, 3050508960, 3103103520, 
    3155698080, 3208294080, 3260888640, 3313483200, 3366077760, 
    3418673760, 3471268320, 3523862880, 3576457440, 3629053440, 
    3681648000, 3734242560, 3786837120, 3839433120, 3892027680, 
    3944622240, 3997216800, 4049812800, 4102407360, 4155001920, 
    4207596480, 4260192480, 4312787040, 4365381600, 4417976160, 
    4470572160, 4523166720, 4575761280, 4628355840, 4680951840, 
    4733546400, 4786140960, 4838735520, 4891331520, 4943926080, 
    4996520640, 5049115200, 5101711200, 5154305760, 5206900320,
    5259494880, 5312090880, 5364685440, 5417280000, 5469874560, 
    5522470560, 5575065120, 5627659680, 5680254240, 5732850240   
#else
             0,   52596000,  105192000,  157788000,  210384000, 
     262980000,  315576000,  368172000,  420768000,  473364000, 
     525960000,  578556000,  631152000,  683748000,  736344000, 
     788940000,  841521600,  894117600,  946712160,  999306720, 
    1051901280, 1104497280, 1157091840, 1209686400, 1262280960, 
    1314876960, 1367471520, 1420066080, 1472660640, 1525256640, 
    1577851200, 1630445760, 1683040320, 1735636320, 1788230880, 
    1840825440, 1893420000, 1946016000, 1998610560, 2051205120, 
    2103799680, 2156395680, 2208990240, 2261584800, 2314179360, 
    2366775360, 2419369920, 2471964480, 2524559040, 2577155040, 
    2629749600, 2682344160, 2734938720, 2787534720, 2840129280, 
    2892723840, 2945318400, 2997914400, 3050508960, 3103103520, 
    3155698080, 3208294080, 3260888640, 3313483200, 3366077760, 
    3418673760, 3471268320, 3523862880, 3576457440, 3629053440, 
    3681648000, 3734242560, 3786837120, 3839433120, 3892027680, 
    3944622240, 3997216800, 4049812800, 4102407360, 4155001920, 
    4207596480, 4260192480, 4312787040, 4365381600, 4417976160, 
    4470572160, 4523166720, 4575761280, 4628355840, 4680951840, 
    4733546400, 4786140960, 4838735520, 4891331520, 4943926080, 
    4996520640, 5049115200, 5101711200, 5154305760, 5206900320,
    5259494880, 5312090880, 5364685440, 5417280000, 5469874560, 
    5522470560, 5575065120, 5627659680, 5680254240, 5732850240
#endif    
};
static const uint32_t hours1kYears[] =
{
#ifndef TC_JULIAN_CAL  
                0,  262975680/60,  525949920/60,  788924160/60, 1051898400/60,
    1314874080/60, 1577848320/60, 1840822560/60, 2103796800/60, 2366772480/60, 
    2629746720/60, 2892720960/60, 3155695200/60, 3418670880/60, 3681645120/60, 
    3944619360/60, 4207593600/60, 4470569280/60, 4733543520/60, 4996517760/60,
    5259492000/60, 5522467680/60 
#elif !defined(JSWITCH_1582)
                0,   52596000/60,  105192000/60,  157788000/60,  210384000/60, 
     262980000/60,  315576000/60,  368172000/60,  420768000/60,  473364000/60, 
     525960000/60,  578556000/60,  631152000/60,  683748000/60,  736344000/60, 
     788940000/60,  841536000/60,  894132000/60,  946712160/60,  999306720/60, 
    1051901280/60, 1104497280/60, 1157091840/60, 1209686400/60, 1262280960/60, 
    1314876960/60, 1367471520/60, 1420066080/60, 1472660640/60, 1525256640/60, 
    1577851200/60, 1630445760/60, 1683040320/60, 1735636320/60, 1788230880/60, 
    1840825440/60, 1893420000/60, 1946016000/60, 1998610560/60, 2051205120/60, 
    2103799680/60, 2156395680/60, 2208990240/60, 2261584800/60, 2314179360/60, 
    2366775360/60, 2419369920/60, 2471964480/60, 2524559040/60, 2577155040/60, 
    2629749600/60, 2682344160/60, 2734938720/60, 2787534720/60, 2840129280/60, 
    2892723840/60, 2945318400/60, 2997914400/60, 3050508960/60, 3103103520/60, 
    3155698080/60, 3208294080/60, 3260888640/60, 3313483200/60, 3366077760/60, 
    3418673760/60, 3471268320/60, 3523862880/60, 3576457440/60, 3629053440/60, 
    3681648000/60, 3734242560/60, 3786837120/60, 3839433120/60, 3892027680/60, 
    3944622240/60, 3997216800/60, 4049812800/60, 4102407360/60, 4155001920/60, 
    4207596480/60, 4260192480/60, 4312787040/60, 4365381600/60, 4417976160/60, 
    4470572160/60, 4523166720/60, 4575761280/60, 4628355840/60, 4680951840/60, 
    4733546400/60, 4786140960/60, 4838735520/60, 4891331520/60, 4943926080/60, 
    4996520640/60, 5049115200/60, 5101711200/60, 5154305760/60, 5206900320/60,
    5259494880/60, 5312090880/60, 5364685440/60, 5417280000/60, 5469874560/60, 
    5522470560/60, 5575065120/60, 5627659680/60, 5680254240/60, 5732850240/60
#else
             0/60,   52596000/60,  105192000/60,  157788000/60,  210384000/60, 
     262980000/60,  315576000/60,  368172000/60,  420768000/60,  473364000/60, 
     525960000/60,  578556000/60,  631152000/60,  683748000/60,  736344000/60, 
     788940000/60,  841521600/60,  894117600/60,  946712160/60,  999306720/60, 
    1051901280/60, 1104497280/60, 1157091840/60, 1209686400/60, 1262280960/60, 
    1314876960/60, 1367471520/60, 1420066080/60, 1472660640/60, 1525256640/60, 
    1577851200/60, 1630445760/60, 1683040320/60, 1735636320/60, 1788230880/60, 
    1840825440/60, 1893420000/60, 1946016000/60, 1998610560/60, 2051205120/60, 
    2103799680/60, 2156395680/60, 2208990240/60, 2261584800/60, 2314179360/60, 
    2366775360/60, 2419369920/60, 2471964480/60, 2524559040/60, 2577155040/60, 
    2629749600/60, 2682344160/60, 2734938720/60, 2787534720/60, 2840129280/60, 
    2892723840/60, 2945318400/60, 2997914400/60, 3050508960/60, 3103103520/60, 
    3155698080/60, 3208294080/60, 3260888640/60, 3313483200/60, 3366077760/60, 
    3418673760/60, 3471268320/60, 3523862880/60, 3576457440/60, 3629053440/60, 
    3681648000/60, 3734242560/60, 3786837120/60, 3839433120/60, 3892027680/60, 
    3944622240/60, 3997216800/60, 4049812800/60, 4102407360/60, 4155001920/60, 
    4207596480/60, 4260192480/60, 4312787040/60, 4365381600/60, 4417976160/60, 
    4470572160/60, 4523166720/60, 4575761280/60, 4628355840/60, 4680951840/60, 
    4733546400/60, 4786140960/60, 4838735520/60, 4891331520/60, 4943926080/60, 
    4996520640/60, 5049115200/60, 5101711200/60, 5154305760/60, 5206900320/60,
    5259494880/60, 5312090880/60, 5364685440/60, 5417280000/60, 5469874560/60, 
    5522470560/60, 5575065120/60, 5627659680/60, 5680254240/60, 5732850240/60
#endif    
};

#ifdef TC_JULIAN_CAL
static const uint64_t tdro = 5258967840;
#ifndef JSWITCH_1582
static const int jCentStart   = 1700;      // Start of century when switch took place
static const int jCentEnd     = 1799;      // Last year of century when switch took place
static const int jSwitchYear  = 1752;      // Year in which switch to Gregorian Cal took place
static const int jSwitchMon   = 9;         // Month in which switch to Gregorian Cal took place
static const int jSwitchDay   = 2;         // Last day of Julian Cal
static const int jSwitchSkipD = 11;        // Number of days skipped
static const int jSwitchSkipH = 11 * 24;   // Num hours skipped
#else
static const int jCentStart   = 1500;      // Start of century when switch took place
static const int jCentEnd     = 1599;      // Last year of century when switch took place
static const int jSwitchYear  = 1582;      // Year in which switch to Gregorian Cal took place
static const int jSwitchMon   = 10;        // Month in which switch to Gregorian Cal took place
static const int jSwitchDay   = 4;         // Last day of Julian Cal
static const int jSwitchSkipD = 10;        // Number of days skipped
static const int jSwitchSkipH = 10 * 24;   // Num hours skipped
#endif
static int mon_yday_jSwitch[13];     // Accumulated days per month in year of Switch
static int mon_ydayt24t60J[13];      // Accumulated mins per month in year of Switch
static int jSwitchYrHrs;
static uint32_t jSwitchHash = 0;
#else
static const uint64_t tdro = 5258964960;
#endif
#define a(f, j) (f << (*monthDays - j))
uint8_t* e(uint8_t *d, uint32_t m, int y) { return (*r)(d, m, y); }

// Acceleraton times
static const int16_t tt_p0_delays_rl[88] =
{
      0, 100, 100,  90,  80,  80,  80,  80,  80,  80,  // 0 - 9  10mph 0.8s    0.8=800ms
     80,  80,  80,  80,  80,  80,  80,  80,  80,  80,  // 10-19  20mph 1.6s    0.8
     90, 100, 110, 110, 110, 110, 110, 110, 120, 120,  // 20-29  30mph 2.7s    1.1
    120, 130, 130, 130, 130, 130, 130, 130, 130, 140,  // 30-39  40mph 4.0s    1.3
    150, 160, 190, 190, 190, 190, 190, 190, 210, 230,  // 40-49  50mph 5.9s    1.9
    230, 230, 240, 240, 240, 240, 240, 240, 250, 250,  // 50-59  60mph 8.3s    2.4
    250, 250, 260, 260, 270, 270, 270, 280, 290, 300,  // 60-69  70mph 11.0s   2.7
    320, 330, 350, 370, 370, 380, 380, 390, 400, 410,  // 70-79  80mph 14.7s   3.7
    410, 410, 410, 410, 410, 410, 410, 410             // 80-87  90mph 18.8s   4.1
};
static const int16_t tt_p0_delays_movie[88] =
{
      0,  90,  90,  90,  90,  90,  90,  95,  95, 100,  // m0 - 9  10mph  0- 9: 0.83s  (m=measured, i=interpolated)
    105, 110, 115, 120, 125, 130, 135, 140, 145, 150,  // i10-19  20mph 10-19: 1.27s
    155, 160, 165, 170, 175, 180, 185, 190, 195, 200,  // i20-29  30mph 20-29: 1.77s
    200, 200, 202, 203, 204, 205, 206, 207, 208, 209,  // m30-39  40mph 30-39: 2s
    210, 211, 212, 213, 214, 215, 216, 217, 218, 219,  // i40-49  50mph 40-49: 2.1s
    220, 221, 222, 223, 224, 225, 226, 227, 228, 229,  // m50-59  60mph 50-59: 2.24s
    230, 233, 236, 240, 243, 246, 250, 253, 256, 260,  // i60-69  70mph 60-69: 2.47s
    263, 266, 270, 273, 276, 280, 283, 286, 290, 293,  // m70-79  80mph 70-79: 2.78s
    296, 300, 300, 303, 303, 306, 310, 310             // i80-88  90mph 80-88: 2.4s   total 17.6 secs
};
static const int16_t *tt_p0_delays = tt_p0_delays_movie;
static long tt_p0_totDelays[88];

// BTTF-Network
bool bttfnHaveClients = false;
#define BTTFN_NOT_PREPARE  1
#define BTTFN_NOT_TT       2
#define BTTFN_NOT_REENTRY  3
#define BTTFN_NOT_ABORT_TT 4
#define BTTFN_NOT_ALARM    5
#define BTTFN_NOT_REFILL   6
#define BTTFN_NOT_FLUX_CMD 7
#define BTTFN_NOT_SID_CMD  8
#define BTTFN_NOT_PCG_CMD  9
#define BTTFN_NOT_WAKEUP   10
#define BTTFN_NOT_AUX_CMD  11
#define BTTFN_NOT_VSR_CMD  12
#define BTTFN_NOT_REM_CMD  13
#define BTTFN_NOT_REM_SPD  14
#define BTTFN_NOT_SPD      15
#define BTTFN_NOT_BUSY     16
#define BTTFN_REMCMD_PING       1   // Implicit "Register"/keep-alive
#define BTTFN_REMCMD_BYE        2   // Forced unregister
#define BTTFN_REMCMD_COMBINED   3   // All switches & speed combined
#define BTTFN_REMCMD_KP_PING    4
#define BTTFN_REMCMD_KP_KEY     5
#define BTTFN_REMCMD_KP_BYE     6
#define BTTFN_SSRC_NONE         0
#define BTTFN_SSRC_GPS          1
#define BTTFN_SSRC_ROTENC       2
#define BTTFN_SSRC_REM          3
#define BTTFN_SSRC_P0           4
#define BTTFN_SSRC_P1           5
#define BTTFN_SSRC_P2           6
#define BTTFN_VERSION              1
#define BTTF_PACKET_SIZE          48
#define BTTF_DEFAULT_LOCAL_PORT 1338
#define BTTFN_MAX_CLIENTS          6
struct _bttfnClient {
    unsigned long ALIVE;
    #ifdef TC_HAVE_REMOTE
    uint32_t      RemID;
    #endif
    union {
        uint8_t   IP[4];
        uint32_t  IP32;
    };
    uint8_t       MC;
    uint8_t       Type;
    char          ID[14];
};
static const uint8_t BTTFUDPHD[BTTF_PACKET_SIZE] = { 'B', 'T', 'T', 'F', BTTFN_VERSION | 0x40, 0};
static WiFiUDP       bttfUDP;
static UDP*          tcdUDP;
static WiFiUDP       bttfmcUDP;
static UDP*          tcdmcUDP;
static byte          BTTFUDPBuf[BTTF_PACKET_SIZE];
static _bttfnClient  bttfnClient[BTTFN_MAX_CLIENTS] = { 0 };
static uint8_t       bttfnDateBuf[8];
static uint32_t      bttfnSeqCnt = 1;
static unsigned long bttfnlastExpire = 0;
static uint32_t      hostNameHash = 0;
static byte          BTTFMCBuf[BTTF_PACKET_SIZE];
static uint8_t       bttfnNotAllSupportMC = 0;
static uint8_t       bttfnAtLeastOneMC = 0;
static int16_t       oldBTTFNSpd = -2;
static uint16_t      oldBTTFNSSrc = 0xffff;
static unsigned long bttfnLastSpeedNot = 0;
#ifdef TC_HAVE_REMOTE
static uint32_t      registeredRemID  = 0;
static uint32_t      registeredRemKPID  = 0;
static uint32_t      bttfnLastSeq_co  = 0;
static uint32_t      bttfnLastSeq_ky  = 0;
#ifdef TC_HAVE_RE
static bool          bttfnBUseRotEnc;
static bool          bttfnBDispRotEnc;
#endif
#ifdef TC_HAVEGPS
static bool          bttfnBDispGPSSpeed;
static bool          bttfnBPovGPS2BTTFN;
#endif
#ifdef TC_HAVETEMP
static bool          bttfnBDispTemp;
#endif
#endif // TC_HAVE_REMOTE

#define GET32(a,b)          \
    (((a)[b])            |  \
    (((a)[(b)+1]) << 8)  |  \
    (((a)[(b)+2]) << 16) |  \
    (((a)[(b)+3]) << 24))
//#define GET32(a,b)    *((uint32_t *)((a) + (b)))
#define SET32(a,b,c)                        \
    (a)[b]       = ((uint32_t)(c)) & 0xff;  \
    ((a)[(b)+1]) = ((uint32_t)(c)) >> 8;    \
    ((a)[(b)+2]) = ((uint32_t)(c)) >> 16;   \
    ((a)[(b)+3]) = ((uint32_t)(c)) >> 24; 
//#define SET32(a,b,c)   *((uint32_t *)((a) + (b))) = c

static void myCustomDelay_Sens(unsigned long mydel);
static void myCustomDelay_GPS(unsigned long mydel);
static void myIntroDelay(unsigned int mydel, bool withGPS = true);
static void waitAudioDoneIntro();

static void startDisplays();

static bool getNTPOrGPSTime(bool weHaveAuthTime, DateTime& dt, bool updateRTC = true);
static bool getNTPTime(bool weHaveAuthTime, DateTime& dt, bool updateRTC = true);
#ifdef TC_HAVEGPS
static bool getGPStime(DateTime& dt, bool updateRTC = true);
static bool setGPStime();
bool        gpsHaveFix();
static bool gpsHaveTime();
#endif
#if defined(TC_HAVEGPS) || defined(TC_HAVE_RE)
static void displayGPSorRESpeed(bool force = false);
#endif
#ifdef TC_HAVE_REMOTE
static void updAndDispRemoteSpeed();
#endif
#ifdef TC_HAVETEMP
static void updateTemperature(bool force = false);
static bool dispTemperature(bool force = false);
#endif
static void dispIdleZero(bool force = false);
#ifdef TC_HAVE_RE                
static void re_init(bool zero = true);
static void re_lockTemp();
#endif

static void triggerLongTT(bool noLead = false);
#if (defined(TC_HAVEGPS) && defined(NOT_MY_RESPONSIBILITY)) || defined(TC_HAVE_RE)
static void checkForSpeedTT(bool isFake, bool isRemote);
#endif

static void copyPresentToDeparted(bool isReturn);

static void ettoPulseStart();
static void ettoPulseStartNoLead();
static void sendTTNetWorkMsg(uint16_t bttfnPayload, uint16_t bttfnPayload2);
static void sendNetWorkMsg(const char *pl, unsigned int len, uint8_t bttfnMsg, uint16_t bttfnPayload = 0, uint16_t bttfnPayload2 = 0);

// Time calculations
static uint64_t  dateToMins(int year, int month, int day, int hour, int minute);
static void      minsToDate(uint64_t total, int& year, int& month, int& day, int& hour, int& minute);
#ifdef TC_JULIAN_CAL
static void      calcJulianData();
#endif
static void      convTime(int diff, int& y, int& m, int& d, int& h, int& mm);

/// Native NTP
static void ntp_setup();
static bool NTPHaveTime();
static bool NTPTriggerUpdate();
static void NTPSendPacket();
static void NTPCheckPacket();
static uint32_t NTPGetSecsSinceTCepoch();
static bool NTPHaveCurrentTime();
static bool NTPGetUTC(int& year, int& month, int& day, int& hour, int& minute, int& second);

// Basic Telematics Transmission Framework
static void bttfn_setup();
static void bttfn_expire_clients();
static uint8_t* bttfn_unrollPacket(uint8_t *d, uint32_t m, int b);
static bool bttfn_handlePacket(uint8_t *buf, bool isMC);
static void bttfn_notify(uint8_t targetType, uint8_t event, uint16_t payload = 0, uint16_t payload2 = 0, uint16_t payload3 = 0);
#ifdef TC_HAVE_REMOTE
static void bttfnMakeRemoteSpeedMaster(bool doit, bool isPwrMaster);
#endif
static bool bttfn_checkmc();
static void bttfn_notify_of_speed();

/*
 * time_boot()
 *
 */
void time_boot()
{
    // Reset TT-out as early as possible
    #ifndef EXPS
    pinMode(EXTERNAL_TIMETRAVEL_OUT_PIN, OUTPUT);
    digitalWrite(EXTERNAL_TIMETRAVEL_OUT_PIN, LOW);
    #endif
   
    // Start the displays early to clear them
    startDisplays();
    
    // Switch LEDs on
    // give user some feedback that the unit is powered
    pinMode(LEDS_PIN, OUTPUT);
    digitalWrite(LEDS_PIN, HIGH);
    // Do not call leds_on(), is not unconditional
}

/*
 * time_setup()
 *
 */
void time_setup()
{
    DateTime dtu, dtl;
    bool rtcbad = false;
    bool tzbad = false;
    bool haveGPS = false;
    bool isVirgin = false;
    #ifdef TC_HAVEGPS
    int quickGPSupdates = -1;
    int speedoUpdateRate = 0;
    bool haveAuthTimeGPS = false;
    #endif
    bool playIntro = false;
    uint64_t oldTime, newTime;
    bool checkAndSaveYOffs = false;
    bool updateTimeDiff = false;
    #ifdef TC_DBG
    const char *funcName = "time_setup: ";
    #endif

    Serial.println("Time Circuits Display version " TC_VERSION " " TC_VERSION_EXTRA);

    // Power management: Set CPU speed
    // to maximum and start timer
    pwrNeedFullNow(true);

    // Pin for monitoring seconds from RTC
    pinMode(SECONDS_IN_PIN, INPUT_PULLDOWN);

    // Init fake power switch
    waitForFakePowerButton = (atoi(settings.fakePwrOn) > 0);
    fakePowerOnKey.setTiming(50, 10, 50);
    if(waitForFakePowerButton) {
        fakePowerOnKey.attachLongPressStart(fpbKeyPressed);
        fakePowerOnKey.attachLongPressStop(fpbKeyLongPressStop);
    }

    playIntro = (atoi(settings.playIntro) > 0);

    // RTC setup
    if(!rtc.begin()) {

        const char *rtcNotFound = "RTC NOT FOUND";
        uint8_t r = HIGH;
        
        destinationTime.showTextDirect(rtcNotFound);
        Serial.printf("%s\n", rtcNotFound);

        // Blink white LED forever
        while(1) {
            digitalWrite(WHITE_LED_PIN, r);
            for(int i = 0; i < 10; i++) {
                delay(100);
                wifi_loop();
            }
            r ^= HIGH;
        }
    }

    #ifdef HAVE_PCF2129
    RTCNeedsOTPR = rtc.NeedOTPRefresh();
    #endif

    if(rtc.lostPower()) {

        // Lost power and battery didn't keep time, so set current time to 
        // default time 1/1/2024, 0:0
        rtc.adjust(0, 0, 0, dayOfWeek(1, 1, 2024), 1, 1, 24);
       
        #ifdef TC_DBG
        Serial.printf("%sRTC lost power, setting default time\n", funcName);
        #endif

        rtcbad = true;
    }

    #ifdef HAVE_PCF2129
    if(RTCNeedsOTPR) {
        rtc.OTPRefresh(true);
        delay(100);
        rtc.OTPRefresh(false);
        delay(100);
        OTPRDoneNow = millis();
    }
    #endif

    // Turn on the RTC's 1Hz clock output
    rtc.clockOutEnable();

    // Calculate data for Julian Calendar
    #ifdef TC_JULIAN_CAL
    calcJulianData();
    #endif

    // Start (reset) the displays
    startDisplays();

    {
        // Initialize clock mode: 12 hour vs 24 hour
        bool tempmode = (atoi(settings.mode24) > 0);
        presentTime.set1224(tempmode);
        destinationTime.set1224(tempmode);
        departedTime.set1224(tempmode);

        tempmode = (atoi(settings.revAmPm) > 0);
        presentTime.setAMPMOrder(tempmode);
        destinationTime.setAMPMOrder(tempmode);
        departedTime.setAMPMOrder(tempmode);
    }

    #ifndef IS_ACAR_DISPLAY
    p3anim = (atoi(settings.p3anim) > 0);
    #endif
    skipTTAnim = (atoi(settings.skipTTAnim) > 0);

    // Configure presentTime as a display that will hold real time
    presentTime.setRTC(true);

    // Configure behavior in night mode
    destinationTime.setNMOff((atoi(settings.dtNmOff) > 0));
    presentTime.setNMOff((atoi(settings.ptNmOff) > 0));
    departedTime.setNMOff((atoi(settings.ltNmOff) > 0));

    // Determine if user wanted Time Travels to be persistent
    // Requires SD card and "Save secondary settings to SD" to
    // be checked if PERSISTENT_SD_ONLY is #defined
    #ifdef PERSISTENT_SD_ONLY
    if(presentTime._configOnSD) {
    #endif  
        timetravelPersistent = (atoi(settings.timesPers) > 0);
    #ifdef PERSISTENT_SD_ONLY
    } else {
        timetravelPersistent = false;
    }
    #endif
    
    // Set initial brightness for present time displqy
    presentTime.setBrightness(atoi(settings.presTimeBright), true);
    
    // Load present time clock state (lastYear, yearOffs)
    {
        int16_t tyo;
        lastYear = presentTime.loadClockStateData(tyo);
        presentTime.setYearOffset(tyo);
    }

    // Load timeDifference
    presentTime.load();

    if(rtcbad) {
        // Default date (see above) requires yoffs 0
        presentTime.setYearOffset(0);
    }

    // Read current time in order to validate/correct timeDifference
    if(timetravelPersistent && timeDifference && !rtcbad) {
        // Load UTC time from RTC
        myrtcnow(dtu);
        oldTime = dateToMins(dtu.year(), dtu.month(), dtu.day(), dtu.hour(), dtu.minute());
        if(timeDiffUp) {
            oldTime += timeDifference;
        } else {
            oldTime -= timeDifference;
        }
        if(oldTime >= 
        #ifndef TC_JULIAN_CAL
                      mins1kYears[(10000 / 500)]
        #else
                      mins1kYears[(10000 / 100)]
        #endif
                                                ) {
            timeDifference = 0;
        }
    } else {
        timeDifference = 0;
    }

    // See if speedo (display) is to be used
    {
        int temp = atoi(settings.speedoType);
        
        if(temp >= SP_NUM_TYPES) {
            // if 99(="None") or bad: Fall back to BTTFN-speedo
            temp = SP_BTTFN;
            bttfnSpeedoFallback = true;
        }
        
        if(!(useSpeedo = speedo.begin(temp))) {
            #ifdef TC_DBG
            Serial.printf("%sSpeedo type %d not found\n", temp, funcName);
            #endif
            // if not found: Fall back to BTTFN-speedo
            if(!(useSpeedo = speedo.begin(SP_BTTFN))) {
                #ifdef TC_DBG
                Serial.printf("%sBTTFN-speedo init failed\n", funcName);
                #endif
            } else {
                bttfnSpeedoFallback = true;
            }
        }

        useSpeedoDisplay = useSpeedo && (speedo.haveSpeedoDisplay() || !bttfnSpeedoFallback);

        // useSpeedo is true if tt sequences should potentially be done with P0 and P2
        //              false if tt sequences will never be done with P0 or P2
        // useSpeedoDisplay is true if a speed-displaying device is present
        //                     false if none, or if BTTFN only as fallback
    }

    // Set up GPS receiver
    #ifdef TC_HAVEGPS
    useGPS = true;    // Use if detected
    
    if(useSpeedoDisplay) {
        // 'dispGPSSpeed' means "display GPS speed on speedo"
        // False if no GPS receiver found, or no speedo found, or option "Display GPS speed" unchecked
        dispGPSSpeed = (atoi(settings.dispGPSSpeed) > 0);
    }

    havePreTTSound = check_file_SD(preTTSound);
    haveAbortTTSound = check_file_SD(abortTTSound);

    provGPS2BTTFN = (atoi(settings.quickGPS) > 0);
    quickGPSupdates = dispGPSSpeed ? 1 : (provGPS2BTTFN ? 0 : -1);
    speedoUpdateRate = atoi(settings.spdUpdRate) & 3;

    // Check for GPS receiver
    // Do so regardless of usage in order to eliminate
    // VEML7700 light sensor with identical i2c address
    if(myGPS.begin(quickGPSupdates, speedoUpdateRate, myCustomDelay_GPS)) {

        haveGPS = true;
          
        // Clear so we don't add to stampAge unnecessarily in
        // boot strap
        GPSupdateFreq = 0;
        
        // We know now we have a possible source for auth time
        couldHaveAuthTime = true;
        
        // Fetch data already in Receiver's buffer [120ms]
        for(int i = 0; i < 10; i++) myGPS.loop(true);
        lastLoopGPS = millis64();
        
        #ifdef TC_DBG
        Serial.printf("%sGPS Receiver found\n", funcName);
        #endif

    } else {
      
        useGPS = dispGPSSpeed = false;

        provGPS2BTTFN = false;
        
        #ifdef TC_DBG
        Serial.printf("%sGPS Receiver not found\n", funcName);
        #endif
        
    }
    #endif

    // If a WiFi network and an NTP server are configured, we might
    // have a source for auth time.
    // (Do not involve WiFi connection status here; might change later)
    if(settings.ntpServer[0] && wifiHaveSTAConf)
        couldHaveAuthTime = true;
    
    // Try to obtain initial authoritative time

    ntp_setup();
    if(settings.ntpServer[0] && (WiFi.status() == WL_CONNECTED)) {
        int timeout = 50;
        do {
            ntp_loop();
            delay(100);
            timeout--;
        } while (!NTPHaveTime() && timeout);
    }

    // Parse TZ to check validity and to get difference to UTC
    // (Year does not matter at this point)
    if(!(parseTZ(0, 2022))) {
        tzbad = true;
        #ifdef TC_DBG
        Serial.printf("%s%s", funcName, badTZ);
        #endif
    }
    
    // Set RTC with NTP time
    if(getNTPTime(true, dtu, true)) {

        // So we have authoritative time now
        haveAuthTime = true;
        lastAuthTime = millis();
        lastAuthTime64 = millis64();
        
        #ifdef TC_DBG
        Serial.printf("%sRTC set through NTP\n", funcName);        
        #endif
        
    } else {
      
        // GPS might have a fix, so try fetching time from GPS
        #ifdef TC_HAVEGPS
        if(useGPS) {

            // Pull old data from buffer
            for(int i = 0; i < 10; i++) myGPS.loop(true);

            for(int i = 0; i < 10; i++) {
                for(int j = 0; j < 4; j++) myGPS.loop(true);
                #ifdef TC_DBG
                if(i == 0) Serial.printf("%sFirst attempt to read time from GPS\n", funcName);
                #endif
                if(getGPStime(dtu, true)) {
                    // So we have authoritative time now
                    haveAuthTime = true;
                    lastAuthTime = millis();
                    lastAuthTime64 = millis64();
                    haveAuthTimeGPS = true;
                    #ifdef TC_DBG
                    Serial.printf("%sRTC set through GPS\n", funcName);
                    #endif
                    break;
                }
                yield();
                delay(100);
            }
            // If we haven't managed to get time here, try
            // a sync in shorter intervals in time_loop.
            if(!haveAuthTimeGPS) resyncInt = 2;
            lastLoopGPS = millis64();
        }
        #endif
    }

    // Start the Config Portal. Now is a good time.
    if(WiFi.status() == WL_CONNECTED) {
        if(playIntro || (waitForFakePowerButton && 
                        (digitalRead(FAKE_POWER_BUTTON_PIN) == HIGH))) {
            wifiStartCP();        
        } else {
            deferredCP = true;
        }
    }

    // Preset this for BTTFN status requests during boot
    #ifdef TC_HAVEMQTT
    if(MQTTPwrMaster) {
        FPBUnitIsOn = shadowMQTTFPBKeyPressed = !MQTTWaitForOn;
    } else 
    #endif
           if(waitForFakePowerButton) {
        FPBUnitIsOn = false;
    }
    
    // Start bttf network
    bttfn_setup();

    // Load UTC time from RTC (requires valid yearOffset)
    myrtcnow(dtu);

    // rtcYear: Current UTC-year (as read from the RTC, minus Yoffs)
    uint16_t rtcYear = dtu.year();

    // lastYear: The UTC-year when the RTC was adjusted for the last time.
    // If the RTC was just updated (haveAuthTime), everything is in place.
    // Otherwise use lastYear loaded from NVM, compare it to RTC year, and 
    // make required re-calculations (based on RTC year) if they don't match.
    if(haveAuthTime) {
        lastYear = rtcYear;
    }

    // If we are switched on after a year switch, re-do
    // RTC year-translation (UTC)
    if(lastYear != rtcYear) {

        // Get RTC-fit year & offs for real year
        int16_t yOffs = 0;
        uint16_t newYear = rtcYear;
        correctYr4RTC(newYear, yOffs);

        // If year-translation changed, update RTC and save
        if((newYear != dtu.hwRTCYear) || (yOffs != presentTime.getYearOffset())) {

            // Update RTC (UTC)
            rtc.adjust(dtu.second(), 
                       dtu.minute(), 
                       dtu.hour(), 
                       dayOfWeek(dtu.day(), dtu.month(), rtcYear),
                       dtu.day(), 
                       dtu.month(), 
                       newYear-2000
            );

            presentTime.setYearOffset(yOffs);

            dtu.hwRTCYear = newYear;
            dtu.setYear(rtcYear);

        }

        lastYear = rtcYear;
    }

    // Write lastYear and yearOffset (UTC) to NVM (if modified)
    presentTime.saveClockStateData(lastYear);

    // Correct timeDifference
    if(timetravelPersistent && timeDifference) {
        // oldTime = time travel target time based on RTC-read UTC time
        newTime = dateToMins(dtu.year(), dtu.month(), dtu.day(), dtu.hour(), dtu.minute());
        if(oldTime > newTime) {
            timeDifference = oldTime - newTime;
            timeDiffUp = true;
        } else {
            timeDifference = newTime - oldTime;
            timeDiffUp = false;
        }
        presentTime.save();
    }

    // Convert UTC to local (also parses timezone)
    UTCtoLocal(dtu, dtl, 0);

    // Parse alternate time zones for WC for testing their validity
    if(settings.timeZoneDest[0] != 0) {
        if(parseTZ(1, dtl.year())) {
            WcHaveTZ1 = true;
        }
    }
    if(settings.timeZoneDep[0] != 0) {
        if(parseTZ(2, dtl.year())) {
            WcHaveTZ2 = true;
        }
    }
    if((haveWcMode = (WcHaveTZ1 || WcHaveTZ2))) {
        if(WcHaveTZ1 && settings.timeZoneNDest[0] != 0) {
            destinationTime.setAltText(settings.timeZoneNDest);
            haveTZName1 = true;
        }
        if(WcHaveTZ2 && settings.timeZoneNDep[0] != 0) {
            departedTime.setAltText(settings.timeZoneNDep);
            haveTZName2 = true;
        }
    }

    // Init Exhibition mode and load time to presentTime display
    loadStaleTime((void *)&stalePresentTime[0], stalePresent);
    if(!timetravelPersistent) {
        memcpy((void *)&stalePresentTime[1], (void *)&stalePresentTime[0], sizeof(dateStruct));
    }
    if(stalePresent)
        presentTime.setFromStruct(&stalePresentTime[1]);
    else
        updatePresentTime(dtu, dtl);

    // Set GPS receiver's RTC
    #ifdef TC_HAVEGPS
    if(useGPS) {
        if(!haveAuthTimeGPS && (haveAuthTime || !rtcbad)) {
            setGPStime();
        }
        
        // Set ms between GPS polls
        // Need more updates if speed is to be displayed
        // RMC is 72-85 chars, so three fit in the 255 byte buffer.
        // VTG is 40+ chars, so six fit into the buffer.
        // We use VTG+RMC+ZDA when displaying speedo on speedo at 200/250ms rates.
        // Otherwise we go with RMC+ZDA.
        // (Must sync readFreq and updFreq for smooth speedo display)

        if(quickGPSupdates < 0) {
            // For time only, the fix update rate is set to 5000ms;
            // We poll every 500ms, so the entire buffer is read every 
            // 2 seconds (64 bytes per poll).
            GPSupdateFreq = 500;
        } else if(!quickGPSupdates) {
            // For GPS speed for BTTFN clients (but no speed display
            // on speedo), the fix update rate is set to 1000ms/500ms,
            // so the buffer fills in 3000/1500ms. We poll every 
            // 250/250ms, hence the entire buffer is read in
            // 1000/1000ms (64 bytes per poll).
            // The max delay to get current speed is therefore 500ms.
            GPSupdateFreq = 250; //(!speedoUpdateRate) ? 250 : 250;
        } else {
            // For when GPS speed is to be displayed on speedo:
            // We read 128 byte blocks from the receiver in this case.
            switch(speedoUpdateRate) {
            case 0:
                // At 1000ms update rate, the buffer fills in 3000ms (with ZDA: 2900)
                // At 500ms readfreq (128 bytes blocks), the entire buffer is read in 1000ms
                // At 250ms readfreq (128 bytes blocks), the entire buffer is read in 500ms
                // We want as little delay as possible with displaying speed, so we go 250.
                //GPSupdateFreq = 500;
                //break;
            case 1:
                // At 500ms update rate, the buffer fills in 1500ms (with ZDA: 1400)
                // At 500ms readfreq (128 bytes blocks), the entire buffer is read in 1000ms
                // At 250ms readfreq (128 bytes blocks), the entire buffer is read in 500ms
                // We want as little delay as possible with displaying speed, so we go 250.
                //GPSupdateFreq = 500;
                //break;
            case 2:
                // With VTG: At 250ms update rate, the buffer fills in 1500ms (with RMC+ZDA: 1400ms on avg)
                // At 250ms readfreq (128 bytes blocks), the entire buffer is read in 500ms
                GPSupdateFreq = 250;
                break;
            case 3:
                // With VTG: At 200ms update rate, the buffer fills in 1200ms (with RMC+ZDA: 1100ms on avg)
                // At 200ms readfreq (128 bytes blocks), the entire buffer is read in 400ms
                GPSupdateFreq = 200;
                break;
            }
        }
    }
    #endif

    // Set initial brightness for dest & last time dep displays
    destinationTime.setBrightness(atoi(settings.destTimeBright), true);
    departedTime.setBrightness(atoi(settings.lastTimeBright), true);

    // Load destination time (and set to default if invalid)
    if(!destinationTime.load()) {
        destinationTime.setFromStruct(&destinationTimes[0]);
        destinationTime.save();
        isVirgin = true;
    }

    // Load departed time (and set to default if invalid)
    if(!departedTime.load()) {
        departedTime.setFromStruct(&departedTimes[0]);
        departedTime.save();
    }

    // If time cycling enabled, put up the first one
    if(autoTimeIntervals[autoInterval]) {
        destinationTime.setFromStruct(&destinationTimes[0]);
        departedTime.setFromStruct(&departedTimes[0]);
    }

    // Load alarm from alarmconfig file
    // Don't care if data invalid, alarm is off in that case
    loadAlarm();

    // Load yearly/monthly reminder settings
    loadReminder();

    // Handle early BTTFN requests (first call)
    // At this point everything that could be used/changed
    // by BTTFN commands (especially the Remote) must be
    // set up. Remote is kept from becoming speedmaster
    // through bootStrap flag. It can already become
    // powermaster, however.
    while(bttfn_loop()) {}

    // Auto-NightMode
    autoNightModeMode = atoi(settings.autoNMPreset);
    if(autoNightModeMode > AUTONM_NUM_PRESETS) autoNightModeMode = 10;
    autoNightMode = (autoNightModeMode != 10);
    autoNMOnHour = atoi(settings.autoNMOn);
    if(autoNMOnHour > 23) autoNMOnHour = 0;
    autoNMOffHour = atoi(settings.autoNMOff);
    if(autoNMOffHour > 23) autoNMOffHour = 0;
    if(autoNightMode && (autoNightModeMode == 0)) {
        if((autoNightMode = (autoNMOnHour != autoNMOffHour))) {
            if(autoNMOnHour < autoNMOffHour) {
                for(int i = autoNMOnHour; i < autoNMOffHour; i++)
                    autoNMdailyPreset |= (1 << (23-i));
            } else {
                autoNMdailyPreset = 0b111111111111111111111111;
                for(int i = autoNMOffHour; i < autoNMOnHour; i++)
                    autoNMdailyPreset &= ~(1 << (23-i));
            }
        }
    }
    if(autoNightMode) forceReEvalANM = true;

    // Set up alarm base: RTC or current "present time"
    alarmRTC = (atoi(settings.alarmRTC) > 0);

    // Set up option to play/mute time travel sounds
    playTTsounds = (atoi(settings.playTTsnds) > 0);

    // Set power-up setting for beep
    muteBeep = true;
    if(beepMode >= 3) {
        beepMode = 3;
        beepTimeout = BEEPM3_SECS*1000;
    } else if(beepMode == 2) 
        beepTimeout = BEEPM2_SECS*1000;

    // Set up speedo display
    if(!useSpeedo || (!atoi(settings.speedoAF))) {
        //tt_p0_delays = tt_p0_delays_movie;  // already initialized
        ttP0TimeFactor = 1.0;
    } else {
        tt_p0_delays = tt_p0_delays_rl;
        ttP0TimeFactor = (float)strtof(settings.speedoFact, NULL);
    }
    
    if(useSpeedo) {
      
        if(useSpeedoDisplay) {
            speedo.setBrightness(atoi(settings.speedoBright), true);
            
            // Negate this flag, option is "Switch off", not "Keep on"
            speedoAlwaysOn = !(atoi(settings.speedoAO) > 0);
    
            // P1/P2 vs P3 speedo style
            if(atoi(settings.speedoP3) > 0) {
                speedo.dispL0Spd = true;
                speedo.thirdDig = false;
                speedo.setDot(false);
            } else {
                speedo.dispL0Spd = false;
                speedo.thirdDig &= (atoi(settings.speedo3rdD) > 0);
                speedo.setDot(true);
            }
        }

        // No TT sounds to play -> no user-provided sound.
        if(!playTTsounds) havePreTTSound = false;

        // Speed factor for acceleration curve
        if(ttP0TimeFactor < 0.5) ttP0TimeFactor = 0.5;
        if(ttP0TimeFactor > 5.0) ttP0TimeFactor = 5.0;

        // Calculate start point of P1 sequence
        for(int i = 1; i < 88; i++) {
            pointOfP1 += (unsigned long)(((float)(tt_p0_delays[i])) / ttP0TimeFactor);
        }
        ettoBase = pointOfP1;
        ettoLeadPoint = origEttoLeadPoint = ettoBase - ettoLeadTime;   // Can be negative!
        pointOfP1NoLead = pointOfP1;  // for lead-less P1
        pointOfP1 -= TT_P1_POINT88;   // for normal P1

        {
            // Calculate total elapsed time for each mph value
            // (in order to time P0/P1 relative to current actual speed)
            long totDelay = 0;
            for(int i = 0; i < 88; i++) {
                totDelay += (long)(((float)(tt_p0_delays[i])) / ttP0TimeFactor);
                tt_p0_totDelays[i] = totDelay;
            }
        }

        if(useSpeedoDisplay) {
            speedo.off();
    
            #ifdef TC_HAVEGPS
            if(dispGPSSpeed) {
                // display (actual) speed, regardless of fake power
                displayGPSorRESpeed(true);
                speedo.on(); 
            } else {
            #endif
                if(speedoAlwaysOn) {
                    if(FPBUnitIsOn) {
                        speedo.setSpeed(0);
                        speedo.on();
                        speedo.show();
                        speedoStatus = SPST_ZERO;
                    }
                }
            #ifdef TC_HAVEGPS
            }
            #endif
        }
        
    }

    // Set up rotary encoders
    // There are primary and secondary i2c addresses for RotEncs.
    // Use primary RotEnc for speed, if either no speedo is present, or,
    // if a speedo is present, GPS speed is not to be displayed on it; or
    // if no GPS receiver is present.
    #ifdef TC_HAVE_RE
    if(!dispGPSSpeed) {
        if(rotEnc.begin(true)) {
            useRotEnc = true;
            dispRotEnc = useSpeedoDisplay;
            re_init();
            re_lockTemp();
            fakeSpeed = oldFSpd = rotEnc.updateFakeSpeed(true);
            if(FPBUnitIsOn) {
                if(dispRotEnc) {
                    displayGPSorRESpeed(true);
                    speedo.on();
                }
            }
            // Temperature-display is now blocked by tempLock
        }
    }
    // Check for secondary RotEnc for volume on secondary i2c addresses
    if(rotEncV.begin(false)) {
        useRotEncVol = true;
        rotEncVol = &rotEncV;
        re_vol_reset();
    }
    #endif

    // Handle early BTTFN requests
    while(bttfn_loop()) {}

    // Set up temperature sensor
    #ifdef TC_HAVETEMP
    useTemp = true;   // Use if detected
    if(!useSpeedoDisplay || dispGPSSpeed || !speedo.supportsTemperature()) {
        dispTemp = false;
    } else {
        dispTemp = (atoi(settings.dispTemp) > 0);
    }
    if(tempSens.begin(myCustomDelay_Sens)) {
        tempUnit = (atoi(settings.tempUnit) > 0);
        tempSens.setOffset((float)strtof(settings.tempOffs, NULL));
        haveRcMode = true;
        tempBrightness = atoi(settings.tempBright);
        tempOffNM = (atoi(settings.tempOffNM) > 0);
        if(dispTemp) {
            if(FPBUnitIsOn) {
                updateTemperature(true);
                dispTemperature(true);
            }
        }
    } else {
        useTemp = dispTemp = false;
    }
    #else
    useTemp = dispTemp = false;
    #endif

    #ifdef TC_HAVELIGHT
    useLight = (atoi(settings.useLight) > 0);
    luxLimit = atoi(settings.luxLimit);
    if(useLight) {
        if(!lightSens.begin(haveGPS, myCustomDelay_Sens)) {
            useLight = false;
        }
    }
    #else
    useLight = false;
    #endif

    // Animate time cycling?
    if(!(autoRotAnim = (atoi(settings.autoRotAnim) > 0)))
        autoIntSec = 0;

    // Evaluate TT-OUT settings and setup flags accordingly
    if(atoi(settings.useETTO) > 0) {
        useETTOWiredNoLead = (atoi(settings.noETTOLead) > 0);
        ETTWithFixedLead = useETTOWired = !useETTOWiredNoLead;
    }
    if(atoi(settings.ETTOalm) > 0) {
        ETTOalarm = true;
        ETTOAlmDur = atoi(settings.ETTOAD) * 1000;
    }
    if(atoi(settings.ETTOcmd) > 0) {
        ETTOcommands = true;
        if(atoi(settings.ETTOpus) > 0) {
            setTTOUTpin(HIGH);
        }
    }

    // MQTT: If 'extended TIMETRAVEL command' is to be used,
    // we need to lead. Otherwise, we do.
    #ifdef TC_HAVEMQTT
    if(pubMQTT) {
        if(MQTTvarLead) pubMQTTVL = true;           // No lead needed
        else            ETTWithFixedLead = true;    // Lead needed
    }
    #endif
    // When ETTWithFixedLead is true, the 5 sec lead is needed (for wired and/or MQTT)
    // When ETTWithFixedLead is false, no lead is ever needed.
    
    // Show "REPLACE BATTERY" message if RTC battery is low or depleted
    // Note: This also shows up the first time you power-up the clock
    // AFTER a battery change.
    if((rtcbad && !isVirgin) || rtc.battLow()) {
        destinationTime.showTextDirect("REPLACE");
        presentTime.showTextDirect("BATTERY");
        destinationTime.on();
        presentTime.on();
        myIntroDelay(5000);
        allOff();
    } else {
        while(bttfn_loop()) {}
    }

    if(tzbad) {
        destinationTime.showTextDirect("BAD");
        presentTime.showTextDirect("TIME ZONE");
        destinationTime.on();
        presentTime.on();
        myIntroDelay(5000);
        allOff();
    }

    // Invoke audio file installer if SD content qualifies
    if(check_allow_CPA()) {
        doCopyAudioFiles();
        // We never return here
    }

    if(!haveAudioFiles) {
        destinationTime.showTextDirect("PLEASE");
        presentTime.showTextDirect("INSTALL");
        departedTime.showTextDirect("SOUND PACK");
        destinationTime.on();
        presentTime.on();
        departedTime.on();
        myIntroDelay(5000);
        allOff();
    }

    if(playIntro) {
        const char *t1 = "             TIME";
        const char *t2 = "             CIRCUITS";
        const char *t3 = "ON";

        timeTravelP0 = true;    // abuse this here to suppress WiFi scan

        play_file("/intro.mp3", PA_LINEOUT|PA_CHECKNM|PA_INTRMUS|PA_ALLOWSD|PA_DYNVOL);

        myIntroDelay(1200);
        destinationTime.setBrightnessDirect(15);
        presentTime.setBrightnessDirect(15);
        departedTime.setBrightnessDirect(0);
        departedTime.off();
        destinationTime.showTextDirect(t1);
        presentTime.showTextDirect(t2);
        departedTime.showTextDirect(t3);
        destinationTime.on();
        presentTime.on();
        for(int i = 0; i < 14; i++) {
           myIntroDelay(50, false);
           destinationTime.showTextDirect(&t1[i]);
           presentTime.showTextDirect(&t2[i]);
        }
        myIntroDelay(500);
        departedTime.on();
        for(int i = 0; i <= 15; i++) {
            departedTime.setBrightnessDirect(i);
            myIntroDelay(100);
        }
        myIntroDelay(1500);
        for(int i = 15; i >= 0; i--) {
            destinationTime.setBrightnessDirect(i);
            presentTime.setBrightnessDirect(i);
            departedTime.setBrightnessDirect(i);
            myIntroDelay(20, false);
        }
        allOff();
        destinationTime.setBrightness(255);
        presentTime.setBrightness(255);
        departedTime.setBrightness(255);

        waitAudioDoneIntro();
        stopAudio();

        timeTravelP0 = false;
    } else {
        while(bttfn_loop()) {}
    }

    {
        bool bootNow = true;
        
        if(!FPBUnitIsOn || bttfnRemPwrMaster) {
            fakePowerOnKey.scan();
            myIntroDelay(70);     // Bridge debounce/longpress time
            fakePowerOnKey.scan();
            if(!isFPBKeyPressed) {
                leds_off();
                FPBUnitIsOn = false;
                bootNow = false;
                digitalWrite(WHITE_LED_PIN, HIGH);
                myIntroDelay(500);
                digitalWrite(WHITE_LED_PIN, LOW);
                #ifdef TC_DBG
                Serial.printf("%swaiting for fake power on\n", funcName);
                #endif
            }            
            isFPBKeyChange = false;
        }  
           
        if(bootNow) {
    
            startup = true;
            startupSound = true;
            FPBUnitIsOn = true;
            leds_on();
            if(beepMode >= 2)      startBeepTimer();
            else if(beepMode == 1) muteBeep = false;
    
        }
    }

    if(deferredCP) deferredCPNow = millis();

    bootStrap = false;
}

/*
 * time_loop()
 *
 */
void time_loop()
{
    unsigned long millisNow = millis();
    #ifdef TC_DBG
    int dbgLastMin;
    const char *funcName = "time_loop: ";
    #endif

    if(waitForFakePowerButton || MQTTPwrMaster || bttfnRemPwrMaster || restoreFakePwr) {

        restoreFakePwr = false;
        
        fakePowerOnKey.scan();

        if(isFPBKeyChange) {
            if(isFPBKeyPressed) {
                if(!FPBUnitIsOn) {
                    startup = true;
                    startupSound = true;
                    FPBUnitIsOn = true;
                    if(beepMode >= 2)      startBeepTimer();
                    else if(beepMode == 1) muteBeep = false;
                    destinationTime.setBrightness(255); // restore brightnesses
                    presentTime.setBrightness(255);     // in case we got switched
                    departedTime.setBrightness(255);    // off during time travel
                    #ifdef TC_HAVE_RE
                    if(useRotEnc) {
                        re_init();
                        re_lockTemp();
                        GPSabove88 = false;
                    }
                    #endif

                    if(useSpeedoDisplay && !dispGPSSpeed && !bttfnRemoteSpeedMaster) {
                        if(speedoAlwaysOn 
                        #ifdef TC_HAVE_RE
                                          || dispRotEnc
                        #endif
                                                       ) {
                            speedo.setSpeed(0);
                            speedo.setBrightness(255);
                            speedo.show();
                            #ifdef TC_HAVE_RE
                            speedoStatus = dispRotEnc ? SPST_RE : SPST_ZERO;
                            #else
                            speedoStatus = SPST_ZERO;
                            #endif
                        }
                    }
                    
                    #ifdef TC_HAVETEMP
                    updateTemperature(true);
                    dispTemperature(true);
                    #endif
                    triggerWC = true;
                    destShowAlt = depShowAlt = 0; // Reset WC's TZ-Name-Animation
                }
            } else {
                if(FPBUnitIsOn) {
                    startup = false;
                    startupSound = false;
                    triggerP1 = false;
                    triggerETTO = false;
                    ettoPulseEnd();
                    send_abort_msg();
                    #ifdef TC_HAVE_REMOTE
                    if(timeTravelP0 && bttfnRemoteSpeedMaster) {
                        // Need to fake if P0 was running
                        // (See comment when cancelling P0)
                        bttfnRemoteSpeed = timeTravelP0Speed;
                    }
                    #endif
                    timeTravelP0 = 0;
                    timeTravelP1 = 0;
                    timeTravelRE = false;
                    timeTravelP2 = 0;
                    timeTravelP0stalled = 0;
                    #ifdef TC_HAVE_REMOTE
                    remoteInducedTT = false;
                    #endif
                    FPBUnitIsOn = false;
                    cancelEnterAnim(false);
                    cancelETTAnim();
                    mp_stop();
                    stopAudio();
                    flushDelayedSave();
                    play_file("/shutdown.mp3", PA_INTSPKR|PA_CHECKNM|PA_INTRMUS|PA_ALLOWSD);
                    mydelay(130);
                    allOff();
                    leds_off();
                    if(useSpeedoDisplay && !dispGPSSpeed && !bttfnRemoteSpeedMaster) speedo.off();
                }
            }
            isFPBKeyChange = false;
            if(deferredCP) deferredCPNow = millis();
        }
    }

    if(deferredCP && (millis() - deferredCPNow > 4000)) {
        wifiStartCP();
        deferredCP = false;
    }

    // Initiate startup delay, play startup sound
    if(startupSound) {
        startupNow = pauseNow = millis();
        play_file("/startup.mp3", PA_INTSPKR|PA_CHECKNM|PA_INTRMUS|PA_ALLOWSD);
        startupSound = false;
        // Don't let autoInt interrupt us
        autoPaused = true;
        pauseDelay = STARTUP_DELAY + 500;
    }

    // Turn display on after startup delay
    if(startup && (millis() - startupNow >= STARTUP_DELAY)) {
        animate(true);
        startup = false;
        if(useSpeedoDisplay && !dispGPSSpeed && !bttfnRemoteSpeedMaster) {
            #ifdef TC_HAVE_RE
            if(dispRotEnc) {
                speedo.on();
            } else {
            #endif
                #ifdef TC_HAVETEMP
                updateTemperature(true);
                if(dispTemp) {
                    dispTemperature(true);
                } else {
                #endif
                    if(speedoAlwaysOn) {
                        dispIdleZero(true);
                    } else {
                        speedo.off(); // Yes, off.
                    }
                #ifdef TC_HAVETEMP
                }
                #endif
            #ifdef TC_HAVE_RE
            }
            #endif    
        }
    }

    // Timer for starting P1 if to be delayed
    if(triggerP1 && (millis() - triggerP1Now >= triggerP1LeadTime)) {
        triggerLongTT(triggerP1NoLead);
        triggerP1 = false;
    }

    // Timer for start of ETTO signal
    if(triggerETTO && (millis() - triggerETTONow >= triggerETTOLeadTime)) {
        sendTTNetWorkMsg(bttfnTTLeadTime, TT_P1_EXT);
        ettoPulseStart();
        triggerETTO = false;
        #ifdef TC_DBG
        Serial.println("ETTO triggered");
        #endif
    }

    // Time travel animation, phase 0: Speed counts up
    if(timeTravelP0 && (millis() - timetravelP0Now >= timetravelP0Delay)) {

        unsigned long univNow = millis();
        long timetravelP0DelayT = 0;

        timeTravelP0stalled = 0;

        timeTravelP0Speed++;

        if(timeTravelP0Speed < 88) {
            unsigned long ttP0NowT = univNow;
            long ttP0LDOver = 0, ttP0LastDelay = ttP0NowT - ttP0Now;
            ttP0Now = ttP0NowT;
            ttP0LDOver = ttP0LastDelay - timetravelP0Delay;
            timetravelP0DelayT = (long)(((float)(tt_p0_delays[timeTravelP0Speed])) / ttP0TimeFactor) - ttP0LDOver;
            while(timetravelP0DelayT <= 0 && timeTravelP0Speed < 88) {
                timeTravelP0Speed++;
                timetravelP0DelayT += (long)(((float)(tt_p0_delays[timeTravelP0Speed])) / ttP0TimeFactor);
            }
        }

        if(timeTravelP0Speed < 88) {

            timetravelP0Delay = timetravelP0DelayT;
            timetravelP0Now = univNow;

        } else {

            timeTravelP0 = 0;

        }

        speedo.setSpeed(timeTravelP0Speed);
        speedo.show();

        // Overwrite fakeSpeed/bttfnRemCurSpd for BTTFN clients who keep polling
        // until P1-ETTO_LEAD
        #ifdef TC_HAVE_RE
        if(useRotEnc) {
            // Need to keep -1, otherwise we can't detect if enc was off
            // ahead of the tt, see also P2
            fakeSpeed = rotEnc.IsOff() ? -1 : timeTravelP0Speed;
        }
        #endif
        #ifdef TC_HAVE_REMOTE
        // Update for BTTFN clients
        bttfnRemCurSpd = timeTravelP0Speed;
        #endif

        // P0 is cancelled if brake is hit on the Remote
        // but not if P1 has already started
        #ifdef TC_HAVE_REMOTE
        if(bttfnRemoteSpeedMaster && timeTravelP0 && bttfnRemStop) {
            if(timeTravelP1 <= 0) {
                if(!triggerETTO) {
                    // Send abort (only if TT was already signalled)
                    send_abort_msg();
                    ettoPulseEnd();
                }
                timeTravelP0 = 0;
                // Fake for following updAndDispRemoteSpeed()-call.
                // (Remote does not send updates while in P0 [which
                // would be pointless as it would only mirror our P0
                // speed], so bttfnRemoteSpeed is outdated now)
                // (This is also needed when fake-switched off)
                bttfnRemoteSpeed = timeTravelP0Speed;
                triggerP1 = false;
                triggerETTO = false;
                timeTravelP0stalled = 0;
                if(playTTsounds) {
                    if(haveAbortTTSound) {
                        play_file(abortTTSound, PA_LINEOUT|PA_CHECKNM|PA_INTRMUS|PA_ALLOWSD|PA_DYNVOL);
                    } else {
                        stopAudio();
                    }
                }
                // useRotEnc=false while Remote=SpeedMaster, no reset required
            }
        }
        #endif
    }

    // Time travel animation, phase 2: Speed counts down
    if(timeTravelP2 && (millis() - timetravelP0Now >= timetravelP0Delay)) {
        bool    countToGPSSpeed = false;
        uint8_t targetSpeed = 0;
        #ifdef TC_HAVE_REMOTE
        if(bttfnRemoteSpeedMaster) {
            countToGPSSpeed = true;
            targetSpeed = bttfnRemStop ? 0 : bttfnRemoteSpeed;
        } else
        #endif
        {
            #ifdef TC_HAVEGPS
            int16_t tgpsSpd = myGPS.getSpeed();
            countToGPSSpeed = (dispGPSSpeed && (tgpsSpd >= 0));
            targetSpeed = countToGPSSpeed ? tgpsSpd : 0;
            #endif
        }
        if((timeTravelP0Speed <= targetSpeed) || (targetSpeed >= 88)) {
            timeTravelP2 = 0;
            #ifdef NOT_MY_RESPONSIBILITY
            if(countToGPSSpeed && targetSpeed >= 88) {
                // Avoid an immediately repeated time travel
                // if speed increased in the meantime
                // (For button-triggered TT while speed < 88)
                GPSabove88 = true;
            }
            #endif
            #ifdef TC_HAVE_RE
            if(useRotEnc) {
                // We MUST reset the encoder; user might have moved
                // it while we blocked updates during P0-P2.
                // If fakeSpeed is -1 at this point, it was disabled
                // when starting P0 (which is why we don't overwrite
                // fakeSpeed when enc is off, see above at P0)
                if(fakeSpeed < 0) {
                    // Reset enc to "disabled" position
                    re_init(false);
                } else {
                    // Reset enc to "zero" position
                    re_init();
                    re_lockTemp();
                }
                GPSabove88 = false;
            }
            #endif
            if(!dispGPSSpeed && !dispRotEnc && !bttfnRemoteSpeedMaster) {
                if(speedoAlwaysOn) {
                    dispIdleZero(true);
                } else {
                    speedo.off();
                }
            }
            #if defined(TC_HAVEGPS) || defined(TC_HAVE_RE)
            displayGPSorRESpeed(true);
            #endif
            #ifdef TC_HAVETEMP
            updateTemperature(true);
            dispTemperature(true);
            #endif
            #ifdef TC_HAVE_REMOTE
            updAndDispRemoteSpeed();
            #endif    
        } else {
            timetravelP0Now = millis();
            timeTravelP0Speed--;
            speedo.setSpeed(timeTravelP0Speed);
            speedo.show();
            #ifdef TC_HAVE_REMOTE
            // Overwrite for BTTFN clients, and in case Remote kicked in during TT
            // in which case the saved displayed speed would be outdated at the end of P2
            // and the Remote speed code would start count down from the outdated
            // speed again.
            bttfnRemCurSpd = timeTravelP0Speed; 
            #endif
            #ifdef TC_HAVE_RE
            if(useRotEnc) {
                // Need to keep -1, otherwise we can't detect if enc was off
                // ahead of the tt, see end of P2
                fakeSpeed = rotEnc.IsOff() ? -1 : timeTravelP0Speed;
            }
            #endif
            #if defined(TC_HAVEGPS) || defined(TC_HAVE_REMOTE)
            if(countToGPSSpeed) {
                if(targetSpeed == timeTravelP0Speed) {
                    timetravelP0Delay = 0;
                } else {
                    uint8_t tt = ((timeTravelP0Speed-targetSpeed)*100) / (88 - targetSpeed);
                    timetravelP0Delay = ((100 - tt) * 150) / 100;
                    if(timetravelP0Delay < 40) timetravelP0Delay = 40;
                }
            } else {
            #endif
                timetravelP0Delay = ((timeTravelP0Speed == 0) && !dispRotEnc) ? 4000 : 40;
            #if defined(TC_HAVEGPS) || defined(TC_HAVE_REMOTE)
            }
            #endif
        }
    }

    // Time travel animation, phase 1: Display disruption
    if((timeTravelP1 > 0) && (millis() - timetravelP1Now >= timetravelP1Delay)) {
        timeTravelP1++;
        timetravelP1Now = millis();
        switch(timeTravelP1) {
        case 2:
            #ifdef TC_DBG
            Serial.printf("TT initiated %d\n", millis());
            #endif
            ettoPulseStartNoLead();
            if(!skipTTAnim) {
                allOff();
            }
            timetravelP1Delay = TT_P1_DELAY_P2;
            break;
        case 3:
            timetravelP1Delay = TT_P1_DELAY_P3;
            break;
        case 4:
            timetravelP1Delay = TT_P1_DELAY_P4;
            break;
        case 5:
            timetravelP1Delay = TT_P1_DELAY_P5;
            break;
        default:
            timeTravelP1 = 0;
            destinationTime.setBrightness(255); // restore
            presentTime.setBrightness(255);
            departedTime.setBrightness(255);
            timeTravel(false, true);
        }
    }

    // Turn display back on after time traveling
    if(timeTravelRE && (millis() - timetravelNow >= TIMETRAVEL_DELAY)) {
        animate();
        timeTravelRE = false;
    }

    // TTOUT alarm signalling timeout
    if(ERROSigAlarm && (millis() - ETTOAlmNow >= ETTOAlmDur)) {
        setTTOUTpin(LOW);
        ERROSigAlarm = false;
    }       
    
    // Save data in time-slices to avoid long stalls
    postHSecChangeBusy = false;
    if(postHSecChange) {
        if(!startup && !timeTravelP0 && !timeTravelP1 && !timeTravelRE && !timeTravelP2) {
            unsigned long pscnow = millis();
            if(checkAudioDone() || mpActive) {    // Dont't interrupt sound effect, but ignore musicplayer
                if(!presentTime.saveClockStateData(lastYear)) {
                    if(!presentTime.saveFlush()) {
                        if(!departedTime.saveFlush()) {
                            if(destinationTime.saveFlush()) {
                                postHSecChangeBusy = true;
                            }
                        } else {
                            postHSecChangeBusy = true;
                        }
                    } else {
                        postHSecChangeBusy = true;
                        #ifdef TC_DBG
                        Serial.printf("Write presentTime to FS %d\n", millis());
                        #endif
                    }
                } else {
                    postHSecChangeBusy = true;
                    #ifdef TC_DBG
                    Serial.printf("Write lastYear to FS %d\n", millis());
                    #endif
                }
                if(!postHSecChangeBusy) {
                    if(volChanged && (pscnow - volChangedNow > 10*1000)) {
                        volChanged = false;
                        saveCurVolume();
                        postHSecChangeBusy = true;
                    }
                }
                // Slot was delayed, but is now taken
                postHSecChange = false;
            }
            if(postHSecChangeBusy) {
                unsigned long psctime = millis() - pscnow;
                unsigned long pscstime = millis() - pscsnow;
                if(psctime < 50 && pscstime < 200) {
                    postHSecChangeBusy = false;
                } else if(psctime > 100) {
                    audio_loop();
                }
            }
        } else {
            // Slot blocked by sequence, wait for next
            postHSecChange = false;
        }
        
    } else if(postSecChange) {

        postSecChange = false;
      
        // Alarm, count-down timer, Sound-on-the-Hour, Reminder

        #ifdef TC_HAVELIGHT
        bool switchNMoff = false;
        #endif
        int compHour = alarmRTC ? gdtl.hour()   : presentTime.getHour();
        int compMin  = alarmRTC ? gdtl.minute() : presentTime.getMinute();
        int weekDay =  alarmRTC ? bttfnDateBuf[7]
                                  : 
                                  dayOfWeek(presentTime.getDay(), 
                                            presentTime.getMonth(), 
                                            presentTime.getYear());

        bool alarmNow = (alarmOnOff && 
                         (alarmHour == compHour) && 
                         (alarmMinute == compMin) && 
                         (alarmWDmasks[alarmWeekday] & (1 << weekDay)));
        
        // Sound to play hourly (if available)
        // Follows setting for alarm as regards the option
        // of "Alarm base is real present time" vs whatever
        // is currently displayed in presentTime.

        if(compMin == 0) {
            if(presentTime.getNightMode() ||
               !FPBUnitIsOn ||
               startup      ||
               timeTravelP0 ||
               timeTravelP1 ||
               timeTravelRE ||
               alarmNow) {
                hourlySoundDone = true;
            }
            if(!hourlySoundDone) {
                play_hour_sound(compHour);
                hourlySoundDone = true;
            }
        } else {
            hourlySoundDone = false;
        }

        // Handle count-down timer
        if(ctDown) {
            if(millis() - ctDownNow > ctDown) {
                if( (!alarmNow) || (alarmDone && checkAudioDone()) ) {
                    play_file("/timer.mp3", PA_INTSPKR|PA_INTRMUS|PA_ALLOWSD);
                    ctDown = 0;
                }
            }
        }

        // Handle reminder
        if(remMonth > 0 || remDay > 0) {
            if( (!remMonth || remMonth == gdtl.month()) &&
                (remDay == gdtl.day())                  &&
                (remHour == gdtl.hour())                &&
                (remMin == gdtl.minute()) ) {
                if(!remDone) {
                    if( (!alarmNow) || (alarmDone && checkAudioDone()) ) {
                        play_file("/reminder.mp3", PA_INTSPKR|PA_INTRMUS|PA_ALLOWSD|PA_DYNVOL);
                        remDone = true;
                    }
                }
            } else {
                remDone = false;
            }
        }

        // Handle alarm
        if(alarmOnOff) {
            if(alarmNow) {
                if(!alarmDone) {
                    play_file("/alarm.mp3", PA_INTSPKR|PA_INTRMUS|PA_ALLOWSD|PA_DYNVOL);
                    alarmDone = true;
                    sendNetWorkMsg("ALARM\0", 6, BTTFN_NOT_ALARM);
                    if(ETTOalarm) {
                        setTTOUTpin(HIGH);
                        ERROSigAlarm = true;
                        ETTOAlmNow = millis();
                    }
                }
            } else {
                alarmDone = false;
            }
        }

        // Handle Auto-NightMode

        // Manually switching NM pauses automatic for 30 mins
        if(manualNightMode >= 0 && (millis() - manualNMNow > 30*60*1000)) {
            manualNightMode = -1;
            // Re-evaluate auto-nm immediately
            forceReEvalANM = true;
        }
        
        if(autoNightMode && (manualNightMode < 0)) {
            if(gdtl.minute() == 0 || forceReEvalANM) {
                if(!autoNMDone || forceReEvalANM) {
                    uint32_t myField;
                    if(autoNightModeMode == 0) {
                        myField = autoNMdailyPreset;
                    } else {
                        const uint32_t *myFieldPtr = autoNMPresets[autoNightModeMode - 1];
                        myField = myFieldPtr[weekDay];
                    }                       
                    if(myField & (1 << (23 - gdtl.hour()))) {
                        nightModeOn();
                        timedNightMode = 1;
                    } else {
                        #ifdef TC_HAVELIGHT
                        switchNMoff = true;
                        #else
                        nightModeOff();
                        #endif
                        timedNightMode = 0;
                    }
                    autoNMDone = true;
                }
            } else {
                autoNMDone = false;
            }
            forceReEvalANM = false;
        }
        #ifdef TC_HAVELIGHT
        // Light sensor overrules scheduled NM only in non-NightMode periods
        if(useLight && (manualNightMode < 0) && (timedNightMode < 1)) {
            int32_t myLux = lightSens.readLux();
            if(myLux >= 0) {
                if(myLux > luxLimit) {
                    sensorNightMode = 0;
                    switchNMoff = true;
                } else {
                    sensorNightMode = 1;
                    nightModeOn();
                }
            } else {
                // Bad lux (probably by sensor overload), switch NM off
                sensorNightMode = -1;
                switchNMoff = true;
            }
        }
        if(switchNMoff) {
            if(sensorNightMode < 1) nightModeOff();
            switchNMoff = false;
        }
        #endif

    }

    y = digitalRead(SECONDS_IN_PIN);
    if((y == x) && !postHSecChangeBusy) {

        bool didUpdSpeedo = false;

        // Less timing critical stuff goes here:
        // (Will be skipped in current iteration if
        // seconds change is detected)

        // Handle Remote
        #ifdef TC_HAVE_REMOTE
        /*
        if(bttfnRemoteSpeedMaster != bttfnOldRemoteSpeedMaster) {
            if((bttfnOldRemoteSpeedMaster = bttfnRemoteSpeedMaster)) {
                // Remote became speed master:
                // useRotEnc, dispRotEnc, dispGPSSpeed, dispTemp already "false" at this point
                // Speedo will be set up in updAndDispRemoteSpeed().
                // Anything else here?
            } else {
                // Remote is no longer speed master
                // useRotEnc, dispRotEnc, dispGPSSpeed, dispTemp already re-set at this point
                // rotEnc was reset; speed or temp was displayed
                // Anything else here?
            }
        }
        */
        if(bttfnRemoteSpeedMaster) {
            // Done in every iteration
            updAndDispRemoteSpeed();
            if(bttfnRemCurSpd != bttfnRemCurSpdOld) {
                if(FPBUnitIsOn && !timeTravelP0 && !timeTravelP1 && !timeTravelRE) {
                    startBeepTimer();
                }
                bttfnRemCurSpdOld = bttfnRemCurSpd;
            }
            if(bttfnRemCurSpd >= 88) {
                checkForSpeedTT(false, true);
            } else if(bttfnRemCurSpd < 10) {
                GPSabove88 = false;
            }
        }
        #endif

        // Handle RotEnc
        #ifdef TC_HAVE_RE
        if(FPBUnitIsOn && useRotEnc && !startup && !timeTravelP0 && !timeTravelP1 && !timeTravelP2) {
            fakeSpeed = rotEnc.updateFakeSpeed();
            if(fakeSpeed != oldFSpd) {
                if((fakeSpeed >= 0) && !timeTravelRE) {
                    startBeepTimer();
                }
                oldFSpd = fakeSpeed;
            }
            displayGPSorRESpeed();
            if(fakeSpeed >= 88) {
                checkForSpeedTT(dispRotEnc, false);
            } else if(!fakeSpeed) {   // Is reset in P2, but if during TT Remote
                GPSabove88 = false;   // took over, P2 does not reset this.
            }
        }
        if(useRotEncVol) {
            int oldVol = curVolume;
            curVolume = rotEncVol->updateVolume(curVolume, false);
            if(oldVol != curVolume) {
                volChanged = true;
                volChangedNow = millis();
            }
        }
        #endif

        millisNow = millis();
        
        // Read GPS, and display GPS speed
        #ifdef TC_HAVEGPS
        if(useGPS) {
            if(millis64() >= lastLoopGPS) {
                lastLoopGPS += (uint64_t)GPSupdateFreq;
                // call loop with doDelay true; delay not needed but
                // this causes a call of audio_loop() which is good
                myGPS.loop(true);

                // Auto-TT on GPS-88mph only if RotEnc/Remote are not source of displayed speed
                if(!dispRotEnc && !bttfnRemoteSpeedMaster) {
                    displayGPSorRESpeed(true);
                    #ifdef NOT_MY_RESPONSIBILITY
                    if(myGPS.getSpeed() >= 88) {
                        checkForSpeedTT(false, false);
                    } else if(myGPS.getSpeed() < 10) {
                        GPSabove88 = false;
                    }
                    #endif
                }
            #ifdef TC_HAVE_REMOTE
            } else if(remoteWasMaster) {
                displayGPSorRESpeed(true);
            #endif
            }
            
        }
        #endif
        
        // Power management: CPU speed
        // Can only reduce when GPS is not used, WiFi is off and no sound playing
        if(!pwrLow &&
           (wifiIsOff || wifiAPIsOff) && 
           #ifdef TC_HAVEGPS
           !useGPS &&
           #endif
           checkAudioDone() && 
           (millisNow - pwrFullNow >= 5*60*1000)) {
            
            setCpuFrequencyMhz(80);
            pwrLow = true;
            #ifdef TC_DBG
            Serial.printf("Reduced CPU speed to %d\n", getCpuFrequencyMhz());
            #endif
        }

        // Beep auto modes
        if(beepTimer && (millisNow - beepTimerNow > beepTimeout)) {
            muteBeep = true;
            beepTimer = false;
        }

        // Update sensors

        #ifdef TC_HAVETEMP
        updateTemperature();
        didUpdSpeedo = dispTemperature();
        #endif

        // speedoAlwaysOn can only be true if useSpeedoDisplay is true
        if(speedoAlwaysOn && !didUpdSpeedo && !dispGPSSpeed && !dispRotEnc && !bttfnRemoteSpeedMaster) {
            dispIdleZero();
        }
        
        #ifdef TC_HAVELIGHT
        if(useLight && (millisNow - lastLoopLight >= 3000)) {
            lastLoopLight = millisNow;
            lightSens.loop();
        }
        #endif

        // End of OTPR for PCF2129
        #ifdef HAVE_PCF2129
        if(OTPRinProgress) {
            if(millisNow - OTPRStarted > 100) {
                rtc.OTPRefresh(false);
                OTPRinProgress = false;
                OTPRDoneNow = millisNow;
            }
        }
        #endif
    }

    bttfn_notify_of_speed();

    y = digitalRead(SECONDS_IN_PIN);
    if(y != x) {

        // Actual clock stuff
      
        if(y == 0) {

            int8_t minNext;

            // Set colon
            destinationTime.setColon(true);
            presentTime.setColon(true);
            departedTime.setColon(true);

            // Play "annoying beep"(tm)
            play_beep();

            // Prepare for time re-adjustment through NTP/GPS

            if(!haveAuthTime) {
                authTimeExpired = true;
            } else if(millis() - lastAuthTime >= 7*24*60*60*1000) {
                authTimeExpired = true;
            }

            #ifdef TC_HAVEGPS
            bool GPShasTime = gpsHaveTime();
            #else
            bool GPShasTime = false;
            #endif

            // Read RTC for UTC time
            myrtcnow(gdtu);

            // Re-adjust time periodically through NTP/GPS
            //
            // This is normally done hourly between hour:01 and hour:02. However:
            // - Exception to time rule: If there is no authTime yet, try syncing 
            //   every resyncInt+1/2 minutes or whenever GPS has time; or if
            //   syncTrigger is set (keypad "7"). (itstime)
            // - Try NTP via WiFi (doWiFi) when/if user configured a network AND
            //   -- WiFi is connected (duh!), or
            //   -- WiFi was connected and is now in power-save, but no authTime yet;
            //      this triggers a re-connect (resulting in a frozen display).
            //      This annoyance is kept to a minimum by setting the WiFi off-timer
            //      to a period longer than the period between two attempts; this in
            //      essence defeats WiFi power-save, but accurate time is more important.
            //   -- or if
            //          --- WiFi was connected and now off OR currently in AP-mode AND
            //          --- authTime has expired (after 7 days) AND
            //          --- it is night (0-6am)
            //      This also triggers a re-connect (frozen display); in this case only
            //      once every 15/90 minutes (see wifiOn) during night time until a time
            //      sync succeeds or the number of attempts exceeds a certain amount.
            // - In any case, do not interrupt any running sequences.

            bool itsTime = false;
            bool doWiFi = false;
            
            if(couldHaveAuthTime && 
               !startup && !timeTravelP0 && !timeTravelP1 && !timeTravelRE && !timeTravelP2) {
            
                itsTime = ( (gdtu.minute() == 1) ||
                            (gdtu.minute() == 2) ||
                            (!haveAuthTime && 
                             ( ((gdtu.minute() % resyncInt) == 1) || 
                               ((gdtu.minute() % resyncInt) == 2) ||
                               GPShasTime 
                             )
                            )                    ||
                            (syncTrigger &&
                             gdtu.second() == 35
                            )
                          );
            
                if(itsTime && !GPShasTime) {
                    doWiFi = wifiHaveSTAConf &&                        // if WiFi network is configured AND
                             ( (!wifiIsOff && !wifiInAPMode)     ||    //   if WiFi-STA is on,                           OR
                               ( wifiIsOff      &&                     //   if WiFi-STA is off (after being connected), 
                                 !haveAuthTime  &&                     //      but no authtime, 
                                 keypadIsIdle() &&                     //      and keypad is idle (no press for >2mins)
                                 checkMP3Done())                 ||    //      and no mp3 is being played                OR
                               ( (wifiIsOff ||                         //   if WiFi-STA is off (after being connected),
                                  (wifiInAPMode && doAPretry)) &&      //                      or in AP-mode
                                 authTimeExpired               &&      //      and authtime expired
                                 checkMP3Done()                &&      //      and no mp3 being played
                                 keypadIsIdle()                &&      //      and keypad is idle (no press for >2mins)
                                 (lastHour <= 6)                       //      during night-time
                               )
                             );
                }
            }
            
            #ifdef TC_DBG
            if(gdtu.second() == 35) {
                Serial.printf("%s%d %d %d %d %d\n", funcName, couldHaveAuthTime, itsTime, doWiFi, haveAuthTime, syncTrigger);
            }
            #endif
            
            if(itsTime && (GPShasTime || doWiFi)) {

                if(!autoReadjust) {

                    uint64_t oldT;

                    if(timeDifference) {
                        oldT = dateToMins(gdtu.year(), gdtu.month(), gdtu.day(), gdtu.hour(), gdtu.minute());
                    }

                    // Note that the following call will fail if getNTPtime reconnects
                    // WiFi; no current NTP time stamp will be available. Repeated calls
                    // will eventually succeed.

                    if(getNTPOrGPSTime(haveAuthTime, gdtu, false)) {

                        bool updateTimeDiff = false;

                        autoReadjust = true;
                        resyncInt = 5;
                        syncTrigger = false;

                        haveAuthTime = true;
                        lastAuthTime = millis();
                        lastAuthTime64 = millis64();
                        authTimeExpired = false;

                        // We have just adjusted, don't do it again below
                        lastYear = gdtu.year();

                        #ifdef TC_DBG
                        Serial.printf("%sRTC re-adjusted using NTP or GPS\n", funcName);
                        #endif

                        // Correct timeDifference by adjustment delta
                        if(timeDifference) {
                            uint64_t newT = dateToMins(gdtu.year(), gdtu.month(), gdtu.day(), gdtu.hour(), gdtu.minute());
                            if(newT != oldT) {
                                if(timeDiffUp) {
                                    oldT += timeDifference;
                                } else {
                                    oldT -= timeDifference;
                                }
                                if(oldT > newT) {
                                    timeDifference = oldT - newT;
                                    timeDiffUp = true;
                                } else {
                                    timeDifference = newT - oldT;
                                    timeDiffUp = false;
                                }
                                if(timetravelPersistent) {
                                    presentTime.savePending();
                                }
                            }
                        }

                    } else {

                        #ifdef TC_DBG
                        Serial.printf("%sRTC re-adjustment via NTP/GPS failed\n", funcName);
                        #endif

                    }

                } 

            } else {
                
                autoReadjust = false;

                // If GPS is used for time, resyncInt is possibly set to 2 during boot;
                // reset it to 5 after 5 minutes since boot.
                if(resyncInt != 5) {
                    if(millis() > 5*60*1000) {
                        resyncInt = 5;
                    }
                }

            }

            // Detect and handle year changes

            {
                uint16_t thisYear = gdtu.year();

                if( (thisYear != lastYear) || (thisYear > 9999) ) {

                    uint16_t rtcYear;
                    int16_t yOffs = 0;
    
                    if(thisYear > 9999) {
                        #ifdef TC_DBG
                        Serial.printf("%sRollover 9999->1 detected\n", funcName);
                        #endif

                        // We do not roll over to year "0".
                        // TZ calc doesn't cope with y0.
                        thisYear = 1;
    
                        if(timeDifference) {
                            // Correct timeDifference [r-o to 0: 5259492000]
                            if(timeDifference >= tdro) {
                                timeDifference -= tdro;
                            } else {
                                timeDifference = tdro - timeDifference;
                                timeDiffUp = !timeDiffUp;
                            }
                            if(timetravelPersistent) {
                                presentTime.savePending();
                            }
                        }
                    }
    
                    // Get RTC-fit year & offs for real year
                    rtcYear = thisYear;
                    correctYr4RTC(rtcYear, yOffs);
    
                    // If year-translation changed, update RTC and save
                    if((rtcYear != gdtu.hwRTCYear) || (yOffs != presentTime.getYearOffset())) {
    
                        // Prepare to update RTC
                        rtc.prepareAdjust(gdtu.second(), 
                                          gdtu.minute(), 
                                          gdtu.hour(), 
                                          dayOfWeek(gdtu.day(), gdtu.month(), thisYear),
                                          gdtu.day(), 
                                          gdtu.month(), 
                                          rtcYear-2000
                        );
    
                        presentTime.setYearOffset(yOffs);

                        gdtu.hwRTCYear = rtcYear;
                        gdtu.setYear(thisYear);
    
                        // YearOffs saved to NVM in next loop iteration
                    }
                }
            }

            // Write changes prepared above to RTC
            rtc.finishAdjust();

            // Update "lastYear" (UTC) (saved to NVM in next loop iteration)
            lastYear = gdtu.year();

            // Convert UTC to local (parses TZ in the process)
            UTCtoLocal(gdtu, gdtl, 0);
            lastHour = gdtl.hour();

            // Write time to presentTime display
            if(stalePresent)
                presentTime.setFromStruct(&stalePresentTime[1]);
            else
                updatePresentTime(gdtu, gdtl);
            
            // Handle WC mode (load dates/times for dest/dep display)
            // (Restoring not needed, done elsewhere)
            if(isWcMode()) {
                if((gdtu.minute() != wcLastMin) || triggerWC) {
                    wcLastMin = gdtu.minute();
                    setDatesTimesWC(gdtu);
                }
                // Put city/loc name on for 3 seconds
                if(gdtu.second() % 10 == 3) {
                    if(haveTZName1) destShowAlt = 3*2;
                    if(haveTZName2) depShowAlt = 3*2;
                }
            }
            triggerWC = false;

            bttfnDateBuf[0] = (uint8_t)gdtl.year() & 0xff;
            bttfnDateBuf[1] = (uint8_t)gdtl.year() >> 8; 
            bttfnDateBuf[2] = (uint8_t)gdtl.month();
            bttfnDateBuf[3] = (uint8_t)gdtl.day();
            bttfnDateBuf[4] = (uint8_t)gdtl.hour();
            bttfnDateBuf[5] = (uint8_t)gdtl.minute();
            bttfnDateBuf[6] = (uint8_t)gdtl.second();
            bttfnDateBuf[7] = (uint8_t)dayOfWeek(gdtl.day(), gdtl.month(), gdtl.year());

            // Debug log beacon
            #ifdef TC_DBG
            if((gdtu.second() == 0) && (gdtu.minute() != dbgLastMin)) {
                dbgLastMin = gdtu.minute();
                Serial.printf("UTC: %d[%d-(%d)]/%02d/%02d %02d:%02d:00 (Chip Temp %.2f) / WD of PT: %d (%d)\n",
                      lastYear, gdtu.hwRTCYear, presentTime.getYearOffset(), gdtu.month(), gdtu.day(), gdtu.hour(), dbgLastMin,
                      rtc.getTemperature(),
                      dayOfWeek(presentTime.getDay(), presentTime.getMonth(), presentTime.getYear()),
                      bttfnDateBuf[7]);
                Serial.printf("    bttfnNotAllSupportMC %d   bttfnAtLeastOneMC %d\n",
                      bttfnNotAllSupportMC, bttfnAtLeastOneMC);
                Serial.printf("Heap %d\n", ESP.getFreeHeap());
            }
            #endif

            // Handle Time Cycling

            // If we animate, do this on previous minute:59
            // We use the displayed minute, not the local time's minute
            // unless we're in Exhibition Mode
            if(stalePresent)
                minNext = gdtl.minute();
            else
                minNext = presentTime.getMinute();

            if(autoRotAnim) {
                minNext++;
                if(minNext > 59) minNext = 0;
            }

            // Check if autoPause has run out
            if(autoPaused && (millis() - pauseNow >= pauseDelay)) {
                autoPaused = false;
            }

            // Only do this on second 59, check if it's time to do so
            if((gdtl.second() == autoIntSec)                            &&
               (!autoPaused)                                            &&
               autoTimeIntervals[autoInterval]                          &&
               (minNext % autoTimeIntervals[autoInterval] == 0)         &&
               #ifdef TC_HAVETEMP                             // Skip in rcMode if (temp&hum available || wcMode)
               ( !(isRcMode() && (isWcMode() || tempSens.haveHum())) )  &&   
               #endif
               (!isWcMode() || !WcHaveTZ1 || !WcHaveTZ2) ) {  // Skip in wcMode if both TZs configured

                if(!autoIntDone) {

                    autoIntDone = true;

                    // cycle through pre-programmed times
                    autoTime++;
                    if(autoTime >= NUM_AUTOTIMES) autoTime = 0;

                    // Show a preset dest and departed time
                    // RcMode irrelevant; continue cycle in the background
                    if(!isWcMode() || !WcHaveTZ1) {
                        destinationTime.setFromStruct(&destinationTimes[autoTime]);
                    }
                    if(!isWcMode() || !WcHaveTZ2) {
                        departedTime.setFromStruct(&departedTimes[autoTime]);
                    }

                    if(autoRotAnim) {
                        allOff();
                        autoIntAnimRunning = 1;
                    }
                
                } 

            } else {

                autoIntDone = false;
                if(autoIntAnimRunning)
                    autoIntAnimRunning++;

            }

            postSecChange = true;

        } else {

            millisNow = millis();

            destinationTime.setColon(false);
            presentTime.setColon(false);
            departedTime.setColon(false);

            // OTPR for PCF2129 every two weeks
            #ifdef HAVE_PCF2129
            if(RTCNeedsOTPR && (millisNow - OTPRDoneNow > 2*7*24*60*60*1000)) {
                rtc.OTPRefresh(true);
                OTPRinProgress = true;
                OTPRStarted = millisNow;
            }
            #endif

            if(autoIntAnimRunning)
                autoIntAnimRunning++;

            #if defined(TC_DBG) && defined(TC_DBGBEEP)
            Serial.printf("Beepmode %d BeepTimer %d, BTNow %d, now %d mute %d\n", beepMode, beepTimer, beepTimerNow, millis(), muteBeep);
            #endif

            postHSecChange = true;
            pscsnow = millis();
        }

        x = y;

        if(!skipTTAnim && timeTravelP1 > 1) {

            int ii = 5, tt;
            switch(timeTravelP1) {
            case 2:
                ((rand() % 10) > 7) ? presentTime.off() : presentTime.on();
                (((rand()+millis()) % 10) > 7) ? destinationTime.off() : destinationTime.on();
                ((rand() % 10) > 7) ? departedTime.off() : departedTime.on();
                break;
            case 3:
                presentTime.off();
                destinationTime.off();
                departedTime.off();
                break;
            case 4:
                destinationTime.show();
                presentTime.show();
                departedTime.show();
                destinationTime.on();
                presentTime.on();
                departedTime.on();
                while(ii--) {
                    tt = rand() % 21;
                    if(tt < 5) destinationTime.show();
                    else {
                        destinationTime.showTextDirect(p1errStrs[(tt - 5) >> 2], CDT_COLON);
                    }
                    if(!(ii % 2)) destinationTime.setBrightnessDirect((1+(rand() % 10)) & 0x0a);
                    if(ii % 2) presentTime.setBrightnessDirect((1+(rand() % 10)) & 0x0b);
                    ((rand() % 10) < 3) ? departedTime.showTextDirect(">ACS2011GIDUW") : departedTime.show();
                    if(ii % 2) departedTime.setBrightnessDirect((1+(rand() % 10)) & 0x07);
                    mydelay(20);
                }
                break;
            case 5:
                departedTime.setBrightness(255);
                departedTime.on();
                while(ii--) {
                    tt = rand() % 10;
                    if(!(ii % 4))   presentTime.setBrightnessDirect(1+(rand() % 8));
                    if(tt < 3)      { presentTime.setBrightnessDirect(4); presentTime.lampTest(true); }
                    else if(tt < 7) { presentTime.show(); presentTime.on(); }
                    else            { presentTime.off(); }
                    tt = (rand() + millis()) % 10;
                    if(tt < 2)      { destinationTime.showTextDirect(p1errStrs[rand() % 4], CDT_COLON); }
                    else if(tt < 6) { destinationTime.show(); destinationTime.on(); }
                    else            { if(!(ii % 2)) destinationTime.setBrightnessDirect(1+(rand() % 8)); }
                    tt = (tt + (rand() + millis())) % 10;
                    if(tt < 4)      { departedTime.setBrightnessDirect(4); departedTime.lampTest(true); }
                    else if(tt < 7) { departedTime.showTextDirect("R 2 0 1 1 T R "); }
                    else            { departedTime.show(); }
                    mydelay(10);
                    
                    #if 0 // Code of death for some red displays, seems they don't like realLampTest
                    tt = rand() % 10; 
                    presentTime.setBrightnessDirect(1+(rand() % 8));
                    if(tt < 3)      { presentTime.realLampTest(); }
                    else if(tt < 7) { presentTime.show(); presentTime.on(); }
                    else            { presentTime.off(); }
                    tt = (rand() + millis()) % 10;
                    if(tt < 2)      { destinationTime.realLampTest(); }
                    else if(tt < 6) { destinationTime.show(); destinationTime.on(); }
                    else            { destinationTime.setBrightnessDirect(1+(rand() % 8)); }  
                    tt = (rand() + millis()) % 10; 
                    if(tt < 4)      { departedTime.realLampTest(); }
                    else if(tt < 8) { departedTime.showTextDirect("00000000000000"); departedTime.on(); }
                    else            { departedTime.off(); }
                    mydelay(10);
                    #endif
                }
                break;
            default:
                allOff();
            }
            
        } else if(autoIntAnimRunning) {

            if(autoIntAnimRunning >= 3) {
                if(FPBUnitIsOn) animate();
                autoIntAnimRunning = 0;
            }
                
        } else if(!startup && !timeTravelRE && FPBUnitIsOn) {

            #ifdef TC_HAVEMQTT
            if(mqttDisp) {
                if(!specDisp) {
                    destinationTime.showTextDirect(mqttMsg + mqttIdx);
                    if(mqttST) {
                        if(!presentTime.getNightMode()) {
                            play_file(mqttAudioFile, PA_INTSPKR|PA_CHECKNM|PA_ALLOWSD);
                        }
                        mqttST = false;
                    }
                    if(mqttOldDisp != mqttDisp) {
                        mqttStartNow = millis();
                        mqttOldDisp = mqttDisp;
                    }
                    if(mqttMaxIdx < 0) {
                        if(millis() - mqttStartNow > 5000) {
                            mqttDisp = mqttOldDisp = 0;
                        }
                    } else {
                        if(millis() - mqttStartNow > 200) { // useless actually, we are in .5 second steps here
                            mqttStartNow = millis();
                            mqttIdx++;
                            if(mqttIdx > mqttMaxIdx) {
                                mqttDisp = mqttOldDisp = 0;
                            }
                        }
                    }
                } else {
                    mqttOldDisp = mqttIdx = 0;
                }
            }
            #endif

            #ifdef TC_HAVETEMP
            if(isRcMode()) {
                
                if(!specDisp && !mqttDisp) {
                    if(!isWcMode() || !WcHaveTZ1) {
                        destinationTime.showTempDirect(tempSens.readLastTemp(), tempUnit);
                    } else {
                        destShowAlt ? destinationTime.showAlt() : destinationTime.show();
                    }
                }
                if(isWcMode() && WcHaveTZ1) {
                    departedTime.showTempDirect(tempSens.readLastTemp(), tempUnit);
                } else if(!isWcMode() && tempSens.haveHum()) {
                    departedTime.showHumDirect(tempSens.readHum());
                } else {
                    depShowAlt ? departedTime.showAlt() : departedTime.show();
                }

            } else {
            #endif
                if(!specDisp && !mqttDisp) {
                    destShowAlt ? destinationTime.showAlt() : destinationTime.show();
                }
                depShowAlt ? departedTime.showAlt() : departedTime.show();
            #ifdef TC_HAVETEMP
            }
            #endif

            presentTime.show();

        }

        if(destShowAlt) destShowAlt--;
        if(depShowAlt) depShowAlt--;
    } 
}


/* Time Travel:
 *
 *  A time travel consists of various phases:
 *  - Speedo acceleration (if applicable; if GPS is in use, starts at actual speed)
 *  - actual tt (temporal displacement = display disruption)
 *  - re-entry
 *  - Speed deceleration (if applicable; if GPS is in use, counts down to actual speed)
 *  - ETTO lead time period (trigger external props, if applicable)
 *
 *  |<--------- P0: speedo acceleration ------>|                         |<P2:speedo de-accleration>|
 *  0....10....20....................xx....87..88------------------------88...87....................0
 *                                   |<-------- travelstart.mp3 -------->|<-tt.mp3>|
 *                                   11111111111111111111111111111111111122222222222
 *                                   |         |<--Actual Time Travel -->|
 *                           P1 starts         |  (Display disruption)   |
 *                                        TT starts                      Reentry phase
 *                                             |                         |
 *               |<---------ETTO lead--------->|                         |
 *               |    (depending on config)    |                         |
 *          ETTO: Pulse                  TT_P1_POINT88                   |
 *               or                    (ms from P1 start)                |
 *           LOW->HIGH                                            ETTO: HIGH->LOW
 *       [MQTT: "TIMETRAVEL']                                    [MQTT: "REENTRY']                          
 *
 *  As regards the mere "time" logic, a time travel means to
 *  -) copy present time into departed time (where it freezes)
 *  -) copy destination time to present time (where it continues to run as a clock)
 *
 */

int timeTravelProbe(bool doComplete, bool withSpeedo, bool forceNoLead)
{
    if(!useSpeedo) {
        withSpeedo = false;
    } else {
        // If no speedo display and no remote -> withSpeedo = off
        if(!useSpeedoDisplay 
                             #ifdef TC_HAVE_REMOTE
                             && (!bttfnHaveRemote || (!bttfnRemoteSpeedMaster && !bttfnRemOffSpd))
                             #endif
                                                                                                  ) {
            withSpeedo = false;
        }
    }

    // Don't start if Remote is Master and brake is on
    #ifdef TC_HAVE_REMOTE
    if(doComplete && withSpeedo && bttfnRemoteSpeedMaster) {
        if(bttfnRemStop) {
            return 1;
        }
    }
    #endif

    return 0;
}

int timeTravel(bool doComplete, bool withSpeedo, bool forceNoLead)
{
    int   tyr = 0;
    int   tyroffs = 0;
    long  currTotDur = 0;
    unsigned long ttUnivNow = millis();

    if(!useSpeedo) {
        withSpeedo = false;
    } else {
        // If no speedo display and no remote -> withSpeedo = off
        if(!useSpeedoDisplay 
                             #ifdef TC_HAVE_REMOTE
                             && (!bttfnHaveRemote || (!bttfnRemoteSpeedMaster && !bttfnRemOffSpd))
                             #endif
                                                                                                  ) {
            withSpeedo = false;
        }
    }

    // Don't start if Remote is Master and brake is on
    #ifdef TC_HAVE_REMOTE
    if(doComplete && withSpeedo && bttfnRemoteSpeedMaster) {
        if(bttfnRemStop) {
            return 1;
        }
    }
    #endif

    pwrNeedFullNow();

    // Disable RC and WC modes
    // Note that in WC mode, the times in red & yellow
    // displays work like "normal" times there: The user
    // travels to whatever time is currently in the red
    // display, and his current time becomes last time
    // departed - and both become stale.
    enableRcMode(false);
    enableWcMode(false);

    cancelEnterAnim();
    cancelETTAnim();

    // Stop music if we are to play time travel sounds
    if(playTTsounds) mp_stop();
    
    // Beep auto mode: Restart timer
    startBeepTimer();

    // Pause autoInterval-cycling so user can play undisturbed
    pauseAuto();

    ttUnivNow = millis();

    /*
     * Complete sequence with speedo: Initiate P0 (speed count-up)
     *
     */
    if(doComplete && withSpeedo) {

        bool doPreTTSound = havePreTTSound;
        bool doLeadLessP1 = doPreTTSound;
        long myPointOfP1  = doPreTTSound ? pointOfP1NoLead : pointOfP1;

        #ifdef TC_HAVE_REMOTE
        remoteInducedTT = false;
        #endif

        timeTravelP0Speed = 0;
        timetravelP0Delay = havePreTTSound ? 500 : 2000;   // Delay of 2000 too long if we play ttaccel
        triggerP1 = false;
        triggerETTO = false;
        ettoPulseEnd();
        ettoLeadPoint = origEttoLeadPoint;
        bttfnTTLeadTime = ettoLeadTime;
        
        #if defined(TC_HAVEGPS) || defined(TC_HAVE_RE) || defined(TC_HAVE_REMOTE)
        if(dispGPSSpeed || dispRotEnc || bttfnRemoteSpeedMaster) {
            #if defined(TC_HAVEGPS) && defined(TC_HAVE_RE)
            int16_t tempSpeed = dispGPSSpeed ? myGPS.getSpeed() : fakeSpeed;
            #elif defined(TC_HAVEGPS)
            int16_t tempSpeed = myGPS.getSpeed();
            #elif defined(TC_HAVE_RE)
            int16_t tempSpeed = fakeSpeed;
            #else
            int16_t tempSpeed = 0;
            #endif
            #ifdef TC_HAVE_REMOTE
            if(bttfnRemoteSpeedMaster) {
                tempSpeed = bttfnRemCurSpd;
            }
            #endif
            if(tempSpeed >= 0) {
                timeTravelP0Speed = tempSpeed;
                timetravelP0Delay = 0;
                if(timeTravelP0Speed < 88) {
                    currTotDur = tt_p0_totDelays[timeTravelP0Speed];

                    // If the strict 5s lead is not required (as is the case
                    // if ETTWithFixedLead is false), we can also cut short 
                    // P1 by skipping the fade-in part of the sound, if the
                    // the remaining part of the acceleration until 88 is too
                    // short.

                    if(!ETTWithFixedLead) {
                        if(currTotDur > pointOfP1) {
                            // If we're past normal P1 start point,
                            // cut P1 short by skipping lead
                            // (and skip user's acceleration sound)
                            myPointOfP1 = pointOfP1NoLead;
                            doLeadLessP1 = true;
                            doPreTTSound = false;
                        } else if(doPreTTSound) {
                            // Don't play user's acceleration sound if it would
                            // be played for only a very short time (~2 secs)
                            // But play with-lead version in that case.
                            if(currTotDur > (pointOfP1 - 600)) {
                                myPointOfP1 = pointOfP1;
                                doLeadLessP1 = false;
                                doPreTTSound = false;
                            } else {
                                myPointOfP1 = pointOfP1NoLead;
                                doLeadLessP1 = true;
                            }
                        }
                    }
                    
                    // If the strict 5s lead is not required (as is the
                    // case if ETTWithFixedLead is false), we can shorten that 
                    // lead. We calculate it based on current speed and
                    // the remaining time until reaching 88.
                    
                    if(!ETTWithFixedLead && (bttfnHaveClients || pubMQTTVL)) {
                        if(currTotDur >= ettoLeadPoint) {
                            // Calc time until 88 for bttfn clients
                            if(ettoLeadPoint < myPointOfP1) {
                                if(currTotDur >= myPointOfP1) {
                                    ettoLeadPoint = myPointOfP1;
                                    bttfnTTLeadTime = ettoBase - myPointOfP1;
                                } else {
                                    ettoLeadPoint = currTotDur;
                                    bttfnTTLeadTime = ettoBase - currTotDur;
                                }
                                if(bttfnTTLeadTime > ETTO_LAT) {
                                    bttfnTTLeadTime -= ETTO_LAT;
                                } else if(bttfnTTLeadTime < 0) {
                                    bttfnTTLeadTime = 0; // should not happen
                                }
                            }
                        }
                    }
                }
            }
        }
        #endif
        
        if(timeTravelP0Speed < 88) {

            // Now calculate the times until P1 and until reaching 88.
            // If time needed to reach 88mph is shorter than ettoLeadTime
            // or pointOfP1: Need add'l delay before speed counter kicks in.
            // This add'l delay is put into timetravelP0Delay.

            if(ETTWithFixedLead || bttfnHaveClients || pubMQTTVL) {

                triggerETTO = true;
                
                if(currTotDur >= ettoLeadPoint || currTotDur >= myPointOfP1) {

                    if(currTotDur >= ettoLeadPoint && currTotDur >= myPointOfP1) {

                        // Both outside our remaining time period:
                        if(ettoLeadPoint <= myPointOfP1) {
                            // ETTO first:
                            triggerETTOLeadTime = 0;
                            triggerP1LeadTime = myPointOfP1 - ettoLeadPoint;
                            timetravelP0Delay = currTotDur - ettoLeadPoint;
                        } else if(ettoLeadPoint > myPointOfP1) {
                            // P1 first:
                            triggerP1LeadTime = 0;
                            triggerETTOLeadTime = ettoLeadPoint - myPointOfP1;
                            timetravelP0Delay = currTotDur - myPointOfP1;
                        }

                    } else if(currTotDur >= ettoLeadPoint) {

                        // etto outside
                        triggerETTOLeadTime = 0;
                        triggerP1LeadTime = myPointOfP1 - ettoLeadPoint;
                        timetravelP0Delay = currTotDur - ettoLeadPoint;

                    } else {

                        // P1 outside
                        triggerP1LeadTime = 0;
                        triggerETTOLeadTime = ettoLeadPoint - myPointOfP1;
                        timetravelP0Delay = currTotDur - myPointOfP1;

                    }

                } else {

                    triggerP1LeadTime = myPointOfP1 - currTotDur + timetravelP0Delay;
                    triggerETTOLeadTime = ettoLeadPoint - currTotDur + timetravelP0Delay;

                }

                // If there is time between NOW and ETTO_LEAD start, send
                // PREPARE message to networked clients.
                if(triggerETTOLeadTime > 500) {
                    sendNetWorkMsg("PREPARE\0", 8, BTTFN_NOT_PREPARE);
                }
                
                ttUnivNow = millis();
                triggerETTONow = triggerP1Now = ttUnivNow;

            } else if(currTotDur >= myPointOfP1) {
                 triggerP1Now = ttUnivNow;
                 triggerP1LeadTime = 0;
                 timetravelP0Delay = currTotDur - myPointOfP1;
            } else {
                 triggerP1Now = ttUnivNow;
                 triggerP1LeadTime = myPointOfP1 - currTotDur + timetravelP0Delay;
            }

            triggerP1 = true;
            triggerP1NoLead = doLeadLessP1;

            speedo.setSpeed(timeTravelP0Speed);
            speedo.setBrightness(255);
            speedo.show();
            speedo.on();
            timetravelP0Now = ttP0Now = ttUnivNow;
            timeTravelP0 = 1;
            timeTravelP2 = 0;

            timeTravelP0stalled = (timetravelP0Delay > 0) ? 1 : 0;

            #ifdef TC_HAVE_REMOTE
            // Set for polling BTTFN clients
            bttfnRemCurSpd = timeTravelP0Speed;
            #endif

            // Now transmit p0 start-speed
            bttfn_notify_of_speed();

            if(doPreTTSound) {
                play_file(preTTSound, PA_LINEOUT|PA_CHECKNM|PA_INTRMUS|PA_ALLOWSD|PA_DYNVOL);
            }

            return 0;
        }
        
        // If (GPS or RotEnc or Remote) speed >= 88, trigger P1 immediately

        if(!ETTWithFixedLead) {
            // If we have GPS, rotEnc or Remote speed and its >= 88, don't
            // waste time on the lead (unless 5s-lead-depending props need it)
            forceNoLead = true;
        }
    }

    /*
     * Complete tt: Trigger P1 (display disruption)
     *
     */
    if(doComplete) {

        #ifdef TC_HAVE_REMOTE
        // Clear it here, will be set after timetravel() in caller
        remoteInducedTT = false;
        #endif

        if(ETTWithFixedLead || bttfnHaveClients || pubMQTTVL) {

            long  ettoLeadT = ettoLeadTime;
            long  P1_88 = TT_P1_POINT88;

            triggerP1 = true;
            triggerP1NoLead = false;
            triggerETTO = true;

            bttfnTTLeadTime = ettoLeadTime;           //
            
            if(forceNoLead && !ETTWithFixedLead) {
                P1_88 = 0;
                triggerP1NoLead = true;
                bttfnTTLeadTime = ettoLeadT = P1_88;  //
            }

            //bttfnTTLeadTime = ettoLeadTime;
            
            //if(!ETTWithFixedLead) {
            //    bttfnTTLeadTime = ettoLeadT = P1_88;
            //}

            if(ettoLeadT >= P1_88) {
                triggerETTOLeadTime = 0;
                triggerP1LeadTime = ettoLeadT - P1_88;
                
                if((triggerP1LeadTime > 3000) && havePreTTSound) {
                    play_file(preTTSound, PA_LINEOUT|PA_CHECKNM|PA_INTRMUS|PA_ALLOWSD|PA_DYNVOL);
                    triggerP1LeadTime = ettoLeadT;
                    triggerP1NoLead = true;
                }
                
            } else {
                triggerP1LeadTime = 0;
                triggerETTOLeadTime = P1_88 - ettoLeadT;

                if(triggerETTOLeadTime > 500) {
                    sendNetWorkMsg("PREPARE\0", 8, BTTFN_NOT_PREPARE);
                }
            }

            ttUnivNow = millis();
            triggerETTONow = triggerP1Now = ttUnivNow;

            // Set now to block input and other interfering actions.
            // Will be set to something valid in triggerLongTT().
            timeTravelP1 = -1;

            return 0;
        }

        triggerP1 = false;
        triggerETTO = false;
        triggerLongTT(forceNoLead);

        return 0;
    }

    /*
     * Re-entry part:
     *
     */

    allOff();

    #ifndef PERSISTENT_SD_ONLY
    if(timetravelPersistent && !presentTime._configOnSD && playTTsounds) {
        // Avoid stutter when writing to esp32 flash
        stopAudio();
    }
    #endif

    // Copy 'present time' to 'last time departed'
    // (plus, in Exhibition Mode: Copy 'destination time' to 'present time')
    copyPresentToDeparted(false);

    // Calculate time difference between RTC and destination time
    // (timeDifference is relative to UTC)
    // Skipped in Exhibition Mode
    if(!stalePresent) {
        DateTime dtu;
        myrtcnow(dtu);
        uint64_t rtcTime = dateToMins(dtu.year(),
                                      dtu.month(),
                                      dtu.day(),
                                      dtu.hour(),
                                      dtu.minute());
    
        uint64_t newTime = dateToMins(
                    destinationTime.getYear(),
                    destinationTime.getMonth(),
                    destinationTime.getDay(),
                    destinationTime.getHour(),
                    destinationTime.getMinute());
    
        if(rtcTime < newTime) {
            timeDifference = newTime - rtcTime;
            timeDiffUp = true;
        } else {
            timeDifference = rtcTime - newTime;
            timeDiffUp = false;
        }
    }

    // We only save the new time data to NVM if user wants persistence.
    if(timetravelPersistent) {
        audio_loop();
        // Don't save delayed; write-delay hurts here less than anywhere else
        departedTime.save();
        audio_loop();
        if(stalePresent) {
            saveStaleTime((void *)&stalePresentTime[0], stalePresent);
        } else {
            presentTime.save();
        }
    }

    timetravelNow = millis();
    timeTravelRE = true;

    if(playTTsounds) {
        play_file("/timetravel.mp3", PA_LINEOUT|PA_CHECKNM|PA_INTRMUS|PA_ALLOWSD|PA_DYNVOL);
    }

    // For external props: Signal Re-Entry
    if(pubMQTT || bttfnHaveClients) {
        sendNetWorkMsg("REENTRY\0", 8, BTTFN_NOT_REENTRY);
    }
    ettoPulseEnd();

    // If P0 was played (or faked when we hit 88 by GPS or user input):
    // Initiate P2 - count speed down
    // (We know P0 was played or faked if timeTravelP0Speed = 88)
    // Additionally, do this only if 
    
    #ifdef TC_HAVE_REMOTE
    // If TT triggered by remote now gone, we do P2
    if(!bttfnRemoteSpeedMaster && remoteInducedTT) {
        if(withSpeedo) timeTravelP0Speed = 88;
    }
    #endif
    if(withSpeedo && (timeTravelP0Speed == 88)) {
        timeTravelP2 = 1;
        timetravelP0Now = ttUnivNow;
        timetravelP0Delay = 2000;
    }

    #ifdef TC_HAVE_REMOTE
    remoteInducedTT = false;
    #endif

    // If there is no P2 (because there is no speedo), 
    // we reset the RotEnc here (it could have been
    // moved during P1 while we didn't poll).
    // This is otherwise done at the end of P2.
    #ifdef TC_HAVE_RE
    if(!timeTravelP2 && useRotEnc) {
        if(rotEnc.IsOff()) {
            re_init(false);
        } else {
            re_init();
            re_lockTemp();
        }
        GPSabove88 = false;
    }
    #endif

    // If there is no P2, we sync to the remote here.
    // This is otherwise done at the end of P2.
    #ifdef TC_HAVE_REMOTE
    if(!timeTravelP2) {
        updAndDispRemoteSpeed();
    }
    #endif

    return 0;
}

static void triggerLongTT(bool noLead)
{
    if(playTTsounds) play_file(noLead ? "/travelstart2.mp3" : "/travelstart.mp3", 
                               PA_LINEOUT|PA_CHECKNM|PA_INTRMUS|PA_ALLOWSD|PA_DYNVOL,
                               TT_SOUND_FACT);
    timetravelP1Now = millis();
    timetravelP1Delay = noLead ? 0 : TT_P1_DELAY_P1;
    timeTravelP1 = 1;
    timeTravelP2 = 0;
}

#if (defined(TC_HAVEGPS) && defined(NOT_MY_RESPONSIBILITY)) || defined(TC_HAVE_RE) || defined(TC_HAVE_REMOTE)
static void checkForSpeedTT(bool isFake, bool isRemote)
{
    if(!GPSabove88) {
        if(FPBUnitIsOn && !presentTime.getNightMode() &&
           !startup && !timeTravelP0 && !timeTravelP1 && !timeTravelRE && !timeTravelP2) {
            timeTravel(true, false, true);
            GPSabove88 = true;
            // If speed source is fake (ie using rotEnc or Remote),
            // we need to set this to trigger P2 at end of TT
            if(isFake) timeTravelP0Speed = 88;
            #ifdef TC_HAVE_REMOTE
            remoteInducedTT = isRemote;
            #endif
        }
    }
}
#endif

void setTTOUTpin(uint8_t val)
{
    #ifndef EXPS
    digitalWrite(EXTERNAL_TIMETRAVEL_OUT_PIN, val);
    #endif
    #ifdef TC_DBG
    digitalWrite(WHITE_LED_PIN, val);
    #endif
}

static void ettoPulseStart()
{
    if(useETTOWired) {
        setTTOUTpin(HIGH);
        #ifdef TC_DBG
        Serial.printf("ETTO Start %d\n", millis());
        #endif
    }
}

static void ettoPulseStartNoLead()
{
    if(useETTOWiredNoLead) {
        setTTOUTpin(HIGH);
        #ifdef TC_DBG
        Serial.printf("ETTO Start (no lead) %d\n", millis());
        #endif
    }
}

void ettoPulseEnd()
{
    if(useETTOWired || useETTOWiredNoLead) {
        setTTOUTpin(LOW);
    }
}

static char *i2a(char *d, unsigned int t)
{
    unsigned const int tt[4] = { 1000, 100, 10, 1 };
    unsigned int ttt;
        
    for(int i = 0; i < 4; i++) {
        ttt = t / tt[i];
        *d++ = ttt + '0';
        t -= (ttt * tt[i]);
    }
    return d;
}

void send_refill_msg()
{
    bttfn_notify(BTTFN_TYPE_PCG, BTTFN_NOT_REFILL, 0, 0);
}

void send_wakeup_msg()
{
    if(pubMQTT || bttfnHaveClients) {
        sendNetWorkMsg("WAKEUP\0", 7, BTTFN_NOT_WAKEUP);
    }
}
                   
void send_abort_msg()
{
    if(pubMQTT || bttfnHaveClients) {
        if(timeTravelP0 || (timeTravelP1 > 0) || timeTravelRE || timeTravelP2) {
            sendNetWorkMsg("ABORT_TT\0", 9, BTTFN_NOT_ABORT_TT);
            #ifdef TC_DBG
            Serial.println("Sending ABORT");
            #endif
        }
    }
}

static void sendTTNetWorkMsg(uint16_t bttfnPayload, uint16_t bttfnPayload2)
{
    #ifdef TC_DBG
    Serial.printf("sendTTNetWorkMsg: %d %d\n", bttfnPayload, bttfnPayload2);
    #endif
    #ifdef TC_HAVEMQTT
    if(pubMQTT) {
        char pl[32] = "TIMETRAVEL\0\0";
        if(MQTTvarLead) {
            pl[10] = '_';
            char *d = i2a(&pl[11], bttfnPayload);
            *d++ = '_';
            d = i2a(d, bttfnPayload2);
            *d = 0;
        }
        mqttPublish("bttf/tcd/pub", pl, strlen(pl)+1);
        return;
    }
    #endif
    bttfn_notify(BTTFN_TYPE_ANY, BTTFN_NOT_TT, bttfnPayload, bttfnPayload2);
}

// Send notification message via MQTT -or- BTTFN.
// If MQTT is enabled in settings, and "Send event notifications"
// is checked, send via MQTT (regardless of connection status). 
// Otherwise, send via BTTFN.
// Props can rely on getting only ONE notification message if they listen
// to both MQTT and BTTFN.
static void sendNetWorkMsg(const char *pl, unsigned int len, uint8_t bttfnMsg, uint16_t bttfnPayload, uint16_t bttfnPayload2)
{
    #ifdef TC_HAVEMQTT
    if(pubMQTT) {
        mqttPublish("bttf/tcd/pub", pl, len);
        return;
    }
    #endif
    bttfn_notify(BTTFN_TYPE_ANY, bttfnMsg, bttfnPayload, bttfnPayload2);
}

void bttfnSendFluxCmd(uint32_t payload)
{
    bttfn_notify(BTTFN_TYPE_FLUX, BTTFN_NOT_FLUX_CMD, payload & 0xffff, payload >> 16);
}
void bttfnSendSIDCmd(uint32_t payload)
{
    bttfn_notify(BTTFN_TYPE_SID, BTTFN_NOT_SID_CMD, payload & 0xffff, payload >> 16);
}
void bttfnSendPCGCmd(uint32_t payload)
{
    bttfn_notify(BTTFN_TYPE_PCG, BTTFN_NOT_PCG_CMD, payload & 0xffff, payload >> 16);
}
void bttfnSendVSRCmd(uint32_t payload)
{
    bttfn_notify(BTTFN_TYPE_VSR, BTTFN_NOT_VSR_CMD, payload & 0xffff, payload >> 16);
}
void bttfnSendAUXCmd(uint32_t payload)
{
    bttfn_notify(BTTFN_TYPE_AUX, BTTFN_NOT_AUX_CMD, payload & 0xffff, payload >> 16);
}
void bttfnSendRemCmd(uint32_t payload)
{
    bttfn_notify(BTTFN_TYPE_REMOTE, BTTFN_NOT_REM_CMD, payload & 0xffff, payload >> 16);
}

/*
 * Reset present time to actual present time
 * (aka "return from time travel")
 */
void resetPresentTime()
{
    if(!currentlyOnTimeTravel())
        return;
    
    pwrNeedFullNow();

    // Disable RC and WC modes
    // Note that in WC mode, the times in red & yellow
    // displays work like "normal" times: The user travels
    // to present time, his current time becomes last
    // time departed; destination time is unchanged, but
    // both dest & last time dep. time become stale.
    enableRcMode(false);
    enableWcMode(false);

    cancelEnterAnim();
    cancelETTAnim();

    if(playTTsounds) {
        mp_stop();
        #ifndef PERSISTENT_SD_ONLY
        // Stop audio to avoid "stutter" when writing to ESP32 flash
        if(timetravelPersistent && !presentTime._configOnSD) {
            stopAudio();
        }
        #endif
    }

    allOff();

    // Copy 'present time' to 'last time departed'
    // (plus, in Exhibition Mode: restore 'present time')
    copyPresentToDeparted(true);

    // Reset time
    // Skipped in Exhibition Mode
    if(!stalePresent)
        timeDifference = 0;
    
    // We only save the new time data if user wants persistence.
    if(timetravelPersistent) {
        audio_loop();
        // Don't save delayed; write-delay hurts here less than anywhere else
        departedTime.save();
        audio_loop();
        if(stalePresent) {
            saveStaleTime((void *)&stalePresentTime[0], stalePresent);
        } else {
            presentTime.save();
        }
    }

    if(playTTsounds) {
        play_file("/timetravel.mp3", PA_LINEOUT|PA_CHECKNM|PA_INTRMUS|PA_ALLOWSD|PA_DYNVOL);
    }

    timetravelNow = millis();
    timeTravelRE = true;

    // Beep auto mode: Restart timer
    startBeepTimer();

    // Wake up network clients
    send_wakeup_msg();
}

static void copyPresentToDeparted(bool isReturn)
{
    if(stalePresent) {
        departedTime.setFromStruct(&stalePresentTime[1]);
        if(isReturn) {
            memcpy((void *)&stalePresentTime[1], (void *)&stalePresentTime[0], sizeof(dateStruct));
        } else {
            destinationTime.getToStruct(&stalePresentTime[1]);
        }
    } else {
        dateStruct s;
        presentTime.getToStruct(&s);
        departedTime.setFromStruct(&s);
    }
}

bool currentlyOnTimeTravel()
{
    if(stalePresent) {
        return (memcmp((void *)&stalePresentTime[0], (void *)&stalePresentTime[1], sizeof(dateStruct)) != 0);
    }
    
    return (timeDifference != 0);
}

/*
 * Send date/time to presentTime: Either vanilla (LOCAL), or with timeDifference(to UTC)
 */
void updatePresentTime(DateTime& dtu, DateTime& dtl)
{
    uint64_t utcMins;
    int year, month, day, hour, minute;

    if(!timeDifference) {
        presentTime.setDateTime(dtl);
        return;
    }

    if(!dtu._minsValid) {
        dtu._mins = dateToMins(dtu.year(), dtu.month(), dtu.day(), dtu.hour(), dtu.minute());
        dtu._minsValid = true;
    }

    utcMins = dtu._mins;
    
    if(timeDiffUp) {
        utcMins += timeDifference;
        // Don't care about 9999-10000 roll-over
        // So we display 0000 for 10000+
        // So be it.
    } else {
        utcMins -= timeDifference;
    }

    minsToDate(utcMins, year, month, day, hour, minute);

    presentTime.setFromParms(year, month, day, hour, minute);
}

// Pause autoInverval-updating for 30 minutes
// Subsequent calls re-start the pause.
void pauseAuto(void)
{
    if(autoTimeIntervals[autoInterval]) {
        pauseDelay = 30 * 60 * 1000;
        autoPaused = true;
        pauseNow = millis();
        #ifdef TC_DBG
        Serial.println("pauseAuto: autoInterval paused for 30 minutes");
        #endif
    }
}

bool checkIfAutoPaused()
{
    if(!autoPaused || (millis() - pauseNow >= pauseDelay)) {
        return false;
    }
    return true;
}

void endPauseAuto(void)
{
    autoPaused = false;
}

/*
 * Callbacks for fake power switch
 */
#if defined(TC_HAVEMQTT)
void mqttFakePowerControl(bool isPwrMaster)
{
    if(MQTTPwrMaster != isPwrMaster) {
        #ifdef TC_HAVE_REMOTE
        if(!bttfnRemPwrMaster) {
        #endif
            if(!isPwrMaster) {
                if(waitForFakePowerButton) {
                    isFPBKeyPressed = shadowFPBKeyPressed;
                    isFPBKeyChange = true;
                } else {
                    restoreFakePwr = true;
                    isFPBKeyPressed = isFPBKeyChange = true;
                }
            } else {
                isFPBKeyPressed = shadowMQTTFPBKeyPressed;
                isFPBKeyChange = true;
            }
        #ifdef TC_HAVE_REMOTE
        }
        #endif
        MQTTPwrMaster = isPwrMaster;
    }
}

void mqttFakePowerOn()
{
    shadowMQTTFPBKeyPressed = true;
    #ifdef TC_HAVE_REMOTE
    if(bttfnRemPwrMaster) return;
    #endif
    if(!MQTTPwrMaster) return;
    isFPBKeyPressed = true;
    isFPBKeyChange = true;
}

void mqttFakePowerOff()
{
    shadowMQTTFPBKeyPressed = false;
    #ifdef TC_HAVE_REMOTE
    if(bttfnRemPwrMaster) return;
    #endif
    if(!MQTTPwrMaster) return;
    isFPBKeyPressed = false;
    isFPBKeyChange = true;
}
#endif // TC_HAVEMQTT

void fpbKeyPressed()
{
    #if defined(TC_HAVE_REMOTE) || defined(TC_HAVEMQTT)
    shadowFPBKeyPressed = true;
    #endif
    #ifdef TC_HAVE_REMOTE
    if(bttfnRemPwrMaster) return;
    #endif
    #ifdef TC_HAVEMQTT
    if(MQTTPwrMaster) return;
    #endif
    isFPBKeyPressed = true;
    isFPBKeyChange = true;
}

void fpbKeyLongPressStop()
{
    #if defined(TC_HAVE_REMOTE) || defined(TC_HAVEMQTT)
    shadowFPBKeyPressed = false;
    #endif
    #ifdef TC_HAVE_REMOTE
    if(bttfnRemPwrMaster) return;
    #endif
    #ifdef TC_HAVEMQTT
    if(MQTTPwrMaster) return;
    #endif
    isFPBKeyPressed = false;
    isFPBKeyChange = true;
}

/*
 * Delay function for keeping essential stuff alive
 * during Intro playback
 * DO NOT USE THIS AS SOON AS THE LOOPs() ARE RUNNING
 * (RotEnc double-polling!)
 * 
 */
static void myIntroDelay(unsigned int mydel, bool withGPS)
{
    unsigned long startNow = millis();
    while(millis() - startNow < mydel) {
        delay((mydel > 100) ? 10 : 5);    // delay(5) does not allow for UDP traffic
        audio_loop();
        ntp_short_loop();
        bttfn_loop();
        #if defined(TC_HAVEGPS) || defined(TC_HAVE_RE)
        if(withGPS) gps_loop(true);
        #endif
        if(withGPS) wifi_loop();
    }
}

/*
 * DO NOT USE THIS AS SOON AS THE LOOPs() ARE RUNNING
 * (RotEnc double-polling!) 
 */
static void waitAudioDoneIntro()
{
    int timeout = 100;

    while(!checkAudioDone() && timeout--) {
        audio_loop();
        ntp_short_loop();
        bttfn_loop();
        #if defined(TC_HAVEGPS) || defined(TC_HAVE_RE)
        gps_loop(true);
        #endif
        wifi_loop();
        audio_loop();
        delay(10);
    }
}

/*
 * Delay function for external modules
 * (gps, temp sensor, light sensor). 
 * Do not call gps_loop() or wifi_loop() here!
 */
static void myCustomDelay_int(unsigned long mydel, uint32_t gran)
{
    unsigned long startNow = millis();
    audio_loop();
    while(millis() - startNow < mydel) {
        delay(gran);
        audio_loop();
        ntp_short_loop();
    }
}
static void myCustomDelay_Sens(unsigned long mydel)
{
    myCustomDelay_int(mydel, 5);
}
static void myCustomDelay_GPS(unsigned long mydel)
{
    myCustomDelay_int(mydel, 1);
}
void myCustomDelay_KP(unsigned long mydel)
{
    myCustomDelay_int(mydel, 1);
}

/*
 *  Short/medium delay
 *  This allows other stuff to proceed
 */
void mydelay(unsigned long mydel)
{
    unsigned long startNow = millis();
    myloops(false);
    while(millis() - startNow < mydel) {
        delay(5);
        myloops(false);
    }
}

void waitAudioDone()
{
    int timeout = 200;

    while(!checkAudioDone() && timeout--) {
        mydelay(10);
        wifi_loop();
        audio_loop();
    }
}

// Call this to get full CPU speed
void pwrNeedFullNow(bool force)
{
    if(pwrLow || force) {
        setCpuFrequencyMhz(240);
        #ifdef TC_DBG
        Serial.print("Setting CPU speed to ");
        Serial.println(getCpuFrequencyMhz());
        #endif
    }
    pwrFullNow = millis();
    pwrLow = false;
}

/*
 * Internal wrapper for RTC.now()
 * Corrects RTC-read year by yearOffset
 * Stores RTC-hardware year to dt.hwRTCYear
 * (RTC sometimes loses sync and does not send data,
 * which is interpreted as 2165/165/165 etc
 * Check for this and retry in case.)
 */
void myrtcnow(DateTime& dt)
{
    int retries = 0;

    rtc.now(dt);

    while ((dt.month() < 1 || dt.month() > 12 ||
            dt.day()   < 1 || dt.day()   > 31 ||
            dt.hour() > 23 ||
            dt.minute() < 0 || dt.minute() > 59) &&
            retries < 30 ) {

            mydelay((retries < 5) ? 50 : 100);
            rtc.now(dt);
            retries++;
    }

    dt.hwRTCYear = dt.year();
    dt.setYear(dt.hwRTCYear - presentTime.getYearOffset());

    if(retries > 0) {
        Serial.printf("%d retries reading RTC. Check your i2c cabling.\n", retries);
    }
}

/*
 * millis64() - 64bit millis()
 * 
 */
uint64_t millis64()
{
    unsigned long millisNow64 = millis();
    
    if(millisNow64 < lastMillis64) {
        millisEpoch += 0x100000000;
    }
    lastMillis64 = millisNow64;

    return millisEpoch | (uint64_t)millisNow64;
}

/*
 * World Clock setters/getters/helpers
 * 
 */

void enableWcMode(bool onOff)
{
    if(haveWcMode) {
        wcMode = onOff;
        triggerWC = true;
    }
}

bool toggleWcMode()
{
    enableWcMode(!wcMode);
    return wcMode;
}

bool isWcMode()
{
    return wcMode;
}

/*
 * Convert UTC time to other time zone, and copy
 * them to display. If a TZ for a display is not
 * configured, it does not touch that display.
 */
void setDatesTimesWC(DateTime& dtu)
{
    DateTime dtl;

    if(WcHaveTZ1) {
        UTCtoLocal(dtu, dtl, 1);
        //destinationTime.setFromParms(dtl.year(), dtl.month(), dtl.day(), dtl.hour(), dtl.minute());
        destinationTime.setDateTime(dtl);
    }
    if(WcHaveTZ2) {
        UTCtoLocal(dtu, dtl, 2);
        //departedTime.setFromParms(dtl.year(), dtl.month(), dtl.day(), dtl.hour(), dtl.minute());
        departedTime.setDateTime(dtl);
    }
}

/*
 * Room Condition setters/getters
 * 
 */

void enableRcMode(bool onOff)
{
    #ifdef TC_HAVETEMP
    if(haveRcMode) {
        rcMode = onOff;
        tempUpdInt = rcMode ? TEMP_UPD_INT_S : TEMP_UPD_INT_L;
    }
    #endif
}

bool toggleRcMode()
{
    #ifdef TC_HAVETEMP
    enableRcMode(!rcMode);
    return rcMode;
    #else
    return false;
    #endif
}

bool isRcMode()
{
    #ifdef TC_HAVETEMP
    return rcMode;
    #else
    return false;
    #endif
}

/*
 * Temperature / GPS speed display
 */
#ifdef TC_HAVETEMP
static void updateTemperature(bool force)
{
    unsigned long tui = tempUpdInt;
    unsigned long now = millis();

    if(!useTemp)
        return;

    if(tempSens.lastTempNan() && (now < 15*1000)) {
        tui = 5 * 1000;
    }
        
    if(force || (now - tempReadNow >= tui)) {
        tempSens.readTemp(tempUnit);
        tempReadNow = now;
    }
}
#endif

#if defined(TC_HAVEGPS) || defined(TC_HAVE_RE)
static void displayGPSorRESpeed(bool force)
{
    bool spdreNM = false; 
    bool spdreChgNM = false;
    
    if(!useSpeedoDisplay || (!dispGPSSpeed && !dispRotEnc))
        return;

    if(timeTravelP0 || timeTravelP1 || timeTravelRE || timeTravelP2)
        return;

    #ifdef TC_HAVE_RE
    if(dispRotEnc && (!FPBUnitIsOn || startup))
        return;
    #endif

    unsigned long now = millis();

    if(dispGPSSpeed) {
        #ifdef TC_HAVEGPS
        int16_t gpsSpeed = myGPS.getSpeed();
        #ifdef TC_HAVE_REMOTE
        if(remoteWasMaster) {
            int8_t speedoSpeed = speedo.getSpeed();
            if(gpsSpeed < 0 || speedoSpeed < 0 || speedoSpeed == gpsSpeed) {
                remoteWasMaster = 0;
                force = true;
            } else if(now - lastRemSpdUpd > remSpdCatchUpDelay) {
                if(speedoSpeed < gpsSpeed) {
                    remSpdCatchUpDelay = (speedoSpeed < 88) ?
                              (unsigned long)(((float)(tt_p0_delays[speedoSpeed])) / ttP0TimeFactor) : 40;
                    speedoSpeed++;
                } else {
                    remSpdCatchUpDelay = 40;
                    speedoSpeed--;
                }
                remoteWasMaster--;    // Limit number of chances to catch up with actual GPS speed
                if(remoteWasMaster) {
                    speedo.setSpeed(speedoSpeed);
                    speedo.show();
                    speedo.on();
                    lastRemSpdUpd = now;
                    speedoStatus = SPST_GPS;
                } else {
                    force = true;
                }
            }
        }
        if(!remoteWasMaster) {
        #endif
            if(force || (now - dispGPSnow >= GPSupdateFreq)) {
                speedo.setSpeed(gpsSpeed);
                speedo.show();
                speedo.on();
                dispGPSnow = now;
                speedoStatus = SPST_GPS;
            }
        #ifdef TC_HAVE_REMOTE
        }
        #endif
        #endif
    } else if(dispRotEnc) {
        #ifdef TC_HAVE_RE
        spdreNM = speedo.getNightMode();
        spdreChgNM = (spdreNM != spdreOldNM);
        spdreOldNM = spdreNM;
        if((spdreChgNM && speedoStatus == SPST_RE) || force || (fakeSpeed != oldFakeSpeed)) {
            if(dispTemp || fakeSpeed >= 0) {
                speedo.setSpeed(fakeSpeed);
                if(speedoStatus == SPST_TEMP) {
                    speedo.setBrightness(255);
                }
                speedo.show();
                speedo.on();
            } else {
                speedo.off();
            }
            if(force || (fakeSpeed != oldFakeSpeed)) {
                if(fakeSpeed >= 0) {
                    re_lockTemp();
                } else {
                    tempLock = false;
                    #ifdef TC_HAVETEMP
                    tempDispNow = millis() - 2*60*1000;   // Force immediate re-display of temp
                    #endif
                }
            }
            oldFakeSpeed = fakeSpeed;
            speedoStatus = SPST_RE;
        }
        #endif
    }
}
#endif
#ifdef TC_HAVETEMP
static bool dispTemperature(bool force)
{
    bool tempNM = speedo.getNightMode();
    unsigned long now = millis();

    if(!dispTemp)
        return false;

    if(!FPBUnitIsOn || startup || timeTravelP0 || timeTravelP1 || timeTravelRE || timeTravelP2)
        return false;

    #ifdef TC_HAVE_RE
    if(dispRotEnc && tempLock) {
        if(now - tempLockNow < 5 * 60 * 1000) {
            return false;
        } else {
            tempLock = false;
            force = true;
            // Reset encoder to "disabled" pos
            re_init(false);
        }
    }
    #endif

    force |= (tempNM != tempOldNM);
    tempOldNM = tempNM;

    force |= (tempSens.lastTempNan() != tempLastNan);
    tempLastNan = tempSens.lastTempNan();

    if(force || (now - tempDispNow >= 2*60*1000)) {
        if(tempNM && tempOffNM) {
            speedo.off();
        } else {
            speedo.setTemperature(tempSens.readLastTemp());
            speedo.show();
            if(!tempNM) {
                speedo.setBrightnessDirect(tempBrightness); // after show b/c brightness touched by show
            }
            speedo.on();
        }
        tempDispNow = now;
        speedoStatus = SPST_TEMP;
    }

    return true;
}
#endif
static void dispIdleZero(bool force)
{
    if(!FPBUnitIsOn || startup || timeTravelP0 || timeTravelP1 || timeTravelRE || timeTravelP2)
        return;

    unsigned long now = millis();

    if(force || (now - dispIdlenow >= 500)) {

        speedo.setSpeed(0);
        speedo.show();
        speedo.on();
        dispIdlenow = now;
        speedoStatus = SPST_ZERO;

    }
}

#ifdef TC_HAVE_REMOTE
static void updAndDispRemoteSpeed()
{
    unsigned long now = millis();
    
    // Look at bttfnRemCurSpd (curr. displayed) and bttfnRemoteSpeed (displayed on remote)
    // and also bttfnRemStop (if true, the brake is on)

    if(!bttfnRemoteSpeedMaster)
        return;

    // To avoid speed-jumps after a tt (such as in the case of the remote's brake being
    // hit while the TCD performs a tt), we do not adapt the "car"-speed to the remote 
    // speed during a tt. We can do that in P2 though, since P2 runs down to the then 
    // current remote speed (and, btw, isn't being run when the remote triggered the TT
    // by hitting 88)

    if(!timeTravelP0 && !timeTravelP1 && !timeTravelRE /*&& !timeTravelP2*/) {
      
        if(useSpeedoDisplay) {

            if(bttfnRemStop) {
                // Brake on: Keep at zero, or go down to zero, regardless of bttfnRemoteSpeed
                if(bttfnRemCurSpd > 0) {
                    remSpdCatchUpDelay = 40;
                    if(now - lastRemSpdUpd > remSpdCatchUpDelay) {
                        bttfnRemCurSpd--;
                        lastRemSpdUpd = now;
                    }
                }            
            } else if(bttfnRemCurSpd != bttfnRemoteSpeed) {
                if(now - lastRemSpdUpd > remSpdCatchUpDelay) {
                    if(bttfnRemCurSpd < bttfnRemoteSpeed) {
                        bttfnRemCurSpd++;
                        if(bttfnRemCurSpd < 88) {
                            float nD = (float)tt_p0_delays[bttfnRemCurSpd]; // 4.2
                            int sD = bttfnRemoteSpeed - bttfnRemCurSpd;
                            if(sD > 4)      nD /= 4.2;
                            else if(sD > 1) nD /= (float)sD;
                            else            nD /= 2.0;
                            remSpdCatchUpDelay = (unsigned long)nD;
                        } else {
                            remSpdCatchUpDelay = 40;
                        }
                    } else {
                        remSpdCatchUpDelay = 40;
                        bttfnRemCurSpd--;
                    }
                    lastRemSpdUpd = now;
                }
            }

        } else {

            // If no speed-displaying device is present, we just copy the 
            // remote speed. No point in smooth updates no one can see.
            bttfnRemCurSpd = bttfnRemStop ? 0 : bttfnRemoteSpeed;

        }
        
    }

    if(useSpeedoDisplay) {
        // We do not interfere with P2 here, since P2, by definition, counts the speed
        // down to the then current remote speed; do not overwrite the displayed speed here!
        if(!timeTravelP0 && !timeTravelP1 && !timeTravelRE && !timeTravelP2) {
            if(speedoStatus != SPST_REM || 
               bttfnRemCurSpd != bttfnOldRemCurSpd ||
               presentTime.getNightMode() != oldptnmrem) {
                bttfnOldRemCurSpd = bttfnRemCurSpd;
                speedo.setSpeed(bttfnRemCurSpd);
                if(speedoStatus == SPST_TEMP) {
                    speedo.setBrightness(255);
                }
                speedo.show();
                speedo.on();
                speedoStatus = SPST_REM;
                oldptnmrem = presentTime.getNightMode();
            }
        }
    }

}
#endif

#ifdef TC_HAVE_RE                
static void re_init(bool zero)
{                
    if(zero) {
        // Reset encoder to "zero" pos
        rotEnc.zeroPos();
        fakeSpeed = oldFSpd = 0;
    } else {
        // Reset encoder to "disabled" pos
        rotEnc.disabledPos();
        fakeSpeed = oldFSpd = -1;
    }
}

static void re_setToSpeed(int16_t speed)
{
    rotEnc.speedPos(speed);
    fakeSpeed = oldFSpd = speed;
}

static void re_lockTemp()
{
    if(dispRotEnc) {
        tempLock = true;
        tempLockNow = millis();
    }
 }

void re_vol_reset()
{
    if(useRotEncVol) {
        rotEncVol->zeroPos((curVolume == 255) ? 0 : curVolume);
    }
}
#endif

// Called before reboot to save regardless of running delay
void flushDelayedSave()
{
    presentTime.saveClockStateData(lastYear);

    presentTime.saveFlush();
    destinationTime.saveFlush();
    departedTime.saveFlush();
                
    if(volChanged) {
        volChanged = false;
        saveCurVolume();
    }
}

/*
 * Display helpers
 */
// Show all, month after a short delay
void animate(bool withLEDs)
{
    #ifdef TC_HAVETEMP
    if(isRcMode() && (!isWcMode() || !WcHaveTZ1)) {
            destinationTime.showTempDirect(tempSens.readLastTemp(), tempUnit, true);
    } else
    #endif
        destinationTime.showAnimate1();

    presentTime.showAnimate1();

    #ifdef TC_HAVETEMP
    if(isRcMode()) {
        if(isWcMode() && WcHaveTZ1) {
            departedTime.showTempDirect(tempSens.readLastTemp(), tempUnit, true);
        } else if(!isWcMode() && tempSens.haveHum()) {
            departedTime.showHumDirect(tempSens.readHum(), true);
        } else {
            departedTime.showAnimate1();
        }
    } else
    #endif
        departedTime.showAnimate1();

    if(withLEDs) {
        leds_on();
    }
        
    mydelay(80);

    #ifdef TC_HAVETEMP
    if(isRcMode() && (!isWcMode() || !WcHaveTZ1)) {
        destinationTime.showTempDirect(tempSens.readLastTemp(), tempUnit);
    } else
    #endif
        destinationTime.showAnimate2();
        
    presentTime.showAnimate2();

    #ifdef TC_HAVETEMP
    if(isRcMode()) {
        if(isWcMode() && WcHaveTZ1) {
            departedTime.showTempDirect(tempSens.readLastTemp(), tempUnit);
        } else if(!isWcMode() && tempSens.haveHum()) {
            departedTime.showHumDirect(tempSens.readHum());
        } else {
            departedTime.showAnimate2();
        }
    } else
    #endif
        departedTime.showAnimate2();
}

// Activate lamp test on all displays and turn on
void allLampTest()
{
    destinationTime.on();
    presentTime.on();
    departedTime.on();
    destinationTime.lampTest();
    presentTime.lampTest();
    departedTime.lampTest();
}

void allOff()
{
    destinationTime.off();
    presentTime.off();
    departedTime.off();
}

static void startDisplays()
{
    presentTime.begin();
    destinationTime.begin();
    departedTime.begin();
}

/**************************************************************
 ***                                                        ***
 ***              NTP/GPS time synchronization              ***
 ***                                                        ***
 **************************************************************/

// choose your time zone from this list
// https://github.com/nayarsystems/posix_tz_db/blob/master/zones.csv

/* ATTN:
 *  DateTime uses 1-12 for months
 *  tm (_timeinfo) uses 0-11 for months
 *  Hardware RTC uses 1-12 for months
 *  Xprintf treats "%B" substition as from timeinfo, ie 0-11
 */

/*
 * Get UTC time from NTP or GPS
 * and update given DateTime
 * 
 */
static bool getNTPOrGPSTime(bool weHaveAuthTime, DateTime& dt, bool updateRTC)
{
    // If GPS has a time and WiFi is off, try GPS first.
    // This avoids a frozen display when WiFi reconnects.
    #ifdef TC_HAVEGPS
    if(gpsHaveTime() && wifiIsOff) {
        if(getGPStime(dt, updateRTC)) return true;
    }
    #endif

    // Now try NTP
    if(getNTPTime(weHaveAuthTime, dt, updateRTC)) return true;

    // Again go for GPS, might have older timestamp
    #ifdef TC_HAVEGPS
    return getGPStime(dt, updateRTC);
    #else
    return false;
    #endif
}

/*
 * Get UTC time from NTP
 * 
 * Saves time to RTC; sets yearOffs (but does not save it to NVM)
 * 
 * Does no re-tries in case NTPGetLocalTime() fails; this is
 * called repeatedly within a certain time window, so we can 
 * retry with a later call.
 */
static bool getNTPTime(bool weHaveAuthTime, DateTime& dt, bool adjustRTC)
{
    uint16_t rtcYear;
    int16_t  rtcYOffs = 0;

    if(settings.ntpServer[0] == 0) {
        return false;
    }

    pwrNeedFullNow();

    // Reconnect for only 3 mins, but only if the user configured
    // a WiFi network to connect to; do not start CP.
    // If we don't have authTime yet, connect for longer to avoid
    // reconnects (aka frozen displays).
    // If WiFi is reconnected here, we won't have a valid time stamp
    // immediately. This will therefore fail the first time called.
    wifiOn(weHaveAuthTime ? 3*60*1000 : 21*60*1000, false, true);    

    if(WiFi.status() == WL_CONNECTED) {

        int nyear, nmonth, nday, nhour, nmin, nsecond;

        if(NTPGetUTC(nyear, nmonth, nday, nhour, nmin, nsecond)) {
            
            // Get RTC-fit year plus offs for given real year
            rtcYear = nyear;
            correctYr4RTC(rtcYear, rtcYOffs);

            rtc.prepareAdjust(nsecond,
                              nmin,
                              nhour,
                              dayOfWeek(nday, nmonth, nyear),
                              nday,
                              nmonth,
                              rtcYear - 2000);

            if(adjustRTC) rtc.finishAdjust();

            presentTime.setYearOffset(rtcYOffs);

            dt.hwRTCYear = rtcYear;
            dt.set(nyear, nmonth, nday, 
                   nhour, nmin,   nsecond);
    
            #ifdef TC_DBG
            Serial.printf("getNTPTime: %d-%02d-%02d %02d:%02d:%02d UTC\n", 
                      nyear, nmonth, nday, nhour, nmin, nsecond);
            #endif
    
            return true;

        } else {

            #ifdef TC_DBG
            Serial.println("getNTPTime: No current NTP timestamp available");
            #endif

            return false;
        }

    } else {

        #ifdef TC_DBG
        Serial.println("getNTPTime: WiFi not connected, NTP time sync skipped");
        #endif
        
        return false;

    }
}

/*
 * Get UTC time from GPS
 * 
 * Saves time to RTC; sets yearOffs (but does not save it to NVM)
 * 
 */
#ifdef TC_HAVEGPS
static bool getGPStime(DateTime& dt, bool adjustRTC)
{
    struct tm timeinfo;
    unsigned long stampAge = 0;
    int nyear, nmonth, nday, nhour, nminute, nsecond;
    uint16_t rtcYear;
    int16_t  rtcYOffs = 0;
   
    if(!useGPS)
        return false;

    if(!myGPS.getDateTime(&timeinfo, &stampAge, GPSupdateFreq))
        return false;
            
    // Correct UTC by stampAge

    nyear = timeinfo.tm_year + 1900;
    nmonth = timeinfo.tm_mon + 1;
    nday = timeinfo.tm_mday;
    nhour = timeinfo.tm_hour;
    nminute = timeinfo.tm_min;
    nsecond = timeinfo.tm_sec + (stampAge / 1000);
    
    if((stampAge % 1000) > 500) nsecond++;
    
    convTime(-(nsecond / 60), nyear, nmonth, nday, nhour, nminute);

    nsecond %= 60;

    // Get RTC-fit year & offs for given real year
    rtcYOffs = 0;
    rtcYear = nyear;
    correctYr4RTC(rtcYear, rtcYOffs);   

    rtc.prepareAdjust(nsecond,
                      nminute,
                      nhour,
                      dayOfWeek(nday, nmonth, nyear),
                      nday,
                      nmonth,
                      rtcYear - 2000);

    if(adjustRTC) rtc.finishAdjust();

    presentTime.setYearOffset(rtcYOffs);

    dt.hwRTCYear = rtcYear;
    dt.set(nyear, nmonth,  nday, 
           nhour, nminute, nsecond);

    #ifdef TC_DBG
    Serial.printf("getGPStime: %d-%02d-%02d %02d:%02d:%02d UTC\n", 
                      nyear, nmonth, nday, nhour, nminute, nsecond);
    #endif
        
    return true;
}

/*
 * Set GPS's own RTC
 * This is only called once upon boot and
 * speeds up finding a fix significantly.
 */
static bool setGPStime()
{
    struct tm timeinfo;
    DateTime dt;

    if(!useGPS)
        return false;

    #ifdef TC_DBG
    Serial.println("setGPStime() called");
    #endif

    myrtcnow(dt);
    
    timeinfo.tm_year = dt.year() - 1900;
    timeinfo.tm_mon  = dt.month() - 1;
    timeinfo.tm_mday = dt.day();
    timeinfo.tm_hour = dt.hour();
    timeinfo.tm_min  = dt.minute();
    timeinfo.tm_sec  = dt.second();

    return myGPS.setDateTime(&timeinfo);
}

bool gpsHaveFix()
{
    if(!useGPS)
        return false;
        
    return myGPS.fix;
}

static bool gpsHaveTime()
{
    if(!useGPS)
        return false;
        
    return myGPS.haveTime();
}
#endif // HAVEGPS

/* Only for menu loop: 
 * Call GPS loop, but do not call custom delay
 * inside, ie no audio_loop().
 * While time_loop() is running, never call 
 * this with (true) (RotEnc double-polling)
 */
#if defined(TC_HAVEGPS) || defined(TC_HAVE_RE)
void gps_loop(bool withRotEnc)
{
    #ifdef TC_HAVEGPS
    if(useGPS && (millis64() >= lastLoopGPS)) {
        lastLoopGPS += (uint64_t)GPSupdateFreq;
        myGPS.loop(false);
        if(dispGPSSpeed) displayGPSorRESpeed(true);
    }
    #endif
    #ifdef TC_HAVE_RE
    if(withRotEnc && useRotEnc && !timeTravelP2) {
        fakeSpeed = oldFSpd = rotEnc.updateFakeSpeed(); // Set oldFSpd as well to avoid unwanted beep-restarts
        if(dispRotEnc) displayGPSorRESpeed();
    }
    #endif
    #ifdef TC_HAVE_REMOTE
    if(withRotEnc) {
        updAndDispRemoteSpeed();
        bttfnRemCurSpdOld = bttfnRemCurSpd; // Avoid unwanted beep-restarts (like above)
    }
    #endif
}
#endif

/*
 * Return doW from given date (year=yyyy)
 */
#ifndef TC_JULIAN_CAL
uint8_t dayOfWeek(int d, int m, int y)
{
    // Sakamoto's method
    const int t[] = { 0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4 };
    if(y > 0) {
        if(m < 3) y -= 1;
        return (y + y/4 - y/100 + y/400 + t[m-1] + d) % 7;
    }
    return (mon_yday[1][m-1] + d + 5) % 7;
}
#else
uint8_t dayOfWeek(int d, int m, int y)
{
    const int t[] = { 0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4 };
    const int u[] = { 5, 1, 0, 3, 5, 1, 3, 6, 2, 4, 0, 2 };
    
    // Sakamoto's method
    if(((y << 16) | (m << 8) | d) > jSwitchHash) {
        if(m < 3) y -= 1;
        return (y + y/4 - y/100 + y/400 + t[m-1] + d) % 7;
    }
    // For Julian Calendar
    if(y > 0) {
        if(m < 3) y -= 1;
        return (y + y/4 + u[m-1] + d) % 7;
    }
    return (mon_yday[1][m-1] + d + 3) % 7;
}
#endif

/* 
 * Find number of days in a month 
 */
int daysInMonth(int month, int year)
{
    if(month == 2 && isLeapYear(year)) {
        return 29;
    }
    return monthDays[month - 1];
}

/* 
 * Determine if provided year is a leap year 
 */
#ifndef TC_JULIAN_CAL 
bool isLeapYear(int year)
{
    if((year & 3) == 0) { 
        if((year % 100) == 0) {
            if((year % 400) == 0) {
                return true;
            } else {
                return false;
            }
        } else {
            return true;
        }
    } else {
        return false;
    }
}
#else
bool isLeapYear(int year)
{
    if((year & 3) == 0) {
        if((year > jSwitchYear) && ((year % 100) == 0)) {
            if((year % 400) == 0) {
                return true;
            } else {
                return false;
            }
        } else {
            return true;
        }
    } else {
        return false;
    }
}
#endif

/*
 *  Convert a date into "minutes since 1/1/0 0:0"
 */
#ifndef TC_JULIAN_CAL 
uint64_t dateToMins(int year, int month, int day, int hour, int minute)
{
    uint64_t total64 = 0;
    uint32_t total32 = 0;
    int c = year, d = 0;        // ny0: d=1

    if(year < 11000) {
        total32 = hours1kYears[year / 500];
        if(total32) d = (year / 500) * 500;
    } else {
        total32 = hours1kYears[(sizeof(hours1kYears)/sizeof(hours1kYears[0]))-1];
        d = ((sizeof(hours1kYears)/sizeof(hours1kYears[0]))-1) * 500;
    }

    while(c-- > d) {
        total32 += (isLeapYear(c) ? (8760+24) : 8760);
    }
    total32 += (mon_yday[isLeapYear(year) ? 1 : 0][month - 1] * 24);
    total32 += (day - 1) * 24;
    total32 += hour;
    total64 = (uint64_t)total32 * 60;
    total64 += minute;
    return total64;
}
#else
uint64_t dateToMins(int year, int month, int day, int hour, int minute)
{
    uint64_t total64 = 0;
    uint32_t total32 = 0;
    int c = year, d = 0;        // ny0: d=1

    if(year < 11000) {
        total32 = hours1kYears[year / 100];
        if(total32) d = (year / 100) * 100;
    } else {
        total32 = hours1kYears[(sizeof(hours1kYears)/sizeof(hours1kYears[0]))-1];
        d = ((sizeof(hours1kYears)/sizeof(hours1kYears[0]))-1) * 100;
    }

    if(c < jCentStart || c > jCentEnd) {
        while(c-- > d) {
            total32 += (isLeapYear(c) ? (8760+24) : 8760);
        }
        total32 += (mon_yday[isLeapYear(year) ? 1 : 0][month - 1] * 24);
        total32 += (day - 1) * 24;
    } else {
        while(c-- > d) {
            total32 += ((c == jSwitchYear) ? jSwitchYrHrs : (isLeapYear(c) ? (8760+24) : 8760));
        }
        if(year == jSwitchYear) {
            total32 += (mon_yday_jSwitch[month - 1] * 24);
            if(month == jSwitchMon) {
                if(day <= jSwitchDay) {
                    total32 += (day - 1) * 24;
                } else if(day > jSwitchDay + jSwitchSkipD) {
                    total32 += (day - jSwitchSkipD - 1) * 24;
                } else {
                    Serial.printf("Bad date!\n");
                }
            } else {
                total32 += (day - 1) * 24;
            }
        } else {
            total32 += (mon_yday[isLeapYear(year) ? 1 : 0][month - 1] * 24);
            total32 += (day - 1) * 24;
        }
    }

    total32 += hour;
    total64 = (uint64_t)total32 * 60;
    total64 += minute;
    return total64;
}
#endif

/*
 *  Convert "minutes since 1/1/0 0:0" into date
 */
#ifndef TC_JULIAN_CAL
void minsToDate(uint64_t total64, int& year, int& month, int& day, int& hour, int& minute)
{
    int c = 0, d = (sizeof(mins1kYears)/sizeof(mins1kYears[0]))-1;  // ny0: c=1
    int temp;
    uint32_t total32;

    year = 0;             // ny0: 1
    month = day = 1;
    hour = minute = 0;

    while(d >= 0) {
        if(total64 > mins1kYears[d]) break;
        d--;
    }
    if(d > 0) {
        total64 -= mins1kYears[d];
        c = year = d * 500;
    }

    total32 = total64;

    while(1) {
        temp = isLeapYear(c++) ? ((8760+24)*60) : (8760*60);
        if(total32 < temp) break;
        year++;
        total32 -= temp;
    }

    c = 1;
    temp = isLeapYear(year) ? 1 : 0;
    while(c < 12) {
        if(total32 < (mon_ydayt24t60[temp][c])) break;
        c++;
    }
    month = c;
    total32 -= (mon_ydayt24t60[temp][c-1]);

    temp = total32 / (24*60);
    day = temp + 1;
    total32 -= (temp * (24*60));

    temp = total32 / 60;
    hour = temp;

    minute = total32 - (temp * 60);
}
#else
void minsToDate(uint64_t total64, int& year, int& month, int& day, int& hour, int& minute)
{
    int c = 0, d = (sizeof(mins1kYears)/sizeof(mins1kYears[0]));    // ny0: c=1
    int temp;
    uint32_t total32;
    
    year = 0;             // ny0: 1
    month = day = 1;
    hour = minute = 0;
  
    d = (total64 < mins1kYears[d/2]) ? d / 2 : d - 1;

    while(d >= 0) {
        if(total64 > mins1kYears[d]) break;
        d--;
    }
    if(d > 0) {
        total64 -= mins1kYears[d];
        c = year = d * 100;
    }
    
    total32 = total64;

    if(c < jCentStart || c > jCentEnd) {
        while(1) {
            temp = isLeapYear(c++) ? ((8760+24)*60) : (8760*60);
            if(total32 < temp) break;
            year++;
            total32 -= temp;
        }
    } else {
        while(1) {
            temp = ((c == jSwitchYear) ? jSwitchYrHrs : (isLeapYear(c) ? (8760+24) : 8760)) * 60;
            if(total32 < temp) break;
            c++;
            year++;
            total32 -= temp;
        }
    }

    c = 1;
    if(year == jSwitchYear) {
        while(c < 12) {
            if(total32 < (mon_ydayt24t60J[c])) break;
            c++;
        }
        month = c;
        total32 -= (mon_ydayt24t60J[c-1]);
  
        temp = total32 / (24*60);
        day = temp + 1;
        if(month == jSwitchMon && day > jSwitchDay) {
            day += jSwitchSkipD;
        }
    } else {      
        temp = isLeapYear(year) ? 1 : 0;
        while(c < 12) {
            if(total32 < (mon_ydayt24t60[temp][c])) break;
            c++;
        }
        month = c;
        total32 -= (mon_ydayt24t60[temp][c-1]);

        temp = total32 / (24*60);
        day = temp + 1;
    }
    total32 -= (temp * (24*60));
    
    temp = total32 / 60;
    hour = temp;

    minute = total32 - (temp * 60);
}
#endif

uint32_t getHrs1KYrs(int index)
{
    return hours1kYears[index*2];
}

#ifdef TC_JULIAN_CAL
static void calcJulianData()
{
    int l = isLeapYear(jSwitchYear) ? 1 : 0;
  
    for(int i = 0; i < 13; i++) {
        mon_yday_jSwitch[i] = mon_yday[l][i];
    }
    for(int i = jSwitchMon; i < 13; i++) {
        mon_yday_jSwitch[i] -= jSwitchSkipD;
    }
    for(int i = 0; i < 13; i++) {
        mon_ydayt24t60J[i] = mon_yday_jSwitch[i] * 24 * 60;
    }
    
    jSwitchYrHrs = (l ? (8760+24) : 8760) - jSwitchSkipH;

    jSwitchHash = (jSwitchYear << 16) | (jSwitchMon << 8) | jSwitchDay;
} 

void correctNonExistingDate(int year, int month, int& day)
{
  if(year == jSwitchYear && month == jSwitchMon) {
      if((day > jSwitchDay) && (day <= jSwitchDay + jSwitchSkipD)) {
          day = jSwitchDay + jSwitchSkipD + 1;
      }
  }
}
#endif

/*
 * Return RTC-fit year & offs for given real year
 */
void correctYr4RTC(uint16_t& year, int16_t& offs)
{
    offs = 0;
    if(year >= 2000 && year <= 2098) return;
    
    if(isLeapYear(year)) {
        offs = 2000 - year;
        year = 2000;
    } else {
        offs = 2001 - year;
        year = 2001;
    }
}

/**************************************************************
 ***                                                        ***
 ***               Timezone and DST handling                ***
 ***                                                        ***
 **************************************************************/

/*
 * Parse integer
 */
static char *parseInt(char *t, int& it)
{
    bool isNeg = false;
    it = 0;
    
    if(*t == '-') {
        t++;
        isNeg = true;
    } else if(*t == '+') {
        t++;
    }
    
    if(*t < '0' || *t > '9') return NULL;
    
    while(*t >= '0' && *t <= '9') {
        it *= 10;
        it += (*t++ - '0');
    }
    if(isNeg) it *= -1;

    return t;
}

/*
 * Parse DST parts of TZ string
 */
static char *parseDST(char *t, int& DSTyear, int& DSTmonth, int& DSTday, int& DSThour, int& DSTmin, int currYear, int correction)
{
    char *u;
    int it, tw, dow;

    DSTyear = currYear;
    DSTmin = 0;
    
    if(*t == 'M') {
        t++;
        u = parseInt(t, it);
        if(!u) return NULL;
        if(it >= 1 && it <= 12) DSTmonth = it;
        else                    return NULL;
            
        t = u;
        if(*t++ != '.') return NULL;
        
        u = parseInt(t, it);
        if(!u) return NULL;
        if(it < 1 || it > 5) return NULL;
        tw = it;
        
        t = u;        
        if(*t++ != '.') return NULL;
        
        u = parseInt(t, it);
        if(!u) return NULL;
        if(it < 0 || it > 6) return NULL;

        t = u;
        
        // it = weekday (0=Su), tw = week (1,2,3,4=nth week; 5=last)
        dow = dayOfWeek(1, DSTmonth, currYear);
        if(dow == 0) dow = 7;
        DSTday = (it+1) - dow;
        if(DSTday < 1) DSTday += 7;
        while(--tw) {
             DSTday += 7;
        }
        if(DSTday > daysInMonth(DSTmonth, currYear)) DSTday -= 7;

    } else if(*t == 'J') {

        t++;
        u = parseInt(t, it);
        if(!u) return NULL;

        if(it < 1 || it > 365) return NULL;
        
        t = u;

        DSTmonth = 0;
        while(it > monthDays[DSTmonth]) {
            it -= monthDays[DSTmonth++];
        }
        DSTmonth++;
        DSTday = it;
      
    } else if(*t >= '0' && *t <= '9') {

        u = parseInt(t, it);
        if(!u) return NULL;

        if(it < 0 || it > 365) return NULL;
        if((it > 364) && (!isLeapYear(currYear))) return NULL;
        
        t = u;

        it++;
        DSTmonth = 1;
        while(it > daysInMonth(DSTmonth, currYear)) {
            it -= daysInMonth(DSTmonth, currYear);
            DSTmonth++;
        }
        DSTday = it;
      
    } else return NULL;

    if(*t == '/') {
        t++;
        u = parseInt(t, it);
        if(!u) return NULL;
        
        t = u;
        if(it >= -167 && it <= 167) DSThour = it;
        else return NULL;
        
        if(*t == ':') {
            t++;
            u = parseInt(t, it);
            if(!u) return NULL;
            
            t = u;
            if(it >= 0 && it <= 59) DSTmin = it;
            else return NULL;
            
            if(*t == ':') {
                t++;
                u = parseInt(t, it);
                if(!u) return NULL;
                t = u;
            }
        }
    } else {
        DSThour = 2;    
    }

    // Correction used for converting DST-end to non-DST
    if(correction > 0) {
        DSTmin -= correction;
        while(DSTmin < 0) {
            DSTmin += 60;
            DSThour--;
        }
    } else if(correction < 0) {
        DSTmin -= correction;
        while(DSTmin > 59) {
            DSTmin -= 60;
            DSThour++;
        }
    }

    if(DSThour > 23) {
        while(DSThour > 23) {
            DSTday++;
            DSThour -= 24;
        }
        while(DSTday > daysInMonth(DSTmonth, DSTyear)) {
            DSTday -= daysInMonth(DSTmonth, DSTyear);
            DSTmonth++;
            if(DSTmonth > 12) {
                DSTyear++;
                DSTmonth = 1;
            }
        }
    } else if(DSThour < 0) {
        while(DSThour < 0) {
            DSTday--;
            DSThour += 24;
        }
        while(DSTday < 1) {
            DSTmonth--;
            if(DSTmonth < 1) {
                DSTyear--;
                DSTmonth = 12;
            }
            DSTday += daysInMonth(DSTmonth, DSTyear);
        }
    }
    
    return t;
}

/*
 * Convert date into minutes since 1/1 00:00 of given year
 */
int mins2Date(int year, int month, int day, int hour, int mins)
{
    return ((((mon_yday[isLeapYear(year) ? 1 : 0][month - 1] + (day - 1)) * 24) + hour) * 60) + mins;
}

/*
 * Parse TZ string and setup DST data
 * 
 * If TZ-part is bad, always returns FALSE
 * If DST-part is bad, only returns FALSE once
 * (DST-part ignored if bad in later calls)
 */
bool parseTZ(int index, int currYear, bool doparseDST)
{
    char *tz, *t, *u, *v;
    int diffNorm = 0;
    int diffDST = 0;
    int it;
    int DSTonYear, DSTonMonth, DSTonDay, DSTonHour, DSTonMinute;
    int DSToffYear, DSToffMonth, DSToffDay, DSToffHour, DSToffMinute;

    switch(index) {
    case 0: tz = settings.timeZone;     break;
    case 1: tz = settings.timeZoneDest; break;
    case 2: tz = settings.timeZoneDep;  break;
    default:
        return false;
    }

    couldDST[index] = false;
    tzForYear[index] = 0;
    if(!tzDSTpart[index]) {
        tzDiffGMT[index] = tzDiffGMTDST[index] = 0;
    }

    // 0) Basic validity check

    if(*tz == 0) {                                    // Empty string. OK, don't use TZ. So be it.
        tzHasDST[index] = 0;
        return true;
    }

    if(tzIsValid[index] < 1) {

        // If previously determined to be invalid, bail.
        if(!tzIsValid[index]) return false;

        // Set TZ to "invalid" until verified
        tzIsValid[index] = 0;

        t = tz;
        while((t = strchr(t, '>'))) { t++; diffNorm++; }
        t = tz;
        while((t = strchr(t, '<'))) { t++; diffDST++; }
        if(diffNorm != diffDST) return false;         // Uneven < and >, string is bad.
    }

    // 1) Find difference between nonDST and DST time

    if(!tzDSTpart[index]) {

        diffNorm = diffDST = 0;

        // a. Skip TZ name and parse GMT-diff
        t = tz;
        if(*t == '<') {
           t = strchr(t, '>');
           if(!t) return false;                       // if <, but no >, string is bad. Bad TZ.
           t++;
        } else {
           while(*t && *t != '-' && (*t < '0' || *t > '9')) {
              if(*t == ',') return false;
              t++;
           }
        }
        
        // t = start of diff to GMT
        if(*t != '-' && *t != '+' && (*t < '0' || *t > '9'))
            return false;                             // No numerical difference after name -> bad string. Bad TZ.
    
        t = parseInt(t, it);
        if(it >= -24 && it <= 24) diffNorm = it * 60;
        else                      return false;       // Bad hr difference. No DST.
        
        if(*t == ':') {
            t++;
            u = parseInt(t, it);
            if(!u) return false;                      // No number following ":". Bad string. Bad TZ.
            t = u;
            if(it >= 0 && it <= 59) {
                if(diffNorm < 0)  diffNorm -= it;
                else              diffNorm += it;
            } else return false;                      // Bad min difference. Bad TZ.
            if(*t == ':') {
                t++;
                u = parseInt(t, it);
                if(u) t = u;
                // Ignore seconds
            }
        }
        
        // b. Skip DST TZ name and parse GMT-diff
        
        if(*t == '<') {
           t = strchr(t, '>');
           if(!t) return false;                       // if <, but no >, string is bad. Bad TZ.
           t++;
        } else {
           while(*t && *t != ',' && *t != '-' && (*t < '0' || *t > '9'))
              t++;
        }
        
        // t = assumed start of DST-diff to GMT
        if(*t == 0) {
            tzDiff[index] = 0;
        } else if(*t != '-' && *t != '+' && (*t < '0' || *t > '9')) {
            tzDiff[index] = 60;                       // No numerical difference after name -> Assume 1 hr
        } else {
            t = parseInt(t, it);
            if(it >= -24 && it <= 24) diffDST = it * 60;
            else                      return false;   // Bad hr difference. Bad TZ.
            if(*t == ':') {
                t++;
                u = parseInt(t, it);
                if(!u) return false;                  // No number following ":". Bad TZ.
                t = u;
                if(it >= 0 && it <= 59) {
                    if(diffDST < 0)  diffDST -= it;
                    else             diffDST += it;
                } else return false;                  // Bad min difference. Bad TZ.
                if(*t == ':') {
                    t++;
                    u = parseInt(t, it);
                    if(u) t = u;
                    // Ignore seconds
                }
            }
            tzDiff[index] = -(diffDST - diffNorm); //abs(diffDST - diffNorm);
        }
    
        tzDiffGMT[index] = diffNorm;
        tzDiffGMTDST[index] = tzDiffGMT[index] - tzDiff[index];
    
        tzDSTpart[index] = t;

        tzIsValid[index] = 1;   // TZ is valid

    } else {

        t = tzDSTpart[index];
      
    }

    if(!tzHasDST[index] || !doparseDST) {
        return true;
    }
    
    if(*t == 0 || *t != ',') {                        // No DST definition. No DST.
        tzHasDST[index] = 0;
        return true;
    }

    t++;

    tzForYear[index] = currYear;

    // Set to "no DST" until verified valid
    tzHasDST[index] = 0;

    // 2) parse DST start

    v = t;
    u = parseDST(t, DSTonYear, DSTonMonth, DSTonDay, DSTonHour, DSTonMinute, currYear, 0);
    if(!u) return false;

    // If start crosses end year (due to hour numbers >= 24), need to calculate 
    // for previous year (which then might be in current year). 
    // The same goes for the other direction vice versa.
    if(DSTonYear > currYear) {
        u = parseDST(v, DSTonYear, DSTonMonth, DSTonDay, DSTonHour, DSTonMinute, currYear-1, 0);
        if(!u) return false;
        // Trigger check below if still outside of current year
        if(DSTonYear != currYear) DSTonYear = currYear + 1;
    } else if(DSTonYear < currYear) {
        u = parseDST(v, DSTonYear, DSTonMonth, DSTonDay, DSTonHour, DSTonMinute, currYear+1, 0);
        if(!u) return false;
        // Trigger check below if still outside of current year
        if(DSTonYear != currYear) DSTonYear = currYear - 1;
    }
    
    t = u;

    if(*t == 0 || *t != ',') return false;          // Have start, but no end. Bad string. No DST.
    t++;

    // 3) parse DST end

    v = t;
    u = parseDST(t, DSToffYear, DSToffMonth, DSToffDay, DSToffHour, DSToffMinute, currYear, tzDiff[index]);
    if(!u) return false;

    // See above
    if(DSToffYear > currYear) {
        u = parseDST(v, DSToffYear, DSToffMonth, DSToffDay, DSToffHour, DSToffMinute, currYear-1, tzDiff[index]);
        if(!u) return false;
        // Trigger check below if still outside of current year
        if(DSToffYear != currYear) DSToffYear = currYear + 1;
    } else if(DSToffYear < currYear) {
        u = parseDST(v, DSToffYear, DSToffMonth, DSToffDay, DSToffHour, DSToffMinute, currYear+1, tzDiff[index]);
        if(!u) return false;
        // Trigger check below if still outside of current year
        if(DSToffYear != currYear) DSToffYear = currYear - 1;
    }

    // 4) Evaluate results

    tzHasDST[index] = 1;  // TZ has valid DST definition

    if((DSToffMonth == DSTonMonth) && (DSToffDay == DSTonDay)) {
        couldDST[index] = false;
        #ifdef TC_DBG
        Serial.printf("parseTZ: (%d) DST not used\n", index);
        #endif
    } else {
        couldDST[index] = true;

        // If start or end still beyond our current year, set to impossible values
        // to allow a valid comparison.
        // Despire our cross-end-check for currYear above, this still can happen!
        // Eg: "CRAZY-3:30<C3ACY>4:56,M1.1.0/-48,M12.5.0/48"
        // For 2023, first calculated start is on 12/30/2022, so we do 2024 above, 
        // but for 2024 start is on 1/5/2024. Nothing in 2023!
        // Likewise 2023's first calculated end is on 1/2/2024, so we do 2022 above,
        // but for 2022, it is on 12/27/2022. Again, outside of our current year!
        // So with this somewhat challenging time zone definition, the entire year 
        // 2023 is DST. Need to set -1/600000 to make comparison right.
        if(DSTonYear < currYear)
            DSTonMins[index] = -1;
        else if(DSTonYear > currYear)
            DSTonMins[index] = 600000;
        else 
            DSTonMins[index] = mins2Date(currYear, DSTonMonth, DSTonDay, DSTonHour, DSTonMinute);
    
        if(DSToffYear < currYear)
            DSToffMins[index] = -1;
        else if(DSToffYear > currYear)
            DSToffMins[index] = 600000;
        else {
            DSToffMins[index] = mins2Date(currYear, DSToffMonth, DSToffDay, DSToffHour, DSToffMinute);
        }

        #ifdef TC_DBG
        Serial.printf("parseTZ: (%d) %d/%d(%d) DST %d-%02d-%02d/%02d:%02d - %d-%02d-%02d/%02d:%02d\n",
                    index,
                    tzDiffGMT[index], tzDiffGMTDST[index], tzDiff[index],
                    DSTonYear, DSTonMonth, DSTonDay, DSTonHour, DSTonMinute,
                    DSToffYear, DSToffMonth, DSToffDay, DSToffHour, DSToffMinute);
        #endif

    }
        
    return true;
}

/*
 * Check if given local date/time is within DST period.
 * Given "Local" is assumed be be non-DST.
 */
int timeIsDST(int index, int year, int month, int day, int hour, int mins, int& currTimeMins)
{
    currTimeMins = mins2Date(year, month, day, hour, mins);

    // DSTxxMins is in non-DST local time (End corrected from DST to non-DST in parseTZ)
    if(DSTonMins[index] < DSToffMins[index]) {
        if((currTimeMins >= DSTonMins[index]) && (currTimeMins < DSToffMins[index]))
            return 1;
        else 
            return 0;
    } else {
        if((currTimeMins >= DSToffMins[index]) && (currTimeMins < DSTonMins[index]))
            return 0;
        else
            return 1;
    }
}

/*
 * Conversion from/to UTC
 */
static void convTime(int diff, int& y, int& m, int& d, int& h, int& mm)
{
    
    if(diff > 0) {

        mm -= diff;
        while(mm < 0) {
            mm += 60;
            h--;
        }
        while(h < 0) {
            h += 24;
            d--;
        }
        while(d < 1) {
            m--;
            if(m < 1) {
                m = 12;
                y--;
                if(y < 1) y = 9999;
            }
            d += daysInMonth(m, y);
        }
        
    } else if(diff < 0) {

        mm -= diff;
        while(mm > 59) {
            mm -= 60;
            h++;
        }
        while(h > 23) {
            h -= 24;
            d++;
        }
        while(d > daysInMonth(m, y)) {
            d -= daysInMonth(m, y);
            m++;
            if(m > 12) {
                m = 1;
                y++;
                if(y > 9999) y = 1;
            }
        }

    }
}

void UTCtoLocal(DateTime &dtu, DateTime& dtl, int index)
{
    int y  = dtu.year();
    int m  = dtu.month();
    int d  = dtu.day();
    int h  = dtu.hour();
    int mm = dtu.minute();
    int y2 = y, m2 = m, d2 = d, h2 = h, mm2 = mm;
    int ctm = 0;

    #ifdef TC_DBG
    if(dtu.second() == 30) {
        Serial.printf("UTCtoLocal: (%d) UTC:   %d-%d-%d %d:%d\n", index, y, m, d, h, mm, dtu.second());
    }
    #endif

    convTime(tzDiffGMT[index], y, m, d, h, mm);
    
    if(couldDST[index]) {
        if(tzForYear[index] != y) {
            parseTZ(index, y);
        }
        if(timeIsDST(index, y, m, d, h, mm, ctm)) {
            y = y2; m = m2; d = d2; h = h2; mm = mm2;
            convTime(tzDiffGMTDST[index], y, m, d, h, mm);
        }
    }

    dtl.set(y, m, d, h, mm, dtu.second());

    #ifdef TC_DBG
    if(dtu.second() == 30) {
        Serial.printf("UTCtoLocal: (%d) Local: %d-%d-%d %d:%d\n", index, y, m, d, h, mm, dtu.second());
    }
    #endif
}

void LocalToUTC(int& ny, int& nm, int& nd, int& nh, int& nmm, int index)
{
    int ctm;
    int diff = tzDiffGMT[index];

    #ifdef TC_DBG
    Serial.printf("LocalToUTC: (%d) Local: %d-%d-%d %d:%d\n", index, ny, nm, nd, nh, nmm);
    #endif

    if(couldDST[index]) {
        if(timeIsDST(index, ny, nm, nd, nh, nmm, ctm)) {
            diff = tzDiffGMTDST[index];
        }
    }

    convTime(-diff, ny, nm, nd, nh, nmm);

    #ifdef TC_DBG
    Serial.printf("LocalToUTC: (%d) UTC:   %d-%d-%d %d:%d\n", index, ny, nm, nd, nh, nmm);
    #endif
}    

/**************************************************************
 ***                                                        ***
 ***                       Native NTP                       ***
 ***                                                        ***
 **************************************************************/

static void ntp_setup()
{
    myUDP = &ntpUDP;
    myUDP->begin(NTP_DEFAULT_LOCAL_PORT);
    NTPfailCount = 0;
}

void ntp_loop()
{
    if(NTPPacketDue) {
        NTPCheckPacket();
    }
    if(!NTPPacketDue) {
        // If WiFi status changed, trigger immediately
        if(!NTPWiFiUp && (WiFi.status() == WL_CONNECTED)) {
            NTPUpdateNow = 0;
        }
        if((!NTPUpdateNow) || (millis() - NTPUpdateNow > 60000)) {
            NTPTriggerUpdate();
        }
    }
}

// Short version for inside delay loops
// Fetches pending packet, if available
void ntp_short_loop()
{
    if(NTPPacketDue) {
        NTPCheckPacket();
    }
}

// Send a new NTP request
static bool NTPTriggerUpdate()
{
    NTPPacketDue = false;

    NTPUpdateNow = millis();

    if(WiFi.status() != WL_CONNECTED) {
        NTPWiFiUp = false;
        return false;
    }

    NTPWiFiUp = true;

    if(settings.ntpServer[0] == 0) {
        return false;
    }

    // Flush old packets received after timeout
    while(myUDP->parsePacket() != 0) {
        myUDP->flush();
    }

    // Send new packet
    NTPSendPacket();
    NTPTSRQAge = millis();
    
    NTPPacketDue = true;
    
    return true;
}

static void NTPSendPacket()
{   
    memset(NTPUDPBuf, 0, NTP_PACKET_SIZE);

    NTPUDPBuf[0] = 0b11100011; // LI, Version, Mode
    NTPUDPBuf[1] = 0;          // Stratum
    NTPUDPBuf[2] = 6;          // Polling Interval
    NTPUDPBuf[3] = 0xEC;       // Peer Clock Precision

    memcpy(NTPUDPBuf + 12, NTPUDPHD, 4);  // Ref ID, anything

    // Transmit, use as id
    NTPUDPID = (uint32_t)millis();
    SET32(NTPUDPBuf, 40, NTPUDPID);

    myUDP->beginPacket(settings.ntpServer, 123);
    myUDP->write(NTPUDPBuf, NTP_PACKET_SIZE);
    myUDP->endPacket();
}

// Check for pending packet and parse it
static void NTPCheckPacket()
{
    unsigned long mymillis = millis();
    
    int psize = myUDP->parsePacket();
    if(!psize) {
        if((mymillis - NTPTSRQAge) > 10000) {
            // Packet timed out
            NTPPacketDue = false;
            // Immediately trigger new request for
            // the first 10 timeouts, after that
            // the new request is only triggered
            // a minute later as per ntp_loop().
            if(NTPfailCount < 10) {
                NTPfailCount++;
                NTPUpdateNow = 0;
            }
        }
        return;
    }

    NTPfailCount = 0;
    
    myUDP->read(NTPUDPBuf, NTP_PACKET_SIZE);

    // Basic validity check
    if((NTPUDPBuf[0] & 0x3f) != 0x24) return;

    //if(*((uint32_t *)(NTPUDPBuf + 24)) != NTPUDPID) {
    if(GET32(NTPUDPBuf, 24) != NTPUDPID) {
        #ifdef TC_DBG
        Serial.println("NTPCheckPacket: Bad packet ID (outdated packet?)");
        #endif
        return;
    }

    // If it's our expected packet, no other is due for now
    NTPPacketDue = false;

    // Baseline for round-trip correction
    NTPTSAge = mymillis - ((mymillis - NTPTSRQAge) / 2);

    // Evaluate data
    uint64_t secsSince1900 = ((uint32_t)NTPUDPBuf[40] << 24) |
                             ((uint32_t)NTPUDPBuf[41] << 16) |
                             ((uint32_t)NTPUDPBuf[42] <<  8) |
                             ((uint32_t)NTPUDPBuf[43]);

    uint32_t fractSec = ((uint32_t)NTPUDPBuf[44] << 24) |
                        ((uint32_t)NTPUDPBuf[45] << 16) |
                        ((uint32_t)NTPUDPBuf[46] <<  8) |
                        ((uint32_t)NTPUDPBuf[47]);
                        
    // Correct era
    if(secsSince1900 < (SECS1900_1970 + TCEPOCH_SECS)) {
        secsSince1900 |= 0x100000000ULL;
    }           

    // Calculate seconds since 1/1/TCEPOCH (without round-trip correction)
    NTPsecsSinceTCepoch = secsSince1900 - (SECS1900_1970 + TCEPOCH_SECS);

    // Convert fraction into ms
    NTPmsSinceSecond = (uint32_t)(((uint64_t)fractSec * 1000ULL) >> 32);
}

static bool NTPHaveTime()
{
    return NTPsecsSinceTCepoch ? true : false;
}

// Get seconds since 1/1/TCEPOCH including round-trip correction
static uint32_t NTPGetCurrSecsSinceTCepoch()
{
    return NTPsecsSinceTCepoch + ((NTPmsSinceSecond + (millis() - NTPTSAge)) / 1000UL);
}

static bool NTPHaveCurrentTime()
{
    // Have no time if no time stamp received, or stamp is older than 10 mins
    if((!NTPsecsSinceTCepoch) || ((millis() - NTPTSAge) > 10*60*1000)) 
        return false;

    return true;
}

// Get UTC time from NTP response
static bool NTPGetUTC(int& year, int& month, int& day, int& hour, int& minute, int& second)
{
    uint32_t temp, c;

    // Fail if no time received, or stamp is older than 10 mins
    if(!NTPHaveCurrentTime()) return false;
    
    uint32_t secsSinceTCepoch = NTPGetCurrSecsSinceTCepoch();
    
    second = secsSinceTCepoch % 60;

    // Calculate minutes since 1/1/TCEpoch
    uint32_t total32 = (secsSinceTCepoch / 60);

    // Calculate current date
    year = c = TCEPOCH;
    while(1) {
        temp = isLeapYear(c++) ? ((8760+24)*60) : (8760*60);
        if(total32 < temp) break;
        year++;
        total32 -= temp;
    }

    c = 1;
    temp = isLeapYear(year) ? 1 : 0;
    while(c < 12) {
        if(total32 < (mon_ydayt24t60[temp][c])) break;
        c++;
    }
    month = c;
    total32 -= (mon_ydayt24t60[temp][c-1]);

    day = total32 / (24*60);
    total32 -= (day * (24*60));
    day++;

    hour = total32 / 60;

    minute = total32 - (hour * 60);

    return true;
}


/**************************************************************
 ***                                                        ***
 ***     Basic Telematics Transmission Framework (BTTFN)    ***
 ***                                                        ***
 **************************************************************/

int bttfnNumClients()
{
    int i;
    
    for(i = 0; i < BTTFN_MAX_CLIENTS; i++) {
        if(!bttfnClient[i].IP32)
            break;
    }
    return i;
}

bool bttfnGetClientInfo(int c, char **id, uint8_t **ip, uint8_t *type)
{
    if(c > BTTFN_MAX_CLIENTS - 1)
        return false;
        
    if(!bttfnClient[c].IP32)
        return false;
        
    *id = bttfnClient[c].ID;
    *ip = bttfnClient[c].IP;

    *type = bttfnClient[c].Type;

    return true;
}

static uint32_t storeBTTFNClient(uint32_t ip, uint8_t *buf, uint8_t type, uint8_t MCSupport)
{
    _bttfnClient *newClient;
    int i;

    // Check if already in list, search for free slot
    for(i = 0; i < BTTFN_MAX_CLIENTS; i++) {
        newClient = &bttfnClient[i];
        if(ip == newClient->IP32)
            goto stcl_ipIdentical;
        else if(!newClient->IP32)
            goto stcl_copyIP;
    }

    // Bail if no slot available
    return 0;

stcl_copyIP:

    newClient->IP32 = ip;

stcl_ipIdentical:

    bttfnHaveClients = true;

    memcpy(newClient->ID, buf + 10, 13);
    //newClient->ID[13] = 0;  // is always 0 already

    newClient->Type = type;
    newClient->ALIVE = millis();
    
    if((newClient->MC = MCSupport)) {
        bttfnNotAllSupportMC = 1;
    } else {
        bttfnAtLeastOneMC = 1;
    }
    
    #ifdef TC_HAVE_REMOTE
    memcpy(&newClient->RemID, &buf[35], 4);
    
    return newClient->RemID;
    
    #else
    
    return 0;
    
    #endif
}

static void bttfn_expire_clients()
{
    bool didST = false;
    int k, numClients = 0;
    unsigned long now = millis();

    if(now - bttfnlastExpire < 57*1000)
        return;
        
    bttfnlastExpire = now;
    
    for(int i = 0; i < BTTFN_MAX_CLIENTS; i++) {
        if(bttfnClient[i].IP32) {
            numClients++;
            if(millis() - bttfnClient[i].ALIVE > 5*60*1000) {
                bttfnClient[i].IP32 = 0;
                #ifdef TC_HAVE_REMOTE
                #ifdef TC_DBG
                Serial.printf("Expiring device type %d\n", bttfnClient[i].Type);
                #endif
                if(bttfnClient[i].Type == BTTFN_TYPE_REMOTE) {
                    #ifdef TC_DBG
                    Serial.printf("Expiring remote id: %u %u\n", bttfnClient[i].RemID, registeredRemID);
                    #endif
                    if(bttfnClient[i].RemID == registeredRemID) {
                        removeRemote();
                    }
                } else if(registeredRemKPID && (bttfnClient[i].RemID == registeredRemKPID)) {
                    #ifdef TC_DBG
                    Serial.printf("Expiring remote KP id: %u %u\n", bttfnClient[i].RemID, registeredRemKPID);
                    #endif
                    removeKPRemote();
                }
                #endif  // TC_HAVE_REMOTE
                numClients--;
                didST = true;
            }
        }
    }

    bttfnHaveClients = (numClients > 0);

    if(!didST)
        return;

    for(int j = 0; j < BTTFN_MAX_CLIENTS - 1; j++) {
        if(!bttfnClient[j].IP32) {
            for(k = j + 1; k < BTTFN_MAX_CLIENTS; k++) {
                if(bttfnClient[k].IP32) {
                    memcpy(&bttfnClient[j], &bttfnClient[k], (BTTFN_MAX_CLIENTS - k) * sizeof(_bttfnClient));
                    for(int l = BTTFN_MAX_CLIENTS - (k - j); l < BTTFN_MAX_CLIENTS; l++) bttfnClient[l].IP32 = 0;
                    break;
                }
            }
            if(k == BTTFN_MAX_CLIENTS) break;
        }
    }

    bttfnNotAllSupportMC = bttfnAtLeastOneMC = 0;
    for(int i = 0; i < BTTFN_MAX_CLIENTS; i++) {
        if(bttfnClient[i].IP32) {
            bttfnNotAllSupportMC |= bttfnClient[i].MC;
            bttfnAtLeastOneMC    |= (bttfnClient[i].MC ^ 1);
        } else
            break;
    }

    if(!bttfnHaveClients) wifiRestartPSTimer();
}

#ifdef TC_HAVE_REMOTE
static bool checkToPlayRemoteSnd()
{
    if(timeTravelP0 || timeTravelP1 || timeTravelRE || menuActive) return false;
    if(!checkMP3Done()) return false;
    if(presentTime.getNightMode()) return false;
    return true;
}

static void bttfnMakeRemoteSpeedMaster(bool doit, bool isPwrMaster)
{
    bool pwrChg = false;
    
    // Powermaster: Anytime.

    if(bttfnRemPwrMaster != isPwrMaster) {
        if(!isPwrMaster) {
            #ifdef TC_HAVEMQTT
            if(MQTTPwrMaster) {
                isFPBKeyPressed = shadowMQTTFPBKeyPressed;
            } else {
            #endif
                if(waitForFakePowerButton) {
                    isFPBKeyPressed = shadowFPBKeyPressed;
                } else {
                    restoreFakePwr = true;
                    isFPBKeyPressed = true;
                }
            #ifdef TC_HAVEMQTT
            }
            #endif
            isFPBKeyChange = true;
        }
        bttfnRemPwrMaster = isPwrMaster;
    }
    if(bttfnRemPwrMaster) {
        if(doit != FPBUnitIsOn) {
            isFPBKeyChange = true;
            isFPBKeyPressed = doit;
            pwrChg = true;
        }
    }

    // Speedmaster: Only after finishing bootStrap.
    
    if((bttfnRemoteSpeedMaster == doit) || bootStrap) {
        return;
    }

    bttfnOldRemCurSpd = 255;
    
    if((bttfnRemoteSpeedMaster = doit)) {
      
        // Remote is now speed master:
        #ifdef TC_DBG
        Serial.println("Remote is now master");
        #endif

        if((!bttfnRemPwrMaster || !pwrChg) && checkToPlayRemoteSnd()) {
            play_file(remoteOnSound, PA_LINEOUT|PA_CHECKNM|PA_ALLOWSD);
        }

        // Disable display of RotEnc/GPS/Temp
        #ifdef TC_HAVE_RE
        bttfnBUseRotEnc = useRotEnc;
        useRotEnc = false;
        bttfnBDispRotEnc = dispRotEnc;
        dispRotEnc = false;
        #endif
        #ifdef TC_HAVEGPS
        bttfnBDispGPSSpeed = dispGPSSpeed;
        dispGPSSpeed = false;
        bttfnBPovGPS2BTTFN = provGPS2BTTFN;   // Need that to provide remote speed to BTTFN clients
        provGPS2BTTFN = false;
        #endif
        #ifdef TC_HAVETEMP
        bttfnBDispTemp = dispTemp;
        dispTemp = false;
        #endif
         
        // Store currently displayed speed
        if(useSpeedoDisplay) {
            bttfnRemCurSpd = (speedo.getOnOff() && (speedo.getSpeed() >= 0)) ? speedo.getSpeed() : 0;
        } else {
            bttfnRemCurSpd = bttfnRemoteSpeed;
        }

        // Beep auto mode: Restart timer
        startBeepTimer();
        
        // Wake up network clients
        send_wakeup_msg();

        // Rest of init done in main loop

    } else {
      
        // Remote no longer speed master:
        // Restore

        #ifdef TC_DBG
        Serial.println("Remote is no longer master");
        #endif

        if((!bttfnRemPwrMaster || !pwrChg) && checkToPlayRemoteSnd()) {
            play_file(remoteOffSound, PA_LINEOUT|PA_CHECKNM|PA_ALLOWSD);
        }
        
        #ifdef TC_HAVE_RE
        useRotEnc = bttfnBUseRotEnc;
        dispRotEnc = bttfnBDispRotEnc;
        #endif
        #ifdef TC_HAVEGPS
        dispGPSSpeed = bttfnBDispGPSSpeed;
        provGPS2BTTFN = bttfnBPovGPS2BTTFN;
        #endif
        #ifdef TC_HAVETEMP
        dispTemp = bttfnBDispTemp;
        #endif

        #ifdef TC_HAVE_RE
        if(useRotEnc) {
            // We MUST reset the encoder; user might have moved
            // it while we had it disabled.
            // If fakeSpeed is -1 at this point, it was in "disabled"
            // position when disabling it.
            if(fakeSpeed < 0) {
                // Reset enc to "disabled" position
                re_init(false);
            } else {
                // Reset enc to curr. speed or "zero" position
                if(dispRotEnc && (speedo.getSpeed() >= 0)) {
                    re_setToSpeed(speedo.getSpeed());
                } else {
                    re_init();
                }
                re_lockTemp();
            }
        }
        #endif
        
        if(useSpeedoDisplay) {
            if(!FPBUnitIsOn) {
                if(!dispGPSSpeed) speedo.off();
            } else if(!dispGPSSpeed && !dispRotEnc) {
                if(speedoAlwaysOn) {
                    dispIdleZero(true);
                } else {
                    speedo.off();
                }
            }
        }
        #ifdef TC_HAVEGPS
        if(dispGPSSpeed) {
            remoteWasMaster = 88;
            lastRemSpdUpd = 0;
            remSpdCatchUpDelay = 0;
        }
        #endif
        #if defined(TC_HAVEGPS) || defined(TC_HAVE_RE)
        displayGPSorRESpeed(true);
        #endif
        
        #ifdef TC_HAVETEMP
        updateTemperature(true);
        if(useSpeedoDisplay) {
            dispTemperature(true);
        }
        #endif
    }
}

void removeRemote()
{
    bttfnMakeRemoteSpeedMaster(false, false);
    bttfnRemStop = false;
    bttfnRemoteSpeed = 0;
    registeredRemID = 0;
    bttfnLastSeq_co = 0;
    bttfnHaveRemote = false;
}

void removeKPRemote()
{
    registeredRemKPID = 0;
    bttfnLastSeq_ky = 0;    // seq cnt starts at 1 after every registration
}

static void bttfn_evalremotecommand(uint32_t seq, uint8_t cmd, uint8_t p1, uint8_t p2)
{
    #ifdef TC_DBG
    Serial.printf("Remote command %d  p1 %d  (seq %d)\n", cmd, p1, seq);
    #endif
            
    switch(cmd) {

    case BTTFN_REMCMD_COMBINED: // Update status and speed from remote
        // Skip outdated packets
        if(seq > bttfnLastSeq_co || seq == 1) {
            bttfnMakeRemoteSpeedMaster(!!(p1 & 0x01), !!(p1 & 0x08));
            bttfnRemStop = !!(p1 & 0x02);
            bttfnRemOffSpd = !!(p1 & 0x10);
            if(p2 > 127) bttfnRemoteSpeed = 0;
            else bttfnRemoteSpeed = p2;  // p2 = speed (0-88)
            if(bttfnRemoteSpeed > 88) bttfnRemoteSpeed = 88;
            #ifdef TC_DBG
            Serial.printf("Remote combined: %d %d\n", bttfnRemoteSpeed, bttfnRemStop);
            #endif
        } else {
            #ifdef TC_DBG
            Serial.printf("Command out of sequence seq:%d last:%d\n", seq, bttfnLastSeq_co);
            #endif
        }
        bttfnLastSeq_co = seq;
        break;

    case BTTFN_REMCMD_BYE:
        // Remote wants to unregister. It should stop sending keep-alives afterwards.
        removeRemote();
        #ifdef TC_DBG
        Serial.printf("Remote unregistered\n");
        #endif
        break;

    case BTTFN_REMCMD_KP_KEY:
        // Skip outdated packets
        if(seq > bttfnLastSeq_ky || seq == 1) {
            injectKeypadKey((char)p1, (int)p2);
        } else {
            #ifdef TC_DBG
            Serial.printf("Command out of sequence seq:%d last:%d\n", seq, bttfnLastSeq_ky);
            #endif
        }
        bttfnLastSeq_ky = seq;
        break;

    case BTTFN_REMCMD_KP_BYE:
        removeKPRemote();
        #ifdef TC_DBG
        Serial.printf("Remote KP unregistered)\n");
        #endif
        break;

    #ifdef TC_DBG
    case BTTFN_REMCMD_PING:
    case BTTFN_REMCMD_KP_PING:
        // Do nothing, command only for registering or keep-alive
        break;
    
    default:
        Serial.printf("Unknown remote command: %d\n", cmd);
    #endif
    }
}
#endif

static void bttfn_setup()
{
    // Prepare hostName hash for discover packets
    hostNameHash = 0;
    unsigned char *s = (unsigned char *)settings.hostName;
    for ( ; *s; ++s) hostNameHash = 37 * hostNameHash + tolower(*s);

    memset(bttfnClient, 0, sizeof(bttfnClient));

    // For testing
    r  = bttfn_unrollPacket;
    
    tcdUDP = &bttfUDP;
    tcdUDP->begin(BTTF_DEFAULT_LOCAL_PORT);

    tcdmcUDP = &bttfmcUDP;
    tcdmcUDP->beginMulticast(IPAddress(224, 0, 0, 224), BTTF_DEFAULT_LOCAL_PORT + 1);
}

bool bttfn_loop()
{
    bool resmc = false;
    
    resmc = bttfn_checkmc();

    int psize = tcdUDP->parsePacket();

    if(!psize) {
        bttfn_expire_clients();
        return false | resmc;
    }
    
    tcdUDP->read(BTTFUDPBuf, BTTF_PACKET_SIZE);

    if(bttfn_handlePacket(BTTFUDPBuf, false)) {
        tcdUDP->beginPacket(tcdUDP->remoteIP(), BTTF_DEFAULT_LOCAL_PORT);
        tcdUDP->write(BTTFUDPBuf, BTTF_PACKET_SIZE);
        tcdUDP->endPacket();
    } else if(!psize) {
        (*r)(BTTFUDPBuf, 0, BTTF_PACKET_SIZE);
    }

    return true;
}

static bool bttfn_checkmc()
{
    int psize = tcdmcUDP->parsePacket();

    if(!psize) {
        return false;
    }
    
    tcdmcUDP->read(BTTFMCBuf, BTTF_PACKET_SIZE);

    #ifdef TC_DBG
    Serial.printf("Received multicast packet from %s\n", tcdmcUDP->remoteIP().toString());
    #endif
    
    if(bttfn_handlePacket(BTTFMCBuf, true)) {
        tcdUDP->beginPacket(tcdmcUDP->remoteIP(), BTTF_DEFAULT_LOCAL_PORT);
        tcdUDP->write(BTTFMCBuf, BTTF_PACKET_SIZE);
        tcdUDP->endPacket();
        #ifdef TC_DBG
        Serial.println("Sent response");
        #endif
    }

    return true;
}

static uint8_t* bttfn_unrollPacket(uint8_t *d, uint32_t m, int b)
{
    b /= 4;
    m += a(m, 7);
    for(uint32_t *dd = (uint32_t *)d; b-- ; dd++, m = m / 2 + a(m, 0)) {
        *dd ^= m;
    }
    return d;
}

static bool bttfn_handlePacket(uint8_t *buf, bool isMC)
{
    uint32_t tip32 = 0;
    uint8_t a = 0, ctype = 0, parm = 0, supportsMC = 0;
    int16_t temp = 0;
    uint32_t receivedRemID;
    
    // Check header
    if(memcmp(buf, BTTFUDPHD, 4))
        return false;

    // Check checksum
    for(int i = 4; i < BTTF_PACKET_SIZE - 1; i++) {
        a += buf[i] ^ 0x55;
    }
    if(buf[BTTF_PACKET_SIZE - 1] != a)
        return false;

    // Check if device supports passive MC
    supportsMC = (buf[4] ^ 0x80) >> 7;
    buf[4] &= 0x0f;
        
    if(buf[4] > BTTFN_VERSION)
        return false;

    // Check if this is a "discover" packet
    if(isMC) {
        if(buf[5] & 0x80) {
            if(GET32(buf, 31) != hostNameHash) {
                return false;
            }
        } else {
            return false;
        }
    }
    
    // Store client ip/id for notifications and keypad menu
    tip32 = isMC ? tcdmcUDP->remoteIP() : tcdUDP->remoteIP();  

    ctype = (uint8_t)buf[10+13];
    
    receivedRemID = storeBTTFNClient(tip32, buf, ctype, supportsMC);

    // Remote time travel?
    if(!isMC && (buf[5] & 0x80)) {
        // Skip if busy
        if(FPBUnitIsOn && !menuActive && !startup &&
           !timeTravelP0 && !timeTravelP1 && !timeTravelRE) {
            isEttKeyPressed = isEttKeyImmediate = true;
            #ifdef TC_DBG
            Serial.printf("BTTFN-wide TT triggered by device type %d\n", buf[10+13]);
            #endif
        }
        buf[5] &= 0x7f;
        // If no data requested, return and don't send response
        if(!buf[5])
            return false;
    }

    // Retrieve (optional) request parameter
    // 0-3: Device type for IP lookup
    // 4-6: For future use
    // 7: Request displayed destination, present, departed times with date/time request
    parm = buf[24];

    // Check if we received a remote command
    #ifdef TC_HAVE_REMOTE
    if(buf[25]) {

        if(!receivedRemID) return false;
      
        if(ctype == BTTFN_TYPE_REMOTE) {
            if(!remoteAllowed) return false;
            if(!registeredRemID) {
                registeredRemID = receivedRemID;
            } else if(registeredRemID != receivedRemID) {
                return false;
            }
            bttfnHaveRemote = true;
        } else if(remoteKPAllowed) {
            if(!registeredRemKPID) {
                registeredRemKPID = receivedRemID;
            } else if(registeredRemKPID != receivedRemID) {
                return false;
            }
        } else
            return false;
    
        // buf[6]-buf[9]: Sequence of packets: 
        // Remote sends separate seq counters for each command type.
        // If seq is < previous, packet is skipped.
        uint32_t seq = GET32(buf, 6);

        // Eval command from remote: 
        // 25: Command code
        // 26, 27: parameters
        bttfn_evalremotecommand(seq, buf[25], buf[26], buf[27]);

        // Send no response
        return false;
        
    } else {
    #endif

        // Eval query and build reply into buf

        // Add response marker to version byte
        buf[4] |= 0x80;

        // Clear, but leave serial# (buf + 6-9) untouched
        memset(buf + 10, 0, BTTF_PACKET_SIZE - 10);
    
        if(buf[5] & 0x01) {    // date/time
            memcpy(&buf[10], bttfnDateBuf, sizeof(bttfnDateBuf));
            if(presentTime.get1224()) buf[17] |= 0x80;
            if(parm & 0x80) {
                uint8_t ov = 0;
                destinationTime.getCompressed(&buf[36], ov);
                ov <<= 2;
                presentTime.getCompressed(&buf[32], ov);
                ov <<= 2;
                departedTime.getCompressed(&buf[40], ov);
                buf[44] = ov;
            }
        }
        if(buf[5] & 0x02) {    // speed  (-1 if unavailable)
            temp = -1;         // (Client is supposed to support MC-notifications instead)
            #ifdef TC_HAVE_REMOTE
            if(bttfnRemoteSpeedMaster) {
                // bttfnRemCurSpd is P0-speed during P0, see below for reason
                temp = bttfnRemCurSpd;
                buf[26] |= 0x20;       // Signal that speed is from Remote
            } else {
            #endif
                #ifdef TC_HAVEGPS
                if(useGPS && provGPS2BTTFN) {
                    // Why "&& provGPS2BTTFN"?
                    // Because: If stationary user uses GPS for time only, speed will
                    // be 0 permanently, and rotary encoder never gets a chance.
                    // In P0, we transmit a fake speed since the props follow speed
                    // after triggering a below-88 TT (which at first only triggers a
                    // wakeup) until the ETTO-timed actual TIMETRAVEL signal
                    temp = timeTravelP0 ? timeTravelP0Speed : myGPS.getSpeed();
                } else {                        
                #endif
                    #ifdef TC_HAVE_RE
                    if(useRotEnc && FPBUnitIsOn) {  // fakespeed only valid if FP on
                        // fakeSpeed is P0-speed during P0, see above for reason
                        temp = fakeSpeed;
                        buf[26] |= 0x80;   // Signal that speed is from RotEnc
                    }
                    #endif
                #ifdef TC_HAVEGPS
                }
                #endif
            #ifdef TC_HAVE_REMOTE
            }
            #endif
            buf[18] = (uint16_t)temp & 0xff;
            buf[19] = (uint16_t)temp >> 8;
        }
        if(buf[5] & 0x04) {    // temperature * 100 (-32768 if unavailable)
            temp = -32768;
            #ifdef TC_HAVETEMP
            if(useTemp) {
                float tempf = tempSens.readLastTemp();
                if(!isnan(tempf)) {
                    temp = (int16_t)(tempf * 100.0);
                }
            }
            #endif
            buf[20] = (uint16_t)temp & 0xff;
            buf[21] = (uint16_t)temp >> 8;
            #ifdef TC_HAVETEMP
            if(tempUnit) buf[26] |= 0x40;   // Signal temp unit (0=F, 1=C)
            #endif
        }
        if(buf[5] & 0x08) {    // lux (-1 if unavailable)
            int32_t temp32 = -1;
            #ifdef TC_HAVELIGHT
            if(useLight) {
                temp32 = lightSens.readLux();
            }
            #endif
            SET32(buf, 22, temp32);
        }
        if(buf[5] & 0x10) {    // Status flags
            a = 0;
            if(presentTime.getNightMode()) a |= 0x01; // bit 0: Night mode (0: off, 1: on)
            if(!FPBUnitIsOn)               a |= 0x02; // bit 1: Fake power (0: on,  1: off)
            #ifdef TC_HAVE_REMOTE
            if(remoteAllowed)              a |= 0x04; // bit 2: Remote controlling allowed (1) or disabled (0)
            if(remoteKPAllowed)            a |= 0x08; // bit 3: Remote Keypad controlling allowed (1) or disabled (0)
            #endif
            if(menuActive)                 a |= 0x10; // bit 4: TCD is busy (eg. not ready for time travel)
            // bit 5 is for "speed from remote", see above
            // bit 6 used for temp unit, see above
            // bit 7 used for "speed from RotEnc", see above
            buf[26] |= a;
        }
        if(buf[5] & 0x20) {    // Request IP of given device type
            buf[5] &= ~0x20;
            parm &= 0x0f;
            if(parm) {
                for(int i = 0; i < BTTFN_MAX_CLIENTS; i++) {
                    if(bttfnClient[i].IP32) {
                        if(parm == bttfnClient[i].Type) {
                            for(int j = 0; j < 4; j++) {
                                buf[27+j] = bttfnClient[i].IP[j];
                            }
                            buf[5] |= 0x20;
                            break;
                        }
                    } else break;
                }
            }
        }
        if(buf[5] & 0x40) {    // Request TCD Capabilities
            a = 0x01 | 0x04 | 0x08;  // Support MC notifications | Can send dest/pres/dep times | Remote KP support
            // all other bits for future use
            buf[31] = a;
        }
        // 0x80 taken (TT)

    #ifdef TC_HAVE_REMOTE
    }
    #endif

    // Calc checksum
    a = 0;
    for(int i = 4; i < BTTF_PACKET_SIZE - 1; i++) {
        a += buf[i] ^ 0x55;
    }
    buf[BTTF_PACKET_SIZE - 1] = a;

    return true;
}

static void bttfn_notify_of_speed()
{
    int16_t  spd = -1;
    uint16_t ssrc = BTTFN_SSRC_NONE;
    uint16_t parm3 = 0;
    unsigned long now;

    if(!bttfnAtLeastOneMC)
        return;

    if(timeTravelP0) {
        spd = timeTravelP0Speed;
        ssrc = BTTFN_SSRC_P0;
        parm3 = timeTravelP0stalled;
    } else if(timeTravelP1) {
        spd = 88;
        ssrc = BTTFN_SSRC_P1;
    } else if(timeTravelP2) {
        spd = timeTravelP0Speed;
        ssrc = BTTFN_SSRC_P2;
    #ifdef TC_HAVE_REMOTE
    } else if(bttfnRemoteSpeedMaster) {
        spd = bttfnRemCurSpd;
        ssrc = BTTFN_SSRC_REM;
    #endif
    #ifdef TC_HAVEGPS
    } else if(useGPS && provGPS2BTTFN) {
        // Why "&& provGPS2BTTFN"?
        // Because: If stationary user uses GPS for time only, speed will
        // be 0 permanently, and rotary encoder never gets a chance.
        spd = myGPS.getSpeed();
        ssrc = BTTFN_SSRC_GPS;
    #endif
    #ifdef TC_HAVE_RE
    } else if(useRotEnc && FPBUnitIsOn) {
        spd = fakeSpeed;
        ssrc = BTTFN_SSRC_ROTENC;
    #endif
    }

    now = millis();
    if(spd != oldBTTFNSpd || ssrc != oldBTTFNSSrc || (now - bttfnLastSpeedNot > 2775)) {
        oldBTTFNSpd = spd;
        oldBTTFNSSrc = ssrc;
        bttfn_notify(BTTFN_TYPE_ANY, BTTFN_NOT_SPD, (uint16_t)spd, ssrc, parm3);
        bttfnLastSpeedNot = now;
        //Serial.printf("notify speed: %d %d %d\n", spd, ssrc, parm3);
    }
}

void bttfn_tcd_busy(bool isBusy)
{
    uint16_t remStat = (remoteAllowed ? 0 : 1) | (remoteKPAllowed ? 0 : 2);
    bttfn_notify(BTTFN_TYPE_ANY, BTTFN_NOT_BUSY, remStat, isBusy ? 1 : 0);
}

// Send event notification to known clients
static void bttfn_notify(uint8_t targetType, uint8_t event, uint16_t payload, uint16_t payload2, uint16_t payload3)
{
    // No clients?
    if(!bttfnHaveClients)
        return;

    IPAddress ip(224, 0, 0, 224);
    bool spdNot = false;

    // Clear & copy ID and VERSION|0x40
    memcpy(BTTFUDPBuf, BTTFUDPHD, BTTF_PACKET_SIZE);

    //BTTFUDPBuf[4] = BTTFN_VERSION + 0x40; // Version + notify marker - already there
    BTTFUDPBuf[5]  = event;                  // Store event id
    BTTFUDPBuf[6]  = payload & 0xff;         // store payload
    BTTFUDPBuf[7]  = payload >> 8;           //
    BTTFUDPBuf[8]  = payload2 & 0xff;        // store payload 2
    BTTFUDPBuf[9]  = payload2 >> 8;          //
    BTTFUDPBuf[10] = payload3 & 0xff;        // store payload 3
    BTTFUDPBuf[11] = payload3 >> 8;          //
    
    if(event == BTTFN_NOT_SPD) {
        SET32(BTTFUDPBuf, 12, bttfnSeqCnt);
        bttfnSeqCnt++;
        if(!bttfnSeqCnt) bttfnSeqCnt = 1;
        spdNot = true;
    }
    
    // Checksum
    uint8_t a = 0;
    for(int i = 4; i < BTTF_PACKET_SIZE - 1; i++) {
        a += BTTFUDPBuf[i] ^ 0x55;
    }
    BTTFUDPBuf[BTTF_PACKET_SIZE - 1] = a;

    if((!targetType && !bttfnNotAllSupportMC) || spdNot) {
        // Send out through multicast
        tcdUDP->beginPacket(ip, BTTF_DEFAULT_LOCAL_PORT + 2);
        tcdUDP->write(BTTFUDPBuf, BTTF_PACKET_SIZE);
        tcdUDP->endPacket();
    } else {
        // Send out to all known clients
        for(int i = 0; i < BTTFN_MAX_CLIENTS; i++) {
            if(!bttfnClient[i].IP32)
                break;
            if(!targetType || targetType == bttfnClient[i].Type) {
                ip = bttfnClient[i].IP32;
                tcdUDP->beginPacket(ip, BTTF_DEFAULT_LOCAL_PORT);
                tcdUDP->write(BTTFUDPBuf, BTTF_PACKET_SIZE);
                tcdUDP->endPacket();
            }
        }
    }
}
