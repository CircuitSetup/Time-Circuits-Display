/*
 * -------------------------------------------------------------------
 * CircuitSetup.us Time Circuits Display
 * (C) 2022-2024 Thomas Winischhofer (A10001986)
 * https://github.com/realA10001986/Time-Circuits-Display
 * https://tcd.out-a-ti.me
 *
 * Keypad_I2C Class, TCButton Class: I2C-Keypad and Button handling
 * 
 * TCRotEnc: Rotary Encoder handling:
 * Supports Adafruit 4991, DuPPA I2CEncoder 2.1, DFRobot Gravity 360.
 * For Speed, the encoders must be set to their default i2c address
 * (DuPPA I2CEncoder 2.1 must be set to i2c address 0x01 (A0 closed)).
 * For Volume, the encoders must be configured as follows:
 * - Ada4991: A0 closed (i2c address 0x37)
 * - DFRobot Gravity 360: SW1 off, SW2 on (i2c address 0x55)
 * - DuPPA I2CEncoder 2.1: A0 and A1 closed (i2c address 0x03)
 * 
 *
 * Keypad part inspired by "Keypad" library by M. Stanley & A. Brevig
 * Fractions of this code are customized, minimized derivates of parts 
 * of the OneButton library by Matthias Hertel.
 * -------------------------------------------------------------------
 * License of Keypad_I2C class: MIT NON-AI
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
 * 4. The source code and the binary form, and any modifications made 
 * to them may not be used for the purpose of training or improving 
 * machine learning algorithms, including but not limited to artificial
 * intelligence, natural language processing, or data mining. This 
 * condition applies to any derivatives, modifications, or updates 
 * based on the Software code. Any usage of the source code or the 
 * binary form in an AI-training dataset is considered a breach of 
 * this License.

 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, 
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF 
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. 
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY 
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, 
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE 
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 * -------------------------------------------------------------------
 * Original OneButton library 
 * Copyright (c) 2005-2014 by Matthias Hertel, http://www.mathertel.de/
 * 
 * License of original OneButton library and this program as regards
 * the TCButton class:
 * Software License Agreement (BSD License)
 * Redistribution and use in source and binary forms, with or without 
 * modification, are permitted provided that the following conditions 
 * are met:
 * - Redistributions of source code must retain the above copyright 
 *   notice, this list of conditions and the following disclaimer. 
 * - Redistributions in binary form must reproduce the above copyright 
 *   notice, this list of conditions and the following disclaimer in 
 *   the documentation and/or other materials provided with the 
 *   distribution. 
 * - Neither the name of the copyright owners nor the names of its 
 *   contributors may be used to endorse or promote products derived 
 *   from this software without specific prior written permission. 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT 
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS 
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE 
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, 
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, 
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; 
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER 
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT 
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN 
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE 
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include "tc_global.h"

#include <Arduino.h>

#include "input.h"

#define OPEN    false
#define CLOSED  true

#define MAX_ROWS  4
#define MAX_COLS  3

static void defaultDelay(unsigned long mydelay)
{
    delay(mydelay);
}

/*
 * Keypad_i2c class
 */

Keypad_I2C::Keypad_I2C(char *userKeymap, const uint8_t *row, const uint8_t *col,
                       uint8_t numRows, uint8_t numCols,
                       int address, TwoWire *awire)
{
    _keymap = userKeymap;

    _rowPins = row;
    _columnPins = col;
    _rows = numRows;
    _columns = numCols;

    // Avoid crash if called with bad parms
    if(_rows > MAX_ROWS) _rows = MAX_ROWS;
    if(_columns > MAX_COLS) _columns = MAX_COLS;

    _i2caddr = address;
    _wire = awire;

    _scanInterval = 10;
    _holdTime = 500;

    _keypadEventListener = NULL;

    _customDelayFunc = defaultDelay;

    _key.kState = TCKS_IDLE;
    _key.kChar = 0;
    _key.kCode = -1;
    _key.stateChanged = false;
}

// Initialize I2C
void Keypad_I2C::begin(unsigned int scanInterval, unsigned int holdTime, void (*myDelay)(unsigned long))
{
    _scanInterval = scanInterval;
    _holdTime = holdTime;
    
    // Set pins to power-up state
    port_write(0xff);

    // Read initial value for shadow output pin state
    _wire->requestFrom(_i2caddr, (int)1);
    _pinState = _wire->read();

    // Build rowMask for quick scanning
    _rowMask = 0;
    for(int i = 0; i < _rows; i++) {
        _rowMask |= (1 << _rowPins[i]);
    }

    _customDelayFunc = myDelay;
}

void Keypad_I2C::addEventListener(void (*listener)(char, KeyState))
{
    _keypadEventListener = listener;
}

// Scan keypad and update key state
bool Keypad_I2C::scanKeypad()
{
    bool keyChanged = false;

    if((millis() - _scanTime) > _scanInterval) {
        keyChanged = scanKeys();
        _scanTime = millis();
    }

    return keyChanged;
}

/*
 * Private
 */

// Hardware scan & update key state
bool Keypad_I2C::scanKeys()
{
    uint16_t pinVals[3][MAX_COLS], rm;
    bool     repeat, haveKey;
    int      maxRetry = 5;
    int      kc;
    uint8_t  c, r, d;

    do {

        repeat = haveKey = false;

        for(d = 0; d < 3; d++) {

            for(c = 0; c < _columns; c++) {

                pin_write(_columnPins[c], LOW);

                _wire->requestFrom(_i2caddr, (int)1);
                if((pinVals[d][c] = _wire->read() & _rowMask) != _rowMask) 
                    haveKey = true;

                pin_write(_columnPins[c], HIGH);

            }

            if(d < 2) (*_customDelayFunc)(5);

        }

        for(c = 0; c < _columns; c++) {
            if((pinVals[0][c] != pinVals[1][c]) || (pinVals[0][c] != pinVals[2][c])) 
                repeat = true;
        }

    } while(maxRetry-- && repeat);

    _key.stateChanged = false;

    // If we currently have an active key, advance its state
    if(_key.kCode >= 0) {

        advanceState(!(pinVals[0][_key.kCode % _columns] & (1 << _rowPins[_key.kCode / _columns])));

    } 

    // If _key is idle, evaluate scanning result
    if(haveKey && _key.kState == TCKS_IDLE) {

        for(r = 0; r < _rows; r++) {

            rm = 1 << _rowPins[r];

            for(c = 0, kc = r * _columns; c < _columns; c++, kc++) {

                bool newstate = !(pinVals[0][c] & rm);

                if(newstate == CLOSED) {

                    // (New) key pressed, handle it
                    _key.kCode = kc;
                    _key.kChar = _keymap[kc];
                    advanceState(newstate);
                    goto quitScan;  // bail, _key is busy

                }
            }
        }
        
    }
quitScan:

    return _key.stateChanged;
}

// State machine. 
// Unlike TCButton, we do not need to debounce.
void Keypad_I2C::advanceState(bool newstate)
{
    switch(_key.kState) {
    case TCKS_IDLE:
        if(newstate == CLOSED) {
            transitionTo(TCKS_PRESSED);
            _key.startTime = millis();
        }
        break;

    case TCKS_PRESSED:
        if((millis() - _key.startTime) > _holdTime)
            transitionTo(TCKS_HOLD);
        else if(newstate == OPEN)
            transitionTo(TCKS_RELEASED);
        break;

    case TCKS_HOLD:
        if(newstate == OPEN)
            transitionTo(TCKS_RELEASED);
        break;

    case TCKS_RELEASED:
        _key.kState = TCKS_IDLE;
        _key.kCode = -1;
        break;
    }
}

void Keypad_I2C::transitionTo(KeyState nextState)
{
    _key.kState = nextState;
    _key.stateChanged = true;

    if(_keypadEventListener) {
        _keypadEventListener(_key.kChar, _key.kState);
    }
}

void Keypad_I2C::pin_write(uint8_t pinNum, bool level)
{
    uint8_t mask = 1 << pinNum;

    if(level == HIGH) {
      	_pinState |= mask;
    } else {
        _pinState &= ~mask;
    }

    port_write(_pinState);
}

void Keypad_I2C::port_write(uint8_t val)
{
    _wire->beginTransmission(_i2caddr);
    _wire->write(val);
    _wire->endTransmission();
    _pinState = val;
}



/*
 * TCButton class
 */

/* pin: The pin to be used
 * activeLow: Set to true when the input level is LOW when the button is pressed, Default is true.
 * pullupActive: Activate the internal pullup when available. Default is true.
 */
TCButton::TCButton(const int pin, const boolean activeLow, const bool pullupActive)
{
    _pin = pin;

    _buttonPressed = activeLow ? LOW : HIGH;
  
    pinMode(pin, pullupActive ? INPUT_PULLUP : INPUT);
}


// debounce: Number of millisec that have to pass by before a click is assumed stable
// press:    Number of millisec that have to pass by before a short press is detected
// lPress:   Number of millisec that have to pass by before a long press is detected
void TCButton::setTiming(const int debounceDur, const int pressDur, const int lPressDur)
{
    _debounceDur = debounceDur;
    _pressDur = pressDur;
    _longPressDur = lPressDur;
}

// Register function for short press event
void TCButton::attachPress(void (*newFunction)(void))
{
    _pressFunc = newFunction;
}

// Register function for long press start event
void TCButton::attachLongPressStart(void (*newFunction)(void))
{
    _longPressStartFunc = newFunction;
}

// Register function for long press stop event
void TCButton::attachLongPressStop(void (*newFunction)(void))
{
    _longPressStopFunc = newFunction;
}

// Check input of the pin and advance the state machine
void TCButton::scan(void)
{
    unsigned long now = millis();
    unsigned long waitTime = now - _startTime;
    bool active = (digitalRead(_pin) == _buttonPressed);
    
    switch(_state) {
    case TCBS_IDLE:
        if(active) {
            transitionTo(TCBS_PRESSED);
            _startTime = now;
        }
        break;

    case TCBS_PRESSED:
        if((!active) && (waitTime < _debounceDur)) {  // de-bounce
            transitionTo(_lastState);
        } else if(!active) {
            transitionTo(TCBS_RELEASED);
            _startTime = now;
        } else if(waitTime > _longPressDur) {
            if(_longPressStartFunc) _longPressStartFunc();
            transitionTo(TCBS_LONGPRESS);
        }
        break;

    case TCBS_RELEASED:
        if((active) && (waitTime < _debounceDur)) {  // de-bounce
            transitionTo(_lastState);
        } else if((!active) && (waitTime > _pressDur)) {
            if(_pressFunc) _pressFunc();
            reset();
        }
        break;
  
    case TCBS_LONGPRESS:
        if(!active) {
            transitionTo(TCBS_LONGPRESSEND);
            _startTime = now;
        }
        break;

    case TCBS_LONGPRESSEND:
        if((active) && (waitTime < _debounceDur)) { // de-bounce
            transitionTo(_lastState);
        } else if(waitTime >= _debounceDur) {
            if(_longPressStopFunc) _longPressStopFunc();
            reset();
        }
        break;

    default:
        transitionTo(TCBS_IDLE);
        break;
    }
}

/*
 * Private
 */

void TCButton::reset(void)
{
    _state = TCBS_IDLE;
    _lastState = TCBS_IDLE;
    _startTime = 0;
}

// Advance to new state
void TCButton::transitionTo(ButtonState nextState)
{
    _lastState = _state;
    _state = nextState;
}


#ifdef TC_HAVE_RE

/*
 * TCRotEnc class
 */

// General

#define HWUPD_DELAY 100   // ms between hw polls for speed
#define FUPD_DELAY  10    // ms between fakeSpeed updates

#define OFF_SLOTS   5

#define HWUPD_DELAY_VOL 150 // ms between hw polls for volume

// Ada4991

#define SEESAW_STATUS_BASE  0x00
#define SEESAW_ENCODER_BASE 0x11

#define SEESAW_STATUS_HW_ID   0x01
#define SEESAW_STATUS_VERSION 0x02
#define SEESAW_STATUS_SWRST   0x7F

#define SEESAW_ENCODER_STATUS   0x00
#define SEESAW_ENCODER_INTENSET 0x10
#define SEESAW_ENCODER_INTENCLR 0x20
#define SEESAW_ENCODER_POSITION 0x30
#define SEESAW_ENCODER_DELTA    0x40

#define SEESAW_HW_ID_CODE_SAMD09   0x55
#define SEESAW_HW_ID_CODE_TINY806  0x84
#define SEESAW_HW_ID_CODE_TINY807  0x85
#define SEESAW_HW_ID_CODE_TINY816  0x86
#define SEESAW_HW_ID_CODE_TINY817  0x87
#define SEESAW_HW_ID_CODE_TINY1616 0x88
#define SEESAW_HW_ID_CODE_TINY1617 0x89

// Duppa V2.1

#define DUV2_BASE     0x100
#define DUV2_CONF     0x00
#define DUV2_CONF2    0x30
#define DUV2_CVALB4   0x08
#define DUV2_CMAXB4   0x0c
#define DUV2_CMINB4   0x10
#define DUV2_ISTEPB4  0x14
#define DUV2_IDCODE   0x70

enum DUV2_CONF_PARAM {
    DU2_FLOAT_DATA   = 0x01,
    DU2_INT_DATA     = 0x00,
    DU2_WRAP_ENABLE  = 0x02,
    DU2_WRAP_DISABLE = 0x00,
    DU2_DIRE_LEFT    = 0x04,
    DU2_DIRE_RIGHT   = 0x00,
    DU2_IPUP_DISABLE = 0x08,
    DU2_IPUP_ENABLE  = 0x00,
    DU2_RMOD_X2      = 0x10,
    DU2_RMOD_X1      = 0x00,
    DU2_RGB_ENCODER  = 0x20,
    DU2_STD_ENCODER  = 0x00,
    DU2_EEPROM_BANK1 = 0x40,
    DU2_EEPROM_BANK2 = 0x00,
    DU2_RESET        = 0x80
};
enum DUV2_CONF2_PARAM {
    DU2_CLK_STRECH_ENABLE  = 0x01,
    DU2_CLK_STRECH_DISABLE = 0x00,
    DU2_REL_MODE_ENABLE    = 0x02,
    DU2_REL_MODE_DISABLE   = 0x00,
};

// DFRobot Gravity 360

#define DFR_BASE      0x100
#define DFR_PID_MSB   0x00
#define DFR_VID_MSB   0x02
#define DFR_COUNT_MSB 0x08
#define DFR_GAIN_REG  0x0b

#define DFR_PID_U     0x01
#define DFR_PID_L     0xf6

#define DFR_GAIN_SPD  (1023 / (88 + OFF_SLOTS))
#define DFR_GAIN_VOL  (1023 / 20)


TCRotEnc::TCRotEnc(int numTypes, uint8_t addrArr[], TwoWire *awire)
{
    _numTypes = min(6, numTypes);

    for(int i = 0; i < _numTypes * 2; i++) {
        _addrArr[i] = addrArr[i];
    }

    _wire = awire;
}

bool TCRotEnc::begin(bool forSpeed)
{
    bool foundSt = false;
    union {
        uint8_t buf[4];
        int32_t ibuf;
    };

    _type = forSpeed ? 0 : 1;

    for(int i = 0; i < _numTypes * 2; i += 2) {

        _i2caddr = _addrArr[i];

        _wire->beginTransmission(_i2caddr);
        if(!_wire->endTransmission(true)) {

            switch(_addrArr[i+1]) {
            case TC_RE_TYPE_ADA4991:
                if(read(SEESAW_STATUS_BASE, SEESAW_STATUS_HW_ID, buf, 1) == 1) {
                    switch(buf[0]) {
                    case SEESAW_HW_ID_CODE_SAMD09:
                    case SEESAW_HW_ID_CODE_TINY806:
                    case SEESAW_HW_ID_CODE_TINY807:
                    case SEESAW_HW_ID_CODE_TINY816:
                    case SEESAW_HW_ID_CODE_TINY817:
                    case SEESAW_HW_ID_CODE_TINY1616:
                    case SEESAW_HW_ID_CODE_TINY1617:
                        //_hwtype = buf[0];
                        // Check for board type
                        read(SEESAW_STATUS_BASE, SEESAW_STATUS_VERSION, buf, 4);   
                        if((((uint16_t)buf[0] << 8) | buf[1]) == 4991) {
                            foundSt = true;
                        }
                    }
                }
                break;

            case TC_RE_TYPE_DUPPAV2:
                read(DUV2_BASE, DUV2_IDCODE, buf, 1);
                if(buf[0] == 0x53) {
                    foundSt = true;
                }
                break;

            case TC_RE_TYPE_DFRGR360:
                read(DFR_BASE, DFR_PID_MSB, buf, 2);
                if(((buf[0] & 0x3f) == DFR_PID_U) &&
                    (buf[1]         == DFR_PID_L)) {
                    foundSt = true;
                }
                break;

            case TC_RE_TYPE_CS:
                // TODO
                break;
            }

            if(foundSt) {
                _st = _addrArr[i+1];
    
                #ifdef TC_DBG
                const char *tpArr[6] = { "ADA4991", "DuPPa V2.1", "DFRobot Gravity 360", "CircuitSetup", "", "" };
                const char *purpose[2] = { "Speed", "Volume" };
                Serial.printf("Rotary encoder: Detected %s, used for %s\n", tpArr[_st], purpose[_type]);
                #endif
    
                break;
            }
        }
    }

    switch(_st) {
      
    case TC_RE_TYPE_ADA4991:
        // Reset
        buf[0] = 0xff;
        write(SEESAW_STATUS_BASE, SEESAW_STATUS_SWRST, &buf[0], 1);
        delay(10);
        // Default values suitable
        break;

    case TC_RE_TYPE_DUPPAV2:
        // Reset
        buf[0] = DU2_RESET;
        write(DUV2_BASE, DUV2_CONF, &buf[0], 1);
        delay(10);
        // Init
        buf[0] = DU2_INT_DATA     | 
                 DU2_WRAP_DISABLE |
                 DU2_DIRE_RIGHT   |
                 DU2_RMOD_X1      |
                 DU2_STD_ENCODER  |
                 DU2_IPUP_DISABLE;
        buf[1] = DU2_CLK_STRECH_DISABLE |
                 DU2_REL_MODE_DISABLE;
        write(DUV2_BASE, DUV2_CONF, &buf[0], 1);
        write(DUV2_BASE, DUV2_CONF2, &buf[1], 1);
        // Reset counter to 0
        ibuf = 0;
        write(DUV2_BASE, DUV2_CVALB4, &buf[0], 4);
        // Step width 1
        buf[3] = 1;
        write(DUV2_BASE, DUV2_ISTEPB4, &buf[0], 4);
        // Set Min/Max
        buf[0] = 0x80; buf[3] = 0x00; // [1], [2] still zero from above
        write(DUV2_BASE, DUV2_CMINB4, &buf[0], 4);
        buf[0] = 0x7f; buf[1] = buf[2] = buf[3] = 0xff;
        write(DUV2_BASE, DUV2_CMAXB4, &buf[0], 4);
        break;

    case TC_RE_TYPE_DFRGR360:
        dfrgain = (!_type) ? DFR_GAIN_SPD : DFR_GAIN_VOL;
        buf[0] = dfrgain;
        write(DFR_BASE, DFR_GAIN_REG, &buf[0], 1);
        dfroffslots = (!_type) ? OFF_SLOTS : 0;
        break;

    case TC_RE_TYPE_CS:
        // TODO
    default:
        return false;
    }

    zeroPos();

    return true;
}

// Init dec into "zero" position (vol and speed)
void TCRotEnc::zeroPos(int offs)
{
    zeroEnc(offs);
    rotEncPos = getEncPos();
    fakeSpeed = targetSpeed = 0;
}

// Init dec into "disabled" position (speed only)
void TCRotEnc::disabledPos()
{
    zeroEnc(-OFF_SLOTS);
    rotEncPos = getEncPos();
    targetSpeed = -OFF_SLOTS;
    fakeSpeed = -1;
}

// Init dec to speed position (speed only)
void TCRotEnc::speedPos(int16_t speed)
{   
    if(speed < 0) {
        disabledPos();
        return;
    }

    zeroEnc(speed);
    rotEncPos = getEncPos();
    
    fakeSpeed = targetSpeed = speed;
}

int16_t TCRotEnc::updateFakeSpeed(bool force)
{
    bool timeout = (millis() - lastUpd > HWUPD_DELAY);
    int32_t updRotPos;
    
    if(force || timeout || (fakeSpeed != targetSpeed)) {

        if(force || timeout) {
            lastUpd = millis();
            
            int32_t t = rotEncPos;
            rotEncPos = updRotPos = getEncPos();

            //Serial.printf("Rot pos %x\n", rotEncPos);

            // If turned in + direction, don't start from <0, but 0: 
            // If we're at -3 (which is displayed as 0), and user 
            // turns +, we don't want -2, but 1.
            // If speed was off (or showing temperature), start 
            // with 0.
            if(rotEncPos > t && targetSpeed < 0) {
                int16_t resetTo = (targetSpeed == -OFF_SLOTS) ? 0 : 1;
                targetSpeed = resetTo;
                if(zeroEnc(resetTo)) {
                    updRotPos = resetTo;
                }
            } else {
                targetSpeed += (rotEncPos - t);
            }
            if(targetSpeed < -OFF_SLOTS) targetSpeed = -OFF_SLOTS;
            else if(targetSpeed > 88)    targetSpeed = 88;

            rotEncPos = updRotPos;
        }
        
        if(fakeSpeed != targetSpeed) {
            if(targetSpeed < 0) {
                fakeSpeed = targetSpeed;
            } else if(force || (millis() - lastFUpd > FUPD_DELAY)) {
                if(fakeSpeed < targetSpeed) fakeSpeed++;
                else fakeSpeed--;
                lastFUpd = millis();
            }
        }

    }

    if(fakeSpeed >= 0)                return fakeSpeed;
    if(fakeSpeed >= -(OFF_SLOTS - 1)) return 0;
    return -1;
}

bool TCRotEnc::IsOff()
{
    return (fakeSpeed < -(OFF_SLOTS - 1));
}

int TCRotEnc::updateVolume(int curVol, bool force)
{
    if(curVol == 255)
        return curVol;
    
    if(force || (millis() - lastUpd > HWUPD_DELAY_VOL)) {

        lastUpd = millis();
        
        int32_t t = rotEncPos;
        rotEncPos = getEncPos();

        curVol += (rotEncPos - t);

        if(curVol < 0)        curVol = 0;
        else if(curVol > 19)  curVol = 19;
    }
    
    return curVol;
}

int32_t TCRotEnc::getEncPos()
{
    uint8_t buf[4];

    switch(_st) {
      
    case TC_RE_TYPE_ADA4991:
        read(SEESAW_ENCODER_BASE, SEESAW_ENCODER_POSITION, buf, 4);
        // Ada4991 reports neg numbers when turned cw, so 
        // negate value
        return -(int32_t)(
                    ((uint32_t)buf[0] << 24) | 
                    ((uint32_t)buf[1] << 16) |
                    ((uint32_t)buf[2] <<  8) | 
                     (uint32_t)buf[3]
                );
    case TC_RE_TYPE_DUPPAV2:
        read(DUV2_BASE, DUV2_CVALB4, buf, 4);
        return (int32_t)(
                    ((uint32_t)buf[0] << 24) | 
                    ((uint32_t)buf[1] << 16) |
                    ((uint32_t)buf[2] <<  8) | 
                     (uint32_t)buf[3]
                );
    case TC_RE_TYPE_DFRGR360:
        read(DFR_BASE, DFR_COUNT_MSB, buf, 2);
        return (int32_t)(((buf[0] << 8) | buf[1]) / dfrgain) - dfroffslots;
        
    case TC_RE_TYPE_CS:
        break;
    }

    return 0;
}

bool TCRotEnc::zeroEnc(int offs)
{
    uint8_t buf[4] = { 0 };
    uint16_t temp;

    // Sets encoder value to "0"-pos+offs. 
    // We don't do this for encoders with a 32bit 
    // range, since turning it that far probably 
    // exceeds the encoder's life span anyway.
    
    switch(_st) {
    case TC_RE_TYPE_DFRGR360:   
        temp = (dfroffslots + offs) * dfrgain;
        buf[0] = temp >> 8;
        buf[1] = temp & 0xff;
        write(DFR_BASE, DFR_COUNT_MSB, &buf[0], 2);
        return true;        
    }
    
    return false;
}

int TCRotEnc::read(uint16_t base, uint8_t reg, uint8_t *buf, uint8_t num)
{
    int i2clen;
    
    _wire->beginTransmission(_i2caddr);
    if(base <= 0xff) _wire->write((uint8_t)base);
    _wire->write(reg);
    _wire->endTransmission(true);
    delay(1);
    i2clen = _wire->requestFrom(_i2caddr, (int)num);
    for(int i = 0; i < i2clen; i++) {
        buf[i] = _wire->read();
    }
    return i2clen;
}

void TCRotEnc::write(uint16_t base, uint8_t reg, uint8_t *buf, uint8_t num)
{
    _wire->beginTransmission(_i2caddr);
    if(base <= 0xff) _wire->write((uint8_t)base);
    _wire->write(reg);
    for(int i = 0; i < num; i++) {
        _wire->write(buf[i]);
    }
    _wire->endTransmission();
}
#endif 
