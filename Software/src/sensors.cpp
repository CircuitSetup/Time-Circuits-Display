/*
 * -------------------------------------------------------------------
 * CircuitSetup.us Time Circuits Display
 * (C) 2022 Thomas Winischhofer (A10001986)
 * https://github.com/realA10001986/Time-Circuits-Display-A10001986
 *
 * Sensor Class: Temperature and Light Sensor handling
 *
 * This is designed for 
 * - MCP9808-based temperature sensors,
 * - BH1750, TSL2561 and VEML7700/VEML6030 light sensors.
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

#if defined(TC_HAVETEMP) || defined(TC_HAVELIGHT)

#include <Arduino.h>
#include <Wire.h>
#include "sensors.h"

static void defaultDelay(unsigned int mydelay)
{
    delay(mydelay);
}

uint16_t tcSensor::read16(uint16_t regno, bool LSBfirst)
{
    uint16_t value = 0;
    size_t i2clen = 0;

    if(regno <= 0xff) {
        Wire.beginTransmission(_address);
        Wire.write((uint8_t)(regno));
        Wire.endTransmission(false);
    }

    i2clen = Wire.requestFrom(_address, (uint8_t)2);

    if(i2clen > 0) {
        value = Wire.read() << 8;
        value |= Wire.read();
    }
    
    if(LSBfirst) {
        value = (value >> 8) | (value << 8);
    }

    return value;
}

uint8_t tcSensor::read8(uint16_t regno)
{
    uint16_t value = 0;

    Wire.beginTransmission(_address);
    Wire.write((uint8_t)(regno));
    Wire.endTransmission(false);

    Wire.requestFrom(_address, (uint8_t)1);

    value = Wire.read();

    return value;
}

void tcSensor::write16(uint16_t regno, uint16_t value, bool LSBfirst)
{
    Wire.beginTransmission(_address);
    Wire.write((uint8_t)(regno));
    if(LSBfirst) {
        value = (value >> 8) | (value << 8);
    } 
    Wire.write((uint8_t)(value >> 8));
    Wire.write((uint8_t)(value & 0xff));
    Wire.endTransmission();
}

void tcSensor::write8(uint16_t regno, uint8_t value)
{
    Wire.beginTransmission(_address);
    if(regno <= 0xff) {
        Wire.write((uint8_t)(regno));
    }
    Wire.write((uint8_t)(value & 0xff));
    Wire.endTransmission();
}
#endif


/*****************************************************************
 * tempSensor Class
 * 
 * Supports MCP9808-based temperature sensors
 * 
 ****************************************************************/

#ifdef TC_HAVETEMP

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
    _numTypes = min(4, numTypes);

    for(int i = 0; i < _numTypes * 2; i++) {
        _addrArr[i] = addrArr[i];
    }
}

// Start the display
bool tempSensor::begin()
{
    bool foundSt = false;
    
    _customDelayFunc = defaultDelay;

    for(int i = 0; i < _numTypes * 2; i += 2) {

        _address = _addrArr[i];
        
        switch(_addrArr[i+1]) {
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

        if(foundSt) {
            _st = _addrArr[i+1];
            break;
        }
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


#endif // TC_HAVETEMP


/*****************************************************************
 * lightSensor Class
 * 
 * Supports TSL2561, BH1750, VEML7700 light sensors
 * 
 * Sensors are set for indoor conditions and might overflow (in 
 * which case -1 is returned) or report bad lux values outdoors.
 * For TSL2561 and BH1750, their settings can be changed by 
 * #defines below. VEML7700 auto-adjusts.
 * 
 ****************************************************************/

#ifdef TC_HAVELIGHT

#define TSL2561_CTRL    0x80
#define TSL2561_TIM     0x81
#define TSL2561_ICTRL   0x86
#define TSL2561_ID      0x8a
#define TSL2561_ADC0    0x8c    // LSBFirst
#define TSL2561_ADC1    0x8e    // LSBFirst

#define TLS_GAIN_LOW    0x00
#define TLS_GAIN_HIGH   0x10

#define TLS_IT_13       0x00
#define TLS_IT_101      0x01
#define TLS_IT_402      0x02

#define TLS_USE_GAIN    TLS_GAIN_LOW    // to be adjusted (TLS_GAIN_xxx)
#define TLS_USE_IT      TLS_IT_101      // to be adjusted (TLS_IT_xxx)

#define BH1750_DUMMY      0x100

#define BH1750_CONT_HRES  0x10  // 1 lux resolution / 120ms
#define BH1750_CONT_HRES2 0x11  // 0.5 lux resolution / 120ms
#define BH1750_CONT_LRES  0x13  // 4 lux resolution / 16ms

#define BH1750_CONV_FACT  1.2f

static const uint8_t bh1750Arr[3][2] = {
      { BH1750_CONT_HRES,   120+30 }, 
      { BH1750_CONT_HRES2,  120+30 }, 
      { BH1750_CONT_HRES,    16+30 }
};

#define BH1750_USE_SET  0   // to be adjusted (0-2, index in array above)

#define VEML7700_CON0   0
#define VEML7700_HTW1   1
#define VEML7700_HTW2   2
#define VEML7700_PSM    3
#define VEML7700_ALS    4
#define VEML7700_WCH    5
#define VEML7700_IS     6

#define VEML7700_G1     0x00
#define VEML7700_G2     0x01
#define VEML7700_G18    0x02
#define VEML7700_G14    0x03

#define VEML7700_AIT25  0x0c
#define VEML7700_AIT50  0x08
#define VEML7700_AIT100 0x00
#define VEML7700_AIT200 0x01
#define VEML7700_AIT400 0x02
#define VEML7700_AIT800 0x03

static const uint8_t gainArr[4] = { 
    VEML7700_G18,
    VEML7700_G14,
    VEML7700_G1,
    VEML7700_G2
};

static const uint8_t AITArr[6][2] = { 
    { VEML7700_AIT25,    7 },
    { VEML7700_AIT50,   13 },
    { VEML7700_AIT100,  25 },
    { VEML7700_AIT200,  50 },
    { VEML7700_AIT400, 100 },
    { VEML7700_AIT800, 200 }
};

static const float resFact[6][4] = {
    { 1.8432, 0.9216, 0.2304, 0.1152 },
    { 0.9216, 0.4608, 0.1152, 0.0576 },
    { 0.4608, 0.2304, 0.0576, 0.0288 },
    { 0.2304, 0.1152, 0.0288, 0.0144 },
    { 0.1152, 0.0576, 0.0144, 0.0072 },
    { 0.0576, 0.0288, 0.0072, 0.0036 }    
};

// Store i2c address
lightSensor::lightSensor(int numTypes, uint8_t addrArr[])
{
    _numTypes = min(6, numTypes);

    for(int i = 0; i < _numTypes * 2; i++) {
        _addrArr[i] = addrArr[i];
    }
}

bool lightSensor::begin(bool skipLast)
{
    bool foundSt = false;

    _customDelayFunc = defaultDelay;

    // VEML7700 shares i2c address with GPS, so
    // skip it if GPS is connected
    if(skipLast) _numTypes--;
    
    for(int i = 0; i < _numTypes * 2; i += 2) {

        _address = _addrArr[i];

        switch(_addrArr[i+1]) {
        case LST_TSL2561:
            Wire.beginTransmission(_address);
            if(!Wire.endTransmission(true)) {
                if((read8(TSL2561_ID) & 0xf0) == 0x50) {
                    foundSt = true;
                }
            }
            break;
        case LST_BH1750:
            Wire.beginTransmission(_address);
            if(!Wire.endTransmission(true)) {
                foundSt = true;
            }
            break;
        case LST_VEML7700:
            Wire.beginTransmission(_address);
            if(!Wire.endTransmission(true)) {
                foundSt = true;
            }
        }

        if(foundSt) {
            _st = _addrArr[i+1];
            
            #ifdef TC_DBG
            const char *tpArr[3] = { "TSL2561", "BH1750", "VEML7700/6030" };
            Serial.print("Sensors: Detected light sensor ");
            Serial.println(tpArr[_st]);
            #endif
            
            break;
        }
    }

    switch(_st) {
    case LST_TSL2561:
        write8(TSL2561_CTRL, 0x03); // Power up
        write8(TSL2561_ICTRL, 0);   // no interrupts
        write8(TSL2561_TIM, TLS_USE_GAIN | TLS_USE_IT);
        break;
        
    case LST_BH1750:
        write8(BH1750_DUMMY, 0x01); // Power up
        _gainIdx = 0;               // Use High res mode (cont.)
        write8(BH1750_DUMMY, bh1750Arr[_gainIdx][0]);
        break;
        
    case LST_VEML7700:
        _accessNum = 0;
        _con0 = 0;
        _gainIdx = 0;
        _AITIdx = 2;
    
        VEML7700OnOff(false);
        VEML7700SetGain(gainArr[_gainIdx], false);
        VEML7700SetAIT(AITArr[_AITIdx][0], false);
        VEML7700OnOff(true);
        break;

    default:
        return false;
    }

    _lastAccess = millis();

    return true;
}

void lightSensor::setCustomDelayFunc(void (*myDelay)(unsigned int))
{
    _customDelayFunc = myDelay;
}

int32_t lightSensor::readLux()
{
    return _lux;
}

#ifdef TC_DBG
int32_t lightSensor::readDebug()
{
    return (_lux << 16) | (_AITIdx << 4) | _gainIdx;
}
#endif

void lightSensor::loop()
{
    uint16_t temp;
    uint32_t temp1;

    switch(_st) {
    case LST_TSL2561:
        if(millis() - _lastAccess < 500)
            return;

        temp  = read16(TSL2561_ADC0, true);
        temp1 = read16(TSL2561_ADC1, true);

        if(temp1 == 0) {
            _lux = 0;
        } else if( ((temp1 << 1) <= temp) && (temp <= 4900)) {
            _lux = TSL2561CalcLux(TLS_USE_GAIN, TLS_USE_IT, temp, temp1);
        } else {
            _lux = -1;
        }
        break;

    case LST_BH1750:
        if(millis() - _lastAccess < (unsigned long)bh1750Arr[_gainIdx][1])
            return;

        temp = read16(BH1750_DUMMY);

        _lux = (int32_t)((float)temp / BH1750_CONV_FACT);
        break;

    case LST_VEML7700:
        if(millis() - _lastAccess < (unsigned long)AITArr[_AITIdx][1] * 8)
            return;

        // Re-write control register now and then; if there was an i2c
        // bus error while writing, the sensor might report values not
        // matching our assumptions about gain and AIT, and therefore
        // wrong lux values.
        _accessNum++;
        if(_accessNum > 100) {
            VEML7700OnOff(false);
            VEML7700OnOff(true, false);
            _accessNum = 0;
            _lastAccess = millis();
            return;
        }
        
        temp = read16(VEML7700_ALS, true);

        // Value too low, increase first gain, then AIT
        if(temp <= 100) {
            bool doQuit = false;
            if(_gainIdx < 3) {
                _gainIdx++;
                VEML7700OnOff(false);
                VEML7700SetGain(gainArr[_gainIdx]);
                doQuit = true;
            } else if(_AITIdx < 5) {
                _AITIdx++;
                VEML7700OnOff(false);
                VEML7700SetAIT(AITArr[_AITIdx][0]);
                doQuit = true;
            }
            if(doQuit) {
                VEML7700OnOff(true, false);
                _lastAccess = millis();
                _accessNum = 0;
                return;
            }
        }
    
        // Value within range
        if(temp < 10000 || ( (_gainIdx == 0) && (_AITIdx == 0) ) ) {
          
            _lux = (int32_t)((float)temp * resFact[_AITIdx][_gainIdx]);

            if(_gainIdx < 2 && _lux > 1000) {
                float lux = (float)_lux;
                _lux = (int32_t)((((6.0135e-13 * lux - 9.3924e-9) * lux + 8.1488e-5) * lux + 1.0023) * lux);
            }
            
        } else {
    
            // Value too high: Restart from low gain
            _gainIdx = 0;
            _AITIdx = 0;
            VEML7700OnOff(false);
            VEML7700SetGain(gainArr[_gainIdx], false);
            VEML7700SetAIT(AITArr[_AITIdx][0]);
            VEML7700OnOff(true, false);
            _accessNum = 0;
            
        }
    }

    _lastAccess = millis();
}

// Private functions ###########################################################

// VEML7700:

void lightSensor::VEML7700SetGain(uint16_t gain, bool doWrite)
{
    _con0 &= ~(0x1800);
    _con0 |= (gain << 11);
    
    if(doWrite) write16(VEML7700_CON0, _con0, true);
}

void lightSensor::VEML7700SetAIT(uint16_t ait, bool doWrite)
{
    _con0 &= ~(0x03c0);
    _con0 |= (ait << 6);
    
    if(doWrite) write16(VEML7700_CON0, _con0, true);
}

void lightSensor::VEML7700OnOff(bool enable, bool doWait)
{
    _con0 &= ~(0x0001);
    if(!enable) _con0 |= 0x0001;
    
    write16(VEML7700_CON0, _con0, true);

    if(enable && doWait) (*_customDelayFunc)(5);
}

// TSL2561
// Reference implementation
// taken from TLS2560/1 data sheet (April 2007, p22+)

#define TSL2561_LUX_SCALE     14   // scale by 2^14
#define TSL2561_RATIO_SCALE    9   // scale ratio by 2^9
#define TSL2561_CH_SCALE      10   // scale channel values by 2^10
#define TSL2561_CHSCALE_TINT0 0x7517 // 322/11 * 2^CH_SCALE
#define TSL2561_CHSCALE_TINT1 0x0fe7 // 322/81 * 2^CH_SCALE

#define TSL2561_K1T 0x0040 // 0.125 * 2^RATIO_SCALE
#define TSL2561_B1T 0x01f2 // 0.0304 * 2^LUX_SCALE
#define TSL2561_M1T 0x01be // 0.0272 * 2^LUX_SCALE
#define TSL2561_K2T 0x0080 // 0.250 * 2^RATIO_SCALE
#define TSL2561_B2T 0x0214 // 0.0325 * 2^LUX_SCALE
#define TSL2561_M2T 0x02d1 // 0.0440 * 2^LUX_SCALE
#define TSL2561_K3T 0x00c0 // 0.375 * 2^RATIO_SCALE
#define TSL2561_B3T 0x023f // 0.0351 * 2^LUX_SCALE
#define TSL2561_M3T 0x037b // 0.0544 * 2^LUX_SCALE
#define TSL2561_K4T 0x0100 // 0.50 * 2^RATIO_SCALE
#define TSL2561_B4T 0x0270 // 0.0381 * 2^LUX_SCALE
#define TSL2561_M4T 0x03fe // 0.0624 * 2^LUX_SCALE
#define TSL2561_K5T 0x0138 // 0.61 * 2^RATIO_SCALE
#define TSL2561_B5T 0x016f // 0.0224 * 2^LUX_SCALE
#define TSL2561_M5T 0x01fc // 0.0310 * 2^LUX_SCALE
#define TSL2561_K6T 0x019a // 0.80 * 2^RATIO_SCALE
#define TSL2561_B6T 0x00d2 // 0.0128 * 2^LUX_SCALE
#define TSL2561_M6T 0x00fb // 0.0153 * 2^LUX_SCALE
#define TSL2561_K7T 0x029a // 1.3 * 2^RATIO_SCALE
#define TSL2561_B7T 0x0018 // 0.00146 * 2^LUX_SCALE
#define TSL2561_M7T 0x0012 // 0.00112 * 2^LUX_SCALE
#define TSL2561_K8T 0x029a // 1.3 * 2^RATIO_SCALE
#define TSL2561_B8T 0x0000 // 0.000 * 2^LUX_SCALE
#define TSL2561_M8T 0x0000 // 0.000 * 2^LUX_SCALE

// TSL2561: Calculate Lux from ch0 and ch1 data
// iGain: 0=1x, 1=16x
// tInt:  0=13.7ms, 1:100ms, 2:402
uint32_t lightSensor::TSL2561CalcLux(uint8_t iGain, uint8_t tInt, uint32_t ch0, uint32_t ch1)
{
    uint32_t chScale, channel0, channel1;
    uint32_t b = 0, m = 0, ratio, ratio1 = 0;
    int32_t  temp;

    switch(tInt) {
    case TLS_IT_13:   // 13.7 msec
        chScale = TSL2561_CHSCALE_TINT0;
        break;
    case TLS_IT_101:  // 101 msec
        chScale = TSL2561_CHSCALE_TINT1;
        break;
    default: // assume no scaling
        chScale = (1 << TSL2561_CH_SCALE);
    }

    if(iGain == TLS_GAIN_LOW) chScale <<= 4;

    channel0 = (ch0 * chScale) >> TSL2561_CH_SCALE;
    channel1 = (ch1 * chScale) >> TSL2561_CH_SCALE;

    if(channel0 != 0) 
        ratio1 = (channel1 << (TSL2561_RATIO_SCALE+1)) / channel0;

    ratio = (ratio1 + 1) >> 1;
    
    if((ratio >= 0) && (ratio <= TSL2561_K1T))
        { b = TSL2561_B1T; m = TSL2561_M1T; }
    else if (ratio <= TSL2561_K2T)
        { b = TSL2561_B2T; m = TSL2561_M2T; }
    else if(ratio <= TSL2561_K3T)
        { b = TSL2561_B3T; m = TSL2561_M3T; }
    else if(ratio <= TSL2561_K4T)
        { b = TSL2561_B4T; m = TSL2561_M4T; }
    else if(ratio <= TSL2561_K5T)
        { b = TSL2561_B5T; m = TSL2561_M5T; }
    else if(ratio <= TSL2561_K6T)
        { b = TSL2561_B6T; m = TSL2561_M6T; }
    else if(ratio <= TSL2561_K7T)
        { b = TSL2561_B7T; m = TSL2561_M7T; }
    else if(ratio > TSL2561_K8T)
        { b = TSL2561_B8T; m = TSL2561_M8T; }

    temp = ((channel0 * b) - (channel1 * m));
    if(temp < 0) temp = 0;
    
    temp += (1 << (TSL2561_LUX_SCALE - 1));

    return (temp >> TSL2561_LUX_SCALE);
}

#endif // TC_HAVELIGHT
