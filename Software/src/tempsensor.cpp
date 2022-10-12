/*
 * -------------------------------------------------------------------
 * CircuitSetup.us Time Circuits Display
 * (C) 2022 Thomas Winischhofer (A10001986)
 *
 * Temperature Sensor handling
 *
 * This is designed for MCP9808-based sensors.
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

#ifdef TC_HAVETEMP

#include "tempsensor.h"

static void defaultDelay(unsigned int mydelay);

// Mode Resolution SampleTime
//  0    0.5°C       30 ms
//  1    0.25°C      65 ms
//  2    0.125°C     130 ms
//  3    0.0625°C    250 ms
#define TC_TEMP_RESOLUTION 2
uint16_t wakeDelay[4] = { 30, 65, 130, 250 };


// Store i2c address
tempSensor::tempSensor(uint8_t address)
{
    _address = address;
}

// Start the display
bool tempSensor::begin()
{
    _customDelayFunc = defaultDelay;

    // Check if supported sensor is connected
    if(read16(MCP9808_REG_MANUF_ID) != 0x0054)
        return false;
    if(read16(MCP9808_REG_DEVICE_ID) != 0x0400)
        return false;

    // Write config register
    write16(MCP9808_REG_CONFIG, 0x0);

    // Set resolution
    write8(MCP9808_REG_RESOLUTION, TC_TEMP_RESOLUTION & 0x03);

    on();

    return true;
}

void tempSensor::setCustomDelayFunc(void (*myDelay)(unsigned int))
{
    _customDelayFunc = myDelay;
}

// Turn on the sensor
void tempSensor::on()
{
    onoff(false);
}

// Turn off the sensor
void tempSensor::off()
{
    onoff(true);
}


double tempSensor::readTemp(bool celsius)
{
    double temp = NAN;
    uint16_t t = read16(MCP9808_REG_AMBIENT_TEMP);

    if(t != 0xffff) {
        temp = ((double)(t & 0x0fff)) / 16.0;
        if(t & 0x1000) temp = 256.0 - temp;
        if(!celsius) temp = temp * 9.0 / 5.0 + 32.0;
    }

    #ifdef TC_DBG
    Serial.print("Read temp: ");
    Serial.println(temp);
    #endif

    return temp;
}

// Private functions ###########################################################

void tempSensor::onoff(bool shutDown)
{
    uint16_t temp = read16(MCP9808_REG_CONFIG);

    if(shutDown) {
        temp |= MCP9808_CONFIG_SHUTDOWN;
    } else {
        temp &= ~MCP9808_CONFIG_SHUTDOWN;
    }

    temp &= ~(MCP9808_CONFIG_CRITLOCKED|MCP9808_CONFIG_WINLOCKED);

    write16(MCP9808_REG_CONFIG, temp);

    if(!shutDown) {
        (*_customDelayFunc)(wakeDelay[TC_TEMP_RESOLUTION]);
    }
}

uint16_t tempSensor::read16(uint16_t regno)
{
    uint16_t value = 0;

    Wire.beginTransmission(_address);
    Wire.write((uint8_t)(regno & 0x0f));
    Wire.endTransmission(false);

    Wire.requestFrom(_address, (uint8_t)2);

    value = Wire.read() << 8;
    value |= Wire.read();

    return value;
}

uint8_t tempSensor::read8(uint16_t regno)
{
    uint16_t value = 0;

    Wire.beginTransmission(_address);
    Wire.write((uint8_t)(regno & 0x0f));
    Wire.endTransmission(false);

    Wire.requestFrom(_address, (uint8_t)1);

    value = Wire.read();

    return value;
}

void tempSensor::write16(uint16_t regno, uint16_t value)
{
    Wire.beginTransmission(_address);
    Wire.write((uint8_t)(regno & 0x0f));
    Wire.write((uint8_t)(value >> 8));
    Wire.write((uint8_t)(value & 0xff));
    Wire.endTransmission();
}

void tempSensor::write8(uint16_t regno, uint8_t value)
{
    Wire.beginTransmission(_address);
    Wire.write((uint8_t)(regno & 0x0f));
    Wire.write((uint8_t)(value & 0xff));
    Wire.endTransmission();
}

static void defaultDelay(unsigned int mydelay)
{
    delay(mydelay);
}
#endif
