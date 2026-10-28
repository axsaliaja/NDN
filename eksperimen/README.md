# Eksperimen ndnSIM — NDN di WiFi ad hoc

Hasil dan angka: [HASIL.md](HASIL.md). Tabel lengkap otomatis: [hasil/ringkasan.md](hasil/ringkasan.md).

## Isi folder

| Berkas | Isi |
|---|---|
| `ndn-wifi-darurat.cc` | Satu skenario ndnSIM berparameter: topologi, face AD_HOC, strategi `jitter-broadcast` (usulan), statistik |
| `jalankan.sh` | Build + semua eksperimen E1–E7 (10 seed per konfigurasi, paralel) → `raw/<E>/` |
| `analisis.py` | `raw/` → `hasil/<E>.csv`, `hasil/ringkasan.md`, `gambar/*.png` |
| `demo.sh` | Demo kelas ±1 menit (satu seed) |
| `raw/`, `hasil/`, `gambar/` | Keluaran mentah, ringkasan, grafik — jangan diedit tangan |
| `arsip/minggu-1/` | Eksperimen minggu 1 (5 node, 3 seed, program lama `wifi-ndn-exp.cc`, screenshot demo) |

## Prasyarat

ndnSIM 2.9 terpasang di Ubuntu 20.04 (native atau WSL) pada `~/ndnSIM/ns-3` (ikuti panduan instalasi ndnSIM;
bukti instalasi kelompok: `../praktik/Instalasi-ndnSIM-Kelompok-3.pdf`). Analisis butuh Python 3 + matplotlib
(boleh di Windows).

## Menjalankan

Dari folder ini di Ubuntu/WSL (lokasi ndnSIM lain: `NS3=/path/ke/ns-3 bash jalankan.sh`):

```bash
bash jalankan.sh            # semua (±20 menit dengan 12 proses paralel)
bash jalankan.sh E2 E3      # sebagian
python3 analisis.py         # tabel + grafik
bash demo.sh                # demo singkat
```

Satu konfigurasi manual (setelah `jalankan.sh` pernah mem-build):

```bash
B=~/ndnSIM/ns-3/build/scratch/ndn-wifi-darurat/ndn-wifi-darurat
LD_LIBRARY_PATH=~/ndnSIM/ns-3/build/lib $B --fix=1 --strategy=jitter-broadcast --jitter=10 --retxSup=50
```

| Opsi | Arti (bawaan) |
|---|---|
| `--topo` | `chain` (rantai), `grid`, `random` (`chain`) |
| `--n`, `--spacing`, `--area`, `--range` | jumlah node (5), jarak rantai/grid m (80), sisi area acak m (300), jangkauan radio m (100) |
| `--speed` | kecepatan gerak acak node non-producer, hanya `random` (0) |
| `--fix` | 1 = face WiFi `AD_HOC`; 0 = bawaan ndnSIM (*point-to-point*) (1) |
| `--strategy` | `multicast`, `best-route`, `self-learning`, `jitter-broadcast` (`multicast`) |
| `--jitter` | jeda acak maksimum sebelum meneruskan Interest, ms (10) |
| `--retxSup` | Interest ulang dari consumer boleh diteruskan lagi bila kiriman terakhir ≥ nilai ini ms lalu; −1 = tidak pernah (perilaku minggu 1) (−1) |
| `--consumers` | indeks node peminta, −1 = node terakhir, atau `rand:K` (`1,3,4`) |
| `--shared` | 1 = semua consumer meminta nama yang sama (0) |
| `--userFreq`, `--dur`, `--run` | laju Interest/s per consumer (10), durasi ukur s (20), seed (1) |
| `--trace` | berkas jejak paket NDN per node/face per detik (kosong) |

Keluaran per run: `RESULT` (parameter), `CONS` per consumer (`sent` = nomor urut berbeda yang dikirim,
`ok` = terjawab, `sat` = %, `dmean/dmed/d95` = waktu tunggu rata-rata/median/persentil-95 ms, `firsttry` =
terjawab tanpa kirim ulang), `SUM` (frame WiFi dipancarkan `phyTx`, frame diterima/rusak/terbuang), `DROPN1`
(alasan frame terbuang di node 1).

## Model simulasi (ringkas)

802.11a ad hoc; semua paket NDN dikirim broadcast pada 6 Mbps tanpa ACK; kanal Yans dengan
`RangePropagationLossModel` (sinyal putus tegas di `range`); producer `/posko` (Data 1.024 B);
consumer `ConsumerCbr` (interval acak seragam, *lifetime* Interest 2 s); Content Store 1.000 paket; rute
bawaan ke semua face. Detail dan batasannya: `../docs/PENELITIAN.md`.
