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

#include "WiFiManager.h"
#include "wm_strings_en.h"

uint8_t WiFiManager::_eventlastconxresult = WL_IDLE_STATUS;
bool    WiFiManager::_gotip = false;

#define STRLEN(x) (sizeof(x)-1)

/*
 * --------------------------------------------------------------------------------
 *  WiFiManagerParameter
 * --------------------------------------------------------------------------------
 */

WiFiManagerParameter::WiFiManagerParameter(const char *custom)
{
    _id             = NULL;
    _label          = NULL;
    _length         = 0;
    _value          = NULL;
    _labelPlacement = WFM_LABEL_DEFAULT;
    _customHTML     = custom;
}

WiFiManagerParameter::WiFiManagerParameter(const char *(*CustomHTMLGenerator)(const char *))
{
    _id             = NULL;
    _label          = NULL;
    _length         = 0;
    //_value is union with Generator
    _labelPlacement = WFM_LABEL_DEFAULT;
    _customHTML     = NULL;
    _customHTMLGenerator = CustomHTMLGenerator;
}

WiFiManagerParameter::WiFiManagerParameter(const char *id, const char *label)
{
    init(id, label, NULL, 0, NULL, WFM_LABEL_DEFAULT);
}

WiFiManagerParameter::WiFiManagerParameter(const char *id, const char *label, const char *defaultValue, int length)
{
    init(id, label, defaultValue, length, NULL, WFM_LABEL_DEFAULT);
}

WiFiManagerParameter::WiFiManagerParameter(const char *id, const char *label, const char *defaultValue, int length, const char *custom)
{
    init(id, label, defaultValue, length, custom, WFM_LABEL_DEFAULT);
}

WiFiManagerParameter::WiFiManagerParameter(const char *id, const char *label, const char *defaultValue, int length, const char *custom, int labelPlacement)
{
    init(id, label, defaultValue, length, custom, labelPlacement);
}

void WiFiManagerParameter::init(const char *id, const char *label, const char *defaultValue, int length, const char *custom, int labelPlacement)
{
    _id             = id;
    _label          = label;
    _labelPlacement = labelPlacement;
    _customHTML     = custom;
    _length         = 0;
    _value          = NULL;
    setValue(defaultValue, length);
}

WiFiManagerParameter::~WiFiManagerParameter()
{
    if(_id && _value != NULL) {
        delete[] _value;
    }

    // setting length 0, ideally the entire parameter should be removed,
    // or added to wifimanager scope so it follows
    _length = 0;
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

    memset(_value, 0, _length + 1); // explicit null

    if(defaultValue) {
        strncpy(_value, defaultValue, _length);
    }
}
const char* WiFiManagerParameter::getValue() const
{
    return _value;
}
const char* WiFiManagerParameter::getID() const
{
    return _id;
}
const char* WiFiManagerParameter::getLabel() const
{
    return _label;
}
int WiFiManagerParameter::getValueLength() const
{
    return _length;
}
int WiFiManagerParameter::getLabelPlacement() const
{
    return _labelPlacement;
}
const char* WiFiManagerParameter::getCustomHTML() const
{
    return _customHTML;
}

/*
 * --------------------------------------------------------------------------------
 *  WiFiManager
 * --------------------------------------------------------------------------------
 */

bool WiFiManager::CheckParmID(const char *id)
{
    for(size_t i = 0; i < strlen(id); i++) {
        if(!(isAlphaNumeric(id[i])) && !(id[i] == '_')) {
            return false;
        }
    }

    return true;
}

// Settings page params

bool WiFiManager::addParameter(WiFiManagerParameter *p)
{
    // check param id is valid, unless null
    if(p->getID()) {
        if(!CheckParmID(p->getID())) return false;
    }

    // init params if never malloc
    if(!_params) {
        _params = (WiFiManagerParameter**)malloc(_max_params * sizeof(WiFiManagerParameter*));
    }

    // resize the params array by increment of WIFI_MANAGER_MAX_PARAMS
    if(_paramsCount == _max_params) {

        _max_params += WIFI_MANAGER_MAX_PARAMS;

        WiFiManagerParameter** new_params = (WiFiManagerParameter**)realloc(_params, _max_params * sizeof(WiFiManagerParameter*));

        if(new_params != NULL) {
            _params = new_params;
        } else {
            return false;
        }
    }

    _params[_paramsCount] = p;
    _paramsCount++;

    return true;
}

WiFiManagerParameter** WiFiManager::getParameters()
{
    return _params;
}

int WiFiManager::getParametersCount()
{
    return _paramsCount;
}

void WiFiManager::allocParms(int numParms)
{
    _max_params = numParms;
}

// Params

bool WiFiManager::addWiFiParameter(WiFiManagerParameter *p)
{
    // check param id is valid, unless null
    if(p->getID()) {
        if(!CheckParmID(p->getID())) return false;
    }

    // init params if never malloc
    if(!_wifiparams) {
        _wifiparams = (WiFiManagerParameter**)malloc(_max_wifi_params * sizeof(WiFiManagerParameter*));
    }

    // resize the params array by increment of WIFI_MANAGER_MAX_PARAMS
    if(_wifiparamsCount == _max_wifi_params) {

        _max_wifi_params += WIFI_MANAGER_MAX_PARAMS;

        WiFiManagerParameter** new_params = (WiFiManagerParameter**)realloc(_wifiparams, _max_wifi_params * sizeof(WiFiManagerParameter*));

        if(new_params) {
            _wifiparams = new_params;
        } else {
            return false;
        }
    }

    _wifiparams[_wifiparamsCount] = p;
    _wifiparamsCount++;

    return true;
}

WiFiManagerParameter** WiFiManager::getWiFiParameters()
{
    return _wifiparams;
}

int WiFiManager::getWiFiParametersCount()
{
    return _wifiparamsCount;
}

void WiFiManager::allocWiFiParms(int numParms)
{
    _max_wifi_params = numParms;
}

// Constructors

WiFiManager::WiFiManager()
{
    WiFiManagerInit();
}

void WiFiManager::WiFiManagerInit()
{
    const int8_t defMenu[3] = { WM_MENU_WIFI, WM_MENU_SEP, WM_MENU_UPDATE };

    setMenu(defMenu, 3);

    _max_params = _max_wifi_params = WIFI_MANAGER_MAX_PARAMS;

    memset(_ssid, 0, sizeof(_ssid));
    memset(_pass, 0, sizeof(_pass));
    memset(_apName, 0, sizeof(_apName));
    memset(_apPassword, 0, sizeof(_apPassword));
}

// destructor
WiFiManager::~WiFiManager()
{
    _end();

    // free parameter arrays
    if(_params) {
        free(_params);
        _params = NULL;
    }
    if(_wifiparams) {
        free(_wifiparams);
        _wifiparams = NULL;
    }
    _paramsCount = _wifiparamsCount = 0;

    // remove event
    if(wm_event_id) {
        WiFi.removeEvent(wm_event_id);
    }
}

void WiFiManager::_begin()
{
    if(_hasBegun) return;
    _hasBegun = true;
}

void WiFiManager::_end()
{
    _hasBegun = false;
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

// autoConnect: WiFi Connect
//
// The name "autoConnect" is somewhat misleading, it does not "auto"
// connect, it simply connects to the given ssid, and falls back to
// AP-mode in case connecting fails.
//
// public; return: bool connected = true if STA connected, false it softAP started

bool WiFiManager::autoConnect(const char *ssid, const char *pass, const char *apName, const char *apPassword)
{
    #ifdef _A10001986_DBG
    Serial.println("AutoConnect");
    #endif

    // Store given ssid/pass

    memset(_ssid, 0, sizeof(_ssid));
    memset(_pass, 0, sizeof(_pass));
    if(ssid && *ssid) {
        strncpy(_ssid, ssid, sizeof(_ssid) - 1);
    }
    if(pass && *pass) {
        strncpy(_pass, pass, sizeof(_pass) - 1);
    }

    // We do never ever use NVS saved data, nor do we save data to NVS
    WiFi.persistent(false);

    // Set hostname (if given)

    // The setHostname() function must be called BEFORE Wi-Fi is started
    // with WiFi.begin(), WiFi.softAP(), WiFi.mode(), or WiFi.run(). To change
    // the name, reset Wi-Fi with WiFi.mode(WIFI_MODE_NULL), then proceed
    // with WiFi.setHostname(...) and restart WiFi from scratch.

    if(*_hostname) {
        if(WiFi.getMode() & WIFI_STA) {
            WiFi.mode(WIFI_OFF);
            unsigned int timeout = millis() + 1200;
            while((WiFi.getMode() != WIFI_OFF) && (millis() < timeout)) {
                _delay(50);
            }
        }
        WiFi.setHostname(_hostname);
    }

    _begin();

    // Attempt to connect to given network, on fail fallback to AP config portal

    // Set STA mode (not strictly required, WiFi.begin will do that as well)
    // This does not actually connect, it just sets (current mode|STA)
    if(!WiFi.enableSTA(true)) {
        #ifdef _A10001986_DBG
        Serial.println("[FATAL] Unable to enable wifi!");
        #endif
        return false;
    }

    // Install WiFi event handler
    WiFi_installEventHandler();

    // Callback: After lowlevelinit
    #if WM_PRECONNECTCB
    if(_preconnectcallback) {
        _preconnectcallback();
    }
    #endif

    bool connected = false;

    if(WiFi.status() == WL_CONNECTED) {

        #ifdef _A10001986_DBG
        Serial.println("AutoConnect: Already connected");
        #endif

        connected = true;
        setStaticConfig();    // Can this still be done when connected?
    }

    if((!ssid || !*ssid) && !connected) {

        _lastconxresult = WL_IDLE_STATUS;

    } else if(connected || (connectWifi(ssid, pass) == WL_CONNECTED)) {

        #ifdef _A10001986_DBG
        Serial.println("AutoConnect: SUCCESS");
        Serial.printf("STA IP Address: %s\n", WiFi.localIP().toString().c_str());
        if(*_hostname) {
            Serial.printf("hostname: %s\n", getWiFiHostname().c_str());
        }
        #endif

        _lastconxresult = WL_CONNECTED;

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
    Serial.println("AutoConnect: Not connected");
    #endif

    // Disable STA if enabled
    if(WiFi.getMode() & WIFI_STA) {
        WiFi.enableSTA(false);
        delay(500);
    }

    // Start softAP and config portal

    startConfigPortal(apName, apPassword, ssid, pass);

    return false; // Yes, false; true means "connected".
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

    _gotip = false;
    if(!(WiFi.getMode() & WIFI_AP)) {
        if(WiFi.enableAP(true)) {
            unsigned long timeoutmillis = millis() + 3000;
            while(millis() < timeoutmillis) {
                if(_gotip) break;
                _delay(100);
            }
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

    // Now fire softAP() which does not much beyond setting our
    // config and restart the AP, and comparing the then current
    // AP config to the desired one.

    _gotip = false;

    // start soft AP; default channel is 1 here and in esp library
    ret = WiFi.softAP(_apName,
            (strlen(_apPassword) > 0) ? _apPassword : NULL,
            (channel > 0) ? channel : 1,
            false,
            _ap_max_clients);

    // Wait for AP_START event to make sure we can fully use the AP
    if(ret) {
        unsigned long timeoutmillis = millis() + 1000;
        while(millis() < timeoutmillis) {
            if(_gotip) break;
            _delay(100);
        }
    }

    #ifdef WM_DODEBUG
    if(_debugLevel >= DEBUG_DEV) debugSoftAPConfig();
    #endif

    #ifdef _A10001986_DBG
    if(!ret) Serial.println("[ERROR] There was a problem starting the AP");
    Serial.printf("AP IP address: %s\n", WiFi.softAPIP().toString().c_str());
    #endif

    // set ap hostname (seems ok to do this after softAP())
    if(ret && *_hostname) {
        bool test = WiFi.softAPsetHostname(_hostname);
        #ifdef _A10001986_DBG
        if(!test) Serial.println("Failed to set AP hostname");
        #endif
    }

    return ret;
}

/*
 * Start/stop web portal (in STA mode = config portal in AP mode)
 *
 */

void WiFiManager::startWebPortal()
{
    if(configPortalActive || webPortalActive)
        return;

    setupConfigPortal();
    webPortalActive = true;
}

void WiFiManager::stopWebPortal()
{
    if(!configPortalActive && !webPortalActive)
        return;

    #ifdef _A10001986_DBG
    Serial.println("Stopping Web Portal");
    #endif

    webPortalActive = false;
    shutdownConfigPortal();
}

/*
 * Server setup: Web, DNS, mDNS
 *
 */

void WiFiManager::setupHTTPServer()
{
    #ifdef _A10001986_DBG
    Serial.println("Starting Web Portal");
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
    server->on(R_wifisave,   std::bind(&WiFiManager::handleWifiSave, this));
    server->on(R_param,      std::bind(&WiFiManager::handleParam, this));
    server->on(R_paramsave,  std::bind(&WiFiManager::handleParamSave, this));
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
    }
    #endif
}

void WiFiManager::setupConfigPortal()
{
    // init WebServer
    setupHTTPServer();

    // reset network scan cache
    _lastscan = 0;
}

// startConfigPortal: Start the softAP and the Config Portal.
//                    (Config Portal in AP mode = Web Portal in STA mode)
//
// public; return: bool success (result from password check)

bool WiFiManager::startConfigPortal(char const *apName, char const *apPassword, const char *ssid, const char *pass)
{
    _begin();

    if(configPortalActive) {
        #ifdef _A10001986_V_DBG
        Serial.println("Config Portal is already running");
        #endif
        return false;
    }

    // We never ever use NVS saved data, not do we write to NVS
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
    if(ssid && *ssid) {
        strncpy(_ssid, ssid, sizeof(_ssid) - 1);
    }
    if(pass && *pass) {
        strncpy(_pass, pass, sizeof(_pass) - 1);
    }

    #ifdef _A10001986_V_DBG
    Serial.println("Starting Config Portal");
    #endif

    if(!*_apName) getDefaultAPName(_apName);

    if(!validApPassword()) return false;

    // Shutdown sta if not connected, or else this will hang
    // channel scanning, and softap will not respond
    // -- Apparently obsolete, not required.
    #if 0
    if(_disableSTA || (!WiFi.isConnected() && _disableSTAConn)) {

        #ifdef WM_DISCONWORKAROUND    // see WiFiManager.h
        WiFi.mode(WIFI_AP_STA);
        #endif

        WiFi.disconnect()();

        WiFi.enableSTA(false);

        #ifdef _A10001986_V_DBG
        Serial.println("Disabling STA");
        #endif

    }
    #endif

    // Disconnect if connected and disable STA if enabled
    if(WiFi.isConnected()) {
        WiFi.disconnect();
        delay(50);
    }
    if(WiFi.getMode() & WIFI_STA) {
        WiFi.enableSTA(false);
        delay(500);
    }

    configPortalActive = true;

    // Install WiFi event handler
    WiFi_installEventHandler();

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

    // init configportal
    setupConfigPortal();

    // init DNS server
    setupDNSD();

    // init mDNS
    setupMDNS();

    #ifdef _A10001986_V_DBG
    Serial.println("Config Portal Running");
    #endif

    return true;
}

// process: Loop function to process Webserver,
//          DNS server and some cleanup.
// public

void WiFiManager::process(bool handleWeb)
{
    if(webPortalActive || configPortalActive) {
        processConfigPortal(handleWeb);
    }
}

// processConfigPortal: Loop function to process Webserver,
//                      DNS server and some cleanup.
// private

void WiFiManager::processConfigPortal(bool handleWeb)
{
    // DNS handler
    if(configPortalActive && dnsServer) {
        dnsServer->processNextRequest();
    }

    // HTTP handler
    if(handleWeb) {
        server->handleClient();
    }

    // Free memory taken for scans
    if(_lastscan && (millis() - _lastscan > _scancachetime)) {
        if((WiFi.scanComplete() != WIFI_SCAN_RUNNING) &&
           (_numNetworksAsync != WM_WIFI_SCAN_BUSY)) {
            WiFi.scanDelete();
            _numNetworks = _numNetworksAsync = 0;
            _lastscan = 0;
            #ifdef _A10001986_DBG
            Serial.println("Freeing scan result memory");
            #endif
        }
    }
}

// shutdownConfigPortal: Shut down CP, Webserver, DNS server,
//                       softAP; disable WiFi.
//
// public; return: bool success (softapdisconnect)

bool WiFiManager::shutdownConfigPortal()
{
    #ifdef _A10001986_V_DBG
    Serial.println("shutdownConfigPortal");
    #endif

    if(webPortalActive)
        return false;

    if(configPortalActive) {
        dnsServer->processNextRequest();
    }

    server->handleClient();

    // @todo what is the proper way to shutdown and free the server up
    // debug - many open issues about port not clearing for use with other servers
    server->stop();
    server.reset();

    // free wifi scan results
    WiFi.scanDelete();

    if(!configPortalActive)
        return false;

    dnsServer->stop();
    dnsServer.reset();

    // turn off AP, (true = set mode WIFI_OFF)
    bool ret = WiFi.softAPdisconnect(true);

    configPortalActive = false;

    #ifdef _A10001986_V_DBG
    Serial.println("configportal closed");
    #endif

    _end();
    return ret;
}

// connectWiFi: Connect to given network, and retry if failed
//
// private; return: uint8: connection result WL_XXX

uint8_t WiFiManager::connectWifi(const char *ssid, const char *pass)
{
    #ifdef _A10001986_V_DBG
    Serial.println("Connecting as wifi client...");
    #endif

    uint8_t retry = 1;
    uint8_t connRes = (uint8_t)WL_NO_SSID_AVAIL;

    // set Static IP if provided
    bool haveStatic = setStaticConfig();

    // disconnect before begin, in case anything is hung, this causes a 2 seconds delay for connect
    // @todo find out what status is when this is needed, can we detect it
    // and handle it, say in between states or idle_status to avoid these

    if(_cleanConnect) WiFi.disconnect();

    // if retry without delay (via begin()), the IDF is still busy even after returning status
    // E (5130) wifi:sta is connecting, return error
    // [E][WiFiSTA.cpp:221] begin(): connect failed!

    while(retry <= _connectRetries && (connRes != WL_CONNECTED)) {

        if(_connectRetries > 1) {
            if(_aggresiveReconn) _delay(1000); // add idle time before recon

            #ifdef _A10001986_DBG
            Serial.printf("Connecting Wifi, attempt %d of %d\n", retry, _connectRetries);
            #endif
        }

        _gotip = false;

        wifiConnectNew(ssid, pass);

        connRes = waitForConnectResult(haveStatic);

        #ifdef _A10001986_V_DBG
        Serial.printf("Connection result: %s\n", getWLStatusString(connRes).c_str());
        #endif

        retry++;
    }

    if(connRes != WL_SCAN_COMPLETED) {
        _lastconxresult = connRes;
        if(_lastconxresult == WL_CONNECT_FAILED || _lastconxresult == WL_DISCONNECTED) {
            if(_eventlastconxresult != WL_IDLE_STATUS) {
                _lastconxresult    = _eventlastconxresult;
                // _eventlastconxresult = WL_IDLE_STATUS;
            }
        }
    }

    #ifdef _A10001986_V_DBG
    Serial.printf("ConnectWifi returns %d (%d)\n", connRes, _lastconxresult);
    #endif

    return connRes;
}

// wifiConnectNew: Connect to a given wifi network.
//
// private; return: bool success = result from WiFi.begin()

bool WiFiManager::wifiConnectNew(const char *ssid, const char *pass)
{
    bool ret = false;

    #ifdef _A10001986_V_DBG
    Serial.printf("Connecting to WiFi network: %s\n", ssid);
    Serial.printf("Using Password: %s\n", pass);
    #endif

    ret = WiFi.begin(ssid, pass, 0, NULL, true);

    #ifdef _A10001986_DBG
    if(!ret) Serial.println("[ERROR] wifi begin failed");
    #endif

    return ret;
}

// setStaticConfig: Set static IP config (if configured)
//
// private; return: bool success = result from WiFi.config()

bool WiFiManager::setStaticConfig()
{
    #ifdef _A10001986_DBG
    Serial.printf("STA static IP: %s\n", _sta_static_ip.toString().c_str());
    #endif

    if(_sta_static_ip) {
        bool ret;

        #ifdef _A10001986_V_DBG
        Serial.println("Custom static IP/GW/Subnet/DNS");
        #endif

        if(_sta_static_dns) {
            ret = WiFi.config(_sta_static_ip, _sta_static_gw, _sta_static_sn, _sta_static_dns);
        } else {
            ret = WiFi.config(_sta_static_ip, _sta_static_gw, _sta_static_sn);
        }

        #ifdef _A10001986_DBG
        if(!ret) Serial.println("[ERROR] wifi config failed");
        else Serial.printf("STA IP set: %s\n", WiFi.localIP().toString().c_str());
        #endif

        return ret;
    }

    return false;
}

uint8_t WiFiManager::waitForConnectResult(bool haveStatic)
{
    return waitForConnectResult(haveStatic, _connectTimeout);
}

// waitForConnectResult
//
// private; return: uint8_t WL_XXX Status

uint8_t WiFiManager::waitForConnectResult(bool haveStatic, uint32_t timeout)
{
    unsigned long timeoutmillis;
    uint8_t status;

    // Event 7 (GOT_IP) is fired both with a static config
    // as well as DHCP. So we overrule the parameter in order
    // to wait in both cases.
    haveStatic = false;

    if(!timeout) {
        #ifdef _A10001986_DBG
        Serial.println("connectTimeout not set, waitForConnectResult...");
        #endif

        status = WiFi.waitForConnectResult();

        if(status == WL_CONNECTED) {
            if(!haveStatic) {
                timeoutmillis = millis() + 5000;
                while(millis() < timeoutmillis) {
                    if(_gotip) break;
                    _delay(100);
                }
            }
        }

        return status;
    }

    timeoutmillis = millis() + timeout;

    #ifdef _A10001986_V_DBG
    Serial.printf("%s ms timeout, waiting for connect...\n", timeout);
    #endif

    status = WiFi.status();

    while(millis() < timeoutmillis) {

        status = WiFi.status();

        if(status == WL_CONNECT_FAILED) {

            return status;

        } else if(status == WL_CONNECTED) {

            if(!haveStatic) {
                timeoutmillis = millis() + 5000;
                while(millis() < timeoutmillis) {
                    if(_gotip) break;
                    _delay(100);
                }
            }

            return status;
        }

        #ifdef _A10001986_DBG
        Serial.println(".");
        #endif

        _delay(100);
    }

    return status;
}


/****************************************************************************
 *
 * Website handling: Constructors for all pages
 *
 ****************************************************************************/

// Calc size of header
unsigned int WiFiManager::getHTTPHeadLength(const char *title, bool includeMSG, bool includeQI)
{
    int bufSize = STRLEN(HTTP_HEAD_START) - 3;  // -3 token
    bufSize += strlen(title);
    bufSize += STRLEN(HTTP_SCRIPT);
    bufSize += STRLEN(HTTP_STYLE);
    if(includeMSG) {
        bufSize += STRLEN(HTTP_STYLE_MSG);
    }
    if(includeQI) {
        bufSize += STRLEN(HTTP_STYLE_QI);
    }
    if(_customHeadElement) {
        bufSize += strlen(_customHeadElement);
    }
    bufSize += STRLEN(HTTP_HEAD_END);
    return bufSize;
}

// Construct header
void WiFiManager::getHTTPHeadNew(String& page, const char *title, bool includeMSG, bool includeQI)
{
    String temp;
    temp.reserve(STRLEN(HTTP_HEAD_START) + strlen(title));

    temp = FPSTR(HTTP_HEAD_START);
    temp.replace(FPSTR(T_v), title);
    page += temp;

    page += FPSTR(HTTP_SCRIPT);
    page += FPSTR(HTTP_STYLE);
    if(includeMSG) {
        page += FPSTR(HTTP_STYLE_MSG);
    }
    if(includeQI) {
        page += FPSTR(HTTP_STYLE_QI);
    }
    if(_customHeadElement) {
        page += _customHeadElement;
    }
    page += FPSTR(HTTP_HEAD_END);
}

// WIFI status at bottom of pages

int WiFiManager::reportStatusLen()
{
    int bufSize = 0;
    String SSID = WiFi_SSID();

    #ifdef _A10001986_V_DBG
    Serial.printf("reportStatus: _lastconxresult %d\n", _lastconxresult);
    #endif

    if(SSID != "") {
        bufSize = STRLEN(HTTP_STATUS_HEAD) - 3 + 1; // class for <span>
        bufSize += htmlEntitiesLen(SSID, true);
        if(WiFi.status() == WL_CONNECTED) {
            bufSize += STRLEN(HTTP_STATUS_ON) - (2*3);
            bufSize += 15;        // max len of ip address
        } else {
            bufSize += STRLEN(HTTP_STATUS_OFF) - (3*3);
            switch(_lastconxresult) {
            case WL_NO_SSID_AVAIL:          // connect failed, or ap not found
                bufSize += (STRLEN(HTTP_STATUS_OFFNOAP) + STRLEN(HTTP_STATUS_APMODE));
                break;
            case WL_CONNECT_FAILED:         // connect failed
                bufSize += (STRLEN(HTTP_STATUS_OFFFAIL) + STRLEN(HTTP_STATUS_APMODE));
                break;
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
        bufSize += STRLEN(HTTP_STATUS_TAIL);
    } else {
        bufSize = STRLEN(HTTP_STATUS_NONE);
    }

    #ifdef _A10001986_DBG
    Serial.printf("WM: reportStatusLen bufSize %d\n", bufSize);
    #endif

    return bufSize + 8;
}

void WiFiManager::reportStatus(String& page, unsigned int estSize)
{
    String SSID = WiFi_SSID();
    String str;
    if(estSize) str.reserve(estSize);

    if(SSID != "") {
        str = FPSTR(HTTP_STATUS_HEAD);
        if(WiFi.status() == WL_CONNECTED) {
            str += FPSTR(HTTP_STATUS_ON);
            str.replace(FPSTR(T_c), "g");
            str.replace(FPSTR(T_i), WiFi.localIP().toString());
            str.replace(FPSTR(T_v), htmlEntities(SSID, true));
        } else {
            str += FPSTR(HTTP_STATUS_OFF);
            str.replace(FPSTR(T_v), htmlEntities(SSID, true));
            switch(_lastconxresult) {
            case WL_NO_SSID_AVAIL:    // connect failed, or ap not found
                str.replace(FPSTR(T_c), "r");
                str.replace(FPSTR(T_r), FPSTR(HTTP_STATUS_OFFNOAP));
                str.replace(FPSTR(T_V), FPSTR(HTTP_STATUS_APMODE));
                break;
            case WL_CONNECT_FAILED:   // connect failed
                str.replace(FPSTR(T_c), "r");
                str.replace(FPSTR(T_r), FPSTR(HTTP_STATUS_OFFFAIL));
                str.replace(FPSTR(T_V), FPSTR(HTTP_STATUS_APMODE));
                break;
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
        str += FPSTR(HTTP_STATUS_TAIL);
    } else {
        str = FPSTR(HTTP_STATUS_NONE);
    }

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

            if(params[i]->getID()) {

                switch (params[i]->getLabelPlacement()) {
                case WFM_LABEL_BEFORE:
                case WFM_LABEL_AFTER:
                    mysize += STRLEN(HTTP_FORM_LABEL) + STRLEN(HTTP_FORM_PARAM);
                    mysize += STRLEN(HTTP_BR);
                    mysize -= (7*3);  // tokens
                    break;
                default:
                    // WFM_NO_LABEL
                    mysize += STRLEN(HTTP_FORM_PARAM);
                    mysize -= (5*3);  // tokens
                    break;
                }

                // <label for='{i}'>{t}</label>
                // <input id='{i}' name='{n}' maxlength='{l}' value='{v}' {c}>\n";
                mysize += (strlen(params[i]->getID()) * 3);    // 2x{i}, 1x{n}
                mysize += strlen(params[i]->getLabel());
                {
                  int vl = params[i]->getValueLength();
                  if     (vl <   10) mysize += 1;
                  else if(vl <  100) mysize += 2;
                  else if(vl < 1000) mysize += 3;
                  else               mysize += 5;
                }
                mysize += strlen(params[i]->getValue());
                if(params[i]->getCustomHTML()) {
                    mysize += strlen(params[i]->getCustomHTML());
                }

            } else if(params[i]->_customHTMLGenerator) {

                const char *t = (params[i]->_customHTMLGenerator)(NULL);
                if(t) {
                    mysize += strlen(t);
                    (params[i]->_customHTMLGenerator)(t);
                } else {
                    continue;
                }

            } else if(params[i]->getCustomHTML()) {

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

        char valLength[6];

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

            // Input templating
            // "<br/><input id='{i}' name='{n}' maxlength='{l}' value='{v}' {c}>";
            // if no ID use customhtml for item, else generate from param string

            if(params[i]->getID()) {

                switch (params[i]->getLabelPlacement()) {
                case WFM_LABEL_BEFORE:
                    pitem = FPSTR(HTTP_FORM_LABEL);
                    pitem += FPSTR(HTTP_BR);
                    pitem += FPSTR(HTTP_FORM_PARAM);
                    break;
                case WFM_LABEL_AFTER:
                    pitem = FPSTR(HTTP_FORM_PARAM);
                    pitem += FPSTR(HTTP_FORM_LABEL);
                    pitem += FPSTR(HTTP_BR);
                    break;
                default:
                    // WFM_NO_LABEL
                    pitem = FPSTR(HTTP_FORM_PARAM);
                    break;
                }

                pitem.replace(FPSTR(T_i), params[i]->getID());     // T_i id name
                pitem.replace(FPSTR(T_n), params[i]->getID());     // T_n id name alias
                pitem.replace(FPSTR(T_t), params[i]->getLabel());  // T_t title/label
                snprintf(valLength, 5, "%d", params[i]->getValueLength());
                pitem.replace(FPSTR(T_l), valLength);              // T_l value length
                pitem.replace(FPSTR(T_v), params[i]->getValue());  // T_v value
                if(params[i]->getCustomHTML()) {                   // T_c additional attributes
                    pitem.replace(FPSTR(T_c), params[i]->getCustomHTML());
                } else {
                    pitem.replace(FPSTR(T_c), "");
                }

            } else if(params[i]->_customHTMLGenerator) {

                const char *t = (params[i]->_customHTMLGenerator)(NULL);
                if(t) {
                    pitem = t;
                    (params[i]->_customHTMLGenerator)(t);
                } else {
                    continue;
                }

            } else if(params[i]->getCustomHTML()) {

                pitem = params[i]->getCustomHTML();

            } else {

                continue;

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
            } else {
                #ifdef _A10001986_DBG
                Serial.printf("doSaveParms: doing parm %s\n", params[i]->getID());
                #endif
            }

            // read parameter from server
            String value = server->arg(params[i]->getID());

            // store it in params array; +1 for null termination
            value.toCharArray(params[i]->_value, params[i]->_length + 1);

            if(!(i % 30) && _gpcallback) {
                _gpcallback(WM_LP_NONE);
            }

        }

    }
}

void WiFiManager::HTTPSend(const String& content)
{
    #ifdef _A10001986_DBG
    unsigned long now = millis();
    #endif

    #ifdef _A10001986_DBG
    Serial.printf("HTTPSend: Heap before %d\n", ESP.getFreeHeap());
    #endif

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
int WiFiManager::getMenuOutLength()
{
    int bufSize = 0;

    if(_menuIdArr) {
        int menuId = 0;
        int8_t t;
        while((t = _menuIdArr[menuId++]) != WM_MENU_END) {
            if(t == WM_MENU_PARAM && !_paramsCount) continue;
            if(t == WM_MENU_CUSTOM && _customMenuHTML) {
                bufSize += strlen(_customMenuHTML);
                continue;
            }
            bufSize += strlen(HTTP_PORTAL_MENU[t]);
        }
    }

    if(_menuoutlencallback) {
        bufSize += _menuoutlencallback();
    }

    return bufSize;
}

// Construct root menu
void WiFiManager::getMenuOut(String& page)
{
    if(_menuIdArr) {
        int menuId = 0;
        int8_t t;
        while((t = _menuIdArr[menuId++]) != WM_MENU_END) {
            if(t == WM_MENU_PARAM && !_paramsCount) continue;
            if(t == WM_MENU_CUSTOM && _customMenuHTML) {
                page += _customMenuHTML;
                continue;
            }
            page += HTTP_PORTAL_MENU[t];
        }
    }

    if(_menuoutcallback) {
        _menuoutcallback(page);
    }
}

unsigned int WiFiManager::calcRootLen(unsigned int& headSize, unsigned int& repSize)
{
    // Calc page size
    unsigned int bufSize = getHTTPHeadLength(_title);

    headSize = STRLEN(HTTP_ROOT_MAIN) - (2*3);
    headSize += strlen(_title);
    if(configPortalActive) {
        headSize += strlen(_apName);
    } else {
        headSize += getWiFiHostname().length() + 3 + 15;
    }
    bufSize += headSize;
    bufSize += getMenuOutLength();
    repSize = reportStatusLen();
    bufSize += repSize;
    bufSize += STRLEN(HTTP_END);

    #ifdef _A10001986_DBG
    Serial.printf("calcRootLen: calced content size %d\n", bufSize);
    #endif

    return bufSize;
}

void WiFiManager::buildRootPage(String& page, unsigned int headSize, unsigned int repSize)
{
    // Build page
    String str;
    str.reserve(headSize);

    str = FPSTR(HTTP_ROOT_MAIN);
    str.replace(FPSTR(T_t), _title);
    str.replace(FPSTR(T_v), configPortalActive ? _apName : (getWiFiHostname() + " - " + WiFi.localIP().toString()));

    getHTTPHeadNew(page, _title);
    page += str;
    getMenuOut(page);
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

    #ifdef _A10001986_DBG
    Serial.println("<- HTTP Root");
    #endif

    String page;
    page.reserve(calcRootLen(headSize, repSize) + 16);

    buildRootPage(page, headSize, repSize);

    if(_gpcallback) {
        _gpcallback(WM_LP_PREHTTPSEND);
    }

    HTTPSend(page);

    if(_gpcallback) {
        _gpcallback(WM_LP_POSTHTTPSEND);
    }
}


/*--------------------------------------------------------------------------*/
/*************************** WIFI CONFIGURATION *****************************/
/*--------------------------------------------------------------------------*/


// Scan-complete Event callback
// Event Callback functions must be thread-safe; they must not access
// shared/global variables directly without locking, and must only call
// similarly thread-safe functions. Some core operations like
// Serial.print() are thread-safe but many functions are not.
// Notably, WiFi.onEvent() and WiFi.removeEvent() are not thread-safe and
// should never be invoked from a callback thread.

void WiFiManager::WiFi_scanComplete(int16_t networksFound)
{
    if((_numNetworksAsync = networksFound) >= 0) {
        _lastscan = millis();
    }

    #ifdef _A10001986_DBG
    Serial.printf("Callback: Async scan finished, %d networks\n", networksFound);
    #endif
}

int16_t WiFiManager::WiFi_waitForScan()
{
    int16_t res;
    unsigned long now;
    bool asyncdone = false;

    // Wait until scanComplete returns != RUNNING
    while((res = WiFi.scanComplete()) == WIFI_SCAN_RUNNING) {

        #ifdef _A10001986_DBG
        Serial.println(".");
        #endif

        _delay(250);
    }

    // Sometimes above returns an error (timeout?)
    // so we now wait for the scanComplete event
    now = millis();
    while(millis() - now < 5000) {
        if(_numNetworksAsync != WM_WIFI_SCAN_BUSY) {
            asyncdone = true;
            break;
        }
        _delay(500);
    }

    // If first loop fails, use event-callback result (if not error)
    if(res < 0 && asyncdone && _numNetworksAsync >= 0) {
        res = _numNetworksAsync;
        #ifdef _A10001986_DBG
        Serial.printf("Async wait failed, using event-callback result\n");
        #endif
    }

    if(res >= 0) _lastscan = millis();

    return res;
}

int16_t WiFiManager::WiFi_scanNetworks(bool force, bool async)
{
    if(!_numNetworks && _autoforcerescan) {
        #ifdef _A10001986_DBG
        Serial.println("No wetworks found, forcing new scan");
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

        if(async && _asyncScan) {

            #ifdef _A10001986_V_DBG
            Serial.println("WiFi Scan ASYNC started");
            #endif

            // Reset marker for event callback
            _numNetworksAsync = WM_WIFI_SCAN_BUSY;

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

            // <div><a href='#p' onclick='c(this)' data-ssid='{V}'>{v}</a>
            // <div role='img' aria-label='{r}%' title='{r}%' class='q q-{q} {i}'></div></div>

            int rssi = WiFi.RSSI(indices[i]);

            if(_minimumRSSI < rssi) {

                String SSID = WiFi.SSID(indices[i]);
                if(SSID == "") {
                    if(showall) {
                        SSID = S_hidden;
                    } else {
                        continue;
                    }
                } else if(!checkSSID(SSID)) {
                    if(showall) {
                        SSID = S_nonprintable;
                    } else {
                        continue;
                    }
                }

                itemSize = STRLEN(HTTP_WIFI_ITEM) - (6*3);
                itemSize += (2 * htmlEntitiesLen(SSID, true));
                itemSize += (4+4+1+1);     // rssi, rssi, qual class, enc class
                if(showall) itemSize += 6; // chnlnum

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

     if(scanErr) {

        page += FPSTR(HTTP_MSG_SCANFAIL);

    } else if(n == 0) {

        page += FPSTR(HTTP_MSG_NONETWORKS);

    } else {

        String item;
        if(maxItemSize) item.reserve(maxItemSize + 8);

        // <div><a href='#p' onclick='{t}(this)' data-ssid='{V}'>{v}</a>
        // <div role='img' aria-label='{r}dbm' title='{r}dbm' class='q q-{q} {i}'></div></div>

        for(int i = 0; i < n; i++) {
            if(indices[i] == -1) continue;

            int rssi = WiFi.RSSI(indices[i]);

            if(_minimumRSSI < rssi) {

                uint8_t enc_type = WiFi.encryptionType(indices[i]);
                String SSID = WiFi.SSID(indices[i]);
                String func = "c";

                if(SSID == "") {
                    if(showall) {
                        SSID = S_hidden;
                        func = "d";
                    } else {
                        continue;
                    }
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
                } else {
                    item.replace(FPSTR(T_c), "");
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
    // <input id='{i}' name='{n}' maxlength='{l}' value='{v}' {c}>

    unsigned int s = STRLEN(HTTP_FORM_LABEL) + STRLEN(HTTP_FORM_PARAM) - (7*3);
    s += (3*strlen(id)) + strlen(title);
    s += 2 + 15;
    if(placeholder) s += strlen(placeholder);

    String item;
    item.reserve(s + 8);

    item = FPSTR(HTTP_FORM_LABEL);
    item += FPSTR(HTTP_FORM_PARAM);
    item.replace(FPSTR(T_i), id);
    item.replace(FPSTR(T_n), id);
    item.replace(FPSTR(T_t), title);
    item.replace(FPSTR(T_l), F("15"));
    item.replace(FPSTR(T_v), value ? value.toString() : "");
    item.replace(FPSTR(T_c), placeholder ? placeholder : "");

    page += item;
}

unsigned int WiFiManager::getStaticLen()
{
    unsigned int mySize = 0;
    bool showSta = (_staShowStaticFields || _sta_static_ip);
    bool showDns = (_staShowDns || _sta_static_dns);

    // "<label for='{i}'>{t}</label>"
    // "<input id='{i}' name='{n}' maxlength='{l}' value='{v}' {c}>\n"

    if(showSta || showDns) {
        mySize += STRLEN(HTTP_FORM_SECT_HEAD);
    }
    if(showSta) {
        mySize += (3 * (STRLEN(HTTP_FORM_LABEL) + STRLEN(HTTP_FORM_PARAM) - (7*3)));
        mySize += (3*STRLEN(S_ip)) + STRLEN(S_staticip) + 2 + 15;
        mySize += STRLEN(HTTP_FORM_WIFI_PH);
        mySize += (3*STRLEN(S_sn)) + STRLEN(S_subnet) + 2 + 15;
        mySize += (3*STRLEN(S_gw)) + STRLEN(S_staticgw) + 2 + 15;
    }
    if(showDns) {
        mySize += STRLEN(HTTP_FORM_LABEL) + STRLEN(HTTP_FORM_PARAM) - (7*3);
        mySize += (3*STRLEN(S_dns)) + STRLEN(S_staticdns) + 2 + 15;
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
    bool scanErr = false, scanallowed = true, showrefresh = false, haveShowAll = false;
    bool force = server->hasArg(F("refresh"));
    bool showall = server->hasArg(F("showall"));
    int n = 0;

    String SSID = WiFi_SSID();
    unsigned int SSID_len = SSID.length();

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
    }

    maxDisplay = n;

    bufSize = getHTTPHeadLength(S_titlewifi, false, scan);
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
    bufSize += (STRLEN(HTTP_FORM_WIFI) - (3*3) + SSID_len + (2*STRLEN(S_passph)));
    if(SSID_len) {
        bufSize += STRLEN(HTTP_ERASE_BUTTON);
    }
    bufSize += STRLEN(HTTP_FORM_WIFI_END);
    bufSize += getStaticLen();
    bufSize += getParamOutSize(_wifiparams, _wifiparamsCount, maxItemSize);
    bufSize += STRLEN(HTTP_FORM_END);
    bufSize += STRLEN(HTTP_SCAN_LINK);
    if(haveShowAll) {
        bufSize += STRLEN(HTTP_SHOWALL_FORM);
    }
    repSize = reportStatusLen();
    bufSize += repSize;
    bufSize += STRLEN(HTTP_END);

    #ifdef _A10001986_DBG
    Serial.printf("handleWiFi: calced content size %d\n", bufSize);
    #endif

    page.reserve(bufSize + 16);

    getHTTPHeadNew(page, S_titlewifi, false, scan);
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
        pitem.reserve(STRLEN(HTTP_FORM_WIFI) - (3*3) + SSID_len + (2*STRLEN(S_passph)) + 8); // +8 safety

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
        page += pitem;
    }

    if(SSID_len) {
        page += FPSTR(HTTP_ERASE_BUTTON);
    }
    page += FPSTR(HTTP_FORM_WIFI_END);
    getStaticOut(page);
    getParamOut(page, _wifiparams, _wifiparamsCount, maxItemSize);
    page += FPSTR(HTTP_FORM_END);
    page += FPSTR(HTTP_SCAN_LINK);
    if(haveShowAll) {
        page += FPSTR(HTTP_SHOWALL_FORM);
    }
    reportStatus(page, repSize);
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

    HTTPSend(page);

    if(_gpcallback) {
        _gpcallback(WM_LP_POSTHTTPSEND);
    }
}

/*
 * HTTPD CALLBACK "Wifi Configuration"->SAVE page handler
 */
void WiFiManager::handleWifiSave()
{
    unsigned long s;
    bool haveNewSSID = false;
    bool networkDeleted = false;

    #ifdef _A10001986_V_DBG
    Serial.println("<- HTTP WiFi save ");
    #endif

    // grab new ssid/pass
    {
        String newSSID = server->arg(F("s"));
        String newPASS = server->arg(F("p"));
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
        if(!haveNewSSID && !pwchg) {
            if(forgetNW) {
                if(*_ssid) networkDeleted = true;
                *_ssid = *_pass = 0;
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

    // opportunity to steal the static ip data from the server
    // for saving
    if(_presavewificallback) {
        _presavewificallback();
    }

    // Copy server parms to wifi params array
    doParamSave(_wifiparams, _wifiparamsCount);

    // callback for saving
    if(_savewificallback) {
        _savewificallback(_ssid, _pass);
    }

    // Build page

    s = getHTTPHeadLength(S_titlewifi, true);
    s += STRLEN(HTTP_PARAMSAVED) + STRLEN(HTTP_PARAMSAVED_END) + STRLEN(HTTP_END);

    if(!haveNewSSID) {
        if(networkDeleted) s += STRLEN(HTTP_SAVED_ERASED);
    } else {
        if(_carMode) s += STRLEN(HTTP_SAVED_CARMODE);
        else         s += STRLEN(HTTP_SAVED_NORMAL);
    }

    String page;
    page.reserve(s + 16);
    getHTTPHeadNew(page, S_titlewifi, true);
    page += FPSTR(HTTP_PARAMSAVED);
    if(!haveNewSSID) {
        if(networkDeleted) page += FPSTR(HTTP_SAVED_ERASED);
    } else {
        if(_carMode) page += FPSTR(HTTP_SAVED_CARMODE);
        else         page += FPSTR(HTTP_SAVED_NORMAL);
    }
    page += FPSTR(HTTP_PARAMSAVED_END);
    page += FPSTR(HTTP_END);

    if(_gpcallback) {
        _gpcallback(WM_LP_PREHTTPSEND);
    }

    #ifdef _A10001986_DBG
    Serial.printf("handleWiFiSave: calced content size %d\n", s);
    #endif

    server->sendHeader(FPSTR(HTTP_HEAD_CORS), FPSTR(HTTP_HEAD_CORS_ALLOW_ALL));
    HTTPSend(page);

    if(_gpcallback) {
        _gpcallback(WM_LP_POSTHTTPSEND);
    }
}


/*--------------------------------------------------------------------------*/
/********************************* SETTINGS *********************************/
/*--------------------------------------------------------------------------*/

// Calc size of params ("Settings") page
int WiFiManager::calcParmPageSize(unsigned int& maxItemSize)
{
    int mySize = getHTTPHeadLength(S_titleparam);

    #ifdef _A10001986_DBG
    Serial.printf("calcParmSize: head %d\n", mySize);
    #endif

    mySize += (STRLEN(HTTP_FORM_START) - 3 + STRLEN(A_paramsave));

    mySize += getParamOutSize(_params, _paramsCount, maxItemSize);

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
void WiFiManager::handleParam()
{
    unsigned int maxItemSize = 0;

    #ifdef _A10001986_V_DBG
    Serial.println("<- HTTP Param");
    #endif

    String page;
    page.reserve(calcParmPageSize(maxItemSize) + 16);

    getHTTPHeadNew(page, S_titleparam);

    {
        String pitem;
        pitem.reserve(STRLEN(HTTP_FORM_START) - 3 + STRLEN(A_paramsave) + 2);
        pitem = FPSTR(HTTP_FORM_START);
        pitem.replace(FPSTR(T_v), FPSTR(A_paramsave));
        page += pitem;
    }

    getParamOut(page, _params, _paramsCount, maxItemSize);

    page += FPSTR(HTTP_FORM_END);
    page += FPSTR(HTTP_END);

    if(_gpcallback) {
        _gpcallback(WM_LP_PREHTTPSEND);
    }

    HTTPSend(page);

    if(_gpcallback) {
        _gpcallback(WM_LP_POSTHTTPSEND);
    }
}

/*
 * HTTPD CALLBACK Settings->SAVE page handler
 */
void WiFiManager::handleParamSave()
{
    unsigned int mySize = 0;
    unsigned int headSize = 0;

    #ifdef _A10001986_V_DBG
    Serial.println("<- HTTP Param save");
    #endif

    if(_presaveparamscallback) {
        _presaveparamscallback();
    }

    doParamSave(_params, _paramsCount);

    if(_saveparamscallback) {
        _saveparamscallback();
    }

    mySize = getHTTPHeadLength(S_titleparam, true);

    headSize = STRLEN(HTTP_ROOT_MAIN) - (2*3);
    headSize += strlen(_title);
    headSize += STRLEN(S_titleparam);
    mySize += headSize;

    mySize += STRLEN(HTTP_PARAMSAVED) + STRLEN(HTTP_PARAMSAVED_END);
    mySize += STRLEN(HTTP_END);

    String page;
    page.reserve(mySize + 16);

    #ifdef _A10001986_DBG
    Serial.printf("handleParamSave: calced content size %d\n", mySize);
    #endif

    getHTTPHeadNew(page, S_titleparam, true);

    {
        String str;
        str.reserve(headSize + 8);
        str = FPSTR(HTTP_ROOT_MAIN);
        str.replace(FPSTR(T_t), _title);
        str.replace(FPSTR(T_v), S_titleparam);
        page += str;
    }

    page += FPSTR(HTTP_PARAMSAVED);
    page += FPSTR(HTTP_PARAMSAVED_END);
    page += FPSTR(HTTP_END);

    if(_gpcallback) {
        _gpcallback(WM_LP_PREHTTPSEND);
    }

    HTTPSend(page);

    if(_gpcallback) {
        _gpcallback(WM_LP_POSTHTTPSEND);
    }
}

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

    mySize = getHTTPHeadLength(S_titleupd);

    headSize = STRLEN(HTTP_ROOT_MAIN) - (2*3);
    headSize += strlen(_title);
    headSize += STRLEN(S_titleupd);
    mySize += headSize;

    mySize += STRLEN(HTTP_UPDATE);
    if(_showUploadSnd) {
        mySize += STRLEN(HTTP_UPLOADSND1);
        mySize += strlen(_sndContName);
        mySize += STRLEN(HTTP_UPLOADSND2);
    } else if(_showContMsg) {
        mySize += STRLEN(HTTP_UPLOAD_SDMSG);
    }
    mySize += STRLEN(HTTP_END);

    #ifdef _A10001986_DBG
    Serial.printf("handleUpdate: calced content size %d\n", mySize);
    #endif

    String page;
    page.reserve(mySize + 16);

    getHTTPHeadNew(page, S_titleupd);

    {
        String str;
        str.reserve(headSize + 8);
        str = FPSTR(HTTP_ROOT_MAIN);
        str.replace(FPSTR(T_t), _title);
        str.replace(FPSTR(T_v), FPSTR(S_titleupd));
        page += str;
    }

    page += FPSTR(HTTP_UPDATE);
    if(_showUploadSnd) {
        page += FPSTR(HTTP_UPLOADSND1);
        page += _sndContName;
        page += FPSTR(HTTP_UPLOADSND2);
    } else if(_showContMsg) {
        page += FPSTR(HTTP_UPLOAD_SDMSG);
    }
    page += FPSTR(HTTP_END);

    if(_gpcallback) {
        _gpcallback(WM_LP_PREHTTPSEND);
    }

    HTTPSend(page);

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

        uint32_t maxSketchSpace;

        _uplError = false;

        // Use new callback for before OTA update
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

        reboot();
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
    bool res;

    #ifdef _A10001986_V_DBG
    Serial.println("<- Handle update done");
    #endif

    mySize = getHTTPHeadLength(S_titleupd, !Update.hasError());

    headSize = STRLEN(HTTP_ROOT_MAIN) - (2*3);
    headSize += strlen(_title);
    headSize += STRLEN(S_titleupd);
    mySize += headSize;

    if((res = !Update.hasError())) {
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

    getHTTPHeadNew(page, S_titleupd, !Update.hasError());

    {
        String str;
        str.reserve(headSize);
        str = FPSTR(HTTP_ROOT_MAIN);
        str.replace(FPSTR(T_t),_title);
        str.replace(FPSTR(T_v), FPSTR(S_titleupd));
        page += str;
    }

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

    HTTPSend(page);

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
    String message;
    message.reserve(STRLEN(S_notfound) + 16);

    message = FPSTR(S_notfound);
    message += F("\n");

    server->sendHeader(F("Cache-Control"), F("no-cache, no-store, must-revalidate"));
    server->sendHeader(F("Pragma"), F("no-cache"));
    server->sendHeader(F("Expires"), F("-1"));
    server->send(404, FPSTR(HTTP_HEAD_CT2), message);
}

/****************************************************************************
 *
 * Misc
 *
 ****************************************************************************/

// PUBLIC METHODS

// stopConfigPortal: Shut down softAP, disable WiFi
bool WiFiManager::stopConfigPortal()
{
    return shutdownConfigPortal();
}

// disconnect(): Disconnect STA
bool WiFiManager::disconnect()
{
    if(WiFi.status() != WL_CONNECTED) {
        #ifdef _A10001986_V_DBG
        Serial.println("Disconnecting: Not connected");
        #endif
        return false;
    }
    #ifdef _A10001986_DBG
    Serial.println("Disconnecting");
    #endif
    return WiFi.disconnect();
}

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

// toggle _cleanconnect, always disconnect before connecting
#ifdef WM_ADDLSETTERS
void WiFiManager::setCleanConnect(bool enable)
{
    _cleanConnect = enable;
}
#endif

// Set static IP for AP
#ifdef WM_AP_STATIC_IP
void WiFiManager::setAPStaticIPConfig(IPAddress ip, IPAddress gw, IPAddress sn)
{
    _ap_static_ip = ip;
    _ap_static_gw = gw;
    _ap_static_sn = sn;
}
#endif

// Set static IP for STA
void WiFiManager::setSTAStaticIPConfig(IPAddress ip, IPAddress gw, IPAddress sn)
{
    _sta_static_ip = ip;
    _sta_static_gw = gw;
    _sta_static_sn = sn;
}

// Set static IP + DNS for STA
void WiFiManager::setSTAStaticIPConfig(IPAddress ip, IPAddress gw, IPAddress sn, IPAddress dns)
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

// CALLBACK SETUP

// setAPCallback(): set a callback when softap is started
#ifdef WM_APCALLBACK
void WiFiManager::setAPCallback(void(*func)(WiFiManager*))
{
    _apcallback = func;
}
#endif

// setPreConnectCallback(): Set a callback when wifi hardware
// is initialized (eg after Wifi.mode()), but no connect
// attempt has been made.
#if WM_PRECONNECTCB
void WiFiManager::setPreConnectCallback(void(*func)())
{
    _preconnectcallback = func;
}
#endif

// setWiFiEventCallback(): Set a callback when a wifi event
// is received.
#ifdef WM_EVENTCB
void WiFiManager::setWiFiEventCallback(void(*func)(WiFiEvent_t event))
{
    _wifieventcallback = func;
}
#endif

// setWebServerCallback(): Set a callback after webserver is reset, and
// before routes are set up.
// on events cannot be overridden once set, and are not multiples
void WiFiManager::setWebServerCallback(void(*func)())
{
    _webservercallback = func;
}

// setSaveWiFiCallback(): Set a save config callback
void WiFiManager::setSaveWiFiCallback(void(*func)(const char *, const char *))
{
    _savewificallback = func;
}

// setPreSaveConfigCallback(): Set a callback to fire before saving wifi
void WiFiManager::setPreSaveWiFiCallback(void(*func)())
{
    _presavewificallback = func;
}

// setSaveParamsCallback(): Set a save params callback on params page
void WiFiManager::setSaveParamsCallback(void(*func)())
{
    _saveparamscallback = func;
}

// setPreSaveParamsCallback(): Set a pre save params callback on params
// save prior to anything else
void WiFiManager::setPreSaveParamsCallback(void(*func)())
{
    _presaveparamscallback = func;
}

// setxxxOtaUpdateCallback(): Set a callback to fire before OTA update
// or after, respectively.
void WiFiManager::setPreOtaUpdateCallback(void(*func)())
{
    _preotaupdatecallback = func;
}
void WiFiManager::setPostOtaUpdateCallback(void(*func)(bool))
{
    _postotaupdatecallback = func;
}

// setMenuOutLenCallback(): Set a callback give size of stuff added
// through setMenuOutCallback
void WiFiManager::setMenuOutLenCallback(int(*func)())
{
    _menuoutlencallback = func;
}

// setMenuOutCallback(): Set a callback to add html to root menu
void WiFiManager::setMenuOutCallback(void(*func)(String &page))
{
    _menuoutcallback = func;
}

// setPreWiFiScanCallback(): Set a callback before a wifi scan is started
// If the callback returns false, the scan is called off.
void WiFiManager::setPreWiFiScanCallback(bool(*func)())
{
    _prewifiscancallback = func;
}

// setDelayReplacement(): Set a delay() replacement function
void WiFiManager::setDelayReplacement(void(*func)(unsigned int))
{
    _delayreplacement = func;
}

// setGPCallback(): Set callback for when stuff takes a while
void WiFiManager::setGPCallback(void(*func)(int))
{
    _gpcallback = func;
}

// VISUAL CUSTOMIZATION

// setCustomHeadElement(): Set custom head html (added to <head>)
void WiFiManager::setCustomHeadElement(const char* html)
{
    _customHeadElement = html;
}

// setCustomMenuHTML(): Set custom menu html, added to menu under custom menu item.
void WiFiManager::setCustomMenuHTML(const char* html)
{
    _customMenuHTML = html;
}

// setShowStaticFields(): Toggle displaying static ip form fields
#ifdef WM_ADDLSETTERS
void WiFiManager::setShowStaticFields(bool doShow)
{
    _staShowStaticFields = doShow;
}
#endif

// setShowDnsFields(): Toggle displaying dns field
#ifdef WM_ADDLSETTERS
void WiFiManager::setShowDnsFields(bool doShow)
{
    _staShowDns = doShow;
}
#endif

// setTitle(): Set page title
void WiFiManager::setTitle(const char *title)
{
    memset(_title, 0, sizeof(_title));
    strncpy(_title, title, sizeof(_title) - 1);
}

// showUploadContainer(): Toggle displaying the sound-pack upload fields
void WiFiManager::showUploadContainer(bool enable, const char *contName, bool showMsg)
{
    if((_showUploadSnd = enable)) {
        memset(_sndContName, 0, sizeof(_sndContName));
        strncpy(_sndContName, contName, sizeof(_sndContName) - 1);
    } else {
        _showContMsg = showMsg;
    }
}

// setMenu(): Set main menu items and order
// eg.:  const int8_t menu[size] = {WM_MENU_WIFI, WM_MENU_PARAM, WM_MENU_END};
//       WiFiManager.setMenu(menu, size);
void WiFiManager::setMenu(const int8_t *menu, uint8_t size, bool doCopy)
{
    if(_menuIdArr) free((void *)_menuIdArr);

    _menuIdArr = NULL;

    if(!size) return;

    if(!doCopy) {
        _menuIdArr = (int8_t *)menu;
        return;
    }

    _menuIdArr = (int8_t *)malloc(size + 1);
    memset(_menuIdArr, WM_MENU_END, size + 1);

    for(int i = 0; i < size; i++) {
        if(menu[i] > WM_MENU_MAX) continue;
        if(menu[i] == WM_MENU_END) break;
        _menuIdArr[i] = menu[i];
    }
}

// setCarMode(): Tell WM whether or not we are in car mode
void WiFiManager::setCarMode(bool enable)
{
    _carMode = enable;
}

// NETWORK-RELATED

// setHostname(): Set the hostname (dhcp client id)
bool WiFiManager::setHostname(const char * hostname)
{
    memset(_hostname, 0, sizeof(_hostname));
    strncpy(_hostname, hostname, sizeof(_hostname) - 1);
    return true;
}

// setWiFiAPChannel(): Set the softAP channel
void WiFiManager::setWiFiAPChannel(int32_t channel)
{
    _apChannel = channel;
}

// setWiFiAPMaxClients(): Set softAP max client number
void WiFiManager::setWiFiAPMaxClients(int newmax)
{
    if(newmax > 10) newmax = 10;
    else if(newmax < 1) newmax = 1;
    _ap_max_clients = newmax;
}


// GETTERS

#ifdef WM_ADDLGETTERS
bool WiFiManager::getConfigPortalActive()
{
    return configPortalActive;
}
#endif

bool WiFiManager::getWebPortalActive()
{
    return webPortalActive;
}

String WiFiManager::getWiFiHostname()
{
    return (String)WiFi.getHostname();
}

uint8_t WiFiManager::getConnectRetries()
{
    return _connectRetries;
}

// getLastConxResult(): Return the last known connection result
#ifdef WM_ADDLGETTERS
uint8_t WiFiManager::getLastConxResult()
{
    return _lastconxresult;
}
#endif

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

void WiFiManager::reboot()
{
    ESP.restart();
}

bool WiFiManager::validApPassword()
{
    // check that ap password has valid length
    if(*_apPassword) {
        size_t t = strlen(_apPassword);
        if(t < 8 || t > 63) {
            *_apPassword = 0;
            return false;
        }
    }
    return true;
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

String WiFiManager::WiFi_SSID() const
{
    return String(_ssid);
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

void WiFiManager::WiFiEvent(WiFiEvent_t event, arduino_event_info_t info)
{
    if(!_hasBegun) {
        return;
    }

    if(event == ARDUINO_EVENT_WIFI_STA_DISCONNECTED) {

        // The core *should* reconnect automatically.
        // Unless: Wrong or missing password. Yes, this
        // is also reported through DISCONNECTED.

        _eventlastconxresult = WiFi.status();

        //WiFi.reconnect();   // No

    } else if(event == ARDUINO_EVENT_WIFI_SCAN_DONE && _asyncScan) {

        WiFi_scanComplete(WiFi.scanComplete());

    } else {

        if(event == ARDUINO_EVENT_WIFI_STA_GOT_IP ||
           event == ARDUINO_EVENT_WIFI_AP_START) {
            _gotip = true;
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
}

void WiFiManager::WiFi_installEventHandler()
{
    using namespace std::placeholders;
    if(wm_event_id == 0) {
        wm_event_id = WiFi.onEvent(std::bind(&WiFiManager::WiFiEvent, this, _1, _2));
    }
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
