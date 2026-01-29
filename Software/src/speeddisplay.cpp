/*
 * -------------------------------------------------------------------
 * CircuitSetup.us Time Circuits Display
 * (C) 2022-2026 Thomas Winischhofer (A10001986)
 * https://github.com/realA10001986/Time-Circuits-Display
 * https://tcd.out-a-ti.me
 *
 * speedDisplay Class: Speedo Display
 *
 * This is designed for CircuitSetup's Speedo display and other Holtek
 * HT16K33-based displays, like the "Grove - 0.54" Dual/Quad 
 * Alphanumeric Display" or some displays with an Adafruit i2c backpack:
 * ADA-878/877/5599 (other colors: 879/880/881/1002/5601/5602/5603/5604)
 * ADA-1270/1271 (other colors: 1269)
 * ADA-1911/1910 (other colors: 1912/2157/2158/2160)
 * The i2c slave address must be 0x70.
 * 
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
#include <math.h>
#include "speeddisplay.h"
#include <Wire.h>

// Speedo displays "--" for NO_FIX_DASHES ms if GPS fix is
// lost, afterwards it will display "00.".
#define NO_FIX_DASHES 1000*60

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

static const uint16_t font7segGeneric[38] = {
    S7G_T|S7G_TR|S7G_BR|S7G_B|S7G_BL|S7G_TL,
    S7G_TR|S7G_BR,
    S7G_T|S7G_TR|S7G_B|S7G_BL|S7G_M,
    S7G_T|S7G_TR|S7G_BR|S7G_B|S7G_M,
    S7G_TR|S7G_BR|S7G_TL|S7G_M,
    S7G_T|S7G_BR|S7G_B|S7G_TL|S7G_M,
    S7G_BR|S7G_B|S7G_BL|S7G_TL|S7G_M,
    S7G_T|S7G_TR|S7G_BR,
    S7G_T|S7G_TR|S7G_BR|S7G_B|S7G_BL|S7G_TL|S7G_M,
    S7G_T|S7G_TR|S7G_BR|S7G_TL|S7G_M,
    S7G_T|S7G_TR|S7G_BR|S7G_BL|S7G_TL|S7G_M,
    S7G_BR|S7G_B|S7G_BL|S7G_TL|S7G_M,
    S7G_T|S7G_B|S7G_BL|S7G_TL,
    S7G_TR|S7G_BR|S7G_B|S7G_BL|S7G_M,
    S7G_T|S7G_B|S7G_BL|S7G_TL|S7G_M,
    S7G_T|S7G_BL|S7G_TL|S7G_M,
    S7G_T|S7G_BR|S7G_B|S7G_BL|S7G_TL,
    S7G_TR|S7G_BR|S7G_BL|S7G_TL|S7G_M,
    S7G_BL|S7G_TL,
    S7G_TR|S7G_BR|S7G_B|S7G_BL,
    S7G_T|S7G_BR|S7G_BL|S7G_TL|S7G_M,
    S7G_B|S7G_BL|S7G_TL,
    S7G_T|S7G_BR|S7G_BL,
    S7G_T|S7G_TR|S7G_BR|S7G_BL|S7G_TL,
    S7G_T|S7G_TR|S7G_BR|S7G_B|S7G_BL|S7G_TL,
    S7G_T|S7G_TR|S7G_BL|S7G_TL|S7G_M,
    S7G_T|S7G_TR|S7G_B|S7G_TL|S7G_M,
    S7G_T|S7G_TR|S7G_BL|S7G_TL,
    S7G_T|S7G_BR|S7G_B|S7G_TL|S7G_M,
    S7G_B|S7G_BL|S7G_TL|S7G_M,
    S7G_TR|S7G_BR|S7G_B|S7G_BL|S7G_TL,
    S7G_TR|S7G_BR|S7G_B|S7G_BL|S7G_TL,
    S7G_TR|S7G_B|S7G_TL,
    S7G_TR|S7G_BR|S7G_BL|S7G_TL|S7G_M,
    S7G_TR|S7G_BR|S7G_B|S7G_TL|S7G_M,
    S7G_T|S7G_TR|S7G_B|S7G_BL|S7G_M,
    S7G_DOT,
    S7G_M
};

static const uint16_t font14segGeneric[38] = {
    S14_T|S14_TL|S14_TR|S14_B|S14_BL|S14_BR,
    S14_TR|S14_BR,
    S14_T|S14_TR|S14_ML|S14_MR|S14_B|S14_BL,
    S14_T|S14_TR|S14_ML|S14_MR|S14_B|S14_BR,
    S14_TL|S14_TR|S14_ML|S14_MR|S14_BR,
    S14_T|S14_TL|S14_ML|S14_MR|S14_B|S14_BR,
    S14_TL|S14_ML|S14_MR|S14_B|S14_BL|S14_BR,
    S14_T|S14_TR|S14_BR,
    S14_T|S14_TL|S14_TR|S14_ML|S14_MR|S14_B|S14_BL|S14_BR,
    S14_T|S14_TL|S14_TR|S14_ML|S14_MR|S14_BR,
    S14_T|S14_TL|S14_TR|S14_ML|S14_MR|S14_BL|S14_BR,
    S14_T|S14_TR|S14_TV|S14_MR|S14_B|S14_BR|S14_BV,
    S14_T|S14_TL|S14_B|S14_BL,
    S14_T|S14_TR|S14_TV|S14_B|S14_BR|S14_BV,
    S14_T|S14_TL|S14_ML|S14_MR|S14_B|S14_BL,
    S14_T|S14_TL|S14_ML|S14_BL,
    S14_T|S14_TL|S14_MR|S14_B|S14_BL|S14_BR,
    S14_TL|S14_TR|S14_ML|S14_MR|S14_BL|S14_BR,
    S14_T|S14_TV|S14_B|S14_BV,
    S14_TR|S14_B|S14_BL|S14_BR,
    S14_TL|S14_TRD|S14_ML|S14_BL|S14_BRD,
    S14_TL|S14_B|S14_BL,
    S14_T|S14_TL|S14_TR|S14_TV|S14_BL|S14_BR,
    S14_TL|S14_TLD|S14_TR|S14_BL|S14_BR|S14_BRD,
    S14_T|S14_TL|S14_TR|S14_B|S14_BL|S14_BR,
    S14_T|S14_TL|S14_TR|S14_ML|S14_MR|S14_BL,
    S14_T|S14_TL|S14_TR|S14_B|S14_BL|S14_BR|S14_BRD,
    S14_T|S14_TL|S14_TR|S14_ML|S14_MR|S14_BL|S14_BRD,
    S14_T|S14_TL|S14_ML|S14_MR|S14_B|S14_BR,
    S14_T|S14_TV|S14_BV,
    S14_TL|S14_TR|S14_B|S14_BL|S14_BR,
    S14_TL|S14_TRD|S14_BL|S14_BLD,
    S14_TL|S14_TR|S14_BL|S14_BLD|S14_BR|S14_BRD,
    S14_TLD|S14_TRD|S14_BLD|S14_BRD,
    S14_TL|S14_TR|S14_ML|S14_MR|S14_B|S14_BR,
    S14_T|S14_TRD|S14_B|S14_BLD,
    S14_DOT,
    S14_ML|S14_MR
};

static const uint16_t font14segGrove[38] = {
    S14GR_T|S14GR_TL|S14GR_TR|S14GR_B|S14GR_BL|S14GR_BR,
    S14GR_TR|S14GR_BR,
    S14GR_T|S14GR_TR|S14GR_ML|S14GR_MR|S14GR_B|S14GR_BL,
    S14GR_T|S14GR_TR|S14GR_ML|S14GR_MR|S14GR_B|S14GR_BR,
    S14GR_TL|S14GR_TR|S14GR_ML|S14GR_MR|S14GR_BR,
    S14GR_T|S14GR_TL|S14GR_ML|S14GR_MR|S14GR_B|S14GR_BR,
    S14GR_TL|S14GR_ML|S14GR_MR|S14GR_B|S14GR_BL|S14GR_BR,
    S14GR_T|S14GR_TR|S14GR_BR,
    S14GR_T|S14GR_TL|S14GR_TR|S14GR_ML|S14GR_MR|S14GR_B|S14GR_BL|S14GR_BR,
    S14GR_T|S14GR_TL|S14GR_TR|S14GR_ML|S14GR_MR|S14GR_BR,
    S14GR_T|S14GR_TL|S14GR_TR|S14GR_ML|S14GR_MR|S14GR_BL|S14GR_BR,
    S14GR_T|S14GR_TR|S14GR_TV|S14GR_MR|S14GR_B|S14GR_BR|S14GR_BV,
    S14GR_T|S14GR_TL|S14GR_B|S14GR_BL,
    S14GR_T|S14GR_TR|S14GR_TV|S14GR_B|S14GR_BR|S14GR_BV,
    S14GR_T|S14GR_TL|S14GR_ML|S14GR_MR|S14GR_B|S14GR_BL,
    S14GR_T|S14GR_TL|S14GR_ML|S14GR_BL,
    S14GR_T|S14GR_TL|S14GR_MR|S14GR_B|S14GR_BL|S14GR_BR,
    S14GR_TL|S14GR_TR|S14GR_ML|S14GR_MR|S14GR_BL|S14GR_BR,
    S14GR_T|S14GR_TV|S14GR_B|S14GR_BV,
    S14GR_TR|S14GR_B|S14GR_BL|S14GR_BR,
    S14GR_TL|S14GR_TRD|S14GR_ML|S14GR_BL|S14GR_BRD,
    S14GR_TL|S14GR_B|S14GR_BL,
    S14GR_T|S14GR_TL|S14GR_TR|S14GR_TV|S14GR_BL|S14GR_BR,
    S14GR_TL|S14GR_TLD|S14GR_TR|S14GR_BL|S14GR_BR|S14GR_BRD,
    S14GR_T|S14GR_TL|S14GR_TR|S14GR_B|S14GR_BL|S14GR_BR,
    S14GR_T|S14GR_TL|S14GR_TR|S14GR_ML|S14GR_MR|S14GR_BL,
    S14GR_T|S14GR_TL|S14GR_TR|S14GR_B|S14GR_BL|S14GR_BR|S14GR_BRD,
    S14GR_T|S14GR_TL|S14GR_TR|S14GR_ML|S14GR_MR|S14GR_BL|S14GR_BRD,
    S14GR_T|S14GR_TL|S14GR_ML|S14GR_MR|S14GR_B|S14GR_BR,
    S14GR_T|S14GR_TV|S14GR_BV,
    S14GR_TL|S14GR_TR|S14GR_B|S14GR_BL|S14GR_BR,
    S14GR_TL|S14GR_TRD|S14GR_BL|S14GR_BLD,
    S14GR_TL|S14GR_TR|S14GR_BL|S14GR_BLD|S14GR_BR|S14GR_BRD,
    S14GR_TLD|S14GR_TRD|S14GR_BLD|S14GR_BRD,
    S14GR_TL|S14GR_TR|S14GR_ML|S14GR_MR|S14GR_B|S14GR_BR,
    S14GR_T|S14GR_TRD|S14GR_B|S14GR_BLD,
    S14GR_DOT,
    S14GR_ML|S14GR_MR
};

static const uint16_t font144segGrove[38] = {
    S14GR4_T|S14GR4_TL|S14GR4_TR|S14GR4_B|S14GR4_BL|S14GR4_BR,
    S14GR4_TR|S14GR4_BR,
    S14GR4_T|S14GR4_TR|S14GR4_ML|S14GR4_MR|S14GR4_B|S14GR4_BL,
    S14GR4_T|S14GR4_TR|S14GR4_ML|S14GR4_MR|S14GR4_B|S14GR4_BR,
    S14GR4_TL|S14GR4_TR|S14GR4_ML|S14GR4_MR|S14GR4_BR,
    S14GR4_T|S14GR4_TL|S14GR4_ML|S14GR4_MR|S14GR4_B|S14GR4_BR,
    S14GR4_TL|S14GR4_ML|S14GR4_MR|S14GR4_B|S14GR4_BL|S14GR4_BR,
    S14GR4_T|S14GR4_TR|S14GR4_BR,
    S14GR4_T|S14GR4_TL|S14GR4_TR|S14GR4_ML|S14GR4_MR|S14GR4_B|S14GR4_BL|S14GR4_BR,
    S14GR4_T|S14GR4_TL|S14GR4_TR|S14GR4_ML|S14GR4_MR|S14GR4_BR,
    S14GR4_T|S14GR4_TL|S14GR4_TR|S14GR4_ML|S14GR4_MR|S14GR4_BL|S14GR4_BR,
    S14GR4_T|S14GR4_TR|S14GR4_TV|S14GR4_MR|S14GR4_B|S14GR4_BR|S14GR4_BV,
    S14GR4_T|S14GR4_TL|S14GR4_B|S14GR4_BL,
    S14GR4_T|S14GR4_TR|S14GR4_TV|S14GR4_B|S14GR4_BR|S14GR4_BV,
    S14GR4_T|S14GR4_TL|S14GR4_ML|S14GR4_MR|S14GR4_B|S14GR4_BL,
    S14GR4_T|S14GR4_TL|S14GR4_ML|S14GR4_BL,
    S14GR4_T|S14GR4_TL|S14GR4_MR|S14GR4_B|S14GR4_BL|S14GR4_BR,
    S14GR4_TL|S14GR4_TR|S14GR4_ML|S14GR4_MR|S14GR4_BL|S14GR4_BR,
    S14GR4_T|S14GR4_TV|S14GR4_B|S14GR4_BV,
    S14GR4_TR|S14GR4_B|S14GR4_BL|S14GR4_BR,
    S14GR4_TL|S14GR4_TRD|S14GR4_ML|S14GR4_BL|S14GR4_BRD,
    S14GR4_TL|S14GR4_B|S14GR4_BL,
    S14GR4_T|S14GR4_TL|S14GR4_TR|S14GR4_TV|S14GR4_BL|S14GR4_BR,
    S14GR4_TL|S14GR4_TLD|S14GR4_TR|S14GR4_BL|S14GR4_BR|S14GR4_BRD,
    S14GR4_T|S14GR4_TL|S14GR4_TR|S14GR4_B|S14GR4_BL|S14GR4_BR,
    S14GR4_T|S14GR4_TL|S14GR4_TR|S14GR4_ML|S14GR4_MR|S14GR4_BL,
    S14GR4_T|S14GR4_TL|S14GR4_TR|S14GR4_B|S14GR4_BL|S14GR4_BR|S14GR4_BRD,
    S14GR4_T|S14GR4_TL|S14GR4_TR|S14GR4_ML|S14GR4_MR|S14GR4_BL|S14GR4_BRD,
    S14GR4_T|S14GR4_TL|S14GR4_ML|S14GR4_MR|S14GR4_B|S14GR4_BR,
    S14GR4_T|S14GR4_TV|S14GR4_BV,
    S14GR4_TL|S14GR4_TR|S14GR4_B|S14GR4_BL|S14GR4_BR,
    S14GR4_TL|S14GR4_TRD|S14GR4_BL|S14GR4_BLD,
    S14GR4_TL|S14GR4_TR|S14GR4_BL|S14GR4_BLD|S14GR4_BR|S14GR4_BRD,
    S14GR4_TLD|S14GR4_TRD|S14GR4_BLD|S14GR4_BRD,
    S14GR4_TL|S14GR4_TR|S14GR4_ML|S14GR4_MR|S14GR4_B|S14GR4_BR,
    S14GR4_T|S14GR4_TRD|S14GR4_B|S14GR4_BLD,
    S14GR4_DOT,
    S14GR4_ML|S14GR4_MR
};

enum speedoTypes : uint8_t {
    SPT_NONE = 0,
    SPT_I2C_14S,
    SPT_I2C_7S,
    SPT_BTTFN
};

static const struct dispConf {
    uint8_t  speedoType;     //   i2c-7, i2c-14, bttfn, ...
    uint8_t  speed_pos10;    //   Speed's 10s position in 16bit buffer
    uint8_t  speed_pos01;    //   Speed's 1s position in 16bit buffer
    uint8_t  dig10_shift;    //   Shift 10s to align in buffer
    uint8_t  dig01_shift;    //   Shift 1s to align in buffer
    uint8_t  dot_pos01;      //   1s dot position in 16bit buffer
    uint8_t  dot01_shift;    //   1s dot shift to align in buffer
    uint8_t  colon_pos;      //   Pos of center colon in 16bit buffer (255 = no colon)
    uint16_t colon_bit;      //   The bitmask for the center colon
    uint8_t  num_digs;       //   total number of digits/letters
    uint8_t  bufPosArr[4];   //   The buffer positions of each of the digits from left to right
    uint8_t  bufShftArr[4];  //   Shift-value for each digit from left to right
    const uint16_t *fontSeg; //   Pointer to font
} displays[SP_NUM_TYPES] = {
  { SPT_I2C_7S,  0, 1, 0, 8, 1, 8, 255,      0, 2, { 0, 1 },       { 0, 8 },       font7segGeneric },  // CircuitSetup Speedo+GPS add-on
  { SPT_I2C_7S,  3, 4, 0, 0, 4, 0,   2, 0x0002, 4, { 0, 1, 3, 4 }, { 0, 0, 0, 0 }, font7segGeneric },  // SP_ADAF_7x4   0.56" (right) (ADA-878/877/5599;879/880/881/1002/5601/5602/5603/5604)
  { SPT_I2C_7S,  0, 1, 0, 0, 1, 0,   2, 0x0002, 4, { 0, 1, 3, 4 }, { 0, 0, 0, 0 }, font7segGeneric },  // SP_ADAF_7x4L  0.56" (left)  (ADA-878/877/5599;879/880/881/1002/5601/5602/5603/5604)
  { SPT_I2C_7S,  3, 4, 0, 0, 4, 0,   2, 0x0002, 4, { 0, 1, 3, 4 }, { 0, 0, 0, 0 }, font7segGeneric },  // SP_ADAF_B7x4  1.2" (right)  (ADA-1270/1271;1269)
  { SPT_I2C_7S,  0, 1, 0, 0, 1, 0,   2, 0x0002, 4, { 0, 1, 3, 4 }, { 0, 0, 0, 0 }, font7segGeneric },  // SP_ADAF_B7x4L 1.2" (left)   (ADA-1270/1271;1269)
  { SPT_I2C_14S, 2, 3, 0, 0, 3, 0, 255,      0, 4, { 0, 1, 2, 3 }, { 0, 0, 0, 0 }, font14segGeneric }, // SP_ADAF_14x4  0.56" (right) (ADA-1911/1910;1912/2157/2158/2160)
  { SPT_I2C_14S, 0, 1, 0, 0, 1, 0, 255,      0, 4, { 0, 1, 2, 3 }, { 0, 0, 0, 0 }, font14segGeneric }, // SP_ADAF_14x4L 0.56" (left)  (ADA-1911/1910;1912/2157/2158/2160)
  { SPT_I2C_14S, 2, 1, 0, 0, 1, 0, 255,      0, 2, { 2, 1 },       { 0, 0, 0, 0 }, font14segGrove },   // SP_GROVE_2DIG14
  { SPT_I2C_14S, 3, 4, 0, 0, 4, 0,   5, 0x2080, 4, { 1, 2, 3, 4 }, { 0, 0, 0, 0 }, font144segGrove },  // SP_GROVE_4DIG14 (right)
  { SPT_I2C_14S, 1, 2, 0, 0, 2, 0,   5, 0x2080, 4, { 1, 2, 3, 4 }, { 0, 0, 0, 0 }, font144segGrove },  // SP_GROVE_4DIG14 (left)
  { SPT_I2C_14S, 0, 1, 0, 0, 1, 0, 255,      0, 2, { 0, 1 },       { 0, 0, 0, 0 }, font14segGeneric }, // like SP_ADAF_14x4L(ADA-1911), but left tube only (TW wall clock)
  { SPT_I2C_7S,  0, 1, 0, 0, 1, 0, 255,      0, 2, { 0, 1 },       { 0, 0, 0, 0 }, font7segGeneric },  // like SP_ADAF_7x4L(ADA-878), but left tube only (TW speedo replica) - needs rewiring
  { SP_BTTFN,    0, 0, 0, 0, 0, 0,   0,      0, 0, { 0 },          { 0, 0, 0, 0 }, NULL            },  // BTTFN
//{ SPT_I2C_7S,  1, 2, 8, 0, 3, 0, 255,      0, 2, { 1, 2 },       { 8, 0 },       font7segGeneric },  // CircuitSetup Speedo+GPS, part-3-"wrong"-style (left dgt covered)
};

// Grove 4-digit special handling
static const uint16_t gr4_sh1[4] = { 1<<4, 1<<6,  1<<5, 1<<10 };
static const uint16_t gr4_sh2[4] = { 1<<3, 1<<14, 1<<9, 1<<8  };

// ADA-1270:
// Pos 2: 0x02 - center colon (both dots),  0x04 - left colon - lower dot
//        0x08 - left colon - upper dot,    0x10 - decimal point (upper right)

// Store i2c address
speedDisplay::speedDisplay(uint8_t address)
{
    _address = address;
}

// Start the display
bool speedDisplay::begin(int dispType, int sSpeedoPin, int sTachopin, int sSpeedoCorr, int sTachoCorr)
{
    if(dispType < 0 || dispType >= SP_NUM_TYPES) {
        #ifdef TC_DBG
        Serial.printf("Bad speedo display type: %d\n", dispType);
        #endif
        dispType = 0;
    }

    _dispType = dispType;
    _speedoType = displays[dispType].speedoType;

    #ifdef SERVOSPEEDO
    if(!_haveSec) {
        if(sSpeedoPin) {
            pinMode(sSpeedoPin, OUTPUT);
            ledcSetup(0, 50, 16);
            ledcAttachPin(sSpeedoPin, 0);
            _haveSSpeedo = true;
            spos_corr = sSpeedoCorr;
        }
        if(sTachopin) {
            pinMode(sTachopin, OUTPUT);
            ledcSetup(1, 50, 16);
            ledcAttachPin(sTachopin, 1);
            _haveSTacho = true;
            tpos_corr = sTachoCorr;
        }
        if(_haveSSpeedo || _haveSTacho) {
            _secOff = true;
            _haveSec = _doSec = true;
            setExSpeed(-1, true);
        }
    }
    #endif
    
    _i2c = false;

    switch(_speedoType) {

    case SPT_I2C_7S:
    case SPT_I2C_14S:
        // Check for speedo on i2c bus
        Wire.beginTransmission(_address);
        if(Wire.endTransmission(true)) {
            _speedoType = SPT_NONE;
            #ifdef SERVOSPEEDO
            return _haveSec;
            #else
            return false;
            #endif
        }
        _i2c = true;
        break;

    //case SPT_BTTFN:
    //    break;
    }

    if(_i2c) {
        _is7seg = (_speedoType == SPT_I2C_7S);
        _speed_pos10 = displays[dispType].speed_pos10;
        _speed_pos01 = displays[dispType].speed_pos01;
        _dig10_shift = displays[dispType].dig10_shift;
        _dig01_shift = displays[dispType].dig01_shift;
        _dot_pos01 = displays[dispType].dot_pos01;
        _dot01_shift = displays[dispType].dot01_shift;
        _colon_pos = displays[dispType].colon_pos;
        _colon_bm = displays[dispType].colon_bit;
        _num_digs = displays[dispType].num_digs;
        _bufPosArr = displays[dispType].bufPosArr;
        _bufShftArr = displays[dispType].bufShftArr;
        
        _fontXSeg = displays[dispType].fontSeg;
    
        directCmd(0x20 | 1); // turn on oscillator
    
        clearBuf();          // clear buffer
        setBrightness(15);   // setup initial brightness
        clearDisplay();      // clear display RAM
        on();                // turn it on
    
        thirdDig = (_dispType == SP_CIRCSETUP);
    }

    return true;
}

void speedDisplay::validateSetup()
{
    if(_i2c) {
        switch(_dispType) {
        case SP_CIRCSETUP:
            break;
        case SP_CIRCSETUP3:
            dispL0Spd = true;
            setDot(false);
            break;
        default:
            thirdDig = false;
        }
    }
}

bool speedDisplay::haveSpeedoDisplay()
{
    #ifdef SERVOSPEEDO
    return _i2c || _haveSec;
    #else
    return _i2c;
    #endif
}

#ifdef TC_HAVETEMP
bool speedDisplay::supportsTemperature()
{
    if(_i2c)
        return true;
        
    // Temperature on i2c only

    return false;
}
#endif

// Turn on the display
void speedDisplay::on()
{
    if(_onCache > 0) return;
    if(_i2c) {
        directCmd(0x80 | 1);
    }
    #ifdef SERVOSPEEDO
    _secOff = false;
    setExSpeed(_secSpeed, true);
    #endif
    _onCache = 1;
    _briCache = 0xfe;
}

// Turn off the display
void speedDisplay::off()
{
    if(!_onCache) return;
    if(_i2c) {
        directCmd(0x80);
    }
    #ifdef SERVOSPEEDO
    _secOff = true;
    setExSpeed(-1, true);
    #endif
    _onCache = 0;
}

// Turn on all LEDs
#if 0
void speedDisplay::lampTest()
{
    if(_i2c) {
        Wire.beginTransmission(_address);
        Wire.write(0x00);  // start address
    
        for(int i = 0; i < 8*2; i++) {
            Wire.write(0xFF);
        }
        Wire.endTransmission();
    }
}
#endif

// Set display brightness
// Valid brighness levels are 0 to 15. Default is 15.
// 255 sets it to previous level
uint8_t speedDisplay::setBrightness(uint8_t level, bool isInitial)
{
    if(level == 255)
        level = _brightness;    // restore to old val

    _brightness = setBrightnessDirect(level);

    if(isInitial)
        _origBrightness = _brightness;

    return _brightness;
}

uint8_t speedDisplay::setBrightnessDirect(uint8_t level)
{
    if(level > 15)
        level = 15;

    if(level != _briCache) {
        if(_i2c) directCmd(0xE0 | level);  // Dimming command
        _briCache = level;
    }

    return level;
}


// Show data in display --------------------------------------------------------


// Show the buffer
void speedDisplay::show()
{
    int i;

    if(_nightmode) {
        if(_oldnm < 1) {
            setBrightness(0);
            _oldnm = 1;
        }
    } else {
        if(_oldnm > 0) {
            setBrightness(_origBrightness);
            _oldnm = 0;
        }
    }

    if(_i2c) {

        switch(_dispType) {
        case SP_ADAF_B7x4:
        case SP_ADAF_B7x4L:
            _displayBuffer[_colon_pos] &= ~(0x10);
            if(_displayBuffer[*(_bufPosArr + 2)] & S7G_DOT) {
                _displayBuffer[_colon_pos] |= 0x10;
            }
            break;
        case SP_GROVE_4DIG14:
        case SP_GROVE_4DIG14L:
            _displayBuffer[_colon_pos] &= ~(0x4778);
            for(int i = 0; i < 4; i++) {
                _displayBuffer[_colon_pos] |= ((_displayBuffer[*(_bufPosArr + i)] & 0x02) ? gr4_sh1[i] : 0);
                _displayBuffer[_colon_pos] |= ((_displayBuffer[*(_bufPosArr + i)] & 0x04) ? gr4_sh2[i] : 0);
            }
        }
    
        Wire.beginTransmission(_address);
        Wire.write(0x00);  // start address
    
        for(i = 0; i < 8; i++) {
            Wire.write(_displayBuffer[i] & 0xFF);
            Wire.write(_displayBuffer[i] >> 8);
        }
    
        Wire.endTransmission();

    }
    
    #ifdef SERVOSPEEDO
    setExSpeed(_secSpeed);
    #endif
}


// Set data in buffer --------------------------------------------------------


// Write given text to buffer
// (including current colon setting; if the text contains a dot,
// the dot between the adjacent letters is lit. dot01 setting is
// ignored.)
void speedDisplay::setText(const char *text)
{
    int idx = 0, pos = 0;
    int temp = 0;

    if(_i2c) {

        clearBuf();
    
        if(_is7seg) {
            while(text[idx] && (pos < _num_digs)) {
                temp = getLEDChar(text[idx]) << (*(_bufShftArr + pos));
                idx++;
                if(text[idx] == '.') {
                    temp |= (getLEDChar('.') << (*(_bufShftArr + pos)));
                    idx++;
                }
                _displayBuffer[*(_bufPosArr + pos)] |= temp;
                pos++;
            }
        } else {
            while(text[idx] && pos < _num_digs) {
                _displayBuffer[*(_bufPosArr + pos)] = getLEDChar(text[idx]);
                idx++;
                if(text[idx] == '.') {
                    _displayBuffer[*(_bufPosArr + pos)] |= getLEDChar('.');
                    idx++;
                }
                pos++;
            }
        }
    
        //handleColon();
    }
}

// Write given speed to buffer
// (including current dot01 setting; colon is cleared and ignored)
void speedDisplay::setSpeed(int speedNum)
{
    _speed = speedNum;

    if(_i2c) {
        unsigned long b1 = 0, b2 = 0; 
        uint8_t b3 = 0b00111111;
        unsigned long now = millis();
    
        clearBuf();
    
        if(speedNum < 0) {
            if(_lastPosSpd > 3) {
                if(!_posSpdNow) _posSpdNow = now;
                if(now - _posSpdNow < NO_FIX_DASHES) {
                    b1 = b2 = 37;
                    b3 = 0;
                } else {
                    _lastPosSpd = 0;
                }
            }
        } else {
            _posSpdNow = now;
            _lastPosSpd = speedNum;
            if(speedNum > 99) {
                b1 = 'H' - 'A' + 10;
                b2 = 'I' - 'A' + 10;
                b3 = 0;
            } else {
                b1 = speedNum / 10;
                b2 = speedNum % 10;
            }
        }
        b1 = *(_fontXSeg + b1);
        b2 = *(_fontXSeg + b2);
    
        // CircuitSetup Speedo: Enable/disable third digit
        if(thirdDig) {
            // Hack to display "0" after dot
            _displayBuffer[2] = b3;
        }

        if(dispL0Spd || speedNum > 9 /*|| b1 == 37*/) _displayBuffer[_speed_pos10] |= (b1 << _dig10_shift);
        _displayBuffer[_speed_pos01] |= (b2 << _dig01_shift);
        
        if(_dot01) _displayBuffer[_dot_pos01] |= (*(_fontXSeg + 36) << _dot01_shift);
    }

    #ifdef SERVOSPEEDO
    _secSpeed = speedNum;
    #endif
}

#ifdef TC_HAVETEMP
void speedDisplay::setTemperature(float temp)
{
    if(_i2c) {

        char buf[8];
        char alignBuf[20];
        int t, strlenBuf = 0;
        const char *myNan = "----";
    
        bool tempNan = isnan(temp);
    
        switch(_num_digs) {
        case 2:
            if(tempNan)            setText(myNan);
            else if(temp <= -10.0f) setText("Lo");
            else if(temp >= 100.0f) setText("Hi");
            else if(temp >= 10.0f || temp < 0.0f) {
                t = (int)roundf(temp);
                sprintf(buf, "%d", t);
                setText(buf);
            } else {
                sprintf(buf, "%.1f", temp);
                setText(buf);
            }
            break;
        case 3:
            if(tempNan)             setText(myNan);
            else if(temp <= -100.0f) setText("Low");
            else if(temp >= 1000.0f) setText("Hi");
            else if(temp >= 100.0f || temp <= -10.0f) {
                t = (int)roundf(temp);
                sprintf(buf, "%d", t);
                setText(buf);
            } else {
                sprintf(buf, "%.1f", temp);
                setText(buf);
            }
            break;
        default:
            sprintf(buf, tempNan ? myNan : "%.1f", temp);
            for(int i = 0; i < strlen(buf); i++) {
                if(buf[i] != '.') strlenBuf++;
            }
            if(strlenBuf < _num_digs) {
                for(int i = 0; i < 15 && i < (_num_digs - strlenBuf); i++) {
                    alignBuf[i] = ' ';
                    alignBuf[i+1] = 0;
                }
                strcat(alignBuf, buf);
                setText(alignBuf);
            } else {
                setText(buf);
            }
        }

    }

    #ifdef SERVOSPEEDO
    _secSpeed = -1;
    #endif
}
#endif

#ifdef SERVOSPEEDO
void speedDisplay::setCalib(int calib)
{
    if(_haveSec) {
        _doSec = (calib < 0);
        setExSpeed(_doSec ? _secSpeed : calib, true);
    }
}
bool speedDisplay::setSCorr(int corr)
{
    if(_haveSec && !_doSec) {
        spos_corr = corr;
        setExSpeed(_secSpeedOld, true);
        return true;
    }
    return false;
}
bool speedDisplay::setTCorr(int corr)
{
    if(_haveSec && !_doSec) {
        tpos_corr = corr;
        setExSpeed(_secSpeedOld, true);
        return true;
    }
    return false;
}
void speedDisplay::getCorr(int& scorr, int &tcorr)
{
    scorr = spos_corr;
    tcorr = tpos_corr;
}
#endif

// Private functions ###########################################################

// Clear the buffer
void speedDisplay::clearBuf()
{
    // must call show() to actually clear display

    for(int i = 0; i < 8; i++) {
        _displayBuffer[i] = 0;
    }
}

/*
void speedDisplay::handleColon()
{
    if(_colon_pos < 255) {
        if(_colon) _displayBuffer[_colon_pos] |= _colon_bm;
        else       _displayBuffer[_colon_pos] &= (~_colon_bm);
    }
}
*/

// Returns bit pattern for provided character
uint16_t speedDisplay::getLEDChar(uint8_t value)
{
    if(value >= '0' && value <= '9') {
        return *(_fontXSeg + (value - '0'));
    } else if(value == '-') {
        return *(_fontXSeg + 37);
    } else if(value >= 'A' && value <= 'Z') {
        return *(_fontXSeg + (value - 'A' + 10));
    } else if(value >= 'a' && value <= 'z') {
        return *(_fontXSeg + (value - 'a' + 10));
    } else if(value == '.')
        return *(_fontXSeg + 36);
    
    return 0;
}

#if 0 // Unused currently
// Directly write to a column with supplied segments
// (leave buffer intact, directly write to display)
void speedDisplay::directCol(int col, int segments)
{
    Wire.beginTransmission(_address);
    Wire.write(col * 2);  // 2 bytes per col * position
    Wire.write(segments & 0xFF);
    Wire.write(segments >> 8);
    Wire.endTransmission();
}
#endif

// Directly clear the display
void speedDisplay::clearDisplay()
{
    Wire.beginTransmission(_address);
    Wire.write(0x00);  // start address

    for(int i = 0; i < 8*2; i++) {
        Wire.write(0x0);
    }

    Wire.endTransmission();
}

void speedDisplay::directCmd(uint8_t val)
{
    Wire.beginTransmission(_address);
    Wire.write(val);
    Wire.endTransmission();
}

#ifdef SERVOSPEEDO

// 1 second = 1000000 us
// 50Hz = 0.02 seconds period = 20000 uS = 65535
// 500 uS  = duty cycle 1638
// 2500 uS = duty cycle 8192
// dc = ((uint32_t)((((270.0 - angle) * 2000.0) / 270.0) * 65536.0 / 20000.0)) + DC_MIN;

static const int sspdpos[17] = {
 //  15    20    25    30    35    40    45    50
    2470, 2345, 2220, 2070, 1925, 1759, 1604, 1430,  
 //  55    60    65    70    75    80    85    90
    1254, 1085,  920,  754,  597,  452,  300,  140, 0
};

void speedDisplay::setExSpeed(int speed, bool force)
{
    #define DC_MIN 1638
    #define DC_MAX 8192
    #define SPD_MAX 95

    if(!_haveSec) return;
    if(!_doSec && !force) return;

    uint8_t oldSpd = _secSpeedOld;

    if(_secOff) speed = -1;

    if((_secSpeedOld == speed) && !force) return;

    _secSpeedOld = speed;

    if(speed > SPD_MAX) speed = SPD_MAX;

    uint32_t dc = 0;
    int angle, temp;

    #define STEPSMAX 1500

    if(_haveSSpeedo) {
        if(speed >= 0) {
            if(speed <= 15) {
                //angle = 2700 - ((2700 - 2437) * speed / 15);
                angle = 2700 - ((2700 - 2470) * speed / 15);
            } else if(speed == SPD_MAX) {
                angle = 0;
            } else {
                temp = (speed - 15) / 5;
                angle = sspdpos[temp];
                angle -= ((angle - sspdpos[temp + 1]) * (speed % 5) / 5);
            }
            angle += spos_corr;
            if(angle < 0) angle = 0;
            else if(angle > 2700) angle = 2700;
            if(!_doSec) {
                Serial.printf("S: Speed %d Raw angle %.1f, corrected angle %.1f\n", speed, (float)(angle - spos_corr)/10.0f, (float)angle/10.0f);
            }
            angle <<= 17; // * 131072;
            dc = (angle / 3375) + (DC_MIN*16);
        } else {
            angle = 2700 + spos_corr;
            if(angle > 2700) angle = 2700;
            angle <<= 17;
            dc = (angle / 3375) + (DC_MIN*16);
        }
        if(force) {
            ledcWrite(0, dc >> 4);
            currentDC = dc;
        }
        targetDC = dc;
        stepDC = (currentDC - targetDC) / 8;
        if(stepDC > STEPSMAX) stepDC = STEPSMAX;
        else if(stepDC < -STEPSMAX) stepDC = -STEPSMAX;
    }

    #define T_IDLE 700    // 70deg = 775 rpm (200deg on servo)
    #define TLIM1 20
    #define T_MXRPM1 1700
    
    #define T_CHGPT2 570  // 70+57deg = 2600rpm
    #define TLIM2 45
    #define T_MXRPM2 (1740 - T_CHGPT2)
    
    #define T_CHGPT3 600  
    #define TLIM3 65
    #define T_MXRPM3 (1670 - T_CHGPT3)
    
    #define T_CHGPT4 620
    #define T_MXRPM4 (2200 - T_CHGPT4)

    #define STEPTMAX 1500
    
    if(_haveSTacho) {
        if(speed >= 0) {
            if(speed > oldSpd) {
                if(speed <= TLIM1) {
                    angle = speed * T_MXRPM1 / TLIM1;
                } else if(speed <= TLIM2) {
                    angle = (speed - TLIM1) * T_MXRPM2 / (TLIM2-TLIM1);
                    angle += T_CHGPT2;
                } else if(speed <= TLIM3) {
                    angle = (speed - TLIM2) * T_MXRPM3 / (TLIM3-TLIM2);
                    angle += T_CHGPT3;
                } else {
                    angle = (speed - TLIM3) * T_MXRPM4 / (SPD_MAX-TLIM3);
                    angle += T_CHGPT4;
                }
                angle += T_IDLE;
            } else {
                angle = T_IDLE;
            }
            angle += tpos_corr;
            if(angle < 0) angle = 0;
            else if(angle > 2700) angle = 2700;
            angle = (2700 - angle) << 17; // * 131072;
            dc = (angle / 3375) + (DC_MIN*16);
            if(!_doSec) {
                Serial.printf("T: Speed %d Raw angle %.1f, corrected angle %.1f\n", speed, (float)(angle - tpos_corr)/10.0, (float)angle/10.0);
            }
        } else {
            angle = 2700 - tpos_corr;
            if(angle > 2700) angle = 2700;
            angle <<= 17;
            dc = (angle / 3375) + (DC_MIN*16);
        }
        if(force) {
            ledcWrite(1, dc >> 4);
            currentTDC = dc;
        }
        targetTDC = dc;
        stepTDC = (currentTDC - targetTDC) / 8;
        if(stepTDC > STEPTMAX) stepTDC = STEPTMAX;
        else if(stepTDC < -STEPTMAX) stepTDC = -STEPTMAX;
    }
}

void speedDisplay::speedoSLoop(bool force)
{
    unsigned long now = millis();
    if(force) {
        if(_haveSSpeedo) {
            if(targetDC != currentDC) {
                currentDC = targetDC;
                ledcWrite(0, currentDC >> 4);
            }
        }
        if(_haveSTacho) {
            if(targetTDC != currentTDC) {
                currentTDC = targetTDC;
                ledcWrite(1, currentTDC >> 4);
            }
        }
    } else if(now - lastSUpdate > 20) {
        lastSUpdate = now;
        if(_haveSSpeedo) {
            if(targetDC < currentDC) {
               currentDC -= stepDC;
               if(currentDC < targetDC) {
                  currentDC = targetDC;
               }
               ledcWrite(0, currentDC >> 4);
            } else if(targetDC > currentDC) {
               currentDC -= stepDC;
               if(targetDC < currentDC) {
                  currentDC = targetDC;
               }
               ledcWrite(0, currentDC >> 4);
            }
        }
        if(_haveSTacho) {
            if(targetTDC < currentTDC) {
               currentTDC -= stepTDC;
               if(currentTDC < targetTDC) {
                  currentTDC = targetTDC;
               }
               ledcWrite(1, currentTDC >> 4);
            } else if(targetTDC > currentTDC) {
               currentTDC -= stepTDC;
               if(targetTDC < currentTDC) {
                  currentTDC = targetTDC;
               }
               ledcWrite(1, currentTDC >> 4);
            }
        }
    }
}
#endif
