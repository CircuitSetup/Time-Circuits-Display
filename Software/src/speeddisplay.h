/*
 * -------------------------------------------------------------------
 * CircuitSetup.us Time Circuits Display
 * (C) 2022-2026 Thomas Winischhofer (A10001986)
 * https://github.com/realA10001986/Time-Circuits-Display
 * https://tcd.out-a-ti.me
 *
 * speedDisplay Class: Speedo Display
 *
 * This is designed for CircuitSetup's Speedo Display and other Holtek
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

#ifndef _speedDisplay_H
#define _speedDisplay_H

// The Speedo is a 3-digit 7-segment display, with the right-most digit 
// covered by Gaffer tape (except in the first scene with the A-Car; the
// speedo has only ever two visible digits otherwise). In parts 1 and 2 of 
// the Series, speed is shown single digit below 10, and with the dot at 
// the center digit lit. In part 3 the dot vanishes, low speeds are shown 
// with a leading zero and within only 4 minutes, the Gaffer tape jumps
// from covering the right-most digit to the left-most, and back.
// CircuitSetup's Speedo matches all criteria, all other supported ones are 
// either 4-digit, and/or use 14-segment tubes, and/or lack the dot.
// For 4-digit displays, there are two entries in this list, one to display
// the speed right-aligned, one for left-aligned.
// Displays with a TM1637 are not supported because this chip is not really
// i2c compatible as it has no slave address, and therefore cannot be part
// of a i2c chain.
//
// The display's i2c slave address is 0x70 (defined in tc_time.h).

#define SP_NUM_TYPES 13  // Number of speedo display types supported (excl "none")

enum dispTypes : uint8_t {
    SP_CIRCSETUP = 0, // Original CircuitSetup speedo
    SP_ADAF_7x4,      // Adafruit 0.56" 4 digits, 7-segment (7x4) (ADA-878)
    SP_ADAF_7x4L,     // " " " (left-aligned)
    SP_ADAF_B7x4,     // Adafruit 1.2" 4 digits, 7-segment (7x4) (ADA-1270)
    SP_ADAF_B7x4L,    // " " " (left-aligned)
    SP_ADAF_14x4,     // Adafruit 0.56" 4 digits, 14-segment (14x4) (ADA-1911)
    SP_ADAF_14x4L,    // " " " (left-aligned)
    SP_GROVE_2DIG14,  // Grove 0.54" Dual Alphanumeric Display
    SP_GROVE_4DIG14,  // Grove 0.54" Quad Alphanumeric Display
    SP_GROVE_4DIG14L, // " " " (left aligned)
    SP_ADAF1911_L,    // Like SP_ADAF_14x4L, but with only left-hand side tube soldered on
    SP_ADAF878L,      // Like SP_ADAF_7x4L, but only left 2 digits soldered on
    SP_BTTFN,         // BTTFN-connected speedo, or "blind-speedo-simulation"
    SP_CIRCSETUP3,    // For future use: CircuitSetup speedo w/ complete 3rd digit, BTTF3-style (left digit covered, 00, no dot)
    SP_NONE = 99
};

// If new displays are added, SP_NUM_TYPES in global.h needs to be adapted.

class speedDisplay {

    public:

        speedDisplay(uint8_t address);
        bool begin(int dispType, int sSpeedoPin = 0, int sTachopin = 0, int sSpeedoCorr = 0, int sTachoCorr = 0);
        void validateSetup();
        bool haveSpeedoDisplay();
        #ifdef TC_HAVETEMP
        bool supportsTemperature();
        #endif
        void on();
        void off();
        bool getOnOff() { return !!_onCache; }

        uint8_t setBrightness(uint8_t level, bool isInitial = false);
        uint8_t setBrightnessDirect(uint8_t level) ;
        uint8_t getBrightness() { return _brightness; }

        void setNightMode(bool mymode)  { _nightmode = mymode; }
        bool getNightMode()             { return _nightmode; }

        void show();

        void setText(const char *text);
        void setSpeed(int speedNum);
        #ifdef TC_HAVETEMP
        void setTemperature(float temp);
        #endif
        #ifdef SERVOSPEEDO
        void setCalib(int calib);
        bool setSCorr(int corr);
        bool setTCorr(int corr);
        void getCorr(int& scorr, int &tcorr);
        void speedoSLoop(bool force = false);
        #endif

        void setDot(bool dot01 = true) { _dot01 = dot01; }
        //void setColon(bool colon)      { _colon = colon; }

        int  getSpeed()   { return _speed; }

        bool getDot()     { return _dot01; }
        //bool getColon()   { return _colon; }

        bool dispL0Spd = true;
        bool thirdDig = false;

    private:

        void clearBuf();

        //void handleColon();
        uint16_t getLEDChar(uint8_t value);
        void clearDisplay();                    // clears display RAM
        void directCmd(uint8_t val);

        #ifdef SERVOSPEEDO
        void setExSpeed(int speed, bool force = false);
        #endif
        
        uint8_t _speedoType;
        bool _i2c;

        #ifdef SERVOSPEEDO
        bool _haveSec = false;
        bool _doSec = false;
        bool _haveSSpeedo = false;
        bool _haveSTacho = false;
        bool _secOff = true;
        int  _secSpeed = -2;
        int  _secSpeedOld = -3;
        int32_t currentDC = 0, targetDC = 0, stepDC = 1;
        int32_t currentTDC = 0, targetTDC = 0, stepTDC = 1;
        unsigned long lastSUpdate = 0;
        int spos_corr = 0;
        int tpos_corr = 0;
        #endif

        uint8_t _address;
        uint16_t _displayBuffer[8];

        int8_t _onCache = -1;                   // Cache for on/off
        uint8_t _briCache = 0xfe;               // Cache for brightness

        bool _dot01 = false;
        //bool _colon = false;

        int  _speed = 0;

        unsigned long _posSpdNow = 0;
        int8_t        _lastPosSpd = 5;

        uint8_t _brightness = 15;
        uint8_t _origBrightness = 15;
        bool    _nightmode = false;
        int     _oldnm = -1;

        uint8_t  _dispType;
        bool     _is7seg;           //      7- or 14-segment-display?
        uint8_t  _speed_pos10;      //      Speed's 10s position in 16bit buffer
        uint8_t  _speed_pos01;      //      Speed's 1s position in 16bit buffer
        uint8_t  _dig10_shift;      //      Shift 10s to align in buffer
        uint8_t  _dig01_shift;      //      Shift 1s to align in buffer
        uint8_t  _dot_pos01;        //      1s dot position in 16bit buffer
        uint8_t  _dot01_shift;      //      1s dot shift to align in buffer
        uint8_t  _colon_pos;        //      Pos of colon in 16bit buffer (255 = no colon)
        uint16_t _colon_bm;         //      bitmask for colon
        unsigned int  _num_digs;    //      total number of digits/letters (max 4)
        unsigned int  _max_buf;     //      highest buffer position
        const uint8_t *_bufPosArr;  //      Array of buffer positions for digits left->right
        const uint8_t *_bufShftArr; //      Array of shift values for each digit

        const uint16_t *_fontXSeg;
};

#endif
