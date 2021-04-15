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

bool x;               // for tracking second change
bool y;               // for tracking second change

struct tm _timeinfo; //for NTP
RTC_DS3231 rtc; //for RTC IC

long timetravelNow = 0;
bool timeTraveled = false;

// The displays
clockDisplay destinationTime(DEST_TIME_ADDR, DEST_TIME_EEPROM);  // i2c address, preferences namespace
clockDisplay presentTime(PRES_TIME_ADDR, PRES_TIME_EEPROM);
clockDisplay departedTime(DEPT_TIME_ADDR, DEPT_TIME_EEPROM);

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

const char* ssid = "linksys";
const char* password = "";

const char* ntpServer = "pool.ntp.org";
const long gmtOffset_sec = -18000;
const int daylightOffset_sec = 3600;

void time_setup() {
    pinMode(SECONDS_IN, INPUT_PULLDOWN);  // for monitoring seconds
    pinMode(STATUS_LED, OUTPUT);  // Status LED

    // RTC setup
    if (!rtc.begin()) {
        //something went wrong with RTC IC
        Serial.println("Couldn't find RTC");
        while (1);
    }

    if (rtc.lostPower() && WiFi.status() != WL_CONNECTED) {
        // Lost power and battery didn't keep time, so set current time to compile time
        rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
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
        destinationTime.setBrightness(15);
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
        departedTime.setBrightness(15);
        departedTime.save();
    }

    if (!presentTime.load()) {  // Time isn't saved here, but other settings are
        validLoad = false;
        presentTime.setBrightness(15);
        presentTime.save();
    }

    if (!loadAutoInterval()) {  // load saved settings
        validLoad = false;
        Serial.println("BAD AUTO INT");
        putAutoInt(1); // default to first option
    }

    if (autoTimeIntervals[autoInterval]) {                    // non zero interval, use auto times
        destinationTime.setFromStruct(&destinationTimes[0]);  // load the first one
        departedTime.setFromStruct(&departedTimes[0]);
    }

    if (!validLoad) {
        // Show message
        destinationTime.showOnlySettingVal("RE", -1, true);
        presentTime.showOnlySettingVal("SET", -1, true);
        delay(1000);
        allOff();
    }

    Serial.println("Update Present Time - Setup");
    presentTime.setDateTime(rtc.now());                 // Load the time for initial animation show
    presentTime.setBrightness(15);  // added
    
    delay(3000); //to sync up with startup sound
    animate();
}

void time_loop() {
// time display update
    DateTime dt = rtc.now();

    // turns display back on after time traveling
    if ((millis() > timetravelNow + 1500) && timeTraveled) {
        timeTraveled = false;
        beepOn = true;
        animate();
    }

    if (presentTime.isRTC()) { //only set real time if present time is RTC
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
                    animate();                    // show all with month showing last

                    // end auto times
                }
            } else {
                autoTrack = false;
            }

            //////////////////////////////
            destinationTime.setColon(true);
            presentTime.setColon(true);
            departedTime.setColon(true);

            //play_file("/beep.mp3", 0.3, 1, false); //TODO: fix - currently causing crash

        } else {  // colon
            destinationTime.setColon(false);
            presentTime.setColon(false);
            departedTime.setColon(false);
        }  // colon

        digitalWrite(STATUS_LED, !y);  // built-in LED shows system is alive, invert
                                      // to light on start of new second
        x = y;                        // remember it
    }

    presentTime.show();  // update display with object's time
    destinationTime.show();
    // destinationTime.showOnlySettingVal("SEC", dt.second(), true); // display
    // end, no numbers, clear rest of screen
    departedTime.show();
}

void timeTravel() {
    timetravelNow = millis();
    timeTraveled = true;
    beepOn = false;
    play_file("/timetravel.mp3", 0.06);
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
    //TODO: figure out way to set MMMDDYYYY, but keep HH:MM as a clock
    presentTime.setRTC(false); //presentTime is no longer 'actual' time
    presentTime.setMonth(destinationTime.getMonth());
    presentTime.setDay(destinationTime.getDay());
    presentTime.setYear(destinationTime.getYear());
    presentTime.setHour(destinationTime.getHour());
    presentTime.setMinute(destinationTime.getMinute());
    presentTime.save();
}

bool getNTPTime() {
    // connect to WiFi if available
    if (connectToWifi()) {
        // if connected to wifi, get NTP time and set RTC
        configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
        int ntpRetries = 0;
        if (!getLocalTime(&_timeinfo)) {
            while (!getLocalTime(&_timeinfo)) {
                if (ntpRetries >= 5) { //retry up to 5 times then quit
                    Serial.println("Couldn't get NTP time");
                    return false;
                } else {
                    ntpRetries++;
                }
                delay(500);
            }
        } else {
            Serial.println(&_timeinfo, "%A, %B %d %Y %H:%M:%S");
            byte byteYear = (_timeinfo.tm_year + 1900) % 100;   // adding to get full YYYY from NTP year format
                                                                // and keeping YY to set DS3232
            presentTime.setDS3232time(_timeinfo.tm_sec, _timeinfo.tm_min,
                                        _timeinfo.tm_hour, 0, _timeinfo.tm_mday,
                                        _timeinfo.tm_mon, byteYear);
            Serial.println("Time Set with NTP");
            return true;
        }
    } else {
        return false;
    }
}

bool connectToWifi() {
    if (ssid != 0 && password != 0) {
        Serial.printf("Connecting to %s ", ssid);
        WiFi.config(WiFi.localIP(), WiFi.gatewayIP(), WiFi.subnetMask(),
                    IPAddress(8, 8, 8, 8));
        WiFi.begin(ssid, password);
        WiFi.enableSTA(true);
        delay(100);

        int wifiRetries = 0;
        while (WiFi.status() != WL_CONNECTED) {
            if (wifiRetries >= 20) {
                Serial.println("Could not connect to WiFi");
                return false;
            } else {
                wifiRetries++;
            }
            delay(500);
            Serial.print(".");
        }
        Serial.println(" CONNECTED");
        return true;
    } else {
        Serial.println("No WiFi credentials set");
        return false;
    }
}

void doGetAutoTimes() {
    // Set the auto times setting
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
        digitalWrite(STATUS_LED, !y);  // update status LED
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
    Wire.beginTransmission(DS3232_I2CADDR);
    Wire.write((byte)0x0E);  // select control register
    Wire.endTransmission();

    Wire.requestFrom(DS3232_I2CADDR, 1);
    readValue = Wire.read();
    readValue = readValue & B11100011;  // enable squarewave and set to 1Hz,
    // Bit 2 INTCN - 0 enables OSC
    // Bit 3 and 4 - 0 0 sets 1Hz

    Wire.beginTransmission(DS3232_I2CADDR);
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
