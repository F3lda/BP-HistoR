#!/bin/bash

#https://gist.githubusercontent.com/fagnercarvalho/2755eaa492a8aa27081e0e0fe7780d14/raw/d1348b8c4fcf96619e10e48de9eda5ff719ab8fe/commands.sh

# Install bluealsa to create interface to Bluetooth device
: <<'MULTILINE_COMMENT'
# NOT WORKING!!!
git clone https://github.com/Arkq/bluez-alsa.git
cd bluez-alsa
su
apt-get install libglib2.0-dev -y
apt-get install -y libasound2-dev
apt install -y build-essential autoconf
apt-get install -y libbluetooth-dev
apt-get install libtool -y
apt install libsbc-dev -y
apt-get install build-essential libdbus-glib-1-dev libgirepository1.0-dev
autoreconf --install
mkdir build && cd build
../configure --enable-ofono --enable-debug
make && make install
MULTILINE_COMMENT