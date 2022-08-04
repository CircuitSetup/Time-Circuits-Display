/*
 * -------------------------------------------------------------------
 * CircuitSetup.us Time Circuits Display
 * Code adapted from Marmoset Electronics 
 * https://www.marmosetelectronics.com/time-circuits-clock
 * by John Monaco
 * Enhanced/modified in 2022 by Thomas Winischhofer (A10001986)
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
 * 
 */

// not yet supported by esp32 1.0.x
//#include "esp_sntp.h" // TW 7/2022

#include "tc_time.h"

bool autoTrack = false;
bool autoReadjust = false;
int8_t minPrev;  // track previous minute

bool x;  // for tracking second change
bool y;  

bool startup = false;
bool startupSound = false;
long startupNow = 0;
int startupDelay = 1700; // the time between startup sound being played and the display coming on

struct tm _timeinfo;  //for NTP
RTC_DS3231 rtc;       //for RTC IC

long timetravelNow = 0;
bool timeTraveled = false;
bool presentTimeBogus = false;

uint8_t timeout = 0;  // for tracking idle time in menus, reset to 0 on each button press

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

/*
 * time_setup()
 * 
 */
void time_setup() 
{
    bool validLoad = true;
    
    pinMode(SECONDS_IN, INPUT_PULLDOWN);  // for monitoring seconds (disabled ATM)
    //pinMode(STATUS_LED, OUTPUT);          // Status LED

    EEPROM.begin(512);
    
    // RTC setup
    if(!rtc.begin()) {
        
        Serial.println("time_setup: Couldn't find RTC. Panic!");
        
        // Setup pins for white LED
        pinMode(WHITE_LED, OUTPUT);

        // Blink forever
        while (1) {
            digitalWrite(WHITE_LED, HIGH);  
            delay(1000);
            digitalWrite(WHITE_LED, LOW);  
            delay(1000);          
        }           
    }

    if(rtc.lostPower() && WiFi.status() != WL_CONNECTED) {
        // Lost power and battery didn't keep time, so set current time to compile time
        rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
        
        Serial.println("time_setup: RTC Lost Power - setting compile time");
    }

    RTCClockOutEnable();  // Turn on the 1Hz second output

    // Start the displays by calling begin()
    presentTime.begin();
    destinationTime.begin();
    departedTime.begin();

    // configure presentTime as a display that will hold real time
    presentTime.setRTC(true);  

    // modify the NTP polling interval; not yet supported by esp32 1.0.x
    //sntp_set_sync_interval(1 * 60 * 60 * 1000UL); // 1 hour

    // Set RTC with NTP time
    if(getNTPTime()) {

        #ifdef TC_DBG
        Serial.print("time_setup: RTC set with NTP: ");
        Serial.println(settings.ntpServer);
        #endif
    }

    // Load destination time (and set to default if invalid)
    if(!destinationTime.load()) {
        validLoad = false;  
        // 10/26/1985 1:21
        destinationTime.setMonth(10);
        destinationTime.setDay(26);
        destinationTime.setYearOffset(0);
        destinationTime.setYear(1985);
        destinationTime.setHour(1);
        destinationTime.setMinute(21);                   
        destinationTime.setBrightness((int)atoi(settings.destTimeBright));
        destinationTime.save();
    }

    // Load departed time (and set to default if invalid)
    if(!departedTime.load()) {
        validLoad = false; 

        // 10/26/1985 1:20
        departedTime.setMonth(10);
        departedTime.setDay(26);
        departedTime.setYearOffset(0);
        departedTime.setYear(1985);
        departedTime.setHour(1);
        departedTime.setMinute(20);
        departedTime.setBrightness((int)atoi(settings.lastTimeBright));
        departedTime.save();
    }

    // "Load" present time; time isn't actually loaded here, but other settings are
    if(!presentTime.load()) {  
        validLoad = false;
        presentTime.setBrightness((int)atoi(settings.presTimeBright));
        presentTime.save();
    }

    // load autoInterval (from settings, not EEPROM)
    if(!loadAutoInterval()) {  
        // Obsolete; is part of settings now, EEPROM not used.        
        //validLoad = false;
        //Serial.println("time_setup: Bad autointerval");
        //EEPROM.write(AUTOINTERVAL_PREF, 1);  // default to first option
        //EEPROM.commit();
    }

    // if non zero autoInterval -> use auto times, load the first one
    if(autoTimeIntervals[autoInterval]) {                    
        destinationTime.setFromStruct(&destinationTimes[0]); 
        departedTime.setFromStruct(&departedTimes[0]);
        #ifdef TC_DBG
        Serial.println("time_setup: autointerval enabled");
        #endif
    }

    // Show "RE-SET" message if data loaded was invalid somehow
    if(!validLoad) {          
        destinationTime.showOnlySettingVal("RE", -1, true);
        presentTime.showOnlySettingVal("SET", -1, true);
        delay(1000);
        allOff();
    }

    // Load the time for initial animation show
    
    #ifdef TC_DBG
    Serial.println("time_setup: Update Present Time");
    #endif
    
    presentTime.setDateTime(rtc.now());  

    startup = true;
    startupSound = true;
}

/*
 * time_loop()
 * 
 */
void time_loop() 
{
    if(startupSound) {
        play_startup();
        startupSound = false;
    }

    // Turn display on after startup delay
    if(startup && (millis() >= (startupNow + startupDelay))) {
        startupNow += startupDelay;
        animate();
        startup = false;
        #ifdef TC_DBG
        Serial.println("time_loop: Startup animate triggered");
        #endif
    }

    // Turn display back on after time traveling
    if((millis() > timetravelNow + 1500) && timeTraveled) {
        timeTraveled = false;
        beepOn = true;
        animate();
        #ifdef TC_DBG
        Serial.println("time_loop: Display on after time travel");
        #endif
    }

    // RTC display update
    DateTime dt = rtc.now();

    if(presentTime.isRTC()) {  // only set real time if present time is RTC  

        if(presentTimeBogus && dt.year() - presentTime.getYearOffset() > 9999) {  
        
            Serial.println("Rollover 9999->0 detected, adjusting RTC and yearOffset");
        
            int tyr = 2016, tyroffs = 2016;

            // No need to calculate this...
            /*while(tyr < 2000) {
                tyr += 28;
                tyroffs += 28;
            }*/

            presentTime.setYearOffset(tyroffs);        

            dt = rtc.now();
            
            rtc.adjust(DateTime(
                tyr,
                dt.month(),
                dt.day(),
                dt.hour(),
                dt.minute(),
                dt.second()                
                )
            );
            
            dt = rtc.now();
        }
        presentTime.setDateTime(dt);
    }

    y = digitalRead(SECONDS_IN);
    if(y != x) {      // different on half second
        if(y == 0) {  // flash colon on half seconds, lit on start of second

            // First, handle colon to keep the pace
            destinationTime.setColon(true);
            presentTime.setColon(true);
            departedTime.setColon(true);
                        
            // Logging beacon
            #ifdef TC_DBG
            if(!(dt.second() % 30)) {
              Serial.print(dt.year());
              Serial.print(":");
              Serial.print(dt.month());
              Serial.print(":");
              Serial.print(dt.minute());
              Serial.print(":");
              Serial.println(dt.second());
            }
            #endif
            
            // Re-adjust time periodically using NTP
            if(!presentTimeBogus && dt.second() == 10 && dt.minute() == 1) {
                if(!autoReadjust) {                    
                    if(getNTPTime()) {                    
                        #ifdef TC_DBG
                        Serial.println("time_loop: RTC re-adjusted using NTP");                    
                        #endif
                        autoReadjust = true;
                    } else {                          
                        Serial.println("time_loop: RTC re-adjustment via NTP failed");
                    }
                }
            } else {
                autoReadjust = false;
            }

            // Handle autoInterval (aka autoTrack):
            
            // Do this on previous minute:59
            if(dt.minute() == 59) {
                minPrev = 0;
            } else {
                minPrev = dt.minute() + 1;
            }
            
            // Only do this on second 59, check if it's time to do so
            if(dt.second() == 59 && autoTimeIntervals[autoInterval] &&
                      (minPrev % autoTimeIntervals[autoInterval] == 0)) {
                
                if(!autoTrack) {

                    #ifdef TC_DBG
                    Serial.println("time_loop: autoInterval");
                    #endif                  
                    
                    autoTrack = true;     // Already did this, don't repeat
                    
                    // do auto times
                    autoTime++;
                    if(autoTime > 4) {    // currently have 5 pre-programmed times
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
                    while(digitalRead(SECONDS_IN) == 0) {  // wait for the complete of this half second
                                                           // Wait for this half second to end
                    }
                    while(digitalRead(SECONDS_IN) == 1) {  // second on next low
                                                           // Wait for the other half to end
                    }

                    #ifdef TC_DBG
                    Serial.println("time_loop: Update Present Time");
                    #endif
                    
                    if(presentTime.isRTC()) {
                        dt = rtc.now();               // New time by now
                        presentTime.setDateTime(dt);  // will be at next minute
                    }
                    
                    animate();  // show all with month showing last
                }
                
            } else {
                
                autoTrack = false;
            
            }                       

        } else {  
            destinationTime.setColon(false);
            presentTime.setColon(false);
            departedTime.setColon(false);
        }                                         
        //digitalWrite(STATUS_LED, !y);  // built-in LED shows system is alive, invert
                                       // to light on start of new second                            
        x = y;                        
    }

    if(startup == false) {
        presentTime.show();  // update display with object's time
        destinationTime.show();       
        departedTime.show();
    }
}


/* Time Travel: User entered a destination time.
 *  
 *  -) copy present time into departed time (where it freezes)
 *  -) copy destination time to present time (where it continues to run as a clock)
 *
 *  This is called from tc_keypad.cpp 
 */

void timeTravel() 
{
    int tyr = 0;
    int tyroffs = 0;
        
    timetravelNow = millis();
    timeTraveled = true;
    beepOn = false;
    play_file("/timetravel.mp3", getVolume());
    allOff();

    // Copy present time to last time departed
    departedTime.setMonth(presentTime.getMonth());
    departedTime.setDay(presentTime.getDay());
    departedTime.setYear(presentTime.getYear() - presentTime.getYearOffset());
    departedTime.setHour(presentTime.getHour());
    departedTime.setMinute(presentTime.getMinute());
    departedTime.setYearOffset(0);
    departedTime.save();

    // Copy destination time to present time
    // ATTN: DateTime does not work before 2000 or after 2159
    // ATTN: RTC only used two-digit year 0-99
    // so we use yearOffs to fake a year with identical calendar within 2000-2050
    // RTC keeps running within 2000-2050, but display is skewed by yearOffs
    // Upper limit 2050 to avoid overflow (RTC 0-99, DateTime 2000-2159) while clock 
    // running and owner still alive.

    tyr = destinationTime.getYear();
    if(tyr < 2000) {
        while(tyr < 2000) {
            tyr += 28;
            tyroffs += 28;
        }
    } else if(tyr > 2050) {
        while(tyr > 2050) {
            tyr -= 28;
            tyroffs -= 28;
        }
    }
    
    presentTime.setRTC(true);
    
    presentTime.setYearOffset(tyroffs);

    presentTimeBogus = true;  // Avoid overwriting this time by NTP time in loop
    
    rtc.adjust(DateTime(
            tyr,
            destinationTime.getMonth(),
            destinationTime.getDay(),
            destinationTime.getHour(),
            destinationTime.getMinute()
            )
    );

    #ifdef TC_DBG
    Serial.println("timeTravel: Success, good luck!");
    #endif
}

/*
 * Reset present time to actual present time
 * (aka "return from time travel")
 */
void resetPresentTime() 
{
    // Copy "present" time to last time departed
    departedTime.setMonth(presentTime.getMonth());
    departedTime.setDay(presentTime.getDay());
    departedTime.setYear(presentTime.getYear() - presentTime.getYearOffset());
    departedTime.setHour(presentTime.getHour());
    departedTime.setMinute(presentTime.getMinute());
    departedTime.setYearOffset(0);
    departedTime.save();
    
    presentTime.setYearOffset(0);
  
    if(presentTimeBogus) {
        presentTimeBogus = false;
        play_file("/timetravel.mp3", getVolume());
    }
  
    allOff();
  
    presentTime.setRTC(true);
  
    getNTPTime(); 
  
    timetravelNow = millis();
    timeTraveled = true; 
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
 * Get time from NTP
 * (Saves time to RTC)
 */
bool getNTPTime() 
{  
    if(WiFi.status() == WL_CONNECTED) { 
      
        // if connected to wifi, get NTP time and set RTC
        configTime(0, 0, settings.ntpServer);
        setenv("TZ", settings.timeZone, 1);   // Set environment variable with time zone
        tzset();

        int ntpRetries = 0;
        if(!getLocalTime(&_timeinfo)) {
          
            while(!getLocalTime(&_timeinfo)) {
                if(ntpRetries >= 20) {  
                    Serial.println("getNTPTime: Couldn't get NTP time");
                    return false;
                } else {
                    ntpRetries++;
                }
                delay(800);
            }
            
        }

        // Don't waste any time here...

        // Timeinfo:  Years since 1900
        // RTC:       0-99, 0 being 2000 
        //            (important for leap year compensation, which only works from 2000-2099, not 2100 on, 
        //            century bit has not influence on leap year comp., is buggy)          
                                                            
        presentTime.setDS3232time(_timeinfo.tm_sec, 
                                    _timeinfo.tm_min,
                                    _timeinfo.tm_hour, 
                                    _timeinfo.tm_wday + 1,  // We use Su=1...Sa=7 on HW-RTC, tm uses 0-6 (days since Sunday)
                                    _timeinfo.tm_mday,
                                    _timeinfo.tm_mon + 1,   // Month needs to be 1-12, timeinfo uses 0-11
                                    _timeinfo.tm_year + 1900 - 2000); 
                                    
        #ifdef TC_DBG            
        Serial.print("getNTPTime: Result from NTP update: ");
        Serial.println(&_timeinfo, "%A, %B %d %Y %H:%M:%S");            
        Serial.println("getNTPTime: Time successfully set with NTP");
        #endif
        
        return true;
        
    } else {
      
        Serial.println("getNTPTime: Time NOT set with NTP, WiFi not connected");
        return false;
    
    }
}

/* 
 * Call this frequently while waiting for button press,  
 * increments timeout each second, returns true when maxtime reached.
 * 
 */
bool checkTimeOut() 
{    
    y = digitalRead(SECONDS_IN);
    if(x != y) {
        x = y;
        //digitalWrite(STATUS_LED, !y);  // update status LED
        if(y == 0) {
            timeout++;
        }
    }

    if(timeout >= maxTime) {
        return true;  
    }
    return false;
}

// enable the 1Hz RTC output
void RTCClockOutEnable() 
{    
    uint8_t readValue = 0;
    
    Wire.beginTransmission(DS3231_I2CADDR);
    Wire.write((byte)0x0E);  // select control register
    Wire.endTransmission();

    Wire.requestFrom(DS3231_I2CADDR, 1);
    readValue = Wire.read();
    readValue = readValue & B11100011;  
    // enable squarewave and set to 1Hz,
    // Bit 2 INTCN - 0 enables OSC
    // Bit 3 and 4 - 0 0 sets 1Hz

    Wire.beginTransmission(DS3231_I2CADDR);
    Wire.write((byte)0x0E);  // select control register
    Wire.write(readValue);
    Wire.endTransmission();
}

// Determine if provided year is a leap year.
bool isLeapYear(int year) 
{
    if(year % 4 == 0) {
        if(year % 100 == 0) {
            if(year % 400 == 0) {
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