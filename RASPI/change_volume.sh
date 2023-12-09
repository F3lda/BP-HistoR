#!/bin/bash

# Source: https://unix.stackexchange.com/a/307302

cat << EOF
pactl set-sink-mute 1 toggle  # toggle mute, also you have true/false
pactl set-sink-volume 1 0     # mute (force)
pactl set-sink-volume 1 100%  # max
pactl set-sink-volume 1 +5%   # +5% (up)
pactl set-sink-volume 1 -5%   # -5% (down)
EOF
