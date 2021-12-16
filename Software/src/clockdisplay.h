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

#ifndef CLOCKDISPLAY_h
#define CLOCKDISPLAY_h

#include <Arduino.h>
#include <RTClib.h>
#include <Wire.h>
#include <EEPROM.h>
#include "tc_settings.h"

#define DS3231_I2CADDR 0x68

static const uint16_t alphaChars[] = {
    0b0000000000000001,
    0b0000000000000010,
    0b0000000000000100,
    0b0000000000001000,
    0b0000000000010000,
    0b0000000000100000,
    0b0000000001000000,
    0b0000000010000000,
    0b0000000100000000,
    0b0000001000000000,
    0b0000010000000000,
    0b0000100000000000,
    0b0001000000000000,
    0b0010000000000000,
    0b0100000000000000,
    0b1000000000000000,
    0b0000000000000000,
    0b0000000000000000,
    0b0000000000000000,
    0b0000000000000000,
    0b0000000000000000,
    0b0000000000000000,
    0b0000000000000000,
    0b0000000000000000,
    0b0001001011001001,
    0b0001010111000000,
    0b0001001011111001,
    0b0000000011100011,
    0b0000010100110000,
    0b0001001011001000,
    0b0011101000000000,
    0b0001011100000000,
    0b0000000000000000,  //
    0b0000000000000110,  // !
    0b0000001000100000,  // "
    0b0001001011001110,  // #
    0b0001001011101101,  // $
    0b0000110000100100,  // %
    0b0010001101011101,  // &
    0b0000010000000000,  // '
    0b0010010000000000,  // (
    0b0000100100000000,  // )
    0b0011111111000000,  // *
    0b0001001011000000,  // +
    0b0000100000000000,  // ,
    0b0000000011000000,  // -
    0b0000000000000000,  // .
    0b0000110000000000,  // /
    0b0000110000111111,  // 0
    0b0000000000000110,  // 1
    0b0000000011011011,  // 2
    0b0000000010001111,  // 3
    0b0000000011100110,  // 4
    0b0010000001101001,  // 5
    0b0000000011111101,  // 6
    0b0000000000000111,  // 7
    0b0000000011111111,  // 8
    0b0000000011101111,  // 9
    0b0001001000000000,  // :
    0b0000101000000000,  // ;
    0b0010010000000000,  // <
    0b0000000011001000,  // =
    0b0000100100000000,  // >
    0b0001000010000011,  // ?
    0b0000001010111011,  // @
    0b0000000011110111,  // A
    0b0001001010001111,  // B
    0b0000000000111001,  // C
    0b0001001000001111,  // D
    0b0000000011111001,  // E
    0b0000000001110001,  // F
    0b0000000010111101,  // G
    0b0000000011110110,  // H
    0b0001001000001001,  // I
    0b0000000000011110,  // J
    0b0010010001110000,  // K
    0b0000000000111000,  // L
    0b0000001000110111,  //0b0000010100110110,  // M
    0b0010000100110110,  // N
    0b0000000000111111,  // O
    0b0000000011110011,  // P
    0b0010000000111111,  // Q
    0b0010000011110011,  // R
    0b0000000011101101,  // S
    0b0001001000000001,  // T
    0b0000000000111110,  // U
    0b0000110000110000,  // V
    0b0010100000110110,  // W
    0b0010110100000000,  // X
    0b0000000011101110,  // Y
    0b0000110000001001,  // Z
    0b0000000000111001,  // [
    0b0010000100000000,  //
    0b0000000000001111,  // ]
    0b0000110000000011,  // ^
    0b0000000000001000,  // _
    0b0000000100000000,  // `
    0b0001000001011000,  // a
    0b0010000001111000,  // b
    0b0000000011011000,  // c
    0b0000100010001110,  // d
    0b0000100001011000,  // e
    0b0000000001110001,  // f
    0b0000010010001110,  // g
    0b0001000001110000,  // h
    0b0001000000000000,  // i
    0b0000000000001110,  // j
    0b0011011000000000,  // k
    0b0000000000110000,  // l
    0b0001000011010100,  // m
    0b0001000001010000,  // n
    0b0000000011011100,  // o
    0b0000000101110000,  // p
    0b0000010010000110,  // q
    0b0000000001010000,  // r
    0b0010000010001000,  // s
    0b0000000001111000,  // t
    0b0000000000011100,  // u
    0b0010000000000100,  // v
    0b0010100000010100,  // w
    0b0010100011000000,  // x
    0b0010000000001100,  // y
    0b0000100001001000,  // z
    0b0000100101001001,  // {
    0b0001001000000000,  // |
    0b0010010010001001,  // }
    0b0000010100100000,  // ~
    0b0011111111111111,
};

static const uint8_t numDigs[36] = {
    0b00111111,  // 0
    0b00000110,  // 1
    0b01011011,  // 2
    0b01001111,  // 3
    0b01100110,  // 4
    0b01101101,  // 5
    0b01111100,  // 6 - edited to take out top serif
    0b00000111,  // 7
    0b01111111,  // 8
    0b01100111,  // 9 - edited to take out bottom serif
    0b01110111, // A 
	0b01111100, // B 
	0b00111001, // C 
	0b01011110, // D 
	0b01111001, // E 
	0b01110001, // F 
	0b00111101, // G 
	0b01110110, // H 
	0b00110000, // I 
	0b00011110, // J 
	0b01110101, // K 
	0b00111000, // L 
	0b00010101, // M 
	0b00110111, // N 
	0b00111111, // O 
	0b01110011, // P 
	0b01101011, // Q 
	0b00110011, // R 
	0b01101101, // S 
	0b01111000, // T 
	0b00111110, // U 
	0b00111110, // V 
	0b00101010, // W 
	0b01110110, // X 
	0b01101110, // Y 
	0b01011011, // Z 
};

const char months[12][4] = {"JAN", "FEB", "MAR", "APR", "MAY", "JUN", "JUL", "AUG", "SEP", "OCT", "NOV", "DEC"};

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
    void lampTest();
    void on();
    void off();
    void clear();
    uint8_t setBrightness(uint8_t level);
    uint8_t getBrightness();

    void setMonth(int monthNum);
    void setDay(int dayNum);
    void setYear(uint16_t yearNum);
    void setYearDirect(uint16_t yearNum);
    void setHour(uint16_t hourNum);
    void setMinute(int minNum);
    void setColon(bool col);
    void colonOn();
    void colonOff();

    uint8_t getMonth();
    uint8_t getDay();
    uint16_t getYear();
    uint8_t getHour();
    uint8_t getMinute();

    void setRTC(bool rtc);  // make this an RTC display
    bool isRTC();

    void AM();
    void PM();

    void showOnlyMonth(int monthNum);  // Show only the supplied month, do not modify object's month
    void showOnlyDay(int dayNum);
    void showOnlyHour(int hourNum);
    void showOnlyMinute(int minuteNum);
    void showOnlyYear(int yearNum);
    void showOnlySave();
    void showOnlySettingVal(const char* setting, int8_t val = -1, bool clear = false);

    void setDateTime(DateTime dt);  // Set object date & time using a DateTime
    void setFromStruct(dateStruct* s);
    void setDS3232time(byte second, byte minute, byte hour, byte dayOfWeek, byte dayOfMonth, byte month, byte year);

    DateTime getDateTime();  // Return object's date & time as a DateTime

    void show();
    void showAnimate1();
    void showAnimate2();

    bool save();
    bool load();
    bool resetClocks();
    bool isPrefData(const char* key);

   private:
    uint8_t _address;
    int _saveAddress;
    uint16_t _displayBuffer[8];  // Segments to make current time.

    uint16_t _year = 2021;  // keep track of these
    uint8_t _month = 1;
    uint8_t _day = 1;
    uint8_t _hour = 0;
    uint8_t _minute = 0;
    bool _colon = false;  // should colon be on?
    bool _rtc = false;    // will this be displaying real time
    uint8_t _brightness = 15;

    uint8_t getLED7SegChar(uint8_t value);
    uint16_t getLEDAlphaChar(char value);

    uint16_t makeNum(uint8_t num);
    uint16_t makeAlpha(uint8_t value);

    void clearDisplay();                    // clears display RAM
    void directCol(int col, int segments);  // directly writes column RAM
    void directAM();
    void directPM();

    byte decToBcd(byte val);
};

#endif
