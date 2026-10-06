// verify_b_gen.cpp -- checker B: generic rigorous verifier for covering-relation .spec files
// (SPEC_GENERIC.md).  Generalised from checker B's own Rikitake verifier (verify_b.cpp).
//
// Method:
//  * Pieces: boxes [ua,ub]x[sa,sb] in chart coords (u,s), s = v - p(u); exact bisection at double midpoints
//    (children share the midpoint), coverage re-audited exactly by audit_b_gen.py from the leaf log.
//  * Initial set: X = origin + u*eu + v*ev, u = um+du, v = p(u)+s.  With Taylor/Lagrange
//      p(u) = p(um) + a0*du + E,  E in (p'(um)-a0)*du + 0.5*p''([ua,ub])*du^2   (interval arithmetic)
//    X = xc + du*g + ry*ev + du*(eu + a0*ev - g), g = double(eu + a0*ev), ry in p(um)+[sa,sb]+E-yc.
//    Represented as CAPD tripleton  xm + C*r0 + r,  C = [g | ev | e_k], r0 = (du, ry, 0),
//    r = (xc - xm) + du*(eu+a0*ev-g): CONTAINS the true curved piece.
//  * Rigorous P^n: CAPD IPoincareMap, ICoordinateSection(3,k,c), direction from dir, iterate n,
//    result requested in affine coords A*(P^n - P0), A = A_Q * D^f, A_Q rows: alpha (u-functional),
//    beta - a1*alpha (v tilted by the chart slope at the image), e_k.  (u,v) = Ginv*(X_i,X_j), Ginv interval.
//    P0 = non-rigorous P^n of the piece centre -- used only to choose the frame.
//  * s' = v(Q) - p(u(Q)):   X = U0 + du',  V = V0 + v' + a1*du'   (U0 = alpha.Q0, V0 = beta.Q0, intervals)
//      S1 = (V0 - p(U0)) + v' + (a1 - p'(U0))*du' - 0.5*p''(hull(U0,X))*du'^2   (Taylor about u(Q0) in U0)
//      S2 = V - p(X);   S = S1 /\ S2.
//  * ell.Q (sym=1) = ell_k*c + (ell.eu)*X + (ell.ev)*V   (Q lies on Sigma).
//  * Piece passes iff: no CAPD exception (transversal n-fold return certified), [sym: ell.Q > 0 with the
//    claimed fold f], sup|s'| < w, [edge runs: the edge inequality].  Else bisect the longer side.
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

// ------------------------------- spec -------------------------------
struct SetSpec { double lo, hi; int orient; double Tlo, Thi; int fold; };
static vector<string> VARS, PNAMES; static vector<double> PVALS; static string VF;
static int K, DIR, NIT, SYM; static double CSEC, Dg[3], ELL[3], EU[3], EV[3], XMID, XSC, W; static vector<double> COEF;
static vector<SetSpec> SETS;
static double FRAC = 1.0, MAXSECS = 7200;

static vector<string> toks(const string& s) { vector<string> v; istringstream is(s); string t; while (is >> t) v.push_back(t); return v; }
static double D(const string& s) { char* e; double x = strtod(s.c_str(), &e); if (*e) { fprintf(stderr, "bad number %s\n", s.c_str()); exit(2); } return x; }

static void readSpec(const char* fn) {
  ifstream f(fn); if (!f) { fprintf(stderr, "cannot open %s\n", fn); exit(2); }
  vector<string> L; string l;
  while (getline(f, l)) { if (!l.empty() && l.back() == '\r') l.pop_back(); L.push_back(l); }
  if (L.size() < 9) { fprintf(stderr, "spec too short\n"); exit(2); }
  VARS = toks(L[0]); PNAMES = toks(L[1]); for (auto& t : toks(L[2])) PVALS.push_back(D(t));
  VF = L[3];
  auto a = toks(L[4]); K = atoi(a[0].c_str()); CSEC = D(a[1]); DIR = atoi(a[2].c_str()); NIT = atoi(a[3].c_str());
  auto b = toks(L[5]); for (int i = 0; i < 3; i++) { Dg[i] = D(b[i]); ELL[i] = D(b[3 + i]); } SYM = atoi(b[6].c_str());
  auto c = toks(L[6]); for (int i = 0; i < 3; i++) { EU[i] = D(c[i]); EV[i] = D(c[3 + i]); }
  auto d = toks(L[7]); XMID = D(d[0]); XSC = D(d[1]); int m = atoi(d[2].c_str());
  if ((int)d.size() != 3 + m) { fprintf(stderr, "coef count mismatch\n"); exit(2); }
  for (int j = 0; j < m; j++) COEF.push_back(D(d[3 + j]));
  auto e = toks(L[8]); W = D(e[0]); int ns = atoi(e[1].c_str());
  for (int i = 0; i < ns; i++) {
    auto s = toks(L[9 + i]);
    SETS.push_back({D(s[0]), D(s[1]), atoi(s[2].c_str()), D(s[3]), D(s[4]), atoi(s[5].c_str())});
  }
  if (VARS.size() != 3 || PNAMES.size() != PVALS.size() || (DIR != 1 && DIR != -1) || NIT < 1 || K < 0 || K > 2) {
    fprintf(stderr, "bad spec header\n"); exit(2);
  }
}

// ------------------------------- polynomial -------------------------------
static vector<interval> iC, iD1, iD2; static interval iXMID, iXSC;
static void setupPoly() {
  int m = COEF.size(); iXMID = interval(XMID); iXSC = interval(XSC);
  for (int j = 0; j < m; j++) iC.push_back(interval(COEF[j]));
  for (int j = 0; j < m - 1; j++) iD1.push_back(interval(double(m - 1 - j)) * iC[j]);
  for (int j = 0; j < m - 2; j++) iD2.push_back(interval(double(m - 2 - j)) * iD1[j]);
  if (iD2.empty()) iD2.push_back(interval(0.0));
  if (iD1.empty()) iD1.push_back(interval(0.0));
}
static interval horner(const vector<interval>& c, const interval& t) { interval r = c[0]; for (size_t j = 1; j < c.size(); j++) r = r * t + c[j]; return r; }
static interval ip(const interval& u) { return horner(iC, (u - iXMID) / iXSC); }
static interval idp(const interval& u) { return horner(iD1, (u - iXMID) / iXSC) / iXSC; }
static interval iddp(const interval& u) { return horner(iD2, (u - iXMID) / iXSC) / (iXSC * iXSC); }
static double dpoly(double u) { double t = (u - XMID) / XSC, r = COEF[0]; for (size_t j = 1; j < COEF.size(); j++) r = r * t + COEF[j]; return r; }

// ------------------------------- chart geometry -------------------------------
static int II, JJ;                 // the two in-plane coordinate indices
static interval iAl[3], iBe[3];    // u = alpha.X', v = beta.X'  (X' = X - origin; alpha_k = beta_k = 0)
static double dAl[3], dBe[3];
static void setupChart() {
  II = (K == 0) ? 1 : 0; JJ = (K == 2) ? 1 : 2;
  if (EU[K] != 0.0 || EV[K] != 0.0) { fprintf(stderr, "eu/ev not in the section plane\n"); exit(2); }
  interval a = interval(EU[II]), b = interval(EV[II]), c = interval(EU[JJ]), d = interval(EV[JJ]);
  interval det = a * d - b * c;
  if (det.contains(0.0)) { fprintf(stderr, "singular chart\n"); exit(2); }
  // [u;v] = (1/det) [ d -b ; -c a ] [X_i ; X_j]
  for (int i = 0; i < 3; i++) { iAl[i] = interval(0.0); iBe[i] = interval(0.0); }
  iAl[II] = d / det; iAl[JJ] = -b / det; iBe[II] = -c / det; iBe[JJ] = a / det;
  for (int i = 0; i < 3; i++) { dAl[i] = iAl[i].mid().leftBound(); dBe[i] = iBe[i].mid().leftBound(); }
}

// ------------------------------- engines -------------------------------
static string formula() {
  string s;
  if (!PNAMES.empty()) { s += "par:"; for (size_t i = 0; i < PNAMES.size(); i++) s += (i ? "," : "") + PNAMES[i]; s += ";"; }
  s += "var:" + VARS[0] + "," + VARS[1] + "," + VARS[2] + ";fun:" + VF + ";";
  return s;
}
static mutex g_mtx;
struct Engine {
  IMap ivf; IOdeSolver isolver; ICoordinateSection isec; IPoincareMap ipm;
  DMap dvf; DOdeSolver dsolver; DCoordinateSection dsec; DPoincareMap dpm;
  Engine()
    : ivf(formula()), isolver(ivf, 20), isec(3, K, interval(CSEC)),
      ipm(isolver, isec, DIR > 0 ? poincare::MinusPlus : poincare::PlusMinus),
      dvf(formula()), dsolver(dvf, 20), dsec(3, K, CSEC),
      dpm(dsolver, dsec, DIR > 0 ? poincare::MinusPlus : poincare::PlusMinus) {
    for (size_t i = 0; i < PNAMES.size(); i++) { ivf.setParameter(PNAMES[i], interval(PVALS[i])); dvf.setParameter(PNAMES[i], PVALS[i]); }
  }
};

struct Piece { double ua, ub, sa, sb; int depth; };
struct Result {
  bool ok = false, exc = false, definite = false; string why;
  bool badSign = false, badS = false, badEdge = false;
  interval X, S, ELLQ;
};

// edgeTest: 0 none, 1: X < bound, 2: X > bound
static Result evalPiece(Engine& E, const Piece& pc, int fold, int edgeTest, double bound) {
  Result R;
  try {
    bool edge = (pc.ua == pc.ub);
    double um = edge ? pc.ua : pc.ua + 0.5 * (pc.ub - pc.ua);
    interval U(pc.ua, pc.ub), iu(um);
    interval du = edge ? interval(0.0) : U - iu;
    interval pum = ip(iu), dpum = idp(iu);
    double a0 = dpum.mid().leftBound();
    interval Erem = edge ? interval(0.0) : (dpum - interval(a0)) * du + interval(0.5) * iddp(U) * sqr(du);
    interval ys = pum + interval(pc.sa, pc.sb) + Erem;
    double yc = ys.mid().leftBound();
    interval ry = ys - interval(yc);
    IVector xc(3), r(3), r0(3), xm(3); IMatrix C(3, 3);
    for (int i = 0; i < 3; i++) {
      interval org = (i == K) ? interval(CSEC) : interval(0.0);
      xc[i] = org + iu * interval(EU[i]) + interval(yc) * interval(EV[i]);
      double g = EU[i] + a0 * EV[i];
      interval gerr = (interval(EU[i]) + interval(a0) * interval(EV[i])) - interval(g);
      xm[i] = interval(xc[i].mid().leftBound());
      r[i] = (xc[i] - xm[i]) + du * gerr;
      C[i][0] = interval(g); C[i][1] = interval(EV[i]); C[i][2] = interval(i == K ? 1.0 : 0.0);
    }
    r0[0] = du; r0[1] = ry; r0[2] = interval(0.0);
    // non-rigorous image of the centre (frame only)
    DVector dc(3);
    double vcen = dpoly(um) + 0.5 * (pc.sa + pc.sb);
    for (int i = 0; i < 3; i++) dc[i] = ((i == K) ? CSEC : 0.0) + um * EU[i] + vcen * EV[i];
    DVector P0 = dc; for (int it = 0; it < NIT; it++) { double tt = 0.0; P0 = E.dpm(P0, tt); }
    double Q0[3]; for (int i = 0; i < 3; i++) Q0[i] = (fold ? Dg[i] : 1.0) * P0[i];
    double u0d = 0; for (int i = 0; i < 3; i++) u0d += dAl[i] * (i == K ? 0.0 : Q0[i]);
    double a1 = idp(interval(u0d)).mid().leftBound();
    IMatrix AQ(3, 3), Af(3, 3), Dm(3, 3);
    for (int i = 0; i < 3; i++) {
      AQ[0][i] = iAl[i]; AQ[1][i] = iBe[i] - interval(a1) * iAl[i]; AQ[2][i] = interval(i == K ? 1.0 : 0.0);
      Dm[i][i] = interval(fold ? Dg[i] : 1.0);
    }
    Af = AQ * Dm;
    IVector x0(3); for (int i = 0; i < 3; i++) x0[i] = interval(P0[i]);
    C0HOTripletonSet set(xm, C, r0, r);
    interval T;
    IVector y = E.ipm(set, x0, Af, T, NIT);
    interval dU = y[0], vp = y[1];
    interval U0(0.0), V0(0.0);
    for (int i = 0; i < 3; i++) if (i != K) { U0 += iAl[i] * interval(Q0[i]); V0 += iBe[i] * interval(Q0[i]); }
    interval ia1(a1);
    interval X = U0 + dU;
    interval V = V0 + vp + ia1 * dU;
    interval Xh = intervalHull(U0, X);
    interval S1 = (V0 - ip(U0)) + vp + (ia1 - idp(U0)) * dU - interval(0.5) * iddp(Xh) * sqr(dU);
    interval S2 = V - ip(X);
    interval S;
    if (!intersection(S1, S2, S)) { R.why = "empty intersection (bug?)"; return R; }
    R.X = X; R.S = S;
    bool ok = true;
    if (SYM) {
      interval eu_l(0.0), ev_l(0.0);
      for (int i = 0; i < 3; i++) { eu_l += interval(ELL[i]) * interval(EU[i]); ev_l += interval(ELL[i]) * interval(EV[i]); }
      interval ellQ = interval(ELL[K]) * interval(CSEC) + eu_l * X + ev_l * V;
      R.ELLQ = ellQ;
      if (!(ellQ.leftBound() > 0.0)) { ok = false; R.badSign = true; R.why += "ell.Q>0 (fold) not certified; "; }
      if (ellQ.rightBound() <= 0.0) { R.definite = true; R.why += "[definite: wrong fold] "; }
    }
    if (!(S.leftBound() > -W && S.rightBound() < W)) { ok = false; R.badS = true; R.why += "|s'|<w not certified; "; }
    if (S.leftBound() >= W || S.rightBound() <= -W) { R.definite = true; R.why += "[definite: |s'|>=w] "; }
    if (edgeTest == 1) {
      if (!(X.rightBound() < bound)) { ok = false; R.badEdge = true; R.why += "edge X<bound not certified; "; }
      if (X.leftBound() >= bound) { R.definite = true; R.why += "[definite: X>=bound] "; }
    }
    if (edgeTest == 2) {
      if (!(X.leftBound() > bound)) { ok = false; R.badEdge = true; R.why += "edge X>bound not certified; "; }
      if (X.rightBound() <= bound) { R.definite = true; R.why += "[definite: X<=bound] "; }
    }
    R.ok = ok;
  } catch (std::exception& e) {
    R.exc = true; string m = string(e.what()).substr(0, 80);
    for (auto& ch : m) if (ch == '\n' || ch == '\r') ch = ' ';
    R.why = string("exception: ") + m;
  }
  return R;
}

struct RunStats {
  string name; long leaves = 0, attempts = 0, exceptions = 0, fails = 0; int maxDepth = 0; bool incomplete = false;
  long failExc = 0, failSign = 0, failS = 0, failEdge = 0;
  double supAbsS = 0, Smin = 1e300, Smax = -1e300, Xmin = 1e300, Xmax = -1e300, minWidth = 1e300, ellMin = 1e300;
  double secs = 0; vector<string> failMsgs;
};

static RunStats runSet(const string& name, int nthreads, vector<Piece> init, int fold, int edgeTest,
                       double bound, double minW, FILE* leafLog) {
  RunStats st; st.name = name;
  auto t0 = chrono::steady_clock::now();
  vector<Piece> stack = init;
  mutex m; int busy = 0; bool abortRun = false;
  auto worker = [&]() {
    Engine* E;
    { lock_guard<mutex> lk(g_mtx); E = new Engine(); }
    while (true) {
      Piece pc;
      {
        unique_lock<mutex> lk(m);
        while (stack.empty() || abortRun) {
          if (abortRun || busy == 0) { delete E; return; }
          lk.unlock(); this_thread::sleep_for(chrono::milliseconds(5)); lk.lock();
        }
        if (chrono::duration<double>(chrono::steady_clock::now() - t0).count() > MAXSECS) {
          abortRun = true; st.incomplete = true; continue;
        }
        pc = stack.back(); stack.pop_back(); busy++;
      }
      Result r = evalPiece(*E, pc, fold, edgeTest, bound);
      lock_guard<mutex> lk(m);
      busy--; st.attempts++;
      if (r.exc) st.exceptions++;
      bool refine = false;
      if (r.ok && FRAC < 1.0) {
        double a = std::max(fabs(r.S.leftBound()), fabs(r.S.rightBound()));
        double wmax = std::max(pc.ub - pc.ua, pc.sb - pc.sa);
        if (a >= FRAC * W && wmax >= 1e-7) refine = true;   // bound tightening only
      }
      if (r.ok && !refine) {
        st.leaves++;
        st.maxDepth = std::max(st.maxDepth, pc.depth);
        double a = std::max(fabs(r.S.leftBound()), fabs(r.S.rightBound()));
        st.supAbsS = std::max(st.supAbsS, a);
        st.Smin = std::min(st.Smin, r.S.leftBound()); st.Smax = std::max(st.Smax, r.S.rightBound());
        st.Xmin = std::min(st.Xmin, r.X.leftBound()); st.Xmax = std::max(st.Xmax, r.X.rightBound());
        if (SYM) st.ellMin = std::min(st.ellMin, r.ELLQ.leftBound());
        st.minWidth = std::min(st.minWidth, std::max(pc.ub - pc.ua, pc.sb - pc.sa));
        fprintf(leafLog, "%s %a %a %a %a OK X=[%.17g,%.17g] S=[%.6e,%.6e]\n", name.c_str(), pc.ua, pc.ub, pc.sa, pc.sb,
                r.X.leftBound(), r.X.rightBound(), r.S.leftBound(), r.S.rightBound());
      } else {
        double wu = pc.ub - pc.ua, ws = pc.sb - pc.sa;
        bool splitU = wu >= ws;
        double lo = splitU ? pc.ua : pc.sa, hi = splitU ? pc.ub : pc.sb;
        double mid = lo + 0.5 * (hi - lo);
        if (!r.ok && (r.definite || std::max(wu, ws) < minW || !(lo < mid && mid < hi))) {
          st.fails++;
          if (r.exc) st.failExc++;
          if (r.badSign) st.failSign++;
          if (r.badS) st.failS++;
          if (r.badEdge) st.failEdge++;
          char buf[640];
          snprintf(buf, sizeof buf, "FAIL piece u=[%.17g,%.17g] s=[%.17g,%.17g]: %s X=[%.8g,%.8g] S=[%.6g,%.6g]", pc.ua, pc.ub,
                   pc.sa, pc.sb, r.why.c_str(), r.X.leftBound(), r.X.rightBound(), r.S.leftBound(), r.S.rightBound());
          if (st.failMsgs.size() < 10) st.failMsgs.push_back(buf);
          fprintf(leafLog, "%s %a %a %a %a FAIL %s\n", name.c_str(), pc.ua, pc.ub, pc.sa, pc.sb, r.why.c_str());
          if (st.fails > 2000) { abortRun = true; st.incomplete = true; }
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

static bool report(const RunStats& s, const string& extra) {
  bool pass = s.fails == 0 && !s.incomplete;
  printf("\n=== %s ===\n", s.name.c_str());
  printf("  verdict: %s%s\n", pass ? "PASS" : "FAIL", s.incomplete ? " (INCOMPLETE: time/fail cap hit)" : "");
  printf("  leaves(certified)=%ld attempts=%ld exceptions(->subdivided)=%ld failed_leaves=%ld [exc=%ld fold/ell=%ld s=%ld edge=%ld] maxDepth=%d minLeafWidth=%.3g time=%.1fs\n",
         s.leaves, s.attempts, s.exceptions, s.fails, s.failExc, s.failSign, s.failS, s.failEdge, s.maxDepth, s.minWidth, s.secs);
  printf("  image u range (union of certified enclosures) = [%.17g, %.17g]\n", s.Xmin, s.Xmax);
  printf("  s' enclosure range = [%.6e, %.6e]   sup|s'| <= %.6e  (w=%.17g)\n", s.Smin, s.Smax, s.supAbsS, W);
  if (SYM) printf("  min ell.Q over certified leaves >= %.6g\n", s.ellMin);
  if (!extra.empty()) printf("  %s\n", extra.c_str());
  for (auto& m : s.failMsgs) printf("  %s\n", m.c_str());
  fflush(stdout);
  return pass;
}

int main(int argc, char** argv) {
  fesetround(FE_TONEAREST);
  if (argc < 3) { fprintf(stderr, "usage: verify_b_gen file.spec leaflog [threads] [all|sets|edges] [FRAC] [maxsecs_per_run] [ngrid]\n"); return 2; }
  readSpec(argv[1]);
  int nth = argc > 3 ? atoi(argv[3]) : 12;
  string only = argc > 4 ? argv[4] : "all";
  if (argc > 5 && only != "one") FRAC = atof(argv[5]);
  if (argc > 6 && only != "one") MAXSECS = atof(argv[6]);
  int ngrid = argc > 7 ? atoi(argv[7]) : 64;
  printf("checker B generic verifier -- spec %s\n", argv[1]);
  printf("vector field: %s\n", formula().c_str());
  printf("params:"); for (size_t i = 0; i < PNAMES.size(); i++) printf(" %s=%a", PNAMES[i].c_str(), PVALS[i]); printf("\n");
  printf("section X_%d = %a, dir=%+d (%s), iterate n=%d; sym=%d D=(%g,%g,%g) ell=(%a,%a,%a)\n", K, CSEC, DIR,
         DIR > 0 ? "MinusPlus" : "PlusMinus", NIT, SYM, Dg[0], Dg[1], Dg[2], ELL[0], ELL[1], ELL[2]);
  printf("eu=(%a,%a,%a) ev=(%a,%a,%a); p: xmid=%a xsc=%a, %zu coefs:", EU[0], EU[1], EU[2], EV[0], EV[1], EV[2], XMID, XSC, COEF.size());
  for (double c : COEF) printf(" %a", c); printf("\n");
  printf("w=%a; %zu sets; FRAC=%g; max %gs per run; initial grid %d strips\n", W, SETS.size(), FRAC, MAXSECS, ngrid);
  for (size_t i = 0; i < SETS.size(); i++)
    printf("  set %zu: [%a,%a] orient=%+d Tlo=%a Thi=%a fold=%d\n", i, SETS[i].lo, SETS[i].hi, SETS[i].orient, SETS[i].Tlo, SETS[i].Thi, SETS[i].fold);
  if (SYM) {
    bool invol = true, anti = true;
    for (int i = 0; i < 3; i++) { if (Dg[i] * Dg[i] != 1.0) invol = false; if (Dg[i] * ELL[i] != -ELL[i]) anti = false; }
    printf("  symmetry checks: D involution (entries +-1): %s; D maps Sigma to itself (D_k=1 or c=0): %s; ell.D = -ell (fold unique): %s\n",
           invol ? "yes" : "NO", (Dg[K] == 1.0 || CSEC == 0.0) ? "yes" : "NO", anti ? "yes" : "NO");
    printf("  (that D commutes with the flow is part of the spec's hypothesis; not re-checked here)\n");
  }
  setupPoly(); setupChart();
  if (only == "one") {   // debug: evaluate one piece single-threaded: args ua ub sa sb (hex ok) set
    Piece pc{strtod(argv[5], nullptr), strtod(argv[6], nullptr), strtod(argv[7], nullptr), strtod(argv[8], nullptr), 0};
    int si = atoi(argv[9]); Engine E;
    Result r = evalPiece(E, pc, SYM ? SETS[si].fold : 0, 0, 0);
    printf("one: ok=%d exc=%d why=%s X=[%.17g,%.17g] S=[%.6g,%.6g]\n", r.ok, r.exc, r.why.c_str(), r.X.leftBound(), r.X.rightBound(), r.S.leftBound(), r.S.rightBound());
    return 0;
  }
  FILE* leaf = fopen(argv[2], "w");
  auto T0 = chrono::steady_clock::now();
  const double minW = 1e-11;
  auto grid = [](double a, double b, int n) {
    vector<Piece> v; vector<double> g(n + 1);
    for (int i = 0; i <= n; i++) g[i] = a + (b - a) * (double(i) / n);
    g[0] = a; g[n] = b;
    for (int i = 0; i < n; i++) { if (!(g[i] < g[i + 1])) { fprintf(stderr, "grid not monotone\n"); exit(3); } v.push_back({g[i], g[i + 1], -W, W, 0}); }
    return v;
  };
  if (only == "strip") {
    // process-parallel mode (CAPD is not safe to run in several threads: see REPORT): one strip / one edge,
    // single-threaded; prints one machine-readable STAT line; the driver merges the strips of a run.
    int si = atoi(argv[8]); string kind = argv[9]; int k = atoi(argv[10]);
    const SetSpec& S = SETS[si]; int f = SYM ? S.fold : 0; string nm = "S" + to_string(si);
    RunStats r;
    if (kind == "set") {
      vector<Piece> g = grid(S.lo, S.hi, ngrid);
      r = runSet(nm, 1, {g[k]}, f, 0, 0, minW, leaf);
    } else if (kind == "lo") {
      int t = S.orient > 0 ? 1 : 2; double b = S.orient > 0 ? S.Tlo : S.Thi;
      r = runSet(nm + "_lo", 1, {{S.lo, S.lo, -W, W, 0}}, f, t, b, minW, leaf);
    } else {
      int t = S.orient > 0 ? 2 : 1; double b = S.orient > 0 ? S.Thi : S.Tlo;
      r = runSet(nm + "_hi", 1, {{S.hi, S.hi, -W, W, 0}}, f, t, b, minW, leaf);
    }
    fclose(leaf);
    printf("STAT %s %ld %ld %ld %ld %ld %ld %ld %ld %d %.17g %.17g %.17g %.17g %.17g %.17g %.17g %.3f %d\n", r.name.c_str(),
           r.leaves, r.attempts, r.exceptions, r.fails, r.failExc, r.failSign, r.failS, r.failEdge, r.maxDepth, r.minWidth,
           r.Xmin, r.Xmax, r.Smin, r.Smax, r.supAbsS, SYM ? r.ellMin : 0.0, r.secs, r.incomplete ? 1 : 0);
    for (auto& m : r.failMsgs) printf("FAILMSG %s %s\n", r.name.c_str(), m.c_str());
    return 0;
  }
  bool okAll = true; long leaves = 0;
  for (size_t i = 0; i < SETS.size(); i++) {
    const SetSpec& S = SETS[i];
    int f = SYM ? S.fold : 0;
    string nm = "S" + to_string(i);
    if (only == "all" || only == "sets") {
      RunStats r = runSet(nm, nth, grid(S.lo, S.hi, ngrid), f, 0, 0, minW, leaf);
      okAll &= report(r, "claims 1+2 on N_" + to_string(i) + (SYM ? " (fold f=" + to_string(f) + ", ell.Q>0)" : " (no symmetry)"));
      leaves += r.leaves;
    }
    if (only == "all" || only == "edges") {
      // orient +1: lo -> X < Tlo, hi -> X > Thi ; orient -1: lo -> X > Thi, hi -> X < Tlo
      int tLo = S.orient > 0 ? 1 : 2; double bLo = S.orient > 0 ? S.Tlo : S.Thi;
      int tHi = S.orient > 0 ? 2 : 1; double bHi = S.orient > 0 ? S.Thi : S.Tlo;
      char buf[200];
      RunStats a = runSet(nm + "_lo", nth, {{S.lo, S.lo, -W, W, 0}}, f, tLo, bLo, minW, leaf);
      snprintf(buf, sizeof buf, "claim 3: edge u=lo -> u(Q) %s %.17g", tLo == 1 ? "<" : ">", bLo);
      okAll &= report(a, buf); leaves += a.leaves;
      RunStats b = runSet(nm + "_hi", nth, {{S.hi, S.hi, -W, W, 0}}, f, tHi, bHi, minW, leaf);
      snprintf(buf, sizeof buf, "claim 3: edge u=hi -> u(Q) %s %.17g", tHi == 1 ? "<" : ">", bHi);
      okAll &= report(b, buf); leaves += b.leaves;
    }
  }
  fclose(leaf);
  double secs = chrono::duration<double>(chrono::steady_clock::now() - T0).count();
  printf("\nTOTAL certified leaves=%ld wall=%.1fs threads=%d  OVERALL=%s\n", leaves, secs, nth, okAll ? "PASS" : "FAIL");
  return okAll ? 0 : 1;
}
