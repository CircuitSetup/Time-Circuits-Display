# Time Circuits Display

![![TCD Front](https://raw.githubusercontent.com/CircuitSetup/Time-Circuits-Display/master/Images/tcd_front.jpg)


This Time Circuits Display has been meticulously reproduced to be as accurate as possible to the movie. The LED displays are custom made to the correct size for CircuitSetup. This includes the month 14 segment/3 character displays being closer together, and both the 7 & 14 segment displays being 0.6" high by 0.35" wide.

The current code for the ESP32 is not yet complete. The Destination Time can be entered via keypad, and the Present Time can keep time via NTP. There is also a time travel mode, which moves the Destination Time to Present Time, and Present Time to Last Time Departed. The startup, keypad dial sounds, and time travel sounds are played using I2S. 

The displays are connected via I2C and will opperate on 0x71, 0x72, or 0x74, depending on the jumper that is set. By default: 
- Red (Destination Time) is 0x71
- Green (Present Time) is 0x72
- Yellow (Last Time Departed) is 0x74

[Displays can be purchased here.](https://circuitsetup.us/index.php/product/time-circuits-display-assembled-pcb-with-i2c-interface-red-green-or-yellow/)

In the software and on the control board (which will be released shortly), the pins used on the ESP32 by default are as follows:
- I2C:
  - SCL - 22
  - SDA - 21
- SD Card (SPI):
  - CS - 5
  - MOSI - 23
  - MISO - 19
  - SCK - 19
- Real Time Clock DS3231M (I2C addr: 0x68):
  - SQW - 15 (used to blink LEDs every second)
- Keypad PCF8574T (I2C addr: 0x20):
  - INT - 4 (can be used as an interrupt, but is not currently)
- Mono sound output I2S / MAX98357
  - DIN - 33
  - BCLK - 26
  - LRCLK - 25
- Other:
  - Keypad White LED - 17
  - keypad Enter Button - 16
  - Status LED - 2 (nodeMCU)


More to come!
