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
 */

#include "clockdisplay.h"
#include "tc_font.h"

const char months[12][4] = {"JAN", "FEB", "MAR", "APR", "MAY", "JUN", "JUL", "AUG", "SEP", "OCT", "NOV", "DEC"};

// Gets the i2c address and eeprom save location, default saveAddress is -1 if not provided to disable saving
clockDisplay::clockDisplay(uint8_t address, int saveAddress) 
{    
    // call begin() to start
    _address = address;
    _saveAddress = saveAddress;
}

// Private functions

// Returns bit pattern for provided number 0-9 or number provided as a char 0-9 for display on 7 segment display
uint8_t clockDisplay::getLED7SegChar(uint8_t value) 
{    
    if(value >= '0' && value <= '9') {  
        // if provided as a char
        return numDigs[value - 48];
    } else if(value <= 9) {
        return numDigs[value];
    }
    return 0x0;   // blank on invalid
}

// Returns bit pattern for provided character for display on alphanumeric 14 segment display
uint16_t clockDisplay::getLEDAlphaChar(char value) 
{    
    if(value == ' ') {
        return alphaChars[0];
    } else {
        return pgm_read_word(alphaChars + value);        
    }
}

// Make a number from the array and place it in the buffer at pos
uint16_t clockDisplay::makeNum(uint8_t num) 
{
    // Each position holds two digits, high byte is 1's, low byte is 10's
    uint16_t segments;
    
    segments = getLED7SegChar(num % 10) << 8;        // Place 1's in upper byte
    segments = segments | getLED7SegChar(num / 10);  // 10's in lower byte
    
    return segments;
}

uint16_t clockDisplay::makeAlpha(uint8_t value) 
{
    // valid positions are 0 to 2
    return getLEDAlphaChar(value);
}

// directly clear the display RAM
void clockDisplay::clearDisplay() 
{    
    Wire.beginTransmission(_address);
    Wire.write(0x00);  // start at address 0x0

    for(int i = 0; i < 16; i++) {
        Wire.write(0x0);
    }
    Wire.endTransmission();
}

// turn on all LEDs
void clockDisplay::lampTest() 
{  
    Wire.beginTransmission(_address);
    Wire.write(0x00);  // start at address 0x0

    for(int i = 0; i < 16; i++) {
        Wire.write(0xFF);
    }
    Wire.endTransmission();
}

// Start the display
void clockDisplay::begin() 
{
    Wire.beginTransmission(_address);
    Wire.write(0x20 | 1);  // turn on oscillator
    Wire.endTransmission();

    clear();            // clear buffer
    setBrightness(15);  // be sure in case coming up for mc reset
    clearDisplay();     // clear display RAM, comes up random.
    on();               // turn it on
}

void clockDisplay::on() 
{
    // Turn on the display
    Wire.beginTransmission(_address);
    Wire.write(0x80 | 1);  // turn on the display
    Wire.endTransmission();
}

// Turn off the display
void clockDisplay::off() 
{
    Wire.beginTransmission(_address);
    Wire.write(0x80);  // turn off the display
    Wire.endTransmission();
}

// clears the buffer
void clockDisplay::clear() 
{
    // must call show() to actually clear display

    #ifdef TC_DBG
    Serial.println("Clockdisplay: Clear Buffer");
    #endif

    // Holds the LED segment status
    // Month 0,1,2
    // Day 3
    // Year 4 and 5
    // Hour 6
    // Min 7
    
    for(int i = 0; i < 8; i++) {
        _displayBuffer[i] = 0;
    }
}

// Set display brightness
uint8_t clockDisplay::setBrightness(uint8_t level) 
{
    // Valid brighness levels are 0 to 15. Default is 15.
    if(level > 15)
        return _brightness;

    Wire.beginTransmission(_address);
    Wire.write(0xE0 | level);  // Dimming command
    Wire.endTransmission();

    #ifdef TC_DBG
    Serial.print("Clockdisplay: Setting brightness: ");
    Serial.println(level, DEC);
    #endif

    _brightness = level;
    return _brightness;
}

uint8_t clockDisplay::getBrightness() 
{
    return _brightness;
}

/* 
 * Save date/time and other settings to EEPROM 
 * Only non-RTC displays save their time.
 * 
 */
bool clockDisplay::save() 
{    
    uint16_t sum = 0;  // a simple checksum
    int i;

    if(!isRTC() && _saveAddress >= 0) {  

        // Non-RTC: Save time

        #ifdef TC_DBG      
        Serial.println("Clockdisplay: Saving non-RTC settings to EEPROM");
        #endif

        uint8_t savBuf[10];
        savBuf[0] = _year & 0xff;
        savBuf[1] = (_year >> 8) & 0xff;
        savBuf[2] = _yearoffset & 0xff;
        savBuf[3] = (_yearoffset >> 8) & 0xff;
        savBuf[4] = _month;
        savBuf[5] = _day;
        savBuf[6] = _hour;
        savBuf[7] = _minute;
        savBuf[8] = 0;        // _brightness;    // now in settings

        for(i = 0; i < 9; i++) {
            sum += savBuf[i];
            EEPROM.write(_saveAddress + i, savBuf[i]);
        }
        
        EEPROM.write(_saveAddress + 9, sum & 0xff);        
        
        EEPROM.commit();
        
    } else if(isRTC() && _saveAddress >= 0) {

        // RTC: do not save time

        #ifdef TC_DBG  
        Serial.println("Clockdisplay: Saving RTC settings to EEPROM");
        #endif

        // We now clear the entire space, since time is not saved,
        // and _brightness is now part of settings (config file)        
        for(i = 0; i < 10; i++) {
            EEPROM.write(_saveAddress + i, 0);  
        }

        // Obsolete, _brightness is saved, but never actually used
        //EEPROM.write(_saveAddress + 8, _brightness);
        //sum = sum + _brightness;

        //sum &= 0xff;  
        //EEPROM.write(_saveAddress + 9, sum);
        
        EEPROM.commit();
        
    } else {
      
        return false;
        
    }

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

    if(_saveAddress >= 0) {
        
        for(i = 0; i < 10; i++) {
            loadBuf[i] = EEPROM.read(_saveAddress + i);
            if(i < 9) sum += loadBuf[i];
        }

        if((sum & 0xff) == loadBuf[9]) {

            if (!isRTC()) {  
              
                // non-RTC, load saved time
                #ifdef TC_DBG  
                Serial.println("Clockdisplay: Loading non-RTC settings from EEPROM");
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
                
            } else {
              
                // RTC, time not saved/loaded, only other settings

                // Reinstate _brightness to keep old behavior
                setBrightness((int)atoi(settings.presTimeBright));      
                
            }
          
        } else {

            Serial.println("Clockdisplay: Bad EEPROM checksum");

            // FIXME - what now?

            return false;
          
        }      

    } else {
      
        return false; 
        
    }

    return true;
}

// Show the buffer
void clockDisplay::show() 
{
    (_hour < 12) ? AM() : PM();

    (_colon) ? colonOn() : colonOff();

    Wire.beginTransmission(_address);
    Wire.write(0x00);  // start at address 0x0

    for(int i = 0; i < 8; i++) {
        Wire.write(_displayBuffer[i] & 0xFF);
        Wire.write(_displayBuffer[i] >> 8);
    }
    Wire.endTransmission();
}

// Show all but month
void clockDisplay::showAnimate1() 
{
    off();

    (_hour < 12) ? AM() : PM();

    (_colon) ? colonOn() : colonOff();

    Wire.beginTransmission(_address);
    Wire.write(0x00);  // start at address 0x0

    for(int i = 0; i < 8; i++) {
        if(i > 2) {
            Wire.write(_displayBuffer[i] & 0xFF);
            Wire.write(_displayBuffer[i] >> 8);
        } else {
            Wire.write(0x00);  //blank month, first 3 16 bit locations
            Wire.write(0x00);
        }
    }
    Wire.endTransmission();
    on();
}

// Show month, assumes showAnimate1() was already called
void clockDisplay::showAnimate2() 
{
    Wire.beginTransmission(_address);
    Wire.write(0x00);  // start at address 0x0
    for (int i = 0; i < 3; i++) {
        Wire.write(_displayBuffer[i] & 0xFF);
        Wire.write(_displayBuffer[i] >> 8);
    }
    Wire.endTransmission();
}

// Makes characters for 3 char month, valid months 1-12
void clockDisplay::setMonth(int monthNum) 
{
    if(monthNum < 1 || monthNum > 12) {        
        Serial.print("Clockdisplay: setMonth: Bad month: "); 
        Serial.println(monthNum, DEC);
        monthNum = 12;                   
    } 
  
    _month = monthNum;
    
#ifdef IS_ACAR_DISPLAY
    _displayBuffer[0] = makeNum(monthNum);
#else
    // We always work with months 1-12, not 0-11
    // Exception: timeinfo (tm) works with 0-11, but we only use this in getNTPtime 
    // for syncing. Therefore, we must ALWAYS decrease monthNum here
    monthNum--;    
    _displayBuffer[0] = makeAlpha(months[monthNum][0]);
    _displayBuffer[1] = makeAlpha(months[monthNum][1]);
    _displayBuffer[2] = makeAlpha(months[monthNum][2]);
#endif
}

// Place LED pattern in day position in buffer, which is 3.
void clockDisplay::setDay(int dayNum) 
{
    if(dayNum < 1 || dayNum > 31) {          
        Serial.print("Clockdisplay: setDay: Bad day: ");
        Serial.println(dayNum, DEC);
        dayNum = 1;
    }
    
    _day = dayNum;
    
    _displayBuffer[3] = makeNum(dayNum);
}

// Place LED pattern in year position in buffer, which is 4 and 5.
void clockDisplay::setYear(uint16_t yearNum) 
{
    if(yearNum > 9999) {        
        Serial.print("Clockdisplay: setYear: Bad year: ");
        Serial.println(yearNum, DEC);
        yearNum = 9999;        
    }
    
    _year = yearNum;
    yearNum -= _yearoffset;
    
    _displayBuffer[4] = makeNum(yearNum / 100);
    _displayBuffer[5] = makeNum(yearNum % 100);
}

// Place LED pattern in year position directly, which is 4 and 5.
// useful for setting the year to the present time
void clockDisplay::setYearDirect(uint16_t yearNum) 
{
    if(yearNum > 9999) {                
        Serial.print("Clockdisplay: setYearDirect: Bad year: ");
        Serial.println(yearNum, DEC);
        yearNum = 9999;
    } 
    
    _year = yearNum;
    yearNum -= _yearoffset;
    
    directCol(4, makeNum(yearNum / 100));
    directCol(5, makeNum(yearNum % 100));
}

void clockDisplay::setYearOffset(int16_t yearOffs) 
{
    _yearoffset = yearOffs;
}

// Place LED pattern in hour position in buffer, which is 6.
void clockDisplay::setHour(uint16_t hourNum) 
{   
    if(hourNum > 23) {                
        Serial.print("Clockdisplay: setHour: Bad hour: ");
        Serial.println(hourNum, DEC);
        hourNum = 23;
    }

    _hour = hourNum;

    // Show it as 12 hour time
    // AM/PM will be set on show() to avoid being overwritten
    if(hourNum == 0) {
        _displayBuffer[6] = makeNum(12);
    } else if(hourNum > 12) {
        // pm
        _displayBuffer[6] = makeNum(hourNum - 12);
    } else if(hourNum <= 12) {
        // am
        _displayBuffer[6] = makeNum(hourNum);
    }
}

// Place LED pattern in minute position in buffer, which is 7
void clockDisplay::setMinute(int minNum) 
{
    if(minNum < 0 || minNum > 59) {
        Serial.print("Clockdisplay: setMinute: Bad Minute: ");
        Serial.println(minNum, DEC);
        minNum = 0;  
    }
    
    _minute = minNum;

    if(minNum < 60) {
        _displayBuffer[7] = makeNum(minNum);
    } else if(minNum >= 60) {
        _displayBuffer[7] = makeNum(0);
    }
}

void clockDisplay::AM() 
{
    _displayBuffer[3] = _displayBuffer[3] | 0x0080;
    _displayBuffer[3] = _displayBuffer[3] & 0x7FFF;
    return;
}

void clockDisplay::PM() 
{
    _displayBuffer[3] = _displayBuffer[3] | 0x8000;
    _displayBuffer[3] = _displayBuffer[3] & 0xFF7F;
    return;
}

void clockDisplay::colonOn() 
{
    _displayBuffer[4] = _displayBuffer[4] | 0x8080;
    return;
}

void clockDisplay::colonOff() 
{
    _displayBuffer[4] = _displayBuffer[4] & 0x7F7F;
    return;
}

// clears the display RAM and only shows the provided month
void clockDisplay::showOnlyMonth(int monthNum) 
{
    clearDisplay();
    
    if(monthNum < 1 || monthNum > 12) {
        Serial.println("Clockdisplay: showOnlyMonth: Bad month");
        if(monthNum < 1) monthNum = 1;
        if(monthNum > 12) monthNum = 12;
    }

    monthNum--;
    directCol(0, makeAlpha(months[monthNum][0]));
    directCol(1, makeAlpha(months[monthNum][1]));
    directCol(2, makeAlpha(months[monthNum][2]));
}

// clears the display RAM and only shows the word save
void clockDisplay::showOnlySave() 
{
    clearDisplay();

    directCol(0, makeAlpha('S'));
    directCol(1, makeAlpha('A'));
    directCol(2, makeAlpha('V'));
    directCol(3, numDigs[14]);    // 14 is 'E'
}

// clears the display RAM and only shows the word "UTES" (as in "MINUTES")
void clockDisplay::showOnlyUtes() 
{
    clearDisplay();

    directCol(0, makeAlpha('U'));
    directCol(1, makeAlpha('T'));
    directCol(2, makeAlpha('E'));
    directCol(3, numDigs[28]);    // 28 is 'S'
}

void clockDisplay::showOnlySettingVal(const char* setting, int8_t val, bool clear) 
{
    int8_t c = 0;
    
    if(clear)
        clearDisplay();

    while(c < 3 && setting[c]) {
        directCol(c, makeAlpha(setting[c]));
        c++;
    }

    if(val >= 0 && val < 100)
        directCol(3, makeNum(val));
    else
        directCol(3, 0x00);
}

// clears the display RAM and only shows the provided day
void clockDisplay::showOnlyDay(int dayNum) 
{    
    clearDisplay();
    directCol(3, makeNum(dayNum));
}

// clears the display RAM and only shows the provided year
void clockDisplay::showOnlyYear(int yearNum) 
{    
    clearDisplay();

    directCol(4, makeNum(yearNum / 100));
    directCol(5, makeNum(yearNum % 100));
}

// clears the display RAM and only shows the provided hour
void clockDisplay::showOnlyHour(int hourNum) 
{
    clearDisplay();

    if(hourNum == 0) {
        directCol(6, makeNum(12));
        directAM();
    }

    else if(hourNum > 12) {
        // pm
        directCol(6, makeNum(hourNum - 12));
    } else {
        // am
        directCol(6, makeNum(hourNum));
        directAM();
    }

    (hourNum > 11) ? directPM() : directAM();    
}

// clears the display RAM and only shows the provided minute
void clockDisplay::showOnlyMinute(int minuteNum) 
{
    clearDisplay();
    
    directCol(7, makeNum(minuteNum));
}

// write directly to a column with supplied segments
void clockDisplay::directCol(int col, int segments) 
{
    // Month/Alpha - first 3 cols
    // Day - column 4
    // Year - column 5 & 6
    // Hour - column 7
    // Min - column 8

    Wire.beginTransmission(_address);
    Wire.write(col * 2);  // 2 bytes per col * position, day is at pos
    // leave buffer intact, direclty write to display
    Wire.write(segments & 0xFF);
    Wire.write(segments >> 8);
    Wire.endTransmission();
}

void clockDisplay::directAM() 
{
    Wire.beginTransmission(_address);
    Wire.write(0x6);  // 2 bytes per col * position, day is at pos
    // leave buffer intact, direclty write to display
    Wire.write(0x80);
    Wire.write(0x0);
    Wire.endTransmission();
}

void clockDisplay::directPM() 
{
    Wire.beginTransmission(_address);
    Wire.write(0x6);  // 2 bytes per col * position, day is at pos
    // leave buffer intact, direclty write to display
    Wire.write(0x0);
    Wire.write(0x80);
    Wire.endTransmission();
}

// Set the displayed time with supplied DateTime object
void clockDisplay::setDateTime(DateTime dt) 
{
    // ATTN: DateTime implemention does not work for years < 2000!
    
    setMonth(dt.month());
    setDay(dt.day());
    setYear(dt.year());
    setHour(dt.hour());
    setMinute(dt.minute());
}

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
    Wire.write(decToBcd(year));        // set year (0~99) 
    Wire.endTransmission();
}

// Convert normal decimal numbers to binary coded decimal
byte clockDisplay::decToBcd(byte val) 
{
    return ((val / 10 * 16) + (val % 10));
}

// Set time from array, YEAR, MONTH, DAY, HOUR, MIN
void clockDisplay::setFromStruct(dateStruct* s) 
{    
    setYear(s->year);
    setMonth(s->month);
    setDay(s->day);
    setHour(s->hour);
    setMinute(s->minute);
}

/*
// returns a DateTime that we're set to
DateTime clockDisplay::getDateTime() 
{
    // this is broken, DateTime implementation doesn't work with years < 2000
    return DateTime(_year, _month, _day, _hour, _minute, 0);
}
*/

uint8_t clockDisplay::getMonth() 
{
    return _month;
}

uint8_t clockDisplay::getDay() 
{
    return _day;
}

uint16_t clockDisplay::getYear() 
{
    return _year;
}

int16_t clockDisplay::getYearOffset() 
{
    return _yearoffset;
}

uint8_t clockDisplay::getHour() 
{
    return _hour;
}

uint8_t clockDisplay::getMinute() {
    return _minute;
}

void clockDisplay::setColon(bool col) 
{
    // set true to turn it on
    _colon = col;
}

// track if this is will be holding real time.
void clockDisplay::setRTC(bool rtc) 
{
    _rtc = rtc;
}

// is this an real time display?
bool clockDisplay::isRTC() 
{    
    return _rtc;
}