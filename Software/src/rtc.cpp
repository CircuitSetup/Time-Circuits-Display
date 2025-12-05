/*
 * -------------------------------------------------------------------
 * CircuitSetup.us Time Circuits Display
 * (C) 2022-2025 Thomas Winischhofer (A10001986)
 * https://github.com/realA10001986/Time-Circuits-Display
 * https://tcd.out-a-ti.me
 * 
 * RTC Class (DS3231/PCF2129 RTC handling) and DateTime Class
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

#include "tc_global.h"

#include <Arduino.h>
#include <Wire.h>
#include "rtc.h"

// Registers
#define DS3231_TIME       0x00 // Time 
#define DS3231_ALARM1     0x07 // Alarm 1 
#define DS3231_ALARM2     0x0B // Alarm 2 
#define DS3231_CONTROL    0x0E // Control
#define DS3231_STATUS     0x0F // Status
#define DS3231_TEMP       0x11 // Temperature

#define PCF2129_CTRL1     0x00 // Control 1
#define PCF2129_CTRL2     0x01 // Control 2
#define PCF2129_CTRL3     0x02 // Control 3
#define PCF2129_TIME      0x03 // Time 
#define PCF2129_ALARM     0x0A // Alarm
#define PCF2129_CLKCTRL   0x0F // CLK Control

#ifdef HAVE_PCF2129
#define PUD 2000
#else
#define PUD 500
#endif

/*****************************************************************
 * DateTime Class
 * 
 * Rudimentary general-purpose date/time class, used as a 
 * vehicle to pass date/time between functions.
 ****************************************************************/

// Nada, nix... all in rtc.h


/****************************************************************
 * tcRTC Class for DS3231/PCF2129
 ****************************************************************/

tcRTC::tcRTC(int numTypes, const uint8_t *addrArr)
{
    _numTypes = min(2, numTypes);
    _addrArr = addrArr;

    _address = addrArr[0];
    _rtcType = addrArr[1];
}

/*
 *  Test i2c connection and detect chip type
 */

bool tcRTC::begin()
{
    // Give the RTC some time to boot
    unsigned long millisNow = millis();

    if(millisNow  < PUD) {
        delay(PUD - millisNow);
    }
    
    for(int i = 0; i < _numTypes * 2; i += 2) {

        // Check for RTC on i2c bus
        Wire.beginTransmission(_addrArr[i]);
        if(!(Wire.endTransmission(true))) {

            _address = _addrArr[i];
            _rtcType = _addrArr[i+1];

            #ifdef TC_DBG
            const char *tpArr[2] = { "DS3231", "PCF2129" };
            Serial.printf("RTC: Detected %s\n", tpArr[_rtcType]);
            #endif

            return true;
        }
    }

    return false;
}

/*
 * Set RTC
 * 
 * - Only years 2000-2099
 * - No calculation of dayOfWeek
 *
 * (year: 0-99; dayOfWeek: 0=Sun..6=Sat)
 */
void tcRTC::adjust(byte second, byte minute, byte hour, byte dayOfWeek, byte dayOfMonth, byte month, byte year)
{
    prepareAdjust(second, minute, hour, dayOfWeek, dayOfMonth, month, year);
    finishAdjust();
}

void tcRTC::prepareAdjust(byte second, byte minute, byte hour, byte dayOfWeek, byte dayOfMonth, byte month, byte year)
{
    buffer[1] = bin2bcd(second);
    buffer[2] = bin2bcd(minute);
    buffer[3] = bin2bcd(hour);
    buffer[6] = bin2bcd(month);
    buffer[7] = bin2bcd(year);

    switch(_rtcType) {

    case RTCT_PCF2129:
        #ifdef HAVE_PCF2129
        buffer[0] = PCF2129_TIME;
        buffer[4] = bin2bcd(dayOfMonth);
        buffer[5] = bin2bcd(dayOfWeek);
        #endif
        break;

    case RTCT_DS3231:
    default:
        #ifdef HAVE_DS3231
        buffer[0] = DS3231_TIME;
        buffer[4] = bin2bcd(dowToDS3231(dayOfWeek));
        buffer[5] = bin2bcd(dayOfMonth);
        #endif
        break;
    }

    _buffervalid = true;
    _buffNow = millis();

    #ifdef TC_DBG
    Serial.printf("RTC: Prepared to adjust to %d-%02d-%02d %02d:%02d:%02d DOW %d\n",
          year+2000, month, dayOfMonth, hour, minute, second, dayOfWeek);
    #endif
}

void tcRTC::finishAdjust()
{
    unsigned long now = millis();
    uint8_t statreg;

    if(!_buffervalid) return;

    if(now - _buffNow > 100) {
        _buffervalid = false;
        #ifdef TC_DBG
        Serial.printf("RTC finishAdjust() too late (%ums)\n", now - _buffNow);
        #endif
        return;
    }

    switch(_rtcType) {

    case RTCT_PCF2129:
        #ifdef HAVE_PCF2129
        write_bytes(buffer, 8); 
        // OSF bit cleared by writing seconds
        #endif
        break;

    case RTCT_DS3231:
    default:
        #ifdef HAVE_DS3231
        write_bytes(buffer, 8);
        // clear OSF bit
        statreg = read_register(DS3231_STATUS);
        statreg &= ~0x80;
        write_register(DS3231_STATUS, statreg);
        #endif
        break;
    }

    _buffervalid = false;

    #ifdef TC_DBG
    Serial.printf("RTC updated (delay %ums, took %ums)\n", now - _buffNow, millis() - now);
    #endif
}

/*
 * Get current date/time
 */
void tcRTC::now(DateTime& dt) 
{
    uint8_t buffer[7];
    uint16_t y = 2000;

    switch(_rtcType) {

    case RTCT_PCF2129:
        #ifdef HAVE_PCF2129
        read_bytes(PCF2129_TIME, buffer, 7);
        dt.set(bcd2bin(buffer[6]) + y,
               bcd2bin(buffer[5] & 0x7f),
               bcd2bin(buffer[3]),
               bcd2bin(buffer[2]),
               bcd2bin(buffer[1]),
               bcd2bin(buffer[0] & 0x7f));
        #endif
        break;

    case RTCT_DS3231:
    default:
        #ifdef HAVE_DS3231
        read_bytes(DS3231_TIME, buffer, 7);
        if(buffer[5] & 0x80) {
             y += 100;
             buffer[5] &= 0x7f;
        }
        dt.set(bcd2bin(buffer[6]) + y,
               bcd2bin(buffer[5]),
               bcd2bin(buffer[4]),
               bcd2bin(buffer[2]),
               bcd2bin(buffer[1]),
               bcd2bin(buffer[0] & 0x7f));
        #endif
    }
}

/*
 * Enable 1Hz clock output
 */
void tcRTC::clockOutEnable()
{
    uint8_t readValue = 0;

    switch(_rtcType) {
      
    case RTCT_PCF2129:
        #ifdef HAVE_PCF2129
        readValue = read_register(PCF2129_CLKCTRL);
        // set squarewave to 1Hz
        // Bits 2:0 - 110  sets 1Hz
        readValue &= B11111000;
        readValue |= B00000110;
        write_register(PCF2129_CLKCTRL, readValue);
        #endif
        break;

    case RTCT_DS3231:
    default:
        #ifdef HAVE_DS3231
        readValue = read_register(DS3231_CONTROL);
        // enable squarewave and set to 1Hz
        // Bit 2 INTCN - 0 enables wave output
        // Bit 3 and 4 - 0 0 sets 1Hz
        readValue &= B11100011;
        write_register(DS3231_CONTROL, readValue);
        #endif
        break;
    }
}

/*
 * OTP refresh (PCF2129 only)
 */
bool tcRTC::NeedOTPRefresh()
{
    #ifdef HAVE_PCF2129
    if(_rtcType == RTCT_PCF2129)
        return true;
    #endif
    
    return false;
}

#ifdef HAVE_PCF2129
bool tcRTC::OTPRefresh(bool start)
{
    uint8_t readValue = 0;

    switch(_rtcType) {
      
    case RTCT_PCF2129:
        readValue = read_register(PCF2129_CLKCTRL);
        // set squarewave to 1Hz
        // Bits 2:0 - 110  sets 1Hz
        readValue &= B11011111;
        if(!start) readValue |= B00100000;
        write_register(PCF2129_CLKCTRL, readValue);
        return true;

    }

    return false;
}
#endif

/*
 * Check if RTC is stopped due to power-loss
 */
bool tcRTC::lostPower(void)
{
    switch(_rtcType) {

    case RTCT_PCF2129:
        // Check Oscillator Stop Flag to see 
        // if RTC stopped due to power loss
        // Only works before writing seconds
        #ifdef HAVE_PCF2129
        return read_register(PCF2129_TIME) >> 7;
        #else
        break;
        #endif

    case RTCT_DS3231:
    default:
        #ifdef HAVE_DS3231
        // Check Oscillator Stop Flag to see 
        // if RTC stopped due to power loss
        return read_register(DS3231_STATUS) >> 7;
        #endif
    }

    return false;
}

/*
 * Check for "low batt warning"
 */
bool tcRTC::battLow(void)
{
    #ifdef HAVE_PCF2129
    switch(_rtcType) {

    case RTCT_PCF2129:
        return (read_register(PCF2129_CTRL3) & 0x04) >> 2;
    }
    #endif

    return false;
}

/*
 * Get DS3231 temperature
 * (Not supported on PCF2129)
 */
float tcRTC::getTemperature() 
{
    #ifdef HAVE_DS3231
    uint8_t buffer[2];

    switch(_rtcType) {

    case RTCT_DS3231:
        read_bytes(DS3231_TEMP, buffer, 2);
        return (float)buffer[0] + (float)(buffer[1] >> 6) * 0.25f;
    }
    #endif
    
    return 0.0f;
}

/*
 * Write value to register
 */
void tcRTC::write_register(uint8_t reg, uint8_t val)
{
    uint8_t buf[2] = { reg, val };
    write_bytes(buf, 2);
}

/*
 * Read value from register
 */
uint8_t tcRTC::read_register(uint8_t reg)
{
    uint8_t buf[1];
    read_bytes(reg, buf, 1);
    return buf[0];
}

void tcRTC::write_bytes(uint8_t *buffer, uint8_t num)
{
    Wire.beginTransmission(_address);
    for(int i = 0; i < num; i++) {
        Wire.write(buffer[i]);
    }
    Wire.endTransmission();   
}

void tcRTC::read_bytes(uint8_t reg, uint8_t *buffer, uint8_t num)
{
    Wire.beginTransmission(_address);
    Wire.write(reg);
    Wire.endTransmission();
    Wire.requestFrom(_address, num);
    for(int i = 0; i < num; i++) {
        buffer[i] = Wire.read();
    }
}
