/*
 * -------------------------------------------------------------------
 * CircuitSetup.us Time Circuits Display
 * (C) 2021-2022 John deGlavina https://circuitsetup.us
 * (C) 2022-2025 Thomas Winischhofer (A10001986)
 * https://github.com/realA10001986/Time-Circuits-Display
 * https://tcd.out-a-ti.me
 *
 * Clockdisplay Class: Handles the TC LED segment displays
 *
 * Based on code by John Monaco, Marmoset Electronics
 * https://www.marmosetelectronics.com/time-circuits-clock
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
#include <Wire.h>

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

extern bool     alarmOnOff;
#ifdef TC_HAVEGPS
extern bool     gpsHaveFix();
#endif

extern uint64_t timeDifference;
extern bool     timeDiffUp;

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

#ifdef BTTF3_ANIM
static const uint8_t idxtbl[] = {
        0, 
        CD_DAY_POS, CD_DAY_POS, 
        CD_YEAR_POS, CD_YEAR_POS, 
        CD_YEAR_POS + 1, CD_YEAR_POS + 1, 
        CD_HOUR_POS, 
        CD_HOUR_POS, CD_HOUR_POS, 
        CD_MIN_POS, CD_MIN_POS
};
#endif

static const char *nullStr = "";

static const char *fnEEPROM[3] = {
    "/tcddt", "/tcdpt", "/tcdlt"
};
static const char *fnSD[3] = {
    "/tcddt.bin", "/tcdpt.bin", "/tcdlt.bin"
};
static const char *fnLastYear   = "/tcdly";
static const char *fnLastYearSD = "/tcdly.bin";


/*
 * ClockDisplay class
 */

// Store i2c address and display ID
clockDisplay::clockDisplay(uint8_t did, uint8_t address)
{
    _did = did;
    _address = address;
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

void clockDisplay::onBlink(uint8_t blink)
{
    directCmd(0x80 | 1 | ((blink & 0x03) << 1)); 
}

// Turn on all LEDs
// Not for use in normal operation
// Red displays apparently draw more power
// and might remain dark after repeatedly
// calling this.
#if 0
void clockDisplay::realLampTest()
{
    Wire.beginTransmission(_address);
    Wire.write(0x00);  // start address

    for(int i = 0; i < CD_BUF_SIZE*2; i++) {
        Wire.write(0xff);
    }
    Wire.endTransmission();
}
#endif

// Turn on some LEDs
// Used for effects and brightness keypad menu
void clockDisplay::lampTest(bool randomize)
{
    Wire.beginTransmission(_address);
    Wire.write(0x00);  // start address

    uint32_t rnd = esp_random();

    for(int i = 0; i < CD_BUF_SIZE; i++) {
        Wire.write(randomize ? ((rand() % 0x7f) ^ rnd) & 0x7f : 0xaa);
        Wire.write(randomize ? (((rand() % 0x7f) ^ (rnd >> 8))) & 0x77 : 0x55);
    }
    
    Wire.endTransmission();
}

// Clear the buffer
void clockDisplay::clearBuf()
{
    for(int i = 0; i < CD_BUF_SIZE; i++) {
        _displayBuffer[i] = 0;
    }
}

// Set display brightness
// Valid brighness levels are 0 to 15.
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

    directCmd(0xe0 | level);

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

bool clockDisplay::isRTC()
{
    return _rtc;
}


// Setup date in buffer --------------------------------------------------------


// Set the displayed time with supplied DateTime object
void clockDisplay::setDateTime(DateTime& dt)
{
    setYear(dt.year());
    setMonth(dt.month());
    setDay(dt.day());
    setHour(dt.hour());
    setMinute(dt.minute());
}

// Set YEAR, MONTH, DAY, HOUR, MIN from structure
void clockDisplay::setFromStruct(const dateStruct *s)
{    
    setYear(s->year);
    setMonth(s->month);
    setDay(s->day);
    setHour(s->hour);
    setMinute(s->minute);
}

void clockDisplay::setFromParms(int year, int month, int day, int hour, int minute)
{
    setYear(year);
    setMonth(month);
    setDay(day);
    setHour(hour);
    setMinute(minute);
}

void clockDisplay::getToParms(int& year, int& month, int& day, int& hour, int& minute)
{
    year = getYear();
    month = getMonth();
    day = getDay();
    hour = getHour();
    minute = getMinute();
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
#ifndef BTTF3_ANIM
void clockDisplay::showAnimate2()
{
    if(_nightmode && _NmOff)
        return;

    Wire.beginTransmission(_address);
    Wire.write(0x00);
    for(int i = 0; i < CD_BUF_SIZE; i++) {
        Wire.write(_displayBuffer[i] & 0xff);
        Wire.write(_displayBuffer[i] >> 8);
    }
    Wire.endTransmission();
}

#else

void clockDisplay::showAnimate2(int until)
{
    if(_nightmode && _NmOff)
        return;

    Wire.beginTransmission(_address);
    Wire.write(0x00);
    for(int i = 0; i < until; i++) {
        Wire.write(_displayBuffer[i] & 0xff);
        Wire.write(_displayBuffer[i] >> 8);
    }
    for(int i = until; i < CD_BUF_SIZE; i++) {
        Wire.write(0x00);
        Wire.write(0x00);
    }
    Wire.endTransmission();
}

void clockDisplay::showAnimate3(int mystep)
{
    uint16_t buf;
    uint16_t *bu;
    
    if(!mystep) {
        if(!handleNM())
            return;
        off();
        AMPMoff();
        colonOff();
        showAnimate2(CD_DAY_POS);
        on();
        if(_NmOff) _oldnm = 0;
    }
    
    uint8_t lim = idxtbl[mystep] + 1;
    
    switch(mystep) {
    case 1:
    case 3:
    case 5:
    case 8:
    case 10:
        bu = &_displayBuffer[idxtbl[mystep]];
        buf = *bu;
        *bu &= 0xff;
        showAnimate2(lim);
        *bu = buf;
        break;
    case 7:
        if(!_mode24) {
            (_hour < 12) ? AM() : PM();
        }
    default:
        showAnimate2(lim);
    }
}
#endif // BTTF3_ANIM

void clockDisplay::showAlt()
{
    showInt(false, true);
}

// Put the given text into _displayBufferAlt
void clockDisplay::setAltText(const char *text)
{
    int idx = 0, pos = CD_MONTH_POS;
    int temp = 0;

#ifdef IS_ACAR_DISPLAY
    while(text[idx] && pos < (CD_MONTH_POS+CD_MONTH_SIZE)) {
        temp = getLED7AlphaChar(text[idx++]);
        if(text[idx]) {
            temp |= (getLED7AlphaChar(text[idx++]) << 8);
        }
        _displayBufferAlt[pos++] = temp;
    }
#else
    while(text[idx] && pos < (CD_MONTH_POS+CD_MONTH_SIZE)) {
        _displayBufferAlt[pos++] = getLEDAlphaChar(text[idx++]);
    }
#endif

    while(pos < CD_DAY_POS) {
        _displayBufferAlt[pos++] = 0;
    }
    
    pos = CD_DAY_POS;
    while(text[idx] && pos <= CD_MIN_POS) {
        temp = getLED7AlphaChar(text[idx++]);
        if(text[idx]) {
            temp |= (getLED7AlphaChar(text[idx++]) << 8);
        }
        _displayBufferAlt[pos++] = temp;
    }

    while(pos <= CD_MIN_POS) {
        _displayBufferAlt[pos++] = 0;
    }
}


// Set fields in buffer --------------------------------------------------------


void clockDisplay::setMonth(int monthNum)
{
    if(monthNum < 1 || monthNum > 12) {
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

void clockDisplay::setDay(int dayNum)
{
    int maxDay = daysInMonth(_month, _year);

    // It is essential that setDay is called AFTER year
    // and month have been set!

    if(dayNum < 1 || dayNum > maxDay) {
        dayNum = (dayNum < 1) ? 1 : maxDay;
    }

    _day = dayNum;

    _displayBuffer[CD_DAY_POS] = makeNum(dayNum);
}

void clockDisplay::setYear(uint16_t yearNum)
{
    uint16_t seg = 0;

    #ifdef TC_HAVEGPS
    if((_did == DISP_PRES) && gpsHaveFix())
        seg = 0x8000;
    #endif

    _year = yearNum;

    while(yearNum >= 10000)
        yearNum -= 10000;

    _displayBuffer[CD_YEAR_POS]     = makeNum(yearNum / 100);
    _displayBuffer[CD_YEAR_POS + 1] = makeNum(yearNum % 100) | seg;
}

void clockDisplay::setHour(uint16_t hourNum)
{
    uint16_t seg = 0;
    
    if(hourNum > 23)
        hourNum = 23;

    _hour = hourNum;

    if(!_mode24) {
        if(hourNum == 0) {
            hourNum = 12;
        } else if(hourNum > 12) {
            hourNum -= 12; 
        }
    }

    _displayBuffer[CD_HOUR_POS] = makeNum(hourNum);

    // AM/PM will be set on show() to avoid being overwritten
}

void clockDisplay::setMinute(int minNum)
{
    uint16_t seg = ((_did == DISP_PRES) && alarmOnOff) ? 0x8000 : 0;
    
    if(minNum < 0 || minNum > 59) {
        minNum = (minNum > 59) ? 59 : 0;
    }

    _minute = minNum;

    _displayBuffer[CD_MIN_POS] = makeNum(minNum) | seg;
}

void clockDisplay::setColon(bool col)
{
    // set true to turn it on
    // colon is on in night mode
    
    _colon = _nightmode ? true : col;
}

void clockDisplay::setYearOffset(int16_t yearOffs)
{
    if(_rtc) {
        _yearoffset = yearOffs;
        #ifdef TC_DBG
        Serial.printf("ClockDisplay: _yearoffset set to %d\n", yearOffs);
        #endif
    }
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

uint16_t clockDisplay::getYear()
{
    return _year;
}

uint8_t clockDisplay::getHour()
{
    return _hour;
}

uint8_t clockDisplay::getMinute()
{
    return _minute;
}

const char * clockDisplay::getMonthString(uint8_t mon)
{
    if(mon >= 1 && mon <= 12)
        return months[mon-1];
    else
        return nullStr;
}

int16_t clockDisplay::getYearOffset()
{
    return _yearoffset;
}

void clockDisplay::getCompressed(uint8_t *buf, uint8_t& over)
{
    
    buf[0] = (_year << 2) | (_month >> 2);
    buf[1] = (_year >> 6);
    buf[2] = (_month << 6)  | (_day << 1) | (_hour >> 4);
    buf[3] = (_hour << 4) | (_minute >> 2);
    over |= _minute & 0x03;
}

// Put data directly on display (bypass buffer) --------------------------------


void clockDisplay::showMonthDirect(int monthNum, uint16_t dflags)
{
    clearDisplay();

    if(monthNum > 12)
        monthNum = 12;

#ifdef IS_ACAR_DISPLAY
    directCol(CD_MONTH_POS, makeNum(monthNum, dflags));
#else
    if(monthNum > 0) {
        monthNum--;
        directCol(CD_MONTH_POS,     getLEDAlphaChar(months[monthNum][0]));
        directCol(CD_MONTH_POS + 1, getLEDAlphaChar(months[monthNum][1]));
        directCol(CD_MONTH_POS + 2, getLEDAlphaChar(months[monthNum][2]));
    } else {
        directCol(CD_MONTH_POS,     0);
        directCol(CD_MONTH_POS + 1, 0);
        directCol(CD_MONTH_POS + 2, getLEDAlphaChar('_'));
    }
#endif
}

void clockDisplay::showDayDirect(int dayNum, uint16_t dflags)
{
    clearDisplay();

    directCol(CD_DAY_POS, makeNum(dayNum, dflags));
}

void clockDisplay::showYearDirect(int yearNum, uint16_t dflags)
{
    uint16_t seg = 0;
    int y100;

    clearDisplay();

    while(yearNum >= 10000) 
        yearNum -= 10000;

    y100 = yearNum / 100;

    if(!(dflags & CDD_NOLEAD0) || y100) {
        seg = makeNum(y100, dflags);
        if(y100) dflags &= ~CDD_NOLEAD0;
    }
    directCol(CD_YEAR_POS, seg);
    directCol(CD_YEAR_POS + 1, makeNum(yearNum % 100, dflags));
}

void clockDisplay::showHourDirect(int hourNum, uint16_t dflags)
{
    clearDisplay();

    // This assumes that CD_HOUR_POS is different to
    // CD_AMPM_POS

    if(!_mode24 && !(dflags & CDD_FORCE24)) {

        (hourNum > 11) ? directPM() : directAM();

        if(hourNum == 0) {
            hourNum = 12;
        } else if(hourNum > 12) {
            hourNum -= 12;
        }

    } else {

        directAMPMoff();

    }

    directCol(CD_HOUR_POS, makeNum(hourNum, dflags));
}

void clockDisplay::showMinuteDirect(int minuteNum, uint16_t dflags)
{
    clearDisplay();

    directCol(CD_MIN_POS, makeNum(minuteNum, dflags));
}


// Special purpose -------------------------------------------------------------


// Show the given text
void clockDisplay::showTextDirect(const char *text, uint16_t flags)
{
    int idx = 0, pos = CD_MONTH_POS;
    int temp = 0;

    _corr6 = (flags & CDT_CORR6) ? true : false;
    _withColon = (flags & CDT_COLON) ? true : false;

#ifdef IS_ACAR_DISPLAY
    while(text[idx] && pos < (CD_MONTH_POS+CD_MONTH_SIZE)) {
        temp = getLED7AlphaChar(text[idx++]);
        if(text[idx]) {
            temp |= (getLED7AlphaChar(text[idx++]) << 8);
        }
        directCol(pos++, temp);
    }
#else
    while(text[idx] && pos < (CD_MONTH_POS+CD_MONTH_SIZE)) {
        directCol(pos++, getLEDAlphaChar(text[idx++]));
    }
#endif

    while(pos < CD_DAY_POS) {
        directCol(pos++, 0);
    }
    
    pos = CD_DAY_POS;
    while(text[idx] && pos <= CD_MIN_POS) {
        temp = getLED7AlphaChar(text[idx++]);
        if(text[idx]) {
            temp |= (getLED7AlphaChar(text[idx++]) << 8);
        }
        directCol(pos++, temp);
    }

    if(flags & CDT_CLEAR) {
        while(pos <= CD_MIN_POS) {
            directCol(pos++, 0);
        }
    }
    
    _corr6 = _withColon = false;
}

// Clear the display RAM and only show the provided 2 numbers (parts of IP)
void clockDisplay::showHalfIPDirect(int a, int b, uint16_t flags)
{
    char buf[16];
    #ifdef IS_ACAR_DISPLAY
    const char *fmt1 = "%3d  %3d";
    const char *fmt2 = "%2d   %3d";
    #else
    const char *fmt = "%3d   %3d";
    #endif

    if(a > 255) a = 255;    // Avoid buf overflow if numbers too high
    if(b > 255) b = 255;

    #ifdef IS_ACAR_DISPLAY
    sprintf(buf, (a >= 100) ? fmt1 : fmt2, a, b);
    #else
    sprintf(buf, fmt, a, b);
    #endif
    showTextDirect(buf, flags);
}

// Show a text part and a number
void clockDisplay::showSettingValDirect(const char* setting, int8_t val, uint16_t flags)
{
    showTextDirect(setting, flags);

    int field = (strlen(setting) <= CD_MONTH_DIGS) ? CD_DAY_POS : CD_MIN_POS;

    if(!(flags & CDT_BLINK) && (val >= 0 && val < 100))
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
        sprintf(buf, "%s--\x7f\x80", thum);
        #else
        sprintf(buf, "%s --\x7f\x80", thum);
        #endif
    } else {
        #ifdef IS_ACAR_DISPLAY
        sprintf(buf, "%s%2d\x7f\x80", thum, hum);
        #else
        sprintf(buf, "%s %2d\x7f\x80", thum, hum);
        #endif
    }

    showTextDirect(buf);

    if(animate || (_NmOff && (_oldnm > 0)) ) on();

    if(_NmOff) _oldnm = 0;
}
#endif


// Load & save data ------------------------------------------------------------


/* 
 * Load time state (lastYear, yearOffset) from NVM 
 * !!! Does not *SET* values, just returns them !!!
 */
int16_t clockDisplay::loadClockStateData(int16_t& yoffs)
{
    uint8_t loadBuf[8] = { 0 };

    yoffs = 0;

    if(!_rtc)
        return -2;

    if(FlashROMode) {
        readFileFromSD(fnLastYearSD, loadBuf, 8);
    } else {
        readFileFromFS(fnLastYear, loadBuf, 8);
    }

    if( (loadBuf[0] == (loadBuf[2] ^ 0xff)) &&
        (loadBuf[1] == (loadBuf[3] ^ 0xff)) ) {

        if( (loadBuf[4] == (loadBuf[6] ^ 0x55)) &&
            (loadBuf[5] == (loadBuf[7] ^ 0x55)) ) {
            yoffs = (int16_t)((loadBuf[5] << 8) | loadBuf[4]);
        }

        _lastWrittenLY = (int16_t)((loadBuf[1] << 8) | loadBuf[0]);
        _lastWrittenYO = yoffs;
        _lastWrittenRead = true;
        
        return _lastWrittenLY;

    }

    return -1;
}

/*
 * Save given Year as "lastYear" and current _yearOffset for 
 * recovering previous time state at boot.
 * Returns true if FS was accessed during operation
 */
bool clockDisplay::saveClockStateData(uint16_t curYear)
{
    uint8_t savBuf[8];

    if(!_rtc)
        return false;

    // Check if values identical to last ones written
    if((_lastWrittenLY == curYear) && (_lastWrittenYO == _yearoffset))
        return false;

    // If not, read to check if identical to stored
    if(!_lastWrittenRead) {
        _lastWrittenLY = loadClockStateData(_lastWrittenYO);
        _lastWrittenRead = true;
        if((_lastWrittenLY == curYear) && (_lastWrittenYO == _yearoffset))
            return true;
    }
    
    savBuf[0] = curYear & 0xff;
    savBuf[1] = (curYear >> 8) & 0xff;    
    savBuf[2] = savBuf[0] ^ 0xff;
    savBuf[3] = savBuf[1] ^ 0xff;
    savBuf[4] = ((uint16_t)_yearoffset) & 0xff;
    savBuf[5] = (((uint16_t)_yearoffset) >> 8) & 0xff;
    savBuf[6] = savBuf[4] ^ 0x55;
    savBuf[7] = savBuf[5] ^ 0x55;

    _lastWrittenLY = curYear;
    _lastWrittenYO = _yearoffset;

    if(FlashROMode) {
        writeFileToSD(fnLastYearSD, savBuf, 8);
    } else {
        writeFileToFS(fnLastYear, savBuf, 8);
    }

    return true;
}

/*
 * Load display specific data from NVM storage
 *
 */
bool clockDisplay::load()
{
    uint8_t loadBuf[10];
    bool flashAccess;

    if(!_rtc) {

        if(loadNVMData(loadBuf, flashAccess)) {

            setYear((loadBuf[1] << 8) | loadBuf[0]);
            setMonth(loadBuf[4]);
            setDay(loadBuf[5]);
            setHour(loadBuf[6]);
            setMinute(loadBuf[7]);

            return true;

        }

    } else {

        if(loadNVMData(loadBuf, flashAccess)) {

              timeDifference = ((uint64_t)loadBuf[3] << 32) |  // Dumb casts for
                               ((uint64_t)loadBuf[4] << 24) |  // silencing compiler
                               ((uint64_t)loadBuf[5] << 16) |
                               ((uint64_t)loadBuf[6] <<  8) |
                                (uint64_t)loadBuf[7];

              timeDiffUp = loadBuf[8] ? true : false;

        } else {

              timeDifference = 0;

        }

        return true;
    }

    // Do NOT clear NVM if data is invalid.
    // All 0s are as bad, wait for NVM to be
    // written by application on purpose

    return false;
}

/*
 * Delayed save
 * Used in time_loop to queue save operations in order
 * to avoid long blocks due to FS operations
 */
void clockDisplay::savePending()
{
    _savePending = 1;
}

// Returns true if FS was accessed
bool clockDisplay::saveFlush()
{    
    return (_savePending == 1) ? save() : false;
}

/*
 * Save display specific data to NVM storage (Flash/SD)
 * Returns true if FS was accessed during operation
 */
bool clockDisplay::save(bool force)
{
    uint8_t savBuf[10]; // Sic! Sum written to last byte by saveNVMData

    _savePending = 0;
    memset(savBuf, 0, 10);

    if(!_rtc) {

        // Non-RTC: Save time

        savBuf[0] = _year & 0xff;
        savBuf[1] = (_year >> 8) & 0xff;
        savBuf[4] = _month;
        savBuf[5] = _day;
        savBuf[6] = _hour;
        savBuf[7] = _minute;

    } else {

        // RTC: Save timeDiff

        savBuf[3] = (timeDifference >> 32) & 0xff;
        savBuf[4] = (timeDifference >> 24) & 0xff;
        savBuf[5] = (timeDifference >> 16) & 0xff;
        savBuf[6] = (timeDifference >>  8) & 0xff;
        savBuf[7] =  timeDifference        & 0xff;

        savBuf[8] = timeDiffUp ? 1 : 0;

    }

    return saveNVMData(savBuf, force);
}

// Private functions ###########################################################

// Returns true if FS was accessed
// Returns false if not (data identical to cached)
bool clockDisplay::saveNVMData(uint8_t *savBuf, bool noReadChk)
{
    uint16_t sum = 0;
    uint8_t loadBuf[10];
    bool skipSave = false;
    bool flashAccess = false;

    // Read to check if values identical to currently stored (reduce wear)        
    if(!noReadChk && loadNVMData(loadBuf, flashAccess)) {
        skipSave = true;
        for(int i = 0; i < 9; i++) {
            if(savBuf[i] != loadBuf[i]) {
                skipSave = false;
                break;
            }
        }
    }

    if(skipSave)
        return flashAccess;

    #ifdef TC_DBG
    unsigned long now = millis();
    #endif

    for(int i = 0; i < 9; i++) {
        sum += (savBuf[i] ^ 0x55);
    }
    savBuf[9] = sum & 0xff;

    if(_configOnSD) {
        writeFileToSD(fnSD[_did], savBuf, 10);
    } else {
        writeFileToFS(fnEEPROM[_did], savBuf, 10);
    }

    // Copy to cache regardless of result of write; otherwise
    // the load function would read corrupt data instead of
    // using the ok data in the cache.
    memcpy(_CacheData, savBuf, 10);
    _Cache = 2;

    #ifdef TC_DBG
    Serial.printf("saveNVMdata took %ld\n", millis()-now);
    #endif

    return true;
}

// Returns true if loaded data is valid, otherwise false
bool clockDisplay::loadNVMData(uint8_t *loadBuf, bool& flashAccess)
{
    uint16_t sum = 0;
    bool dataOk = false;
    #ifdef TC_DBG
    unsigned long now = millis();
    #endif

    flashAccess = false;
    memset(loadBuf, 0, 10);

    if(_Cache == 2) {
        memcpy(loadBuf, _CacheData, 10);
        for(int i = 0; i < 9; i++) {
           sum += (loadBuf[i] ^ 0x55);
        }
        dataOk = ((sum & 0xff) == loadBuf[9]);
    }

    if(!dataOk) {

        _Cache = -1;  // Invalidate; either empty or corrupt
        flashAccess = true;
            
        if(_configOnSD) {
            dataOk = readFileFromSD(fnSD[_did], loadBuf, 10);
        } 
        if(!dataOk) {
            readFileFromFS(fnEEPROM[_did], loadBuf, 10);
        }

        for(int i = 0; i < 9; i++) {
           sum += (loadBuf[i] ^ 0x55);
        }
        dataOk = ((sum & 0xff) == loadBuf[9]);
        if(dataOk) {
            memcpy(_CacheData, loadBuf, 10);
            _Cache = 2;
        }
    }

    #ifdef TC_DBG
    Serial.printf("loadNVMdata took %ld\n", millis()-now);
    #endif

    return dataOk;
}

/*
 * Segment helpers
 */

// Returns bit pattern for provided value 0-9 or number provided as a char '0'-'9'
// for display on 7 segment display
uint8_t clockDisplay::getLED7NumChar(uint8_t value)
{
    if(value >= '0' && value <= '9') {
        return numDigs[value - 32];
    } else if(value <= 9) {
        return numDigs[value + '0' - 32];
    }
    return 0;
}
  
// Returns bit pattern for provided character for display on 7 segment display
uint8_t clockDisplay::getLED7AlphaChar(uint8_t value)
{
    if(value < 32 || value >= 127 + 2)
        return 0;
    
    if(value == '6' && _corr6)
        return numDigs[value - 32] | 0x01;
    
    return numDigs[value - 32];
}

// Returns bit pattern for provided character for display on 14 segment display
#ifndef IS_ACAR_DISPLAY
uint16_t clockDisplay::getLEDAlphaChar(uint8_t value)
{
    if(value < 32 || value >= 127)
        return 0;

    // For text, use common "6" pattern to conform with 7-seg-part
    if(value == '6' && _corr6)
        return alphaChars['6' - 32] | 0x0001;

    return alphaChars[value - 32];
}
#endif

// Make a 2 digit number from the array and return the segment data
// (makes leading 0s)
uint16_t clockDisplay::makeNum(uint8_t num, uint16_t dflags)
{
    uint16_t segments = 0;

    // Each position holds two digits
    // MSB = 1s, LSB = 10s

    segments = getLED7NumChar(num % 10) << 8;     
    if(!(dflags & CDD_NOLEAD0) || (num / 10)) {
        segments |= getLED7NumChar(num / 10);   
    }

    return segments;
}

// Directly write to a column with supplied segments
// (leave buffer intact, directly write to display)
void clockDisplay::directCol(int col, int segments)
{
    if((col == CD_YEAR_POS + 1) && _yearDot) {
        segments |= 0x8000;
    } else if((col == CD_YEAR_POS) && _withColon) {
        segments |= 0x8080;
    }
    Wire.beginTransmission(_address);
    Wire.write(col * 2);
    Wire.write(segments & 0xff);
    Wire.write(segments >> 8);
    Wire.endTransmission();
}

// Directly clear the display
void clockDisplay::clearDisplay()
{
    Wire.beginTransmission(_address);
    Wire.write(0x00);

    for(int i = 0; i < CD_BUF_SIZE*2; i++) {
        Wire.write(0x00);
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
void clockDisplay::showInt(bool animate, bool Alt)
{
    int i = 0;
    uint16_t *db = Alt ? _displayBufferAlt : _displayBuffer;

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
    Wire.write(0x00);

    if(animate) {
        for(i = 0; i < CD_DAY_POS; i++) {
            Wire.write(0x00);  // blank month
            Wire.write(0x00);
        }
    }

    for(; i < CD_BUF_SIZE; i++) {
        Wire.write(db[i] & 0xff);
        Wire.write(db[i] >> 8);
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
#ifndef REV_AMPM
    _displayBuffer[CD_AMPM_POS] |= 0x0080;
    _displayBuffer[CD_AMPM_POS] &= 0x7FFF;
#else
    _displayBuffer[CD_AMPM_POS] |= 0x8000;
    _displayBuffer[CD_AMPM_POS] &= 0xFF7F;
#endif
}

void clockDisplay::PM()
{
#ifndef REV_AMPM  
    _displayBuffer[CD_AMPM_POS] |= 0x8000;
    _displayBuffer[CD_AMPM_POS] &= 0xFF7F;
#else
    _displayBuffer[CD_AMPM_POS] |= 0x0080;
    _displayBuffer[CD_AMPM_POS] &= 0x7FFF;
#endif    
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
#ifndef REV_AMPM  
    directAMPM(0x80, 0x00);
#else
    directAMPM(0x00, 0x80);
#endif    
}

void clockDisplay::directPM()
{
#ifndef REV_AMPM  
    directAMPM(0x00, 0x80);
#else
    directAMPM(0x80, 0x00);
#endif
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
