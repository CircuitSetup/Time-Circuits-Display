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

#include "tc_settings.h"

void settings_setup() {
    //read configuration from FS json
    Serial.println("mounting FS...");

    if (SPIFFS.begin()) {
        Serial.println("mounted file system");
        if (SPIFFS.exists("/config.json")) {
            //file exists, reading and loading
            Serial.println("reading config file");
            File configFile = SPIFFS.open("/config.json", "r");
            if (configFile) {
                Serial.println("opened config file");

                StaticJsonDocument<1024> json;
                DeserializationError error = deserializeJson(json, configFile);

                serializeJson(json, Serial);
                if (!error) {
                    Serial.println("\nparsed json");
                    strcpy(settings.ntpServer, json["ntpServer"]);
                    strcpy(settings.gmtOffset, json["gmtOffset"]);
                    strcpy(settings.daylightOffset, json["daylightOffset"]);
                    strcpy(settings.autoRotateTimes, json["autoRotateTimes"]);
                    strcpy(settings.destTimeBright, json["destTimeBright"]);
                    strcpy(settings.presTimeBright, json["presTimeBright"]);
                    strcpy(settings.lastTimeBright, json["lastTimeBright"]);
                    //strcpy(settings.beepSound, json["beepSound"]);
                } else {
                    Serial.println("failed to load json config");
                }
                configFile.close();
            }
        } else {
            //config file does not exist - create it with default values
            Serial.println("write new JSON file");
            StaticJsonDocument<1024> json;

            json["ntpServer"] = settings.ntpServer;
            json["gmtOffset"] = settings.gmtOffset;
            json["daylightOffset"] = settings.daylightOffset;
            json["autoRotateTimes"] = settings.autoRotateTimes;
            json["destTimeBright"] = settings.destTimeBright;
            json["presTimeBright"] = settings.presTimeBright;
            json["lastTimeBright"] = settings.lastTimeBright;
            json["beepSound"] = settings.beepSound;

            File configFile = SPIFFS.open("/config.json", FILE_WRITE);

            serializeJson(json, Serial);
            serializeJson(json, configFile);

            configFile.close();
        }
    } else {
        Serial.println("failed to mount FS");
    }
    //end read
}

