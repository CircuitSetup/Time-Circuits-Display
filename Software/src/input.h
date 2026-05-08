/*
 * -------------------------------------------------------------------
 * CircuitSetup.us Time Circuits Display
 * (C) 2022-2026 Thomas Winischhofer (A10001986)
 * https://github.com/realA10001986/Time-Circuits-Display
 * https://tcd.out-a-ti.me
 *
 * Keypad_I2C Class, TCButton Class: I2C-Keypad and Button handling
 * 
 * TCRotEnc: Rotary Encoder handling:
 * Supports Adafruit 4991/5880, DuPPA I2CEncoder 2.1, DFRobot Gravity 360.
 * For speed, the encoders must be set to their default i2c address
 * (DuPPA I2CEncoder 2.1 must be set to i2c address 0x01 (A0 closed)).
 * For volume, the encoders must be configured as follows:
 * - Ada4991/5880: A0 closed (i2c address 0x37)
 * - DFRobot Gravity 360: SW1 off, SW2 on (i2c address 0x55)
 * - DuPPA I2CEncoder 2.1: A0 and A1 closed (i2c address 0x03)
 *
 * Keypad part inspired by "Keypad" library by M. Stanley & A. Brevig
 * -------------------------------------------------------------------
 * License: Modified MIT NON-AI
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
 * Links inside the Software pointing to the original source must not 
 * be changed or removed.
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
 * 4. The source code and the binary form, and any modifications made 
 * to them may not be used for the purpose of training or improving 
 * machine learning algorithms, including but not limited to artificial
 * intelligence, natural language processing, or data mining. This 
 * condition applies to any derivatives, modifications, or updates 
 * based on the Software code. Any usage of the source code or the 
 * binary form in an AI-training dataset is considered a breach of 
 * this License.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, 
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF 
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. 
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY 
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, 
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE 
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef _TCINPUT_H
#define _TCINPUT_H

#include <Wire.h>

/*
 * Keypad_i2c class
 */

typedef enum {
    TCKS_IDLE,
    TCKS_PRESSED,
    TCKS_HOLD,
    TCKS_RELEASED
} KeyState;


struct KeyStruct {
    unsigned long startTime;
    int       kCode;
    KeyState  kState;
    char      kChar;
    bool      stateChanged;
};

class Keypad_I2C {

    public:

        Keypad_I2C(char *userKeymap, const uint8_t *row, const uint8_t *col, 
                   uint8_t numRows, uint8_t numCols,
                   int address);

        void begin(unsigned int scanInterval, unsigned int holdTime, void (*myDelay)(int, unsigned long));

        void addEventListener(void (*listener)(char, KeyState)) { _keypadEventListener = listener; }

        bool scanKeypad();

    private:

        bool scanKeys();
        void advanceState(bool kstate);
        void transitionTo(KeyState nextState);

        void (*_keypadEventListener)(char, KeyState);

        void pin_write(uint8_t pinNum, bool level);
        void port_write(uint8_t i2cportval);

        unsigned int  _scanInterval;
        unsigned int  _holdTime;
        const uint8_t *_rowPins;
        const uint8_t *_columnPins;
        uint8_t       _rows;
        uint8_t       _columns;
        char          *_keymap;
        int           _i2caddr;

        unsigned long _scanTime = 0;        
        uint16_t      _rowMask;

        uint8_t       _pinState;  // shadow for output pins

        KeyStruct     _key;

        // Ptr to custom delay function
        void (*_customDelayFunc)(int, unsigned long) = NULL;
};

/*
 * TCButton class
 */

typedef enum {
    TCBS_IDLE,
    TCBS_PRESSED,
    TCBS_RELEASED,
    TCBS_LONGPRESS,
    TCBS_LONGPRESSEND
} ButtonState;

class TCButton {
  
    public:
        TCButton(const int pin, const bool activeLow = true, const bool pullupActive = true);
        void begin();
      
        void setTiming(const int debounceDur, const int pressDur, const int lPressDur);

        void attachPressStart(void (*newFunction)(void))     { _pressStartFunc = newFunction; }
        void attachPress(void (*newFunction)(void))          { _pressFunc = newFunction; }
        void attachLongPressStart(void (*newFunction)(void)) { _longPressStartFunc = newFunction; }
        void attachLongPressStop(void (*newFunction)(void))  { _longPressStopFunc = newFunction; }

        void scan();

    private:

        void reset();
        void transitionTo(ButtonState nextState);

        void (*_pressStartFunc)(void) = NULL;
        void (*_pressFunc)(void) = NULL;
        void (*_longPressStartFunc)(void) = NULL;
        void (*_longPressStopFunc)(void) = NULL;

        int _pin;
        bool _pullupActive;
        
        unsigned int _debounceDur = 50;
        unsigned int _pressDur = 400;
        unsigned int _longPressDur = 800;
      
        int _buttonPressed;
      
        ButtonState _state     = TCBS_IDLE;
        ButtonState _lastState = TCBS_IDLE;
      
        unsigned long _startTime;
};

#ifdef TC_HAVE_RE
/*
 * TCRotEnc class
 */

#define TC_RE_TYPE_ADA4991    0     // Adafruit 4991/5880       https://www.adafruit.com/product/4991 https://www.adafruit.com/product/5880
#define TC_RE_TYPE_DUPPAV2    1     // DuPPA I2CEncoder V2.1    https://www.duppa.net/shop/i2cencoder-v2-1/
#define TC_RE_TYPE_DFRGR360   2     // DFRobot Gravity 360      https://www.dfrobot.com/product-2575.html
#define TC_RE_TYPE_CS         3     // CircuitSetup             <yet to be designed>

class TCRotEnc {
  
    public:
        TCRotEnc(int numTypes, const uint8_t addrArr[]);
        bool    begin(bool forSpeed);
        void    zeroPos(int offs = 0);
        void    disabledPos();
        void    speedPos(int speed);
        int     updateFakeSpeed(bool force = false);
        int     updateVolume(int curVol, bool force = false);

        bool    IsOff();

    private:
        int32_t getEncPos();
        int     read(uint16_t base, uint8_t reg, uint8_t *buf, uint8_t num);
        void    write(uint16_t base, uint8_t reg, uint8_t *buf, uint8_t num);

        bool    zeroEnc(int offs = 0);

        int           _numTypes = 0;
        const uint8_t *_addrArr;
        int8_t        _st = -1;
        int8_t        _type = 0;        // 0=speed; 1=vol
        
        int           _i2caddr;

        int           fakeSpeed = 0;
        int           targetSpeed = 0;
        int32_t       rotEncPos = 0;
        unsigned long lastUpd = 0;
        unsigned long lastFUpd = 0;

        int           dfrgain;
        int           dfroffslots;
};
#endif  // TC_HAVE_RE

#endif
