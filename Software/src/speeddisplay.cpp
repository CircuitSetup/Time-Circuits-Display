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

#include "tc_global.h"

#ifdef TC_HAVESPEEDO

#include "speeddisplay.h"

#ifdef IS_SPEED_7SEG
const uint16_t font7Seg[36] = { 
    S7_T|S7_TR|S7_BR|S7_B|S7_BL|S7_TL,
    S7_TR|S7_BR,
    S7_T|S7_TR|S7_B|S7_BL|S7_M,
    S7_T|S7_TR|S7_BR|S7_B|S7_M,
    S7_TR|S7_BR|S7_TL|S7_M,
    S7_T|S7_BR|S7_B|S7_TL|S7_M,
    S7_BR|S7_B|S7_BL|S7_TL|S7_M,
    S7_T|S7_TR|S7_BR,
    S7_T|S7_TR|S7_BR|S7_B|S7_BL|S7_TL|S7_M,
    S7_T|S7_TR|S7_BR|S7_TL|S7_M,
    S7_T|S7_TR|S7_BR|S7_BL|S7_TL|S7_M,
    S7_BR|S7_B|S7_BL|S7_TL|S7_M,
    S7_T|S7_B|S7_BL|S7_TL,
    S7_TR|S7_BR|S7_B|S7_BL|S7_M,
    S7_T|S7_B|S7_BL|S7_TL|S7_M,
    S7_T|S7_BL|S7_TL|S7_M,
    S7_T|S7_BR|S7_B|S7_BL|S7_TL,
    S7_TR|S7_BR|S7_BL|S7_TL|S7_M,
    S7_BL|S7_TL,
    S7_TR|S7_BR|S7_B|S7_BL,
    S7_T|S7_BR|S7_BL|S7_TL|S7_M,
    S7_B|S7_BL|S7_TL,
    S7_T|S7_BR|S7_BL,
    S7_T|S7_TR|S7_BR|S7_BL|S7_TL,
    S7_T|S7_TR|S7_BR|S7_B|S7_BL|S7_TL,
    S7_T|S7_TR|S7_BL|S7_TL|S7_M,
    S7_T|S7_TR|S7_B|S7_TL|S7_M,
    S7_T|S7_TR|S7_BL|S7_TL,
    S7_T|S7_BR|S7_B|S7_TL|S7_M,
    S7_B|S7_BL|S7_TL|S7_M,
    S7_TR|S7_BR|S7_B|S7_BL|S7_TL,
    S7_TR|S7_BR|S7_B|S7_BL|S7_TL,
    S7_TR|S7_B|S7_TL,
    S7_TR|S7_BR|S7_BL|S7_TL|S7_M,
    S7_TR|S7_BR|S7_B|S7_TL|S7_M,
    S7_T|S7_TR|S7_B|S7_BL|S7_M
};
#endif

#ifndef IS_SPEED_7SEG
const uint16_t font14Seg[36] = { 
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
    S14_T|S14_TRD|S14_B|S14_BLD
};    
#endif

// Store i2c address
speedDisplay::speedDisplay(uint8_t address) 
{    
    _address = address;
}

// Start the display
void speedDisplay::begin() 
{
    Wire.beginTransmission(_address);
    Wire.write(0x20 | 1);  // turn on oscillator
    Wire.endTransmission();

    clear();            // clear buffer
    setBrightness(15);  // setup initial brightness
    clearDisplay();     // clear display RAM
    on();               // turn it on
}

// Turn on the display
void speedDisplay::on() 
{
    Wire.beginTransmission(_address);
    Wire.write(0x80 | 1);  
    Wire.endTransmission();
}

// Turn off the display
void speedDisplay::off() 
{
    Wire.beginTransmission(_address);
    Wire.write(0x80);  
    Wire.endTransmission();
}

// Turn on all LEDs
void speedDisplay::lampTest() 
{  
    Wire.beginTransmission(_address);
    Wire.write(0x00);  // start at address 0x0

    for(int i = 0; i < SD_BUF_SIZE*2; i++) {
        Wire.write(0xFF);
    }
    Wire.endTransmission();
}

// Clear the buffer
void speedDisplay::clear() 
{
    // must call show() to actually clear display
    
    for(int i = 0; i < SD_BUF_SIZE; i++) {
        _displayBuffer[i] = 0;
    }
}

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

    Wire.beginTransmission(_address);
    Wire.write(0xE0 | level);  // Dimming command
    Wire.endTransmission();

    return level;
}

uint8_t speedDisplay::getBrightness() 
{
    return _brightness;
}

void speedDisplay::setNightMode(bool mymode)
{
    _nightmode = mymode ? true : false;
}

bool speedDisplay::getNightMode(void)
{
    return _nightmode;
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

    Wire.beginTransmission(_address);
    Wire.write(0x00);  // start at address 0x0

    for(i = 0; i < SD_BUF_SIZE; i++) {
        Wire.write(_displayBuffer[i] & 0xFF);
        Wire.write(_displayBuffer[i] >> 8);
    }
    
    Wire.endTransmission();
}


// Set fields in buffer --------------------------------------------------------


// Place LED pattern in speed position in buffer
void speedDisplay::setSpeed(uint8_t speedNum) 
{   
    if(speedNum > 99) {                
        Serial.print(F("speedDisplay: setSpeed: Bad speed: "));
        Serial.println(speedNum, DEC);
        speedNum = 99;
    }

    _speed = speedNum;

    _displayBuffer[SD_SPEED_POS01] = 0;
    _displayBuffer[SD_SPEED_POS10] = 0;

#ifdef IS_SPEED_7SEG    
    _displayBuffer[SD_SPEED_POS10] |= (font7Seg[speedNum / 10] << SD_DIG10_SHIFT); 
    _displayBuffer[SD_SPEED_POS01] |= (font7Seg[speedNum % 10] << SD_DIG01_SHIFT);    
    if(_dot10) _displayBuffer[SD_DOT10_POS] |= (S7_DOT << SD_DOT10_SHIFT);
    else       _displayBuffer[SD_DOT10_POS] &= (~(S7_DOT << SD_DOT10_SHIFT));
    if(_dot01) _displayBuffer[SD_DOT01_POS] |= (S7_DOT << SD_DOT01_SHIFT);
    else       _displayBuffer[SD_DOT01_POS] &= (~(S7_DOT << SD_DOT01_SHIFT));
#else    
    _displayBuffer[SD_SPEED_POS10] |= (font14Seg[speedNum / 10]);
    _displayBuffer[SD_SPEED_POS01] |= (font14Seg[speedNum % 10]);
    if(_dot10) _displayBuffer[SD_DOT10_POS] |= S14_DOT;
    else       _displayBuffer[SD_DOT10_POS] &= (~S14_DOT);
    if(_dot01) _displayBuffer[SD_DOT01_POS] |= S14_DOT;
    else       _displayBuffer[SD_DOT01_POS] &= (~S14_DOT);
#endif
}

void speedDisplay::setDots(bool dot10, bool dot01)
{
    _dot10 = dot10;
    _dot01 = dot01;
}

// Query data ------------------------------------------------------------------


uint8_t speedDisplay::getSpeed() 
{
    return _speed;
}

// Special purpose -------------------------------------------------------------


// clears the display RAM and only shows the given text
void speedDisplay::showOnlyText(const char *text)
{
    int idx = 0, pos = 0;
    int temp = 0;
    SD_BUF_ARR;
    
    clearDisplay();
    
#ifdef IS_SPEED_7SEG
    while(text[idx] && (pos < (SD_NUM_DIGS / (1<<SD_BUF_PACKED)))) {
        temp = getLED7Char(text[idx]) << SD_DIG10_SHIFT;
        idx++;
        if(SD_BUF_PACKED && text[idx]) {
            temp |= (getLED7Char(text[idx]) << SD_DIG01_SHIFT);
            idx++;            
        }
        directCol(bufPosArr[pos], temp);
        pos++;
    }
#else
    while(text[idx] && pos < SD_NUM_DIGS) {
        directCol(bufPosArr[pos], getLED14Char(text[idx]));
        idx++;
        pos++;
    }
#endif
}

// Private functions ###########################################################


// Returns bit pattern for provided character for display on 7 segment display
#ifdef IS_SPEED_7SEG
uint8_t speedDisplay::getLED7Char(uint8_t value) 
{    
    if(value >= '0' && value <= '9') {
        return font7Seg[value - '0'];        
    } else if(value >= 'A' && value <= 'Z') {
        return font7Seg[value - 'A' + 10];
    } else if(value >= 'a' && value <= 'z') {
        return font7Seg[value - 'a' + 10];
    }

    return 0;
}
#endif

// Returns bit pattern for provided character for display on alphanumeric 14 segment display
#ifndef IS_SPEED_7SEG
uint16_t speedDisplay::getLED14Char(uint8_t value) 
{
    if(value >= '0' && value <= '9') 
        return font14Seg[value - '0'];
    if(value >= 'A' && value <= 'Z') 
        return font14Seg[value - 'A' + 10];
    if(value >= 'a' && value <= 'z') 
        return font14Seg[value - 'a' + 10];
        
    return 0;    
}
#endif


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

// Directly clear the display 
void speedDisplay::clearDisplay() 
{    
    Wire.beginTransmission(_address);
    Wire.write(0x00);  // start at address 0x0

    for(int i = 0; i < SD_BUF_SIZE*2; i++) {
        Wire.write(0x0);
    }
    
    Wire.endTransmission();
}

#endif
