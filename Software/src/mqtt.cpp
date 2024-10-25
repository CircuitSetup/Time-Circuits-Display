/*
 *
 * PubSubClient.cpp - A simple client for MQTT.
 * Nick O'Leary
 * http://knolleary.net
 * Minimized & adapted by Thomas Winischhofer (A10001986) in 2023
 *
 * Copyright (c) 2008-2020 Nicholas O'Leary
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

static void defLooper()
{
}

PubSubClient::PubSubClient(WiFiClient& client)
{
    this->_state = MQTT_DISCONNECTED;
    this->_client = &client;
    this->bufferSize = 0;
    setBufferSize(MQTT_MAX_PACKET_SIZE);
    this->keepAlive = MQTT_KEEPALIVE;
    this->socketTimeout = MQTT_SOCKET_TIMEOUT * 1000;
    setLooper(defLooper);
}

PubSubClient::~PubSubClient()
{
    free(this->buffer);
}

bool PubSubClient::connect(const char *id)
{
    return connect(id, NULL, NULL, true);
}

bool PubSubClient::connect(const char *id, const char *user, const char *pass)
{
    return connect(id, user, pass, true);
}

bool PubSubClient::connect(const char *id, const char *user, const char *pass, bool cleanSession)
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
            uint16_t length = MQTT_MAX_HEADER_SIZE;
            unsigned int j;

#if MQTT_VERSION == MQTT_VERSION_3_1
            uint8_t d[9] = { 0x00, 0x06, 'M', 'Q', 'I', 's', 'd', 'p', MQTT_VERSION };
            #define MQTT_HEADER_VERSION_LENGTH 9
#elif MQTT_VERSION == MQTT_VERSION_3_1_1
            uint8_t d[7] = { 0x00, 0x04, 'M', 'Q', 'T', 'T', MQTT_VERSION };
            #define MQTT_HEADER_VERSION_LENGTH 7
#endif
            for(j = 0; j < MQTT_HEADER_VERSION_LENGTH; j++) {
                this->buffer[length++] = d[j];
            }

            uint8_t v = 0;
            
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

            CHECK_STRING_LENGTH(length, id)
            length = writeString(id, this->buffer, length);

            if(user) {
                CHECK_STRING_LENGTH(length, user)
                length = writeString(user, this->buffer, length);
                if(pass) {
                    CHECK_STRING_LENGTH(length, pass)
                    length = writeString(pass, this->buffer, length);
                }
            }

            write(MQTTCONNECT, this->buffer, length - MQTT_MAX_HEADER_SIZE);

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
    
    bool isPublish = ((this->buffer[0] & 0xf0) == MQTTPUBLISH);
    
    uint32_t multiplier = 1;
    uint32_t length = 0;
    uint8_t  digit = 0;
    uint32_t start = 0;

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
        
    } while((digit & 0x80) != 0);
    
    *lengthLength = len - 1;

    if(isPublish) {
      
        // Read in topic length to calculate bytes to skip over for Stream writing
        if(!readByte(this->buffer, &len)) return 0;
        if(!readByte(this->buffer, &len)) return 0;
        start = 2;
        
    }
    
    uint32_t idx = len;

    for(uint32_t i = start; i < length; i++) {
      
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

bool PubSubClient::loop()
{
    if(_state == MQTT_CONNECTING) {

        if(!_client->available()) {

            if(millis() - lastInActivity >= this->socketTimeout) {
                _state = MQTT_CONNECTION_TIMEOUT;
                _client->stop();
                #ifdef TC_DBG
                Serial.println("MQTT: CONNACK timed-out");
                #endif
                return false;
            }

            return true;
            
        } else {

            uint8_t llen;
            uint32_t len = readPacket(&llen);

            if(len == 4) {
                if(buffer[3] == 0) {
                    lastInActivity = millis();
                    pingOutstanding = false;
                    _state = MQTT_CONNECTED;
                    #ifdef TC_DBG
                    Serial.println("MQTT: CONNACK received");
                    #endif
                    return true;
                } else {
                    _state = buffer[3];
                }
            } else {
                _state = MQTT_CONNECT_BAD_PROTOCOL;
            }
            _client->stop();

            #ifdef TC_DBG
            Serial.printf("MQTT: CONNACK failed, state %d\n", _state);
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
                
                if(type == MQTTPUBLISH) {
                  
                    if(callback) {
                        // topic length in bytes
                        uint16_t tl = (this->buffer[llen+1] << 8) + this->buffer[llen+2];
                        
                        // move topic inside buffer 1 byte to front to make room for 0-terminator
                        memmove(this->buffer + llen + 2, this->buffer + llen + 3, tl); 
                        this->buffer[llen + 2 + tl] = 0;
                         
                        char *topic = (char *)this->buffer + llen + 2;
                        
                        if((this->buffer[0] & 0x06) == MQTTQOS1) {

                            // msgId only present for QOS>0
                            msgId1 = this->buffer[llen + 3 + tl]; 
                            msgId2 = this->buffer[llen + 3 + tl + 1];
                            
                            payload = this->buffer + llen + 3 + tl + 2;
                            callback(topic, payload, len - llen - 3 - tl - 2);

                            this->buffer[0] = MQTTPUBACK;
                            this->buffer[1] = 2;
                            this->buffer[2] = msgId1;
                            this->buffer[3] = msgId2;
                            _client->write(this->buffer, 4);
                            lastOutActivity = t;

                        } else {
                          
                            payload = this->buffer + llen + 3 + tl;
                            callback(topic, payload, len - llen - 3 - tl);
                            
                        }
                    }
                    
                } else if(type == MQTTPINGREQ) {
                  
                    this->buffer[0] = MQTTPINGRESP;
                    this->buffer[1] = 0;
                    _client->write(this->buffer, 2);
                    
                } else if(type == MQTTPINGRESP) {
                  
                    pingOutstanding = false;
                    
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
        if (this->bufferSize < MQTT_MAX_HEADER_SIZE + 2+strnlen(topic, this->bufferSize) + plength) {
            // Too long
            return false;
        }
        
        // Leave room in the buffer for header and variable length field
        uint16_t length = MQTT_MAX_HEADER_SIZE;
        length = writeString(topic, this->buffer, length);

        // Add payload
        uint16_t i;
        for(i = 0; i < plength; i++) {
            this->buffer[length++] = payload[i];
        }

        // Write the header
        uint8_t header = MQTTPUBLISH;
        
        if(retained) header |= 1;
        
        return write(header, this->buffer, length - MQTT_MAX_HEADER_SIZE);
    }
    
    return false;
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
        buf[MQTT_MAX_HEADER_SIZE - llen + i] = lenBuf[i];
    }
    
    return llen + 1; // Full header size is variable length bit plus the 1-byte fixed header
}

bool PubSubClient::write(uint8_t header, uint8_t *buf, uint16_t length)
{
    uint16_t rc;
    uint8_t hlen = buildHeader(header, buf, length);

#ifdef MQTT_MAX_TRANSFER_SIZE

    uint8_t* writeBuf = buf + (MQTT_MAX_HEADER_SIZE - hlen);
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

    rc = _client->write(buf + (MQTT_MAX_HEADER_SIZE - hlen), length + hlen);
    lastOutActivity = millis();
    return (rc == hlen + length);
    
#endif
}

bool PubSubClient::subscribe(const char *topic, const char *topic2, uint8_t qos)
{
    return subscribe_int(false, topic, topic2, qos);
}

bool PubSubClient::unsubscribe(const char* topic)
{
    return subscribe_int(true, topic, NULL, 0);
}

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
        if(this->bufferSize < MQTT_MAX_HEADER_SIZE+2 + 2+topicLength+1 + (topicLength2 ? 2+topicLength+1 : 0))
            return false;
    } else {
        if(this->bufferSize < MQTT_MAX_HEADER_SIZE+2 + 2+topicLength)
            return false;
    }
    
    if(connected()) {
        // Leave room in the buffer for header and variable length field
        uint16_t length = MQTT_MAX_HEADER_SIZE;
        nextMsgId++;
        if(!nextMsgId) nextMsgId++;
        this->buffer[length++] = (nextMsgId >> 8);
        this->buffer[length++] = (nextMsgId & 0xff);
        
        length = writeString((char*)topic, this->buffer, length);
        if(!unsubscribe) this->buffer[length++] = qos;

        if(topic2 && topicLength2) {
            length = writeString((char*)topic2, this->buffer, length);
            this->buffer[length++] = qos;
        }

        return write(header | MQTTQOS1, this->buffer, length - MQTT_MAX_HEADER_SIZE);
    }
    
    return false;
}

void PubSubClient::disconnect()
{
    this->buffer[0] = MQTTDISCONNECT;
    this->buffer[1] = 0;

    _client->write(this->buffer, 2);

    _state = MQTT_DISCONNECTED;

    _client->flush();
    _client->stop();

    lastInActivity = lastOutActivity = millis();
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

void PubSubClient::setServer(IPAddress ip, uint16_t port)
{
    this->ip = ip;
    this->port = port;
    this->domain = NULL;
}

void PubSubClient::setServer(const char *domain, uint16_t port)
{
    this->domain = domain;
    this->port = port;
}

void PubSubClient::setCallback(void (*callback)(char*, uint8_t*, unsigned int))
{
    this->callback = callback;
}

void PubSubClient::setLooper(void (*looper)())
{
    this->looper = looper;
}

int PubSubClient::state()
{
    return this->_state;
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

/*
 * Async PING
 * We PING the broker before connecting in order
 * to avoid a blocking connect().
 */

#define PING_ID 0xAFAF

bool PubSubClient::sendPing()
{
    struct sockaddr_in    address;
    ip4_addr_t            ping_target;
    struct icmp_echo_hdr *iecho;
    struct sockaddr_in    to;
    struct timeval        tout;
    int    size       =   32;
    size_t ping_size  =   sizeof(struct icmp_echo_hdr) + size;
    size_t data_len   =   ping_size - sizeof(struct icmp_echo_hdr);
    int    err;

    #ifdef TC_DBG
    Serial.printf("MQTT: Sending ping\n");
    #endif

    // We only PING if we have an IP address.
    // No point in avoiding a blocking connect
    // if a DNS call is required beforehand
    if(this->domain)
        return false;

    if((_s = socket(AF_INET, SOCK_RAW, IP_PROTO_ICMP)) < 0)
        return false;

    address.sin_addr.s_addr = ip;
    ping_target.addr        = ip;

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

int PubSubClient::pstate()
{
    return this->_pstate;
}

#endif  // TC_HAVEMQTT
