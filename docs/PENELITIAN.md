# Rancangan Penelitian

Acuan arah untuk artikel dan slide. Angka hanya di `eksperimen/HASIL.md`; rujukan di `docs/LITERATUR.md`
(nomor [n] di bawah mengikuti daftar pustaka di sana).

## Judul

**Named Data Networking pada Jaringan WiFi Ad Hoc untuk Komunikasi Darurat Pascabencana: Evaluasi dan
Perbaikan Strategi Forwarding Berbasis Simulasi ndnSIM**

## Latar belakang (satu paragraf)

Saat bencana besar, jaringan seluler sering lumpuh (gempa Palu 2018: 1.678 dari 4.193 BTS tidak berfungsi [4]),
padahal ponsel, laptop, dan router warga serta relawan masih punya WiFi yang bisa saling terhubung langsung
(WiFi ad hoc). Agar informasi dari posko sampai ke pengungsi yang jauh, perangkat di tengah harus meneruskan data
secara estafet (multi-hop). NDN cocok untuk kondisi ini karena meminta data berdasarkan nama, tanpa alamat atau
server pusat, dan setiap node bisa menyimpan salinan data [1], [2]. Tantangannya, WiFi ad hoc memakai siaran tanpa
ACK, sehingga tabrakan dan *broadcast storm* mudah terjadi [5], [16], [23].

## Rumusan masalah

1. Apakah NDN (ndnSIM/NFD) dapat meneruskan data multi-hop melalui WiFi ad hoc, dan apa syaratnya?
2. Mengapa sebagian node gagal mendapat data, dan apakah jeda acak sebelum meneruskan Interest memperbaikinya?
3. Berapa harga jeda acak (waktu tunggu, beban kanal), dan bagaimana menguranginya?
4. Apakah temuan tetap berlaku pada rantai lebih panjang, beban lebih tinggi, topologi acak, node bergerak, dan
   data yang diminta banyak orang?

## Tujuan

Mengevaluasi forwarding NDN di WiFi ad hoc dengan simulasi ndnSIM, menemukan penyebab kegagalannya, serta
merancang dan menguji perbaikan strategi forwarding yang sederhana.

## Hipotesis dan statusnya

| # | Hipotesis (dinyatakan sebelum eksperimen) | Eksp. | Status |
|---|---|---|---|
| H1 | Face WiFi bawaan ndnSIM tidak meneruskan data lebih dari 1 hop | E1 | Terbukti |
| H2 | Node dekat producer gagal karena *hidden terminal* | E2 | **Terbantah** → penyebabnya transmisi serempak |
| H3 | Jeda acak sebelum meneruskan Interest memulihkan semua node | E1, E2 | Terbukti |
| H4 | Waktu tunggu naik karena Interest ulang ditahan di relay, bukan karena jeda | E3 | Terbukti; perbaikan diuji |
| H5 | Perbaikan tetap berlaku untuk jaringan lebih besar, beban lebih tinggi, dan node bergerak | E4–E6 | Terbukti (dengan batas jenuh pada beban tinggi, E5) |
| H6 | Nama bersama mengurangi beban kanal berkat agregasi PIT dan *cache* | E7 | Terbukti |

## Metodologi

**Alat:** ndnSIM 2.9 (NFD 22.02, ns-3 3.35) [22]. Strategi baru ditulis sebagai strategi NFD sungguhan
(`jitter-broadcast`, di `eksperimen/ndn-wifi-darurat.cc`).

**Skenario utama:** rantai 5 node berjarak 80 m dengan jangkauan radio 100 m (tiap node hanya menjangkau
tetangga langsung): Posko (producer `/posko`) – Relawan 1 – Relawan 2 – Pengungsi A – Pengungsi B. Relawan 1,
Pengungsi A, Pengungsi B masing-masing meminta 10 Interest/s; Relawan 2 hanya meneruskan.

**Variabel bebas:** jenis face (bawaan vs AD_HOC), strategi (multicast, best-route, self-learning,
jitter-broadcast), jeda maksimum (0–20 ms), izin meneruskan Interest ulang, jumlah node/hop (2–9), laju Interest
(2–40/s), topologi (rantai, acak 20 node), kecepatan gerak (0–5 m/s), nama berbeda vs sama.

**Variabel terikat:** % Interest terjawab, Data diterima per detik, waktu tunggu (rata-rata dan median), frame
WiFi, frame per Data, frame terbuang di node 1 (definisi: `eksperimen/HASIL.md`).

**Variabel kontrol:** 802.11a ad hoc, broadcast 6 Mbps tanpa ACK, Data 1.024 B, *lifetime* Interest 2 s,
Content Store 1.000 paket, durasi ukur 20 s.

**Pengulangan dan statistik:** 10 seed per konfigurasi (topologi acak: 10 topologi berbeda); dilaporkan rata-rata
± setengah lebar interval kepercayaan 95% (distribusi t). Total 690 simulasi, dapat diulang dengan satu perintah
(`eksperimen/jalankan.sh`) dan hasilnya deterministik per seed.

**Uji kontrol (E2):** untuk membedakan *hidden terminal* dari transmisi serempak, jangkauan diperbesar sehingga
kedua pemancar saling mendengar; penyebab dipastikan dari alasan frame terbuang yang dicatat ns-3.

## Strategi usulan: `jitter-broadcast`

1. Interest dari aplikasi di node itu sendiri langsung dikirim.
2. Interest yang diteruskan untuk node lain ditunda acak U(0, J) ms (RFC 5148 [7]).
3. Bila selama menunggu Interest itu sudah diteruskan atau Data sudah lewat, penerusan dibatalkan (hemat frame).
4. (Perbaikan E3) Interest ulang dari consumer — Nonce baru setelah RTO — boleh diteruskan lagi, alih-alih
   ditahan sampai entri PIT kedaluwarsa.

Posisinya terhadap literatur: versi paling sederhana dari keluarga "penundaan + supresi" (LFBL, E-CHANET,
LBDB [8]–[12]), tanpa informasi jarak atau lokasi.

## Temuan utama (detail dan angka: `eksperimen/HASIL.md`)

1. Face WiFi bawaan ndnSIM bertipe *point-to-point*; multi-hop baru berfungsi dengan face `AD_HOC`.
2. Dengan face AD_HOC dan strategi bawaan, node 1 hop dari producer hampir selalu gagal karena **transmisi
   serempak** (producer dan relay dipicu frame yang sama), bukan *hidden terminal* murni.
3. Jeda acak 5–20 ms memulihkan semua node dan menghemat frame; mengizinkan penerusan Interest ulang memangkas
   waktu tunggu rata-rata sekitar tiga perempat dan menaikkan Data yang diterima.
4. Perbaikan bertahan sampai 9 hop, pada topologi acak 20 node, dan saat node bergerak; nama bersama
   memanfaatkan agregasi PIT dan *cache* NDN sehingga beban kanal turun separuh atau lebih.

## Batasan (wajib disebut di artikel)

- **Simulasi**, kanal ideal: sinyal putus tegas di 100 m, tanpa *fading* atau rintangan; satu laju 6 Mbps.
- NFD di ndnSIM memproses paket tanpa jeda waktu, sehingga keserempakan di simulasi lebih sempurna daripada di
  perangkat nyata.
- Beban berupa Interest dengan laju tetap; bukan pola lalu lintas nyata saat bencana.
- Mobilitas hanya *random walk* di area 300 × 300 m selama 20 s.
- Tidak mengukur energi baterai dan tidak menguji keamanan (tanda tangan Data tidak diverifikasi).
- Mini-NDN-WiFi (emulasi) tidak dipakai karena butuh fitur WiFi virtual kernel yang tidak tersedia di WSL2.

## Pengembangan lanjutan (saran)

Kanal realistis (*log-distance* + *Nakagami*), jeda yang menyesuaikan kepadatan tetangga, unicast setelah
tetangga diketahui (NDN-NDP, Feature #4973 [17], [19]), dan uji di perangkat nyata (Raspberry Pi/ponsel).
