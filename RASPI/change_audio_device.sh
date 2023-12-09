#!/bin/bash

# https://askubuntu.com/a/435197
# https://askubuntu.com/questions/71863/how-to-change-pulseaudio-sink-with-pacmd-set-default-sink-during-playback

echo "Input devices:"
pactl list short sources
echo
echo "Set input device: pactl set-default-source <Your_Device_Number_Or_Name>"

echo
echo
echo "Output devices:"
pactl list short sinks
echo
echo "Set output device: pactl set-default-sink <Your_Device_Number_Or_Name>"
echo
echo
echo "play song: mplayer './MUSIC/Modern Talking - Cheri Cheri Lady (T-Beat Rework) 2k23.mp3'"
echo
