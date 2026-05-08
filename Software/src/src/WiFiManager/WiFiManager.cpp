/**
 * WiFiManager.cpp
 *
 * Based on:
 * WiFiManager, a library for the ESP32/Arduino platform
 * Creator tzapu (tablatronix)
 * Version 2.0.15
 * License MIT
 *
 * Adapted by Thomas Winischhofer (A10001986)
 */

// prep string concat vars
#define WM_STRING2(x) #x
#define WM_STRING(x) WM_STRING2(x)

#include "WiFiManager.h"

#ifdef WM_PARAM2
#ifndef WM_PARAM2_CAPTION
#define WM_PARAM2_CAPTION   "Settings 2"
#endif
#ifndef WM_PARAM2_TITLE
#define WM_PARAM2_TITLE     "Settings 2"
#endif
#ifdef WM_PARAM3
#ifndef WM_PARAM3_CAPTION
#define WM_PARAM3_CAPTION   "Settings 3"
#endif
#ifndef WM_PARAM3_TITLE
#define WM_PARAM3_TITLE     "Settings 3"
#endif
#endif
#endif

#ifndef WM_PARAM2_TITLE
#define WM_PARAM2_TITLE     ""
#endif
#ifndef WM_PARAM3_TITLE
#define WM_PARAM3_TITLE     ""
#endif

#include "wm_strings_en.h"

//#include <freertos/atomic.h>

// params will autoincrement and realloc by this amount when max is reached
// can (and should) be overruled by allocParms()
#ifndef WIFI_MANAGER_MAX_PARAMS
#define WIFI_MANAGER_MAX_PARAMS 5
#endif

// Internal event bits (converted from system WiFi events)
#define WM_EVB_APSTART      (1<<0)
#define WM_EVB_APSTOP       (1<<1)
#define WM_EVB_STASTART     (1<<2)
#define WM_EVB_STASTOP      (1<<3)
#define WM_EVB_GOTIP        (1<<4)
#define WM_EVB_DISCONNECTED (1<<5)
#define WM_EVB_SCAN_DONE    (1<<6)
#define WM_EVB_STACONN      (1<<7)

// Flags for HTML Head generation
#define incGFXMSG  0x01
#define incQI      0x02
#define incSET     0x04
#define incUPL     0x08
#define incSTA     0x10
#define incC80     0x20
#define incUPLF    0x40

#define STRLEN(x) (sizeof(x)-1)

#define WM_WIFI_SCAN_BUSY -133

#define DNS_PORT           53

// Maximum buffer for scan list on WiFi Config page
#define MAX_SCAN_OUTPUT_SIZE  6144

#if defined(ESP_ARDUINO_VERSION) && defined(ESP_ARDUINO_VERSION_VAL)
    #if ESP_ARDUINO_VERSION < ESP_ARDUINO_VERSION_VAL(2,0,0)
        #define WM_NOCOUNTRY
    #endif
#else
    #define WM_NOCOUNTRY
#endif

/**********************************************************************************
 * --------------------------------------------------------------------------------
 *  WiFiManagerParameter Class
 * --------------------------------------------------------------------------------
 **********************************************************************************/

WiFiManagerParameter::WiFiManagerParameter(const char *id, const char *label, const char *defaultValue, int length, uint8_t flags)
{
    init(id, label, defaultValue, length, NULL, flags);
}

WiFiManagerParameter::WiFiManagerParameter(const char *id, const char *label, const char *defaultValue, int length, const char *custom, uint8_t flags)
{
    init(id, label, defaultValue, length, custom, flags);
}

WiFiManagerParameter::WiFiManagerParameter(const char *id, const char *label, const char *defaultValue, const char *custom, uint8_t flags)
{
    init(id, label, defaultValue, 1, custom, flags);
}

WiFiManagerParameter::WiFiManagerParameter(const char *custom, uint8_t flags)
{
    initC(custom, NULL, flags);
}

WiFiManagerParameter::WiFiManagerParameter(const char *(*CustomHTMLGenerator)(const char *, int), uint8_t flags)
{
    initC(NULL, CustomHTMLGenerator, flags);
}

WiFiManagerParameter::~WiFiManagerParameter()
{
    if(_id && _value != NULL) {
        delete[] _value;
        _value = NULL;
    }

    // setting length 0, ideally the entire parameter should be removed,
    // or added to wifimanager scope so it follows
    _length = 0;
}

void WiFiManagerParameter::init(const char *id, const char *label, const char *defaultValue, int length, const char *custom, uint8_t flags)
{
    _id             = id;
    _label          = label;
    _flags          = flags;
    _customHTML     = custom;
    _length         = 0;
    _value          = NULL;
    if(flags & WFM_IS_CHKBOX) length = 1;
    setValue(defaultValue, length);
}

void WiFiManagerParameter::initC(const char *custom, const char *(*CustomHTMLGenerator)(const char *, int), uint8_t flags)
{
    _id             = NULL;
    _label          = NULL;
    _length         = 0;
    //_value is union with Generator
    _flags          = flags;
    _customHTML     = custom;
    _customHTMLGenerator = CustomHTMLGenerator;
}

void WiFiManagerParameter::setValue(const char *defaultValue, int length)
{
    if(!_id)
        return;

    if(_length != length || !_value) {
        _length = length;
        if(_value) {
            delete[] _value;
        }
        _value  = new char[_length + 1];
    }

    setValue(defaultValue);
}

void WiFiManagerParameter::setValue(const char *defaultValue)
{
    if(!_id)
        return;

    memset(_value, 0, _length + 1); // +1 for 0-term
    if(defaultValue) {
        strncpy(_value, defaultValue, _length);
    }
}

/**********************************************************************************
 * --------------------------------------------------------------------------------
 *  WiFiManager Class
 * --------------------------------------------------------------------------------
 **********************************************************************************/

// Constructors

WiFiManager::WiFiManager()
{
    WiFiManagerInit();
}

void WiFiManager::WiFiManagerInit()
{
    for(int i = 0; i < WM_PARAM_ARRS; i++) {
        _max_params[i] = WIFI_MANAGER_MAX_PARAMS;
    }

    memset(_ssid, 0, sizeof(_ssid));
    memset(_pass, 0, sizeof(_pass));
    memset(_bssid, 0, sizeof(_bssid));
    memset(_apName, 0, sizeof(_apName));
    memset(_apPassword, 0, sizeof(_apPassword));
    _title = S_brand;
}

// destructor
WiFiManager::~WiFiManager()
{
    _end();

    // free parameter arrays
    for(int i = 0; i < WM_PARAM_ARRS; i++) {
        if(_params[i]) {
            free(_params[i]);
            _params[i] = NULL;
        }
        _paramsCount[i] = 0;
    }

    // remove event handler
    if(wm_event_id) {
        WiFi.removeEvent(wm_event_id);
    }
}

void WiFiManager::_begin()
{
    if(_beginSemaphore) return;
    _beginSemaphore = true;
}

void WiFiManager::_end()
{
    _beginSemaphore = false;
}

// Params handling

bool WiFiManager::CheckParmID(const char *id)
{
    for(int i = 0; i < strlen(id); i++) {
        if(!(isAlphaNumeric(id[i])) && !(id[i] == '_')) {
            return false;
        }
    }

    return true;
}

bool WiFiManager::addParameter(int idx, WiFiManagerParameter *p)
{
    // check param id is valid, unless null
    if(p->getID()) {
        if(!CheckParmID(p->getID())) return false;
    }

    // init params if never malloc
    if(!_params[idx]) {
        _params[idx] = (WiFiManagerParameter**)malloc(_max_params[idx] * sizeof(WiFiManagerParameter*));
    }

    // resize the params array by increment of WIFI_MANAGER_MAX_PARAMS
    if(_paramsCount[idx] == _max_params[idx]) {

        _max_params[idx] += WIFI_MANAGER_MAX_PARAMS;

        WiFiManagerParameter** new_params = (WiFiManagerParameter**)realloc(_params[idx], _max_params[idx] * sizeof(WiFiManagerParameter*));

        if(!new_params)
            return false;

        _params[idx] = new_params;
    }

    _params[idx][_paramsCount[idx]] = p;
    _paramsCount[idx]++;

    return true;
}

/****************************************************************************
 *
 * WiFi Connection (STA mode)
 *
 ****************************************************************************/

// wifiConnect: WiFi Connect
//
// public; return: bool = true if STA connected, false if fall-back to AP-mode

bool WiFiManager::wifiConnect(const char *ssid, const char *pass, const char *bssid, const char *apName, const char *apPassword)
{
    #ifdef _A10001986_DBG
    Serial.println("wifiConnect");
    #endif

    _begin();

    // Store given ssid/pass/bssid
    memset(_ssid, 0, sizeof(_ssid));
    memset(_pass, 0, sizeof(_pass));
    memset(_bssid, 0, sizeof(_bssid));
    if(ssid && *ssid) {
        strncpy(_ssid, ssid, sizeof(_ssid) - 1);
    }
    if(pass && *pass) {
        strncpy(_pass, pass, sizeof(_pass) - 1);
    }
    if(bssid && *bssid) {
        strncpy(_bssid, bssid, sizeof(_bssid) - 1);
    }

    _wifiOffFlag = 0;

    // We do never ever use NVS saved data, nor do we save data to NVS
    WiFi.persistent(false);

    // Install WiFi event handler
    WiFi_installEventHandler();

    // Set hostname (if given)

    // The setHostname() function must be called BEFORE Wi-Fi is started
    // with WiFi.begin(), WiFi.softAP(), WiFi.mode(), or WiFi.run(). To change
    // the name, reset Wi-Fi with WiFi.mode(WIFI_MODE_NULL), then proceed
    // with WiFi.setHostname(...) and restart WiFi from scratch.

    if(*_hostname) {
        if(WiFi.getMode() & (WIFI_STA|WIFI_AP)) {
            andWiFiEventMask(~(WM_EVB_STASTOP|WM_EVB_APSTOP));
            WiFi.mode(WIFI_OFF);
            waitEvent(WM_EVB_STASTOP|WM_EVB_APSTOP, 1200);
        }
        WiFi.setHostname(_hostname);
    }

    // Attempt to connect to given network, on fail fallback to AP config portal

    // Set STA mode (not strictly required, WiFi.begin will do that as well)
    // This does not actually connect, it just sets (current mode|STA)
    if(!wifiSTAOn()) {
        #ifdef _A10001986_DBG
        Serial.println("[FATAL] Unable to enable STA mode");
        #endif
        return false;
    }

    // Callback: After lowlevelinit
    #if WM_PRECONNECTCB
    if(_preconnectcallback) {
        _preconnectcallback();
    }
    #endif

    bool connected = false;

    // If, for some reason, we already are connected, check
    // to which network, and act accordingly.
    if(WiFi.status() == WL_CONNECTED) {
        if(ssid && *ssid) {
            if(!strcmp(ssid, WiFi.SSID().c_str())) {
                connected = true;
                _lastconxresult = WL_CONNECTED;
                // Connected is connected, including IP. Don't mess
                // with this here.
                //setStaticConfig();
                #ifdef _A10001986_DBG
                Serial.println("wifiConnect: Already connected");
                #endif
            } else {
                #ifdef _A10001986_DBG
                Serial.println("wifiConnect: Already connected to different network, disconnecting...");
                #endif
                andWiFiEventMask(~WM_EVB_DISCONNECTED);
                WiFi.disconnect();
                waitEvent(WM_EVB_DISCONNECTED, 2000);
            }
        }
    }

    if((!ssid || !*ssid) && !connected) {

        _lastconxresult = TWL_STATUS_NONE;

    } else if(connected || (connectWifi(ssid, pass, bssid) == WL_CONNECTED)) {

        #ifdef _A10001986_DBG
        Serial.printf("wifiConnect: SUCCESS\nSTA IP Address: %s\n", WiFi.localIP().toString().c_str());
        if(*_hostname) {
            Serial.printf("hostname: %s\n", WiFi.getHostname());
        }
        #endif

        // Needs to be set AFTER WiFi is up
        #ifndef WM_NOCOUNTRY
        esp_wifi_set_country_code("01", true);
        #endif

        // init mDNS
        setupMDNS();

        return true; // connected success
    }

    // Not connected:

    #ifdef _A10001986_DBG
    Serial.println("wifiConnect: Not connected");
    #endif

    // Disable STA if enabled
    wifiSTAOff();

    // Start softAP and Web portal

    startAPModeAndPortal(apName, apPassword, ssid, pass, bssid);

    return false; // Yes, false; true means "connected".
}

// Start/stop web portal in STA mode
//
// public

void WiFiManager::startWebPortal()
{
    if(APPortalActive || STAPortalActive)
        return;

    // Start WebServer
    setupHTTPServer();

    // Reset network scan cache
    _lastscan = 0;

    STAPortalActive = true;
}

void WiFiManager::stopWebPortal()
{
    if(!STAPortalActive)
        return;

    #ifdef _A10001986_DBG
    Serial.println("Stopping Web Portal (STA mode)");
    #endif

    shutdownWebPortal();
    STAPortalActive = false;
}

// connectWiFi: Connect to given network, and retry if failed
//
// private; return: uint8: connection result WL_XXX

uint8_t WiFiManager::connectWifi(const char *ssid, const char *pass, const char *bssid)
{
    const uint8_t *pbssid = NULL;
    unsigned int  b[6];
    uint8_t       br[6];

    uint8_t       connRes = (uint8_t)WL_NO_SSID_AVAIL;    // Init to anything != WL_CONNECTED
    uint8_t       retry = 1;
    bool          DHCPtimeout = false;
    bool          waitTimedOut = false;

    #ifdef _A10001986_V_DBG
    Serial.println("connectWifi");
    #endif

    _badBSSID = false;

    if(bssid && *bssid) {
        if(sscanf(bssid, "%x:%x:%x:%x:%x:%x", &b[0], &b[1], &b[2], &b[3], &b[4], &b[5]) == 6) {
            int j = 0;
            for(int i = 0; i < 6; i++) {
                if(b[i] <= 255) br[i] = b[i];
                else j++;
            }
            if(!j) {
                pbssid = br;
                if(_connectRetries == 1) _connectRetries++;
                #ifdef _A10001986_DBG
                Serial.printf("BSSID: %02x:%02x:%02x:%02x:%02x:%02x\n", b[0], b[1], b[2], b[3], b[4], b[5]);
                #endif
            } else {
                #ifdef _A10001986_DBG
                Serial.println("[WARN] bad BSSID");
                #endif
            }
        }
    }

    // set Static IP if provided
    bool haveStatic = setStaticConfig();

    // disconnect() before begin(), if status is != DISCONNECTED
    uint8_t bstatus = (uint8_t)WiFi.status();
    #ifdef _A10001986_DBG
    Serial.printf("connectWiFi: Status before connect: %d\n", bstatus);
    #endif
    if(bstatus != WL_DISCONNECTED) {
        WiFi.disconnect();
        #ifdef _A10001986_DBG
        Serial.println("connectWiFi: disconnect()");
        #endif
        _delay(1000);
    }

    while((connRes != WL_CONNECTED) && (retry <= _connectRetries)) {

        if(_connectRetries > 1) {

            // Add a delay before calling WiFi.begin() again unless
            // waitForConnectResult() timed-out.
            if(retry > 1 && !waitTimedOut) {
                _delay(1000);
            }

            #ifdef _A10001986_DBG
            Serial.printf("Connecting Wifi, attempt %d of %d\n", retry, _connectRetries);
            #endif

            // We try without a user-provided BSSID on the last connection attempt.
            // Still better than to fall-back to AP mode.
            if(retry == _connectRetries) {
                if(pbssid) {
                    _badBSSID = true;
                    pbssid = NULL;
                    #ifdef _A10001986_DBG
                    Serial.println("Clearing BSSID for last connection attempt");
                    #endif
                }
            }
        }

        andWiFiEventMask(~(WM_EVB_GOTIP|WM_EVB_STACONN));
        DHCPtimeout = false;

        // If the begin() call fails, there is no point in waiting or retrying.
        if((connRes = WiFi.begin(ssid, pass, 0, pbssid, true)) == WL_CONNECT_FAILED)
            break;

        connRes = waitForConnectResult(haveStatic, _connectTimeout, waitTimedOut, DHCPtimeout);

        #ifdef _A10001986_V_DBG
        Serial.printf("Connection result: %s\n", getWLStatusString(connRes).c_str());
        #endif

        retry++;
    }

    _lastconxresult = DHCPtimeout ? TWL_DHCP_TIMEOUT : connRes;

    #ifdef _A10001986_V_DBG
    Serial.printf("ConnectWifi returning %d\n", connRes);
    #endif

    return connRes;
}

// waitForConnectResult
//
// private; return: uint8_t WL_XXX Status

uint8_t WiFiManager::waitForConnectResult(bool haveStatic, unsigned long timeout, bool& waitTimedOut, bool& DHCPtimeout)
{
    unsigned long startmillis;
    uint8_t       status;
    bool          waitforDHCP = false;

    // Event 7 (GOT_IP) is fired both with a static config
    // as well as DHCP. So we overrule the parameter in order
    // to wait in both cases.
    haveStatic = false;

    waitTimedOut = false;

    if(!timeout) {
        #ifdef _A10001986_DBG
        Serial.println("connectTimeout not set, waitForConnectResult...");
        #endif

        status = WiFi.waitForConnectResult();

        // WL_CONNECTED means "connected to AP, IP address obtained"
        // No point in waiting for the event
        //if(status == WL_CONNECTED) {
        //    if(!haveStatic) {
        //        waitEvent(WM_EVB_GOTIP, 5000);
        //    }
        //}

        return status;
    }

    #ifdef _A10001986_V_DBG
    Serial.printf("%s ms timeout, waiting for connect...\n", timeout);
    #endif

    startmillis = millis();
    status = WiFi.status();

    /* This is the core's waitForConnectResult():
     * Does not work for our purposes.
     * 1) WiFi runs async, and reports some stati with a delay (especially when changing
     *    wifi config between calls of begin(), such as with/without BSSID)
     * 2) It does not use our _delay(), of course
     while((!status() || status() >= WL_DISCONNECTED) && (millis() - start) < timeoutLength) {
        delay(100);
     }
     */

    while(millis() - startmillis < timeout) {

        status = WiFi.status();

        #ifdef _A10001986_DBG
        Serial.printf(". (%d)\n", status);
        #endif

        if(status == WL_CONNECT_FAILED) {

            // Can't quit on WL_NO_SSID_AVAIL, because this is sent for
            // a while after first attempt timed out; and also when BSSID
            // is cleared, at first WL_NO_SSID_AVAIL comes along before
            // CONNECT.
            // Also: I don't think WL_CONNECT_FAILED is ever sent over status(),
            // it looks like WL_CONNECT_FAILED is only used for return values.

            return status;

        } else if(status == WL_CONNECTED) {

            // WL_CONNECTED means "connected to AP, IP address obtained"
            // No point in waiting for the event
            //if(!haveStatic) {
            //    waitEvent(WM_EVB_GOTIP, 5000);
            //}

            return status;
        }

        // Upon STA-Connect, we renew our timeout for receiving
        // our DHCP packet.
        if(!haveStatic) {
            if((_WiFiEventMask & WM_EVB_STACONN) && !waitforDHCP) {
                startmillis = millis();
                waitforDHCP = true;
                #ifdef _A10001986_DBG
                Serial.println("Restarting Timeout for DHCP");
                #endif
            }
        }

        _delay(100);
    }

    #ifdef _A10001986_DBG
    Serial.printf("Timeout (%d)\n", status);
    #endif

    DHCPtimeout = waitforDHCP;
    waitTimedOut = true;

    return status;
}

// setStaticConfig: Set static IP config (if configured)
//
// private; return: bool success = result from WiFi.config()

bool WiFiManager::setStaticConfig()
{
    if(_sta_static_ip) {
        bool ret;

        #ifdef _A10001986_DBG
        Serial.printf("STA static IP: %s\n", _sta_static_ip.toString().c_str());
        #endif

        if(_sta_static_dns) {
            ret = WiFi.config(_sta_static_ip, _sta_static_gw, _sta_static_sn, _sta_static_dns);
        } else {
            ret = WiFi.config(_sta_static_ip, _sta_static_gw, _sta_static_sn);
        }

        #ifdef _A10001986_DBG
        if(!ret) Serial.println("[ERROR] wifi config failed");
        #ifdef _A10001986_V_DBG
        else Serial.printf("STA IP set: %s\n", WiFi.localIP().toString().c_str());
        #endif
        #endif

        return ret;

    } else {

        #ifdef _A10001986_DBG
        Serial.println("No static IP configured");
        #endif
    }

    return false;
}

/****************************************************************************
 *
 * AP mode
 *
 ****************************************************************************/

// startAPModeAndPortal: Start AP-Mode incl. Web Portal
//
// public; return: bool success (result from password check)

bool WiFiManager::startAPModeAndPortal(char const *apName, char const *apPassword, const char *ssid, const char *pass, const char *bssid)
{
    _begin();

    if(APPortalActive) {
        #ifdef _A10001986_V_DBG
        Serial.println("Config Portal is already running");
        #endif
        return false;
    }

    _wifiOffFlag = 0;

    // We never ever use NVS saved data, nor do we write to NVS
    WiFi.persistent(false);

    // setup AP
    memset(_apName, 0, sizeof(_apName));
    memset(_apPassword, 0, sizeof(_apPassword));
    if(apName && *apName) {
        strncpy(_apName, apName, sizeof(_apName) - 1);
    }
    if(apPassword && *apPassword) {
        strncpy(_apPassword, apPassword, sizeof(_apPassword) - 1);
    }

    // SSID/pass only used for status report and pre-filling forms
    memset(_ssid, 0, sizeof(_ssid));
    memset(_pass, 0, sizeof(_pass));
    memset(_bssid, 0, sizeof(_bssid));
    if(ssid && *ssid) {
        strncpy(_ssid, ssid, sizeof(_ssid) - 1);
    }
    if(pass && *pass) {
        strncpy(_pass, pass, sizeof(_pass) - 1);
    }
    if(bssid && *bssid) {
        strncpy(_bssid, bssid, sizeof(_bssid) - 1);
    }

    #ifdef _A10001986_V_DBG
    Serial.println("Starting Config Portal");
    #endif

    if(!*_apName) getDefaultAPName(_apName);

    if(*_apPassword) {
        size_t t = strlen(_apPassword);
        if(t < 8 || t > 63) {
            return false;
        }
    }

    // Install WiFi event handler
    WiFi_installEventHandler();

    // Disconnect if connected
    if(WiFi.isConnected()) {
        WiFi.disconnect();
        _delay(1000);
    }

    // Disable STA if enabled
    wifiSTAOff();

    // start access point
    #ifdef _A10001986_V_DBG
    Serial.println("Starting AP");
    #endif

    startAP();

    // Needs to be set AFTER WiFi is up
    #ifndef WM_NOCOUNTRY
    esp_wifi_set_country_code("01", true);
    #endif

    // do AP callback if set
    #ifdef WM_APCALLBACK
    if(_apcallback) {
        _apcallback(this);
    }
    #endif

    // Start WebServer
    setupHTTPServer();

    // Start DNS server
    setupDNSD();

    // Start mDNS
    setupMDNS();

    // Reset network scan cache
    _lastscan = 0;

    APPortalActive = true;

    #ifdef _A10001986_V_DBG
    Serial.println("Config Portal Running");
    #endif

    return true;
}

// stopAPModeAndPortal:
//
// Shut down APMode incl. WebPortal, DNS, MDNS, etc.

bool WiFiManager::stopAPModeAndPortal()
{
    #ifdef _A10001986_DBG
    Serial.println("Stopping Web Portal (AP mode)");
    #endif

    return shutdownWebPortal();
}

// startAP: start softAP
//
// private; return: bool success

bool WiFiManager::startAP()
{
    int32_t channel = _apChannel;
    bool ret = true;

    #ifdef _A10001986_DBG
    Serial.printf("StartAP with SSID: %s\n", _apName);
    #endif

    #ifdef _A10001986_DBG
    if(channel > 0) {
        Serial.printf("Starting AP on channel: %d\n", channel);
    }
    #endif

    // Enable AP mode. This is also called by softAP(), but
    // letting it be done there leads to a series of AP_START,
    // AP_STOP, AP_START events, difficult to follow.
    // In order to wait orderly, we do it here first.
    if(!(WiFi.getMode() & WIFI_AP)) {
        andWiFiEventMask(~WM_EVB_APSTART);
        if(WiFi.enableAP(true)) {
            waitEvent(WM_EVB_APSTART, 3000);
        } else {
            #ifdef _A10001986_DBG
            Serial.println("Error enabling AP mode");
            #endif
        }
    }

    // Set up optional soft AP static ip config
    // This enables AP mode if not already enabled, it might
    // cause another series of AP_STOP, AP_START events.
    // We don't use it, so I never tested it.
    #ifdef WM_AP_STATIC_IP
    if(_ap_static_ip) {
        if(!WiFi.softAPConfig(_ap_static_ip, _ap_static_gw, _ap_static_sn)) {
            #ifdef _A10001986_DBG
            Serial.println("[ERROR] softAPConfig failed"));
            #endif
        }
    }
    #endif

    // set ap hostname
    if(*_hostname) {
        if(!WiFi.softAPsetHostname(_hostname)) {
            #ifdef _A10001986_DBG
            Serial.println("Failed to set AP hostname");
            #endif
        }
    }

    // Now fire softAP() which doesn't do much beyond setting our
    // config and restarting the AP, and comparing the then current
    // AP config to the desired one.

    andWiFiEventMask(~WM_EVB_APSTART);

    // start soft AP; default channel is 1 (as is the system's)
    ret = WiFi.softAP(_apName,
            (strlen(_apPassword) > 0) ? _apPassword : NULL,
            (channel > 0) ? channel : 1,
            false,
            _ap_max_clients);

    // Wait for AP_START event to make sure we can fully use the AP
    if(ret) {
        waitEvent(WM_EVB_APSTART, 1000);
    }

    #ifdef _A10001986_DBG_AP
    debugSoftAPConfig();
    #endif

    #ifdef _A10001986_DBG
    if(!ret) {
        Serial.println("[ERROR] Starting the AP failed");
    } else {
        Serial.printf("AP IP address: %s\n", WiFi.softAPIP().toString().c_str());
        const char *aphn = WiFi.softAPgetHostname();
        Serial.printf("AP hostname: %s\n", aphn ? aphn : "UNSET");
    }
    #endif

    return ret;
}


/****************************************************************************
 *
 * Server setup: Web, DNS, mDNS - all private
 *
 ****************************************************************************/


void WiFiManager::setupHTTPServer()
{
    #ifdef _A10001986_DBG
    Serial.println("Starting Web Server");
    #endif

    server.reset(new WebServer(_httpPort));
    // This is not the safest way to reset the webserver, it can cause crashes
    // on callbacks initialized before this and since its a shared pointer...

    if(_webservercallback) {
        _webservercallback();
    }

    // Setup httpd callbacks

    server->on(R_root,       std::bind(&WiFiManager::handleRoot, this));
    server->on(R_wifi,       std::bind(&WiFiManager::handleWifi, this, true));
    server->on(R_wifisave,   HTTP_POST, std::bind(&WiFiManager::handleWifiSave, this));
    server->on(R_param,      std::bind(&WiFiManager::handleParam, this));
    server->on(R_paramsave,  HTTP_POST, std::bind(&WiFiManager::handleParamSave, this));
    #ifdef WM_PARAM2
    server->on(R_param2,     std::bind(&WiFiManager::handleParam2, this));
    server->on(R_param2save, HTTP_POST, std::bind(&WiFiManager::handleParam2Save, this));
    #ifdef WM_PARAM3
    server->on(R_param3,     std::bind(&WiFiManager::handleParam3, this));
    server->on(R_param3save, HTTP_POST, std::bind(&WiFiManager::handleParam3Save, this));
    #endif
    #endif
    server->on(R_update,     std::bind(&WiFiManager::handleUpdate, this));
    server->on(R_updatedone, HTTP_POST, std::bind(&WiFiManager::handleUpdateDone, this), std::bind(&WiFiManager::handleUpdating, this));

    server->onNotFound(std::bind(&WiFiManager::handleNotFound, this));

    // Web server start

    server->begin();

    #ifdef _A10001986_DBG
    Serial.println("HTTP server started");
    #endif
}

void WiFiManager::setupDNSD()
{
    dnsServer.reset(new DNSServer());

    // The DNS server is a single-domain DNS.
    // If a hostname is set, we let it resolve it to our IP.
    // If no hostname is set, we resolve ALL requests to our IP.

    if(!*_hostname) {
        dnsServer->setErrorReplyCode(DNSReplyCode::NoError);
    }

    dnsServer->start(DNS_PORT,
              *_hostname ? String(_hostname) : F("*"),
              WiFi.softAPIP());
}

void WiFiManager::setupMDNS()
{
    #ifdef WM_MDNS
    if(MDNS.begin(_hostname)) {
        MDNS.addService("http", "tcp", 80);
        _mdnsStarted = true;
    }
    #endif
}

/****************************************************************************
 *
 * process() - loop() function
 *
 ****************************************************************************/

// process: Loop function to process Webserver,
//          DNS server and some cleanup.
// public

void WiFiManager::process(bool handleWeb)
{
    if(STAPortalActive || APPortalActive) {

        // DNS handler
        if(APPortalActive && dnsServer) {
            dnsServer->processNextRequest();
        }

        // HTTP handler
        if(handleWeb && server) {
            server->handleClient();
        }

        // Free memory taken for scans
        if(_lastscan && (millis() - _lastscan > _scancachetime)) {
            if((WiFi.scanComplete() != WIFI_SCAN_RUNNING) &&
               (_numNetworksAsync != WM_WIFI_SCAN_BUSY)) {
                WiFi.scanDelete();
                _numNetworks = 0;
                _numNetworksAsync = 0;    // This is as atomic as it gets
                _lastscan = 0;
                #ifdef _A10001986_DBG
                Serial.println("Freeing scan result memory");
                #endif
            }
        }

    } else if(_wifiOffFlag) {

        switch(_wifiOffFlag) {
        case 1:
          if( (_WiFiEventMask & WM_EVB_APSTART) ||   /*WM_EVB_APSTOP*/
              (millis() - _wifiOffNow > 2000) ) {
              _wifiOffNow = millis();
              _wifiOffFlag++;
          }
          break;
        case 2:
          if(millis() - _wifiOffNow > 1000) {
              WiFi.mode(WIFI_OFF);
              _wifiOffFlag = 0;
              #ifdef _A10001986_DBG
              Serial.println("WiFi turned off");
              #endif
          }
        }

    }
}

/****************************************************************************
 *
 * Shutdown
 *
 ****************************************************************************/


// disableWiFi
//
// disable WiFi no matter what: Shutdown AP mode (incl Web, DNS, etc), or
// disable STA mode (incl Web).
//
// public

void WiFiManager::disableWiFi(bool waitForOFF)
{
    wifi_mode_t md = WiFi.getMode();

    if(md == WIFI_OFF)
        return;

    if(md & WIFI_AP) {
        shutdownWebPortal();  // Clears WM_EVB_APSTOP|WM_EVB_APSTART
        // If we don't wait, an immediate re-connect
        // will result in same run-time errors as
        // described for softAPdisconnect(true).
        if(waitForOFF && _wifiOffFlag) {
            // (Why START, not STOP? Because softAPdisconnect restarts the AP
            // with a zero SSID; in this process, a STOP and a START event are
            // sent; if we wait for STOP, we might switch off WiFi while the
            // AP is restarted with that zero-SSID; by waiting for START, we
            // stop it right after it has started already.)
            if(!waitEvent(WM_EVB_APSTART, 2000)) {        /*WM_EVB_APSTOP*/
                #ifdef _A10001986_DBG
                Serial.println("disableWiFi: Waiting for WM_EVB_APSTART timed-out");
                #endif
            }
            _delay(1000);   // Apparently need this to avoid possible race/crash
            WiFi.mode(WIFI_OFF);
            _wifiOffFlag = 0;
            return;
        }
    } else if(md & WIFI_STA) {
        stopWebPortal();
        WiFi.mode(WIFI_OFF);
    }
}

// shutdownWebPortal:
//
// Shut down Webserver, MDNS, and if in AP mode, also DNS server and softAP
//
// This is used to shut down AP mode operation entirely, as well as
// the web portal in STA mode.
//
// private; return: bool success

bool WiFiManager::shutdownWebPortal()
{
    #ifdef _A10001986_V_DBG
    Serial.println("shutdownWebPortal");
    #endif

    if(APPortalActive && dnsServer) {
        dnsServer->processNextRequest();
    }

    if(server) {
        server->handleClient();

        // @todo what is the proper way to shutdown and free the server up
        // debug - many open issues about port not clearing for use with other servers
        server->stop();
        server.reset();
    }

    // free wifi scan results
    WiFi.scanDelete();
    _numNetworks = 0;
    _numNetworksAsync = 0;
    _lastscan = 0;

    // Stop MDNS
    #ifdef WM_MDNS
    if(_mdnsStarted) {
        MDNS.end();
        _mdnsStarted = false;
    }
    #endif

    // Bail here if we are not in AP mode
    if(!APPortalActive)
        return true;

    // Stop DNS server
    if(dnsServer) {
        dnsServer->stop();
        dnsServer.reset();
    }

    // Turn off AP (true = set mode WIFI_OFF)
    // softAPdisconnect(true) causes run-time errors:
    // E (611610) wifi_netif: esp_wifi_internal_reg_rxcb for if=1 failed with 12289
    // E (611611) wifi_init_default: esp_wifi_register_if_rWxcb for if=0x3ffb582c failed with 259

    andWiFiEventMask(~(WM_EVB_APSTOP|WM_EVB_APSTART));
    bool ret = WiFi.softAPdisconnect(false);
    if(ret) {
        _wifiOffFlag = 1;
        _wifiOffNow = millis();
    }
    // softAPdisconnect returns false if WiFi is already off or error;
    // in that case, do not wait for anything. Otherwise:
    // WiFi will be switched off in process() after AP_START event
    // (Why START, not STOP? Because softAPdisconnect restarts the AP
    // with a zero SSID; in this process, a STOP and a START event are
    // sent; if we wait for STOP, we might switch off WiFi while the
    // AP is restarted with that zero-SSID; by waiting for START, we
    // stop it right after it has started already)

    APPortalActive = false;

    #ifdef _A10001986_DBG
    Serial.println("configportal & softAP closed");
    #endif

    return ret;
}


/****************************************************************************
 *
 * Helpers
 *
 ****************************************************************************/

bool WiFiManager::wifiSTAOn()
{
    if(WiFi.getMode() & WIFI_STA)
        return true;

    andWiFiEventMask(~WM_EVB_STASTART);

    if(!WiFi.enableSTA(true))
        return false;

    waitEvent(WM_EVB_STASTART, 500);

    return true;
}

bool WiFiManager::wifiSTAOff()
{
    if(!(WiFi.getMode() & WIFI_STA))
        return true;

    andWiFiEventMask(~WM_EVB_STASTOP);

    if(!WiFi.enableSTA(false))
        return false;

    waitEvent(WM_EVB_STASTOP, 500);

    return true;
}

bool WiFiManager::waitEvent(uint32_t mask, unsigned long timeout)
{
    unsigned long now = millis();

    while(millis() - now < timeout) {
        if(_WiFiEventMask & mask) {
            #ifdef _A10001986_DBG
            Serial.printf("Event 0x%x occured (evmask 0x%x)\n", mask, _WiFiEventMask);
            #endif
            return true;
        }
        _delay(100);
    }

    #ifdef _A10001986_DBG
    Serial.printf("Waiting for event 0x%x timed-out\n", mask);
    #endif

    return false;
}


/****************************************************************************
 *
 * Website handling: Constructors for all pages
 *
 ****************************************************************************/

unsigned int WiFiManager::calcTitleLen(const char *title)
{
    unsigned int s = STRLEN(HTTP_ROOT_MAIN) - (2*3);
    s += strlen(_title);
    if(!title) {
        if(APPortalActive) {
            s += strlen(_apName);
        } else {
            s += strlen(WiFi.getHostname()) + 3 + 15;
        }
    } else {
        s += strlen(title);
    }

    return s;
}

// Calc size of header
unsigned int WiFiManager::getHTTPHeadLength(const char *title, uint32_t incFlags)
{
    int bufSize = STRLEN(HTTP_HEAD_START) - 3;  // -3 token

    #ifdef INDIV_TITLES
    bufSize += strlen(title ? title : _title);
    #else
    bufSize += strlen(_title);
    #endif
    bufSize += STRLEN(HTTP_SCRIPT) + STRLEN(HTTP_STYLE) + STRLEN(HTTP_STYLE_END);
    if(incFlags & incSTA) {
        bufSize += STRLEN(HTTP_STYLE_STA);
    }
    if(incFlags & incGFXMSG) {
        bufSize += STRLEN(HTTP_STYLE_MSG);
    }
    if(incFlags & incC80) {
        bufSize += STRLEN(HTTP_STYLE_C80);
        if(incFlags & incUPLF) {
            bufSize += STRLEN(HTTP_STYLE_UPLF);
        } else {
            bufSize += STRLEN(HTTP_STYLE_UPLN);
        }
    }
    if(incFlags & incQI) {
        bufSize += STRLEN(HTTP_SCRIPT_QI) + STRLEN(HTTP_STYLE_QI);
    }
    if(incFlags & incSET) {
        bufSize += STRLEN(HTTP_STYLE_SET);
    }
    if(incFlags & incUPL) {
        bufSize += STRLEN(HTTP_SCRIPT_UPL) + STRLEN(HTTP_STYLE_UPL);
    }

    if(_customHeadElement) {
        bufSize += strlen(_customHeadElement);
    }
    bufSize += STRLEN(HTTP_HEAD_END);

    bufSize += calcTitleLen(title);

    return bufSize;
}

// Construct header
void WiFiManager::getHTTPHeadNew(String& page, const char *title, uint32_t incFlags)
{
    String temp;
    #ifdef INDIV_TITLES
    temp.reserve(STRLEN(HTTP_HEAD_START) + strlen(title ? title : _title));
    #else
    temp.reserve(STRLEN(HTTP_HEAD_START) + strlen(_title));
    #endif

    temp = FPSTR(HTTP_HEAD_START);
    #ifdef INDIV_TITLES
    temp.replace(FPSTR(T_v), title ? title : _title);
    #else
    temp.replace(FPSTR(T_v), _title);
    #endif
    page += temp;

    page += FPSTR(HTTP_SCRIPT);
    if(incFlags & incUPL) {
        page += HTTP_SCRIPT_UPL;
    }
    if(incFlags & incQI) {
        page += HTTP_SCRIPT_QI;
    }
    page += FPSTR(HTTP_STYLE);    // closes <script>
    if(incFlags & incGFXMSG) {
        page += FPSTR(HTTP_STYLE_MSG);
    }
    if(incFlags & incSTA) {
        page += FPSTR(HTTP_STYLE_STA);
    }
    if(incFlags & incC80) {
        page += FPSTR(HTTP_STYLE_C80);
        if(incFlags & incUPLF) {
            page += FPSTR(HTTP_STYLE_UPLF);
        } else {
            page += FPSTR(HTTP_STYLE_UPLN);
        }
    }
    if(incFlags & incQI) {
        page += FPSTR(HTTP_STYLE_QI);
    }
    if(incFlags & incSET) {
        page += FPSTR(HTTP_STYLE_SET);
    }
    if(incFlags & incUPL) {
        page += FPSTR(HTTP_STYLE_UPL);
    }
    page += FPSTR(HTTP_STYLE_END);

    if(_customHeadElement) {
        page += _customHeadElement;
    }
    page += FPSTR(HTTP_HEAD_END);

    String str;
    str.reserve(calcTitleLen(title) + 8);
    str = FPSTR(HTTP_ROOT_MAIN);
    str.replace(FPSTR(T_t), _title);
    if(!title) {
        str.replace(FPSTR(T_v), APPortalActive ? _apName : (String(WiFi.getHostname()) + " - " + WiFi.localIP().toString()));
    } else {
        str.replace(FPSTR(T_v), title);
    }
    page += str;
}

// WIFI status at bottom of pages

int WiFiManager::reportStatusLen(bool withMac)
{
    int bufSize = 0;
    String SSID = String(_ssid);

    #ifdef _A10001986_V_DBG
    Serial.printf("reportStatus: _lastconxresult %d\n", _lastconxresult);
    #endif

    bufSize = STRLEN(HTTP_STATUS_HEAD) - 3 + 1; // - token + class for <span>
    if(SSID != "") {
        bufSize += htmlEntitiesLen(SSID, true);
        if(WiFi.status() == WL_CONNECTED) {
            bufSize += STRLEN(HTTP_STATUS_ON) - (4*3);
            bufSize += 15;                                              // max len of ip address
            if(*_bssid)   bufSize += (STRLEN(HTTP_BSSID_FOOT)-2+17+1);  // bssid
            if(_badBSSID) bufSize += STRLEN(HTTP_STATUS_BADBSSID);
        } else {
            bufSize += STRLEN(HTTP_STATUS_OFF) - (3*3);
            switch(_lastconxresult) {
            case TWL_DHCP_TIMEOUT:          // dhcp timeout
                bufSize += (STRLEN(HTTP_STATUS_NODHCP) + STRLEN(HTTP_STATUS_APMODE));
                break;
            case WL_NO_SSID_AVAIL:          // ap not found
                bufSize += (STRLEN(HTTP_STATUS_OFFNOAP) + STRLEN(HTTP_STATUS_APMODE));
                break;
            case WL_CONNECT_FAILED:         // connect failed
            case WL_CONNECTION_LOST:
                bufSize += (STRLEN(HTTP_STATUS_OFFFAIL) + STRLEN(HTTP_STATUS_APMODE));
                break;
            case WL_DISCONNECTED:
                bufSize += (STRLEN(HTTP_STATUS_DISCONN) + STRLEN(HTTP_STATUS_APMODE));
                break;
            default:
                bufSize += _carMode ? STRLEN(HTTP_STATUS_CARMODE) : STRLEN(HTTP_STATUS_APMODE);
                break;
            }
        }
    } else {
        bufSize += STRLEN(HTTP_STATUS_NONE);
    }

    if(withMac) bufSize += 4 + 18;  // <br>, MAC address

    bufSize += STRLEN(HTTP_STATUS_TAIL);

    #ifdef _A10001986_DBG
    Serial.printf("WM: reportStatusLen bufSize %d\n", bufSize);
    #endif

    return bufSize + 8;
}

void WiFiManager::reportStatus(String& page, unsigned int estSize, bool withMac)
{
    char pbssid[STRLEN(HTTP_BSSID_FOOT)-2+17+1];
    String SSID = String(_ssid);
    String str;
    if(estSize) str.reserve(estSize);

    str = FPSTR(HTTP_STATUS_HEAD);
    if(SSID != "") {
        if(WiFi.status() == WL_CONNECTED) {
            str += FPSTR(HTTP_STATUS_ON);
            if(_badBSSID) {
                str.replace(FPSTR(T_V), FPSTR(HTTP_STATUS_BADBSSID));
                str.replace(FPSTR(T_c), "o");
            } else {
                str.replace(FPSTR(T_V), "");
                str.replace(FPSTR(T_c), "g");
            }
            memset(pbssid, 0, sizeof(pbssid));
            //if(*_bssid) {
                snprintf(pbssid, STRLEN(HTTP_BSSID_FOOT)-2+17+1, HTTP_BSSID_FOOT, WiFi.BSSIDstr().c_str());
                if(strlen(pbssid) <= STRLEN(HTTP_BSSID_FOOT)-2+1) *pbssid = 0;
            //}
            str.replace(FPSTR(T_I), pbssid);
            str.replace(FPSTR(T_i), WiFi.localIP().toString());
            str.replace(FPSTR(T_v), htmlEntities(SSID, true));
        } else {
            str += FPSTR(HTTP_STATUS_OFF);
            str.replace(FPSTR(T_v), htmlEntities(SSID, true));
            switch(_lastconxresult) {
            case TWL_DHCP_TIMEOUT:    // dhcp timeout
                str.replace(FPSTR(T_c), "r");
                str.replace(FPSTR(T_r), FPSTR(HTTP_STATUS_NODHCP));
                str.replace(FPSTR(T_V), FPSTR(HTTP_STATUS_APMODE));
                break;
            case WL_NO_SSID_AVAIL:    // connect failed, or ap not found
                str.replace(FPSTR(T_c), "r");
                str.replace(FPSTR(T_r), FPSTR(HTTP_STATUS_OFFNOAP));
                str.replace(FPSTR(T_V), FPSTR(HTTP_STATUS_APMODE));
                break;
            case WL_CONNECT_FAILED:   // connect failed
            case WL_CONNECTION_LOST:  // connect failed, state is ambiguous
                str.replace(FPSTR(T_c), "r");
                str.replace(FPSTR(T_r), FPSTR(HTTP_STATUS_OFFFAIL));
                str.replace(FPSTR(T_V), FPSTR(HTTP_STATUS_APMODE));
                break;
            case WL_DISCONNECTED:     // disconnected; wrong or missing password
                str.replace(FPSTR(T_c), "r");
                str.replace(FPSTR(T_r), FPSTR(HTTP_STATUS_DISCONN));
                str.replace(FPSTR(T_V), FPSTR(HTTP_STATUS_APMODE));
                break;
            default:
                str.replace(FPSTR(T_c), "n");
                str.replace(FPSTR(T_r), "");
                str.replace(FPSTR(T_V), _carMode ? FPSTR(HTTP_STATUS_CARMODE) : FPSTR(HTTP_STATUS_APMODE));
                break;
            }
        }
    } else {
        str.replace(FPSTR(T_c), "n");
        str += FPSTR(HTTP_STATUS_NONE);
    }

    if(withMac) {
        str += FPSTR(HTTP_BR);
        str += WiFi.macAddress();
    }

    str += FPSTR(HTTP_STATUS_TAIL);

    page += str;
}

/****************************************************************************
 *
 * Website handling: Helpers
 *
 ****************************************************************************/

// Calc size of params (without 0-term)
unsigned int WiFiManager::getParamOutSize(WiFiManagerParameter** params,
                    int paramsCount, unsigned int& maxItemSize)
{
    unsigned int mysize = 0;
    unsigned int totalsize = 0;

    maxItemSize = 0;

    if(paramsCount > 0) {

        for(int i = 0; i < paramsCount; i++) {

            if(!params[i] || params[i]->_length > 99999) {
                #ifdef _A10001986_DBG
                Serial.println("[ERROR getParamOutSize] WiFiManagerParameter is out of scope");
                #endif
                continue;
            }

            mysize = 0;

            uint8_t pflags = params[i]->getFlags();

            if(pflags & WFM_SECTS_HEAD) {
                mysize += STRLEN(HTTP_SECT_HEAD);
            } else if(pflags & WFM_SECTS) {
                mysize += STRLEN(HTTP_SECT_START) + STRLEN(HTTP_SECT_HEAD);
            }
            if(pflags & WFM_FOOT) {
                mysize += STRLEN(HTTP_SECT_FOOT);
            }

            if(params[i]->getID()) {

                bool haveLabel = true;

                switch(pflags & WFM_LABEL_MASK) {
                case WFM_LABEL_BEFORE:
                    if(!(pflags & WFM_NO_BR)) mysize += STRLEN(HTTP_BR);
                    // fall through
                case WFM_LABEL_AFTER:
                    mysize += STRLEN(HTTP_FORM_LABEL) + STRLEN(HTTP_FORM_PARAM);
                    mysize += STRLEN(HTTP_BR);
                    mysize -= (8*3);  // tokens
                    break;
                default:
                    // WFM_NO_LABEL
                    mysize += STRLEN(HTTP_FORM_PARAM);
                    mysize -= (6*3);  // tokens
                    haveLabel = false;
                    break;
                }

                // <label for='{i}'>{t}</label>
                // <input id='{i}' name='{n}' {l} value='{v}' {c} {f}>
                mysize += (strlen(params[i]->getID()) * 3);    // 2x{i}, 1x{n}
                if(haveLabel && params[i]->getLabel()) {
                    mysize += strlen(params[i]->getLabel());
                }
                if(pflags & WFM_IS_CHKBOX) {
                    mysize += 1;    // value="1"
                    if(*(params[i]->getValue()) == '1') mysize += 7;  // "checked"
                    mysize += STRLEN(HTML_CHKBOX);
                } else {
                    int vl = params[i]->getValueLength();
                    if     (vl <   10) mysize += 1+12;
                    else if(vl <  100) mysize += 2+12;
                    else if(vl < 1000) mysize += 3+12;
                    else               mysize += 5+12;
                    mysize += strlen(params[i]->getValue());
                }
                if(params[i]->getCustomHTML()) {
                    mysize += strlen(params[i]->getCustomHTML());
                }

            } else if(params[i]->_customHTMLGenerator) {

                const char *t = (params[i]->_customHTMLGenerator)(NULL, WM_CP_LEN);
                if(t) {
                    mysize += *(unsigned int *)t;
                }

            } else if(params[i]->getCustomHTML()) {

                if(pflags & WFM_HL) {
                    mysize += STRLEN(HTTP_HL_S) + STRLEN(HTTP_HL_E);
                }

                mysize += strlen(params[i]->getCustomHTML());

            } else {

                continue;

            }

            totalsize += mysize;

            if(mysize > maxItemSize) maxItemSize = mysize;
        }
    }

    maxItemSize += 8;  // safety

    #ifdef _A10001986_DBG
    Serial.printf("getParamOutSize: totalsize %d maxItemSize %d\n", totalsize, maxItemSize - 8);
    #endif

    return totalsize;
}

void WiFiManager::getParamOut(String &page, WiFiManagerParameter** params,
                    int paramsCount, unsigned int maxItemSize)
{
    if(paramsCount > 0) {

        char valLength[12+6];

        // Allocate it once, re-use it
        String pitem;
        pitem.reserve(maxItemSize ? maxItemSize : 2048);

        // add the extra parameters to the form
        for(int i = 0; i < paramsCount; i++) {

            // Just see if any of our params has been destructed in the meantime
            if(!params[i] || params[i]->_length > 99999) {
                #ifdef _A10001986_DBG
                Serial.println("[ERROR] WiFiManagerParameter is out of scope");
                #endif
                continue;
            }

            pitem = "";

            // Input templating
            // <label for='{i}'>{t}</label>
            // <input id='{i}' name='{n}' {l} value='{v}' {c} {f}>
            // if no ID, use customhtml for item, else generate from param string

            uint8_t pflags = params[i]->getFlags();

            if(pflags & WFM_SECTS_HEAD) {
                pitem += FPSTR(HTTP_SECT_HEAD);
            } else if(pflags & WFM_SECTS) {
                pitem += FPSTR(HTTP_SECT_START);
                pitem += FPSTR(HTTP_SECT_HEAD);
            }

            if(params[i]->getID()) {

                bool haveLabel = true;

                switch(pflags & WFM_LABEL_MASK) {
                case WFM_LABEL_BEFORE:
                    pitem += FPSTR(HTTP_FORM_LABEL);
                    if(!(pflags & WFM_NO_BR)) pitem += FPSTR(HTTP_BR);
                    pitem += FPSTR(HTTP_FORM_PARAM);
                    pitem += FPSTR(HTTP_BR);
                    break;
                case WFM_LABEL_AFTER:
                    pitem += FPSTR(HTTP_FORM_PARAM);
                    pitem += FPSTR(HTTP_FORM_LABEL);
                    pitem += FPSTR(HTTP_BR);
                    break;
                default:
                    // WFM_NO_LABEL
                    pitem += FPSTR(HTTP_FORM_PARAM);
                    haveLabel = false;
                    break;
                }

                pitem.replace(FPSTR(T_i), params[i]->getID());     // T_i id name
                pitem.replace(FPSTR(T_n), params[i]->getID());     // T_n id name alias
                if(haveLabel) {
                    if(params[i]->getLabel()) {
                        pitem.replace(FPSTR(T_t), params[i]->getLabel());  // T_t title/label
                    } else {
                        pitem.replace(FPSTR(T_t), "");
                    }
                }
                if(pflags & WFM_IS_CHKBOX) {
                    pitem.replace(FPSTR(T_l), (*(params[i]->getValue()) == '1') ? "checked" : "");
                    pitem.replace(FPSTR(T_v), "1"); // value is ALWAYS "1"!
                    pitem.replace(FPSTR(T_f), HTML_CHKBOX);
                } else {
                    snprintf(valLength, 12+5, "maxlength='%d'", params[i]->getValueLength());
                    pitem.replace(FPSTR(T_l), valLength);              // T_l maxlength='value length'
                    pitem.replace(FPSTR(T_v), params[i]->getValue());  // T_v value
                    pitem.replace(FPSTR(T_f), "");
                }
                if(params[i]->getCustomHTML()) {                   // T_c additional attributes
                    pitem.replace(FPSTR(T_c), params[i]->getCustomHTML());
                } else {
                    pitem.replace(FPSTR(T_c), "");
                }

            } else if(params[i]->_customHTMLGenerator) {

                const char *t = (params[i]->_customHTMLGenerator)(NULL, WM_CP_CREATE);
                if(t) {
                    pitem += t;
                    (params[i]->_customHTMLGenerator)(t, WM_CP_DESTROY);
                }

            } else if(params[i]->getCustomHTML()) {

                if(pflags & WFM_HL) {
                    pitem += FPSTR(HTTP_HL_S);
                }

                pitem += params[i]->getCustomHTML();

                if(pflags & WFM_HL) {
                    pitem += FPSTR(HTTP_HL_E);
                }

            } else {

                continue;

            }

            if(pflags & WFM_FOOT) {
                pitem += FPSTR(HTTP_SECT_FOOT);
            }

            page += pitem;

            if(!(i % 30) && _gpcallback) {
                _gpcallback(WM_LP_NONE);
            }
        }
    }
}

void WiFiManager::doParamSave(WiFiManagerParameter** params, int paramsCount)
{
    if(paramsCount > 0) {

        for(int i = 0; i < paramsCount; i++) {

            // Just see if any of our params has been destructed in the meantime
            if(!params[i] || params[i]->_length > 99999) {
                #ifdef _A10001986_DBG
                Serial.println("[ERROR] WiFiManagerParameter is out of scope");
                #endif
                break;
            }

            // Skip pure customHTML parms
            if(!params[i]->getID()) {
                #ifdef _A10001986_DBG
                Serial.printf("doSaveParms: skipped parm %d\n", i);
                #endif
                continue;
            }

            // read parameter from server
            String value = server->arg(params[i]->getID());

            if(value == "" && (params[i]->getFlags() & WFM_IS_CHKBOX)) {
                strcpy(params[i]->_value, "0");
                #ifdef _A10001986_DBG
                Serial.printf("doSaveParms: checkbox '%s' set to 0\n", params[i]->getID());
                #endif
            } else {
                // store it in params array; +1 for zero termination
                value.toCharArray(params[i]->_value, params[i]->_length + 1);
                #ifdef _A10001986_DBG
                Serial.printf("doSaveParms: '%s' set to '%s'\n", params[i]->getID(), params[i]->_value);
                #endif
            }

            if(!(i % 30) && _gpcallback) {
                _gpcallback(WM_LP_NONE);
            }

        }

    }
}

void WiFiManager::send_cc()
{
    server->sendHeader("Cache-Control", "no-store, must-revalidate, no-cache");
    server->sendHeader("Pragma", "no-cache");
    server->sendHeader("Expires", "0");
}

void WiFiManager::HTTPSend(const String& content, bool sendCC)
{
    #ifdef _A10001986_DBG
    unsigned long now = millis();
    #endif

    #ifdef _A10001986_DBG
    Serial.printf("HTTPSend: Heap before %d\n", ESP.getFreeHeap());
    #endif

    #if 0
    if(server->client()) {
        #ifdef _A10001986_DBG
        Serial.printf("TCP_NODELAY is %d\n", server->client().getNoDelay());
        #endif
        server->client().setNoDelay(true);
    }
    #endif

    if(sendCC) {
        send_cc();
    }

    server->send(200, HTTP_HEAD_CT, content);

    #ifdef _A10001986_DBG
    Serial.printf("HTTPSend took %d, content size %d, heap %d\n\n", millis() - now, content.length(), ESP.getFreeHeap());
    #endif

    yield();
}

/****************************************************************************
 *
 * Website handling: Page handlers
 *
 ****************************************************************************/

/*--------------------------------------------------------------------------*/
/*********************************** ROOT ***********************************/
/*--------------------------------------------------------------------------*/

// Calc size of root menu (without 0-term)
int WiFiManager::getMenuOutLength(unsigned int& appExtraSize)
{
    int bufSize = 0;

    if(_menuIdArr) {
        int menuId = 0;
        int8_t t;
        while((t = _menuIdArr[menuId++]) != WM_MENU_END) {
            if(t == WM_MENU_PARAM && !_paramsCount[1]) continue;
            #ifdef WM_PARAM2
            if(t == WM_MENU_PARAM2 && !_paramsCount[2]) continue;
            #ifdef WM_PARAM3
            if(t == WM_MENU_PARAM3 && !_paramsCount[3]) continue;
            #endif
            #endif
            if(t == WM_MENU_CUSTOM && _customMenuHTML) {
                bufSize += strlen(_customMenuHTML);
                continue;
            }
            bufSize += strlen(HTTP_PORTAL_MENU[t]);
        }
    } else {
        bufSize += strlen(HTTP_PORTAL_MENU[WM_MENU_WIFI]);
        bufSize += strlen(HTTP_PORTAL_MENU[WM_MENU_UPDATE]);
    }

    if(_menuoutlencallback) {
        appExtraSize = _menuoutlencallback();
        bufSize += appExtraSize;
    }

    return bufSize;
}

// Construct root menu
void WiFiManager::getMenuOut(String& page, unsigned int appExtraSize)
{
    if(_menuIdArr) {
        int menuId = 0;
        int8_t t;
        while((t = _menuIdArr[menuId++]) != WM_MENU_END) {
            if(t == WM_MENU_PARAM && !_paramsCount[1]) continue;
            #ifdef WM_PARAM2
            if(t == WM_MENU_PARAM2 && !_paramsCount[2]) continue;
            #ifdef WM_PARAM3
            if(t == WM_MENU_PARAM3 && !_paramsCount[3]) continue;
            #endif
            #endif
            if(t == WM_MENU_CUSTOM && _customMenuHTML) {
                page += _customMenuHTML;
                continue;
            }
            page += HTTP_PORTAL_MENU[t];
        }
    } else {
        page += HTTP_PORTAL_MENU[WM_MENU_WIFI];
        page += HTTP_PORTAL_MENU[WM_MENU_UPDATE];
    }

    if(_menuoutcallback) {
        _menuoutcallback(page, appExtraSize);
    }
}

unsigned int WiFiManager::calcRootLen(unsigned int& repSize, unsigned int& appExtraSize)
{
    // Calc page size
    unsigned int bufSize;
    uint32_t incFlags = incSTA|incC80;

    #ifdef WM_UPLOAD
    if(_nv || !_sndIsInstalled) incFlags |= incUPLF;
    #else
    if(_nv) incFlags |= incUPLF;
    #endif

    bufSize = getHTTPHeadLength(NULL, incFlags);

    bufSize += getMenuOutLength(appExtraSize);

    repSize = reportStatusLen();
    bufSize += repSize;

    bufSize += STRLEN(HTTP_END);

    #ifdef _A10001986_DBG
    Serial.printf("calcRootLen: calced content size %d\n", bufSize);
    #endif

    return bufSize;
}

void WiFiManager::buildRootPage(String& page, unsigned int repSize, unsigned int appExtraSize)
{
    // Build page
    uint32_t incFlags = incSTA|incC80;

    #ifdef WM_UPLOAD
    if(_nv || !_sndIsInstalled) incFlags |= incUPLF;
    #else
    if(_nv) incFlags |= incUPLF;
    #endif

    getHTTPHeadNew(page, NULL, incFlags);

    getMenuOut(page, appExtraSize);

    reportStatus(page, repSize);

    page += FPSTR(HTTP_END);
}

/**
 * HTTPD CALLBACK root
 */
void WiFiManager::handleRoot()
{
    unsigned int headSize = 0;
    unsigned int repSize = 0;
    unsigned int appExtraSize = 0;

    #ifdef _A10001986_DBG
    Serial.println("<- HTTP Root");
    #endif

    String page;
    page.reserve(calcRootLen(repSize, appExtraSize) + 16);

    buildRootPage(page, repSize, appExtraSize);

    if(_gpcallback) {
        _gpcallback(WM_LP_PREHTTPSEND);
    }

    HTTPSend(page, false);

    if(_gpcallback) {
        _gpcallback(WM_LP_POSTHTTPSEND);
    }
}


/*--------------------------------------------------------------------------*/
/*************************** WIFI CONFIGURATION *****************************/
/*--------------------------------------------------------------------------*/

int16_t WiFiManager::WiFi_waitForScan()
{
    int16_t res, resa;
    unsigned long now;
    bool asyncdone = false;

    // Wait until scanComplete returns != RUNNING
    while((res = WiFi.scanComplete()) == WIFI_SCAN_RUNNING) {

        #ifdef _A10001986_DBG
        Serial.println(".");
        #endif

        _delay(100);
    }

    // Sometimes above returns an error (timeout?)
    // so we now wait for the scanComplete event
    now = millis();
    while(millis() - now < 5000) {
        if(_WiFiEventMask & WM_EVB_SCAN_DONE) {
            asyncdone = true;
            break;
        }
        _delay(100);
    }

    // If first loop fails, use event-callback result (if not error)
    if(res < 0 && asyncdone && ((resa = _numNetworksAsync) >= 0)) {
        res = resa;
        #ifdef _A10001986_DBG
        Serial.printf("Async wait failed, using event-callback result\n");
        #endif
    }

    if(res >= 0) {
        _lastscan = millis();
        if(!_lastscan) _lastscan++;

        // Core activates STA on scanning.
        // Switch off STA again here if we are in AP mode.
        if(WiFi.getMode() & WIFI_AP) {
            wifiSTAOff();
        }
    }

    return res;
}

int16_t WiFiManager::WiFi_scanNetworks(bool force, bool async)
{
    if(!_numNetworks && _autoforcerescan) {
        #ifdef _A10001986_DBG
        Serial.println("No networks found, forcing new scan");
        #endif
        force = true;
    }

    // Use cache if available
    if(!_lastscan) {
        force = true;
    }

    if(force) {

        int16_t res;

        _numNetworks = 0;

        if(async) {

            #ifdef _A10001986_V_DBG
            Serial.println("WiFi Scan ASYNC started");
            #endif

            // Reset marker for event callback
            _numNetworksAsync = WM_WIFI_SCAN_BUSY;    // This is atomic enough
            andWiFiEventMask(~WM_EVB_SCAN_DONE);

            // Start scan
            return WiFi.scanNetworks(true);

        }

        #ifdef _A10001986_V_DBG
        Serial.println("WiFi Scan SYNC started");
        #endif

        res = WiFi.scanNetworks();

        if(res == WIFI_SCAN_FAILED) {

            #ifdef _A10001986_DBG
            Serial.println("[ERROR] scan failed");
            #endif

        } else if(res >= 0) {

            _numNetworks = res;
            _lastscan = millis();
            if(!_lastscan) _lastscan++;

            // Core activates STA on scanning.
            // Switch off STA again here if we are in AP mode.
            if(WiFi.getMode() & WIFI_AP) {
                wifiSTAOff();
            }

        }

        #ifdef _A10001986_V_DBG
        Serial.println("WiFi Scan completed");
        #endif

        return res;

    }

    return 0;
}

void WiFiManager::sortNetworks(int n, int *indices, int& haveDupes, bool removeDupes)
{
    if(n == 0) {

        #ifdef _A10001986_DBG
        Serial.println("No networks found");
        #endif

    } else {

        #ifdef _A10001986_DBG
        Serial.printf("%d networks found\n", n);
        #endif

        // sort networks
        for(int i = 0; i < n; i++) {
            indices[i] = i;
        }

        // RSSI sort
        for(int i = 0; i < n; i++) {
            for(int j = i + 1; j < n; j++) {
                if(WiFi.RSSI(indices[j]) > WiFi.RSSI(indices[i])) {
                    std::swap(indices[i], indices[j]);
                }
            }
        }

        // remove duplicates (must be RSSI sorted to remove the weaker one here)
        {
            String cssid;
            for(int i = 0; i < n; i++) {
                if(indices[i] == -1) continue;
                cssid = WiFi.SSID(indices[i]);
                for(int j = i + 1; j < n; j++) {
                    if(cssid == WiFi.SSID(indices[j])) {
                        haveDupes++;
                        if(removeDupes) {
                            indices[j] = -1;
                        }
                    }
                }
            }
        }
    }
}

unsigned int WiFiManager::getScanItemsLen(int n, bool scanErr, int *indices, unsigned int& maxItemSize, int& stopAt, bool showall)
{
    unsigned int mySize = 0, itemSize;

    stopAt = n;

    if(scanErr) {

        mySize += STRLEN(HTTP_MSG_SCANFAIL);
        maxItemSize = mySize;

    } else if(n == 0) {

        mySize += STRLEN(HTTP_MSG_NONETWORKS);
        maxItemSize = mySize;

    } else {

        maxItemSize = 0;

        for(int i = 0; i < n; i++) {

            if(indices[i] == -1) continue;

            // <div><a href='#p' onclick='return {t}(this)' data-ssid='{V}' title='{R}'>{v}</a>{c}
            // <div role='img' aria-label='{r}dBm' title='{r}dBm' class='q q-{q} {i}'></div></div>

            int rssi = WiFi.RSSI(indices[i]);

            if(_minimumRSSI < rssi) {

                String SSID = WiFi.SSID(indices[i]);
                if(SSID == "") {
                    continue;
                } else if(!checkSSID(SSID)) {
                    if(showall) {
                        SSID = S_nonprintable;
                    } else {
                        continue;
                    }
                }

                itemSize = STRLEN(HTTP_WIFI_ITEM) - (9*3);
                itemSize += (2 * htmlEntitiesLen(SSID, true));
                itemSize += (4+4+1+1);     // rssi, rssi, qual class, enc class
                if(showall) {
                    itemSize += (6 + 17);  // chnlnum, bssid
                }

                if(itemSize > maxItemSize) maxItemSize = itemSize;

                mySize += itemSize;

                if(mySize > MAX_SCAN_OUTPUT_SIZE) {
                    stopAt = i + 1;
                    #ifdef _A10001986_DBG
                    Serial.printf("WM: Maximum scan output size reached, stop at %d\n", stopAt);
                    #endif
                    break;
                }

            } else {

                #ifdef _A10001986_DBG
                Serial.printf("WM: skipping %s, rssi %d\n", WiFi.SSID(indices[i]).c_str(), rssi);
                #endif

            }

        }
    }

    #ifdef _A10001986_DBG
    Serial.printf("WM: getScanItemsLen %d, maxItemSize %d\n", mySize, maxItemSize);
    #endif

    return mySize;
}

void WiFiManager::getScanItemsOut(String& page, int n, bool scanErr, int *indices, unsigned int maxItemSize, bool showall)
{
    char chnlnum[8];
    uint8_t bssid[6];
    char pbssid[20] = { 0 };

     if(scanErr) {

        page += FPSTR(HTTP_MSG_SCANFAIL);

    } else if(n == 0) {

        page += FPSTR(HTTP_MSG_NONETWORKS);

    } else {

        String item;
        if(maxItemSize) item.reserve(maxItemSize + 8);

        // <div><a href='#p' onclick='return {t}(this)' data-ssid='{V}' title='{R}'>{v}</a>{c}
        // <div role='img' aria-label='{r}dBm' title='{r}dBm' class='q q-{q} {i}'></div></div>

        for(int i = 0; i < n; i++) {
            if(indices[i] == -1) continue;

            int rssi = WiFi.RSSI(indices[i]);

            if(_minimumRSSI < rssi) {

                uint8_t enc_type = WiFi.encryptionType(indices[i]);
                String SSID = WiFi.SSID(indices[i]);
                String func = "c";

                if(SSID == "") {
                    continue;
                } else if(!checkSSID(SSID)) {
                    if(showall) {
                        SSID = S_nonprintable;
                        func = "d";
                    } else {
                        continue;
                    }
                }

                item = FPSTR(HTTP_WIFI_ITEM);

                item.replace(FPSTR(T_t), func);
                item.replace(FPSTR(T_V), htmlEntities(SSID));
                item.replace(FPSTR(T_v), htmlEntities(SSID, true));
                if(showall) {
                    sprintf(chnlnum, " (%d)", WiFi.channel(indices[i]));
                    item.replace(FPSTR(T_c), chnlnum);              // channel
                    if(WiFi.BSSID(indices[i])) {
                        memcpy(bssid, WiFi.BSSID(indices[i]), 6);
                        sprintf(pbssid, "%02x:%02x:%02x:%02x:%02x:%02x",
                            bssid[0], bssid[1], bssid[2], bssid[3], bssid[4], bssid[5]);
                    }
                    item.replace(FPSTR(T_R), pbssid);               // bssid
                } else {
                    item.replace(FPSTR(T_c), "");
                    item.replace(FPSTR(T_R), "");
                }
                item.replace(FPSTR(T_r), String(rssi));             // rssi
                item.replace(FPSTR(T_q), String(wmmap(rssi)));      // quality icon 1-4
                item.replace(FPSTR(T_i), (enc_type != WIFI_AUTH_OPEN) ? "l" : "");

                page += item;
                delay(0);

            }

            if(!(i % 20) && _gpcallback) {
                _gpcallback(WM_LP_NONE);
            }
        }
    }
}

// static ip fields

void WiFiManager::getIpForm(String& page, const char *id, const char *title, IPAddress& value, const char *placeholder)
{
    // <label for='{i}'>{t}</label>
    // <input id='{i}' name='{n}' {l} value='{v}' {c} {f}>

    unsigned int s = STRLEN(HTTP_FORM_LABEL) + STRLEN(HTTP_FORM_PARAM) - (8*3) + (2*STRLEN(HTTP_BR));
    s += (3*strlen(id)) + strlen(title);
    s += (12+2) + 15;
    if(placeholder) s += strlen(placeholder);

    String item;
    item.reserve(s + 8);

    item = FPSTR(HTTP_FORM_LABEL);
    item += FPSTR(HTTP_BR);
    item += FPSTR(HTTP_FORM_PARAM);
    item += FPSTR(HTTP_BR);
    item.replace(FPSTR(T_i), id);
    item.replace(FPSTR(T_n), id);
    item.replace(FPSTR(T_t), title);
    item.replace(FPSTR(T_l), F("maxlength='15'"));
    item.replace(FPSTR(T_v), value ? value.toString() : "");
    item.replace(FPSTR(T_c), placeholder ? placeholder : "");
    item.replace(FPSTR(T_f), "");

    page += item;
}

unsigned int WiFiManager::getStaticLen()
{
    unsigned int mySize = 0;
    bool showSta = (_staShowStaticFields || _sta_static_ip);
    bool showDns = (_staShowDns || _sta_static_dns);

    // "<label for='{i}'>{t}</label>"
    // "<input id='{i}' name='{n}' {l} value='{v}' {c} {f}>\n"

    if(showSta || showDns) {
        mySize += STRLEN(HTTP_FORM_SECT_HEAD);
    }
    if(showSta) {
        mySize += (3 * (STRLEN(HTTP_FORM_LABEL) + STRLEN(HTTP_FORM_PARAM) - (8*3) + (2*STRLEN(HTTP_BR))));
        mySize += (3*STRLEN(S_ip)) + STRLEN(S_staticip) + (12+2) + 15;
        mySize += STRLEN(HTTP_FORM_WIFI_PH);
        mySize += (3*STRLEN(S_sn)) + STRLEN(S_subnet) + (12+2) + 15;
        mySize += (3*STRLEN(S_gw)) + STRLEN(S_staticgw) + (12+2) + 15;
    }
    if(showDns) {
        mySize += STRLEN(HTTP_FORM_LABEL) + STRLEN(HTTP_FORM_PARAM) - (8*3) + (2*STRLEN(HTTP_BR));
        mySize += (3*STRLEN(S_dns)) + STRLEN(S_staticdns) + (12+2) + 15;
    }
    if(showSta || showDns) {
        mySize += STRLEN(HTTP_FORM_SECT_FOOT);
    }

    return mySize;
}

void WiFiManager::getStaticOut(String& page)
{
    bool showSta = (_staShowStaticFields || _sta_static_ip);
    bool showDns = (_staShowDns || _sta_static_dns);

    if(showSta || showDns) {
        page += FPSTR(HTTP_FORM_SECT_HEAD);
    }
    if(showSta) {
        getIpForm(page, S_ip, S_staticip, _sta_static_ip, HTTP_FORM_WIFI_PH);
        getIpForm(page, S_sn, S_subnet, _sta_static_sn);
        getIpForm(page, S_gw, S_staticgw, _sta_static_gw);
    }
    if(showDns) {
        getIpForm(page, S_dns, S_staticdns, _sta_static_dns);
    }
    if(showSta || showDns) {
        page += FPSTR(HTTP_FORM_SECT_FOOT);
    }
}

// Build WiFi page

void WiFiManager::buildWifiPage(String& page, bool scan)
{
    unsigned int bufSize = 0, repSize = 0;
    unsigned int maxScanItemSize = 0;
    unsigned int maxItemSize = 0;
    int numDupes = 0, maxDisplay;
    uint32_t incFlags = incSET|incSTA;
    bool scanErr = false, scanallowed = true, showrefresh = false, haveShowAll = false;
    bool force = server->hasArg(F("refresh"));
    bool showall = server->hasArg(F("showall"));
    int n = 0;

    String SSID = String(_ssid);
    unsigned int SSID_len = SSID.length();
    String BSSID = String(_bssid);
    unsigned int BSSID_len = BSSID.length();

    if(showall) scan = true;

    // No scan (unless forced) if we have a configured network
    if(scan && !force && !showall && SSID != "") {
        scan = false;
        showrefresh = true;
    }

    // PreScanCallback can cancel scan, eg if
    // device is busy and must not be interrupted.
    // We use the cache then, if still valid.
    if(scan && _prewifiscancallback) {
        if(!(scanallowed = _prewifiscancallback())) {
            if(!_autoforcerescan && _lastscan) {
                scanallowed = true;
                force = false;
            } else {
                scan = false;
            }
        }
    }

    if(scan) {
        // This resets _numNetWorks to 0 if actually scanning. Force if arg "refresh".
        int16_t res = WiFi_scanNetworks(force, true);
        if(res == WIFI_SCAN_RUNNING) {
            _numNetworks = WiFi_waitForScan();
            #ifdef _A10001986_DBG
            Serial.printf("handleWiFi: waitForScan returned %d\n", _numNetworks);
            #endif
        } else {
            #ifdef _A10001986_DBG
            Serial.printf("handleWiFi: scanNetworks returned %d (0=using cache, >=0 numNetworks)\n", res);
            #endif
        }
        n = _numNetworks;
    }

    if(n < 0) {
        #ifdef _A10001986_DBG
        Serial.printf("handleWiFi: last Scan returned %d\n", n);
        #endif
        scanErr = true;
        n = 0;
    }

    int indices[n];
    if(scan) {
        sortNetworks(n, indices, numDupes, !showall);
        incFlags |= incQI;
    }

    maxDisplay = n;

    bufSize = getHTTPHeadLength(S_titlewifi, incFlags);

    if(showrefresh) {
        // No message
    } else if(!scanallowed) {
        bufSize += STRLEN(HTTP_MSG_NOSCAN);
    } else if(scan) {
        bufSize += getScanItemsLen(n, scanErr, indices, maxScanItemSize, maxDisplay, showall);
        if(!showall && n > 0 && !scanErr) {
            bufSize += STRLEN(HTTP_SHOWALL);
            haveShowAll = true;
        }
    }
    bufSize += (STRLEN(HTTP_FORM_START) - 3 + STRLEN(A_wifisave));
    bufSize += (STRLEN(HTTP_FORM_WIFI) - (4*3) + SSID_len + (2*STRLEN(S_passph))) + BSSID_len;
    if(SSID_len) {
        bufSize += STRLEN(HTTP_ERASE_BUTTON);
    }
    bufSize += STRLEN(HTTP_FORM_WIFI_END);
    bufSize += getStaticLen();
    bufSize += getParamOutSize(_params[0], _paramsCount[0], maxItemSize);
    bufSize += STRLEN(HTTP_FORM_END);
    bufSize += STRLEN(HTTP_SCAN_LINK);
    if(haveShowAll) {
        bufSize += STRLEN(HTTP_SHOWALL_FORM);
    }
    repSize = reportStatusLen(true);
    bufSize += repSize;
    bufSize += STRLEN(HTTP_END);

    #ifdef _A10001986_DBG
    Serial.printf("handleWiFi: calced content size %d\n", bufSize);
    #endif

    page.reserve(bufSize + 16);

    getHTTPHeadNew(page, S_titlewifi, incFlags);

    if(showrefresh) {
        // No message
    } else if(!scanallowed) {
        page += FPSTR(HTTP_MSG_NOSCAN);
    } else if(scan) {
        getScanItemsOut(page, maxDisplay, scanErr, indices, maxScanItemSize, showall);
        if(haveShowAll) {
            page += FPSTR(HTTP_SHOWALL);
        }
    }

    {
        String pitem;
        pitem.reserve(STRLEN(HTTP_FORM_WIFI) - (4*3) + SSID_len + (2*STRLEN(S_passph)) + BSSID_len + 8); // +8 safety

        pitem = FPSTR(HTTP_FORM_START);
        pitem.replace(FPSTR(T_v), FPSTR(A_wifisave));
        page += pitem;

        pitem = FPSTR(HTTP_FORM_WIFI);
        pitem.replace(FPSTR(T_V), SSID);
        if(*_pass) {
            pitem.replace(FPSTR(T_p), FPSTR(S_passph));  // twice!
        } else {
            pitem.replace(FPSTR(T_p), "");
        }
        pitem.replace(FPSTR(T_h), BSSID);
        page += pitem;
    }

    if(SSID_len) {
        page += FPSTR(HTTP_ERASE_BUTTON);
    }
    page += FPSTR(HTTP_FORM_WIFI_END);
    getStaticOut(page);
    getParamOut(page, _params[0], _paramsCount[0], maxItemSize);
    page += FPSTR(HTTP_FORM_END);
    page += FPSTR(HTTP_SCAN_LINK);
    if(haveShowAll) {
        page += FPSTR(HTTP_SHOWALL_FORM);
    }
    reportStatus(page, repSize, true);
    page += FPSTR(HTTP_END);

    // Add a delay in order to minimize time
    // first HTTPSend takes after scan
    if(scan && _lastscan) {
        unsigned int mssincescan = millis() - _lastscan;
        if(mssincescan < 4000) {
            _delay(4000 - mssincescan);
        }
    }
}

/*
 * HTTPD CALLBACK "Wifi Configuration" page handler
 */
void WiFiManager::handleWifi(bool scan)
{
    String page;

    #ifdef _A10001986_V_DBG
    Serial.println("<- HTTP Wifi");
    #endif

    buildWifiPage(page, scan);

    if(_gpcallback) {
        _gpcallback(WM_LP_PREHTTPSEND);
    }

    HTTPSend(page, true);

    if(_gpcallback) {
        _gpcallback(WM_LP_POSTHTTPSEND);
    }
}

/*
 * HTTPD CALLBACK "Wifi Configuration"->SAVE page handler
 */
void WiFiManager::handleWifiSave()
{
    unsigned long s, headSize;
    bool haveNewSSID = false;
    bool networkDeleted = false;

    #ifdef _A10001986_V_DBG
    Serial.println("<- HTTP WiFi save ");
    #endif

    // grab new ssid/pass
    {
        String newSSID = server->arg(F("s"));
        String newPASS = server->arg(F("p"));
        String newBSSID = server->arg(F("b"));
        bool forgetNW = server->hasArg(F("fgn"));
        bool pwchg = false;

        if(newSSID != "") {
            memset(_ssid, 0, sizeof(_ssid));
            memset(_pass, 0, sizeof(_pass));
            strncpy(_ssid, newSSID.c_str(), sizeof(_ssid) - 1);
            strncpy(_pass, newPASS.c_str(), sizeof(_pass) - 1);
            haveNewSSID = true;
        } else if(newPASS != "") {
            memset(_pass, 0, sizeof(_pass));
            // Don't save password if we have no SSID
            if(*_ssid) {
                strncpy(_pass, newPASS.c_str(), sizeof(_pass) - 1);
                #ifdef _A10001986_DBG
                Serial.println("password change detected");
                #endif
                pwchg = true;
            }
        }

        memset(_bssid, 0, sizeof(_bssid));
        strncpy(_bssid, newBSSID.c_str(), sizeof(_bssid) - 1);

        if(!haveNewSSID && !pwchg) {
            if(forgetNW) {
                if(*_ssid) networkDeleted = true;
                *_ssid = *_pass = *_bssid = 0;
            }
        }
    }

    // At this point, _ssid and _pass are either the newly entered ones,
    // the previous ones (if the server-provided ones were empty), or
    // 0-strings (if the user checked "Forget" while SSID and password
    // fields were empty)

    // set static ips from server args
    // We skip this because we reboot as a result of "save", there
    // is no point is settings this here.
    #if 0
    {
        String temp;
        temp.reserve(16);
        if((temp = server->arg(FPSTR(S_ip))) != "") {
            optionalIPFromString(&_sta_static_ip, temp.c_str());
        }
        if((temp = server->arg(FPSTR(S_sn))) != "") {
            optionalIPFromString(&_sta_static_sn, temp.c_str());
        }
        if((temp = server->arg(FPSTR(S_gw))) != "") {
            optionalIPFromString(&_sta_static_gw, temp.c_str());
        }
        if((temp = server->arg(FPSTR(S_dns))) != "") {
            optionalIPFromString(&_sta_static_dns, temp.c_str());
        }
    }
    #endif

    // callback before saving
    if(_presavewificallback) {
        _presavewificallback();
    }

    // Copy server parms to wifi params array
    doParamSave(_params[0], _paramsCount[0]);

    // callback for saving or grabbing other parameters from server
    if(_savewificallback) {
        _savewificallback(_ssid, _pass, _bssid);
    }

    // Build page

    s = getHTTPHeadLength(S_titlewifi, incGFXMSG);
    s += STRLEN(HTTP_PARAMSAVED) + STRLEN(HTTP_PARAMSAVED_END) + STRLEN(HTTP_END);

    if(!haveNewSSID) {
        if(networkDeleted) s += STRLEN(HTTP_SAVED_ERASED);
    } else {
        if(_carMode) s += STRLEN(HTTP_SAVED_CARMODE);
        else         s += STRLEN(HTTP_SAVED_NORMAL);
    }

    String page;
    page.reserve(s + 16);

    getHTTPHeadNew(page, S_titlewifi, incGFXMSG);

    page += FPSTR(HTTP_PARAMSAVED);
    if(!haveNewSSID) {
        if(networkDeleted) page += FPSTR(HTTP_SAVED_ERASED);
    } else {
        if(_carMode) page += FPSTR(HTTP_SAVED_CARMODE);
        else         page += FPSTR(HTTP_SAVED_NORMAL);
    }
    page += FPSTR(HTTP_PARAMSAVED_END);
    page += FPSTR(HTTP_END);

    #ifdef _A10001986_DBG
    Serial.printf("handleWiFiSave: calced content size %d\n", s);
    #endif

    if(_gpcallback) {
        _gpcallback(WM_LP_PREHTTPSEND);
    }

    HTTPSend(page, false);

    if(_gpcallback) {
        _gpcallback(WM_LP_POSTHTTPSEND);
    }
}


/*--------------------------------------------------------------------------*/
/********************************* SETTINGS *********************************/
/*--------------------------------------------------------------------------*/

// Calc size of params ("Settings") page
int WiFiManager::calcParmPageSize(int aidx, unsigned int& maxItemSize, const char *title, const char *action)
{
    int mySize = getHTTPHeadLength(title, incSET);

    #ifdef _A10001986_DBG
    Serial.printf("calcParmSize: head %d\n", mySize);
    #endif

    mySize += (STRLEN(HTTP_FORM_START) - 3 + strlen(action));

    mySize += getParamOutSize(_params[aidx], _paramsCount[aidx], maxItemSize);

    mySize += STRLEN(HTTP_FORM_END);
    mySize += STRLEN(HTTP_END);

    #ifdef _A10001986_DBG
    Serial.printf("calcParmSize: calced content size %d\n", mySize);
    #endif

    return mySize;
}

/*
 * HTTPD CALLBACK Settings page handler
 */
void WiFiManager::_handleParam(int aidx, const char *title, const char *action)
{
    unsigned int maxItemSize = 0;

    #ifdef _A10001986_V_DBG
    Serial.println("<- HTTP Param");
    #endif

    String page;
    page.reserve(calcParmPageSize(aidx, maxItemSize, title, action) + 16);

    getHTTPHeadNew(page, title, incSET);

    {
        String pitem;
        pitem.reserve(STRLEN(HTTP_FORM_START) - 3 + strlen(action) + 2);
        pitem = FPSTR(HTTP_FORM_START);
        pitem.replace(FPSTR(T_v), action);
        page += pitem;
    }

    getParamOut(page, _params[aidx], _paramsCount[aidx], maxItemSize);

    page += FPSTR(HTTP_FORM_END);
    page += FPSTR(HTTP_END);

    if(_gpcallback) {
        _gpcallback(WM_LP_PREHTTPSEND);
    }

    HTTPSend(page, true);

    if(_gpcallback) {
        _gpcallback(WM_LP_POSTHTTPSEND);
    }
}

void WiFiManager::handleParam()
{
    _handleParam(1, S_titleparam[0], A_paramsave);
}

#ifdef WM_PARAM2
void WiFiManager::handleParam2()
{
    _handleParam(2, S_titleparam[1], A_param2save);
}

#ifdef WM_PARAM3
void WiFiManager::handleParam3()
{
    _handleParam(3, S_titleparam[2], A_param3save);
}
#endif
#endif

/*
 * HTTPD CALLBACK Settings->SAVE page handler
 */
void WiFiManager::_handleParamSave(int aidx, const char *title)
{
    unsigned int mySize = 0;
    unsigned int headSize = 0;

    #ifdef _A10001986_V_DBG
    Serial.printf("<- HTTP Param save %d\n", aidx);
    #endif

    #ifdef WM_PRESAVECB
    if(_presaveparamscallback) {
        _presaveparamscallback(aidx);
    }
    #endif

    doParamSave(_params[aidx], _paramsCount[aidx]);

    if(_saveparamscallback) {
        _saveparamscallback(aidx);
    }

    mySize = getHTTPHeadLength(title, incGFXMSG);

    mySize += STRLEN(HTTP_PARAMSAVED) + STRLEN(HTTP_PARAMSAVED_END);
    mySize += STRLEN(HTTP_END);

    String page;
    page.reserve(mySize + 16);

    #ifdef _A10001986_DBG
    Serial.printf("handleParamSave %d: calced content size %d\n", aidx, mySize);
    #endif

    getHTTPHeadNew(page, title, incGFXMSG);

    page += FPSTR(HTTP_PARAMSAVED);
    page += FPSTR(HTTP_PARAMSAVED_END);
    page += FPSTR(HTTP_END);

    if(_gpcallback) {
        _gpcallback(WM_LP_PREHTTPSEND);
    }

    HTTPSend(page, false);

    if(_gpcallback) {
        _gpcallback(WM_LP_POSTHTTPSEND);
    }
}

void WiFiManager::handleParamSave()
{
    _handleParamSave(1, S_titleparam[0]);
}

#ifdef WM_PARAM2
void WiFiManager::handleParam2Save()
{
    _handleParamSave(2, S_titleparam[1]);
}

#ifdef WM_PARAM3
void WiFiManager::handleParam3Save()
{
    _handleParamSave(3, S_titleparam[2]);
}
#endif
#endif


/*--------------------------------------------------------------------------*/
/********************************* UPDATE ***********************************/
/*--------------------------------------------------------------------------*/


/*
 * HTTPD CALLBACK Update page handler
 */
void WiFiManager::handleUpdate()
{
    unsigned int mySize = 0;
    unsigned int headSize = 0;

    #ifdef _A10001986_V_DBG
    Serial.println("<- Handle update");
    #endif

    mySize = getHTTPHeadLength(S_titleupd, incSET|incUPL);

    mySize += STRLEN(HTTP_UPDATE_FORM) + STRLEN(HTTP_UPDATE1) + STRLEN(HTTP_UPDATE2);
    if(_downloadLink) {
        mySize += STRLEN(HTTP_UPLOAD_LINK0) + STRLEN(HTTP_UPLOAD_LINK1);
        mySize += strlen(_downloadLink);
        if(_nv) {
            mySize += STRLEN(HTTP_UPLOAD_LINKY) + STRLEN(HTTP_UPLOAD_LINK2) + STRLEN(HTTP_UPLOAD_LINK3);
            mySize += strlen(_nv);
        } else if(_vd) {
            mySize += STRLEN(HTTP_UPLOAD_LINKX) + STRLEN(HTTP_UPLOAD_LINKA);
        } else {
            mySize += STRLEN(HTTP_UPLOAD_LINKZ) + STRLEN(HTTP_UPLOAD_LINKA);
        }
    }

    #ifdef WM_FW_HW_VER
    if(_rfw) {
        mySize += STRLEN(HTTP_UPLOAD_LINK0) + STRLEN(HTTP_UPLOAD_V_REQ) + STRLEN(HTTP_DIV_END);
        mySize += strlen(_rfw);
    }
    #endif

#ifdef WM_UPLOAD
    if(_showUploadSnd) {
        mySize += STRLEN(HTTP_UPDATE_FORM) + STRLEN(HTTP_UPLOADSND1);
        mySize += STRLEN(HTTP_UPLOADSND1A) + STRLEN(HTTP_UPLOADSND2) + STRLEN(HTTP_UPLOADSND3);
    } else {
        mySize += STRLEN(HTTP_UPLOADSND1A) + STRLEN(HTTP_UPLOAD_SDMSG);
    }
    mySize += strlen(_sndContName);
    if(_sndContVer) {
        mySize += STRLEN(HTTP_UPLOAD_SLINK1) + STRLEN(HTTP_UPLOAD_SLINK1B) + STRLEN(HTTP_UPLOAD_SLINK2) + STRLEN(HTTP_UPLOAD_SLINK3);
        mySize += strlen(_sndContVer);
        if(!_sndIsInstalled) mySize += STRLEN(HTTP_UPLOAD_SLINK1A) + STRLEN(HTTP_UPLOAD_SLINK2A);
    }
#endif

    mySize += STRLEN(HTTP_END);

    #ifdef _A10001986_DBG
    Serial.printf("handleUpdate: calced content size %d\n", mySize);
    #endif

    String page;
    page.reserve(mySize + 16);

    getHTTPHeadNew(page, S_titleupd, incSET|incUPL);

    page += FPSTR(HTTP_UPDATE_FORM);
    page += FPSTR(HTTP_UPDATE1);
    if(_downloadLink) {
        page += FPSTR(HTTP_UPLOAD_LINK0);
        if(_nv) {
            page += FPSTR(HTTP_UPLOAD_LINKY);
        } else if(_vd) {
            page += FPSTR(HTTP_UPLOAD_LINKX);
        } else {
            page += FPSTR(HTTP_UPLOAD_LINKZ);
        }
        page += FPSTR(HTTP_UPLOAD_LINK1);
        page += _downloadLink;
        if(_nv) {
            page += FPSTR(HTTP_UPLOAD_LINK2);
            page += _nv;
            page += FPSTR(HTTP_UPLOAD_LINK3);
        } else {
            page += FPSTR(HTTP_UPLOAD_LINKA);
        }
    }
    #ifdef WM_FW_HW_VER
    if(_rfw) {
        page += FPSTR(HTTP_UPLOAD_LINK0);
        page += FPSTR(HTTP_UPLOAD_V_REQ);
        page += _rfw;
        page += FPSTR(HTTP_DIV_END);
    }
    #endif
    page += FPSTR(HTTP_UPDATE2);

#ifdef WM_UPLOAD
    if(_showUploadSnd) {
        page += FPSTR(HTTP_UPDATE_FORM);
        page += FPSTR(HTTP_UPLOADSND1);
    }
    page += FPSTR(HTTP_UPLOADSND1A);
    page += _sndContName;
    page += FPSTR(HTTP_UPLOADSND2);
    if(_sndContVer) {
        page += FPSTR(HTTP_UPLOAD_SLINK1);
        if(!_sndIsInstalled) page += FPSTR(HTTP_UPLOAD_SLINK1A);
        page += FPSTR(HTTP_UPLOAD_SLINK1B);
        page += _sndContVer;
        page += FPSTR(HTTP_UPLOAD_SLINK2);
        if(!_sndIsInstalled) page += FPSTR(HTTP_UPLOAD_SLINK2A);
        page += FPSTR(HTTP_UPLOAD_SLINK3);
    }
    if(_showUploadSnd) {
        page += FPSTR(HTTP_UPLOADSND3);
    } else {
        page += FPSTR(HTTP_UPLOAD_SDMSG);
    }
#endif

    page += FPSTR(HTTP_END);

    if(_gpcallback) {
        _gpcallback(WM_LP_PREHTTPSEND);
    }

    HTTPSend(page, false);

    if(_gpcallback) {
        _gpcallback(WM_LP_POSTHTTPSEND);
    }
}

// upload via /u POST
void WiFiManager::handleUpdating()
{
    // handler for the file upload, get's the sketch bytes, and writes
    // them through the Update object
    HTTPUpload& upload = server->upload();

    if(upload.status == UPLOAD_FILE_START) {

        _uplError = false;

        // Callback for before OTA update
        if(_preotaupdatecallback) {
            _preotaupdatecallback();
        }

        #ifdef _A10001986_V_DBG
        Serial.printf("[OTA] Update file: %s\n", upload.filename.c_str());
        #endif

        if(!Update.begin(UPDATE_SIZE_UNKNOWN)) {
            #ifdef _A10001986_DBG
            Serial.printf("[ERROR] OTA Update ERROR %d\n", Update.getError());
            #endif
            _uplError = true;
        }

    } else if(upload.status == UPLOAD_FILE_WRITE) {

        if(!_uplError) {

            if(Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
                #ifdef _A10001986_DBG
                Serial.printf("[ERROR] OTA Update WRITE ERROR %d\n", Update.getError());
                #endif
                _uplError = true;
            }

        }

    } else if(upload.status == UPLOAD_FILE_END) {

        if(!_uplError) {

            #ifdef _A10001986_V_DBG
            Serial.printf("[OTA] OTA FILE END bytes: %d\n", upload.totalSize);
            #endif

            if(!Update.end(true)) {
                _uplError = true;
            }

        }

    } else if(upload.status == UPLOAD_FILE_ABORTED) {

        Update.abort();

        #ifdef _A10001986_DBG
        Serial.println("[OTA] Update was aborted");
        #endif

        if(_postotaupdatecallback) {
            _postotaupdatecallback(false);
        }

        delay(1000);  // Not '_delay'

        ESP.restart();
    }

    delay(0);
}

/*
 * HTTPD CALLBACK Update->UPDATE page handler
 */
void WiFiManager::handleUpdateDone()
{
    unsigned int mySize = 0;
    unsigned int headSize = 0;
    uint32_t incFlags = 0;
    bool res = !Update.hasError();

    #ifdef _A10001986_V_DBG
    Serial.println("<- Handle update done");
    #endif

    if(res) incFlags = incGFXMSG;

    mySize = getHTTPHeadLength(S_titleupd, incFlags);

    if(res) {
        mySize += STRLEN(HTTP_UPDATE_SUCCESS);
    } else {
        mySize += STRLEN(HTTP_UPDATE_FAIL1) + STRLEN(HTTP_UPDATE_FAIL2);
        mySize += strlen(Update.errorString());
    }

    mySize += STRLEN(HTTP_END);

    #ifdef _A10001986_DBG
    Serial.printf("handleUpdateDone: calced content size %d\n", mySize);
    #endif

    String page;
    page.reserve(mySize + 16);

    getHTTPHeadNew(page, S_titleupd, incFlags);

    if(res) {
        page += FPSTR(HTTP_UPDATE_SUCCESS);
        #ifdef _A10001986_DBG
        Serial.println("[OTA] update ok");
        #endif
    } else {
        page += FPSTR(HTTP_UPDATE_FAIL1);
        page += Update.errorString();
        page += FPSTR(HTTP_UPDATE_FAIL2);
        #ifdef _A10001986_DBG
        Serial.println("[OTA] update failed");
        #endif
    }

    page += FPSTR(HTTP_END);

    HTTPSend(page, false);

    if(_postotaupdatecallback) {
        _postotaupdatecallback(res);
    }

    delay(1000);  // Not '_delay'

    if(res) {
        ESP.restart();
    }
}

/**
 * HTTPD CALLBACK 404
 */
void WiFiManager::handleNotFound()
{
    server->send(404, FPSTR(HTTP_HEAD_CT2), S_notfound);
}

/****************************************************************************
 *
 * Misc
 *
 ****************************************************************************/

// PUBLIC METHODS

// SETTERS

void WiFiManager::setConnectTimeout(unsigned long seconds)
{
    _connectTimeout = seconds * 1000;
}

void WiFiManager::setConnectRetries(uint8_t numRetries)
{
    _connectRetries = constrain(numRetries, 1, 10);
}

#ifdef WM_ADDLSETTERS
void WiFiManager::setHttpPort(uint16_t port)
{
    _httpPort = port;
}
#endif

// Set static IP for AP
#ifdef WM_AP_STATIC_IP
void WiFiManager::setAPStaticIPConfig(IPAddress& ip, IPAddress& gw, IPAddress& sn)
{
    _ap_static_ip = ip;
    _ap_static_gw = gw;
    _ap_static_sn = sn;
}
#endif

// Set static IP for STA
void WiFiManager::setSTAStaticIPConfig(IPAddress& ip, IPAddress& gw, IPAddress& sn)
{
    _sta_static_ip = ip;
    _sta_static_gw = gw;
    _sta_static_sn = sn;
}

// Set static IP + DNS for STA
void WiFiManager::setSTAStaticIPConfig(IPAddress& ip, IPAddress& gw, IPAddress& sn, IPAddress& dns)
{
    setSTAStaticIPConfig(ip,gw,sn);
    _sta_static_dns = dns;
}

#ifdef WM_ADDLSETTERS
void WiFiManager::setMinimumRSSI(int rssi)
{
    _minimumRSSI = rssi;
}
#endif

// showUploadContainer(): Toggle displaying the sound-pack upload fields
#ifdef WM_UPLOAD
void WiFiManager::showUploadContainer(bool enable, const char *contName, const char *contVer, bool isInstalled)
{
    _showUploadSnd = enable;
    memset(_sndContName, 0, sizeof(_sndContName));
    if(contName) {
        strncpy(_sndContName, contName, sizeof(_sndContName) - 1);
    }
    _sndContVer = contVer;
    _sndIsInstalled = isInstalled;
}
#endif

#ifdef WM_FW_HW_VER
void WiFiManager::setReqFirmwareVersion(const char *x)
{
    _rfw = x;
}
#endif

void WiFiManager::setDownloadLink(const char *theLink, bool haveCVer, const char *nv)
{
    _downloadLink = theLink;
    _nv = nv;
    _vd = haveCVer;
}

// setMenu(): Set main menu items and order
// eg.:  const int8_t menu[size] = {WM_MENU_WIFI, WM_MENU_PARAM, WM_MENU_END};
//       WiFiManager.setMenu(menu, size);
void WiFiManager::setMenu(const int8_t *menu, unsigned int size, bool doCopy)
{
    if(_menuIdArr && !_menuArrConst)
        free((void *)_menuIdArr);

    _menuIdArr = NULL;

    if(!size) return;

    if(!doCopy) {
        _menuIdArr = (int8_t *)menu;
        _menuArrConst = true;
        return;
    }

    _menuArrConst = false;
    _menuIdArr = (int8_t *)malloc(size + 1);
    memset(_menuIdArr, WM_MENU_END, size + 1);

    for(int i = 0; i < size; i++) {
        if(menu[i] > WM_MENU_MAX) continue;
        if(menu[i] == WM_MENU_END) break;
        _menuIdArr[i] = menu[i];
    }
}

// setWiFiAPMaxClients(): Set softAP max client number
void WiFiManager::setWiFiAPMaxClients(int newmax)
{
    if(newmax > 10) newmax = 10;
    else if(newmax < 1) newmax = 1;
    _ap_max_clients = newmax;
}


// GETTERS


// Make some HTML templates available to user app
const char * WiFiManager::getHTTPSTART(int& titleStart)
{
    char *t = strstr(HTTP_HEAD_START, T_v);
    if(t) {
        titleStart = t - HTTP_HEAD_START;
    } else {
        titleStart = -1;
    }
    return HTTP_HEAD_START;
}

const char * WiFiManager::getHTTPSCRIPT()
{
    return HTTP_SCRIPT;
}

const char * WiFiManager::getHTTPSTYLE()
{
    return HTTP_STYLE;
}

const char * WiFiManager::getHTTPSTYLEOK()
{
    return HTTP_STYLE_MSG;
}

/*
 * AP channel selection
 *
 */

bool WiFiManager::_getbestapchannel(int32_t& channel, int& quality)
{
    int32_t myarr[13];
    bool    chfree[13];
    int32_t minRSSIch = 0;
    int32_t minRSSI = 1, j, k;
    int32_t chmax[3];
    int32_t chlp[3];
    int i, mainfree = 0, mainrfree = 0;

    quality = 1;

    for(i = 0; i < 13; i++) {
        myarr[i] = -1000;
        chfree[i] = true;
    }
    for(i = 0; i < 3; i++) {
      chmax[i] = 1;
      chlp[i] = 1;
    }

    #define TAKE_AS_FREE -84

    if(_numNetworks > 0 && _lastscan) {
        for(i = 0; i < _numNetworks; i++) {
            j = WiFi.channel(i);
            if(j >= 1 && j <= 13) {
                j--;
                k = WiFi.RSSI(i);
                // Take channels with very bad RSSI as free
                if(j == 0 || j == 5|| j == 10 || k > TAKE_AS_FREE) {
                    chfree[j] = false;
                    if(myarr[j] < k) myarr[j] = k;
                }
                if(k < minRSSI) {
                    minRSSI = k;
                    minRSSIch = j + 1;
                }
            }
        }

        #define CHFREE(x) (chfree[x-1])
        #define CHRSSI(x) (myarr[x-1])
        #define MAX3(a, b, c) (((a) > (b)) ? (((a) > (c)) ? (a) : (c)) : (((b) > (c)) ? (b) : (c)))

        // If all main channels (1,6,11) are somewhat taken, see
        // if their RSSI is below TAKE_AS_FREE, and count them as
        // unused if that is the case.
        if(!CHFREE(1) && !CHFREE(6) && !CHFREE(11)) {
            if(CHRSSI(1)  <= TAKE_AS_FREE) { CHFREE(1)  = true; CHRSSI(1)  = -1000; }
            if(CHRSSI(6)  <= TAKE_AS_FREE) { CHFREE(6)  = true; CHRSSI(6)  = -1000; }
            if(CHRSSI(11) <= TAKE_AS_FREE) { CHFREE(11) = true; CHRSSI(11) = -1000; }
        }

        // Check if 1, 6, 11 are entirely free (no overlapping channels used)
        // (and find the maximum RSSI of overlapping used channels in the process)
        if(CHFREE(1)) {
            if(CHFREE(2) && CHFREE(3)) {
                if(CHFREE(4)) { channel = 1; return true; }
                chlp[0] = CHRSSI(4);
                mainrfree++;
            }
            chmax[0] = MAX3(CHRSSI(2), CHRSSI(3), CHRSSI(4));
            mainfree++;
        }
        if(CHFREE(6)) {
            if(CHFREE(3) && CHFREE(4) && CHFREE(5) &&
               CHFREE(7) && CHFREE(8) && CHFREE(9)) { channel = 6; return true; }
            chmax[1]  = MAX3(CHRSSI(7), CHRSSI(8), CHRSSI(9));
            int32_t ch6max1 = MAX3(CHRSSI(3), CHRSSI(4), CHRSSI(5));
            if(ch6max1 > chmax[1]) chmax[1] = ch6max1;
            if(CHFREE(4) && CHFREE(5) && CHFREE(7) && CHFREE(8)) {
                chlp[1] = CHRSSI(3);
                if(chlp[1] < CHRSSI(9)) chlp[1] = CHRSSI(9);
                mainrfree++;
            }
            mainfree++;
        }
        if(CHFREE(11)) {
            if(CHFREE(10) && CHFREE(9) && CHFREE(8) &&
               CHFREE(12) && CHFREE(13)) { channel = 11; return true; }
            chmax[2] = MAX3(CHRSSI(8), CHRSSI(9), CHRSSI(10));
            if(CHRSSI(12) > chmax[2]) chmax[2] = CHRSSI(12);
            if(CHRSSI(13) > chmax[2]) chmax[2] = CHRSSI(13);
            if(CHFREE(9) && CHFREE(10) && CHFREE(12)) {
                chlp[2] = CHRSSI(8);
                if(chlp[2] < CHRSSI(13)) chlp[2] = CHRSSI(13);
                mainrfree++;
            }
            mainfree++;
        }

        // Find lowest RSSI on least problematic overlapping channels
        if(mainrfree) {
            if(chlp[0] <= chlp[1]) {
                if(chlp[0] < chlp[2]) { channel = 1; }
                else { channel = 11; }
            } else if(chlp[1] < chlp[2]) {
                channel = 6;
            } else {
                channel = 11;
            }
            return true;
        }

        quality = 0;

        // If two or more of the "main" channels are free, find minimum RSSI of
        // used overlapping neighbor channels
        if(mainfree > 1) {
            if(chmax[0] <= chmax[1]) {
                if(chmax[0] < chmax[2]) { channel = 1; }
                else { channel = 11; }
            } else if(chmax[1] < chmax[2]) {
                channel = 6;
            } else {
                channel = 11;
            }
            return true;
        }

        // If only one of the mains is free, check neighborhood.
        #define MINRSSI -75
        if(CHFREE(1)) {
            if(CHRSSI(2) < MINRSSI && CHRSSI(3) < MINRSSI) {
                channel = 1; return true;
            }
        } else if(CHFREE(6)) {
            if(CHRSSI(4) < MINRSSI && CHRSSI(5) < MINRSSI &&
               CHRSSI(7) < MINRSSI && CHRSSI(8) < MINRSSI) {
                channel = 6; return true;
            }
        } else if(CHFREE(11)) {
            if(CHRSSI(9) < MINRSSI && CHRSSI(10) < MINRSSI &&
               CHRSSI(12) < MINRSSI && CHRSSI(13) < MINRSSI) {
               channel = 11; return true;
            }
        }

        // See if we fit inbetween
        if(CHFREE(2) && CHFREE(3) && CHFREE(4) && CHFREE(5)) {
            if(CHRSSI(1) < CHRSSI(6)) { channel = 3; return true; }
            else                      { channel = 4; return true; }
        }

        if(CHFREE(7) && CHFREE(8) && CHFREE(9) && CHFREE(10)) {
            if(CHRSSI(6) < CHRSSI(11)) { channel = 8; return true; }
            else                       { channel = 9; return true; }
        }

        // find one with unused immediate neighbor channels
        // (yes, 10, max we can use is 11, so we check up to 12)
        for(i = 0; i < 10; i++) {
            if(chfree[i] && chfree[i+1] && chfree[i+2]) {
                channel = i+1+1;
                return true;
            }
        }

        quality = -1;

        // find any free one
        for(i = 0; i < 11; i++) {
            if(chfree[i]) {
                channel = i + 1;
                return true;
            }
        }

        // return channel with lowest RSSI
        channel = minRSSIch;

        return true;
    }

    return false;
}

bool WiFiManager::getBestAPChannel(int32_t& channel, int& quality)
{
    if(_lastscan && (_lastscan == _bestChCacheTime)) {
        quality = _bestChCache >> 8;
        channel = _bestChCache & 0xff;
        return true;
    }

    if(_numNetworks > 0 && _lastscan) {
         bool ret = _getbestapchannel(channel, quality);
         if(ret) {
            _bestChCacheTime = _lastscan;
            _bestChCache = (quality << 8) | channel;
         }
         return ret;
    }

    return false;
}


// HELPERS

void WiFiManager::getDefaultAPName(char *apName)
{
    sprintf(apName, "ESP32_%x", ESP.getEfuseMac());
    while(*apName) {
        *apName = toupper((unsigned char) *apName);
        apName++;
    }
}


// htmlEntities(): Encode for HTML, but do not garble UTF8 characters
String WiFiManager::htmlEntities(String& str, bool forprint)
{
    int i, e, slen = str.length();
    unsigned char c;
    char buf[8];

    String dstr;
    dstr.reserve(htmlEntitiesLen(str, forprint) + 8);

    for(i = 0; i < slen; i++) {
        c = (unsigned char)str.charAt(i);
        if(c == '&')        dstr += "&amp;";
        else if(c == '\'')  dstr += "&#39;";
        else if(c == '<' )  dstr += "&lt;";
        else if(c == '>')   dstr += "&gt;";
        else if(c == ' ' && forprint) dstr += "&nbsp;";
        else {
            e = 1;
            if     (c >= 192 && c < 224)  e = 2;
            else if(c >= 224 && c < 240)  e = 3;
            else if(c >= 240 && c < 248)  e = 4;  // Invalid UTF8 >= 245, but consider 4-byte char anyway

            if((i + e) >= slen) {
                e = slen - i;
            }
            memcpy(buf, str.c_str() + i, e);
            buf[e] = 0;
            dstr += (const char *)buf;

            i += (e - 1);
        }
    }

    return dstr;
}

int WiFiManager::htmlEntitiesLen(String& str, bool forprint)
{
    int size = 0;
    int i, e, slen = str.length();
    unsigned char c;

    for(i = 0; i < slen; i++) {
        c = (unsigned char)str.charAt(i);
        if(c == '&' || c == '\'')       size += 5;
        else if(c == '<' || c == '>')   size += 4;
        else if(c == ' ')               size += forprint ? 6 : 1;
        else {
            e = 1;
            if     (c >= 192 && c < 224)  e = 2;
            else if(c >= 224 && c < 240)  e = 3;
            else if(c >= 240 && c < 248)  e = 4;  // Invalid UTF8 >= 245, but consider 4-byte char anyway

            if((i + e) >= slen) {
                e = slen - i;
            }

            size += e;

            i += (e - 1);
        }
    }

    return size;
}

// Check SSID: Reject SSIDs with chars < 32 and 127
bool WiFiManager::checkSSID(String &ssid)
{
    int i, e, slen = ssid.length();
    unsigned char c;

    for(i = 0; i < slen; i++) {
        c = (unsigned char)ssid.charAt(i);
        if(c < 32 || c == 127) return false;

        e = 1;
        if     (c >= 192 && c < 224)  e = 2;
        else if(c >= 224 && c < 240)  e = 3;
        else if(c >= 240 && c < 248)  e = 4;  // Invalid UTF8 >= 245, but consider 4-byte char anyway

        if((i + e) >= slen) {
            e = slen - i;
        }

        i += (e - 1);
    }

    return true;
}

// map rssi to "quality" 1-4
long WiFiManager::wmmap(long x)
{
    if(x <= -83) return 1;
    if(x <= -74) return 2;
    if(x <= -65) return 3;
    return 4;
}

// delay() replacement
void WiFiManager::_delay(unsigned int mydel)
{
    if(_delayreplacement) {
        _delayreplacement(mydel);
    } else {
        delay(mydel);
    }
}

/*
 * getStoredCredentials()
 *
 * Transitional function to read out the NVS-stored credentials
 * This MUST NOT be called after WiFi is initialized
 * [xlen = size of available memory, INCLUDING 0-term!]
 *
 */
void WiFiManager::getStoredCredentials(char *ssid, size_t slen, char *pass, size_t plen)
{
    size_t sslen = slen - 1;
    size_t pplen = plen - 1;

    WiFi.mode(WIFI_AP_STA);

    wifi_config_t conf;
    esp_wifi_get_config(WIFI_IF_STA, &conf);

    memset(ssid, 0, slen);
    memset(pass, 0, plen);

    if(sslen > sizeof(conf.sta.ssid)) sslen = sizeof(conf.sta.ssid);
    if(pplen > sizeof(conf.sta.password)) pplen = sizeof(conf.sta.password);

    memcpy(ssid, conf.sta.ssid, sslen);
    memcpy(pass, conf.sta.password, pplen);

    delay(100);
    WiFi.mode(WIFI_OFF);
    delay(500);
}

/*
 * WiFI Event handling
 *
 */

// Event callback
// Event Callback functions must be thread-safe; they must not access
// shared/global variables directly without locking, and must only call
// similarly thread-safe functions. Some core operations like
// Serial.print() are thread-safe but many functions are not.
// Notably, WiFi.onEvent() and WiFi.removeEvent() are not thread-safe and
// should never be invoked from a callback thread.

void WiFiManager::WiFiEvent(WiFiEvent_t event, arduino_event_info_t info)
{
    if(!_beginSemaphore)
        return;

    uint32_t v = 0;

    switch(event) {
    case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
        // The core *should* reconnect automatically.
        // Unless: Wrong or missing password. Yes, this
        // is also reported through DISCONNECTED.
        v = WM_EVB_DISCONNECTED;
        break;
    case ARDUINO_EVENT_WIFI_STA_GOT_IP:
        v = WM_EVB_GOTIP;
        break;
    case ARDUINO_EVENT_WIFI_AP_START:
        v = WM_EVB_APSTART;
        break;
    case ARDUINO_EVENT_WIFI_AP_STOP:
        v = WM_EVB_APSTOP;
        break;
    case ARDUINO_EVENT_WIFI_STA_START:
        v = WM_EVB_STASTART;
        break;
    case ARDUINO_EVENT_WIFI_STA_STOP:
        v = WM_EVB_STASTOP;
        break;
    case ARDUINO_EVENT_WIFI_STA_CONNECTED:
        v = WM_EVB_STACONN;
        break;
    case ARDUINO_EVENT_WIFI_SCAN_DONE:
        v = WM_EVB_SCAN_DONE;
        _numNetworksAsync = WiFi.scanComplete();
        #ifdef _A10001986_DBG
        Serial.printf("Callback: Async scan finished, %d networks\n", _numNetworksAsync);
        #endif
        break;
    }

    if(v) {
        _WiFiEventMask |= v;
        //Atomic_OR_u32(&_WiFiEventMask, v);
    }

    #ifdef _A10001986_DBG
    Serial.printf("WM: WiFi Event %d\n", event);
    #endif

    #ifdef WM_EVENTCB
    if(_wifieventcallback) {
        _wifieventcallback(event);
    }
    #endif
}

void WiFiManager::WiFi_installEventHandler()
{
    using namespace std::placeholders;
    if(wm_event_id == 0) {
        wm_event_id = WiFi.onEvent(std::bind(&WiFiManager::WiFiEvent, this, _1, _2));
    }
}

void WiFiManager::andWiFiEventMask(uint32_t to_and)
{
    _WiFiEventMask &= to_and;
    //Atomic_AND_u32(&_WiFiEventMask, to_and);
}

/////////////////////////////////////////////////////////////////////////////////////
//                                     DEBUG                                       //
/////////////////////////////////////////////////////////////////////////////////////

#ifdef _A10001986_DBG

void WiFiManager::debugSoftAPConfig()
{
    char myssid[34];
    wifi_config_t conf_config;

    esp_wifi_get_config(WIFI_IF_AP, &conf_config); // == ESP_OK
    wifi_ap_config_t config = conf_config.ap;

    memset(myssid, 0, sizeof(myssid));
    memcpy(myssid, config.ssid, 32);

    Serial.println("SoftAP Configuration:");
    Serial.printf("ssid:            %s\n", myssid);
    Serial.printf("password:        %s\n", (char *) config.password);
    Serial.printf("ssid_len:        %d\n", config.ssid_len);
    Serial.printf("channel:         %d\n", config.channel);
    Serial.printf("authmode:        %d\n", config.authmode);
    Serial.printf("ssid_hidden:     %d\n", config.ssid_hidden);
    Serial.printf("max_connection:  %d\n", config.max_connection);
}

String WiFiManager::getWLStatusString(uint8_t status)
{
    if(status <= 7) return WIFI_STA_STATUS[status];
    return FPSTR(S_NA);
}

String WiFiManager::getModeString(uint8_t mode) {
    if(mode <= 3) return WIFI_MODES[mode];
    return FPSTR(S_NA);
}

#endif  // _A10001986_DBG
