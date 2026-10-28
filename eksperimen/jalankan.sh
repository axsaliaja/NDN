#!/bin/bash
# Build skenario lalu jalankan eksperimen (10 seed per konfigurasi, paralel) -> raw/<E>/<tag>__s<seed>.txt
# Dari WSL Ubuntu-20.04 (ndnSIM 2.9 di ~/ndnSIM/ns-3):
#   bash jalankan.sh            # semua eksperimen (E1–E7, ±15 menit di 16 thread)
#   bash jalankan.sh E2 E3      # sebagian
# Lalu di Windows/Linux: python analisis.py
set -e
HERE=$(cd "$(dirname "$0")" && pwd)
NS3=${NS3:-~/ndnSIM/ns-3}
SEEDS=${SEEDS:-"1 2 3 4 5 6 7 8 9 10"}
JOBS=${JOBS:-12}

mkdir -p $NS3/scratch/ndn-wifi-darurat
tr -d '\r' < "$HERE/ndn-wifi-darurat.cc" > $NS3/scratch/ndn-wifi-darurat/ndn-wifi-darurat.cc
(cd $NS3 && ./waf build -j$JOBS | tail -1)

# job <E> <tag> <argumen...>: satu konfigurasi, semua seed
job() { local e=$1 tag=$2; shift 2; for s in $SEEDS; do echo "$e $tag --run=$s $*"; done; }

J10="--fix=1 --strategy=jitter-broadcast --jitter=10"
RND="--topo=random --n=20 --area=300 --consumers=rand:5"

E1() { # replikasi 5 node: bawaan vs ad hoc (3 strategi bawaan) vs jeda acak
  job E1 bawaan --fix=0
  for s in multicast best-route self-learning; do job E1 adhoc-$s --fix=1 --strategy=$s; done
  for j in 0 5 10 20; do job E1 jeda$j --fix=1 --strategy=jitter-broadcast --jitter=$j; done
}
E2() { # uji dugaan hidden terminal: consumer hanya di node 1, node 2 sekadar meneruskan
  job E2 dua-node --n=2 --consumers=1 --fix=1 --strategy=multicast
  job E2 tiga-node-tersembunyi --n=3 --consumers=1 --fix=1 --strategy=multicast
  job E2 tiga-node-terdengar --n=3 --consumers=1 --fix=1 --strategy=multicast --range=170
  job E2 tiga-node-jeda10 --n=3 --consumers=1 $J10
}
E3() { # penyebab waktu tunggu naik: Interest ulang ditahan di relay
  job E3 multicast --fix=1 --strategy=multicast
  for r in -1 0 20 50 100 200; do job E3 retx$r $J10 --retxSup=$r; done
}
E4() { # panjang rantai (jumlah hop)
  for n in 3 4 5 6 8 10; do
    job E4 n$n-multicast --n=$n --consumers=1,-1 --fix=1 --strategy=multicast
    job E4 n$n-jeda10 --n=$n --consumers=1,-1 $J10
    job E4 n$n-jeda10-retx50 --n=$n --consumers=1,-1 $J10 --retxSup=50
  done
}
E5() { # beban: laju Interest per consumer
  for f in 2 5 10 20 40; do
    job E5 f$f-multicast --userFreq=$f --fix=1 --strategy=multicast
    job E5 f$f-jeda10 --userFreq=$f $J10
    job E5 f$f-jeda10-retx50 --userFreq=$f $J10 --retxSup=50
  done
}
E6() { # topologi acak 2D (20 node, 300×300 m), lalu node bergerak
  job E6 multicast $RND --fix=1 --strategy=multicast
  job E6 jeda10 $RND $J10
  for j in 5 10 20; do job E6 jeda$j-retx50 $RND --fix=1 --strategy=jitter-broadcast --jitter=$j --retxSup=50; done
  for v in 1.5 5; do
    job E6 gerak$v-multicast $RND --speed=$v --fix=1 --strategy=multicast
    job E6 gerak$v-jeda10-retx50 $RND --speed=$v $J10 --retxSup=50
  done
}
E7() { # nama bersama: agregasi PIT + cache NDN
  for sh in 0 1; do
    job E7 rantai-bersama$sh-multicast --shared=$sh --fix=1 --strategy=multicast
    job E7 rantai-bersama$sh-jeda10-retx50 --shared=$sh $J10 --retxSup=50
    job E7 acak-bersama$sh-multicast $RND --shared=$sh --fix=1 --strategy=multicast
    job E7 acak-bersama$sh-jeda10-retx50 $RND --shared=$sh $J10 --retxSup=50
  done
}

LIST=${*:-"E1 E2 E3 E4 E5 E6 E7"}
for e in $LIST; do rm -rf "$HERE/raw/$e"; mkdir -p "$HERE/raw/$e"; done
export LD_LIBRARY_PATH=$NS3/build/lib BIN=$NS3/build/scratch/ndn-wifi-darurat/ndn-wifi-darurat OUT=$HERE/raw
for e in $LIST; do $e; done | xargs -P $JOBS -L 1 bash -c '
  e=$0 tag=$1 seed=${2#--run=}
  "$BIN" "${@:2}" > "$OUT/$e/${tag}__s$seed.txt" 2>&1 || echo "GAGAL $e $tag seed $seed"'
echo "selesai: $(find "$HERE/raw" -name "*.txt" | wc -l) berkas di raw/"
