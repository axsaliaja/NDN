> **Arsip minggu 1 (14 Okt 2026).** Sudah digantikan `../../ndn-wifi-darurat.cc` dan `../../HASIL.md`; path di bawah merujuk ke lokasi lama.

# Eksperimen: NDN di WiFi ad hoc untuk komunikasi darurat (ndnSIM 2.9)

5 node berjajar 80 m, jangkauan radio 100 m:
`Posko (producer) ~ Relawan1 ~ Relawan2 ~ PengungsiA ~ PengungsiB`

| File | Isi |
|---|---|
| `wifi-ndn-exp.cc` | Skenario ndnSIM: face WiFi AD_HOC + strategi `jitter-broadcast` (jeda rebroadcast acak) |
| `demo.sh` | Demo kelas: 3 konfigurasi, seed 1 (~1 menit) |
| `jalankan.sh` | Build + semua konfigurasi × 3 seed → `raw/` |
| `analisis.py` | `raw/` → `hasil.csv` + `grafik_keberhasilan.png` (Python Windows, butuh matplotlib) |
| `screenshot_demo.png` | Screenshot terminal saat `demo.sh` dijalankan |

## Menjalankan (WSL Ubuntu-20.04)

```bash
bash /mnt/c/Users/axsal/Downloads/NDN/eksperimen-validasi/jalankan.sh   # pertama kali / setelah ubah kode
bash /mnt/c/Users/axsal/Downloads/NDN/eksperimen-validasi/demo.sh       # demo di kelas
```

Satu konfigurasi manual:

```bash
cd ~/ndnSIM/ns-3
./waf --run "wifi-ndn-exp --fix=0"                                            # face default ndnSIM
./waf --run "wifi-ndn-exp --fix=1 --strategy=multicast"                       # face AD_HOC
./waf --run "wifi-ndn-exp --fix=1 --strategy=jitter-broadcast --jitter=10"    # + jeda acak maks 10 ms
```

Opsi lain: `--dur` (detik), `--run` (seed), `--spacing` (m), `--range` (m), `--userFreq` (Interest/s).

## Hasil (rata-rata 3 seed)

| Konfigurasi | Relawan 1 | Pengungsi A | Pengungsi B | Frame WiFi |
|---|---|---|---|---|
| Default ndnSIM | 100% | 0% | 0% | 894 |
| Patch ad hoc (multicast / best-route / self-learning) | 13% | 100% | 100% | 4.127 |
| + jeda acak maks 5 ms | 100% | 100% | 100% | 2.927 |
| + jeda acak maks 20 ms | 100% | 100% | 99,7% | 2.890 |

Trade-off: delay pengungsi naik dari 44–72 ms menjadi 267–430 ms. Detail di `hasil.csv`.
