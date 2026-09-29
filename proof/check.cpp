// Rigorous (interval arithmetic, CAPD) check of covering relations for the Rikitake two-disc dynamo
//   x' = -mu x + y z,  y' = -mu y + (z - a) x,  z' = 1 - x y
// Poincare section: plane z = c, crossings with z decreasing. Symmetry S(x,y,z) = (-x,-y,z) commutes with the flow.
// Reduced map Q = P on x>0 landings, S o P on x<0 landings (a "flip" = polarity reversal of both disc currents).
// Global chart on the section: (u,s) = (x, y - p(x)), p a fixed polynomial; (x,y) -> (u,s) is a homeomorphism of R^2.
// h-sets: N_L = [l0,l1] x [-w,w], N_R = [r0,r1] x [-w,w] in chart coordinates.
// Verified conditions (every bound below is a rigorous interval enclosure):
//   (1) Q is defined on N_L and N_R (transversal first return); N_L never flips, N_R always flips;
//   (2) |s| < w on Q(N_L) and Q(N_R);
//   (3) u < r0 on Q(left edge of N_L), u > r1 on Q(right edge of N_L);
//       u > r1 on Q(left edge of N_R), u < l0 on Q(right edge of N_R).
// => covering relations N_L => N_R, N_R => N_L, N_R => N_R (Zgliczynski-Gidea 2004).
// Usage: check <design file> <initial pieces along u>
#include <iostream>
#include <fstream>
#include <vector>
#include <functional>
#include "capd/capdlib.h"
using namespace capd; using namespace std;

interval mu, a, c, xmid, xsc, l0, l1, r0, r1, w; vector<double> pcd;
// p and p' in interval arithmetic (Horner in t = (x - xmid)/xsc)
interval poly(interval x) { interval t = (x - xmid) / xsc, v(0.); for (double q : pcd) v = v * t + interval(q); return v; }
interval dpoly(interval x) {
  interval t = (x - xmid) / xsc, v(0.); int n = pcd.size();
  for (int k = 0; k < n - 1; ++k) v = v * t + interval(pcd[k]) * interval(n - 1 - k);
  return v / xsc;
}

interval ddpoly(interval x) {
  interval t = (x - xmid) / xsc, v(0.); int n = pcd.size();
  for (int k = 0; k < n - 2; ++k) v = v * t + interval(pcd[k]) * interval(n - 1 - k) * interval(n - 2 - k);
  return v / (xsc * xsc);
}
struct Img { interval u, s; bool flipped; };
IPoincareMap* pm; DPoincareMap* dpm;

// Rigorous enclosure of Q over the chart box U x S = {(u, p(u)+s)}.
Img image(interval U, interval S) {
  // initial set as an affine set aligned with the curve: (u, p(u)+s, c) = X0 + C*(du, s + R), R = curvature remainder
  double uh = U.mid().leftBound();
  interval u0(uh), p0 = poly(u0), d0 = dpoly(u0);
  interval dU = U - u0;
  interval R = interval(0.5) * ddpoly(U) * sqr(dU);           // p(u) - p(uh) - p'(uh)(u - uh), Lagrange form
  IVector X0({u0, p0, c});
  IMatrix C({{1., 0., 0.}, {0., 1., 0.}, {0., 0., 1.}}); C[1][0] = d0;
  IVector r({dU, S + R, interval(0.)});
  C0HOTripletonSet set(X0, C, r);
  // non-rigorous prediction of the landing point (only used to choose coordinates)
  DVector xd({uh, p0.mid().leftBound(), c.leftBound()});
  DVector yd = (*dpm)(xd);
  double sg = yd[0] < 0 ? -1. : 1.;
  double xh = sg * yd[0];
  interval xhI(xh), ph = poly(xhI), dh = dpoly(xhI);
  // folded coordinates: v = A*(D P - Xh) = (A D)(P - D Xh), D = diag(sg, sg, 1)
  IMatrix AD({{sg, 0., 0.}, {0., sg, 0.}, {0., 0., 1.}}); AD[1][0] = -dh * sg;
  IVector DXh({interval(sg) * xhI, interval(sg) * ph, c});
  interval rt;
  IVector v = (*pm)(set, DXh, AD, rt);
  // v0 = X - xh, v1 = (Y - p(xh)) - p'(xh)(X - xh), with (X,Y) the folded landing point
  interval X = xhI + v[0];
  if (!(X > 0)) throw runtime_error("landing sign not determined");
  Img out; out.flipped = sg < 0; out.u = X;
  out.s = v[1] - interval(0.5) * ddpoly(X) * sqr(v[0]);     // s = Y - p(X), Lagrange remainder
  return out;
}

int MAXD = 14; int SPLIT = 0;   // 0: bisect u only, 1: s only, 2: both
int main(int argc, char* argv[]) {
  if (argc > 3) MAXD = atoi(argv[3]);
  if (argc > 4) SPLIT = atoi(argv[4]);
  cout.precision(10); cout << boolalpha;
  ifstream in(argv[1]); int nU = atoi(argv[2]);
  double d[5]; for (int i = 0; i < 5; ++i) in >> d[i];
  mu = d[0]; a = d[1]; c = d[2]; xmid = d[3]; xsc = d[4];     // decimals -> exact doubles; theorem is for these values
  int n; in >> n; pcd.resize(n); for (auto& q : pcd) in >> q;
  double e[5]; for (int i = 0; i < 5; ++i) in >> e[i];
  l0 = e[0]; l1 = e[1]; r0 = e[2]; r1 = e[3]; w = e[4];
  cout << "mu=" << mu << " a=" << a << " c=" << c << "\nL=[" << l0.leftBound() << "," << l1.leftBound() << "] R=["
       << r0.leftBound() << "," << r1.leftBound() << "] w=" << w.leftBound() << endl;

  IMap vf("par:mu,a;var:x,y,z;fun:-mu*x+y*z,-mu*y+(z-a)*x,1-x*y;");
  vf.setParameter("mu", mu); vf.setParameter("a", a);
  IOdeSolver solver(vf, 20);
  ICoordinateSection section(3, 2, c);
  IPoincareMap ipm(solver, section, poincare::PlusMinus);   // z - c goes from + to -
  DMap dvf("par:mu,a;var:x,y,z;fun:-mu*x+y*z,-mu*y+(z-a)*x,1-x*y;");
  dvf.setParameter("mu", d[0]); dvf.setParameter("a", d[1]);
  DOdeSolver dsolver(dvf, 20);
  DCoordinateSection dsection(3, 2, d[2]);
  DPoincareMap dpmap(dsolver, dsection, poincare::PlusMinus);
  pm = &ipm; dpm = &dpmap;

  bool ok = true; long nPieces = 0; int maxDepth = 0; double smax = 0;
  // A piece that fails (band, flip, or any exception) is bisected in u and s, up to depth 26; a failure there is final.
  function<void(interval, interval, bool, int)> piece = [&](interval U, interval S, bool wantFlip, int depth) {
    bool good = false; Img im;
    try { im = image(U, S); good = (im.flipped == wantFlip) && (abs(im.s) < w); } catch (exception&) { good = false; }
    if (good) { smax = std::max(smax, abs(im.s).rightBound()); ++nPieces; maxDepth = std::max(maxDepth, depth); return; }
    if (depth >= MAXD) { cout << "FAIL piece U=" << U << " S=" << S;
      try { Img im2 = image(U, S); cout << " flipped=" << im2.flipped << " s=" << im2.s << " u=" << im2.u; } catch (exception& e) { cout << " exception: " << string(e.what()).substr(0, 80); }
      cout << endl; ok = false; return; }
    interval Um(U.mid()), Sm(S.mid());
    interval U1(U.leftBound(), Um.rightBound()), U2(Um.leftBound(), U.rightBound());
    interval S1(S.leftBound(), Sm.rightBound()), S2(Sm.leftBound(), S.rightBound());
    int mode = SPLIT;
    if (SPLIT == 3) {
      double uh = U.mid().leftBound(), h = 1e-6, cu, cs;
      auto fu = [&](double uu, double ss) { DVector xd({uu, poly(interval(uu)).mid().leftBound() + ss, c.leftBound()});
        DVector yd = (*dpm)(xd); return fabs(yd[0]); };
      try { double f0 = fu(uh, 0.); cu = fabs(fu(uh + h, 0.) - f0) / h * (U.rightBound() - U.leftBound());
            cs = fabs(fu(uh, h) - f0) / h * (S.rightBound() - S.leftBound()); mode = cu > cs ? 0 : 1; } catch (exception&) { mode = 2; }
    }
    if (mode == 0) { piece(U1, S, wantFlip, depth + 1); piece(U2, S, wantFlip, depth + 1); }
    else if (mode == 1) { piece(U, S1, wantFlip, depth + 1); piece(U, S2, wantFlip, depth + 1); }
    else { piece(U1, S1, wantFlip, depth + 1); piece(U2, S1, wantFlip, depth + 1);
           piece(U1, S2, wantFlip, depth + 1); piece(U2, S2, wantFlip, depth + 1); }
  };
  auto sweep = [&](interval A, interval B, bool wantFlip) {
    interval du = (B - A) / nU;
    for (int i = 0; i < nU; ++i) {
      interval U = A + du * interval(i, i + 1);
      if (i == 0) U = interval(A.leftBound(), U.rightBound());          // pieces cover [A,B] exactly
      if (i == nU - 1) U = interval(U.leftBound(), B.rightBound());
      long before = nPieces; int md0 = maxDepth; maxDepth = 0;
      piece(U, interval(-w.rightBound(), w.rightBound()), wantFlip, 0);
      cout << "  piece " << i << " u=[" << U.leftBound() << "," << U.rightBound() << "] leaves " << (nPieces - before)
           << " depth " << maxDepth << (ok ? "" : "  (FAILURE SO FAR)") << endl;
      maxDepth = std::max(md0, maxDepth);
      if (!ok) return;
    }
  };
  // Edge {U} x [-w,w]: adaptive bisection in s until each image satisfies u < bound (wantLess) or u > bound.
  // Stops at once with a definite failure if an image enclosure lies entirely on the wrong side.
  function<int(interval, interval, interval, bool, double&, double&, int)> edgePiece =
    [&](interval U, interval S, interval bound, bool wantLess, double& lo, double& hi, int depth) -> int {
    try { Img im = image(U, S);
      if (wantLess ? (im.u < bound) : (im.u > bound)) { lo = std::min(lo, im.u.leftBound()); hi = std::max(hi, im.u.rightBound()); return 1; }
      if (wantLess ? (im.u.leftBound() >= bound.rightBound()) : (im.u.rightBound() <= bound.leftBound())) {
        cout << "EDGE DEFINITE FAIL U=" << U << " S=" << S << " image u=" << im.u << endl; return -1; }
    } catch (exception&) {}
    if (depth >= 30) { cout << "EDGE FAIL (depth) U=" << U << " S=" << S << endl; return -1; }
    interval Sm(S.mid());
    int b1 = edgePiece(U, interval(S.leftBound(), Sm.rightBound()), bound, wantLess, lo, hi, depth + 1);
    if (b1 < 0) return -1;
    return edgePiece(U, interval(Sm.leftBound(), S.rightBound()), bound, wantLess, lo, hi, depth + 1);
  };
  auto edge = [&](interval U, interval bound, bool wantLess, const char* label) {
    double lo = 1e300, hi = -1e300;
    bool b = edgePiece(U, interval(-w.rightBound(), w.rightBound()), bound, wantLess, lo, hi, 0) > 0;
    cout << label << " u in [" << lo << ", " << hi << "] : " << b << endl; return b;
  };
  try {
    sweep(l0, l1, false); cout << "N_L: band |s|<w and no-flip verified; pieces " << nPieces << ", max depth " << maxDepth << ", sup|s| <= " << smax << endl;
    sweep(r0, r1, true);  cout << "N_R: band |s|<w and flip verified;    pieces " << nPieces << ", max depth " << maxDepth << ", sup|s| <= " << smax << endl;
    bool c1 = edge(l0, r0, true, "Q(left  edge N_L), need u < r0:");
    bool c2 = edge(l1, r1, false, "Q(right edge N_L), need u > r1:");
    bool c3 = edge(r0, r1, false, "Q(left  edge N_R), need u > r1:");
    bool c4 = edge(r1, l0, true, "Q(right edge N_R), need u < l0:");
    ok = ok && c1 && c2 && c3 && c4;
  } catch (exception& ex) { cout << "EXCEPTION: " << ex.what() << endl; ok = false; }
  cout << (ok ? "RESULT: ALL COVERING CONDITIONS VERIFIED" : "RESULT: NOT VERIFIED") << endl;
  return ok ? 0 : 1;
}
