#!/usr/bin/env bash
# Reproduce every result in this repository. Linux/WSL, g++ >= 11, cmake, git. about 3 hours in total.
set -euo pipefail
HERE="$(cd "$(dirname "$0")" && pwd)"
CAPD_COMMIT=03dc5628203334b214bb7d9fd63788a175521005
CAPD="${CAPD_DIR:-$HERE/_capd}"
if [ ! -f "$CAPD/build/libcapd.a" ]; then
  git clone https://github.com/CAPDGroup/CAPD.git "$CAPD"
  git -C "$CAPD" checkout "$CAPD_COMMIT"
  cmake -S "$CAPD" -B "$CAPD/build" -DCMAKE_BUILD_TYPE=Release
  make -C "$CAPD/build" -j"$(nproc)"
fi
CF=$(sed -n 's/^Cflags: //p' "$CAPD/build/bin/capd.pc"); LB=$(sed -n 's/^Libs: //p' "$CAPD/build/bin/capd.pc")
cd "$HERE/proof"
g++ $CF check.cpp $LB -o check
g++ $CF "$CAPD/capdDynSys/examples/RosslerChaoticDynamics/RosslerChaoticDynamics.cpp" $LB -o rossler_control
echo "== control: Rossler horseshoe (Zgliczynski 1997), every line must read true"; ./rossler_control
for d in design_mu1_a2_c2.5.txt design_mu1_a3.75_c4.txt design_mu2_a5_c5.8.txt; do
  log="run_${d#design_}"; log="${log%.txt}.log"
  echo "== checker A on $d (log: proof/$log)"
  { time ./check "$d" 40 40 3; } > "$log" 2>&1 || true
  grep -v '^  piece' "$log"
done
echo "== negative control, checker A (must end NOT VERIFIED)"
{ time ./check negctl_r1_3.40.txt 40 40 3; } > negctl.log 2>&1 || true
grep -E 'RESULT|DEFINITE' negctl.log
cd "$HERE/independent"
g++ $CF -pthread verify_b.cpp $LB -o verify_b
echo "== checker B on design_mu1_a2_c2.5.txt"
./verify_b design_mu1_a2_c2.5.txt leaves_b.txt "$(nproc)" all 1 > run_b.log 2>&1 || true
tail -2 run_b.log; python3 audit_b.py design_mu1_a2_c2.5.txt leaves_b.txt | tail -1
for d in design_mu1_a3.75_c4 design_mu2_a5_c5.8; do
  echo "== checker B on $d.txt"
  ./verify_b "$d.txt" "leaves_b_$d.txt" "$(nproc)" all 1 > "run_b_$d.log" 2>&1 || true
  tail -1 "run_b_$d.log"; python3 audit_b.py "$d.txt" "leaves_b_$d.txt" | tail -1
done
echo "== negative control, checker B (must end OVERALL=FAIL)"
./verify_b negctrl/design_NEG_r1_3.40.txt negctrl/leaves_neg.txt "$(nproc)" all 1 > negctrl/run_neg.log 2>&1 || true
tail -1 negctrl/run_neg.log
