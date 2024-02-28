#!/bin/bash

# Source: https://github.com/JvanKatwijk/terminal-DAB-xxx
#https://askubuntu.com/questions/663837/unable-to-locate-package-libfaac-dev
#https://askubuntu.com/questions/378558/unable-to-locate-package-while-trying-to-install-packages-with-apt
: <<'MULTILINE_COMMENT'
sudo add-apt-repository main
sudo add-apt-repository universe
sudo add-apt-repository restricted
sudo add-apt-repository multiverse

sudo apt-get update -y
MULTILINE_COMMENT


# START HERE---!!! WORKING
sudo apt-get install git cmake -y
sudo apt-get install build-essential g++ -y
sudo apt-get install pkg-config -y
sudo apt-get install libsndfile1-dev -y
sudo apt-get install libfftw3-dev -y
sudo apt-get install portaudio19-dev -y
sudo apt-get install zlib1g-dev -y
sudo apt-get install libusb-1.0-0-dev -y
sudo apt-get install libsamplerate0-dev -y
sudo apt-get install curses -y
sudo apt-get install libncurses5-dev -y



#sudo apt-get install opencv-dev -y




sudo apt-get install libfaad-dev -y
#sudo apt-get install libfdk-aac-dev -y



sudo apt install librtlsdr-dev




git clone https://github.com/JvanKatwijk/terminal-DAB-xxx

cd terminal-DAB-xxx
mkdir build
cd build
cmake .. -DRTLSDR=ON -DPICTURES=OFF #-DFAAD=OFF
make
sudo make install




#https://vportal.ctu.cz/dab/
# find transmitter channel

terminal-DAB-rtlsdr -C 8A -C 12D -Q
terminal-DAB-rtlsdr -C 5B -C 8A -C 8B -C 11C -C 12C -Q



aplay -l
ps -af
kill -9 <PID>
