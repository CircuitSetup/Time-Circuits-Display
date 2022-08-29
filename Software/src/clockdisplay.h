/*
 * -------------------------------------------------------------------
 * CircuitSetup.us Time Circuits Display
 * (C) 2021-2022 John deGlavina https://circuitsetup.us 
 * (C) 2022 Thomas Winischhofer (A10001986)
 * 
 * Clockdisplay and keypad menu code based on code by John Monaco
 * Marmoset Electronics 
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef _CLOCKDISPLAY_H
#define _CLOCKDISPLAY_H

#include <Arduino.h>
#include <RTClib.h>
#include <Wire.h>
#include <EEPROM.h>

#include "tc_global.h"
#include "tc_settings.h"

#define DS3231_I2CADDR 0x68

#ifdef IS_ACAR_DISPLAY      // A-Car (2-digit-month) ---------------------
#define CD_MONTH_POS  0     //      possibly needs to be adapted
#define CD_DAY_POS    3
#define CD_YEAR_POS   4
#define CD_HOUR_POS   6
#define CD_MIN_POS    7
  
#define CD_AMPM_POS   CD_DAY_POS
#define CD_COLON_POS  CD_YEAR_POS

#define CD_MONTH_SIZE 1     //      number of words
#define CD_MONTH_DIGS 2     //      number of digits/letters

#define CD_BUF_SIZE   8     //      in words (16bit)
#else                       // All others (3-char month) -----------------
#define CD_MONTH_POS  0
#define CD_DAY_POS    3
#define CD_YEAR_POS   4
#define CD_HOUR_POS   6
#define CD_MIN_POS    7

#define CD_AMPM_POS   CD_DAY_POS
#define CD_COLON_POS  CD_YEAR_POS

#define CD_MONTH_SIZE 3     //      number of words
#define CD_MONTH_DIGS 3     //      number of digits/letters

#define CD_BUF_SIZE   8     //      in words (16bit)
#endif                      // -------------------------------------------

extern bool alarmOnOff;

extern uint64_t timeDifference;
extern bool     timeDiffUp;

extern uint64_t dateToMins(int year, int month, int day, int hour, int minute);
extern void     minsToDate(uint64_t total, int& year, int& month, int& day, int& hour, int& minute);

extern int      daysInMonth(int month, int year);
 
struct dateStruct {
    uint16_t year;
    uint8_t month;
    uint8_t day;
    uint8_t hour;
    uint8_t minute;
};

class clockDisplay {
  
    public:
    
        clockDisplay(uint8_t address, int saveAddress);
        void begin();    
        void on();
        void off();
        void lampTest();
        
        void clear();
        
        uint8_t setBrightness(uint8_t level);
        uint8_t setBrightnessDirect(uint8_t level) ;
        uint8_t getBrightness();
    
        void set1224(bool hours24);
        bool get1224();
    
        void setNightMode(bool mode);
        bool getNightMode();
    
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
        
        void setColon(bool col);
    
        uint8_t getMonth();
        uint8_t getDay();
        int16_t getYearOffset();
        uint16_t getYear();    
        uint8_t getHour();
        uint8_t getMinute();
        
        void showOnlyMonth(int monthNum);  
        void showOnlyDay(int dayNum);
        void showOnlyHour(int hourNum);
        void showOnlyMinute(int minuteNum);
        void showOnlyYear(int yearNum);
    
        void showOnlySettingVal(const char* setting, int8_t val = -1, bool clear = false);
        void showOnlyText(const char *text);
        void showOnlyHalfIP(int a, int b, bool clear = false);
    
        bool save();
        bool saveYOffs();
        bool load();
        int16_t loadYOffs();
        
        void setDS3232time(byte second, byte minute, byte hour, byte dayOfWeek, byte dayOfMonth, byte month, byte year);    

    private:
    
        uint8_t _address;
        int _saveAddress;
        uint16_t _displayBuffer[8]; // Segments to make current time.
    
        uint16_t _year = 2021;      // keep track of these
        int16_t  _yearoffset = 0;   // Offset for faking years < 2000 or > 2159
        
        uint8_t _month = 1;
        uint8_t _day = 1;
        uint8_t _hour = 0;
        uint8_t _minute = 0;
        bool _colon = false;        // should colon be on?
        bool _rtc = false;          // will this be displaying real time
        uint8_t _brightness = 15;   // display brightness
        bool _mode24 = false;       // true = 24 hour mode, false 12 hour mode
        bool _nightmode = false;    // true = dest/dept times off
        int _oldnm = -1;
    
        uint8_t  getLED7NumChar(uint8_t value);
        uint8_t  getLED7AlphaChar(uint8_t value);
        #ifndef IS_ACAR_DISPLAY
        uint16_t getLEDAlphaChar(uint8_t value);
        #endif
    
        uint16_t makeNum(uint8_t num);
        uint16_t makeNumN0(uint8_t num);
    
        void directCol(int col, int segments);  // directly writes column RAM
    
        void clearDisplay();                    // clears display RAM
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
        
        byte decToBcd(byte val);
};

#endif
