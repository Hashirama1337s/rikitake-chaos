# Independent verification spec (checker B)

## System
Rikitake two-disc dynamo: x' = -mu*x + y*z,  y' = -mu*y + (z - a)*x,  z' = 1 - x*y,  with mu = 1, a = 2 (exact).

## Map
Section Sigma = plane z = c, c = 2.5 (exact); crossings with z DECREASING (z - c goes from + to -).
P = first return map to Sigma (after leaving Sigma), points written as (x, y).
Symmetry S(x,y,z) = (-x,-y,z) maps solutions to solutions.
Reduced map Q(x,y) = P(x,y) if P(x,y) has x > 0 ("no flip"); Q(x,y) = -P(x,y) if P(x,y) has x < 0 ("flip").

## Chart
p(u) = sum_{k=0}^{8} C[k] * t^(8-k), t = (u - XMID)/XSC  (Horner order, C[0] is the leading coefficient).
Chart: (u, s) = (x, y - p(x)). Numbers are in design_mu1_a2_c2.5.txt:
  line 1: mu a c XMID XSC ; line 2: 9 C[0..8] ; line 3: l0 l1 r0 r1 w
Treat every decimal in that file as the exact IEEE double it parses to.

## Sets (in chart coordinates)
N_L = [l0,l1] x [-w,w],  N_R = [r0,r1] x [-w,w]; the actual sets are {(u, p(u)+s)} on Sigma.

## Claims to verify RIGOROUSLY (interval arithmetic / validated ODE integration; CAPD is installed in WSL at ~/capd_src,
## build flags in ~/capd_src/build/bin/capd.pc: use the Cflags and Libs lines)
1. Q is defined on all of N_L and N_R (every point returns transversally), N_L never flips, N_R always flips.
2. For every point of N_L and of N_R, the image Q(point) = (X, Y) satisfies |Y - p(X)| < w.
3. Edges: image of {l0} x [-w,w] has X < r0 ;  image of {l1} x [-w,w] has X > r1 ;
          image of {r0} x [-w,w] has X > r1 ;  image of {r1} x [-w,w] has X < l0.
Report PASS/FAIL for each of the 3 claims (and each edge), with the rigorous bounds you obtained, and wall time.
