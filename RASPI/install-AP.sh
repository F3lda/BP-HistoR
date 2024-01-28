#disk manager -> backup sdcard image
#https://pimylifeup.com/backup-raspberry-pi/



# instal flask
sudo apt install python3-flask
# run flask
sudo python app.py

##############
from flask import Flask, render_template_string, request

app = Flask(__name__)

@app.route('/')
def index():
    return 'Hello world!'

@app.route('/hello/<name>', strict_slashes=False)
@app.route('/hello/<name>/<page>', strict_slashes=False)
@app.route('/hello/<name>/filter/<filter>', strict_slashes=False)
def hello(name,page="0",filter=""):
    #page = request.args.get('page', default = 1, type = int)
    if filter == "":
        filter = request.args.get('filter', default = '*', type = str)
    return render_template_string('<h1>Hello {{name}}! Page: {{str}}; Filter: {{filter}}</h1>', name=name, str=page, filter=filter)

if __name__ == '__main__':
    app.run(debug=True, host='0.0.0.0', port=80)
###################




# https://pimylifeup.com/raspberry-pi-dns-server/
# https://www.tecmint.com/setup-a-dns-dhcp-server-using-dnsmasq-on-centos-rhel/
# https://pimylifeup.com/raspberry-pi-dns-server/
# https://manpages.ubuntu.com/manpages/noble/en/man8/dnsmasq.8.html
# dnsmasq -> dhcp + dns (redirect to web server) for AP
sudo apt install dnsmasq
sudo nano /etc/dnsmasq.conf

sudo mkdir /var/lib/dnsmasq && sudo touch /var/lib/dnsmasq/dnsmasq.leases

dnsmasq --test
sudo systemctl restart dnsmasq
sudo systemctl status dnsmasq


## If wlan0 connected to internet -> clear dnsmasq.config
sudo mv /etc/dnsmasq.conf /etc/dnsmasq.conf_bk
sudo mv /etc/dnsmasq.conf_bk /etc/dnsmasq.conf



#############################
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

###################################


## android captive portal: https://www.reddit.com/r/paloaltonetworks/comments/191l28l/captive_portal_login_form_not_showing_on_android/
# https://github.com/23ewrdtf/Captive-Portal/blob/master/dnsmasq.conf
# https://www.reddit.com/r/networking/comments/t4webr/push_captive_portal_after_wifi_association/ 
# https://datatracker.ietf.org/doc/html/rfc8910#name-ipv4-dhcp-option
# https://datatracker.ietf.org/doc/html/rfc7710#section-2.1




sudo systemctl restart NetworkManager


sudo nano /etc/NetworkManager/system-connections/Hotspot.nmconnection



#https://www.reddit.com/r/kde/comments/soqzo4/unable_to_connect_to_hotspot_made_with/
#https://wiki.debian.org/NetworkManager/iwd
##Enabling IWD backend for NetworkManager -> AP with password not working with default wpa_supplicant
sudo apt install iwd

sudo nano /etc/NetworkManager/conf.d/iwd.conf

[device]
wifi.backend=iwd


sudo systemctl stop NetworkManager
sudo systemctl disable --now wpa_supplicant
sudo systemctl restart NetworkManager






#https://unix.stackexchange.com/questions/717200/setting-up-a-fixed-ip-wifi-hotspot-with-no-internet-with-dhcp-and-dns-using-dn

### Hotspot connection - set ip address manualy
sudo nmcli con modify Hotspot 802-11-wireless.mode ap ipv4.method manual ipv4.addresses 192.168.11.1/24 ipv4.gateway 192.168.11.1



## TODO create Hotspot connection

sudo nmcli con modify Hotspot 802-11-wireless-security.proto rsn
sudo nmcli con modify Hotspot 802-11-wireless-security.group ccmp
sudo nmcli con modify Hotspot 802-11-wireless-security.pairwise ccmp
sudo nmcli con modify Hotspot 802-11-wireless-security.pmf 1
systemctl restart dnsmasq



sudo nmcli con modify Hotspot 802-11-wireless.band bg
sudo nmcli con modify Hotspot 802-11-wireless.channel 6
sudo nmcli con modify Hotspot 802-11-wireless-security.key-mgmt wpa-psk
sudo nmcli con modify Hotspot ipv4.method shared
sudo nmcli con modify Hotspot 802-11-wireless-security.psk 123456789


sudo nmcli connection add type wifi ifname wlan0 con-name access_point autoconnect yes ssid my_ssid
sudo nmcli connection modify access_point 802-11-wireless.mode ap 802-11-wireless.band bg ipv4.method shared
sudo nmcli connection modify access_point wifi-sec.key-mgmt wpa-psk
sudo nmcli connection modify access_point wifi-sec.psk "my_password"
sudo nmcli connection up access_point

