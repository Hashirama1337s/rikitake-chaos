# Generic covering-relation spec (checker B)

A `.spec` file describes a claimed set of covering relations for a Poincare return map of a 3-D autonomous ODE.
Every decimal is the exact IEEE double it parses to (round-to-nearest).

Lines:
1. variable names (space separated), e.g. `x y z`
2. parameter names (space separated; may be empty)
3. parameter values (same order)
4. vector field components, comma separated, in CAPD formula syntax using those names (e.g. `y,z,-x+y*y-mu*z`)
5. `k c dir n`: section Sigma = {X_k = c} (k = 0,1,2); dir = +1 means crossings with X_k increasing, -1 decreasing;
   n = the iterate: P is the first return map to Sigma in that direction, and the map studied is P^n.
6. `D0 D1 D2 l0 l1 l2 sym`: if sym = 1, D = diag(D0,D1,D2) is an involution commuting with the flow, and
   Q = D^f o P^n with f in {0,1} chosen so that ell . Q > 0 where ell = (l0,l1,l2); if sym = 0, Q = P^n.
7. `eu0 eu1 eu2 ev0 ev1 ev2`: points of Sigma are parametrised X = origin + u*eu + v*ev, origin = c*e_k
   (eu, ev lie in the plane; they need not be exactly orthonormal: (u,v) of a point of Sigma is obtained by solving
   this 2x2 linear system).
8. `xmid xsc m C[0] ... C[m-1]`: polynomial p(u) = sum_j C[j] t^(m-1-j), t = (u - xmid)/xsc (Horner order).
9. `w nsets`, followed by nsets lines `lo hi orient Tlo Thi fold`.

Chart: (u, s) with s = v - p(u). Set i is N_i = {lo <= u <= hi, |s| <= w} (the true curved set on Sigma).

Claims to verify rigorously for every set i:
1. Q is defined on all of N_i (every point returns transversally n times); if sym = 1 the fold f is the same value
   `fold` on all of N_i and ell . Q > 0 there.
2. |s(Q(x))| < w for every x in N_i.
3. orient = +1: u(Q(x)) < Tlo on the edge u = lo and u(Q(x)) > Thi on the edge u = hi;
   orient = -1: u(Q(x)) > Thi on the edge u = lo and u(Q(x)) < Tlo on the edge u = hi   (edges include all |s| <= w).
Report PASS/FAIL per set and per claim, the rigorous bounds (sup |s| of images, edge image u-ranges), pieces and time.

In this repository: rikitake_published_*.spec are the three designs of proof/design_*.txt written in this format (same
chart polynomial and sets); negctrl/rikitake_NEG_r1_3.40.spec is the negative control (r1 moved to 3.40).
