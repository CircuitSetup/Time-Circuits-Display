//Connect all displays to 5V and I2C (SDA/SCL) on the displays
//Load this scetch on an Arduino compatible device
//all displays should light up every LED sequencially

#include <Arduino.h>
#include <Wire.h>

uint16_t displayBuffer[8];
const uint8_t displayAddr[]= {0x71,0x72,0x74};

void setup() {
  Wire.begin();

  for(int i = 0; i < 3 ; i++){
    Wire.beginTransmission(displayAddr[i]);
    Wire.write(0x20 | 1); // turn on oscillator
    Wire.endTransmission();
    
    Wire.beginTransmission(displayAddr[i]);
    Wire.write(0x80 | 0 << 1 | 1); // Blinking / blanking command
    Wire.endTransmission();
    
    Wire.beginTransmission(displayAddr[i]);
    Wire.write(0xE0 | 15); // Dimming command
    Wire.endTransmission();
  }

  for(int i = 0; i < 8; i++){
    for(int k = 0; k < 16; k++){
      displayBuffer[i] |= 1 << k;
      for(int c = 0; c < 3; c++){
        show(displayAddr[c]);
      }
      delay(10);
      for(int c = 0; c < 3; c++){
        show(displayAddr[c]);
      }
    }
    for(int c = 0; c < 3; c++){
      displayBuffer[i]|= B11111111+B11111111*256;
      show(displayAddr[c]);
    }
  }
}

void show(uint8_t addr){
  Wire.beginTransmission(addr);
  Wire.write(0x00); // start at address 0x0
  for (byte i = 0; i < 8; i++) {
    Wire.write(displayBuffer[i] & 0xFF);    
    Wire.write(displayBuffer[i] >> 8);    
  }
  Wire.endTransmission();  
}

void loop() {

}
