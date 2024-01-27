##man nmcli-examples

##https://www.raspberrypi.com/tutorials/host-a-hotel-wifi-hotspot/

nmcli general
nmcli device

nmcli connection

nmcli device wifi list ifname wlan0

nmcli device wifi connect "ACMOTO" password "test" ifname wlan0
nmcli --ask device wifi connect "ACMOTO" password "test" ifname wlan0

sudo nmcli device wifi hotspot ssid "RaspiNet" password "12345678" ifname wlan0

nmcli dev wifi show-password

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
