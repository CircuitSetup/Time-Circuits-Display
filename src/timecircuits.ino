/*
 * -------------------------------------------------------------------
 * CircuitSetup.us Time Circuits Display
 * Code adapted from Marmoset Electronics 
 * https://www.marmosetelectronics.com/time-circuits-clock
 * by John Monaco
 * -------------------------------------------------------------------
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <Arduino.h>
#include "clockdisplay.h"
#include "tc_audio.h"
#include "tc_keypad.h"
#include "tc_menus.h"
#include "tc_time.h"

///////////////////////////////////////////////////////
void setup() {
    Serial.begin(115200);
    delay(1000);  // wait for console opening

    Wire.begin();
    scan();
    Serial.println();
    eepromSize();
    
    audio_setup();
    play_file("/startup.mp3");

    allLampTest();

    //menu_setup();
    keypad_setup();
    time_setup();
}

///////////////////////////////////////////////////////

void loop() {
    keypadLoop();
    get_key();
    time_loop();
    audio_loop();
}

void scan() {
    Serial.println(" Scanning I2C Addresses");
    uint8_t cnt = 0;
    for (uint8_t i = 0; i < 128; i++) {
        Wire.beginTransmission(i);
        uint8_t ec = Wire.endTransmission(true);
        if (ec == 0) {
            if (i < 16) Serial.print('0');
            Serial.print(i, HEX);
            cnt++;
        } else
            Serial.print("..");
        Serial.print(' ');
        if ((i & 0x0f) == 0x0f) Serial.println();
    }
    Serial.print("Scan Completed, ");
    Serial.print(cnt);
    Serial.println(" I2C Devices found.");
}

bool i2cReady(uint8_t adr) {
    uint32_t timeout = millis();
    bool ready = false;
    while ((millis() - timeout < 100) && (!ready)) {
        Wire.beginTransmission(adr);
        ready = (Wire.endTransmission() == 0);
    }
    return ready;
}

void eepromSize() {
    Serial.println("Discovering eeprom sizes 0x10..0x57");
    uint8_t adr = 0x10, i;
    uint16_t size;
    char buf[64];
    while (adr < 0x58) {
        i = 0;
        size = 0x1000;  // Start at 4k
        i += sprintf_P(&buf[i], PSTR("0x%02X: "), adr);
        if (i2cReady(adr)) {  // EEPROM answered
            uint8_t zeroByte;
            Wire.beginTransmission(adr);
            Wire.write((uint8_t)0);  // set address ptr to 0, two bytes High
            Wire.write((uint8_t)0);  // set address ptr to 0, two bytes Low
            uint8_t err = Wire.endTransmission();
            if (err == 0) {  // worked
                err = Wire.requestFrom(adr, (uint8_t)1);
                if (err == 1) {  // got the value of the byte at address 0
                    zeroByte = Wire.read();
                    uint8_t saveByte, testByte;
                    do {
                        if (i2cReady(adr)) {
                            Wire.beginTransmission(adr);
                            Wire.write(highByte(size));  // set next test address
                            Wire.write(lowByte(size));
                            Wire.endTransmission();
                            err = Wire.requestFrom(adr, (uint8_t)1);
                            if (err == 1) {
                                saveByte = Wire.read();
                                Wire.beginTransmission(adr);
                                Wire.write(highByte(size));  // set next test address
                                Wire.write(lowByte(size));
                                Wire.write((uint8_t)~zeroByte);  // change it
                                err = Wire.endTransmission();
                                if (err == 0) {  // changed it
                                    if (!i2cReady(adr)) {
                                        i += sprintf_P(&buf[i], PSTR(" notReady2.\n"));
                                        Serial.print(buf);
                                        adr++;
                                        break;
                                    }
                                    Wire.beginTransmission(adr);
                                    Wire.write((uint8_t)0);  // address 0 byte High
                                    Wire.write((uint8_t)0);  // address 0 byte Low
                                    err = Wire.endTransmission();
                                    if (err == 0) {
                                        err = Wire.requestFrom(adr, (uint8_t)1);
                                        if (err == 1) {  // now compare it
                                            testByte = Wire.read();
                                        } else {
                                            testByte = ~zeroByte;  // error out
                                        }
                                    } else {
                                        testByte = ~zeroByte;
                                    }
                                } else {
                                    testByte = ~zeroByte;
                                }
                                //restore byte
                                if (!i2cReady(adr)) {
                                    i += sprintf_P(&buf[i], PSTR(" notReady4.\n"));
                                    Serial.print(buf);
                                    adr++;
                                    break;
                                }

                                Wire.beginTransmission(adr);
                                Wire.write(highByte(size));  // set next test address
                                Wire.write(lowByte(size));
                                Wire.write((uint8_t)saveByte);  // restore it
                                Wire.endTransmission();
                            } else
                                testByte = ~zeroByte;
                        } else
                            testByte = ~zeroByte;
                        if (testByte == zeroByte) {
                            size = size << 1;
                        }
                    } while ((testByte == zeroByte) && (size > 0));
                    if (size == 0)
                        i += sprintf_P(&buf[i], PSTR("64k Bytes"));
                    else
                        i += sprintf_P(&buf[i], PSTR("%dk Bytes"), size / 1024);
                    if (!i2cReady(adr)) {
                        i += sprintf_P(&buf[i], PSTR(" notReady3.\n"));
                        Serial.print(buf);
                        adr++;
                        continue;
                    }
                    Wire.beginTransmission(adr);
                    Wire.write((uint8_t)0);  // set address ptr to 0, two bytes High
                    Wire.write((uint8_t)0);  // set address ptr to 0, two bytes Low
                    Wire.write(zeroByte);    //Restore
                    err = Wire.endTransmission();
                } else
                    i += sprintf_P(&buf[i], PSTR("Read 0 Failure"));
            } else
                i += sprintf_P(&buf[i], PSTR("Write Adr 0 Failure"));

        } else
            i += sprintf_P(&buf[i], PSTR("Not Present."));
        Serial.println(buf);
        adr++;
    }
}