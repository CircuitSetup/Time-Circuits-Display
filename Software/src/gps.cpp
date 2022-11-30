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

#include "tc_global.h"

#ifdef TC_HAVEGPS

#include <Arduino.h>
#include <Wire.h>

#include "gps.h"

#define GPS_MPH_PER_KNOT  1.15077945
#define GPS_KMPH_PER_KNOT 1.852

#ifdef TC_DBG
static int DBGloopCnt = 0;
#endif

static uint8_t parseHex(char c);
static void defaultDelay(unsigned int mydelay);

// Store i2c address
tcGPS::tcGPS(uint8_t address)
{
    _address = address;
}

// Start and init the GPS module
bool tcGPS::begin(unsigned long powerupTime, bool quickUpdates)
{
    uint8_t testBuf;
    int i2clen;
    
    _customDelayFunc = defaultDelay;

    _lineBufIdx = 0;
    _currentline = _lineBufArr[_lineBufIdx];

    _lenIdx = 0;

    fix = false;
    speed = -1;
    _haveSpeed = false;
    _haveDateTime = false;
    _haveDateTime2 = false;

    // Check for GPS module on i2c bus
    Wire.beginTransmission(_address);
    if(Wire.endTransmission(true))
        return false;

    // Give the receiver some time to boot
    unsigned long millisNow = millis();
    if(millisNow - powerupTime < 1000) {
        delay(1000 - (millisNow - powerupTime));
    }

    // Test reading the sensor
    i2clen = Wire.requestFrom(_address, (uint8_t)8);
    if(i2clen == 8) {
        for(int i = 0; i < 8; i++) {
            testBuf = Wire.read();
            // Bail if illegal characters returned
            if(testBuf != 0x0A && testBuf != 0x0D && (testBuf < ' ' || testBuf > 0x7f)) {
                return false;
            }
        }
    } else
        return false;

    // Send xxRMC and xxZDA only
    // If we use GPS for speed, we need more frequent updates.
    // The value in PKT 314 is apparently a multiplier for the value 
    // of PKT 220.
    // For speed we want updates every second for the RMC sentence,
    // so we set the fix update to 1000ms, and the multiplier to 1.
    // For mere time, we set the fix update to 5000, and the
    // multiplier to 1 as well, so we get it every 5th second.
    if(quickUpdates) {
        sendCommand("$PMTK314,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,5,0,0*30");
        // Set update rate to 1000ms
        sendCommand("$PMTK220,1000*1F");
    } else {
        //sendCommand("$PMTK314,0,5,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,5,0,0*34");
        sendCommand("$PMTK314,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0*34");
        // Set update rate to 5000ms
        sendCommand("$PMTK220,5000*1B");
    }

    // No antenna status
    sendCommand("$PGCMD,33,0*6D");

    // Search for GPS, GLONASS, Galileo satellites
    //sendCommand("$PMTK353,1,1,1,0,0*2A");   // GPS, GLONASS, Galileo
    sendCommand("$PMTK353,1,1,0,0,0*2B");     // GPS, GLONASS
    //sendCommand("$PMTK353,1,0,0,0,0*2A");   // GPS

    // Send "hot start" to clear i2c buffer 
    sendCommand("$PMTK101*32");
    delay(800);

    return true;
}

/*
 * loop()
 *
 */
void tcGPS::loop(bool doDelay)
{
    unsigned long myNow = millis();
    #ifdef TC_DBG
    unsigned long myLater;
    #endif

    readAndParse(doDelay);

    // Time needed:
    // read 32 bytes: 9ms
    // read 64 bytes: 12ms

    // Expire time/date info after 15 minutes
    if(_haveDateTime && (myNow - _curTS >= 15*60*1000))
        _haveDateTime = false;
    if(_haveDateTime2 && (myNow - _curTS2 >= 15*60*1000))
        _haveDateTime2 = false;

    // Expire speed info after 2 seconds
    if(_haveSpeed && (myNow - _curspdTS >= 2000))
        _haveSpeed = false;
}

/*
 * Return current speed
 *
 */
int16_t tcGPS::getSpeed()
{
    if(_haveSpeed) return speed;

    return -1;
}


bool tcGPS::haveTime()
{
    return (_haveDateTime || _haveDateTime2);
}

/*
 * Set GPS' RTC time
 * timeinfo returned is in UTC
 */
bool tcGPS::getDateTime(struct tm *timeinfo, unsigned long *fixAge, unsigned long updInt)
{
    char tmp[4] = { 0, 0, 0, 0 };

    if(_haveDateTime) {

        timeinfo->tm_mday = atoi(_curDay);
        timeinfo->tm_mon = atoi(_curMonth) - 1;
        timeinfo->tm_year = atoi(_curYear) - 1900;

        tmp[0] = _curTime[0];
        tmp[1] = _curTime[1];
        timeinfo->tm_hour = atoi(tmp);
        tmp[0] = _curTime[2];
        tmp[1] = _curTime[3];
        timeinfo->tm_min = atoi(tmp);
        tmp[0] = _curTime[4];
        tmp[1] = _curTime[5];
        timeinfo->tm_sec = atoi(tmp);

        timeinfo->tm_wday = 0;

        *fixAge = (unsigned long)(updInt + atoi(_curFrac) + millis() - _curTS);

        return true;

    } else if(_haveDateTime2) {

        timeinfo->tm_mday = atoi(_curDay2);
        timeinfo->tm_mon = atoi(_curMonth2) - 1;
        timeinfo->tm_year = atoi(_curYear2) - 1900;
        
        if(timeinfo->tm_year < (TCEPOCH-1900))
            timeinfo->tm_year += 100;

        tmp[0] = _curTime2[0];
        tmp[1] = _curTime2[1];
        timeinfo->tm_hour  = atoi(tmp);
        tmp[0] = _curTime2[2];
        tmp[1] = _curTime2[3];
        timeinfo->tm_min  = atoi(tmp);
        tmp[0] = _curTime2[4];
        tmp[1] = _curTime2[5];
        timeinfo->tm_sec = atoi(tmp);

        timeinfo->tm_wday = 0;

        *fixAge = (unsigned long)(updInt + atoi(_curFrac) + millis() - _curTS2);

        return true;

    }

    return false;
}

/*
 * Set GPS' RTC time
 * timeinfo needs to be in UTC
 * 
 * Different firmwares support different packet types. 
 * We send both to make sure. If the GPS has updated 
 * its RTC from the GPS signal, "supported, but failed" 
 * is returned; so there is no danger of overwriting 
 * the GPS's RTC with less accurate time.
 * 
 */
bool tcGPS::setDateTime(struct tm *timeinfo)
{
    char pkt335[64] = "$PMTK335";
    char pkt740[64] = "$PMTK740";
    char pktgen[64];
    char temp[8];
    int checksum = 0;

    // If the date is clearly invalid, bail
    if(timeinfo->tm_year + 1900 < TCEPOCH)
        return false;

    sprintf(pktgen, ",%d,%d,%d,%d,%d,%d",
                  timeinfo->tm_year + 1900,
                  timeinfo->tm_mon + 1,
                  timeinfo->tm_mday,
                  timeinfo->tm_hour,
                  timeinfo->tm_min,
                  timeinfo->tm_sec);

    strcat(pkt335, pktgen);
    strcat(pkt740, pktgen);
    
    for(int i = 1; pkt335[i] != 0; i++) {
        checksum ^= pkt335[i];
    }
    sprintf(temp, "*%02x", (checksum & 0xff));
    strcat(pkt335, temp);

    checksum = 0;
    for(int i = 1; pkt740[i] != 0; i++) {
        checksum ^= pkt740[i];
    }
    sprintf(temp, "*%02x", (checksum & 0xff));
    strcat(pkt740, temp);
    
    #ifdef TC_DBG
    Serial.print("GPS setDateTime(): ");
    Serial.println(pkt740);    
    #endif

    sendCommand(pkt740);

    sendCommand(pkt335);

    return true;
}

void tcGPS::setCustomDelayFunc(void (*myDelay)(unsigned int))
{
    _customDelayFunc = myDelay;
}

// Private functions ###########################################################


void tcGPS::sendCommand(const char *str)
{ 
    Wire.beginTransmission(_address);
    for(int i = 0; i < strlen(str); i++) {
        Wire.write((uint8_t)(str[i] & 0xff));
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
    int8_t buff_max = 0;
    int8_t buff_idx = 0;
    bool   haveParsedSome = false;
    unsigned long myNow = millis();

    // Read i2c data in small blocks, call delay between
    // transfers to allow uninterrupted sound playback.

    i2clen = Wire.requestFrom(_address, _lenArr[_lenIdx++]);
    _lenIdx &= GPS_LENBUFLIMIT;

    if(i2clen) {

        // Read i2c data to _buffer
        for(int i = 0; i < i2clen; i++) {
            curr_char = Wire.read();
            // Skip "empty data" (ie LF if not preceeded by CR)
            if((curr_char != 0x0A) || (_last_char == 0x0D)) {
                 _buffer[buff_max++] = curr_char;
            }
            _last_char = curr_char;
        }

        if(doDelay) (*_customDelayFunc)(5);

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
        if(curr_char == 0x0A) {

            // terminate current line
            _currentline[_lineidx] = 0;

            // parse this line
            parseNMEA(_currentline, _currentTS);
            haveParsedSome = true;

            // Shift line buffers
            _currentline = _lineBufArr[_lineBufIdx++];
            _lineBufIdx &= 0x01;
            
            _lineidx = 0;
        }

        // Continue loop and write remaining data into new line

    }

    return haveParsedSome;
}


bool tcGPS::parseNMEA(char *nmea, unsigned long nmeaTS)
{
    char *t = nmea, *t2;
    char buf[16];
    char *bufp = buf;

    if(!checkNMEA(nmea)) {
        #ifdef TC_DBG
        Serial.print(F("parseNMEA: Bad NMEA "));
        Serial.print(nmea);
        //for(int i = 0; i < strlen(nmea); i++) {
        //    Serial.print(nmea[i], HEX);
        //    Serial.print(" ");
        //}
        //Serial.println(" ");
        #endif
        return false;
    }

    #ifdef TC_DBG
    Serial.print(nmea);
    #endif

    if(*(t+3) == 'R') {   // RMC

        t = strchr(t, ',') + 1;  // Skip to time
        strncpy(_curTime2, t, 6);
        if(*(t+6) == '.') {
            strncpy(_curFrac2, t+7, 3);
        } else {
            _curFrac2[0] = 0;
        }
        t = strchr(t, ',') + 1;  // Skip to validity
        fix = (*t == 'A');

        if(!fix) return true;

        t = strchr(t, ',') + 1;  // Skip to lat
        t = strchr(t, ',') + 1;  // Skip to n/s
        t = strchr(t, ',') + 1;  // Skip to long
        t = strchr(t, ',') + 1;  // Skip to e/w
        t2 = t = strchr(t, ',') + 1;  // Skip to speed

        *bufp = 0;
        while(*t2 && *t2 != ',' && (bufp - 16 < buf))
            *bufp++ = *t2++;
        *bufp = 0;
        if(*buf) {
            speed = (int16_t)(round(atof(buf) * GPS_MPH_PER_KNOT));
            if(speed <= 2) speed = 0;
            _haveSpeed = true;
            _curspdTS = nmeaTS;
        }

        t = strchr(t, ',') + 1;  // Skip to track made good
        t = strchr(t, ',') + 1;  // Skip to date
        strncpy(_curDay2, t, 2);
        strncpy(_curMonth2, t+2, 2);
        strncpy(&_curYear2[2], t+4, 2);
          
        _curTS2 = nmeaTS;
        _haveDateTime2 = true;

    } else {      // ZDA

        // Only use ZDA if we have a fix, no point in reading back
        // the GPS' own RTC.
        if(fix) {
            t = strchr(t, ',') + 1; // Skip to time
            strncpy(_curTime, t, 6);
            if(*(t+6) == '.') {
                strncpy(_curFrac, t+7, 3);
            } else {
                _curFrac[0] = 0;
            }
            t = strchr(t, ',') + 1; // Skip to day
            strncpy(_curDay, t, 2);
            t = strchr(t, ',') + 1; // Skip to month
            strncpy(_curMonth, t, 2);
            t = strchr(t, ',') + 1; // Skip to year
            strncpy(_curYear, t, 4);
            _curTS = nmeaTS;
            _haveDateTime = true;
        }

    }

    return true;
}

bool tcGPS::checkNMEA(char *nmea)
{
    uint16_t sum = 0;
    char *p = nmea;

    // check sentence type

    if(*nmea != '$')
        return false;

    if((strncmp(nmea+3, "RMC", 3)) && (strncmp(nmea+3, "ZDA", 3)))
        return false;

    // check checksum

    char *ast = nmea;
    while(*ast)
        ast++;

    while(*ast != '*' && ast > nmea)
        ast--;

    if(*ast != '*')
        return false;

    sum = parseHex(*(ast + 1)) << 4;
    sum += parseHex(*(ast + 2));

    for(char *p1 = p + 1; p1 < ast; p1++)
        sum ^= *p1;

    return (sum == 0);
}

static uint8_t parseHex(char c)
{
    if(c < '0')   return 0;
    if(c <= '9')  return c - '0';
    if(c < 'A')   return 0;
    if(c <= 'F')  return (c - 'A') + 10;
    if(c < 'a')   return 0;
    if(c <= 'f')  return (c - 'a') + 10;
    return 0;
}

static void defaultDelay(unsigned int mydelay)
{
    delay(mydelay);
}
#endif
