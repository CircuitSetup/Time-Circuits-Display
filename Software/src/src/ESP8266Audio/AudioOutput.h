/*
  AudioOutput
  Base class of an audio output player

  Copyright (C) 2017  Earle F. Philhower, III

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.

  Adapted by Thomas Winischhofer, 2023-2025
*/

#ifndef _AUDIOOUTPUT_H
#define _AUDIOOUTPUT_H

#include <Arduino.h>
#include "AudioLogger.h"
#include "AudioOutputLocal.h"

class AudioOutput
{
  public:
    AudioOutput() { };
    virtual ~AudioOutput() {};
    virtual bool SetRate(int hz) { hertz = hz; return true; }
    virtual bool SetBitsPerSample(int bits) { bps = bits; return true; }
    virtual bool SetChannels(int chan) { channels = chan; return true; }
    #ifndef TWESP32
    virtual bool SetGain(float f) {
              if (f>4.0) f = 4.0;
              else if (f<0.0) f=0.0;
              gainF2P6 = (uint8_t)(f*(1<<6));
              return true;
    }
    #else
    virtual bool SetGain(float f1, int mutechnls = 0) {
              int16_t g;
              // TW: We know the limits. Also, 4.0 would be 0, hence mute.
              //if (f1>4.0) f1 = 4.0; if (f1<0.0) f1=0.0;
              g = (int16_t)(f1*(1<<6));
              if(!mutechnls)           gainF2P6_R = gainF2P6_L = g;
              else if(mutechnls > 0) { gainF2P6_R = g; gainF2P6_L = 0; }
              else {                   gainF2P6_L = g; gainF2P6_R = 0; }
              gainF2P6_R <<= 10;
              return true;
    }
    #endif
    virtual bool begin() { return false; };
    typedef enum { LEFTCHANNEL=0, RIGHTCHANNEL=1 } SampleIndex;
    #ifdef TWESP32
    virtual size_t ConsumeSample(int16_t sL, int16_t sR) { (void)sL;(void)sR; return 0; }
    #else
    virtual bool ConsumeSample(int16_t sL, int16_t sR) { (void)sL;(void)sR; return false; }
    #endif
    /*
    virtual uint16_t ConsumeSamples(int16_t *samples, uint16_t count)
    {
      for (uint16_t i=0; i<count; i++) {
        if (!ConsumeSample(samples)) return i;
        samples += 2;
      }
      return count;
    }
    */
    virtual bool stop() { return false; }
    virtual void flush() { return; }
    virtual bool loop() { return true; }

  protected:
    #ifndef TWESP32
    void MakeSampleStereo16(int16_t& sL, int16_t& sR) {
      // Mono to "stereo" conversion
      if (channels == 1)
        sR = sL;
      if (bps == 8) {
        // Upsample from unsigned 8 bits to signed 16 bits
        sL = (((int16_t)(sL&0xff)) - 128) << 8;
        sR = (((int16_t)(sR&0xff)) - 128) << 8;
      }
    };
    #endif

    // TW: This relies on a gcc extension to the C standard: Shifting negative
    // values is sign-extended with gcc. In standard C, behavior is undefined.
    #ifndef TWESP32
    inline int16_t Amplify(int16_t s) {
      int32_t v = (s * gainF2P6)>>6;
      if (v < -32767) return -32767;
      else if (v > 32767) return 32767;
      else return (int16_t)(v&0xffff);
    }
    #else
    inline void AmplifyL(int16_t& s) {
      int32_t v = (s * gainF2P6_L) >> 6;
      // TW: We NEVER amplify, we only ever attenuate
      s = v;
    }
    inline int32_t AmplifyR(int16_t s) {
      return (s * gainF2P6_R) & 0xffff0000;
      // TW: We NEVER amplify, we only ever attenuate
    }
    #endif

  protected:
    uint16_t hertz;
    uint8_t bps;
    uint8_t channels;
    #ifndef TWESP32
    uint8_t gainF2P6; // Fixed point 2.6
    #else
    int16_t gainF2P6_L; // Fixed point 2.6
    int32_t gainF2P6_R; // Fixed point 2.6
    #endif
};

#endif

