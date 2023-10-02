/*
 * -------------------------------------------------------------------
 * CircuitSetup.us Time Circuits Display
 * (C) 2021-2022 John deGlavina https://circuitsetup.us
 * (C) 2022-2023 Thomas Winischhofer (A10001986)
 * https://github.com/realA10001986/Time-Circuits-Display
 * http://tcd.backtothefutu.re
 *
 * WiFi and Config Portal handling
 *
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

#ifndef _TC_WIFI_H
#define _TC_WIFI_H

extern bool wifiIsOff;
extern bool wifiAPIsOff;
extern bool wifiInAPMode;

extern bool wifiHaveSTAConf;

extern bool carMode;

#ifdef TC_HAVEMQTT
extern bool useMQTT;
extern const char *mqttAudioFile;
extern bool pubMQTT;
#endif

void wifi_setup();
void wifi_loop();
void wifiOn(unsigned long newDelay = 0, bool alsoInAPMode = false, bool deferConfigPortal = false);
bool wifiOnWillBlock();
void wifiStartCP();

void updateConfigPortalValues();

int  wifi_getStatus();
bool wifi_getIP(uint8_t& a, uint8_t& b, uint8_t& c, uint8_t& d);
void wifi_getMAC(char *buf);
void ipToString(char *str, IPAddress ip);

int16_t filterOutUTF8(char *src, char *dst, int srcLen, int maxChars);

#ifdef TC_HAVEMQTT
bool mqttState();
void mqttPublish(const char *topic, const char *pl, unsigned int len);
#endif

#endif
