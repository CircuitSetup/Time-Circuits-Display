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
 * Mount SPIFFS/LittleFS, und SD (if available)
 * 
 * Read configuration from JSON config file
 * If config file not found, create one with default settings
 * 
 * Read IP settings if a static IP is configured. If the device
 * is powered on or reset while ENTER is held down, the IP
 * settings file will be deleted and the device will use DHCP.
 * 
 */

/* If SPIFFS/LittleFS is mounted */
bool haveFS = false;

/* If a SD card is found */
bool haveSD = false;

/* If SD contains all default audio files */
bool allowCPA = false;

#define NUM_AUDIOFILES 11
const char *audioFiles[NUM_AUDIOFILES] = {
      "/alarm.mp3\0", 
      "/alarmoff.mp3\0", 
      "/alarmon.mp3\0",
      "/baddate.mp3\0",
      "/enter.mp3\0",
      "/intro.mp3\0",
      "/nmoff.mp3\0",
      "/nmon.mp3\0",
      "/startup.mp3\0",
      "/timetravel.mp3\0",
      "/travelstart.mp3\0"
};

/*
 * settings_setup()
 * 
 */
void settings_setup() 
{
    bool writedefault = false;

    // Pre-maturely use ENTER button (initialized again in keypad_setup())
    pinMode(ENTER_BUTTON_PIN, INPUT_PULLUP);
    delay(20);
  
    #ifdef TC_DBG
    Serial.println(F("settings_setup: Mounting SD..."));
    #endif

    EEPROM.begin(512);
  
    // set up SD card
    SPI.begin(SPI_SCK_PIN, SPI_MISO_PIN, SPI_MOSI_PIN);

    haveSD = false;
    
    if (!SD.begin(SD_CS_PIN)) {

        #ifdef TC_DBG
        Serial.println(F("No SD card found"));
        #endif
    
    } else {
      
        #ifdef TC_DBG
        Serial.println(F("SD card initialized"));
        #endif

        uint8_t cardType = SD.cardType();
        if(cardType == CARD_NONE) {
          
            Serial.println(F("No SD card inserted"));
        
        } else {
          
            #ifdef TC_DBG
            Serial.print(F("SD card type "));
            Serial.println(cardType, DEC);
            #endif
            
            haveSD = true;
            
        }
    }

    #ifdef TC_DBG
    Serial.println(F("settings_setup: Mounting flash FS..."));
    #endif

    if(SPIFFS.begin()) {
  
        #ifdef TC_DBG
        Serial.println(F("settings_setup: Mounted flash FS"));
        #endif

        haveFS = true;

        // Allow user to delete static IP data by holding ENTER 
        // while booting
        if(digitalRead(ENTER_BUTTON_PIN)) {
          
            Serial.println(F("settings_setup: ENTER pressed - deleting ipconfig file"));
          
            deleteIpSettings();

            // Pre-maturely use white led (initialized again in keypad_setup())
            pinMode(WHITE_LED_PIN, OUTPUT);
            digitalWrite(WHITE_LED_PIN, HIGH);
            while(digitalRead(ENTER_BUTTON_PIN)) { }
            digitalWrite(WHITE_LED_PIN, LOW);                         
        }

        // Check if SD contains our default sound files
        if(haveSD) {
            allowCPA = check_if_default_audio_present();            
        }

        // Read our main config file
        if(SPIFFS.exists("/config.json")) {
             
            File configFile = SPIFFS.open("/config.json", "r");
            
            if(configFile) {
      
                #ifdef TC_DBG
                Serial.println(F("settings_setup: Opened config file"));
                #endif
        
                StaticJsonDocument<1024> json;
                DeserializationError error = deserializeJson(json, configFile);
        
                #ifdef TC_DBG
                serializeJson(json, Serial);
                Serial.println(F(" "));
                #endif
                
                if(!error) {
        
                    #ifdef TC_DBG
                    Serial.println(F("settings_setup: Parsed json"));
                    #endif
                    
                    if(json["ntpServer"]) {
                        strcpy(settings.ntpServer, json["ntpServer"]);
                    } else writedefault = true;          
                    if(json["timeZone"]) {
                        strcpy(settings.timeZone, json["timeZone"]);
                    } else writedefault = true;
                    if(json["autoRotateTimes"]) {
                        strcpy(settings.autoRotateTimes, json["autoRotateTimes"]);
                        writedefault |= checkValidNumParm(settings.autoRotateTimes, 0, 5, DEF_AUTOROTTIMES);
                    } else writedefault = true;
                    if(json["destTimeBright"]) {
                        strcpy(settings.destTimeBright, json["destTimeBright"]);
                        writedefault |= checkValidNumParm(settings.destTimeBright, 0, 15, DEF_BRIGHT_DEST);
                    } else writedefault = true;
                    if(json["presTimeBright"]) {
                        strcpy(settings.presTimeBright, json["presTimeBright"]);
                        writedefault |= checkValidNumParm(settings.presTimeBright, 0, 15, DEF_BRIGHT_PRES);
                    } else writedefault = true;
                    if(json["lastTimeBright"]) {
                        strcpy(settings.lastTimeBright, json["lastTimeBright"]);
                        writedefault |= checkValidNumParm(settings.lastTimeBright, 0, 15, DEF_BRIGHT_DEPA);
                    } else writedefault = true;                    
                    if(json["wifiConRetries"]) {
                        strcpy(settings.wifiConRetries, json["wifiConRetries"]);
                        writedefault |= checkValidNumParm(settings.wifiConRetries, 1, 15, DEF_WIFI_RETRY);
                    } else writedefault = true;
                    if(json["wifiConTimeout"]) {
                        strcpy(settings.wifiConTimeout, json["wifiConTimeout"]);
                        writedefault |= checkValidNumParm(settings.wifiConTimeout, 1, 15, DEF_WIFI_TIMEOUT);
                    } else writedefault = true;
                    if(json["mode24"]) {
                        strcpy(settings.mode24, json["mode24"]);
                        writedefault |= checkValidNumParm(settings.mode24, 0, 1, DEF_MODE24);
                    } else writedefault = true;
                    if(json["timeTrPers"]) {
                        strcpy(settings.timesPers, json["timeTrPers"]);
                        writedefault |= checkValidNumParm(settings.timesPers, 0, 1, DEF_TIMES_PERS);
                    } else writedefault = true;
                    #ifdef FAKE_POWER_ON
                    if(json["fakePwrOn"]) {
                        strcpy(settings.fakePwrOn, json["fakePwrOn"]);
                        writedefault |= checkValidNumParm(settings.fakePwrOn, 0, 1, DEF_FAKE_PWR);
                    } else writedefault = true;
                    #endif
                    if(json["alarmRTC"]) {
                        strcpy(settings.alarmRTC, json["alarmRTC"]);
                        writedefault |= checkValidNumParm(settings.alarmRTC, 0, 1, DEF_ALARM_RTC);
                    } else writedefault = true;
                    if(json["playIntro"]) {
                        strcpy(settings.playIntro, json["playIntro"]);
                        writedefault |= checkValidNumParm(settings.playIntro, 0, 1, DEF_PLAY_INTRO);
                    } else writedefault = true;
                  
                } else {
                  
                    Serial.println(F("settings_setup: Failed to parse settings file"));
          
                    writedefault = true;
                  
                }
              
                configFile.close();
              
            } else {

                writedefault = true;
                
            }
          
        } else {
    
            writedefault = true;
          
        }
    
        if(writedefault) {
          
            // config file does not exist or is incomplete - create one 
            
            Serial.println(F("settings_setup: Settings missing or incomplete; writing new file"));
            
            write_settings();
          
        }
      
    } else {
      
        Serial.println(F("settings_setup: Failed to mount flash FS"));
    
    }
}

void write_settings() 
{
    StaticJsonDocument<1024> json;
  
    if(!haveFS) {
        Serial.println(F("write_settings: Cannot write settings, flash FS not mounted"));
        return;
    } 
  
    #ifdef TC_DBG
    Serial.println(F("write_settings: Writing config file"));
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
  
    File configFile = SPIFFS.open("/config.json", FILE_WRITE);
  
    #ifdef TC_DBG
    serializeJson(json, Serial);
    Serial.println(F(" "));
    #endif
    
    if(configFile) {
        serializeJson(json, configFile);
        configFile.close(); 
    } else {
        Serial.println(F("write_settings: Failed to open file for writing"));
    }
}

// Helper for checking validity of numerical user-entered parameters
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
 */

bool loadAlarm()
{
    bool writedefault = false;
    
    if(!haveFS) {
      
        Serial.println(F("loadAlarm(): flash FS not mounted, using EEPROM"));
        
        return loadAlarmEEPROM();
        
    } 

    if(SPIFFS.exists("/alarmconfig.json")) {
      
        File configFile = SPIFFS.open("/alarmconfig.json", "r");
        
        if(configFile) {

            #ifdef TC_DBG
            Serial.println(F("loadAlarm: Opened alarmconfig file"));
            #endif

            StaticJsonDocument<1024> json;
            DeserializationError error = deserializeJson(json, configFile);

            #ifdef TC_DBG
            serializeJson(json, Serial);
            Serial.println(F(" "));
            #endif
            
            if(!error) {

                #ifdef TC_DBG
                Serial.println(F("loadAlarm: Parsed json"));
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
              
                Serial.println(F("loadAlarm: Failed to parse alarm settings file"));

                writedefault = true;
              
            }
            
            configFile.close();
            
        } else {
          
            writedefault = true;
            
        }
      
    } else {

        writedefault = true;
      
    }

    if(writedefault) {
        
        // alarmconfig file does not exist or is incomplete - create one 
        
        Serial.println(F("loadAlarm: Alarm settings missing or incomplete; writing new file"));
        
        saveAlarm();
      
    }    

    return true;
}

bool loadAlarmEEPROM() 
{
    #ifdef TC_DBG
    Serial.println(F("Load Alarm EEPROM"));
    #endif

    uint8_t loadBuf[4];    // on/off, hour, minute, checksum
    uint16_t sum = 0;
    int i;    
        
    for(i = 0; i < 4; i++) {
        loadBuf[i] = EEPROM.read(ALARM_PREF + i);
        if(i < 3) sum += (loadBuf[i] ^ 0x55);
    }

    if((sum & 0xff) != loadBuf[3]) {
          
        Serial.println(F("loadAlarm: Invalid alarm data in EEPROM"));

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
    Serial.println(F("Save Alarm"));
    #endif

    if(!haveFS) {
        Serial.println(F("saveAlarm(): flash FS not mounted, using EEPROM"));
        
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
    Serial.println(F(" "));
    #endif

    if(configFile) {        
        serializeJson(json, configFile);        
        configFile.close();         
    } else {
        Serial.println(F("saveAlarm: Failed to open alarm settings file for writing"));
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

/*
 * Load/save/delete settings for static IP configuration
 */

bool loadIpSettings()
{
    bool invalid = false;
    bool haveConfig = false;
    
    if(!haveFS) {
        return false;
    }
    
    if(SPIFFS.exists("/ipconfig.json")) {
             
        File configFile = SPIFFS.open("/ipconfig.json", "r");
            
        if(configFile) {
  
            #ifdef TC_DBG
            Serial.println(F("loadIpSettings: Opened ip config file"));
            #endif
    
            StaticJsonDocument<1024> json;
            DeserializationError error = deserializeJson(json, configFile);
    
            #ifdef TC_DBG
            serializeJson(json, Serial);
            Serial.println(F(" "));
            #endif
            
            if(!error) {
    
                #ifdef TC_DBG
                Serial.println(F("loadIpSettings: Parsed json"));
                #endif
                
                if(json["IpAddress"]) {
                    strcpy(ipsettings.ip, json["IpAddress"]);
                } else invalid = true;   
                if(json["Gateway"]) {
                    strcpy(ipsettings.gateway, json["Gateway"]);
                } else invalid = true;   
                if(json["Netmask"]) {
                    strcpy(ipsettings.netmask, json["Netmask"]);
                } else invalid = true; 
                if(json["DNS"]) {
                    strcpy(ipsettings.dns, json["DNS"]);
                } else invalid = true; 

                if(!invalid) haveConfig = true;

            } else {
              
                Serial.println(F("loadIpSettings: Failed to parse file"));

                invalid = true;
              
            }
            
            configFile.close();

        } 

    }
    
    if(invalid) {
      
        // config file is invalid - delete it
        
        Serial.println(F("loadIpSettings: IP settings invalid; deleting file"));
        
        deleteIpSettings();

        ipsettings.ip[0] = '\0';
        ipsettings.gateway[0] = '\0';
        ipsettings.netmask[0] = '\0';
        ipsettings.dns[0] = '\0';
     
    }

    return haveConfig;
}

void writeIpSettings()
{
    StaticJsonDocument<1024> json;
  
    if(!haveFS) {
        Serial.println(F("writeIpSettings: Cannot write ip settings, flash FS not mounted"));
        return;
    } 
  
    #ifdef TC_DBG
    Serial.println(F("writeIpSettings: Writing ipconfig file"));
    #endif
    
    json["IpAddress"] = ipsettings.ip;
    json["Gateway"] = ipsettings.gateway;
    json["Netmask"] = ipsettings.netmask;
    json["DNS"] = ipsettings.dns;

    File configFile = SPIFFS.open("/ipconfig.json", FILE_WRITE);
  
    #ifdef TC_DBG
    serializeJson(json, Serial);
    Serial.println(F(" "));
    #endif
    
    if(configFile) {
        serializeJson(json, configFile);
        configFile.close(); 
    } else {
        Serial.println(F("writeIpSettings: Failed to open file for writing"));
    }
}

void deleteIpSettings()
{
    if(!haveFS) {
        return;
    }

    #ifdef TC_DBG
    Serial.println(F("deleteIpSettings: Deleting ipconfig.json"));
    #endif

    SPIFFS.remove("/ipconfig.json");
}

/*
 * Audio file installer 
 * 
 * Copies our default audio files from SD to flash FS.
 * The is restricted to the original default audio
 * files that came with the software. If you want to
 * customize your sounds, put them on a FAT formatted
 * SD card and leave this SD card in the slot.
 */

bool check_allow_CPA()
{
    return allowCPA;
}

bool check_if_default_audio_present()
{
    File file;
    size_t ts;
    int i, idx = 0;    
    char dtmf_buf[16] = "/Dtmf-0.mp3\0";
    size_t sizes[10+NUM_AUDIOFILES] = {
#ifndef TWSOUND
      4178, 4178, 4178, 4178, 4178, 4178, 3760, 3760, 4596, 3760, // DTMF
      70664, 71500, 60633, 10478,   // alarm, alarmoff, alarmon, baddate
      13374, 125804, 33853, 47228,  // enter, intro, nmoff, nmon
      21907, 38899, 135447          // startup, timetravel, travelstart
#else      
      4178, 4178, 4178, 4178, 4178, 4178, 3760, 3760, 4596, 3760, //DTMF
      70664, 71500, 60633, 10478,   // alarm, alarmoff, alarmon, baddate
      12149, 125804, 33853, 47228,  // enter, intro, nmoff, nmon
      18419, 38899, 135447          // startup, timetravel, travelstart
#endif      
    };
    
    if(!haveSD)       
        return false;

    // If identifier missing, quit now
    if(!(SD.exists("/TCD_def_snd.txt"))) {
        Serial.println("SD ID missing");    
        return false;
    }

    for(i = 0; i < 10; i++) {
        dtmf_buf[6] = i + '0';
        if(!(SD.exists(dtmf_buf))) {
            #ifdef TC_DBG
            Serial.print("missing: ");
            Serial.println(dtmf_buf);   
            #endif
            return false;
        }
        if(!(file = SD.open(dtmf_buf)))
            return false;             
        ts = file.size();
        file.close();
        #ifdef TC_DBG
        sizes[idx++] = ts;
        #else
        if(sizes[idx++] != ts)
            return false;
        #endif
    }

    for(i = 0; i < NUM_AUDIOFILES; i++) {     
        if(!SD.exists(audioFiles[i])) {
            #ifdef TC_DBG
            Serial.print("missing: ");
            Serial.println(audioFiles[i]);   
            #endif
            return false;
        }
        if(!(file = SD.open(audioFiles[i])))
            return false;             
        ts = file.size();
        file.close();
        #ifdef TC_DBG
        sizes[idx++] = ts;
        #else
        if(sizes[idx++] != ts)
            return false;
        #endif
    }

    #ifdef TC_DBG
    for(i = 0; i < (10+NUM_AUDIOFILES); i++) {
        Serial.print(sizes[i], DEC);
        Serial.print(", ");
    }
    Serial.println("");
    #endif

    return true;
}

bool copy_audio_files()
{
    char dtmf_buf[16] = "/Dtmf-0.mp3\0";    
    int i, haveErr = 0;

    if(!allowCPA) {
        Serial.println(F("copy_audio_files: SD missing or flash FS not mounted"));
        return false;
    }

    start_file_copy(); 

    /* Copy DTMF files */
    for(i = 0; i < 10; i++) {
        dtmf_buf[6] = i + '0';
        open_and_copy(dtmf_buf, haveErr);        
    }

    for(i = 0; i < NUM_AUDIOFILES; i++) {     
        open_and_copy(audioFiles[i], haveErr);         
    }

    if(haveErr) {
        file_copy_error();        
    } else {
        file_copy_done();
    }

    return (haveErr == 0);
}

void open_and_copy(const char *fn, int& haveErr)
{
    File sFile, dFile;
    
    if((sFile = SD.open(fn, FILE_READ))) {
        #ifdef TC_DBG
        Serial.print(F("copy_audio_files: Opened source file: "));
        Serial.println(fn);
        #endif
        if((dFile = SPIFFS.open(fn, FILE_WRITE))) {
            #ifdef TC_DBG
            Serial.print(F("copy_audio_files: Opened destination file: "));
            Serial.println(fn);
            #endif
            if(!filecopy(sFile, dFile)) {
                haveErr++;                    
            }
            dFile.close();
        } else {                
            Serial.print(F("copy_audio_files: Error opening destination file: "));
            Serial.println(fn);
            haveErr++;
        }
        sFile.close();
    } else {
        Serial.print(F("copy_audio_files: Error opening source file: "));
        Serial.println(fn);
        haveErr++;              
    } 
}         

bool filecopy(File source, File dest)
{
    uint8_t buffer[1024];
    size_t bytesr, bytesw;
    
    while((bytesr = source.read(buffer, 1024))) {
        if((bytesw = dest.write(buffer, bytesr)) != bytesr) {
            Serial.println(F("filecopy: Error writing data"));
            return false;   
        }
        file_copy_progress();
    }

    return true;   
}
