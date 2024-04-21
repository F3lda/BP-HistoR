#!/bin/bash
# * @file install-HistoRPi.sh
# *
# * @brief HistoRPi - Audio streaming device for historic radio receivers - installation file
# * @date 2024-01-23
# * @author F3lda (Karel Jirgl)
# * @update 2024-04-21 (v1.2)

# !!! if your RaspberryPi's username is not 'histor' -> replace all 'histor' in this file for your username

#   !!! EDIT THESE LINES !!!
###############################
WIFI_SSID=""
WIFI_PASSWORD=""

AP_SSID="HistoRPi"
AP_PASSWORD="12345678"

DEVICE_NAME="HistoRPi"
###############################





echo "|----------|";
echo "| HistoRPi |";
echo "|----------|";
echo "Installing...";

if [ "$AP_SSID" = "" ] || [ "$AP_PASSWORD" = "" ] || [ "$DEVICE_NAME" = "" ]; then
    echo "Failed!";
    echo "Installation file is not configured!";
    echo "Please, edit this file: $(pwd)/$(basename "$0")";
    read -p "Press ENTER to continue..." CONT
    exit -1;
fi

### SET DEVICE NAME
echo "Setting Device name...";
echo "Editting file /etc/machine-info:";
sudo tee /etc/machine-info <<EOF
PRETTY_HOSTNAME=${DEVICE_NAME}
EOF

echo "Checking internet connection...";
res=$(iwconfig 2>&1 | grep ESSID);
if [[ $res =~ ':"' ]];
then
    echo "Connected: True";
    echo "Connected to: ${res##*:}";
else
    echo "ERROR!"
    echo "Connected: False";
    echo "If it's a first installation on this device, please connect to the internet!"
    read -p "Continue? [y/n]: " CONT
    if [ $CONT = "n" ] || [ $CONT = "N" ]; then
        exit -2;
    fi
fi



echo ""
echo "Raspberry Pi Wifi info"
echo "----------------------"
iw list | grep "Supported interface modes" -A 8
echo ""
iw list | grep "valid interface combinations" -A 8
echo ""
echo ""
echo ""





# enable auto-login for user context
#https://raspberrypi.stackexchange.com/questions/135455/enable-console-auto-login-via-commandline-ansible-script
sudo raspi-config nonint do_boot_behaviour B2



### INSTALL ALL FIRST
#sudo bash -c 'apt update -y && apt full-upgrade -y && reboot'
sudo apt-get update -y
sudo apt-get upgrade -y

# install git
sudo apt-get install git -y
# install flask
sudo apt install python3-flask -y
# install dnsmasq
sudo apt install dnsmasq -y
# install iwd for network-manager
sudo apt install iwd -y
# install festival for IPtoSpeech
sudo apt-get install festival -y


## install PulseAudio
sudo apt install pulseaudio -y
sudo apt-get install pulseaudio-utils -y
sudo apt install mplayer -y

## install rtl_fm
sudo apt-get install rtl-sdr -y

## install bluetooth a2dp
sudo apt install -y --no-install-recommends bluez-tools pulseaudio-module-bluetooth
sudo apt install -y pulseaudio-module-bluetooth bluez python3-dbus
# SETUP bluetooth a2dp SCRIPT
echo "Editting file /home/histor/web-server/device.conf:";
sudo mkdir -p /home/histor/web-server/LIBS/promiscuous-bluetooth-audio-sinc # The parameter mode specifies the permissions to use.
sudo tee /home/histor/web-server/LIBS/promiscuous-bluetooth-audio-sinc/a2dp-agent <<EOF
#!/usr/bin/python3
# SPDX-License-Identifier: LGPL-2.1-or-later
# Source: https://github.com/spmp/promiscuous-bluetooth-audio-sinc (file changed)

from __future__ import absolute_import, print_function, unicode_literals

import os
import signal
import dbus
import dbus.service
import dbus.mainloop.glib
try:
  from gi.repository import GLib
except ImportError:
  import gobject as GLib
from functools import partial

# The ENV VAR if set should be of the form 'hci0', 'hci1' etc.
DEVICE_ENV_VAR = 'BLUETOOTH_ADAPTER'
DEVICE_PATH_BASE = '/org/bluez/'
DEVICE_DEFAULT = 'hci0'
AGENT_INTERFACE = 'org.bluez.Agent1'
AGENT_PATH = "/org/bluez/promiscuousAgent"

def set_trusted(path):
    props = dbus.Interface(bus.get_object("org.bluez", path),
                    "org.freedesktop.DBus.Properties")
    props.Set("org.bluez.Device1", "Trusted", True)

class Rejected(dbus.DBusException):
    _dbus_error_name = "org.bluez.Error.Rejected"

class Agent(dbus.service.Object):

    exit_on_release = True

    def set_exit_on_release(self, exit_on_release):
        self.exit_on_release = exit_on_release

    @dbus.service.method(AGENT_INTERFACE,
                    in_signature="", out_signature="")
    def Release(self):
        print("Release")
        if self.exit_on_release:
            mainloop.quit()

    @dbus.service.method(AGENT_INTERFACE,
                    in_signature="os", out_signature="")
    def AuthorizeService(self, device, uuid):
        print(f"AuthorizeService ({device}, {uuid})")
        if uuid.upper() == "0000110B-0000-1000-8000-00805F9B34FB" or uuid.upper() == "0000110A-0000-1000-8000-00805F9B34FB" or uuid.upper() == "0000110D-0000-1000-8000-00805F9B34FB": # AudioSink, AudioSource , A2DP (Advanced Audio Distribution Profile)
            print("Authorized A2DP Service")
            return
        print("Rejecting non-A2DP Service")
        raise Rejected("Connection rejected")

    @dbus.service.method(AGENT_INTERFACE,
                    in_signature="ou", out_signature="")
    def RequestConfirmation(self, device, passkey):
        print(f"RequestConfirmation ({device}, {passkey})")
        print("Auto confirming...")

    @dbus.service.method(AGENT_INTERFACE,
                    in_signature="o", out_signature="s")
    def RequestPinCode(self, device):
        print(f"RequestPinCode '0000' from {device}")
        set_trusted(device)
        return "0000"

    @dbus.service.method(AGENT_INTERFACE,
                    in_signature="", out_signature="")
    def Cancel(self):
        print("Cancel")

def quit(manager, mloop):
    manager.UnregisterAgent(AGENT_PATH)
    print("\nAgent unregistered")

    mloop.quit()


    os.system("sudo rfkill block bluetooth")


if __name__ == '__main__':
    os.system("sudo rfkill unblock bluetooth")


    dbus.mainloop.glib.DBusGMainLoop(set_as_default=True)

    bus = dbus.SystemBus()

    capability = "DisplayYesNo"

    agent = Agent(bus, AGENT_PATH)

    mainloop = GLib.MainLoop()

    obj = bus.get_object("org.bluez", "/org/bluez")

    # Get the device from ENV_VAR if set
    adapters=os.getenv(DEVICE_ENV_VAR, DEVICE_DEFAULT).split(",")

    for adapter in adapters:

      adapterPath = DEVICE_PATH_BASE + adapter

      # Set Discoverable and Pairable to always on
      print(f"Setting {adapterPath} to 'discoverable' and 'pairable'...")
      prop = dbus.Interface(bus.get_object("org.bluez", adapterPath), "org.freedesktop.DBus.Properties")
      prop.Set("org.bluez.Adapter1", "DiscoverableTimeout", dbus.UInt32(0))
      prop.Set("org.bluez.Adapter1", "PairableTimeout", dbus.UInt32(0))
      prop.Set("org.bluez.Adapter1", "Discoverable", dbus.Boolean(True))
      prop.Set("org.bluez.Adapter1", "Pairable", dbus.Boolean(True))

    # Create the agent manager
    manager = dbus.Interface(obj, "org.bluez.AgentManager1")
    manager.RegisterAgent(AGENT_PATH, capability)
    manager.RequestDefaultAgent(AGENT_PATH)

    print("Agent registered")

    # Ensure that ctrl+c is caught properly
    ## Assign the 'quit' function to a variable
    mquit = partial(quit, manager=manager, mloop=mainloop)
    GLib.unix_signal_add(GLib.PRIORITY_DEFAULT, signal.SIGINT, mquit)

    mainloop.run()
EOF
sudo chmod 777 /home/histor/web-server/LIBS/promiscuous-bluetooth-audio-sinc/a2dp-agent

## install DAB terminal
sudo apt-get install git cmake -y
sudo apt-get install build-essential g++ -y
sudo apt-get install pkg-config -y
sudo apt-get install libsndfile1-dev -y
sudo apt-get install libfftw3-dev -y
sudo apt-get install portaudio19-dev -y
sudo apt-get install zlib1g-dev -y
sudo apt-get install libusb-1.0-0-dev -y
sudo apt-get install libsamplerate0-dev -y
sudo apt-get install curses -y
sudo apt-get install libncurses5-dev -y

#sudo apt-get install opencv-dev -y

sudo apt-get install libfaad-dev -y
#sudo apt-get install libfdk-aac-dev -y

sudo apt install librtlsdr-dev -y

cd /home/histor/web-server/LIBS
sudo git clone https://github.com/JvanKatwijk/terminal-DAB-xxx
cd terminal-DAB-xxx
sudo mkdir build
cd build
sudo cmake .. -DRTLSDR=ON -DPICTURES=OFF #-DFAAD=OFF
sudo make
sudo make install

## install SDR
sudo apt install ffmpeg -y
# install rpitx
cd /home/histor/web-server/LIBS
sudo git clone https://github.com/F5OEO/rpitx
cd rpitx
yes | sudo ./install.sh

# install command for "list open files"
sudo apt-get install lsof -y



### SETUP DNSMASQ
# https://pimylifeup.com/raspberry-pi-dns-server/
# https://www.tecmint.com/setup-a-dns-dhcp-server-using-dnsmasq-on-centos-rhel/
# https://pimylifeup.com/raspberry-pi-dns-server/
# https://manpages.ubuntu.com/manpages/noble/en/man8/dnsmasq.8.html
# dnsmasq -> dhcp + dns (redirect to web server) for AP

## If wlan0 connected to internet -> clear dnsmasq.config
#sudo mv /etc/dnsmasq.conf /etc/dnsmasq.conf_ap
#sudo mv /etc/dnsmasq.conf_ap /etc/dnsmasq.conf


# create file for DHCP leased ip addresses
sudo mkdir /var/lib/dnsmasq && sudo touch /var/lib/dnsmasq/dnsmasq.leases

# change DNSMASQ config
echo "Editting file /etc/dnsmasq.conf_ap:";
sudo tee /etc/dnsmasq.conf_ap <<EOF
interface=wlan0,eth0

# set the IP address, where dnsmasq will listen on.
listen-address=127.0.0.1,192.168.11.1

# dnsmasq server domain
domain=raspi.local

# This option changes the DNS server so that it does not forward names that do not contain a dot (.) or a domain name (.com) to upstream nameservers.
# Doing this will keep any plain names such as “localhost” or “dlinkrouter” to the local network.
domain-needed

# This option stops the DNS server from forwarding reverse-lookup queries that have a local IP range to the upstream DNS servers.
# Doing this helps prevent leaking the setup of a local network as the IP addresses will never be sent to the upstream servers.
bogus-priv



no-resolv
no-poll
no-hosts


cache-size=1000
no-negcache
local-ttl=30
address=/#/192.168.11.1



dhcp-range=192.168.11.10,192.168.11.100,12h
dhcp-leasefile=/var/lib/dnsmasq/dnsmasq.leases
dhcp-authoritative
dhcp-option=option:router,192.168.11.1
dhcp-option=114,http://192.168.11.1/
dhcp-option=160,http://192.168.11.1/
EOF
sudo chmod 600 /etc/dnsmasq.conf_ap
dnsmasq --test

# restart DNSMASQ
sudo systemctl restart dnsmasq
#sudo systemctl status dnsmasq


## CAPTIVE PORTAL INFO
# android captive portal: https://www.reddit.com/r/paloaltonetworks/comments/191l28l/captive_portal_login_form_not_showing_on_android/
# https://github.com/23ewrdtf/Captive-Portal/blob/master/dnsmasq.conf
# https://www.reddit.com/r/networking/comments/t4webr/push_captive_portal_after_wifi_association/
# https://datatracker.ietf.org/doc/html/rfc8910#name-ipv4-dhcp-option
# https://datatracker.ietf.org/doc/html/rfc7710#section-2.1





### SETUP IWD
##Enabling IWD backend for NetworkManager -> AP with password not working with default wpa_supplicant
#https://forums.raspberrypi.com/viewtopic.php?t=341580
#https://www.reddit.com/r/kde/comments/soqzo4/unable_to_connect_to_hotspot_made_with/
#https://wiki.debian.org/NetworkManager/iwd


echo "Editting file /etc/NetworkManager/conf.d/iwd.conf:";
sudo tee /etc/NetworkManager/conf.d/iwd.conf <<EOF
[device]
wifi.backend=iwd
EOF

#sudo systemctl stop NetworkManager
sudo systemctl disable --now wpa_supplicant
#sudo systemctl restart NetworkManager




### SETUP NETWORK MANAGER
# https://raspberrypi.stackexchange.com/a/142588
# This command will enable NetworkManager, without needing to jump through GUI:
sudo raspi-config nonint do_netconf 2
#sudo reboot
# sudo service network-manager restart





### SETUP ON STARTUP
## using A Systemd Service
## https://github.com/thagrol/Guides/blob/main/boot.pdf
echo "Editting file /etc/NetworkManager/conf.d/iwd.conf:";
sudo tee /etc/systemd/system/APwebserver.service <<EOF
[Unit]
Description=CaptivePortalWebService
[Service]
ExecStart=python /home/histor/web-server/HistoRPi.py
Restart=always
[Install]
WantedBy=multi-user.target
EOF
# the leading "-" is used to note that failure is tolerated for these commands
sudo systemctl daemon-reload
sudo systemctl enable APwebserver
sudo systemctl start APwebserver


echo "Editting file /etc/NetworkManager/conf.d/iwd.conf:";
sudo tee /etc/systemd/system/APconnection.service <<EOF
[Unit]
Description=CaptivePortalConnectionService
[Service]
ExecStart=/home/histor/web-server/connection.sh
Restart=always
[Install]
WantedBy=multi-user.target
EOF
# the leading "-" is used to note that failure is tolerated for these commands
sudo systemctl daemon-reload
sudo systemctl enable APconnection
sudo systemctl start APconnection



### SETUP FLASK WEB SERVER
echo "Editting file /home/histor/web-server/HistoRPi.py:";
sudo mkdir -p /home/histor/web-server # The parameter mode specifies the permissions to use.
sudo chmod 777 /home/histor/web-server
sudo mv /home/histor/HistoRPi.py /home/histor/web-server/HistoRPi.py
sudo chmod 777 /home/histor/web-server/HistoRPi.py



### SETUP CONNECTION CHECK SCRIPT
echo "Editting file /home/histor/web-server/connection.sh:";
sudo mkdir -p /home/histor/web-server # The parameter mode specifies the permissions to use.
sudo tee /home/histor/web-server/connection.sh <<EOF
#!/bin/bash

DIR=\$(dirname "\$0")
FILE="\$DIR/device.conf"
. \$FILE || true



# wait for NetworkManager to startup
while true; do
    # https://www.golinuxcloud.com/nmcli-command-examples-cheatsheet-centos-rhel/
    if [ "\$(nmcli -t -f RUNNING general)" = "running" ]
    then
        sleep 10 # wait for wifi to startup
        sudo nmcli device wifi list # wait for wifi to startup
        break
    fi
    sleep 1
done



# set up hotspot
#https://www.raspberrypi.com/tutorials/host-a-hotel-wifi-hotspot/
sudo nmcli device wifi hotspot ssid "\${AP_SSID}" password "\${AP_PASSWORD}" ifname wlan0
#https://unix.stackexchange.com/questions/717200/setting-up-a-fixed-ip-wifi-hotspot-with-no-internet-with-dhcp-and-dns-using-dn
sudo nmcli connection modify Hotspot 802-11-wireless.mode ap ipv4.method manual ipv4.addresses 192.168.11.1/24 ipv4.gateway 192.168.11.1
sudo nmcli connection down Hotspot



# connect to wifi
sudo rm -f /etc/dnsmasq.conf # empty dns config
sudo systemctl restart dnsmasq
sleep 10 # wait for wifi to startup
sudo nmcli device wifi list # wait for wifi to startup


#https://unix.stackexchange.com/questions/420640/unable-to-connect-to-any-wifi-with-networkmanager-due-to-error-secrets-were-req
sudo nmcli connection delete "\${WIFI_SSID}"
sudo nmcli device wifi connect "\${WIFI_SSID}" password "\${WIFI_PASSWORD}" ifname wlan0
#https://askubuntu.com/questions/947965/how-to-trigger-network-manager-autoconnect
sudo nmcli device set wlan0 autoconnect yes
sudo nmcli connection modify "\${WIFI_SSID}" connection.autoconnect yes # nmcli -f name,autoconnect con
# cat't just add connection: https://askubuntu.com/questions/1165133/networkmanager-will-not-autoconnect-to-wireless-if-it-is-unavailable-at-creation
# sudo nmcli connection add type wifi con-name "\$WIFI_SSID" autoconnect yes ssid "\$WIFI_SSID" 802-11-wireless-security.key-mgmt WPA-PSK 802-11-wireless-security.psk "\$WIFI_PASSWORD"


#nmcli --ask device wifi connect "\${WIFI_SSID}" password "\${WIFI_PASSWORD}" ifname wlan0
#nmcli connection show "\${WIFI_SSID}"
#nmcli dev wifi show-password

#nmcli general
#nmcli device
#nmcli connection
#nmcli device wifi list ifname wlan0
#nmcli device wifi show-password | grep "SSID:" | cut -d ':' -f 2
#sudo nmcli device wifi show-password | grep "Password:" | cut -d ':' -f 2
#https://unix.stackexchange.com/questions/717200/setting-up-a-fixed-ip-wifi-hotspot-with-no-internet-with-dhcp-and-dns-using-dn
#https://askubuntu.com/questions/1460268/how-do-i-setup-an-access-point-that-starts-on-every-boot



### Remove WIFI connection data to not delete wifi connection at startup: (up) sudo nmcli connection delete "\${WIFI_SSID}"
sudo tee \$FILE <<ENDOFFILE
WIFI_SSID=""
WIFI_PASSWORD=""

AP_SSID="\${AP_SSID}"
AP_PASSWORD="\${AP_PASSWORD}"

DEVICE_NAME="\${DEVICE_NAME}"
ENDOFFILE



# set-up ethernet connection
sudo nmcli connection delete Wired\ connection\ 1
sudo nmcli connection add type ethernet ifname eth0 con-name Wired\ connection\ 1



# Turn off WiFi power saving mode
#sudo iw wlan0 set power_save off
# iw wlan0 get power_save
#https://raspberrypi.stackexchange.com/questions/96606/make-iw-wlan0-set-power-save-off-permanent



attempts=0

while true; do
    if [ "\$(hostname -I)" = "" ]
    then
        echo "No network: \$(date)"

        if [ \$attempts -lt 7 ]; then # 60 seconds
            attempts=\$((attempts+1))
        elif [ \$attempts -eq 7 ]; then
			## Set-up Hotspot
            sudo nmcli connection down Hotspot
            #set up dnsmasq
            sudo \cp -f /etc/dnsmasq.conf_ap /etc/dnsmasq.conf # AP config
            #restart dnsmasq
            sudo systemctl restart dnsmasq
            # set up AP
            sudo nmcli connection up Hotspot

            ## Set-up ethernet connection
            sudo nmcli connection modify Wired\ connection\ 1 ipv4.method manual ipv4.addresses 192.168.11.1/24 ipv4.gateway 192.168.11.1

			attempts=8
        fi
    else
        echo "I have network: \$(date)"

        # IP to Speech
        . \$FILE || true
        if [ "\$IPtoSPEECH" = true ] ; then
            echo  "My IP address is \$(hostname -I)" | festival --tts
        fi

        if [ \$attempts -ne 0 ]; then
            attempts=0
        fi
    fi
    sleep 10
done
EOF
sudo chmod 777 /home/histor/web-server/connection.sh



### SETUP CONNECTION CHECK SCRIPT
echo "Editting file /home/histor/web-server/device.conf:";
sudo mkdir -p /home/histor/web-server # The parameter mode specifies the permissions to use.
sudo tee /home/histor/web-server/device.conf <<EOF
WIFI_SSID="${WIFI_SSID}"
WIFI_PASSWORD="${WIFI_PASSWORD}"

AP_SSID="${AP_SSID}"
AP_PASSWORD="${AP_PASSWORD}"

DEVICE_NAME="${DEVICE_NAME}"

IPtoSPEECH=true
EOF
sudo chmod 777 /home/histor/web-server/device.conf



### REBOOT
echo "Done!";
sudo reboot


#disk manager -> backup sdcard image
#https://pimylifeup.com/backup-raspberry-pi/
