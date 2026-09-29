# audit_b.py -- checker B: exact coverage audit of the certified leaves written by verify_b.
# Leaves are read as exact doubles (hex floats).  For every run:
#   * any FAIL leaf -> report it;
#   * 2-D sets: sweep over all distinct u-breakpoints; each elementary u-strip must be chain-covered in s
#     by the boxes spanning it (exact).  Exact rational area sum is also printed (equality => no overlaps).
#   * edges: 1-D exact chaining cover of [-w,w] at the fixed u.
import sys
from fractions import Fraction as F

design = open(sys.argv[1]).read().split()
leaf = sys.argv[2]
vals = [float(x) for x in design]
l0, l1, r0, r1, w = vals[-5:]
target = {'N_L': (l0, l1, -w, w), 'N_R': (r0, r1, -w, w),
          'edge_l0': (l0, l0, -w, w), 'edge_l1': (l1, l1, -w, w),
          'edge_r0': (r0, r0, -w, w), 'edge_r1': (r1, r1, -w, w)}
runs = {}
fails = {}
for line in open(leaf):
    t = line.split()
    name = t[0]
    box = tuple(float.fromhex(x) for x in t[1:5])
    if t[5] == 'OK':
        runs.setdefault(name, []).append(box)
    else:
        fails.setdefault(name, []).append(line.strip())

allok = True
for name, tgt in target.items():
    boxes = runs.get(name, [])
    nf = len(fails.get(name, []))
    if name.startswith('edge'):
        ok = all(b[0] == tgt[0] and b[1] == tgt[1] for b in boxes)
        iv = sorted((b[2], b[3]) for b in boxes)
        cur = tgt[2]; gap = None
        for a, b in iv:
            if a > cur: gap = (cur, a); break
            cur = max(cur, b)
        cov = ok and gap is None and cur >= tgt[3]
        print(f"{name}: leaves={len(boxes)} fails={nf} cover_exact={cov} gap={gap}")
    else:
        area = sum((F(b[1]) - F(b[0])) * (F(b[3]) - F(b[2])) for b in boxes)
        tarea = (F(tgt[1]) - F(tgt[0])) * (F(tgt[3]) - F(tgt[2]))
        inside = all(tgt[0] <= b[0] < b[1] <= tgt[1] and tgt[2] <= b[2] < b[3] <= tgt[3] for b in boxes)
        # sweep over the distinct u-breakpoints: on every elementary strip [x_i,x_{i+1}] the s-intervals of the
        # boxes whose u-range contains the strip must chain-cover [-w,w] (exact float comparisons)
        xs = sorted(set([b[0] for b in boxes] + [b[1] for b in boxes]))
        adds = {}; rems = {}
        for k, b in enumerate(boxes):
            adds.setdefault(b[0], []).append(k); rems.setdefault(b[1], []).append(k)
        active = set(); bad = None
        for i in range(len(xs) - 1):
            for k in rems.get(xs[i], []): active.discard(k)
            for k in adds.get(xs[i], []): active.add(k)
            iv = sorted((boxes[k][2], boxes[k][3]) for k in active)
            cur = tgt[2]; ok1 = True
            for a, b in iv:
                if a > cur: ok1 = False; break
                cur = max(cur, b)
            if not ok1 or cur < tgt[3]:
                bad = (xs[i], xs[i + 1]); break
        cov = inside and bad is None and xs[0] == tgt[0] and xs[-1] == tgt[1]
        print(f"{name}: leaves={len(boxes)} fails={nf} inside={inside} strips={len(xs)-1} strip_cover_exact={cov} "
              f"first_bad_strip={bad} area_sum==area: {area == tarea}")
    allok = allok and cov and nf == 0
    for m in fails.get(name, [])[:5]: print('   ', m)
print('COVERAGE AUDIT:', 'PASS' if allok else 'FAIL')
