/*
 * -------------------------------------------------------------------
 * CircuitSetup.us Time Circuits Display
 * (C) 2021-2022 John deGlavina https://circuitsetup.us
 * (C) 2022-2026 Thomas Winischhofer (A10001986)
 * https://github.com/realA10001986/Time-Circuits-Display
 * https://tcd.out-a-ti.me
 *
 * Sound handling
 *
 * -------------------------------------------------------------------
 * License: Modified MIT NON-AI
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
 * Links inside the Software pointing to the original source must not 
 * be changed or removed.
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

#ifdef TC_DBG_AUDIO
#define TC_DBG_MP     // debug music player
#endif

#include <Arduino.h>
#include "src/SD/SD.h"
#include <FS.h>

#include "AudioFileSourceLoop.h"
#include "src/ESP8266Audio/AudioFileSourcePROGMEM.h"

#include "src/ESP8266Audio/AudioGeneratorMP3.h"
#include "src/ESP8266Audio/AudioGeneratorWAV.h"

#include "src/ESP8266Audio/AudioOutputI2S.h"

#include "tc_main.h"
#include "tc_settings.h"
#include "tc_audio.h"
#include "tc_keypad.h"
#include "tc_wifi.h"

class AudioGeneratorWAVP : public AudioGeneratorWAV
{
  public:
    bool beginQuick(AudioFileSource *source, AudioOutput *output, int chnls, uint32_t sr, uint32_t stPos, uint32_t slen)
    {
        file = source;
        this->output = output;
        
        bitsPerSample = 16;
        channels = chnls;
        sampleRate = sr;
        availBytes = slen;

        file->seek(stPos, SEEK_SET);  // 12-13ms
      
        // Now set up the buffer or fail
        buff = reinterpret_cast<uint8_t *>(malloc(buffSize));
        if(!buff) {
            return false;
        }
        buffPtr = 0;
        buffLen = 0;

        sL = sR = 0;
      
        if(!output->SetRate(sampleRate)) {
            return freeBuf();
        }
        if(!output->SetBitsPerSample(bitsPerSample)) {
            return freeBuf();
        }
        if(!output->SetChannels(channels)) {
            return freeBuf();
        }
      
        if(!output->begin()) {
            return freeBuf();
        }
      
        running = true;
      
        return true;
    };
};

static AudioGeneratorMP3 *mp3;
static AudioGeneratorWAVP *wav;

static AudioFileSourceFSLoop *myFS0;
static AudioFileSourceSDLoop *mySD0;
static AudioFileSourcePROGMEM *myPM;

static AudioOutputI2S *out;

bool audioInitDone = false;
bool lookupComplete = false;

bool        muteBeep    = true;
static bool beepRunning = false;

bool            mpActive  = false;
static uint16_t *playList = NULL;
static int      mpCurrIdx = 0;

Aud_State  aud_state  = { .state = 0, .curVolume = DEFAULT_VOLUME, .curTrack = 0, .maxMusic = 0, .mpShuffle = 0 };
#ifdef HAVE_MQTT
Aud_State  mpOldState = { .state = -1 };
#endif

static const float volTable[VOL_LEVELS] = {
    0.00f, 0.02f, 0.04f, 0.06f,
    0.08f, 0.10f, 0.12f, 0.14f,
    0.16f, 0.19f, 0.22f, 0.26f, 
    0.30f, 0.35f, 0.40f, 0.50f, 
    0.60f, 0.70f, 0.80f, 0.90f, 
    1.00f
};
static const float beepLevels[4] = {
    0.1f, 0.2f, 0.3f, 0.4f
};
int           volumePin = VOLUME_PIN;
// Resolution for pot, 9-12 allowed
#define POT_RESOLUTION 9
#define VOL_SMOOTH_SIZE 4
static int    rawVol[VOL_SMOOTH_SIZE];
static int    rawVolIdx = 0;
static int    anaReadCount = 0;
static long   prev_avg, prev_raw, prev_raw2;

static float   curVolFact = 1.0f;
static bool    curChkNM   = true;
static bool    dynVol     = true;
static int     sampleCnt  = 0;
static int     mutechannels = 0;

static uint32_t key_playing = 0;
static uint32_t sig_playing = 0;
static int      alarmCanRunOut = 0;

static int           lastDoorNum = 0;
static unsigned long lastDoorSoundNow = 0;

bool          haveLineOut = false;
bool          useLineOut  = false;
static bool   playLineOut = false;

unsigned int  beepLvlIdx  = 2;
float         beepLevel   = 0.3f;

static char     keySnd[] = "/key3.mp3";   // not const
static uint32_t haveKeySnd = 0;

int8_t          mfstatus[10] = { 0 };

bool                  haveTCC = false;
static bool           sayTimeOnTheHour = false;
static const char     tcc_fn[] = "/TCC.bin";
static const uint32_t tcc_magic = (TCC_VER << 24) | 0x434354;
static int16_t        segList[5];
static int16_t        tsSegList[3] = { 1, 0 };

/*
static char     append_audio_file[32];
static float    append_vol;
static uint32_t append_flags;
static int      appendFile = 0;
*/

static const char *cachefn  = "/music%1dc";
bool          headLineShown = false;
bool          blinker       = true;
unsigned long renNow1, renNow2;

static const char dtmfFn[] = "/dtmf.bin";

static const uint16_t koffs[10] = {
         (0+44)/2,  (0x2e04+44)/2,  (0x5c00+44)/2,  (0x8a00+44)/2,  (0xb82a+44)/2,
    (0xe646+44)/2, (0x11462+44)/2, (0x14242+44)/2, (0x16ff4+44)/2, (0x19e02+44)/2
};

static const uint16_t klens[10] = {
    11780-44, 11772-44, 11776-44, 11818-44, 11804-44,
    11804-44, 11744-44, 11698-44, 11790-44, 11818-44
};

#define HHS_HAVEHRSOUND 0x80000000
static const char *hsnd     = "/hour.mp3";
static char       shsnd[]   = "/hour-00.mp3";   // Not const
static uint32_t   haveSpHrSnd = 0;

#define MAXID3LEN 2048
char id3artist[16] = { 0 };
char id3track[16]  = { 0 };

static float  getVolume();
static void   setLineOut(bool doLineOut);

static void   clear_sig_playing(int ranOut = 0);

static int    mp_findMaxNum(bool writeCache = true);
static void   mp_nextprev(bool forcePlay, bool next);
static bool   mp_play_int(bool force);
static void   mp_buildFileName(char *fnbuf, int num);
static bool   mp_renameFilesInDir(bool isSetup);
static void   mpren_insertionSort(char **a, int n);

static void   decodeID3(char *artist, char *track, int maxChrs, uint8_t *id3, int id3size);

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
    // Hardware does auto-mono, no need to ever call this later
    // (Also, the mono code is commented out in the audio lib)
    out->SetOutputModeMono(false); 
    out->SetPinout(I2S_BCLK_PIN, I2S_LRCLK_PIN, I2S_DIN_PIN);

    mp3 = new AudioGeneratorMP3();
    wav = new AudioGeneratorWAVP();

    myFS0 = new AudioFileSourceFSLoop();
    
    if(haveSD) {
        mySD0 = new AudioFileSourceSDLoop();
    }

    myPM = new AudioFileSourcePROGMEM();

    loadCurVolume();

    setBeepLevel(beepLvlIdx);

    loadMusFoldNum();
    loadShuffle();

    if(haveLineOut) {
        loadLineOut();
    }

    // MusicPlayer init
    mp_init(true);

    // Check for sound files to avoid unsuccessful file-lookups later
    audio_loopup_files();

    audioInitDone = true;

    #ifdef TC_DBG_AUDIO
    Serial.printf("haveKeySnd 0x%x, haveSPHrSnd 0x%x\n", haveKeySnd, haveSpHrSnd);
    #endif
}

void audio_loopup_files()
{
    if(lookupComplete)
        return;

    for(int i = 1, bm = 1 << 8; i < 10; i++, bm <<= 1) {
        keySnd[4] = '0' + i;
        if(check_file_SD(keySnd)) haveKeySnd |= bm;
    }

    for(int i = 0; i < 10; i++) {
        mfstatus[i] = mp_checkForFolder(i);
    }

    if(check_file_SD(hsnd)) haveSpHrSnd |= HHS_HAVEHRSOUND;

    for(int i = 0; i <= 23; i++) {
        shsnd[6] = (i / 10) + '0';
        shsnd[7] = (i % 10) + '0';
        if(check_file_SD(shsnd)) haveSpHrSnd |= (1 << i);
    }

    lookupComplete = true;
}

/*
static int checkAppend()
{
    if(appendFile) {
        play_file(append_audio_file, append_flags, append_vol);
        return 1;
    }
    return 0;
}
*/

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
            //checkAppend();
        }
    } else if(mp3->isRunning()) {
        if(!mp3->loop()) {
            mp3->stop();
            key_playing = 0;
            clear_sig_playing(alarmCanRunOut);
            if(mpActive) {    //if(!checkAppend() && mpActive) {
                mp_next(true);
            }
        } else if(dynVol) {
            sampleCnt++;
            if(sampleCnt > 1) {
                out->SetGain(getVolume(), mutechannels);
                sampleCnt = 0;
            }
        }
    } else if(mpActive) {    //if(!checkAppend() && mpActive) {
        pwrNeedFullNow();
        mp_next(true);
    }

    #ifdef HAVE_MQTT
    mp_sendStatus();
    #endif
}

void audio_loop_quick()
{
    if(wav->isRunning()) {
        if(!wav->loop()) {
            wav->stop();
            beepRunning = false;
            //checkAppend();
        }
    } else if(mp3->isRunning()) {
        if(!mp3->loop()) {
            mp3->stop();
            key_playing = 0;
            clear_sig_playing(alarmCanRunOut);
            //checkAppend();
        }
    }
}

static int32_t skipID3(uint8_t *buf)
{
    if(buf[0] == 'I' && buf[1] == 'D' && buf[2] == '3' && 
       buf[3] >= 0x02 && buf[3] <= 0x04 && buf[4] == 0 &&
       (!(buf[5] & 0x80))) {
        int32_t pos = ((buf[6] << (24-3)) |
                       (buf[7] << (16-2)) |
                       (buf[8] << (8-1))  |
                       (buf[9])) + 10;
        #ifdef TC_DBG_AUDIO
        Serial.printf("Skipping ID3 tags, seeking to %d (0x%x)\n", pos, pos);
        #endif
        return pos;
    }
    return 0;
}

static void setupEndPos(AudioFileSourceLoop *src, uint8_t *buf)
{
    src->seek(-128, SEEK_END);
    src->read((void *)buf, 3);
    src->setEndPos((buf[0] == 'T' && buf[1] == 'A' && buf[2] == 'G') ? src->getPos() - 3 : 0);
}

static void setupLoopAndBegin(AudioFileSourceLoop *src, uint32_t flags)
{
    int32_t pos = 0;
    uint8_t buf[10];

    buf[0] = 0;

    src->setEndPos(0);

    if(flags & PA_ISWAV) {
        src->setPlayLoop(false);
        wav->begin(src, out);
    } else {
        src->setPlayLoop(!!(flags & PA_LOOP));
        if(flags & PA_DOID3TS) {
            src->read((void *)buf, 10);
            pos = skipID3(buf);
            //if(flags & PA_MUSIC) {
                setupEndPos(src, buf);
            //}
            src->seek(pos, SEEK_SET);
        }
        src->setStartPos(pos);
        mp3->begin(src, out);
    }
}

void play_file(const char *audio_file, uint32_t flags, float volumeFactor)
{
    uint8_t *id3;
    int32_t pos = 0;
    #ifdef HAVE_MQTT
    bool    mpWasActive = false;
    #endif

    //appendFile = 0;   // Clear appended, append must be called AFTER play_file

    // Only signals can interrupt signals
    if(sig_playing & PA_SIGNAL) {
        if(!(flags & PA_SIGNAL)) return;
    }

    if(!(flags & PA_MUSIC)) {
        if(flags & PA_INTRMUS) {
            #ifdef HAVE_MQTT
            mpWasActive = mpActive;
            #endif
            mpActive = false;
        } else {
            if(mpActive) return;
        }
    }

    pwrNeedFullNow();
    
    #ifdef TC_DBG_AUDIO
    Serial.printf("Audio: Playing %s\n", audio_file);
    #endif

    // If something is currently on, kill it
    stopAudio();
    beepRunning = false;

    mutechannels = alarmCanRunOut = 0;

    playLineOut = (haveLineOut && useLineOut && (flags & PA_LINEOUT));
    setLineOut(playLineOut);
    if(playLineOut) {
        curChkNM = dynVol = false;
        if(flags & PA_DOOR) {
            if(flags & PA_DOORL) mutechannels = -1;
            else if(flags & PA_DOORR) mutechannels = 1;
            flags &= ~PA_KEYMASK;
        }
    } else {
        curChkNM = !!(flags & PA_CHECKNM);
        dynVol   = !!(flags & PA_DYNVOL);
        if(flags & PA_DOOR) {
            flags &= ~PA_KEYMASK;
        }
    }
    key_playing = flags & PA_KEYMASK;
    sig_playing = flags & PA_SIGMASK;
    if(sig_playing & PA_ALARM) {
        if(flags & PA_LOOP) sig_playing |= PA_LOOP;
        else                alarmCanRunOut = 1;
    }

    curVolFact = volumeFactor;

    // Reset vol smoothing
    // (user might have turned the pot while no sound was played)
    rawVolIdx = 0;
    anaReadCount = 0;

    *id3artist = *id3track = 0;

    out->SetGain(getVolume(), mutechannels);

    if(flags & PA_TCSEGS) {
        if(haveTCC && (mySD0->c = t) && mySD0->open_c(tcc_fn, (const int16_t *)audio_file)) {
            if(flags & PA_ISWAV) {
                wav->begin(mySD0, out);
            } else {
                mp3->begin(mySD0, out);
            }
        /*
         * Should we ever play signals or key sounds through segments, enable this. Not likely.
        } else {
            key_playing = 0;
            clear_sig_playing();
        */
        }
    } else if(haveSD && ((flags & PA_ALLOWSD) || FlashROMode) && mySD0->open(audio_file)) {
        if((flags & PA_DOID3TS) && ((id3 = (uint8_t *)malloc(MAXID3LEN)))) {
            id3[0] = 0;
            mySD0->read((void *)id3, 10);
            if((pos = skipID3(id3))) {
                int Id3Size = pos <= MAXID3LEN ? pos : MAXID3LEN;
                mySD0->read((void *)(id3 + 10), Id3Size - 10);
                decodeID3(id3artist, id3track, 15, id3, Id3Size);
            }
            mySD0->setPlayLoop(!!(flags & PA_LOOP));
            mySD0->setStartPos(pos);
            setupEndPos(mySD0, id3);
            free(id3);
            mySD0->seek(pos, SEEK_SET);
            mp3->begin(mySD0, out);
        } else {
            setupLoopAndBegin(mySD0, flags|PA_DOID3TS);
        }
        #ifdef TC_DBG_AUDIO
        Serial.println("Playing from SD");
        #endif
    } else if(haveFS && myFS0->open(audio_file)) {
        setupLoopAndBegin(myFS0, flags);
        #ifdef TC_DBG_AUDIO
        Serial.println("Playing from flash FS");
        #endif
    } else {
        key_playing = 0;
        clear_sig_playing();
        #ifdef TC_DBG_AUDIO
        Serial.println("Audio file not found");
        #endif
    }

    #ifdef HAVE_MQTT
    if(mpWasActive) mp_sendStatus();
    #endif
}

/*
 * Play specific sounds
 * 
 */

uint32_t play_keypad_sound(char key)
{
    uint32_t kp = key_playing;
    AudioFileSourceLoop *src = NULL;

    if(mpActive || sig_playing) return kp;

    pwrNeedFullNow();

    stopAudio();    // Clears key_playing, sig_playing, id3
    beepRunning = playLineOut = false;
    setLineOut(playLineOut);
    mutechannels = 0;
    curChkNM = true;
    curVolFact = 0.6f;
    rawVolIdx = 0;
    anaReadCount = 0;
    //appendFile = 0;

    out->SetGain(getVolume(), 0);

    // open: 26ms
    if(FlashROMode && mySD0->open(dtmfFn)) src = mySD0;
    else if(haveFS && myFS0->open(dtmfFn)) src = myFS0;

    if(src) {
        src->setPlayLoop(false);
        wav->beginQuick(src, out, 1, 32000, (uint32_t)koffs[key-'0'] << 1, (uint32_t)klens[key-'0']);
    }
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
    } else if(haveSpHrSnd & HHS_HAVEHRSOUND) {
        play_file(hsnd, PA_INTSPKR|PA_ALLOWSD);
    } else if(haveTCC && evalBool(settings.sayTOTH)) {
        say_time(-1, 1, hour, 0);
    }
}

void play_beep()
{
    bool wavRunning = wav->isRunning();
    
    if(muteBeep                               ||
       mp3->isRunning()                       ||
       (csf & (CSF_NM|CSF_OFF|CSF_AL|CSF_AE)) ||
       mpActive                               ||
       //appendFile                           ||
       (wavRunning && !beepRunning)) {
        return;
    }

    pwrNeedFullNow();

    if(wavRunning) {
        wav->stop();
    }

    setLineOut(false);
    playLineOut = false;

    curVolFact = beepLevel; //0.3f;
    curChkNM   = false;
    // Reset vol smoothing
    // (user might have turned the pot while no sound was played)
    rawVolIdx = 0;
    anaReadCount = 0;
    out->SetGain(getVolume());

    key_playing = 0;
    clear_sig_playing();
    *id3artist = *id3track = 0;

    myPM->open(data_beep_wav, data_beep_wav_len);
    wav->beginQuick(myPM, out, 1, 32000, 44, data_beep_wav_len - 44);
    beepRunning = true;
}

void play_key(int k, uint32_t preDTMFkp)
{
    uint32_t pa_key = 1 << (7+k);
    
    if(!(haveKeySnd & pa_key))
        return;

    if(pa_key == preDTMFkp) {
        // Logic for keypad sound having interrupted
        // us already; no need to stop().
        return;
    }
    if(pa_key == key_playing) {
        stopAudio();  // Clears key_playing, sig_playing, id3
        return;
    }
    
    keySnd[4] = '0' + k;
    
    play_file(keySnd, pa_key|PA_LINEOUT|PA_CHECKNM|PA_INTRMUS|PA_ALLOWSD|PA_DYNVOL);
}

void play_door_snd(int doorNum, int state, uint32_t doorFlags)
{
    // Sounds for same door may interrupt themselves; if sound for
    // other door is to be played while first door's is running, we 
    // only interrupt if reasonable part of the first door's sound
    // is already played back.
    unsigned long now = millis();
    if((!(csf & CSF_ST)) && ((!(csf & (CSF_P0|CSF_P1|CSF_RE))) || !playTTsounds || checkAudioFree())) {
        if((lastDoorNum == doorNum) || !lastDoorNum || (now - lastDoorSoundNow > 750)) {
            play_file((state < 0) ? "/doorclose.mp3" : "/dooropen.mp3", 
                        doorFlags|PA_LINEOUT|PA_CHECKNM|PA_INTRMUS|PA_ALLOWSD);
            lastDoorSoundNow = now;
            lastDoorNum = doorNum;
        }
    }
}

void play_ts_snd(int16_t *s)
{
    unsigned long now = millis();
    memcpy((void *)tsSegList, (void *)s, 3*2);
    if(!tsSegList[0] || tsSegList[0] > 2) return;
    if(haveTCC && (!(csf & CSF_ST)) && ((!(csf & (CSF_P0|CSF_P1|CSF_RE))) || !playTTsounds || checkAudioFree())) {
        play_file((const char *)tsSegList, PA_TCSEGS|PA_LINEOUT|PA_CHECKNM|PA_INTRMUS);
    }
}

bool say_time(int pbt, int whichone, int gh, int gm)
{
    int h, m;
    
    if(haveTCC) {

        // We are not called when mp is active, but better safe than sorry.
        mp_stop();
        stopAudio();

        // pbt: -1 soth, 0 normal, 1 fancy; wo: -1 = displayed, 0 = current, 1 = given
        get_time_segs(pbt, whichone, segList, gh, gm);

        play_file((const char *)segList, PA_TCSEGS|PA_LINEOUT|PA_CHECKNM);
        return true;
    }
    
    return false;
}

/*
 * Append file to currently played one
 * Unused on TCD
 */
/*
void append_file(const char *audio_file, uint32_t flags, float volumeFactor)
{
    if(strlen(audio_file) >= sizeof(append_audio_file) - 1) {
        #ifdef TC_DBG_AUDIO
        Serial.printf("Internal error: Sound file name too long (%d vs max %d)\n", strlen(audio_file), sizeof(append_audio_file));
        #endif
        return;
    }
    strcpy(append_audio_file, audio_file);
    append_flags = flags;
    append_vol = volumeFactor;
    appendFile = 1;
}

bool append_pending()
{
    return appendFile;
}
*/

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

    if((raw + prev_raw + prev_raw2 > 0) && vol_val < 0.01f) vol_val = 0.01f;

    prev_raw2 = prev_raw;
    prev_raw = raw;

    //Serial.println(vol_val);

    return vol_val;
}

static float getVolume()
{
    float vol_val = 1.0f;

    if(!playLineOut) {
        if(aud_state.curVolume == 255) {
            vol_val = getRawVolume();
        } else {
            vol_val = volTable[aud_state.curVolume];
        }

        // If user muted, return 0
        if(vol_val == 0.0f) return vol_val;
    }

    vol_val *= curVolFact;

    // Reduce volume in night mode, if requested
    if(curChkNM && (csf & CSF_NM)) {
        vol_val *= 0.3f;
    }

    // Do not totally mute
    // 0.02 is the lowest audible gain
    if(vol_val < 0.02f) vol_val = 0.02f;
    //else if(vol_val > 1.0f) vol_val = 1.0f;

    return vol_val;
}

void setBeepLevel(unsigned int levelIdx)
{
    if(levelIdx > 3) levelIdx = 3;
    beepLvlIdx = levelIdx;
    beepLevel = beepLevels[levelIdx];
}

static void setLineOut(bool doLineOut)
{
    if(haveLineOut) {
        if(useLineOut && doLineOut) {
            // Mute/un-mute secondary DAC? No, for now.
            //digitalWrite(MUTE_LINEOUT_PIN, doLineOut ? HIGH : LOW);
            // Switch off MAX98357, switch on PCM510x
            digitalWrite(SWITCH_LINEOUT_PIN, HIGH);
        } else {
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
    #ifdef TC_DBG_AUDIO
    Serial.printf("check_file_SD: Checking for %s\n", audio_file);
    #endif
    return (haveSD && SD.exists(audio_file));
}

static unsigned int check_file_len_SD(const char *audio_file, uint8_t *tbuf = NULL, uint32_t tsz = 0)
{
    unsigned int s = 0;
    if(haveSD) {
        File file;
        if(file = SD.open(audio_file, FILE_READ)) {
            s = file.size();
            if(tbuf && tsz) {
                if(file.read(tbuf, tsz) != tsz) s = 0;
            }
            file.close();
        }
    }
    return s;
}

void checkForTCC()
{
    unsigned int sps = 0;
    uint32_t tbuf[3];
    
    if((sps = check_file_len_SD(tcc_fn, (uint8_t *)&tbuf[0], 12))) {
        haveTCC = ((tbuf[0] == tcc_magic) && (tbuf[1] == sps ^ tcc_magic));
    }
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

// Audio is really done (beep included)
bool checkAudioDone()
{
    if(mp3->isRunning() || wav->isRunning()) return false;
    return true;
}

// Audio is free for use; beep is low prio and ignored
bool checkAudioFree()
{
    if(mp3->isRunning() || (wav->isRunning() && !beepRunning)) return false;
    return true;
}

bool checkMP3Running()
{
    return mp3->isRunning();
}

uint32_t isSignalPlaying()
{
    return sig_playing;
}

bool isUISignalPlaying()
{
    return !!(sig_playing & PA_SIGNAL);
}

void stopAudio()
{
    if(mp3->isRunning()) {
        mp3->stop();
    } else if(wav->isRunning()) {
        wav->stop();
    }
    key_playing = 0;    
    clear_sig_playing();
    *id3artist = *id3track = 0;
    //appendFile = 0;   // Clear appended, stop means stop.
}

void stop_key()
{
    if(key_playing) {
        stopAudio();
    }
}

void stopAlarm(bool force)
{
    if(sig_playing & PA_ALARM) {
        if(!force && (sig_playing & PA_LOOP)) {
            if(haveSD) mySD0->setPlayLoop(false);
            if(haveFS) myFS0->setPlayLoop(false);
        } else {
            stopAudio();
        }
    }

    // Cancel timeout in main_loop().
    // Do not do that in clear_sig_playing; if the alarm
    // is interrupted by another sound, stopAudio() calls
    // clear_sig_playing(), which clears CSF_AL and thereby
    // releases the keypad. The timeout-loop in main_loop() 
    // (controlled by alarmPlaying) can continue to run and
    // eventually initiate auto-snooze. (Note that this does
    // not work for non-looped alarm sounds.)
    // Also, we use the duality of alarmPlaying vs CSF_AL 
    // to handle interruptions of the alarm by the Timer.
    alarmPlaying = 0;
    alf &= ~ALF_RANOUT;
}

static void clear_sig_playing(int ranOut)
{
    if(sig_playing & PA_ALARM) {
        // Check CSF_AL so we only ever set ALF_RANOUT in Extended Mode
        if(csf & CSF_AL) {
            csf &= ~CSF_AL;
            if(ranOut) alf |= ALF_RANOUT;
        }
    }
    sig_playing = 0;
}

/*
 * ID3 handling
 */

static void copyId3String(uint8_t *src, char *dst, int tagSz, int maxChrs)
{
    uint8_t *send = src + tagSz;
    char    *tend = dst + maxChrs;
    uint8_t enc = *src++;
    uint16_t chr;
    uint8_t c;

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
                *dst++ = (char)c;
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
        filterOutUTF8((char *)src, dst, tagSz - 1, maxChrs);
        break;
    }
}

static void decodeID3(char *artist, char *track, int maxChrs, uint8_t *id3, int Id3Size)
{
    uint8_t rev = id3[3];
    uint8_t *ptr  = id3 + 10;
    uint8_t *eptr = id3 + Id3Size;
    int     stopLoop = 0;
    uint8_t tag0, tag1, tag2, tag3;
    uint8_t badFlags = 0;
    unsigned long tagSz, offSet;
    //uint8_t tFlags0 = 0;
    uint8_t tFlags1 = 0;

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
            
            //tFlags0 = ptr[4];   // Status - don't care
            tFlags1 = ptr[5];

            ptr += 6;

        }

        if(ptr + tagSz > eptr) return;

        // Compression & unsynchronization not supported
        if(!(tFlags1 & badFlags) && (tag0 == 'T')) {
            // Check for grouping ID, and skip if there.
            offSet = (rev >= 3 && (tFlags1 & 0x40)) ? 1 : 0;
            // Check for data length indicator despite no other flags
            // Should not happen; we ignore it if it is there
            offSet += (rev == 4 && (tFlags1 & 0x01)) ? 4 : 0;
            if((rev == 2 && tag1 == 'T' && tag2 == '2') ||
               (rev != 2 && tag1 == 'I' && tag2 == 'T' && tag3 == '2')) {
                // Copy song title
                copyId3String(ptr+offSet, track, tagSz-offSet, maxChrs);
                stopLoop |= 1;
            } else if((tag1 == 'P') && 
                      ((rev == 2 && tag2 == '1') ||
                       (rev != 2 && tag2 == 'E' && tag3 == '1'))) {
                // Copy artist
                copyId3String(ptr+offSet, artist, tagSz-offSet, maxChrs);
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
    int t;
    
    csf |= CSF_NOMUSIC;

    if(playList) {
        free(playList);
        playList = NULL;
    }

    mpCurrIdx = aud_state.curTrack = aud_state.maxMusic = 0;
    
    if(haveSD) {
        #ifdef TC_DBG_MP
        Serial.println("MusicPlayer: Checking for music files");
        #endif

        if(mp_renameFilesInDir(isSetup)) {
        
            if((t = mp_findMaxNum()) >= 0) {

                csf &= ~CSF_NOMUSIC;
                
                aud_state.maxMusic = t;
                #ifdef TC_DBG_MP
                Serial.printf("MusicPlayer: last file num %d\n", aud_state.maxMusic);
                #endif
    
                if(!(playList = (uint16_t *)malloc((t + 1) * 2))) {
    
                    csf |= CSF_NOMUSIC;
                    #ifdef TC_DBG_MP
                    Serial.println("MusicPlayer: Failed to allocate PlayList");
                    #endif
    
                } else {
    
                    // Init play list
                    mp_makeShuffle(!!aud_state.mpShuffle);

                    aud_state.curTrack = playList[0];
                    
                }

            } else {
                #ifdef TC_DBG_MP
                Serial.printf("MusicPlayer: mp_findMaxNum returned -1 for folder %d\n", musFolderNum);
                #endif
            }

        } else {
            #ifdef TC_DBG_MP
            Serial.printf("MusicPlayer: mp_renameFilesInDir failed for folder %d\n", musFolderNum);
            #endif
        }
    }

    #ifdef HAVE_MQTT
    mp_sendStatus();
    #endif
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

static bool checkCacheFile(char *fn, int& result)
{
    int j, k;
    uint8_t buf[4];
    
    result = -1;

    if(readFileFromSD(fn, buf, 4)) {
        k = buf[0] | (buf[1] << 8);
        j = (buf[2] | (buf[3] << 8)) ^ 0xaa55;
        if(k == j) {
            if(j == 0xffff) return true;
            else if(j <= 999) { result = j; return true; }
        }
        deleteFileFromSD(fn);
    }

    return false;
}

// Find highest track number.
// Returns -1 if no audio files present
static int mp_findMaxNum(bool writeCache)
{
    int i = -1, j;
    uint8_t buf[4];
    char fnbuf[32];

    sprintf(fnbuf, cachefn, musFolderNum);

    if(checkCacheFile(fnbuf, j))
        return j;

    if(mp_checkForFile(0)) {

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

    }

    if(writeCache) {
        buf[0] = i & 0xff;
        buf[1] = i >> 8;
        j = i ^ 0xaa55;
        buf[2] = j & 0xff;
        buf[3] = j >> 8;
        if(writeFileToSD(fnbuf, buf, 4)) {
            #ifdef TC_DBG_MP
            Serial.printf("find_max: Wrote %s (%d)\n", fnbuf, i);
            #endif
        }
    }
    
    return i;
}

void mp_makeShuffle(bool enable)
{
    int numMsx = aud_state.maxMusic + 1;

    aud_state.mpShuffle = enable ? 1 : 0;
    saveShuffle();

    if(!(csf & CSF_NOMUSIC)) {
    
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
            /*
            #ifdef TC_DBG_MP
            for(int i = 0; i <= aud_state.maxMusic; i++) {
                Serial.printf("%d ", playList[i]);
                if((i+1) % 16 == 0 || i == aud_state.maxMusic) Serial.printf("\n");
            }
            #endif
            */
        }

    }

    #ifdef HAVE_MQTT
    mp_sendStatus();
    #endif
}

void mp_play(bool forcePlay)
{
    int oldIdx = mpCurrIdx;

    if((csf & CSF_NOMUSIC) || isSignalPlaying()) return;
    
    do {
        if(mp_play_int(forcePlay)) {
            break;
        }
        mpCurrIdx++;
        if(mpCurrIdx > aud_state.maxMusic) mpCurrIdx = 0;
    } while(oldIdx != mpCurrIdx);
}

bool mp_stop(bool forceStatus)
{
    bool ret = mpActive;
    
    if(mpActive) {
        mp3->stop();
        mpActive = false;
        *id3artist = *id3track = 0;
        #ifdef HAVE_MQTT
        mp_sendStatus();
        #endif
    #ifdef HAVE_MQTT
    } else if(forceStatus) {
        mp_sendStatus();
    #endif
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

    if((csf & CSF_NOMUSIC) || isSignalPlaying()) return;
    
    do {
        if(next) {
            mpCurrIdx++;
            if(mpCurrIdx > aud_state.maxMusic) mpCurrIdx = 0;
        } else {
            mpCurrIdx--;
            if(mpCurrIdx < 0) mpCurrIdx = aud_state.maxMusic;
        }
        if(mp_play_int(forcePlay)) {
            break;
        }
    } while(oldIdx != mpCurrIdx);
}

int mp_gotonum(int num, bool forcePlay)
{
    if(csf & CSF_NOMUSIC) return 0;

    if(num < 0) num = 0;
    else if(num > aud_state.maxMusic) num = aud_state.maxMusic;

    if(aud_state.mpShuffle) {
        for(int i = 0; i <= aud_state.maxMusic; i++) {
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
        if(force) play_file(fnbuf, PA_MUSIC|PA_LINEOUT|PA_DOID3TS|PA_CHECKNM|PA_INTRMUS|PA_ALLOWSD|PA_DYNVOL);
        mpActive = force;
        aud_state.curTrack = playList[mpCurrIdx];
        #ifdef HAVE_MQTT
        mp_sendStatus();
        #endif
        return true;
    }
    return false;
}

int mp_get_currently_playing()
{
    if((csf & CSF_NOMUSIC) || !mpActive)
        return -1;

    return aud_state.curTrack;
}

#ifdef HAVE_MQTT
void mp_sendStatus(int force)
{
    if(pubMP && mqttConnected()) {
        aud_state.state = (csf & (CSF_OFF|CSF_MA|CSF_ST|CSF_P0|CSF_P1|CSF_RE|CSF_AL|CSF_AE|CSF_REBOOT|CSF_NOMUSIC)) ? 0 : (mpActive ? 1 : 2);         
        if(memcmp((void *)&mpOldState, (void *)&aud_state, sizeof(aud_state)) || force) {
            static const char statec[] = "OPI";
            char msg[128];
            sprintf(msg, 
                "{\"S\":\"%c\",\"C\":\"%d\",\"V\":\"%d\",\"F\":\"0\",\"L\":\"%d\",\"SH\":\"%d\"}", 
                    statec[aud_state.state], 
                    aud_state.curTrack, 
                    (aud_state.curVolume == 255) ? -1 : (aud_state.curVolume * 100 / (VOL_LEVELS - 1)), 
                    aud_state.maxMusic, 
                    aud_state.mpShuffle);
            if(mqttPublish("bttf/tcd/mpstatus", msg, strlen(msg) + 1)) {
                memcpy((void *)&mpOldState, (void *)&aud_state, sizeof(aud_state));
            } else {
                mpOldState.state = -1;
            }
        }
    }
}
#endif

static void mp_buildFileName(char *fnbuf, int num)
{
    sprintf(fnbuf, "/music%1d/%03d.mp3", musFolderNum, num);
}

// For keypad menu only
int mp_checkForFolder(int num)
{
    char fnbuf[32];
    char fnbuf2[32];
    int t;

    // returns 
    // 1 if folder is ready (valid cache file, 0-999)
    // 0 if folder does not exist
    // -1 if folder exists but needs processing (no cache)
    // -2 if musicX contains no audio files (checkCacheFile reporting -1)
    // -3 if musicX is not a folder
    // -4 if no SD

    if(!haveSD)
        return -4;

    if(num < 0 || num > 9)
        return 0;

    sprintf(fnbuf, "/music%1d", num);
    sprintf(fnbuf2, cachefn, num);

    File origin = SD.open(fnbuf);

    // If folder does not exist, return 0
    if(!origin) {
        deleteFileFromSD(fnbuf2);
        return 0;
    }

    // Check if folder is folder
    if(!origin.isDirectory()) {
        // If musicX is not a folder, return -3
        origin.close();
        deleteFileFromSD(fnbuf2);
        return -3;
    }
    origin.close();

    // Check cache file
    if(checkCacheFile(fnbuf2, t)) {
        if(t >= 0) return 1;
        else return -2;
    }
      
    // cache not present (or invalid): Needs processing
    return -1;
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

    // Filename shorter than "x.mp3"? Ignore.
    if(s < 5) return true;

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
        #ifdef ACAR_DISPLAY
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

    // We are only ever called from keypad menu (or from audio_setup,
    // but in that case the loop calls are skipped)
    // So speedoUpdate_loop(true) is appropriate.

    if(now - renNow1 > 250) {
        wifi_loop();
        if(!isSetup) {
            ntp_loop();
            #if defined(HAVE_GPS) || defined(HAVE_RE) || defined(HAVE_REMOTE)
            speedoUpdate_loop(true);
            #endif
            while(bttfn_loop(BNLP_SK_EXPIRE)) { }
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
            allOn();
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
    char **a, **d;
    char *c;
    int count = 0;
    int fileNum = 0;
    int strLength;
    int nameOffs = 8;
    int allocBufIdx = 0;
    static const unsigned long bufSizes[8] = {
        16384, 16384, 8192, 8192, 8192, 8192, 8192, 4096 
    };
    char *bufs[8] = { NULL };
    unsigned long sz, bufSize;
    bool stopLoop = false;
    bool isDir;
    bool hls = false;
    #ifdef TC_DBG_MP
    const char *funcName = "MusicPlayer/Renamer: ";
    #endif

    headLineShown = false;
    blinker = true;
    renNow1 = renNow2 = millis();

    // We check for basics (folder exists, is a folder)
    // then we look for the cache file. If these checks
    // pass, we assume everything in order.

    sprintf(fnbuf3, cachefn, musFolderNum);
    sprintf(fnbuf, "/music%1d", musFolderNum);

    // Open folder and check if it exists and is actually a folder
    File origin = SD.open(fnbuf);
    if(!origin) {
        deleteFileFromSD(fnbuf3);
        return false;
    }
    if(!origin.isDirectory()) {
        origin.close();
        deleteFileFromSD(fnbuf3);
        return false;
    }

    // Check cache file
    if(checkCacheFile(fnbuf3, strLength)) {
        origin.close();
        #ifdef TC_DBG_MP
        Serial.printf("%s%s exists and is valid\n", funcName, fnbuf3);
        #endif
        return true;
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

    String fileName = origin.getNextFileName(&isDir);
    // Check if File::name() returns FQN or plain name
    if(fileName.length() > 0) nameOffs = (fileName.charAt(0) == '/') ? 8 : 0;
    
    while(!stopLoop && fileName.length() > 0) {

        mpren_looper(isSetup, true, 0);

        if(!isDir) {
            const char *fn = fileName.c_str();
            if(!mpren_checkFN(fn + nameOffs)) {
                strLength = strlen(fn);
                sz = strLength - nameOffs - 4 + 1;
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
                    *d++ = c;
                    memcpy(c, fn + nameOffs, sz - 1);
                    c[sz -1] = 0;
                    //strcpy(c, fn + nameOffs);
                    #ifdef TC_DBG_MP
                    Serial.printf("%sAdding '%s' (%d)\n", funcName, c, sz);
                    #endif
                    c += sz;
                    bufSize -= sz;
                    fileNum++;
                } else if(sz > bufSize) {
                    stopLoop = true;
                    #ifdef TC_DBG_MP
                    Serial.printf("%sSort buffer(s) exhausted, %d files stored, remaining files ignored\n", fileNum, funcName);
                    #endif
                }
            }
        }
        
        if(fileNum >= 1000) stopLoop = true;

        if(!stopLoop) {          
            fileName = origin.getNextFileName(&isDir);
        }
    }

    origin.close();

    #ifdef TC_DBG_MP
    Serial.printf("%s%d files to process\n", funcName, fileNum);
    #endif

    // Sort file names, and rename

    if(fileNum) {

        char fnbuf2[256+8];
        
        // Sort file names
        mpren_insertionSort(a, fileNum);
    
        sprintf(fnbuf2, "/music%1d/", musFolderNum);
        strcpy(fnbuf, fnbuf2);

        // If 000.mp3 exists, find current count
        // the usual way. Otherwise start at 000.
        strcpy(fnbuf + 8, "000.mp3");
        if(SD.exists(fnbuf)) {
            count = mp_findMaxNum(false) + 1;
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
            strcat(fnbuf2, ".mp3");
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
            #ifdef TC_DBG_MP
            Serial.printf("%sRenamed '%s' to '%s'\n", funcName, fnbuf2, fnbuf);
            #endif
            
            count++;
        }
    }

    for(int i = 0; i <= allocBufIdx; i++) {
        if(bufs[i]) free(bufs[i]);
    }
    free(a);

    // Find max track num and save it to new cache file
    mp_findMaxNum();

    // Update mfstatus for current folder
    mfstatus[musFolderNum] = mp_checkForFolder(musFolderNum);

    // Clear displays
    if(hls || headLineShown) {
        destinationTime.showTextDirect("");
        presentTime.showTextDirect("");
        departedTime.showTextDirect("");
    }

    return true;
}

/*
 * Insertion Sort for file names
 */

static unsigned char mpren_toUpper(char a)
{
    if(a >= 'a' && a <= 'z')
        a &= ~0x20;

    return (unsigned char)a;
}

static bool mpren_strGT(const char *a, const char *b)
{
    int aa = strlen(a);
    int bb = strlen(b);
    int cc = aa < bb ? aa : bb;

    for(int i = 0; i < cc; i++) {
        unsigned char aaa = mpren_toUpper(*a);
        unsigned char bbb = mpren_toUpper(*b);
        if(aaa < bbb) return false;
        if(aaa > bbb) return true;
        a++; b++;
    }

    return false;
}

static void mpren_insertionSort(char **a, int n)
{
    for(int i = 1; i < n; i++) {
        char *k = a[i];
        int j = i - 1;
        while(j >= 0 && mpren_strGT(a[j], k)) {
            a[j+1] = a[j];
            j--;
        }
        a[j + 1] = k;
    }
}
