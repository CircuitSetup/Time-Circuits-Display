/*
 * -------------------------------------------------------------------
 * CircuitSetup.us Time Circuits Display
 * (C) 2022-2023 Thomas Winischhofer (A10001986)
 * https://github.com/realA10001986/Time-Circuits-Display
 * https://tcd.backtothefutu.re
 *
 * Keypad_I2C Class, TCButton Class: I2C-Keypad and Button handling
 *
 * Keypad part inspired by "Keypad" library by M. Stanley & A. Brevig
 * Fractions of this code are customized, minimized derivates of parts 
 * of OneButton library by Matthias Hertel.
 * -------------------------------------------------------------------
 * License of Keypad_I2C class: MIT
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

static void defaultDelay(unsigned int mydelay)
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

    setScanInterval(10);
    setHoldTime(500);

    _keypadEventListener = NULL;

    _customDelayFunc = defaultDelay;

    _key.kState = TCKS_IDLE;
    _key.kChar = 0;
    _key.kCode = -1;
    _key.stateChanged = false;
}

// Initialize I2C
void Keypad_I2C::begin()
{
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
}

void Keypad_I2C::setScanInterval(unsigned int interval)
{
    _scanInterval = (interval < 1) ? 1 : interval;
}

void Keypad_I2C::setHoldTime(unsigned int hold)
{
    _holdTime = hold;
}

void Keypad_I2C::addEventListener(void (*listener)(char, KeyState))
{
    _keypadEventListener = listener;
}

void Keypad_I2C::setCustomDelayFunc(void (*myDelay)(unsigned int))
{
    _customDelayFunc = myDelay;
}

// Scan keypad and update key state
bool Keypad_I2C::scanKeypad()
{
    bool keyChanged = false;
    unsigned long now = millis();

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


// Number of millisec that have to pass by before a click is assumed stable.
void TCButton::setDebounceTicks(const int ticks)
{
    _debounceTicks = ticks;
}


// Number of millisec that have to pass by before a short press is detected.
void TCButton::setPressTicks(const int ticks)
{
    _pressTicks = ticks;
}


// Number of millisec that have to pass by before a long press is detected.
void TCButton::setLongPressTicks(const int ticks)
{
    _longPressTicks = ticks;
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
        if((!active) && (waitTime < _debounceTicks)) {  // de-bounce
            transitionTo(_lastState);
        } else if(!active) {
            transitionTo(TCBS_RELEASED);
            _startTime = now;
        } else if((active) && (waitTime > _longPressTicks)) {
            if(_longPressStartFunc) _longPressStartFunc();
            transitionTo(TCBS_LONGPRESS);
        }
        break;

    case TCBS_RELEASED:
        if((active) && (waitTime < _debounceTicks)) {  // de-bounce
            transitionTo(_lastState);
        } else if((!active) && (waitTime > _pressTicks)) {
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
        if((active) && (waitTime < _debounceTicks)) { // de-bounce
            transitionTo(_lastState);
        } else if(waitTime >= _debounceTicks) {
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

TCRotEnc::TCRotEnc(int numTypes, uint8_t addrArr[], TwoWire *awire)
{
    _numTypes = min(4, numTypes);

    for(int i = 0; i < _numTypes * 2; i++) {
        _addrArr[i] = addrArr[i];
    }

    _wire = awire;
}

bool TCRotEnc::begin()
{
    bool foundSt = false;
    uint8_t buf[4];

    for(int i = 0; i < _numTypes * 2; i += 2) {

        _i2caddr = _addrArr[i];

        switch(_addrArr[i+1]) {
        case TC_RE_TYPE_ADA4991:
            _wire->beginTransmission(_i2caddr);
            if(!_wire->endTransmission(true))
                foundSt = true;
            break;
        }

        if(foundSt) {
            _st = _addrArr[i+1];

            #ifdef TC_DBG
            const char *tpArr[4] = { "ADA4991", "unknown", "unknown", "unknown" };
            Serial.printf("Rotary encoder: Detected %s\n", tpArr[_st]);
            #endif

            break;
        }
    }

    switch(_st) {
      
    case TC_RE_TYPE_ADA4991:
        reset();
        delay(10);
    
        if(read(SEESAW_STATUS_BASE, SEESAW_STATUS_HW_ID, buf, 1) != 1) {
            return false;
        }
    
        switch(buf[0]) {
        case SEESAW_HW_ID_CODE_SAMD09:
        case SEESAW_HW_ID_CODE_TINY806:
        case SEESAW_HW_ID_CODE_TINY807:
        case SEESAW_HW_ID_CODE_TINY816:
        case SEESAW_HW_ID_CODE_TINY817:
        case SEESAW_HW_ID_CODE_TINY1616:
        case SEESAW_HW_ID_CODE_TINY1617:
            _hwtype = buf[0];
            break;
        default:
            return false;
        }
    
        // Check for board type
        read(SEESAW_STATUS_BASE, SEESAW_STATUS_VERSION, buf, 4);   
        if( (((uint32_t)buf[0] <<  8) | 
              (uint32_t)buf[1]) != 4991) {
            return false;
        }

        break;

    default:
        return false;
    }

    zeroPos();

    return true;
}

void TCRotEnc::zeroPos()
{
    rotEncPos = getEncPos();
    fakeSpeed = targetSpeed = 0;
}

#define HWUPD_DELAY 100   // ms between hw polls
#define FUPD_DELAY  20    // ms between fakeSpeed updates

#define OFF_SLOTS   5

int16_t TCRotEnc::updateFakeSpeed(bool force)
{
    bool timeout = (millis() - lastUpd > HWUPD_DELAY);
    
    if(force || timeout || (fakeSpeed != targetSpeed)) {

        if(force || timeout) {
            lastUpd = millis();
            
            int32_t t = rotEncPos;
            rotEncPos = getEncPos();

            //Serial.printf("Rot pos %x\n", rotEncPos);

            // If turned in + direction, don't start from 
            // <0, but 0: If we're at -3 (which is displayed
            // as 0), and user turns +, we don't want -2, but 1.
            if(t - rotEncPos > 0) {
                if(targetSpeed < 0) targetSpeed = 0;
            }
            targetSpeed += (t - rotEncPos);
            if(targetSpeed < -OFF_SLOTS) targetSpeed = -OFF_SLOTS;
            else if(targetSpeed > 88)    targetSpeed = 88;
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

int32_t TCRotEnc::getEncPos()
{
    uint8_t buf[4];

    switch(_st) {
      
    case TC_RE_TYPE_ADA4991:
        read(SEESAW_ENCODER_BASE, SEESAW_ENCODER_POSITION, buf, 4);
        return (int32_t)(
                    ((uint32_t)buf[0] << 24) | 
                    ((uint32_t)buf[1] << 16) |
                    ((uint32_t)buf[2] <<  8) | 
                     (uint32_t)buf[3]
                );
    }

    return 0;
}

void TCRotEnc::reset()
{
    uint8_t buf = 0xff;
    
    write(SEESAW_STATUS_BASE, SEESAW_STATUS_SWRST, &buf, 1);
}

int TCRotEnc::read(uint8_t base, uint8_t reg, uint8_t *buf, uint8_t num)
{
    int i2clen;
    
    _wire->beginTransmission(_i2caddr);
    _wire->write(base);
    _wire->write(reg);
    _wire->endTransmission(true);
    delay(1);
    i2clen = _wire->requestFrom(_i2caddr, (int)num);
    for(int i = 0; i < i2clen; i++) {
        buf[i] = _wire->read();
    }
    return i2clen;
}

void TCRotEnc::write(uint8_t base, uint8_t reg, uint8_t *buf, uint8_t num)
{
    _wire->beginTransmission(_i2caddr);
    _wire->write(base);
    _wire->write(reg);
    for(int i = 0; i < num; i++) {
        _wire->write(buf[i]);
    }
    _wire->endTransmission();
}
#endif 
