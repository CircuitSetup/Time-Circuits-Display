/*
 * -------------------------------------------------------------------
 * CircuitSetup.us Time Circuits Display
 * (C) 2022 Thomas Winischhofer (A10001986)
 *
 * Optional Speedo Display
 * This is an example implementation, designed for a HT16K33-based 
 * display, like the "Grove - 0.54" Dual Alphanumeric Display" or
 * some display with the Adafruit i2c backpack, as long as the
 * display is either 7 or 14 segments.
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

#ifndef _speedDisplay_H
#define _speedDisplay_H

#include <Arduino.h>
#include <Wire.h>

// Type displays
// Only one may be active/uncommented
#define SP_ADAF_7x4         // Adafruit 4 digits, 7-segment (7x4) (ADA-878)     [tested]
//#define SP_ADAF_14x4      // Adafruit 4 digits, 14-segment (14x4) (ADA-2157)  [TODO]
//#define SP_GROVE_2DIG14   // Grove 0.54" Dual Alphanumeric Display            [tested]
//#define SP_TCD_TEST7      // TimeCircuits Display 7 (for testing)             [tested]
//#define SP_TCD_TEST14     // TimeCircuits Display 14 (for testing)            [tested]

//#define SP_ALIGN_LEFT       // If more than 2 digits, align left (default right)

// Display configuration

#ifdef SP_ADAF_7x4          // ##### ADAFRUIT 7x4 ############################

#define IS_SPEED_7SEG       //      7-segment tubes

#ifdef SP_ALIGN_LEFT
#define SD_SPEED_POS10 0    //      Speed's 10s position in 16bit buffer
#define SD_SPEED_POS01 1    //      Speed's 1s position in 16bit buffer
#define SD_DIG10_SHIFT 0    //      Shift 10s to align in buffer
#define SD_DIG01_SHIFT 0    //      Shift 01s to align in buffer
#define SD_DOT10_POS   0    //      10s dot position in 16bit buffer
#define SD_DOT01_POS   1    //      1s dot position in 16bit buffer
#define SD_DOT10_SHIFT 0    //      10s dot shift to align in buffer
#define SD_DOT01_SHIFT 0    //      01s dot shift to align in buffer
#else
#define SD_SPEED_POS10 3    //      Speed's 10s position in 16bit buffer
#define SD_SPEED_POS01 4    //      Speed's 1s position in 16bit buffer
#define SD_DIG10_SHIFT 0    //      Shift 10s to align in buffer
#define SD_DIG01_SHIFT 0    //      Shift 01s to align in buffer
#define SD_DOT10_POS   3    //      10s dot position in 16bit buffer
#define SD_DOT01_POS   4    //      1s dot position in 16bit buffer
#define SD_DOT10_SHIFT 0    //      10s dot shift to align in buffer
#define SD_DOT01_SHIFT 0    //      01s dot shift to align in buffer
#endif

#define SD_BUF_SIZE    8    //      total buffer size in words (16bit)

#define SD_NUM_DIGS    4    //      total number of digits/letters
#define SD_BUF_PACKED  0    //      2 digits in one buffer pos? (0=no, 1=yes)
#define SD_BUF_ARR     uint8_t bufPosArr[SD_NUM_DIGS / (1<<SD_BUF_PACKED)] = { 0, 1, 3, 4};
#endif

#ifdef SP_GROVE_2DIG14      // ##### Grove 0.56" 14x2 ################

#define SD_SPEED_POS10 2    //      Speed's 10s position in 16bit buffer
#define SD_SPEED_POS01 1    //      Speed's 1s position in 16bit buffer
#define SD_DOT10_POS   2    //      10s dot position in 16bit buffer
#define SD_DOT01_POS   1    //      1s dot position in 16bit buffer

#define SD_BUF_SIZE    8    //      total buffer size in words (16bit)

#define SD_NUM_DIGS    2    //      total number of digits/letters
#define SD_BUF_ARR     uint8_t bufPosArr[SD_NUM_DIGS] = {2, 1};
#endif

#ifdef SP_ADAF_14x4         // ##### ADAFRUIT 14x4 ###################

#ifdef SP_ALIGN_LEFT
#define SD_SPEED_POS10 0    //      Speed's 10s position in 16bit buffer
#define SD_DOT10_POS01 1    //      Speed's 1s position in 16bit buffer
#define SD_DOT10_POS   0    //      10s dot position in 16bit buffer
#define SD_DOT01_POS   1    //      1s dot position in 16bit buffer
#else
#define SD_SPEED_POS10 2    //      Speed's 10s position in 16bit buffer
#define SD_DOT10_POS01 3    //      Speed's 1s position in 16bit buffer
#define SD_DOT10_POS   2    //      10s dot position in 16bit buffer
#define SD_DOT01_POS   3    //      1s dot position in 16bit buffer
#endif

#define SD_BUF_SIZE    8    //      total buffer size in words (16bit)

#define SD_NUM_DIGS    4    //      total number of digits/letters
#define SD_BUF_ARR     uint8_t bufPosArr[SD_NUM_DIGS] = {0, 1, 2, 3};
#endif

#ifdef SP_TCD_TEST7         // ##### TCD 7 (for testing, in min) ###########

#define IS_SPEED_7SEG

#define SD_SPEED_POS10 7    //      Speed's 10s position in 16bit buffer
#define SD_SPEED_POS01 7    //      Speed's 1s position in 16bit buffer

#define SD_DIG10_SHIFT 0    //      Shift 10s to align in buffer
#define SD_DIG01_SHIFT 8    //      Shift 01s to align in buffer

#define SD_DOT10_POS   7    //      10s dot position in 16bit buffer
#define SD_DOT01_POS   7    //      1s dot position in 16bit buffer
#define SD_DOT10_SHIFT 0    //      10s dot shift to align in buffer
#define SD_DOT01_SHIFT 8    //      1s dot shift to align in buffer

#define SD_BUF_SIZE    8    //      total buffer size in words (16bit)

#define SD_NUM_DIGS    2    //      total number of digits/letters
#define SD_BUF_PACKED  1    //      2 digits in one buffer pos? (0=no, 1=yes)
#define SD_BUF_ARR     uint8_t bufPosArr[SD_NUM_DIGS / (1<<SD_BUF_PACKED)] = {7};
#endif

#ifdef SP_TCD_TEST14        // ##### TCD 14 (for testing, in month) ########

#ifdef SP_ALIGN_LEFT
#define SD_SPEED_POS10 0    //      Speed's 10s position in 16bit buffer
#define SD_SPEED_POS01 1    //      Speed's 1s position in 16bit buffer
#define SD_DOT10_POS   0    //      10s dot position in 16bit buffer
#define SD_DOT01_POS   1    //      1s dot position in 16bit buffer
#else
#define SD_SPEED_POS10 1    //      Speed's 10s position in 16bit buffer
#define SD_SPEED_POS01 2    //      Speed's 1s position in 16bit buffer
#define SD_DOT10_POS   1    //      10s dot position in 16bit buffer
#define SD_DOT01_POS   2    //      1s dot position in 16bit buffer
#endif

#define SD_BUF_SIZE    8    //      total buffer size in words (16bit)

#define SD_NUM_DIGS    3    //      total number of digits/letters
#define SD_BUF_ARR     uint8_t bufPosArr[SD_NUM_DIGS] = { 0, 1, 2 };
#endif


// The segments' wiring to buffer bits
// This needs to be adapted to actual hardware wiring

// 7 segment displays 

#ifdef SP_ADAF_7x4
#define S7_T   0b00000001        // top
#define S7_TR  0b00000010        // top right
#define S7_BR  0b00000100        // bottom right
#define S7_B   0b00001000        // bottom
#define S7_BL  0b00010000        // bottom left
#define S7_TL  0b00100000        // top left
#define S7_M   0b01000000        // middle 
#define S7_DOT 0b10000000        // dot
#endif

#ifdef SP_TCD_TEST7
#define S7_T   0b00000001        // top
#define S7_TR  0b00000010        // top right
#define S7_BR  0b00000100        // bottom right
#define S7_B   0b00001000        // bottom
#define S7_BL  0b00010000        // bottom left
#define S7_TL  0b00100000        // top left
#define S7_M   0b01000000        // middle 
#define S7_DOT 0b10000000        // dot
#endif

// 14 segment displays

#ifdef SP_GROVE_2DIG14
#define S14_T   0x0400        // top
#define S14_TL  0x4000        // top left
#define S14_TLD 0x2000        // top left diag
#define S14_TR  0x0100        // top right
#define S14_TRD 0x0800        // top right diagonal
#define S14_TV  0x1000        // top vertical
#define S14_ML  0x0200        // middle left
#define S14_MR  0x0010        // middle right
#define S14_B   0x0020        // bottom
#define S14_BL  0x0001        // bottom left
#define S14_BLD 0x0002        // bottom left diag
#define S14_BR  0x0080        // bottom right
#define S14_BRD 0x0008        // bottom right diag
#define S14_BV  0x0004        // bottom vertical
#define S14_DOT 0x0040        // dot
#endif

#ifdef SP_ADAF_14x4
#define S14_T   0b0000000000000001        // top
#define S14_TR  0b0000000000000010        // top right
#define S14_BR  0b0000000000000100        // bottom right
#define S14_B   0b0000000000001000        // bottom
#define S14_BL  0b0000000000010000        // bottom left
#define S14_TL  0b0000000000100000        // top left
#define S14_ML  0b0000000001000000        // middle left
#define S14_MR  0b0000000010000000        // middle right
#define S14_TLD 0b0000000100000000        // top left diag
#define S14_TV  0b0000001000000000        // top vertical
#define S14_TRD 0b0000010000000000        // top right diagonal
#define S14_BLD 0b0000100000000000        // bottom left diag
#define S14_BV  0b0001000000000000        // bottom vertical
#define S14_BRD 0b0010000000000000        // bottom right diag
#define S14_DOT 0b0100000000000000        // dot
#endif

#ifdef SP_TCD_TEST14
#define S14_T   0b0000000000000001        // top
#define S14_TR  0b0000000000000010        // top right
#define S14_BR  0b0000000000000100        // bottom right
#define S14_B   0b0000000000001000        // bottom
#define S14_BL  0b0000000000010000        // bottom left
#define S14_TL  0b0000000000100000        // top left
#define S14_ML  0b0000000001000000        // middle left
#define S14_MR  0b0000000010000000        // middle right
#define S14_TLD 0b0000000100000000        // top left diag
#define S14_TV  0b0000001000000000        // top vertical
#define S14_TRD 0b0000010000000000        // top right diagonal
#define S14_BLD 0b0000100000000000        // bottom left diag
#define S14_BV  0b0001000000000000        // bottom vertical
#define S14_BRD 0b0010000000000000        // bottom right diag
#define S14_DOT 0                         // dot (has none)
#endif

// ------

class speedDisplay {
  
    public:

        speedDisplay(uint8_t address);
        void begin();    
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

        void setSpeed(uint8_t speedNum);
        void setDots(bool dot10 = false, bool dot01 = true);

        uint8_t getSpeed();

        void showOnlyText(const char *text);

    private:

        uint8_t _address;
        uint16_t _displayBuffer[8]; // Segments to make current speed

        bool _dot10 = false;
        bool _dot01 = false;

        uint8_t _speed = 1;

        uint8_t _brightness = 15;   // display brightness
        uint8_t _origBrightness = 15;
        bool _nightmode = false;    // true = dest/dept times off
        int _oldnm = -1;

        #ifdef IS_SPEED_7SEG
        uint8_t  getLED7Char(uint8_t value);
        #else
        uint16_t getLED14Char(uint8_t value);
        #endif

        void directCol(int col, int segments);  // directly writes column RAM

        void clearDisplay();                    // clears display RAM
};

#endif
