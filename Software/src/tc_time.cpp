/*
 * -------------------------------------------------------------------
 * CircuitSetup.us Time Circuits Display
 * (C) 2021-2022 John deGlavina https://circuitsetup.us
 * (C) 2022-2023 Thomas Winischhofer (A10001986)
 * https://github.com/realA10001986/Time-Circuits-Display-A10001986
 *
 * Time and Main Controller
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
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
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
#ifdef FAKE_POWER_ON
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
#define SHT40_ADDR     0x44 // [default]
#define SI7021_ADDR    0x40 // [default]
#define TMP117_ADDR    0x49 // [non-default]
#define AHT20_ADDR     0x38 // [default]
#define HTU31_ADDR     0x41 // [non-default]

                            // light sensors 
#define LTR3xx_ADDR    0x29 // [default]                            
#define TSL2561_ADDR   0x29 // [default]
#define BH1750_ADDR    0x23 // [default]
#define VEML6030_ADDR  0x48 // [non-default] (can also be 0x10 but then conflicts with GPS)
#define VEML7700_ADDR  0x10 // [default] (conflicts with GPS!)

// The time between reentry sound being started and the display coming on
// Must be sync'd to the sound file used! (startup.mp3/timetravel.mp3)
#ifdef TWSOUND
#define STARTUP_DELAY 900
#else
#define STARTUP_DELAY 1050
#endif
#define TIMETRAVEL_DELAY 1500

// Time between the phases of (long) time travel
// Sum of all must be 8000
#define TT_P1_DELAY_P1  1400                                                                    // Normal
#define TT_P1_DELAY_P2  (4200-TT_P1_DELAY_P1)                                                   // Light flicker
#define TT_P1_DELAY_P3  (5800-(TT_P1_DELAY_P2+TT_P1_DELAY_P1))                                  // Off
#define TT_P1_DELAY_P4  (6800-(TT_P1_DELAY_P3+TT_P1_DELAY_P2+TT_P1_DELAY_P1))                   // Random display I
#define TT_P1_DELAY_P5  (8000-(TT_P1_DELAY_P4+TT_P1_DELAY_P3+TT_P1_DELAY_P2+TT_P1_DELAY_P1))    // Random display II
// ACTUAL POINT OF TIME TRAVEL:
#define TT_P1_POINT88   1000    // ms into "starttravel" sample, when 88mph is reached.
#define TT_SNDLAT        400    // DO NOT CHANGE

// Preprocessor config for External Time Travel Output (ETTO):
// Lead time from trigger (LOW->HIGH) to actual tt (ie when 88mph is reached)
// The external prop has ETTO_LEAD_TIME ms to play its pre-tt sequence. After
// ETTO_LEAD_TIME ms, 88mph is reached, and the actual tt takes place.
#define ETTO_LEAD_TIME      5000
// Trigger mode:
// true:  Pulse for ETTO_PULSE_DURATION ms on tt start minus leadTime
// false: LOW->HIGH on tt start minus leadTime, HIGH->LOW on start of reentry
#define ETTO_USE_PULSE      false
// If ETTO_USE_PULSE is true, pulse for approx. ETTO_PULSE_DURATION ms
#define ETTO_PULSE_DURATION 1000
// End of ETTO config

// Native NTP
#define NTP_PACKET_SIZE 48
#define NTP_DEFAULT_LOCAL_PORT 1337

unsigned long  powerupMillis = 0;

static bool    couldHaveAuthTime = false;
static bool    haveAuthTime = false;
uint16_t       lastYear = 0;
static uint8_t resyncInt = 5;
static uint8_t dstChkInt = 5;

// For tracking second changes
static bool x = false;  
static bool y = false;

// The startup sequence
bool                 startup      = false;
static bool          startupSound = false;
static unsigned long startupNow   = 0;

// Pause auto-time-cycling if user played with time travel
static bool          autoPaused = false;
static unsigned long pauseNow = 0;
static unsigned long pauseDelay = 30*60*1000;  // Pause for 30 minutes

// "Room condition" mode
static bool          rcMode = false;
bool                 haveRcMode = false;
#define TEMP_UPD_INT_L (2*60*1000)
#define TEMP_UPD_INT_S (30*1000)

// The timetravel sequence: Timers, triggers, state
static bool          playTTsounds = true;
int                  timeTravelP0 = 0;
int                  timeTravelP2 = 0;

#ifdef TC_HAVESPEEDO
bool                 useSpeedo = DEF_USE_SPEEDO;
static unsigned long timetravelP0Now = 0;
static unsigned long timetravelP0Delay = 0;
static unsigned long ttP0Now = 0;
static uint8_t       timeTravelP0Speed = 0;
static long          pointOfP1 = 0;
static bool          didTriggerP1 = false;
static float         ttP0TimeFactor = 1.0;
#ifdef TC_HAVEGPS
static unsigned long dispGPSnow = 0;
#endif
#ifdef TC_HAVETEMP
static int           tempBrightness = DEF_TEMP_BRIGHT;
#endif
#endif // HAVESPEEDO
bool                 useTemp = false;
bool                 dispTemp = true;
bool                 tempOffNM = true;
#ifdef TC_HAVETEMP
bool                 tempUnit = DEF_TEMP_UNIT;
static unsigned long tempReadNow = 0;
static unsigned long tempDispNow = 0;
static unsigned long tempUpdInt = TEMP_UPD_INT_L;
#endif
static unsigned long timetravelP1Now = 0;
static unsigned long timetravelP1Delay = 0;
int                  timeTravelP1 = 0;
static bool          triggerP1 = false;
static unsigned long triggerP1Now = 0;
static long          triggerP1LeadTime = 0;
#ifdef EXTERNAL_TIMETRAVEL_OUT
static bool          useETTO = DEF_USE_ETTO;
static bool          ettoUsePulse = ETTO_USE_PULSE;
static long          ettoLeadTime = ETTO_LEAD_TIME;
static bool          triggerETTO = false;
static long          triggerETTOLeadTime = 0;
static unsigned long triggerETTONow = 0;
static bool          ettoPulse = false;
static unsigned long ettoPulseNow = 0;
static long          ettoLeadPoint = 0;
#endif

// The timetravel re-entry sequence
static unsigned long timetravelNow = 0;
bool                 timeTraveled = false;

int specDisp = 0;

// CPU power management
static bool          pwrLow = false;
static unsigned long pwrFullNow = 0;

// For displaying times off the real time
uint64_t timeDifference = 0;
bool     timeDiffUp = false;  // true = add difference, false = subtract difference

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
bool timetravelPersistent = true;

// Alarm/HourlySound based on RTC (or presentTime's display)
static bool alarmRTC = true;

bool useGPS      = false;
bool useGPSSpeed = false;

// TZ/DST status & data
static bool useDST       = false; // Do use own DST management (and DST is defined in TZ)
bool        couldDST     = false; // Could use own DST management (and DST is defined in TZ)
static int  tzForYear    = 0;     // Parsing done for this very year
static bool tzIsValid    = false;
static char *tzDSTpart   = NULL;
static int  tzDiffGMT    = 0;     // Difference to UTC in nonDST time
static int  tzDiffGMTDST = 0;     // Difference to UTC in DST time
static int  tzDiff       = 0;     // difference between DST and non-DST in minutes
static int  DSTonMins    = -1;    // DST-on date/time in minutes since 1/1 00:00 (in non-DST time)
static int  DSToffMins   = 600000;// DST-off date/time in minutes since 1/1 00:00 (in DST time)

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
static uint8_t       NTPUDPID[4] = { 0, 0, 0, 0};
 
// The RTC object
tcRTC rtc(2, (uint8_t[2*2]){ PCF2129_ADDR, RTCT_PCF2129, 
                             DS3231_ADDR,  RTCT_DS3231 });
static unsigned long OTPRDoneNow = 0;
static bool          RTCNeedsOTPR = false;
static bool          OTPRinProgress = false;
static unsigned long OTPRStarted = 0;

// The GPS object
#ifdef TC_HAVEGPS
tcGPS myGPS(GPS_ADDR);
static unsigned long lastLoopGPS = 0;
static unsigned long GPSupdateFreq    = 1000;
static unsigned long GPSupdateFreqMin = 2000;
#endif

// The TC display objects
clockDisplay destinationTime(DISP_DEST, DEST_TIME_ADDR, DEST_TIME_PREF);
clockDisplay presentTime(DISP_PRES, PRES_TIME_ADDR, PRES_TIME_PREF);
clockDisplay departedTime(DISP_LAST, DEPT_TIME_ADDR, DEPT_TIME_PREF);

// The speedo and temp.sensor objects
#ifdef TC_HAVESPEEDO
speedDisplay speedo(SPEEDO_ADDR);
#endif
#ifdef TC_HAVETEMP
tempSensor tempSens(7, 
            (uint8_t[7*2]){ MCP9808_ADDR, MCP9808,
                            BMx280_ADDR,  BMx280,
                            SHT40_ADDR,   SHT40,
                            SI7021_ADDR,  SI7021,
                            TMP117_ADDR,  TMP117,
                            AHT20_ADDR,   AHT20,
                            HTU31_ADDR,   HTU31
                          });
static bool tempOldNM = false;
#endif
#ifdef TC_HAVELIGHT
lightSensor lightSens(5, 
            (uint8_t[5*2]){ LTR3xx_ADDR,  LST_LTR3xx,   // must be before TSL2651
                            TSL2561_ADDR, LST_TSL2561,  // must be after LTR3xx
                            BH1750_ADDR,  LST_BH1750,
                            VEML6030_ADDR,LST_VEML7700,
                            VEML7700_ADDR,LST_VEML7700  // must be last
                          });
#endif

// Automatic times ("decorative mode")

static int8_t minNext  = 0;
int8_t        autoTime = 0;  // Selects date/time from array below

dateStruct destinationTimes[NUM_AUTOTIMES] = {
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
dateStruct departedTimes[NUM_AUTOTIMES] = {
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

unsigned long ctDown = 0;
unsigned long ctDownNow = 0;

// State flags
static bool DSTcheckDone = false;
static bool autoIntDone = false;
static int  autoIntAnimRunning = 0;
static bool autoReadjust = false;
static unsigned long lastAuthTime = 0;
static bool authTimeExpired = false;
static bool alarmDone = false;
static bool hourlySoundDone = false;
static bool autoNMDone = false;

// Fake "power" switch
bool FPBUnitIsOn = true;
#ifdef FAKE_POWER_ON
static TCButton fakePowerOnKey = TCButton(FAKE_POWER_BUTTON_PIN,
    true,    // Button is active LOW
    true     // Enable internal pull-up resistor
);
static bool isFPBKeyChange = false;
static bool isFPBKeyPressed = false;
bool        waitForFakePowerButton = false;
#endif

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
             0,  262975680,  525949920,  788924160, 1051898400,
    1314874080, 1577848320, 1840822560, 2103796800, 2366772480,
    2629746720, 2892720960, 3155695200, 3418670880, 3681645120,
    3944619360, 4207593600, 4470569280, 4733543520, 4996517760
};
static const uint32_t hours1kYears[] =
{
                0,  262975680/60,  525949920/60,  788924160/60, 1051898400/60,
    1314874080/60, 1577848320/60, 1840822560/60, 2103796800/60, 2366772480/60, 
    2629746720/60, 2892720960/60, 3155695200/60, 3418670880/60, 3681645120/60, 
    3944619360/60, 4207593600/60, 4470569280/60, 4733543520/60, 4996517760/60  
};

#ifdef TC_HAVESPEEDO
static const int16_t tt_p0_delays[88] =
{
      0, 100, 100,  90,  80,  80,  80,  80,  80,  80,  // 0 - 9  10mph 0.8s    0.8=800ms
     80,  80,  80,  80,  80,  80,  80,  80,  80,  80,  // 10-19  20mph 1.6s    0.8
     90, 100, 110, 110, 110, 110, 110, 110, 120, 120,  // 20-29  30mph 2.7s    1.1
    120, 130, 130, 130, 130, 130, 130, 130, 130, 140,  // 30-39  40mph 4.0s    1.3
    150, 160, 190, 190, 190, 190, 190, 190, 210, 230,  // 40-49  50mph 5.9s    1.9
    230, 230, 240, 240, 240, 240, 240, 240, 250, 250,  // 50-59  60mph 8.3s    2.4
    250, 250, 260, 260, 270, 270, 270, 280, 290, 300,  // 60-69  70mph 11.0s   2.7
    320, 330, 350, 370, 370, 380, 380, 390, 400, 410,  // 70-79  80mph 14.7s   3.7
    410, 410, 410, 410, 410, 410, 410, 410             // 80-87  90mph 18.8s   4,1
};
static long tt_p0_totDelays[88];
#endif

static void myCustomDelay(unsigned int mydel);
static void myIntroDelay(unsigned int mydel, bool withGPS = true);
static void waitAudioDoneIntro();

static bool getNTPOrGPSTime(bool weHaveAuthTime);
static bool getNTPTime(bool weHaveAuthTime);
#ifdef TC_HAVEGPS
static bool getGPStime();
static bool setGPStime();
bool        gpsHaveFix();
static bool gpsHaveTime();
static void dispGPSSpeed(bool force = false);
#endif

#ifdef TC_HAVETEMP
static void updateTemperature(bool force = false);
#ifdef TC_HAVESPEEDO
static void dispTemperature(bool force = false);
#endif
#endif

static void triggerLongTT();

#ifdef EXTERNAL_TIMETRAVEL_OUT
static void ettoPulseStart();
static void ettoPulseEnd();
#endif

// Time calculations
static int  mins2Date(int year, int month, int day, int hour, int mins);
static bool blockDSTChange(int currTimeMins);
static void localToDST(int& year, int& month, int& day, int& hour, int& minute, int& isDST);
static void handleDSTFlag(struct tm *ti, int nisDST = -1);

/// Native NTP
static void ntp_setup();
static bool NTPHaveTime();
static bool NTPTriggerUpdate();
static void NTPSendPacket();
static void NTPCheckPacket();
static uint32_t NTPGetSecsSinceTCepoch();
static bool NTPGetLocalTime(int& year, int& month, int& day, int& hour, int& minute, int& second, int& isDST);
static bool NTPHaveLocalTime();


/*
 * time_boot()
 *
 */
void time_boot()
{
    // Start the displays early to clear them
    presentTime.begin();
    destinationTime.begin();
    departedTime.begin();

    // Switch LEDs on
    pinMode(LEDS_PIN, OUTPUT);
    leds_on();
}

/*
 * time_setup()
 *
 */
void time_setup()
{
    bool rtcbad = false;
    bool tzbad = false;
    bool haveGPS = false;
    #ifdef TC_HAVEGPS
    bool haveAuthTimeGPS = false;
    #endif

    Serial.println(F("Time Circuits Display version " TC_VERSION " " TC_VERSION_EXTRA));

    // Power management: Set CPU speed
    // to maximum and start timer
    #ifdef TC_DBG
    Serial.print("Initial CPU speed is ");
    Serial.println(getCpuFrequencyMhz(), DEC);
    #endif
    pwrNeedFullNow(true);

    // Pin for monitoring seconds from RTC
    pinMode(SECONDS_IN_PIN, INPUT_PULLDOWN);  

    // Status LED
    pinMode(STATUS_LED_PIN, OUTPUT);          

    // Init fake power switch
    #ifdef FAKE_POWER_ON
    waitForFakePowerButton = ((int)atoi(settings.fakePwrOn) > 0);
    if(waitForFakePowerButton) {
        fakePowerOnKey.setPressTicks(10);     // ms after short press is assumed (default 400)
        fakePowerOnKey.setLongPressTicks(50); // ms after long press is assumed (default 800)
        fakePowerOnKey.setDebounceTicks(50);
        fakePowerOnKey.attachLongPressStart(fpbKeyPressed);
        fakePowerOnKey.attachLongPressStop(fpbKeyLongPressStop);
    }
    #endif

    // Init external time travel output ("etto")
    #ifdef EXTERNAL_TIMETRAVEL_OUT
    pinMode(EXTERNAL_TIMETRAVEL_OUT_PIN, OUTPUT);
    ettoPulseEnd();
    #endif

    // RTC setup
    if(!rtc.begin(powerupMillis)) {

        Serial.println(F("time_setup: Couldn't find RTC. Panic!"));

        // Blink white LED forever
        pinMode(WHITE_LED_PIN, OUTPUT);
        while (1) {
            digitalWrite(WHITE_LED_PIN, HIGH);
            delay(1000);
            digitalWrite(WHITE_LED_PIN, LOW);
            delay(1000);
        }
    }

    RTCNeedsOTPR = rtc.NeedOTPRefresh();

    if(rtc.lostPower()) {

        // Lost power and battery didn't keep time, so set current time to 
        // default time 1/1/2023, 0:0
        rtc.adjust(0, 0, 0, dayOfWeek(1, 1, 2023), 1, 1, 23);
       
        #ifdef TC_DBG
        Serial.println(F("time_setup: RTC lost power, setting default time."));
        #endif

        rtcbad = true;
    }

    if(RTCNeedsOTPR) {
        rtc.OTPRefresh(true);
        delay(100);
        rtc.OTPRefresh(false);
        delay(100);
        OTPRDoneNow = millis();
    }

    // Turn on the RTC's 1Hz clock output
    rtc.clockOutEnable();  

    // Start the displays
    presentTime.begin();
    destinationTime.begin();
    departedTime.begin();

    // Initialize clock mode: 12 hour vs 24 hour
    bool mymode24 = ((int)atoi(settings.mode24) > 0);
    presentTime.set1224(mymode24);
    destinationTime.set1224(mymode24);
    departedTime.set1224(mymode24);

    // Configure presentTime as a display that will hold real time
    presentTime.setRTC(true);

    // Configure behavior in night mode
    destinationTime.setNMOff(((int)atoi(settings.dtNmOff) > 0));
    presentTime.setNMOff(((int)atoi(settings.ptNmOff) > 0));
    departedTime.setNMOff(((int)atoi(settings.ltNmOff) > 0));

    // Determine if user wanted Time Travels to be persistent
    timetravelPersistent = ((int)atoi(settings.timesPers) > 0);

    // Load present time settings (yearOffs, timeDifference, isDST)
    presentTime.load((int)atoi(settings.presTimeBright));

    if(!timetravelPersistent) {
        timeDifference = 0;
    }

    if(rtcbad) {
        presentTime.setYearOffset(0);
        presentTime.setDST(0);
    }

    // See if speedo display is to be used
    #ifdef TC_HAVESPEEDO
    useSpeedo = ((int)atoi(settings.useSpeedo) > 0);
    #endif

    // Set up GPS receiver
    #ifdef TC_HAVEGPS
    useGPS = ((int)atoi(settings.useGPS) > 0);
    
    #ifdef TC_HAVESPEEDO
    if(useSpeedo) {
        useGPSSpeed = ((int)atoi(settings.useGPSSpeed) > 0);
    }
    #endif

    // Check for GPS receiver
    // Do so regardless of usage in order to eliminate
    // VEML7700 light sensor with identical i2c address
    if(myGPS.begin(powerupMillis, useGPSSpeed)) {

        myGPS.setCustomDelayFunc(myCustomDelay);
        haveGPS = true;
        
        if(useGPS || useGPSSpeed) {
          
            // Clear so we don't add to stampAge unnecessarily in
            // boot strap
            GPSupdateFreq = GPSupdateFreqMin = 0;
            
            // We know now we have a possible source for auth time
            couldHaveAuthTime = true;
            
            // Fetch data already in Receiver's buffer [120ms]
            for(int i = 0; i < 10; i++) myGPS.loop(true);
            
            #ifdef TC_DBG
            Serial.println(F("time_setup: GPS Receiver found and initialized"));
            #endif

        } 

    } else {
      
        useGPS = useGPSSpeed = false;
        
        #ifdef TC_DBG
        Serial.println(F("time_setup: GPS Receiver not found"));
        #endif
        
    }
    #endif

    // If NTP server configured and we are not in WiFi-AccessPoint-Mode, 
    // we might have a source for auth time.
    // (Do not involve WiFi connection status here; might change later)
    if((settings.ntpServer[0] != 0) && (!wifiInAPMode))
        couldHaveAuthTime = true;
    
    // Try to obtain initial authoritative time

    ntp_setup();
    if((settings.ntpServer[0] != 0) && (WiFi.status() == WL_CONNECTED)) {
        int timeout = 50;
        do {
            ntp_loop();
            delay(100);
            timeout--;
        } while (!NTPHaveTime() && timeout);
    }

    // Parse TZ and set TZ parameters
    // Year does not matter at this point. We only parse 
    // the actual time zone for the first calculations.
    if(!(parseTZ(settings.timeZone, 2022, false))) {
        tzbad = true;
        #ifdef TC_DBG
        Serial.println(F("time_setup: Failed to parse TZ"));
        #endif
    }

    // Set RTC with NTP time
    if(getNTPTime(true)) {

        // So we have authoritative time now
        haveAuthTime = true;
        lastAuthTime = millis();
        
        #ifdef TC_DBG
        Serial.println(F("time_setup: RTC set through NTP"));        
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
                if(i == 0) Serial.println(F("time_setup: First attempt to read time from GPS"));
                #endif
                if(getGPStime()) {
                    // So we have authoritative time now
                    haveAuthTime = true;
                    lastAuthTime = millis();
                    haveAuthTimeGPS = true;
                    #ifdef TC_DBG
                    Serial.println(F("time_setup: RTC set through GPS"));
                    #endif
                    break;
                }
                yield();
                delay(100);
            }
            // If we haven't managed to get time here, try
            // a sync in shorter intervals in time_loop.
            if(!haveAuthTimeGPS) resyncInt = 2;
        }
        #endif
    }

    if(haveAuthTime) {
        // Save YearOffs & isDST to NVM if change is detected
        if( (presentTime.getYearOffset() != presentTime.loadYOffs()) ||
            (presentTime.getDST()        != presentTime.loadDST()) ) {
            presentTime.save();
        }
    }
    
    // Reset (persistent) time travel if RTC is bad and no NTP/GPS time
    if(rtcbad && !haveAuthTime) {
        timeDifference = 0;
    }

    // Start the Config Portal. Now is a good time, we had
    // our NTP access, so a WiFiScan does not disturb anything
    // at this point.
    if(WiFi.status() == WL_CONNECTED) {
        wifiStartCP();
    }

    // Load the time for initial display
    
    DateTime dt = myrtcnow();

    // rtcYear: Current year as read from the RTC
    uint16_t rtcYear = dt.year() - presentTime.getYearOffset();

    // lastYear: The year when the RTC was adjusted for the last time.
    // If the RTC was just updated, everything is in place.
    // Otherwise load from NVM, and make required re-calculations.
    if(haveAuthTime) {
        lastYear = rtcYear;
    } else {
        lastYear = presentTime.loadLastYear();
    }

    // Parse (complete) TZ and set TZ/DST parameters
    // (sets/clears couldDST)
    if(!(parseTZ(settings.timeZone, rtcYear))) {
        tzbad = true;
    }
 
    // If we are switched on after a year switch, re-do
    // RTC year-translation.
    if(lastYear != rtcYear) {

        // Get RTC-fit year & offs for real year
        int16_t yOffs = 0;
        uint16_t newYear = rtcYear;
        correctYr4RTC(newYear, yOffs);

        // If year-translation changed, update RTC and save
        if((newYear != dt.year()) || (yOffs != presentTime.getYearOffset())) {

            // Update RTC
            dt = myrtcnow();
            rtc.adjust(dt.second(), 
                       dt.minute(), 
                       dt.hour(), 
                       dayOfWeek(dt.day(), dt.month(), rtcYear),
                       dt.day(), 
                       dt.month(), 
                       newYear-2000
            );

            presentTime.setYearOffset(yOffs);

            // Save YearOffs to NVM if change is detected
            if(presentTime.getYearOffset() != presentTime.loadYOffs()) {
                if(timetravelPersistent) {
                    presentTime.save();
                } else {
                    presentTime.saveYOffs();
                }
            }
            
            presentTime.saveLastYear(rtcYear);

            lastYear = rtcYear;

            // Re-read RTC
            dt = myrtcnow();
        }
    }

    // Load to display
    presentTime.setDateTimeDiff(dt);

    // If we don't have authTime, DST status (time & flag) might be 
    // wrong at this point if we're switched on after a long time. 
    // We leave DST determination to time_loop; just cut short the 
    // check interval here.
    if(!haveAuthTime) dstChkInt = 1;

    // Set GPS RTC
    // It does not matter, if DST status (time & flag) is wrong here,
    // as long as time matches DST flag setting (which it does even
    // if we have no authTime).
    #ifdef TC_HAVEGPS
    if(useGPS || useGPSSpeed) {
        if(!haveAuthTimeGPS && (haveAuthTime || !rtcbad)) {
            setGPStime();
        }
        
        // Set ms between GPS polls
        // Need more updates if speed is to be displayed
        GPSupdateFreq    = useGPSSpeed ? 250 : 500;
        GPSupdateFreqMin = useGPSSpeed ? 500 : 500;
    }
    #endif

    // Load destination time (and set to default if invalid)
    if(!destinationTime.load((int)atoi(settings.destTimeBright))) {
        destinationTime.setYearOffset(0);
        destinationTime.setFromStruct(&destinationTimes[0]);
        destinationTime.setBrightness((int)atoi(settings.destTimeBright));
        destinationTime.save();
    }

    // Load departed time (and set to default if invalid)
    if(!departedTime.load((int)atoi(settings.lastTimeBright))) {
        departedTime.setYearOffset(0);
        departedTime.setFromStruct(&departedTimes[0]);
        departedTime.setBrightness((int)atoi(settings.lastTimeBright));
        departedTime.save();
    }

    // Load autoInterval ("time cycling interval") from settings
    loadAutoInterval();

    // Load alarm from alarmconfig file
    // Don't care if data invalid, alarm is off in that case
    loadAlarm();

    // Auto-NightMode
    autoNightMode = (int)atoi(settings.autoNM);
    autoNightModeMode = (int)atoi(settings.autoNMPreset);
    if(autoNightModeMode > AUTONM_NUM_PRESETS) autoNightModeMode = 0;
    autoNMOnHour = (int)atoi(settings.autoNMOn);
    if(autoNMOnHour > 23) autoNMOnHour = 0;
    autoNMOffHour = (int)atoi(settings.autoNMOff);
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

    // If using auto times, put up the first one
    if(autoTimeIntervals[autoInterval]) {
        destinationTime.setFromStruct(&destinationTimes[0]);
        departedTime.setFromStruct(&departedTimes[0]);
    }

    // Set up alarm base: RTC or current "present time"
    alarmRTC = ((int)atoi(settings.alarmRTC) > 0);

    // Set up option to play/mute time travel sounds
    playTTsounds = ((int)atoi(settings.playTTsnds) > 0);

    // Set power-up setting for beep
    muteBeep = ((int)atoi(settings.beep) == 0);
    
    // Set up speedo display
    #ifdef TC_HAVESPEEDO
    if(useSpeedo) {
        speedo.begin((int)atoi(settings.speedoType));

        speedo.setBrightness((int)atoi(settings.speedoBright), true);
        speedo.setDot(true);

        // Speed factor for acceleration curve
        ttP0TimeFactor = (float)atof(settings.speedoFact);
        if(ttP0TimeFactor < 0.5) ttP0TimeFactor = 0.5;
        if(ttP0TimeFactor > 5.0) ttP0TimeFactor = 5.0;

        // Calculate start point of P1 sequence
        for(int i = 1; i < 88; i++) {
            pointOfP1 += (unsigned long)(((float)(tt_p0_delays[i])) / ttP0TimeFactor);
        }
        #ifdef EXTERNAL_TIMETRAVEL_OUT
        ettoLeadPoint = pointOfP1 - ettoLeadTime;   // Can be negative!
        #endif
        pointOfP1 -= TT_P1_POINT88;
        pointOfP1 -= TT_SNDLAT;     // Correction for sound/mp3 latency

        {
            // Calculate total elapsed delay for each mph value
            // (in order to time P0/P1 relative to current actual speed)
            long totDelay = 0;
            for(int i = 0; i < 88; i++) {
                totDelay += (long)(((float)(tt_p0_delays[i])) / ttP0TimeFactor);
                tt_p0_totDelays[i] = totDelay;
            }
        }

        speedo.off();

        #ifdef FAKE_POWER_ON
        if(!waitForFakePowerButton) {
        #endif
            speedo.setSpeed(0);
            speedo.on();
            speedo.show();
        #ifdef FAKE_POWER_ON
        }
        #endif

        #ifdef TC_HAVEGPS
        if(useGPSSpeed) {
            // display (actual) speed, regardless of fake power
            dispGPSSpeed(true);
            speedo.on(); 
        }
        #endif
    }
    #endif

    // Set up temperature sensor
    #ifdef TC_HAVETEMP
    useTemp = ((int)atoi(settings.useTemp) > 0);
    #ifdef TC_HAVESPEEDO
    dispTemp = ((int)atoi(settings.dispTemp) > 0);
    #else
    dispTemp = false;
    #endif
    if(useTemp) {
        if(tempSens.begin(powerupMillis)) {
            tempSens.setCustomDelayFunc(myCustomDelay);
            tempUnit = ((int)atoi(settings.tempUnit) > 0);
            tempSens.setOffset((float)atof(settings.tempOffs));
            #ifdef TC_HAVESPEEDO
            tempBrightness = (int)atoi(settings.tempBright);
            tempOffNM = ((int)atoi(settings.tempOffNM) > 0);
            if(!useSpeedo || useGPSSpeed) dispTemp = false;
            if(dispTemp) {
                #ifdef FAKE_POWER_ON
                if(!waitForFakePowerButton) {
                #endif
                    updateTemperature(true);
                    dispTemperature(true);
                #ifdef FAKE_POWER_ON
                }
                #endif
            }
            #endif
            haveRcMode = true;
        } else {
            useTemp = dispTemp = false;
        }
    } else {
        dispTemp = false;
    }
    #else
    useTemp = dispTemp = false;
    #endif

    #ifdef TC_HAVELIGHT
    useLight = ((int)atoi(settings.useLight) > 0);
    luxLimit = atoi(settings.luxLimit);
    if(useLight) {
        if(lightSens.begin(haveGPS, powerupMillis)) {
            lightSens.setCustomDelayFunc(myCustomDelay);
        } else {
            useLight = false;
        }
    }
    #else
    useLight = false;
    #endif

    // Set up tt trigger for external props
    #ifdef EXTERNAL_TIMETRAVEL_OUT
    useETTO = ((int)atoi(settings.useETTO) > 0);
    #endif

    // Show "REPLACE BATTERY" message if RTC battery is low or depleted
    // Note: This also shows up the first time you power-up the clock
    // AFTER a battery change.
    if(rtcbad || rtc.battLow()) {
        destinationTime.showTextDirect("REPLACE");
        presentTime.showTextDirect("BATTERY");
        destinationTime.on();
        presentTime.on();
        myIntroDelay(5000);
        allOff();
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
        #ifdef TC_DBG
        Serial.println(F("time_setup: calling doCopyAudioFiles()"));
        #endif
        destinationTime.showTextDirect("INSTALL");
        presentTime.showTextDirect("AUDIO FILES?");
        destinationTime.on();
        presentTime.on();
        doCopyAudioFiles();
        // We return here only if user cancelled or
        // if the menu timed-out
        allOff();
        waitForEnterRelease();
        isEnterKeyHeld = false;
        isEnterKeyPressed = false;
        #ifdef EXTERNAL_TIMETRAVEL_IN
        isEttKeyPressed = false;
        isEttKeyHeld = false;
        #endif
    }

    if(!audio_files_present()) {
        destinationTime.showTextDirect("PLEASE");
        presentTime.showTextDirect("INSTALL");
        departedTime.showTextDirect("AUDIO FILES");
        destinationTime.on();
        presentTime.on();
        departedTime.on();
        myIntroDelay(5000);
        allOff();
    }

    if((int)atoi(settings.playIntro)) {
        const char *t1 = "             BACK";
        const char *t2 = "TO";
        const char *t3 = "THE FUTURE";
        
        play_file("/intro.mp3", 1.0, true, true);

        myIntroDelay(1200);
        destinationTime.setBrightnessDirect(15);
        presentTime.setBrightnessDirect(0);
        departedTime.setBrightnessDirect(0);
        presentTime.off();
        departedTime.off();
        destinationTime.showTextDirect(t1);
        presentTime.showTextDirect(t2);
        departedTime.showTextDirect(t3);
        destinationTime.on();
        for(int i = 0; i < 14; i++) {
           myIntroDelay(50, false);
           destinationTime.showTextDirect(&t1[i]);
        }
        myIntroDelay(500);
        presentTime.on();
        departedTime.on();
        for(int i = 0; i <= 15; i++) {
            presentTime.setBrightnessDirect(i);
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
    }

#ifdef FAKE_POWER_ON
    if(waitForFakePowerButton) {
        digitalWrite(WHITE_LED_PIN, HIGH);
        myIntroDelay(500);
        digitalWrite(WHITE_LED_PIN, LOW);
        leds_off();
        isFPBKeyChange = false;
        FPBUnitIsOn = false;

        #ifdef TC_DBG
        Serial.println(F("time_setup: waiting for fake power on"));
        #endif

    } else {
#endif

        startup = true;
        startupSound = true;
        FPBUnitIsOn = true;

#ifdef FAKE_POWER_ON
    }
#endif
}

/*
 * time_loop()
 *
 */
void time_loop()
{
    #ifdef TC_DBG
    int dbgLastMin;
    #endif

    #ifdef FAKE_POWER_ON
    if(waitForFakePowerButton) {
        fakePowerOnKey.scan();

        if(isFPBKeyChange) {
            if(isFPBKeyPressed) {
                if(!FPBUnitIsOn) {
                    startup = true;
                    startupSound = true;
                    FPBUnitIsOn = true;
                    leds_on();
                    destinationTime.setBrightness(255); // restore brightnesses
                    presentTime.setBrightness(255);     // in case we got switched
                    departedTime.setBrightness(255);    // off during time travel
                    #ifdef TC_HAVETEMP
                    updateTemperature(true);
                    #ifdef TC_HAVESPEEDO
                    dispTemperature(true);
                    #endif
                    #endif
                }
            } else {
                if(FPBUnitIsOn) {
                    startup = false;
                    startupSound = false;
                    timeTraveled = false;
                    timeTravelP0 = 0;
                    timeTravelP1 = 0;
                    timeTravelP2 = 0;
                    triggerP1 = 0;
                    #ifdef EXTERNAL_TIMETRAVEL_OUT
                    triggerETTO = false;
                    if(useETTO && !ettoUsePulse) {
                        ettoPulseEnd();
                        ettoPulse = false;
                    }
                    #endif
                    FPBUnitIsOn = false;
                    cancelEnterAnim(false);
                    cancelETTAnim();
                    mp_stop();
                    play_file("/shutdown.mp3", 1.0, true, true);
                    mydelay(130);
                    allOff();
                    leds_off();
                    #ifdef TC_HAVESPEEDO
                    if(useSpeedo && !useGPSSpeed) speedo.off();
                    #endif
                    waitAudioDone();
                    stopAudio();
                }
            }
            isFPBKeyChange = false;
        }
    }
    #endif

    // Power management: CPU speed
    // Can only reduce when GPS is not used and WiFi is off
    if(!pwrLow && checkAudioDone() &&
                  #ifdef TC_HAVEGPS
                  !useGPS && !useGPSSpeed &&
                  #endif
                             (wifiIsOff || wifiAPIsOff) && (millis() - pwrFullNow >= 5*60*1000)) {
        setCpuFrequencyMhz(80);
        pwrLow = true;

        #ifdef TC_DBG
        Serial.printf("Reduced CPU speed to %d\n", getCpuFrequencyMhz());
        #endif
    }

    if(OTPRinProgress) {
        if(millis() - OTPRStarted > 100) {
            rtc.OTPRefresh(false);
            OTPRinProgress = false;
            OTPRDoneNow = millis();
        }
    }

    // Initiate startup delay, play startup sound
    if(startupSound) {
        startupNow = pauseNow = millis();
        play_file("/startup.mp3", 1.0, true, true);
        startupSound = false;
        // Don't let autoInt interrupt us
        autoPaused = true;
        pauseDelay = STARTUP_DELAY + 500;
    }

    // Turn display on after startup delay
    if(startup && (millis() - startupNow >= STARTUP_DELAY)) {
        animate();
        startup = false;
        #ifdef TC_HAVESPEEDO
        if(useSpeedo && !useGPSSpeed) {
            speedo.off(); // Yes, off.
            #ifdef TC_HAVETEMP
            updateTemperature(true);
            dispTemperature(true);
            #endif
        }
        #endif
    }

    #if defined(EXTERNAL_TIMETRAVEL_OUT) || defined(TC_HAVESPEEDO)
    // Timer for starting P1 if to be delayed
    if(triggerP1 && (millis() - triggerP1Now >= triggerP1LeadTime)) {
        triggerLongTT();
        triggerP1 = false;
    }
    #endif

    #ifdef EXTERNAL_TIMETRAVEL_OUT
    // Timer for start of ETTO signal/pulse
    if(triggerETTO && (millis() - triggerETTONow >= triggerETTOLeadTime)) {
        ettoPulseStart();
        triggerETTO = false;
        ettoPulse = true;
        ettoPulseNow = millis();
        #ifdef TC_DBG
        Serial.println(F("ETTO triggered"));
        #endif
    }
    // Timer to end ETTO pulse
    if(ettoUsePulse && ettoPulse && (millis() - ettoPulseNow >= ETTO_PULSE_DURATION)) {
        ettoPulseEnd();
        ettoPulse = false;
    }
    #endif

    // Time travel animation, phase 0: Speed counts up
    #ifdef TC_HAVESPEEDO
    if(timeTravelP0 && (millis() - timetravelP0Now >= timetravelP0Delay)) {

        unsigned long univNow = millis();
        long timetravelP0DelayT = 0;

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
    }

    // Time travel animation, phase 2: Speed counts down
    if(timeTravelP2 && (millis() - timetravelP0Now >= timetravelP0Delay)) {
        #ifdef TC_HAVEGPS
        bool countToGPSSpeed = (useGPSSpeed && (myGPS.getSpeed() >= 0));
        uint8_t targetSpeed = countToGPSSpeed ? myGPS.getSpeed() : 0;
        #else
        uint8_t targetSpeed = 0;
        #endif
        if((timeTravelP0Speed <= targetSpeed) || (targetSpeed >= 88)) {
            timeTravelP2 = 0;
            if(!useGPSSpeed) speedo.off();
            #ifdef TC_HAVEGPS
            dispGPSSpeed(true);
            #endif
            #ifdef TC_HAVETEMP
            updateTemperature(true);
            dispTemperature(true);
            #endif
        } else {
            timetravelP0Now = millis();
            timeTravelP0Speed--;
            speedo.setSpeed(timeTravelP0Speed);
            speedo.show();
            #ifdef TC_HAVEGPS
            if(countToGPSSpeed) {
                if(targetSpeed == timeTravelP0Speed) {
                    timetravelP0Delay = 0;
                } else {
                    uint8_t tt = ((timeTravelP0Speed-targetSpeed)*100) / (88 - targetSpeed);
                    timetravelP0Delay = ((100 - tt) * 150) / 100;
                    if(timetravelP0Delay < 40) timetravelP0Delay = 40;
                }
            } else {
                timetravelP0Delay = (timeTravelP0Speed == 0) ? 4000 : 40;
            }
            #else
            timetravelP0Delay = (timeTravelP0Speed == 0) ? 4000 : 40;
            #endif
        }
    }
    #endif  // TC_HAVESPEEDO

    // Time travel animation, phase 1: Display disruption
    if(timeTravelP1 && (millis() - timetravelP1Now >= timetravelP1Delay)) {
        timeTravelP1++;
        timetravelP1Now = millis();
        switch(timeTravelP1) {
        case 2:
            allOff();
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
            timeTravel(false);
        }
    }

    // Turn display back on after time traveling
    if(timeTraveled && (millis() - timetravelNow >= TIMETRAVEL_DELAY)) {
        animate();
        timeTraveled = false;
    }

    // Read GPS, and display GPS speed or temperature

    #ifdef TC_HAVEGPS
    if(useGPS || useGPSSpeed) {
        if(millis() - lastLoopGPS >= GPSupdateFreq) {
            lastLoopGPS = millis();
            myGPS.loop(true);
            #ifdef TC_HAVESPEEDO
            dispGPSSpeed(true);
            #endif
        }
    }
    #endif

    #ifdef TC_HAVETEMP
    updateTemperature();
    #ifdef TC_HAVESPEEDO
    dispTemperature();
    #endif
    #endif
    
    #ifdef TC_HAVELIGHT
    if(useLight && (millis() - lastLoopLight >= 3000)) {
        lastLoopLight = millis();
        lightSens.loop();
    }
    #endif

    // Actual clock stuff

    y = digitalRead(SECONDS_IN_PIN);
    if(y != x) {      // different on half second
        if(y == 0) {  // flash colon on half seconds, lit on start of second

            // Set colon
            destinationTime.setColon(true);
            presentTime.setColon(true);
            departedTime.setColon(true);

            // Prepare for time re-adjustment through NTP/GPS

            if(millis() - lastAuthTime >= 7*24*60*60*1000) {
                authTimeExpired = true;
            }

            #ifdef TC_HAVEGPS
            bool GPShasTime = gpsHaveTime();
            #else
            bool GPShasTime = false;
            #endif

            DateTime dt = myrtcnow();

            // Re-adjust time periodically through NTP/GPS
            // This is normally done hourly between hour:01 and hour:02. However:
            // - If no GPS time is available and Wifi is in power-save, update only once 
            //   a week during night hours. We don't want a stalled display due to 
            //   WiFi re-connect during day time for too often.
            // - Then again, ignore rule above if we have no authoritative time yet. In 
            //   that case, try every 5th+1 and 5th+2 (resyncInt) minute. Frozen displays
            //   are avoided by setting the WiFi off-timer to a period longer than the
            //   period between two attempts; this in essence defeats WiFi power-save, 
            //   but accurate time is more important.
            // - In any case, do not interrupt any running sequences.

            bool itsTime = ( (dt.minute() == 1) || 
                             (dt.minute() == 2) ||
                             (!haveAuthTime && 
                              ( ((dt.minute() % resyncInt) == 1) || 
                                ((dt.minute() % resyncInt) == 2) ||
                                GPShasTime 
                              ) 
                             )
                           );
            
            if( couldHaveAuthTime                       &&
                itsTime                                 &&
                (!haveAuthTime  || 
                 GPShasTime     || 
                 !wifiIsOff     || 
                 (authTimeExpired && (dt.hour() <= 6))) &&
                !timeTraveled && !timeTravelP0 && !timeTravelP1 && !timeTravelP2 ) {

                if(!autoReadjust) {

                    uint64_t oldT;

                    if(timeDifference) {
                        oldT = dateToMins(dt.year() - presentTime.getYearOffset(),
                                       dt.month(), dt.day(), dt.hour(), dt.minute());
                    }

                    // Note that the following call will fail if getNTPtime reconnects
                    // WiFi; no current NTP time stamp will be available. Repeated calls
                    // will eventually succeed.

                    if(getNTPOrGPSTime(haveAuthTime)) {

                        bool wasFakeRTC = false;
                        uint64_t allowedDiff = 61;

                        autoReadjust = true;
                        resyncInt = 5;

                        // We have current DST determination, skip it below
                        useDST = false;

                        if(couldDST) allowedDiff = abs(tzDiff) + 1;

                        haveAuthTime = true;
                        lastAuthTime = millis();
                        authTimeExpired = false;

                        dt = myrtcnow();

                        // We have just adjusted, don't do it again below
                        lastYear = dt.year() - presentTime.getYearOffset();

                        #ifdef TC_DBG
                        Serial.println(F("time_loop: RTC re-adjusted using NTP or GPS"));
                        #endif

                        if(timeDifference) {
                            uint64_t newT = dateToMins(dt.year() - presentTime.getYearOffset(),
                                                    dt.month(), dt.day(), dt.hour(), dt.minute());
                            wasFakeRTC = (newT > oldT) ? (newT - oldT > allowedDiff) : (oldT - newT > allowedDiff);

                            // User played with RTC; return to actual present time
                            // (Allow a difference of 'allowedDiff' minutes to keep
                            // timeDifference in the event of a DST change)
                            if(wasFakeRTC) timeDifference = 0;
                        }

                        // Save to NVM if change is detected, or if RTC was way off
                        if( (presentTime.getYearOffset() != presentTime.loadYOffs()) || 
                            wasFakeRTC                                               ||
                            (presentTime.getDST() != presentTime.loadDST()) ) {
                            if(timetravelPersistent) {
                                presentTime.save();
                            } else {
                                presentTime.saveYOffs();
                            }
                        }

                    } else {

                        // No auth time at this point, do DST check below
                        useDST = couldDST;

                        #ifdef TC_DBG
                        Serial.println(F("time_loop: RTC re-adjustment via NTP/GPS failed"));
                        #endif

                    }

                } else {

                    // We have current DST determination, skip it below
                    useDST = false;

                }

            } else {
                
                autoReadjust = false;

                // If GPS is used for time, resyncInt is possibly set to 2 during boot;
                // reset it to 5 after 5 minutes since boot.
                if(resyncInt != 5) {
                    if(millis() - powerupMillis > 5*60*1000) {
                        resyncInt = 5;
                    }
                }

                // No auth time at this point, do DST check below
                useDST = couldDST;

            }

            // Detect and handle year changes

            {
                uint16_t thisYear = dt.year() - presentTime.getYearOffset();

                if( (thisYear != lastYear) || (thisYear > 9999) ) {

                    uint16_t rtcYear;
                    int16_t yOffs = 0;
    
                    if(thisYear > 9999) {
                        #ifdef TC_DBG
                        Serial.println(F("time_loop: Rollover 9999->1 detected"));
                        #endif

                        // We do not roll over to year "0".
                        // TZ calc doesn't cope with y0.
                        thisYear = 1;
    
                        if(timeDifference) {
                            // Correct timeDifference [r-o to 0: 5259492000]
                            if(timeDifference >= 5258964960) {
                                timeDifference -= 5258964960;
                            } else {
                                timeDifference = 5258964960 - timeDifference;
                                timeDiffUp = !timeDiffUp;
                            }
                        }
                    }
    
                    // Parse TZ and set up DST data for now current year
                    if(!(parseTZ(settings.timeZone, thisYear))) {
                        #ifdef TC_DBG
                        Serial.println(F("time_loop: [year change] Failed to parse TZ"));
                        #endif
                    }
    
                    // Get RTC-fit year & offs for real year
                    rtcYear = thisYear;
                    correctYr4RTC(rtcYear, yOffs);
    
                    // If year-translation changed, update RTC and save
                    if((rtcYear != dt.year()) || (yOffs != presentTime.getYearOffset())) {
    
                        // Update RTC
                        dt = myrtcnow();
                        rtc.adjust(dt.second(), 
                                   dt.minute(), 
                                   dt.hour(), 
                                   dayOfWeek(dt.day(), dt.month(), thisYear),
                                   dt.day(), 
                                   dt.month(), 
                                   rtcYear-2000
                        );
    
                        presentTime.setYearOffset(yOffs);
    
                        // Save YearOffs to NVM if change is detected
                        if(yOffs != presentTime.loadYOffs()) {
                            if(timetravelPersistent) {
                                presentTime.save();
                            } else {
                                presentTime.saveYOffs();
                            }
                        }

                        // Re-read RTC
                        dt = myrtcnow();
    
                    }
                }
            }

            // Check if we need to switch from/to DST

            if(useDST && !DSTcheckDone && ((dt.minute() % dstChkInt) == 0)) {

                int oldDST = presentTime.getDST();
                int currTimeMins = 0;
                int myDST = timeIsDST(dt.year() - presentTime.getYearOffset(), dt.month(), 
                                            dt.day(), dt.hour(), dt.minute(), currTimeMins);

                DSTcheckDone = true;
                dstChkInt = 5;

                if( (myDST != oldDST) &&
                    (!(blockDSTChange(currTimeMins))) ) {
                  
                    int nyear, nmonth, nday, nhour, nminute;
                    int myDiff = tzDiff;
                    uint64_t rtcTime;
                    uint16_t rtcYear;
                    int16_t  yOffs = 0;

                    #ifdef TC_DBG
                    Serial.printf("time_loop: DST change detected: %d -> %d\n", presentTime.getDST(), myDST);
                    #endif

                    presentTime.setDST(myDST);

                    // If DST status is -1 (unknown), do not adjust clock,
                    // only save corrected flag
                    if(oldDST >= 0) {

                        if(myDST == 0) myDiff *= -1;

                        dt = myrtcnow();
    
                        rtcTime = dateToMins(dt.year() - presentTime.getYearOffset(),
                                           dt.month(), dt.day(), dt.hour(), dt.minute());
    
                        rtcTime += myDiff;
    
                        minsToDate(rtcTime, nyear, nmonth, nday, nhour, nminute);
    
                        // Get RTC-fit year & offs for our real year
                        rtcYear = nyear;
                        correctYr4RTC(rtcYear, yOffs);
    
                        rtc.adjust(dt.second(), 
                                   nminute, 
                                   nhour, 
                                   dayOfWeek(nday, nmonth, nyear),
                                   nday, 
                                   nmonth, 
                                   rtcYear-2000
                        );
    
                        presentTime.setYearOffset(yOffs);

                    }

                    // Save yearOffset & isDTS to NVM
                    if(timetravelPersistent) {
                        presentTime.save();
                    } else {
                        presentTime.saveYOffs();
                    }

                    dt = myrtcnow();

                }
                
            } else {

                DSTcheckDone = false;
                
            }

            // Write time to presentTime display
            presentTime.setDateTimeDiff(dt);

            // Update "lastYear" and save to NVM
            lastYear = dt.year() - presentTime.getYearOffset();
            presentTime.saveLastYear(lastYear);

            // Debug log beacon
            #ifdef TC_DBG
            if((dt.second() == 0) && (dt.minute() != dbgLastMin)) {
                dbgLastMin = dt.minute();
                Serial.printf("%d[%d-(%d)]/%02d/%02d %02d:%02d:00 (Chip Temp %.2f) / WD of PT: %d (%d)\n",
                      lastYear, dt.year(), presentTime.getYearOffset(), dt.month(), dt.day(), dt.hour(), dbgLastMin,
                      rtc.getTemperature(),
                      dayOfWeek(presentTime.getDay(), presentTime.getMonth(), presentTime.getDisplayYear()),
                      dayOfWeek(dt.day(), dt.month(), dt.year() - presentTime.getYearOffset()));
            }
            #endif

            // Alarm, count-down timer, Sound-on-the-Hour

            {
                #ifdef TC_HAVELIGHT
                bool switchNMoff = false;
                #endif
                int compHour = alarmRTC ? dt.hour()   : presentTime.getHour();
                int compMin  = alarmRTC ? dt.minute() : presentTime.getMinute();
                int weekDay =  alarmRTC ? dayOfWeek(dt.day(), 
                                                    dt.month(), 
                                                    dt.year() - presentTime.getYearOffset())
                                          : 
                                          dayOfWeek(presentTime.getDay(), 
                                                    presentTime.getMonth(), 
                                                    presentTime.getDisplayYear());

                // Sound to play hourly (if available)
                // Follows setting for alarm as regards the option
                // of "real actual present time" vs whatever is currently
                // displayed on presentTime.

                if(compMin == 0) {
                    if(presentTime.getNightMode() ||
                       !FPBUnitIsOn ||
                       startup      ||
                       timeTraveled ||
                       timeTravelP0 ||
                       timeTravelP1 ||
                       (alarmOnOff && (alarmHour == compHour) && (alarmMinute == compMin))) {
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
                        if( (!(alarmOnOff && (alarmHour == compHour) && (alarmMinute == compMin))) ||
                            (alarmDone && checkAudioDone()) ) {
                            play_file("/timer.mp3", 1.0, false, true);
                            ctDown = 0;
                        }
                    }
                }

                // Handle alarm

                if(alarmOnOff) {
                    if((alarmHour == compHour) && (alarmMinute == compMin) && 
                                  (alarmWDmasks[alarmWeekday] & (1 << weekDay))) {
                        if(!alarmDone) {
                            play_file("/alarm.mp3", 1.0, false, true);
                            alarmDone = true;
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
                    if(compMin == 0 || forceReEvalANM) {
                        if(!autoNMDone || forceReEvalANM) {
                            uint32_t myField;
                            if(autoNightModeMode == 0) {
                                myField = autoNMdailyPreset;
                            } else {
                                const uint32_t *myFieldPtr = autoNMPresets[autoNightModeMode - 1];
                                myField = myFieldPtr[weekDay];
                            }                       
                            if(myField & (1 << (23 - compHour))) {
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

            // Handle Time Cycling ("decorative mode")

            // Do this on previous minute:59
            minNext = (dt.minute() == 59) ? 0 : dt.minute() + 1;

            // Check if autoPause has run out
            if(autoPaused && (millis() - pauseNow >= pauseDelay)) {
                autoPaused = false;
            }

            // Only do this on second 59, check if it's time to do so
            if(dt.second() == 59  &&
               (!autoPaused)      &&
               (!isRcMode())      &&
               autoTimeIntervals[autoInterval] &&
               (minNext % autoTimeIntervals[autoInterval] == 0)) {

                if(!autoIntDone) {

                    autoIntDone = true;

                    // cycle through pre-programmed times
                    autoTime++;
                    if(autoTime >= NUM_AUTOTIMES) autoTime = 0;

                    // Show a preset dest and departed time
                    destinationTime.setFromStruct(&destinationTimes[autoTime]);
                    departedTime.setFromStruct(&departedTimes[autoTime]);

                    allOff();

                    autoIntAnimRunning = 1;
                
                } 

            } else {

                autoIntDone = false;
                if(autoIntAnimRunning)
                    autoIntAnimRunning++;

            }

        } else {

            unsigned long millisNow = millis();

            destinationTime.setColon(false);
            presentTime.setColon(false);
            departedTime.setColon(false);

            // OTPR for PCF2129 every two weeks
            if(RTCNeedsOTPR && (millisNow - OTPRDoneNow > 2*7*24*60*60*1000)) {
                rtc.OTPRefresh(true);
                OTPRinProgress = true;
                OTPRStarted = millisNow;
            }

            if(autoIntAnimRunning)
                autoIntAnimRunning++;

            play_beep();

        }

        x = y;

        if(timeTravelP1 > 1) {
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
                    #ifdef IS_ACAR_DISPLAY
                    #define JAN011885 "010118851200"
                    #else
                    #define JAN011885 "JAN0118851200"
                    #endif
                    ((rand() % 10) < 4) ? destinationTime.showTextDirect(JAN011885) : destinationTime.show();
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
                    if(tt < 3)      { presentTime.setBrightnessDirect(4); presentTime.lampTest(); }
                    else if(tt < 7) { presentTime.show(); presentTime.on(); }
                    else            { presentTime.off(); }
                    tt = (rand() + millis()) % 10;
                    if(tt < 2)      { destinationTime.showTextDirect("8888888888888"); }
                    else if(tt < 6) { destinationTime.show(); destinationTime.on(); }
                    else            { if(!(ii % 2)) destinationTime.setBrightnessDirect(1+(rand() % 8)); }
                    tt = (tt + (rand() + millis())) % 10;
                    if(tt < 4)      { departedTime.setBrightnessDirect(4); departedTime.lampTest(); }
                    else if(tt < 7) { departedTime.showTextDirect("R 2 0 1 1 T R "); }
                    else            { departedTime.show(); }
                    mydelay(10);
                    
                    #if 0 // Code of death for some red displays, seems they don't like (real)LampTest
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
                
        } else if(!startup && !timeTraveled && FPBUnitIsOn) {

            #ifdef TC_HAVETEMP
            if(isRcMode()) {
                if(!specDisp) {
                    destinationTime.showTempDirect(tempSens.readLastTemp(), tempUnit);
                }
                if(tempSens.haveHum()) {
                    departedTime.showHumDirect(tempSens.readHum());
                } else {
                    departedTime.show();
                }
            } else {
            #endif
                if(!specDisp) {
                    destinationTime.show();
                }
                departedTime.show();
            #ifdef TC_HAVETEMP
            }
            #endif

            presentTime.show();
            
        }
    } 
}


/* Time Travel:
 *
 *  A time travel consists of various phases:
 *  - Speedo acceleration (if applicable; if GPS is in use, starts at actual speed)
 *  - actual tt (display disruption)
 *  - re-entry
 *  - Speed de-acceleration (if applicable; if GPS is in use, counts down to actual speed)
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
 *               |                             |                         |
 *          ETTO: Pulse                  TT_P1_POINT88                   |
 *               or                    (ms from P1 start)                |
 *           LOW->HIGH                                            ETTO: HIGH->LOW
 *
 *
 *  As regards the mere "time" logic, a time travel means to
 *  -) copy present time into departed time (where it freezes)
 *  -) copy destination time to present time (where it continues to run as a clock)
 *
 *  This is also called from tc_keypad.cpp
 */

void timeTravel(bool doComplete, bool withSpeedo)
{
    int   tyr = 0;
    int   tyroffs = 0;
    long  currTotDur = 0;
    unsigned long ttUnivNow = millis();

    pwrNeedFullNow();

    enableRcMode(false);

    cancelEnterAnim();
    cancelETTAnim();

    // Stop music if we are to play time travel sounds
    if(playTTsounds) mp_stop();

    // Pause autoInterval-cycling so user can play undisturbed
    pauseAuto();

    /*
     * Complete sequence with speedo: Initiate P0 (speed count-up)
     *
     */
    #ifdef TC_HAVESPEEDO
    if(doComplete && useSpeedo && withSpeedo) {

        timeTravelP0Speed = 0;
        timetravelP0Delay = 2000;
        triggerP1 = false;
        #ifdef EXTERNAL_TIMETRAVEL_OUT
        triggerETTO = ettoPulse = false;
        if(useETTO) ettoPulseEnd();
        #endif

        #ifdef TC_HAVEGPS
        if(useGPSSpeed) {
            int16_t tempSpeed = myGPS.getSpeed();
            if(tempSpeed >= 0) {
                timeTravelP0Speed = tempSpeed;
                timetravelP0Delay = 0;
                if(timeTravelP0Speed < 88) {
                    currTotDur = tt_p0_totDelays[timeTravelP0Speed];
                }
            }
        }
        #endif

        if(timeTravelP0Speed < 88) {

            // If time needed to reach 88mph is shorter than ettoLeadTime
            // or pointOfP1: Need add'l delay before speed counter kicks in.
            // This add'l delay is put into timetravelP0Delay.

            #ifdef EXTERNAL_TIMETRAVEL_OUT
            if(useETTO) {

                triggerETTO = true;
                triggerP1 = true;
                triggerETTONow = triggerP1Now = ttUnivNow;

                if(currTotDur >= ettoLeadPoint || currTotDur >= pointOfP1) {

                    if(currTotDur >= ettoLeadPoint && currTotDur >= pointOfP1) {

                        // Both outside our remaining time period:
                        if(ettoLeadPoint <= pointOfP1) {
                            // ETTO first:
                            triggerETTOLeadTime = 0;
                            triggerP1LeadTime = pointOfP1 - ettoLeadPoint;
                            timetravelP0Delay = currTotDur - ettoLeadPoint;
                        } else if(ettoLeadPoint > pointOfP1) {
                            // P1 first:
                            triggerP1LeadTime = 0;
                            triggerETTOLeadTime = ettoLeadPoint - pointOfP1;
                            timetravelP0Delay = currTotDur - pointOfP1;
                        }

                    } else if(currTotDur >= ettoLeadPoint) {

                        // etto outside
                        triggerETTOLeadTime = 0;
                        triggerP1LeadTime = pointOfP1 - ettoLeadPoint;
                        timetravelP0Delay = currTotDur - ettoLeadPoint;

                    } else {

                        // P1 outside
                        triggerP1LeadTime = 0;
                        triggerETTOLeadTime = ettoLeadPoint - pointOfP1;
                        timetravelP0Delay = currTotDur - pointOfP1;

                    }

                } else {

                    //triggerP1LeadTime = pointOfP1 - currTotDur;
                    //triggerETTOLeadTime = ettoLeadPoint - currTotDur;
                    //timetravelP0Delay = 0;
                    triggerP1LeadTime = pointOfP1 - currTotDur + timetravelP0Delay;
                    triggerETTOLeadTime = ettoLeadPoint - currTotDur + timetravelP0Delay;

                }

            } else
            #endif
            if(currTotDur >= pointOfP1) {
                 triggerP1 = true;
                 triggerP1Now = ttUnivNow;
                 triggerP1LeadTime = 0;
                 timetravelP0Delay = currTotDur - pointOfP1;
            } else {
                 triggerP1 = true;
                 triggerP1Now = ttUnivNow;
                 triggerP1LeadTime = pointOfP1 - currTotDur + timetravelP0Delay;
            }

            speedo.setSpeed(timeTravelP0Speed);
            speedo.setBrightness(255);
            speedo.show();
            speedo.on();
            timetravelP0Now = ttP0Now = ttUnivNow;
            timeTravelP0 = 1;
            timeTravelP2 = 0;

            return;
        }
        // If (actual) speed >= 88, trigger P1 immediately
    }
    #endif

    /*
     * Complete tt: Trigger P1 (display disruption)
     *
     */
    if(doComplete) {

        #ifdef EXTERNAL_TIMETRAVEL_OUT
        if(useETTO) {
            triggerP1 = true;
            triggerETTO = true;
            triggerETTONow = triggerP1Now = ttUnivNow;

            if(ettoLeadTime >= (TT_P1_POINT88 + TT_SNDLAT)) {
                triggerETTOLeadTime = 0;
                triggerP1LeadTime = ettoLeadTime - (TT_P1_POINT88 + TT_SNDLAT);
            } else {
                triggerP1LeadTime = 0;
                triggerETTOLeadTime = (TT_P1_POINT88 + TT_SNDLAT) - ettoLeadTime;
            }

            return;
        }
        #endif

        triggerLongTT();

        return;
    }

    /*
     * Re-entry part:
     *
     */

    timetravelNow = ttUnivNow;
    timeTraveled = true;

    if(playTTsounds) play_file("/timetravel.mp3", 1.0, true, true);

    allOff();

    // Copy present time to last time departed
    departedTime.setYear(presentTime.getDisplayYear());
    departedTime.setMonth(presentTime.getMonth());
    departedTime.setDay(presentTime.getDay());
    departedTime.setHour(presentTime.getHour());
    departedTime.setMinute(presentTime.getMinute());
    departedTime.setYearOffset(0);

    // We only save the new time to NVM if user wants persistence.
    // Might not be preferred; first, this messes with the user's custom
    // times. Secondly, it wears the flash memory.
    if(timetravelPersistent) {
        departedTime.save();
    }

    // Calculate time difference between RTC and destination time

    DateTime dt = myrtcnow();
    uint64_t rtcTime = dateToMins(dt.year() - presentTime.getYearOffset(),
                                  dt.month(),
                                  dt.day(),
                                  dt.hour(),
                                  dt.minute());

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

    // Save presentTime settings (timeDifference) if to be persistent
    if(timetravelPersistent) {
        presentTime.save();
    }

    // If speedo was used: Initiate P2: Count speed down
    #ifdef TC_HAVESPEEDO
    if(useSpeedo && (timeTravelP0Speed == 88)) {
        timeTravelP2 = 1;
        timetravelP0Now = ttUnivNow;
        timetravelP0Delay = 2000;
    }
    #endif

    // For external props: Signal Re-Entry by ending tigger signal
    #ifdef EXTERNAL_TIMETRAVEL_OUT
    if(useETTO) {
        ettoPulseEnd();
    }
    #endif
}

static void triggerLongTT()
{
    if(playTTsounds) play_file("/travelstart.mp3", 1.0, true, true);
    timetravelP1Now = millis();
    timetravelP1Delay = TT_P1_DELAY_P1;
    timeTravelP1 = 1;
}

#ifdef EXTERNAL_TIMETRAVEL_OUT
static void ettoPulseStart()
{
    digitalWrite(EXTERNAL_TIMETRAVEL_OUT_PIN, HIGH);
    #ifdef TC_DBG
    digitalWrite(WHITE_LED_PIN, HIGH);
    #endif
}
static void ettoPulseEnd()
{
    digitalWrite(EXTERNAL_TIMETRAVEL_OUT_PIN, LOW);
    #ifdef TC_DBG
    digitalWrite(WHITE_LED_PIN, LOW);
    #endif
}
#endif

/*
 * Reset present time to actual present time
 * (aka "return from time travel")
 */
void resetPresentTime()
{
    pwrNeedFullNow();

    timetravelNow = millis();
    timeTraveled = true;
    
    if(timeDifference && playTTsounds) {
        mp_stop();
        play_file("/timetravel.mp3", 1.0, true, true);
    }

    enableRcMode(false);

    cancelEnterAnim();
    cancelETTAnim();

    allOff();

    // Copy "present" time to last time departed
    departedTime.setYear(presentTime.getDisplayYear());
    departedTime.setMonth(presentTime.getMonth());
    departedTime.setDay(presentTime.getDay());
    departedTime.setHour(presentTime.getHour());
    departedTime.setMinute(presentTime.getMinute());
    departedTime.setYearOffset(0);

    // We only save the new time if user wants persistence.
    // Might not be preferred; first, this messes with the user's 
    // custom times. Secondly, it wears the flash memory.
    if(timetravelPersistent) {
        departedTime.save();
    }

    // Reset time, Yes, it's that simple.
    timeDifference = 0;

    // Save presentTime settings (timeDifference) if to be persistent
    if(timetravelPersistent) {
        presentTime.save();
    }
}

// Pause autoInverval-updating for 30 minutes
// Subsequent calls re-start the pause; therefore, it
// is not advised to use different pause durations
void pauseAuto(void)
{
    if(autoTimeIntervals[autoInterval]) {
        pauseDelay = 30 * 60 * 1000;
        autoPaused = true;
        pauseNow = millis();
        #ifdef TC_DBG
        Serial.println(F("pauseAuto: autoInterval paused for 30 minutes"));
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

/*
 * Callbacks for fake power switch
 */
#ifdef FAKE_POWER_ON
void fpbKeyPressed()
{
    isFPBKeyPressed = true;
    isFPBKeyChange = true;
}
void fpbKeyLongPressStop()
{
    isFPBKeyPressed = false;
    isFPBKeyChange = true;
}
#endif

/*
 * Delay function for keeping essential stuff alive
 * during Intro playback
 * 
 */
static void myIntroDelay(unsigned int mydel, bool withGPS)
{
    unsigned long startNow = millis();
    while(millis() - startNow < mydel) {
        delay(5);
        audio_loop();
        ntp_short_loop();
        #ifdef TC_HAVEGPS
        if(withGPS) gps_loop();
        #endif
    }
}

static void waitAudioDoneIntro()
{
    int timeout = 100;

    while(!checkAudioDone() && timeout--) {
        audio_loop();
        ntp_short_loop();
        #ifdef TC_HAVEGPS
        gps_loop();
        #endif
        delay(10);
    }
}

/*
 * Delay function for external modules
 * (gsp, temp sensor). 
 * Do not call gps_loop() here!
 */
static void myCustomDelay(unsigned int mydel)
{
    unsigned long startNow = millis();
    audio_loop();
    while(millis() - startNow < mydel) {
        delay(5);
        ntp_short_loop();
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
 * Internal replacement for RTC.now()
 * RTC sometimes loses sync and does not send data,
 * which is interpreted as 2165/165/165 etc
 * Check for this and retry in case.
 */
DateTime myrtcnow()
{
    DateTime dt = rtc.now();
    int retries = 0;

    while ((dt.month() < 1 || dt.month() > 12 ||
            dt.day()   < 1 || dt.day()   > 31 ||
            dt.hour() > 23 ||
            dt.minute() < 0 || dt.minute() > 59) &&
            retries < 30 ) {

            mydelay((retries < 5) ? 50 : 100);
            dt = rtc.now();
            retries++;
    }

    if(retries > 0) {
        Serial.printf("myrtcnow: %d retries needed to read RTC\n", retries);
    }

    return dt;
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

#ifdef TC_HAVETEMP
static void updateTemperature(bool force)
{
    if(!useTemp)
        return;
        
    if(force || (millis() - tempReadNow >= tempUpdInt)) {
        tempSens.readTemp(tempUnit);
        tempReadNow = millis();
    }
}
#endif

#ifdef TC_HAVESPEEDO
#ifdef TC_HAVEGPS
static void dispGPSSpeed(bool force)
{
    if(!useSpeedo || !useGPSSpeed)
        return;

    if(timeTraveled || timeTravelP0 || timeTravelP1 || timeTravelP2)
        return;

    if(force || (millis() - dispGPSnow >= 500)) {

        speedo.setSpeed(myGPS.getSpeed());
        speedo.show();
        speedo.on();
        dispGPSnow = millis();

    }
}
#endif
#ifdef TC_HAVETEMP
static void dispTemperature(bool force)
{
    bool tempNM = speedo.getNightMode();
    bool tempChgNM = false;

    if(!dispTemp)
        return;

    if(!FPBUnitIsOn || startup || timeTraveled || timeTravelP0 || timeTravelP1 || timeTravelP2)
        return;

    tempChgNM = (tempNM != tempOldNM);
    tempOldNM = tempNM;

    if(tempChgNM || force || (millis() - tempDispNow >= 2*60*1000)) {
        if(tempNM && tempOffNM) {
            speedo.off();
        } else {
            speedo.setTemperature(tempSens.readLastTemp());
            speedo.show();
            if(!tempNM) {
                speedo.setBrightnessDirect(tempBrightness); // after show b/c brightness
            }
            speedo.on();
        }
        tempDispNow = millis();
    }
}
#endif
#endif

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
 * Get time from NTP or GPS
 * 
 */
static bool getNTPOrGPSTime(bool weHaveAuthTime)
{
    // If GPS has a time and WiFi is off, try GPS first.
    // This avoids a frozen display when WiFi reconnects.
    #ifdef TC_HAVEGPS
    if(gpsHaveTime() && wifiIsOff) {
        if(getGPStime()) return true;
    }
    #endif

    // Now try NTP
    if(getNTPTime(weHaveAuthTime)) return true;

    // Again go for GPS, might have older timestamp
    #ifdef TC_HAVEGPS
    return getGPStime();
    #else
    return false;
    #endif
}

/*
 * Get time from NTP
 * 
 * Saves time to RTC; does not save YOffs/isDST to NVM.
 * 
 * Does no re-tries in case NTPGetLocalTime() fails; this is
 * called repeatedly within a certain time window, so we can 
 * retry with a later call.
 */
static bool getNTPTime(bool weHaveAuthTime)
{
    uint16_t newYear;
    int16_t  newYOffs = 0;

    if(settings.ntpServer[0] == 0) {
        #ifdef TC_DBG
        Serial.println(F("getNTPTime: NTP skipped, no server configured"));
        #endif
        return false;
    }

    pwrNeedFullNow();

    // Reconnect for only 3 mins, but not in AP mode; do not start CP.
    // If we don't have authTime yet, connect for longer to avoid
    // reconnects (aka frozen displays).
    // If WiFi is reconnected here, we won't have a valid time stamp
    // immediately. This will therefore fail the first time called
    wifiOn(weHaveAuthTime ? 3*60*1000 : 21*60*1000, false, true);    

    if(WiFi.status() == WL_CONNECTED) {

        int nyear, nmonth, nday, nhour, nmin, nsecond, nisDST;
                
        if(NTPGetLocalTime(nyear, nmonth, nday, nhour, nmin, nsecond, nisDST)) {
            
            // Get RTC-fit year plus offs for given real year
            newYear = nyear;
            correctYr4RTC(newYear, newYOffs);
    
            rtc.adjust(nsecond,
                       nmin,
                       nhour,
                       dayOfWeek(nday, nmonth, nyear),
                       nday,
                       nmonth,
                       newYear - 2000);
    
            presentTime.setYearOffset(newYOffs);
    
            // Parse TZ and set up DST data for now current year
            if((!couldDST) || (tzForYear != nyear)) {
                if(!(parseTZ(settings.timeZone, nyear))) {
                    #ifdef TC_DBG
                    Serial.println(F("getNTPTime: Failed to parse TZ"));
                    #endif
                }
            }

            handleDSTFlag(NULL, nisDST);
    
            #ifdef TC_DBG
            Serial.printf("getNTPTime: New time %d-%02d-%02d %02d:%02d:%02d DST: %d\n", 
                      nyear, nmonth, nday, nhour, nmin, nsecond, nisDST);
            #endif
    
            return true;

        } else {

            #ifdef TC_DBG
            Serial.println(F("getNTPTime: No current NTP timestamp available"));
            #endif

            return false;
        }

    } else {

        #ifdef TC_DBG
        Serial.println(F("getNTPTime: WiFi not connected, NTP time sync skipped"));
        #endif
        
        return false;

    }
}

/*
 * Get time from GPS
 * 
 * Saves time to RTC; does not save YOffs/isDST to NVM.
 * 
 */
#ifdef TC_HAVEGPS
static bool getGPStime()
{
    struct tm timeinfo;
    uint64_t utcMins;
    unsigned long stampAge = 0;
    int nyear, nmonth, nday, nhour, nminute, nsecond, isDST = 0;
    uint16_t newYear;
    int16_t  newYOffs = 0;
   
    if(!useGPS)
        return false;

    if(!myGPS.getDateTime(&timeinfo, &stampAge, GPSupdateFreq))
        return false;

    #ifdef TC_DBG
    Serial.printf("getGPStime: stamp age %d\n", stampAge);
    #endif
            
    // Convert UTC to local (non-DST) time
    utcMins = dateToMins(timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday, 
                         timeinfo.tm_hour, timeinfo.tm_min);

    // Correct seconds by stampAge
    nsecond = timeinfo.tm_sec + (stampAge / 1000);
    if((stampAge % 1000) > 500) nsecond++;
    
    utcMins += (uint64_t)(nsecond / 60);
    nsecond %= 60;

    utcMins -= tzDiffGMT;

    minsToDate(utcMins, nyear, nmonth, nday, nhour, nminute);

    // DST handling: Parse TZ and setup DST data for now current year
    if(!couldDST || tzForYear != nyear) {
        if(!(parseTZ(settings.timeZone, nyear))) {
            #ifdef TC_DBG
            Serial.println(F("getGPStime: Failed to parse TZ"));
            #endif
        }
    }

    // Determine DST from local nonDST time
    localToDST(nyear, nmonth, nday, nhour, nminute, isDST);

    // Get RTC-fit year & offs for given real year
    newYOffs = 0;
    newYear = nyear;
    correctYr4RTC(newYear, newYOffs);   

    rtc.adjust(nsecond,
               nminute,
               nhour,
               dayOfWeek(nday, nmonth, nyear),
               nday,
               nmonth,
               newYear - 2000);

    presentTime.setYearOffset(newYOffs);

    // Parse TZ and set up DST data for now current year
    if((!couldDST) || (tzForYear != nyear)) {
        if(!(parseTZ(settings.timeZone, nyear))) {
            #ifdef TC_DBG
            Serial.println(F("getGPStime: Failed to parse TZ"));
            #endif
        }
    }

    // Handle presentTime's DST flag
    handleDSTFlag(NULL, isDST);

    #ifdef TC_DBG
    Serial.printf("getGPStime: New time %d-%02d-%02d %02d:%02d:%02d DST: %d\n", 
                      nyear, nmonth, nday, nhour, nminute, nsecond, isDST);
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
    int nyear, nmonth, nday, nhour, nminute;
    uint64_t utcMins;

    if(!useGPS && !useGPSSpeed)
        return false;

    #ifdef TC_DBG
    Serial.println(F("setGPStime() called"));
    #endif

    DateTime dt = myrtcnow();

    // Convert local time to UTC
    utcMins = dateToMins(dt.year() - presentTime.getYearOffset(), dt.month(), dt.day(), 
                         dt.hour(), dt.minute());

    if(couldDST && presentTime.getDST() > 0)
        utcMins += tzDiffGMTDST;
    else
        utcMins += tzDiffGMT;
    
    minsToDate(utcMins, nyear, nmonth, nday, nhour, nminute);
    
    timeinfo.tm_year = nyear - 1900;
    timeinfo.tm_mon = nmonth - 1;
    timeinfo.tm_mday = nday;
    timeinfo.tm_hour = nhour;
    timeinfo.tm_min = nminute;
    timeinfo.tm_sec = dt.second();

    return myGPS.setDateTime(&timeinfo);
}

bool gpsHaveFix()
{
    if(!useGPS && !useGPSSpeed)
        return false;
        
    return myGPS.fix;
}

static bool gpsHaveTime()
{
    if(!useGPS && !useGPSSpeed)
        return false;
        
    return myGPS.haveTime();
}

/* Only for menu loop: 
 * Call GPS loop, but do not call
 * custom delay inside, ie no audio_loop()
 */
void gps_loop()
{
    if(!useGPS && !useGPSSpeed)
        return;

    if(millis() - lastLoopGPS > GPSupdateFreqMin) {
        lastLoopGPS = millis();
        myGPS.loop(false);
        #ifdef TC_HAVESPEEDO
        dispGPSSpeed(true);
        #endif
    }
}
#endif

/* 
 * Determine if provided year is a leap year 
 */
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
 *  Convert a date into "minutes since 1/1/0 0:0"
 */
uint64_t dateToMins(int year, int month, int day, int hour, int minute)
{
    uint64_t total64 = 0;
    uint32_t total32 = 0;
    int c = year, d = 0;        // ny0: d=1

    total32 = hours1kYears[year / 500];
    if(total32) d = (year / 500) * 500;

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

/*
 *  Convert "minutes since 1/1/0 0:0" into date
 */
void minsToDate(uint64_t total64, int& year, int& month, int& day, int& hour, int& minute)
{
    int c = 0, d = 19;    // ny0: c=1
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

uint32_t getHrs1KYrs(int index)
{
    return hours1kYears[index*2];
}

/*
 * Return doW from given date (year=yyyy)
 */
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
static char *parseDST(char *t, int& DSTyear, int& DSTmonth, int& DSTday, int& DSThour, int& DSTmin, int currYear)
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
static int mins2Date(int year, int month, int day, int hour, int mins)
{
    return ((((mon_yday[isLeapYear(year) ? 1 : 0][month - 1] + (day - 1)) * 24) + hour) * 60) + mins;
}

/*
 * Parse TZ string and setup DST data
 */
bool parseTZ(char *tz, int currYear, bool doparseDST)
{
    char *t, *u, *v;
    int diffNorm = 0;
    int diffDST = 0;
    int it;
    int DSTonYear, DSTonMonth, DSTonDay, DSTonHour, DSTonMinute;
    int DSToffYear, DSToffMonth, DSToffDay, DSToffHour, DSToffMinute;

    couldDST = false;
    tzForYear = 0;
    if(!tzDSTpart) {
        tzDiffGMT = tzDiffGMTDST = 0;
    }

    // 0) Basic validity check

    if(*tz == 0) return true;                         // Empty string. OK, no DST.

    if(!tzIsValid) {
        t = tz;
        while((t = strchr(t, '>'))) { t++; diffNorm++; }
        t = tz;
        while((t = strchr(t, '<'))) { t++; diffDST++; }
        if(diffNorm != diffDST) return false;         // Uneven < and >, string is bad. No DST.
        tzIsValid = true;
    }

    // 1) Find difference between nonDST and DST time

    if(!tzDSTpart) {

        diffNorm = diffDST = 0;
    
        // a. Skip TZ name and parse GMT-diff
        t = tz;
        if(*t == '<') {
           t = strchr(t, '>');
           if(!t) return false;                       // if <, but no >, string is bad. No DST.
           t++;
        } else {
           while(*t && *t != '-' && (*t < '0' || *t > '9')) {
              if(*t == ',') return false;
              t++;
           }
        }
        
        // t = start of diff to GMT
        if(*t != '-' && *t != '+' && (*t < '0' || *t > '9'))
            return false;                             // No numerical difference after name -> bad string. No DST.
    
        t = parseInt(t, it);
        if(it >= -24 && it <= 24) diffNorm = it * 60;
        else                      return false;       // Bad hr difference. No DST.
        
        if(*t == ':') {
            t++;
            u = parseInt(t, it);
            if(!u) return false;                      // No number following ":". Bad string. No DST.
            t = u;
            if(it >= 0 && it <= 59) {
                if(diffNorm < 0)  diffNorm -= it;
                else              diffNorm += it;
            } else return false;                      // Bad min difference. No DST.
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
           if(!t) return false;                       // if <, but no >, string is bad. No DST.
           t++;
        } else {
           while(*t && *t != ',' && *t != '-' && (*t < '0' || *t > '9'))
              t++;
        }
        
        // t = assumed start of DST-diff to GMT
        if(*t != '-' && *t != '+' && (*t < '0' || *t > '9')) {
            tzDiff = 60;                              // No numerical difference after name -> Assume 1 hr
        } else {
            t = parseInt(t, it);
            if(it >= -24 && it <= 24) diffDST = it * 60;
            else                      return false;   // Bad hr difference. No DST. Bad string.
            if(*t == ':') {
                t++;
                u = parseInt(t, it);
                if(!u) return false;                  // No number following ":". Bad string. No DST.
                t = u;
                if(it >= 0 && it <= 59) {
                    if(diffNorm < 0)  diffDST -= it;
                    else              diffDST += it;
                } else return false;                  // Bad min difference. No DST. Bad string.
                if(*t == ':') {
                    t++;
                    u = parseInt(t, it);
                    if(u) t = u;
                    // Ignore seconds
                }
            }
            tzDiff = abs(diffDST - diffNorm);
        }
    
        tzDiffGMT = diffNorm;
        tzDiffGMTDST = tzDiffGMT - tzDiff;
    
        tzDSTpart = t;

    } else {

        t = tzDSTpart;
      
    }

    if(*t == 0 || *t != ',' || !doparseDST) return true;    // No DST definition. No DST.
    t++;

    tzForYear = currYear;

    // 2) parse DST start

    v = t;
    u = parseDST(t, DSTonYear, DSTonMonth, DSTonDay, DSTonHour, DSTonMinute, currYear);
    if(!u) return false;

    // If start crosses end year (due to hour numbers >= 24), need to calculate 
    // for previous year (which then might be in current year). 
    // The same goes for the other direction vice versa.
    if(DSTonYear > currYear) {
        u = parseDST(v, DSTonYear, DSTonMonth, DSTonDay, DSTonHour, DSTonMinute, currYear-1);
        if(!u) return false;
    } else if(DSTonYear < currYear) {
        u = parseDST(v, DSTonYear, DSTonMonth, DSTonDay, DSTonHour, DSTonMinute, currYear+1);
        if(!u) return false;
    }
    
    t = u;

    if(*t == 0 || *t != ',') return false;          // Have start, but no end. Bad string. No DST.
    t++;

    // 3) parse DST end

    v = t;
    u = parseDST(t, DSToffYear, DSToffMonth, DSToffDay, DSToffHour, DSToffMinute, currYear);
    if(!u) return false;

    // See above
    if(DSToffYear > currYear) {
        u = parseDST(v, DSToffYear, DSToffMonth, DSToffDay, DSToffHour, DSToffMinute, currYear-1);
        if(!u) return false;
    } else if(DSToffYear < currYear) {
        u = parseDST(v, DSToffYear, DSToffMonth, DSToffDay, DSToffHour, DSToffMinute, currYear+1);
        if(!u) return false;
    }
    
    t = u;

    if((DSToffMonth == DSTonMonth) && (DSToffDay == DSTonDay)) {
        couldDST = false;
        #ifdef TC_DBG
        Serial.print(F("parseTZ: DST not used"));
        #endif
    } else {
        couldDST = true;

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
            DSTonMins = -1;
        else if(DSTonYear > currYear)
            DSTonMins = 600000;
        else 
            DSTonMins = mins2Date(currYear, DSTonMonth, DSTonDay, DSTonHour, DSTonMinute);
    
        if(DSToffYear < currYear)
            DSToffMins = -1;
        else if(DSToffYear > currYear)
            DSToffMins = 600000;
        else 
            DSToffMins = mins2Date(currYear, DSToffMonth, DSToffDay, DSToffHour, DSToffMinute);

        #ifdef TC_DBG
        Serial.printf("parseTZ: DST dates/times: %d/%d Start: %d-%02d-%02d/%02d:%02d End: %d-%02d-%02d/%02d:%02d\n",
                    tzDiffGMT, tzDiffGMTDST, 
                    DSTonYear, DSTonMonth, DSTonDay, DSTonHour, DSTonMinute,
                    DSToffYear, DSToffMonth, DSToffDay, DSToffHour, DSToffMinute);
        #endif

    }
        
    return true;
}

/*
 * Check if given local date/time is within DST period.
 * "Local" can be non-DST or DST; therefore a change possibly
 * needs to be blocked, see below.
 */
int timeIsDST(int year, int month, int day, int hour, int mins, int& currTimeMins)
{
    currTimeMins = mins2Date(year, month, day, hour, mins);

    if(DSTonMins < DSToffMins) {
        if((currTimeMins >= DSTonMins) && (currTimeMins < DSToffMins))
            return 1;
        else 
            return 0;
    } else {
        if((currTimeMins >= DSToffMins) && (currTimeMins < DSTonMins))
            return 0;
        else
            return 1;
    }
}

/*
 * Check if given currminutes fall in period where DST determination needs 
 * to be blocked because it would yield a wrong result. The elephant in the 
 * room here is the 'time-loop-hour' after switching to nonDST time, ie the 
 * period (where we already are on nonDST time) within tzDiff minutes ahead 
 * of a (would-be-repeated) change to nonDST time.
 * Block if getDST <= 0 because
 * - if 0, block because it has already been set, so block a reset to DST;
 * - if -1, block because determination not possible at this point.
 */
static bool blockDSTChange(int currTimeMins)
{
    return (
             (presentTime.getDST() <= 0) &&
             (currTimeMins < DSToffMins) &&
             (DSToffMins - currTimeMins <= abs(tzDiff))
           );
}

/*
 * Determine DST from non-DST local time. This is used for NTP and GPS, 
 * where we get UTC time, and have converted it to local nonDST time.
 * There is no need for blocking as there is no ambiguity.
 */
static void localToDST(int& year, int& month, int& day, int& hour, int& minute, int& isDST)
{
    if(couldDST) {
      
        // Get mins since 1/1 of current year in local (nonDST) time
        int currTimeMins = mins2Date(year, month, day, hour, minute);

        // Determine DST
        // DSTonMins is in non-DST time
        // DSToffMins is in DST time, so correct it for the comparison
        if(DSTonMins < DSToffMins) {
            if((currTimeMins >= DSTonMins) && (currTimeMins < (DSToffMins - abs(tzDiff))))
                isDST = 1;
            else 
                isDST = 0;
        } else {
            if((currTimeMins >= (DSToffMins - abs(tzDiff))) && (currTimeMins < DSTonMins))
                isDST = 0;
            else
                isDST = 1;
        }

        // Translate to DST local time
        if(isDST) {
            uint64_t myTime = dateToMins(year, month, day, hour, minute);

            myTime += abs(tzDiff);

            minsToDate(myTime, year, month, day, hour, minute);
        }
    }
}

/*
 * Evaluate DST flag from authoritative time source
 * and update presentTime's isDST flag
 */
static void handleDSTFlag(struct tm *ti, int nisDST)
{
    int myisDST = nisDST;
    
    if(ti) myisDST = ti->tm_isdst;
    
    if(presentTime.getDST() != myisDST) {
      
        int myDST = -1, currTimeMins = 0;
        #ifdef TC_DBG
        bool isAuth = false;
        #endif
        
        if(myisDST >= 0) {

            // If authority has a clear answer, use it.
            myDST = myisDST;
            if(myDST > 1) myDST = 1;    // Who knows....
            #ifdef TC_DBG
            isAuth = true;
            #endif
            
        } else if(couldDST && ti) {

            // If authority is unsure, try for ourselves
            myDST = timeIsDST(ti->tm_year + 1900, ti->tm_mon + 1, ti->tm_mday, 
                              ti->tm_hour, ti->tm_min, currTimeMins);
                              
            // If we are within block-period, set to current to prohibit a change
            if(blockDSTChange(currTimeMins)) myDST = presentTime.getDST();
             
        }
        
        if(presentTime.getDST() != myDST) {
            #ifdef TC_DBG
            Serial.printf("handleDSTFlag: Updating isDST from %d to %s%d\n", 
                  presentTime.getDST(), isAuth ? "[authoritative] " : "", myDST);
            #endif
            presentTime.setDST(myDST);
        }
        
    }
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
    unsigned long mymillis = millis();
    
    memset(NTPUDPBuf, 0, NTP_PACKET_SIZE);

    NTPUDPBuf[0] = B11100011; // LI, Version, Mode
    NTPUDPBuf[1] = 0;         // Stratum
    NTPUDPBuf[2] = 6;         // Polling Interval
    NTPUDPBuf[3] = 0xEC;      // Peer Clock Precision

    NTPUDPBuf[12] = 'T';      // Ref ID, anything
    NTPUDPBuf[13] = 'C';
    NTPUDPBuf[14] = 'D';
    NTPUDPBuf[15] = '1';

    // Transmit, use as id
    NTPUDPBuf[40] = NTPUDPID[0] = ((mymillis >> 24) & 0xff);      
    NTPUDPBuf[41] = NTPUDPID[1] = ((mymillis >> 16) & 0xff);
    NTPUDPBuf[42] = NTPUDPID[2] = ((mymillis >>  8) & 0xff);
    NTPUDPBuf[43] = NTPUDPID[3] = (mymillis         & 0xff);

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
    if(NTPUDPBuf[24] != NTPUDPID[0] || 
       NTPUDPBuf[25] != NTPUDPID[1] || 
       NTPUDPBuf[26] != NTPUDPID[2] || 
       NTPUDPBuf[27] != NTPUDPID[3]) {
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

// Get local time
static bool NTPGetLocalTime(int& year, int& month, int& day, int& hour, int& minute, int& second, int& isDST)
{
    uint32_t temp, c;

    // Fail if no time received, or stamp is older than 10 mins
    if(!NTPHaveLocalTime()) return false;

    isDST = 0;
    
    uint32_t secsSinceTCepoch = NTPGetCurrSecsSinceTCepoch();
    
    second = secsSinceTCepoch % 60;

    // Calculate minutes since 1/1/TCEpoch for local (nonDST) time zone
    uint32_t total32 = (secsSinceTCepoch / 60) - tzDiffGMT;

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

    temp = total32 / (24*60);
    day = temp + 1;
    total32 -= (temp * (24*60));

    temp = total32 / 60;
    hour = temp;

    minute = total32 - (temp * 60);

    // DST handling: Parse TZ and setup DST data for now current year
    if(!couldDST || tzForYear != year) {
        if(!(parseTZ(settings.timeZone, year))) {
            #ifdef TC_DBG
            Serial.println(F("NTPGetLocalTime: Failed to parse TZ"));
            #endif
        }
    }

    // Determine DST from local nonDST time
    localToDST(year, month, day, hour, minute, isDST);

    return true;
}

static bool NTPHaveLocalTime()
{
    // Have no time if no time stamp received, or stamp is older than 10 mins
    if((!NTPsecsSinceTCepoch) || ((millis() - NTPTSAge) > 10*60*1000)) 
        return false;

    return true;
}
