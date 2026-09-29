// verify_b.cpp -- checker B: independent rigorous verification of the Rikitake
// reduced return-map claims (SPEC.md).  Written from SPEC.md only.
//
// Method (summary):
//  * Pieces are boxes [ua,ub] x [sa,sb] in chart coordinates (u,s), produced by exact
//    bisection at double midpoints (children [ua,m],[m,ub] share the endpoint m), so the
//    leaves cover the parent exactly.  Coverage is re-audited afterwards (exact rational
//    area sum / exact 1-D chaining) by audit_b.py from the leaf log.
//  * Initial set for a piece: centre um (double), du = [ua,ub]-um (interval), and
//      y = p(um) + a0*du + E + s,   E = (p'(um)-a0)*du + 0.5*p''([ua,ub])*du^2
//    (Taylor with Lagrange remainder, all in interval arithmetic; a0 is any double).
//    The CAPD tripleton  x + C*r0  with C = [[1,0,0],[a0,1,0],[0,0,1]],
//    r0 = (du, p(um)-yc + [sa,sb] + E, 0) therefore CONTAINS {(u, p(u)+s, c)}.
//  * Rigorous Poincare map (CAPD IPoincareMap, section z=c, PlusMinus = z decreasing),
//    result requested in affine coordinates aligned with the chart at the (non-rigorous)
//    image Q0 of the centre:   du' = Q_x - Q0x,  v = (Q_y - Q0y) - a1*(Q_x - Q0x),
//    with Q = sigma*P (sigma = +1 for N_L "no flip", -1 for N_R "flip").
//  * s' = Y - p(X) is enclosed by the mean-value/Taylor form
//      s' in (Q0y - p(Q0x)) + v + (a1 - p'(Q0x))*du' - 0.5*p''(X)*du'^2 ,  X = Q0x + du'
//    intersected with the naive form (Q0y + v + a1*du') - p(X).
//  * A piece passes iff CAPD certifies the transversal return (no exception), X > 0
//    (i.e. sign of P_x is as claimed), sup|s'| < w, and (edge runs) the edge inequality.
//    Otherwise it is bisected (longer side); below a minimum width it is recorded FAIL.
#include <iostream>
#include <fstream>
#include <sstream>
#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <cfenv>
#include <vector>
#include <string>
#include <thread>
#include <mutex>
#include <atomic>
#include <chrono>
#include <algorithm>
#include "capd/capdlib.h"
using namespace capd;
using namespace std;

static double FRAC = 1.0;  // refinement target: keep bisecting while sup|s'| >= FRAC*w (bounds only; claim test is < w)
static double MU, AA, CC, XMID, XSC, COEF[9], L0, L1, R0, R1, W;

// ---------- the chart polynomial, interval arithmetic ----------
static interval iXMID, iXSC, iC[9], iD1[8], iD2[7];
static void setupPoly() {
  iXMID = interval(XMID); iXSC = interval(XSC);
  for (int k = 0; k < 9; k++) iC[k] = interval(COEF[k]);                    // P(t) = sum C[k] t^(8-k)
  for (int k = 0; k < 8; k++) iD1[k] = interval(double(8 - k)) * iC[k];     // P'(t) = sum (8-k) C[k] t^(7-k)
  for (int k = 0; k < 7; k++) iD2[k] = interval(double(7 - k)) * iD1[k];    // P''(t)
}
static interval ip(const interval& u) { interval t = (u - iXMID) / iXSC, r = iC[0]; for (int k = 1; k < 9; k++) r = r * t + iC[k]; return r; }
static interval idp(const interval& u) { interval t = (u - iXMID) / iXSC, r = iD1[0]; for (int k = 1; k < 8; k++) r = r * t + iD1[k]; return r / iXSC; }
static interval iddp(const interval& u) { interval t = (u - iXMID) / iXSC, r = iD2[0]; for (int k = 1; k < 7; k++) r = r * t + iD2[k]; return r / (iXSC * iXSC); }
// non-rigorous double versions (only used for choosing centres / coordinate frames)
static double dp(double u) { double t = (u - XMID) / XSC, r = COEF[0]; for (int k = 1; k < 9; k++) r = r * t + COEF[k]; return r; }

// ---------- per-thread machinery ----------
static mutex g_mtx;
struct Engine {
  IMap ivf; IOdeSolver isolver; ICoordinateSection isec; IPoincareMap ipm;
  DMap dvf; DOdeSolver dsolver; DCoordinateSection dsec; DPoincareMap dpm;
  Engine()
    : ivf("par:mu,a;var:x,y,z;fun:-mu*x+y*z,-mu*y+(z-a)*x,1-x*y;"), isolver(ivf, 20), isec(3, 2, interval(CC)),
      ipm(isolver, isec, poincare::PlusMinus),
      dvf("par:mu,a;var:x,y,z;fun:-mu*x+y*z,-mu*y+(z-a)*x,1-x*y;"), dsolver(dvf, 20), dsec(3, 2, CC),
      dpm(dsolver, dsec, poincare::PlusMinus) {
    ivf.setParameter("mu", interval(MU)); ivf.setParameter("a", interval(AA));
    dvf.setParameter("mu", MU); dvf.setParameter("a", AA);
  }
};

struct Piece { double ua, ub, sa, sb; int depth; };
struct Result {
  bool ok = false; bool exc = false; bool definiteViolation = false; string why;
  interval X, S;   // enclosures of X = Q_x and s' = Q_y - p(Q_x)
};

// kind: 0 = 2-D set; 1 = edge (u fixed). sigma: +1 (N_L, no flip) / -1 (N_R, flip)
// edgeTest: 0 none, 1: X < bound, 2: X > bound
static Result evalPiece(Engine& E, const Piece& pc, int sigma, int edgeTest, double bound) {
  Result R;
  try {
    // ---- initial set containing {(u, p(u)+s, c) : u in [ua,ub], s in [sa,sb]} ----
    double um = (pc.ua == pc.ub) ? pc.ua : pc.ua + 0.5 * (pc.ub - pc.ua);
    interval U(pc.ua, pc.ub), iu(um);
    interval du = U - iu;                              // contains u - um
    interval pum = ip(iu), dpum = idp(iu);
    double a0 = dpum.mid().leftBound();
    interval Erem = (dpum - interval(a0)) * du + interval(0.5) * iddp(U) * sqr(du);
    if (pc.ua == pc.ub) Erem = interval(0.0);           // du is exactly 0
    interval ys = pum + interval(pc.sa, pc.sb) + Erem; // y - a0*du
    double yc = ys.mid().leftBound();
    IVector x0c(3); x0c[0] = iu; x0c[1] = interval(yc); x0c[2] = interval(CC);
    IMatrix Cm(3, 3); Cm[0][0] = 1.; Cm[1][0] = interval(a0); Cm[1][1] = 1.; Cm[2][2] = 1.;
    IVector r0(3); r0[0] = du; r0[1] = ys - interval(yc); r0[2] = interval(0.0);
    // ---- non-rigorous image of the centre: defines the affine frame only ----
    DVector dc(3); dc[0] = um; dc[1] = dp(um) + 0.5 * (pc.sa + pc.sb); dc[2] = CC;
    double tt; DVector P0 = E.dpm(dc, tt);
    double Q0x = sigma * P0[0], Q0y = sigma * P0[1];
    double a1 = idp(interval(Q0x)).mid().leftBound();
    IVector xref(3); xref[0] = interval(P0[0]); xref[1] = interval(P0[1]); xref[2] = interval(CC);
    IMatrix A(3, 3);
    A[0][0] = interval(double(sigma));
    A[1][0] = interval(-sigma * a1); A[1][1] = interval(double(sigma));
    A[2][2] = 1.;
    // ---- rigorous Poincare map ----
    C0HOTripletonSet set(x0c, Cm, r0);
    interval T;
    IVector y = E.ipm(set, xref, A, T);
    interval dX = y[0], v = y[1];
    interval iQ0x(Q0x), iQ0y(Q0y), ia1(a1);
    interval X = iQ0x + dX;
    interval S1 = (iQ0y - ip(iQ0x)) + v + (ia1 - idp(iQ0x)) * dX - interval(0.5) * iddp(X) * sqr(dX);
    interval S2 = (iQ0y + v + ia1 * dX) - ip(X);
    interval S;
    if (!intersection(S1, S2, S)) { R.why = "empty intersection (bug?)"; return R; }
    R.X = X; R.S = S;
    bool ok = true;
    if (!(X.leftBound() > 0.0)) { ok = false; R.why += "sign(P_x) not certified; "; }
    if (!(S.leftBound() > -W && S.rightBound() < W)) { ok = false; R.why += "|s'|<w not certified; "; }
    if (edgeTest == 1 && !(X.rightBound() < bound)) { ok = false; R.why += "edge X<bound not certified; "; }
    if (edgeTest == 2 && !(X.leftBound() > bound)) { ok = false; R.why += "edge X>bound not certified; "; }
    // a claim is DEFINITELY violated on this piece if the whole enclosure lies on the wrong side
    // (the true image point of any point of the piece lies in the enclosure)
    if (X.rightBound() <= 0.0) { R.definiteViolation = true; R.why += "[definite: P_x has the wrong sign] "; }
    if (S.leftBound() >= W || S.rightBound() <= -W) { R.definiteViolation = true; R.why += "[definite: |s'|>=w] "; }
    if (edgeTest == 1 && X.leftBound() >= bound) { R.definiteViolation = true; R.why += "[definite: X>=bound] "; }
    if (edgeTest == 2 && X.rightBound() <= bound) { R.definiteViolation = true; R.why += "[definite: X<=bound] "; }
    R.ok = ok;
  } catch (std::exception& e) {
    R.exc = true; R.why = string("exception: ") + string(e.what()).substr(0, 80);
  }
  return R;
}

struct RunStats {
  string name; long leaves = 0, attempts = 0, exceptions = 0, fails = 0; int maxDepth = 0;
  double supAbsS = 0, Smin = 1e300, Smax = -1e300, Xmin = 1e300, Xmax = -1e300, minWidth = 1e300;
  double secs = 0; vector<string> failMsgs;
};

static RunStats runSet(const string& name, int nthreads, vector<Piece> init, int sigma, int edgeTest,
                       double bound, double minW, FILE* leafLog) {
  RunStats st; st.name = name;
  auto t0 = chrono::steady_clock::now();
  vector<Piece> stack = init;
  mutex m; int busy = 0;
  auto worker = [&]() {
    Engine* E;
    { lock_guard<mutex> lk(g_mtx); E = new Engine(); }
    while (true) {
      Piece pc;
      {
        unique_lock<mutex> lk(m);
        while (stack.empty()) {
          if (busy == 0) { delete E; return; }
          lk.unlock(); this_thread::sleep_for(chrono::milliseconds(5)); lk.lock();
        }
        pc = stack.back(); stack.pop_back(); busy++;
      }
      Result r = evalPiece(*E, pc, sigma, edgeTest, bound);
      lock_guard<mutex> lk(m);
      busy--; st.attempts++;
      if (r.exc) st.exceptions++;
      bool refine = false;
      if (r.ok && FRAC < 1.0) {
        double a = std::max(fabs(r.S.leftBound()), fabs(r.S.rightBound()));
        double wmax = std::max(pc.ub - pc.ua, pc.sb - pc.sa);
        if (a >= FRAC * W && wmax >= 1e-6) refine = true;   // certified already; subdivide only to tighten the reported bound
      }
      if (r.ok && !refine) {
        st.leaves++;
        st.maxDepth = std::max(st.maxDepth, pc.depth);
        double a = std::max(fabs(r.S.leftBound()), fabs(r.S.rightBound()));
        st.supAbsS = std::max(st.supAbsS, a);
        st.Smin = std::min(st.Smin, r.S.leftBound()); st.Smax = std::max(st.Smax, r.S.rightBound());
        st.Xmin = std::min(st.Xmin, r.X.leftBound()); st.Xmax = std::max(st.Xmax, r.X.rightBound());
        st.minWidth = std::min(st.minWidth, std::max(pc.ub - pc.ua, pc.sb - pc.sa));
        fprintf(leafLog, "%s %a %a %a %a OK X=[%.17g,%.17g] S=[%.6e,%.6e]\n", name.c_str(), pc.ua, pc.ub, pc.sa, pc.sb,
                r.X.leftBound(), r.X.rightBound(), r.S.leftBound(), r.S.rightBound());
      } else {
        double wu = pc.ub - pc.ua, ws = pc.sb - pc.sa;
        bool splitU = wu >= ws;
        double lo = splitU ? pc.ua : pc.sa, hi = splitU ? pc.ub : pc.sb;
        double mid = lo + 0.5 * (hi - lo);
        if (!r.ok && (r.definiteViolation || st.attempts > 3000000 || std::max(wu, ws) < minW || !(lo < mid && mid < hi))) {
          st.fails++;
          char buf[512];
          snprintf(buf, sizeof buf, "FAIL piece u=[%.17g,%.17g] s=[%.17g,%.17g]: %s X=[%.6g,%.6g] S=[%.6g,%.6g]", pc.ua, pc.ub,
                   pc.sa, pc.sb, r.why.c_str(), r.X.leftBound(), r.X.rightBound(), r.S.leftBound(), r.S.rightBound());
          if (st.failMsgs.size() < 20) st.failMsgs.push_back(buf);
          fprintf(leafLog, "%s %a %a %a %a FAIL %s\n", name.c_str(), pc.ua, pc.ub, pc.sa, pc.sb, r.why.c_str());
          continue;
        }
        Piece a = pc, b = pc; a.depth = b.depth = pc.depth + 1;
        if (splitU) { a.ub = mid; b.ua = mid; } else { a.sb = mid; b.sa = mid; }
        stack.push_back(a); stack.push_back(b);
      }
    }
  };
  vector<thread> th;
  for (int i = 0; i < nthreads; i++) th.emplace_back(worker);
  for (auto& t : th) t.join();
  st.secs = chrono::duration<double>(chrono::steady_clock::now() - t0).count();
  fflush(leafLog);
  return st;
}

static void report(const RunStats& s, const string& extra) {
  printf("\n=== %s ===\n", s.name.c_str());
  printf("  verdict: %s\n", s.fails == 0 ? "PASS" : "FAIL");
  printf("  leaves(certified)=%ld attempts=%ld exceptions(->subdivided)=%ld failed_leaves=%ld maxDepth=%d minLeafWidth=%.3g time=%.1fs\n",
         s.leaves, s.attempts, s.exceptions, s.fails, s.maxDepth, s.minWidth, s.secs);
  printf("  image X range (union of enclosures) = [%.17g, %.17g]\n", s.Xmin, s.Xmax);
  printf("  s' = Y-p(X) enclosure range = [%.6e, %.6e]   sup|s'| <= %.6e  (w=%.17g)\n", s.Smin, s.Smax, s.supAbsS, W);
  if (!extra.empty()) printf("  %s\n", extra.c_str());
  for (auto& m : s.failMsgs) printf("  %s\n", m.c_str());
  fflush(stdout);
}

int main(int argc, char** argv) {
  fesetround(FE_TONEAREST);           // parse the design numbers under round-to-nearest (strtod honours the mode)
  if (argc < 3) { fprintf(stderr, "usage: verify_b design.txt leaflog [threads] [only]\n"); return 2; }
  {
    ifstream f(argv[1]); string l1, l2, l3; getline(f, l1); getline(f, l2); getline(f, l3);
    auto rd = [](const string& s) { vector<double> v; istringstream is(s); string tok; while (is >> tok) v.push_back(strtod(tok.c_str(), nullptr)); return v; };
    vector<double> a = rd(l1), b = rd(l2), c = rd(l3);
    if (a.size() != 5 || b.size() != 10 || c.size() != 5 || b[0] != 9.0) { fprintf(stderr, "bad design file\n"); return 2; }
    MU = a[0]; AA = a[1]; CC = a[2]; XMID = a[3]; XSC = a[4];
    for (int k = 0; k < 9; k++) COEF[k] = b[k + 1];
    L0 = c[0]; L1 = c[1]; R0 = c[2]; R1 = c[3]; W = c[4];
  }
  int nth = argc > 3 ? atoi(argv[3]) : 12;
  string only = argc > 4 ? argv[4] : "all";
  if (argc > 5) FRAC = atof(argv[5]);
  printf("refinement target FRAC=%g (bisect certified pieces while sup|s'| >= FRAC*w, down to width 1e-6)\n", FRAC);
  printf("checker B -- design %s\n", argv[1]);
  printf("parsed (hex): mu=%a a=%a c=%a XMID=%a XSC=%a\n", MU, AA, CC, XMID, XSC);
  printf("  C = "); for (int k = 0; k < 9; k++) printf("%a ", COEF[k]); printf("\n");
  printf("  l0=%a l1=%a r0=%a r1=%a w=%a\n", L0, L1, R0, R1, W);
  if (!(MU == 1.0 && AA == 2.0)) { printf("WARNING: mu,a not 1,2\n"); }
  setupPoly();
  cout.precision(17);
  // sanity: the section is crossed with z decreasing on the initial sets: z' = 1 - x*y < 0
  for (int side = 0; side < 2; side++) {
    interval U = side ? interval(R0, R1) : interval(L0, L1);
    interval zdot = interval(1.0) - U * (ip(U) + interval(-W, W));
    printf("  z' = 1 - x y on %s (crude hull) = [%.6g, %.6g]\n", side ? "N_R" : "N_L", zdot.leftBound(), zdot.rightBound());
  }
  FILE* leaf = fopen(argv[2], "w");
  auto T0 = chrono::steady_clock::now();
  const double minW = 1e-9;
  auto grid2 = [](double a, double b, int n) {
    vector<Piece> v; vector<double> g(n + 1);
    for (int i = 0; i <= n; i++) g[i] = a + (b - a) * (double(i) / n);
    g[0] = a; g[n] = b;
    for (int i = 0; i < n; i++) { if (!(g[i] < g[i + 1])) { fprintf(stderr, "grid not monotone\n"); exit(3); } v.push_back({g[i], g[i + 1], -W, W, 0}); }
    return v;
  };
  vector<RunStats> all;
  bool okAll = true;
  if (only == "all" || only == "sets") {
    RunStats sL = runSet("N_L", nth, grid2(L0, L1, 32), +1, 0, 0, minW, leaf);
    report(sL, "claim1(N_L): no exception & X>0 with sigma=+1 => every point returns transversally and P_x>0 (no flip)");
    RunStats sR = runSet("N_R", nth, grid2(R0, R1, 64), -1, 0, 0, minW, leaf);
    report(sR, "claim1(N_R): no exception & X>0 with sigma=-1 => every point returns transversally and P_x<0 (flip)");
    all.push_back(sL); all.push_back(sR);
  }
  if (only == "all" || only == "edges") {
    RunStats e1 = runSet("edge_l0", nth, {{L0, L0, -W, W, 0}}, +1, 1, R0, minW, leaf);
    report(e1, "claim3: image of {l0}x[-w,w] has X < r0 = " + to_string(R0));
    RunStats e2 = runSet("edge_l1", nth, {{L1, L1, -W, W, 0}}, +1, 2, R1, minW, leaf);
    report(e2, "claim3: image of {l1}x[-w,w] has X > r1 = " + to_string(R1));
    RunStats e3 = runSet("edge_r0", nth, {{R0, R0, -W, W, 0}}, -1, 2, R1, minW, leaf);
    report(e3, "claim3: image of {r0}x[-w,w] has X > r1 = " + to_string(R1));
    RunStats e4 = runSet("edge_r1", nth, {{R1, R1, -W, W, 0}}, -1, 1, L0, minW, leaf);
    report(e4, "claim3: image of {r1}x[-w,w] has X < l0 = " + to_string(L0));
    all.push_back(e1); all.push_back(e2); all.push_back(e3); all.push_back(e4);
  }
  fclose(leaf);
  long leaves = 0; for (auto& s : all) { leaves += s.leaves; okAll = okAll && s.fails == 0; }
  double secs = chrono::duration<double>(chrono::steady_clock::now() - T0).count();
  printf("\nTOTAL certified leaves=%ld wall=%.1fs threads=%d  OVERALL=%s\n", leaves, secs, nth, okAll ? "PASS" : "FAIL");
  return okAll ? 0 : 1;
}
