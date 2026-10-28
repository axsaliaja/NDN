"""Ringkas raw/*.txt (output wifi-ndn-exp) menjadi hasil.csv + grafik_keberhasilan.png."""
import glob, os, re, statistics as st
import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt

HERE = os.path.dirname(os.path.abspath(__file__))
SERIES = ["#2a78d6", "#eb6834", "#1baf7a", "#eda100"]  # slot kategori 1-4 (urutan tetap)
INK, MUTED, GRID = "#0b0b0b", "#52514e", "#e4e3df"
USERS = ["Relawan1(1hop)", "PengungsiA(3hop)", "PengungsiB(4hop)"]
TICKS = ["Relawan 1\n(1 hop)", "Pengungsi A\n(3 hop)", "Pengungsi B\n(4 hop)"]


def parse(path):
    txt = open(path, encoding="utf-8", errors="replace").read()
    h = re.search(r"RESULT (.*)", txt)
    if not h:
        return None
    r = dict(kv.split("=") for kv in h.group(1).split())
    r["key"] = (r["fix"], r["strategy"], r.get("jitter"))
    r["users"] = {m[0]: (float(m[1]), float(m[2])) for m in
                  re.findall(r"^\s+(\S+) sent=\d+ ok=\d+ sat=([\d.]+)% delay=([\d.]+)ms", txt, re.M)}
    r["macTx"] = float(re.search(r"wifiMacTx=(\d+)", txt).group(1))
    return r


rows = [r for r in map(parse, glob.glob(os.path.join(HERE, "raw", "*.txt"))) if r]
mean = lambda xs: st.mean(xs) if xs else float("nan")

CONFIGS = [  # (kunci, label) - 4 pertama digambar
    (("0", "multicast", None), "Default ndnSIM"),
    (("1", "multicast", None), "Patch ad hoc (multicast)"),
    (("1", "jitter-broadcast", "5"), "+ jeda acak maks 5 ms"),
    (("1", "jitter-broadcast", "20"), "+ jeda acak maks 20 ms"),
    (("1", "best-route", None), "Patch ad hoc (best-route)"),
    (("1", "self-learning", None), "Patch ad hoc (self-learning)"),
    (("1", "jitter-broadcast", "0"), "jitter-broadcast 0 ms"),
    (("1", "jitter-broadcast", "10"), "+ jeda acak maks 10 ms"),
]

summary = {}
csv = ["konfigurasi,consumer,terjawab_pct,delay_ms,frame_wifi,seed"]
for key, label in CONFIGS:
    rs = [r for r in rows if r["key"] == key]
    sat = [mean([r["users"][u][0] for r in rs]) for u in USERS]
    dly = [mean([r["users"][u][1] for r in rs if r["users"][u][0] > 0]) for u in USERS]
    tx = mean([r["macTx"] for r in rs])
    summary[label] = (sat, dly, tx)
    for u, s, d in zip(USERS, sat, dly):
        csv.append(f"{label},{u},{s:.1f},{d:.1f},{tx:.0f},{len(rs)}")
open(os.path.join(HERE, "hasil.csv"), "w").write("\n".join(csv))
print("\n".join(csv))


# Grafik slide hasil: hanya konfigurasi SETELAH face ad hoc diperbaiki, supaya dampak perbaikan
# konfigurasi tidak tercampur dengan dampak jeda acak. 1100 x 600 px, huruf besar untuk proyektor.
PLOT = [("Patch ad hoc (multicast)", "Tanpa jeda (multicast)"),
        ("jitter-broadcast 0 ms", "Jeda 0 ms (pembanding)"),
        ("+ jeda acak maks 5 ms", "Jeda acak maks 5 ms"),
        ("+ jeda acak maks 10 ms", "Jeda acak maks 10 ms"),
        ("+ jeda acak maks 20 ms", "Jeda acak maks 20 ms")]
COLORS = SERIES + ["#e87ba4"]  # slot kategori 1-5, urutan tetap (lolos validasi CVD/normal-vision)
fig, ax = plt.subplots(figsize=(11, 6), dpi=100)
w = 0.16
for i, (key, label) in enumerate(PLOT):
    sat = summary[key][0]
    xs = [j + (i - 2) * w for j in range(len(USERS))]
    ax.bar(xs, sat, w - 0.025, color=COLORS[i], label=label)
    for x, s in zip(xs, sat):
        if s < 25:  # batang pendek tetap terbaca
            ax.text(x, s + 2, f"{s:.0f}%", ha="center", fontsize=15, color=INK, fontweight="bold")
ax.set_ylabel("% Interest terjawab", color=MUTED, fontsize=16)
ax.grid(axis="y", color=GRID, lw=0.8)
ax.set_axisbelow(True)
for s in ("top", "right"):
    ax.spines[s].set_visible(False)
for s in ("left", "bottom"):
    ax.spines[s].set_color(GRID)
ax.tick_params(colors=MUTED, labelsize=15)
ax.set_xticks(range(len(USERS)))
ax.set_xticklabels(TICKS, color=INK, fontsize=16)
ax.set_ylim(0, 108)
ax.legend(frameon=False, fontsize=14, loc="upper center", bbox_to_anchor=(0.5, -0.17), ncol=3)
fig.tight_layout()
fig.savefig(os.path.join(HERE, "grafik_keberhasilan.png"), facecolor="#ffffff")
