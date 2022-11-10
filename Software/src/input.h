/*
 * -------------------------------------------------------------------
 * CircuitSetup.us Time Circuits Display
 * (C) 2022 Thomas Winischhofer (A10001986)
 * https://github.com/realA10001986/Time-Circuits-Display-A10001986
 *
 * Keypad_I2C Class, TCButton Class: I2C-Keypad and Button handling
 *
 * Parts of this are customized minimizations of parts of the Keypad 
 * and the OneButton libraries.
 * Authors of Keypad library: Mark Stanley, Alexander Brevig
 * Author of OneButton library: Matthias Hertel
 * -------------------------------------------------------------------
 * License of original KeyPad library and this program as regards the 
 * Key and Keypad_I2C classes:
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; version
 * 2.1 of the License or later versions.
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see
 * <https://www.gnu.org/licenses/>
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

#ifndef _TCINPUT_H
#define _TCINPUT_H

#include <Wire.h>

#define KI2C_LISTMAX 1    // Max number of keys recognized simultaneously
#define KI2C_MAPSIZE 10   // Number of rows (x 16 columns)

typedef enum {
    TCKS_IDLE,
    TCKS_PRESSED,
    TCKS_HOLD,
    TCKS_RELEASED
} KeyState;

const char NO_KEY = '\0';

/*
 * TCKey class
 */

class TCKey {

    public:

        TCKey();

        char      kchar;
        int       kcode;
        KeyState  kstate;
        bool      stateChanged;
        unsigned long holdTimer;
};

/*
 * Keypad_i2c class
 */

#define makeKeymap(x) ((char*)x)

class Keypad_I2C {

    public:

        Keypad_I2C(char *userKeymap, const uint8_t *row, const uint8_t *col, 
                   uint8_t numRows, uint8_t numCols,
                   int address, TwoWire *awire = &Wire);

        void begin();

        // Setter for custom delay function
        void setCustomDelayFunc(void (*myDelay)(unsigned int));

        void setScanInterval(unsigned int interval);
        void setHoldTime(unsigned int holdTime);

        void addEventListener(void (*listener)(char, KeyState));

        bool scanKeypad();

    private:

        void scanKeys();
        bool updateList();
        void nextKeyState(uint8_t n, bool kstate);
        int  findInList(int keyCode);
        void transitionTo(uint8_t n, KeyState nextState);

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

        unsigned long _startTime = 0;        
        uint16_t      _rowMask;

        uint8_t       _pinState;  // shadow for output pins

        uint16_t      _bitMap[KI2C_MAPSIZE];
        TCKey         _key[KI2C_LISTMAX];

        TwoWire       *_wire;

        // Ptr to custom delay function
        void (*_customDelayFunc)(unsigned int) = NULL;
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
        TCButton(const int pin, const boolean activeLow = true, const bool pullupActive = true);
      
        void setDebounceTicks(const int ticks);
        void setPressTicks(const int ticks);
        void setLongPressTicks(const int ticks);
      
        void attachPress(void (*newFunction)(void));
        void attachLongPressStart(void (*newFunction)(void));
        void attachLongPressStop(void (*newFunction)(void));

        void scan(void);

    private:

        void reset(void);
        void transitionTo(ButtonState nextState);

        void (*_pressFunc)(void) = NULL;
        void (*_longPressStartFunc)(void) = NULL;
        void (*_longPressStopFunc)(void) = NULL;

        int _pin;
        
        unsigned int _debounceTicks = 50;
        unsigned int _pressTicks = 400;
        unsigned int _longPressTicks = 800;
      
        int _buttonPressed;
      
        ButtonState _state     = TCBS_IDLE;
        ButtonState _lastState = TCBS_IDLE;
      
        unsigned long _startTime;
};

#endif
