# Checker B (written independently)

Checker B was written from `SPEC.md` and `design_mu1_a2_c2.5.txt` alone, without access to checker A. It uses its
own subdivision (bisect the longer side, multi-threaded), its own enclosure of s = Y − p(X) (intersection of a
mean-value/Taylor form and the direct form), and an exact coverage audit (`audit_b.py`) that proves, on the exact
doubles, that the certified pieces tile every set and edge with no gaps.

| Run (all rows (1, 2) unless stated) | Pieces | Wall time (10–12 threads) | Result | Coverage audit |
|---|---|---|---|---|
| primary (accept when \|s\| < w) | 44,166 | 1 min 44 s | PASS (`run_b.log`) | PASS (`audit_b.log`) |
| tightened (refine until \|s\| < 0.3 w) | 644,383 | 28 min 26 s | PASS (`run_b_tight.log`) | PASS (`audit_b_tight.log`) |
| (1, 3.75), primary | 164,809 | 4 min 53 s | PASS (`run_b_design_mu1_a3.75_c4.log`) | PASS |
| (2, 5), primary | 323,590 | 11 min 22 s | PASS (`run_b_design_mu2_a5_c5.8.log`) | PASS |
| negative control (r1 = 3.40) | 43,264 | 1 min 28 s | FAIL, as required (`negctrl/run_neg.log`) | FAIL |

The tightened run shows the band condition holds with margin: every image satisfies |s| ≤ 3.0e-3 against w = 0.01.
Proven edge images (primary run): l0 → X ∈ [2.95685, 2.99457] < r0; l1 → X ∈ [3.76850, 3.93228] > r1;
r0 → X ∈ [3.85138, 4.05378] > r1; r1 → X ∈ [2.61471, 2.63854] < l0.

Build: `g++ <Cflags from capd.pc> -pthread verify_b.cpp <Libs from capd.pc> -o verify_b`.
Run: `./verify_b design_mu1_a2_c2.5.txt leaves_b.txt 12 all 1` (last argument 0.3 for the tightened run),
then `python3 audit_b.py design_mu1_a2_c2.5.txt leaves_b.txt`.
