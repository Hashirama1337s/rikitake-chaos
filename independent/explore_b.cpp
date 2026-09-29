// checker B: NON-rigorous exploration only (used to pick subdivision heuristics, not for any claim)
#include <iostream>
#include <fstream>
#include <cstdio>
#include <cmath>
#include "capd/capdlib.h"
using namespace capd; using namespace std;
double MU,A,C,XMID,XSC,CO[9],L0,L1,R0,R1,W;
double p(double u){double t=(u-XMID)/XSC,r=CO[0];for(int k=1;k<9;k++)r=r*t+CO[k];return r;}
int main(int argc,char**argv){
  ifstream f(argv[1]); int n; f>>MU>>A>>C>>XMID>>XSC>>n; for(int k=0;k<9;k++)f>>CO[k]; f>>L0>>L1>>R0>>R1>>W;
  DMap vf("par:mu,a;var:x,y,z;fun:-mu*x+y*z,-mu*y+(z-a)*x,1-x*y;"); vf.setParameter("mu",MU); vf.setParameter("a",A);
  DOdeSolver solver(vf,20); DCoordinateSection sec(3,2,C); DPoincareMap pm(solver,sec,poincare::PlusMinus);
  auto Q=[&](double u,double s,double&X,double&S,double&T,int&fl){DVector x{u,p(u)+s,C}; double t; DVector y=pm(x,t); fl=y[0]<0; if(fl){y=-y;} X=y[0]; S=y[1]-p(y[0]); T=t;};
  int N=atoi(argv[2]);
  for(int side=0;side<2;side++){double a=side?R0:L0,b=side?R1:L1;
    for(int i=0;i<=N;i++){double u=a+(b-a)*i/N; double X,S,T,X1,S1,X2,S2; int fl,f1,f2; double h=1e-7;
      Q(u,0,X,S,T,fl); Q(u+h,0,X1,S1,T,f1); Q(u,W,X2,S2,T,f2);
      printf("%d u=%.6f xy=%.4f X=%.6f s'=%+.5f flip=%d T=%.3f dX/du=%.3g ds/du=%.3g dX/ds=%.3g ds/ds=%.3g\n",side,u,u*p(u),X,S,fl,T,(X1-X)/h,(S1-S)/h,(X2-X)/W,(S2-S)/W);}}
}
