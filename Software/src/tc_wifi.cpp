/*
 * -------------------------------------------------------------------
 * CircuitSetup.us Time Circuits Display
 * (C) 2021-2022 John deGlavina https://circuitsetup.us 
 * (C) 2022 Thomas Winischhofer (A10001986)
 * 
 * Clockdisplay and keypad menu code based on code by John Monaco
 * Marmoset Electronics 
 * https://www.marmosetelectronics.com/time-circuits-clock
 * -------------------------------------------------------------------
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "tc_wifi.h"

// If undefined, use the checkbox-hacks. 
// If defined, go back to standard text boxes
//#define TC_NOCHECKBOXES

Settings settings;

IPSettings ipsettings;

WiFiManager wm;

WiFiManagerParameter custom_headline("<h2 style='margin-bottom:0px'>TimeCircuits Setup</h2>");
#ifdef TC_NOCHECKBOXES  // --- Standard text boxes: -------
WiFiManagerParameter custom_ttrp("ttrp", "<p></p>Make time travels persistent (0=no, 1=yes)", settings.timesPers, 2, "title='If disabled, displays are reset after reboot'");
WiFiManagerParameter custom_alarmRTC("artc", "Alarm base is RTC (1) or displayed \"present\" time (0)", settings.alarmRTC, 2);
WiFiManagerParameter custom_playIntro("plIn", "Play intro (1=on, 0=off)", settings.playIntro, 2);
WiFiManagerParameter custom_mode24("md24", "Enable 24-hour clock mode: (0=12hr, 1=24hr)", settings.mode24, 2);
#ifdef EXTERNAL_TIMETRAVEL_IN
WiFiManagerParameter custom_ettLong("ettLg", "Time travel sequence (1=long, 0=short)", settings.ettLong, 2);
#endif
#ifdef FAKE_POWER_ON
WiFiManagerParameter custom_fakePwrOn("fpo", "<p></p>Enable fake power switch (0=no, 1=yes)", settings.fakePwrOn, 2, "title='Use fake power switch to power-up and power-down the device'");
#endif
#else // -------------------- Checkbox hack: --------------
WiFiManagerParameter custom_ttrp("ttrp", "Make time travels persistent<br>", settings.timesPers, 2, "title='If disabled, displays are reset after reboot' type='checkbox'", WFM_LABEL_AFTER);
WiFiManagerParameter custom_alarmRTC("artc", "Alarm base is real present time<br>", settings.alarmRTC, 2, "title='If disabled, Alarm base is displayed \"present\" time' type='checkbox'", WFM_LABEL_AFTER);
WiFiManagerParameter custom_playIntro("plIn", "Play intro<br>", settings.playIntro, 2, "type='checkbox'", WFM_LABEL_AFTER);
WiFiManagerParameter custom_mode24("md24", "Enable 24-hour clock mode<br>", settings.mode24, 2, "type='checkbox'", WFM_LABEL_AFTER);
#ifdef EXTERNAL_TIMETRAVEL_IN
WiFiManagerParameter custom_ettLong("ettLg", "Play long time travel sequence<br>", settings.ettLong, 2, "title='If disabled, short sequence is played' type='checkbox' style='margin-top:4px'", WFM_LABEL_AFTER);
#endif
#ifdef FAKE_POWER_ON
WiFiManagerParameter custom_fakePwrOn("fpo", "Enable fake power switch<br>", settings.fakePwrOn, 2, "title='Use fake power switch to power-up and power-down the device' type='checkbox' style='margin-top:16px'", WFM_LABEL_AFTER);
#endif
#endif // -------------------------------------------------
WiFiManagerParameter custom_autoRotateTimes("rotate_times", "<p></p>Time-rotation interval<br>0=off, 1-5=every 5/15/30/45/60th minute",settings.autoRotateTimes, 3, "title='Selects the interval for automatic time-cycling when idle'");
WiFiManagerParameter custom_wifiConRetries("wifiret", "<p></p>WiFi Connection attempts (1-15)", settings.wifiConRetries, 3, "", WFM_LABEL_BEFORE);
WiFiManagerParameter custom_wifiConTimeout("wificon", "WiFi Connection timeout in seconds (1-15)", settings.wifiConTimeout, 3);
WiFiManagerParameter custom_ntpServer("ntp_server", "NTP Server (empty to disable NTP)", settings.ntpServer, 63);
WiFiManagerParameter custom_timeZone("time_zone", "Timezone (in <a href='https://github.com/nayarsystems/posix_tz_db/blob/master/zones.csv' target=_blank>Posix</a> format)", settings.timeZone, 63);
WiFiManagerParameter custom_destTimeBright("dt_bright", "<p></p>Destination Time display brightness (1-15)", settings.destTimeBright, 3, "", WFM_LABEL_BEFORE);
WiFiManagerParameter custom_presTimeBright("pt_bright", "Present Time display brightness (1-15)", settings.presTimeBright, 3);
WiFiManagerParameter custom_lastTimeBright("lt_bright", "Last Time Dep. display brightness (1-15)", settings.lastTimeBright, 3);
WiFiManagerParameter custom_autoNMOn("anmon", "Auto-NightMode start hour (0-23)", settings.autoNMOn, 3, "title='To disable, set start and end to same value'");
WiFiManagerParameter custom_autoNMOff("anmoff", "Auto-NightMode end hour (0-23)", settings.autoNMOff, 3, "title='To disable, set start and end to same value'");
#ifdef EXTERNAL_TIMETRAVEL_IN
WiFiManagerParameter custom_ettDelay("ettDe", "<p></p>Externally triggered time travel:<br>Delay (ms)", settings.ettDelay, 6, "title='Externally triggered time travel will be delayed by specified number of millisecs'");
#endif

WiFiManagerParameter custom_copyAudio("cpAu", "<p></p>Audio file installation: Write COPY here to copy the original audio files from the SD card to the internal flash file system", settings.copyAudio, 5, "autocomplete='off'");

bool shouldSaveConfig = false;
bool shouldSaveIPConfig = false;
bool shouldDeleteIPConfig = false;

/*
 * wifi_setup()
 * 
 */
void wifi_setup() 
{ 
    int temp;
    
    #define TC_MENUSIZE (7)
    const char* wifiMenu[TC_MENUSIZE] = {"wifi", "info", "param", "sep", "restart", "update", "custom" };
    // We are running in non-blocking mode, so no point in "exit".

    // explicitly set mode, esp allegedly defaults to STA_AP
    WiFi.mode(WIFI_MODE_STA);
    
    #ifndef TC_DBG
    wm.setDebugOutput(false);
    #endif
    
    wm.setParamsPage(true);
    wm.setBreakAfterConfig(true);
    wm.setConfigPortalBlocking(false);
    wm.setPreSaveConfigCallback(preSaveConfigCallback);
    wm.setSaveConfigCallback(saveConfigCallback);
    wm.setSaveParamsCallback(saveParamsCallback);
    wm.setHostname("TIMECIRCUITS");

    // Center-align looks better
    wm.setCustomHeadElement("<style type='text/css'>H1,H2{margin-top:0px;margin-bottom:0px;text-align:center;}</style>");
    wm.setTitle(F("TimeCircuits"));

    // Hack version number into WiFiManager main page
    wm.setCustomMenuHTML("<img style='display:block;margin-left:auto;margin-right:auto' src='data:image/png;base64,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'><div style='font-size:9px;margin-left:auto;margin-right:auto;text-align:center;'>Version " TC_VERSION "(" TC_VERSION_EXTRA ")</div>");

    // Static IP info is not saved by WiFiManager,
    // have to do this "manually". Hence ipsettings.
    wm.setShowStaticFields(true);
    wm.setShowDnsFields(true);

    temp = (int)atoi(settings.wifiConTimeout);
    if(temp < 1) temp = 1;    // Do not allow 0, might make it hang 
    if(temp > 15) temp = 15;
    wm.setConnectTimeout(temp);
    
    temp = (int)atoi(settings.wifiConRetries);
    if(temp < 1) temp = 1;
    if(temp > 15) temp = 15;
    wm.setConnectRetries(temp);
               
    wm.setCleanConnect(true);
    //wm.setRemoveDuplicateAPs(false);

    wm.setMenu(wifiMenu, TC_MENUSIZE);

    wm.addParameter(&custom_headline);
    wm.addParameter(&custom_ttrp);
    wm.addParameter(&custom_alarmRTC);
    wm.addParameter(&custom_playIntro);
    wm.addParameter(&custom_mode24);
    wm.addParameter(&custom_autoRotateTimes);
    wm.addParameter(&custom_wifiConRetries);
    wm.addParameter(&custom_wifiConTimeout);
    wm.addParameter(&custom_ntpServer);
    wm.addParameter(&custom_timeZone);
    wm.addParameter(&custom_destTimeBright);
    wm.addParameter(&custom_presTimeBright);
    wm.addParameter(&custom_lastTimeBright);
    wm.addParameter(&custom_autoNMOn);
    wm.addParameter(&custom_autoNMOff);
    #ifdef EXTERNAL_TIMETRAVEL_IN
    wm.addParameter(&custom_ettDelay);
    wm.addParameter(&custom_ettLong);
    #endif
    #ifdef FAKE_POWER_ON
    wm.addParameter(&custom_fakePwrOn);
    #endif
    if(check_allow_CPA()) {
        wm.addParameter(&custom_copyAudio);
    }
    
    updateConfigPortalValues();

    // Configure static IP
    if(loadIpSettings()) {
        setupStaticIP();
    }

    // Automatically connect using saved credentials if they exist
    // If connection fails it starts an access point with the specified name
    if(wm.autoConnect("TCD-AP")) {
        #ifdef TC_DBG
        Serial.println(F("WiFi connected"));
        #endif
        wm.startWebPortal();  //start config portal in STA mode
    } else {
        #ifdef TC_DBG
        Serial.println(F("Config portal running"));
        #endif
    }
}

/*
 * wifi_loop()
 * 
 */
void wifi_loop() 
{
    wm.process();

    if(shouldSaveIPConfig) {
      
        #ifdef TC_DBG
        Serial.println(F("WiFi: Saving IP config"));
        #endif

        writeIpSettings();
        
        shouldSaveIPConfig = false;

    } else if(shouldDeleteIPConfig) { 

        #ifdef TC_DBG
        Serial.println(F("WiFi: Deleting IP config"));
        #endif

        deleteIpSettings();
        
        shouldDeleteIPConfig = false;
      
    }
    
    if(shouldSaveConfig) {
      
        // Save settings and restart esp32

        #ifdef TC_DBG
        Serial.println(F("WiFi: Saving config"));
        #endif

        strcpy(settings.ntpServer, custom_ntpServer.getValue());
        strcpy(settings.timeZone, custom_timeZone.getValue());
        strcpy(settings.autoRotateTimes, custom_autoRotateTimes.getValue());
        strcpy(settings.destTimeBright, custom_destTimeBright.getValue());
        strcpy(settings.presTimeBright, custom_presTimeBright.getValue());
        strcpy(settings.lastTimeBright, custom_lastTimeBright.getValue());
        strcpy(settings.autoNMOn, custom_autoNMOn.getValue());
        strcpy(settings.autoNMOff, custom_autoNMOff.getValue());
        strcpy(settings.wifiConRetries, custom_wifiConRetries.getValue());
        strcpy(settings.wifiConTimeout, custom_wifiConTimeout.getValue());
        #ifdef EXTERNAL_TIMETRAVEL_IN
        strcpy(settings.ettDelay, custom_ettDelay.getValue());
        #endif
        
        #ifdef TC_NOCHECKBOXES // ---------
        
        strcpy(settings.mode24, custom_mode24.getValue());                
        strcpy(settings.alarmRTC, custom_alarmRTC.getValue());
        strcpy(settings.timesPers, custom_ttrp.getValue()); 
        strcpy(settings.playIntro, custom_playIntro.getValue()); 
        #ifdef FAKE_POWER_ON      
        strcpy(settings.fakePwrOn, custom_fakePwrOn.getValue()); 
        #endif 
        #ifdef EXTERNAL_TIMETRAVEL_IN
        strcpy(settings.ettLong, custom_ettLong.getValue());
        #endif
        
        #else // if checkboxes are used: ---
        
        strcpy(settings.mode24, ((int)atoi(custom_mode24.getValue()) > 0) ? "1" : "0");
        strcpy(settings.alarmRTC, ((int)atoi(custom_alarmRTC.getValue()) > 0) ? "1" : "0");
        strcpy(settings.timesPers, ((int)atoi(custom_ttrp.getValue()) > 0) ? "1" : "0"); 
        strcpy(settings.playIntro, ((int)atoi(custom_playIntro.getValue()) > 0) ? "1" : "0"); 
        #ifdef FAKE_POWER_ON      
        strcpy(settings.fakePwrOn, ((int)atoi(custom_fakePwrOn.getValue()) > 0) ? "1" : "0"); 
        #endif
        #ifdef EXTERNAL_TIMETRAVEL_IN
        strcpy(settings.ettLong, ((int)atoi(custom_ettLong.getValue()) > 0) ? "1" : "0");
        #endif
        
        #endif  // -------------------------
        
        write_settings();

        if(check_allow_CPA()) {
            if(!strcmp(custom_copyAudio.getValue(), "COPY")) {
                #ifdef TC_DBG
                Serial.println(F("WiFi: Copying audio files...."));
                #endif
                copy_audio_files();
                delay(2000);
            }
        }

        shouldSaveConfig = false;

        // Reset esp32 to load new settings

        #ifdef TC_DBG
        Serial.println(F("WiFi: Restarting ESP...."));
        #endif

        Serial.flush();
        
        esp_restart();
    }
    
}

// This is called when the WiFi config changes, so it has
// nothing to do with our settings here. Despite that,
// we write out our config file so that if the user initially
// configures WiFi, a default settings file exists upon reboot.
// Also, this causes a reboot, so if the user entered static
// IP data, it becomes active after this reboot.
void saveConfigCallback() 
{
    shouldSaveConfig = true;    
}

void saveParamsCallback() 
{
    shouldSaveConfig = true;
}

// Grab static IP parameters from WiFiManager's server.
// Since there is no public method for this, we steal
// the html form parameters in this callback.
void preSaveConfigCallback()
{
    char ipBuf[20] = "";
    char gwBuf[20] = "";
    char snBuf[20] = "";
    char dnsBuf[20] = "";
    bool invalConf = false;

    #ifdef TC_DBG
    Serial.println("preSaveConfigCallback");
    #endif
    
    if(wm.server->arg(FPSTR(S_ip)) != "") {
        strncpy(ipBuf, wm.server->arg(FPSTR(S_ip)).c_str(), 19);
    } else invalConf |= true;
    if(wm.server->arg(FPSTR(S_gw)) != "") {
        strncpy(gwBuf, wm.server->arg(FPSTR(S_gw)).c_str(), 19);
    } else invalConf |= true;
    if(wm.server->arg(FPSTR(S_sn)) != "") {
        strncpy(snBuf, wm.server->arg(FPSTR(S_sn)).c_str(), 19);
    } else invalConf |= true;
    if(wm.server->arg(FPSTR(S_dns)) != "") {
        strncpy(dnsBuf, wm.server->arg(FPSTR(S_dns)).c_str(), 19);
    } else invalConf |= true;

    #ifdef TC_DBG
    Serial.println(ipBuf);   
    Serial.println(gwBuf);
    Serial.println(snBuf);
    Serial.println(dnsBuf);
    #endif

    if(!invalConf && isIp(ipBuf) && isIp(gwBuf) && isIp(snBuf) && isIp(dnsBuf)) {

        #ifdef TC_DBG
        Serial.println("All IPs valid");
        #endif
        
        strcpy(ipsettings.ip, ipBuf);
        strcpy(ipsettings.gateway, gwBuf);
        strcpy(ipsettings.netmask, snBuf);
        strcpy(ipsettings.dns, dnsBuf);
        
        shouldSaveIPConfig = true;
    
    } else {

        #ifdef TC_DBG
        Serial.println("Invalid IP"); 
        #endif

        shouldDeleteIPConfig = true;     
    
    }
}

void setupStaticIP()
{
    IPAddress ip;
    IPAddress gw;
    IPAddress sn;
    IPAddress dns;

    if(strlen(ipsettings.ip) > 0 &&
        isIp(ipsettings.ip) &&
        isIp(ipsettings.gateway) &&
        isIp(ipsettings.netmask) &&
        isIp(ipsettings.dns)) {
  
        ip = stringToIp(ipsettings.ip);
        gw = stringToIp(ipsettings.gateway);
        sn = stringToIp(ipsettings.netmask);
        dns = stringToIp(ipsettings.dns);

        wm.setSTAStaticIPConfig(ip, gw, sn, dns);
           
    }
}       

void updateConfigPortalValues()
{
    #ifndef TC_NOCHECKBOXES
    const char makeCheck[] = "1' checked a='";
    #endif
    
    // Make sure the settings form has the correct values
    custom_wifiConTimeout.setValue(settings.wifiConTimeout, 3);
    custom_wifiConRetries.setValue(settings.wifiConRetries, 3);
    custom_ntpServer.setValue(settings.ntpServer, 63);
    custom_timeZone.setValue(settings.timeZone, 63);

    #ifdef TC_NOCHECKBOXES  // Standard text boxes: -------
    
    custom_mode24.setValue(settings.mode24, 2);           
    custom_alarmRTC.setValue(settings.alarmRTC, 2);
    custom_ttrp.setValue(settings.timesPers, 2);
    custom_playIntro.setValue(settings.playIntro, 2);
    #ifdef FAKE_POWER_ON 
    custom_fakePwrOn.setValue(settings.fakePwrOn, 2);
    #endif
    #ifdef EXTERNAL_TIMETRAVEL_IN
    custom_ettLong.setValue(settings.ettLong, 2);
    #endif

    #else   // For checkbox hack --------------------------

    custom_mode24.setValue(((int)atoi(settings.mode24) > 0) ? makeCheck : "1", 14);
    custom_alarmRTC.setValue(((int)atoi(settings.alarmRTC) > 0) ? makeCheck : "1", 14);
    custom_ttrp.setValue(((int)atoi(settings.timesPers) > 0) ? makeCheck : "1", 14);           
    custom_playIntro.setValue(((int)atoi(settings.playIntro) > 0) ? makeCheck : "1", 14);
    #ifdef FAKE_POWER_ON 
    custom_fakePwrOn.setValue(((int)atoi(settings.fakePwrOn) > 0) ? makeCheck : "1", 14);
    #endif
    #ifdef EXTERNAL_TIMETRAVEL_IN
    custom_ettLong.setValue(((int)atoi(settings.ettLong) > 0) ? makeCheck : "1", 14);
    #endif

    #endif // ---------------------------------------------

    custom_autoRotateTimes.setValue(settings.autoRotateTimes, 3);
    custom_destTimeBright.setValue(settings.destTimeBright, 3);
    custom_presTimeBright.setValue(settings.presTimeBright, 3);
    custom_lastTimeBright.setValue(settings.lastTimeBright, 3);
    custom_autoNMOn.setValue(settings.autoNMOn, 3);
    custom_autoNMOff.setValue(settings.autoNMOff, 3);
    #ifdef EXTERNAL_TIMETRAVEL_IN
    custom_ettDelay.setValue(settings.ettDelay, 6);
    #endif
    
    custom_copyAudio.setValue("", 5);   // Always clear
}

int wifi_getStatus()
{
    switch(WiFi.getMode()) {
      case WIFI_MODE_STA:
          return (int)WiFi.status();
      case WIFI_MODE_AP:
          return 0x10000;     // AP MODE
      case WIFI_MODE_NULL:
          return 0x10001;     // OFF          
    }

    return 0x10002;           // UNKNOWN
}

bool wifi_getIP(uint8_t& a, uint8_t& b, uint8_t& c, uint8_t& d)
{
    IPAddress myip;
  
    switch(WiFi.getMode()) {
      case WIFI_MODE_STA:
          myip = WiFi.localIP();
          break;
      case WIFI_MODE_AP:
          myip = WiFi.softAPIP();
          break;
      default:
          a = b = c = d = 0;
          return true;
    }
  
    a = myip[0];
    b = myip[1];
    c = myip[2];
    d = myip[3];
    
    return true;
}

// Check if String is a valid IP address
bool isIp(char *str) 
{
    int segs = 0;
    int digcnt = 0; 
    int num = 0;
      
    while(*str != '\0') {
        
        if(*str == '.') {
            
            if(!digcnt || (++segs == 4))
                return false;
                
            num = digcnt = 0;
            str++;
            continue;
            
        } else if((*str < '0') || (*str > '9')) {
          
            return false;
            
        }

        if((num = (num * 10) + (*str - '0')) > 255)
            return false;

        digcnt++;
        str++;
    }

    return true;
}

// IPAddress to string
void ipToString(char *str, IPAddress ip) 
{
    sprintf(str, "%d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);
}

// String to IPAddress
IPAddress stringToIp(char *str)
{
    int ip1, ip2, ip3, ip4;
    
    sscanf(str, "%d.%d.%d.%d", &ip1, &ip2, &ip3, &ip4);
    
    return IPAddress(ip1, ip2, ip3, ip4);
}

/*
 * read parameter from server, for customhmtl input
 */
/* 
String getParam(String name)
{  
  String value;

  if(wm.server->hasArg(name)) {
      value = wm.server->arg(name);
  }
  
  value = wm.server->arg(FPSTR(S_gw));
  
  return value;
}
*/
