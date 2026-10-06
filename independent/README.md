# Checker B (written independently)

## Independence
Checker B was written from the mathematical specification only (`SPEC.md`, a generic format for covering relations
of a Poincaré return map), without access to checker A. Its design choices differ from checker A's:
- its own subdivision;
- its own chart handling;
- its own enclosure of s = Y − p(X), which intersects a Taylor form (Lagrange point on the hull of the expansion
  point and the image) with the direct form;
- an exact coverage audit (`audit_b_gen.py`), which proves on the exact doubles that the certified pieces tile every
  set and edge with no gaps.

The three designs of this repository are written in that format in `rikitake_published_*.spec` (same chart
polynomial and sets as `../proof/design_*.txt`).

## Single-threaded by design
CAPD is not thread-safe, so every CAPD computation runs in its own single-threaded process. `drive_b.py` starts one
process per strip or edge, in parallel, and merges their results.

## Version 1.1
This replaces the version-1.0 checker B. That version had the same Lagrange-remainder slip as checker A (see the main
README, "Correction") and ran CAPD in several threads of one process.

## Results
Filled in from the clean-room run (`../reproduce_full.log`); per-case logs are `run_b_pub_*.log` and
`audit_b_pub_*.log`.

| spec | pieces | wall time (6 processes) | result | coverage audit |
|---|---|---|---|---|
| rikitake_published_mu1_a2_c2.5 | 44,166 | 127 s | OVERALL=PASS | PASS |
| rikitake_published_mu1_a3.75_c4 | 164,809 | 405 s | OVERALL=PASS | PASS |
| rikitake_published_mu2_a5_c5.8 | 323,590 | 1,123 s | OVERALL=PASS | PASS |
| negctrl/rikitake_NEG_r1_3.40 (r1 moved to 3.40) | 43,264 | 85 s | OVERALL=FAIL (as required) | n/a |

## Build and run

Build (as in `../reproduce.sh`):

```
g++ <Cflags from capd.pc> -pthread verify_b_gen.cpp <Libs from capd.pc> -o verify_b_gen
```

Run:

```
python3 drive_b.py rikitake_published_mu1_a2_c2.5.spec pub_mu1_a2_c2.5 $(nproc)
```

This writes:
- `run_b_<tag>.log` and `audit_b_<tag>.log`;
- `leaves_b_<tag>.txt`, the certified pieces, which are gzipped in this repository.
