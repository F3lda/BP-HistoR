#!/bin/bash
# NOT WORKING!!!
# install first: install-dab-terminal.sh
# then:
git clone https://github.com/JvanKatwijk/dab-cmdline

cd dab-cmdline
cd example-3
mkdir build
cd build
cmake .. -DRTLSDR=ON # (replace XXX by the name of the device)
make
sudo make install
#cd ../../..



dab-rtlsdr-3 -C 8A -P "DAB" -D 60 -d 60 | aplay -r 48000 -f S16_LE -t raw -c 2


echo
echo "Run:"
echo "dab-rtlsdr-3 -M 1 -B "BAND III" -C 8A -P "DAB PLUS TOP 40" -G 80 -A default | aplay -r 48000 -f S16_LE -t raw -c 2"
echo "dab-rtlsdr-3 -M 1 -B BAND_III -C 1A -P "TOP40" -G 80 | aplay -r 48000 -f S16_LE -t raw -c 2"
echo
echo "If ERRROR:"
echo "https://github.com/potree/PotreeConverter/issues/281#issuecomment-375997005 -> I had to edit the file /etc/locale.gen -> uncomment: en_US.UTF-8 UTF-8 and finally run the command: sudo locale-gen"
echo "or"
echo "https://github.com/google/sanitizers/issues/796#issuecomment-578073928 -> export ASAN_OPTIONS=verify_asan_link_order=0"
echo
