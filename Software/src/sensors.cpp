/*
 * -------------------------------------------------------------------
 * CircuitSetup.us Time Circuits Display
 * (C) 2022-2023 Thomas Winischhofer (A10001986)
 * https://github.com/realA10001986/Time-Circuits-Display-A10001986
 *
 * Sensor Class: Temperature/humidty and Light Sensor handling
 *
 * This is designed for 
 * - MCP9808, TMP117, BMx280, SHT4x, SI7012, AHT20/AM2315C, HTU31D 
 *   temperature/humidity sensors,
 * - BH1750, TSL2561, LTR303/329 and VEML7700/VEML6030 light sensors.
 *                             
 * The i2c slave addresses need to be:
 * MCP9808:       0x18                [temperature only]
 * TMP117:        0x49 [non-default]  [temperature only]
 * BMx280:        0x77                ['P' t only, 'E' t+h]
 * SHT40:         0x44                [temperature, humidity]
 * SI7021:        0x40                [temperature, humidity]
 * AHT20/AM2315C: 0x38                [temperature, humidity]
 * HTU31D:        0x41 [non-default]  [temperature, humidity]
 * 
 * TSL2561:       0x29 
 * LTR303/329:    0x29
 * BH1750:        0x23
 * VEML6030:      0x10, 0x48 [non-default]
 * VEML7700:      0x10
 * 
 * If a GPS receiver is connected at the same time, 
 * - the VEML7700 cannot be used;
 * - the VEML6030 needs to be set to address 0x48.
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

void tcSensor::prepareRead(uint16_t regno)
{
    Wire.beginTransmission(_address);
    Wire.write((uint8_t)(regno));
    Wire.endTransmission(false);
}

uint16_t tcSensor::read16(uint16_t regno, bool LSBfirst)
{
    uint16_t value = 0;
    size_t i2clen = 0;

    if(regno <= 0xff) {
        prepareRead(regno);
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

    prepareRead(regno);

    Wire.requestFrom(_address, (uint8_t)1);

    value = Wire.read();

    return value;
}

void tcSensor::write16(uint16_t regno, uint16_t value, bool LSBfirst)
{
    Wire.beginTransmission(_address);
    if(regno <= 0xff) {
        Wire.write((uint8_t)(regno));
    }
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

uint8_t tcSensor::crc8(uint8_t initVal, uint8_t poly, uint8_t len, uint8_t *buf)
{
    uint8_t crc = initVal;
 
    while(len--) {
        crc ^= *buf++;
 
        for(uint8_t i = 0; i < 8; i++) {
            crc = (crc & 0x80) ? (crc << 1) ^ poly : (crc << 1);
        }
    }
 
    return crc;
}
#endif


/*****************************************************************
 * tempSensor Class
 * 
 * Supports MCP9808 and BMx280 temperature sensors
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
#define TC_TEMP_RES_MCP9808 3

#define BMx280_DUMMY      0x100
#define BMx280_REG_DIG_T1 0x88
#define BMx280_REG_DIG_T2 0x8a
#define BMx280_REG_DIG_T3 0x8c
#define BMx280_REG_DIG_H1 0xa1
#define BMx280_REG_DIG_H2 0xe1
#define BMx280_REG_DIG_H3 0xe3
#define BMx280_REG_DIG_H4 0xe4
#define BMx280_REG_DIG_H5 0xe5
#define BMx280_REG_DIG_H6 0xe7
#define BMx280_REG_ID     0xd0
#define BME280_REG_RESET  0xe0
#define BMx280_REG_CTRLH  0xf2
#define BMx280_REG_STATUS 0xf3
#define BMx280_REG_CTRLM  0xf4
#define BMx280_REG_CONF   0xf5
#define BMx280_REG_TEMP   0xfa
#define BMx280_REG_HUM    0xfd

#define SHT40_DUMMY       0x100
#define SHT40_CMD_RTEMPL  0xe0    // 1.3ms
#define SHT40_CMD_RTEMPM  0xf6    // 4.5ms
#define SHT40_CMD_RTEMPH  0xfd    // 8.3ms
#define SHT40_CMD_RESET   0x94    // 2ms

#define SHT40_CRC_INIT    0xff
#define SHT40_CRC_POLY    0x31

#define SI7021_DUMMY      0x100
#define SI7021_REG_U1W    0xe6
#define SI7021_REG_U1R    0xe7
#define SI7021_REG_HCRW   0x51
#define SI7021_REG_HCRR   0x11
#define SI7021_CMD_RTEMP  0xf3
#define SI7021_CMD_RHUM   0xf5
#define SI7021_CMD_RTEMPQ 0xe0
#define SI7021_RESET      0xfe

#define SI7021_CRC_INIT   0x00
#define SI7021_CRC_POLY   0x31

#define TMP117_REG_TEMP   0x00
#define TMP117_REG_CONF   0x01
#define TMP117_REG_DEV_ID 0x0f

#define TMP117_C_MODE_CC  (0x00 << 10)
#define TMP117_C_MODE_M   (0x03 << 10)
#define TMP117_C_AVG_0    (0x00 << 5)
#define TMP117_C_AVG_M    (0x03 << 5)
#define TMP117_C_CONV_16s (0x07 << 7)
#define TMP117_C_CONV_M   (0x07 << 7)
#define TMP117_C_RESET    0x0002

#define AHT20_DUMMY       0x100
#define AHT20_CRC_INIT    0xff
#define AHT20_CRC_POLY    0x31

#define HTU31_DUMMY       0x100
#define HTU31_RESET       0x1e
#define HTU31_CONV        (0x40 + 0x00 + 0x04)
#define HTU31_READTRH     0x00
#define HTU31_HEATEROFF   0x02

#define HTU31_CRC_INIT    0x00
#define HTU31_CRC_POLY    0x31

// Store i2c address
tempSensor::tempSensor(int numTypes, uint8_t addrArr[])
{
    _numTypes = min(8, numTypes);

    for(int i = 0; i < _numTypes * 2; i++) {
        _addrArr[i] = addrArr[i];
    }
}

// Start the display
bool tempSensor::begin(unsigned long powerupTime)
{
    bool foundSt = false;
    uint8_t temp, timeOut = 20;
    uint16_t t16 = 0;
    size_t i2clen;
    uint8_t buf[6];
    unsigned long millisNow = millis();
    
    _customDelayFunc = defaultDelay;

    // Give the sensor some time to boot
    if(millisNow - powerupTime < 50) {
        delay(50 - (millisNow - powerupTime));
    }

    for(int i = 0; i < _numTypes * 2; i += 2) {

        _address = _addrArr[i];
        
        switch(_addrArr[i+1]) {
        case MCP9808:
            if( (read16(MCP9808_REG_MANUF_ID)  == 0x0054) && 
                (read16(MCP9808_REG_DEVICE_ID) == 0x0400) ) {
                foundSt = true;
            }
            break;
        case BMx280:
            temp = read8(BMx280_REG_ID);
            if(temp == 0x60 || temp == 0x58) {
                foundSt = true;
                if(temp == 0x60) _haveHum = true;
            }
            break;
        case SHT40:
            Wire.beginTransmission(_address);
            if(!Wire.endTransmission(true)) {
                // Do a test-measurement for id
                write8(SHT40_DUMMY, SHT40_CMD_RTEMPL);
                (*_customDelayFunc)(5);
                if(Wire.requestFrom(_address, (uint8_t)6) == 6) {
                    for(uint8_t i = 0; i < 6; i++) buf[i] = Wire.read();
                    if(crc8(SHT40_CRC_INIT, SHT40_CRC_POLY, 2, buf) == buf[2]) {
                        foundSt = true;
                    }
                }
            }
            break;
        case SI7021:
            Wire.beginTransmission(_address);
            if(!Wire.endTransmission(true)) {
                // Check power-up value of user1 register for id
                write8(SI7021_DUMMY, SI7021_RESET);
                (*_customDelayFunc)(20);
                if(read8(SI7021_REG_U1R) == 0x3a) {
                    foundSt = true;
                }
            }
            break;
        case TMP117:
            if((read16(TMP117_REG_DEV_ID) & 0xfff) == 0x117) {
                foundSt = true;
            }
            break;
        case AHT20:
        case HTU31:
            Wire.beginTransmission(_address);
            if(!Wire.endTransmission(true)) {
                foundSt = true;
            }
            break;
        }
        
        if(foundSt) {
            _st = _addrArr[i+1];

            #ifdef TC_DBG
            const char *tpArr[7] = { "MCP9808", "BMx280", "SHT4x", "SI7021", "TMP117", "AHT20/AM2315C", "HTU31D" };
            Serial.printf("Temperature sensor: Detected %s\n", tpArr[_st]);
            #endif

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
        
    case BMx280:
        // Reset
        write8(BME280_REG_RESET, 0xb6);
        (*_customDelayFunc)(10);

        // Wait for sensor to copy its calib data
        do {
            temp = read8(BMx280_REG_STATUS);
            (*_customDelayFunc)(10);
        } while((temp & 1) && --timeOut);

        // read relevant calib data
        _BMx280_CD_T1 = read16(BMx280_REG_DIG_T1, true);
        _BMx280_CD_T2 = (int32_t)((int16_t)read16(BMx280_REG_DIG_T2, true));
        _BMx280_CD_T3 = (int32_t)((int16_t)read16(BMx280_REG_DIG_T3, true));
        if(_haveHum) {
            _BMx280_CD_H1 = read8(BMx280_REG_DIG_H1);
            _BMx280_CD_H2 = (int32_t)((int16_t)read16(BMx280_REG_DIG_H2, true));
            _BMx280_CD_H3 = read8(BMx280_REG_DIG_H3);
            buf[0] = read8(BMx280_REG_DIG_H4);
            buf[1] = read8(BMx280_REG_DIG_H5);
            buf[2] = read8(BMx280_REG_DIG_H5+1);
            _BMx280_CD_H4 = (int32_t)((int16_t)((buf[0] << 4) | (buf[1] & 0x0f)));
            _BMx280_CD_H5 = (int32_t)((int16_t)(((buf[1] & 0xf0) >> 4) | (buf[2] << 4)));
            _BMx280_CD_H6 = read8(BMx280_REG_DIG_H6);
            _BMx280_CD_H4 *= 1048576;
        }

        #ifdef TC_DBG
        Serial.printf("BMx280 T calib values: %d %d %d\n", _BMx280_CD_T1, _BMx280_CD_T2, _BMx280_CD_T3);
        if(_haveHum) {
            Serial.printf("BMx280 H calib values: %d %d %d %d %d %d\n", 
                _BMx280_CD_H1, _BMx280_CD_H2, _BMx280_CD_H3,
                _BMx280_CD_H4 / 1048576, _BMx280_CD_H5, _BMx280_CD_H6);
        }
        #endif

        // setup sensor parameters
        write8(BMx280_REG_CTRLM, 0x20);     // Temp OSx1; Pres skipped; "sleep mode"
        if(_haveHum) {
            write8(BMx280_REG_CTRLH, 0x01); // Hum OSx1
        }
        write8(BMx280_REG_CONF,  0xa0);     // t_sb 1000ms; filter off, SPI3w off
        write8(BMx280_REG_CTRLM, 0x23);     // Temp OSx1; Pres skipped; "normal mode"
        break;

    case SHT40:
        // Reset
        write8(SHT40_DUMMY, SHT40_CMD_RESET);
        (*_customDelayFunc)(5);
        // Trigger measurement
        write8(SHT40_DUMMY, SHT40_CMD_RTEMPM);
        _haveHum = true;
        _delayNeeded = 10;
        break;

    case SI7021:
        temp = read8(SI7021_REG_U1R);
        // 13bit temp, 10bit rh, heater off
        temp &= 0x3a;                    
        temp |= 0x80;
        write8(SI7021_REG_U1W, temp);
        // (Heater minimum)
        temp = read8(SI7021_REG_HCRR);
        temp &= 0xf0;
        write8(SI7021_REG_HCRW, temp);
        // Trigger measurement
        write8(SI7021_DUMMY, SI7021_CMD_RHUM); 
        _haveHum = true;
        _delayNeeded = 7+5+1;
        break;

    case TMP117:
        // Reset
        t16 = read16(TMP117_REG_CONF);
        t16 |= TMP117_C_RESET;
        write16(TMP117_REG_CONF, t16);
        (*_customDelayFunc)(5);
        // Config: cont. mode, no avg, 16s conversion cycle
        t16 = read16(TMP117_REG_CONF);
        t16 &= ~(TMP117_C_MODE_M | TMP117_C_AVG_M | TMP117_C_CONV_M);
        t16 |= (TMP117_C_MODE_CC | TMP117_C_AVG_0 | TMP117_C_CONV_16s);
        write16(TMP117_REG_CONF, t16);
        break;

    case AHT20:
        // reset
        write8(AHT20_DUMMY, 0xba); 
        (*_customDelayFunc)(21);
        // init (calibrate)
        write16(0xbe, 0x0800);     
        (*_customDelayFunc)(11);
        // trigger measurement
        write16(0xac, 0x3300);
        _haveHum = true;
        _delayNeeded = 85;
        break;

    case HTU31:
        // reset
        write8(HTU31_DUMMY, HTU31_RESET);
        (*_customDelayFunc)(20);
        // heater off
        write8(HTU31_DUMMY, HTU31_HEATEROFF);
        // Trigger conversion
        write8(HTU31_DUMMY, HTU31_CONV);
        _haveHum = true;
        _delayNeeded = 20;
        break;

    default:
        return false;
    }

    _tempReadNow = millis();

    return true;
}

void tempSensor::setCustomDelayFunc(void (*myDelay)(unsigned int))
{
    _customDelayFunc = myDelay;
}

// Read temperature
float tempSensor::readTemp(bool celsius)
{
    float temp = NAN;
    uint16_t t = 0;
    

    if(_delayNeeded > 0) {
        unsigned long elapsed = millis() - _tempReadNow;
        if(elapsed < _delayNeeded) (*_customDelayFunc)(_delayNeeded - elapsed);
    }

    switch(_st) {

    case MCP9808:
        t = read16(MCP9808_REG_AMBIENT_TEMP);
        if(t != 0xffff) {
            temp = ((float)(t & 0x0fff)) / 16.0;
            if(t & 0x1000) temp = 256.0 - temp;
        }
        break;

    case BMx280:
        write8(BMx280_DUMMY, BMx280_REG_TEMP);
        t = _haveHum ? 5 : 3;
        if(Wire.requestFrom(_address, (uint8_t)t) == t) {
            uint8_t buf[5];
            uint32_t t1; 
            uint16_t t2 = 0;
            for(uint8_t i = 0; i < t; i++) buf[i] = Wire.read();
            t1 = (buf[0] << 16) | (buf[1] << 8) | buf[2];
            if(_haveHum) t2 = (buf[3] << 8) | buf[4];
            temp = BMx280_CalcTemp(t1, t2);
        }
        break;

    case SHT40:
        if(Wire.requestFrom(_address, (uint8_t)6) == 6) {
            uint8_t buf[6];
            for(uint8_t i = 0; i < 6; i++) buf[i] = Wire.read();
            if(crc8(SHT40_CRC_INIT, SHT40_CRC_POLY, 2, buf) == buf[2]) {
                t = (buf[0] << 8) | buf[1];
                temp = ((175.0 * (float)t) / 65535.0) - 45.0;
            }
            if(crc8(SHT40_CRC_INIT, SHT40_CRC_POLY, 2, buf+3) == buf[5]) {
                t = (buf[3] << 8) | buf[4];
                _hum = (int8_t)(((125.0 * (float)t) / 65535.0) - 6.0);
                if(_hum < 0) _hum = 0;
            }
        }
        write8(SHT40_DUMMY, SHT40_CMD_RTEMPM);    // Trigger new measurement
        break;

    case SI7021:
        if(Wire.requestFrom(_address, (uint8_t)3) == 3) {
            uint8_t buf[3];
            for(uint8_t i = 0; i < 3; i++) buf[i] = Wire.read();
            if(crc8(SI7021_CRC_INIT, SI7021_CRC_POLY, 2, buf) == buf[2]) {
                t = (buf[0] << 8) | buf[1];
                _hum = (int8_t)(((125.0 * (float)t) / 65536.0) - 6.0);
                if(_hum < 0) _hum = 0;
            }
        }
        write8(SI7021_DUMMY, SI7021_CMD_RTEMPQ);
        if(Wire.requestFrom(_address, (uint8_t)2) == 2) {
            uint8_t buf[2];
            for(uint8_t i = 0; i < 2; i++) buf[i] = Wire.read();
            t = (buf[0] << 8) | buf[1];
            temp = ((175.72 * (float)t) / 65536.0) - 46.85;
        }
        write8(SI7021_DUMMY, SI7021_CMD_RHUM);    // Trigger new measurement
        break;

    case TMP117:
        t = read16(TMP117_REG_TEMP);
        if(t != 0x8000) {
            temp = (float)((int16_t)t) / 128.0;
        }
        break;

    case AHT20:
        if(Wire.requestFrom(_address, (uint8_t)7) == 7) {
            uint8_t buf[7];
            for(uint8_t i = 0; i < 7; i++) buf[i] = Wire.read();
            if(crc8(AHT20_CRC_INIT, AHT20_CRC_POLY, 6, buf) == buf[6]) {
                _hum = ((uint32_t)((buf[1] << 12) | (buf[2] << 4) | (buf[3] >> 4))) * 100 / 1048576;
                if(_hum < 0) _hum = 0;
                temp = ((float)((uint32_t)(((buf[3] & 0x0f) << 16) | (buf[4] << 8) | buf[5]))) * 200.0 / 1048576.0 - 50.0;
            }
        }
        write16(0xac, 0x3300);    // Trigger new measurement
        break;

    case HTU31:
        write8(HTU31_DUMMY, HTU31_READTRH);  // Read t+rh
        if(Wire.requestFrom(_address, (uint8_t)6) == 6) {
            uint8_t buf[6];
            for(uint8_t i = 0; i < 6; i++) buf[i] = Wire.read();
            if(crc8(HTU31_CRC_INIT, HTU31_CRC_POLY, 2, buf) == buf[2]) {
                t = (buf[0] << 8) | buf[1];
                temp = ((165.0 * (float)t) / 65535.0) - 40.0;
            }
            if(crc8(HTU31_CRC_INIT, HTU31_CRC_POLY, 2, buf+3) == buf[5]) {
                t = (buf[3] << 8) | buf[4];
                _hum = (int8_t)((100.0 * (float)t) / 65535.0);
                if(_hum < 0) _hum = 0;
            }
        }
        write8(HTU31_DUMMY, HTU31_CONV);  // Trigger new conversion
        break;
    }

    _tempReadNow = millis();

    if(!isnan(temp)) {
        if(!celsius) temp = temp * 9.0 / 5.0 + 32.0;
        temp += _userOffset;
    }

    // We use only 2 digits, so truncate
    if(_hum > 99) _hum = 99;
    
    #ifdef TC_DBG
    Serial.printf("Sensor temp+offset: %f\n", temp);
    if(_haveHum) {
        Serial.printf("Sensor humidity: %d\n", _hum);
    }
    #endif

    _lastTemp = temp;

    return temp;
}

void tempSensor::setOffset(float myOffs)
{
    _userOffset = myOffs;
}

// Private functions ###########################################################

float tempSensor::BMx280_CalcTemp(uint32_t ival, uint32_t hval)
{
    int32_t var1, var2, fine_t, temp;

    if(ival == 0x800000) return NAN;

    ival >>= 4;
    
    var1 = ((((ival >> 3) - (_BMx280_CD_T1 << 1))) * _BMx280_CD_T2) / 2048;
    var2 = (ival >> 4) - _BMx280_CD_T1;
    var2 = (((var2 * var2) / 4096) * _BMx280_CD_T3) / 16384;

    fine_t = var1 + var2;

    if(_haveHum) {
        temp = fine_t - 76800;
    
        temp = ((((hval << 14) - _BMx280_CD_H4 - (_BMx280_CD_H5 * temp)) + 16384) / 32768) * 
               ( ( ((( ((temp * _BMx280_CD_H6) / 1024) * (((temp * _BMx280_CD_H3) >> 11) + 32768)) / 1024) + 2097152) * 
                  _BMx280_CD_H2 + 8192 ) / 16384);
        temp -= (((((temp / 32768) * (temp / 32768)) / 128) * _BMx280_CD_H1) / 16);
        if(temp < 0) temp = 0;
    
        _hum = temp >> 22;
    }
    
    return (float)((fine_t * 5 + 128) / 256) / 100.0;
}

#endif // TC_HAVETEMP


/*****************************************************************
 * lightSensor Class
 * 
 * Supports TSL2561, LTR3xx, BH1750, VEML7700/6030 light sensors
 * 
 * Sensors are set for indoor conditions and might overflow (in 
 * which case -1 is returned) or report bad lux values outdoors
 * or if subject to intensive IR light (from eg cameras).
 * The settings for LTR3xx, TSL2561 and BH1750 can be changed by 
 * #defines below. VEML7700/6030 auto-adjust.
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

#define LTR303_DUMMY    0x100
#define LTR303_CTRL     0x80
#define LTR303_MRATE    0x85
#define LTR303_DATA1    0x88  // LSBFirst
#define LTR303_DATA0    0x8a  // LSBFirst
#define LTR303_PID      0x86
#define LTR303_MID      0x87

static const uint8_t ltr3xxGains[6] = {
    0x00, 0x01, 0x02, 0x03, 0x06, 0x07
//   1x    2x    4x    8x   48x   96x
};
static const uint8_t ltr3xxITs[8] = {
    0x01, 0x00, 0x04, 0x02, 0x05, 0x06, 0x07, 0x03
//   50   100   150   200   250   300   350   400ms
};
#define LTR303_USE_GAIN   1     // to be adjusted (index in above array)
#define LTR303_USE_IT     1     // to be adjusted (index in above array)

#define BH1750_DUMMY      0x100

#define BH1750_CONT_HRES  0x10  // 1 lux resolution / 120ms
#define BH1750_CONT_HRES2 0x11  // 0.5 lux resolution / 120ms
#define BH1750_CONT_LRES  0x13  // 4 lux resolution / 16ms

#define BH1750_CONV_FACT  1.2f

static const uint8_t bh1750Arr[3][2] = {
      { BH1750_CONT_HRES,   120+30 }, 
      { BH1750_CONT_HRES2,  120+30 }, 
      { BH1750_CONT_LRES,    16+30 }
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

bool lightSensor::begin(bool skipLast, unsigned long powerupTime)
{
    bool foundSt = false;
    unsigned long millisNow = millis();

    _customDelayFunc = defaultDelay;

    // VEML7700 shares i2c address with GPS, so
    // skip it if GPS is connected
    if(skipLast) _numTypes--;

    // Give the sensor some time to boot
    if(millisNow - powerupTime < 100) {
        delay(100 - (millisNow - powerupTime));
    }
    
    for(int i = 0; i < _numTypes * 2; i += 2) {

        _address = _addrArr[i];

        switch(_addrArr[i+1]) {
        case LST_LTR3xx:
            // Needs to be checked before the TSL2561
            Wire.beginTransmission(_address);
            if(!Wire.endTransmission(true)) {
                if((read8(LTR303_MID) == 0x05) &&
                   ((read8(LTR303_PID) & 0xf0) == 0xa0)) {
                    // 303 and 329 share part number. Smart move.
                    foundSt = true;
                }
            }
            break;
        case LST_TSL2561:
            // LTR3xx must be tested for before this
            Wire.beginTransmission(_address);
            if(!Wire.endTransmission(true)) {
                if((read8(TSL2561_ID) & 0xf0) == 0x50) {
                    foundSt = true;
                }
            }
            break;
        case LST_BH1750:
        case LST_VEML7700:
            Wire.beginTransmission(_address);
            if(!Wire.endTransmission(true)) {
                foundSt = true;
            }
        }

        if(foundSt) {
            _st = _addrArr[i+1];
            
            #ifdef TC_DBG
            const char *tpArr[4] = { "TSL2561", "BH1750", "VEML7700/6030", "LTR303/329" };
            Serial.printf("Light sensor: Detected %s\n", tpArr[_st]);
            #endif
            
            break;
        }
    }

    switch(_st) {
    case LST_LTR3xx:
        write8(LTR303_CTRL, 0x02);  // Reset
        (*_customDelayFunc)(10);
        // Set IT; MRate 2000ms
        write8(LTR303_MRATE, (ltr3xxITs[LTR303_USE_IT] << 3) | 0x03); 
        // Set gain, activate
        write8(LTR303_CTRL, (ltr3xxGains[LTR303_USE_GAIN] << 2) | 0x01);  
        break;

    case LST_TSL2561:
        write8(TSL2561_CTRL, 0x03); // Power up
        write8(TSL2561_ICTRL, 0);   // no interrupts
        write8(TSL2561_TIM, TLS_USE_GAIN | TLS_USE_IT);
        break;
        
    case LST_BH1750:
        write8(BH1750_DUMMY, 0x01); // Power up
        _gainIdx = BH1750_USE_SET;  // Set mode
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

void lightSensor::loop()
{
    uint16_t temp;
    uint32_t temp1;
    unsigned long elapsed = millis() - _lastAccess;

    switch(_st) {
    case LST_TSL2561:
        if(elapsed < 500)
            return;

        temp  = read16(TSL2561_ADC0, true);
        temp1 = read16(TSL2561_ADC1, true);

        if(temp1 == 0) {
            _lux = 0;
        } else if( ((temp1 << 1) <= temp) && (temp <= 4900)) {
            _lux = TSL2561CalcLux(TLS_USE_GAIN, TLS_USE_IT, temp, temp1);
        } else {
            // IR-overload, human eye light level not determinable
            _lux = -1;
        }
        break;

    case LST_LTR3xx:
        if(elapsed < 500)
            return;

        write8(LTR303_DUMMY, LTR303_DATA1);
        if(Wire.requestFrom(_address, (uint8_t)4) == 4) {
            temp1 = Wire.read();
            temp1 |= (Wire.read() << 8);
            temp  = Wire.read();
            temp  |= (Wire.read() << 8);
            if(temp + temp1 == 0) {
                _lux = 0;
            } else {
                _lux = LTR3xxCalcLux(LTR303_USE_GAIN, LTR303_USE_IT, temp, temp1);
            }
        }
        break;

    case LST_BH1750:
        if(elapsed < (unsigned long)bh1750Arr[_gainIdx][1])
            return;

        temp = read16(BH1750_DUMMY);

        _lux = (int32_t)((float)temp / BH1750_CONV_FACT);
        break;

    case LST_VEML7700:
        if(elapsed < (unsigned long)AITArr[_AITIdx][1] * 8)
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

int32_t lightSensor::LTR3xxCalcLux(uint8_t iGain, uint8_t tInt, uint32_t ch0, uint32_t ch1)
{
    double gains[]   = {   1.0,   2.0,   4.0,  8.0, 48.0, 96.0 };
    int32_t maxLux[] = { 64000, 32000, 16000, 8000, 1300, 600 };
    double its[]     = { 0.5, 1.0, 1.5, 2.0, 2.5, 3.0, 3.5, 4.0 };
    //                   50   100  150  200  250  300  350  400
    int32_t lux = -1;
    double dch0 = (double)ch0;
    double dch1 = (double)ch1;

    double ratio = dch1 / (dch0 + dch1);

    if(ratio < 0.45)
        lux = (int32_t)((1.7743 * dch0 + 1.1059 * dch1) / gains[iGain] / its[tInt]);
    else if (ratio < 0.64)
        lux = (int32_t)((4.2785 * dch0 - 1.9548 * dch1) / gains[iGain] / its[tInt]);
    else if (ratio < 0.85)
        lux = (int32_t)((0.5926 * dch0 + 0.1185 * dch1) / gains[iGain] / its[tInt]);
    else {
        // IR-overload, human eye light level not determinable
        return -1;
    }

    return min(lux, maxLux[iGain]);
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
    
    if(ratio <= TSL2561_K1T) {
        b = TSL2561_B1T; m = TSL2561_M1T;
    } else if (ratio <= TSL2561_K2T) {
        b = TSL2561_B2T; m = TSL2561_M2T;
    } else if(ratio <= TSL2561_K3T) {
        b = TSL2561_B3T; m = TSL2561_M3T;
    } else if(ratio <= TSL2561_K4T) {
        b = TSL2561_B4T; m = TSL2561_M4T;
    } else if(ratio <= TSL2561_K5T) {
        b = TSL2561_B5T; m = TSL2561_M5T;
    } else if(ratio <= TSL2561_K6T) {
        b = TSL2561_B6T; m = TSL2561_M6T;
    } else if(ratio <= TSL2561_K7T) {
        b = TSL2561_B7T; m = TSL2561_M7T;
    } else { //if(ratio > TSL2561_K8T)
        b = TSL2561_B8T; m = TSL2561_M8T; 
    }

    temp = ((channel0 * b) - (channel1 * m));
    if(temp < 0) temp = 0;
    
    temp += (1 << (TSL2561_LUX_SCALE - 1));

    return (temp >> TSL2561_LUX_SCALE);
}

#endif // TC_HAVELIGHT
