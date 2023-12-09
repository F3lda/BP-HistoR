#!/bin/bash

# SOurce: https://github.com/JvanKatwijk/dab-scanner

sudo apt-get update -y
sudo apt-get install qt5-qmake build-essential g++ git -y
sudo apt-get install qt5-default libfftw3-dev -y
sudo apt-get install libsndfile1-dev -y
sudo apt-get install zlib1g-dev rtl-sdr libusb-1.0-0-dev mesa-common-dev -y
sudo apt-get install libgl1-mesa-dev libqt5opengl5-dev -y
sudo apt-get install qtbase5-dev libqwt-qt5-dev -y
git clone https://github.com/JvanKatwijk/dab-scanner.git
cd dab-scanner
qmake
make

# terminal-DAB-rtlsdr -C 8A -Q
