#!/bin/bash
# Demo di kelas (±1 menit, satu seed): jalankan jalankan.sh sekali dulu agar program sudah di-build.
#   bash demo.sh
NS3=${NS3:-~/ndnSIM/ns-3}
export LD_LIBRARY_PATH=$NS3/build/lib
BIN=$NS3/build/scratch/ndn-wifi-darurat/ndn-wifi-darurat
show() {
  echo -e "\n\e[1;36m\$ ndn-wifi-darurat $1\e[0m   \e[2m# $2\e[0m"
  "$BIN" $1 | grep -E '^(CONS|SUM)' | awk '{printf "   "; for (i=1;i<=NF;i++) if ($i ~ /^(CONS|SUM|node|hop|sent|ok|sat|dmean|dmed|phyTx|dropN1|rxOkN1)=?/) printf "%s ", $i; print ""}'
}
echo -e "\e[1mRantai 5 node: Posko ~ Relawan1 ~ Relawan2 ~ PengungsiA ~ PengungsiB (80 m, jangkauan 100 m)\e[0m"
show "--fix=0" "face WiFi bawaan ndnSIM (point-to-point)"
show "--fix=1 --strategy=multicast" "face AD_HOC + multicast"
show "--fix=1 --strategy=jitter-broadcast --jitter=10" "AD_HOC + jeda acak maks 10 ms (minggu 1)"
show "--fix=1 --strategy=jitter-broadcast --jitter=10 --retxSup=50" "+ Interest ulang boleh diteruskan lagi"
echo -e "\n\e[1mE2: kenapa Relawan 1 gagal? (3 node, hanya Relawan 1 yang meminta)\e[0m"
show "--n=3 --consumers=1 --fix=1 --strategy=multicast" "Posko & Relawan 2 memancar serempak"
show "--n=3 --consumers=1 --fix=1 --strategy=multicast --range=170" "saling dengar pun tetap gagal"
show "--n=3 --consumers=1 --fix=1 --strategy=jitter-broadcast --jitter=10" "jeda acak memecah keserempakan"
