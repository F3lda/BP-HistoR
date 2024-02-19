#!/bin/bash

# Source: https://unix.stackexchange.com/a/307302

cat << EOF
pactl set-sink-mute 1 toggle  # toggle mute, also you have true/false
pactl set-sink-volume 1 0     # mute (force)
pactl set-sink-volume 1 100%  # max
pactl set-sink-volume 1 +5%   # +5% (up)
pactl set-sink-volume 1 -5%   # -5% (down)

# Get volume # https://unix.stackexchange.com/a/230533
SINK=1
pactl list sinks | grep '^[[:space:]]Volume:' | head -n $(( $SINK + 1 )) | tail -n 1 | sed -e 's,.* \([0-9][0-9]*\)%.*,\1,'


## https://unix.stackexchange.com/questions/457946/pactl-works-in-userspace-not-as-root-on-i3
## user id: id -u
## user id = 1000
#os.system("sudo -u '#1000' XDG_RUNTIME_DIR=/run/user/1000 pactl set-sink-volume 0 +5%")
EOF

