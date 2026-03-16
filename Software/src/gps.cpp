/*
 * -------------------------------------------------------------------
 * CircuitSetup.us Time Circuits Display
 * (C) 2022-2026 Thomas Winischhofer (A10001986)
 * https://github.com/realA10001986/Time-Circuits-Display
 * https://tcd.out-a-ti.me
 *
 * GPS Class: GPS receiver handling and data parsing
 *
 * This is designed for MTK3333-based modules with i2c communication
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

#ifdef TC_HAVEGPS

#include <Arduino.h>
#include <Wire.h>
#include "gps.h"

#define GPS_MPH_PER_KNOT  1.15077945f
#define GPS_KMPH_PER_KNOT 1.852f

// For deeper debugging
#ifdef TC_DBG_GPS
#define TC_DBG_PRINT_NMEA
//#define GPS_SPEED_SIMU
#endif

#ifdef TC_DBG_GPS
static int DBGloopCnt = 0;
#endif

static const struct GPSsetupStruct {
    uint8_t rmc;
    uint8_t vtg;
    uint8_t zda;
    const char *cmd2;
} GPSSetup[7] = {
    {  1, 0,  1, "5000*1B" }, // Time only: 5000ms, RMC & ZDA every 5th second
    {  1, 0,  5, "1000*1F" }, // BTTFN-clients, no speedo, slow: 1000ms, RMC 1x/sec, ZDA every 5th second
    {  1, 0, 20,  "500*2B" }, // BTTFN-clients, no speedo, fast:  500ms, RMC 2x/sec, ZDA every 10th second
    {  1, 0,  5, "1000*1F" }, // W/speedo, 1000ms: RMC 1x per sec, ZDA every 5th second
    {  1, 0, 20,  "500*2B" }, // W/speedo, 500ms:  RMC 2x per sec, ZDA every 10th second
    { 25, 1, 40,  "250*29" }, // W/speedo, 250ms:  VTG 4x per sec, RMC every approx 6th sec, ZDA every 10th second
    { 31, 1, 50,  "200*2C" }  // W/speedo, 200ms:  VTG 5x per sec, RMC every approx 6th sec, ZDA every 10th second
};

/*
 * Helpers
 */

static void defaultDelay(unsigned long mydelay)
{
    delay(mydelay);
}

static uint8_t parseHex(char c)
{
    if(c <= '9') {
        if(c >= '0') return c - '0';
    } else if(c <= 'F') {
        if(c >= 'A') return c - 'A' + 10;
    } else if(c <= 'f') {
        if(c >= 'a') return c - 'a' + 10;
    }
    return 0;
}

static uint8_t AToI2(char *s)
{ 
    return ((*s - '0') * 10) + (*(s+1) - '0');
}

static uint16_t AToI4(char *s)
{
    return (AToI2(s) * 100) + AToI2(s+2);
}

static unsigned long getFracs(char *s)
{   
    // We only care about 10ths, everything
    // beyond is a waste of ... time.
    if(*s++ == '.') {
        if(*s >= '0' && *s <= '9') {
            return (*s - '0') * 100;
        }
    }
    return 0;
}

static float spdFromStr(char *s)
{
    int r = 0;
    int f = 2;
    int df = 0;
    float rr;
    
    while(*s && f) {
        if(*s >= '0' && *s <= '9') {
            r *= 10;
            r += *s++ - '0';
            f -= df;
        } else if(*s == '.') {
            s++;
            df = 1;
        } else {
            return 0.0f;
        }
    }

    rr = r;
    
    if(!f)     return rr / 100.0f;
    if(f == 1) return rr / 10.0f;
    return rr;
}

static void copy8chars(char *a, char *b)
{   
#ifdef ESP32
    *(uint32_t *)a = *(uint32_t *)b;
    a += 4; b += 4;
    *(uint32_t *)a = *(uint32_t *)b;
#else
    memcpy((void *)a, (void *)b, 6);
#endif
}

static void copy12chars(char *a, char *b)
{
#ifdef ESP32
    *(uint32_t *)a = *(uint32_t *)b;
    a += 4; b += 4;
    *(uint32_t *)a = *(uint32_t *)b;
    a += 4; b += 4;
    *(uint32_t *)a = *(uint32_t *)b;
#else
    memcpy((void *)a, (void *)b, 10);
#endif
}

static void calcNMEAcheckSum(char *cmdbuf)
{
    int checksum = 0;

    cmdbuf++;
    while(*cmdbuf) {
        checksum ^= *cmdbuf++;
    }
    
    sprintf(cmdbuf, "*%02X", checksum);
}

/*
 * Class functions
 */

// Store i2c address
tcGPS::tcGPS(const uint8_t *addrArr)
{
    _addrArr = addrArr;
}

// Start and init the GPS module
bool tcGPS::begin(int numTypes, int quickUpdates, int speedRate, void (*myDelay)(unsigned long))
{
    char cmdbuf[64];
    int i2clen;
    int idx;
    
    _customDelayFunc = defaultDelay;

    speedRate &= 3;

    // Give the receiver some time to boot
    unsigned long millisNow = millis();
    if(millisNow < 1000) {
        delay(1000 - millisNow);
    }

    _type = 0;
    bool found = false;

    for(int i = 0; i < numTypes*2; i += 2) {

        // Check for GPS module on i2c bus
        Wire.beginTransmission(_addrArr[i]);
        if(!(Wire.endTransmission())) {

            _address = _addrArr[i];
            _type = _addrArr[i+1];

            // Test reading the sensor
            switch(_type) {
            case GPST_MTK333X:
                i2clen = Wire.requestFrom(_address, (uint8_t)8);
                if(i2clen == 8) {
                    found = true;
                    for(int i = 0; i < 8; i++) {
                        uint8_t testBuf = Wire.read();
                        // Bail if illegal characters returned
                        if(testBuf != 0x0a && testBuf != 0x0d && (testBuf < ' ' || testBuf > 0x7e)) {
                            found = false;
                        }
                    }
                }
                break;
            default:
                return false;
            }

            if(found)
                break;
        }
    }

    if(!found)
        return false;
    
    _buffer = (char *)malloc(GPS_MAX_I2C_LEN + GPS_MAXLINELEN);
    if(!_buffer) return false;

    _line1 = _buffer + GPS_MAX_I2C_LEN;

    _currentline = _line1;
    _lenIdx = 0;

    // Configure receiver
    switch(_type) {
    case GPST_MTK333X:
        // Send xxRMC and xxZDA only, for high rates also xxVTG
        // If we use GPS for speed, we need more frequent updates.
        // The value in PKT 314 is a multiplier for the value of PKT 220.
        if(quickUpdates < 0) {
            idx = 0;
        } else if(!quickUpdates) {
            idx = (!speedRate) ? 1 : 2;
        } else {
            idx = speedRate + 3;
            // For reading 128 bytes per call:
            _lenLimit = 0x01;
            _lenArr[0] = 128; _lenArr[1] = 127;
        }
    
        sprintf(cmdbuf, "$PMTK314,0,%d,%d,0,0,0,0,0,0,0,0,0,0,0,0,0,0,%d,0,0", 
              GPSSetup[idx].rmc, GPSSetup[idx].vtg, GPSSetup[idx].zda);
        calcNMEAcheckSum(cmdbuf);
        sendCommand(NULL, cmdbuf);
        sendCommand("$PMTK220,", GPSSetup[idx].cmd2);
    
        #ifdef TC_DBG_PRINT_NMEA
        // Enable leap second indicate; RMC and ZDA are
        // corrected already, no need to take leap
        // seconds into account here.
        // (enable; setting is persistent!)
        //sendCommand(NULL, "$PMTK875,1,1*38");
        // (disable)
        //sendCommand(NULL, "$PMTK875,1,0*39");
        #endif
    
        // Search for satellites
        //sendCommand(NULL, "$PMTK353,1,1,1,0,0*2A");   // GPS, GLONASS, Galileo
        sendCommand(NULL, "$PMTK353,1,1,0,0,0*2B");     // GPS, GLONASS
        //sendCommand(NULL, "$PMTK353,1,0,0,0,0*2A");   // GPS
    
        // Send "hot start" to clear i2c buffer 
        //sendCommand(NULL, "$PMTK101*32");
        //delay(800);
        // No, might lose fix in the process; empty
        // buffer by reading it
        for(int i = 0; i <= _lenLimit; i++) {
            readAndParse(false);
        }
        break;
    default:
        return false;
    }

    _customDelayFunc = myDelay;

    return true;
}

/*
 * loop()
 *
 */
void tcGPS::loop(bool doDelay)
{
    unsigned long myNow;
    
    #ifdef TC_DBG_GPS
    myNow = millis();
    #endif

    readAndParse(doDelay);
    // takes (with delay):
    // 6ms? when reading blocks of 32 bytes
    // 8ms " " " 64 bytes   (6ms without delay)  (ex: 11 w/o)
    // 13ms " " " 128 bytes  (12ms without delay) (ex: 17 w/o)

    #ifdef TC_DBG_GPS
    if((DBGloopCnt++) % 10 == 0) {
        Serial.printf("readAndParse took %d ms\n", millis()-myNow);
    }
    #endif

    myNow = millis();

    // Speed "simulator" for debugging
    #ifdef GPS_SPEED_SIMU
    _haveSpeed = true;        
    _curspdTS = myNow;

    //_speed++;
    //if(_speed > 87) _speed = 0;
    _speed = 33;
    #endif

    // Expire time/date info after 15 minutes
    if(_haveDateTime && (myNow - _curTS >= 15*60*1000))
        _haveDateTime = false;
    if(_haveDateTime2 && (myNow - _curTS2 >= 15*60*1000))
        _haveDateTime2 = false;

    // Expire speed info after 2 seconds
    if(_haveSpeed && (myNow - _curspdTS >= 2000))
        _haveSpeed = false;

    // Expire pos info after 5 minutes
    if(_havePos && (myNow - _curposTS >= 5*60*1000))
        _havePos = false;

    // Read Quectel firmware version
    if(!triedQ && *model) {
        if((*model == 'Q') && ((model[3] & ~0x20) == 'C')) {
            sendCommand(NULL, "$PQVERNO,R*3F");
        }
        triedQ = true;
    }
}

/*
 * Return current speed
 *
 */
int tcGPS::getSpeed()
{
    float speedfk;
    
    if(_haveSpeed) {
      
        #ifdef GPS_SPEED_SIMU
        
        // Speed "simulator" for debugging
        //_speed++;
        // if(_speed >= 88) _speed &= 0x03;
        
        #else
        
        // Fake 1 and 2mph; GPS is not reliable at
        // low speeds, need to ignore everything below
        // 2.17(=2.5mph) as this much might appear even
        // at stand-still.
        speedfk = spdFromStr(_sbuf);
        if(speedfk >= 2.61f) {        // 3.00mph
            _speed = (int)roundf(speedfk * GPS_MPH_PER_KNOT);
            if(_speed > 127) _speed = 127;
        } else if(speedfk <= 2.17f) { // 2.50mph
            _speed = 0;
        } else if(speedfk <= 2.40f) { // 2.76mph
            _speed = 1;
        } else {
            _speed = 2;
        }
        
        #endif // GPS_SPEED_SIMU
        
        return _speed;
    }

    return -1;
}

/*
 * Get GPS time
 * timeinfo returned is UTC
 */
bool tcGPS::getDateTime(struct tm *timeinfo, unsigned long *fixAge, unsigned long updInt)
{

    if(_haveDateTime) {

        timeinfo->tm_mday = AToI2(&_curDate[0]);
        timeinfo->tm_mon  = AToI2(&_curDate[3]) - 1;
        timeinfo->tm_year = AToI4(&_curDate[6]) - 1900;

        timeinfo->tm_hour = AToI2(&_curTime[0]);
        timeinfo->tm_min  = AToI2(&_curTime[2]);
        timeinfo->tm_sec  = AToI2(&_curTime[4]);
        
        timeinfo->tm_wday = 0;

        #ifdef TC_DBG_GPS
        Serial.printf("GPS time: %d %d %d - %d %d %d\n", 
            timeinfo->tm_mday,
            timeinfo->tm_mon,
            timeinfo->tm_year,
            timeinfo->tm_hour,
            timeinfo->tm_min,
            timeinfo->tm_sec);
        #endif

        *fixAge = (unsigned long)(millis() - _curTS + _curFrac + updInt);

        return true;

    } else if(_haveDateTime2) {
        
        timeinfo->tm_mday = AToI2(&_curDate2[0]);
        timeinfo->tm_mon  = AToI2(&_curDate2[2]) - 1;
        timeinfo->tm_year = AToI2(&_curDate2[4]) + 2000 - 1900;

        /* Can't do that, it clashes with our roll-over
         * detection; the roll-over problem is way more
         * acute than the century.
        if(timeinfo->tm_year < (TCEPOCH_GEN-1900))
            timeinfo->tm_year += 100;
        */
        
        timeinfo->tm_hour = AToI2(&_curTime2[0]);
        timeinfo->tm_min  = AToI2(&_curTime2[2]);
        timeinfo->tm_sec  = AToI2(&_curTime2[4]);

        timeinfo->tm_wday = 0;

        #ifdef TC_DBG_GPS
        Serial.printf("GPS time2: %d %d %d - %d %d %d\n", 
            timeinfo->tm_mday,
            timeinfo->tm_mon,
            timeinfo->tm_year,
            timeinfo->tm_hour,
            timeinfo->tm_min,
            timeinfo->tm_sec);
        #endif

        *fixAge = (unsigned long)(millis() - _curTS2 + _curFrac2 + updInt);

        return true;

    }

    return false;
}

/*
 * Set GPS' RTC time
 * timeinfo needs to be in UTC
 * 
 * I don't know what the difference between 335 and
 * 740 is. We send both to make sure. If the GPS has 
 * updated its RTC from the GPS signal, "supported, 
 * but failed" (2) is returned; so there is no danger 
 * of overwriting the GPS's RTC.
 * 
 */
bool tcGPS::setDateTime(struct tm *timeinfo)
{
    char pkt335[64] = "$PMTK335";
    char pkt740[64] = "$PMTK740";
    char pktgen[64];

    // If the date is clearly invalid, bail
    if(timeinfo->tm_year + 1900 < TCEPOCH_GEN)
        return false;

    switch(_type) {
    case GPST_MTK333X:
        // Generate command string
        sprintf(pktgen, ",%d,%d,%d,%d,%d,%d",
                      timeinfo->tm_year + 1900,
                      timeinfo->tm_mon + 1,
                      timeinfo->tm_mday,
                      timeinfo->tm_hour,
                      timeinfo->tm_min,
                      timeinfo->tm_sec);
    
        strcat(pkt335, pktgen);
        strcat(pkt740, pktgen);
    
        calcNMEAcheckSum(pkt335);
        calcNMEAcheckSum(pkt740);
        
        sendCommand(NULL, pkt740);
        sendCommand(NULL, pkt335);
    
        #ifdef TC_DBG_GPS
        Serial.printf("GPS setDateTime(): %s\n", pkt740);  
        #endif
    
        return true;
    default:
        return false;
    }
}

const char *tcGPS::getPos(bool& cur)
{
    if(_havePos) {
        cur = fix;
        return _curPos;
    }

    return NULL;
}

void tcGPS::requestVersion()
{
    switch(_type) {
    case GPST_MTK333X:
        sendCommand(NULL, "$PMTK605*31");
        break;
    default:
        break;
    }
}

// Private functions ###########################################################


void tcGPS::sendCommand(const char *prefix, const char *str)
{ 
    Wire.beginTransmission(_address);
    if(prefix) {
        for(int i = 0; i < strlen(prefix); i++) {
            Wire.write((uint8_t)prefix[i]);
        }
    }
    for(int i = 0; i < strlen(str); i++) {
        Wire.write((uint8_t)str[i]);
    }
    Wire.write(0x0d);
    Wire.write(0x0a);
    Wire.endTransmission(true);
    (*_customDelayFunc)(30);
}

bool tcGPS::readAndParse(bool doDelay)
{
    char   curr_char = 0;
    size_t i2clen = 0;
    int    buff_max = 0;
    int    buff_idx = 0;
    bool   haveParsedSome = false;
    unsigned long myNow = millis();

    switch(_type) {
    case GPST_MTK333X:
        i2clen = Wire.requestFrom(_address, _lenArr[_lenIdx++]);
        _lenIdx &= _lenLimit;
    
        if(i2clen) {
    
            // Read i2c data to _buffer
            for(int i = 0; i < i2clen; i++) {
                curr_char = Wire.read();
                // Skip "empty data" (ie LF if not preceeded by CR)
                if((curr_char != 0x0a) || (_last_char == 0x0d)) {
                     _buffer[buff_max++] = curr_char;
                }
                _last_char = curr_char;
            }
    
            if(doDelay) (*_customDelayFunc)(1);
    
        }
        break;
    default:
        return false;
    }

    // Form line(s) out of buffer data

    buff_max--;
    buff_idx = 0;
    while(buff_idx <= buff_max) {
      
        if(!_lineidx) _currentTS = myNow;
        
        curr_char = _currentline[_lineidx++] = _buffer[buff_idx++];

        if(_lineidx >= GPS_MAXLINELEN) {
            _lineidx = GPS_MAXLINELEN - 1;
        }

        // Finish a line on LF
        if(curr_char == 0x0a) {

            // terminate current line
            _currentline[_lineidx] = 0;

            // parse this line
            parseNMEA(_currentline, _lineidx, _currentTS);
            haveParsedSome = true;
            
            _lineidx = 0;
        }

        // Continue loop and write remaining data into new line

    }

    return haveParsedSome;
}

#ifdef TC_DBG_PRINT_NMEA
unsigned long lastMillis = 0;
#endif

bool tcGPS::parseNMEA(char *nmea, unsigned int len, unsigned long nmeaTS)
{
    #define MAXFIELDS 12
    char *t = nmea;
    char *bufp = _sbuf;
    char *idx[MAXFIELDS];
    int  numfields = MAXFIELDS;
    int type = 9; // set invalid
    unsigned long i;

    if(len < 20)
        return false;

    if(*nmea != '$')
        return false;

    if(checkNMEA(nmea, len, &idx[0], numfields, type)) {
        #ifdef TC_DBG_GPS
        Serial.printf("parseNMEA: Bad NMEA (%d): %s", len, nmea);
        #endif
        return false;
    }

    numfields = MAXFIELDS - numfields;

    #ifdef TC_DBG_PRINT_NMEA
    Serial.printf("%dms ", millis() - lastMillis);
    lastMillis = millis();
    Serial.print(nmea);
    #endif

    switch(type) {

    case 0:
        //        0    1  3 4    5 6    7 8
        // $GNVTG,0.00,T,,M,0.00,N,0.00,K,N*2C
        // $GPVTG,140.88,T,,M,8.04,N,14.89,K,D*05

        if(numfields < 9)
            return false;

        fix = (*idx[8] != 'N');
        if(!fix) return true;

        t = idx[4];
        i = idx[5] - t - 1;
        if(i && i < 16) {
            while(i--) *bufp++ = *t++;
            *bufp = 0;
            _haveSpeed = true;
            _curspdTS = nmeaTS;
        }
        break;
        
    case 1:
        //        0          1     6    7    8
        // $GNRMC,213928.720,V,,,,,0.00,0.00,221225,,,N*51
        // $GPRMC,123519,A,4807.038,N,01131.000,E,022.4,084.4,230394,003.1,W*6A

        if(numfields < 12)
            return false;

        fix = (*idx[1] == 'A');
        if(!fix) return true;

        // We only use the RMC time if there is no ZDA.
        if(!_haveDateTime) {
            t = idx[0];
            copy8chars(_curTime2, t); // 6 needed, 8 faster
            _curFrac2 = getFracs(t + 6);
            copy8chars(_curDate2, idx[8]); // 6 needed, 8 faster
            _curTS2 = nmeaTS;
            _haveDateTime2 = true;
        }
        
        t = idx[6];
        i = idx[7] - t - 1;
        if(i && i < 16) {
            while(i--) *bufp++ = *t++;
            *bufp = 0;
            _haveSpeed = true;
            _curspdTS = nmeaTS;
        }

        t = idx[2];
        i = idx[6] - t - 1;
        if(i > 7 && i < 32) {
            bufp = _curPos;
            while(i--) *bufp++ = *t++;
            *bufp = 0;
            _havePos = true;
            _curposTS = nmeaTS;
        } else {
            _havePos = false;
        }
        
        break;
        
    case 2:
        //        0          1  2  3
        // $GNZDA,213926.720,22,12,2025,,*46
        // $GPZDA,172809.456,12,07,1996,00,00*45

        if(numfields < 6)
            return false;

        // Only use ZDA if we have a fix, no point in reading back
        // the GPS' own RTC.
        if(fix) {
            t = idx[0];
            copy8chars(_curTime, t); // 6 needed, 8 faster
            _curFrac = getFracs(t + 6);
            copy12chars(_curDate, idx[1]); // 10 needed, 12 faster
            _curTS = nmeaTS;
            _haveDateTime = true;
            _haveDateTime2 = false;
        }
        break;

    case 3:
        //         0                       1    2            3
        //$PMTK705,AXN_5.1.6_3333_19010218,0007,Quectel-L76L,1.0*53
        
        if(numfields < 4)
            return false;

        t = idx[0];
        if((i = idx[1] - t - 1)) {
            if(i >= sizeof(axnfw)) i = sizeof(axnfw) - 1;
            bufp = axnfw;
            while(i--) *bufp++ = *t++;
            *bufp = 0;
        }
        t = idx[2];
        if((i = idx[3] - t - 1)) {
            if(i >= sizeof(model)) i = sizeof(model) - 1;
            bufp = model;
            while(i--) *bufp++ = *t++;
            *bufp = 0;
        }

        Serial.printf("GPS receiver '%s', firmware '%s'\n", model, axnfw);
        break;

    case 4:
        // .        0 1             2          3
        // $PQVERNO,R,L76LNR02A02SC,2019/06/28,04:29*6C
    
        if(numfields < 4)
            return false;

        t = idx[1];
        if((i = idx[2] - t - 1)) {
            if(i >= sizeof(qfw)) i = sizeof(qfw) - 1;
            bufp = qfw;
            while(i--) *bufp++ = *t++;
            *bufp = 0;
        }

        Serial.printf("GPS receiver Quectel firmware '%s'\n", qfw);
        break;
    }

    return true;
}

uint32_t tcGPS::checkNMEA(char *nmea, unsigned int len, char **pidx, int& maxfields, int& type)
{
    uint32_t sum = 0;
    char *p = nmea;

    #ifdef ESP32
    uint32_t c = *(uint32_t *)(nmea + 3);
    if(c == 0x2c475456)
        type = 0; // VTG
    else if(c == 0x2c434d52)
        type = 1; // RMC
    else if(c == 0x2c41445a)
        type = 2; // ZDA
    else if(c == 0x30374b54)
        type = 3; // PKTK705 response
    else if(c == 0x4e524556)
        type = 4; // PQVERNO response
    else
        return 1;
    #else
    if(!strncmp(nmea+3, "VTG", 3))
        type = 0;
    else if(!strncmp(nmea+3, "RMC", 3))
        type = 1;
    else if(!strncmp(nmea+3, "ZDA", 3))
        type = 2;
    else if(!strncmp(nmea+3, "TK7", 3))
        type = 3;
    else if(!strncmp(nmea+3, "VER", 3))
        type = 4;
    else
        return 1;
    #endif

    // check checksum
    
    char *ast = nmea + len - 5; // *XX<cr><lf><zero>
    if(*ast != '*')
        return 1;

    sum = parseHex(*(ast+1)) << 4;
    sum += parseHex(*(ast+2));

    for(p++; p < ast; p++) {
        sum ^= *p;
        if(*p == ',' && maxfields) {
            *pidx++ = p + 1;
            maxfields--;
        }
    }

    return sum;
}

#endif
