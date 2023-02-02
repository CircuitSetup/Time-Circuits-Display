/*
 * -------------------------------------------------------------------
 * CircuitSetup.us Time Circuits Display
 * (C) 2022-2023 Thomas Winischhofer (A10001986)
 * https://github.com/realA10001986/Time-Circuits-Display-A10001986
 * 
 * RTC Class (DS3231/PCF2129 RTC handling) and DateTime Class
 * 
 * DateTime mirrors the features of RTC; it only works for dates
 * from 1/1/2000 to 31/12/2099. Idea taken from RTCLib by
 * Adafruit.
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

#ifndef _RTC_H_
#define _RTC_H_

/*****************************************************************
 * DateTime Class
 * 
 * Rudimentary general-purpose date/time class, used as a 
 * vehicle to pass date/time between functions.
 * 
 * Supports dates in the range from 1 Jan 2000 to 31 Dec 2099.
 ****************************************************************/

class DateTime {

    public:

        DateTime();
        DateTime(uint16_t year, uint8_t month, uint8_t day, 
                 uint8_t hour = 0, uint8_t min = 0, uint8_t sec = 0);
        DateTime(const DateTime &copy);

        uint16_t year()   const { return 2000U + yOff; }
        uint8_t  month()  const { return m; }
        uint8_t  day()    const { return d; }
        uint8_t  hour()   const { return hh; }

        uint8_t  minute() const { return mm; }
        uint8_t  second() const { return ss; }

        uint8_t  dayOfTheWeek() const;

    protected:

        uint8_t yOff; // Year offset from 2000
        uint8_t m;    // Month 1-12
        uint8_t d;    // Day 1-31
        uint8_t hh;   // Hours 0-23
        uint8_t mm;   // Minutes 0-59
        uint8_t ss;   // Seconds 0-59
};


/****************************************************************
 * tcRTC Class
 ****************************************************************/

enum  {
    RTCT_DS3231 = 0,
    RTCT_PCF2129
};

class tcRTC
{
    public:

        tcRTC(int numTypes, uint8_t addrArr[]);

        bool begin(unsigned long powerupTime);

        void adjust(const DateTime &dt);
        void adjust(byte second, byte minute, byte hour, byte dayOfWeek, byte dayOfMonth, byte month, byte year);

        DateTime now();

        void clockOutEnable();

        bool NeedOTPRefresh();
        bool OTPRefresh(bool start);

        bool lostPower(void);
        bool battLow(void);

        float getTemperature();

        static uint8_t dowToDS3231(uint8_t d) { return d == 0 ? 7 : d; }

    private:

        uint8_t read_register(uint8_t reg);
        void    write_register(uint8_t reg, uint8_t val);
        void    read_bytes(uint8_t reg, uint8_t *buf, uint8_t num);
        void    write_bytes(uint8_t *buffer, uint8_t num);
        static uint8_t bcd2bin(uint8_t val) { return val - 6 * (val >> 4); }
        static uint8_t bin2bcd(uint8_t val) { return val + 6 * (val / 10); }
        
        int     _numTypes = 0;
        uint8_t _addrArr[2*2];
        uint8_t _address;
        uint8_t _rtcType = RTCT_DS3231;
};

#endif
