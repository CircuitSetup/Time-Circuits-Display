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

WiFiManagerParameter custom_ntpServer("ntp_server", "NTP Server (eg. 'pool.ntp.org')", settings.ntpServer, 63);
WiFiManagerParameter custom_timeZone("time_zone", "Timezone (<a href='https://github.com/nayarsystems/posix_tz_db/blob/master/zones.csv' target=_blank>Posix</a>, eg. 'CST6CDT,M3.2.0,M11.1.0')", settings.timeZone, 63);
WiFiManagerParameter custom_autoRotateTimes("rotate_times", "Auto Rotate Times (0=off, 1-5=5-60min)",settings.autoRotateTimes, 3);
WiFiManagerParameter custom_destTimeBright("dt_bright", "Destination Time Brightness (1-15)", settings.destTimeBright, 3);
WiFiManagerParameter custom_presTimeBright("pt_bright", "Present Time Brightness (1-15)", settings.presTimeBright, 3);
WiFiManagerParameter custom_lastTimeBright("lt_bright", "Last Time Departed Brightness (1-15)", settings.lastTimeBright, 3);
//WiFiManagerParameter custom_beepSound;

bool shouldSaveConfig = false;

/*
 * wifi_setup()
 * 
 */
void wifi_setup() 
{
    WiFi.mode(WIFI_STA);  // explicitly set mode, esp defaults to STA_AP

    //wm.setCountry("");                  // was US; not needed at all
    wm.setParamsPage(true);
    wm.setBreakAfterConfig(true);
    wm.setConfigPortalBlocking(false);
    wm.setSaveConfigCallback(saveConfigCallback);
    wm.setSaveParamsCallback(saveParamsCallback);
    wm.setHostname("TCD-Settings");

    wm.setConnectRetries(10);           
    wm.setConnectTimeout(15);           
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
    wm.addParameter(&custom_ntpServer);
    wm.addParameter(&custom_timeZone);
    wm.addParameter(&custom_autoRotateTimes);
    wm.addParameter(&custom_destTimeBright);
    wm.addParameter(&custom_presTimeBright);
    wm.addParameter(&custom_lastTimeBright);
 //   wm.addParameter(&custom_beepSound);

    //make sure the settings form has the correct values
    custom_ntpServer.setValue(settings.ntpServer, 63);
    custom_timeZone.setValue(settings.timeZone, 63);
    custom_autoRotateTimes.setValue(settings.autoRotateTimes, 3);
    custom_destTimeBright.setValue(settings.destTimeBright, 3);
    custom_presTimeBright.setValue(settings.presTimeBright, 3);
    custom_lastTimeBright.setValue(settings.lastTimeBright, 3);
 //   custom_beepSound.setValue(settings.beepSound, 3);

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