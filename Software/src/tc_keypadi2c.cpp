/*
||
|| @file Keypad_I2C.cpp
|| @version 3.0tc - Time Circuits Special Edition by A10001986
|| @version 3.0 - multiple WireX support
|| @version 2.0 - PCF8575 support added by Paul Williamson
|| @author G. D. (Joe) Young, ptw
|| @contact "G. D. (Joe) Young" <jyoung@islandnet.com>
||
|| @description
|| | Keypad_I2C provides an interface for using matrix keypads that
|| | are attached with I2C port expanders. It supports multiple keypads,
|| | user selectable pins, and user defined keymaps.
|| #
||
|| @version 2.0 - April 5, 2020
|| | MKRZERO, ESP32 compile error from inheriting TwoWire that was OK with
|| | original ATMEGA boards; possibly because newer processors can have
|| | multiple I2C WireX ports. Consequently, added the ability to specify
|| | an alternate Wire as optional parameter in constructor.
|| #
||
|| @license
|| | This library is free software; you can redistribute it and/or
|| | modify it under the terms of the GNU Lesser General Public
|| | License as published by the Free Software Foundation; version
|| | 2.1 of the License or later versions.
|| |
|| | This library is distributed in the hope that it will be useful,
|| | but WITHOUT ANY WARRANTY; without even the implied warranty of
|| | MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
|| | Lesser General Public License for more details.
|| |
|| | You should have received a copy of the GNU Lesser General Public
|| | License along with this library; if not, see
|| | <https://www.gnu.org/licenses/>
|| #
||
*/

#include "tc_keypadi2c.h"

// Let the user define a keymap - assume the same row/column count as defined in constructor
void Keypad_I2C::begin(char *userKeymap) 
{
    Keypad::begin(userKeymap);
	  port_write(0xffff);			//set to power-up state
	  pinState = pinState_set();
}

// Initialize I2C
void Keypad_I2C::begin(void) 
{
	  port_write(0xffff);			//set to power-up state
	  pinState = pinState_set();
}

void Keypad_I2C::pin_write(byte pinNum, boolean level) 
{
  	word mask = 1 << pinNum;

    // Reset row-counter. pin_write() is called
    // at the beginning and the end of scanKeys()'s
    // column loop, so what follows is reading 
    // the row pins.
    count = 0;
    
    if(level == HIGH) {
      	pinState |= mask;
    } else {
	      pinState &= ~mask;
    }      
   
  	port_write(pinState);
} 


int Keypad_I2C::pin_read(byte pinNum) 
{
  	word mask = 1 << pinNum;
    word pinVal = 1, pinVal2 = 2;

    if(scanKeys) {

        // Special case for keypad's scankeys function to reduce
        // i2c traffic. We only read the value once, and send back
        // the same value rowCnt-1 times afterwards.
        if(count == 0) {
            while(pinVal != pinVal2) {
                _wire->requestFrom((int)i2caddr, (int)i2cwidth);
                pinVal = _wire->read();
                if(i2cwidth > 1) {
                    pinVal |= (_wire->read() << 8);
                }
                delay(5); 
                _wire->requestFrom((int)i2caddr, (int)i2cwidth);
                pinVal2 = _wire->read();
                if(i2cwidth > 1) {
                    pinVal2 |= (_wire->read() << 8);
                }                
            }
            pinValBuf = pinVal;
            count++;
        } else {            
            pinVal = pinValBuf; 
            count++;
            if(count >= rowCnt) count = 0; // next column          
        }        
      
    } else {
  
        while(pinVal != pinVal2) {
      	    _wire->requestFrom((int)i2caddr, (int)i2cwidth);
      	    pinVal = _wire->read();
            if(i2cwidth > 1) {
                pinVal |= _wire->read() << 8;
            } 
            delay(5);
            _wire->requestFrom((int)i2caddr, (int)i2cwidth);
            pinVal2 = _wire->read();
            if(i2cwidth > 1) {
                pinVal2 |= _wire->read() << 8;
            }
        }

    }    
    
  	pinVal &= mask;
  	if(pinVal == mask) {
        return 1;
  	} else {
        return 0;
  	}
}

void Keypad_I2C::port_write( word i2cportval ) 
{
  	_wire->beginTransmission((int)i2caddr);
  	_wire->write(i2cportval & 0x00FF);
  	if(i2cwidth > 1) {
  		  _wire->write(i2cportval >> 8);
  	}
  	_wire->endTransmission();
  	pinState = i2cportval;
} 

word Keypad_I2C::pinState_set() 
{
  	_wire->requestFrom((int)i2caddr, (int)i2cwidth);
    
  	pinState = _wire->read();
  	if(i2cwidth > 1) {
  		  pinState |= _wire->read() << 8;
  	}
   
  	return pinState;
} 

/*
|| @changelog
|| |
|| | 3.0tc 2022-08-11 - A10001986; Read twice to catch ghost key presses,and
|| |                    reduce i2c traffic by buffering row pin status
|| | 3.0  2020-04-06 - Joe Young : multiple WireX param in constructor
|| | 2.0  2013-08-31 - Paul Williamson : Added i2cwidth parameter for PCF8575 support
|| |
|| | 1.0  2012-07-12 - Joe Young : Initial Release
|| #
*/
