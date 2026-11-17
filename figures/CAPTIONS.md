# rikitake-chaos: figure captions and sources

All three figures are drawn by `figures/make_figures.py` (run: `python figures/make_figures.py .` from the repo root;
Python 3.11, numpy, scipy, matplotlib 3.11, about 1 minute). No number is typed into a plot: box ends come from
`proof/design_*.txt`, landing ranges from `proof/run_*.log`, and the dots and curves are float simulations of the
equations x' = -mu x + y z, y' = -mu y + (z - a) x, z' = 1 - x y.

## 1. `figures/rikitake-return-map-proof.svg` (hero)

**Point.** The whole proof fits in one picture: two boxes, and three proved stretches that carry each box right across
the box it leads to.

**Caption.** Each dot compares the dynamo's state at one pass through the snapshot plane z = 2.5 with its state at the
next pass (mu = 1, a = 2; dots from a simulation, not proved, folded so that a field and its reverse count as the same state).
Blue dots: the next loop keeps the field's polarity; orange dots: it reverses it. Shaded columns: the two proof boxes,
N_L (next loop keeps the polarity) and N_R (next loop reverses it). Green bars: where the edges of the boxes are proved
to land after one loop, with every rounding error counted. N_L's edges land below and above N_R, so one loop stretches
N_L right across N_R; N_R's edges land above N_R and below N_L, so it is stretched across both. That is enough to prove
that every flip pattern with no two "keep" loops in a row is followed by a true motion of the dynamo.

**Alt text.** Return-map scatter: a blue rising branch and an orange falling branch meet in a sharp peak between the two boxes. Two shaded
vertical columns mark the boxes N_L (about 2.75 to 3.02) and N_R (about 3.11 to 3.70). Four short green bars at the box
edges show proved landing ranges that clear the boxes on the correct side. A text panel explains how to read the bars.

**Sources.**
| element | file | field |
|---|---|---|
| mu = 1, a = 2, c = 2.5 | `proof/design_mu1_a2_c2.5.txt` | line 1, first three numbers |
| N_L = [2.74952, 3.01882], N_R = [3.10618, 3.69748] (≈, 5 decimals) | `proof/design_mu1_a2_c2.5.txt` | line 3: l0 l1 r0 r1 |
| four landing ranges 2.93428-3.01536, 3.74415-4.01580, 3.71240-4.19544, 2.61255-2.64086 | `proof/run_mu1_a2_c2.5.log` | lines `Q(left/right edge N_L/N_R) ... u in [lo, hi] : true`; printed rounded outward to 5 decimals (same as TECHNICAL's table) |
| dots | computed | DOP853, rtol 1e-11, start (1, 0.5, 2), 300 time units discarded, then 8,000 time units of crossings of z = 2.5 with z decreasing |
| u = x on the plane | TECHNICAL "How it is proved" | chart (u, s) = (x, y − p(x)) |

The script re-reads the four inequalities from the log and asserts each one before drawing.

## 2. `figures/rikitake-polarity-flips.svg`

**Point.** What the dynamo does: its field flips at irregular moments.

**Caption.** The current x in the first disc over 300 time units (mu = 1, a = 2; a simulation, not proved). Its sign is the
field's polarity, so every crossing of the zero line is a reversal; the gaps between reversals are irregular. This is
the behaviour the theorem explains: it proves that motions with chaotic flip patterns exist, not that this particular
simulated run is one of them.

**Alt text.** A green oscillating curve over time 0 to 300 that stays above zero for a while, then below, switching at
irregular intervals.

**Sources.** Parameters from `proof/design_mu1_a2_c2.5.txt` line 1; curve computed (DOP853, rtol 1e-11, max step 0.02,
starting from the state reached after the 300-unit transient used in figure 1).

## 3. `figures/rikitake-three-settings.svg`

**Point.** The same two-box proof works at all three parameter settings.

**Caption.** Return maps at the three proved settings, (mu, a) = (1, 2), (1, 3.75) and (2, 5), with snapshot planes
z = 2.5, 4 and 5.8 (dots: simulation, not proved). Shaded columns: the boxes N_L (blue) and N_R (orange); green bars: the proved
landing ranges of their edges. In every panel the bars clear the boxes on the required side.

**Alt text.** Three side-by-side return-map panels of the same shape, each with two shaded columns labelled N_L and N_R
and four green bars.

**Sources.**
| element | file | field |
|---|---|---|
| settings and planes | `proof/design_mu1_a2_c2.5.txt`, `proof/design_mu1_a3.75_c4.txt`, `proof/design_mu2_a5_c5.8.txt` | line 1 |
| boxes | same files | line 3 |
| green bars | `proof/run_mu1_a2_c2.5.log`, `proof/run_mu1_a3.75_c4.log`, `proof/run_mu2_a5_c5.8.log` | `Q(...) u in [lo, hi]` lines; each log also must contain `RESULT: ALL COVERING CONDITIONS VERIFIED` |

Dropped: nothing else considered; checker B's piece counts are an audit statistic, not a picture.
