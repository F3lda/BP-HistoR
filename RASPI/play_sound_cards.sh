#!/bin/bash

### https://stackoverflow.com/questions/43806447/how-i-can-play-any-audiofile-to-any-connected-usb-audio-card
#### aplay -l (list devices)
#### mplayer -af channels=2:2:0:0:1:0 -ao alsa:device=hw=0.0 song3.mp3
### https://github.com/raspberrypi/linux/issues/3962
## sudo apt install -y mplayer
## sudo apt install pulseaudio
#### sudo apt-get install pulseaudio-utils


echo ""
echo "Playing MUSIC:"
echo ""



set -x # enable commands output

mplayer -ao alsa:device=hw=1.0 ./MUSIC/Bloodhound\ Gang\ -\ The\ Bad\ Touch\ \(Hugh\ Graham\ Bootleg\)\ \[FREE\ DOWNLOAD\].mp3 </dev/null >/dev/null 2>&1 &
mplayer -ao alsa:device=hw=0.0 ./MUSIC/Creedence\ Clearwater\ Revival\ -\ Fortunate\ Son\ \(Official\ Music\ Video\).mp3 </dev/null >/dev/null 2>&1 &
mplayer -ao alsa:device=hw=2.0 ./MUSIC/Modern\ Talking\ -\ Cheri\ Cheri\ Lady\ \(T-Beat\ Rework\)\ 2k23.mp3 </dev/null >/dev/null 2>&1 &

{ set +x; } &> /dev/null # disable commands output



sleep 1
echo ""
read -n 1 -s -r -p "Press any key to stop..."
echo ""
echo ""
echo ""
echo "Currently playing:"
echo ""



ps aux | grep "mplayer"



killall -9 mplayer



echo ""
echo "All done."
echo ""
sleep 1



set -x # enable commands output


# Play different songs on left and right speaker. Using PulseAudio because Alsa blocks audio output and can play only one song.
# List cards:
# pactl list cards short

# will play sound on left speaker. 2 (the count of output channels) : 2 (the count of moving signal) : 0:0 (left channel route to left channel) : 1:0 (right channel route to left channel)
mplayer -ao pulse::1 -af channels=2:2:0:0:1:0 ./MUSIC/Bloodhound\ Gang\ -\ The\ Bad\ Touch\ \(Hugh\ Graham\ Bootleg\)\ \[FREE\ DOWNLOAD\].mp3 </dev/null >/dev/null 2>&1 &

# will play sound on right speaker.
mplayer -ao pulse::1 -af channels=2:2:0:1:1:1 ./MUSIC/Creedence\ Clearwater\ Revival\ -\ Fortunate\ Son\ \(Official\ Music\ Video\).mp3 </dev/null >/dev/null 2>&1 &


{ set +x; } &> /dev/null # disable commands output



sleep 1
echo ""
read -n 1 -s -r -p "Press any key to stop..."
echo ""
echo ""
echo ""
echo "Currently playing:"
echo ""



ps aux | grep "mplayer"



killall -9 mplayer



echo ""
echo "All done."
echo ""
sleep 1



# Play internet stream
mplayer -ao pulse::1 https://ice5.abradio.cz/hitvysocina128.mp3
