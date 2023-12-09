#!/bin/bash

# Source: https://www.linux-magazine.com/Issues/2018/206/Pi-FM-Radio

echo
echo "Install:"
echo "sudo apt-get install rtl-sdr"
echo
echo "rtl_test"
echo
echo "rtl_fm -f 93.4e6 -s 200000 -r 48000 | aplay -r 48000 -f S16_LE"
rtl_fm -f 93.4e6 -s 200000 -r 48000 | aplay -r 48000 -f S16_LE
echo
echo "ps -e | grep rtl_fm"
echo "kill <id>"
echo
