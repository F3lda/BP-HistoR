# HistoRPi
Embedded system based on Raspberry Pi platform for receiving audio streams on historical radios by simulating FM and AM broadcasts using SDR. This application also allows you to play audio streams from the internet, SD card, Bluetooth or
available FM and DAB broadcasts to any audio output device connected to the Raspberry Pi.

## Installation
1. Set-up Raspberry Pi with Raspbian OS using username '`histor`'.
2. Copy files `install-HistoRPi.sh` and `HistoRPi.py` to the user home directory `/home/histor/`.
3. Use command `chmod 777 install-HistoRPi.sh` to make file executable.
4. Run `./install-HistoRPi.sh`.

## Start
-  Connect to the Raspberry Pi via browser by entering its IP address or via its Wi-Fi hotspot.
	- To get its IP address you can listen on default audio output device.
- In the HistoRPi web app, you can play selected audio streams to individual Raspberry Pi audio output devices and broadcast the audio stream of the selected audio output device on AM or FM frequencies using a software-defined radio.
- DAB and FM broadcasts can be received by connecting an RTL-SDR dongle.
- Start playing and Listen!

## Video
[https://www.youtube.com/watch?v=wEk9IPGQwg4](https://www.youtube.com/watch?v=wEk9IPGQwg4&list=PLyx6PxqS5pZ5c0ZQkjKCWe2286lkeXFXk)
