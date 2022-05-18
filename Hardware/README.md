# Control Board
Changes between v1.1 (unreleased) & v1.2
- Fixed footprint for enter button
- Added 5.0V/3.3V level shifter, and ESD protection for I2C lines. This will prevent random glitches in I2C, and keep the ESP32 happy.
- Added footprint for PCF2129AT RTC in case DS3231 becomes unavailable or too expensive. Both should not be populated at the same time.
- Changed volume to connect to GPIO32 on ESP32 instead of amp GAIN_SLOT
- Updated schematic to include some notes

[OctoPart BOM](https://octopart.com/bom-tool/zCnlEeUa)

# Time Circuits Display
Changes between v1.3 & v1.31
- Made mounting holes slightly larger
- Moved some traces so they are not as close to pins
