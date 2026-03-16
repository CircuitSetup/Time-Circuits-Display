/*
 * -------------------------------------------------------------------
 * CircuitSetup.us Time Circuits Display
 * (C) 2021-2022 John deGlavina https://circuitsetup.us
 * (C) 2022-2026 Thomas Winischhofer (A10001986)
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
extern int      gpsGetDM();
#endif

extern uint64_t timeDifference;
extern bool     timeDiffUp;

extern int      daysInMonth(int month, int year);

extern uint16_t    loadClockState(int16_t& yoffs);
extern bool        saveClockState(uint16_t curYear, int16_t yearoffset);
extern void        getClockDataP(uint64_t& timeDifference, bool &timeDiffUp);
extern dateStruct *getClockDataDL(uint8_t did);
extern bool        saveClockDataP(bool force);
extern bool        saveClockDataDL(bool force, uint8_t did, uint16_t year, uint8_t month, uint8_t day, uint8_t hour, uint8_t minute);

#ifndef IS_ACAR_DISPLAY
static const char months[13][4] = {
    "JAN", "FEB", "MAR", "APR", "MAY", "JUN",
    "JUL", "AUG", "SEP", "OCT", "NOV", "DEC",
    "  _"
};

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
    _rtc = (_did == DISP_PRES);
    
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

void clockDisplay::onBlink(uint8_t blink)
{
    directCmd(0x80 | 1 | ((blink & 0x03) << 1)); 
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
#if 0
void clockDisplay::lampTest()
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
void clockDisplay::showPattern(bool randomize)
{
    uint16_t db[CD_BUF_SIZE];
    uint32_t rnd = esp_random();

    for(int i = 0; i < CD_BUF_SIZE; i++) {
        db[i] =  (randomize ? ((rand() % 0x7f) ^ rnd) & 0x7f : 0xaa) << 8;
        db[i] |= (randomize ? (((rand() % 0x7f) ^ (rnd >> 8))) & 0x77 : 0x55);
    }

    directBuf(db);
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
    level &= 0x0f;

    directCmd(0xe0 | level);

    return level;
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

void clockDisplay::getToStruct(dateStruct *s)
{
    s->year = getYear();
    s->month = getMonth();
    s->day = getDay();
    s->hour = getHour();
    s->minute = getMinute();
}


// Show data in display --------------------------------------------------------


// Show the buffer
void clockDisplay::show()
{
    showInt(false);
}

// Show all but month
#ifndef TC_NO_MONTH_ANIM
void clockDisplay::showAnimate(bool firstStage)
{
    if(firstStage) showInt(true);
    else showAnimate2();
}
#endif

#ifndef IS_ACAR_DISPLAY
bool clockDisplay::showAnimate3(int mystep)
{
    uint16_t buf;
    uint16_t *bu;
    
    if(!mystep) {
        if(!handleNM())
            return false;
        off();
        _displayBuffer[CD_AMPM_POS] &= 0x7F7F;  // AM/PM off
        _displayBuffer[CD_COLON_POS] &= 0x7F7F; // Colon off
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
        // fall through
    default:
        showAnimate2(lim);
    }

    return true;
}
#endif // IS_ACAR_DISPLAY

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

    _WCtimeFits = (pos <= CD_HOUR_POS);

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

void clockDisplay::setYearOffset(int16_t yearOffs)
{
    if(_did == DISP_PRES) {
        _yearoffset = yearOffs;
        #ifdef TC_DBG_TIME
        Serial.printf("ClockDisplay: _yearoffset set to %d\n", yearOffs);
        #endif
    }
}


// Query data ------------------------------------------------------------------


#ifndef IS_ACAR_DISPLAY
const char * clockDisplay::getMonthString(uint8_t mon)
{
    if(mon >= 1 && mon <= 12)
        return months[mon-1];
    else
        return nullStr;
}
#endif

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
    uint16_t db[CD_BUF_SIZE] = { 0 };

    if(monthNum > 12)
        monthNum = 12;

#ifdef IS_ACAR_DISPLAY
    db[CD_MONTH_POS] = makeNum(monthNum, dflags);
#else
    if(monthNum > 0) {
        monthNum--;
    } else {
        monthNum = 12;
    }
    db[CD_MONTH_POS]     = getLEDAlphaChar(months[monthNum][0]);
    db[CD_MONTH_POS + 1] = getLEDAlphaChar(months[monthNum][1]);
    db[CD_MONTH_POS + 2] = getLEDAlphaChar(months[monthNum][2]);
#endif
    directBuf(db);
}

void clockDisplay::showDayDirect(int dayNum, uint16_t dflags)
{
    uint16_t db[CD_BUF_SIZE] = { 0 };

    db[CD_DAY_POS] = makeNum(dayNum, dflags);
    directBuf(db);
}

void clockDisplay::showYearDirect(int yearNum, uint16_t dflags)
{
    uint16_t seg = 0;
    int y100;
    uint16_t db[CD_BUF_SIZE] = { 0 };

    while(yearNum >= 10000)
        yearNum -= 10000;

    y100 = yearNum / 100;

    if(!(dflags & CDD_NOLEAD0) || y100) {
        seg = makeNum(y100, dflags);
        if(y100) dflags &= ~CDD_NOLEAD0;
    }
    db[CD_YEAR_POS] = seg;
    db[CD_YEAR_POS + 1] = makeNum(yearNum % 100, dflags);
    directBuf(db);
}

void clockDisplay::showHourDirect(int hourNum, uint16_t dflags)
{
    uint16_t db[CD_BUF_SIZE] = { 0 };

    // This assumes that CD_HOUR_POS is different to CD_AMPM_POS

    if(!_mode24 && !(dflags & CDD_FORCE24)) {

        db[CD_AMPM_POS] = (hourNum > 11) ? _pm : _am;

        if(hourNum == 0) {
            hourNum = 12;
        } else if(hourNum > 12) {
            hourNum -= 12;
        }

    } else {

        db[CD_AMPM_POS] = 0;

    }

    db[CD_HOUR_POS] = makeNum(hourNum, dflags);
    directBuf(db);
}

void clockDisplay::showMinuteDirect(int minuteNum, uint16_t dflags)
{
    uint16_t db[CD_BUF_SIZE] = { 0 };

    db[CD_MIN_POS] = makeNum(minuteNum, dflags);
    directBuf(db);    
}


// Special purpose -------------------------------------------------------------


// Show the given text
void clockDisplay::showTextDirect(const char *text, uint16_t flags)
{
    int idx = 0, pos = CD_MONTH_POS;
    int temp = 0;
    uint16_t db[CD_BUF_SIZE];

    _corr6 = !!(flags & CDT_CORR6);

#ifdef IS_ACAR_DISPLAY
    while(text[idx] && pos < (CD_MONTH_POS+CD_MONTH_SIZE)) {
        temp = getLED7AlphaChar(text[idx++]);
        if(text[idx]) {
            temp |= (getLED7AlphaChar(text[idx++]) << 8);
        }
        db[pos++] = temp;
    }
#else
    while(text[idx] && pos < (CD_MONTH_POS+CD_MONTH_SIZE)) {
        db[pos++] = getLEDAlphaChar(text[idx++]);
    }
#endif

    while(pos < CD_DAY_POS) {
        db[pos++] = 0;
    }
    
    pos = CD_DAY_POS;
    while(text[idx] && pos <= CD_MIN_POS) {
        temp = getLED7AlphaChar(text[idx++]);
        if(text[idx]) {
            temp |= (getLED7AlphaChar(text[idx++]) << 8);
        }
        db[pos++] = temp;
    }

    if(flags & CDT_CLEAR) {
        while(pos <= CD_MIN_POS) {
            db[pos++] = 0;
        }
    }

    if(flags & CDT_YRDOT)
        db[CD_YEAR_POS + 1] |= 0x8000;

    if(flags & CDT_COLON)
        db[CD_YEAR_POS] |= 0x8080;

    directBuf(db, pos);

    _corr6 = false;
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
    else if(a < 0) a = 0;
    if(b > 255) b = 255;
    else if(b < 0) b = 0;

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
    char u = tempUnit ? 'C' : 'F';

    if(!handleNM())
        return;

    if(isnan(temp)) {
        #ifdef IS_ACAR_DISPLAY
        sprintf(buf, "%s  ----~%c", ttem, u);
        #else
        sprintf(buf, "%s   ----~%c", ttem, u);
        #endif
    } else {
        t2 = abs((int)(temp * 100.0f) - ((int)temp * 100));
        #ifdef IS_ACAR_DISPLAY
        sprintf(buf, "%s%4d%02d~%c", ttem, (int)temp, t2, u);
        #else
        sprintf(buf, "%s %4d%02d~%c", ttem, (int)temp, t2, u);
        #endif
    }
    
    showTextDirect(buf, CDT_CLEAR|CDT_YRDOT);

    showIntTail(animate);
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

    showIntTail(animate);
}
#endif

#ifdef TC_HAVEGPS
void clockDisplay::showNavDirect(char *msg, bool animate)
{
    char buf[16];
    char *myMsg = msg;
    uint16_t flags;

    if(!handleNM())
        return;

    if(animate) {
        strcpy(buf, msg);
        #ifdef IS_ACAR_DISPLAY
        buf[0] = buf[1] = ' ';
        #else
        buf[0] = buf[1] = buf[2] = ' ';
        #endif
        myMsg = buf;
    }

    switch(gpsGetDM()) {
    case 1:
      flags = CDT_COLON;
      break;
    default:
      flags = CDT_YRDOT;
    }

    showTextDirect(myMsg, CDT_CLEAR|flags);

    showIntTail(animate);
}
#endif


// Load & save data ------------------------------------------------------------


/* 
 * Load time state (lastYear, yearOffset) from NVM 
 * !!! Does not *SET* values, just returns them !!!
 */
uint16_t clockDisplay::loadClockStateData(int16_t& yoffs)
{
    yoffs = 0;
    
    if(_did != DISP_PRES)
        return 0;

    return loadClockState(yoffs);
}

/*
 * Save given Year as "lastYear" and current _yearOffset for 
 * recovering previous time state at boot.
 * Returns true if FS was accessed during operation
 */
bool clockDisplay::saveClockStateData(uint16_t curYear)
{
    if(_did != DISP_PRES)
        return false;

    return saveClockState(curYear, _yearoffset);
}

/*
 * Load display specific data from NVM storage
 * Returns false if no valid data stored
 *
 */

bool clockDisplay::load()
{
    dateStruct *mydate;
    
    switch(_did) {
    case DISP_PRES:
        getClockDataP(timeDifference, timeDiffUp);
        break;
    case DISP_DEST:
    case DISP_LAST:
        if(!(mydate = getClockDataDL(_did)))
            return false;
        setFromStruct(mydate);
        break;
    }
    
    return true;
}

/*
 * Delayed save
 * Used in time_loop to queue save operations in order
 * to avoid long delays due to FS operations
 */
void clockDisplay::savePending()
{
    _savePending = 1;
}

// Returns true if FS was accessed
bool clockDisplay::saveFlush()
{
    return _savePending ? save() : false;
}

/*
 * Save display specific data to NVM storage
 * Returns true if FS was accessed during operation
 */
bool clockDisplay::save(bool force)
{
    _savePending = 0;

    if(_did == DISP_PRES) {
        return saveClockDataP(force);
    }

    return saveClockDataDL(force, _did, _year, _month, _day, _hour, _minute);
}

// Private functions ###########################################################

/*
 * Segment helpers
 */

// Returns bit pattern for provided value 0-9 or number provided as a char '0'-'9'
// for display on 7 segment display
uint8_t clockDisplay::getLED7NumChar(uint8_t value)
{
    if(value >= '0') { 
        if(value <= '9') {
            return numDigs[value - 32];
        }
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
        return numDigs['6' - 32] | 0x01;
    
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

// Make a 2 digit number from num and return the segment data
uint16_t clockDisplay::makeNum(uint8_t num, uint16_t dflags)
{
    // Each position holds two digits
    // MSB = 1s, LSB = 10s

    uint16_t segments = getLED7NumChar(num % 10) << 8;     
    if(!(dflags & CDD_NOLEAD0) || (num / 10)) {
        segments |= getLED7NumChar(num / 10);   
    }

    return segments;
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

#ifdef IS_ACAR_DISPLAY
#ifndef TC_NO_MONTH_ANIM
void clockDisplay::showAnimate2()
{
    if(_nightmode && _NmOff)
        return;

    directBuf(_displayBuffer);
}
#endif
#else // IS_ACAR_DISPLAY
void clockDisplay::showAnimate2(int until)
{
    if(_nightmode && _NmOff)
        return;

    uint16_t db[CD_BUF_SIZE];
    
    for(int i = 0; i < until; i++) {
        db[i] = _displayBuffer[i];
    }
    for(int i = until; i < CD_BUF_SIZE; i++) {
        db[i] = 0;
    }
    directBuf(db);
}
#endif

// Show the buffer
void clockDisplay::showInt(bool animate, bool Alt)
{
    int i = 0;
    uint16_t *dbuf; 
    uint16_t db[CD_BUF_SIZE];

    if(!handleNM())
        return;

    if(animate) {
        off();
        Alt = false;
        db[i++] = 0;
        db[i++] = 0;
        db[i++] = 0;
    }

    if(!_mode24) {
        (_hour < 12) ? AM() : PM();
    } else {
        _displayBuffer[CD_AMPM_POS] &= 0x7F7F;
    }

    if(_colon) _displayBuffer[CD_COLON_POS] |= 0x8080;
    else       _displayBuffer[CD_COLON_POS] &= 0x7F7F;
    
    dbuf = Alt ? _displayBufferAlt : _displayBuffer;
    
    for(; i < CD_BUF_SIZE; i++) {
        db[i] = dbuf[i];
    }

    if(Alt && _WCtimeFits) {
        for(i = CD_HOUR_POS; i < CD_BUF_SIZE; i++) {
            db[i] = _displayBuffer[i];
        }
        //db[CD_AMPM_POS] &= 0x7f7f;  // bits already clear
        //db[CD_COLON_POS] &= 0x7f7f; // bits already clear
        db[CD_AMPM_POS] |= (_displayBuffer[CD_AMPM_POS] & 0x8080);
        db[CD_COLON_POS] |= (_displayBuffer[CD_COLON_POS] & 0x8080);
    }
    
    directBuf(db);

    showIntTail(animate);
}

void clockDisplay::showIntTail(bool animate)
{
    if(animate) directCmd(0x20 | 1);
    if(animate || (_NmOff && (_oldnm > 0))) on();

    if(_NmOff) _oldnm = 0;
}
    
inline void clockDisplay::AM()
{
    _displayBuffer[CD_AMPM_POS] |= _am; 
    _displayBuffer[CD_AMPM_POS] &= (~_pm);
}

inline void clockDisplay::PM()
{
    _displayBuffer[CD_AMPM_POS] |= _pm;
    _displayBuffer[CD_AMPM_POS] &= (~_am);
}

// Directly clear the display
void clockDisplay::clearDisplay()
{
    uint16_t db[CD_BUF_SIZE] = { 0 };
    directBuf(db);
}

void clockDisplay::directCmd(uint8_t val)
{
    Wire.beginTransmission(_address);
    Wire.write(val);
    Wire.endTransmission();
}

void clockDisplay::directBuf(uint16_t *db, int len)
{
    Wire.beginTransmission(_address);
    Wire.write(0x00);
    for(int i = 0; i < len; i++) {
        Wire.write(db[i] & 0xff);
        Wire.write(db[i] >> 8);
    }
    Wire.endTransmission();
}

// Directly write to a column with supplied segments
// (leave buffer intact, directly write to display)
void clockDisplay::directCol(int col, int segments)
{
    Wire.beginTransmission(_address);
    Wire.write(col * 2);
    Wire.write(segments & 0xff);
    Wire.write(segments >> 8);
    Wire.endTransmission();
}
