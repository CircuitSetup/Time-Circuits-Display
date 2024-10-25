/*
 *
 * PubSubClient.h - A simple client for MQTT.
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

#ifndef PubSubClient_h
#define PubSubClient_h

#include <Arduino.h>
#include <IPAddress.h>
#include <WiFiClient.h>

#define MQTT_VERSION_3_1      3
#define MQTT_VERSION_3_1_1    4

// MQTT_VERSION : Pick the version
//#define MQTT_VERSION MQTT_VERSION_3_1
#ifndef MQTT_VERSION
#define MQTT_VERSION MQTT_VERSION_3_1_1
#endif

// MQTT_MAX_PACKET_SIZE : Maximum packet size. Override with setBufferSize().
#ifndef MQTT_MAX_PACKET_SIZE
#define MQTT_MAX_PACKET_SIZE 512
#endif

// MQTT_KEEPALIVE : keepAlive interval in Seconds.
#ifndef MQTT_KEEPALIVE
#define MQTT_KEEPALIVE 15
#endif

// MQTT_SOCKET_TIMEOUT: socket timeout interval in Seconds.
#ifndef MQTT_SOCKET_TIMEOUT
#define MQTT_SOCKET_TIMEOUT 15
#endif

// MQTT_MAX_TRANSFER_SIZE : limit how much data is passed to the network client
//  in each write call. Needed for the Arduino Wifi Shield. Leave undefined to
//  pass the entire MQTT packet in each write call.
//#define MQTT_MAX_TRANSFER_SIZE 80

// Possible values for client.state()
#define MQTT_CONNECTING             -5
#define MQTT_CONNECTION_TIMEOUT     -4
#define MQTT_CONNECTION_LOST        -3
#define MQTT_CONNECT_FAILED         -2
#define MQTT_DISCONNECTED           -1
#define MQTT_CONNECTED               0
#define MQTT_CONNECT_BAD_PROTOCOL    1
#define MQTT_CONNECT_BAD_CLIENT_ID   2
#define MQTT_CONNECT_UNAVAILABLE     3
#define MQTT_CONNECT_BAD_CREDENTIALS 4
#define MQTT_CONNECT_UNAUTHORIZED    5

#define MQTTCONNECT     1 << 4  // Client request to connect to Server
#define MQTTCONNACK     2 << 4  // Connect Acknowledgment
#define MQTTPUBLISH     3 << 4  // Publish message
#define MQTTPUBACK      4 << 4  // Publish Acknowledgment
#define MQTTPUBREC      5 << 4  // Publish Received (assured delivery part 1)
#define MQTTPUBREL      6 << 4  // Publish Release (assured delivery part 2)
#define MQTTPUBCOMP     7 << 4  // Publish Complete (assured delivery part 3)
#define MQTTSUBSCRIBE   8 << 4  // Client Subscribe request
#define MQTTSUBACK      9 << 4  // Subscribe Acknowledgment
#define MQTTUNSUBSCRIBE 10 << 4 // Client Unsubscribe request
#define MQTTUNSUBACK    11 << 4 // Unsubscribe Acknowledgment
#define MQTTPINGREQ     12 << 4 // PING Request
#define MQTTPINGRESP    13 << 4 // PING Response
#define MQTTDISCONNECT  14 << 4 // Client is Disconnecting
#define MQTTReserved    15 << 4 // Reserved

#define MQTTQOS0        (0 << 1)
#define MQTTQOS1        (1 << 1)
#define MQTTQOS2        (2 << 1)

// Maximum size of fixed header and variable length size header
#define MQTT_MAX_HEADER_SIZE 5

#define PING_ERROR    -1
#define PING_IDLE     0
#define PING_PINGING  1

#define CHECK_STRING_LENGTH(l,s) if(l+2+strnlen(s, this->bufferSize) > this->bufferSize) { _client->stop(); return false; }

class PubSubClient {

    public:
        PubSubClient(WiFiClient& client);
    
        ~PubSubClient();
    
        void setServer(IPAddress ip, uint16_t port);
        void setServer(const char *domain, uint16_t port);
        void setCallback(void (*callback)(char *, uint8_t *, unsigned int));
        void setLooper(void (*looper)());
    
        bool setBufferSize(uint16_t size);
    
        bool connect(const char *id);
        bool connect(const char *id, const char *user, const char *pass);
        bool connect(const char *id, const char *user, const char *pass, bool cleanSession);
       
        void disconnect();

        bool publish(const char *topic, const uint8_t *payload, unsigned int plength, bool retained = false);
             
        bool subscribe(const char *topic, const char *topic2 = NULL, uint8_t qos = 0);
        bool unsubscribe(const char *topic);
        
        bool loop();
        
        bool connected();
        int  state();

        bool sendPing();
        bool pollPing();
        void cancelPing();
        int  pstate();
    
    private:

        bool subscribe_int(bool unsubscribe, const char *topic, const char *topic2L, uint8_t qos);
        uint32_t readPacket(uint8_t *);
        bool readByte(uint8_t *result);
        bool readByte(uint8_t *result, uint16_t *index);
        bool write(uint8_t header, uint8_t *buf, uint16_t length);
        uint16_t writeString(const char *string, uint8_t *buf, uint16_t pos);
        // Build up the header ready to send
        // Returns the size of the header
        // Note: the header is built at the end of the first MQTT_MAX_HEADER_SIZE bytes, so will start
        //       (MQTT_MAX_HEADER_SIZE - <returned size>) bytes into the buffer
        size_t buildHeader(uint8_t header, uint8_t* buf, uint16_t length);
       
        WiFiClient* _client;
        uint8_t* buffer;
        uint16_t bufferSize;
        uint16_t keepAlive;
        unsigned long socketTimeout;
        uint16_t nextMsgId;
        unsigned long lastOutActivity;
        unsigned long lastInActivity;
        bool pingOutstanding;
        void (*callback)(char *, uint8_t *, unsigned int);
        void (*looper)();

        IPAddress ip;
        const char* domain;
        uint16_t port;
        int _state;

        int _s;
        int _pstate = PING_IDLE;
        uint16_t _pseq_num = 34;
};

#endif
