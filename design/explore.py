"""Numerical scouting for the proof (not rigorous). Rikitake: x'=-mu x+yz, y'=-mu y+(z-a)x, z'=1-xy."""
import numpy as np, sys
from scipy.integrate import solve_ivp
mu, a = float(sys.argv[1]), float(sys.argv[2])
f = lambda t, s: [-mu*s[0]+s[1]*s[2], -mu*s[1]+(s[2]-a)*s[0], 1-s[0]*s[1]]
s0 = solve_ivp(f, (0, 300), [1, .5, 2], rtol=1e-11, atol=1e-12).y[:, -1]
sol = solve_ivp(f, (0, 8000), s0, rtol=1e-11, atol=1e-12, max_step=0.01)
x, y, z = sol.y
K = np.sqrt((a/mu + np.sqrt((a/mu)**2 + 4))/2)   # a = mu(K^2 - K^-2)
print(f"mu={mu} a={a} K={K:.4f} equilibria x=+-{K:.4f} y=+-{1/K:.4f} z={mu*K*K:.4f}")
i = np.where((z[1:-1] > z[:-2]) & (z[1:-1] > z[2:]))[0] + 1
j = np.where((z[1:-1] < z[:-2]) & (z[1:-1] < z[2:]))[0] + 1
print(f"z max range {z[i].min():.3f}..{z[i].max():.3f}; z min range {z[j].min():.3f}..{z[j].max():.3f}")
zm = z[i]; sx = np.sign(x[i]); flip = sx[1:] != sx[:-1]
zc = mu*K*K
print("peaks n with z<zeq: next-flip fraction", flip[zm[:-1] < zc].mean().round(3), " z>zeq:", flip[zm[:-1] > zc].mean().round(3))
print("slopes: left", np.polyfit(zm[:-1][zm[:-1]<zc-0.1], zm[1:][zm[:-1]<zc-0.1],1)[0].round(2),
      " right", np.polyfit(zm[:-1][zm[:-1]>zc+0.1], zm[1:][zm[:-1]>zc+0.1],1)[0].round(2))
np.save(f"traj_{mu}_{a}.npy", np.vstack([sol.t, x, y, z]))
