/*
 * -------------------------------------------------------------------
 * CircuitSetup.us Time Circuits Display
 * (C) 2022-2024 Thomas Winischhofer (A10001986)
 * https://github.com/realA10001986/Time-Circuits-Display
 * https://tcd.out-a-ti.me
 *
 * speedDisplay Class: Speedo Display
 *
 * This is designed for CircuitSetup's Speedo display and other 
 * HT16K33-based displays, like the "Grove - 0.54" Dual/Quad 
 * Alphanumeric Display" or some displays with an Adafruit i2c
 * backpack:
 * ADA-878/877/5599;879/880/881/1002/5601/5602/5603/5604
 * ADA-1270/1271;1269
 * ADA-1911/1910;1912/2157/2158/2160
 * 
 * The i2c slave address must be 0x70.
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

#ifndef _speedDisplay_H
#define _speedDisplay_H

// The supported display types:
// The speedo is a 2-digit 7-segment display, with the bottom/right dot lit
// in the movies.
// CircuitSetup's Speedo is perfect, all other supported ones are either
// 4-digit, and/or use 14-segment tubes, and/or lack the dot.
// For 4-digit displays, there are two entries in this list, one to display
// the speed right-aligned, one for left-aligned.
// If you have the skills, you could also remove the 4-tube display from
// the pcb and connect a 2-tube one. Unfortunately, the pin assignments
// usually do not match, which makes some re-wiring necessary.
// Displays with a TM1637 are not supported because this chip is not really
// i2c compatible as it has no slave address, and therefore cannot be part
// of a i2c chain.
//
// The display's i2c slave address is 0x70 (defined in tc_time.h).
//
enum dispTypes : uint8_t {
    SP_CIRCSETUP = 0, // Original CircuitSetup.us speedo
    SP_ADAF_7x4,      // Adafruit 0.56" 4 digits, 7-segment (7x4) (ADA-878)
    SP_ADAF_7x4L,     // " " " (left-aligned)
    SP_ADAF_B7x4,     // Adafruit 1.2" 4 digits, 7-segment (7x4) (ADA-1270)
    SP_ADAF_B7x4L,    // " " " (left-aligned)
    SP_ADAF_14x4,     // Adafruit 0.56" 4 digits, 14-segment (14x4) (ADA-1911)
    SP_ADAF_14x4L,    // " " " (left-aligned)
    SP_GROVE_2DIG14,  // Grove 0.54" Dual Alphanumeric Display
    SP_GROVE_4DIG14,  // Grove 0.54" Quad Alphanumeric Display
    SP_GROVE_4DIG14L, // " " " (left aligned)
    SP_ADAF1911_L,    // Like SP_ADAF_14x4L, but with only left hand side tube soldered on
    SP_ADAF878L,      // Like SP_ADAF_7x4L, but only left 2 digits soldered on
// ----- do not use the ones below ----
    SP_TCD_TEST7,     // TimeCircuits Display 7 (for testing)
    SP_TCD_TEST14,    // TimeCircuits Display 14 (for testing)
    SP_TCD_TEST14L    // " " " (left-aligned)
};

// If new displays are added, SP_NUM_TYPES in global.h needs to be adapted.

class speedDisplay {

    public:

        speedDisplay(uint8_t address);
        bool begin(int dispType);
        void on();
        void off();
        #if 0
        void lampTest();
        #endif

        void clearBuf();

        uint8_t setBrightness(uint8_t level, bool isInitial = false);
        uint8_t setBrightnessDirect(uint8_t level) ;
        uint8_t getBrightness();

        void setNightMode(bool mode);
        bool getNightMode();

        void show();

        void setText(const char *text);
        void setSpeed(int8_t speedNum);
        #ifdef TC_HAVETEMP
        void setTemperature(float temp);
        #endif
        void setDot(bool dot01 = true);
        void setColon(bool colon);

        uint8_t getSpeed();
        bool getDot();
        bool getColon();

        #if 0
        void showTextDirect(const char *text);
        void setColonDirect(bool colon);
        #endif

    private:

        void handleColon();
        uint16_t getLEDChar(uint8_t value);
        #if 0
        void directCol(int col, int segments);  // directly writes column RAM
        #endif
        void clearDisplay();                    // clears display RAM
        void directCmd(uint8_t val);

        uint8_t _address;
        uint16_t _displayBuffer[8];

        int8_t _onCache = -1;                   // Cache for on/off
        uint8_t _briCache = 0xfe;               // Cache for brightness

        bool _dot01 = false;
        bool _colon = false;

        uint8_t _speed = 1;

        uint8_t _brightness = 15;
        uint8_t _origBrightness = 15;
        bool    _nightmode = false;
        int     _oldnm = -1;

        uint8_t  _dispType;
        bool     _is7seg;         //      7- or 14-segment-display?
        uint8_t  _speed_pos10;    //      Speed's 10s position in 16bit buffer
        uint8_t  _speed_pos01;    //      Speed's 1s position in 16bit buffer
        uint8_t  _dig10_shift;    //      Shift 10s to align in buffer
        uint8_t  _dig01_shift;    //      Shift 1s to align in buffer
        uint8_t  _dot_pos01;      //      1s dot position in 16bit buffer
        uint8_t  _dot01_shift;    //      1s dot shift to align in buffer
        uint8_t  _colon_pos;      //      Pos of colon in 16bit buffer (255 = no colon)
        uint16_t _colon_bm;       //      bitmask for colon
        uint8_t  _num_digs;       //      total number of digits/letters (max 4)
        uint8_t  _buf_packed;     //      2 digits in one buffer pos? (0=no, 1=yes)
        uint8_t *_bufPosArr;      //      Array of buffer positions for digits left->right
        uint8_t *_bufShftArr;     //      Array of shift values for each digit

        const uint16_t *_fontXSeg;

        uint16_t _lastBufPosCol;
};

#endif
