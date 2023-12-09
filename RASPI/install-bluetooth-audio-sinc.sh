#!/bin/bash -e

if [[ $(id -u) -ne 0 ]] ; then echo "Please run as root" ; exit 1 ; fi

read -p "Hostname [$(hostname)]: " HOSTNAME
raspi-config nonint do_hostname ${HOSTNAME:-$(hostname)}

CURRENT_PRETTY_HOSTNAME=$(hostnamectl status --pretty)
read -p "Pretty hostname [${CURRENT_PRETTY_HOSTNAME:-Raspberry Pi}]: " PRETTY_HOSTNAME
hostnamectl set-hostname --pretty "${PRETTY_HOSTNAME:-${CURRENT_PRETTY_HOSTNAME:-Raspberry Pi}}"



echo "Installing PulseAudio"
sudo apt install -y --no-install-recommends pulseaudio



echo
echo -n "Do you want to install Bluetooth Audio (PulseAudio)? [y/N] "
read REPLY
if [[ ! "$REPLY" =~ ^(yes|y|Y)$ ]]; then exit 0; fi

sudo apt install -y --no-install-recommends bluez-tools pulseaudio-module-bluetooth
sudo apt install -y pulseaudio-module-bluetooth bluez python3-dbus

sudo apt-get install -y git 

git clone https://github.com/spmp/promiscuous-bluetooth-audio-sinc
cd promiscuous-bluetooth-audio-sinc

sudo cp -a a2dp-agent /usr/local/bin/
sudo chmod +x /usr/local/bin/a2dp-agent


echo
echo "reboot now"
echo
echo "then RUN"
echo
echo "sudo /usr/local/bin/a2dp-agent"
echo
# https://gist.github.com/mill1000/74c7473ee3b4a5b13f6325e9994ff84c
