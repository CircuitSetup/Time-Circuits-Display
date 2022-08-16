/*
 * -------------------------------------------------------------------
 * CircuitSetup.us Time Circuits Display
 * Code adapted from Marmoset Electronics 
 * https://www.marmosetelectronics.com/time-circuits-clock
 * by John Monaco
 * Enhanced/modified in 2022 by Thomas Winischhofer (A10001986)
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
 * 
 */

#include "tc_wifi.h"

Settings settings;

WiFiManager wm;

WiFiManagerParameter custom_wifiConTimeout("wificon", "WiFi Connection Timeout in seconds (1-15)", settings.wifiConTimeout, 3);
WiFiManagerParameter custom_wifiConRetries("wifiret", "WiFi Connection Retries (1-15)", settings.wifiConRetries, 3);
WiFiManagerParameter custom_ntpServer("ntp_server", "NTP Server (eg. 'pool.ntp.org'; empty to disable NTP)", settings.ntpServer, 63);
WiFiManagerParameter custom_timeZone("time_zone", "Timezone (<a href='https://github.com/nayarsystems/posix_tz_db/blob/master/zones.csv' target=_blank>Posix</a>, eg. 'CST6CDT,M3.2.0,M11.1.0')", settings.timeZone, 63);
WiFiManagerParameter custom_mode24("md24", "Enable 24-hour clock mode: (0=12hr, 1=24hr)", settings.mode24, 2);
WiFiManagerParameter custom_ttrp("ttrp", "Make time travels persistent: (0=no, 1=yes)", settings.timesPers, 2);
WiFiManagerParameter custom_autoRotateTimes("rotate_times", "Time-rotation interval (0=off, 1-5=every 5th/15th/30th/45th/60th minute)",settings.autoRotateTimes, 3);
WiFiManagerParameter custom_destTimeBright("dt_bright", "Destination Time display brightness (1-15)", settings.destTimeBright, 3);
WiFiManagerParameter custom_presTimeBright("pt_bright", "Present Time display brightness (1-15)", settings.presTimeBright, 3);
WiFiManagerParameter custom_lastTimeBright("lt_bright", "Last Time Departed display brightness (1-15)", settings.lastTimeBright, 3);
#ifdef FAKE_POWER_ON
WiFiManagerParameter custom_fakePwrOn("fpo", "Enable fake power switch (0=no, 1=yes)", settings.fakePwrOn, 3);
#endif
WiFiManagerParameter custom_alarmRTC("artc", "Alarm base is RTC (1) or current present time (0)", settings.alarmRTC, 3);
//WiFiManagerParameter custom_beepSound;

bool shouldSaveConfig = false;

/*
 * wifi_setup()
 * 
 */
void wifi_setup() 
{ 
    int temp;
    
    WiFi.mode(WIFI_STA);  // explicitly set mode, esp defaults to STA_AP
    
    #ifndef TC_DBG
    wm.setDebugOutput(false);
    #endif

    //wm.setCountry("");                  // was US; not needed at all
    wm.setParamsPage(true);
    wm.setBreakAfterConfig(true);
    wm.setConfigPortalBlocking(false);
    wm.setSaveConfigCallback(saveConfigCallback);
    wm.setSaveParamsCallback(saveParamsCallback);
    wm.setHostname("TimeCircuits");

    temp = (int)atoi(settings.wifiConTimeout);
    if(temp < 0) temp = 0;
    if(temp > 15) temp = 15;
    wm.setConnectTimeout(temp);
    
    temp = (int)atoi(settings.wifiConRetries);
    if(temp < 0) temp = 0;
    if(temp > 15) temp = 15;
    wm.setConnectRetries(temp);           
               
    wm.setCleanConnect(true);           
    //wm.setRemoveDuplicateAPs(false);  

/*
    const char* beepSound_radio_str =
        "<p>Seconds Beep Sound (not yet implemented)</p>"
        "<input style='width: auto; margin: 0 10px 10px 10px;' type='radio' id='bs_yes' name='beepSound' value='1'>"
        "<label for='bs_yes'>Yes</label><br>"
        "<input style='width: auto; margin: 0 10px 10px 10px;' type='radio' id='bs_no' name='beepSound' value='0' checked>"
        "<label for='bs_no'>No</label><br><br>";
    new (&custom_beepSound) WiFiManagerParameter(beepSound_radio_str);
*/

    std::vector<const char*> menu = {"wifi", "info", "param", "sep", "restart", "exit", "update"};
    wm.setMenu(menu);

    //reset settings - wipe credentials for testing
    //wm.resetSettings();
    wm.addParameter(&custom_wifiConTimeout);
    wm.addParameter(&custom_wifiConRetries);
    wm.addParameter(&custom_ntpServer);
    wm.addParameter(&custom_timeZone);
    wm.addParameter(&custom_mode24);
    wm.addParameter(&custom_alarmRTC);
    wm.addParameter(&custom_ttrp);
    wm.addParameter(&custom_autoRotateTimes);
    wm.addParameter(&custom_destTimeBright);
    wm.addParameter(&custom_presTimeBright);
    wm.addParameter(&custom_lastTimeBright);
    #ifdef FAKE_POWER_ON    
    wm.addParameter(&custom_fakePwrOn);
    #endif    
 //   wm.addParameter(&custom_beepSound);

    updateConfigPortalValues();

    // Automatically connect using saved credentials if they exist
    // If connection fails it starts an access point with the specified name
    if(wm.autoConnect("TCD-AP")) {
        #ifdef TC_DBG
        Serial.println("WiFi connected...yeey :)");
        #endif
        wm.startWebPortal();  //start config portal in STA mode
    } else {
        #ifdef TC_DBG
        Serial.println("Config portal running");
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
    
    if(shouldSaveConfig) {
      
        // Save settings and restart esp32

        #ifdef TC_DBG
        Serial.println("WiFi: Saving config");
        #endif

        strcpy(settings.ntpServer, custom_ntpServer.getValue());
        strcpy(settings.timeZone, custom_timeZone.getValue());
        strcpy(settings.autoRotateTimes, custom_autoRotateTimes.getValue());                 
        strcpy(settings.destTimeBright, custom_destTimeBright.getValue());                
        strcpy(settings.presTimeBright, custom_presTimeBright.getValue());        
        strcpy(settings.lastTimeBright, custom_lastTimeBright.getValue());                  
        //strcpy(settings.beepSound, getParam("custom_beepSound"));
        strcpy(settings.wifiConRetries, custom_wifiConRetries.getValue()); 
        strcpy(settings.wifiConTimeout, custom_wifiConTimeout.getValue()); 
        strcpy(settings.mode24, custom_mode24.getValue()); 
        strcpy(settings.timesPers, custom_ttrp.getValue()); 
        #ifdef FAKE_POWER_ON      
        strcpy(settings.fakePwrOn, custom_fakePwrOn.getValue()); 
        #endif        
        strcpy(settings.alarmRTC, custom_alarmRTC.getValue()); 

        write_settings();        

        shouldSaveConfig = false;

        // Reset esp32 to load new settings

        #ifdef TC_DBG
        Serial.println("WiFi: Restarting ESP....");
        #endif
        
        esp_restart();
    }
}

void saveConfigCallback() 
{
    //Serial.println("Should save config");
    shouldSaveConfig = true;
}

void saveParamsCallback() 
{
    //Serial.println("Should save params config");
    shouldSaveConfig = true;
    wm.stopConfigPortal();
}

void updateConfigPortalValues()
{
    // Make sure the settings form has the correct values
    custom_wifiConTimeout.setValue(settings.wifiConTimeout, 3);
    custom_wifiConRetries.setValue(settings.wifiConRetries, 3);
    custom_ntpServer.setValue(settings.ntpServer, 63);
    custom_timeZone.setValue(settings.timeZone, 63);
    custom_mode24.setValue(settings.mode24, 2);
    custom_ttrp.setValue(settings.timesPers, 2);
    custom_autoRotateTimes.setValue(settings.autoRotateTimes, 3);
    custom_destTimeBright.setValue(settings.destTimeBright, 3);
    custom_presTimeBright.setValue(settings.presTimeBright, 3);
    custom_lastTimeBright.setValue(settings.lastTimeBright, 3);
    #ifdef FAKE_POWER_ON 
    custom_fakePwrOn.setValue(settings.fakePwrOn, 2);
    #endif    
    custom_alarmRTC.setValue(settings.alarmRTC, 2);
    //custom_beepSound.setValue(settings.beepSound, 3);
}

/*
int wifi_getmode()
{
  WiFiMode_t mymode = WiFi.getMode();

  switch(mymode) {
    case WIFI_STA:
        return 1;
    case WIFI_AP:
        return 2;
  }
   
  return 0;  
}
*/

bool wifi_getIP(uint8_t& a, uint8_t& b, uint8_t& c, uint8_t& d)
{
  IPAddress myip = WiFi.localIP();

  a = myip[0];
  b = myip[1];
  c = myip[2];
  d = myip[3];
  
  return true;
}

/*
 * read parameter from server, for customhmtl input
 * /
String getParam(String name){  
  String value;
  if(wm.server->hasArg(name)) {
    value = wm.server->arg(name);
  }
  return value;
}
*/
