/*
 * -------------------------------------------------------------------
 * CircuitSetup.us Time Circuits Display
 * (C) 2022-2024 Thomas Winischhofer (A10001986)
 * https://github.com/realA10001986/Time-Circuits-Display
 * https://tcd.out-a-ti.me
 * 
 * RTC Class (DS3231/PCF2129 RTC handling) and DateTime Class
 * 
 * DateTime mirrors the features of RTC; it only works for dates
 * from 1/1/2000 to 31/12/2099. Inspired by Adafruit's RTCLib.
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


/*****************************************************************
 * DateTime Class
 * 
 * Rudimentary general-purpose date/time class, used as a 
 * vehicle to pass date/time between functions.
 * 
 * Supports dates in the range from 1 Jan 2000 to 31 Dec 2099.
 ****************************************************************/

/*
 *  Copy constructor
 */
DateTime::DateTime(const DateTime& copy)
    : yOff(copy.yOff), 
      m(copy.m), 
      d(copy.d), 
      hh(copy.hh), 
      mm(copy.mm),
      ss(copy.ss) {}

/*
 * Return weekday (0=Sun)
 */
uint8_t DateTime::dayOfTheWeek() const 
{
    const int t[] = { 0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4 };
    int y = yOff + 2000;
    if(m < 3) y -= 1;
    return (y + y/4 - y/100 + y/400 + t[m-1] + d) % 7;
}


/****************************************************************
 * tcRTC Class for DS3231/PCF2129
 ****************************************************************/

tcRTC::tcRTC(int numTypes, uint8_t addrArr[])
{
    _numTypes = min(2, numTypes);

    for(int i = 0; i < _numTypes * 2; i++) {
        _addrArr[i] = addrArr[i];
    }

    _address = addrArr[0];
    _rtcType = addrArr[1];
}

/*
 *  Test i2c connection and detect chip type
 */
bool tcRTC::begin(unsigned long powerupTime)
{

    // Give the RTC some time to boot
    unsigned long millisNow = millis();
    if(millisNow - powerupTime < 2000) {
        delay(2000 - (millisNow - powerupTime));
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
 * Set the date/time from DateTime object
 * 
 * - Only years 2000-2099
 * - doW is calculated from object
 *
 * (year: 2000-2099; dayOfWeek: 0=Sun..6=Sat)
 */
void tcRTC::adjust(const DateTime& dt)
{
    adjust(dt.second(),
           dt.minute(),
           dt.hour(),
           dt.dayOfTheWeek(),
           dt.day(),
           dt.month(),
           dt.year() - 2000U);
}

/*
 * Set the date/time from individual elements
 * 
 * - Only years 2000-2099
 * - No calculation of dayOfWeek
 *
 * (year: 0-99; dayOfWeek: 0=Sun..6=Sat)
 */
void tcRTC::adjust(byte second, byte minute, byte hour, byte dayOfWeek, byte dayOfMonth, byte month, byte year)
{
    uint8_t buffer[8];
    uint8_t statreg;

    buffer[1] = bin2bcd(second);
    buffer[2] = bin2bcd(minute);
    buffer[3] = bin2bcd(hour);
    buffer[6] = bin2bcd(month);
    buffer[7] = bin2bcd(year);

    switch(_rtcType) {

    case RTCT_PCF2129:
        buffer[0] = PCF2129_TIME;
        buffer[4] = bin2bcd(dayOfMonth);
        buffer[5] = bin2bcd(dayOfWeek);
        write_bytes(buffer, 8); 

        // OSF bit cleared by writing seconds
        break;

    case RTCT_DS3231:
    default:
        buffer[0] = DS3231_TIME;
        buffer[4] = bin2bcd(dowToDS3231(dayOfWeek));
        buffer[5] = bin2bcd(dayOfMonth);
        write_bytes(buffer, 8);  

        // clear OSF bit
        statreg = read_register(DS3231_STATUS);
        statreg &= ~0x80;
        write_register(DS3231_STATUS, statreg);
        break;
    }

    #ifdef TC_DBG
    Serial.printf("RTC: Adjusted to %d-%02d-%02d %02d:%02d:%02d DOW %d\n",
          year+2000, month, dayOfMonth, hour, minute, second, dayOfWeek);
    #endif
}

/*
 * Get current date/time
 */
void tcRTC::now(DateTime& dt) 
{
    uint8_t buffer[7];

    switch(_rtcType) {

    case RTCT_PCF2129:
        read_bytes(PCF2129_TIME, buffer, 7);
        dt.set(bcd2bin(buffer[6]) + 2000U,
               bcd2bin(buffer[5] & 0x7F),
               bcd2bin(buffer[3]),
               bcd2bin(buffer[2]),
               bcd2bin(buffer[1]),
               bcd2bin(buffer[0] & 0x7F));

    case RTCT_DS3231:
    default:
        read_bytes(DS3231_TIME, buffer, 7);
        dt.set(bcd2bin(buffer[6]) + 2000U,
               bcd2bin(buffer[5] & 0x7F),
               bcd2bin(buffer[4]),
               bcd2bin(buffer[2]),
               bcd2bin(buffer[1]),
               bcd2bin(buffer[0] & 0x7F));
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
        readValue = read_register(PCF2129_CLKCTRL);
        // set squarewave to 1Hz
        // Bits 2:0 - 110  sets 1Hz
        readValue &= B11111000;
        readValue |= B00000110;
        write_register(PCF2129_CLKCTRL, readValue);
        break;

    case RTCT_DS3231:
    default:
        readValue = read_register(DS3231_CONTROL);
        // enable squarewave and set to 1Hz
        // Bit 2 INTCN - 0 enables wave output
        // Bit 3 and 4 - 0 0 sets 1Hz
        readValue &= B11100011;
        write_register(DS3231_CONTROL, readValue);
        break;
    }
}

/*
 * OTP refresh (PCF2129 only)
 */
bool tcRTC::NeedOTPRefresh()
{
    if(_rtcType == RTCT_PCF2129)
        return true;

    return false;
}

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
        return read_register(PCF2129_TIME) >> 7;

    case RTCT_DS3231:
    default:
        // Check Oscillator Stop Flag to see 
        // if RTC stopped due to power loss
        return read_register(DS3231_STATUS) >> 7;
    }

    return false;
}

/*
 * Check for "low batt warning"
 */
bool tcRTC::battLow(void)
{
    switch(_rtcType) {

    case RTCT_PCF2129:
        return (read_register(PCF2129_CTRL3) & 0x04) >> 2;
    }

    return false;
}

/*
 * Get DS3231 temperature
 * (Not supported on PCF2129)
 */
float tcRTC::getTemperature() 
{
    uint8_t buffer[2];

    switch(_rtcType) {

    case RTCT_DS3231:
        read_bytes(DS3231_TEMP, buffer, 2);
        return (float)buffer[0] + (float)(buffer[1] >> 6) * 0.25f;
    }

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
