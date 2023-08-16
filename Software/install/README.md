This folder holds all files necessary for immediate installation on your Time Circuits Display. Here you'll find
- a binary of the current firmware, ready for upload to the device;
- the latest audio files

### Firmware installation

#### Updating Firmware
If a previous version of the Time Circuits firmware was installed on your device, you can upload the provided binary to update to the current version: 
- Download the latest version of the firmware from here
- Go to the TCD Config Portal ([https://github.com/CircuitSetup/Time-Circuits-Display/wiki/8.-WiFi-Connection-&-TCD-Settings#connecting-to-your-wifi-network](see here for how to connect to it) - you do not need to connect the TCD to your WiFi network to access it)
- Click on "Update"
- Select the previously downloaded binary (.bin)
- Click "Update"

The TCD will restart.

#### Clean Install
For a fresh installation, the provided binary is not usable. You'll need to use the Arduino IDE or PlatformIO, download the sketch source code, all required libraries (info in the timecircuits.ino file) and compile it. Then upload the sketch to the device. This method is the one for fresh ESP32 boards and/or folks familiar with Arduino programming.

#### Audio file installation

The sound pack is not updated as often as the firmware itself. If you have previously installed the latest version of the sound-pack, you normally don't have to re-install the audio files when you update the firmware. Only if either a new version of the sound-pack is released here, or your clock is quiet after a firmware update, a re-installation is needed.

- Download "sound-pack-xxxxxxxx.zip" and extract it to the root directory of of a FAT32 formatted SD card
- power down the clock,
- insert this SD card into the device's slot and 
- power up the clock.

If (and only if) the exact and complete contents of sound-pack archive is found on the SD card, the clock will show "INSTALL AUDIO FILES?" after power-up. Press ENTER briefly to toggle between "CANCEL" and "PROCEED". Choose "PROCEED" and hold the ENTER key for 2 seconds.
