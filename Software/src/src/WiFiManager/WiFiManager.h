/**
 * WiFiManager.h
 *
 * Based on:
 * WiFiManager, a library for the ESP32/Arduino platform
 * Creator tzapu (tablatronix)
 * Version 2.0.15
 * License MIT
 *
 * Adapted by Thomas Winischhofer (A10001986)
 */


#ifndef WiFiManager_h
#define WiFiManager_h

//#define _A10001986_DBG
//#define _A10001986_V_DBG

// #define WM_AP_STATIC_IP
// #define WM_APCALLBACK
// #define WM_PRECONNECTCB
// #define WM_EVENTCB
// #define WM_MDNS
// #define WM_ADDLGETTERS
// #define WM_ADDLSETTERS

#include <WiFi.h>
#include <esp_wifi.h>
#include <Update.h>

#ifndef WEBSERVER_H
#include <WebServer.h>
#endif

#ifdef WM_MDNS
#include <ESPmDNS.h>
#endif

#include <DNSServer.h>
#include <memory>

// prep string concat vars
#define WM_STRING2(x) #x
#define WM_STRING(x) WM_STRING2(x)

// menu ids
#define WM_MENU_WIFI        0
#define WM_MENU_PARAM       1
#define WM_MENU_UPDATE      2
#define WM_MENU_SEP         3
#define WM_MENU_CUSTOM      4
#define WM_MENU_END        -1
#define WM_MENU_MAX         WM_MENU_CUSTOM

// params will autoincrement and realloc by this amount when max is reached
// can (and should) be overruled by allocParms()/allocWiFiParms()
#ifndef WIFI_MANAGER_MAX_PARAMS
#define WIFI_MANAGER_MAX_PARAMS 5
#endif

// Label placement for parameters
#define WFM_NO_LABEL      0
#define WFM_LABEL_BEFORE  1
#define WFM_LABEL_AFTER   2
#define WFM_LABEL_DEFAULT WFM_LABEL_BEFORE

// HTML id:s of "static IP" parameters on "WiFi Configuration" page
#define WMS_ip    "ip"
#define WMS_gw    "gw"
#define WMS_sn    "sn"
#define WMS_dns   "dns"

// Parm handed to GPCallback
#define WM_LP_NONE          0   // No special reason (just do over-due stuff)
#define WM_LP_PREHTTPSEND   1   // pre-HTTPSend() (problem with mp3 in AP mode)
#define WM_LP_POSTHTTPSEND  2   // post-HTTPSend() (just do over-due stuff, ...)

#define WM_WIFI_SCAN_BUSY -133

#define DNS_PORT           53

#define MAX_SCAN_OUTPUT_SIZE  6144  // Maximum buffer for scan list on WiFi Config page

#if defined(ESP_ARDUINO_VERSION) && defined(ESP_ARDUINO_VERSION_VAL)
    #if ESP_ARDUINO_VERSION < ESP_ARDUINO_VERSION_VAL(2,0,0)
        #define WM_NOCOUNTRY
    #endif
    //#if ESP_ARDUINO_VERSION <= ESP_ARDUINO_VERSION_VAL(2,0,5)
        #define WM_DISCONWORKAROUND
    //#endif
#else
    #define WM_NOCOUNTRY
#endif

class WiFiManagerParameter {
  public:
    /**
        Create custom parameters that can be added to the WiFiManager setup web page
        @id is used for HTTP queries and must not contain spaces nor other special characters
    */
    WiFiManagerParameter(const char *custom);
    WiFiManagerParameter(const char *(*CustomHTMLGenerator)(const char *));
    WiFiManagerParameter(const char *id, const char *label);
    WiFiManagerParameter(const char *id, const char *label, const char *defaultValue, int length);
    WiFiManagerParameter(const char *id, const char *label, const char *defaultValue, int length, const char *custom);
    WiFiManagerParameter(const char *id, const char *label, const char *defaultValue, int length, const char *custom, int labelPlacement);
    ~WiFiManagerParameter();

    const char *getID() const;
    const char *getValue() const;
    const char *getLabel() const;
    int         getValueLength() const;
    int         getLabelPlacement() const;
    virtual const char *getCustomHTML() const;
    void        setValue(const char *defaultValue, int length);

  protected:
    void init(const char *id, const char *label, const char *defaultValue, int length, const char *custom, int labelPlacement);

  private:
    WiFiManagerParameter& operator=(const WiFiManagerParameter&);
    const char *_id;
    const char *_label;
    union {
        char       *_value;
        const char *(*_customHTMLGenerator)(const char *);
    };
    int         _length;
    int         _labelPlacement;
  protected:
    const char *_customHTML;
    friend class WiFiManager;
};


class WiFiManager
{
    /////////////////////////////////////////////////////////////////////////////
    //                                 Public                                  //
    /////////////////////////////////////////////////////////////////////////////

  public:
    WiFiManager();
    ~WiFiManager();
    void WiFiManagerInit();

    // auto connect to saved wifi, or custom, and start config portal on failures
    bool          autoConnect(const char *ssid, const char *pass, const char *apName, const char *apPassword = NULL);

    // manually start the config portal (AP mode)
    bool          startConfigPortal(const char *apName, const char *apPassword = NULL,
                            const char *ssid = NULL, const char *pass = NULL);

    // manually stop the config portal if started manually, stop immediatly
    bool          stopConfigPortal();

    // manually start the web portal (STA/connected) (=same as Config Portal in AP mode)
    void          startWebPortal();

    // manually stop the web portal if started manually (used for when connected)
    void          stopWebPortal();

    // Run webserver processing. Param: Do handle webserver, or skip
    void          process(bool handleWeb = true);

    // disconnect wifi
    bool          disconnect();

    // allocate numParms entries in params array (overrules WIFI_MANAGER_MAX_PARAMS)
    void          allocParms(int numParms);

    // adds a custom parameter, returns false on failure
    bool          addParameter(WiFiManagerParameter *p);

    // returns the list of Parameters
    WiFiManagerParameter** getParameters();

    // returns the Parameters Count
    int           getParametersCount();

    // same as above for WiFi Configuration page
    void          allocWiFiParms(int numParms);
    bool          addWiFiParameter(WiFiManagerParameter *p);
    WiFiManagerParameter** getWiFiParameters();
    int           getWiFiParametersCount();

    // SET CALLBACKS

    // called after AP mode and config portal has started
    #ifdef WM_APCALLBACK
    void          setAPCallback(void(*func)(WiFiManager*));
    #endif

    // called after wifi hw init, but before connection attempts
    #ifdef WM_PRECONNECTCB
    void          setPreConnectCallback(void(*func)());
    #endif

    #ifdef WM_EVENTCB
    void          setWiFiEventCallback(void(*func)(WiFiEvent_t event));
    #endif

    // called after webserver has started
    void          setWebServerCallback(void(*func)());

    // called when saving params-in-wifi before anything else happens
    void          setPreSaveWiFiCallback(void(*func)());

    // called when wifi settings have been read from the webform
    void          setSaveWiFiCallback(void(*func)(const char *, const char *));

    // called when saving params before anything else happens
    void          setPreSaveParamsCallback(void(*func)());

    // called when saving either params-in-wifi or params page
    void          setSaveParamsCallback(void(*func)());

    // called just before/after OTA update
    void          setPreOtaUpdateCallback(void(*func)());
		void          setPostOtaUpdateCallback(void(*func)(bool));

    // add stuff to the main menu; second one to give WM the length for buf sizing
  	void          setMenuOutCallback(void(*func)(String &page));
  	void          setMenuOutLenCallback(int(*func)());

  	// app-specific replacement for delay()
  	void          setDelayReplacement(void(*func)(unsigned int));

  	// called to give app chance to update stuff when WM op might take a while
  	// WM_LP_xxx given as arg
  	void          setGPCallback(void(*func)(int));

  	// Pre-scan, allows app to forbid scan (use cache, or display no list)
  	void          setPreWiFiScanCallback(bool(*func)());

  	// Set connection parameters

    //sets timeout for which to attempt connecting, useful if you get a lot of failed connects
    void          setConnectTimeout(unsigned long seconds);

    // sets number of retries for autoconnect, force retry after wait failure exit
    void          setConnectRetries(uint8_t numRetries); // default 1

    // set min rssi to include in scan, defaults to -80 if not specified
    #ifdef WM_ADDLSETTERS
    void          setMinimumRSSI(int rssi = -80);
    #endif

    // sets a custom ip /gateway /subnet configuration
    #ifdef WM_AP_STATIC_IP
    void          setAPStaticIPConfig(IPAddress ip, IPAddress gw, IPAddress sn);
    #endif

    // sets config for a static IP
    void          setSTAStaticIPConfig(IPAddress ip, IPAddress gw, IPAddress sn);

    // sets config for a static IP with DNS
    void          setSTAStaticIPConfig(IPAddress ip, IPAddress gw, IPAddress sn, IPAddress dns);

    // set a custom hostname, sets sta and ap dhcp client id for esp32, and sta for esp8266
    bool          setHostname(const char * hostname);

    // set ap channel
    void          setWiFiAPChannel(int32_t channel);

    // set ap max clients
    void          setWiFiAPMaxClients(int max); // default 4

    // clean connect, always disconnect before connecting
    #ifdef WM_ADDLSETTERS
    void          setCleanConnect(bool enable); // default true
    #endif

    // set port of webserver, 80
    #ifdef WM_ADDLSETTERS
    void          setHttpPort(uint16_t port);
    #endif

    // CONFIG PORTAL

    // set custom menu items and order
    // docopy can be false if the menu array is a global const (that does not
    // get destructed) that also has WM_MENU_END at the end.
    void          setMenu(const int8_t *menu, uint8_t size, bool doCopy = true);

    // set the webapp title, default WiFiManager
    void          setTitle(const char *title);

    // show audio upload on Update page
    void          showUploadContainer(bool enable, const char *contName, bool showMsg = false);

    // Custom
    void          setCarMode(bool enable);

    // add custom html at inside <head> for all pages
    void          setCustomHeadElement(const char* html);

    // if this is set, customise style
    void          setCustomMenuHTML(const char* html);

    // if true, always show static net inputs, IP, subnet, gateway, else only show if set via setSTAStaticIPConfig
    #ifdef WM_ADDLSETTERS
    void          setShowStaticFields(bool alwaysShow);
    #endif

    // if true, always show static dns, esle only show if set via setSTAStaticIPConfig
    #ifdef WM_ADDLSETTERS
    void          setShowDnsFields(bool alwaysShow);
    #endif

    // get last connection result, includes autoconnect and wifisave
    #ifdef WM_ADDLGETTERS
    uint8_t       getLastConxResult();
    #endif

    // gets number of retries for autoconnect, force retry after wait failure exit
    uint8_t       getConnectRetries();

    // make some HTML templates available for app
    const char *  getHTTPSTART(int& titleStart);
    const char *  getHTTPSCRIPT();
    const char *  getHTTPSTYLE();
    const char *  getHTTPSTYLEOK();

    // check if config portal is active (true)
    #ifdef WM_ADDLGETTERS
    bool          getConfigPortalActive();
    #endif

    // check if web portal is active (true)
    bool          getWebPortalActive();

    // get hostname helper
    String        getWiFiHostname();

    bool          getBestAPChannel(int32_t& channel, int& quality);

    // Transitional function to read out the NVS-stored credentials
    void          getStoredCredentials(char *ssid, size_t slen, char *pass, size_t plen);

    std::unique_ptr<DNSServer> dnsServer;

    std::unique_ptr<WebServer> server;

    /////////////////////////////////////////////////////////////////////////////
    //                                 Private                                 //
    /////////////////////////////////////////////////////////////////////////////

  private:
    // vars
    int8_t *      _menuIdArr = NULL;

    // ip configs
    #ifdef WM_AP_STATIC_IP
    IPAddress     _ap_static_ip;
    IPAddress     _ap_static_gw;
    IPAddress     _ap_static_sn;
    #endif
    IPAddress     _sta_static_ip;
    IPAddress     _sta_static_gw;
    IPAddress     _sta_static_sn;
    IPAddress     _sta_static_dns;

    uint8_t       _lastconxresult         = WL_IDLE_STATUS; // store last result when doing connect operations
    int           _numNetworks            = 0;
    int16_t       _numNetworksAsync       = 0;
    unsigned long _lastscan               = 0; // ms for timing wifi scans
    unsigned long _bestChCacheTime        = 0;
    uint16_t      _bestChCache            = 0;

    // SSIDs and passwords
    char          _apName[34]             = "";
    char          _apPassword[66]         = "";
    char          _ssid[34]               = "";    // currently used ssid
    char          _pass[66]               = "";    // currently used psk

    char          _hostname[34]           = "";    // hostname for dhcp, and/or MDNS

    char          _title[64]              = "WiFiManager"; // app title

    // options & flags
    unsigned long _connectTimeout         = 0;     // ms stop trying to connect to ap if set

    bool          _cleanConnect           = true;  // disconnect before connect in connectwifi, increases stability on connects
    #if 0
    bool          _disableSTA             = false; // disable sta when starting ap, always
    bool          _disableSTAConn         = true;  // disable sta when starting ap, if sta is not connected ( stability )
    #endif
    int32_t       _apChannel              = 0;     // default channel to use for ap, 0 for auto
    int           _ap_max_clients         = 4;     // softap max clients
    uint16_t      _httpPort               = 80;    // port for webserver
    uint8_t       _connectRetries         = 1;     // number of sta connect retries, force reconnect, wait loop (connectimeout) does not always work and first disconnect bails
    bool          _aggresiveReconn        = true;  // use an aggressive reconnect strategy, WILL delay conxs
                                                   // on some conn failure modes will add delays and many retries to work around esp and ap bugs, ie, anti de-auth protections
                                                   // https://github.com/tzapu/WiFiManager/issues/1067

    wifi_event_id_t wm_event_id           = 0;
    static uint8_t  _eventlastconxresult;          // for wifi event callback
    static bool     _gotip;                        // for wifi event callback

    int           _minimumRSSI            = -1000; // filter wifiscan ap by this rssi
    bool          _staShowStaticFields    = true;
    bool          _staShowDns             = true;

    bool          _showUploadSnd          = false; // Show upload audio on Update page
    bool          _showContMsg            = false;
    char          _sndContName[8]         = "";    // File name of BIN file to upload

    const char*   _customHeadElement      = NULL;  // store custom head element html from user inside <head>
    const char*   _customMenuHTML         = NULL;  // store custom element html from user inside menu

    // internal options
    unsigned int  _scancachetime          = 30000; // ms cache time for preload scans
    bool          _asyncScan              = true;  // perform wifi network scan async

    bool          _autoforcerescan        = false; // automatically force rescan if scan networks is 0, ignoring cache

    bool          _carMode                = false; // Custom

    bool          _hasBegun               = false; // flag wm loaded,unloaded

    void          _begin();
    void          _end();

	  void          _delay(unsigned int mydel);

	  bool          CheckParmID(const char *id);

    void          setupConfigPortal();
    bool          shutdownConfigPortal();

    bool          startAP();

    void          setupDNSD();
    void          setupHTTPServer();
    void          setupMDNS();

    uint8_t       connectWifi(const char *ssid, const char *pass);
    bool          setStaticConfig();
    bool          wifiConnectNew(const char *ssid, const char *pass);

    uint8_t       waitForConnectResult(bool haveStatic);
    uint8_t       waitForConnectResult(bool haveStatic, uint32_t timeout);

    // webserver handlers
	  unsigned int  getHTTPHeadLength(const char *title, bool includeMSG = false, bool includeQI = false);
	  void          getHTTPHeadNew(String& page, const char *title, bool includeMSG = false, bool includeQI = false);

	  unsigned int  getParamOutSize(WiFiManagerParameter** params,
                        int paramsCount, unsigned int& maxItemSize);
  	void          getParamOut(String &page, WiFiManagerParameter** params,
                        int paramsCount, unsigned int maxItemSize);
    void          doParamSave(WiFiManagerParameter** params, int paramsCount);

	  int           reportStatusLen();
    void          reportStatus(String &page, unsigned int estSize = 0);

    void          HTTPSend(const String &content);

	  // Root menu
	  int           getMenuOutLength();
    void          getMenuOut(String& page);
    unsigned int  calcRootLen(unsigned int& headSize, unsigned int& repSize);
    void          buildRootPage(String& page, unsigned int headSize, unsigned int repSize);
    void          handleRoot();

  	// WiFi page
  	int           getScanItemStart();
  	void          sortNetworks(int n, int *indices, int& haveDupes, bool removeDupes);
  	unsigned int  getScanItemsLen(int n, bool scanErr, int *indices, unsigned int& maxItemSize, int& stopAt, bool showall);
    void          getScanItemsOut(String& page, int n, bool scanErr, int *indices, unsigned int maxItemSize, bool showall);
	  void          getIpForm(String& page, const char *id, const char *title, IPAddress& value, const char *ph = NULL);
    void          getStaticOut(String& page);
	  unsigned int  getStaticLen();
    void          buildWifiPage(String& page, bool scan);
	  void          handleWifi(bool scan);
    void          handleWifiSave();

  	// Param page
  	int	  	      calcParmPageSize(unsigned int& maxItemSize);
  	void          handleParam();
    void          handleParamSave();

  	// OTA Update page
  	void          handleUpdate();
  	void          handleUpdating();
  	void          handleUpdateDone();

  	// Other
  	void          handleNotFound();

    void          processConfigPortal(bool handleWeb);

    // wifi platform abstractions
    void          WiFi_installEventHandler();
    String        WiFi_SSID() const;

    int16_t       WiFi_scanNetworks(bool force, bool async);
	  int16_t       WiFi_waitForScan();
	  void          WiFi_scanComplete(int16_t networksFound);

    void          WiFiEvent(WiFiEvent_t event, arduino_event_info_t info);

    // helpers
    bool          validApPassword();

    // helper for html
    String        htmlEntities(String& str, bool forprint = false);
	  int           htmlEntitiesLen(String& str, bool forprint = false);
	  bool          checkSSID(String& ssid);

	  long          wmmap(long x);

    // get default ap esp uses, esp_chipid
    void          getDefaultAPName(char *apname);

    // internal version
    bool          _getbestapchannel(int32_t& channel, int& quality);

    // reboot esp32
    void          reboot();

    // flags
    bool          configPortalActive  = false;
    bool          webPortalActive     = false;

    // WiFiManagerParameter
    int           _paramsCount         = 0;
    int           _max_params;
    WiFiManagerParameter** _params     = NULL;
    int           _wifiparamsCount     = 0;
    int           _max_wifi_params;
    WiFiManagerParameter** _wifiparams = NULL;

    bool          _uplError            = false;

    // callbacks
    #ifdef WM_APCALLBACK
    void (*_apcallback)(WiFiManager*);
    #endif
    void (*_webservercallback)(void);
    void (*_savewificallback)(const char *, const char *);
    void (*_presavewificallback)(void);
    void (*_presaveparamscallback)(void);
    void (*_saveparamscallback)(void);
    void (*_preotaupdatecallback)(void);
    void (*_postotaupdatecallback)(bool);
	  void (*_menuoutcallback)(String &page);
	  int  (*_menuoutlencallback)(void);
	  void (*_delayreplacement)(unsigned int);
	  void (*_gpcallback)(int);
	  bool (*_prewifiscancallback)(void);
	  #ifdef WM_PRECONNECTCB
	  void (*_preconnectcallback)(void);
	  #endif
	  #ifdef WM_EVENTCB
	  void (_wifieventcallback)(WiFiEvent_t event);
	  #endif

    #ifdef _A10001986_DBG
    // get a status as string
    String        getWLStatusString(uint8_t status);
    // get wifi mode as string
    String        getModeString(uint8_t mode);
    // debug output the softap config
    void          debugSoftAPConfig();
    #endif

    #if 0
    template <class T>
    auto optionalIPFromString(T *obj, const char *s) -> decltype(  obj->fromString(s)  ) {
        return  obj->fromString(s);
    }
    auto optionalIPFromString(...) -> bool {
        return false;
    }
    #endif
};

#endif
