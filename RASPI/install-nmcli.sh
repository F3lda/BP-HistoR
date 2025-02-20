##man nmcli-examples

##https://www.raspberrypi.com/tutorials/host-a-hotel-wifi-hotspot/

nmcli general
nmcli device

nmcli connection

nmcli device wifi list ifname wlan0

# https://unix.stackexchange.com/questions/420640/unable-to-connect-to-any-wifi-with-networkmanager-due-to-error-secrets-were-req
nmcli device wifi connect "ACMOTO" password "test" ifname wlan0
nmcli --ask device wifi connect "ACMOTO" password "test" ifname wlan0

sudo nmcli device wifi hotspot ssid "RaspiNet" password "12345678" ifname wlan0
#https://unix.stackexchange.com/questions/717200/setting-up-a-fixed-ip-wifi-hotspot-with-no-internet-with-dhcp-and-dns-using-dn
sudo nmcli connection modify Hotspot 802-11-wireless.mode ap ipv4.method manual ipv4.addresses 192.168.11.1/24 ipv4.gateway 192.168.11.1


# show passwords
sudo nmcli dev wifi show-password
sudo grep --with-filename psk= /etc/NetworkManager/system-connections/*
#https://askubuntu.com/questions/1090065/how-to-find-the-saved-wifi-password-in-ubuntu


nmcli connection show "ACMOTO"

nmcli connection down Hotspot
nmcli connection delete Hotspot
nmcli connection up Hotspot
nmcli connection up "ACMOTO"


# https://raspberrypi.stackexchange.com/a/142588
# This command will enable NetworkManager, without needing to jump through GUI:
sudo raspi-config nonint do_netconf 2
sudo reboot
# sudo service network-manager restart


# to get system name
# hostnamectl

## result = subprocess.check_output(["nmcli", "--colors", "no", "-m", "multiline", "--get-value", "SSID", "dev", "wifi", "list", "ifname", wifi_device])
nmcli --colors no -m multiline --get-value SSID dev wifi list ifname wlan0


## connection_command = ["nmcli", "--colors", "no", "device", "wifi", "connect", ssid, "ifname", wifi_device]
nmcli --colors no device wifi connect "ACMOTO" ifname wlan0
