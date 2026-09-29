"""Figure for the README (illustration only; the proof is proof/check.cpp)."""
import numpy as np
from scipy.integrate import solve_ivp
import matplotlib; matplotlib.use("Agg"); import matplotlib.pyplot as plt
mu, a, c = 1.0, 2.0, 2.5
l0, l1, r0, r1 = [float(v) for v in open("proof/design_mu1_a2_c2.5.txt").read().split("\n")[2].split()[:4]]
f = lambda t, s: [-mu*s[0]+s[1]*s[2], -mu*s[1]+(s[2]-a)*s[0], 1-s[0]*s[1]]
ev = lambda t, s: s[2]-c; ev.direction = -1
s0 = solve_ivp(f, (0, 300), [1, .5, 2], rtol=1e-11, atol=1e-12).y[:, -1]
sol = solve_ivp(f, (0, 12000), s0, rtol=1e-11, atol=1e-13, events=ev, dense_output=False)
E = sol.y_events[0]; sg = np.sign(E[:, 0]); X = E[:, 0]*sg; flip = sg[1:] != sg[:-1]
fig, ax = plt.subplots(1, 2, figsize=(12, 4.8), gridspec_kw={"width_ratios": [1, 1.25]})
ax[0].scatter(X[:-1][~flip], X[1:][~flip], s=1.5, c="#2b6cb0", label="next loop: same polarity")
ax[0].scatter(X[:-1][flip], X[1:][flip], s=1.5, c="#c53030", label="next loop: polarity reversal")
for (lo, hi, col, nm) in [(l0, l1, "#2b6cb0", "$N_L$"), (r0, r1, "#c53030", "$N_R$")]:
    ax[0].axvspan(lo, hi, color=col, alpha=0.12); ax[0].text((lo+hi)/2, 2.05, nm, ha="center", color=col, fontsize=12)
    ax[0].axhspan(lo, hi, color=col, alpha=0.06)
ax[0].plot([2, 5.3], [2, 5.3], "k:", lw=0.8)
ax[0].set_xlabel("$x$ at crossing $n$ (plane $z=2.5$, folded by symmetry)"); ax[0].set_ylabel("$x$ at crossing $n+1$")
ax[0].set_title("Return map, $\mu=1,\ a=2$: the two proven h-sets"); ax[0].legend(loc="upper right", fontsize=8, markerscale=6)
t = solve_ivp(f, (0, 300), s0, rtol=1e-10, atol=1e-12, max_step=0.02)
ax[1].plot(t.t, t.y[0], lw=0.7, c="#333"); ax[1].axhline(0, c="#999", lw=0.5)
ax[1].set_xlabel("time"); ax[1].set_ylabel("current in disc 1, $x(t)$"); ax[1].set_title("Irregular polarity reversals")
fig.tight_layout(); fig.savefig("figure.png", dpi=130); print("ok", len(X))
