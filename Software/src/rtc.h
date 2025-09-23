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

#ifndef _RTC_H_
#define _RTC_H_

/*****************************************************************
 * DateTime Class
 * 
 * Rudimentary general-purpose date/time class, used as a 
 * vehicle to pass date/time between functions.
 ****************************************************************/

class DateTime {

    public:

        DateTime() 
                    { 
                        _y = 2000; _m = 1; _d = 1; 
                        _hh = 0; _mm = 0; _ss = 0; 
                        _minsValid = false; 
                    }
        
        DateTime(uint16_t year, uint8_t month, uint8_t day, 
                 uint8_t hour = 0, uint8_t min = 0, uint8_t sec = 0)
                    {
                        _y  = year;  _m  = month; _d  = day;
                        _hh = hour;  _mm = min;   _ss = sec;
                        _minsValid = false;
                    }

        uint16_t year()   const { return _y; }
        uint8_t  month()  const { return _m; }
        uint8_t  day()    const { return _d; }
        uint8_t  hour()   const { return _hh; }
        uint8_t  minute() const { return _mm; }
        uint8_t  second() const { return _ss; }

        void     set(uint16_t yyy, uint8_t mon, uint8_t ddd, 
                     uint8_t hhh = 0, uint8_t mmm = 0, uint8_t sss = 0) 
                    {
                        _y  = yyy; _m  = mon; _d  = ddd; 
                        _hh = hhh; _mm = mmm; _ss = sss;
                        _minsValid = false;
                    }
        void     setYear(uint16_t yyy) 
                    {
                        _y = yyy;
                        _minsValid = false;
                    }

        uint16_t hwRTCYear = 0;
        
        bool     _minsValid = false;
        uint64_t _mins = 0;

    private:

        uint16_t _y;    // Year 0-9999
        uint8_t  _m;    // Month 1-12
        uint8_t  _d;    // Day 1-31
        uint8_t  _hh;   // Hours 0-23
        uint8_t  _mm;   // Minutes 0-59
        uint8_t  _ss;   // Seconds 0-59
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

        tcRTC(int numTypes, const uint8_t addrArr[]);

        bool begin(unsigned long powerupTime);

        void adjust(byte second, byte minute, byte hour, byte dayOfWeek, byte dayOfMonth, byte month, byte year);

        void now(DateTime& dt);

        void clockOutEnable();

        bool NeedOTPRefresh();
        #ifdef HAVE_PCF2129
        bool OTPRefresh(bool start);
        #endif

        bool lostPower(void);
        bool battLow(void);

        float getTemperature();

    private:

        static uint8_t dowToDS3231(uint8_t d) { return d == 0 ? 7 : d; }

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
