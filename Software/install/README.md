This folder holds all files necessary for immediate installation on your Time Circuits Display. Here you'll find
- a binary of the current firmware, ready for upload to the device;
- the latest audio data

## Firmware Installation

If a previous version of the Time Circuits Display firmware is installed on your device, you can update easily using the pre-compiled binary. Enter the [Config Portal](https://github.com/CircuitSetup/Time-Circuits-Display/tree/master?tab=readme-ov-file#the-config-portal), click on "Update" and select the pre-compiled binary file provided in this repository. 

## Audio data installation

The firmware comes with some audio data ("sound-pack") which needs to be installed separately. The audio data is not updated as often as the firmware itself. If you have previously installed the latest version of the sound-pack, you normally don't have to re-install the audio data when you update the firmware. Only if the TCD displays "PLEASE INSTALL AUDIO FILES" during boot, an update of the audio data is needed.

The first step is to download "install/sound-pack-xxxxxxxx.zip" and extract it. It contains one file named "TCDA.bin".

Then there are two alternative ways to proceed. Note that both methods *require an SD card*.

1) Through the Config Portal. Click on *Update*, select the "TCDA.bin" file in the bottom file selector and click on *Upload*. Note that an SD card must be in the slot during this operation.

2) Via SD card:
- Copy "TCDA.bin" to the root directory of of a FAT32 formatted SD card;
- power down the TCD,
- insert this SD card into the slot and 
- power up the TCD; the audio data will be installed automatically.

See also [here](https://github.com/CircuitSetup/Time-Circuits-Display/tree/master?tab=readme-ov-file#audio-data-installation).
