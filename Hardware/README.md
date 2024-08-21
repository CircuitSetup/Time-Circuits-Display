#PCB Revision History
## Control Board
v1.4.5 (8/1/24)
- Consolodated inputs/outputs into 8 pin screw connector
- Added label for TFC connections
- Added ground to audio line output screw connector
- Moved reset and io0 buttons to back of the board
- Added ability to switch between line out and speaker audio

v1.4 (7/1/24)
- Added dedicated audio line out
- Switched to original style TRW enter button
- Added on-board ESP32 module
- Added 3.3V power supply
- Added serial USB chip with ESD filtering
- Added micro-USB connector to back of board
- Moved RTC battery to rear of the PCB for ease of changing
- Changed volume pot to rear facing
- Moved micro-SD card to the rear of the board, and swtiched to a flip out holder
- 3d printed lens holder is now screwed to the PCB
- Changed Speedo and I2C to 2.54mm screw connectors
- Removed PCF2129AT and extra caps footprints
- Changed to TRW style mounting holes (3)

v1.3 (7/1/22)
- Added switch for lens LEDs
- Added header for TT In (GPIO 27)
- Added reverse polarity protection for power connectors
- Split resistors for keypad LEDs to make it so green LED can be brighter
- Added labels for PWR Trigger, TT Out, TT In, and Speedo

v1.2 (6/1/21)
- Fixed footprint for enter button
- Added 5.0V/3.3V level shifter, and ESD protection for I2C lines. This will prevent random glitches in I2C, and keep the ESP32 happy.
- Added footprint for PCF2129AT RTC in case DS3231 becomes unavailable or too expensive. Both should not be populated at the same time.
- Changed volume to connect to GPIO32 on ESP32 instead of amp GAIN_SLOT
- Updated schematic to include some notes

[OctoPart BOM](https://octopart.com/bom-tool/zCnlEeUa)

## Time Circuits Display
v1.31
- Made mounting holes slightly larger
- Moved some traces so they are not as close to pins
