/*
 * -------------------------------------------------------------------
 * CircuitSetup.us Time Circuits Display
 * 
 * Code based on Marmoset Electronics 
 * https://www.marmosetelectronics.com/time-circuits-clock
 * by John Monaco
 *
 * Enhanced/modified/written in 2022 by Thomas Winischhofer (A10001986)
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
 * Mount SPIFF, und SD (if available)
 * 
 * Read configuration from JSON config file
 * If config file not found, create one with default settings
 */

/* If SPIFFS is mounted */
bool haveFS = false;

/* If a SD card is found */
bool haveSD = false;

/*
 * settings_setup()
 * 
 */
void settings_setup() 
{
    bool writedefault = false;
  
    #ifdef TC_DBG
    Serial.println("settings_setup: Mounting SD...");
    #endif

    EEPROM.begin(512);
  
    // set up SD card
    SPI.begin(SPI_SCK_PIN, SPI_MISO_PIN, SPI_MOSI_PIN);

    haveSD = false;
    
    if (!SD.begin(SD_CS_PIN)) {

        #ifdef TC_DBG
        Serial.println("No SD card found");
        #endif
    
    } else {
      
        #ifdef TC_DBG
        Serial.println("SD card initialized");
        #endif

        uint8_t cardType = SD.cardType();
        if(cardType == CARD_NONE) {
          
            Serial.println("No SD card inserted");
        
        } else {
          
            #ifdef TC_DBG
            Serial.print("SD card type ");
            Serial.println(cardType, DEC);
            #endif
            
            haveSD = true;
            
        }
    }

    #ifdef TC_DBG
    Serial.println("settings_setup: Mounting SPIFFS...");
    #endif

    if(SPIFFS.begin()) {
  
        #ifdef TC_DBG
        Serial.println("settings_setup: Mounted SPIFFS");
        #endif
        
        haveFS = true;
        
        if(SPIFFS.exists("/config.json")) {
             
            File configFile = SPIFFS.open("/config.json", "r");
            
            if(configFile) {
      
                #ifdef TC_DBG
                Serial.println("settings_setup: Opened config file");
                #endif
        
                StaticJsonDocument<1024> json;
                DeserializationError error = deserializeJson(json, configFile);
        
                #ifdef TC_DBG
                serializeJson(json, Serial);
                Serial.println(" ");
                #endif
                
                if(!error) {
        
                    #ifdef TC_DBG
                    Serial.println("settings_setup: Parsed json");
                    #endif
                    
                    if(json["ntpServer"]) {
                        strcpy(settings.ntpServer, json["ntpServer"]);
                    } else writedefault = true;          
                    if(json["timeZone"]) {
                        strcpy(settings.timeZone, json["timeZone"]);
                    } else writedefault = true;
                    if(json["autoRotateTimes"]) {
                        strcpy(settings.autoRotateTimes, json["autoRotateTimes"]);
                        writedefault |= checkValidNumParm(settings.autoRotateTimes, 0, 5, 0);
                    } else writedefault = true;
                    if(json["destTimeBright"]) {
                        strcpy(settings.destTimeBright, json["destTimeBright"]);
                        writedefault |= checkValidNumParm(settings.destTimeBright, 0, 15, 15);
                    } else writedefault = true;
                    if(json["presTimeBright"]) {
                        strcpy(settings.presTimeBright, json["presTimeBright"]);
                        writedefault |= checkValidNumParm(settings.presTimeBright, 0, 15, 15);
                    } else writedefault = true;
                    if(json["lastTimeBright"]) {
                        strcpy(settings.lastTimeBright, json["lastTimeBright"]);
                        writedefault |= checkValidNumParm(settings.lastTimeBright, 0, 15, 15);
                    } else writedefault = true;                    
                    if(json["wifiConRetries"]) {
                        strcpy(settings.wifiConRetries, json["wifiConRetries"]);
                        writedefault |= checkValidNumParm(settings.wifiConRetries, 1, 15, 3);
                    } else writedefault = true;
                    if(json["wifiConTimeout"]) {
                        strcpy(settings.wifiConTimeout, json["wifiConTimeout"]);
                        writedefault |= checkValidNumParm(settings.wifiConTimeout, 1, 15, 7);
                    } else writedefault = true;
                    if(json["mode24"]) {
                        strcpy(settings.mode24, json["mode24"]);
                        writedefault |= checkValidNumParm(settings.mode24, 0, 1, 0);
                    } else writedefault = true;
                    if(json["timeTrPers"]) {
                        strcpy(settings.timesPers, json["timeTrPers"]);
                        writedefault |= checkValidNumParm(settings.timesPers, 0, 1, 1);
                    } else writedefault = true;
                    #ifdef FAKE_POWER_ON
                    if(json["fakePwrOn"]) {
                        strcpy(settings.fakePwrOn, json["fakePwrOn"]);
                        writedefault |= checkValidNumParm(settings.fakePwrOn, 0, 1, 0);
                    } else writedefault = true;
                    #endif
                    if(json["alarmRTC"]) {
                        strcpy(settings.alarmRTC, json["alarmRTC"]);
                        writedefault |= checkValidNumParm(settings.alarmRTC, 0, 1, 1);
                    } else writedefault = true;
                    if(json["playIntro"]) {
                        strcpy(settings.playIntro, json["playIntro"]);
                        writedefault |= checkValidNumParm(settings.playIntro, 0, 1, 1);
                    } else writedefault = true;
                    //if(json["beepSound"]) {
                    //  strcpy(settings.beepSound, json["beepSound"]);
                    //} else writedefault = true;
                  
                } else {
                  
                    Serial.println("settings_setup: Failed to parse settings file");
          
                    writedefault = true;
                  
                }
              
              configFile.close();
            }
          
        } else {
    
            writedefault = true;
          
        }
    
        if(writedefault) {
          
            // config file does not exist or is incomplete - create one 
            
            Serial.println("settings_setup: Settings missing or incomplete; writing new file");
            
            write_settings();
          
        }
      
    } else {
      
        Serial.println("settings_setup: Failed to mount SPIFFS");
    
    }
}

void write_settings() 
{
    StaticJsonDocument<1024> json;
  
    if(!haveFS) {
        Serial.println("write_settings: Cannot write settings, SPIFFS not mounted");
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
    json["wifiConRetries"] = settings.wifiConRetries;
    json["wifiConTimeout"] = settings.wifiConTimeout;
    json["mode24"] = settings.mode24;
    json["timeTrPers"] = settings.timesPers;
    #ifdef FAKE_POWER_ON    
    json["fakePwrOn"] = settings.fakePwrOn;    
    #endif
    json["alarmRTC"] = settings.alarmRTC;
    json["playIntro"] = settings.playIntro;
    //json["beepSound"] = settings.beepSound;
  
    File configFile = SPIFFS.open("/config.json", FILE_WRITE);
  
    #ifdef TC_DBG
    serializeJson(json, Serial);
    Serial.println(" ");
    #endif
    
    if(configFile) {
        serializeJson(json, configFile);
        configFile.close(); 
    } else {
        Serial.println("write_settings: Failed to open file for writing");
    }
}

bool checkValidNumParm(char *text, int lowerLim, int upperLim, int setDefault)
{
    int i, len = strlen(text);

    if(len == 0) {
        sprintf(text, "%d", setDefault);
        return true;
    }

    for(i = 0; i < len; i++) {
        if(text[i] < '0' || text[i] > '9') {
            sprintf(text, "%d", setDefault);
            return true;
        }        
    }
    
    i = (int)(atoi(text));
    
    if(i < lowerLim) {
        sprintf(text, "%d", lowerLim);
        return true;
    }
    if(i > upperLim) {
        sprintf(text, "%d", upperLim);
        return true;
    }

    return false;
}
    
/* 
 *  Load the Alarm time and settings from alarmconfig
 *  
 */

bool loadAlarm()
{
    bool writedefault = false;
    
    if(!haveFS) {
      
        Serial.println("loadAlarm(): SPIFFS not mounted, using EEPROM");
        
        return loadAlarmEEPROM();
        
    } 

    if(SPIFFS.exists("/alarmconfig.json")) {
      
        File configFile = SPIFFS.open("/alarmconfig.json", "r");
        
        if(configFile) {

            #ifdef TC_DBG
            Serial.println("loadAlarm: Opened alarmconfig file");
            #endif

            StaticJsonDocument<1024> json;
            DeserializationError error = deserializeJson(json, configFile);

            #ifdef TC_DBG
            serializeJson(json, Serial);
            Serial.println(" ");
            #endif
            
            if(!error) {

                #ifdef TC_DBG
                Serial.println("loadAlarm: Parsed json");
                #endif
                
                if(json["alarmonoff"]) {
                    alarmOnOff = (atoi(json["alarmonoff"]) != 0) ? true : false;              
                } else {
                    writedefault = true;                            
                }
                if(json["alarmhour"]) {
                    alarmHour = atoi(json["alarmhour"]);              
                } else {
                    writedefault = true;                    
                }
                if(json["alarmmin"]) {
                    alarmMinute = atoi(json["alarmmin"]); 
                } else {
                    writedefault = true;                                        
                }
              
            } else {
              
                Serial.println("loadAlarm: Failed to parse alarm settings file");

                writedefault = true;
              
            }
            
            configFile.close();
        }
      
    } else {

        writedefault = true;
      
    }

    if(writedefault) {
        
        // alarmconfig file does not exist or is incomplete - create one 
        
        Serial.println("loadAlarm: Alarm settings missing or incomplete; writing new file");
        
        saveAlarm();
      
    }    

    return true;
}

bool loadAlarmEEPROM() 
{
    #ifdef TC_DBG
    Serial.println("Load Alarm EEPROM");
    #endif

    uint8_t loadBuf[4];    // on/off, hour, minute, checksum
    uint16_t sum = 0;
    int i;    
        
    for(i = 0; i < 4; i++) {
        loadBuf[i] = EEPROM.read(ALARM_PREF + i);
        if(i < 3) sum += (loadBuf[i] ^ 0x55);
    }

    if((sum & 0xff) != loadBuf[3]) {
          
        Serial.println("loadAlarm: Invalid alarm data in EEPROM");

        alarmOnOff = false;
        alarmHour = alarmMinute = 255;    // means "unset"
    
        return false;
    }        

    alarmOnOff = loadBuf[0] ? true : false;
    alarmHour = loadBuf[1];
    alarmMinute = loadBuf[2];

    return true;
}

void saveAlarm() 
{
    char hourBuf[8];
    char minBuf[8];
    
    StaticJsonDocument<1024> json;
    
    #ifdef TC_DBG
    Serial.println("Save Alarm");
    #endif

    if(!haveFS) {
        Serial.println("saveAlarm(): SPIFFS not mounted, using EEPROM");
        
        saveAlarmEEPROM();
        
        return;        
    } 
  
    json["alarmonoff"] = (char *)(alarmOnOff ? "1" : "0");

    sprintf(hourBuf, "%d", alarmHour);
    sprintf(minBuf, "%d", alarmMinute);
    
    json["alarmhour"] = (char *)hourBuf;
    json["alarmmin"] = (char *)minBuf;

    File configFile = SPIFFS.open("/alarmconfig.json", FILE_WRITE);
  
    #ifdef TC_DBG
    serializeJson(json, Serial);
    Serial.println(" ");
    #endif

    if(configFile) {        
        serializeJson(json, configFile);        
        configFile.close();         
    } else {
        Serial.println("saveAlarm: Failed to open alarm settings file for writing");
    }
}    

void saveAlarmEEPROM()
{
    uint8_t savBuf[4];    // on/off, hour, minute, checksum
    uint16_t sum = 0;
    int i;    

    savBuf[0] = alarmOnOff ? 1 : 0;
    savBuf[1] = alarmHour;
    savBuf[2] = alarmMinute;
        
    for(i = 0; i < 3; i++) {
        EEPROM.write(ALARM_PREF + i, savBuf[i]);
        sum += (savBuf[i] ^ 0x55);
    }
    EEPROM.write(ALARM_PREF + 3, (sum & 0xff));   
    
    EEPROM.commit();
}
