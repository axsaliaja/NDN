# Tinjauan Literatur

Semua rujukan di bawah sudah dicek keberadaannya (judul, penulis, venue, tahun) pada 28 Okt 2026.
Nomor mengikuti urutan kemunculan (gaya IEEE). Bila artikel/slide memakai rujukan lain, cek dulu
keberadaannya; jangan menambah rujukan yang belum pernah dibuka.

## 1. Masalah: komunikasi saat bencana

- Indonesia mencatat **2.107 kejadian bencana pada 2024** (BNPB, konferensi pers Jan 2025) [3]. Catatan:
  BNPB mengganti cara pencatatan pada 2024, jadi angka ini **tidak bisa dibandingkan langsung** dengan 5.400
  kejadian pada 2023.
- Setelah gempa Palu–Donggala 28 Sep 2018, **1.678 dari 4.193 BTS** di Sulawesi Tengah tidak berfungsi
  (Kemkominfo, 29 Sep 2018, 13.00 WIB); terbanyak di Kota Palu (1.167 BTS); penyebab utamanya pasokan listrik
  [4]. **Koreksi:** deck minggu 1 menulis "500+ BTS padam" — angka itu tidak ditemukan di sumber; pakai 1.678.
- RFC 8884 (IRTF ICNRG) memetakan arah riset ICN/NDN untuk skenario bencana: jaringan terfragmentasi, tanpa
  infrastruktur, perlu komunikasi berbasis data dan *delay-tolerant* [2].

## 2. Mengapa NDN

NDN [1] mengganti alamat host dengan **nama data**. Consumer mengirim *Interest* berisi nama; siapa pun yang
punya Data dengan nama itu (producer atau *cache* di tengah jalan) boleh menjawab, dan Data kembali menelusuri
jejak *Interest* yang tercatat di **PIT** (*Pending Interest Table*). Tiga sifat yang relevan untuk bencana:
tidak perlu alamat/server pusat, *cache* di setiap node (Content Store), dan Data bertanda tangan sehingga
keasliannya bisa dicek di mana pun ia disimpan [1], [2].

Penelitian NDN di MANET menunjukkan NDN bisa lebih unggul dari IP: Rahman & Zhang membandingkan forwarding
berbasis data (DAF) dengan AODV dan melaporkan latensi pengambilan data lebih rendah serta *overhead* transmisi
lebih kecil pada sebagian besar kasus, berkat *cache*, multicast bawaan, dan agregasi *Interest* [14]. DAPES
memakai NDN untuk berbagi berkas *off-the-grid* lewat beberapa hop nirkabel dan melaporkan *overhead* 50–71% dan
waktu berbagi 15–33% lebih rendah daripada solusi IP-MANET [15].

## 3. Masalah teknis: NDN di WiFi ad hoc

1. **Siaran (*broadcast*) di 802.11 tidak andal.** Frame broadcast/multicast dikirim tanpa ACK dan tanpa
   pengiriman ulang di lapisan MAC, pada laju dasar (*basic rate*) yang paling lambat; tingkat galat paket bisa mencapai 5% atau lebih [23]. Di NFD, semua
   paket ke face multicast/ad hoc menjadi broadcast. Pengembang NFD sendiri mengukur goodput multicast di WiFi
   hanya ±344 kbit/s dibanding ±1,70 Mbit/s untuk unicast (Feature #4973) [19].
2. ***Broadcast storm*.** *Flooding* naif menghasilkan redundansi, perebutan kanal, dan tabrakan [5].
3. ***Hidden terminal*.** Dua pengirim yang saling di luar jangkauan tidak bisa saling mendeteksi (CSMA gagal),
   sehingga frame mereka bertabrakan di penerima yang berada di tengah [6]. RTS/CTS tidak berlaku untuk
   broadcast.
4. **Face ad hoc.** NFD punya *link type* `AD_HOC` (Feature #3967): hanya pada face bertipe ini forwarder boleh
   mengirim paket keluar lewat face tempat paket itu masuk — syarat mutlak multi-hop di WiFi dengan satu antena
   [20], [21]. Face WiFi bawaan ndnSIM bertipe *point-to-point* (temuan kami, E1).
5. Survei terbaru menyimpulkan multicast NDN di nirkabel, terutama ad hoc, masih sulit karena mobilitas,
   ketidakstabilan link, dan medium bersama [16].

## 4. Pendekatan yang sudah ada

| Kelompok | Contoh | Ide | Kaitan dengan kami |
|---|---|---|---|
| *Flooding* murni | strategi `multicast` NFD | Interest disiarkan ke semua tetangga | Pembanding (E1–E7) |
| Penundaan acak (*jitter*) | RFC 5148 [7] | Jeda acak U(0, MAXJITTER) sebelum meneruskan/mengirim memecah sinkronisasi pengirim | **Dasar strategi kami** |
| Penundaan + supresi | LFBL [8], [9]; E-CHANET [10]; LBDB [12] | Penerima menunda dengan timer (berdasarkan jarak/lokasi), batal meneruskan bila sudah ada yang meneruskan | Strategi kami versi paling sederhana: timer acak tanpa informasi jarak |
| *Provider-aware* / unicast | Amadeo dkk. [11]; NDN-NDP [17]; Feature #4973 [19] | Setelah sumber/tetangga diketahui, kirim unicast agar ada ACK dan laju lebih tinggi | Pengembangan lanjutan (tidak kami uji) |
| Berbasis lokasi | Navigo [13], LBDB [12] | Interest diarahkan ke wilayah data | Butuh GPS; di luar lingkup |
| Optimasi lapisan link | de Sena & Dias (Wi-Fi 6) [18] | NDN native di atas Wi-Fi modern | Konteks kendaraan |

**Posisi kami.** Kami tidak mengusulkan protokol baru yang rumit. Kami (1) menunjukkan konfigurasi ndnSIM bawaan
gagal untuk multi-hop WiFi, (2) membuktikan dengan eksperimen kontrol bahwa kegagalan node 1-hop disebabkan
*hidden terminal* akibat penerusan serempak, (3) menerapkan jeda acak ala RFC 5148 sebagai strategi NFD dan
mengukur untung-ruginya, lalu (4) menemukan dan memperbaiki penyebab waktu tunggu yang naik (penahanan Interest
ulang), dan (5) mengujinya di topologi lebih besar, beban berbeda, mobilitas, dan data bersama.

## 5. Alat

ndnSIM [22] adalah simulator NDN di atas ns-3 yang memakai NFD dan ndn-cxx asli, sehingga strategi forwarding
yang ditulis untuk simulasi adalah kode NFD sungguhan. Kanal WiFi memakai model Yans ns-3.

## Daftar Pustaka

[1] L. Zhang *et al.*, "Named data networking," *ACM SIGCOMM Comput. Commun. Rev.*, vol. 44, no. 3, pp. 66–73, Jul. 2014.

[2] J. Seedorf, M. Arumaithurai, A. Tagami, K. Ramakrishnan, and N. Blefari Melazzi, "Research directions for using information-centric networking (ICN) in disaster scenarios," RFC 8884, IRTF, Oct. 2020. https://www.rfc-editor.org/rfc/rfc8884

[3] Metro TV News, "BNPB catat 2.107 bencana sepanjang 2024," Jan. 2025. https://www.metrotvnews.com/read/b7WCgXOX-bnpb-catat-2-107-bencana-sepanjang-2024

[4] detikInet, "Dampak gempa Donggala bikin 1.678 BTS tak berfungsi," 29 Sep. 2018. https://inet.detik.com/read/2018/09/29/180952/4234684/328/dampak-gempa-donggala-bikin-1678-bts-tak-berfungsi

[5] S.-Y. Ni, Y.-C. Tseng, Y.-S. Chen, and J.-P. Sheu, "The broadcast storm problem in a mobile ad hoc network," in *Proc. ACM MobiCom*, 1999, pp. 151–162, doi: 10.1145/313451.313525.

[6] F. Tobagi and L. Kleinrock, "Packet switching in radio channels: Part II—The hidden terminal problem in carrier sense multiple-access and the busy-tone solution," *IEEE Trans. Commun.*, vol. 23, no. 12, pp. 1417–1433, Dec. 1975.

[7] T. Clausen, C. Dearlove, and B. Adamson, "Jitter considerations in mobile ad hoc networks (MANETs)," RFC 5148, IETF, Feb. 2008. https://www.rfc-editor.org/rfc/rfc5148

[8] M. Meisel, V. Pappas, and L. Zhang, "Listen first, broadcast later: Topology-agnostic forwarding under high dynamics," in *Proc. Annu. Conf. Int. Technology Alliance (ACITA)*, Sep. 2010.

[9] M. Meisel, V. Pappas, and L. Zhang, "Ad hoc networking via named data," in *Proc. ACM MobiArch*, Chicago, Sep. 2010.

[10] M. Amadeo, G. Ruggeri, and A. Molinaro, "E-CHANET: Routing, forwarding and transport in information-centric multihop wireless networks," *Comput. Commun.*, vol. 36, no. 7, pp. 792–803, 2013.

[11] M. Amadeo, C. Campolo, and A. Molinaro, "Forwarding strategies in named data wireless ad hoc networks: Design and evaluation," *J. Netw. Comput. Appl.*, vol. 50, pp. 148–158, Apr. 2015, doi: 10.1016/j.jnca.2014.06.007.

[12] M. Kuai and X. Hong, "Location-based deferred broadcast for ad-hoc named data networking," *Future Internet*, vol. 11, no. 6, art. 139, 2019, doi: 10.3390/fi11060139.

[13] G. Grassi, D. Pesavento, G. Pau, L. Zhang, and S. Fdida, "Navigo: Interest forwarding by geolocations in vehicular named data networking," in *Proc. IEEE WoWMoM*, 2015.

[14] M. A. Rahman and B. Zhang, "On data-centric forwarding in mobile ad-hoc networks: Baseline design and simulation analysis," in *Proc. ICCCN*, 2021. arXiv:2105.07584

[15] S. Mastorakis, T. Li, and L. Zhang, "DAPES: Named data for off-the-grid file sharing with peer-to-peer interactions," in *Proc. IEEE ICDCS*, 2020. arXiv:2006.01651

[16] A. L. R. Madureira and L. N. Sampaio, "NDN multicast over wireless networks: A survey on fundamentals, challenges, and open issues," *Comput. Netw.*, vol. 254, art. 110815, 2024, doi: 10.1016/j.comnet.2024.110815.

[17] T. Chuachan, K. Djemame, and S. Puangpronpitag, "Solving MTU mismatch and broadcast overhead of NDN over link-layer networks," *Int. J. Netw. Distrib. Comput.*, vol. 8, no. 2, pp. 67–75, 2020, doi: 10.2991/ijndc.k.200213.001.

[18] Y. A. B. L. de Sena and K. L. Dias, "Towards a high efficiency of native NDN over Wi-Fi 6 for the Internet of Vehicles," in *Proc. SBRC*, 2022, pp. 601–614, doi: 10.5753/sbrc.2022.222382.

[19] NFD Redmine, "Feature #4973: Self-learning: switch from multicast to unicast on Wi-Fi infrastructure," 2019. https://redmine.named-data.net/issues/4973

[20] NFD Redmine, "Feature #3967: 'ad hoc' property on multi-access face," Feb. 2017. https://redmine.named-data.net/issues/3967

[21] A. Afanasyev *et al.*, "NFD developer's guide," NDN Project, Tech. Rep. NDN-0021, rev. 11, 2021. https://named-data.net/publications/techreports/ndn-0021-11-nfd-guide/

[22] S. Mastorakis, A. Afanasyev, and L. Zhang, "On the evolution of ndnSIM: An open-source simulator for NDN experimentation," *ACM SIGCOMM Comput. Commun. Rev.*, vol. 47, no. 3, pp. 19–33, Jul. 2017.

[23] C. Perkins, M. McBride, D. Stanley, W. Kumari, and J. C. Zúñiga, "Multicast considerations over IEEE 802 wireless media," RFC 9119, IETF, Oct. 2021. https://www.rfc-editor.org/rfc/rfc9119

### Rujukan yang **dikeluarkan** dari deck minggu 1

- *NFD Issue #4973 sebagai bukti "broadcast tidak dikirim ulang"*: isu itu sebenarnya tentang self-learning yang
  pindah dari multicast ke unicast di WiFi infrastruktur; dipakai di sini hanya untuk angka goodput [19].
- *"500+ BTS padam di Palu dan Donggala"*: diganti angka Kemkominfo [4].
