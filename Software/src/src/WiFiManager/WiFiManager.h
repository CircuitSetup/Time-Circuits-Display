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

#include "wm_local.h"

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

// Menu IDs
#define WM_MENU_WIFI        0
#define WM_MENU_PARAM       1
#define WM_MENU_PARAM2      2
#define WM_MENU_PARAM3      3
#define WM_MENU_UPDATE      4
#define WM_MENU_SEP         5
#define WM_MENU_SEP_F       6
#define WM_MENU_CUSTOM      7
#if defined(WM_PARAM2) && defined(WM_DYNPARM3)
#define WM_MENU_PARAM2A     8
#define WM_MENU_MAX         WM_MENU_PARAM2A
#else
#define WM_MENU_MAX         WM_MENU_CUSTOM
#endif
#define WM_MENU_END        -1

// Operator for generated HTML params
#define WM_CP_LEN           1
#define WM_CP_CREATE        2
#define WM_CP_DESTROY       3

// Selector for allocParms, addParameter, getParameters, getParameterCount
#define WM_PARM_WIFI       0
#define WM_PARM_SETTINGS   1
#ifdef WM_PARAM2
#define WM_PARM_SETTINGS2  2
#ifdef WM_PARAM3
#define WM_PARM_SETTINGS3  3
#endif
#endif

// Flags:
// Label placement for parameters
#define WFM_NO_LABEL      0
#define WFM_LABEL_BEFORE  1
#define WFM_LABEL_AFTER   2
#define WFM_LABEL_DEFAULT WFM_LABEL_BEFORE
#define WFM_LABEL_MASK    0x03
// Other flags:
#define WFM_IS_CHKBOX     4   // Parm is a checkbox
#define WFM_NO_BR         8   // Skip <br> between label and input (LABEL_BEFORE only)
#define WFM_SECTS_HEAD   16   // Parm is first in first section of page (opens new section)
#define WFM_SECTS        32   // Parm is first in new section (closes prev section, opens new)
#define WFM_FOOT         64   // Parm is last in last section of page (closes section)
#define WFM_HL          128   // Parm is a headline (customHTML only; enclosed in <div cl=hl></div>)

// HTML id:s of "static IP" parameters on "WiFi Configuration" page
#define WMS_ip    "ip"
#define WMS_gw    "gw"
#define WMS_sn    "sn"
#define WMS_dns   "dns"

// Parm handed to GPCallback()
#define WM_LP_NONE          0   // No special reason (just do over-due stuff)
#define WM_LP_PREHTTPSEND   1   // pre-HTTPSend()
#define WM_LP_POSTHTTPSEND  2   // post-HTTPSend() (just do over-due stuff, ...)

#ifdef WM_PARAM2
#ifdef WM_PARAM3
#define WM_PARAM_ARRS       4
#else
#define WM_PARAM_ARRS       3
#endif
#else
#define WM_PARAM_ARRS       2
#endif

// Private extentions to WL_XXX status
#define TWL_DHCP_TIMEOUT 0x1000
#define TWL_STATUS_NONE  0x2000

class WiFiManagerParameter {
  public:
    WiFiManagerParameter(const char *id, const char *label, const char *defaultValue, int length, const char *custom, uint8_t flags = WFM_LABEL_DEFAULT);
    WiFiManagerParameter(const char *id, const char *label, const char *defaultValue, int length, uint8_t flags = WFM_LABEL_DEFAULT);
    WiFiManagerParameter(const char *id, const char *label, const char *defaultValue, const char *custom, uint8_t flags = WFM_LABEL_DEFAULT);
    WiFiManagerParameter(const char *custom, uint8_t flags = 0);
    WiFiManagerParameter(const char *(*CustomHTMLGenerator)(const char *, int), uint8_t flags = 0);
    ~WiFiManagerParameter();

    const char *getID() const                 { return _id; };
    const char *getValue() const              { return _value; };
    const char *getLabel() const              { return _label; };
    int         getValueLength() const        { return _length; };
    uint8_t     getFlags() const              { return _flags; };
    virtual const char *getCustomHTML() const { return _customHTML; };
    void        setValue(const char *defaultValue, int length);
    void        setValue(const char *defaultValue);

  protected:
    void init(const char *id, const char *label, const char *defaultValue, int length, const char *custom, uint8_t flags);
    void initC(const char *custom, const char *(*CustomHTMLGenerator)(const char *, int), uint8_t flags);

  private:
    WiFiManagerParameter& operator=(const WiFiManagerParameter&);
    const char *_id;
    const char *_label;
    union {
        char       *_value;
        const char *(*_customHTMLGenerator)(const char *, int);
    };
    int         _length;
    uint8_t     _flags;
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

    // Connect to given wifi network, and fall-back to AP mode on fail
    bool          wifiConnect(const char *ssid, const char *pass, const char *bssid, const char *apName, const char *apPassword = NULL);

    // Start/stop the web portal in STA mode. Note: Web is not started by wifiConnect().
    void          startWebPortal();
    void          stopWebPortal();

    // Start/stop the AP mode and Web Portal
    bool          startAPModeAndPortal(const char *apName, const char *apPassword = NULL,
                            const char *ssid = NULL, const char *pass = NULL, const char *bssid = NULL);
    bool          stopAPModeAndPortal();

    // loop() function: Run webserver and DNS processing. Param: Do handle web requests, or skip
    void          process(bool handleWeb = true);

    // Disable WiFi all together (result: WiFi mode = WIFI_OFF if in STA; in AP mode only if waitForOff = true)
    void          disableWiFi(bool waitForOFF = true);

    // allocate numParms entries in params array (overrules WIFI_MANAGER_MAX_PARAMS)
    void          allocParms(int idx, int numParms)     { _max_params[idx] = numParms; };

    // adds a custom parameter, returns false on failure
    bool          addParameter(int idx, WiFiManagerParameter *p);

    // returns the list of Parameters
    WiFiManagerParameter** getParameters(int idx)       { return _params[idx];} ;

    // returns the Parameters Count
    int           getParametersCount(int idx)           { return _paramsCount[idx]; };

    // SET CALLBACKS

    // called after AP mode and config portal has started
    #ifdef WM_APCALLBACK
    void          setAPCallback(void(*func)(WiFiManager*))
                                  { _apcallback = func; };
    #endif

    // called after wifi hw init, but before connection attempts
    #ifdef WM_PRECONNECTCB
    void          setPreConnectCallback(void(*func)())
                                  { _preconnectcallback = func; };
    #endif

    #ifdef WM_EVENTCB
    void          setWiFiEventCallback(void(*func)(WiFiEvent_t event))
                                  { _wifieventcallback = func; };
    #endif

    // called after webserver has started
    void          setWebServerCallback(void(*func)())
                                  { _webservercallback = func; };

    // called when saving params-in-wifi before anything else happens
    void          setPreSaveWiFiCallback(void(*func)())
                                  { _presavewificallback = func; };

    // called when wifi settings have been read from the webform
    void          setSaveWiFiCallback(void(*func)(const char *, const char *, const char *))
                                  { _savewificallback = func; };

    // called when saving params before anything else happens
    #ifdef WM_PRESAVECB
    void          setPreSaveParamsCallback(void(*func)(int))
                                  { _presaveparamscallback = func; };
    #endif

    // called when saving either params-in-wifi or params page
    void          setSaveParamsCallback(void(*func)(int))
                                  { _saveparamscallback = func; };

    // called just before/after OTA update
    void          setPreOtaUpdateCallback(void(*func)())
                                  { _preotaupdatecallback = func; };
		void          setPostOtaUpdateCallback(void(*func)(bool))
		                              { _postotaupdatecallback = func; };

    // add stuff to the main menu; second one to give WM the length for buf sizing
  	void          setMenuOutCallback(void(*func)(String &page, unsigned int appExtraSize))
  	                              { _menuoutcallback = func; };
  	void          setMenuOutLenCallback(int(*func)())
  	                              { _menuoutlencallback = func; };

  	// app-specific replacement for delay()
  	void          setDelayReplacement(void(*func)(unsigned int))
  	                              { _delayreplacement = func; };

  	// called to give app chance to update stuff when WM op might take a while
  	// WM_LP_xxx given as arg
  	void          setGPCallback(void(*func)(int))
  	                              { _gpcallback = func; };

  	// Pre-scan, allows app to forbid scan (use cache, or display no list)
  	void          setPreWiFiScanCallback(bool(*func)())
  	                              { _prewifiscancallback = func; };

  	// Set connection parameters

    // sets timeout for which to attempt connecting, useful if you get a lot of failed connects
    void          setConnectTimeout(unsigned long seconds);

    // sets number of retries for autoconnect, force retry after wait failure exit
    void          setConnectRetries(uint8_t numRetries); // default 1

    // set min rssi to include in scan, defaults to -80 if not specified
    #ifdef WM_ADDLSETTERS
    void          setMinimumRSSI(int rssi = -80);
    #endif

    // sets a custom ip /gateway /subnet configuration
    #ifdef WM_AP_STATIC_IP
    void          setAPStaticIPConfig(IPAddress& ip, IPAddress& gw, IPAddress& sn);
    #endif

    // sets config for a static IP
    void          setSTAStaticIPConfig(IPAddress& ip, IPAddress& gw, IPAddress& sn);

    // sets config for a static IP with DNS
    void          setSTAStaticIPConfig(IPAddress& ip, IPAddress& gw, IPAddress& sn, IPAddress& dns);

    // set a custom hostname, sets sta and ap dhcp client id for esp32
    // Given hostname is not copied but referenced by pointer
    void          setHostname(const char * hostname)
                                  { _hostname = hostname; };

    // set ap channel
    void          setWiFiAPChannel(int32_t channel)
                                  { _apChannel = channel; };

    // set ap max clients
    void          setWiFiAPMaxClients(int max); // default 4

    // set port of webserver, 80
    #ifdef WM_ADDLSETTERS
    void          setHttpPort(uint16_t port);
    #endif

    // CONFIG PORTAL

    // set custom menu items and order
    // docopy can be false if the menu array is a global const (that does not
    // get destructed) that also has WM_MENU_END at the end.
    void          setMenu(const int8_t *menu, unsigned int size, bool doCopy = true);

    // set the webapp title, default WiFiManager
    // Given title is not copied but referenced by pointer
    void          setTitle(const char *title)
                                  { _title = title; };

    #ifdef WM_FW_HW_VER
    void          setReqFirmwareVersion(const char *x);
    #endif

    // show audio upload on Update page
    #ifdef WM_UPLOAD
    void          showUploadContainer(bool enable, const char *contName, const char *contVer, bool isInstalled);
    #endif

    // set download link for updates
    void          setDownloadLink(const char *theLink, bool haveCV, const char *nv);

    // Custom
    void          setCarMode(bool enable)
                                  { _carMode = enable; };

    // add custom html at inside <head> for all pages
    void          setCustomHeadElement(const char* html)
                                  { _customHeadElement = html; };

    // if this is set, customise style
    void          setCustomMenuHTML(const char* html)
                                  { _customMenuHTML = html; };

    // if true, always show static net inputs, IP, subnet, gateway, else only show if set via setSTAStaticIPConfig
    #ifdef WM_ADDLSETTERS
    void          setShowStaticFields(bool alwaysShow)
                                  { _staShowStaticFields = doShow; };
    #endif

    // if true, always show static dns, esle only show if set via setSTAStaticIPConfig
    #ifdef WM_ADDLSETTERS
    void          setShowDnsFields(bool alwaysShow)
                                  { _staShowDns = doShow; };
    #endif

    // get last connection result, includes autoconnect and wifisave
    #ifdef WM_ADDLGETTERS
    uint8_t       getLastConxResult()
                                  { return _lastconxresult; };
    #endif

    // gets number of retries for autoconnect, force retry after wait failure exit
    uint8_t       getConnectRetries()
                                  { return _connectRetries; };

    // make some HTML templates available for app
    const char *  getHTTPSTART(int& titleStart);
    const char *  getHTTPSCRIPT();
    const char *  getHTTPSTYLE();
    const char *  getHTTPSTYLEOK();

    // check if config portal is active (true)
    #ifdef WM_ADDLGETTERS
    bool          getAPPortalActive()
                                  { return APPortalActive; };
    #endif

    // check if web portal is active (true)
    bool          getWebPortalActive()
                                  { return STAPortalActive; };

    bool          getBestAPChannel(int32_t& channel, int& quality);

    // Transitional function to read out the NVS-stored credentials
    void          getStoredCredentials(char *ssid, size_t slen, char *pass, size_t plen);

    std::unique_ptr<DNSServer> dnsServer = NULL;

    std::unique_ptr<WebServer> server = NULL;

    /////////////////////////////////////////////////////////////////////////////
    //                                 Private                                 //
    /////////////////////////////////////////////////////////////////////////////

  private:
    // vars
    int8_t *      _menuIdArr = NULL;
    bool          _menuArrConst = false;

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

    uint16_t      _lastconxresult         = TWL_STATUS_NONE; // store last result when doing connect operations
    bool          _badBSSID               = false;
    int           _numNetworks            = 0;
    unsigned long _lastscan               = 0; // ms for timing wifi scans
    unsigned long _bestChCacheTime        = 0;
    uint16_t      _bestChCache            = 0;

    volatile uint32_t _WiFiEventMask      = 0;
    volatile int32_t  _numNetworksAsync   = 0;

    // SSIDs and passwords
    char          _apName[34]             = "";
    char          _apPassword[66]         = "";
    char          _ssid[34]               = "";    // currently used ssid
    char          _pass[66]               = "";    // currently used psk
    char          _bssid[18]              = "";    // currently used bssid

    const char *  _hostname               = "";    // hostname for dhcp, and/or MDNS

    const char *  _title                  = NULL;  // app title

    // options & flags
    unsigned long _connectTimeout         = 7000;  // ms stop trying to connect to ap

    int32_t       _apChannel              = 0;     // default channel to use for ap, 0 for auto
    int           _ap_max_clients         = 4;     // softap max clients
    uint16_t      _httpPort               = 80;    // port for webserver
    uint8_t       _connectRetries         = 1;     // number of sta connect retries, force reconnect, wait loop (connectimeout) does not always work and first disconnect bails

    wifi_event_id_t wm_event_id           = 0;
    int           _wifiOffFlag            = 0;
    unsigned long _wifiOffNow             = 0;

    int           _minimumRSSI            = -1000; // filter wifiscan ap by this rssi
    bool          _staShowStaticFields    = true;
    bool          _staShowDns             = true;

    #ifdef WM_UPLOAD
    bool          _showUploadSnd          = false; // Show upload audio on Update page
    char          _sndContName[8]         = "";    // File name of BIN file to upload
    const char *  _sndContVer             = NULL;
    bool          _sndIsInstalled         = false;
    #endif

    #ifdef WM_FW_HW_VER
    const char *  _rfw                    = NULL;
    #endif

    const char *  _customHeadElement      = NULL;  // store custom head element html from user inside <head>
    const char *  _customMenuHTML         = NULL;  // store custom element html from user inside root menu

    const char *  _downloadLink           = NULL;
    const char *  _nv                     = NULL;
    bool          _vd                     = false;

    // internal options
    unsigned int  _scancachetime          = 30000; // ms cache time for preload scans

    bool          _autoforcerescan        = false; // automatically force rescan if scan networks is 0, ignoring cache

    bool          _carMode                = false; // Custom

    volatile bool _beginSemaphore         = false;

    #ifdef WM_MDNS
    bool          _mdnsStarted            = false;
    #endif

    void          _begin();
    void          _end();

	  bool          CheckParmID(const char *id);
	  bool          _addParameter(int idx, WiFiManagerParameter *p);

	  uint8_t       connectWifi(const char *ssid, const char *pass, const char *bssid = NULL);
    uint8_t       waitForConnectResult(bool haveStatic, unsigned long timeout, bool& timedout, bool& DHCPtimeout);
    bool          setStaticConfig();

    bool          startAP();

    void          setupDNSD();
    void          setupHTTPServer();
    void          setupMDNS();

    bool          shutdownWebPortal();

    bool          wifiSTAOn();
    bool          wifiSTAOff();
    bool          waitEvent(uint32_t mask, unsigned long timeout);

    // Webserver handlers
    unsigned int  calcTitleLen(const char *title);
	  unsigned int  getHTTPHeadLength(const char *title, uint32_t incFlags = 0);
	  void          getHTTPHeadNew(String& page, const char *title, uint32_t incFlags = 0);

	  unsigned int  getParamOutSize(WiFiManagerParameter** params,
                        int paramsCount, unsigned int& maxItemSize);
  	void          getParamOut(String &page, WiFiManagerParameter** params,
                        int paramsCount, unsigned int maxItemSize);
    void          doParamSave(WiFiManagerParameter** params, int paramsCount);

	  int           reportStatusLen(bool withMac = false);
    void          reportStatus(String &page, unsigned int estSize = 0, bool withMac = false);

    void          send_cc();
    void          HTTPSend(const String &content, bool sendCC);

	  // Root menu
	  int           getMenuOutLength(unsigned int& appExtraSize);
    void          getMenuOut(String& page, unsigned int appExtraSize);
    unsigned int  calcRootLen(unsigned int& repSize, unsigned int& appExtraSize);
    void          buildRootPage(String& page, unsigned int repSize, unsigned int appExtraSize);
    void          handleRoot();

  	// WiFi page
  	int16_t       WiFi_waitForScan();
  	int16_t       WiFi_scanNetworks(bool force, bool async);
  	void          sortNetworks(int n, int *indices, int& haveDupes, bool removeDupes);
  	unsigned int  getScanItemsLen(int n, bool scanErr, int *indices, unsigned int& maxItemSize, int& stopAt, bool showall);
    void          getScanItemsOut(String& page, int n, bool scanErr, int *indices, unsigned int maxItemSize, bool showall);
	  void          getIpForm(String& page, const char *id, const char *title, IPAddress& value, const char *ph = NULL);
    void          getStaticOut(String& page);
	  unsigned int  getStaticLen();
    void          buildWifiPage(String& page, bool scan);
	  void          handleWifi(bool scan);
    void          handleWifiSave();

  	// Param pages
  	int	  	      calcParmPageSize(int aidx, unsigned int& maxItemSize, const char *title, const char *action);
  	void          _handleParam(int aidx, const char *title, const char *action);
  	void          _handleParamSave(int aidx, const char *title);
  	void          handleParam();
    void          handleParamSave();
    #ifdef WM_PARAM2
  	void          handleParam2();
  	void          handleParam2Save();
  	#ifdef WM_PARAM3
  	void          handleParam3();
  	void          handleParam3Save();
  	#endif
  	#endif

  	// OTA Update page
  	void          handleUpdate();
  	void          handleUpdating();
  	void          handleUpdateDone();

  	// Other
  	void          handleNotFound();

    // get default ap esp uses, esp_chipid
    void          getDefaultAPName(char *apname);

    // Helpers for html
    String        htmlEntities(String& str, bool forprint = false);
	  int           htmlEntitiesLen(String& str, bool forprint = false);
	  bool          checkSSID(String& ssid);

	  long          wmmap(long x);
	  void          _delay(unsigned int mydel);

    // internal version
    bool          _getbestapchannel(int32_t& channel, int& quality);

    // Wifi events
    void          WiFiEvent(WiFiEvent_t event, arduino_event_info_t info);
    void          WiFi_installEventHandler();
    void          andWiFiEventMask(uint32_t to_and);

    // Flags
    bool          APPortalActive       = false;
    bool          STAPortalActive      = false;

    bool          _uplError            = false;

    // WiFiManagerParameters
    int           _paramsCount[WM_PARAM_ARRS]     = { 0 };
    int           _max_params[WM_PARAM_ARRS];     // Initialized in WiFiManagerInit()
    WiFiManagerParameter** _params[WM_PARAM_ARRS] = { NULL };

    // Callbacks
    #ifdef WM_APCALLBACK
    void (*_apcallback)(WiFiManager*)                                   = NULL;
    #endif
    void (*_webservercallback)(void)                                    = NULL;
    void (*_savewificallback)(const char *, const char *, const char *) = NULL;
    void (*_presavewificallback)(void)                                  = NULL;
    #ifdef WM_PRESAVECB
    void (*_presaveparamscallback)(int)                                 = NULL;
    #endif
    void (*_saveparamscallback)(int)                                    = NULL;
    void (*_preotaupdatecallback)(void)                                 = NULL;
    void (*_postotaupdatecallback)(bool)                                = NULL;
	  void (*_menuoutcallback)(String&, unsigned int)                     = NULL;
	  int  (*_menuoutlencallback)(void)                                   = NULL;
	  void (*_delayreplacement)(unsigned int)                             = NULL;
	  void (*_gpcallback)(int)                                            = NULL;
	  bool (*_prewifiscancallback)(void)                                  = NULL;
	  #ifdef WM_PRECONNECTCB
	  void (*_preconnectcallback)(void)                                   = NULL;
	  #endif
	  #ifdef WM_EVENTCB
	  void (_wifieventcallback)(WiFiEvent_t event)                        = NULL;
	  #endif

    #ifdef _A10001986_DBG
    // get a status as string
    String        getWLStatusString(uint8_t status);
    // get wifi mode as string
    String        getModeString(uint8_t mode);
    // debug output the softap config
    void          debugSoftAPConfig();
    #endif
};

#endif
