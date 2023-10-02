/*
 * -------------------------------------------------------------------
 * CircuitSetup.us Time Circuits Display
 * (C) 2021-2022 John deGlavina https://circuitsetup.us
 * (C) 2022-2023 Thomas Winischhofer (A10001986)
 * https://github.com/realA10001986/Time-Circuits-Display
 * http://tcd.backtothefutu.re
 *
 * Clockdisplay Class: Handles the TC LED segment displays
 *
 * Based on code by John Monaco, Marmoset Electronics
 * https://www.marmosetelectronics.com/time-circuits-clock
 * -------------------------------------------------------------------
 * License: MIT
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
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, 
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF 
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. 
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY 
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, 
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE 
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef _CLOCKDISPLAY_H
#define _CLOCKDISPLAY_H

#include "rtc.h"

struct dateStruct {
    uint16_t year;
    uint8_t month;
    uint8_t day;
    uint8_t hour;
    uint8_t minute;
};

#define CD_BUF_SIZE   8  // Buffer size in words (16bit)

// Flags for textDirect() etc (flags)
#define CDT_CLEAR 0x0001
#define CDT_CORR6 0x0002
#define CDT_COLON 0x0004
#define CDT_BLINK 0x0008

// Flags for xxxDirect (dflags)
#define CDD_FORCE24 0x0001
#define CDD_NOLEAD0 0x0002

class clockDisplay {

    public:

        clockDisplay(uint8_t did, uint8_t address);
        void begin();
        void on();
        void onCond();
        void off();
        void onBlink(uint8_t blink);
        #if 0
        void realLampTest();
        #endif
        void lampTest(bool randomize = false);

        void clearBuf();

        uint8_t setBrightness(uint8_t level, bool setInitial = false);
        void    resetBrightness();
        uint8_t setBrightnessDirect(uint8_t level);
        uint8_t getBrightness();

        void set1224(bool hours24);
        bool get1224();

        void setNightMode(bool mode);
        bool getNightMode();
        void setNMOff(bool NMOff);

        void setRTC(bool rtc);  // make this an RTC display
        bool isRTC();

        void show();
        void showAnimate1();
        #ifdef BTTF3_ANIM
        void showAnimate2(int until = CD_BUF_SIZE);
        void showAnimate3(int mystep);
        #else
        void showAnimate2();
        #endif

        void showAlt();
        void setAltText(const char *text);

        void setDateTime(DateTime& dt);          // Set object date & time using a DateTime ignoring timeDiff
        void setDateTimeDiff(DateTime& dt);      // Set object date & time using a DateTime plus/minus timeDiff
        void setFromStruct(const dateStruct *s); // Set object date & time from struct; never use this with RTC
        void setFromParms(int year, int month, int day, int hour, int minute);

        void getToParms(int& year, int& yo, int& month, int& day, int& hour, int& minute);

        void setMonth(int monthNum);
        void setDay(int dayNum);
        void setYearOffset(int16_t yearOffs);
        void setYear(uint16_t yearNum);
        void setHour(uint16_t hourNum);
        void setMinute(int minNum);
        void setDST(int8_t isDST);

        void setColon(bool col);

        uint8_t  getMonth();
        uint8_t  getDay();
        int16_t  getYearOffset();
        uint16_t getYear();
        uint16_t getDisplayYear();
        uint8_t  getHour();
        uint8_t  getMinute();
        int8_t   getDST();

        const char* getMonthString(uint8_t month);

        void showMonthDirect(int monthNum, uint16_t dflags = 0);
        void showDayDirect(int dayNum, uint16_t dflags = 0);
        void showHourDirect(int hourNum, uint16_t dflags = 0);
        void showMinuteDirect(int minuteNum, uint16_t dflags = 0);
        void showYearDirect(int yearNum, uint16_t dflags = 0);

        void showTextDirect(const char *text, uint16_t flags = CDT_CLEAR);
        void showHalfIPDirect(int a, int b, uint16_t flags = 0);
        void showSettingValDirect(const char* setting, int8_t val = -1, uint16_t flags = 0);

        #ifdef TC_HAVETEMP
        void showTempDirect(float temp, bool tempUnit, bool animate = false);
        void showHumDirect(int hum, bool animate = false);
        #endif

        bool    save();
        bool    saveYOffs();
        bool    load();
        int16_t loadYOffs();
        int8_t  loadDST();

        bool    saveLastYear(uint16_t theYear);
        int16_t loadLastYear();

    private:

        bool     saveNVMData(uint8_t *savBuf, bool noReadChk = false);
        bool     loadNVMData(uint8_t *loadBuf);

        uint8_t  getLED7NumChar(uint8_t value);
        uint8_t  getLED7AlphaChar(uint8_t value);
        #ifndef IS_ACAR_DISPLAY
        uint16_t getLEDAlphaChar(uint8_t value);
        #endif

        uint16_t makeNum(uint8_t num, uint16_t dflags = 0);

        void directCol(int col, int segments);

        void clearDisplay();
        bool handleNM();
        void showInt(bool animate = false, bool Alt = false);

        void colonOn();
        void colonOff();

        void AM();
        void PM();
        void AMPMoff();
        void directAMPM(int val1, int val2);
        void directAM();
        void directPM();
        void directAMPMoff();

        void directCmd(uint8_t val);

        uint8_t  _did = 0;
        uint8_t  _address = 0;
        uint16_t _displayBuffer[CD_BUF_SIZE];
        uint16_t _displayBufferAlt[CD_BUF_SIZE];

        uint16_t _year = 2021;          // keep track of these
        int16_t  _yearoffset = 0;       // Offset for faking years < 2000, > 2098

        int16_t  _lastWrittenLY = -333; // Not to be confused with possible results from loadLastYear

        int8_t  _isDST = 0;             // DST active? -1:dunno 0:no 1:yes

        uint8_t _month = 1;
        uint8_t _day = 1;
        uint8_t _hour = 0;
        uint8_t _minute = 0;
        bool _colon = false;          // should colon be on?
        bool _rtc = false;            // will this be displaying real time
        uint8_t _brightness = 15;     // current display brightness
        uint8_t _origBrightness = 15; // value from settings
        bool _mode24 = false;         // true = 24 hour mode, false = 12 hour mode
        bool _nightmode = false;      // true = dest/dept times off
        bool _NmOff = false;          // true = off during night mode, false = dimmed
        int _oldnm = -1;
        bool _corr6 = false;
        bool _yearDot = false;
        bool _withColon = false;

        int8_t _Cache = -1;
        char   _CacheData[10];
};

#endif
