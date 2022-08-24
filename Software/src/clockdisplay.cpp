/*
 * -------------------------------------------------------------------
 * CircuitSetup.us Time Circuits Display
 * 
 * Code based on Marmoset Electronics 
 * https://www.marmosetelectronics.com/time-circuits-clock
 * by John Monaco
 *
 * Enhanced/modified/written in 2022 by Thomas Winischhofer (A10001986)
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

#include "clockdisplay.h"
#include "tc_font.h"

const char months[12][4] = {
      "JAN", "FEB", "MAR", "APR", "MAY", "JUN", "JUL", "AUG", "SEP", "OCT", "NOV", "DEC"
};

// Store i2c address and eeprom data location
clockDisplay::clockDisplay(uint8_t address, int saveAddress) 
{    
    _address = address;
    _saveAddress = saveAddress;
}

// Start the display
void clockDisplay::begin() 
{
    Wire.beginTransmission(_address);
    Wire.write(0x20 | 1);  // turn on oscillator
    Wire.endTransmission();

    clear();            // clear buffer
    setBrightness(15);  // setup initial brightness
    clearDisplay();     // clear display RAM
    on();               // turn it on
}

// Turn on the display
void clockDisplay::on() 
{
    Wire.beginTransmission(_address);
    Wire.write(0x80 | 1);  
    Wire.endTransmission();
}

// Turn off the display
void clockDisplay::off() 
{
    Wire.beginTransmission(_address);
    Wire.write(0x80);  
    Wire.endTransmission();
}

// Turn on all LEDs
void clockDisplay::lampTest() 
{  
    Wire.beginTransmission(_address);
    Wire.write(0x00);  // start at address 0x0

    for(int i = 0; i < CD_BUF_SIZE*2; i++) {
        Wire.write(0xFF);
    }
    Wire.endTransmission();
}

// Clear the buffer
void clockDisplay::clear() 
{
    // must call show() to actually clear display

    #ifdef TC_DBG
    Serial.println(F("Clockdisplay: Clear Buffer"));
    #endif
    
    for(int i = 0; i < CD_BUF_SIZE; i++) {
        _displayBuffer[i] = 0;
    }
}

// Set display brightness
// Valid brighness levels are 0 to 15. Default is 15.
uint8_t clockDisplay::setBrightness(uint8_t level) 
{
    if(level > 15)
        level = 15;

    Wire.beginTransmission(_address);
    Wire.write(0xE0 | level);  // Dimming command
    Wire.endTransmission();

    #ifdef TC_DBG
    Serial.print(F("Clockdisplay: Setting brightness: "));
    Serial.println(level, DEC);
    #endif

    _brightness = level;
    return _brightness;
}

uint8_t clockDisplay::getBrightness() 
{
    return _brightness;
}

void clockDisplay::set1224(bool hours24)
{
    _mode24 = hours24 ? true : false;
}

bool clockDisplay::get1224(void)
{
    return _mode24;
}

void clockDisplay::setNightMode(bool mymode)
{
    _nightmode = mymode ? true : false;
}

bool clockDisplay::getNightMode(void)
{
    return _nightmode;
}

// Track if this is will be holding real time.
void clockDisplay::setRTC(bool rtc) 
{
    _rtc = rtc;
}

// is this an real time display?
bool clockDisplay::isRTC() 
{    
    return _rtc;
}

// Setup date in buffer --------------------------------------------------------


// Set the displayed time with supplied DateTime object, ignores timeDifference
void clockDisplay::setDateTime(DateTime dt) 
{
    // ATTN: DateTime implemention does not work for years < 2000!    

    setYear(dt.year());
    setMonth(dt.month());
    setDay(dt.day());    
    setHour(dt.hour());
    setMinute(dt.minute());
}

// Set the displayed time with supplied DateTime object with timeDifference
void clockDisplay::setDateTimeDiff(DateTime dt) 
{
    uint64_t rtcTime;
    int year, month, day, hour, minute;

    if(!timeDifference) {
        setDateTime(dt);
        return;
    }

    rtcTime = dateToMins(dt.year() - _yearoffset, dt.month(), dt.day(), dt.hour(), dt.minute());

    if(timeDiffUp) {
        rtcTime += timeDifference;
        // Don't care about 9999-10000 roll-over
        // So we display 0000 for 10000+
        // So be it.        
    } else {
        rtcTime -= timeDifference;
    }

    minsToDate(rtcTime, year, month, day, hour, minute); 

    setYear(year + _yearoffset);
    setMonth(month);
    setDay(day);    
    setHour(hour);
    setMinute(minute);
}

// Set YEAR, MONTH, DAY, HOUR, MIN from structure 
// Never use for RTC!
void clockDisplay::setFromStruct(dateStruct* s) 
{    
    if(isRTC()) {
        Serial.println(F("Clockdisplay: Internal error; setFromStruct() called for RTC"));
    }
    setYear(s->year);
    setMonth(s->month);
    setDay(s->day);
    setHour(s->hour);
    setMinute(s->minute);
}

// Show data in display --------------------------------------------------------


// Show the buffer 
void clockDisplay::show() 
{
    showInt(false);
}

// Show all but month
void clockDisplay::showAnimate1() 
{    
    showInt(true);
}

// Show month, assumes showAnimate1() was already called
void clockDisplay::showAnimate2() 
{
    if(_nightmode && !isRTC()) {
        return;
    }
    
    Wire.beginTransmission(_address);
    Wire.write(0x00);  // start at address 0x0
    for(int i = 0; i < CD_BUF_SIZE; i++) {
        Wire.write(_displayBuffer[i] & 0xFF);
        Wire.write(_displayBuffer[i] >> 8);
    }
    Wire.endTransmission();
}


// Set fields in buffer --------------------------------------------------------


// Set yearOffset
void clockDisplay::setYearOffset(int16_t yearOffs) 
{
    _yearoffset = yearOffs;
}

// Place LED pattern in year position in buffer
void clockDisplay::setYear(uint16_t yearNum) 
{
    if(yearNum < 1) {         
        Serial.print(F("Clockdisplay: setYear: Bad year: "));
        Serial.println(yearNum, DEC);
        yearNum = 1;        
    }
    
    _year = yearNum;
    yearNum -= _yearoffset;

    while(yearNum >= 10000) 
        yearNum -= 10000;
    
    _displayBuffer[CD_YEAR_POS]     = makeNum(yearNum / 100);
    _displayBuffer[CD_YEAR_POS + 1] = makeNum(yearNum % 100);
}

// Place LED pattern in month position in buffer
void clockDisplay::setMonth(int monthNum) 
{
    if(monthNum < 1 || monthNum > 12) {        
        Serial.print(F("Clockdisplay: setMonth: Bad month: ")); 
        Serial.println(monthNum, DEC); 
        monthNum = (monthNum > 12) ? 12 : 1;                   
    } 
  
    _month = monthNum;
    
#ifdef IS_ACAR_DISPLAY
    _displayBuffer[CD_MONTH_POS] = makeNum(monthNum);
#else
    // We always work with months 1-12, not 0-11
    // Exception: timeinfo (tm) works with 0-11, but we only use this in getNTPtime 
    // for syncing. Therefore, we must ALWAYS decrease monthNum here
    monthNum--;    
    _displayBuffer[CD_MONTH_POS]     = getLEDAlphaChar(months[monthNum][0]);
    _displayBuffer[CD_MONTH_POS + 1] = getLEDAlphaChar(months[monthNum][1]);
    _displayBuffer[CD_MONTH_POS + 2] = getLEDAlphaChar(months[monthNum][2]);
#endif
}

// Place LED pattern in day position in buffer
void clockDisplay::setDay(int dayNum) 
{
    int maxDay = daysInMonth(_month, _year-_yearoffset);

    // It is essential that setDay is called AFTER year
    // and month have been set!
    
    if(dayNum < 1 || dayNum > maxDay) {          
        Serial.print(F("Clockdisplay: setDay: Bad day: "));
        Serial.println(dayNum, DEC);
        dayNum = (dayNum < 1) ? 1 : maxDay;
    }
    
    _day = dayNum;
    
    _displayBuffer[CD_DAY_POS] = makeNum(dayNum);
}

// Place LED pattern in hour position in buffer.
void clockDisplay::setHour(uint16_t hourNum) 
{   
    if(hourNum > 23) {                
        Serial.print(F("Clockdisplay: setHour: Bad hour: "));
        Serial.println(hourNum, DEC);
        hourNum = 23;
    }

    _hour = hourNum;

    if(!_mode24) {
      
        if(hourNum == 0) {
            _displayBuffer[CD_HOUR_POS] = makeNum(12);
        } else if(hourNum > 12) {
            // pm
            _displayBuffer[CD_HOUR_POS] = makeNum(hourNum - 12);
        } else if(hourNum <= 12) {
            // am
            _displayBuffer[CD_HOUR_POS] = makeNum(hourNum);
        }
        
    } else {
      
        _displayBuffer[CD_HOUR_POS] = makeNum(hourNum);      
        
    }

    // AM/PM will be set on show() to avoid being overwritten
}

// Place LED pattern in minute position in buffer
void clockDisplay::setMinute(int minNum) 
{
    if(minNum < 0 || minNum > 59) {
        Serial.print(F("Clockdisplay: setMinute: Bad Minute: "));
        Serial.println(minNum, DEC);
        minNum = (minNum > 59) ? 59 : 0;
    }
    
    _minute = minNum;
    
    _displayBuffer[CD_MIN_POS] = makeNum(minNum);

    if(isRTC()) {
        if(alarmOnOff) 
            _displayBuffer[CD_MIN_POS] |= 0x8000;
    }
}

void clockDisplay::setColon(bool col) 
{
    // set true to turn it on
    _colon = col;
}


// Query data ------------------------------------------------------------------


uint8_t clockDisplay::getMonth() 
{
    return _month;
}

uint8_t clockDisplay::getDay() 
{
    return _day;
}

int16_t clockDisplay::getYearOffset() 
{
    return _yearoffset;
}

uint16_t clockDisplay::getYear() 
{
    return _year;
}

uint8_t clockDisplay::getHour() 
{
    return _hour;
}

uint8_t clockDisplay::getMinute() {
    return _minute;
}


// Put data directly on display (bypass buffer) --------------------------------


// clears the display RAM and only shows the provided month
void clockDisplay::showOnlyMonth(int monthNum) 
{
    clearDisplay();
    
    if(monthNum < 1 || monthNum > 12) {
        Serial.println(F("Clockdisplay: showOnlyMonth: Bad month"));
        monthNum = (monthNum > 12) ? 12 : 1;
    }
    
#ifdef IS_ACAR_DISPLAY    
    directCol(CD_MONTH_POS, makeNum(monthNum));
#else
    monthNum--;
    directCol(CD_MONTH_POS,     getLEDAlphaChar(months[monthNum][0]));
    directCol(CD_MONTH_POS + 1, getLEDAlphaChar(months[monthNum][1]));
    directCol(CD_MONTH_POS + 2, getLEDAlphaChar(months[monthNum][2]));
#endif    
}
// clears the display RAM and only shows the provided day
void clockDisplay::showOnlyDay(int dayNum) 
{    
    clearDisplay();
    
    directCol(CD_DAY_POS, makeNum(dayNum));
}

// clears the display RAM and only shows the provided year
void clockDisplay::showOnlyYear(int yearNum) 
{    
    clearDisplay();

    if(yearNum > 10000) yearNum -= 10000;

    directCol(CD_YEAR_POS,     makeNum(yearNum / 100));
    directCol(CD_YEAR_POS + 1, makeNum(yearNum % 100));
}

// clears the display RAM and only shows the provided hour
void clockDisplay::showOnlyHour(int hourNum) 
{
    clearDisplay();

    if(!_mode24) {
      
        if(hourNum == 0) {
            directCol(CD_HOUR_POS, makeNum(12));
            directAM();
        } else if(hourNum > 12) {
            // pm
            directCol(CD_HOUR_POS, makeNum(hourNum - 12));
        } else {
            // am
            directCol(CD_HOUR_POS, makeNum(hourNum));
            directAM();
        }
    
        (hourNum > 11) ? directPM() : directAM();
        
    }  else {
      
        directCol(CD_HOUR_POS, makeNum(hourNum));
        
        directAMPMoff();
      
    }
}

// clears the display RAM and only shows the provided minute
void clockDisplay::showOnlyMinute(int minuteNum) 
{
    clearDisplay();
    
    directCol(CD_MIN_POS, makeNum(minuteNum));
}


// Special purpose -------------------------------------------------------------


// clears the display RAM and only shows the given text
void clockDisplay::showOnlyText(const char *text)
{
    int idx = 0, pos = CD_MONTH_POS;
    int temp = 0;
    
    clearDisplay();
    
#ifdef IS_ACAR_DISPLAY
    while(text[idx] && pos < (CD_MONTH_POS+CD_MONTH_SIZE)) {
        temp = getLED7AlphaChar(text[idx]);
        idx++;
        if(text[idx]) {
            temp |= (getLED7AlphaChar(text[idx]) << 8);
            idx++;            
        }
        directCol(pos, temp);
        pos++;
    }
#else
    while(text[idx] && pos < (CD_MONTH_POS+CD_MONTH_SIZE)) {
        directCol(pos, getLEDAlphaChar(text[idx]));
        idx++;
        pos++;
    }
#endif    
    pos = CD_DAY_POS;
    while(text[idx] && pos <= CD_MIN_POS) {
        temp = getLED7AlphaChar(text[idx]);
        idx++;
        if(text[idx]) {
            temp |= (getLED7AlphaChar(text[idx]) << 8);
            idx++;
        }
        directCol(pos, temp);
        pos++;
    }
}

// clears the display RAM and only shows the provided 2 numbers (parts of IP)
void clockDisplay::showOnlyHalfIP(int a, int b, bool clear) 
{
    char buf[6];
    
    if(clear)
          clearDisplay();
  
#ifdef IS_ACAR_DISPLAY
    if(a >= 100) {
        directCol(CD_MONTH_POS, makeNum(a / 10));
        directCol(CD_DAY_POS, numDigs[(a % 10)]);
    } else {
        directCol(CD_MONTH_POS, makeNumN0(a));
    }
#else    
    sprintf(buf, "%d", a);
    directCol(CD_MONTH_POS, getLEDAlphaChar(buf[0]));
    if(buf[1]) {
        directCol(CD_MONTH_POS + 1, getLEDAlphaChar(buf[1]));
        if(buf[2]) {
            directCol(CD_MONTH_POS + 2, getLEDAlphaChar(buf[2]));
        }
    }
#endif    

    if(b >= 100) {
        directCol(CD_YEAR_POS, makeNumN0(b / 100));
    }
    directCol(CD_YEAR_POS + 1, ((b / 100) ? makeNum(b % 100) : makeNumN0(b % 100)));      
}

// Shows a text part and a number
void clockDisplay::showOnlySettingVal(const char* setting, int8_t val, bool clear) 
{   
    if(clear)
        clearDisplay();

#ifdef IS_ACAR_DISPLAY
    directCol(CD_MONTH_POS, getLED7AlphaChar(setting[0]) |
             ((setting[1] ? getLED7AlphaChar(setting[1]) : 0) << 8));       
#else
    directCol(CD_MONTH_POS, getLEDAlphaChar(setting[0]));
    if(setting[1]) {
        directCol(CD_MONTH_POS + 1, getLEDAlphaChar(setting[1]));
        if(setting[2]) {
            directCol(CD_MONTH_POS + 2, getLEDAlphaChar(setting[2]));
        }
    }
#endif

    if(val >= 0 && val < 100)
        directCol(CD_DAY_POS, makeNum(val));
    else
        directCol(CD_DAY_POS, 0x00);
}


// Save & load data ------------------------------------------------------------


/* 
 * Save date/time and other settings to EEPROM 
 * Only non-RTC displays save their time.
 * We stick with the EEPROM here because the times
 * probably won't be changed that often to cause
 * a problem with flash wear.
 * "Persistent" time travel causes more wear than
 * "Non-Persistent".
 * 
 */
bool clockDisplay::save() 
{    
    uint8_t savBuf[10];
    uint16_t sum = 0; 
    int i;

    if(!isRTC() && _saveAddress >= 0) {  

        // Non-RTC: Save time

        #ifdef TC_DBG      
        Serial.println(F("Clockdisplay: Saving non-RTC settings to EEPROM"));
        #endif
        
        savBuf[0] = _year & 0xff;
        savBuf[1] = (_year >> 8) & 0xff;
        savBuf[2] = _yearoffset & 0xff;
        savBuf[3] = (_yearoffset >> 8) & 0xff;
        savBuf[4] = _month;
        savBuf[5] = _day;
        savBuf[6] = _hour;
        savBuf[7] = _minute;
        savBuf[8] = 0;        // was _brightness, now in settings

        for(i = 0; i < 9; i++) {
            sum += savBuf[i];
            EEPROM.write(_saveAddress + i, savBuf[i]);
        }
        
        EEPROM.write(_saveAddress + 9, sum & 0xff);        
        
        EEPROM.commit();
        
    } else if(isRTC() && _saveAddress >= 0) {

        // RTC: Save yearoffs, timeDiff (time comes from battery-backed RTC)

        #ifdef TC_DBG  
        Serial.println(F("Clockdisplay: Saving RTC settings to EEPROM"));
        #endif
                
        savBuf[0] = 0; // unused
        savBuf[1] = _yearoffset & 0xff;
        savBuf[2] = (_yearoffset >> 8) & 0xff;

        savBuf[3] = (timeDifference >> 32) & 0xff;
        savBuf[4] = (timeDifference >> 24) & 0xff;
        savBuf[5] = (timeDifference >> 16) & 0xff;
        savBuf[6] = (timeDifference >>  8) & 0xff;
        savBuf[7] =  timeDifference        & 0xff;

        savBuf[8] = timeDiffUp ? 1 : 0;                                             
        
        for(i = 0; i < 9; i++) {
            sum += savBuf[i] ^ 0x55;
            EEPROM.write(_saveAddress + i, savBuf[i]);            
        }
        
        EEPROM.write(_saveAddress + 9, sum & 0xff);        
        
        EEPROM.commit();
                
    } else {
      
        return false;
        
    }

    return true;
}

// Save YOffs and clear timeDifference in EEPROM
bool clockDisplay::saveYOffs() 
{    
    uint8_t savBuf[10];
    uint16_t sum = 0; 
    int i;

    if(!isRTC() || _saveAddress < 0)
        return false;

    // RTC: Save yearoffs; zero timeDifference

    #ifdef TC_DBG  
    Serial.println(F("Clockdisplay: Saving RTC/YOffs setting to EEPROM"));
    #endif
                
    savBuf[0] = 0;        
    savBuf[1] = _yearoffset & 0xff;
    savBuf[2] = (_yearoffset >> 8) & 0xff;

    savBuf[3] = 0;
    savBuf[4] = 0;
    savBuf[5] = 0;
    savBuf[6] = 0;
    savBuf[7] = 0;

    savBuf[8] = 0;                                             
    
    for(i = 0; i < 9; i++) {
        sum += savBuf[i] ^ 0x55;
        EEPROM.write(_saveAddress + i, savBuf[i]);
    }
    
    EEPROM.write(_saveAddress + 9, sum & 0xff);        
    
    EEPROM.commit();    

    return true;
}

/* 
 * Load saved date/time from eeprom 
 * 
 */
bool clockDisplay::load() 
{
    uint8_t loadBuf[10];
    uint16_t sum = 0;
    int i;

    if(_saveAddress < 0) 
        return false;
        
    if(!isRTC()) {  

        for(i = 0; i < 10; i++) {
            loadBuf[i] = EEPROM.read(_saveAddress + i);
            if(i < 9) sum += loadBuf[i];                 
        }    
      
        // Non-RTC: Load saved time
        // 16bit sum cannot be zero; if it is, the data
        // is clear, which means it is invalid.

        if( (sum != 0) && ((sum & 0xff) == loadBuf[9])) { 
                                   
            #ifdef TC_DBG  
            Serial.println(F("Clockdisplay: Loading non-RTC settings from EEPROM"));
            #endif
            
            setYearOffset((loadBuf[3] << 8) | loadBuf[2]);
            setYear((loadBuf[1] << 8) | loadBuf[0]);
            setMonth(loadBuf[4]);
            setDay(loadBuf[5]);
            setHour(loadBuf[6]);
            setMinute(loadBuf[7]);

            // Reinstate _brightness to keep old behavior
            if (_saveAddress == DEST_TIME_PREF) { 
                setBrightness((int)atoi(settings.destTimeBright));  
            } else if (_saveAddress == DEPT_TIME_PREF) {
                setBrightness((int)atoi(settings.lastTimeBright));
            }
            
            return true;
            
        } 

    } else {

        // RTC: yearoffs & timeDiff is saved
        
        for(i = 0; i < 10; i++) {
            loadBuf[i] = EEPROM.read(_saveAddress + i);
            if(i < 9) sum += loadBuf[i] ^ 0x55;                    
        }    

        if((sum & 0xff) == loadBuf[9]) {               
              
              setYearOffset((loadBuf[2] << 8) | loadBuf[1]); 

              timeDifference = ((uint64_t)loadBuf[3] << 32) |  // Dumb casts for 
                               ((uint64_t)loadBuf[4] << 24) |  // silencing compiler
                               ((uint64_t)loadBuf[5] << 16) |
                               ((uint64_t)loadBuf[6] <<  8) |
                                (uint64_t)loadBuf[7];
                                
              timeDiffUp = loadBuf[8] ? true : false;

              #ifdef TC_DBG  
              Serial.println(F("Clockdisplay: Loading RTC settings from EEPROM"));
              #endif
                     
        } else {

              setYearOffset(0);

              timeDifference = 0;

              Serial.println(F("Clockdisplay: Invalid RTC EEPROM data"));
              
        }

        // Reinstate _brightness to keep old behavior
        setBrightness((int)atoi(settings.presTimeBright));        

        return true;             
    }
     
    Serial.println(F("Clockdisplay: Invalid EEPROM data"));

    // Do NOT clear EEPROM if data is invalid.
    // All 0s are as bad, wait for EEPROM to be
    // written by application on purpose
        
    return false;
}

// Only load yearOffset from EEPROM
// !!! Does not *SET* yearOffs, just returns it !!!
int16_t clockDisplay::loadYOffs() 
{
    uint8_t loadBuf[10];
    uint16_t sum = 0;
    int i;

    if(_saveAddress < 0 || !isRTC()) 
        return -1;    
    
    for(i = 0; i < 10; i++) {
        loadBuf[i] = EEPROM.read(_saveAddress + i);
        if(i < 9) sum += loadBuf[i] ^ 0x55;                 
    }    

    if((sum & 0xff) == loadBuf[9]) { 
                
          return ((loadBuf[2] << 8) | loadBuf[1]);  
                 
    } 

    return -1;        
}

// Write time to RTC chip
void clockDisplay::setDS3232time(byte second, byte minute, byte hour, byte dayOfWeek, byte dayOfMonth, byte month, byte year) 
{
    Wire.beginTransmission(DS3231_I2CADDR);
    Wire.write(0);                     // sends 00h - seconds register
    Wire.write(decToBcd(second));      // set seconds
    Wire.write(decToBcd(minute));      // set minutes
    Wire.write(decToBcd(hour));        // set hours
    Wire.write(decToBcd(dayOfWeek));   // set day of week (1-7; user defined; we use 1=Sunday, 7=Saturday)
    Wire.write(decToBcd(dayOfMonth));  // set date (1~31)
    Wire.write(decToBcd(month));       // set month (1~12)
    Wire.write(decToBcd(year));        // set year (0~99) (ie 2000-2099)
    Wire.endTransmission();
}


// Private functions ###########################################################


// Returns bit pattern for provided value 0-9 or number provided as a char '0'-'9' 
// for display on 7 segment display
uint8_t clockDisplay::getLED7NumChar(uint8_t value) 
{    
    if(value >= '0' && value <= '9') {  
        // if provided as a char
        return numDigs[value - 48];
    } else if(value <= 9) {
        return numDigs[value];
    }
    return 0;   // blank on invalid
}

// Returns bit pattern for provided character for display on 7 segment display
uint8_t clockDisplay::getLED7AlphaChar(uint8_t value) 
{    
    if(value == ' ' || value == 0) {
        return 0;
    } else if(value >= '0' && value <= '9') {
        return numDigs[value - 48];        
    } else if(value >= 'A' && value <= 'Z') {
        return numDigs[value - 'A' + 10];
    }   

    return 0;
}

// Returns bit pattern for provided character for display on alphanumeric 14 segment display
#ifndef IS_ACAR_DISPLAY
uint16_t clockDisplay::getLEDAlphaChar(uint8_t value) 
{
    if(value > 127) return 0;
        
    return alphaChars[value];
}
#endif

// Make a 2 digit number from the array and place it in the buffer at pos
// (makes leading 0s)
uint16_t clockDisplay::makeNum(uint8_t num) 
{
    uint16_t segments = 0;

    // Each position holds two digits, high byte is 1's, low byte is 10's
    
    segments = getLED7NumChar(num % 10) << 8;     // Place 1's in upper byte
    segments |= getLED7NumChar(num / 10);         // 10's in lower byte
    
    return segments;
}

// Make a 2 digit number from the array and place it in the buffer at pos
// (no leading 0s)
uint16_t clockDisplay::makeNumN0(uint8_t num) 
{    
    uint16_t segments = 0;

    // Each position holds two digits, high byte is 1's, low byte is 10's
    
    segments = getLED7NumChar(num % 10) << 8;     // Place 1's in upper byte
    if(num / 10) {
        segments |= getLED7NumChar(num / 10);     // 10's in lower byte
    }
    
    return segments;
}

// Directly write to a column with supplied segments
// (leave buffer intact, directly write to display)
void clockDisplay::directCol(int col, int segments) 
{
    Wire.beginTransmission(_address);
    Wire.write(col * 2);  // 2 bytes per col * position    
    Wire.write(segments & 0xFF);
    Wire.write(segments >> 8);
    Wire.endTransmission();
}

// Directly clear the display 
void clockDisplay::clearDisplay() 
{    
    Wire.beginTransmission(_address);
    Wire.write(0x00);  // start at address 0x0

    for(int i = 0; i < CD_BUF_SIZE*2; i++) {
        Wire.write(0x0);
    }
    Wire.endTransmission();
}

// Show the buffer 
void clockDisplay::showInt(bool animate) 
{
    int i = 0; 

    if(_nightmode) {        
        if(!isRTC()) {    
            off();   
            _oldnm = 1;         
            return;
        } else {          
            if(_oldnm < 1) { 
                setBrightness(0); 
            }
            _oldnm = 1;
        }
    } else if(isRTC()) {
        if(_oldnm > 0) {
            setBrightness((int)atoi(settings.presTimeBright));             
        }
        _oldnm = 0;
    }

    if(animate) off();

    if(!_mode24) {
        (_hour < 12) ? AM() : PM();
    } else {    
        AMPMoff();
    }

    (_colon) ? colonOn() : colonOff();

    Wire.beginTransmission(_address);
    Wire.write(0x00);  // start at address 0x0

    if(animate) {
        for(int c = 0; c < CD_MONTH_SIZE; c++) {
            Wire.write(0x00);  //blank month
            Wire.write(0x00);
        }        
        i = CD_DAY_POS;
    }
    
    for(; i < CD_BUF_SIZE; i++) {
        Wire.write(_displayBuffer[i] & 0xFF);
        Wire.write(_displayBuffer[i] >> 8);
    }
    
    Wire.endTransmission();

    if(animate || (!isRTC() && (_oldnm > 0)) ) on();
    
    if(!isRTC()) _oldnm = 0;
}

void clockDisplay::colonOn() 
{
    _displayBuffer[CD_COLON_POS] |= 0x8080;    
}

void clockDisplay::colonOff() 
{
    _displayBuffer[CD_COLON_POS] &= 0x7F7F;    
}

void clockDisplay::AM() 
{
    _displayBuffer[CD_AMPM_POS] |= 0x0080;
    _displayBuffer[CD_AMPM_POS] &= 0x7FFF;
}

void clockDisplay::PM() 
{
    _displayBuffer[CD_AMPM_POS] |= 0x8000;
    _displayBuffer[CD_AMPM_POS] &= 0xFF7F;   
}

void clockDisplay::AMPMoff() 
{
    _displayBuffer[CD_AMPM_POS] &= 0x7F7F;
}

void clockDisplay::directAMPM(int val1, int val2) 
{
    Wire.beginTransmission(_address);
    Wire.write(CD_DAY_POS * 2);    
    Wire.write(val1 & 0xff);
    Wire.write(val2 & 0xff);
    Wire.endTransmission();
}
    
void clockDisplay::directAM() 
{
    directAMPM(0x80, 0x00);
}

void clockDisplay::directPM() 
{
    directAMPM(0x00, 0x80);
}

void clockDisplay::directAMPMoff() 
{
    directAMPM(0x00, 0x00);
}

// Convert normal decimal numbers to binary coded decimal
byte clockDisplay::decToBcd(byte val) 
{
    return ((val / 10 * 16) + (val % 10));
}

    
