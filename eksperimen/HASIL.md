# Hasil Eksperimen

**Satu-satunya sumber angka untuk artikel dan slide.** Semua angka = rata-rata **10 seed** ± setengah lebar
**interval kepercayaan 95%** (distribusi t), dijalankan 28 Okt 2026 dengan ndnSIM 2.9 (NFD 22.02, ns-3 3.35),
program `ndn-wifi-darurat.cc` versi commit repo ini. Tabel lengkap (semua metrik, per node):
[hasil/ringkasan.md](hasil/ringkasan.md); CSV per eksperimen: `hasil/E*.csv`; grafik: `gambar/`.
Kegagalan dan dugaan yang terbantah tetap dicatat.

## Ringkasan

| Eksp. | Pertanyaan | Jawaban singkat | Status |
|---|---|---|---|
| E1 | Bisakah NDN di ndnSIM meneruskan data multi-hop lewat WiFi? | Bawaan: tidak (pengungsi 0%). Face AD_HOC: bisa, tetapi Relawan 1 hanya **17%**. Jeda acak 5–20 ms: semua **≈100%**, frame WiFi −28% | Terbukti |
| E2 | Mengapa Relawan 1 gagal? | **Transmisi serempak**, bukan *hidden terminal* murni: tetap gagal (1%) walau Posko dan Relawan 2 saling mendengar; ±446 frame per run terbuang di Relawan 1 | Dugaan minggu 1 **dikoreksi** |
| E3 | Mengapa waktu tunggu naik dengan jeda acak, dan bisakah diperbaiki? | Interest ulang ditahan di relay sampai 2 s. Diizinkan diteruskan lagi → waktu tunggu rata-rata **286 → 68 ms**, Data diterima **17,9 → 25,1 /s** | Terbukti + perbaikan |
| E4 | Tetap berlaku untuk rantai lebih panjang (2–9 hop)? | Ya: jeda acak + kirim ulang 100% sampai 9 hop; waktu tunggu node terjauh 25 → 121 ms | Terbukti |
| E5 | Bagaimana pada beban lebih tinggi? | Jeda + kirim ulang selalu tertinggi (74 Data/s pada 40 Interest/s per consumer vs 56 multicast); jenuh mulai ±20 Interest/s | Selesai |
| E6 | Topologi acak 20 node dan node bergerak? | Multicast runtuh (**32%** terjawab); jeda acak **98–99%**. Jeda 20 ms paling hemat (32 frame/Data). Bergerak 1,5–5 m/s: tetap ≥97% | Terbukti |
| E7 | Apa untungnya NDN bila banyak orang meminta data yang sama? | Agregasi PIT + *cache*: frame per Data turun **51%** (rantai) dan **65%** (acak); waktu tunggu rantai 68 → 21 ms | Terbukti |

Konfigurasi bersama (kecuali disebut lain): rantai 5 node (Posko–Relawan 1–Relawan 2–Pengungsi A–Pengungsi B,
jarak 80 m, jangkauan 100 m), consumer Relawan 1, Pengungsi A, Pengungsi B masing-masing 10 Interest/s, durasi
ukur 20 s. "Jeda N" = strategi `jitter-broadcast` dengan jeda acak maksimum N ms; "+ kirim ulang" = `--retxSup=50`.

### Definisi metrik

| Metrik | Arti |
|---|---|
| % terjawab | Interest bernomor urut berbeda yang mendapat Data ÷ yang dikirim selama 20 s |
| Data diterima per detik | Jumlah Data diterima semua consumer ÷ 20 s. **Lebih jujur daripada % terjawab**: consumer ndnSIM memakai giliran kirimnya untuk mengirim ulang, sehingga Interest yang macet mengurangi Interest baru (ideal = 10 × jumlah consumer) |
| Waktu tunggu | Dari Interest pertama dikirim sampai Data tiba, **termasuk kirim ulang**; hanya Interest yang terjawab. Dilaporkan rata-rata dan median |
| Frame WiFi | Frame yang dipancarkan semua node (PhyTxBegin) selama simulasi |
| Frame per Data | Frame WiFi ÷ Data diterima (ukuran efisiensi kanal; makin kecil makin hemat) |
| Frame terbuang di Relawan 1 | Frame yang tiba di Relawan 1 tetapi dibuang lapisan fisik (alasan dicatat ns-3) |

---

## E1 — Replikasi 5 node dengan 10 seed

**Pertanyaan:** apakah hasil minggu 1 (3 seed) tetap berlaku dengan 10 seed? **Perintah:** `bash jalankan.sh E1`.
Grafik: `gambar/E1_terjawab.png`.

| Konfigurasi | Relawan 1 (1 hop) | Pengungsi A (3 hop) | Pengungsi B (4 hop) | Data/s (ideal 30) | Frame WiFi | Frame per Data |
|---|---|---|---|---|---|---|
| Bawaan ndnSIM | 100 ± 0% | **0%** | **0%** | 9,9 ± 0,3 | 903 ± 16 | 4,6 |
| Ad hoc + multicast | **16,7 ± 4,9%** | 99,8 ± 0,2% | 100% | 17,2 ± 1,0 | 4.146 ± 133 | 12,1 ± 0,4 |
| Ad hoc + best-route | identik dengan multicast (semua angka sama) | | | | | |
| Ad hoc + self-learning | 16,4 ± 4,3% | 99,8 ± 0,2% | 100% | 17,0 ± 1,0 | 4.129 ± 106 | 12,2 ± 0,4 |
| Jeda 0 ms (pembanding) | **10,3 ± 3,9%** | 99,5 ± 0,3% | 99,9 ± 0,2% | 12,9 ± 0,8 | 3.302 ± 118 | 12,9 ± 0,4 |
| Jeda 5 ms | 99,9 ± 0,2% | 100% | 99,9 ± 0,2% | 18,1 ± 0,5 | 2.999 ± 60 | 8,3 ± 0,1 |
| Jeda 10 ms | 100% | 100% | 99,8 ± 0,3% | 17,9 ± 0,7 | 2.972 ± 93 | 8,3 ± 0,1 |
| Jeda 20 ms | 100% | 99,9 ± 0,2% | 99,9 ± 0,2% | 18,1 ± 0,4 | 2.956 ± 76 | 8,1 ± 0,1 |

Waktu tunggu jeda 10 ms (rata-rata / median): Relawan 1 199 / 7 ms, Pengungsi A 314 / 21 ms, Pengungsi B
375 / 28 ms. Median kecil, rata-rata besar → sebagian kecil Interest sangat lambat (dibahas E3).

**Kesimpulan.** (1) Face WiFi bawaan ndnSIM bertipe *point-to-point*, sehingga NFD menolak meneruskan paket
keluar lewat face tempat paket masuk; karena tiap node hanya punya satu antena, tidak ada data yang melewati
lebih dari 1 hop. (2) Face `AD_HOC` membuat multi-hop berfungsi, tetapi node 1 hop dari Posko justru gagal;
ketiga strategi bawaan NFD tidak berbeda. (3) Jeda acak 5–20 ms memperbaiki Relawan 1 menjadi ≈100% dan
mengurangi frame WiFi 28% (4.146 → ±2.970) karena penerusan dibatalkan bila Data sudah lewat. Jeda 0 ms tetap
gagal → yang menolong adalah **acaknya**, bukan strategi barunya. Konsisten dengan minggu 1 (Relawan 1: 13%
dengan 3 seed).

## E2 — Mengapa Relawan 1 gagal? (uji kontrol)

**Dugaan minggu 1:** *hidden terminal* — Posko dan Relawan 2 (160 m, di luar jangkauan satu sama lain) memancar
bersamaan dan bertabrakan di Relawan 1. **Cara:** hanya Relawan 1 yang meminta data; Relawan 2 sekadar
meneruskan. Bila dugaan benar, memperbesar jangkauan ke 170 m (Posko dan Relawan 2 saling mendengar sehingga
*carrier sense* bekerja) seharusnya memulihkan Relawan 1. **Perintah:** `bash jalankan.sh E2`. Grafik:
`gambar/E2_transmisi_serempak.png`.

| Kondisi | % Relawan 1 terjawab | Frame diterima utuh di Relawan 1 | Frame terbuang di Relawan 1 |
|---|---|---|---|
| A. 2 node (tanpa Relawan 2) | 100% | 219 ± 5 | 0,1 ± 0,2 |
| B. 3 node, jangkauan 100 m (saling tersembunyi) | **0%** | 1,1 ± 0,7 | **447 ± 10** |
| C. 3 node, jangkauan 170 m (saling mendengar) | **1,1 ± 0,7%** | 5,0 ± 3,1 | **445 ± 10** |
| D. 3 node, 100 m, jeda acak 10 ms | 100% | 422 ± 23 | 32 ± 4 |

Alasan frame terbuang (B dan C, satu seed diperiksa): 100% `PREAMBLE_DETECT_FAILURE` — dua frame tiba pada
saat yang sama sehingga preambul keduanya tidak terbaca.

**Kesimpulan.** Dugaan *hidden terminal* **terbantah** (C tetap gagal). Penyebabnya **transmisi serempak**:
Interest Relawan 1 diterima Posko dan Relawan 2 pada saat yang persis sama; Posko langsung membalas Data,
Relawan 2 langsung meneruskan Interest. Pada 802.11 DCF, stasiun yang mendapati kanal kosong boleh memancar
setelah DIFS tanpa *backoff* acak, jadi keduanya mulai bersamaan dan *carrier sense* tidak sempat mencegahnya.
Ini persis kasus "penerusan seketika" yang diperingatkan RFC 5148 [docs/LITERATUR.md, 7]. Pengungsi tidak
terkena karena di sepanjang rantai hanya satu node "baru" yang menerima tiap Interest/Data. Jeda acak memecah
keserempakan (D).

Batasan: di ndnSIM pemrosesan NFD tidak memakan waktu, jadi keserempakan di simulasi lebih sempurna daripada di
perangkat nyata (yang punya variasi waktu proses mikrodetik). Arah temuannya tetap sama: kedua pemancar dipicu
oleh frame yang sama.

## E3 — Penyebab waktu tunggu naik, dan perbaikannya

**Dugaan:** jeda acak hanya menambah ≤ 10 ms per hop, jadi waktu tunggu rata-rata ±300 ms bukan dari jedanya.
Penyebab yang diduga: bila sebuah Data hilang, consumer mengirim Interest ulang (Nonce baru, setelah RTO ≥ 200
ms), tetapi strategi minggu 1 **menahannya** di relay selama Interest lama masih tercatat di PIT (sampai
*lifetime* 2 s). **Perbaikan:** opsi `--retxSup`: Interest ulang boleh diteruskan lagi bila kiriman terakhir sudah
≥ retxSup ms lalu. **Perintah:** `bash jalankan.sh E3`. Grafik: `gambar/E3_waktu_tunggu.png`.

| Konfigurasi (rantai 5 node) | Waktu tunggu rata-rata R1 / PA / PB (ms) | Median R1 / PA / PB (ms) | Data/s (ideal 30) | Frame WiFi | Frame per Data |
|---|---|---|---|---|---|
| Multicast | 4.471 / 50 / 51 | 2.864 / 5 / 7 | 17,2 ± 1,0 | 4.146 ± 133 | 12,1 |
| Jeda 10, Interest ulang ditahan (minggu 1) | 199 / 314 / 375 | 7 / 21 / 28 | 17,9 ± 0,7 | 2.972 ± 93 | 8,3 |
| **Jeda 10 + kirim ulang** (retxSup 0–100 ms) | **38 / 71 / 100** | 7 / 22 / 29 | **25,1 ± 0,6** | 4.282 ± 93 | 8,5 |
| Jeda 10 + kirim ulang (retxSup 200 ms) | 37 / 73 / 99 | 8 / 22 / 29 | 25,1 ± 0,6 | 4.278 ± 93 | 8,5 |

**Kesimpulan.** Dugaan terbukti: median tidak berubah (jeda itu sendiri murah), yang turun adalah ekor
distribusinya. Waktu tunggu rata-rata gabungan 286 → 68 ms (−76%) dan Data diterima per detik +40%. Harganya:
frame WiFi naik 44%, tetapi **frame per Data tetap ±8,5** (sama efisien) dan tetap 30% lebih hemat daripada
multicast. Nilai retxSup 0–100 ms memberi hasil identik karena consumer ndnSIM baru mengirim ulang setelah RTO
minimum 200 ms; yang penting adalah *mengizinkan* penerusan ulang. Untuk E4–E7 dipakai `--retxSup=50`.

## E4 — Panjang rantai (2–9 hop)

**Cara:** rantai 3–10 node, consumer di node 1 dan node terjauh. **Perintah:** `bash jalankan.sh E4`. Grafik:
`gambar/E4_panjang_rantai.png`.

| Hop ke node terjauh | Multicast: % node 1 / % terjauh | Jeda 10: % node 1 / % terjauh | Jeda 10 + kirim ulang: % keduanya | Waktu tunggu node terjauh, jeda 10 / + kirim ulang (ms) | Frame per Data, + kirim ulang |
|---|---|---|---|---|---|
| 2 | 1,2 / 100 | 100 / 99,9 | 100 | 97 / 25 | 4,6 |
| 3 | 3,8 / 100 | 100 / 100 | 100 | 210 / 46 | 6,6 |
| 4 | 11,9 / 100 | 100 / 99,9 | 100 | 232 / 62 | 8,2 |
| 5 | 12,6 / 100 | 100 / 100 | 100 | 259 / 72 | 9,7 |
| 7 | 13,8 / 100 | 100 / 99,7 | 100 | 297 / 99 | 12,9 |
| 9 | 11,6 / 100 | 100 / 99,8 | 100 | 393 / 121 | 15,9 |

**Kesimpulan.** Kegagalan node 1-hop pada multicast muncul di semua panjang rantai (1–14%), sedangkan node
terjauh selalu 100% — sesuai mekanisme E2. Jeda acak + kirim ulang menjaga 100% sampai 9 hop; median waktu tunggu
node terjauh naik ±7 ms per hop (13,5 ms pada 2 hop → 63 ms pada 9 hop), sesuai rata-rata jeda 5 ms + waktu
pancar frame. Frame per Data naik hampir linear dengan jumlah hop (sifat penerusan hop-demi-hop).

## E5 — Beban (laju Interest per consumer)

**Cara:** rantai 5 node, 2–40 Interest/s per consumer (3 consumer). **Perintah:** `bash jalankan.sh E5`.
Grafik: `gambar/E5_beban.png`.

| Interest/s per consumer (ideal Data/s) | Data/s multicast | Data/s jeda 10 | Data/s jeda 10 + kirim ulang | Waktu tunggu rata-rata + kirim ulang (ms) | Terjawab tanpa kirim ulang, + kirim ulang |
|---|---|---|---|---|---|
| 2 (6) | 3,7 ± 0,1 | 5,0 ± 0,2 | **5,4 ± 0,2** | 61 ± 12 | 94% |
| 5 (15) | 8,9 ± 0,6 | 10,8 ± 0,4 | **13,4 ± 0,4** | 51 ± 5 | 92% |
| 10 (30) | 17,2 ± 1,0 | 17,9 ± 0,7 | **25,1 ± 0,6** | 68 ± 5 | 86% |
| 20 (60) | 31,3 ± 1,2 | 28,9 ± 0,9 | **44,3 ± 0,6** | 107 ± 5 | 76% |
| 40 (120) | 56,3 ± 1,2 | 43,8 ± 0,5 | **74,2 ± 0,7** | 183 ± 6 | 63% |

**Kesimpulan.** Jeda acak + kirim ulang selalu memberi Data/s tertinggi. Efisiensinya terhadap ideal turun dari
90% (2/s) ke 62% (40/s): kanal broadcast 6 Mbps mulai jenuh, terlihat dari porsi Interest yang perlu kirim
ulang naik dari 6% ke 37%. Pada beban tinggi, jeda tanpa kirim ulang lebih buruk daripada multicast — perbaikan
E3 wajib, bukan opsional.

## E6 — Topologi acak 20 node (area 300 × 300 m) dan node bergerak

**Cara:** 20 node acak (topologi berbeda tiap seed, diulang sampai terhubung), Posko di tengah, 5 consumer acak.
Lalu semua node kecuali Posko bergerak acak (*random walk*) 1,5 m/s (jalan kaki) atau 5 m/s. **Perintah:**
`bash jalankan.sh E6`. Grafik: `gambar/E6_topologi_acak.png`, `gambar/E6_mobilitas.png`.

| Konfigurasi (diam) | % terjawab | Data/s (ideal 50) | Waktu tunggu rata-rata (ms) | Frame per Data |
|---|---|---|---|---|
| Multicast | **31,5 ± 17,1** | 8,7 ± 5,2 | 2.502 ± 1.661 | 298 ± 318 |
| Jeda 10 | 98,1 ± 1,6 | 27,5 ± 3,2 | 343 ± 116 | 35,8 ± 3,1 |
| Jeda 5 + kirim ulang | 99,4 ± 0,7 | 36,1 ± 3,3 | 198 ± 114 | 43,0 ± 3,2 |
| Jeda 10 + kirim ulang | 99,3 ± 0,8 | 37,3 ± 3,1 | 156 ± 75 | 36,4 ± 2,9 |
| **Jeda 20 + kirim ulang** | **99,5 ± 0,7** | **38,5 ± 3,2** | **145 ± 70** | **32,0 ± 2,4** |

| Kecepatan | Multicast % terjawab | Jeda 10 + kirim ulang % terjawab | Jeda 10 + kirim ulang waktu tunggu (ms) |
|---|---|---|---|
| 0 m/s | 31,5 ± 17,1 | 99,3 ± 0,8 | 156 ± 75 |
| 1,5 m/s | 29,3 ± 14,8 | 98,3 ± 2,4 | 174 ± 107 |
| 5 m/s | 42,1 ± 12,3 | 97,5 ± 3,7 | 250 ± 188 |

**Kesimpulan.** Di jaringan padat (rata-rata banyak tetangga), transmisi serempak dan *broadcast storm* membuat
multicast runtuh dan sangat bervariasi antar-topologi. Jeda acak menjaga ≥98%. Jeda lebih besar (20 ms) paling
hemat frame — sesuai saran RFC 5148 bahwa jeda maksimum sebaiknya lebih besar pada jaringan lebih padat.
Mobilitas 1,5–5 m/s selama 20 s hampir tidak berpengaruh, karena area padat dan selalu ada rute lain (tanpa tabel
rute, flooding NDN langsung memakai tetangga yang ada). Biaya flooding tetap terlihat: ±32–43 frame per Data
(hampir semua node ikut meneruskan tiap Interest).

## E7 — Nama bersama (agregasi PIT + *cache*)

**Cara:** semua consumer meminta nama yang sama (`/posko/info/bersama/<nomor>`), mis. buletin posko, dibanding
nama berbeda per consumer. **Perintah:** `bash jalankan.sh E7`. Grafik: `gambar/E7_nama_bersama.png`.

| Topologi, strategi | Frame per Data: berbeda → sama | Frame WiFi: berbeda → sama | Waktu tunggu (ms): berbeda → sama | % terjawab: berbeda → sama |
|---|---|---|---|---|
| Rantai 5 node, multicast | 12,1 → 5,0 | 4.146 → 2.835 | 218 → 228 | 84,2 → 99,9 |
| Rantai 5 node, jeda 10 + kirim ulang | 8,5 → **4,2** (−51%) | 4.282 → 2.340 (−45%) | 68 → **21** | 100 → 100 |
| Acak 20 node, multicast | 298 → 30 | 15.409 → 11.100 | 2.502 → 1.969 | 31,5 → 75,8 |
| Acak 20 node, jeda 10 + kirim ulang | 36,4 → **12,7** (−65%) | 27.106 → 9.414 (−65%) | 156 → 102 | 99,3 → 99,3 |

**Kesimpulan.** Ini keunggulan khas NDN: Interest untuk nama yang sama digabung di PIT (Data cukup dikirim
sekali) dan Data yang lewat disimpan di *cache* node lain, jadi permintaan berikutnya dijawab dari dekat. Untuk
informasi darurat yang dibutuhkan banyak orang (peta evakuasi, daftar posko, peringatan), beban kanal turun
separuh atau lebih. Pada rantai multicast, nama bersama bahkan "menyembuhkan" Relawan 1 (84 → 99,9%) karena Data
untuk node lain lewat dan tersimpan di *cache*-nya.

---

## Arsip minggu 1 (14 Okt 2026)

`arsip/minggu-1/`: program lama `wifi-ndn-exp.cc`, 3 seed, hasil `hasil.csv`, screenshot demo. Angkanya
konsisten dengan E1 (Relawan 1 multicast 13% vs 16,7 ± 4,9% kini). Program baru menghasilkan angka yang sama
persis untuk konfigurasi bawaan, tetapi sedikit berbeda untuk konfigurasi lain karena urutan kejadian simulasi
berubah (penambahan pengukuran); jangan mencampur kedua set angka.

## Belum dikerjakan / batasan

- Kanal ideal (`RangePropagationLossModel`: sinyal putus tegas, tanpa *fading*). Model *log-distance* + *Nakagami*
  akan menambah kehilangan paket acak; arah temuan diperkirakan sama tetapi angkanya berubah.
- Hanya simulasi; belum diuji di Mini-NDN-WiFi atau perangkat nyata (lihat `docs/PENELITIAN.md`).
- Strategi berbasis jarak/lokasi (LFBL, LBDB) dan unicast setelah belajar tetangga belum dibandingkan.
