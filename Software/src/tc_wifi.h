/*
 * -------------------------------------------------------------------
 * CircuitSetup.us Time Circuits Display
 * (C) 2021-2022 John deGlavina https://circuitsetup.us
 * (C) 2022-2026 Thomas Winischhofer (A10001986)
 * https://github.com/realA10001986/Time-Circuits-Display
 * https://tcd.out-a-ti.me
 *
 * WiFi and Config Portal handling
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

#ifndef _TC_WIFI_H
#define _TC_WIFI_H

extern bool wifiIsOff;
extern bool wifiAPIsOff;
extern bool wifiInAPMode;

extern bool wifiHaveSTAConf;

extern bool blockWiFiSTAPS;

extern bool carMode;

extern bool pubMQTT;
#ifdef TC_HAVEMQTT
extern bool useMQTT;
extern const char *mqttAudioFile;
extern bool MQTTvarLead;
#endif

void wifi_setup();
void wifi_loop();
void wifiOn(unsigned long newDelay = 0, bool alsoInAPMode = false, bool deferConfigPortal = false);
bool wifiOnWillBlock();
void wifiRestartPSTimer();
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
