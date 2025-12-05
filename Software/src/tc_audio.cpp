/*
 * -------------------------------------------------------------------
 * CircuitSetup.us Time Circuits Display
 * (C) 2021-2022 John deGlavina https://circuitsetup.us
 * (C) 2022-2025 Thomas Winischhofer (A10001986)
 * https://github.com/realA10001986/Time-Circuits-Display
 * https://tcd.out-a-ti.me
 *
 * Sound handling
 *
 * -------------------------------------------------------------------
 * License: MIT NON-AI
 * 
 * Permission is hereby granted, free of charge, to any person 
 * obtaining a copy of this software and associated documentation 
 * files (the "Software"), to deal in the Software without restriction, 
 * including without limitation the rights to use, copy, modify, 
 * merge, publish, distribute, sublicense, and/or sell copies of the 
 * Software, and to permit persons to whom the Software is furnished to 
 * do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be 
 * included in all copies or substantial portions of the Software.
 *
 * In addition, the following restrictions apply:
 * 
 * 1. The Software and any modifications made to it may not be used 
 * for the purpose of training or improving machine learning algorithms, 
 * including but not limited to artificial intelligence, natural 
 * language processing, or data mining. This condition applies to any 
 * derivatives, modifications, or updates based on the Software code. 
 * Any usage of the Software in an AI-training dataset is considered a 
 * breach of this License.
 *
 * 2. The Software may not be included in any dataset used for 
 * training or improving machine learning algorithms, including but 
 * not limited to artificial intelligence, natural language processing, 
 * or data mining.
 *
 * 3. Any person or organization found to be in violation of these 
 * restrictions will be subject to legal action and may be held liable 
 * for any damages resulting from such use.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, 
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF 
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. 
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY 
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, 
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE 
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include "tc_global.h"

//#define TC_DBG_AUDIO      // debug audio library

#include <Arduino.h>
#include <SD.h>
#include <FS.h>
#ifdef USE_SPIFFS
#include <SPIFFS.h>
#include "src/ESP8266Audio/AudioFileSourceSPIFFS.h"
#else
#include <LittleFS.h>
#include "src/ESP8266Audio/AudioFileSourceLittleFS.h"
#endif
#include "src/ESP8266Audio/AudioFileSourceSD.h"
#include "src/ESP8266Audio/AudioFileSourcePROGMEM.h"

#include "src/ESP8266Audio/AudioGeneratorMP3.h"
#include "src/ESP8266Audio/AudioGeneratorWAV.h"

#include "src/ESP8266Audio/AudioOutputI2S.h"

#include "tc_time.h"
#include "tc_settings.h"
#include "tc_audio.h"
#include "tc_keypad.h"
#include "tc_wifi.h"

static AudioGeneratorMP3 *mp3;
static AudioGeneratorWAV *wav;

#ifdef USE_SPIFFS
static AudioFileSourceSPIFFS *myFS0;
#else
static AudioFileSourceLittleFS *myFS0;
#endif
static AudioFileSourceSD *mySD0;
static AudioFileSourcePROGMEM *myPM;

static AudioOutputI2S *out;

bool audioInitDone = false;
bool audioMute     = false;

bool muteBeep      = true;
static bool beepRunning = false;

bool            haveMusic = false;
bool            mpActive = false;
static uint16_t maxMusic = 0;
static uint16_t *playList = NULL;
static int      mpCurrIdx = 0;
static bool     mpShuffle = false;
static uint16_t currPlaying = 0;
#define         MAXID3LEN 2048

static const float volTable[20] = {
    0.00, 0.02, 0.04, 0.06,
    0.08, 0.10, 0.13, 0.16,
    0.19, 0.22, 0.26, 0.30,
    0.35, 0.40, 0.50, 0.60,
    0.70, 0.80, 0.90, 1.00
};
int           volumePin = VOLUME_PIN;
// Resolution for pot, 9-12 allowed
#define POT_RESOLUTION 9
int           curVolume = DEFAULT_VOLUME;
#define VOL_SMOOTH_SIZE 4
static int    rawVol[VOL_SMOOTH_SIZE];
static int    rawVolIdx = 0;
static int    anaReadCount = 0;
static long   prev_avg, prev_raw, prev_raw2;

static float  curVolFact = 1.0;
static bool   curChkNM   = true;
static bool   dynVol     = true;
static int    sampleCnt  = 0;

static uint32_t key_playing = 0;

bool          haveLineOut = false;
bool          useLineOut  = false;
static bool   playLineOut = false;

static char   keySnd[] = "/key3.mp3";   // not const
static uint16_t haveKeySnd = 0;

static const char *tcdrdone = "/TCD_DONE.TXT";
bool          headLineShown = false;
bool          blinker       = true;
unsigned long renNow1, renNow2;

static char       dtmfBuf[] = "/Dtmf-0.wav";    // Not const
static const char *hsnd     = "/hour.mp3";
static char       shsnd[]   = "/hour-00.mp3";   // Not const
static bool       haveHrSnd = false;
static uint32_t   haveSpHrSnd = 0;

char id3artist[16] = { 0 };
char id3track[16] = { 0 };

static float  getVolume();
static void   setLineOut(bool doLineOut);

static int    mp_findMaxNum();
static void   mp_nextprev(bool forcePlay, bool next);
static bool   mp_play_int(bool force);
static void   mp_buildFileName(char *fnbuf, int num);
static bool   mp_renameFilesInDir(bool isSetup);
static void   mpren_quickSort(char **a, int s, int e);

static void   decodeID3(char *artist, char *track, char *id3, int id3size);

#include "tc_beep.h"

/*
 * audio_setup()
 */
void audio_setup()
{
    #ifdef TC_DBG_AUDIO
    audioLogger = &Serial;
    #endif

    // Init line-out
    if(haveLineOut) {
        // Switch to internal speaker
        pinMode(SWITCH_LINEOUT_PIN, OUTPUT);
        digitalWrite(SWITCH_LINEOUT_PIN, LOW);
        // Unmute line-out DAC
        pinMode(MUTE_LINEOUT_PIN, OUTPUT);
        //digitalWrite(MUTE_LINEOUT_PIN, LOW);  // Mute
        digitalWrite(MUTE_LINEOUT_PIN, HIGH);   // Unmute
    }

    // Set resolution for volume pot
    analogReadResolution(POT_RESOLUTION);
    analogSetWidth(POT_RESOLUTION);

    out = new AudioOutputI2S(0, AudioOutputI2S::EXTERNAL_I2S, 32, AudioOutputI2S::APLL_DISABLE);
    out->SetOutputModeMono(true);
    out->SetPinout(I2S_BCLK_PIN, I2S_LRCLK_PIN, I2S_DIN_PIN);

    mp3 = new AudioGeneratorMP3();
    wav = new AudioGeneratorWAV();

    #ifdef USE_SPIFFS
    myFS0 = new AudioFileSourceSPIFFS();
    #else
    myFS0 = new AudioFileSourceLittleFS();
    #endif

    if(haveSD) {
        mySD0 = new AudioFileSourceSD();
    }

    myPM = new AudioFileSourcePROGMEM();

    loadCurVolume();

    loadMusFoldNum();
    mpShuffle = (settings.shuffle[0] != '0');

    if(haveLineOut) {
        loadLineOut();
    }

    // MusicPlayer init
    mp_init(true);

    // Check for sound files to avoid unsuccessful file-lookups later
    
    for(int i = 1; i < 10; i++) {
        keySnd[4] = '0' + i;
        if(check_file_SD(keySnd)) {
            haveKeySnd |= (1 << i);
        }
    }

    haveHrSnd = check_file_SD(hsnd);

    for(int i = 0; i <= 23; i++) {
        shsnd[6] = (i / 10) + '0';
        shsnd[7] = (i % 10) + '0';
        if(check_file_SD(shsnd)) {
            haveSpHrSnd |= (1 << i);
        }
    }

    audioInitDone = true;

    #ifdef TC_DBG
    Serial.printf("haveKeySnd 0x%x, haveSPHrSnd 0x%x\n", haveKeySnd, haveSpHrSnd);
    #endif    
}

/*
 * audio_loop()
 *
 */
void audio_loop()
{
    if(wav->isRunning()) {
        if(!wav->loop()) {
            wav->stop();
            beepRunning = false;
        }
    } else if(mp3->isRunning()) {
        if(!mp3->loop()) {
            mp3->stop();
            key_playing = 0;
            if(mpActive) {
                mp_next(true);
            }
        } else if(dynVol) {
            sampleCnt++;
            if(sampleCnt > 1) {
                out->SetGain(getVolume());
                sampleCnt = 0;
            }
        }
    } else if(mpActive) {
        pwrNeedFullNow();
        mp_next(true);
    }
}

static int skipID3(char *buf)
{
    if(buf[0] == 'I' && buf[1] == 'D' && buf[2] == '3' && 
       buf[3] >= 0x02 && buf[3] <= 0x04 && buf[4] == 0 &&
       (!(buf[5] & 0x80))) {
        int32_t pos = ((buf[6] << (24-3)) |
                       (buf[7] << (16-2)) |
                       (buf[8] << (8-1))  |
                       (buf[9])) + 10;
        #ifdef TC_DBG
        Serial.printf("Skipping ID3 tags, seeking to %d (0x%x)\n", pos, pos);
        #endif
        return pos;
    }
    return 0;
}

void play_file(const char *audio_file, uint32_t flags, float volumeFactor)
{
    char buf[10];
    int pos;
    
    if(audioMute) return;

    if(flags & PA_INTRMUS) {
        mpActive = false;
    } else {
        if(mpActive) return;
    }

    pwrNeedFullNow();

    #ifdef TC_DBG
    Serial.printf("Audio: Playing %s\n", audio_file);
    #endif

    // If something is currently on, kill it
    stopAudio();
    beepRunning = false;

    playLineOut = (haveLineOut && useLineOut && (flags & PA_LINEOUT)) ? true : false;
    setLineOut(playLineOut);
    if(playLineOut) flags &= ~(PA_DYNVOL|PA_CHECKNM);
    
    curChkNM    = (flags & PA_CHECKNM) ? true : false;
    dynVol      = (flags & PA_DYNVOL) ? true : false;
    key_playing = flags & 0x1ff00;

    curVolFact = volumeFactor;

    // Reset vol smoothing
    // (user might have turned the pot while no sound was played)
    rawVolIdx = 0;
    anaReadCount = 0;

    id3artist[0] = id3track[0] = 0;

    out->SetGain(getVolume());

    if(haveSD && ((flags & PA_ALLOWSD) || FlashROMode) && mySD0->open(audio_file)) {
        if(flags & PA_DOID3TS) {
            char *id3 = (char *)malloc(MAXID3LEN);
            if(id3) {
                id3[0] = 0;
                mySD0->read((void *)id3, 10);
                if((pos = skipID3(id3))) {
                    int Id3Size = pos <= MAXID3LEN ? pos : MAXID3LEN;
                    mySD0->read((void *)((char *)id3 + 10), Id3Size - 10);
                    decodeID3(id3artist, id3track, id3, Id3Size);
                }
                free(id3);
                mySD0->seek(pos, SEEK_SET);
            }
        } else {
            buf[0] = 0;
            mySD0->read((void *)buf, 10);
            mySD0->seek(skipID3(buf), SEEK_SET);
        }
        mp3->begin(mySD0, out);
        #ifdef TC_DBG
        Serial.println("Playing from SD");
        #endif
    }
    #ifdef USE_SPIFFS
      else if(haveFS && SPIFFS.exists(audio_file) && myFS0->open(audio_file))
    #else    
      else if(haveFS && myFS0->open(audio_file))
    #endif
    {
        if(!(flags & PA_ISWAV)) {
            buf[0] = 0;
            myFS0->read((void *)buf, 10);
            myFS0->seek(skipID3(buf), SEEK_SET);
            mp3->begin(myFS0, out);
        } else {
            wav->begin(myFS0, out);
        }
        #ifdef TC_DBG
        Serial.println("Playing from flash FS");
        #endif
    } else {
        key_playing = 0;
        #ifdef TC_DBG
        Serial.println("Audio file not found");
        #endif
    }
}

/*
 * Play specific sounds
 * 
 */

uint32_t play_keypad_sound(char key)
{
    uint32_t kp = key_playing;
    dtmfBuf[6] = key;
    play_file(dtmfBuf, PA_ISWAV|PA_INTSPKR|PA_CHECKNM, 0.6);
    return kp;
}

void play_hour_sound(int hour)
{
    if(mpActive) return;
    // Not even called in night mode

    if(haveSpHrSnd & (1 << hour)) {
        shsnd[6] = (hour / 10) + '0';
        shsnd[7] = (hour % 10) + '0';
        play_file(shsnd, PA_INTSPKR|PA_ALLOWSD);
    } else if(haveHrSnd) {
        play_file(hsnd, PA_INTSPKR|PA_ALLOWSD);
    }
}

void play_beep()
{
    if(!FPBUnitIsOn     || 
       muteBeep         || 
       mpActive         || 
       mp3->isRunning() || 
       (wav->isRunning() && !beepRunning) || 
       presentTime.getNightMode() ||
       audioMute) {
        return;
    }

    pwrNeedFullNow();

    if(wav->isRunning()) {
        wav->stop();
    }

    setLineOut(false);
    playLineOut = false;

    curVolFact = 0.3;
    curChkNM   = true;
    // Reset vol smoothing
    // (user might have turned the pot while no sound was played)
    rawVolIdx = 0;
    anaReadCount = 0;
    out->SetGain(getVolume());

    key_playing = 0;
    id3artist[0] = id3track[0] = 0;

    myPM->open(data_beep_wav, data_beep_wav_len);
    wav->begin(myPM, out);
    beepRunning = true;
}

void play_key(int k, uint32_t preDTMFkp)
{
    uint32_t pa_key;
    
    if(!(haveKeySnd & (1 << k)))
        return;

    pa_key = (1 << (7+k));

    if(pa_key == preDTMFkp) {
        // Logic for keypad sound having interrupted
        // us already; no need to stop().
        return;
    }
    if(pa_key == key_playing) {
        stopAudio();
        return;
    }
    
    keySnd[4] = '0' + k;
    
    play_file(keySnd, pa_key|PA_LINEOUT|PA_CHECKNM|PA_INTRMUS|PA_ALLOWSD|PA_DYNVOL);
}

// Returns value for volume based on the position of the pot
// Since the values vary we do some noise reduction
static float getRawVolume()
{
    float vol_val;
    long avg = 0, avg1 = 0, avg2 = 0;
    long raw;

    raw = analogRead(volumePin);

    if(anaReadCount > 1) {
      
        rawVol[rawVolIdx] = raw;

        if(anaReadCount < VOL_SMOOTH_SIZE) {
        
            avg = 0;
            for(int i = rawVolIdx; i > rawVolIdx - anaReadCount; i--) {
                avg += rawVol[i & (VOL_SMOOTH_SIZE-1)];
            }
            avg /= anaReadCount;
            anaReadCount++;

        } else {

            for(int i = rawVolIdx; i > rawVolIdx - anaReadCount; i--) {
                if(i & 1) { 
                    avg1 += rawVol[i & (VOL_SMOOTH_SIZE-1)];
                } else {
                    avg2 += rawVol[i & (VOL_SMOOTH_SIZE-1)];
                }
            }
            avg1 = roundf((float)avg1 / (float)(VOL_SMOOTH_SIZE/2));
            avg2 = roundf((float)avg2 / (float)(VOL_SMOOTH_SIZE/2));
            avg = (abs(avg1-prev_avg) < abs(avg2-prev_avg)) ? avg1 : avg2;

            /*
            Serial.printf("%d %d %d %d\n", raw, avg1, avg2, avg);
            */
            
            prev_avg = avg;
        }
        
    } else {
      
        anaReadCount++;
        rawVol[rawVolIdx] = avg = prev_avg = prev_raw = prev_raw2 = raw;
        
    }

    rawVolIdx++;
    rawVolIdx &= (VOL_SMOOTH_SIZE-1);

    vol_val = (float)avg / (float)((1<<POT_RESOLUTION)-1);

    if((raw + prev_raw + prev_raw2 > 0) && vol_val < 0.01) vol_val = 0.01;

    prev_raw2 = prev_raw;
    prev_raw = raw;

    //Serial.println(vol_val);

    return vol_val;
}

static float getVolume()
{
    float vol_val = 1.0;

    if(!playLineOut) {
        if(curVolume == 255) {
            vol_val = getRawVolume();
        } else {
            vol_val = volTable[curVolume];
        }

        // If user muted, return 0
        if(vol_val == 0.0) return vol_val;
    }

    vol_val *= curVolFact;

    // Reduce volume in night mode, if requested
    if(curChkNM && presentTime.getNightMode()) {
        vol_val *= 0.3;
    }

    // Do not totally mute
    // 0.02 is the lowest audible gain
    if(vol_val < 0.02) vol_val = 0.02;

    return vol_val;
}

static void setLineOut(bool doLineOut)
{
    if(haveLineOut) {
        if(useLineOut && doLineOut) {
            // Mute/un-mute secondary DAC? No, for now.
            //digitalWrite(MUTE_LINEOUT_PIN, doLineOut ? HIGH : LOW);
            out->SetOutputModeMono(false);
            // Switch off MAX98357, switch on PCM510x
            digitalWrite(SWITCH_LINEOUT_PIN, HIGH);
        } else {
            out->SetOutputModeMono(true);
            // Switch on MAX98357, switch off PCM510x
            digitalWrite(SWITCH_LINEOUT_PIN, LOW);
        }
    }
}

/*
 * Helpers
 */
bool check_file_SD(const char *audio_file)
{
    #ifdef TC_DBG
    Serial.printf("check_file_SD: Checking for %s\n", audio_file);
    #endif
    return (haveSD && SD.exists(audio_file));
}

int getSWVolFromHWVol()
{
    float curHWvol = getRawVolume();
    int i;
    
    for(i = 0; i < 20-1; i++) {
        if(curHWvol <= volTable[i])
            break;
    }

    return i;
}

bool checkAudioDone()
{
    if(mp3->isRunning() || wav->isRunning()) return false;
    return true;
}

bool checkMP3Done()
{
    if(mp3->isRunning() || (wav->isRunning() && !beepRunning)) return false;
    return true;
}

bool checkMP3Running()
{
    return mp3->isRunning();
}

void stopAudio()
{
    if(mp3->isRunning()) {
        mp3->stop();
    } else if(wav->isRunning()) {
        wav->stop();
    }
    key_playing = 0;
    id3artist[0] = id3track[0] = 0;
}

void stop_key()
{
    if(key_playing) {
        mp3->stop();
        key_playing = 0;
        id3artist[0] = id3track[0] = 0;
    }
}

/*
 * ID3 handling
 */

static void copyId3String(char *src, char *dst, int tagSz, int maxChrs)
{
    uint16_t chr;
    char c;
    char *send = src + tagSz;
    char *tend = dst + maxChrs;
    char enc = *src++;

    dst[0] = 0;

    if(enc == 1) {
        uint16_t be = *src++ << 8;
        be |= *src++;
        if(be == 0xfeff) enc = 2;
        else if(be != 0xfffe) src -= 2;
    }

    switch(enc) {
    case 0:        // ISO-8859-1
        while(dst < tend && src < send) {
            if(!*src) return;
            c = *src++;
            if(c >= ' ' && c <= 126)  {
                if(c >= 'a' && c <= 'z') c &= ~0x20;
                else if(c == 126) c = '-';  // 126 = ~ but displayed as °, so make it '-'
                *dst++ = c;
                *dst = 0;
            }
        }
        break;
    case 1:        // UTF-16LE
    case 2:        // UTF-16BE
        while(dst < tend && src < send) {
            if(enc == 1) { chr = *src++; chr |= (*src++ << 8); }
            else         { chr = *src++ << 8; chr |= *src++;   }
            if(!chr) return;
            if(chr >= 0xd800 && chr <= 0xdbff) {
                src += 2;
            } else if(chr >= ' ' && chr <= 126) {   
                if(chr >= 'a' && chr <= 'z') chr &= ~0x20;
                else if(chr == 126) chr = '-';  // 126 = ~ but displayed as °, so make it '-'
                *dst++ = chr & 0xff;
                *dst = 0;
            }
        }
        break;
    case 3:        // UTF-8
        filterOutUTF8(src, dst, tagSz, maxChrs);
        break;
    }
}

static void decodeID3(char *artist, char *track, char *id3, int Id3Size)
{
    uint8_t rev = id3[3];
    char *ptr  = id3 + 10;
    char *eptr = id3 + Id3Size;
    int  stopLoop = 0;
    uint8_t tag0, tag1, tag2, tag3;
    uint8_t badFlags = 0;
    unsigned long tagSz, offSet;
    char tFlags[2] = { 0, 0 };

    *artist = *track = 0;

    // Unsynchronizing not supported
    if(id3[5] & 0x80) return;
    
    // Skip extended header
    if(rev >= 3 && id3[5] & 0x40) {
        if(Id3Size < 16) return;
        if(rev == 3) {
            ptr += 4;
            ptr += ((id3[10] << 24) |
                    (id3[11] << 16) |
                    (id3[12] <<  8) |
                    (id3[13]));
        } else {
            ptr += ((id3[10] << (24-3)) |
                    (id3[11] << (16-2)) |
                    (id3[12] << (8-1))  |
                    (id3[13]));
        }
        if(ptr >= eptr) return;
    }

    while((stopLoop != 3) && ptr < (eptr - 4)) {
        tag0 = *ptr++;
        tag1 = *ptr++;
        tag2 = *ptr++;
        tag3 = (rev == 2) ? 0 : *ptr++;
    
        // Quit when we are in padding
        if(!(tag0 | tag1 | tag2 | tag3)) return;
        
        if(rev == 2) {

            if(ptr > eptr - 3) return;

            tagSz = ((ptr[0] << 16) |
                     (ptr[1] <<  8) |
                     (ptr[2]));

            ptr += 3;
          
        } else {
            
            if(ptr > eptr - 6) return;

            if(rev == 3) {
                tagSz = ((ptr[0] << 24) |
                         (ptr[1] << 16) |
                         (ptr[2] <<  8) |
                         (ptr[3]));
                badFlags = 0x80 + 0x40;         // Compression/Encryption
            } else {
                tagSz = ((ptr[0] << (24-3)) |
                         (ptr[1] << (16-2)) |
                         (ptr[2] << (8-1))  |
                         (ptr[3]));
                badFlags = 0x08 + 0x04 + 0x02;  // Compression/Encryption/Unsync
            }
            
            tFlags[0] = ptr[4];
            tFlags[1] = ptr[5];

            ptr += 6;

        }

        if(ptr + tagSz > eptr) return;

        // Compression & unsynchronization not supported
        if(!(tFlags[1] & badFlags) && (tag0 == 'T')) {
            // Check for data length indicator despite no other flags
            // Should not happen; we ignore it if it is there
            offSet = (rev == 4 && tFlags[1] & 0x01) ? 4 : 0;
            if((rev == 2 && tag1 == 'T' && tag2 == '2') ||
               (rev != 2 && tag1 == 'I' && tag2 == 'T' && tag3 == '2')) {
                // Copy song title
                copyId3String(ptr+offSet, track, tagSz-offSet, 15);
                stopLoop |= 1;
            } else if((rev == 2 && tag1 == 'P' && tag2 == '1') ||
                      (rev != 2 && tag1 == 'P' && tag2 == 'E' && tag3 == '1')) {
                // Copy artist
                copyId3String(ptr+offSet, artist, tagSz-offSet, 15);
                stopLoop |= 2;
            }
        }

        ptr += tagSz;
    }
}

/*
 * The Music Player
 */
 
void mp_init(bool isSetup) 
{
    char fnbuf[20];
    
    haveMusic = false;

    if(playList) {
        free(playList);
        playList = NULL;
    }

    mpCurrIdx = 0;
    
    if(haveSD) {
        int i, j;

        #ifdef TC_DBG_MP
        Serial.println("MusicPlayer: Checking for music files");
        #endif

        mp_renameFilesInDir(isSetup);

        mp_buildFileName(fnbuf, 0);
        if(SD.exists(fnbuf)) {
            haveMusic = true;
            
            maxMusic = mp_findMaxNum();
            #ifdef TC_DBG_MP
            Serial.printf("MusicPlayer: last file num %d\n", maxMusic);
            #endif

            playList = (uint16_t *)malloc((maxMusic + 1) * 2);

            if(!playList) {

                haveMusic = false;
                #ifdef TC_DBG_MP
                Serial.println("MusicPlayer: Failed to allocate PlayList");
                #endif

            } else {

                // Init play list
                mp_makeShuffle(mpShuffle);
                
            }

        } else {
            #ifdef TC_DBG_MP
            Serial.printf("MusicPlayer: Failed to open %s\n", fnbuf);
            #endif
        }
    }
}

static bool mp_checkForFile(int num)
{
    char fnbuf[20];

    if(num > 999) return false;

    mp_buildFileName(fnbuf, num);
    if(SD.exists(fnbuf)) {
        return true;
    }
    return false;
}

static int mp_findMaxNum()
{
    int i, j;

    for(j = 256, i = 512; j >= 2; j >>= 1) {
        if(mp_checkForFile(i)) {
            i += j;    
        } else {
            i -= j;
        }
    }
    if(mp_checkForFile(i)) {
        if(mp_checkForFile(i+1)) i++;
    } else {
        i--;
        if(!mp_checkForFile(i)) i--;
    }

    return i;
}

void mp_makeShuffle(bool enable)
{
    int numMsx = maxMusic + 1;

    mpShuffle = enable;

    if(!haveMusic) return;
    
    for(int i = 0; i < numMsx; i++) {
        playList[i] = i;
    }
    
    if(enable && numMsx > 2) {
        for(int i = 0; i < numMsx; i++) {
            int ti = esp_random() % numMsx;
            uint16_t t = playList[ti];
            playList[ti] = playList[i];
            playList[i] = t;
        }
        #ifdef TC_DBG_MP
        for(int i = 0; i <= maxMusic; i++) {
            Serial.printf("%d ", playList[i]);
            if((i+1) % 16 == 0 || i == maxMusic) Serial.printf("\n");
        }
        #endif
    }
}

void mp_play(bool forcePlay)
{
    int oldIdx = mpCurrIdx;

    if(!haveMusic) return;
    
    do {
        if(mp_play_int(forcePlay)) {
            mpActive = forcePlay;
            break;
        }
        mpCurrIdx++;
        if(mpCurrIdx > maxMusic) mpCurrIdx = 0;
    } while(oldIdx != mpCurrIdx);
}

bool mp_stop()
{
    bool ret = mpActive;
    
    if(mpActive) {
        mp3->stop();
        mpActive = false;
        id3artist[0] = id3track[0] = 0;
    }
    
    return ret;
}

void mp_next(bool forcePlay)
{
    mp_nextprev(forcePlay, true);
}

void mp_prev(bool forcePlay)
{   
    mp_nextprev(forcePlay, false);
}

static void mp_nextprev(bool forcePlay, bool next)
{
    int oldIdx = mpCurrIdx;

    if(!haveMusic) return;
    
    do {
        if(next) {
            mpCurrIdx++;
            if(mpCurrIdx > maxMusic) mpCurrIdx = 0;
        } else {
            mpCurrIdx--;
            if(mpCurrIdx < 0) mpCurrIdx = maxMusic;
        }
        if(mp_play_int(forcePlay)) {
            mpActive = forcePlay;
            break;
        }
    } while(oldIdx != mpCurrIdx);
}

int mp_gotonum(int num, bool forcePlay)
{
    if(!haveMusic) return 0;

    if(num < 0) num = 0;
    else if(num > maxMusic) num = maxMusic;

    if(mpShuffle) {
        for(int i = 0; i <= maxMusic; i++) {
            if(playList[i] == num) {
                mpCurrIdx = i;
                break;
            }
        }
    } else 
        mpCurrIdx = num;

    mp_play(forcePlay);

    return playList[mpCurrIdx];
}

static bool mp_play_int(bool force)
{
    char fnbuf[20];

    mp_buildFileName(fnbuf, playList[mpCurrIdx]);
    if(SD.exists(fnbuf)) {
        if(force) play_file(fnbuf, PA_LINEOUT|PA_DOID3TS|PA_CHECKNM|PA_INTRMUS|PA_ALLOWSD|PA_DYNVOL);
        currPlaying = playList[mpCurrIdx];
        return true;
    }
    return false;
}

int mp_get_currently_playing()
{
    if(!haveMusic || !mpActive)
        return -1;

    return currPlaying;
}

static void mp_buildFileName(char *fnbuf, int num)
{
    sprintf(fnbuf, "/music%1d/%03d.mp3", musFolderNum, num);
}

// For keypad menu only
int mp_checkForFolder(int num)
{
    char fnbuf[32];
    int ret;

    // returns 
    // 1 if folder is ready (contains 000.mp3 and DONE)
    // 0 if folder does not exist
    // -1 if folder exists but needs processing
    // -2 if musicX contains no audio files
    // -3 if musicX is not a folder

    if(num < 0 || num > 9)
        return 0;

    // If folder does not exist, return 0
    sprintf(fnbuf, "/music%1d", num);
    if(!SD.exists(fnbuf))
        return 0;

    // Check if DONE exists
    sprintf(fnbuf, "/music%1d%s", num, tcdrdone);
    if(SD.exists(fnbuf)) {
        sprintf(fnbuf, "/music%1d/000.mp3", num);
        if(SD.exists(fnbuf)) {
            // If 000.mp3 and DONE exists, return 1
            return 1;
        }
        // If DONE, but no 000.mp3, assume no audio files
        return -2;
    }
      
    // Check if folder is folder
    sprintf(fnbuf, "/music%1d", num);
    File origin = SD.open(fnbuf);
    if(!origin) return 0;
    if(!origin.isDirectory()) {
        // If musicX is not a folder, return -3
        ret = -3;
    } else {
        // If it is a folder, it needs processing
        ret = -1;
    }
    origin.close();
    return ret;
}

/*
 * Auto-renamer
 */

// Check file is eligible for renaming:
// - not a hidden/exAtt file,
// - file name ends with ".mp3"
// - filename not already "/musicX/ddd.mp3"
static bool mpren_checkFN(const char *buf)
{
    // Hidden or macOS exAttr file? Ignore.
    if(buf[0] == '.') return true;

    size_t s = strlen(buf);

    // Filename shorter than ".mp3"? Ignore.
    if(s < 4) return true;

    s -= 4;
    // Not an mp3? Ignore.
    if(buf[s] != '.' || buf[s+3] != '3')
        return true;
    if(buf[s+1] != 'm' && buf[s+1] != 'M')
        return true;
    if(buf[s+2] != 'p' && buf[s+2] != 'P')
        return true;

    // Now check for xxx.mp3 (xxx=000-999)

    // Filename shorter or longer? Do it.
    if(s != 3)
        return false;

    // Filename not a 3-digit number? Do it.
    if(buf[0] < '0' || buf[0] > '9' ||
       buf[1] < '0' || buf[1] > '9' ||
       buf[2] < '0' || buf[2] > '9')
        return false;

    // Otherwise ignore.
    return true;
}

static void mpren_showHeadLine(bool checking)
{
    destinationTime.showTextDirect(checking ? "CHECKING" : "RENAMING");
    presentTime.showTextDirect("MUSIC FILES");
}

static void mpren_showBlinker(bool blinker, int fileNum)
{
    if(!fileNum) {
        departedTime.showTextDirect(blinker ? "PLEASE" : "WAIT");
    } else {
        char buf[16];
        #ifdef IS_ACAR_DISPLAY
        sprintf(buf, "%-9s%3d", blinker ? "PLEASE" : "WAIT", fileNum);
        #else
        sprintf(buf, "%-10s%3d", blinker ? "PLEASE" : "WAIT", fileNum);
        #endif
        departedTime.showTextDirect(buf);
    }
}

static void mpren_looper(bool isSetup, bool checking, int fileNum)
{
    unsigned long now = millis();

    // We are only ever called from keypad menu.
    // So gps_loop(true) is allowed.

    if(now - renNow1 > 250) {
        wifi_loop();
        if(!isSetup) {
            ntp_loop();
            #if defined(TC_HAVEGPS) || defined(TC_HAVE_RE)
            gps_loop(true);
            #endif
            bttfn_loop();
            // audio_loop not required, never
            // called when audio is active
        }
        delay(10);
        renNow1 = now;
    }
    if(now - renNow2 > 2000) {
        mpren_showBlinker(blinker, fileNum);
        if(!headLineShown) {
            mpren_showHeadLine(checking);
            destinationTime.on();
            presentTime.on();
            departedTime.on();
            headLineShown = true;
        }
        blinker = !blinker;
        renNow2 = now;
    }
}

static bool mp_renameFilesInDir(bool isSetup)
{
    char fnbuf[20];
    char fnbuf3[32];
    char fnbuf2[256];
    char **a, **d;
    char *c;
    int num = musFolderNum;
    int count = 0;
    int fileNum = 0;
    int strLength;
    int nameOffs = 8;
    int allocBufs = 1;
    int allocBufIdx = 0;
    const unsigned long bufSizes[8] = {
        16384, 16384, 8192, 8192, 8192, 8192, 8192, 4096 
    };
    char *bufs[8] = { NULL };
    unsigned long sz, bufSize;
    bool stopLoop = false;
    bool hls = false;
#ifdef HAVE_GETNEXTFILENAME
    bool isDir;
#endif
    #if defined(TC_DBG) || defined(TC_DBG_MP)
    const char *funcName = "MusicPlayer/Renamer: ";
    #endif

    headLineShown = false;
    blinker = true;
    renNow1 = renNow2 = millis();

    // Build "DONE"-file name
    sprintf(fnbuf, "/music%1d", num);
    strcpy(fnbuf3, fnbuf);
    strcat(fnbuf3, tcdrdone);

    // Check for DONE file
    if(SD.exists(fnbuf3)) {
        #ifdef TC_DBG_MP
        Serial.printf("%s%s exists\n", funcName, fnbuf3);
        #endif
        return true;
    }

    // Check if folder exists
    if(!SD.exists(fnbuf)) {
        return false;
    }

    // Open folder and check if it is actually a folder
    File origin = SD.open(fnbuf);
    if(!origin) {
        return false;
    }
    if(!origin.isDirectory()) {
        origin.close();
        return false;
    }
        
    // Allocate pointer array
    if(!(a = (char **)malloc(1000*sizeof(char *)))) {
        origin.close();
        return false;
    }

    // Allocate (first) buffer for file names
    if(!(bufs[0] = (char *)malloc(bufSizes[0]))) {
        origin.close();
        free(a);
        return false;
    }

    c = bufs[0];
    bufSize = bufSizes[0];
    d = a;

    // Loop through all files in folder

#ifdef HAVE_GETNEXTFILENAME
    String fileName = origin.getNextFileName(&isDir);
    // Check if File::name() returns FQN or plain name
    if(fileName.length() > 0) nameOffs = (fileName.charAt(0) == '/') ? 8 : 0;
    while(!stopLoop && fileName.length() > 0)
#else
    File file = origin.openNextFile();
    // Check if File::name() returns FQN or plain name
    if(file) nameOffs = (file.name()[0] == '/') ? 8 : 0;
    while(!stopLoop && file)
#endif
    {

        mpren_looper(isSetup, true, 0);

#ifdef HAVE_GETNEXTFILENAME

        if(!isDir) {
            const char *fn = fileName.c_str();
            strLength = strlen(fn);
            sz = strLength - nameOffs + 1;
            if((sz > bufSize) && (allocBufIdx < 7)) {
                allocBufIdx++;
                if(!(bufs[allocBufIdx] = (char *)malloc(bufSizes[allocBufIdx]))) {
                    #ifdef TC_DBG_MP
                    Serial.printf("%sFailed to allocate additional sort buffer\n", funcName);
                    #endif
                } else {
                    #ifdef TC_DBG_MP
                    Serial.printf("%sAllocated additional sort buffer\n", funcName);
                    #endif
                    c = bufs[allocBufIdx];
                    bufSize = bufSizes[allocBufIdx];
                }
            }
            if((strLength < 256) && (sz <= bufSize)) {
                if(!mpren_checkFN(fn + nameOffs)) {
                    *d++ = c;
                    strcpy(c, fn + nameOffs);
                    #ifdef TC_DBG_MP
                    Serial.printf("%sAdding '%s'\n", funcName, c);
                    #endif
                    c += sz;
                    bufSize -= sz;
                    fileNum++;
                }
            } else if(sz > bufSize) {
                stopLoop = true;
                #ifdef TC_DBG_MP
                Serial.printf("%sSort buffer(s) exhausted, remaining files ignored\n", funcName);
                #endif
            }
        }
        
#else // --------------

        if(!file.isDirectory()) {
            strLength = strlen(file.name());
            sz = strLength - nameOffs + 1;
            if((sz > bufSize) && (allocBufIdx < 7)) {
                allocBufIdx++;
                if(!(bufs[allocBufIdx] = (char *)malloc(bufSizes[allocBufIdx]))) {
                    #ifdef TC_DBG_MP
                    Serial.printf("%sFailed to allocate additional sort buffer\n", funcName);
                    #endif
                } else {
                    #ifdef TC_DBG_MP
                    Serial.printf("%sAllocated additional sort buffer\n", funcName);
                    #endif
                    c = bufs[allocBufIdx];
                    bufSize = bufSizes[allocBufIdx];
                }
            }
            if((strLength < 256) && (sz <= bufSize)) {
                if(!mpren_checkFN(file.name() + nameOffs)) {
                    *d++ = c;
                    strcpy(c, file.name() + nameOffs);
                    #ifdef TC_DBG_MP
                    Serial.printf("%sAdding '%s'\n", funcName, c);
                    #endif
                    c += sz;
                    bufSize -= sz;
                    fileNum++;
                }
            } else if(sz > bufSize) {
                stopLoop = true;
                #ifdef TC_DBG_MP
                Serial.printf("%sSort buffer(s) exhausted, remaining files ignored\n", funcName);
                #endif
            }
        }
        file.close();
        
#endif
        
        if(fileNum >= 1000) stopLoop = true;

        if(!stopLoop) {
            #ifdef HAVE_GETNEXTFILENAME
            fileName = origin.getNextFileName(&isDir);
            #else
            file = origin.openNextFile();
            #endif
        }
    }

    origin.close();

    #ifdef TC_DBG_MP
    Serial.printf("%s%d files to process\n", funcName, fileNum);
    #endif

    // Sort file names, and rename

    if(fileNum) {
        
        // Sort file names
        mpren_quickSort(a, 0, fileNum - 1);
    
        sprintf(fnbuf2, "/music%1d/", num);
        strcpy(fnbuf, fnbuf2);

        // If 000.mp3 exists, find current count
        // the usual way. Otherwise start at 000.
        strcpy(fnbuf + 8, "000.mp3");
        if(SD.exists(fnbuf)) {
            count = mp_findMaxNum() + 1;
        }

        // Trigger head line change
        if((hls = headLineShown)) {
            renNow2 = 0;
            headLineShown = false;
        }

        for(int i = 0; i < fileNum && count <= 999; i++) {
            
            mpren_looper(isSetup, false, fileNum - i);

            sprintf(fnbuf + 8, "%03d.mp3", count);
            strcpy(fnbuf2 + 8, a[i]);
            if(!SD.rename(fnbuf2, fnbuf)) {
                bool done = false;
                while(!done) {
                    count++;
                    if(count <= 999) {
                        sprintf(fnbuf + 8, "%03d.mp3", count);
                        done = SD.rename(fnbuf2, fnbuf);
                    } else {
                        done = true;
                    }
                }
            }
            #ifdef TC_DBG
            Serial.printf("%sRenamed '%s' to '%s'\n", funcName, fnbuf2, fnbuf);
            #endif
            
            count++;
        }
    }

    for(int i = 0; i <= allocBufIdx; i++) {
        if(bufs[i]) free(bufs[i]);
    }
    free(a);

    // Write "DONE" file
    if((origin = SD.open(fnbuf3, FILE_WRITE))) {
        origin.close();
        #ifdef TC_DBG_MP
        Serial.printf("%sWrote %s\n", funcName, fnbuf3);
        #endif
    }

    // Clear displays
    if(hls || headLineShown) {
        destinationTime.showTextDirect("");
        presentTime.showTextDirect("");
        departedTime.showTextDirect("");
    }

    return true;
}

/*
 * QuickSort for file names
 */

static unsigned char mpren_toUpper(char a)
{
    if(a >= 'a' && a <= 'z')
        a &= ~0x20;

    return (unsigned char)a;
}

static bool mpren_strLT(const char *a, const char *b)
{
    int aa = strlen(a);
    int bb = strlen(b);
    int cc = aa < bb ? aa : bb;

    for(int i = 0; i < cc; i++) {
        unsigned char aaa = mpren_toUpper(*a);
        unsigned char bbb = mpren_toUpper(*b);
        if(aaa < bbb) return true;
        if(aaa > bbb) return false;
        *a++; *b++;
    }

    return false;
}

static int mpren_partition(char **a, int s, int e)
{
    char *t;
    char *p = a[e];
    int   i = s - 1;
 
    for(int j = s; j <= e - 1; j++) {
        if(mpren_strLT(a[j], p)) {
            i++;
            t = a[i];
            a[i] = a[j];
            a[j] = t;
        }
    }

    i++;

    t = a[i];
    a[i] = a[e];
    a[e] = t;
    
    return i;
}

static void mpren_quickSort(char **a, int s, int e)
{
    if(s < e) {
        int p = mpren_partition(a, s, e);
        mpren_quickSort(a, s, p - 1);
        mpren_quickSort(a, p + 1, e);
    }
}
