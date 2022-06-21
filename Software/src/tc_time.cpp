/*
 * -------------------------------------------------------------------
 * CircuitSetup.us Time Circuits Display
 * Code adapted from Marmoset Electronics 
 * https://www.marmosetelectronics.com/time-circuits-clock
 * by John Monaco
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

bool autoTrack = false;
int8_t minPrev;  // track previous minute

bool x;  // for tracking second change
bool y;  // for tracking second change

bool startup = false;
bool startupSound = false;
long startupNow = 0;
int startupDelay = 1700; //the time between startup sound being played and the display coming on

struct tm _timeinfo;  //for NTP
RTC_DS3231 rtc;       //for RTC IC

long timetravelNow = 0;
bool timeTraveled = false;

// The displays
clockDisplay destinationTime(DEST_TIME_ADDR, DEST_TIME_PREF);
clockDisplay presentTime(PRES_TIME_ADDR, PRES_TIME_PREF);
clockDisplay departedTime(DEPT_TIME_ADDR, DEPT_TIME_PREF);

// Automatic times
dateStruct destinationTimes[8] = {
    //YEAR, MONTH, DAY, HOUR, MIN
    {1985, 10, 26, 1, 21},
    {1985, 10, 26, 1, 24},
    {1955, 11, 5, 6, 0},
    {1985, 10, 27, 11, 0},
    {2015, 10, 21, 16, 29},
    {1955, 11, 12, 6, 0},
    {1885, 1, 1, 0, 0},
    {1885, 9, 2, 12, 0}};

dateStruct departedTimes[8] = {
    {1985, 10, 26, 1, 20},
    {1955, 11, 12, 22, 4},
    {1985, 10, 26, 1, 34},
    {1885, 9, 7, 9, 10},
    {1985, 10, 26, 11, 35},
    {1985, 10, 27, 2, 42},
    {1955, 11, 12, 21, 44},
    {1955, 11, 13, 12, 0}};

int8_t autoTime = 0;  // selects the above array time

const uint8_t monthDays[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

void time_setup() {
    pinMode(SECONDS_IN, INPUT_PULLDOWN);  // for monitoring seconds
    //pinMode(STATUS_LED, OUTPUT);          // Status LED

    EEPROM.begin(512);
    
    // RTC setup
    if (!rtc.begin()) {
        //something went wrong with RTC IC
        Serial.println("Couldn't find RTC");
        while (1)
            ;
    }

    if (rtc.lostPower() && WiFi.status() != WL_CONNECTED) {
        // Lost power and battery didn't keep time, so set current time to compile time
        rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
        Serial.println("RTC Lost Power - set compile time");
    }

    RTCClockOutEnable();  // Turn on the 1Hz second output

    bool validLoad = true;

    // Start the displays by calling begin()
    presentTime.begin();
    destinationTime.begin();
    departedTime.begin();

    presentTime.setRTC(true);  // configure as a display that will hold real time

    if (getNTPTime()) {
        //set RTC with NTP time
        Serial.print("RTC Set with NTP: ");
        Serial.println(settings.ntpServer);
    }

    if (!destinationTime.load()) {
        validLoad = false;
        // Set valid time and set if invalid load
        // 10/26/1985 1:21
        destinationTime.setMonth(10);
        destinationTime.setDay(26);
        destinationTime.setYear(1985);
        destinationTime.setHour(1);
        destinationTime.setMinute(21);
        destinationTime.setBrightness((int)atoi(settings.destTimeBright));
        destinationTime.save();
    }

    if (!departedTime.load()) {
        validLoad = false;
        // Set valid time and set if invalid load
        // 10/26/1985 1:20
        departedTime.setMonth(10);
        departedTime.setDay(26);
        departedTime.setYear(1985);
        departedTime.setHour(1);
        departedTime.setMinute(20);
        departedTime.setBrightness((int)atoi(settings.lastTimeBright));
        departedTime.save();
    }

    if (!presentTime.load()) {  // Time isn't saved here, but other settings are
        validLoad = false;
        presentTime.setBrightness((int)atoi(settings.presTimeBright));
        presentTime.save();
    }

    if (!loadAutoInterval()) {  // load saved settings
        validLoad = false;
        Serial.println("BAD AUTO INT");
        EEPROM.write(AUTOINTERVAL_PREF, 1);  // default to first option
        EEPROM.commit();
    }

    if (autoTimeIntervals[autoInterval]) {                    // non zero interval, use auto times
        destinationTime.setFromStruct(&destinationTimes[0]);  // load the first one
        departedTime.setFromStruct(&departedTimes[0]);
        Serial.println("SetFromStruct");
    }

    if (!validLoad) {
        // Show message
        destinationTime.showOnlySettingVal("RE", -1, true);
        presentTime.showOnlySettingVal("SET", -1, true);
        delay(1000);
        allOff();
    }

    Serial.println("Update Present Time - Setup");
    presentTime.setDateTime(rtc.now());  // Load the time for initial animation show

    startup = true;
    startupSound = true;
}

void time_loop() {
    if (startupSound) {
        play_startup();
        startupSound = false;
    }

    if(startup && (millis() >= (startupNow + startupDelay))) {
        startupNow += startupDelay;
        animate();
        startup = false;
        Serial.println("Startup animate triggered");
    }

    // time display update
    DateTime dt = rtc.now();

    // turns display back on after time traveling
    if ((millis() > timetravelNow + 1500) && timeTraveled) {
        timeTraveled = false;
        beepOn = true;
        animate();
        Serial.println("Display on after time travel");
    }

    if (presentTime.isRTC()) {  //only set real time if present time is RTC
        presentTime.setDateTime(dt);
    }

    y = digitalRead(SECONDS_IN);
    if (y != x) {      // different on half second
        if (y == 0) {  // flash colon on half seconds, lit on start of second
            /////////////////////////////
            //
            // auto display some times
            Serial.print(dt.minute());
            Serial.print(":");
            Serial.println(dt.second());

            // Do this on previous minute:59
            if (dt.minute() == 59) {
                minPrev = 0;
            } else {
                minPrev = dt.minute() + 1;
            }

            // only do this on second 59, check if it's time to do so
            if (dt.second() == 59 && autoTimeIntervals[autoInterval] &&
                (minPrev % autoTimeIntervals[autoInterval] == 0)) {
                Serial.println("DO IT");
                if (!autoTrack) {
                    autoTrack = true;  // Already did this, don't repeat
                    // do auto times
                    autoTime++;
                    if (autoTime > 4) {  // currently have 5 times
                        autoTime = 0;
                    }

                    // Show a preset dest and departed time
                    destinationTime.setFromStruct(&destinationTimes[autoTime]);
                    departedTime.setFromStruct(&departedTimes[autoTime]);

                    destinationTime.setColon(true);
                    presentTime.setColon(true);
                    departedTime.setColon(true);

                    allOff();

                    // Blank on second 59, display when new minute begins
                    while (digitalRead(SECONDS_IN) == 0) {  // wait for the complete of this half second
                                                            // Wait for this half second to end
                    }
                    while (digitalRead(SECONDS_IN) == 1) {  // second on next low
                                                            // Wait for the other half to end
                    }

                    Serial.println("Update Present Time 2");
                    if (presentTime.isRTC()) {
                        dt = rtc.now();               // New time by now
                        presentTime.setDateTime(dt);  // will be at next minute
                    }
                    animate();  // show all with month showing last

                    // end auto times
                }
            } else {
                autoTrack = false;
            }

            //////////////////////////////
            destinationTime.setColon(true);
            presentTime.setColon(true);
            departedTime.setColon(true);

            //play_file("/beep.mp3", 0.03, 1, false); //TODO: fix - currently causing crash

        } else {  // colon
            destinationTime.setColon(false);
            presentTime.setColon(false);
            departedTime.setColon(false);
        }  // colon

        //digitalWrite(STATUS_LED, !y);  // built-in LED shows system is alive, invert
                                       // to light on start of new second
        x = y;                         // remember it
    }

    if (startup == false) {
        presentTime.show();  // update display with object's time
        destinationTime.show();
        // destinationTime.showOnlySettingVal("SEC", dt.second(), true); // display
        // end, no numbers, clear rest of screen
        departedTime.show();
    }
}

void timeTravel() {
    timetravelNow = millis();
    timeTraveled = true;
    beepOn = false;
    play_file("/timetravel.mp3", getVolume());
    allOff();

    //copy present time to last time departed
    if (presentTime.isRTC()) {
        //has to be +1 since presentTime is currently RTC
        departedTime.setMonth(presentTime.getMonth() + 1);
    } else {
        departedTime.setMonth(presentTime.getMonth());
    }
    departedTime.setDay(presentTime.getDay());
    departedTime.setYear(presentTime.getYear());
    departedTime.setHour(presentTime.getHour());
    departedTime.setMinute(presentTime.getMinute());
    departedTime.save();

    //copy destination time to present time
    //DateTime does not work before 2000 or after 2159
    if (destinationTime.getYear() < 2000 || destinationTime.getYear() > 2159) {
        if (presentTime.isRTC()) presentTime.setRTC(false);  //presentTime is no longer 'actual' time
        presentTime.setMonth(destinationTime.getMonth());
        presentTime.setDay(destinationTime.getDay());
        presentTime.setYear(destinationTime.getYear());
        //TODO: figure out way to set MMMDDYYYY, but keep HH:MM incrementing every min like a clock
        presentTime.setHour(destinationTime.getHour());
        presentTime.setMinute(destinationTime.getMinute());
        presentTime.save();
    } else if (destinationTime.getYear() >= 2000 || destinationTime.getYear() <= 2159) {
        //present time still works as a clock, the time is just set differently
        if (!presentTime.isRTC()) presentTime.setRTC(true);
        rtc.adjust(DateTime(
            destinationTime.getYear(),
            destinationTime.getMonth() - 1,
            destinationTime.getDay(),
            destinationTime.getHour(),
            destinationTime.getMinute()));
    } else {
        if (getNTPTime()) {
            //if nothing or the same exact date try to get NTP time again
        }
    }
}

bool getNTPTime() {
    if (WiFi.status() == WL_CONNECTED) {  //connectToWifi()) {
        // if connected to wifi, get NTP time and set RTC
        configTime((long)atoi(settings.gmtOffset), (int)atoi(settings.daylightOffset), settings.ntpServer);
        int ntpRetries = 0;
        if (!getLocalTime(&_timeinfo)) {
            while (!getLocalTime(&_timeinfo)) {
                if (ntpRetries >= 5) {  //retry up to 5 times then quit
                    Serial.println("Couldn't get NTP time");
                    return false;
                } else {
                    ntpRetries++;
                }
                delay(500);
            }
        }
        Serial.println(&_timeinfo, "%A, %B %d %Y %H:%M:%S");
        byte byteYear = (_timeinfo.tm_year + 1900) % 100;  // adding to get full YYYY from NTP year format
                                                            // and keeping YY to set DS3232
        presentTime.setDS3232time(_timeinfo.tm_sec, _timeinfo.tm_min,
                                    _timeinfo.tm_hour, 0, _timeinfo.tm_mday,
                                    _timeinfo.tm_mon, byteYear);
        Serial.println("Time Set with NTP");
        return true;
    } else {
        return false;
    }
}

void doGetAutoTimes() {
    destinationTime.showOnlySettingVal("INT", autoTimeIntervals[autoInterval], true);

    presentTime.on();
    if (autoTimeIntervals[autoInterval] == 0) {
        presentTime.showOnlySettingVal("CUS", -1, true);
        departedTime.showOnlySettingVal("TOM", -1, true);
        departedTime.on();
    } else {
        departedTime.off();
        presentTime.showOnlySettingVal("MIN", -1, true);
    }

    while (!checkTimeOut() && !digitalRead(ENTER_BUTTON)) {
        autoTimesEnter();
        delay(100);
    }

    if (!checkTimeOut()) {  // only if there wasn't a timeout
        presentTime.off();
        departedTime.off();
        destinationTime.showOnlySave();
        saveAutoInterval();
        delay(1000);
    }
}

bool checkTimeOut() {
    // Call frequently while waiting for button press, increments timeout each
    // second, returns true when maxtime reached.
    y = digitalRead(SECONDS_IN);
    if (x != y) {
        x = y;
        //digitalWrite(STATUS_LED, !y);  // update status LED
        if (y == 0) {
            timeout++;
        }
    }

    if (timeout >= maxTime) {
        return true;  // timed out
    }
    return false;
}

void RTCClockOutEnable() {
    // enable the 1Hz RTC output
    uint8_t readValue = 0;
    Wire.beginTransmission(DS3231_I2CADDR);
    Wire.write((byte)0x0E);  // select control register
    Wire.endTransmission();

    Wire.requestFrom(DS3231_I2CADDR, 1);
    readValue = Wire.read();
    readValue = readValue & B11100011;  // enable squarewave and set to 1Hz,
    // Bit 2 INTCN - 0 enables OSC
    // Bit 3 and 4 - 0 0 sets 1Hz

    Wire.beginTransmission(DS3231_I2CADDR);
    Wire.write((byte)0x0E);  // select control register
    Wire.write(readValue);
    Wire.endTransmission();
}

bool isLeapYear(int year) {
    // Determine if provided year is a leap year.
    if (year % 4 == 0) {
        if (year % 100 == 0) {
            if (year % 400 == 0) {
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
}  // isLeapYear

int daysInMonth(int month, int year) {
    // Find number of days in a month
    if (month == 2 && isLeapYear(year)) {
        return 29;
    }
    return monthDays[month - 1];
}  // daysInMonth
