#!/usr/bin/env python3
# drive_b.py -- checker B: process-parallel driver for verify_b_gen (one single-threaded process per strip/edge).
# usage (inside WSL): python3 drive_b.py SPEC TAG [nproc=12] [FRAC=1.0] [maxsecs_per_job=7000] [ngrid=64]
# Writes leaves_b_TAG.txt (concatenated leaf logs), run_b_TAG.log (merged per-run stats), then runs audit_b_gen.py.
import sys, os, subprocess, time, concurrent.futures as cf

spec, tag = sys.argv[1], sys.argv[2]
nproc = int(sys.argv[3]) if len(sys.argv) > 3 else 12
frac = sys.argv[4] if len(sys.argv) > 4 else "1.0"
maxsecs = sys.argv[5] if len(sys.argv) > 5 else "7000"
ngrid = int(sys.argv[6]) if len(sys.argv) > 6 else 64
here = os.path.dirname(os.path.abspath(__file__))
exe = os.path.join(here, "verify_b_gen")
tmp = os.path.join(here, f"tmp_{tag}")
os.makedirs(tmp, exist_ok=True)
lines = [l.rstrip() for l in open(spec)]
ns = int(lines[8].split()[1])
jobs = []
for si in range(ns):
    jobs += [(si, "lo", 0), (si, "hi", 0)]
    jobs += [(si, "set", k) for k in range(ngrid)]

def run(job):
    si, kind, k = job
    lf = os.path.join(tmp, f"leaf_{si}_{kind}_{k}.txt")
    out = subprocess.run([exe, spec, lf, "1", "strip", frac, maxsecs, str(ngrid), str(si), kind, str(k)],
                         capture_output=True, text=True)
    return job, out.returncode, out.stdout, out.stderr, lf

t0 = time.time()
stats = {}; failmsgs = {}; errors = []
header = None
with cf.ThreadPoolExecutor(max_workers=nproc) as ex:
    for job, rc, so, se, lf in ex.map(run, jobs):
        if header is None:
            header = "\n".join(l for l in so.splitlines() if not l.startswith(("STAT", "FAILMSG", "===", "  verdict")))
        st = [l for l in so.splitlines() if l.startswith("STAT ")]
        if rc != 0 or len(st) != 1:
            errors.append((job, rc, se[-500:])); continue
        t = st[0].split()
        name = t[1]
        v = dict(leaves=int(t[2]), attempts=int(t[3]), exc=int(t[4]), fails=int(t[5]), fexc=int(t[6]), fsign=int(t[7]),
                 fs=int(t[8]), fedge=int(t[9]), depth=int(t[10]), minw=float(t[11]), xmin=float(t[12]), xmax=float(t[13]),
                 smin=float(t[14]), smax=float(t[15]), sup=float(t[16]), ell=float(t[17]), secs=float(t[18]), inc=int(t[19]), n=1)
        if name not in stats: stats[name] = v
        else:
            a = stats[name]
            for key in ("leaves", "attempts", "exc", "fails", "fexc", "fsign", "fs", "fedge", "secs", "n"): a[key] += v[key]
            for key in ("depth", "xmax", "smax", "sup", "inc"): a[key] = max(a[key], v[key])
            for key in ("minw", "xmin", "smin", "ell"): a[key] = min(a[key], v[key])
        for l in so.splitlines():
            if l.startswith("FAILMSG"): failmsgs.setdefault(name, []).append(l)
wall = time.time() - t0
with open(os.path.join(here, f"leaves_b_{tag}.txt"), "w") as out:
    for si, kind, k in jobs:
        lf = os.path.join(tmp, f"leaf_{si}_{kind}_{k}.txt")
        if os.path.exists(lf):
            out.write(open(lf).read()); os.remove(lf)
try: os.rmdir(tmp)
except OSError: pass
w = float(lines[8].split()[0])
sets = [l.split() for l in lines[9:9 + ns]]
rep = [f"checker B process-parallel run: spec={spec} tag={tag} nproc={nproc} FRAC={frac} ngrid={ngrid} maxsecs/job={maxsecs}",
       header or "", ""]
allok = not errors
for si in range(ns):
    lo, hi, orient, Tlo, Thi, fold = sets[si]
    for name, desc in ((f"S{si}", "claims 1+2 on the whole set"),
                       (f"S{si}_lo", f"claim 3 edge u=lo: u(Q) {'<' if orient == '1' else '>'} {Tlo if orient == '1' else Thi}"),
                       (f"S{si}_hi", f"claim 3 edge u=hi: u(Q) {'>' if orient == '1' else '<'} {Thi if orient == '1' else Tlo}")):
        a = stats.get(name)
        if a is None:
            rep.append(f"=== {name}: MISSING (job error)"); allok = False; continue
        ok = a["fails"] == 0 and a["inc"] == 0
        allok &= ok
        rep.append(f"=== {name} ({desc}) ===")
        rep.append(f"  verdict: {'PASS' if ok else 'FAIL'}{' (INCOMPLETE)' if a['inc'] else ''}")
        rep.append(f"  jobs={a['n']} leaves={a['leaves']} attempts={a['attempts']} exceptions(->subdivided)={a['exc']} "
                   f"failed_leaves={a['fails']} [exc={a['fexc']} fold/ell={a['fsign']} s={a['fs']} edge={a['fedge']}] "
                   f"maxDepth={a['depth']} minLeafWidth={a['minw']:.3g} cpu={a['secs']:.1f}s")
        rep.append(f"  image u range = [{a['xmin']:.17g}, {a['xmax']:.17g}]")
        rep.append(f"  s' range = [{a['smin']:.6e}, {a['smax']:.6e}]  sup|s'| <= {a['sup']:.6e}  (w={w})")
        if lines[5].split()[6] == "1": rep.append(f"  min ell.Q >= {a['ell']:.6g}")
        for m in failmsgs.get(name, [])[:8]: rep.append("  " + m)
for e in errors: rep.append(f"JOB ERROR {e}")
rep.append(f"\nTOTAL leaves={sum(a['leaves'] for a in stats.values())} wall={wall:.1f}s  OVERALL={'PASS' if allok else 'FAIL'}")
open(os.path.join(here, f"run_b_{tag}.log"), "w").write("\n".join(rep) + "\n")
print("\n".join(rep))
au = subprocess.run(["python3", os.path.join(here, "audit_b_gen.py"), spec, os.path.join(here, f"leaves_b_{tag}.txt")],
                    capture_output=True, text=True)
open(os.path.join(here, f"audit_b_{tag}.log"), "w").write(au.stdout + au.stderr)
print(au.stdout + au.stderr)
