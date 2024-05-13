
# HistoRinvaz
Invasive ESP32-based solution for playing Internet audio streams on vintage radios. Audio stream selection is done by connecting the ESP32 to the radio's tuning capacitor. Playback is done with an internal DAC converter.

## Installation
1. Connect ESP32 pins 32 and 33 to the tuning capacitor disconnected from the vintage radio electronics.
2. Connect pins 25 (right), 26 (left) and GND to the radio's internal or other external speaker.
3. Upload `HistoRinvaz.ino.esp32.bin` binary or source code with `HistoRinvaz.ino` root file via Arduino IDE (version 1.8.13 used) using Partition Scheme `Huge APP` to ESP32.

## Start
- Connect to the ESP32 via a browser by entering its IP address or via its Wi-Fi hotspot.
- Add your URL audio streams at different tuning capacitor values.
- Listen!

## Video
[https://www.youtube.com/watch?v=tp0sWiJ3ixc](https://www.youtube.com/watch?v=tp0sWiJ3ixc&list=PLyx6PxqS5pZ5c0ZQkjKCWe2286lkeXFXk)
