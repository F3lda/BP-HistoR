#!/bin/bash

# Source: https://www.root.cz/clanky/chytre-radio-postavene-z-raspberry-pi-probudte-stary-kazetak-k-zivotu/
# https://forums.raspberrypi.com/viewtopic.php?t=179909
# sudo apt-get install festival

# Print the IP address
_IP=$(hostname -I) || true
if [ "$_IP" ]; then
  echo  "My IP address is $_IP" | festival --tts
fi

exit 0

# Add this script to file /etc/rc.local to run it on startup and make file executable:
# sudo chmod +x /etc/rc.local
# Source: https://askubuntu.com/questions/299792/why-is-the-command-in-etc-rc-local-not-executed-during-startup
