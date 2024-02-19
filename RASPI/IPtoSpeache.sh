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


#https://forums.raspberrypi.com/viewtopic.php?t=111883&sid=2844ae7fc1ea3a6436e7557d37630887&start=25


##!/bin/bash
#pin=7
#while true; do
#        button=$( echo $( raspi-gpio get ${pin} ) | awk -F 'level=| fsel=' '{print $2}' )
#        if [ "$button" -eq "0" ]
#        then
#           echo "Button is pressed"
#			# Print the IP address
#			_IP=$(hostname -I) || true
#			if [ "$_IP" ]; then
#			  echo  "My IP address is $_IP" | festival --tts
#			fi
#           sleep 2
#        else
#           sleep 0.1
#        fi
#done
