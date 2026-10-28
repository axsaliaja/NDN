#!/bin/bash
# Demo di kelas (WSL Ubuntu-20.04): 3 konfigurasi, 5 node, 20 detik simulasi, seed 1.
#   bash /mnt/c/Users/axsal/Downloads/NDN/eksperimen-validasi/demo.sh
cd ~/ndnSIM/ns-3 || exit 1
show() {
  echo -e "\n\e[1;36m\$ ./waf --run \"wifi-ndn-exp $1\"\e[0m   \e[2m# $2\e[0m"
  ./waf --run "wifi-ndn-exp $1" 2>&1 | grep -E 'sat=|SUMMARY' | sed 's/^  /   /'
}
echo -e "\e[1mNDN WiFi ad hoc - Posko ~ Relawan1 ~ Relawan2 ~ PengungsiA ~ PengungsiB (80 m, jangkauan 100 m)\e[0m"
show "--fix=0" "face WiFi default ndnSIM (point-to-point)"
show "--fix=1 --strategy=multicast" "face WiFi AD_HOC"
show "--fix=1 --strategy=jitter-broadcast --jitter=10" "AD_HOC + jeda rebroadcast acak maks 10 ms"
