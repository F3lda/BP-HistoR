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
echo "rtl_fm -f 105.5e6 -s 200000 -r 48000 | ffmpeg -use_wallclock_as_timestamps 1 -f s16le -ac 1 -ar 48000 -i - -ac 2 -f pulse -device alsa_output.usb-C-Media_Electronics_Inc._USB_Audio_Device-00.analog-stereo 'stream-title'"
echo
echo
echo "ps -e | grep rtl_fm"
echo "kill <id>"
echo
