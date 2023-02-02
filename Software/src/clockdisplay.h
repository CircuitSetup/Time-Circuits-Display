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

class clockDisplay {

    public:

        clockDisplay(uint8_t did, uint8_t address, int saveAddress);
        void begin();
        void on();
        void onCond();
        void off();
        void realLampTest();
        void lampTest();

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
        void showAnimate2();

        void setDateTime(DateTime dt);      // Set object date & time using a DateTime ignoring timeDiff
        void setDateTimeDiff(DateTime dt);  // Set object date & time using a DateTime plus/minus timeDiff
        void setFromStruct(dateStruct* s);  // Set object date & time from struct; never use this with RTC

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

        void showOnlyMonth(int monthNum);
        void showOnlyDay(int dayNum);
        void showOnlyHour(int hourNum, bool force24 = false);
        void showOnlyMinute(int minuteNum);
        void showOnlyYear(int yearNum);

        void showSettingValDirect(const char* setting, int8_t val = -1, bool clear = false);
        void showTextDirect(const char *text, bool clear = true, bool corr6 = false);
        void showHalfIPDirect(int a, int b, bool clear = false);

        #ifdef TC_HAVETEMP
        void showTempDirect(float temp, bool tempUnit, bool animate = false);
        void showHumDirect(int hum, bool animate = false);
        #endif

        bool    save();
        bool    saveYOffs();
        bool    load(int initialBrightness = -1);
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

        uint16_t makeNum(uint8_t num);
        #if 0
        uint16_t makeNumN0(uint8_t num);
        #endif

        void directCol(int col, int segments);  // directly writes column RAM

        void clearDisplay();                    // clears display RAM
        bool handleNM();
        void showInt(bool animate = false);

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
        int      _saveAddress = -1;
        uint16_t _displayBuffer[8];     // Segments to make current time

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
};

#endif
