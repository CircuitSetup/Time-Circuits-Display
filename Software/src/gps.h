/*
 * -------------------------------------------------------------------
 * CircuitSetup.us Time Circuits Display
 * (C) 2022-2025 Thomas Winischhofer (A10001986)
 * https://github.com/realA10001986/Time-Circuits-Display
 * https://tcd.out-a-ti.me
 *
 * GPS Class: GPS receiver handling and data parsing
 *
 * This is designed for MTK3333-based modules.
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

#ifndef _tcGPS_H
#define _tcGPS_H

#define GPS_MAX_I2C_LEN   255
#define GPS_MAXLINELEN    128

class tcGPS {

    public:

        tcGPS(uint8_t address);
        bool    begin(unsigned long powerupTime, int quickUpdates, int speedRate, void (*myDelay)(unsigned long));

        void    loop(bool doDelay);

        int16_t getSpeed();
        bool    haveTime();
        bool    getDateTime(struct tm *timeInfo, unsigned long *fixAge, unsigned long updInt);
        bool    setDateTime(struct tm *timeinfo);
        
        bool    fix = false;

    private:

        bool    readAndParse(bool doDelay = true);

        void    sendCommand(const char *, const char *);

        bool    parseNMEA(char *nmea, unsigned long nmeaTS);
        bool    checkNMEA(char *nmea);

        uint8_t _address;

        #define GPS_LENBUFLIMIT 0x03
        uint8_t _lenArr[4] = { 64, 64, 64, 63 };
        //#define GPS_LENBUFLIMIT 0x07
        //uint8_t _lenArr[8] = { 32, 32, 32, 32, 32, 32, 32, 31 };
        int     _lenIdx = 0;
        int     _lenLimit = GPS_LENBUFLIMIT;

        char    _buffer[GPS_MAX_I2C_LEN];
        char    _last_char = 0;

        char    _line1[GPS_MAXLINELEN];
        char    *_currentline;
        uint8_t _lineidx = 0;
        unsigned long _currentTS = 0;

        int     _speed = -1;
        bool    _haveSpeed = false;
        unsigned long _curspdTS = 0;
        char     _sbuf[16];

        char    _curTime[8]   = { 0, 0, 0, 0, 0, 0, 0, 0 };
        unsigned long _curFrac;
        uint8_t  _curDay;
        uint8_t  _curMonth;
        uint16_t _curYear;
        char    _curTime2[8]  = { 0, 0, 0, 0, 0, 0, 0, 0 };
        unsigned long _curFrac2;
        uint8_t  _curDay2;
        uint8_t  _curMonth2;
        uint16_t _curYear2;
        unsigned long _curTS = 0;
        unsigned long _curTS2 = 0;
        bool    _haveDateTime = false;
        bool    _haveDateTime2 = false;

        // Ptr to custom delay function
        void (*_customDelayFunc)(unsigned long) = NULL;
};

#endif
