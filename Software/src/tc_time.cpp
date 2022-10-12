/*
 * -------------------------------------------------------------------
 * CircuitSetup.us Time Circuits Display
 * (C) 2021-2022 John deGlavina https://circuitsetup.us
 * (C) 2022 Thomas Winischhofer (A10001986)
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "tc_time.h"

bool autoIntDone = false;
bool autoReadjust = false;
bool alarmDone = false;
bool hourlySoundDone = false;
bool autoNMDone = false;
int8_t minNext;
bool haveAuthTime = false;
uint16_t lastYear = 0;

bool autoNightMode = false;
uint8_t autoNMOnHour = 0;
uint8_t autoNMOffHour = 0;

bool x;  // for tracking second changes
bool y;

// The startup sequence
bool startup              = false;
bool startupSound         = false;
unsigned long startupNow  = 0;

// Pause auto-time-cycling if user played with time travel
unsigned long pauseNow;
unsigned long pauseDelay = 30*60*1000;  // Pause for 30 minutes
bool          autoPaused = false;

// The timetravel sequence: Timers, triggers, state
int           timeTravelP0 = 0;
int           timeTravelP2 = 0;
#ifdef TC_HAVESPEEDO
bool          useSpeedo = false;
unsigned long timetravelP0Now = 0;
unsigned long timetravelP0Delay = 0;
unsigned long ttP0Now = 0;
uint8_t       timeTravelP0Speed = 0;
long          pointOfP1 = 0;
bool          didTriggerP1 = false;
double        ttP0TimeFactor = 1.0;
bool          useGPS = false;
bool          useTemp = false;
#ifdef TC_HAVEGPS
unsigned long dispGPSnow;
#endif
#ifdef TC_HAVETEMP
bool          tempUnit = false;
unsigned long tempReadNow;
int           tempBrightness = DEF_TEMP_BRIGHT;
#endif
#endif
unsigned long timetravelP1Now = 0;
unsigned long timetravelP1Delay = 0;
int           timeTravelP1 = 0;
bool          triggerP1 = false;
unsigned long triggerP1Now = 0;
long          triggerP1LeadTime = 0;
#ifdef EXTERNAL_TIMETRAVEL_OUT
bool          useETTO = true;
bool          ettoUsePulse = ETTO_USE_PULSE;
long          ettoLeadTime = ETTO_LEAD_TIME;
bool          triggerETTO = false;
long          triggerETTOLeadTime = 0;
unsigned long triggerETTONow = 0;
bool          ettoPulse = false;
unsigned long ettoPulseNow = 0;
long          ettoLeadPoint = 0;
#ifdef TC_DBG
unsigned long ettope, ettops;
#endif
#endif

// The timetravel re-entry sequence
unsigned long timetravelNow = 0;
bool          timeTraveled = false;

int           specDisp = 0;

// CPU power management
bool          pwrLow = false;
unsigned long pwrFullNow = 0;

// For displaying times off the real time
uint64_t timeDifference = 0;
bool     timeDiffUp = false;  // true = add difference, false = subtract difference

// Persistent time travels:
// This controls the app's behavior as regards saving times to the EEPROM.
// If this is true, times are saved to the EEPROM, whenever
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
bool alarmRTC = true;

// For tracking idle time in menus
uint8_t timeout = 0;

// For NTP/GPS
struct tm _timeinfo;
 
// The RTC object
tcRTC rtc(2, (uint8_t[2*2]){ DS3231_ADDR,  RTCT_DS3231, 
                             PCF2129_ADDR, RTCT_PCF2129 }); 

// The TC display objects
clockDisplay destinationTime(DEST_TIME_ADDR, DEST_TIME_PREF);
clockDisplay presentTime(PRES_TIME_ADDR, PRES_TIME_PREF);
clockDisplay departedTime(DEPT_TIME_ADDR, DEPT_TIME_PREF);

#ifdef TC_HAVESPEEDO
speedDisplay speedo(SPEEDO_ADDR);
#ifdef TC_HAVEGPS
tcGPS myGPS(GPS_ADDR);
#endif
#ifdef TC_HAVETEMP
tempSensor tempSens(TEMP_ADDR);
bool tempOldNM = false;
#endif
#endif

// Automatic times ("decorative mode")

int8_t autoTime = 0;  // Selects date/time from array below

dateStruct destinationTimes[NUM_AUTOTIMES] = {
    //YEAR, MONTH, DAY, HOUR, MIN
    {1985, 10, 26,  1, 21},   // Einstein 1:20 -> 1:21
    {1955, 11,  5,  6,  0},   // Marty -> 1955
    {1985, 10, 26,  1, 24},   // Marty -> 1985
    {2015, 10, 21, 16, 29},   // Marty, Doc, Jennifer -> 2015 (mins random)
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

#ifdef FAKE_POWER_ON
OneButton fakePowerOnKey = OneButton(FAKE_POWER_BUTTON_PIN,
    true,    // Button is active LOW
    true     // Enable internal pull-up resistor
);
bool isFPBKeyChange = false;
bool isFPBKeyPressed = false;
bool waitForFakePowerButton = false;
#endif

bool FPBUnitIsOn = true;

const uint8_t monthDays[] =
{
    31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31
};
const unsigned int mon_yday[2][13] =
{
    { 0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334, 365 },
    { 0, 31, 60, 91, 121, 152, 182, 213, 244, 274, 305, 335, 366 }
};
const unsigned int mon_ydayt24t60[2][13] =
{
    { 0, 31*24*60,  59*24*60,  90*24*60, 120*24*60, 151*24*60, 181*24*60, 
        212*24*60, 243*24*60, 273*24*60, 304*24*60, 334*24*60, 365*24*60 },
    { 0, 31*24*60,  60*24*60,  91*24*60, 121*24*60, 152*24*60, 182*24*60, 
        213*24*60, 244*24*60, 274*24*60, 305*24*60, 335*24*60, 366*24*60 }
};
const uint64_t mins1kYears[] =
{
             0,  262448640,  525422880,  788397120, 1051371360, 
    1314347040, 1577321280, 1840295520, 2103269760, 2366245440, 
    2629219680, 2892193920, 3155168160, 3418143840, 3681118080, 
    3944092320, 4207066560, 4470042240, 4733016480, 4995990720
};
const uint32_t hours1kYears[] =
{
                0,  262448640/60,  525422880/60,  788397120/60, 1051371360/60, 
    1314347040/60, 1577321280/60, 1840295520/60, 2103269760/60, 2366245440/60, 
    2629219680/60, 2892193920/60, 3155168160/60, 3418143840/60, 3681118080/60, 
    3944092320/60, 4207066560/60, 4470042240/60, 4733016480/60, 4995990720/60
};
#ifdef TC_HAVESPEEDO
const int16_t tt_p0_delays[88] =
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
long tt_p0_totDelays[88];
#endif

static void triggerLongTT();

#ifdef EXTERNAL_TIMETRAVEL_OUT
static void ettoPulseStart();
static void ettoPulseEnd();
#endif

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
}

/*
 * time_setup()
 *
 */
void time_setup()
{
    bool validLoad = true;
    bool rtcbad = false;

    // Power management: Set CPU speed
    // to maximum and start timer
    #ifdef TC_DBG
    Serial.print("Initial CPU speed is ");
    Serial.println(getCpuFrequencyMhz(), DEC);
    #endif
    pwrNeedFullNow(true);

    pinMode(SECONDS_IN_PIN, INPUT_PULLDOWN);  // for monitoring seconds

    pinMode(STATUS_LED_PIN, OUTPUT);          // Status LED

    #ifdef FAKE_POWER_ON
    waitForFakePowerButton = ((int)atoi(settings.fakePwrOn) > 0) ? true : false;

    if(waitForFakePowerButton) {
        fakePowerOnKey.setClickTicks(10);     // millisec after single click is assumed (default 400)
        fakePowerOnKey.setPressTicks(50);     // millisec after press is assumed (default 800)
        fakePowerOnKey.setDebounceTicks(50);  // millisec after safe click is assumed (default 50)
        fakePowerOnKey.attachLongPressStart(fpbKeyPressed);
        fakePowerOnKey.attachLongPressStop(fpbKeyLongPressStop);
    }
    #endif

    #ifdef EXTERNAL_TIMETRAVEL_OUT
    pinMode(EXTERNAL_TIMETRAVEL_OUT_PIN, OUTPUT);
    ettoPulseEnd();
    #endif

    // RTC setup
    if(!rtc.begin()) {

        Serial.println(F("time_setup: Couldn't find RTC. Panic!"));

        // Setup pins for white LED
        pinMode(WHITE_LED_PIN, OUTPUT);

        // Blink forever
        while (1) {
            digitalWrite(WHITE_LED_PIN, HIGH);
            delay(1000);
            digitalWrite(WHITE_LED_PIN, LOW);
            delay(1000);
        }

    }

    if(rtc.lostPower() && WiFi.status() != WL_CONNECTED) {

        // Lost power and battery didn't keep time, so set current time to compile time
        rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));

        Serial.println(F("time_setup: RTC lost power, setting default time. Change battery!"));

        rtcbad = true;

    }

    rtc.clockOutEnable();  // Turn on the 1Hz second output

    // Start the displays
    presentTime.begin();
    destinationTime.begin();
    departedTime.begin();

    // Initialize clock mode: 12 hour vs 24 hour
    bool mymode24 = (int)atoi(settings.mode24) ? true : false;
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
    timetravelPersistent = (int)atoi(settings.timesPers) ? true : false;

    // Load present time settings (yearOffs, timeDifference)
    presentTime.load((int)atoi(settings.presTimeBright));
    if(!timetravelPersistent) {
        timeDifference = 0;
    }

    if(rtcbad) {
        presentTime.setYearOffset(0);
        timeDifference = 0;
    }

    // Set RTC with NTP time
    if(getNTPTime()) {

        #ifdef TC_DBG
        Serial.print(F("time_setup: RTC set through NTP from "));
        Serial.println(settings.ntpServer);
        #endif

        // Save YearOffs to EEPROM if change is detected
        if(presentTime.getYearOffset() != presentTime.loadYOffs()) {
            presentTime.save();
        }

        // So we have authoritative time now
        haveAuthTime = true;

    }

    // Load destination time (and set to default if invalid)
    if(!destinationTime.load((int)atoi(settings.destTimeBright))) {
        validLoad = false;
        destinationTime.setYearOffset(0);
        destinationTime.setYear(destinationTimes[0].year);
        destinationTime.setMonth(destinationTimes[0].month);
        destinationTime.setDay(destinationTimes[0].day);
        destinationTime.setHour(destinationTimes[0].hour);
        destinationTime.setMinute(destinationTimes[0].minute);
        destinationTime.setBrightness((int)atoi(settings.destTimeBright));
        destinationTime.save();
    }

    // Load departed time (and set to default if invalid)
    if(!departedTime.load((int)atoi(settings.lastTimeBright))) {
        validLoad = false;
        departedTime.setYearOffset(0);
        departedTime.setYear(departedTimes[0].year);
        departedTime.setMonth(departedTimes[0].month);
        departedTime.setDay(departedTimes[0].day);
        departedTime.setHour(departedTimes[0].hour);
        departedTime.setMinute(departedTimes[0].minute);
        departedTime.setBrightness((int)atoi(settings.lastTimeBright));
        departedTime.save();
    }

    // Load autoInterval ("time rotation interval") from settings
    loadAutoInterval();

    // Load alarm from alarmconfig file
    // Don't care if data invalid, alarm is off in that case
    loadAlarm();

    // Auto-NightMode
    autoNMOnHour = (int)atoi(settings.autoNMOn);
    if(autoNMOnHour > 23) autoNMOnHour = 0;
    autoNMOffHour = (int)atoi(settings.autoNMOff);
    if(autoNMOffHour > 23) autoNMOffHour = 0;
    autoNightMode = (autoNMOnHour != autoNMOffHour);

    // If using auto times, load the first one
    if(autoTimeIntervals[autoInterval]) {
        destinationTime.setFromStruct(&destinationTimes[0]);
        departedTime.setFromStruct(&departedTimes[0]);
    }

    // Set up alarm base: RTC or current "present time"
    alarmRTC = ((int)atoi(settings.alarmRTC) > 0) ? true : false;

    // Set up speedo display, GPS receiver, Temp sensor
    #ifdef TC_HAVESPEEDO
    useSpeedo = ((int)atoi(settings.useSpeedo) > 0) ? true : false;
    if(useSpeedo) {
        speedo.begin((int)atoi(settings.speedoType));

        speedo.setBrightness((int)atoi(settings.speedoBright), true);
        speedo.setDot(true);

        // Speed factor for acceleration curve
        ttP0TimeFactor = (double)atof(settings.speedoFact);
        if(ttP0TimeFactor < 0.5) ttP0TimeFactor = 0.5;
        if(ttP0TimeFactor > 5.0) ttP0TimeFactor = 5.0;

        // Calculate start point of P1 sequence
        for(int i = 1; i < 88; i++) {
            pointOfP1 += (unsigned long)(((double)(tt_p0_delays[i])) / ttP0TimeFactor);
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
                totDelay += (long)(((double)(tt_p0_delays[i])) / ttP0TimeFactor);
                tt_p0_totDelays[i] = totDelay;
            }
        }

        speedo.off();

        #ifdef FAKE_POWER_ON
        if(!waitForFakePowerButton) {
            speedo.setSpeed(0);
            speedo.on();
            speedo.show();
        }
        #endif

        #ifdef TC_HAVEGPS
        useGPS = ((int)atoi(settings.useGPS) > 0) ? true : false;
        if(useGPS) {
            #ifdef TC_DBG
            Serial.println("myGPS.begin()");
            #endif
            if(myGPS.begin() || 1) {    // QQQ

                #ifdef TC_DBG
                Serial.println("myGPS.begin() success");
                #endif

                myGPS.setCustomDelayFunc(my2delay);

                // Set GPS RTC
                if(!rtcbad || haveAuthTime) {
                    setGPStime();
                    delay(30);
                }

                // display (actual) speed, regardless of fake power
                dispGPSSpeed(true);
                speedo.on();

            } else {
                useGPS = false;

                Serial.println("myGPS.begin() failed. Receiver not found.");
            }
        }
        #endif

        #ifdef TC_HAVETEMP
        useTemp = ((int)atoi(settings.useTemp) > 0) ? true : false;
        if(useTemp && !useGPS) {
            tempUnit = ((int)atoi(settings.tempUnit) > 0) ? true : false;
            if(tempSens.begin()) {
                tempSens.setCustomDelayFunc(my2delay);

                tempBrightness = (int)atoi(settings.tempBright);

                #ifdef FAKE_POWER_ON
                if(!waitForFakePowerButton) {
                    dispTemperature(true);
                }
                #endif
            } else {
                useTemp = false;
            }
        }
        #endif
    }
    #endif

    // Set up tt trigger for external props
    #ifdef EXTERNAL_TIMETRAVEL_OUT
    useETTO = ((int)atoi(settings.useETTO) > 0) ? true : false;
    #endif

    // Show "RESET" message if data loaded was invalid somehow
    if(!validLoad) {
        destinationTime.showOnlyText("RESET");
        destinationTime.on();
        delay(1000);
        allOff();
    }

    // Show "REPLACE BATTERY" message if RTC battery is low or depleted
    if(rtcbad || rtc.battLow()) {
        destinationTime.showOnlyText("REPLACE");
        presentTime.showOnlyText("BATTERY");
        destinationTime.on();
        presentTime.on();
        delay(3000);
        allOff();
    }

    if((int)atoi(settings.playIntro)) {
        const char *t1 = "             BACK";
        const char *t2 = "TO";
        const char *t3 = "THE FUTURE";
        int oldBriDest = destinationTime.getBrightness();
        int oldBriPres = presentTime.getBrightness();
        int oldBriDep = departedTime.getBrightness();

        play_file("/intro.mp3", 1.0, true, 0);

        my2delay(1200);
        destinationTime.setBrightness(15);
        presentTime.setBrightness(0);
        departedTime.setBrightness(0);
        presentTime.off();
        departedTime.off();
        destinationTime.showOnlyText(t1);
        presentTime.showOnlyText(t2);
        departedTime.showOnlyText(t3);
        destinationTime.on();
        for(int i = 0; i < 14; i++) {
           my2delay(50);
           destinationTime.showOnlyText(&t1[i]);
        }
        my2delay(500);
        presentTime.on();
        departedTime.on();
        for(int i = 0; i <= 15; i++) {
            presentTime.setBrightness(i);
            departedTime.setBrightness(i);
            my2delay(100);
        }
        my2delay(1500);
        for(int i = 15; i >= 0; i--) {
            destinationTime.setBrightness(i);
            presentTime.setBrightness(i);
            departedTime.setBrightness(i);
            my2delay(20);
        }
        allOff();
        destinationTime.setBrightness(oldBriDest);
        presentTime.setBrightness(oldBriPres);
        departedTime.setBrightness(oldBriDep);

        waitAudioDoneIntro();
        stopAudio();
    }

    // Load the time for initial animation show
    DateTime dt = myrtcnow();
    presentTime.setDateTimeDiff(dt);
    lastYear = dt.year() - presentTime.getYearOffset();

#ifdef FAKE_POWER_ON
    if(waitForFakePowerButton) {
        digitalWrite(WHITE_LED_PIN, HIGH);
        delay(500);
        digitalWrite(WHITE_LED_PIN, LOW);
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

    #ifdef TC_DBG
    Serial.println(F("time_setup: Done."));
    #endif

    //getGPStime(); // QQQ for testing
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
        fakePowerOnKey.tick();

        if(isFPBKeyChange) {
            if(isFPBKeyPressed) {
                if(!FPBUnitIsOn) {
                    startup = true;
                    startupSound = true;
                    FPBUnitIsOn = true;
                    destinationTime.setBrightness(255); // restore brightnesses
                    presentTime.setBrightness(255);     // in case we got switched
                    departedTime.setBrightness(255);    // off during time travel
                    #ifdef TC_HAVETEMP
                    dispTemperature(true);
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
                    cancelEnterAnim();
                    cancelETTAnim();
                    play_file("/shutdown.mp3", 1.0, true, 0);
                    mydelay(130);
                    allOff();
                    #ifdef TC_HAVESPEEDO
                    if(useSpeedo && !useGPS) speedo.off();
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
    if(!pwrLow &&
                  #ifdef TC_HAVEGPS
                  !useGPS &&
                  #endif
                             (wifiIsOff || wifiAPIsOff) && (millis() - pwrFullNow >= 5*60*1000)) {
        setCpuFrequencyMhz(80);
        pwrLow = true;

        #ifdef TC_DBG
        Serial.print(F("Reduced CPU speed to "));
        Serial.println(getCpuFrequencyMhz());
        #endif
    }

    // Initiate startup delay, play startup sound
    if(startupSound) {
        startupNow = millis();
        play_file("/startup.mp3", 1.0, true, 0);
        startupSound = false;
    }

    // Turn display on after startup delay
    if(startup && (millis() - startupNow >= STARTUP_DELAY)) {
        animate();
        startup = false;
        #ifdef TC_HAVESPEEDO
        if(useSpeedo && !useGPS) {
            speedo.off(); // Yes, off.
            #ifdef TC_HAVETEMP
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
            timetravelP0DelayT = (long)(((double)(tt_p0_delays[timeTravelP0Speed])) / ttP0TimeFactor) - ttP0LDOver;
            while(timetravelP0DelayT <= 0 && timeTravelP0Speed < 88) {
                timeTravelP0Speed++;
                timetravelP0DelayT += (long)(((double)(tt_p0_delays[timeTravelP0Speed])) / ttP0TimeFactor);
            }
        }

        if(timeTravelP0Speed < 88) {

            timetravelP0Delay = timetravelP0DelayT;
            timetravelP0Now = univNow;

        } else {

            timeTravelP0 = 0;

            #ifdef TC_DBG
            #ifdef EXTERNAL_TIMETRAVEL_OUT
            ettope = univNow;
            Serial.print(F("##### ETTO total elapsed lead time: "));
            Serial.println(ettope - ettops, DEC);
            #endif
            #endif
        }

        speedo.setSpeed(timeTravelP0Speed);
        speedo.show();
    }

    // Time travel animation, phase 2: Speed counts down
    if(timeTravelP2 && (millis() - timetravelP0Now >= timetravelP0Delay)) {
        if((timeTravelP0Speed == 0)
            #ifdef TC_HAVEGPS
                || (useGPS && (myGPS.getSpeed() >= 0) && (myGPS.getSpeed() >= timeTravelP0Speed))
            #endif
                                    ) {
            timeTravelP2 = 0;
            if(!useGPS) speedo.off();
            #ifdef TC_HAVEGPS
            dispGPSSpeed(true);
            #endif
            #ifdef TC_HAVETEMP
            dispTemperature(true);
            #endif
        } else {
            timeTravelP0Speed--;
            speedo.setSpeed(timeTravelP0Speed);
            speedo.show();
            timetravelP0Delay = (timeTravelP0Speed == 0) ? 4000 : 30;
            timetravelP0Now = millis();
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

    // Read and display GPS/temp

    #ifdef TC_HAVESPEEDO
    #ifdef TC_HAVEGPS
    if(useGPS) {
        myGPS.loop();
        dispGPSSpeed();
    }
    #endif
    #ifdef TC_HAVETEMP
    dispTemperature();
    #endif
    #endif

    // Actual clock stuff

    y = digitalRead(SECONDS_IN_PIN);
    if(y != x) {      // different on half second
        if(y == 0) {  // flash colon on half seconds, lit on start of second

            // First, handle colon
            destinationTime.setColon(true);
            presentTime.setColon(true);
            departedTime.setColon(true);

            // RTC display update

            DateTime dt = myrtcnow();

            // Re-adjust time periodically using NTP/GPS
            // (Do not interrupt sequences though)
            // If Wifi is in power-save, update only during night hours. (Reason: Re-connect
            // stalls display for a short while)
            // Do the update regardless of wifi and time if we have no authoritative time yet
            if( (!haveAuthTime || !wifiIsOff || (dt.hour() <= 6))   &&
                (dt.second() == 10 && dt.minute() == 1)             &&
                !timeTraveled && !timeTravelP0 && !timeTravelP1 && !timeTravelP2 ) {

                if(!autoReadjust) {

                    uint64_t oldT;

                    autoReadjust = true;

                    if(timeDifference) {
                        oldT = dateToMins(dt.year() - presentTime.getYearOffset(),
                                       dt.month(), dt.day(), dt.hour(), dt.minute());
                    }

                    if(getNTPOrGPSTime()) {

                        bool wasFakeRTC = false;

                        haveAuthTime = true;

                        dt = myrtcnow();

                        // We have just adjusted, don't do it again below
                        lastYear = dt.year() - presentTime.getYearOffset();

                        #ifdef TC_DBG
                        Serial.println(F("time_loop: RTC re-adjusted using NTP or GPS"));
                        #endif

                        if(timeDifference) {
                            uint64_t newT = dateToMins(dt.year() - presentTime.getYearOffset(),
                                                    dt.month(), dt.day(), dt.hour(), dt.minute());
                            wasFakeRTC = (newT > oldT) ? (newT - oldT > 30) : (oldT - newT > 30);

                            // User played with RTC; return to actual present time
                            if(wasFakeRTC) timeDifference = 0;
                        }

                        // Save to EEPROM if change is detected, or if RTC was way off
                        if( (presentTime.getYearOffset() != presentTime.loadYOffs()) || wasFakeRTC ) {
                            if(timetravelPersistent) {
                                presentTime.save();
                            } else {
                                presentTime.saveYOffs();
                            }
                        }

                    } else {

                        Serial.println(F("time_loop: RTC re-adjustment via NTP/GPS failed"));

                    }
                }

            } else {

                autoReadjust = false;

            }

            if(dt.year() - presentTime.getYearOffset() > 9999) {

                // RTC(+yearOffs) year 9999 -> 1 roll-over

                #ifdef TC_DBG
                Serial.println(F("Rollover 9999->1 detected, adjusting RTC"));
                #endif

                if(timeDifference) {

                    // Correct timeDifference
                    timeDifference = 5255474400 - timeDifference;
                    timeDiffUp = !timeDiffUp;

                }

                uint16_t newYear = 1;
                int16_t yOffs = 0;

                // Get RTC-fit year plus offs for given real year
                correctYr4RTC(newYear, yOffs);
                
                presentTime.setYearOffset(yOffs);

                // Update RTC
                dt = myrtcnow();
                rtc.adjust(dt.second(), 
                           dt.minute(), 
                           dt.hour(), 
                           dayOfWeek(dt.day(), dt.month(), 1),
                           dt.day(), 
                           dt.month(), 
                           newYear-2000
                );
                  
                // Save YearOffs to EEPROM
                if(timetravelPersistent) {
                    presentTime.save();
                } else {
                    presentTime.saveYOffs();
                }

                // Re-read RTC
                dt = myrtcnow();

            } else if(dt.year() - presentTime.getYearOffset() != lastYear) {

                // Any year change
              
                uint16_t newYear, realYear = dt.year() - presentTime.getYearOffset();
                int16_t yOffs = 0;

                newYear = realYear;

                // Get RTC-fit year plus offs for given real year
                correctYr4RTC(newYear, yOffs);

                // If year-translation changed, update RTC and save
                if((newYear != realYear) || (yOffs != presentTime.getYearOffset())) {
                
                    presentTime.setYearOffset(yOffs);

                    // Update RTC
                    dt = myrtcnow();
                    rtc.adjust(dt.second(), 
                               dt.minute(), 
                               dt.hour(), 
                               dayOfWeek(dt.day(), dt.month(), realYear),
                               dt.day(), 
                               dt.month(), 
                               newYear-2000
                    );

                    // Save YearOffs to EEPROM if change is detected
                    if(presentTime.getYearOffset() != presentTime.loadYOffs()) {
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

            // Write time to presentTime display
            presentTime.setDateTimeDiff(dt);

            lastYear = dt.year() - presentTime.getYearOffset();

            // Logging beacon
            #ifdef TC_DBG
            if((dt.second() == 0) && (dt.minute() != dbgLastMin)) {
                int ttt;
                Serial.print(dt.year());
                Serial.print(F("/"));
                Serial.print(dt.month());
                Serial.print(F(" "));
                dbgLastMin = dt.minute();
                Serial.print(dbgLastMin);
                Serial.print(F("."));
                Serial.print(dt.second());
                Serial.print(F(" "));
                Serial.println(rtc.getTemperature());
                ttt = dayOfWeek(presentTime.getDay(), presentTime.getMonth(), presentTime.getDisplayYear());
                Serial.print("Weekday of current presentTime: ");
                Serial.println(ttt);
            }
            #endif

            {
                int compHour = alarmRTC ? dt.hour()   : presentTime.getHour();
                int compMin  = alarmRTC ? dt.minute() : presentTime.getMinute();
                int weekDay =  alarmRTC ? dayOfWeek(dt.year() - presentTime.getYearOffset(), 
                                                    dt.month(), 
                                                    dt.day()) 
                                          : 
                                          dayOfWeek(presentTime.getDisplayYear(), 
                                                    presentTime.getMonth(), 
                                                    presentTime.getDay());

                // Sound to play hourly (if available)
                // Follows setting for alarm as regards the options
                // of "real actual present time" vs whatever is currently
                // displayed on presentTime.

                if(compMin == 0) {
                    if(presentTime.getNightMode() ||
                       !FPBUnitIsOn ||
                       startup ||
                       timeTraveled ||
                       timeTravelP0 ||
                       timeTravelP1 ||
                       (alarmOnOff && (alarmHour == compHour) && (alarmMinute == compMin))) {
                        hourlySoundDone = true;
                    }
                    if(!hourlySoundDone) {
                        play_file("/hour.mp3", 1.0, false, 0);
                        hourlySoundDone = true;
                    }
                } else {
                    hourlySoundDone = false;
                }

                // Handle alarm

                if(alarmOnOff) {
                    if((alarmHour == compHour) && (alarmMinute == compMin) ) {
                        if(!alarmDone) {
                            play_file("/alarm.mp3", 1.0, false, 0);
                            alarmDone = true;
                        }
                    } else {
                        alarmDone = false;
                    }
                }

                // Handle Auto-NightMode

                if(autoNightMode) {
                    if((autoNMOnHour == compHour) && (compMin == 0)) {
                        if(!autoNMDone) {
                            nightModeOn();
                            autoNMDone = true;
                        }
                    } else if((autoNMOffHour == compHour) && (compMin == 0)) {
                        if(!autoNMDone) {
                            nightModeOff();
                            autoNMDone = true;
                        }
                    } else {
                        autoNMDone = false;
                    }
                }

            }

            // Handle autoInterval

            // Do this on previous minute:59
            minNext = (dt.minute() == 59) ? 0 : dt.minute() + 1;

            // End autoPause if run out
            if(autoPaused && (millis() - pauseNow >= pauseDelay)) {
                autoPaused = false;
            }

            // Only do this on second 59, check if it's time to do so
            if(dt.second() == 59  &&
               (!autoPaused)      &&
               autoTimeIntervals[autoInterval] &&
               (minNext % autoTimeIntervals[autoInterval] == 0)) {

                if(!autoIntDone) {

                    #ifdef TC_DBG
                    Serial.println(F("time_loop: autoInterval"));
                    #endif

                    autoIntDone = true;     // Already did this, don't repeat

                    // cycle through pre-programmed times
                    autoTime++;
                    if(autoTime >= NUM_AUTOTIMES) {
                        autoTime = 0;
                    }

                    // Show a preset dest and departed time
                    destinationTime.setFromStruct(&destinationTimes[autoTime]);
                    departedTime.setFromStruct(&departedTimes[autoTime]);

                    allOff();

                    // Blank on second 59, display when new minute begins
                    while(digitalRead(SECONDS_IN_PIN) == 0) {  // wait for the complete of this half second
                                                           // Wait for this half second to end
                        myloop();
                    }
                    while(digitalRead(SECONDS_IN_PIN) == 1) {  // second on next low
                                                           // Wait for the other half to end
                        myloop();
                    }

                    #ifdef TC_DBG
                    Serial.println(F("time_loop: Update Present Time"));
                    #endif

                    dt = myrtcnow();                  // New time by now
                    presentTime.setDateTimeDiff(dt);  // will be at next minute

                    if(FPBUnitIsOn) animate();        // show all with month showing last
                }

            } else {

                autoIntDone = false;

            }

        } else {

            destinationTime.setColon(false);
            presentTime.setColon(false);
            departedTime.setColon(false);

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
                    ((rand() % 10) < 5) ? destinationTime.showOnlyText("MALFUNCTION") : destinationTime.show();
                    destinationTime.setBrightnessDirect((1+(rand() % 10)) & 0x0a);
                    presentTime.setBrightnessDirect((1+(rand() % 10)) & 0x0b);
                    ((rand() % 10) < 3) ? departedTime.showOnlyText(">ACS2011GIDUW") : departedTime.show();
                    departedTime.setBrightnessDirect((1+(rand() % 10)) & 0x07);
                    mydelay(20);
                }
                //allOff();
                break;
            case 5:
                departedTime.setBrightness(255);
                while(ii--) {
                    tt = rand() % 10;
                    presentTime.setBrightnessDirect(1+(rand() % 8));
                    if(tt < 3)      { presentTime.lampTest(); }
                    else if(tt < 7) { presentTime.show(); presentTime.on(); }
                    else            { presentTime.off(); }
                    tt = (rand() + millis()) % 10;
                    if(tt < 2)      { destinationTime.lampTest(); }
                    else if(tt < 6) { destinationTime.show(); destinationTime.on(); }
                    else            { destinationTime.setBrightnessDirect(1+(rand() % 8)); }
                    tt = (rand() + millis()) % 10;
                    if(tt < 4)      { departedTime.lampTest(); }
                    else if(tt < 8) { departedTime.showOnlyText("00000000000000"); departedTime.on(); }
                    else            { departedTime.off(); }
                    mydelay(10);
                }
                break;
            default:
                allOff();
            }
        }

        if(!startup && !timeTraveled && (timeTravelP1 <= 1) && FPBUnitIsOn) {
            if(!specDisp) {
                destinationTime.show();
            }
            presentTime.show();
            departedTime.show();
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
 *  |<---------- P0: speedo accleration ------>|                         |<P2:speedo de-accleration>|
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

    cancelEnterAnim();
    cancelETTAnim();

    // Pause autoInterval-cycling so user can play undisturbed
    pauseAuto();

    /*
     * Complete sequence with speedo: Initiate P0 (speed count-up)
     *
     */
    #ifdef TC_HAVESPEEDO
    if(doComplete && useSpeedo && withSpeedo) {

        #ifdef TC_DBG
        Serial.println("timeTravel(): Using speedo");
        #endif

        timeTravelP0Speed = 0;
        timetravelP0Delay = 2000;
        triggerP1 = false;
        #ifdef EXTERNAL_TIMETRAVEL_OUT
        triggerETTO = ettoPulse = false;
        if(useETTO) ettoPulseEnd();
        #endif

        #ifdef TC_HAVEGPS
        if(useGPS) {
            int16_t tempSpeed = myGPS.getSpeed();
            if(tempSpeed > 0) {
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

                    triggerP1LeadTime = pointOfP1 - currTotDur;
                    triggerETTOLeadTime = ettoLeadPoint - currTotDur;
                    timetravelP0Delay = 0;

                }

            } else
            #endif
            if(currTotDur >= pointOfP1) {
                 triggerP1 = true;
                 triggerP1Now = ttUnivNow;
                 triggerP1LeadTime = 0;
                 timetravelP0Delay = currTotDur - pointOfP1;
            }

            speedo.setSpeed(timeTravelP0Speed);
            speedo.setBrightness(255);
            speedo.show();
            speedo.on();
            timetravelP0Now =  ttP0Now = ttUnivNow;
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

    play_file("/timetravel.mp3", 1.0, true, 0);

    allOff();

    // Copy present time to last time departed
    departedTime.setYear(presentTime.getDisplayYear());
    departedTime.setMonth(presentTime.getMonth());
    departedTime.setDay(presentTime.getDay());
    departedTime.setHour(presentTime.getHour());
    departedTime.setMinute(presentTime.getMinute());
    departedTime.setYearOffset(0);

    // We only save the new time to the EEPROM if user wants persistence.
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

    #ifdef TC_DBG
    Serial.println(F("timeTravel: Re-entry successful, welcome back!"));
    #endif
}

static void triggerLongTT()
{
    play_file("/travelstart.mp3", 1.0, true, 0);
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
    ettops = millis();
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
    if(timeDifference) {
        play_file("/timetravel.mp3", 1.0, true, 0);
    }

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

    // We only save the new time to the EEPROM if user wants persistence.
    // Might not be preferred; first, this messes with the user's custom
    // times. Secondly, it wears the flash memory.
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

// choose your time zone from this list
// https://github.com/nayarsystems/posix_tz_db/blob/master/zones.csv

/* ATTN:
 *  DateTime uses 1-12 for months
 *  tm (_timeinfo) uses 0-11 for months
 *  Hardware RTC uses 1-12 for months
 *  Xprintf treats "%B" substition as from timeinfo, hence 0-11
 */

/*
 * Get time from NTP or GPS.
 * Try NTP first.
 */
bool getNTPOrGPSTime()
{
    if(getNTPTime()) return true;
    #ifdef TC_HAVEGPS
    return getGPStime();
    #else
    return false;
    #endif
}

/*
 * Get time from NTP
 * (Saves time to RTC)
 */
bool getNTPTime()
{
    uint16_t newYear;
    int16_t  newYOffs = 0;

    pwrNeedFullNow();
    wifiOn(2*60*1000, false);    // reconnect for only 2 mins, not in AP mode

    if(WiFi.status() == WL_CONNECTED) {

        // if connected to wifi, get NTP time and set RTC

        configTime(0, 0, settings.ntpServer);

        setenv("TZ", settings.timeZone, 1);   // Set environment variable with time zone
        tzset();

        if(strlen(settings.ntpServer) == 0) {
            #ifdef TC_DBG
            Serial.println(F("getNTPTime: NTP skipped, no server configured"));
            #endif
            return false;
        }

        int ntpRetries = 0;
        if(!getLocalTime(&_timeinfo)) {

            while(!getLocalTime(&_timeinfo)) {
                if(ntpRetries >= 20) {
                    Serial.println(F("getNTPTime: Couldn't get NTP time"));
                    return false;
                } else {
                    ntpRetries++;
                }
                mydelay((ntpRetries >= 3) ? 300 : 50);
            }

        }

        // Don't waste any time here...

        // Timeinfo:  Years since 1900
        // RTC:       0-99, 0 being 2000
        //            (important for leap year compensation, which only works from 2000-2099, not 2100 on,
        //            century bit has not influence on leap year comp., is buggy)

        newYear = _timeinfo.tm_year + 1900;

        // Get RTC-fit year plus offs for given real year
        correctYr4RTC(newYear, newYOffs);

        presentTime.setYearOffset(newYOffs);

        rtc.adjust(_timeinfo.tm_sec,
                   _timeinfo.tm_min,
                   _timeinfo.tm_hour,
                   dayOfWeek(_timeinfo.tm_mday, _timeinfo.tm_mon + 1, _timeinfo.tm_year + 1900),
                   _timeinfo.tm_mday,
                   _timeinfo.tm_mon + 1,   // Month needs to be 1-12, timeinfo uses 0-11
                   newYear - 2000);

        #ifdef TC_DBG
        Serial.print(F("getNTPTime: Result from NTP update: "));
        Serial.println(&_timeinfo, "%A, %B %d %Y %H:%M:%S");
        #endif

        return true;

    } else {

        Serial.println(F("getNTPTime: WiFi not connected, NTP time sync skipped"));
        return false;

    }
}

/*
 * Get time from GPS
 * (Saves time to RTC)
 */
#ifdef TC_HAVEGPS
bool getGPStime()
{
    struct tm timeinfo;
    time_t epoch_time;
    char *tz;
    uint16_t newYear, realYear;
    int16_t  newYOffs = 0;
    time_t stampAge = 0;

    if(!useGPS)
        return false;

    if(!myGPS.getDateTime(&timeinfo, &stampAge))
        return false;

    newYear = timeinfo.tm_year + 1900;
    while(newYear >= 2038) {
        newYear -= 28;
        newYOffs += 28;
    }
    timeinfo.tm_year = newYear - 1900;

    setenv("TZ", "UTC0", 1);
    tzset();

    epoch_time = mktime(&timeinfo);
    epoch_time += (stampAge / 1000);

    setenv("TZ", settings.timeZone, 1);
    tzset();

    struct tm *ti = localtime_r(&epoch_time, &timeinfo);

    realYear = newYear = ti->tm_year + newYOffs + 1900;
    
    newYOffs = 0;

    // Get RTC-fit year plus offs for given real year
    correctYr4RTC(newYear, newYOffs);

    presentTime.setYearOffset(newYOffs);

    rtc.adjust(ti->tm_sec,
               ti->tm_min,
               ti->tm_hour,
               dayOfWeek(ti->tm_mday, ti->tm_mon + 1, realYear),
               ti->tm_mday,
               ti->tm_mon + 1,    // Month needs to be 1-12, timeinfo uses 0-11
               newYear - 2000);

    #ifdef TC_DBG
    Serial.print(F("getGPSTime: GPS time: "));
    Serial.println(ti, "%A, %B %d %Y %H:%M:%S");
    Serial.println(F("getGPSTime: GPS time sync successful"));
    #endif
        
    return true;
}

/*
 * Set GPS's own RTC
 * This is supposed to speed up finding a fix
 */
bool setGPStime()
{
    struct tm timeinfo;
    time_t epoch_time;
    int newYear, yearOffs = 0;

    if(!useGPS)
        return false;

    #ifdef TC_DBG
    Serial.println(F("setGPSTime() called"));
    #endif

    DateTime dt = myrtcnow();

    newYear = dt.year() - presentTime.getYearOffset();
    while(newYear >= 2038) {
        newYear -= 28;
        yearOffs += 28;
    }
    timeinfo.tm_year = newYear - 1900;
    timeinfo.tm_mon = dt.month() - 1;
    timeinfo.tm_mday = dt.day();
    timeinfo.tm_hour = dt.hour();
    timeinfo.tm_min = dt.minute();
    timeinfo.tm_sec = dt.second();

    epoch_time = mktime(&timeinfo);

    struct tm *ti = gmtime_r(&epoch_time, &timeinfo);
    timeinfo.tm_year += yearOffs;

    return myGPS.setDateTime(ti);
}
#endif

/*
 * Call this frequently while waiting for button press,
 * increments timeout each second, returns true when maxtime reached.
 *
 */
bool checkTimeOut()
{
    y = digitalRead(SECONDS_IN_PIN);
    if(x != y) {
        x = y;
        if(y == 0) {
            timeout++;
        }
    }

    if(timeout >= maxTime) {
        return true;
    }

    return false;
}

// Determine if provided year is a leap year.
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

// Find number of days in a month
int daysInMonth(int month, int year)
{
    if(month == 2 && isLeapYear(year)) {
        return 29;
    }
    return monthDays[month - 1];
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
        Serial.print(F("myrtcnow: "));
        Serial.print(retries, DEC);
        Serial.println(F(" retries needed to read RTC"));
    }

    return dt;
}

/*
 *  Convert a date into "minutes since 1/1/1 0:0"
 */
uint64_t dateToMins(int year, int month, int day, int hour, int minute)
{
    uint64_t total64 = 0;
    uint32_t total32 = 0;
    int c = year, d = 1;

    total32 = hours1kYears[year / 500];   //1000];
    if(total32) d = (year / 500) * 500;   //1000) * 1000;

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
 *  Convert "minutes since 1/1/1 0:0" into date
 */
void minsToDate(uint64_t total64, int& year, int& month, int& day, int& hour, int& minute)
{
    int c = 1, d = 19; // 9
    int temp;
    uint32_t total32;

    year = month = day = 1;
    hour = 0;
    minute = 0;

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
    if(m < 3) y -= 1;
    return (y + y/4 - y/100 + y/400 + t[m-1] + d) % 7;
}

/*
 * Return RTC-fit year plus offs for given real year
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

void my2delay(unsigned int mydel)
{
    unsigned long startNow = millis();
    while(millis() - startNow < mydel) {
        delay(5);
        audio_loop();
    }
}

void waitAudioDoneIntro()
{
    int timeout = 100;

    while(!checkAudioDone() && timeout--) {
         audio_loop();
         delay(10);
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

#ifdef TC_HAVESPEEDO
#ifdef TC_HAVEGPS
void dispGPSSpeed(bool force)
{
    int16_t myspeed;

    if(!useSpeedo || !useGPS)
        return;

    if(timeTraveled || timeTravelP0 || timeTravelP1 || timeTravelP2)
        return;

    if(force || (millis() - dispGPSnow >= 500)) {

        myspeed = myGPS.getSpeed();
        if(myspeed < 0) {
            speedo.setText("--");
        } else if(myspeed > 99) {
            speedo.setText("HI");
        } else {
            speedo.setSpeed(myspeed);
        }
        speedo.show();
        speedo.on();
        dispGPSnow = millis();

    }
}
#endif
#ifdef TC_HAVETEMP
void dispTemperature(bool force)
{
    int temp;

    if(!useSpeedo || !useTemp || useGPS)
        return;

    if(!FPBUnitIsOn || startup || timeTraveled || timeTravelP0 || timeTravelP1 || timeTravelP2)
        return;

    if(speedo.getNightMode()) {
        speedo.off();
        tempOldNM = true;
        return;
    }

    if(tempOldNM || (force || (millis() - tempReadNow >= 2*60*1000))) {
        tempOldNM = false;
        tempSens.on();
        speedo.setTemperature(tempSens.readTemp(tempUnit));
        speedo.show();
        speedo.setBrightnessDirect(tempBrightness); // after show b/c brightness
        speedo.on();
        tempSens.off();
        tempReadNow = millis();
    }

}
#endif
#endif
