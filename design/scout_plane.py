"""Scout: reduced return map on plane z=c (downward crossings), symmetry (x,y,z)->(-x,-y,z). Not rigorous."""
import numpy as np, sys
from scipy.integrate import solve_ivp
import matplotlib; matplotlib.use("Agg"); import matplotlib.pyplot as plt
mu, a, c = float(sys.argv[1]), float(sys.argv[2]), float(sys.argv[3])
f = lambda t, s: [-mu*s[0]+s[1]*s[2], -mu*s[1]+(s[2]-a)*s[0], 1-s[0]*s[1]]
ev = lambda t, s: s[2]-c; ev.direction = -1
s0 = solve_ivp(f, (0, 300), [1, .5, 2], rtol=1e-11, atol=1e-12).y[:, -1]
sol = solve_ivp(f, (0, 20000), s0, rtol=1e-11, atol=1e-13, events=ev)
P = sol.y_events[0]; sg = np.sign(P[:, 0]); R = P[:, :2]*sg[:, None]   # reduced: fold to x>0
flip = sg[1:] != sg[:-1]
x0, x1 = R[:-1, 0], R[1:, 0]
print(f"mu={mu} a={a} c={c}: {len(R)} crossings; x range {R[:,0].min():.3f}..{R[:,0].max():.3f}; y range {R[:,1].min():.3f}..{R[:,1].max():.3f}")
# thickness of curve: fit y as function of x
o = np.argsort(R[:,0]); print("  curve y(x) residual std:", np.std(R[o,1]-np.convolve(R[o,1],np.ones(9)/9,'same'))[()].round(5))
# which x values lead to a flip
for q in np.linspace(R[:,0].min(), R[:,0].max(), 13)[:-1]:
    m = (x0 >= q) & (x0 < q + (R[:,0].max()-R[:,0].min())/12)
    if m.sum(): print(f"  x in [{q:.3f},..): n={m.sum():5d} flip frac={flip[m].mean():.2f}  next x {x1[m].min():.3f}..{x1[m].max():.3f}")
fig, ax = plt.subplots(1, 2, figsize=(11, 4.5))
ax[0].plot(R[:,0], R[:,1], '.', ms=1); ax[0].set_title(f"section z={c} (folded)"); ax[0].set_xlabel('x'); ax[0].set_ylabel('y')
ax[1].scatter(x0, x1, s=1, c=flip, cmap='coolwarm'); ax[1].set_title("x_n -> x_n+1 (red = polarity flip)")
fig.tight_layout(); fig.savefig(f"scout_{mu}_{a}_{c}.png", dpi=100)
