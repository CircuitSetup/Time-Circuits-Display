/*
 * -------------------------------------------------------------------
 * CircuitSetup.us Time Circuits Display
 * (C) 2022 Thomas Winischhofer (A10001986)
 *
 * Speedo Display
 *
 * This is designed for HT16K33-based displays, like the "Grove - 0.54"
 * Dual/Quad Alphanumeric Display" or some displays with the Adafruit
 * i2c backpack (878, 1911, 1270; product numbers vary with color).
 * -------------------------------------------------------------------
 * License: MIT
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef _speedDisplay_H
#define _speedDisplay_H

#include <Arduino.h>
#include <Wire.h>

// The supported display types:
// The speedo is a 2-digit 7-segment display, with the bottom/right dot lit
// in the movies. I have not found a readily made one that fits exactly.
// CircuitSetup have yet to design one, all other supported ones are either
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
enum dispTypes : int {
    SP_CIRCSETUP,     // Original CircuitSetup.us speedo                        [yet to be designed]
    SP_ADAF_7x4,      // Adafruit 0.56" 4 digits, 7-segment (7x4) (ADA-878)
    SP_ADAF_7x4L,     // " " " (left-aligned)
    SP_ADAF_B7x4,     // Adafruit 1.2" 4 digits, 7-segment (7x4) (ADA-1270)
    SP_ADAF_B7x4L,    // " " " (left-aligned)
    SP_ADAF_14x4,     // Adafruit 0.56" 4 digits, 14-segment (14x4) (ADA-1911)
    SP_ADAF_14x4L,    // " " " (left-aligned)
    SP_GROVE_2DIG14,  // Grove 0.54" Dual Alphanumeric Display
    SP_GROVE_4DIG14,  // Grove 0.54" Quad Alphanumeric Display
    SP_GROVE_4DIG14L, // " " " (left aligned)
#ifdef TWPRIVATE
    SP_TWCUSTOM1,     // Like SP_ADAF_14x4, but with only left hand side tube soldered on
    SP_TWCUSTOM2,     // Like SP_ADAF_7x4L, but only 2 digits soldered on
#endif
// ----- do not use the ones below ----
    SP_TCD_TEST7,     // TimeCircuits Display 7 (for testing)
    SP_TCD_TEST14,    // TimeCircuits Display 14 (for testing)
    SP_TCD_TEST14L    // " " " (left-aligned)
};

// If new displays are added, SP_NUM_TYPES in global.h needs to be adapted.

struct dispConf {
    bool     is7seg;         //   7- or 14-segment-display?
    uint8_t  speed_pos10;    //   Speed's 10s position in 16bit buffer
    uint8_t  speed_pos01;    //   Speed's 1s position in 16bit buffer
    uint8_t  dig10_shift;    //   Shift 10s to align in buffer
    uint8_t  dig01_shift;    //   Shift 1s to align in buffer
    uint8_t  dot_pos01;      //   1s dot position in 16bit buffer
    uint8_t  dot01_shift;    //   1s dot shift to align in buffer
    uint8_t  colon_pos;      //   Pos of colon in 16bit buffer
    uint8_t  colon_shift;    //   Colon shift to align in buffer
    uint16_t colon_bit;      //   The bitmask for the colon
    uint8_t  buf_size;       //   total buffer size in words (16bit)
    uint8_t  num_digs;       //   total number of digits/letters
    uint8_t  buf_packed;     //   2 digits in one buffer pos? (0=no, 1=yes) (for 7seg only)
    uint8_t  bufPosArr[8];   //   The buffer positions of each of the digits from left to right
    const uint16_t *fontSeg; //   Pointer to font
};

// The segments' wiring to buffer bits
// This reflects the actual hardware wiring

// 7 segment displays

// 7 seg generic
#define S7G_T   0b00000001    // top
#define S7G_TR  0b00000010    // top right
#define S7G_BR  0b00000100    // bottom right
#define S7G_B   0b00001000    // bottom
#define S7G_BL  0b00010000    // bottom left
#define S7G_TL  0b00100000    // top left
#define S7G_M   0b01000000    // middle
#define S7G_DOT 0b10000000    // dot

// 14 segment displays

// Generic
#define S14_T   0b0000000000000001    // top
#define S14_TR  0b0000000000000010    // top right
#define S14_BR  0b0000000000000100    // bottom right
#define S14_B   0b0000000000001000    // bottom
#define S14_BL  0b0000000000010000    // bottom left
#define S14_TL  0b0000000000100000    // top left
#define S14_ML  0b0000000001000000    // middle left
#define S14_MR  0b0000000010000000    // middle right
#define S14_TLD 0b0000000100000000    // top left diag
#define S14_TV  0b0000001000000000    // top vertical
#define S14_TRD 0b0000010000000000    // top right diagonal
#define S14_BLD 0b0000100000000000    // bottom left diag
#define S14_BV  0b0001000000000000    // bottom vertical
#define S14_BRD 0b0010000000000000    // bottom right diag
#define S14_DOT 0b0100000000000000    // dot

// Grove 2-dig
#define S14GR_T    0x0400     // top
#define S14GR_TL   0x4000     // top left
#define S14GR_TLD  0x2000     // top left diag
#define S14GR_TR   0x0100     // top right
#define S14GR_TRD  0x0800     // top right diagonal
#define S14GR_TV   0x1000     // top vertical
#define S14GR_ML   0x0200     // middle left
#define S14GR_MR   0x0010     // middle right
#define S14GR_B    0x0020     // bottom
#define S14GR_BL   0x0001     // bottom left
#define S14GR_BLD  0x0002     // bottom left diag
#define S14GR_BR   0x0080     // bottom right
#define S14GR_BRD  0x0008     // bottom right diag
#define S14GR_BV   0x0004     // bottom vertical
#define S14GR_DOT  0x0040     // dot

// Grove 4-dig
#define S14GR4_T   0x0010     // top
#define S14GR4_TL  0x4000     // top left
#define S14GR4_TLD 0x0080     // top left diag
#define S14GR4_TR  0x0040     // top right
#define S14GR4_TRD 0x0002     // top right diagonal
#define S14GR4_TV  0x2000     // top vertical
#define S14GR4_ML  0x0200     // middle left
#define S14GR4_MR  0x0100     // middle right
#define S14GR4_B   0x0400     // bottom
#define S14GR4_BL  0x0008     // bottom left
#define S14GR4_BLD 0x1000     // bottom left diag
#define S14GR4_BR  0x0020     // bottom right
#define S14GR4_BRD 0x0004     // bottom right diag
#define S14GR4_BV  0x0800     // bottom vertical
#define S14GR4_DOT 0x0000     // dot (has none)

// ------

class speedDisplay {

    public:

        speedDisplay(uint8_t address);
        void begin(int dispType);
        void on();
        void off();
        void lampTest();

        void clear();

        uint8_t setBrightness(uint8_t level, bool isInitial = false);
        uint8_t setBrightnessDirect(uint8_t level) ;
        uint8_t getBrightness();

        void setNightMode(bool mode);
        bool getNightMode();

        void show();

        void setText(const char *text);
        void setSpeed(uint8_t speedNum);
        #ifdef TC_HAVETEMP
        void setTemperature(double temp);
        #endif
        void setDot(bool dot01 = true);
        void setColon(bool colon);

        uint8_t getSpeed();
        bool getDot();
        bool getColon();

        void showTextDirect(const char *text);
        void setColonDirect(bool colon);

    private:

        uint8_t _address;
        uint16_t _displayBuffer[8];

        bool _dot01 = false;
        bool _colon = false;

        uint8_t _speed = 1;

        uint8_t _brightness = 15;
        uint8_t _origBrightness = 15;
        bool    _nightmode = false;
        int     _oldnm = -1;

        int      _dispType;
        bool     _is7seg;         //      7- or 14-segment-display?
        uint8_t  _speed_pos10;    //      Speed's 10s position in 16bit buffer
        uint8_t  _speed_pos01;    //      Speed's 1s position in 16bit buffer
        uint8_t  _dig10_shift;    //      Shift 10s to align in buffer
        uint8_t  _dig01_shift;    //      Shift 1s to align in buffer
        uint8_t  _dot_pos01;      //      1s dot position in 16bit buffer
        uint8_t  _dot01_shift;    //      1s dot shift to align in buffer
        uint8_t  _colon_pos;      //      Pos of colon in 16bit buffer (255 = no colon)
        uint8_t  _colon_shift;    //      Colon shift to align in buffer
        uint16_t _colon_bm;       //      bitmask for colon
        uint8_t  _buf_size;       //      total buffer size in words (16bit)
        uint8_t  _num_digs;       //      total number of digits/letters
        uint8_t  _buf_packed;     //      2 digits in one buffer pos? (0=no, 1=yes)
        uint8_t *_bufPosArr;      //      Array of buffer positions for digits left->right

        const uint16_t *_fontXSeg;

        uint16_t _lastBufPosCol;

        void handleColon();
        uint16_t getLEDChar(uint8_t value);
        void directCol(int col, int segments);  // directly writes column RAM
        void clearDisplay();                    // clears display RAM
};

#endif
