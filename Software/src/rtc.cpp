/*
 * -------------------------------------------------------------------
 * CircuitSetup.us Time Circuits Display
 * (C) 2022 Thomas Winischhofer (A10001986)
 * DateTime part: Based on code Copyright (C) 2019 Adafruit Industries
 * 
 * DS3231/PCF2129 RTC handling and DateTime Class
 * 
 * Note: DateTime mirrors the features of RTC; it only works for
 * dates from 1/1/2000 to 31/12/2099.
 * 
 * DateTime is a cut-down and customized fork of Adafruit's RTClib
 * The original version can be found here:
 * https://github.com/adafruit/RTClib
 * -------------------------------------------------------------------
 * License: MIT
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
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
 * Rudimentary general-purpose date/time class 
 * (no TZ / DST / leap seconds).
 * 
 * Used as a vehicle to pass date between functions only; all
 * other features have been removed.
 * 
 * Supports dates in the range from 1 Jan 2000 to 31 Dec 2099.
 ****************************************************************/

/*
 * Convert a string containing two digits to uint8_t
 */
static uint8_t conv2d(const char *p)
{
    uint8_t v = 0;

    if(*p >= '0' && *p <= '9')
        v = (*p - '0') * 10;

    return v + (*++p - '0');
}

/*
 * Convert 3-letter-month to numerical
 */
static uint8_t convMS2m(const char *date)
{
    // Jan Feb Mar Apr May Jun Jul Aug Sep Oct Nov Dec
    switch(date[0]) {
    case 'J':
        return (date[1] == 'a') ? 1 : ((date[2] == 'n') ? 6 : 7);
    case 'F':
        return 2;
    case 'M':
        return date[2] == 'r' ? 3 : 5;
    case 'A':
        return date[2] == 'r' ? 4 : 8;
    case 'S':
        return 9;
    case 'O':
        return 10;
    case 'N':
        return 11;
    }
    return 12;
}

/*
 * Constructor for empty DateTime object (1-Jan-2000 00:00)
 */
DateTime::DateTime()
{
    yOff = 0;
    m = 1;
    d = 1;
    hh = 0;
    mm = 0;
    ss = 0;
}

/*
 * Constructor from (year, month, day, hour, minute, second) 
 */
DateTime::DateTime(uint16_t year, uint8_t month, uint8_t day,
                   uint8_t  hour, uint8_t min,   uint8_t sec)
{
    if(year >= 2000U)
        year -= 2000U;
        
    yOff = year;
    m = month;
    d = day;
    hh = hour;
    mm = min;
    ss = sec;
}

/*
 * Constructor for generating the build time
 * from C strings
 */
DateTime::DateTime(const char *date, const char *time)
{
    yOff = conv2d(date + 9);
    m = convMS2m(date);
    d  = conv2d(date + 4);
    hh = conv2d(time);
    mm = conv2d(time + 3);
    ss = conv2d(time + 6);
}

/*
 * Constructor for generating the build time
 * from __FlashStringHelper strings
 * 
 * DateTime buildTime(F(__DATE__), F(__TIME__));
 */
DateTime::DateTime(const __FlashStringHelper *date,
                   const __FlashStringHelper *time) 
{
    char buff[11];
    memcpy_P(buff, date, 11);
    yOff = conv2d(buff + 9);
    m = convMS2m((const char *)buff);
    d = conv2d(buff + 4);
    memcpy_P(buff, time, 8);
    hh = conv2d(buff);
    mm = conv2d(buff + 3);
    ss = conv2d(buff + 6);
}

/*
 *  Copy constructor
 */
DateTime::DateTime(const DateTime &copy)
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
bool tcRTC::begin()
{
    for(int i = 0; i < _numTypes * 2; i += 2) {

        // Check for RTC on i2c bus
        Wire.beginTransmission(_addrArr[i]);

        if(!(Wire.endTransmission(true))) {

            _address = _addrArr[i];
            _rtcType = _addrArr[i+1];

            #ifdef TC_DBG
            const char *tpArr[2] = { "DS3231", "PCF2129" };
            Serial.print(F("RTC: Detected "));
            Serial.println(tpArr[_rtcType]);
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
void tcRTC::adjust(const DateTime &dt)
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
 * - Allows independent calculation of dayOfWeek
 *
 * (year: 0-99; dayOfWeek: 0=Sun..6=Sat)
 */
void tcRTC::adjust(byte second, byte minute, byte hour, byte dayOfWeek, byte dayOfMonth, byte month, byte year)
{
    uint8_t buffer[8];
    uint8_t statreg;

    switch(_rtcType) {

    case RTCT_PCF2129:
        buffer[0] = PCF2129_TIME;
        buffer[1] = bin2bcd(second);
        buffer[2] = bin2bcd(minute);
        buffer[3] = bin2bcd(hour);
        buffer[4] = bin2bcd(dayOfMonth);
        buffer[5] = bin2bcd(dayOfWeek);
        buffer[6] = bin2bcd(month);
        buffer[7] = bin2bcd(year);

        Wire.beginTransmission(_address);
        for(int i = 0; i < 8; i++) {
            Wire.write(buffer[i]);
        }
        Wire.endTransmission();

        // OSF bit cleared by writing seconds
        break;

    case RTCT_DS3231:
    default:
        buffer[0] = DS3231_TIME;
        buffer[1] = bin2bcd(second);
        buffer[2] = bin2bcd(minute);
        buffer[3] = bin2bcd(hour);
        buffer[4] = bin2bcd(dowToDS3231(dayOfWeek));
        buffer[5] = bin2bcd(dayOfMonth);
        buffer[6] = bin2bcd(month);
        buffer[7] = bin2bcd(year);

        Wire.beginTransmission(_address);
        for(int i = 0; i < 8; i++) {
            Wire.write(buffer[i]);
        }
        Wire.endTransmission();   

        // clear OSF bit
        statreg = read_register(DS3231_STATUS);
        statreg &= ~0x80;
        write_register(DS3231_STATUS, statreg);
        break;
    }

    #ifdef TC_DBG
    Serial.print("RTC: Adjusted to ");
    Serial.print(dayOfMonth, DEC);
    Serial.print("-");
    Serial.print(month, DEC);
    Serial.print("-");
    Serial.print(year+2000, DEC);
    Serial.print(" ");
    Serial.print(hour, DEC);
    Serial.print(":");
    Serial.print(minute, DEC);
    Serial.print(":");
    Serial.print(second, DEC);
    Serial.print(" ");
    Serial.println(dayOfWeek);
    #endif
}

/*
 * Get current date/time
 */
DateTime tcRTC::now() 
{
    uint8_t buffer[7];

    switch(_rtcType) {

    case RTCT_PCF2129:
        Wire.beginTransmission(_address);
        Wire.write(PCF2129_TIME); 
        Wire.endTransmission();
        Wire.requestFrom(_address, (uint8_t)7);
        for(int i = 0; i < 7; i++) {
            buffer[i] = Wire.read();
        }

        return DateTime(bcd2bin(buffer[6]) + 2000U,
                        bcd2bin(buffer[5] & 0x7F),
                        bcd2bin(buffer[3]),
                        bcd2bin(buffer[2]),
                        bcd2bin(buffer[1]),
                        bcd2bin(buffer[0] & 0x7F));

    case RTCT_DS3231:
    default:
        Wire.beginTransmission(_address);
        Wire.write(DS3231_TIME); 
        Wire.endTransmission();
        Wire.requestFrom(_address, (uint8_t)7);
        for(int i = 0; i < 7; i++) {
            buffer[i] = Wire.read();
        }

        return DateTime(bcd2bin(buffer[6]) + 2000U,
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
        Wire.beginTransmission(_address);
        Wire.write((byte)PCF2129_CLKCTRL);  
        Wire.endTransmission();

        Wire.requestFrom(_address, (uint8_t)1);
        readValue = Wire.read();
        // set squarewave to 1Hz
        // Bits 2:0 - 110  sets 1Hz
        readValue &= B11111000;
        readValue |= B11111110;

        Wire.beginTransmission(_address);
        Wire.write((byte)PCF2129_CLKCTRL);
        Wire.write(readValue);
        Wire.endTransmission();
        break;

    case RTCT_DS3231:
    default:
        Wire.beginTransmission(_address);
        Wire.write((byte)DS3231_CONTROL);
        Wire.endTransmission();

        Wire.requestFrom(_address, (uint8_t)1);
        readValue = Wire.read();
        // enable squarewave and set to 1Hz
        // Bit 2 INTCN - 0 enables wave output
        // Bit 3 and 4 - 0 0 sets 1Hz
        readValue &= B11100011;

        Wire.beginTransmission(_address);
        Wire.write((byte)DS3231_CONTROL);
        Wire.write(readValue);
        Wire.endTransmission();
        break;
    }
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
    default:
        Wire.beginTransmission(_address);
        Wire.write(DS3231_TEMP); 
        Wire.endTransmission();
        Wire.requestFrom(_address, (uint8_t)2);
        buffer[0] = Wire.read();
        buffer[1] = Wire.read();
  
        return (float)buffer[0] + (float)(buffer[1] >> 6) * 0.25f;
    }

    return 0.0f;
}

/*
 * Write value to register
 */
void tcRTC::write_register(uint8_t reg, uint8_t val)
{
    Wire.beginTransmission(_address);
    Wire.write(reg);
    Wire.write(val);
    Wire.endTransmission();
}

/*
 * Read value from register
 */
uint8_t tcRTC::read_register(uint8_t reg)
{
    Wire.beginTransmission(_address);
    Wire.write(reg);
    Wire.endTransmission();
    Wire.requestFrom(_address, (uint8_t)1);
    return Wire.read();
}
