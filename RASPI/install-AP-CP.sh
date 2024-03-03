#!/bin/bash

#   !!! EDIT THESE LINES !!!
###############################
WIFI_SSID="ACMOTO"
WIFI_PASSWORD="vnuci321"

AP_SSID="HistoRAP"
AP_PASSWORD="12345678"

DEVICE_NAME="HistoRPi"
###############################





echo "|----------|";
echo "| HistoRPi |";
echo "|----------|";
echo "Installing...";

if [ "$WIFI_SSID" = "" ] || [ "$WIFI_PASSWORD" = "" ] || [ "$AP_SSID" = "" ] || [ "$AP_PASSWORD" = "" ] || [ "$DEVICE_NAME" = "" ]; then
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

# install flask
sudo apt install python3-flask -y
# install dnsmasq
sudo apt install dnsmasq -y
# install iwd
sudo apt install iwd -y
# install festival for IPtoSpeech
sudo apt-get install festival -y


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
interface=wlan0

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
ExecStart=python /home/histor/web-server/app.py
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
echo "Editting file /home/histor/web-server/app.py:";
sudo mkdir -p /home/histor/web-server # The parameter mode specifies the permissions to use.
sudo tee /home/histor/web-server/app.py <<EOF
from flask import Flask, render_template, render_template_string, request, redirect

app = Flask(__name__)

@app.route('/')
def index():
    #return render_template('index.html')
    return 'Hello world! - ${AP_SSID} - ${AP_PASSWORD}'

@app.route('/hello/<name>', strict_slashes=False)
@app.route('/hello/<name>/<page>', strict_slashes=False)
@app.route('/hello/<name>/filter/<filter>', strict_slashes=False)
def hello(name,page="0",filter=""):
    #page = request.args.get('page', default = 1, type = int)
    if filter == "":
        filter = request.args.get('filter', default = '*', type = str)
    return render_template_string('<h1>Hello {{name}}! Page: {{str}}; Filter: {{filter}}</h1>', name=name, str=page, filter=filter)


#https://stackoverflow.com/questions/14023864/flask-url-route-route-all-other-urls-to-some-function
@app.errorhandler(404)
def page_not_found(e):
    # your processing here
    #return 'Hello world! 404', 302
    return redirect("/",code=302)


if __name__ == '__main__':
    app.run(debug=True, host='0.0.0.0', port=80)
EOF
sudo chmod 777 /home/histor/web-server/app.py



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
            sudo nmcli connection down Hotspot
            #set up dnsmasq
            sudo \cp -f /etc/dnsmasq.conf_ap /etc/dnsmasq.conf # AP config
            #restart dnsmasq
            sudo systemctl restart dnsmasq
            # set up AP
            sudo nmcli connection up Hotspot
            
            attempts=8
        fi
    else
        echo "I have network: $(date)"
        
        # IP to Speech
        . $FILE || true
        if [ "$IPtoSPEECH" = true ] ; then
            echo  "My IP address is $(hostname -I)" | festival --tts
        fi
        
        if [ $attempts -ne 0 ]; then
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
