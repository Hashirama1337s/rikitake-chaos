"""Design h-sets in the global chart (u,s) = (x, y - p(x)) on plane z=c (downward crossings), reduced by the symmetry
(x,y,z)->(-x,-y,z). N_L = {x in [l0,l1], |s|<=w}, N_R = {x in [r0,r1], |s|<=w}. Non-rigorous; writes design_<tag>.txt.
Conditions (all rigorous-checked later in CAPD):
  L: lands same polarity; f(l0-edge) < r0 ; f(l1-edge) > r1 ; |s(image)| < w
  R: lands flipped;       f(r0-edge) > r1 ; f(r1-edge) < l0 ; |s(image)| < w
=> covering relations L=>R, R=>L, R=>R (transition matrix [[0,1],[1,1]], entropy >= log golden ratio)."""
import numpy as np, sys
from scipy.integrate import solve_ivp
mu, a, c, deg = float(sys.argv[1]), float(sys.argv[2]), float(sys.argv[3]), 8
W = float(sys.argv[4]); GAP = float(sys.argv[5])
f = lambda t, s: [-mu*s[0]+s[1]*s[2], -mu*s[1]+(s[2]-a)*s[0], 1-s[0]*s[1]]
def P(x, y):
    ev = lambda t, s: s[2]-c; ev.direction = -1; ev.terminal = True
    s = solve_ivp(f, (0, 0.05), [x, y, c], rtol=1e-12, atol=1e-13).y[:, -1]
    r = solve_ivp(f, (0, 200), s, rtol=1e-12, atol=1e-13, events=ev).y_events[0][0]
    sg = np.sign(r[0]); return r[0]*sg, r[1]*sg, sg < 0
s0 = solve_ivp(f, (0, 300), [1, .5, 2], rtol=1e-11, atol=1e-12).y[:, -1]
ev = lambda t, s: s[2]-c; ev.direction = -1
E = solve_ivp(f, (0, 8000), s0, rtol=1e-11, atol=1e-13, events=ev).y_events[0][:, :2]
E = E*np.sign(E[:, :1])
xmid = E[:, 0].mean(); xsc = (E[:, 0].max()-E[:, 0].min())/2
pc = np.polyfit((E[:, 0]-xmid)/xsc, E[:, 1], deg); p = lambda x: np.polyval(pc, (x-xmid)/xsc)
res = E[:, 1]-p(E[:, 0]); print(f"attractor x {E[:,0].min():.3f}..{E[:,0].max():.3f}; poly deg {deg} residual max {np.abs(res).max():.2e}")
xs = np.linspace(E[:, 0].min()+0.01, E[:, 0].max()-0.01, 500)
F = np.array([P(x, p(x)) for x in xs]); fx = F[:, 0]
k = np.argmax(fx); xc = xs[k]; print(f"cusp x~{xc:.4f}  f max {fx[k]:.3f}")
g = lambda x: np.interp(x, xs, fx)
best = None
for m in np.arange(0.12, 0.01, -0.01):
    for r0 in np.arange(xc+GAP, xc+0.5, 0.002):
        L0 = xs[(xs < xc) & (fx < r0-m)]
        if not len(L0): continue
        l0 = L0.max()-0.002
        Rr = xs[(xs > r0) & (fx < l0-m)]
        if not len(Rr): continue
        r1 = Rr.min()+0.002
        if g(r0) <= r1+m: continue
        L1 = xs[(xs > l0) & (xs < xc-GAP) & (fx > r1+m)]
        if not len(L1): continue
        best = (m, l0, L1.min()+0.002, r0, r1); break
    if best: break
m, l0, l1, r0, r1 = best
print(f"margin {m:.2f}: L=[{l0:.4f},{l1:.4f}]  R=[{r0:.4f},{r1:.4f}]")
# image band: how far do images of the band leave the curve?
for w in [0.02, 0.05, 0.1, 0.2]:
    smax = 0
    for (x0, x1) in [(l0, l1), (r0, r1)]:
        for x in np.linspace(x0, x1, 25):
            for s in (-w, 0, w):
                X, Y, _ = P(x, p(x)+s); smax = max(smax, abs(Y-p(X)))
    print(f"  w={w}: max |s| of images = {smax:.2e}  ratio {smax/w:.3f}")
edges = {}
for nm, x in [("l0", l0), ("l1", l1), ("r0", r0), ("r1", r1)]:
    vals = [P(x, p(x)+s) for s in np.linspace(-W, W, 5)]
    edges[nm] = (min(v[0] for v in vals), max(v[0] for v in vals), set(bool(v[2]) for v in vals))
print("edge images (w=W):", {k: (round(v[0],3), round(v[1],3), v[2]) for k, v in edges.items()})
tag = f"mu{mu:g}_a{a:g}_c{c:g}"
with open(f"design_{tag}.txt", "w") as fo:
    fo.write(" ".join(repr(float(v)) for v in (mu, a, c, xmid, xsc)) + "\n")
    fo.write(f"{len(pc)} " + " ".join(repr(float(v)) for v in pc) + "\n")
    fo.write(" ".join(repr(float(v)) for v in (l0, l1, r0, r1, W)) + "\n")
print("wrote", f"design_{tag}.txt")
