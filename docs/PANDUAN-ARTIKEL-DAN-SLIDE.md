# Panduan Menulis Artikel dan Slide

Untuk Daffa, Ridho, dan agen AI kalian. Tujuannya: artikel dan slide **satu suara**, angkanya bisa ditelusuri,
dan tidak mengulang kesalahan minggu 1. Bila dosen memberi templat/format sendiri, ikuti templat itu; isi tetap
dari repo ini.

## 1. Pesan inti (hafalkan; semua bagian harus mendukung empat kalimat ini)

1. **Bawaan ndnSIM tidak bisa multi-hop di WiFi.** Face WiFi-nya *point-to-point*; harus diubah ke `AD_HOC`.
2. **Setelah ad hoc, node terdekat posko malah gagal** karena **transmisi serempak**: posko dan relay dipicu
   frame yang sama dan memancar bersamaan. (Bukan *hidden terminal* murni — dibuktikan uji kontrol E2.)
3. **Jeda acak sebelum meneruskan Interest + izin meneruskan Interest ulang** membuat semua node terlayani,
   lebih cepat, dan lebih hemat kanal daripada multicast — sampai 9 hop, di topologi acak 20 node, dan saat
   node bergerak.
4. **Keunggulan khas NDN:** bila banyak orang meminta data yang sama, agregasi PIT dan *cache* memotong beban
   kanal 51–65%.

## 2. Glosarium (pakai istilah ini, konsisten)

| Istilah di artikel/slide | Arti | Jangan ditulis sebagai |
|---|---|---|
| Interest | Paket permintaan data berisi nama | "request", "paket permintaan" bergantian |
| Data | Paket jawaban berisi isi + tanda tangan | "respon", "konten" |
| Producer (Posko) / consumer (peminta) | Penyedia data / node yang meminta | "server/klien" |
| PIT | Tabel Interest yang masih menunggu Data | |
| *Cache* (Content Store) | Salinan Data di setiap node | |
| Face | Antarmuka NFD (di sini: antena WiFi) | "port" |
| Face AD_HOC | Face yang boleh meneruskan paket keluar lewat face yang sama | "mode ad hoc diaktifkan" saja (sebut face-nya) |
| Strategi forwarding | Aturan NFD kapan/ke mana Interest diteruskan | "routing" |
| Multicast | Strategi bawaan NFD: teruskan ke semua tetangga seketika | "flooding biasa" bergantian |
| Jeda acak (*jitter*) | Tunda U(0, J) ms sebelum meneruskan Interest | "delay", "timer" |
| Interest ulang | Interest yang dikirim lagi consumer karena Data tak kunjung datang | "retransmisi" bergantian |
| Jitter-broadcast | Nama strategi usulan kami | nama lain |
| Transmisi serempak | Dua node dipicu frame yang sama lalu memancar bersamaan | "hidden terminal" (lihat koreksi) |
| Hop | Satu lompatan antar-node | |
| Waktu tunggu | Dari Interest pertama dikirim sampai Data tiba (termasuk kirim ulang) | "latency" bergantian |
| Frame WiFi / frame per Data | Jumlah pancaran / pancaran per Data diterima | "overhead" tanpa definisi |
| Seed | Satu pengulangan simulasi dengan bilangan acak berbeda | "percobaan" (ambigu) |

## 3. Aturan angka

- Ambil angka **hanya** dari `eksperimen/HASIL.md`; selalu tulis kondisinya, mis. "rata-rata 10 seed, IK 95%,
  rantai 5 node".
- Di slide boleh dibulatkan (17%, 286 → 68 ms); di artikel tulis dengan ± IK pada tabel.
- **% terjawab** bisa menyesatkan pada beban tinggi; untuk membandingkan throughput pakai **Data diterima per
  detik** (penjelasan di definisi metrik HASIL.md).
- Grafik: pakai `eksperimen/gambar/*.png` apa adanya (atau gambar ulang dari `eksperimen/hasil/E*.csv`, bukan
  dari angka yang diketik ulang).

## 4. Koreksi dari minggu 1 (wajib)

| Minggu 1 menulis | Yang benar sekarang | Bukti |
|---|---|---|
| Relawan 1 gagal karena *hidden terminal* (dugaan) | Transmisi serempak; *hidden terminal* bukan syarat | E2 |
| "Harganya, data ke pengungsi lebih lambat (±0,4 s)" | Lambat karena Interest ulang ditahan; setelah diperbaiki ±0,07–0,1 s | E3 |
| 3 percobaan, 3 seed | 7 eksperimen, 690 simulasi, 10 seed + IK 95% | HASIL.md |
| 500+ BTS padam di Palu–Donggala | 1.678 dari 4.193 BTS tidak berfungsi (Kemkominfo, 29 Sep 2018) | LITERATUR [4] |
| NFD Issue #4973 = "broadcast tidak dikirim ulang" | Isu itu soal self-learning multicast→unicast; dipakai hanya untuk angka goodput WiFi | LITERATUR [19] |

## 5. Struktur artikel (saran, format IEEE)

1. **Abstrak** (150–200 kata): masalah → metode (ndnSIM, 690 simulasi) → 3–4 temuan dengan angka kunci →
   kesimpulan.
2. **Pendahuluan:** bencana dan BTS lumpuh [3], [4]; WiFi ad hoc; NDN [1], [2]; tantangan siaran [5], [23];
   rumusan masalah dan kontribusi (4 butir, dari `docs/PENELITIAN.md`).
3. **Tinjauan pustaka:** NDN; NDN di MANET [14], [15]; masalah siaran dan *hidden terminal* [5], [6], [16];
   pendekatan penundaan [7]–[12]; posisi kami (tabel di `docs/LITERATUR.md` §4).
4. **Metodologi:** skenario + gambar topologi, parameter (tabel variabel kontrol), strategi `jitter-broadcast`
   (4 langkah), metrik, 10 seed + IK 95%.
5. **Hasil dan pembahasan:** satu subbab per pertanyaan, urutan E1 → E7, masing-masing: grafik, 2–3 kalimat
   hasil, 2–3 kalimat penjelasan mekanisme. E2 ditulis sebagai "dugaan awal kami terbantah" — ini nilai
   ilmiahnya.
6. **Batasan** (dari `docs/PENELITIAN.md`).
7. **Kesimpulan dan saran.**
8. **Daftar pustaka** (dari `docs/LITERATUR.md`, nomor ulang sesuai urutan kemunculan di artikel).

## 6. Slide progres (saran ±15 slide)

| # | Slide | Isi | Gambar |
|---|---|---|---|
| 1 | Judul | Judul, anggota, "Progres minggu 2" | |
| 2 | Latar | Bencana, 1.678 BTS mati, WiFi ad hoc | |
| 3 | NDN | Interest/Data, PIT, *cache* | |
| 4 | Tantangan | Siaran tanpa ACK, serempak, storm | |
| 5 | Rumusan masalah | 4 pertanyaan | |
| 6 | Metode | Topologi 5 node, ndnSIM, 10 seed, 690 simulasi | diagram topologi |
| 7 | Strategi usulan | 4 langkah jitter-broadcast | |
| 8 | E1 | Bawaan 0%, ad hoc 17%, jeda ≈100% | `E1_terjawab.png` |
| 9 | E2 | Dugaan *hidden terminal* terbantah; transmisi serempak | `E2_transmisi_serempak.png` |
| 10 | E3 | Waktu tunggu 286 → 68 ms | `E3_waktu_tunggu.png` |
| 11 | E4–E5 | Sampai 9 hop; beban tinggi | `E4_panjang_rantai.png`, `E5_beban.png` |
| 12 | E6 | Acak 20 node: multicast 32% vs jeda ≥98%; bergerak | `E6_topologi_acak.png` |
| 13 | E7 | Nama bersama: frame −51…65% | `E7_nama_bersama.png` |
| 14 | Kesimpulan + batasan | 4 pesan inti, 3 batasan utama | |
| 15 | Referensi | | |

Demo langsung (opsional): `bash eksperimen/demo.sh` (±1 menit).

## 7. Pertanyaan yang mungkin muncul (siapkan jawabannya)

- *Kenapa simulasi, bukan perangkat nyata?* → Batasan di PENELITIAN.md; Mini-NDN-WiFi tidak jalan di WSL2.
- *Kenapa 10 seed cukup?* → IK 95% sempit untuk metrik utama (mis. Relawan 1: 16,7 ± 4,9%); E6 lebar karena
  topologi berbeda tiap seed.
- *Bukankah 802.11 punya carrier sense?* → Ya, tapi kedua node memancar pada saat yang sama setelah DIFS;
  *carrier sense* tidak bisa mendeteksi pancaran yang dimulai bersamaan (E2-C).
- *Apa bedanya dengan LFBL/LBDB?* → Mereka memakai jarak/lokasi untuk menentukan timer; kami acak murni (lebih
  sederhana, tanpa GPS), cukup untuk memecah keserempakan.
- *Kenapa retxSup 0–100 ms hasilnya sama?* → consumer ndnSIM mengirim ulang paling cepat setelah RTO 200 ms.
