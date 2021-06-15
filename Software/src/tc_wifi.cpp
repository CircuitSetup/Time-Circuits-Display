/*
 * -------------------------------------------------------------------
 * CircuitSetup.us Time Circuits Display
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

WiFiManager wm;
WiFiManagerParameter beep_sound;
WiFiManagerParameter ntp_server("ntp_server", "NTP Time Server", "pool.ntp.org", 40);
WiFiManagerParameter dest_time_bright("dt_bright", "Destination Time Brightness", "", 15);
WiFiManagerParameter pres_time_bright("pt_bright", "Present Time Brightness", "", 15);
WiFiManagerParameter last_time_bright("lt_bright", "Last Time Departed Brightness", "", 15);

bool shouldSaveParams = false;

void wifi_setup() {
    WiFi.mode(WIFI_STA); // explicitly set mode, esp defaults to STA+AP

    wm.setCountry("US"); 
    wm.setParamsPage(true);

    const char* beep_sound_radio_str = "<p>Beep Sound</p>"
                                        "<input style='width: auto; margin: 0 10px 10px 10px;' type='radio' id='bs_yes' name='beep_sound' value='2'>"
                                        "<label for='bs_yes'>Yes</label><br>"
                                        "<input style='width: auto; margin: 0 10px 10px 10px;' type='radio' id='bs_no' name='beep_sound' value='1' checked>"
                                        "<label for='bs_no'>No</label><br><br>";
    new (&beep_sound) WiFiManagerParameter (beep_sound_radio_str); 

    std::vector<const char *> menu = {"wifi","info","param","sep","restart","exit","update"};
    wm.setMenu(menu);
    
    //reset settings - wipe credentials for testing
    //wm.resetSettings();
    wm.addParameter(&beep_sound);
    wm.addParameter(&ntp_server);
    wm.addParameter(&dest_time_bright);
    wm.addParameter(&pres_time_bright);
    wm.addParameter(&last_time_bright);
    wm.setConfigPortalBlocking(false);
    wm.setSaveParamsCallback(saveParamsCallback);

    //automatically connect using saved credentials if they exist
    //If connection fails it starts an access point with the specified name
    if(wm.autoConnect("TCD-AP")){
        Serial.println("connected...yeey :)");
        wm.startWebPortal(); //start config portal in STA mode
    }
    else {
        Serial.println("Config portal running");
    }
}

void wifi_loop() {
    wm.process();
    // put your main code here, to run repeatedly:
}

void saveParamsCallback() {
    Serial.println("Get Params:");
    Serial.print(beep_sound.getID());
    Serial.print(" : ");
    Serial.println(beep_sound.getValue());
    shouldSaveParams = true;
}

String getParam(String name){
  //read parameter from
  String value;
  if(wm.server->hasArg(name)) {
    value = wm.server->arg(name);
  }
  return value;
}