/*
 * -------------------------------------------------------------------
 * CircuitSetup.us Time Circuits Display
 * (C) 2022 Thomas Winischhofer (A10001986)
 * https://github.com/realA10001986/Time-Circuits-Display-A10001986
 *
 * tempSensor Class: Temperature Sensor handling
 *
 * This is designed for MCP9808-based sensors.
 * -------------------------------------------------------------------
 * License: MIT
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
 */

#include "tc_global.h"

#ifdef TC_HAVETEMP

#include <Arduino.h>
#include <Wire.h>
#include "tempsensor.h"

#define MCP9808_REG_CONFIG        0x01   // MCP9808 config register
#define MCP9808_REG_UPPER_TEMP    0x02   // upper alert boundary
#define MCP9808_REG_LOWER_TEMP    0x03   // lower alert boundary
#define MCP9808_REG_CRIT_TEMP     0x04   // critical temperature
#define MCP9808_REG_AMBIENT_TEMP  0x05   // ambient temperature
#define MCP9808_REG_MANUF_ID      0x06   // manufacturer ID
#define MCP9808_REG_DEVICE_ID     0x07   // device ID
#define MCP9808_REG_RESOLUTION    0x08   // resolution

#define MCP9808_CONFIG_SHUTDOWN   0x0100  // shutdown config
#define MCP9808_CONFIG_CRITLOCKED 0x0080  // critical trip lock
#define MCP9808_CONFIG_WINLOCKED  0x0040  // alarm window lock
#define MCP9808_CONFIG_INTCLR     0x0020  // interrupt clear
#define MCP9808_CONFIG_ALERTSTAT  0x0010  // alert output status
#define MCP9808_CONFIG_ALERTCTRL  0x0008  // alert output control
#define MCP9808_CONFIG_ALERTSEL   0x0004  // alert output select
#define MCP9808_CONFIG_ALERTPOL   0x0002  // alert output polarity
#define MCP9808_CONFIG_ALERTMODE  0x0001  // alert output mode

static void defaultDelay(unsigned int mydelay);

// Mode Resolution SampleTime
//  0    0.5°C       30 ms
//  1    0.25°C      65 ms
//  2    0.125°C     130 ms
//  3    0.0625°C    250 ms
#define TC_TEMP_RES_MCP9808 2
static const uint16_t wakeDelayMCP9808[4] = { 30, 65, 130, 250 };


// Store i2c address
tempSensor::tempSensor(int numTypes, uint8_t addrArr[])
{
    _numTypes = min(2, numTypes);

    for(int i = 0; i < _numTypes * 2; i++) {
        _addrArr[i] = addrArr[i];
    }

    _address = addrArr[0];
    _st = addrArr[1];
}

// Start the display
bool tempSensor::begin()
{
    bool foundSt = false;
    _customDelayFunc = defaultDelay;

    for(int i = 0; i < _numTypes * 2; i += 2) {

        _address = _addrArr[i];
        _st = _addrArr[i+1];
        
        switch(_st) {
        case MCP9808:
            if( (read16(MCP9808_REG_MANUF_ID)  == 0x0054) && 
                (read16(MCP9808_REG_DEVICE_ID) == 0x0400) ) {
                foundSt = true;
                #ifdef TC_DBG
                Serial.println("tempSensor: Detected MCP9808 temperature sensor");
                #endif
            }
            break;
        /*...*/
        }

        if(foundSt) break;
    }

    switch(_st) {
    case MCP9808:
        // Write config register
        write16(MCP9808_REG_CONFIG, 0x0);

        // Set resolution
        write8(MCP9808_REG_RESOLUTION, TC_TEMP_RES_MCP9808 & 0x03);
        break;
    default:
        return false;
    }

    on();

    return true;
}

void tempSensor::setCustomDelayFunc(void (*myDelay)(unsigned int))
{
    _customDelayFunc = myDelay;
}

// Turn on the sensor
void tempSensor::on()
{
    onoff(false);
}

// Turn off the sensor
void tempSensor::off()
{
    onoff(true);
}

// Read temperature
double tempSensor::readTemp(bool celsius)
{
    double temp = NAN;

    switch(_st) {
    case MCP9808:
        uint16_t t = read16(MCP9808_REG_AMBIENT_TEMP);

        if(t != 0xffff) {
            temp = ((double)(t & 0x0fff)) / 16.0;
            if(t & 0x1000) temp = 256.0 - temp;
            if(!celsius) temp = temp * 9.0 / 5.0 + 32.0;
        }
        break;
    }

    #ifdef TC_DBG
    Serial.print("Read temp: ");
    Serial.println(temp);
    #endif

    return temp;
}

// Private functions ###########################################################

void tempSensor::onoff(bool shutDown)
{
    uint16_t mydelay = 0;
    uint16_t temp;
    
    switch(_st) {
    case MCP9808:
        temp = read16(MCP9808_REG_CONFIG);
    
        if(shutDown) {
            temp |= MCP9808_CONFIG_SHUTDOWN;
        } else {
            temp &= ~MCP9808_CONFIG_SHUTDOWN;
        }
    
        temp &= ~(MCP9808_CONFIG_CRITLOCKED|MCP9808_CONFIG_WINLOCKED);
    
        write16(MCP9808_REG_CONFIG, temp);
        
        mydelay = wakeDelayMCP9808[TC_TEMP_RES_MCP9808];
        break;
    default:
        return;
    }

    if(!shutDown && mydelay) {
        (*_customDelayFunc)(mydelay);
    }
}

uint16_t tempSensor::read16(uint16_t regno)
{
    uint16_t value = 0;

    Wire.beginTransmission(_address);
    Wire.write((uint8_t)(regno & 0x0f));
    Wire.endTransmission(false);

    Wire.requestFrom(_address, (uint8_t)2);

    value = Wire.read() << 8;
    value |= Wire.read();

    return value;
}

uint8_t tempSensor::read8(uint16_t regno)
{
    uint16_t value = 0;

    Wire.beginTransmission(_address);
    Wire.write((uint8_t)(regno & 0x0f));
    Wire.endTransmission(false);

    Wire.requestFrom(_address, (uint8_t)1);

    value = Wire.read();

    return value;
}

void tempSensor::write16(uint16_t regno, uint16_t value)
{
    Wire.beginTransmission(_address);
    Wire.write((uint8_t)(regno & 0x0f));
    Wire.write((uint8_t)(value >> 8));
    Wire.write((uint8_t)(value & 0xff));
    Wire.endTransmission();
}

void tempSensor::write8(uint16_t regno, uint8_t value)
{
    Wire.beginTransmission(_address);
    Wire.write((uint8_t)(regno & 0x0f));
    Wire.write((uint8_t)(value & 0xff));
    Wire.endTransmission();
}

static void defaultDelay(unsigned int mydelay)
{
    delay(mydelay);
}
#endif
