#!/usr/bin/env bash
# Reproduce every result in this repository (v1.1). Linux/WSL, g++ >= 11, cmake, git, python3. About 3 hours in total
# (checker A is single-threaded; checker B runs one single-threaded process per strip).
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
echo "CAPD commit: $(git -C "$CAPD" rev-parse HEAD)"
CF=$(sed -n 's/^Cflags: //p' "$CAPD/build/bin/capd.pc"); LB=$(sed -n 's/^Libs: //p' "$CAPD/build/bin/capd.pc")
echo "sha256 of the proof inputs:"; (cd "$HERE" && sha256sum proof/check.cpp proof/design_*.txt independent/verify_b_gen.cpp independent/*.spec)

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
g++ $CF -pthread verify_b_gen.cpp $LB -o verify_b_gen
for d in mu1_a2_c2.5 mu1_a3.75_c4 mu2_a5_c5.8; do
  echo "== checker B on rikitake_published_$d.spec (logs: independent/run_b_pub_$d.log, audit_b_pub_$d.log)"
  python3 drive_b.py "rikitake_published_$d.spec" "pub_$d" "$(nproc)" > /dev/null 2>&1 || true
  tail -1 "run_b_pub_$d.log"; tail -1 "audit_b_pub_$d.log"
  gzip -f "leaves_b_pub_$d.txt"
done
echo "== negative control, checker B (must end OVERALL=FAIL)"
python3 drive_b.py negctrl/rikitake_NEG_r1_3.40.spec NEG_r1_3.40 "$(nproc)" > /dev/null 2>&1 || true
mv run_b_NEG_r1_3.40.log audit_b_NEG_r1_3.40.log negctrl/; gzip -c leaves_b_NEG_r1_3.40.txt > negctrl/leaves_b_NEG_r1_3.40.txt.gz; rm leaves_b_NEG_r1_3.40.txt
tail -1 negctrl/run_b_NEG_r1_3.40.log
