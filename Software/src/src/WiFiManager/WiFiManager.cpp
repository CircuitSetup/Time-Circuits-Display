/**
 * WiFiManager.cpp
 *
 * Based on:
 *
 * WiFiManager, a library for the ESP32/Arduino platform
 *
 * @author Creator tzapu
 * @author tablatronix
 * @version 2.0.15+A10001986
 * @license MIT
 *
 * Adapted by Thomas Winischhofer (A10001986)
 * Relevant changes until Sep 17, 2025 backported
 */

#include "WiFiManager.h"
#include "wm_strings_en.h"

uint8_t WiFiManager::_lastconxresulttmp = WL_IDLE_STATUS;

#define STRLEN(x) (sizeof(x)-1)

/**
 * --------------------------------------------------------------------------------
 *  WiFiManagerParameter
 * --------------------------------------------------------------------------------
**/

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

/**
 * --------------------------------------------------------------------------------
 *  WiFiManager
 * --------------------------------------------------------------------------------
**/

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

// WiFi page params

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

WiFiManager::WiFiManager(Print& consolePort):_debugPort(consolePort)
{
    WiFiManagerInit();
}

WiFiManager::WiFiManager()
{
    WiFiManagerInit();
}

void WiFiManager::WiFiManagerInit()
{
    const int8_t defMenu[3] = { WM_MENU_WIFI, WM_MENU_SEP, WM_MENU_UPDATE };

    setMenu(defMenu, 3);

    _debugPrefix = FPSTR(S_debugPrefix);

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

// WiFI Connect

// The name "autoConnect" is somewhat misleading, it does not "auto"
// connect, it simply connects to the given ssid, and falls back to
// AP-mode in case the connection fails.

bool WiFiManager::autoConnect(const char *ssid, const char *pass, const char *apName, const char *apPassword)
{
    #ifdef WM_DEBUG_LEVEL
    DEBUG_WM(F("AutoConnect"));
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
        #ifdef WM_DEBUG_LEVEL
        DEBUG_WM(DEBUG_ERROR,F("[FATAL] Unable to enable wifi!"));
        #endif
        return false;
    }

    #ifndef _A10001986_NO_COUNTRY
    WiFiSetCountry();
    #endif

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

        #ifdef WM_DEBUG_LEVEL
        DEBUG_WM(F("AutoConnect: Already connected"));
        #endif

        connected = true;
        setStaticConfig();
    }

    if((!ssid || !*ssid) && !connected) {

        _lastconxresult = WL_IDLE_STATUS;

    } else if(connected || (connectWifi(ssid, pass) == WL_CONNECTED)) {

        #ifdef WM_DEBUG_LEVEL
        DEBUG_WM(F("AutoConnect: SUCCESS"));
        DEBUG_WM(F("STA IP Address:"),WiFi.localIP());
        if(*_hostname) {
            DEBUG_WM(DEBUG_DEV,F("hostname: "), getWiFiHostname());
        }
        #endif

        _lastconxresult = WL_CONNECTED;

        // init mDNS
        setupMDNS();

        return true; // connected success
    }

    // Not connected:

    #ifdef WM_DEBUG_LEVEL
    DEBUG_WM(F("AutoConnect: Not connected"));
    #endif

    // Start softAP and config portal

    startConfigPortal(apName, apPassword, ssid, pass);

    return false; // Yes, false; true means "connected".
}

// AP-mode / Config Portal

bool WiFiManager::startAP()
{
    int32_t channel = _apChannel;
    bool ret = true;

    #ifdef WM_DEBUG_LEVEL
    DEBUG_WM(F("StartAP with SSID: "), _apName);
    #endif

    // setup optional soft AP static ip config
    #ifdef WM_AP_STATIC_IP
    if(_ap_static_ip) {

        if(!WiFi.softAPConfig(_ap_static_ip, _ap_static_gw, _ap_static_sn)) {

            #ifdef WM_DEBUG_LEVEL
            DEBUG_WM(DEBUG_ERROR,F("[ERROR] softAPConfig failed!"));
            #endif

        }
    }
    #endif

    #ifdef WM_DEBUG_LEVEL
    if(channel > 0) {
        DEBUG_WM(DEBUG_VERBOSE,F("Starting AP on channel:"), channel);
    }
    #endif

    // start soft AP; default channel is 1 here and in esp library
    ret = WiFi.softAP(_apName,
            (strlen(_apPassword) > 0) ? _apPassword : NULL,
            (channel > 0) ? channel : 1,
            false,
            _ap_max_clients);

    #ifdef WM_DODEBUG
    if(_debugLevel >= DEBUG_DEV) debugSoftAPConfig();
    #endif

    _delay(500); // slight delay to make sure we get an AP IP

    #ifdef WM_DEBUG_LEVEL
    if(!ret) DEBUG_WM(DEBUG_ERROR,F("[ERROR] There was a problem starting the AP"));
    DEBUG_WM(F("AP IP address:"), WiFi.softAPIP());
    #endif

    // set ap hostname (seems ok to do this after softAP())
    if(ret && *_hostname) {
        WiFi.softAPsetHostname(_hostname);
    }

    return ret;
}

// Start/stop web portal (= config portal in STA mode)

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

    #ifdef WM_DEBUG_LEVEL
    DEBUG_WM(DEBUG_VERBOSE,F("Stopping Web Portal"));
    #endif

    webPortalActive = false;
    shutdownConfigPortal();
}

// Setup server functions

void WiFiManager::setupHTTPServer()
{
    #ifdef WM_DEBUG_LEVEL
    DEBUG_WM(F("Starting Web Portal"));
    #endif

    server.reset(new WebServer(_httpPort));
    // This is not the safest way to reset the webserver, it can cause crashes
    // on callbacks initialized before this and since its a shared pointer...

    if(_webservercallback) {
        _webservercallback();
    }

    // Setup httpd callbacks

    // G macro workaround for Uri() bug https://github.com/esp8266/Arduino/issues/7102
    server->on(WM_G(R_root),       std::bind(&WiFiManager::handleRoot, this));
    server->on(WM_G(R_wifi),       std::bind(&WiFiManager::handleWifi, this, true));
    server->on(WM_G(R_wifinoscan), std::bind(&WiFiManager::handleWifi, this, false));
    server->on(WM_G(R_wifisave),   std::bind(&WiFiManager::handleWifiSave, this));
    server->on(WM_G(R_param),      std::bind(&WiFiManager::handleParam, this));
    server->on(WM_G(R_paramsave),  std::bind(&WiFiManager::handleParamSave, this));
    server->on(WM_G(R_update),     std::bind(&WiFiManager::handleUpdate, this));
    server->on(WM_G(R_updatedone), HTTP_POST, std::bind(&WiFiManager::handleUpdateDone, this), std::bind(&WiFiManager::handleUpdating, this));
    server->on(WM_G(R_erase),      std::bind(&WiFiManager::handleErase, this));

    server->onNotFound(std::bind(&WiFiManager::handleNotFound, this));

    // Web server start

    server->begin();

    #ifdef WM_DEBUG_LEVEL
    DEBUG_WM(DEBUG_VERBOSE,F("HTTP server started"));
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

// startConfigPortal: Start the softAP and the web portal.
// (Config Portal in AP mode = Web Portal in STA mode)

bool WiFiManager::startConfigPortal(char const *apName, char const *apPassword, const char *ssid, const char *pass)
{
    _begin();

    if(configPortalActive) {
        #ifdef WM_DEBUG_LEVEL
        DEBUG_WM(DEBUG_VERBOSE,F("Config Portal is already running"));
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

    #ifdef WM_DEBUG_LEVEL
    DEBUG_WM(DEBUG_VERBOSE,F("Starting Config Portal"));
    #endif

    if(!*_apName) getDefaultAPName(_apName);

    if(!validApPassword()) return false;

    // Shutdown sta if not connected, or else this will hang
    // channel scanning, and softap will not respond
    if(_disableSTA || (!WiFi.isConnected() && _disableSTAConn)) {

        #ifdef WM_DISCONWORKAROUND    // see WiFiManager.h
        WiFi.mode(WIFI_AP_STA);
        #endif

        WiFi_Disconnect();

        WiFi_enableSTA(false);

        #ifdef WM_DEBUG_LEVEL
        DEBUG_WM(DEBUG_VERBOSE,F("Disabling STA"));
        #endif

    }

    configPortalActive = true;

    // start access point
    #ifdef WM_DEBUG_LEVEL
    DEBUG_WM(DEBUG_VERBOSE,F("Enabling AP"));
    #endif

    startAP();

    #ifndef _A10001986_NO_COUNTRY
    WiFiSetCountry();
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

    #ifdef WM_DEBUG_LEVEL
    DEBUG_WM(DEBUG_VERBOSE,F("Config Portal Running"));
    #endif

    // Install WiFi event handler
    WiFi_installEventHandler();

    return true;
}

/**
 * [process description]
 * @access public
 * @return bool connected
 */
void WiFiManager::process(bool handleWeb)
{
    if(webPortalActive || configPortalActive) {
        processConfigPortal(handleWeb);
    }
}

/**
 * [processConfigPortal description]
 *
 * @return {[type]} [description]
 */
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

/**
 * [shutdownConfigPortal description]
 * @access public
 * @return bool success (softapdisconnect)
 */
bool WiFiManager::shutdownConfigPortal()
{
    #ifdef WM_DEBUG_LEVEL
    DEBUG_WM(DEBUG_VERBOSE,F("shutdownConfigPortal"));
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

    DEBUG_WM(DEBUG_VERBOSE,F("configportal closed"));

    _end();
    return ret;
}

// @todo refactor this up into seperate functions
// one for connecting to flash , one for new client
// clean up, flow is convoluted, and causes bugs
uint8_t WiFiManager::connectWifi(const char *ssid, const char *pass)
{
    #ifdef WM_DEBUG_LEVEL
    DEBUG_WM(DEBUG_VERBOSE,F("Connecting as wifi client..."));
    #endif

    uint8_t retry = 1;
    uint8_t connRes = (uint8_t)WL_NO_SSID_AVAIL;

    // set Static IP if provided
    setStaticConfig();

    // disconnect before begin, in case anything is hung, this causes a 2 seconds delay for connect
    // @todo find out what status is when this is needed, can we detect it
    // and handle it, say in between states or idle_status to avoid these

    if(_cleanConnect) WiFi_Disconnect();

    // if retry without delay (via begin()), the IDF is still busy even after returning status
    // E (5130) wifi:sta is connecting, return error
    // [E][WiFiSTA.cpp:221] begin(): connect failed!

    while(retry <= _connectRetries && (connRes != WL_CONNECTED)) {

        if(_connectRetries > 1) {
            if(_aggresiveReconn) _delay(1000); // add idle time before recon

            #ifdef WM_DEBUG_LEVEL
            DEBUG_WM(F("Connect Wifi, ATTEMPT #"),(String)retry+" of "+(String)_connectRetries);
            #endif
        }

        wifiConnectNew(ssid, pass);

        connRes = waitForConnectResult();

        #ifdef WM_DEBUG_LEVEL
        DEBUG_WM(DEBUG_VERBOSE,F("Connection result:"), getWLStatusString(connRes));
        #endif

        retry++;
    }

    if(connRes != WL_SCAN_COMPLETED) {
        _lastconxresult = connRes;
        if(_lastconxresult == WL_CONNECT_FAILED || _lastconxresult == WL_DISCONNECTED) {
            if(_lastconxresulttmp != WL_IDLE_STATUS) {
                _lastconxresult    = _lastconxresulttmp;
                // _lastconxresulttmp = WL_IDLE_STATUS;
            }
        }
    }

    return connRes;
}

/**
 * connect to a given wifi ap
 * @since $dev
 * @param  String ssid
 * @param  String pass
 * @return bool success
 * @return result from WiFi.begin()
 */
bool WiFiManager::wifiConnectNew(const char *ssid, const char *pass)
{
    bool ret = false;

    #ifdef WM_DEBUG_LEVEL
    DEBUG_WM(F("Connecting to WiFI network:"),ssid);
    DEBUG_WM(DEBUG_DEV,F("Using Password:"),pass);
    #endif

    ret = WiFi.begin(ssid, pass, 0, NULL, true);

    #ifdef WM_DEBUG_LEVEL
    if(!ret) DEBUG_WM(DEBUG_ERROR,F("[ERROR] wifi begin failed"));
    #endif

    return ret;
}

/**
 * set static IP config if configured
 * @since $dev
 * @return bool success
 */
bool WiFiManager::setStaticConfig()
{
    #ifdef WM_DEBUG_LEVEL
    DEBUG_WM(DEBUG_DEV,F("STA static IP:"), _sta_static_ip);
    #endif

    bool ret = true;

    if(_sta_static_ip) {
        #ifdef WM_DEBUG_LEVEL
        DEBUG_WM(DEBUG_VERBOSE,F("Custom static IP/GW/Subnet/DNS"));
        #endif
        if(_sta_static_dns) {
            ret = WiFi.config(_sta_static_ip, _sta_static_gw, _sta_static_sn, _sta_static_dns);
        } else {
            ret = WiFi.config(_sta_static_ip, _sta_static_gw, _sta_static_sn);
        }

        #ifdef WM_DEBUG_LEVEL
        if(!ret) DEBUG_WM(DEBUG_ERROR,F("[ERROR] wifi config failed"));
        else DEBUG_WM(F("STA IP set:"), WiFi.localIP());
        #endif
    }

    return ret;
}

uint8_t WiFiManager::waitForConnectResult()
{
    return waitForConnectResult(_connectTimeout);
}

/**
 * waitForConnectResult
 * @param  uint16_t timeout  in seconds
 * @return uint8_t  WL Status
 */
uint8_t WiFiManager::waitForConnectResult(uint32_t timeout)
{
    if(!timeout) {
        #ifdef WM_DEBUG_LEVEL
        DEBUG_WM(F("connectTimeout not set, waitForConnectResult..."));
        #endif

        return WiFi.waitForConnectResult();
    }

    unsigned long timeoutmillis = millis() + timeout;

    #ifdef WM_DEBUG_LEVEL
    DEBUG_WM(DEBUG_VERBOSE,timeout,F("ms timeout, waiting for connect..."));
    #endif

    uint8_t status = WiFi.status();

    while(millis() < timeoutmillis) {

        status = WiFi.status();
        // @todo detect additional states, connect happens, then dhcp then get ip,
        // there is some delay here, make sure not to timeout if waiting on IP
        if(status == WL_CONNECTED || status == WL_CONNECT_FAILED) {
            return status;
        }

        #ifdef WM_DEBUG_LEVEL
        DEBUG_WM (DEBUG_VERBOSE,F("."));
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
unsigned int WiFiManager::getHTTPHeadLength(const char *title, bool includeQI)
{
    int bufSize = STRLEN(HTTP_HEAD_START) - 3;  // -3 token
    bufSize += strlen(title);
    bufSize += STRLEN(HTTP_SCRIPT);
    bufSize += STRLEN(HTTP_STYLE);
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
void WiFiManager::getHTTPHeadNew(String& page, const char *title, bool includeQI)
{
    String temp;
    temp.reserve(STRLEN(HTTP_HEAD_START) + strlen(title));

    temp = FPSTR(HTTP_HEAD_START);
    temp.replace(FPSTR(T_v), title);
    page += temp;

    page += FPSTR(HTTP_SCRIPT);
    page += FPSTR(HTTP_STYLE);
    if(includeQI) {
        page += FPSTR(HTTP_STYLE_QI);
    }
    if(_customHeadElement) {
        page += _customHeadElement;
    }
    page += FPSTR(HTTP_HEAD_END);
}

// WIFI STATUS at bottom of pages

int WiFiManager::reportStatusLen()
{
    int bufSize = 0;
    String SSID = WiFi_SSID();

    if(SSID != "") {
        if(WiFi.status() == WL_CONNECTED) {
            bufSize = STRLEN(HTTP_STATUS_ON) - (2*3);
            bufSize += 15;        // max len of ip address
            bufSize += htmlEntitiesLen(SSID);
        } else {
            bufSize = STRLEN(HTTP_STATUS_OFF);
            bufSize += htmlEntitiesLen(SSID);
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
            default:
                bufSize += _carMode ? STRLEN(HTTP_STATUS_CARMODE) : STRLEN(HTTP_STATUS_APMODE);
                break;
            }
        }
    } else {
        bufSize = strlen(HTTP_STATUS_NONE);
    }

    #ifdef _A10001986_DBG
    Serial.printf("WM: reportStatusLen bufSize %d\n", bufSize);
    #endif

    return bufSize + 16;
}

void WiFiManager::reportStatus(String &page, unsigned int estSize)
{
    String SSID = WiFi_SSID();
    String str;
    if(estSize) str.reserve(estSize);

    if(SSID != "") {
        if(WiFi.status() == WL_CONNECTED) {
            str = FPSTR(HTTP_STATUS_ON);
            str.replace(FPSTR(T_i), WiFi.localIP().toString());
            str.replace(FPSTR(T_v), htmlEntities(SSID));
        } else {
            str = FPSTR(HTTP_STATUS_OFF);
            str.replace(FPSTR(T_v), htmlEntities(SSID));
            switch(_lastconxresult) {
            case WL_NO_SSID_AVAIL:          // connect failed, or ap not found
                str.replace(FPSTR(T_c), "D");
                str.replace(FPSTR(T_r), FPSTR(HTTP_STATUS_OFFNOAP));
                str.replace(FPSTR(T_V), FPSTR(HTTP_STATUS_APMODE));
                break;
            case WL_CONNECT_FAILED:         // connect failed
                str.replace(FPSTR(T_c), "D");
                str.replace(FPSTR(T_r), FPSTR(HTTP_STATUS_OFFFAIL));
                str.replace(FPSTR(T_V), FPSTR(HTTP_STATUS_APMODE));
                break;
            case WL_CONNECTION_LOST:        // connect failed, MOST likely 4WAY_HANDSHAKE_TIMEOUT/incorrect
                str.replace(FPSTR(T_c), "D"); // password, state is ambiguous however
                str.replace(FPSTR(T_r), FPSTR(HTTP_STATUS_OFFFAIL));
                str.replace(FPSTR(T_V), FPSTR(HTTP_STATUS_APMODE));
                break;
            default:
                str.replace(FPSTR(T_c), "");
                str.replace(FPSTR(T_r), "");
                str.replace(FPSTR(T_V), _carMode ? FPSTR(HTTP_STATUS_CARMODE) : FPSTR(HTTP_STATUS_APMODE));
                break;
            }
        }
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
                #ifdef WM_DEBUG_LEVEL
                DEBUG_WM(DEBUG_ERROR,F("[ERROR getParamOutSize] WiFiManagerParameter is out of scope"));
                #endif
                continue;
            }

            mysize = 0;

            if(params[i]->getID() != NULL) {
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
                    break;
                }

                // <label for='{i}'>{t}</label>
                // <input id='{i}' name='{n}' maxlength='{l}' value='{v}' {c}>\n";
                mysize += (strlen(params[i]->getID()) * 3);    // 2xi, 1xn
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

void WiFiManager::getParamOut(WiFiManagerParameter** params,
                    int paramsCount, String &page, unsigned int maxItemSize)
{
    if(paramsCount > 0) {

        char valLength[5];

        // Allocate it once, re-use it
        String pitem;
        pitem.reserve(maxItemSize ? maxItemSize : 3072);

        // add the extra parameters to the form
        for(int i = 0; i < paramsCount; i++) {

            // Just see if any of our params has been destructed in the meantime
            if(!params[i] || params[i]->_length > 99999) {
                #ifdef WM_DEBUG_LEVEL
                DEBUG_WM(DEBUG_ERROR,F("[ERROR] WiFiManagerParameter is out of scope"));
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
                pitem.replace(FPSTR(T_l), valLength);               // T_l value length
                pitem.replace(FPSTR(T_v), params[i]->getValue());  // T_v value
                if(params[i]->getCustomHTML()) {                   // T_c additional attributes
                    pitem.replace(FPSTR(T_c), params[i]->getCustomHTML());
                }

            } else if(params[i]->_customHTMLGenerator) {

                const char *t = (params[i]->_customHTMLGenerator)(NULL);
                if(t) {
                    pitem = String(t);
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

            if(!(i % 30)) {
                if(_gpcallback) {
                    _gpcallback(WM_LP_NONE);
                }
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
                #ifdef WM_DEBUG_LEVEL
                DEBUG_WM(DEBUG_ERROR,F("[ERROR] WiFiManagerParameter is out of scope"));
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

            if(!(i % 30)) {
                if(_gpcallback) {
                    _gpcallback(WM_LP_NONE);
                }
            }

        }

    }
}

void WiFiManager::HTTPSend(const String &content)
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
    unsigned int bufSize = getHTTPHeadLength(_title, false);

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

    #ifdef WM_DEBUG_LEVEL
    DEBUG_WM(DEBUG_VERBOSE,F("<- HTTP Root"));
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
/******************************* WIFI CONFIG ********************************/
/*--------------------------------------------------------------------------*/


// Event callback
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

        #ifdef WM_DEBUG_LEVEL
        DEBUG_WM(DEBUG_ERROR,".");
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
        #ifdef WM_DEBUG_LEVEL
        DEBUG_WM(DEBUG_DEV,"NO APs found forcing new scan");
        #endif
        force = true;
    }

    // Use cache if called again within _scancachetime
    if(!_lastscan || (millis() - _lastscan > _scancachetime)) {
        force = true;
    }

    if(force) {

        int16_t res;

        _numNetworks = 0;
        #ifdef WM_DEBUG_LEVEL
        _startscan = millis();
        #endif

        if(async && _asyncScan) {

            #ifdef WM_DEBUG_LEVEL
            DEBUG_WM(DEBUG_VERBOSE,F("WiFi Scan ASYNC started"));
            #endif

            // Reset marker for event callback
            _numNetworksAsync = WM_WIFI_SCAN_BUSY;

            // Start scan
            return WiFi.scanNetworks(true);

        }

        #ifdef WM_DEBUG_LEVEL
        DEBUG_WM(DEBUG_VERBOSE,F("WiFi Scan SYNC started"));
        #endif

        res = WiFi.scanNetworks();

        if(res == WIFI_SCAN_FAILED) {

            #ifdef WM_DEBUG_LEVEL
            DEBUG_WM(DEBUG_ERROR,F("[ERROR] scan failed"));
            #endif

        } else if(res >= 0) {

            _numNetworks = res;
            _lastscan = millis();

        }

        #ifdef WM_DEBUG_LEVEL
        DEBUG_WM(DEBUG_VERBOSE,F("WiFi Scan completed"), "in "+(String)(_lastscan - _startscan)+" ms");
        #endif

        return res;

    }

    return 0;
}

void WiFiManager::sortNetworks(int n, int *indices)
{
    if(n == 0) {

        #ifdef WM_DEBUG_LEVEL
        DEBUG_WM(F("No networks found"));
        #endif

    } else {

        #ifdef WM_DEBUG_LEVEL
        DEBUG_WM(n,F("networks found"));
        #endif

        // sort networks
        for(int i = 0; i < n; i++) {
            indices[i] = i;
        }

        // RSSI SORT
        for(int i = 0; i < n; i++) {
            for(int j = i + 1; j < n; j++) {
                if(WiFi.RSSI(indices[j]) > WiFi.RSSI(indices[i])) {
                    std::swap(indices[i], indices[j]);
                }
            }
        }

        // remove duplicates (must be RSSI sorted to remove the weaker one here)
        if(_removeDuplicateAPs) {
            String cssid;
            for(int i = 0; i < n; i++) {
                if(indices[i] == -1) continue;
                cssid = WiFi.SSID(indices[i]);
                for(int j = i + 1; j < n; j++) {
                    if(cssid == WiFi.SSID(indices[j])) {
                        indices[j] = -1; // set dup aps to index -1
                    }
                }
            }
        }
    }
}

unsigned int WiFiManager::getScanItemsLen(int n, bool scanErr, int *indices, unsigned int& maxItemSize)
{
    unsigned int mySize = 0;

    if(scanErr) {

        mySize += STRLEN(HTTP_MSG_SCANFAIL);
        maxItemSize = mySize;

    } else if(n == 0) {

        mySize += STRLEN(HTTP_MSG_NONETWORKS);
        maxItemSize = mySize;

    } else {

        maxItemSize = 0;

        for(int i = 0; i < n; i++) {
            if(indices[i] == -1) continue; // skip dups

            // <div><a href='#p' onclick='c(this)' data-ssid='{V}'>{v}</a>
            // <div role='img' aria-label='{r}%' title='{r}%' class='q q-{q} {i}'></div></div>

            int rssiperc = getRSSIasQuality(WiFi.RSSI(indices[i]));

            if(_minimumQuality == -1 || _minimumQuality < rssiperc) {

                String SSID = WiFi.SSID(indices[i]);
                if(SSID == "") continue;

                mySize += STRLEN(HTTP_WIFI_ITEM) - (6*3);
                mySize += htmlEntitiesLen(SSID);
                mySize += htmlEntitiesLen(SSID, true);
                mySize += ((2*3)+1+1); // 2xrssi perc, mapped percentage, enc class

                if(mySize > maxItemSize) maxItemSize = mySize;

            } else {

                #ifdef _A10001986_DBG
                Serial.printf("WM: skipping %s, qual %d\n", WiFi.SSID(indices[i]).c_str(), rssiperc);
                #endif

            }
        }
    }

    #ifdef _A10001986_DBG
    Serial.printf("WM: getScanItemsLen %d, maxSize %d\n", mySize, maxItemSize);
    #endif

    return mySize;
}

String WiFiManager::getScanItemsOut(int n, bool scanErr, int *indices, unsigned int maxItemSize)
{
    String page;

     if(scanErr) {

        page += FPSTR(HTTP_MSG_SCANFAIL);

    } else if(n == 0) {

        page += FPSTR(HTTP_MSG_NONETWORKS);

    } else {

        String item;
        if(maxItemSize) item.reserve(maxItemSize);

        // <div><a href='#p' onclick='c(this)' data-ssid='{V}'>{v}</a>
        // <div role='img' aria-label='{r}%' title='{r}%' class='q q-{q} {i}'></div></div>

        // display networks in page
        for(int i = 0; i < n; i++) {
            if(indices[i] == -1) continue; // skip dups

            #ifdef WM_DEBUG_LEVEL
            DEBUG_WM(DEBUG_VERBOSE,F("AP: "),(String)WiFi.RSSI(indices[i]) + " " + (String)WiFi.SSID(indices[i]));
            #endif

            int rssiperc = getRSSIasQuality(WiFi.RSSI(indices[i]));

            if(_minimumQuality == -1 || _minimumQuality < rssiperc) {

                uint8_t enc_type = WiFi.encryptionType(indices[i]);
                String SSID = WiFi.SSID(indices[i]);

                if(SSID == "") continue;

                item = FPSTR(HTTP_WIFI_ITEM);

                item.replace(FPSTR(T_V), htmlEntities(SSID)); // ssid no encoding
                item.replace(FPSTR(T_v), htmlEntities(SSID, true)); // ssid no encoding
                item.replace(FPSTR(T_r), (String)rssiperc); // rssi percentage 0-100
                item.replace(FPSTR(T_q), (String)int(round(map(rssiperc,0,100,1,4)))); //quality icon 1-4
                item.replace(FPSTR(T_i), (enc_type != WM_WIFIOPEN) ? "l" : "");

                #ifdef WM_DEBUG_LEVEL
                DEBUG_WM(DEBUG_DEV,item);
                #endif

                page += item;
                delay(0);
            }

            if(!(i % 20)) {
                if(_gpcallback) {
                    _gpcallback(WM_LP_NONE);
                }
            }
        }
    }

    return page;
}

// static ip fields

String WiFiManager::getIpForm(const char *id, const char *title, IPAddress& value, const char *placeholder)
{
    // <label for='{i}'>{t}</label>
    // <input id='{i}' name='{n}' maxlength='{l}' value='{v}' {c}>\n
    String item;
    item.reserve(STRLEN(HTTP_FORM_LABEL) + STRLEN(HTTP_FORM_PARAM) +
                 (3*strlen(id)) + strlen(title) + 15 + 16);

    item = FPSTR(HTTP_FORM_LABEL);
    item += FPSTR(HTTP_FORM_PARAM);
    item.replace(FPSTR(T_i), id);
    item.replace(FPSTR(T_n), id);
    item.replace(FPSTR(T_t), title);
    item.replace(FPSTR(T_l), F("15"));
    item.replace(FPSTR(T_v), value ? value.toString() : "");
    item.replace(FPSTR(T_c), placeholder ? placeholder : "");
    return item;
}

unsigned int WiFiManager::getStaticLen()
{
    unsigned int mySize = 0;
    bool showSta = (_staShowStaticFields || _sta_static_ip);
    bool showDns = (_staShowDns || _sta_static_dns);

    // "<label for='{i}'>{t}</label>"
    // "<input id='{i}' name='{n}' maxlength='{l}' value='{v}' {c}>\n"

    if(showSta || showDns) {
        mySize += STRLEN(HTTP_FORM_SECTION_HEAD);
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
        mySize += STRLEN(HTTP_FORM_SECTION_FOOT);
    }

    return mySize;
}

String WiFiManager::getStaticOut(unsigned int estPageSize)
{
    bool showSta = (_staShowStaticFields || _sta_static_ip);
    bool showDns = (_staShowDns || _sta_static_dns);

    String page;
    if(estPageSize) {
        page.reserve(estPageSize);
    }

    if(showSta || showDns) {
        page += FPSTR(HTTP_FORM_SECTION_HEAD);
    }
    if(showSta) {
        page += getIpForm(S_ip, S_staticip, _sta_static_ip, HTTP_FORM_WIFI_PH);
        page += getIpForm(S_sn, S_subnet, _sta_static_sn);
        page += getIpForm(S_gw, S_staticgw, _sta_static_gw);
    }
    if(showDns) {
        page += getIpForm(S_dns, S_staticdns, _sta_static_dns);
    }
    if(showSta || showDns) {
        page += FPSTR(HTTP_FORM_SECTION_FOOT);
    }

    return page;
}

// Build WiFi page

void WiFiManager::buildWifiPage(bool scan, String& page)
{
    unsigned int bufSize = 0, statBufSize = 0, repSize = 0;
    unsigned int maxScanItemSize = 0;
    unsigned int maxItemSize = 0;
    bool scanErr = false, scanallowed = true, showrefresh = false;
    bool force = server->hasArg(F("refresh"));
    int n = 0;

    String SSID = WiFi_SSID();
    unsigned int SSID_len = SSID.length();

    // No scan (unless forced) if we have a configured network
    if(scan && !force && SSID != "") {
        scan = false;
        showrefresh = true;
    }

    // PreScanCallback can cancel scan, eg if
    // device is busy and must not be interrupted.
    // We use the cache then, if still valid.
    if(scan && _prewifiscancallback) {
        if(!(scanallowed = _prewifiscancallback())) {
            if(!_autoforcerescan && _lastscan && (millis() - _lastscan < _scancachetime - 20)) {
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
        sortNetworks(n, indices);
    }

    bufSize = getHTTPHeadLength(S_titlewifi, scan);
    if(showrefresh) {
        // No message
    } else if(!scanallowed) {
        bufSize += STRLEN(HTTP_MSG_NOSCAN);
    } else if(scan) {
        bufSize += getScanItemsLen(n, scanErr, indices, maxScanItemSize);
    }
    bufSize += (STRLEN(HTTP_FORM_START) - 3 + STRLEN(A_wifisave));
    bufSize += (STRLEN(HTTP_FORM_WIFI) - (3*3) + SSID_len + (2*STRLEN(S_passph)));
    if(SSID_len) {
        bufSize += STRLEN(HTTP_ERASE_BUTTON);
    }
    bufSize += STRLEN(HTTP_FORM_WIFI_END);
    statBufSize = getStaticLen();
    bufSize += statBufSize;
    bufSize += getParamOutSize(_wifiparams, _wifiparamsCount, maxItemSize);
    bufSize += STRLEN(HTTP_FORM_END);
    bufSize += STRLEN(HTTP_SCAN_LINK);
    if(SSID_len) {
        bufSize += STRLEN(HTTP_ERASE_FORM);
    }
    if(_showBack) bufSize += STRLEN(HTTP_BACKBTN);
    repSize = reportStatusLen();
    bufSize += repSize;
    bufSize += STRLEN(HTTP_END);

    #ifdef _A10001986_DBG
    Serial.printf("handleWiFi: calced content size %d; statSize %d\n", bufSize, statBufSize);
    #endif

    page.reserve(bufSize + 16);

    getHTTPHeadNew(page, S_titlewifi, scan);
    if(showrefresh) {
        // No message
    } else if(!scanallowed) {
        page += FPSTR(HTTP_MSG_NOSCAN);
    } else if(scan) {
        page += getScanItemsOut(n, scanErr, indices, maxScanItemSize);
    }

    String pitem;
    pitem.reserve(STRLEN(HTTP_FORM_WIFI) - (3*3) + SSID_len + (2*STRLEN(S_passph)) + 8); // +8 safety

    pitem = FPSTR(HTTP_FORM_START);
    pitem.replace(FPSTR(T_v), FPSTR(A_wifisave));
    page += pitem;

    pitem = FPSTR(HTTP_FORM_WIFI);
    pitem.replace(FPSTR(T_V), SSID);
    if(WiFi_psk_len()) {
        pitem.replace(FPSTR(T_p),FPSTR(S_passph));  // twice!
    } else {
        pitem.replace(FPSTR(T_p),"");
    }
    page += pitem;
    if(SSID_len) {
        page += FPSTR(HTTP_ERASE_BUTTON);
    }
    page += FPSTR(HTTP_FORM_WIFI_END);
    page += getStaticOut(statBufSize);
    getParamOut(_wifiparams, _wifiparamsCount, page, maxItemSize);
    page += FPSTR(HTTP_FORM_END);
    page += FPSTR(HTTP_SCAN_LINK);
    if(SSID_len) {
        page += FPSTR(HTTP_ERASE_FORM);
    }
    if(_showBack) page += FPSTR(HTTP_BACKBTN);
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

/**
 * HTTPD CALLBACK "Wifi Configuration" page handler
 */
void WiFiManager::handleWifi(bool scan)
{
    String page;

    #ifdef WM_DEBUG_LEVEL
    DEBUG_WM(DEBUG_VERBOSE,F("<- HTTP Wifi"));
    #endif

    buildWifiPage(scan, page);

    if(_gpcallback) {
        _gpcallback(WM_LP_PREHTTPSEND);
    }

    HTTPSend(page);

    if(_gpcallback) {
        _gpcallback(WM_LP_POSTHTTPSEND);
    }
}

/**
 * HTTPD CALLBACK "Wifi Configuration"->SAVE page handler
 */
void WiFiManager::handleWifiSave()
{
    bool newSSID = false;

    #ifdef WM_DEBUG_LEVEL
    DEBUG_WM(DEBUG_VERBOSE,F("<- HTTP WiFi save "));
    DEBUG_WM(DEBUG_DEV,F("Method:"),server->method() == HTTP_GET  ? (String)FPSTR(S_GET) : (String)FPSTR(S_POST));
    #endif

    // grab new ssid/pass
    {
        String newSSID = server->arg(F("s"));
        String newPASS = server->arg(F("p"));

        if(newSSID != "") {
            memset(_ssid, 0, sizeof(_ssid));
            memset(_pass, 0, sizeof(_pass));
            strncpy(_ssid, newSSID.c_str(), sizeof(_ssid) - 1);
            strncpy(_pass, newPASS.c_str(), sizeof(_pass) - 1);
            newSSID = true;
        } else if(newPASS != "") {
            memset(_pass, 0, sizeof(_pass));
            strncpy(_pass, newPASS.c_str(), sizeof(_pass) - 1);
            #ifdef _A10001986_DBG
            Serial.println("password change detected");
            #endif
        }
    }

    // At this point, _ssid and _pass are either the newly entered ones,
    // or the previous ones (if the server-provided ones were empty)

    // set static ips from server args
    // We skip this because we reboot as a result of "save", there
    // is no point is settings this here.
    #if 0
    {
        String temp;
        temp.reserve(16);
        if((temp = server->arg(FPSTR(S_ip))) != "") {
            optionalIPFromString(&_sta_static_ip, temp.c_str());
            #ifdef WM_DEBUG_LEVEL
            DEBUG_WM(DEBUG_DEV,F("static ip:"),temp);
            #endif
        }
        if((temp = server->arg(FPSTR(S_sn))) != "") {
            optionalIPFromString(&_sta_static_sn, temp.c_str());
            #ifdef WM_DEBUG_LEVEL
            DEBUG_WM(DEBUG_DEV,F("static netmask:"),temp);
            #endif
        }
        if((temp = server->arg(FPSTR(S_gw))) != "") {
            optionalIPFromString(&_sta_static_gw, temp.c_str());
            #ifdef WM_DEBUG_LEVEL
            DEBUG_WM(DEBUG_DEV,F("static gateway:"),temp);
            #endif
        }
        if((temp = server->arg(FPSTR(S_dns))) != "") {
            optionalIPFromString(&_sta_static_dns, temp.c_str());
            #ifdef WM_DEBUG_LEVEL
            DEBUG_WM(DEBUG_DEV,F("static DNS:"),temp);
            #endif
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

    unsigned long addLen = STRLEN(HTTP_END);
    if(_showBack) addLen += STRLEN(HTTP_BACKBTN);

    String page;

    if(!newSSID) {
        page.reserve(getHTTPHeadLength(S_titlewifi) + addLen + STRLEN(HTTP_PARAMSAVED) + 16);
        getHTTPHeadNew(page, S_titlewifi);
        page += FPSTR(HTTP_PARAMSAVED);
    } else {
        unsigned long s = getHTTPHeadLength(S_titlewifi) + addLen + STRLEN(HTTP_SAVED) + 16;
        if(_carMode) s += STRLEN(HTTP_SAVED_CARMODE);
        else         s += STRLEN(HTTP_SAVED_NORMAL);
        page.reserve(s);
        getHTTPHeadNew(page, S_titlewifi);
        page += FPSTR(HTTP_SAVED);
        if(_carMode) page += FPSTR(HTTP_SAVED_CARMODE);
        else         page += FPSTR(HTTP_SAVED_NORMAL);
    }

    if(_showBack) page += FPSTR(HTTP_BACKBTN);
    page += FPSTR(HTTP_END);

    if(_gpcallback) {
        _gpcallback(WM_LP_PREHTTPSEND);
    }

    server->sendHeader(FPSTR(HTTP_HEAD_CORS), FPSTR(HTTP_HEAD_CORS_ALLOW_ALL));
    HTTPSend(page);

    if(_gpcallback) {
        _gpcallback(WM_LP_POSTHTTPSEND);
    }
}


/*--------------------------------------------------------------------------*/
/********************************** PARAM ***********************************/
/*--------------------------------------------------------------------------*/

// Calc size of params page
int WiFiManager::calcParmPageSize(unsigned int& maxItemSize)
{
    int mySize = getHTTPHeadLength(S_titleparam);

    #ifdef _A10001986_DBG
    Serial.printf("calcParmSize: head %d\n", mySize);
    #endif

    mySize += (STRLEN(HTTP_FORM_START) - 3 + STRLEN(A_paramsave));

    mySize += getParamOutSize(_params, _paramsCount, maxItemSize);

    mySize += STRLEN(HTTP_FORM_END);
    if(_showBack) mySize += STRLEN(HTTP_BACKBTN);
    mySize += STRLEN(HTTP_END);

    #ifdef _A10001986_DBG
    Serial.printf("calcParmSize: calced content size %d\n", mySize);
    #endif

    return mySize;
}

/**
 * HTTPD CALLBACK param page handler
 */
void WiFiManager::handleParam()
{
    unsigned int maxItemSize = 0;

    #ifdef WM_DEBUG_LEVEL
    DEBUG_WM(DEBUG_VERBOSE,F("<- HTTP Param"));
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

    getParamOut(_params, _paramsCount, page, maxItemSize);

    page += FPSTR(HTTP_FORM_END);
    if(_showBack) page += FPSTR(HTTP_BACKBTN);
    page += FPSTR(HTTP_END);

    if(_gpcallback) {
        _gpcallback(WM_LP_PREHTTPSEND);
    }

    HTTPSend(page);

    if(_gpcallback) {
        _gpcallback(WM_LP_POSTHTTPSEND);
    }
}

void WiFiManager::handleParamSave()
{
    unsigned int mySize = 0;
    unsigned int headSize = 0;

    #ifdef WM_DEBUG_LEVEL
    DEBUG_WM(DEBUG_VERBOSE,F("<- HTTP Param save "));
    DEBUG_WM(DEBUG_DEV,F("Method:"),server->method() == HTTP_GET  ? (String)FPSTR(S_GET) : (String)FPSTR(S_POST));
    #endif

    if(_presaveparamscallback) {
        _presaveparamscallback();
    }

    doParamSave(_params, _paramsCount);

    if(_saveparamscallback) {
        _saveparamscallback();
    }

    mySize = getHTTPHeadLength(S_titleparam);

    headSize = STRLEN(HTTP_ROOT_MAIN) - (2*3);
    headSize += strlen(_title);
    headSize += STRLEN(S_titleparam);
    mySize += headSize;

    mySize += STRLEN(HTTP_PARAMSAVED);
    if(_showBack) mySize += STRLEN(HTTP_BACKBTN);
    mySize += STRLEN(HTTP_END);

    String page;
    page.reserve(mySize + 16);

    #ifdef _A10001986_DBG
    Serial.printf("handleParamSave: calced content size %d\n", mySize);
    #endif

    getHTTPHeadNew(page, S_titleparam);

    {
        String str;
        str.reserve(headSize + 8);
        str = FPSTR(HTTP_ROOT_MAIN);
        str.replace(FPSTR(T_t), _title);
        str.replace(FPSTR(T_v), S_titleparam);
        page += str;
    }

    page += FPSTR(HTTP_PARAMSAVED);
    if(_showBack) page += FPSTR(HTTP_BACKBTN);
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
/********************************** ERASE ***********************************/
/*--------------------------------------------------------------------------*/


/**
 * HTTPD CALLBACK erase page
 */

void WiFiManager::handleErase()
{
    unsigned int mysize = 0;
    unsigned int headSize = 0;

    #ifdef WM_DEBUG_LEVEL
    DEBUG_WM(DEBUG_NOTIFY,F("<- HTTP Erase"));
    #endif

    if(_erasecallback) {
        _erasecallback(true);
    }

    unsigned int mySize = getHTTPHeadLength(S_titlewifi);

    headSize = STRLEN(HTTP_ROOT_MAIN) - (2*3);
    headSize += strlen(_title);
    headSize += STRLEN(S_titlewifi);
    mySize += headSize;

    mysize += STRLEN(HTTP_ERASED);
    mysize += STRLEN(HTTP_END);

    #ifdef _A10001986_DBG
    Serial.printf("handleErase: calced content size %d\n", mySize);
    #endif

    String page;
    page.reserve(mysize + 16);

    getHTTPHeadNew(page, S_titlewifi);

    {
        String str;
        str.reserve(headSize);
        str = FPSTR(HTTP_ROOT_MAIN);
        str.replace(FPSTR(T_t), _title);
        str.replace(FPSTR(T_v), FPSTR(S_titlewifi));
        page += str;
    }

    page += FPSTR(HTTP_ERASED);
    page += FPSTR(HTTP_END);

    HTTPSend(page);

    delay(1000);

    if(_erasecallback) {
        _erasecallback(false);
    }

    reboot();
}

/*--------------------------------------------------------------------------*/
/********************************* UPDATE ***********************************/
/*--------------------------------------------------------------------------*/


// Called when /update is requested
void WiFiManager::handleUpdate()
{
    unsigned int mySize = 0;
    unsigned int headSize = 0;

    #ifdef WM_DEBUG_LEVEL
    DEBUG_WM(DEBUG_VERBOSE,F("<- Handle update"));
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

        #ifdef WM_DEBUG_LEVEL
        DEBUG_WM(DEBUG_VERBOSE,"[OTA] Update file: ", upload.filename.c_str());
        #endif

        if(!Update.begin(UPDATE_SIZE_UNKNOWN)) {
            #ifdef WM_DEBUG_LEVEL
            DEBUG_WM(DEBUG_ERROR,F("[ERROR] OTA Update ERROR"), Update.getError());
            #endif
            _uplError = true;
        }

    } else if(upload.status == UPLOAD_FILE_WRITE) {

        if(!_uplError) {

            if(Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
                #ifdef WM_DEBUG_LEVEL
                DEBUG_WM(DEBUG_ERROR,F("[ERROR] OTA Update WRITE ERROR"), Update.getError());
                #endif
                _uplError = true;
            }

        }

    } else if(upload.status == UPLOAD_FILE_END) {

        if(!_uplError) {

            #ifdef WM_DEBUG_LEVEL
            DEBUG_WM(DEBUG_VERBOSE,F("\n\n[OTA] OTA FILE END bytes: "), upload.totalSize);
            #endif

            if(!Update.end(true)) {
                _uplError = true;
            }

        }

    } else if(upload.status == UPLOAD_FILE_ABORTED) {

        Update.abort();

        #ifdef WM_DEBUG_LEVEL
        DEBUG_WM(F("[OTA] Update was aborted"));
        #endif

        if(_postotaupdatecallback) {
            _postotaupdatecallback(false);
        }

        delay(1000);  // Not '_delay'

        reboot();
    }

    delay(0);
}

// upload and ota done, show status
void WiFiManager::handleUpdateDone()
{
    unsigned int mySize = 0;
    unsigned int headSize = 0;
    bool res;

    #ifdef WM_DEBUG_LEVEL
    DEBUG_WM(DEBUG_VERBOSE, F("<- Handle update done"));
    #endif

    mySize = getHTTPHeadLength(S_titleupd);

    headSize = STRLEN(HTTP_ROOT_MAIN) - (2*3);
    headSize += strlen(_title);
    headSize += STRLEN(S_titleupd);
    mySize += headSize;

    if((res = !Update.hasError())) {
        mySize += STRLEN(HTTP_UPDATE_SUCCESS);
    } else {
        mySize += STRLEN(HTTP_UPDATE_FAIL1) + STRLEN(HTTP_UPDATE_FAIL2);
        mySize += 7 + strlen(Update.errorString());
    }

    mySize += STRLEN(HTTP_END);

    #ifdef _A10001986_DBG
    Serial.printf("handleUpdateDone: calced content size %d\n", mySize);
    #endif

    String page;
    page.reserve(mySize + 16);

    getHTTPHeadNew(page, S_titleupd);

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
        #ifdef WM_DEBUG_LEVEL
        DEBUG_WM(F("[OTA] update ok"));
        #endif
    } else {
        page += FPSTR(HTTP_UPDATE_FAIL1);
        page += "Error: " + String(Update.errorString());
        page += FPSTR(HTTP_UPDATE_FAIL2);
        #ifdef WM_DEBUG_LEVEL
        DEBUG_WM(F("[OTA] update failed"));
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

// PUBLIC

// METHODS

/**
 * [stopConfigPortal description]
 * @return {[type]} [description]
 */
bool WiFiManager::stopConfigPortal()
{
    return shutdownConfigPortal();
}

/**
 * disconnect
 * @access public
 * @since $dev
 * @return bool success
 */
bool WiFiManager::disconnect()
{
    if(WiFi.status() != WL_CONNECTED) {
        #ifdef WM_DEBUG_LEVEL
        DEBUG_WM(DEBUG_VERBOSE,F("Disconnecting: Not connected"));
        #endif
        return false;
    }
    #ifdef WM_DEBUG_LEVEL
    DEBUG_WM(F("Disconnecting"));
    #endif
    return WiFi_Disconnect();
}

/**
 * reboot the device
 * @access private
 */
void WiFiManager::reboot()
{
    ESP.restart();
}

// SETTERS

/**
 * [setConnectTimeout description]
 * @access public
 * @param {[type]} unsigned long seconds [description]
 */
void WiFiManager::setConnectTimeout(unsigned long seconds)
{
    _connectTimeout = seconds * 1000;
}

/**
 * [setConnectRetries description]
 * @access public
 * @param {[type]} uint8_t numRetries [description]
 */
void WiFiManager::setConnectRetries(uint8_t numRetries)
{
    _connectRetries = constrain(numRetries, 1, 10);
}

/**
 * setHttpPort
 * @param uint16_t port webserver port number default 80
 */
void WiFiManager::setHttpPort(uint16_t port)
{
    _httpPort = port;
}

/**
 * toggle _cleanconnect, always disconnect before connecting
 * @param {[type]} bool enable [description]
 */
void WiFiManager::setCleanConnect(bool enable)
{
    _cleanConnect = enable;
}

/**
 * [setAPStaticIPConfig description]
 * @access public
 * @param {[type]} IPAddress ip [description]
 * @param {[type]} IPAddress gw [description]
 * @param {[type]} IPAddress sn [description]
 */
#ifdef WM_AP_STATIC_IP
void WiFiManager::setAPStaticIPConfig(IPAddress ip, IPAddress gw, IPAddress sn)
{
    _ap_static_ip = ip;
    _ap_static_gw = gw;
    _ap_static_sn = sn;
}
#endif

/**
 * [setSTAStaticIPConfig description]
 * @access public
 * @param {[type]} IPAddress ip [description]
 * @param {[type]} IPAddress gw [description]
 * @param {[type]} IPAddress sn [description]
 */
void WiFiManager::setSTAStaticIPConfig(IPAddress ip, IPAddress gw, IPAddress sn)
{
    _sta_static_ip = ip;
    _sta_static_gw = gw;
    _sta_static_sn = sn;
}

/**
 * [setSTAStaticIPConfig description]
 * @since $dev
 * @access public
 * @param {[type]} IPAddress ip [description]
 * @param {[type]} IPAddress gw [description]
 * @param {[type]} IPAddress sn [description]
 * @param {[type]} IPAddress dns [description]
 */
void WiFiManager::setSTAStaticIPConfig(IPAddress ip, IPAddress gw, IPAddress sn, IPAddress dns)
{
    setSTAStaticIPConfig(ip,gw,sn);
    _sta_static_dns = dns;
}

/**
 * [setMinimumSignalQuality description]
 * @access public
 * @param {[type]} int quality [description]
 */
void WiFiManager::setMinimumSignalQuality(int quality)
{
    _minimumQuality = quality;
}

/**
 * setAPCallback, set a callback when softap is started
 * @access public
 * @param {[type]} void (*func)(WiFiManager* wminstance)
 */
#ifdef WM_APCALLBACK
void WiFiManager::setAPCallback(void(*func)(WiFiManager*))
{
    _apcallback = func;
}
#endif

/**
 * setPreConnectCallback, set a callback when wifi hardware
 * is initialized (eg after Wifi.mode()), but no connect
 * attempt has been made.
 * @access public
 * @param {[type]} void (*func)(WiFiManager* wminstance)
 */
#if WM_PRECONNECTCB
void WiFiManager::setPreConnectCallback(void(*func)())
{
    _preconnectcallback = func;
}
#endif

/**
 * setWebServerCallback, set a callback after webserver is reset, and
 * before routes are set up
 * on events cannot be overridden once set, and are not multiples
 * @access public
 * @param {[type]} void (*func)(void)
 */
void WiFiManager::setWebServerCallback(void(*func)())
{
    _webservercallback = func;
}

/**
 * setSaveWiFiCallback, set a save config callback
 * @access public
 * @param {[type]} void (*func)(void)
 */
void WiFiManager::setSaveWiFiCallback(void(*func)(const char *, const char *))
{
    _savewificallback = func;
}

/**
 * setPreSaveConfigCallback, set a callback to fire before saving wifi
 * @access public
 * @param {[type]} void (*func)(void)
 */
void WiFiManager::setPreSaveWiFiCallback(void(*func)())
{
    _presavewificallback = func;
}

/**
 * setSaveParamsCallback, set a save params callback on params page
 * @access public
 * @param {[type]} void (*func)(void)
 */
void WiFiManager::setSaveParamsCallback(void(*func)())
{
    _saveparamscallback = func;
}

/**
 * setPreSaveParamsCallback, set a pre save params callback on params
 * save prior to anything else
 * @access public
 * @param {[type]} void (*func)(void)
 */
void WiFiManager::setPreSaveParamsCallback(void(*func)())
{
    _presaveparamscallback = func;
}

/**
 * setxxxOtaUpdateCallback, set a callback to fire before OTA update
 * or after, respectively.
 * @access public
 * @param {[type]} void (*func)(void)
 */
void WiFiManager::setPreOtaUpdateCallback(void(*func)())
{
    _preotaupdatecallback = func;
}
void WiFiManager::setPostOtaUpdateCallback(void(*func)(bool))
{
    _postotaupdatecallback = func;
}

/**
 * setEraseCallback, set an erase callback
 * @access public
 * @param {[type]} void (*func)(void)
 */
void WiFiManager::setEraseCallback(void(*func)(bool))
{
    _erasecallback = func;
}

/* setMenuOutLenCallback, set a callback give size of stuff added
 * through setMenuOutCallback
 * @access public
 * @param {[type]} int (*func)(void)
 */
void WiFiManager::setMenuOutLenCallback(int(*func)())
{
    _menuoutlencallback = func;
}

/* setMenuOutCallback, set a callback to add html to root menu
 * @access public
 * @param {[type]} void (*func)(String &page)
 */
void WiFiManager::setMenuOutCallback(void(*func)(String &page))
{
    _menuoutcallback = func;
}

/* setPreWiFiScanCallback, set a callback before a wifi scan is started
 * @access public
 * @param {[type]} bool (*func)()
 * @return true if scan can proceed, false if scan needs to cancelled
 */
void WiFiManager::setPreWiFiScanCallback(bool(*func)())
{
    _prewifiscancallback = func;
}

/* setDelayReplacement, set a delay replacement function
 * @access public
 * @param {[type]} void (*func)(unsigned int)
 */
void WiFiManager::setDelayReplacement(void(*func)(unsigned int))
{
    _delayreplacement = func;
}

/* setGPCallback, set callback for when stuff takes a while
 * @access public
 * @param {[type]} void (*func)(int) - WM_LP_xxx
 */
void WiFiManager::setGPCallback(void(*func)(int))
{
    _gpcallback = func;
}

/**
 * set custom head html
 * custom element will be added to head
 * @access public
 * @param char element
 */
void WiFiManager::setCustomHeadElement(const char* html)
{
    _customHeadElement = html;
}

/**
 * set custom menu html
 * custom element will be added to menu under custom menu item.
 * @access public
 * @param char element
 */
void WiFiManager::setCustomMenuHTML(const char* html)
{
    _customMenuHTML = html;
}

/**
 * toggle wifiscan hiding of duplicate ssid names
 * if this is false, wifiscan will remove duplicat Access Points - default true
 * @access public
 * @param bool removeDuplicates [true]
 */
void WiFiManager::setRemoveDuplicateAPs(bool removeDuplicates)
{
    _removeDuplicateAPs = removeDuplicates;
}

/**
 * toggle showing static ip form fields
 * if enabled, then the static ip, gateway, subnet fields will be visible
 * @since $dev
 * @access public
 * @param bool alwaysShow [false]
 */
void WiFiManager::setShowStaticFields(bool doShow)
{
    _staShowStaticFields = doShow;
}

/**
 * toggle showing dns fields
 * if enabled, then the dns field will be visible, even if not set in code
 * @since $dev
 * @access public
 * @param bool alwaysShow [false]
 */
void WiFiManager::setShowDnsFields(bool doShow)
{
    _staShowDns = doShow;
}

/**
 * set the hostname (dhcp client id)
 * @since $dev
 * @access public
 * @param  char* hostname 32 character hostname to use for sta+ap
 * @return bool false if hostname is not valid
 */
bool WiFiManager::setHostname(const char * hostname)
{
    memset(_hostname, 0, sizeof(_hostname));
    strncpy(_hostname, hostname, sizeof(_hostname) - 1);
    return true;
}

/**
 * set the soft ap channel
 * @param int32_t   wifi channel, 0 to disable
 */
void WiFiManager::setWiFiAPChannel(int32_t channel)
{
    _apChannel = channel;
}

/**
 * set the soft ap max client number
 * @param int
 */
void WiFiManager::setWiFiAPMaxClients(int newmax)
{
    if(newmax > 10) newmax = 10;
    else if(newmax < 1) newmax = 1;
    _ap_max_clients = newmax;
}

/**
 * [setTitle description]
 * @param String title, set app title
 */
void WiFiManager::setTitle(const char *title)
{
    memset(_title, 0, sizeof(_title));
    strncpy(_title, title, sizeof(_title) - 1);
}

void WiFiManager::showUploadContainer(bool enable, const char *contName, bool showMsg)
{
    if((_showUploadSnd = enable)) {
        memset(_sndContName, 0, sizeof(_sndContName));
        strncpy(_sndContName, contName, sizeof(_sndContName) - 1);
    } else {
        _showContMsg = showMsg;
    }
}

/**
 * set menu items and order
 * eg.
 *  const int8_t menu[size] = {WM_MENU_WIFI, WM_MENU_PARAM};
 *  WiFiManager.setMenu(menu , size);
 * @since $dev
 * @param int8_t menu[] array of menu ids
 */

void WiFiManager::setMenu(const int8_t *menu, uint8_t size)
{
    if(_menuIdArr) free((void *)_menuIdArr);

    _menuIdArr = NULL;

    if(!size) return;

    _menuIdArr = (int8_t *)malloc(size + 1);
    memset(_menuIdArr, WM_MENU_END, size + 1);

    for(int i = 0; i < size; i++) {
        if(menu[i] > WM_MENU_MAX) continue;
        if(menu[i] == WM_MENU_END) break;
        _menuIdArr[i] = menu[i];
    }
}

void WiFiManager::setCarMode(bool enable)
{
    _carMode = enable;
}

/**
 * [setDebugOutput description]
 * @access public
 * @param {[type]} bool debug [description]
 */
void WiFiManager::setDebugOutput(bool debug)
{
    _debug = debug;

}

void WiFiManager::setDebugOutput(bool debug, String prefix)
{
    _debugPrefix = prefix;
    setDebugOutput(debug);
}

/**
 * setCountry
 * @since $dev
 * @param String cc country code, must be defined in WiFiSetCountry, US, JP, CN
 */
#ifndef _A10001986_NO_COUNTRY
void WiFiManager::setCountry(String cc)
{
    _wificountry = cc;
}
#endif

// GETTERS

/**
 * check if the config portal is running
 * @return bool true if active
 */
bool WiFiManager::getConfigPortalActive()
{
    return configPortalActive;
}

/**
 * [getConfigPortalActive description]
 * @return bool true if active
 */
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

/**
 * return the last known connection result
 * logged on autoconnect and wifisave, can be used to check why failed
 * get as readable string with getWLStatusString(getLastConxResult);
 * @since $dev
 * @access public
 * @return bool return wl_status codes
 */
uint8_t WiFiManager::getLastConxResult()
{
    return _lastconxresult;
}

/**
 * getDefaultAPName
 * @since $dev
 * @return string
 */
void WiFiManager::getDefaultAPName(char *apName)
{
    sprintf(apName, "ESP32_%x", WIFI_getChipId());
    while(*apName) {
        *apName = toupper((unsigned char) *apName);
        apName++;
    }
}

const char * WiFiManager::getHTTPSTART(int& titleStart)
{
    char *t = strstr(HTTP_HEAD_START, "{v}");
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

int WiFiManager::getRSSIasQuality(int RSSI)
{
    if(RSSI <= -100) {
        return 0;
    } else if(RSSI >= -50) {
        return 100;
    }
    return 2 * (RSSI + 100);
}

// HELPERS

/** IP to String? */
String WiFiManager::toStringIp(IPAddress ip)
{
    char str[20];
    sprintf(str, "%d.%d.%d.%d", ip[0] & 0xff, ip[1] & 0xff, ip[2] & 0xff, ip[3] & 0xff);
    return String(str);
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

/**
 * encode htmlentities
 * @since $dev
 * @param  string str  string to replace entities
 * @return string      encoded string
 */
String WiFiManager::htmlEntities(String str, bool whitespace)
{
    str.replace("&", "&amp;");
    str.replace("<", "&lt;");
    str.replace(">", "&gt;");
    str.replace("'", "&#39;");
    if(whitespace) str.replace(" ", "&#160;");
    // str.replace("-","&ndash;");
    // str.replace("\"","&quot;");
    // str.replace("/": "&#x2F;");
    // str.replace("`": "&#x60;");
    // str.replace("=": "&#x3D;");
    return str;
}

int WiFiManager::htmlEntitiesLen(String& str, bool whitespace)
{
    int size = 0;
    for(int i = 0; i < str.length(); i++) {
        switch(str.charAt(i)) {
        case '&':
        case '\'':
            size += 5;
            break;
        case '<':
        case '>':
            size += 4;
            break;
        case ' ':
            size += whitespace ? 6 : 1;
            break;
        default:
            size++;
        }
    }
    return size;
}

// sta disconnect
bool WiFiManager::WiFi_Disconnect()
{
    return WiFi.disconnect();
}

// toggle STA mode
bool WiFiManager::WiFi_enableSTA(bool enable)
{
    return WiFi.enableSTA(enable);
}

uint8_t WiFiManager::WiFi_softap_num_stations()
{
    return WiFi.softAPgetStationNum();
}

String WiFiManager::WiFi_SSID() const
{
    return String(_ssid);
}

size_t WiFiManager::WiFi_psk_len() const
{
    return strlen(_pass);
}

// Transitional function to read out the NVS-stored credentials
// This MUST NOT be called after WiFi is initialized
// xlen = size of available memory, INCLUDING 0-term!
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

void WiFiManager::WiFiEvent(WiFiEvent_t event, arduino_event_info_t info)
{
    if(!_hasBegun) {
        return;
    }

    if(event == ARDUINO_EVENT_WIFI_STA_DISCONNECTED) {

        // The core *should* reconnect automatically.

        _lastconxresulttmp = WiFi.status();

        //WiFi.reconnect();   // No

    } else if(event == ARDUINO_EVENT_WIFI_SCAN_DONE && _asyncScan) {

        WiFi_scanComplete(WiFi.scanComplete());

    } else {

        #ifdef _A10001986_DBG
        Serial.printf("WM: WiFi Event %d\n", event);
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

// @todo fix DEBUG_WM(0,0);
template <typename Generic>
void WiFiManager::DEBUG_WM(Generic text)
{
    DEBUG_WM(DEBUG_NOTIFY,text,"");
}

template <typename Generic>
void WiFiManager::DEBUG_WM(wm_debuglevel_t level,Generic text)
{
    if(_debugLevel >= level) DEBUG_WM(level,text,"");
}

template <typename Generic, typename Genericb>
void WiFiManager::DEBUG_WM(Generic text,Genericb textb)
{
    DEBUG_WM(DEBUG_NOTIFY,text,textb);
}

template <typename Generic, typename Genericb>
void WiFiManager::DEBUG_WM(wm_debuglevel_t level,Generic text,Genericb textb)
{
    if(!_debug || _debugLevel < level) return;

    if(_debugLevel > DEBUG_MAX) {

        // total_free_bytes;      ///<  Total free bytes in the heap. Equivalent to multi_free_heap_size().
        // total_allocated_bytes; ///<  Total bytes allocated to data in the heap.
        // largest_free_block;    ///<  Size of largest free block in the heap. This is the largest malloc-able size.
        // minimum_free_bytes;    ///<  Lifetime minimum free heap size. Equivalent to multi_minimum_free_heap_size().
        // allocated_blocks;      ///<  Number of (variable size) blocks allocated in the heap.
        // free_blocks;           ///<  Number of (variable size) free blocks in the heap.
        // total_blocks;          ///<  Total number of (variable size) blocks in the heap.
        multi_heap_info_t info;
        heap_caps_get_info(&info, MALLOC_CAP_INTERNAL);
        uint32_t free = info.total_free_bytes;
        uint16_t max  = info.largest_free_block;
        uint8_t frag = 100 - (max * 100) / free;
        _debugPort.printf("[MEM] free: %5u | max: %5d | frag: %3d%% \n", free, max, frag);
    }

    _debugPort.print(_debugPrefix);
    if(_debugLevel >= debugLvlShow) _debugPort.print("["+(String)level+"] ");
    _debugPort.print(text);
    if(textb) {
        _debugPort.print(" ");
        _debugPort.print(textb);
    }
    _debugPort.println();
}

/**
 * [debugSoftAPConfig description]
 * @access public
 * @return {[type]} [description]
 */
#ifdef WM_DODEBUG
void WiFiManager::debugSoftAPConfig()
{
    #if !defined(WM_NOCOUNTRY)
    wifi_country_t country;
    #endif
    wifi_config_t conf_config;

    esp_wifi_get_config(WIFI_IF_AP, &conf_config); // == ESP_OK
    wifi_ap_config_t config = conf_config.ap;

    #if !defined(WM_NOCOUNTRY)
    esp_wifi_get_country(&country);
    #endif

    #ifdef WM_DEBUG_LEVEL
    DEBUG_WM(F("SoftAP Configuration"));
    DEBUG_WM(FPSTR(D_HR));
    DEBUG_WM(F("ssid:            "),(char *) config.ssid);
    DEBUG_WM(F("password:        "),(char *) config.password);
    DEBUG_WM(F("ssid_len:        "),config.ssid_len);
    DEBUG_WM(F("channel:         "),config.channel);
    DEBUG_WM(F("authmode:        "),config.authmode);
    DEBUG_WM(F("ssid_hidden:     "),config.ssid_hidden);
    DEBUG_WM(F("max_connection:  "),config.max_connection);
    #endif
    #if !defined(WM_NOCOUNTRY)
    #ifdef WM_DEBUG_LEVEL
    DEBUG_WM(F("country:         "),(String)country.cc);
    #endif
    DEBUG_WM(F("beacon_interval: "),(String)config.beacon_interval + "(ms)");
    DEBUG_WM(FPSTR(D_HR));
    #endif
}
#endif

#ifdef WM_DEBUG_LEVEL
/**
 * [getWLStatusString description]
 * @access public
 * @param  {[type]} uint8_t status        [description]
 * @return {[type]}         [description]
 */
String WiFiManager::getWLStatusString(uint8_t status)
{
    if(status <= 7) return WIFI_STA_STATUS[status];
    return FPSTR(S_NA);
}

String WiFiManager::getWLStatusString()
{
    uint8_t status = WiFi.status();
    if(status <= 7) return WIFI_STA_STATUS[status];
    return FPSTR(S_NA);
}

String WiFiManager::getModeString(uint8_t mode) {
    if(mode <= 3) return WIFI_MODES[mode];
    return FPSTR(S_NA);
}
#endif  // WM_DEBUG_LEVEL

#ifndef _A10001986_NO_COUNTRY
bool WiFiManager::WiFiSetCountry()
{
    if(_wificountry == "") return false; // skip not set

    #ifdef WM_DEBUG_LEVEL
    DEBUG_WM(DEBUG_VERBOSE,F("WiFiSetCountry to"),_wificountry);
    #endif

    /*
     * @return
     *    - ESP_OK: succeed
     *    - ESP_ERR_WIFI_NOT_INIT: WiFi is not initialized by eps_wifi_init
     *    - ESP_ERR_WIFI_IF: invalid interface
     *    - ESP_ERR_WIFI_ARG: invalid argument
     *    - others: refer to error codes in esp_err.h
     */

    // @todo move these definitions, and out of cpp `esp_wifi_set_country(&WM_COUNTRY_US)`
    bool ret = true;
    // ret = esp_wifi_set_bandwidth(WIFI_IF_AP,WIFI_BW_HT20); // WIFI_BW_HT40

    esp_err_t err = ESP_OK;
    // @todo check if wifi is init, no idea how, doesnt seem to be exposed atm
    // ( check again it might be now! )
    if(WiFi.getMode() == WIFI_MODE_NULL) {
        DEBUG_WM(DEBUG_ERROR,"[ERROR] cannot set country, wifi not init");
        // exception if wifi not init!
        // Assumes that _wificountry is set to one of the supported country codes : "01"(world safe mode) "AT","AU","BE","BG","BR",
        //               "CA","CH","CN","CY","CZ","DE","DK","EE","ES","FI","FR","GB","GR","HK","HR","HU",
        //               "IE","IN","IS","IT","JP","KR","LI","LT","LU","LV","MT","MX","NL","NO","NZ","PL","PT",
        //               "RO","SE","SI","SK","TW","US"
        // If an invalid country code is passed, ESP_ERR_WIFI_ARG will be returned
        // This also uses 802.11d mode, which matches the STA to the country code of the
        // AP it connects to (meaning that the country code will be overridden if
        // connecting to a "foreign" AP)
    } else {
        #ifndef WM_NOCOUNTRY
        err = esp_wifi_set_country_code(_wificountry.c_str(), true);
        #else
        DEBUG_WM(DEBUG_ERROR,"[ERROR] esp wifi set country is not available");
        err = true;
        #endif
    }

    #ifdef WM_DEBUG_LEVEL
    if(err) {
        if(err == ESP_ERR_WIFI_NOT_INIT) DEBUG_WM(DEBUG_ERROR,"[ERROR] ESP_ERR_WIFI_NOT_INIT");
        else if(err == ESP_ERR_INVALID_ARG) DEBUG_WM(DEBUG_ERROR,"[ERROR] ESP_ERR_WIFI_ARG (invalid country code)");
        else if(err != ESP_OK)DEBUG_WM(DEBUG_ERROR,"[ERROR] unknown error",(String)err);
    }
    #endif

    ret = err == ESP_OK;

    #ifdef WM_DEBUG_LEVEL
    if(ret) DEBUG_WM(DEBUG_VERBOSE,F("[OK] esp_wifi_set_country: "),_wificountry);
    else DEBUG_WM(DEBUG_ERROR,F("[ERROR] esp_wifi_set_country failed"));
    #endif

    return ret;
}
#endif

