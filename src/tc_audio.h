#ifndef _TC_AUDIO_H
#define _TC_AUDIO_H

#include <Arduino.h>
#include <SPIFFS.h>
#include <AudioOutputI2S.h>
#include <AudioFileSourceSPIFFS.h>
#include <AudioFileSourceSD.h>
#include <AudioGeneratorMP3.h>

#include <FS.h>
#include <SD.h>
#include <SPI.h>

#include "driver/i2s.h"
#include "tc_keypad.h"

// SD Card
#define SD_CS 5
#define SPI_MOSI 23
#define SPI_MISO 19
#define SPI_SCK 18

//I2S audio
#define I2S_BCLK 26
#define I2S_LRCLK 25
#define I2S_DIN 33

extern void audio_setup();
extern void play_keypad_sound(char key);
extern void audio_loop();
extern void play_file(const char *audio_file);

#endif