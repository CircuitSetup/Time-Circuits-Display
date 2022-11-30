/*
 * -------------------------------------------------------------------
 * CircuitSetup.us Time Circuits Display
 * (C) 2022 Thomas Winischhofer (A10001986)
 * https://github.com/realA10001986/Time-Circuits-Display-A10001986
 *
 * Sensor Class: Temperature and Light Sensor handling
 *
 * This is designed for 
 * - MCP9808-based temperature sensors,
 * - BH1750, TSL2561 and VEML77000 light sensors.
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

#ifndef _TCSENSOR_H
#define _TCSENSOR_H

#if defined(TC_HAVETEMP) || defined(TC_HAVELIGHT)

class tcSensor {
  
    protected:
    
        uint16_t read16(uint16_t regno, bool LSBfirst = false);
        uint8_t  read8(uint16_t regno);
        void     write16(uint16_t regno, uint16_t value, bool LSBfirst = false);
        void     write8(uint16_t regno, uint8_t value);

        uint8_t _address;
};

#endif

#ifdef TC_HAVETEMP    // -----------------------------------------

enum {
    MCP9808 = 0
};

class tempSensor : tcSensor {

    public:

        tempSensor(int numTypes, uint8_t addrArr[]);
        bool begin();

        // Setter for custom delay function
        void setCustomDelayFunc(void (*myDelay)(unsigned int));

        void on();
        void off();
        double readTemp(bool celsius = true);

    private:

        int     _numTypes = 0;
        uint8_t _addrArr[4*2];    // up to 4 sensor types fit here
        int8_t  _st = -1;

        void onoff(bool shutDown);

        // Ptr to custom delay function
        void (*_customDelayFunc)(unsigned int) = NULL;
};
#endif

#ifdef TC_HAVELIGHT   // -----------------------------------------

enum {
    LST_TSL2561 = 0, // 0x29, (0x39, 0x49)
    LST_BH1750,      // 0x23  (0x5c)
    LST_VEML7700     // 0x48, 0x10  !! (used for VEML6030, too)
};

class lightSensor : tcSensor {

    public:

        lightSensor(int numTypes, uint8_t addrArr[]);
        bool begin(bool skipLast);

        // Setter for custom delay function
        void setCustomDelayFunc(void (*myDelay)(unsigned int));

        int32_t readLux();
        #ifdef TC_DBG
        int32_t readDebug();
        #endif
        
        void loop();

    private:
        void VEML7700SetGain(uint16_t gain, bool doWrite = true);
        void VEML7700SetAIT(uint16_t ait, bool doWrite = true);
        void VEML7700OnOff(bool enable, bool doWait = true);

        uint32_t TSL2561CalcLux(uint8_t iGain, uint8_t tInt, uint32_t ch0, uint32_t ch1);

        int     _numTypes = 0;
        uint8_t _addrArr[6*2];    // up to 6 sensor types fit here
        int8_t  _st = -1;

        unsigned long _lastAccess;
        uint8_t _accessNum = 0;

        uint8_t _gainIdx;
        uint8_t _AITIdx;

        int32_t _lux = -1;

        uint16_t _con0 = 0;

        // Ptr to custom delay function
        void (*_customDelayFunc)(unsigned int) = NULL;
};

#endif

#endif  // _TCSENSOR_H
