"""Ringkas raw/<E>/*.txt (keluaran ndn-wifi-darurat) menjadi hasil/<E>.csv, hasil/ringkasan.md, gambar/*.png.

Setiap angka = rata-rata dari seed (biasanya 10) ± setengah lebar interval kepercayaan 95% (distribusi t).
Jalankan dari folder mana pun: python eksperimen/analisis.py  (butuh matplotlib)
"""
import glob
import os
import re
import statistics as st
from collections import defaultdict

import matplotlib

matplotlib.use("Agg")
import matplotlib.pyplot as plt

HERE = os.path.dirname(os.path.abspath(__file__))
RAW, OUT, FIG = (os.path.join(HERE, d) for d in ("raw", "hasil", "gambar"))
T95 = {1: 12.706, 2: 4.303, 3: 3.182, 4: 2.776, 5: 2.571, 6: 2.447, 7: 2.365, 8: 2.306, 9: 2.262,
       10: 2.228, 11: 2.201, 12: 2.179, 13: 2.160, 14: 2.145, 19: 2.093, 29: 2.045}
# Warna kategori tetap (urutan tidak berubah antar-grafik; lolos validasi CVD minggu 1)
C = ["#2a78d6", "#eb6834", "#1baf7a", "#eda100", "#e87ba4", "#7a5bd6"]
INK, MUTED, GRID = "#0b0b0b", "#52514e", "#e4e3df"


def kv(s):
    return {k: float(v) if re.fullmatch(r"-?[\d.e+-]+", v) else v for k, v in (x.split("=") for x in s.split())}


def parse(path):
    txt = open(path, encoding="utf-8", errors="replace").read()
    head = re.search(r"^RESULT (.*)$", txt, re.M)
    if not head:
        print("GAGAL/tidak lengkap:", path)
        return None
    r = kv(head.group(1))
    r["cons"] = [kv(m) for m in re.findall(r"^CONS (.*)$", txt, re.M)]
    r["sum"] = kv(re.search(r"^SUM (.*)$", txt, re.M).group(1))
    e, name = os.path.basename(os.path.dirname(path)), os.path.basename(path)[:-4]
    r["E"], r["tag"], r["seed"] = e, name.rsplit("__s", 1)[0], int(name.rsplit("__s", 1)[1])
    return r


def ci(xs):
    xs = [x for x in xs if x == x]
    if not xs:
        return float("nan"), float("nan"), 0
    m = st.mean(xs)
    h = T95.get(len(xs) - 1, 2.0) * st.stdev(xs) / len(xs) ** 0.5 if len(xs) > 1 else 0.0
    return m, h, len(xs)


def per_run(r):
    """Metrik satu run: per node consumer (kunci 'n<id>') dan gabungan semua consumer ('semua')."""
    dur, out = r["dur"], {}
    tot_sent = tot_ok = tot_first = 0
    dsum = 0.0
    for c in r["cons"]:
        ok, sent = c["ok"], c["sent"]
        node = {
            "hop": c["hop"], "terjawab_pct": c["sat"], "goodput_per_s": ok / dur,
            "tunggu_rata_ms": c["dmean"] if ok else float("nan"),
            "tunggu_median_ms": c["dmed"] if ok else float("nan"),
            "tunggu_p95_ms": c["d95"] if ok else float("nan"),
            "sekali_kirim_pct": 100 * c["firsttry"] / ok if ok else float("nan"),
        }
        if r["topo"] != "random":  # topologi acak: consumer berbeda tiap seed, hanya angka gabungan bermakna
            out["n%d" % c["node"]] = node
        tot_sent, tot_ok, tot_first = tot_sent + sent, tot_ok + ok, tot_first + c["firsttry"]
        dsum += c["dmean"] * ok
    s = r["sum"]
    out["semua"] = {
        "terjawab_pct": 100 * tot_ok / tot_sent if tot_sent else 0, "goodput_per_s": tot_ok / dur,
        "tunggu_rata_ms": dsum / tot_ok if tot_ok else float("nan"),
        "sekali_kirim_pct": 100 * tot_first / tot_ok if tot_ok else float("nan"),
        "frame_wifi": s["phyTx"], "frame_per_data": s["phyTx"] / tot_ok if tot_ok else float("nan"),
        "rx_rusak": s["rxErr"], "rx_rusak_n1": s["rxErrN1"], "rx_ok_n1": s["rxOkN1"],
        "rx_terbuang_n1": s.get("dropN1", float("nan")),
    }
    return out


def load():
    rows = [r for r in map(parse, sorted(glob.glob(os.path.join(RAW, "E*", "*.txt")))) if r]
    agg = defaultdict(lambda: defaultdict(list))  # (E, tag, scope) -> metrik -> [nilai per seed]
    order = {}
    for r in rows:
        order.setdefault((r["E"], r["tag"]), len(order))
        for scope, met in per_run(r).items():
            for k, v in met.items():
                agg[(r["E"], r["tag"], scope)][k].append(v)
    return rows, agg, order


def write_tables(agg, order):
    os.makedirs(OUT, exist_ok=True)
    md = ["# Ringkasan otomatis (dibangkitkan analisis.py; jangan diedit tangan)\n",
          "Format sel: rata-rata ± setengah lebar IK 95% (n seed).\n"]
    by_e = defaultdict(list)
    for key in sorted(agg, key=lambda k: (k[0], order[(k[0], k[1])], k[2] != "semua", k[2])):
        by_e[key[0]].append(key)
    for e, keys in by_e.items():
        metrics = sorted({m for k in keys for m in agg[k]}, key=lambda m: list(agg[keys[0]]).index(m)
                         if m in agg[keys[0]] else 99)
        lines = ["konfigurasi,cakupan,metrik,rata,ik95,n"]
        for k in keys:
            for m in metrics:
                if m in agg[k]:
                    mu, h, n = ci(agg[k][m])
                    lines.append(f"{k[1]},{k[2]},{m},{mu:.3f},{h:.3f},{n}")
        open(os.path.join(OUT, f"{e}.csv"), "w", encoding="utf-8", newline="\n").write("\n".join(lines) + "\n")
        # tabel markdown: baris = (konfigurasi, cakupan), kolom = metrik utama
        cols = [m for m in ("hop", "terjawab_pct", "goodput_per_s", "tunggu_rata_ms", "tunggu_median_ms",
                            "sekali_kirim_pct", "frame_wifi", "frame_per_data", "rx_ok_n1", "rx_terbuang_n1") if m in metrics]
        md.append(f"\n## {e}\n\n| konfigurasi | cakupan | " + " | ".join(cols) + " |\n|" + "---|" * (len(cols) + 2))
        for k in keys:
            cells = []
            for m in cols:
                mu, h, n = ci(agg[k].get(m, []))
                cells.append("" if n == 0 else (f"{mu:.0f}" if m == "hop" else f"{mu:.1f} ± {h:.1f}"))
            md.append(f"| {k[1]} | {k[2]} | " + " | ".join(cells) + " |")
    open(os.path.join(OUT, "ringkasan.md"), "w", encoding="utf-8", newline="\n").write("\n".join(md) + "\n")


# ---------------- grafik ----------------
def style(ax, ylabel):
    ax.set_ylabel(ylabel, color=MUTED, fontsize=13)
    ax.grid(axis="y", color=GRID, lw=0.8)
    ax.set_axisbelow(True)
    for s in ("top", "right"):
        ax.spines[s].set_visible(False)
    for s in ("left", "bottom"):
        ax.spines[s].set_color(GRID)
    ax.tick_params(colors=MUTED, labelsize=12)


def bars(ax, groups, series, get, ylabel, label_fmt=None):
    """groups: label sumbu x; series: [(label, key)]; get(key, group) -> (mean, ci)."""
    w = 0.8 / len(series)
    for i, (lab, key) in enumerate(series):
        xs = [j + (i - (len(series) - 1) / 2) * w for j in range(len(groups))]
        vals = [get(key, g) for g in groups]
        ax.bar(xs, [v[0] for v in vals], w - 0.02, yerr=[v[1] for v in vals], color=C[i], label=lab,
               error_kw={"ecolor": INK, "elinewidth": 1, "capsize": 3})
        if label_fmt:
            for x, v in zip(xs, vals):
                ax.text(x, v[0] + v[1] + 1, label_fmt(v[0]), ha="center", fontsize=10, color=INK)
    ax.set_xticks(range(len(groups)))
    style(ax, ylabel)


def save(fig, name):
    os.makedirs(FIG, exist_ok=True)
    fig.tight_layout()
    fig.savefig(os.path.join(FIG, name), dpi=130, facecolor="#ffffff")
    plt.close(fig)


def figures(agg):
    g = lambda e, tag, scope, m: ci(agg.get((e, tag, scope), {}).get(m, []))[:2]
    NODE5 = [("n1", "Relawan 1\n(1 hop)"), ("n3", "Pengungsi A\n(3 hop)"), ("n4", "Pengungsi B\n(4 hop)")]

    if any(k[0] == "E1" for k in agg):
        cfg = [("Bawaan ndnSIM", "bawaan"), ("Ad hoc + multicast", "adhoc-multicast"),
               ("Ad hoc + jeda 0 ms", "jeda0"), ("Ad hoc + jeda acak 10 ms", "jeda10")]
        fig, ax = plt.subplots(figsize=(11, 5.5))
        bars(ax, [n for n, _ in NODE5], cfg, lambda t, n: g("E1", t, n, "terjawab_pct"), "% Interest terjawab",
             lambda v: f"{v:.0f}%")
        ax.set_xticklabels([l for _, l in NODE5], fontsize=13, color=INK)
        ax.set_ylim(0, 115)
        ax.legend(frameon=False, fontsize=11, ncol=4, loc="upper center", bbox_to_anchor=(0.5, -0.15))
        save(fig, "E1_terjawab.png")

    if any(k[0] == "E2" for k in agg):
        cfg = [("A. 2 node", "dua-node"), ("B. 3 node\n(100 m)", "tiga-node-tersembunyi"),
               ("C. 3 node\n(170 m)", "tiga-node-terdengar"), ("D. 3 node (100 m)\n+ jeda 10 ms", "tiga-node-jeda10")]
        fig, (a1, a2) = plt.subplots(1, 2, figsize=(13, 5))
        for ax, m, yl in ((a1, "terjawab_pct", "% Interest Relawan 1 terjawab"), (a2, "rx_terbuang_n1", "Frame terbuang di Relawan 1\n(preambul gagal dideteksi)")):
            scope = "n1" if m == "terjawab_pct" else "semua"
            vals = [g("E2", t, scope, m) for _, t in cfg]
            ax.bar(range(4), [v[0] for v in vals], 0.6, yerr=[v[1] for v in vals], color=[C[0], C[1], C[2], C[3]],
                   error_kw={"ecolor": INK, "capsize": 3})
            ax.set_xticks(range(4))
            ax.set_xticklabels([l for l, _ in cfg], fontsize=10, color=INK)
            style(ax, yl)
        save(fig, "E2_transmisi_serempak.png")

    if any(k[0] == "E3" for k in agg):
        tags = [("multi-\ncast", "multicast"), ("tidak\npernah", "retx-1"), ("0", "retx0"), ("20", "retx20"),
                ("50", "retx50"), ("100", "retx100"), ("200", "retx200")]
        fig, (a1, a2) = plt.subplots(1, 2, figsize=(14, 5))
        x = range(len(tags))
        for i, (m, lab) in enumerate((("tunggu_rata_ms", "rata-rata"), ("tunggu_median_ms", "median"))):
            # median per node dirata-rata atas 3 consumer
            vals = []
            for _, t in tags:
                per = [ci(agg[("E3", t, n)][m])[0] for n, _ in NODE5 if ("E3", t, n) in agg]
                vals.append(st.mean(per) if per else float("nan"))
            a1.plot(x, vals, "o-", color=C[i], lw=2, label=lab)
        a1.set_xticks(list(x))
        a1.set_xticklabels([l for l, _ in tags], fontsize=10, color=INK)
        style(a1, "Waktu tunggu (ms), rerata 3 consumer")
        a1.legend(frameon=False)
        for ax in (a1, a2):
            ax.set_xlabel("Jeda 10 ms; Interest ulang boleh diteruskan lagi setelah … ms", color=MUTED)
        v = [g("E3", t, "semua", "frame_wifi") for _, t in tags]
        a2.bar(x, [y[0] for y in v], 0.6, yerr=[y[1] for y in v], color=C[2], error_kw={"ecolor": INK, "capsize": 3})
        a2.set_xticks(list(x))
        a2.set_xticklabels([l for l, _ in tags], fontsize=10, color=INK)
        style(a2, "Frame WiFi dipancarkan (20 s)")
        save(fig, "E3_waktu_tunggu.png")

    if any(k[0] == "E4" for k in agg):
        ns = [3, 4, 5, 6, 8, 10]
        cfg = [("Ad hoc + multicast", "multicast"), ("Jeda acak 10 ms", "jeda10"), ("Jeda acak 10 ms + kirim ulang 50 ms", "jeda10-retx50")]
        fig, (a1, a2) = plt.subplots(1, 2, figsize=(13, 5))
        for i, (lab, c) in enumerate(cfg):
            far = [g("E4", f"n{n}-{c}", f"n{n-1}", "terjawab_pct") for n in ns]
            near = [g("E4", f"n{n}-{c}", "n1", "terjawab_pct") for n in ns]
            a1.errorbar([n - 1 for n in ns], [v[0] for v in far], [v[1] for v in far], fmt="o-", color=C[i], lw=2, capsize=3, label=lab)
            a1.plot([n - 1 for n in ns], [v[0] for v in near], "o--", color=C[i], lw=1, alpha=0.7)
            d = [g("E4", f"n{n}-{c}", f"n{n-1}", "tunggu_rata_ms") for n in ns]
            a2.errorbar([n - 1 for n in ns], [v[0] for v in d], [v[1] for v in d], fmt="o-", color=C[i], lw=2, capsize=3, label=lab)
        style(a1, "% terjawab (garis penuh: node terjauh,\nputus-putus: node 1 hop)")
        style(a2, "Waktu tunggu node terjauh (ms)")
        for ax in (a1, a2):
            ax.set_xlabel("Jumlah hop ke node terjauh", color=MUTED)
        a1.set_ylim(0, 105)
        a1.legend(frameon=False, fontsize=10)
        save(fig, "E4_panjang_rantai.png")

    if any(k[0] == "E5" for k in agg):
        fs = [2, 5, 10, 20, 40]
        cfg = [("Ad hoc + multicast", "multicast"), ("Jeda acak 10 ms", "jeda10"), ("Jeda acak 10 ms + kirim ulang 50 ms", "jeda10-retx50")]
        fig, (a1, a2) = plt.subplots(1, 2, figsize=(13, 5))
        for i, (lab, c) in enumerate(cfg):
            s = [g("E5", f"f{f}-{c}", "semua", "goodput_per_s") for f in fs]
            a1.errorbar(fs, [v[0] for v in s], [v[1] for v in s], fmt="o-", color=C[i], lw=2, capsize=3, label=lab)
            d = [g("E5", f"f{f}-{c}", "semua", "tunggu_rata_ms") for f in fs]
            a2.errorbar(fs, [v[0] for v in d], [v[1] for v in d], fmt="o-", color=C[i], lw=2, capsize=3, label=lab)
        a1.plot(fs, [3 * f for f in fs], ":", color=MUTED, lw=1, label="ideal (3 × laju)")
        for ax in (a1, a2):
            ax.set_xscale("log")
            ax.set_xticks(fs)
            ax.set_xticklabels(fs)
            ax.set_xlabel("Laju Interest per consumer (Interest/s)", color=MUTED)
        style(a1, "Data diterima per detik (3 consumer)")
        style(a2, "Waktu tunggu rata-rata (ms)")
        a1.legend(frameon=False, fontsize=10)
        save(fig, "E5_beban.png")

    if any(k[0] == "E6" for k in agg):
        cfg = [("Multi-\ncast", "multicast"), ("Jeda\n10", "jeda10"), ("Jeda 5\n+ulang", "jeda5-retx50"),
               ("Jeda 10\n+ulang", "jeda10-retx50"), ("Jeda 20\n+ulang", "jeda20-retx50")]
        fig, axs = plt.subplots(1, 3, figsize=(16, 5))
        for ax, m, yl in zip(axs, ("terjawab_pct", "tunggu_rata_ms", "frame_per_data"),
                             ("% terjawab (5 consumer)", "Waktu tunggu rata-rata (ms, log)", "Frame WiFi per Data diterima (log)")):
            v = [g("E6", t, "semua", m) for _, t in cfg]
            if m != "terjawab_pct":
                ax.set_yscale("log")
            ax.bar(range(len(cfg)), [y[0] for y in v], 0.65, yerr=[y[1] for y in v], color=C[:len(cfg)],
                   error_kw={"ecolor": INK, "capsize": 3})
            ax.set_xticks(range(len(cfg)))
            ax.set_xticklabels([l for l, _ in cfg], fontsize=10, color=INK)
            style(ax, yl)
        save(fig, "E6_topologi_acak.png")
        sp = [0, 1.5, 5]
        cfg = [("Multicast", "multicast"), ("Jeda 10 ms + kirim ulang 50 ms", "jeda10-retx50")]
        fig, (a1, a2) = plt.subplots(1, 2, figsize=(12, 5))
        for i, (lab, c) in enumerate(cfg):
            tg = [c if v == 0 else f"gerak{v}-{c}" for v in sp]
            s = [g("E6", t, "semua", "terjawab_pct") for t in tg]
            d = [g("E6", t, "semua", "tunggu_rata_ms") for t in tg]
            a1.errorbar(sp, [v[0] for v in s], [v[1] for v in s], fmt="o-", color=C[i], lw=2, capsize=3, label=lab)
            a2.errorbar(sp, [v[0] for v in d], [v[1] for v in d], fmt="o-", color=C[i], lw=2, capsize=3, label=lab)
        for ax in (a1, a2):
            ax.set_xticks(sp)
            ax.set_xlabel("Kecepatan gerak node (m/s)", color=MUTED)
        style(a1, "% terjawab")
        style(a2, "Waktu tunggu rata-rata (ms)")
        a1.legend(frameon=False, fontsize=10)
        save(fig, "E6_mobilitas.png")

    if any(k[0] == "E7" for k in agg):
        groups = [("Rantai 5 node\nmulticast", "rantai-bersama{}-multicast"), ("Rantai 5 node\njeda + kirim ulang", "rantai-bersama{}-jeda10-retx50"),
                  ("Acak 20 node\nmulticast", "acak-bersama{}-multicast"), ("Acak 20 node\njeda + kirim ulang", "acak-bersama{}-jeda10-retx50")]
        ser = [("Nama berbeda per consumer", "0"), ("Nama sama (data bersama)", "1")]
        fig, (a1, a2) = plt.subplots(1, 2, figsize=(14, 5))
        for ax, m, yl in ((a1, "frame_per_data", "Frame WiFi per Data diterima (log)"), (a2, "tunggu_rata_ms", "Waktu tunggu rata-rata (ms, log)")):
            ax.set_yscale("log")
            bars(ax, [t for _, t in groups], ser, lambda sh, t: g("E7", t.format(sh), "semua", m), yl)
            ax.set_xticklabels([l for l, _ in groups], fontsize=10, color=INK)
        a1.legend(frameon=False, fontsize=10)
        save(fig, "E7_nama_bersama.png")


if __name__ == "__main__":
    rows, agg, order = load()
    write_tables(agg, order)
    figures(agg)
    print(f"{len(rows)} run dibaca; tabel di hasil/, grafik di gambar/")
