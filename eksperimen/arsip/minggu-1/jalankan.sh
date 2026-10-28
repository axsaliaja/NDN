#!/bin/bash
# Build skenario lalu jalankan semua konfigurasi (3 seed) secara paralel -> raw/*.txt
# Pakai dari WSL Ubuntu-20.04:  bash /mnt/c/Users/axsal/Downloads/NDN/eksperimen-validasi/jalankan.sh
set -e
HERE=/mnt/c/Users/axsal/Downloads/NDN/eksperimen-validasi
NS3=~/ndnSIM/ns-3
mkdir -p $NS3/scratch/wifi-ndn-exp
tr -d '\r' < $HERE/wifi-ndn-exp.cc > $NS3/scratch/wifi-ndn-exp/wifi-ndn-exp.cc
cd $NS3 && ./waf build -j8 | tail -1

export LD_LIBRARY_PATH=$NS3/build/lib BIN=$NS3/build/scratch/wifi-ndn-exp/wifi-ndn-exp OUT=$HERE/raw
mkdir -p $OUT
for run in 1 2 3; do
  echo "--fix=0 --strategy=multicast --run=$run"
  for s in multicast best-route self-learning; do echo "--fix=1 --strategy=$s --run=$run"; done
  for j in 0 5 10 20; do echo "--fix=1 --strategy=jitter-broadcast --jitter=$j --run=$run"; done
done | xargs -P 8 -I{} bash -c 'f=$(echo "{}" | tr -d " -" | tr "=" "_"); "$BIN" {} > "$OUT/$f.txt" 2>&1 && echo "ok {}"'
