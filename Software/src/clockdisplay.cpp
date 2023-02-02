/*
 * -------------------------------------------------------------------
 * CircuitSetup.us Time Circuits Display
 * (C) 2021-2022 John deGlavina https://circuitsetup.us
 * (C) 2022-2023 Thomas Winischhofer (A10001986)
 * https://github.com/realA10001986/Time-Circuits-Display-A10001986
 *
 * Clockdisplay Class: Handles the TC LED segment displays
 *
 * Based on code by John Monaco, Marmoset Electronics
 * https://www.marmosetelectronics.com/time-circuits-clock
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
#include <Wire.h>
#include <EEPROM.h>

#include "clockdisplay.h"
#include "tc_font.h"

#define CD_MONTH_POS  0
#ifdef IS_ACAR_DISPLAY      // A-Car (2-digit-month) ---------------------
#define CD_MONTH_SIZE 1     //      number of words
#define CD_MONTH_DIGS 2     //      number of digits/letters
#else                       // All others (3-char month) -----------------
#define CD_MONTH_SIZE 3     //      number of words
#define CD_MONTH_DIGS 3     //      number of digits/letters
#endif                      // -------------------------------------------
#define CD_DAY_POS    3
#define CD_YEAR_POS   4
#define CD_HOUR_POS   6
#define CD_MIN_POS    7

#define CD_AMPM_POS   CD_DAY_POS
#define CD_COLON_POS  CD_YEAR_POS

#define CD_BUF_SIZE   8     //      in words (16bit)

extern bool     alarmOnOff;
#ifdef TC_HAVEGPS
extern bool     gpsHaveFix();
#endif

extern uint64_t timeDifference;
extern bool     timeDiffUp;

extern uint64_t dateToMins(int year, int month, int day, int hour, int minute);
extern void     minsToDate(uint64_t total, int& year, int& month, int& day, int& hour, int& minute);

extern int      daysInMonth(int month, int year);

extern bool FlashROMode;
extern bool readFileFromSD(const char *fn, uint8_t *buf, int len);
extern bool writeFileToSD(const char *fn, uint8_t *buf, int len);
extern bool readFileFromFS(const char *fn, uint8_t *buf, int len);
extern bool writeFileToFS(const char *fn, uint8_t *buf, int len);

static const char months[12][4] = {
      "JAN", "FEB", "MAR", "APR", "MAY", "JUN",
      "JUL", "AUG", "SEP", "OCT", "NOV", "DEC"
};

static const char *fnLastYear = "/tcdly";
static const char *fnEEPROM[3] = {
    "/tcddt", "/tcdpt", "/tcdlt"
};

// Store i2c address and eeprom data location
clockDisplay::clockDisplay(uint8_t did, uint8_t address, int saveAddress)
{
    _did = did;
    _address = address;
    _saveAddress = saveAddress;
}

// Start the display
void clockDisplay::begin()
{
    directCmd(0x20 | 1); // turn on oscillator

    clearBuf();          // clear buffer
    setBrightness(15);   // setup initial brightness
    clearDisplay();      // clear display RAM
    on();                // turn it on
}

// Turn on the display
void clockDisplay::on()
{
    directCmd(0x80 | 1);
}

// Turn on the display unless off due to night mode
void clockDisplay::onCond()
{
    if(!_nightmode || !_NmOff) {
        directCmd(0x80 | 1);
    }
}

// Turn off the display
void clockDisplay::off()
{
    directCmd(0x80);
}

// Turn on all LEDs
// Not for use in normal operation
// Red displays apparently draw more power
// and might remain dark after repeatedly
// calling this.
void clockDisplay::realLampTest()
{
    Wire.beginTransmission(_address);
    Wire.write(0x00);  // start at address 0x0

    for(int i = 0; i < CD_BUF_SIZE*2; i++) {
        Wire.write(0xff);
    }
    Wire.endTransmission();
}

// Turn on some LEDs
// Used for effects and brightness keypad menu
void clockDisplay::lampTest()
{
    Wire.beginTransmission(_address);
    Wire.write(0x00);  // start at address 0x0

    for(int i = 0; i < CD_BUF_SIZE*2; i++) {
        Wire.write(0x55);
    }
    Wire.endTransmission();
}

// Clear the buffer
void clockDisplay::clearBuf()
{
    // must call show() to actually clear display

    for(int i = 0; i < CD_BUF_SIZE; i++) {
        _displayBuffer[i] = 0;
    }
}

// Set display brightness
// Valid brighness levels are 0 to 15. Default is 15.
// 255 sets it to previous level
uint8_t clockDisplay::setBrightness(uint8_t level, bool setInitial)
{
    if(level == 255)
        level = _brightness;    // restore to old val

    _brightness = setBrightnessDirect(level);

    if(setInitial) _origBrightness = _brightness;

    return _brightness;
}

void clockDisplay::resetBrightness()
{
    _brightness = setBrightnessDirect(_origBrightness);
    // This forces handleNM to reset NM-brightness if
    // this is called while NM is active.
    if(!_NmOff) _oldnm = 0;
}

uint8_t clockDisplay::setBrightnessDirect(uint8_t level)
{
    if(level > 15)
        level = 15;

    directCmd(0xE0 | level);  // Dimming command

    return level;
}

uint8_t clockDisplay::getBrightness()
{
    return _brightness;
}

void clockDisplay::set1224(bool hours24)
{
    _mode24 = hours24;
}

bool clockDisplay::get1224(void)
{
    return _mode24;
}

void clockDisplay::setNightMode(bool mymode)
{
    _nightmode = mymode;
}

bool clockDisplay::getNightMode(void)
{
    return _nightmode;
}

void clockDisplay::setNMOff(bool NMOff)
{
    _NmOff = NMOff;
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
    // ATTN: DateTime implemention does not work for years < 2000, > 2099!

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
    #ifdef TC_DBG
    if(isRTC()) {
        Serial.println(F("Clockdisplay: Internal error; setFromStruct() called for RTC"));
    }
    #endif
    
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

// Show month, assumes showAnimate1() was called before
void clockDisplay::showAnimate2()
{
    if(_nightmode && _NmOff)
        return;

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

    #ifdef TC_DBG
    if(isRTC()) {
        Serial.printf("ClockDisplay: _yearoffset set to %d\n", yearOffs);
    }
    #endif
}

// Place LED pattern in year position in buffer
void clockDisplay::setYear(uint16_t yearNum)
{
    if((int16_t)yearNum - _yearoffset < 0) {    // ny0: < 1
        #ifdef TC_DBG
        Serial.printf("Clockdisplay: setYear: Bad year: %d / yearOffset %d\n", yearNum, _yearoffset);
        #endif
        
        yearNum = _yearoffset;                  // ny0: yo+1
    }

    _year = yearNum;
    yearNum -= _yearoffset;

    while(yearNum >= 10000)
        yearNum -= 10000;

    _displayBuffer[CD_YEAR_POS]     = makeNum(yearNum / 100);
    _displayBuffer[CD_YEAR_POS + 1] = makeNum(yearNum % 100);

    #ifdef TC_HAVEGPS
    if(_did == DISP_PRES) {
        if(gpsHaveFix())
            _displayBuffer[CD_YEAR_POS + 1] |= 0x8000;
    }
    #endif
}

// Place LED pattern in month position in buffer
void clockDisplay::setMonth(int monthNum)
{
    if(monthNum < 1 || monthNum > 12) {
        #ifdef TC_DBG
        Serial.printf("Clockdisplay: setMonth: Bad month: %d\n", monthNum);
        #endif
        monthNum = (monthNum > 12) ? 12 : 1;
    }

    _month = monthNum;

#ifdef IS_ACAR_DISPLAY
    _displayBuffer[CD_MONTH_POS] = makeNum(monthNum);
#else
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
        #ifdef TC_DBG
        Serial.printf("Clockdisplay: setDay: Bad day: %d\n", dayNum);
        #endif
        dayNum = (dayNum < 1) ? 1 : maxDay;
    }

    _day = dayNum;

    _displayBuffer[CD_DAY_POS] = makeNum(dayNum);
}

// Place LED pattern in hour position in buffer.
void clockDisplay::setHour(uint16_t hourNum)
{
    if(hourNum > 23) {
        #ifdef TC_DBG
        Serial.printf("Clockdisplay: setHour: Bad hour: %d\n", hourNum);
        #endif
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
        #ifdef TC_DBG
        Serial.printf("Clockdisplay: setMinute: Bad Minute: %d\n", minNum);
        #endif
        minNum = (minNum > 59) ? 59 : 0;
    }

    _minute = minNum;

    _displayBuffer[CD_MIN_POS] = makeNum(minNum);

    if(_did == DISP_PRES) {
        if(alarmOnOff)
            _displayBuffer[CD_MIN_POS] |= 0x8000;
    }
}

void clockDisplay::setDST(int8_t isDST)
{
    if(isRTC()) {
        _isDST = isDST;
    }
}

void clockDisplay::setColon(bool col)
{
    // set true to turn it on
    // colon is off in night mode
    
    if(_nightmode) col = false;
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

uint16_t clockDisplay::getDisplayYear()
{
    return _year - _yearoffset;
}

uint8_t clockDisplay::getHour()
{
    return _hour;
}

uint8_t clockDisplay::getMinute()
{
    return _minute;
}

int8_t clockDisplay::getDST()
{
    return _isDST;
}

// Put data directly on display (bypass buffer) --------------------------------


// clears the display RAM and only shows the provided month
void clockDisplay::showOnlyMonth(int monthNum)
{
    clearDisplay();

    if(monthNum < 1 || monthNum > 12) {
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

    while(yearNum >= 10000) 
        yearNum -= 10000;

    directCol(CD_YEAR_POS,     makeNum(yearNum / 100));
    directCol(CD_YEAR_POS + 1, makeNum(yearNum % 100));
}

// clears the display RAM and only shows the provided hour
void clockDisplay::showOnlyHour(int hourNum, bool force24)
{
    clearDisplay();

    if(!_mode24 && !force24) {

        if(hourNum == 0) {
            directCol(CD_HOUR_POS, makeNum(12));
        } else if(hourNum > 12) {
            directCol(CD_HOUR_POS, makeNum(hourNum - 12));
        } else {
            directCol(CD_HOUR_POS, makeNum(hourNum));
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


// Shows the given text
void clockDisplay::showTextDirect(const char *text, bool clear, bool corr6)
{
    int idx = 0, pos = CD_MONTH_POS;
    int temp = 0;

    _corr6 = corr6;

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

    while(pos < CD_DAY_POS) {
        directCol(pos, 0);
        pos++;
    }
    
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

    if(clear) {
        while(pos <= CD_MIN_POS) {
            directCol(pos, 0);
            pos++;
        }
    }
    
    _corr6 = false;
}

// clears the display RAM and only shows the provided 2 numbers (parts of IP)
void clockDisplay::showHalfIPDirect(int a, int b, bool clear)
{
    char buf[16];
    #ifdef IS_ACAR_DISPLAY
    const char *fmt1 = "%3d  %3d";
    const char *fmt2 = "%2d   %3d";
    #else
    const char *fmt = "%3d   %3d";
    #endif

    #ifdef IS_ACAR_DISPLAY
    sprintf(buf, (a >= 100) ? fmt1 : fmt2, a, b);
    #else
    sprintf(buf, fmt, a, b);
    #endif
    showTextDirect(buf, clear);
}

// Shows a text part and a number
void clockDisplay::showSettingValDirect(const char* setting, int8_t val, bool clear)
{
    showTextDirect(setting, clear);

    int field = (strlen(setting) <= CD_MONTH_DIGS) ? CD_DAY_POS : CD_MIN_POS;

    if(val >= 0 && val < 100)
         directCol(field, makeNum(val));
    else
         directCol(field, 0x00);
}

#ifdef TC_HAVETEMP
void clockDisplay::showTempDirect(float temp, bool tempUnit, bool animate)
{
    char buf[32];
    const char *ttem = animate ? "    " : "TEMP";
    int t2; 

    if(!handleNM())
        return;

    if(isnan(temp)) {
        #ifdef IS_ACAR_DISPLAY
        sprintf(buf, "%s  ----~%c", ttem, tempUnit ? 'C' : 'F');
        #else
        sprintf(buf, "%s   ----~%c", ttem, tempUnit ? 'C' : 'F');
        #endif
    } else {
        t2 = abs((int)(temp * 100.0) - ((int)temp * 100));
        #ifdef IS_ACAR_DISPLAY
        sprintf(buf, "%s%4d%02d~%c", ttem, (int)temp, t2, tempUnit ? 'C' : 'F');
        #else
        sprintf(buf, "%s %4d%02d~%c", ttem, (int)temp, t2, tempUnit ? 'C' : 'F');
        #endif
    }
    
    _yearDot = true;
    showTextDirect(buf);
    _yearDot = false;

    if(animate || (_NmOff && (_oldnm > 0)) ) on();

    if(_NmOff) _oldnm = 0;
}

void clockDisplay::showHumDirect(int hum, bool animate)
{
    char buf[16];
    const char *thum = animate ? "        " : "HUMIDITY";

    if(!handleNM())
        return;

    if(hum < 0) {
        #ifdef IS_ACAR_DISPLAY
        sprintf(buf, "%s--%%&", thum);
        #else
        sprintf(buf, "%s --%%&", thum);
        #endif
    } else {
        #ifdef IS_ACAR_DISPLAY
        sprintf(buf, "%s%2d%%&", thum, hum);
        #else
        sprintf(buf, "%s %2d%%&", thum, hum);
        #endif
    }

    showTextDirect(buf);

    if(animate || (_NmOff && (_oldnm > 0)) ) on();

    if(_NmOff) _oldnm = 0;
}
#endif

// Save & load data ------------------------------------------------------------


/*
 * Save date/time and other settings to NVM storage (Flash/SD)
 * Only non-RTC displays save their time.
 * "Persistent" time travel causes more wear than "Non-Persistent".
 */
bool clockDisplay::save()
{
    uint8_t savBuf[10]; // Sic! Sum written to last byte by saveNVMData

    if(_saveAddress < 0)
        return false;

    if(!isRTC()) {

        // Non-RTC: Save time

        #ifdef TC_DBG
        Serial.println(F("Clockdisplay: Saving non-RTC settings"));
        #endif

        savBuf[0] = _year & 0xff;
        savBuf[1] = (_year >> 8) & 0xff;
        savBuf[2] = _yearoffset & 0xff;
        savBuf[3] = (_yearoffset >> 8) & 0xff;
        savBuf[4] = _month;
        savBuf[5] = _day;
        savBuf[6] = _hour;
        savBuf[7] = _minute;
        savBuf[8] = 0;        // unused

    } else {

        // RTC: Save IsDST, yearoffs, timeDiff
        
        #ifdef TC_DBG
        Serial.println(F("Clockdisplay: Saving RTC settings"));
        #endif

        savBuf[0] = _isDST + 1;
        
        savBuf[1] = _yearoffset & 0xff;
        savBuf[2] = (_yearoffset >> 8) & 0xff;

        savBuf[3] = (timeDifference >> 32) & 0xff;
        savBuf[4] = (timeDifference >> 24) & 0xff;
        savBuf[5] = (timeDifference >> 16) & 0xff;
        savBuf[6] = (timeDifference >>  8) & 0xff;
        savBuf[7] =  timeDifference        & 0xff;

        savBuf[8] = timeDiffUp ? 1 : 0;

    }

    saveNVMData(savBuf);

    return true;
}

// Save YOffs and isDST, and clear timeDifference in storage
bool clockDisplay::saveYOffs()
{
    uint8_t savBuf[10];  // Sic! Sum written to last byte by saveNVMData
    int i;

    if(!isRTC() || _saveAddress < 0)
        return false;

    // RTC: Save yearoffs; zero timeDifference

    #ifdef TC_DBG
    Serial.println(F("Clockdisplay: Saving RTC/YOffs setting"));
    #endif

    memset(savBuf, 0, 10);
    
    savBuf[0] = _isDST + 1;
    
    savBuf[1] = _yearoffset & 0xff;
    savBuf[2] = (_yearoffset >> 8) & 0xff;

    saveNVMData(savBuf);

    return true;
}

// Save given Year as "lastYear" for previous time state at boot
bool clockDisplay::saveLastYear(uint16_t theYear)
{
    uint8_t savBuf[4];

    if(!isRTC())
        return false;

    // Check if value identical to last one written
    if(_lastWrittenLY == theYear)
        return true;

    // If not, read to check if identical to stored
    _lastWrittenLY = loadLastYear();
    if(_lastWrittenLY == theYear)
        return true;

    #ifdef TC_DBG
    Serial.printf("Clockdisplay: Saving RTC/LastYear to %s\n", FlashROMode ? "SD" : "Flash");
    #endif
    
    savBuf[0] = theYear & 0xff;
    savBuf[1] = (theYear >> 8) & 0xff;    
    savBuf[2] = savBuf[0] ^ 0xff;
    savBuf[3] = savBuf[1] ^ 0xff;

    if(FlashROMode) {
        writeFileToSD(fnLastYear, savBuf, 4);
    } else {
        writeFileToFS(fnLastYear, savBuf, 4);
        /*
        for(uint8_t i = 0; i < 4; i++) {
            EEPROM.write(PRES_LY_PREF + i, savBuf[i]);
        }
        EEPROM.commit();
        */
    }

    _lastWrittenLY = theYear;

    return true;
}

/*
 * Load data from NVM storage
 *
 */
bool clockDisplay::load(int initialBrightness)
{
    uint8_t loadBuf[10];

    if(_saveAddress < 0)
        return false;

    if(initialBrightness >= 0) {
        if(initialBrightness > 15)
            initialBrightness = 15;

        _origBrightness = initialBrightness;
    }

    if(!isRTC()) {

        if(loadNVMData(loadBuf)) {

            #ifdef TC_DBG
            Serial.println(F("Clockdisplay: Loaded non-RTC settings"));
            #endif

            setYearOffset((loadBuf[3] << 8) | loadBuf[2]);
            setYear((loadBuf[1] << 8) | loadBuf[0]);
            setMonth(loadBuf[4]);
            setDay(loadBuf[5]);
            setHour(loadBuf[6]);
            setMinute(loadBuf[7]);

            // Set initial _brightness
            setBrightness(_origBrightness);

            return true;

        }

    } else {

        // RTC: IsDST, yearoffs & timeDiff is saved

        if(loadNVMData(loadBuf)) {

              _isDST = (int8_t)loadBuf[0] - 1;

              setYearOffset((loadBuf[2] << 8) | loadBuf[1]);

              timeDifference = ((uint64_t)loadBuf[3] << 32) |  // Dumb casts for
                               ((uint64_t)loadBuf[4] << 24) |  // silencing compiler
                               ((uint64_t)loadBuf[5] << 16) |
                               ((uint64_t)loadBuf[6] <<  8) |
                                (uint64_t)loadBuf[7];

              timeDiffUp = loadBuf[8] ? true : false;

              #ifdef TC_DBG
              Serial.println(F("Clockdisplay: Loaded RTC settings"));
              #endif

        } else {

              _isDST = -1;

              setYearOffset(0);

              timeDifference = 0;

              #ifdef TC_DBG
              Serial.println(F("Clockdisplay: Invalid RTC data"));
              #endif

        }

        // Reinstate _brightness to keep old behavior
        setBrightness(_origBrightness);

        return true;
    }

    #ifdef TC_DBG
    Serial.println(F("Clockdisplay: Invalid Non-RTC data"));
    #endif

    // Do NOT clear NVM if data is invalid.
    // All 0s are as bad, wait for NVM to be
    // written by application on purpose

    return false;
}

// Only load yearOffset from NVM
// !!! Does not *SET* yearOffs, just returns it !!!
// Returns impossible values in case of error to make
// comparisons fail orderly
int16_t clockDisplay::loadYOffs()
{
    uint8_t loadBuf[10];

    if(_saveAddress < 0 || !isRTC())
        return -11111;

    if(loadNVMData(loadBuf)) {
        return ((loadBuf[2] << 8) | loadBuf[1]);
    }

    return -11111;
}

// Only load isDST from NVM
// !!! Does not *SET* it, just returns it !!!
// Returns <= -1 in case of error (which makes
// comparisons fail orderly)
int8_t clockDisplay::loadDST()
{
    uint8_t loadBuf[10];

    if(_saveAddress < 0 || !isRTC())
        return -2;

    if(loadNVMData(loadBuf)) {
        return (int8_t)loadBuf[0] - 1;
    }

    return -1;
}

// Load lastYear from NVM
// !!! Does not *SET* it, just returns it !!!
int16_t clockDisplay::loadLastYear()
{
    uint8_t loadBuf[4];
    bool writeNew = false;

    if(!isRTC())
        return -2;

    memset(loadBuf, 0, 4);

    if(FlashROMode) {
        readFileFromSD(fnLastYear, loadBuf, 4);
    } else {
        if(!readFileFromFS(fnLastYear, loadBuf, 4)) {
            for(uint8_t i = 0; i < 4; i++) {
                loadBuf[i] = EEPROM.read(PRES_LY_PREF + i);
            }
            writeNew = true;
        }
    }

    if( (loadBuf[0] == (loadBuf[2] ^ 0xff)) &&
        (loadBuf[1] == (loadBuf[3] ^ 0xff)) ) {

        if(writeNew) {
            #ifdef TC_DBG
            Serial.println("loadLastYear: Transitioning to flash FS");
            #endif
            if(writeFileToFS(fnLastYear, loadBuf, 4)) {
                // Invalidate EEPROM contents
                EEPROM.write(PRES_LY_PREF, loadBuf[0] + 1);
                EEPROM.commit();
            }
        }

        return (int16_t)((loadBuf[1] << 8) | loadBuf[0]);

    }

    return -1;
}


// Private functions ###########################################################

bool clockDisplay::saveNVMData(uint8_t *savBuf, bool noReadChk)
{
    uint16_t sum = 0;
    uint8_t loadBuf[10];
    bool skipSave = false;

    // Read to check if values identical to currently stored (reduce wear)        
    if(!noReadChk && loadNVMData(loadBuf)) {
        skipSave = true;
        for(uint8_t i = 0; i < 9; i++) {
            if(savBuf[i] != loadBuf[i]) {
                skipSave = false;
                break;
            }
        }
    }

    if(!skipSave) {
        #ifdef TC_DBG
        Serial.printf("saveNVMData to %s\n", FlashROMode ? "SD" : "Flash");
        #endif
        for(uint8_t i = 0; i < 9; i++) {
            sum += (savBuf[i] ^ 0x55);
        }
        savBuf[9] = sum & 0xff;
        if(FlashROMode) {
            return writeFileToSD(fnEEPROM[_did], savBuf, 10);
        } else {
            return writeFileToFS(fnEEPROM[_did], savBuf, 10);
            /*
            for(uint8_t i = 0; i < 10; i++) {
               EEPROM.write(_saveAddress + i, savBuf[i]);
            }
            EEPROM.commit();
            */
        }
    }
    return true;
}

bool clockDisplay::loadNVMData(uint8_t *loadBuf)
{
    uint16_t sum = 0;
    uint8_t  mxor = 0x55;
    bool     newSum = true;
    bool     writeNew = false;
    bool     sumOK = false;

    memset(loadBuf, 0, 10);

    #ifdef TC_DBG
    Serial.printf("loadNVMData from %s\n", FlashROMode ? "SD" : "Flash");
    #endif
        
    if(FlashROMode) {
        readFileFromSD(fnEEPROM[_did], loadBuf, 10);
    } else {
        if(!readFileFromFS(fnEEPROM[_did], loadBuf, 10)) {
            for(uint8_t i = 0; i < 10; i++) {
                loadBuf[i] = EEPROM.read(_saveAddress + i);
            }
            if(!isRTC()) {
                newSum = false;
                mxor = 0x00;
            }
            writeNew = true;
        }
    }
    for(uint8_t i = 0; i < 9; i++) {
        sum += (loadBuf[i] ^ mxor);
    }

    sumOK = newSum ? ((sum & 0xff) == loadBuf[9]) : ((sum != 0) && ((sum & 0xff) == loadBuf[9]));

    if(writeNew && sumOK) {
        #ifdef TC_DBG
        Serial.println("loadNVMData: Transitioning to flash FS");
        #endif
        if(saveNVMData(loadBuf, true)) {
            // Invalidate EEPROM contents
            EEPROM.write(_saveAddress, loadBuf[0] + 1);
            EEPROM.commit();
        }
    }
    
    return sumOK;
}

// Returns bit pattern for provided value 0-9 or number provided as a char '0'-'9'
// for display on 7 segment display
uint8_t clockDisplay::getLED7NumChar(uint8_t value)
{
    if(value >= '0' && value <= '9') {
        return numDigs[value - 48];
    } else if(value <= 9) {
        return numDigs[value];
    }
    return 0;   // blank on invalid
}

// Returns bit pattern for provided character for display on 7 segment display
uint8_t clockDisplay::getLED7AlphaChar(uint8_t value)
{
    if(value >= '0' && value <= '9') {
        // Use common "6" pattern to differ from "b" if desired
        if(_corr6 && value == '6') return numDigs[6] | 0x01;
        return numDigs[value - 48];
    } else if(value >= 'A' && value <= 'Z') {
        return numDigs[value - 'A' + 10];
    } else if(value >= 'a' && value <= 'z') {
        return numDigs[value - 'a' + 10];
    }
    switch(value) {
    case '-':
        return numDigs[36];
    case '(':
    case '[':
        return numDigs[37];
    case ')':
    case ']':
        return numDigs[38];
    case '_':
    case '.':
        return numDigs[39];
    case '~':   // used for "°"
        return numDigs[40];
    case '%':   // left part of double-digit "%"
        return numDigs[41];
    case '&':   // right part of double-digit "%"
        return numDigs[42];
    case '?':
        return numDigs[43];
    }

    return 0;
}

// Returns bit pattern for provided character for display on alphanumeric 14 segment display
#ifndef IS_ACAR_DISPLAY
uint16_t clockDisplay::getLEDAlphaChar(uint8_t value)
{
    if(value < 32 || value > 127) return 0;

    // For text, use common "6" pattern to conform with 7-seg-part
    if(_corr6 && value == '6') return alphaChars['6' - 32] | 0x0001;

    return alphaChars[value-32];
}
#endif

// Make a 2 digit number from the array and return the segment data
// (makes leading 0s)
uint16_t clockDisplay::makeNum(uint8_t num)
{
    uint16_t segments = 0;

    // Each position holds two digits, high byte is 1's, low byte is 10's

    segments = getLED7NumChar(num % 10) << 8;     // Place 1's in upper byte
    segments |= getLED7NumChar(num / 10);         // 10's in lower byte

    return segments;
}

// Make a 2 digit number from the array and return the segment data
// (no leading 0s)
#if 0
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
#endif

// Directly write to a column with supplied segments
// (leave buffer intact, directly write to display)
void clockDisplay::directCol(int col, int segments)
{
    if(_yearDot && (col == CD_YEAR_POS + 1)) {
        segments |= 0x8000;
    }
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

bool clockDisplay::handleNM()
{
    if(_nightmode) {
        if(_NmOff) {
            off();
            _oldnm = 1;
            return false;
        } else {
            if(_oldnm < 1) {
                setBrightness(0);
            }
            _oldnm = 1;
        }
    } else if(!_NmOff) {
        if(_oldnm > 0) {
            setBrightness(_origBrightness);
        }
        _oldnm = 0;
    }

    return true;
}

// Show the buffer
void clockDisplay::showInt(bool animate)
{
    int i = 0;

    if(!handleNM())
        return;

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
        for(i = 0; i < CD_MONTH_SIZE; i++) {
            Wire.write(0x00);  // blank month
            Wire.write(0x00);
        }
        i = CD_DAY_POS;
    }

    for(; i < CD_BUF_SIZE; i++) {
        Wire.write(_displayBuffer[i] & 0xFF);
        Wire.write(_displayBuffer[i] >> 8);
    }

    Wire.endTransmission();

    if(animate || (_NmOff && (_oldnm > 0)) ) on();

    if(_NmOff) _oldnm = 0;
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
    Wire.write(CD_AMPM_POS * 2);
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

void clockDisplay::directCmd(uint8_t val)
{
    Wire.beginTransmission(_address);
    Wire.write(val);
    Wire.endTransmission();
}
