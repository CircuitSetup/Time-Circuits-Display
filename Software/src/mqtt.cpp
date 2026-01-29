/*
 *
 * PubSubClient.cpp - A simple client for MQTT.
 * Nick O'Leary
 * http://knolleary.net
 *
 * Copyright (c) 2008-2020 Nicholas O'Leary
 * Minimized & adapted by Thomas Winischhofer (A10001986) in 2023
 * MQTT 5.0 support by Thomas Winischhofer (A10001986) in 2025
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
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
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
 * LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 * OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 * 
 */

#include "tc_global.h"

//#define MQTT_DBG

#ifdef TC_HAVEMQTT

#include "mqtt.h"

#include "lwip/inet_chksum.h"
#include "lwip/ip.h"
#include "lwip/ip4.h"
#include "lwip/err.h"
#include "lwip/icmp.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "lwip/netdb.h"
#include "lwip/dns.h"

#define MPL 500
static uint8_t mytt5_connect_props[8] = {
    0x07,                               // prop length
    0x27, 0, 0, MPL >> 8, MPL & 0xff,   // maximum packet size: 500 bytes (modified by code)
    0x17, 0                             // Request Problem Information -> no
};

static void defLooper()
{
}

PubSubClient::PubSubClient(WiFiClient& client)
{
    this->_state = MQTT_DISCONNECTED;
    this->_client = &client;
    this->bufferSize = 0;
    this->keepAlive = MQTT_KEEPALIVE;
    this->socketTimeout = MQTT_SOCKET_TIMEOUT * 1000;
    setLooper(defLooper);
    // app MUST call setClientID() before connecting
    // app MUST call setBufferSize() before setVersion()
    // app MUST call setVersion() before connecting
}

PubSubClient::~PubSubClient()
{
    if(this->bufferSize) 
        free(this->buffer);
}

void PubSubClient::setClientID(const char *src)
{
    int j = 0;
    uint8_t t;

    for( ; *src && (j < 23); ++src) {
        t = *src;
        if( (t >= '0' && t <= '9') ||
            (t >= 'A' && t <= 'Z') ||
            (t >= 'a' && t <= 'z') ) {
            _clientID[j++] = (uint8_t)t;
        }
    }
    _clientID[j] = 0;
    if(!j) {
        sprintf(_clientID, "%s%d", "AUTO", esp_random() & 0xffff);
    }
}

bool PubSubClient::setBufferSize(uint16_t size)
{
    if(size == 0)
        return false;

    if(this->bufferSize == 0) {
        this->buffer = (uint8_t*)malloc(size);
    } else {
        uint8_t* newBuffer = (uint8_t*)realloc(this->buffer, size);
        if(newBuffer) {
            this->buffer = newBuffer;
        } else {
            return false;
        }
    }
    
    this->bufferSize = size;
    
    return (this->buffer != NULL);
}

void PubSubClient::setVersion(int mqtt_version)
{
    #ifdef MQTT_DBG
    if(connected()) {
        Serial.println("MQTT: setVersion called while connected\n");
        return;
    }
    if(!this->buffer) {
        Serial.println("MQTT: setVersion called before setBufferSize\n");
        return;
    }
    #endif
    
    switch(mqtt_version) {
    case 3:
        mqtt_max_header_size = MQTT_MAX_HEADER_SIZE_3_1_1;
        _phdr[6] = MQTT_VERSION_3_1_1;
        _v3 = true;
        break;
    case 5:
        mqtt_max_header_size = MQTT_MAX_HEADER_SIZE_5_0;
        _phdr[6] = MQTT_VERSION_5_0;
        mytt5_connect_props[4] = (this->bufferSize - 10) >> 8;
        mytt5_connect_props[5] = (this->bufferSize - 10) & 0xff;
        _v3 = false;
        break;
    #ifdef MQTT_DBG
    default:
        Serial.printf("MQTT: setVersion: invalid %d\n", mqtt_version);
    #endif
    }
}

bool PubSubClient::connect()
{
    return connect(NULL, NULL, true);
}

bool PubSubClient::connect(const char *user, const char *pass)
{
    return connect(user, pass, true);
}

bool PubSubClient::connect(const char *user, const char *pass, bool cleanSession)
{
    if(!connected()) {

        int result = 0;

        if(_client->connected()) {
            result = 1;
        } else {
            if(domain) {
                result = _client->connect(this->domain, this->port, (int)5000);
            } else {
                result = _client->connect(this->ip, this->port, (int)5000);
            }
        }

        if(result == 1) {
          
            nextMsgId = 1;
            
            // Leave room in the buffer for header and variable length field
            uint16_t length = mqtt_max_header_size;
            unsigned int j;            

            for(j = 0; j < mqtt_version_header_length; j++) {
                this->buffer[length++] = _phdr[j];
            }

            uint8_t v = 0;

            // Clean Session aka Clean Start
            if(cleanSession) v |= 0x02;

            if(user) {
                v |= 0x80;
                if(pass) {
                    v |= 0x40;
                }
            }
            this->buffer[length++] = v;

            this->buffer[length++] = (this->keepAlive >> 8);
            this->buffer[length++] = (this->keepAlive & 0xff);

            if(!_v3) { 
                // v5: properties
                for(j = 0; j < sizeof(mytt5_connect_props); j++) {
                    this->buffer[length++] = mytt5_connect_props[j];
                }
            }

            CHECK_STRING_LENGTH(length, (const char *)_clientID)
            length = writeString((const char *)_clientID, this->buffer, length);

            if(user) {
                CHECK_STRING_LENGTH(length, user)
                length = writeString(user, this->buffer, length);
                if(pass) {
                    CHECK_STRING_LENGTH(length, pass)
                    length = writeString(pass, this->buffer, length);
                }
            }

            write(MQTTCONNECT, this->buffer, length - mqtt_max_header_size);

            lastInActivity = lastOutActivity = millis();

            _state = MQTT_CONNECTING;

            return true;
            
        } else {

            _state = MQTT_CONNECT_FAILED;

        }
        
        return false;
        
    }
    
    return true;
}

bool PubSubClient::connected()
{
    bool rc = false;

    if(_client) {
      
        rc = (int)_client->connected();
        
        if(!rc) {
            if(this->_state == MQTT_CONNECTED) {
                this->_state = MQTT_CONNECTION_LOST;
                _client->flush();
                _client->stop();
            }
        } else {
            return (this->_state == MQTT_CONNECTED);
        }
    }
    return rc;
}

bool PubSubClient::loop()
{
    if(_state == MQTT_CONNECTING) {

        if(!_client->available()) {

            if(millis() - lastInActivity >= this->socketTimeout) {
                _state = MQTT_CONNECTION_TIMEOUT;
                _client->stop();
                
                #ifdef MQTT_DBG
                Serial.println("MQTT: CONNACK timed-out");
                #endif
                
                return false;
            }

            return true;
            
        } else if(_v3) {

            uint8_t llen;
            uint32_t len = readPacket(&llen);

            if(len == 4) {
                if(buffer[3] == 0) {
                    lastInActivity = millis();
                    pingOutstanding = false;
                    _state = MQTT_CONNECTED;
                    
                    #ifdef MQTT_DBG
                    Serial.println("MQTTv3: CONNACK received");
                    #endif
                    
                    return true;
                } else {
                    _state = buffer[3];
                }
            } else {
                _state = MQTT_CONNECT_BAD_PROTOCOL;
            }
            _client->stop();

            #ifdef MQTT_DBG
            Serial.printf("MQTTv3: CONNACK failed, state %d\n", _state);
            #endif

            return false;
            
        } else {    // v5.0

            uint8_t llen;
            uint32_t len = readPacket(&llen);

            #ifdef MQTT_DBG
            Serial.printf("MQTTv5: packet %d, len %d\n", (buffer[0] & 0xf0) >> 4, len);
            #endif

            if(len >= 4 && ((buffer[0] & 0xf0) == MQTTCONNACK)) {

                  unsigned int vbl = 0;
                  int bo = _vbl(&buffer[1], vbl);

                  #ifdef MQTT_DBG
                  Serial.printf("MQTTv5: CONNACK bo %d vbl %d\n", bo, vbl);
                  #endif

                  if(bo < 0 || vbl < 2) {
                    
                      _state = MQTT_CONNECT_BAD_PROTOCOL;
                      
                  } else if(buffer[1+bo+1] == 0) {
                    
                      lastInActivity = millis();
                      pingOutstanding = false;
                      _state = MQTT_CONNECTED;
                      
                      #ifdef MQTT_DBG
                      Serial.println("MQTTv5: CONNACK received");
                      #endif

                      // Scan CONNACK properties and check if the
                      // server mandates stuff we need to obey to
                      if(vbl > 2) {
                          unsigned int pl = 0;
                          int bbo = _vbl(&buffer[1+bo+2], pl);
                          if(pl > 0) {
                               // Keep Alive
                               int idx = _searchProp(&buffer[1+bo+2+bbo], 0x13, pl);
                               if(idx >= 0) {
                                    this->keepAlive = (buffer[1+bo+2+bbo+idx] << 8) | buffer[1+bo+2+bbo+idx+1];
                                    
                                    #ifdef MQTT_DBG
                                    Serial.printf("MQTTv5: keepAlive overruled %d\n", this->keepAlive);
                                    #endif
                               }
                          }
                          
                      }
                      
                      return true;
                      
                  } else {
                    
                      if(vbl > 1) {
                          _state = buffer[1+bo+1];
                      } else {
                          _state = MQTT_CONNECT_FAILED;
                      }
                  }
              
            } else {
                _state = MQTT_CONNECT_BAD_PROTOCOL;
            }
            _client->stop();

            #ifdef MQTT_DBG
            Serial.printf("MQTTv5: CONNACK failed, state %d\n", _state);
            #endif

            return false;
          
        }
      
    } else if(connected()) {
        
        unsigned long t = millis();
        unsigned long ka = this->keepAlive * 1000UL;
        
        if((t - lastInActivity > ka) || (t - lastOutActivity > ka)) {

            if(pingOutstanding) {
                this->_state = MQTT_CONNECTION_TIMEOUT;
                _client->stop();
                return false;
            } else {
                this->buffer[0] = MQTTPINGREQ;
                this->buffer[1] = 0;
                _client->write(this->buffer, 2);
                lastOutActivity = t;
                lastInActivity = t;
                pingOutstanding = true;
            }

        }
        
        if(_client->available()) {
          
            uint8_t  llen;
            uint16_t len = readPacket(&llen);
            uint8_t  msgId1 = 0, msgId2 = 0;
            uint8_t  *payload;
            
            if(len > 0) {
                
                lastInActivity = t;
                uint8_t type = this->buffer[0] & 0xf0;

                switch(type) {
                case MQTTPUBLISH:
                    if(callback) {
                        // topic length in bytes
                        uint16_t tl = (this->buffer[llen+1] << 8) + this->buffer[llen+2];
                        
                        // zero length topics and topic-aliases not supported
                        if(tl) {

                            int pl = 0;
                            bool valMsg = true;
                            
                            // move topic inside buffer 1 byte to front to make room for 0-terminator
                            memmove(this->buffer + llen + 2, this->buffer + llen + 3, tl); 
                            this->buffer[llen + 2 + tl] = 0;
                         
                            char *topic = (char *)this->buffer + llen + 2;
    
                            if(!_v3) {
                                // Skip properties
                                unsigned int temp;
                                pl = _vbl(&this->buffer[llen + 3 + tl], temp);
                                if(pl > 0) {
                                  
                                    #ifdef MQTT_DBG
                                    if(temp) {
                                        // Test property search
                                        int idx = _searchProp(&this->buffer[llen + 3 + tl + pl], 0x11, temp);
                                        Serial.printf("MQTTv5: property test: temp %d pl %d; 0x11 at %d; \n", temp, pl, idx);
                                    }
                                    #endif
                                    
                                    pl += temp;
                                } else {
                                    valMsg = false;
                                }
                            }

                            if(valMsg) {
                                if((this->buffer[0] & 0x06) == MQTTQOS1) {
        
                                    // msgId only present for QOS>0
                                    msgId1 = this->buffer[llen + 3 + tl + pl]; 
                                    msgId2 = this->buffer[llen + 3 + tl + pl + 1];
                                    
                                    payload = this->buffer + llen + 3 + tl + pl + 2;
                                    callback(topic, payload, len - llen - 3 - tl - pl - 2);

                                    // OK for v3 and v5
                                    this->buffer[0] = MQTTPUBACK;
                                    this->buffer[1] = 2;
                                    this->buffer[2] = msgId1;
                                    this->buffer[3] = msgId2;
                                    _client->write(this->buffer, 4);
                                    lastOutActivity = t;
        
                                } else {
                                  
                                    payload = this->buffer + llen + 3 + tl + pl;
                                    callback(topic, payload, len - llen - 3 - tl - pl);
                                    
                                }
                            }
                        }
                    }
                    break;
                    
                case MQTTPINGREQ:
                    this->buffer[0] = MQTTPINGRESP;
                    this->buffer[1] = 0;
                    _client->write(this->buffer, 2);
                    break;
                    
                case MQTTPINGRESP:
                    pingOutstanding = false;
                    break;
                    
                case MQTTDISCONNECT:
                    if(!_v3) {
                        _state = MQTT_DISCONNECTED;
                        _client->flush();
                        _client->stop();
                        lastInActivity = lastOutActivity = millis();
                    }
                    break;
                
                case MQTTSUBACK:
                    // Ignore
                    #ifdef MQTT_DBG
                    if(len >= 4) {
                        unsigned int temp;
                        int pl = _vbl(&this->buffer[1 + llen + 2], temp);
                        pl += temp;
                        for(int i = 0; i < len - (1 + llen + 2 + pl); i++) {
                            Serial.printf("SUBACK payload %d: %d\n", i, this->buffer[1 + llen + 2 + pl + i]);
                        }
                    }
                    #endif
                    break;

                #ifdef MQTT_DBG
                default:
                    Serial.printf("MQTT: Unhandled packet received: %d\n", type);
                #endif

                }

            } else if(!connected()) {
              
                // readPacket has closed the connection
                return false;
                
            }
        }
        
        return true;
    }
    
    return false;
}

bool PubSubClient::publish(const char* topic, const uint8_t* payload, unsigned int plength, bool retained)
{
    if(connected()) {
        if(this->bufferSize < mqtt_max_header_size + 2+strnlen(topic, this->bufferSize) + plength) {
            // Too long
            return false;
        }
        
        // Leave room in the buffer for header and variable length field
        uint16_t length = mqtt_max_header_size;
        length = writeString(topic, this->buffer, length);

        if(!_v3) {
            // v5: No properties
            this->buffer[length++] = 0;
        }

        // Add payload
        uint16_t i;
        for(i = 0; i < plength; i++) {
            this->buffer[length++] = payload[i];
        }

        // Write the header
        uint8_t header = MQTTPUBLISH;
        
        if(retained) header |= 1;
        
        return write(header, this->buffer, length - mqtt_max_header_size);
    }
    
    return false;
}

bool PubSubClient::subscribe(const char *topic, const char *topic2, uint8_t qos)
{
    return subscribe_int(false, topic, topic2, qos);
}

bool PubSubClient::unsubscribe(const char* topic)
{
    return subscribe_int(true, topic, NULL, 0);
}

void PubSubClient::disconnect()
{
    this->buffer[0] = MQTTDISCONNECT;

    if(_v3) {
        this->buffer[1] = 0;
        _client->write(this->buffer, 2);
    } else {
        this->buffer[1] = 2;
        this->buffer[2] = 0;
        this->buffer[3] = 0;
        _client->write(this->buffer, 4);
    }

    _state = MQTT_DISCONNECTED;

    _client->flush();
    _client->stop();

    lastInActivity = lastOutActivity = millis();
}


// Private


bool PubSubClient::subscribe_int(bool unsubscribe, const char *topic, const char *topic2, uint8_t qos)
{
    size_t  topicLength = strnlen(topic, this->bufferSize);
    size_t  topicLength2 = topic2 ? strnlen(topic2, this->bufferSize) : 0;
    uint8_t header = unsubscribe ? MQTTUNSUBSCRIBE : MQTTSUBSCRIBE;
    
    if(!topic)
        return false;
    
    if(!unsubscribe) {
        if(qos > 1)
            return false;
        if(this->bufferSize < mqtt_max_header_size+2 + 2+topicLength+1 + (topicLength2 ? 2+topicLength+1 : 0))
            return false;
    } else {
        if(this->bufferSize < mqtt_max_header_size+2 + 2+topicLength)
            return false;
    }
    
    if(connected()) {
        // Leave room in the buffer for header and variable length field
        uint16_t length = mqtt_max_header_size;

        // Packet identifier
        nextMsgId++;
        if(!nextMsgId) nextMsgId++;
        this->buffer[length++] = (nextMsgId >> 8);
        this->buffer[length++] = (nextMsgId & 0xff);

        // v5 properties
        if(!_v3) {
            this->buffer[length++] = 0;     // no properties
            qos |= 4;                       // "no local" - don't need our message sent back to us
        }
        
        length = writeString((char*)topic, this->buffer, length);
        if(!unsubscribe) this->buffer[length++] = qos;

        if(topic2 && topicLength2) {
            length = writeString((char*)topic2, this->buffer, length);
            this->buffer[length++] = qos;
        }

        return write(header | 0x02, this->buffer, length - mqtt_max_header_size);
    }
    
    return false;
}

// reads a byte into result
bool PubSubClient::readByte(uint8_t *result)
{
    unsigned long mnow, currentMillis;
    unsigned long previousMillis = millis();

    mnow = previousMillis;
    
    while(!_client->available()) {

        currentMillis =  millis();
        
        if(currentMillis - previousMillis >= this->socketTimeout)
            return false;
        
        if(currentMillis - mnow > 20) {
            looper();
            mnow = millis();
        }

        delay(2);

    }
    
    *result = _client->read();
    
    return true;
}

// reads a byte into result[*index] and increments index
bool PubSubClient::readByte(uint8_t *result, uint16_t *index)
{
    uint16_t current_index = *index;
    uint8_t *write_address = &(result[current_index]);
    
    if(readByte(write_address)) {
        *index = current_index + 1;
        return true;
    }
    
    return false;
}

uint32_t PubSubClient::readPacket(uint8_t *lengthLength)
{
    uint16_t len = 0;
    
    if(!readByte(this->buffer, &len))
        return 0;
    
    uint32_t multiplier = 1;
    uint32_t length = 0;
    uint8_t  digit = 0;
    uint32_t idx = 0;

    do {

        if(len == 5) {
            // Invalid remaining length encoding - kill the connection
            _state = MQTT_DISCONNECTED;
            _client->stop();
            return 0;
        }
        
        if(!readByte(&digit)) return 0;
        
        this->buffer[len++] = digit;
        length += (digit & 0x7f) * multiplier;
        multiplier <<= 7;
        
    } while(digit & 0x80);
    
    *lengthLength = len - 1;
       
    idx = len;

    for(uint32_t i = 0; i < length; i++) {
      
        if(!readByte(&digit)) 
            return 0;

        if(len < this->bufferSize) {
            this->buffer[len] = digit;
            len++;
        }
        idx++;
    }

    if(idx > this->bufferSize)
        len = 0; // This will cause the packet to be ignored.
    
    return len;
}

size_t PubSubClient::buildHeader(uint8_t header, uint8_t *buf, uint16_t length)
{
    uint8_t lenBuf[4];
    uint8_t llen = 0;
    uint8_t digit;
    uint8_t pos = 0;
    uint16_t len = length;
    
    do {

        digit = len & 0x7f;
        len >>= 7; 
        if(len > 0) digit |= 0x80;
        lenBuf[pos++] = digit;
        llen++;
        
    } while(len > 0);

    buf[4 - llen] = header;
    
    for(int i = 0; i < llen; i++) {
        buf[mqtt_max_header_size - llen + i] = lenBuf[i];
    }
    
    return llen + 1; // Full header size is variable length bit plus the 1-byte fixed header
}

bool PubSubClient::write(uint8_t header, uint8_t *buf, uint16_t length)
{
    uint16_t rc;
    uint8_t hlen = buildHeader(header, buf, length);

#ifdef MQTT_MAX_TRANSFER_SIZE

    uint8_t* writeBuf = buf + (mqtt_max_header_size - hlen);
    uint16_t bytesRemaining = length + hlen;  // Match the length type
    uint8_t bytesToWrite;
    bool result = true;
    
    while((bytesRemaining > 0) && result) {
        bytesToWrite = (bytesRemaining > MQTT_MAX_TRANSFER_SIZE) ? MQTT_MAX_TRANSFER_SIZE : bytesRemaining;
        rc = _client->write(writeBuf, bytesToWrite);
        result = (rc == bytesToWrite);
        bytesRemaining -= rc;
        writeBuf += rc;
    }
    
    return result;
    
#else

    rc = _client->write(buf + (mqtt_max_header_size - hlen), length + hlen);

    /*#ifdef MQTT_DBG
    for(int i = 0; i < length + hlen; i++) {
        Serial.printf("%02x ", *(buf + (mqtt_max_header_size - hlen) + i));
    }
    Serial.println(" ");
    #endif*/
    
    lastOutActivity = millis();
    return (rc == hlen + length);
    
#endif
}

// Decode variable length
// Returns length of length (1-4), 0 on error
// Parameter 'length' is set to decoded length
int PubSubClient::_vbl(const uint8_t *buf, unsigned int& length)
{
    unsigned int lenLen = 0;
    uint32_t multiplier = 1;
    uint8_t  digit = 0;

    length = 0;
    
    do {
        if(lenLen == 5) return 0;  // Invalid length encoding
        
        digit = buf[lenLen++];
        length += (digit & 0x7f) * multiplier;
        multiplier <<= 7;
        
    } while(digit & 0x80);

    return lenLen;
}

int PubSubClient::_searchProp(uint8_t *buf, uint8_t prop, const int propLength)
{
    int idx = 0;
    int temp;

    do {
      
        #ifdef MQTT_DBG
        Serial.printf("MQTTv5: _searchProp: property %d found\n", buf[idx]);
        #endif
        
        if(buf[idx] == prop) {
             return idx + 1;
        }
        
        switch(buf[idx]) {
        case 1:
        case 23:
        case 25:
        case 36:
        case 37:
        case 40:
        case 41:
        case 42:                    // byte
            idx += 2;
            break;
        case 19:
        case 33:
        case 34:
        case 35:                    // two byte integer
            idx += 3;
            break;
        case 2:
        case 17:
        case 24:
        case 39:                    // four byte integer
            idx += 5;
            break;
        case 3:
        case 8:
        case 18:
        case 21:
        case 26:
        case 28:
        case 31:                    // UTF8 string
        case 9:
        case 22:                    // binary data
            idx++;
            temp = (buf[idx] << 8) | buf[idx + 1];
            idx += 2 + temp;
            break;
        case 38:                    // UTF8 string pair
            idx++;
            temp = (buf[idx] << 8) | buf[idx + 1];
            idx += 2 + temp;
            if(idx >= propLength) return -1;
            temp = (buf[idx] << 8) | buf[idx + 1];
            idx += 2 + temp;
            break;
        case 11:                    // variable byte integer
            idx++;
            if(buf[idx] & 0x80) {
                idx++;
                if(buf[idx] & 0x80) {
                    idx++;
                    if(buf[idx] & 0x80) {
                        idx++;
                    }
                }
            }
            idx++;
            break;
        default:
            #ifdef MQTT_DBG
            Serial.printf("MQTTv5: _searchProp: property %d unknown\n", buf[idx]);
            #endif
            return -1;
        }
    } while(idx < propLength);

    return -1;
}

uint16_t PubSubClient::writeString(const char *string, uint8_t *buf, uint16_t pos)
{
    const char *idp = string;
    uint16_t i = 0, oldPos = pos;
    
    pos += 2;
    while(*idp) {
        buf[pos++] = *idp++;
        i++;
    }
    buf[oldPos++] = (i >> 8);
    buf[oldPos]   = (i & 0xff);
    
    return pos;
}

/*
 * Async PING
 * We PING the broker before connecting in order
 * to avoid a blocking connect().
 */

#define PING_ID 0xAFAF

bool PubSubClient::sendPing()
{
    ip4_addr_t            ping_target;
    struct icmp_echo_hdr *iecho;
    struct sockaddr_in    to;
    struct timeval        tout;
    int    size       =   32;
    size_t ping_size  =   sizeof(struct icmp_echo_hdr) + size;
    int    err;

    #ifdef MQTT_DBG
    Serial.printf("MQTT: Sending ping\n");
    #endif

    // We only PING if we have an IP address.
    // No point in avoiding a blocking connect
    // if a DNS call is required beforehand
    if(this->domain)
        return false;

    if((_s = socket(AF_INET, SOCK_RAW, IP_PROTO_ICMP)) < 0)
        return false;

    ping_target.addr = ip;

    tout.tv_sec  = 0;
    tout.tv_usec = 2000;    // 2ms

    if(setsockopt(_s, SOL_SOCKET, SO_RCVTIMEO, &tout, sizeof(tout)) < 0) {
        closesocket(_s);
        return false;
    }

    iecho = (struct icmp_echo_hdr *)mem_malloc((mem_size_t)ping_size);
    if(!iecho) {
        closesocket(_s);
        return false;
    }

    ICMPH_TYPE_SET(iecho, ICMP_ECHO);
    ICMPH_CODE_SET(iecho, 0);
    iecho->chksum = 0;
    iecho->id = PING_ID;
    iecho->seqno = htons(++_pseq_num);

    iecho->chksum = inet_chksum(iecho, (uint16_t)ping_size);

    to.sin_len = sizeof(to);
    to.sin_family = AF_INET;
    inet_addr_from_ip4addr(&to.sin_addr, &ping_target);

    err = sendto(_s, iecho, ping_size, 0, (struct sockaddr*)&to, sizeof(to));
        
    mem_free(iecho);

    if(err) {
        _pstate = PING_PINGING;
    } else {
        closesocket(_s);
        _pstate = PING_IDLE;
    }
        
    return (err ? true : false);
}

bool PubSubClient::pollPing()
{
    char buf[64];
    int len, fromlen;
    struct sockaddr_in    from;
    struct ip_hdr        *iphdr;
    struct icmp_echo_hdr *iecho = NULL;
    bool success = false;

    if(_pstate != PING_PINGING)
        return false;

    while((len = recvfrom(_s, buf, sizeof(buf), 0, (struct sockaddr*)&from, (socklen_t*)&fromlen)) > 0) {
        if(len >= (int)(sizeof(struct ip_hdr) + sizeof(struct icmp_echo_hdr))) {
            iphdr = (struct ip_hdr *)buf;
            iecho = (struct icmp_echo_hdr *)(buf + (IPH_HL(iphdr) * 4));
            success = ((iecho->id == PING_ID) && (iecho->seqno == htons(_pseq_num)));
        }
    }

    if(success) {
        closesocket(_s);
        _pstate = PING_IDLE;
    }

    return success;
}

void PubSubClient::cancelPing()
{
    if(_pstate != PING_PINGING)
        return;

    closesocket(_s);
    _pstate = PING_IDLE;   
}

#endif  // TC_HAVEMQTT
