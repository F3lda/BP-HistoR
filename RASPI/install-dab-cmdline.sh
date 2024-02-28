#!/bin/bash

# Source: https://github.com/JvanKatwijk/dab-cmdline/tree/master
: <<'MULTILINE_COMMENT'
sudo apt-get update -y
sudo apt-get install git cmake -y
sudo apt-get install build-essential g++ -y
sudo apt-get install pkg-config -y
sudo apt-get install libsndfile1-dev -y
sudo apt-get install libfftw3-dev portaudio19-dev -y
sudo apt-get install libfaad-dev zlib1g-dev -y
sudo apt-get install libusb-1.0-0-dev mesa-common-dev -y
sudo apt-get install libgl1-mesa-dev -y
sudo apt-get install libsamplerate0-dev -y




git clone https://gitea.osmocom.org/sdr/rtl-sdr.git
cd rtl-sdr/
mkdir build
cd build
cmake ../ -DINSTALL_UDEV_RULES=ON -DDETACH_KERNEL_DRIVER=ON
make
sudo make install
sudo ldconfig
cd ..
rm -rf build
cd ..

MULTILINE_COMMENT

# UPPER cmds skip
# install first: install-dab-terminal.sh
# then:

git clone https://github.com/JvanKatwijk/dab-cmdline

cd dab-cmdline
cd example-2
mkdir build
cd build
cmake .. -DRTLSDR=ON # (replace XXX by the name of the device)
make
sudo make install
cd ../../..




## WORKING!!!
dab-rtlsdr-2 -C 8A -P "DAB" -D 60 -d 60 -G 80





cd dab-cmdline
cd example-4
mkdir build
cd build
cmake .. -DRTLSDR=ON # (replace XXX by the name of the device)
make
sudo make install



### WORKING!!!
dab-rtlsdr-4 -C 8A -P "DAB" -D 60 -d 60 | ffmpeg -loglevel error -i pipe: -c:a pcm_s16le -f s16le pipe: | aplay -r 48000 -f S16_LE -t raw -c 2


#FFmpeg = Faithless and Frenzy Mastodontic Practical Elected Gnome

echo
echo "Run:"
echo "dab-rtlsdr-2 -M 1 -B "BAND III" -C 1A -P "TOP40" -G 80 -A defaul"
echo
echo "If ERRROR:"
echo "https://github.com/potree/PotreeConverter/issues/281#issuecomment-375997005 -> I had to edit the file /etc/locale.gen -> uncomment: en_US.UTF-8 UTF-8 and finally run the command: sudo locale-gen"
echo "or"
echo "https://github.com/google/sanitizers/issues/796#issuecomment-578073928 -> export ASAN_OPTIONS=verify_asan_link_order=0"
echo

