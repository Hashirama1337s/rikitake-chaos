"""Figures for the rikitake-chaos repository.

Usage:  python figures/make_figures.py <repo root>
Writes figures/*.svg next to this script. Inputs, all shipped in the repository:
  proof/design_mu*_a*_c*.txt   line 1: mu, a, c (snapshot plane z = c); line 3: box ends l0 l1 r0 r1 and width w
  proof/run_mu*_a*_c*.log      checker A's proved landing ranges of the four box edges ("Q(... edge ...) u in [lo, hi]")
The return-map dots and the time series are a float illustration recomputed here from the equations
x' = -mu x + y z, y' = -mu y + (z - a) x, z' = 1 - x y (scipy DOP853, fixed start); they are not part of the proof.
Checked copy (figure referee, 2026-11-17): simulated layers say "not proved"; box names on the three panels.
"""
import glob
import os
import re
import sys

import numpy as np
import matplotlib

matplotlib.use("Agg")
import matplotlib.pyplot as plt
from matplotlib.patches import FancyBboxPatch
from scipy.integrate import solve_ivp

BG, BORDER, GRID, AXC, TXT, ACC = "#1c2026", "#2f353d", "#22272e", "#9aa4ae", "#e8edf2", "#39ff14"
CAT = ["#3987e5", "#d95926", "#199e70", "#c98500", "#d55181", "#0ca30c", "#9085e9", "#e66767"]


def style():
    plt.rcParams.update({
        "svg.fonttype": "none", "svg.hashsalt": "rikitake-chaos-figures",
        "font.family": "sans-serif", "font.sans-serif": ["Segoe UI", "Helvetica", "Arial", "DejaVu Sans"],
        "font.size": 14, "axes.titlesize": 15, "axes.labelsize": 14, "xtick.labelsize": 12.5, "ytick.labelsize": 12.5,
        "legend.fontsize": 12.5, "figure.facecolor": "none", "axes.facecolor": BG, "axes.edgecolor": AXC,
        "axes.labelcolor": TXT, "axes.titlecolor": TXT, "xtick.color": AXC, "ytick.color": AXC, "text.color": TXT,
        "axes.grid": True, "grid.color": GRID, "grid.linewidth": 0.8, "axes.spines.top": False,
        "axes.spines.right": False, "legend.facecolor": BG, "legend.edgecolor": BORDER, "legend.labelcolor": TXT,
        "legend.framealpha": 1.0, "lines.linewidth": 2.0,
    })


def frame(fig):
    w, h = fig.get_size_inches()
    fig.patches.append(FancyBboxPatch((0.03, 0.03), w - 0.06, h - 0.06, boxstyle="round,pad=0,rounding_size=0.18",
                                      transform=fig.dpi_scale_trans, fc=BG, ec=BORDER, lw=1.5, zorder=-100))


def save(fig, name):
    here = os.path.dirname(os.path.abspath(__file__))
    out = here if os.path.basename(here) == "figures" else os.path.join(here, "figures")   # repo: figures/make_figures.py
    os.makedirs(out, exist_ok=True)
    fig.savefig(os.path.join(out, name), format="svg", dpi=200, metadata={"Date": None, "Creator": None})
    plt.close(fig)
    print("wrote figures/" + name)


def mix(col, a):
    """col at opacity a over the background, as a solid colour (renders the same everywhere)."""
    c = np.array([int(col[i:i + 2], 16) for i in (1, 3, 5)], float)
    b = np.array([int(BG[i:i + 2], 16) for i in (1, 3, 5)], float)
    return "#%02x%02x%02x" % tuple(int(round(v)) for v in a * c + (1 - a) * b)


def out5(lo, hi):
    """outward rounding to 5 decimals, so a printed range always contains the proved one."""
    return "%.5f-%.5f" % (np.floor(lo * 1e5) / 1e5, np.ceil(hi * 1e5) / 1e5)


def read_cases(root):
    cases = []
    for f in sorted(glob.glob(os.path.join(root, "proof", "design_mu*_a*_c*.txt"))):
        lines = open(f, encoding="utf-8").read().split("\n")
        mu, a, c = map(float, lines[0].split()[:3])
        l0, l1, r0, r1, w = map(float, lines[2].split()[:5])
        tag = os.path.basename(f)[len("design_"):-len(".txt")]
        log = open(os.path.join(root, "proof", "run_%s.log" % tag), encoding="utf-8").read()
        assert "RESULT: ALL COVERING CONDITIONS VERIFIED" in log
        edges = {}
        for side, box, lo, hi, ok in re.findall(
                r"Q\((left|right)\s+edge (N_L|N_R)\), need [^:]*: u in \[([\d.]+), ([\d.]+)\] : (true|false)", log):
            assert ok == "true"
            edges[(box, side)] = (float(lo), float(hi))
        assert len(edges) == 4
        cases.append(dict(mu=mu, a=a, c=c, l0=l0, l1=l1, r0=r0, r1=r1, w=w, edges=edges))
    order = {(1.0, 2.0): 0, (1.0, 3.75): 1, (2.0, 5.0): 2}
    return sorted(cases, key=lambda d: order.get((d["mu"], d["a"]), 9))


def field(mu, a):
    return lambda t, s: [-mu * s[0] + s[1] * s[2], -mu * s[1] + (s[2] - a) * s[0], 1 - s[0] * s[1]]


def return_map(cs, T=8000.0):
    f = field(cs["mu"], cs["a"])
    ev = lambda t, s: s[2] - cs["c"]
    ev.direction = -1
    s0 = solve_ivp(f, (0, 300), [1, 0.5, 2], method="DOP853", rtol=1e-11, atol=1e-12).y[:, -1]
    sol = solve_ivp(f, (0, T), s0, method="DOP853", rtol=1e-11, atol=1e-12, events=ev)
    E = sol.y_events[0]
    sg = np.sign(E[:, 0])
    X = np.abs(E[:, 0])
    flip = sg[1:] != sg[:-1]
    return X[:-1], X[1:], flip, s0


def draw_map(ax, cs, small=False):
    xn, xn1, flip, _ = return_map(cs)
    ms = 2.0 if not small else 1.6
    ax.plot(xn[~flip], xn1[~flip], ".", ms=ms, color=CAT[0], rasterized=True, zorder=2,
            label="next loop keeps the polarity")
    ax.plot(xn[flip], xn1[flip], ".", ms=ms, color=CAT[1], rasterized=True, zorder=2,
            label="next loop reverses it")
    for lo, hi, col in ((cs["l0"], cs["l1"], CAT[0]), (cs["r0"], cs["r1"], CAT[1])):
        ax.axhspan(lo, hi, color=mix(col, 0.13), lw=0, zorder=0.5)
    for lo, hi, col in ((cs["l0"], cs["l1"], CAT[0]), (cs["r0"], cs["r1"], CAT[1])):
        ax.axvspan(lo, hi, fc=mix(col, 0.30), ec=col, lw=1, zorder=1)
    # proved landing ranges of the four box edges (checker A), drawn as intervals
    for (box, side), (lo, hi) in cs["edges"].items():
        x = cs[{"N_L": {"left": "l0", "right": "l1"}, "N_R": {"left": "r0", "right": "r1"}}[box][side]]
        ax.plot([x, x], [lo, hi], "-", color=ACC, lw=4 if not small else 3, solid_capstyle="butt", zorder=5)
        ax.plot([x - 0.05, x + 0.05], [lo, lo], "-", color=ACC, lw=1.5, zorder=5)
        ax.plot([x - 0.05, x + 0.05], [hi, hi], "-", color=ACC, lw=1.5, zorder=5)
    return xn, xn1


def fig_hero(cs):
    fig = plt.figure(figsize=(12.5, 7.4))
    frame(fig)
    ax = fig.add_axes([0.085, 0.10, 0.60, 0.70])
    fig.text(0.03, 0.94, "Two boxes, three proved stretches: chaotic flip patterns exist in the dynamo", fontsize=17,
             weight="bold")
    fig.text(0.03, 0.855, "Return map at mu = %g, a = %g: the dynamo's x each time z falls through %g, against the next one "
             "(dots: simulation, not proved).\nShaded: the two proof boxes. Green bars: where the boxes' edges are proved to land."
             % (cs["mu"], cs["a"], cs["c"]), fontsize=13, color=AXC, linespacing=1.4)
    xn, xn1 = draw_map(ax, cs)
    ax.plot([2.0, 5.4], [2.0, 5.4], ":", color=AXC, lw=1, zorder=1)
    ax.set_xlim(2.0, 5.4)
    ax.set_ylim(2.0, 5.4)
    ax.set_aspect("equal")
    ax.set_xlabel("x at one crossing (folded by symmetry)")
    ax.set_ylabel("x at the next crossing")
    ax.text((cs["l0"] + cs["l1"]) / 2, 5.3, "N_L", color=CAT[0], ha="center", va="top", fontsize=14, weight="bold")
    ax.text((cs["r0"] + cs["r1"]) / 2, 5.3, "N_R", color=CAT[1], ha="center", va="top", fontsize=14, weight="bold")
    ax.legend(loc="upper right", markerscale=6, fontsize=12)
    tx = fig.add_axes([0.70, 0.10, 0.28, 0.70])
    tx.axis("off")
    e = cs["edges"]
    txt = ("How to read the green bars\n\n"
           "N_L (no flip next loop):\n left edge lands in %s,\n  below N_R (starts at ≈%.5f)\n"
           " right edge lands in %s,\n  above N_R (ends at ≈%.5f)\n=> one loop stretches N_L across N_R.\n\n"
           "N_R (flip next loop):\n left edge lands in %s,\n  above N_R\n"
           " right edge lands in %s,\n  below N_L (starts at ≈%.5f)\n=> one loop stretches N_R across both.\n\n"
           "So every flip pattern with no two\n\"keep\" loops in a row is followed\nby a true motion of the dynamo."
           % (out5(*e[("N_L", "left")]), cs["r0"], out5(*e[("N_L", "right")]), cs["r1"],
              out5(*e[("N_R", "left")]), out5(*e[("N_R", "right")]), cs["l0"]))
    tx.text(0.0, 1.0, txt, va="top", ha="left", fontsize=12.5, color=TXT, linespacing=1.35, transform=tx.transAxes)
    save(fig, "rikitake-return-map-proof.svg")


def fig_flips(cs):
    _, _, _, s0 = return_map(cs, T=10.0)
    f = field(cs["mu"], cs["a"])
    t = solve_ivp(f, (0, 300), s0, method="DOP853", rtol=1e-11, atol=1e-12, max_step=0.02)
    fig = plt.figure(figsize=(12.5, 5.2))
    frame(fig)
    ax = fig.add_axes([0.085, 0.15, 0.89, 0.62])
    fig.text(0.03, 0.91, "The dynamo's field flips at irregular moments", fontsize=17, weight="bold")
    fig.text(0.03, 0.85, "Current x in the first disc over time, mu = %g, a = %g (simulation, not proved). Its sign "
             "is the field's polarity." % (cs["mu"], cs["a"]), fontsize=13, color=AXC)
    ax.axhline(0, color=AXC, lw=1.2, zorder=1)
    ax.plot(t.t, t.y[0], "-", color=ACC, lw=2, zorder=2)
    ax.set_xlabel("time (dimensionless units of the model)")
    ax.set_ylabel("current x (sign = polarity)")
    ax.set_xlim(0, 300)
    save(fig, "rikitake-polarity-flips.svg")


def fig_three(cases):
    fig = plt.figure(figsize=(12.5, 5.6))
    frame(fig)
    fig.text(0.03, 0.92, "The same proof works at all three settings", fontsize=17, weight="bold")
    fig.text(0.03, 0.865, "Return maps (dots: simulation, not proved), the proof boxes N_L and N_R (shaded) and the proved "
             "landing ranges of their edges (green bars)", fontsize=13, color=AXC)
    for i, cs in enumerate(cases):
        ax = fig.add_axes([0.075 + i * 0.315, 0.13, 0.27, 0.62])
        xn, xn1 = draw_map(ax, cs, small=True)
        lo = min(xn.min(), cs["l0"]) - 0.1
        hi = max(xn.max(), cs["edges"][("N_R", "left")][1]) + 0.1
        ax.plot([lo, hi], [lo, hi], ":", color=AXC, lw=1, zorder=1)
        ax.set_xlim(lo, hi)
        ax.set_ylim(lo, hi)
        ax.set_aspect("equal")
        ax.set_title("mu = %g, a = %g (z = %g)" % (cs["mu"], cs["a"], cs["c"]), fontsize=13.5, loc="left")
        for nm, a0, a1, col in (("N_L", cs["l0"], cs["l1"], CAT[0]), ("N_R", cs["r0"], cs["r1"], CAT[1])):
            ax.text((a0 + a1) / 2, lo + 0.03 * (hi - lo), nm, color=col, ha="center", va="bottom", fontsize=12,
                    weight="bold", zorder=6, bbox=dict(boxstyle="round,pad=0.15", fc=BG, ec="none", alpha=0.85))
        ax.set_xlabel("x at one crossing")
        if i == 0:
            ax.set_ylabel("x at the next")
        ax.locator_params(nbins=5)
    save(fig, "rikitake-three-settings.svg")


def main():
    root = os.path.abspath(sys.argv[1] if len(sys.argv) > 1 else os.path.join(os.path.dirname(__file__), ".."))
    style()
    cases = read_cases(root)
    assert len(cases) == 3
    for cs in cases:
        e = cs["edges"]   # the four covering inequalities, re-read from the log
        assert e[("N_L", "left")][1] < cs["r0"] and e[("N_L", "right")][0] > cs["r1"]
        assert e[("N_R", "left")][0] > cs["r1"] and e[("N_R", "right")][1] < cs["l0"]
    fig_hero(cases[0])
    fig_flips(cases[0])
    fig_three(cases)


if __name__ == "__main__":
    main()
