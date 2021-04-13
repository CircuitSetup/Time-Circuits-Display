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

#include "clockdisplay.h"

Preferences pref; //to save preferences in namespaces

clockDisplay::clockDisplay(uint8_t address, const char* saveAddress) {
    // Gets the i2c address and eeprom save location, default saveAddress is -1 if not provided to disable saving
    // call begin() to start
    _address = address;
    _saveAddress = saveAddress;
}

//private functions
uint8_t clockDisplay::getLED7SegChar(uint8_t value) {
    // returns bit pattern for provided number 0-9 or number provided as a char 0-9 for display on 7 segment display
    if (value >= '0' && value <= '9') {  // it was provided as a char
        return numDigs[value - 48];
    } else if (value <= 9) {
        return numDigs[value];
    }
    return 0x0;  // blank on invalid
}

uint16_t clockDisplay::getLEDAlphaChar(char value) {
    // returns bit pattern for provided character for display on alphanumeric 14 segment display
    if (value == ' ') {
        return alphaChars[0];
    } else {
        return pgm_read_word(alphaChars + value);
        Serial.print("Alpha Char: ");
        Serial.println(pgm_read_word(alphaChars + value));
    }
}

uint16_t clockDisplay::makeNum(uint8_t num) {
    // Make a number from the array and place it in the buffer at pos
    // Each position holds two digits, high byte is 1's, low byte is 10's
    uint16_t segments;
    segments = getLED7SegChar(num % 10) << 8;        // Place 1's in upper byte
    segments = segments | getLED7SegChar(num / 10);  // 10's in lower byte
    return segments;
}

uint16_t clockDisplay::makeAlpha(uint8_t value) {
    //positions are 0 to 2
    return getLEDAlphaChar(value);
}

void clockDisplay::clearDisplay() {
    // directly clear the display RAM
    Wire.beginTransmission(_address);
    Wire.write(0x00);  // start at address 0x0

    for (int i = 0; i < 16; i++) {
        Wire.write(0x0);
    }
    Wire.endTransmission();
}

void clockDisplay::lampTest() {  // turn on all LEDs
    Wire.beginTransmission(_address);
    Wire.write(0x00);  // start at address 0x0

    for (int i = 0; i < 16; i++) {
        Wire.write(0xFF);
    }
    Wire.endTransmission();
}

void clockDisplay::begin() {
    // Start the display
    Wire.beginTransmission(_address);
    Wire.write(0x20 | 1);  // turn on oscillator
    Wire.endTransmission();

    clear();            //clear buffer
    setBrightness(15);  // be sure in case coming up for mc reset
    clearDisplay();     //clear display RAM, comes up random.
    on();               // turn it on
}

void clockDisplay::on() {
    // Turn on the display
    Wire.beginTransmission(_address);
    Wire.write(0x80 | 1);  // turn on the display
    Wire.endTransmission();
}

void clockDisplay::off() {
    // Turn off the display
    Wire.beginTransmission(_address);
    Wire.write(0x80);  // turn off the display
    Wire.endTransmission();
}

void clockDisplay::clear() {
    // clears the buffer
    // must call show to clear display

    // Holds the LED segment status
    // Month 0,1,2
    // Day 3
    // Year 4 and 5
    // Hour 6
    // Min 7
    Serial.println("Clear Buffer");
    for (int i = 0; i < 8; i++) {
        _displayBuffer[i] = 0;
    }
}

uint8_t clockDisplay::setBrightness(uint8_t level) {
    // Valid brighness levels are 0 to 15. Default is 15.
    if (level > 15)
        return _brightness;

    Wire.beginTransmission(_address);
    Wire.write(0xE0 | level);  // Dimming command
    Wire.endTransmission();

    Serial.print("Setting brightness: ");
    Serial.println(level, DEC);

    _brightness = level;
    return _brightness;
}

uint8_t clockDisplay::getBrightness() {
    return _brightness;
}

bool clockDisplay::save() {
    // save date/time to preferences

    uint16_t dates[] = {_year, _month, _day, _hour, _minute, 15/*_brightness*/};

    Serial.printf("%d %d %d %d %d %d\n",
        dates[0], dates[1], _day, _hour, _minute, _brightness);

    if (!isRTC() && _saveAddress >= 0) {  // rtc can't save, save address was not set and can't save if negative
        Serial.println(" Save in Preferences - NOT rtc");
        pref.begin(_saveAddress, false);
        pref.putBytes(_saveAddress, dates, sizeof(dates));
        pref.end();
    } else if (isRTC() && _saveAddress >= 0) {
        Serial.println("Save RTC in Prof");
        pref.begin(_saveAddress, false);
        pref.putUChar(_saveAddress, _brightness); //only save brightness
        pref.end();
    } else {
        return false;
    }
    return true;
}

bool clockDisplay::load() {
    // Load saved date/time from preferences

    Serial.println(">>>>>>>>>>>>>>>>>>> LOADING SAVED SETTINGS <<<<<<<<<<<<<<<<<<<");
    Serial.println(_saveAddress);
    if (isPrefData(_saveAddress) && !isRTC()) {                          // not a rtc, load saved values
        pref.begin(_saveAddress, false);

        //pref.clear(); //clears all data

        size_t dateSize = pref.getBytesLength(_saveAddress);
        char buffer[dateSize]; // prepare a buffer for the data

        pref.getBytes(_saveAddress, buffer, dateSize);
        saveDateStruct *_saveAddress = (saveDateStruct *) buffer; // cast the bytes into a struct ptr

        Serial.printf("%d %d %d %d %d %d\n", 
            _saveAddress[0].year, _saveAddress[0].month,
            _saveAddress[0].day, _saveAddress[0].hour,
            _saveAddress[0].minute, _saveAddress[0].brightness);
        
        setYear(_saveAddress[0].year);
        setMonth(_saveAddress[0].month);
        setDay(_saveAddress[0].day);
        setHour(_saveAddress[0].hour);
        setMinute(_saveAddress[0].minute);
        setBrightness(_saveAddress[0].brightness);
        
        pref.end();
        return true;
    }
    else if (isPrefData(_saveAddress) && isRTC()) {
        // rtc doesnt save any time
        pref.begin(_saveAddress, false);
        setBrightness(pref.getUChar(_saveAddress, _brightness));
        pref.end();
        return true;
    }
    else {
        return false;
    }
return true;
}

bool clockDisplay::resetClocks() {
    // removes all data from preferences
    if (pref.remove(_saveAddress)) {
        return true;
    } else {
        return false;
    }
}

bool clockDisplay::isPrefData(const char* address) {
    //is there data stored in Preferences?
    if (pref.getBytesLength(address) > 0) {
        return true;
    } else {
        return false;
    }
}

void clockDisplay::show() {
    // Show the buffer
    if (_hour < 12) {
        AM();
    } else {
        PM();
    }

    if (_colon) {
        colonOn();
    } else {
        colonOff();
    }

    Wire.beginTransmission(_address);
    Wire.write(0x00);  // start at address 0x0

    for (int i = 0; i < 8; i++) {
        Wire.write(_displayBuffer[i] & 0xFF);
        Wire.write(_displayBuffer[i] >> 8);
    }
    Wire.endTransmission();
}

void clockDisplay::showAnimate1() {
    // Show all but month
    off();

    if (_hour < 12) {
        AM();
    } else {
        PM();
    }
    if (_colon) {
        colonOn();
    } else {
        colonOff();
    }

    Wire.beginTransmission(_address);
    Wire.write(0x00);  // start at address 0x0

    for (int i = 0; i < 8; i++) {
        if (i > 2) {
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

void clockDisplay::showAnimate2() {
    // Show month, assumes showAnimate1() was already called
    Wire.beginTransmission(_address);
    Wire.write(0x00);  // start at address 0x0
    for (int i = 0; i < 3; i++) {
        Wire.write(_displayBuffer[i] & 0xFF);
        Wire.write(_displayBuffer[i] >> 8);
    }
    Wire.endTransmission();
}

void clockDisplay::setMonth(int monthNum) {
    // Makes characters for 3 char month, valid months 1-12
    if (monthNum < 13) {
        _month = monthNum;         // keep track
    } else {
        _month = 12; // set month to the max otherwise month isn't displayed at all
        monthNum = 12;
    }
    if (!isRTC()) monthNum--;  //array starts at 0
    _displayBuffer[0] = makeAlpha(months[monthNum][0]);
    _displayBuffer[1] = makeAlpha(months[monthNum][1]);
    _displayBuffer[2] = makeAlpha(months[monthNum][2]);
}

void clockDisplay::setDay(int dayNum) {
    // Place LED pattern in day position in buffer, which is 3.
    //Serial.println("Setting day");
    _day = dayNum;
    _displayBuffer[3] = makeNum(dayNum);
}

void clockDisplay::setYear(uint16_t yearNum) {
    // Place LED pattern in year position in buffer, which is 4 and 5.
    _year = yearNum;
    _displayBuffer[4] = makeNum(yearNum / 100);
    _displayBuffer[5] = makeNum(yearNum % 100);
}

void clockDisplay::setYearDirect(uint16_t yearNum) {
    // Place LED pattern in year position directly, which is 4 and 5.
    //useful for setting the year to the present time
    _year = yearNum;
    directCol(4, makeNum(yearNum / 100));
    directCol(5, makeNum(yearNum % 100));
}

void clockDisplay::setHour(uint16_t hourNum) {
    // Place LED pattern in hour position in buffer, which is 6.
    //Serial.println("Setting hour");
    _hour = hourNum;

    // Show it as 12 hour time
    // AM/PM will be set on show() to avoid being overwritten
    if (hourNum == 0 || hourNum > 24) {
        _displayBuffer[6] = makeNum(12);
    } else if (hourNum > 12) {
        // pm
        _displayBuffer[6] = makeNum(hourNum - 12);
    } else if (hourNum <= 12) {
        // am
        _displayBuffer[6] = makeNum(hourNum);
    }
}

void clockDisplay::setMinute(int minNum) {
    // Place LED pattern in minute position in buffer, which is 7.
    //Serial.println("Setting min");
    _minute = minNum;

    if (minNum < 60 ) {
        _displayBuffer[7] = makeNum(minNum);
    } else if (minNum >= 60) {
        _displayBuffer[7] = makeNum(0);
    }
}

void clockDisplay::AM() {
    _displayBuffer[3] = _displayBuffer[3] | 0x0080;
    _displayBuffer[3] = _displayBuffer[3] & 0x7FFF;
    return;
}

void clockDisplay::PM() {
    _displayBuffer[3] = _displayBuffer[3] | 0x8000;
    _displayBuffer[3] = _displayBuffer[3] & 0xFF7F;
    return;
}

void clockDisplay::colonOn() {
    _displayBuffer[4] = _displayBuffer[4] | 0x8080;
    return;
}

void clockDisplay::colonOff() {
    _displayBuffer[4] = _displayBuffer[4] & 0x7F7F;
    return;
}

void clockDisplay::showOnlyMonth(int monthNum) {
    // clears the display RAM and only shows the provided month
    clearDisplay();

    directCol(0, makeAlpha(months[monthNum - 1][0]));
    directCol(1, makeAlpha(months[monthNum - 1][1]));
    directCol(2, makeAlpha(months[monthNum - 1][2]));
}

void clockDisplay::showOnlySave() {
    // clears the display RAM and only shows the word save
    clearDisplay();

    directCol(0, makeAlpha('S'));
    directCol(1, makeAlpha('A'));
    directCol(2, makeAlpha('V'));
    directCol(3, numDigs[10]);  // 10 is an E
                                /*    }
  */
}

void clockDisplay::showOnlySettingVal(const char* setting, int8_t val, bool clear) {
    if (clear)
        clearDisplay();

    int8_t c = 0;
    while (c < 3 && setting[c]) {
        directCol(c, makeAlpha(setting[c]));
        c++;
    }

    if (val >= 0 && val < 100)
        directCol(3, makeNum(val));
    else
        directCol(3, 0x00);
}

void clockDisplay::showOnlyDay(int dayNum) {
    // clears the display RAM and only shows the provided day
    clearDisplay();
    directCol(3, makeNum(dayNum));
}

void clockDisplay::showOnlyYear(int yearNum) {
    // clears the display RAM and only shows the provided year
    clearDisplay();

    directCol(4, makeNum(yearNum / 100));
    directCol(5, makeNum(yearNum % 100));
}

void clockDisplay::showOnlyHour(int hourNum) {
    // clears the display RAM and only shows the provided hour
    clearDisplay();

    if (hourNum == 0) {
        directCol(6, makeNum(12));
        directAM();
    }

    else if (hourNum > 12) {
        // pm
        directCol(6, makeNum(hourNum - 12));
    } else {
        // am
        directCol(6, makeNum(hourNum));
        directAM();
    }

    if (hourNum > 11) {
        directPM();
    } else {
        directAM();
    }
}

void clockDisplay::showOnlyMinute(int minuteNum) {
    // clears the display RAM and only shows the provided minute
    clearDisplay();
    directCol(7, makeNum(minuteNum));
}

void clockDisplay::directCol(int col, int segments) {
    // write directly to a column with supplied segments
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

void clockDisplay::directAM() {
    Wire.beginTransmission(_address);
    Wire.write(0x6);  // 2 bytes per col * position, day is at pos
    // leave buffer intact, direclty write to display
    Wire.write(0x80);
    Wire.write(0x0);
    Wire.endTransmission();
}

void clockDisplay::directPM() {
    Wire.beginTransmission(_address);
    Wire.write(0x6);  // 2 bytes per col * position, day is at pos
    // leave buffer intact, direclty write to display
    Wire.write(0x0);
    Wire.write(0x80);
    Wire.endTransmission();
}

void clockDisplay::setDateTime(DateTime dt) {
    // Set the displayed time with supplied DateTime object
    //
    // DateTime implemention does not work for years < 2000!
    setMonth(dt.month());
    setDay(dt.day());
    setYear(dt.year());
    setHour(dt.hour());
    setMinute(dt.minute());
}

void clockDisplay::setDS3232time(byte second, byte minute, byte hour, byte dayOfWeek, byte dayOfMonth, byte month, byte year) {
    Wire.beginTransmission(DS3232_I2CADDR);
    Wire.write(0);                     // sends 00h - seconds register
    Wire.write(decToBcd(second));      // set seconds
    Wire.write(decToBcd(minute));      // set minutes
    Wire.write(decToBcd(hour));        // set hours
    Wire.write(decToBcd(dayOfWeek));   // set day of week (1=Sunday, 7=Saturday)
    Wire.write(decToBcd(dayOfMonth));  // set date (1~31)
    Wire.write(decToBcd(month));       // set month
    Wire.write(decToBcd(year));        // set year (0~99)
    Wire.endTransmission();
}

// Convert normal decimal numbers to binary coded decimal
byte clockDisplay::decToBcd(byte val) {
    return ((val / 10 * 16) + (val % 10));
}

void clockDisplay::setFromStruct(dateStruct* s) {
    // Set time from array, YEAR, MONTH, DAY, HOUR, MIN
    // Values not checked for correctness
    setYear(s->year);
    setMonth(s->month);
    setDay(s->day);
    setHour(s->hour);
    setMinute(s->minute);
}

DateTime clockDisplay::getDateTime() {
    // returns a DateTime that we're set to
    // this could be broken, DateTime implementation doesn't work with years < 2000
    return DateTime(_year, _month, _day, _hour, _minute, 0);
}

uint8_t clockDisplay::getMonth() {
    return _month;
}

uint8_t clockDisplay::getDay() {
    return _day;
}

uint16_t clockDisplay::getYear() {
    return _year;
}

uint8_t clockDisplay::getHour() {
    return _hour;
}

uint8_t clockDisplay::getMinute() {
    return _minute;
}
void clockDisplay::setColon(bool col) {
    // set true to turn it on
    _colon = col;
}

void clockDisplay::setRTC(bool rtc) {
    // track if this is will be holding real time.
    _rtc = rtc;
}

bool clockDisplay::isRTC() {
    // is this an real time display?
    return _rtc;
}
