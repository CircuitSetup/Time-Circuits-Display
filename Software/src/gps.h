/*
 * -------------------------------------------------------------------
 * CircuitSetup.us Time Circuits Display
 * (C) 2022 Thomas Winischhofer (A10001986)
 * https://github.com/realA10001986/Time-Circuits-Display-A10001986
 *
 * GPS Class: GPS receiver handling and data parsing
 *
 * This is designed for MTK3333-based modules.
 * 
 * Some ideas taken from the Adafruit_GPS Library
 * https://github.com/adafruit/Adafruit_GPS
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

#ifndef _tcGPS_H
#define _tcGPS_H

#define GPS_MAX_I2C_LEN   255
#define GPS_MAXLINELEN    256

class tcGPS {

    public:

        tcGPS(uint8_t address);
        bool    begin(unsigned long powerupTime, bool quickUpdates);

        // Setter for custom delay function
        void    setCustomDelayFunc(void (*myDelay)(unsigned int));

        void    loop(bool doDelay);

        int16_t getSpeed();
        bool    haveTime();
        bool    getDateTime(struct tm *timeInfo, unsigned long *fixAge, unsigned long updInt);
        bool    setDateTime(struct tm *timeinfo);

        int16_t speed = -1;
        bool    fix = false;

    private:

        bool    readAndParse(bool doDelay = true);

        void    sendCommand(const char *);

        bool    parseNMEA(char *nmea, unsigned long nmeaTS);
        bool    checkNMEA(char *nmea);

        uint8_t _address;

        #define GPS_LENBUFLIMIT 0x03
        uint8_t _lenArr[32] = { 64, 64, 64, 63 };
        //#define GPS_LENBUFLIMIT 0x07
        //uint8_t _lenArr[32] = { 32, 32, 32, 32, 32, 32, 32, 31 };
        int     _lenIdx = 0;

        char    _buffer[GPS_MAX_I2C_LEN];
        char    _last_char = 0;

        char    _line1[GPS_MAXLINELEN];
        char    _line2[GPS_MAXLINELEN];
        char    *_lineBufArr[4] = { _line1, _line2 };
        int     _lineBufIdx = 0;
        char    *_currentline;
        uint8_t _lineidx = 0;
        unsigned long _currentTS = 0;

        bool    _haveSpeed = false;
        unsigned long _curspdTS = 0;

        char    _curTime[8]   = { 0, 0, 0, 0, 0, 0, 0, 0 };
        char    _curFrac[4]   = { 0, 0, 0, 0 };
        char    _curDay[4]    = { 0, 0, 0, 0 };
        char    _curMonth[4]  = { 0, 0, 0, 0 };
        char    _curYear[6]   = { 0, 0, 0, 0, 0, 0 };
        char    _curTime2[8]  = { 0, 0, 0, 0, 0, 0, 0, 0 };
        char    _curFrac2[4]  = { 0, 0, 0, 0 };
        char    _curDay2[4]   = { 0, 0, 0, 0 };
        char    _curMonth2[4] = { 0, 0, 0, 0 };
        char    _curYear2[6]  = { '2', '0', 0, 0, 0, 0 };
        unsigned long _curTS = 0;
        unsigned long _curTS2 = 0;
        bool    _haveDateTime = false;
        bool    _haveDateTime2 = false;

        // Ptr to custom delay function
        void (*_customDelayFunc)(unsigned int) = NULL;
};

#endif
