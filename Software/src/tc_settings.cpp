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

#include "tc_settings.h"

/*
 * Read configuration from JSON config file
 * If config file not found, create one with default settings
 */

bool haveFS = false;

/*
 * settings_setup()
 * 
 */
void settings_setup() 
{
  bool writedefault = false;

  #ifdef TC_DBG
  Serial.println("settings_setup: Mounting FS...");
  #endif

  if(SPIFFS.begin()) {

    #ifdef TC_DBG
    Serial.println("settings_setup: Mounted file system");
    #endif
    
    haveFS = true;
    
    if(SPIFFS.exists("/config.json")) {
      
      //file exists, load and parse it     
      File configFile = SPIFFS.open("/config.json", "r");
      
      if (configFile) {

        #ifdef TC_DBG
        Serial.println("settings_setup: Opened config file");
        #endif

        StaticJsonDocument<1024> json;
        DeserializationError error = deserializeJson(json, configFile);

        #ifdef TC_DBG
        serializeJson(json, Serial);
        #endif
        
        if (!error) {

          #ifdef TC_DBG
          Serial.println("\nsettings_setup: Parsed json");
          #endif
          
          if(json["ntpServer"]) {
              strcpy(settings.ntpServer, json["ntpServer"]);
          } else writedefault = true;          
          if(json["timeZone"]) {
              strcpy(settings.timeZone, json["timeZone"]);
          } else writedefault = true;
          if(json["autoRotateTimes"]) {
              strcpy(settings.autoRotateTimes, json["autoRotateTimes"]);
          } else writedefault = true;
          if(json["destTimeBright"]) {
              strcpy(settings.destTimeBright, json["destTimeBright"]);
          } else writedefault = true;
          if(json["presTimeBright"]) {
              strcpy(settings.presTimeBright, json["presTimeBright"]);
          } else writedefault = true;
          if(json["lastTimeBright"]) {
              strcpy(settings.lastTimeBright, json["lastTimeBright"]);
          } else writedefault = true;
          //if(json["beepSound"]) {
          //  strcpy(settings.beepSound, json["beepSound"]);
          //} else writedefault = true;
          
        } else {
          
          Serial.println("settings_setup: Failed to load json config");
          
        }
        
        configFile.close();
      }
      
    } else {

      writedefault = true;
      
    }

    if(writedefault) {
      
      // config file does not exist or is incomplete - create one 
      
      Serial.println("settings_setup: Settings missing or inomplete; writing new config file");
      
      write_settings();
      
    }
    
  } else {
    
    Serial.println("settings_setup: Failed to mount FS");
  
  }
}

void write_settings() 
{
  StaticJsonDocument<1024> json;

  if(!haveFS) {
    Serial.println("write_settings: Cannot write settings, FS not mounted");
    return;
  } 

  #ifdef TC_DBG
  Serial.println("write_settings: Writing config file");
  #endif
  
  json["ntpServer"] = settings.ntpServer;
  json["timeZone"] = settings.timeZone;
  json["autoRotateTimes"] = settings.autoRotateTimes;
  json["destTimeBright"] = settings.destTimeBright;
  json["presTimeBright"] = settings.presTimeBright;
  json["lastTimeBright"] = settings.lastTimeBright;
  //json["beepSound"] = settings.beepSound;

  File configFile = SPIFFS.open("/config.json", FILE_WRITE);

  #ifdef TC_DBG
  serializeJson(json, Serial);
  Serial.println("\n");
  #endif
  
  serializeJson(json, configFile);

  configFile.close(); 
  
}