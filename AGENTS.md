# Konteks Proyek — NDN pada WiFi Ad Hoc untuk Komunikasi Darurat (Mata Kuliah NDN, Kelompok 3)

File ini dibaca otomatis oleh agen AI (Claude Code lewat `CLAUDE.md`, Codex, Antigravity, Cursor, dll.).
Tujuannya: semua anggota dan agennya menulis artikel dan slide yang **searah, konsisten, dan jujur terhadap
data**. Jelaskan ke mahasiswa dalam **Bahasa Indonesia**.

## Tim

| Nama | NIM | Peran saat ini |
|---|---|---|
| Raqinnaja Axsali | 18123054 | Eksperimen ndnSIM, riset literatur, repo ini |
| Daffa Naufal | 18123023 | Artikel dan slide (berdasarkan repo ini) |
| Ridho Radifa Al-Haq | 18123010 | Artikel dan slide (berdasarkan repo ini) |

Teknik Telekomunikasi, Institut Teknologi Bandung. Mata kuliah Named Data Networking (semester ganjil 2026/2027).

## Judul

> **Named Data Networking pada Jaringan WiFi Ad Hoc untuk Komunikasi Darurat Pascabencana: Evaluasi dan
> Perbaikan Strategi Forwarding Berbasis Simulasi ndnSIM**

## Satu sumber kebenaran per informasi

| Informasi | Satu-satunya tempat | Catatan |
|---|---|---|
| **Angka hasil eksperimen** | `eksperimen/HASIL.md` | Jangan menulis angka yang tidak ada di sana. Tabel lengkap otomatis: `eksperimen/hasil/ringkasan.md` |
| Rumusan masalah, hipotesis, metodologi, batasan | `docs/PENELITIAN.md` | |
| Literatur dan daftar pustaka (sudah diverifikasi) | `docs/LITERATUR.md` | |
| Cara menulis artikel dan slide, istilah, pesan kunci | `docs/PANDUAN-ARTIKEL-DAN-SLIDE.md` | **Baca ini sebelum menulis apa pun** |
| Cara menjalankan simulasi | `eksperimen/README.md` | |
| Grafik siap pakai | `eksperimen/gambar/*.png` | Dibangkitkan `analisis.py`; jangan digambar ulang dengan angka lain |

Bila dua berkas bertentangan, `eksperimen/HASIL.md` (angka) dan `docs/PENELITIAN.md` (arah) yang menang.
Bila menemukan kesalahan, perbaiki di sumbernya (lalu commit), jangan hanya di artikel/slide.

## Aturan untuk agen AI

1. **Jangan mengarang angka, rujukan, atau kutipan.** Angka hanya dari `eksperimen/HASIL.md`; rujukan hanya
   dari `docs/LITERATUR.md`. Rujukan baru boleh ditambahkan hanya setelah judul, penulis, venue, dan tahunnya
   dicek dari sumber asli, lalu dicatat di `docs/LITERATUR.md`.
2. **Selalu sebut kondisi angka**: "rata-rata 10 seed ± interval kepercayaan 95%" (dan topologinya). Angka
   minggu 1 (3 seed, `eksperimen/arsip/minggu-1/`) hanya untuk riwayat, jangan dicampur.
3. **Pakai istilah di glosarium** (`docs/PANDUAN-ARTIKEL-DAN-SLIDE.md`) secara konsisten; jangan bergantian
   menyebut hal yang sama dengan nama berbeda.
4. **Koreksi yang wajib diikuti** (jangan mengulang kesalahan minggu 1):
   - Relawan 1 gagal **bukan karena *hidden terminal* murni**, melainkan **transmisi serempak**: Posko dan
     Relawan 2 menerima Interest yang sama pada saat yang sama lalu sama-sama langsung memancar; preambul
     kedua frame bertabrakan di Relawan 1. Bukti: E2 (tetap gagal walau keduanya saling mendengar).
   - Waktu tunggu yang naik dengan jeda acak **bukan akibat jeda itu sendiri** (jeda hanya beberapa ms),
     melainkan karena Interest ulang dari consumer ditahan di relay sampai 2 detik. Bukti dan perbaikannya: E3.
   - "500+ BTS padam di Palu–Donggala" → yang benar **1.678 dari 4.193 BTS** tidak berfungsi (Kemkominfo).
5. **Sederhana.** Ini tugas mata kuliah, bukan tesis. Pesan inti cukup 3–4 temuan; jangan menambah
   eksperimen, metrik, atau istilah baru di artikel/slide yang tidak ada di repo.
6. **Klaim sesuai bukti.** Ini simulasi (kanal ideal, jangkauan terpotong tegas, tanpa *fading*). Tulis
   "dalam simulasi ndnSIM" dan cantumkan batasan dari `docs/PENELITIAN.md`.
7. **Jangan mengubah** `eksperimen/raw/`, `eksperimen/hasil/`, `eksperimen/gambar/` dengan tangan. Bila
   perlu angka baru, jalankan ulang eksperimen (`eksperimen/README.md`) dan perbarui `HASIL.md`.
8. Repo ini **publik**: jangan commit materi kuliah dosen (folder `materi-kuliah/` sengaja di-*ignore*),
   data pribadi selain nama/NIM anggota, kata sandi, atau token.

## Lingkungan simulasi (untuk yang ingin menjalankan ulang)

- ndnSIM 2.9 (NFD 22.02, ns-3 3.35) di Ubuntu 20.04 (laptop Raqinnaja: WSL `Ubuntu-20.04`, folder
  `~/ndnSIM/ns-3`). Satu program: `eksperimen/ndn-wifi-darurat.cc`; semua eksperimen: `eksperimen/jalankan.sh`;
  tabel dan grafik: `python eksperimen/analisis.py` (Python 3 + matplotlib).
- Di laptop Raqinnaja, WSL `Ubuntu` (24.04) berisi proyek TA lain — jangan diubah.

## Status (28 Okt 2026)

- [x] Minggu 1: 5 node, 3 seed, jeda acak (arsip di `eksperimen/arsip/minggu-1/`).
- [x] Minggu 2: E1–E7, 10 seed, literatur diverifikasi, folder dirapikan (lihat `eksperimen/HASIL.md`).
- [ ] Artikel (Daffa, Ridho) — ikuti `docs/PANDUAN-ARTIKEL-DAN-SLIDE.md`.
- [ ] Slide presentasi progres (Daffa, Ridho).
- [ ] Opsional berikutnya: kanal realistis (*log-distance* + *fading*), uji di Mini-NDN-WiFi bila tersedia.
