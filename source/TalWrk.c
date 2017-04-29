// ======================================================== TalWrk.c
//
// Particle driver for AUSTAL2000
// ==============================
//
// Copyright (C) Umweltbundesamt, Dessau-Roﬂlau, Germany, 2002-2013
// Copyright (C) Janicke Consulting, 88662 ‹berlingen, Germany, 2002-2013
// email: info@austal2000.de
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation; either version 2 of
// the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//
// last change:  2013-07-08 uj
//
// ==================================================================

#include <math.h>

#define  STDMYMAIN  WrkMain
#include "IBJmsg.h"
#include "IBJary.h"
#include "IBJtxt.h"
#include "IBJdmn.h"
#include "IBJstd.h"
static char *eMODn = "TalWrk";

//=========================================================================

STDPGM(tstwrk, WrkServer, 2, 6, 8);

//=========================================================================

#include "genutl.h"
//#include "genio.h"

#include "TalWnd.h"
#include "TalPrf.h"
#include "TalGrd.h"
#include "TalMat.h"
#include "TalMod.h"
#include "TalNms.h"
#include "TalPlm.h"
#include "TalPpm.h"
#include "TalPrm.h"
#include "TalPtl.h"
#include "TalSrc.h"
#include "TalTmn.h"
#include "TalDef.h"                                               //-2011-12-05
#include "TalWrk.h"
#include "TalWrk.nls"

#define  LOC 0x02000000L
#ifndef  PI
#define  PI  3.1415926
#endif

#define  WRK_REFB   0x0001
#define  WRK_REFT   0x0002
#define  WRK_USE_H  0x0100
#define  WRK_USE_AZ 0x0200

#define  INSOLID(a,b,c)  ((Pbn) && *(long*)AryPtrX(Pbn,(a),(b),(c))&GRD_VOLOUT)

typedef struct {
   int i, j, k, is, js, ks;
   float ax, ay, az;
   } LOCREC;

typedef struct xfrrec { int ir, ic; float val;                    //-2011-10-13
} XFRREC;

   
/*======================== Funktions-Prototypen ============================*/

static int ClcAfu(    // Berechnung der Abgasfahnen-Ueberhoehung
SRCREC *ps,           // Pointer auf die Quelle
float v2,             // Geschwindigkeitsquadrat
float stab,           // Stabilitaetsklasse oder Temperatur-Gradient
float *pu,            // Ergebnis: Ueberhoehung.
float *pt );          // Ergebnis: Anstiegszeit.

static int ClcPtlLoc(     // Teilchen im Netz einordnen, evtl. reflektieren
PTLREC *pa,               // Pointer auf alte Teilchen-Koordinaten
PTLREC *pp,               // Pointer auf neue Teilchen-Koordinaten
LOCREC *pl,               // Pointer auf Index-Werte
int h2z );                // h-Wert nach z-Wert umwandeln

static int ClcLocMod(       // Feld am Ort des Teilchens berechnen
LOCREC *pl,                 // Pointer auf Index-Werte
MD3REC *p3,                 // Pointer auf Feld-Werte
int clceta );               // <=0, wenn Eta nicht interpoliert werden soll

static int RunPtl(          // Teilchen-Trajektorien berechnen.
void );

static int Try2Escape(  // In einem Gebaeude erzeugte Partikel herausdraengen
PTLREC *pa,
LOCREC *pl );



/*=================== Alle Groessen nur intern fuer Worker ====================*/

static long Flags = 0;
static int Valid = 1;
static int Nptl = 0;
static int Ndsc = 0;
static int NumRun = 0;
static int NumRemove = 0;
static int MaxRetry = 50;
static int MinRetry = 6;
static int Nx, Ny, Nz, Nzm, PrfMode, DefMode, TrcMode, Nc;
static int OnlyAdvec, NoPredict, ExactTau;
static long T1, T2, DosT1, DosT2;
static long PrmT2, PpmT2, WdpT2, SrcT2, XfrT2, PtlT2, ModT2;
static float Dd, Xmin, Xmax, Gx, Ymin, Ymax, Gy, Xbmin, Xbmax, Ybmin, Ybmax;
static float Zref, Zscl, Stab, *Sk, Hm;
static int Gl, Gi, Gp;
static char AltName[256];
static FILE *TrcFile;

static GRDPARM *Pgp;    /*  Grid-Parameter      */
static PRMINF  *Ppi;    /*  General Parameter   */
static ARYDSC  *Pma;    /*  Model-Array         */
static ARYDSC  *Ppp;    /*  Particle-Parameter  */
static ARYDSC  *Ppa;    /*  Particle-Array      */
static ARYDSC  *Pda;    /*  Dose-Array          */
static ARYDSC  *Pwa;    /*  Wdep-Array          */
static ARYDSC  *Pxa;    /*  Xfr-Array (Chem)    */
static ARYDSC  *Psa;    /*  Source-Array        */
static ARYDSC  *Pbn;    /*  Boundaries-Array    */

static int bPerx, bPery, bDdmet;
static int TotalTime, TotalDrop, AverTime, AverDrop, InterTime, InterDrop, DropT1;
//static int PartTotal, PartNew, PartDrop;                        //-2001-06-29
static char PhaseSymbols[] = "|/-\\";
static int CountStep, CountPhase, CountMax=50000;
static int ShowProgress, Progress=-1;                             //-2001-07-10

static int Nxfr;                                                  //-2011-10-13
static XFRREC *Pxfr;
static int *Ixfr;

//-------------------------------------------------------------

static unsigned long WrkRndSeed = 123456;
static int WrkRndIset = 0;

static unsigned long WrkRndULong() {
  WrkRndSeed = 1664525*WrkRndSeed + 1013904223;
  return WrkRndSeed;
}

static unsigned long WrkRndGetSeed() {
  return WrkRndSeed;
}

static void WrkRndSetSeed(unsigned long seed) {
  if (seed != 0)  WrkRndSeed = seed;
  else            WrkRndSeed = 123456;
  WrkRndIset = 0;
}

static float WrkRndFloat() {
  unsigned long r = WrkRndULong();
  float f = ((r>>8) & 0x00ffffff)/((float)0x01000000);
  return f;
}

#define WrkRnd01()  WrkRndFloat()  // random numbers
#define WrkRnd()    WrkRndNrm()    // use normal distribution

//================================================================== WrkRndNrm
//
static float WrkRndNrm()   // normal distribution, average=0, variance=1  -2008-12-11
  {
  static float gset;
  float fac, r, v1, v2;
  if (WrkRndIset == 0) {
    do {
      r = WrkRndFloat();
      v1 = 2.0*r - 1.0;
      r = WrkRndFloat();
      v2 = 2.0*r - 1.0;
      r = v1*v1 + v2*v2;
      } while (r >= 1.0);
    fac = sqrt( -2.0*log(r)/r );
    gset = v1*fac;
    WrkRndIset = 1;
    return v2*fac;
    }
  else {
    WrkRndIset = 0;
    return gset;
    }
  }

/*--------------------------------------------------------------------------*/
#define  a  (*pa)
static void prnPtl( FILE *prn, char *txt, int np, PTLREC *pa, double tp )
  {
  int i;
  fprintf(prn, "%s %4d: t=%ld flg=%02x rfl=%02x src=%ld off=%d rnd=%ld tp=%1.1lf\n",
    txt, np, a.t, a.flag, a.refl, a.srcind, a.offcmp, a.rnd, tp );
  fprintf(prn, "   x=(%11.3lf,%11.3lf,%11.3lf/%11.3lf)  v=(%7.3f,%7.3f,%7.3f) \n",
    a.x, a.y, a.z, a.h, a.u, a.v, a.w );
  fprintf(prn, "   afu=(%10.2e,%10.2e) \n",  a.afuhgt, a.afutsc );
  fprintf(prn, "   g=(");
  for (i=0; i<a.numcmp; i++)  fprintf(prn, " %10.3e", a.g[2*i]);
  fprintf(prn, " )\n");
  }
static void trcPtl( int np, PTLREC *pa, double tp, MD3REC m )
  {
  int i;
  fprintf(TrcFile,
  "# %3d %ld %d %02x %02x: %8.2lf | %8.2lf %8.2lf %7.3lf | "
  "%6.3f %6.3f %6.3f | %6.3f %6.3f %6.3f |",
    np, a.srcind, a.offcmp, a.flag, a.refl, tp, a.x, a.y, a.z,
    m.vx, m.vy, m.vz, a.u, a.v, a.w );
  for (i=0; i<a.numcmp; i++)  fprintf(TrcFile, " %10.3e", a.g[2*i]);
  fprintf(TrcFile, "\n");
  }
#undef  a

static void trcInit( int gp, int nl, int ni, long t1, long t2 )
  {
  fprintf( TrcFile,
    "#### %d %d %d [%ld,%ld]\n", gp, nl, ni, t1, t2 );
  }


/*===================================================================== ClcAfu
*/
static int ClcAfu(    /* Berechnung der Abgasfahnen-Ueberhoeung         */
SRCREC *ps,           /* Pointer auf die Quelle                        */
float v2,             /* Geschwindigkeitsquadrat                       */
float stab,           /* Stabilitaetsklasse oder Temperatur-Gradient    */
float *pu,            /* Ergebnis: Ueberhoeung.                         */
float *pt )           /* Ergebnis: Anstiegszeit.                       */
  {
  dP(ClcAfu);
  float uq, dh, xe;
  int m, rc;
  if (!ps)                                                          eX(1);
  if (ps->ts >= 0) {
    *pt = ps->ts;
    *pu = ps->ts*ps->vq;
  }
  else {
    m = PLM_VDI3782_3;
    uq = sqrt(v2);
    rc = TalPlm(m, &dh, &xe, ps->qq, ps->h, uq, stab,
                ps->vq, ps->dq, ps->rhy, ps->lwc, ps->tmp);       //-2002-12-10
    if (rc  < 0) {
      if (rc == -1 && (PlmMsg))                            eX(3); //-2002-09-23
      eX(2);
    }
    *pu = dh;
    if (uq < 1.0)  uq = 1.0;
    if (ps->qq>0 || ps->tmp>10)  *pt = 0.4*xe/uq;                  //-2002-12-10
    else if (ps->vq > 0)  *pt = dh/ps->vq;
         else  *pt = 0;
  }
  return 0;
eX_1:
  eMSG(_no_source_data_);
eX_3:
  vMsg("%s", PlmMsg);
  PlmMsg = NULL;
eX_2:
  eMSG(_cant_calculate_plume_rise_);
}

/*================================================================= ClcAfuRxyz
*/
static int ClcAfuRxyz( SRCREC *ps, float *pvs, float *prx, float *pry, float *prz )
  {
  dP(ClcAfuRxyz);
  float sif, cof, sig, cog, dq;
  float rx, ry, rz, ax, ay, az, r, vs, fq, gq, sh, sv, sl;
  *prx = 0;
  *pry = 0;
  *prz = 1;
  if (!ps)                                      eX(1);
  if (ps->qq)
    return 0;
  fq = ps->fq;
  sh = ps->sh;
  sv = ps->sv;
  sl = ps->sl;
  if (fq==0 && sh<=0 && sv<=0 && sl<=0)  return 0;
  vs = *pvs;
  dq = ps->d;
  cof = cos(fq*PI/180);
  sif = sin(fq*PI/180);
  if (ps->gq > -999) {
    gq = ps->gq;          // gq: meterological orientation
    sig = cos(gq*PI/180); // sig: mathematical orientation
    cog = sin(gq*PI/180);
  }
  else {
    gq = (ps->aq >= 0) ? 90-dq : dq;
    cog = ps->co;
    sig = ps->si;
  }
  rz = vs*cof;                              //-99-08-15
  ry = vs*sif*sig;
  rx = vs*sif*cog;
  if (sh > 0) {                             //-99-08-05
    ax = sig;
    ay = -cog;
    r = sh*WrkRndNrm();
    rx += r*ax;
    ry += r*ay;
  }
  if (sv > 0) {
    ax = -cof*cog;
    ay = -cof*sig;
    az = sif;
    r = sv*WrkRndNrm();
    rx += r*ax;
    ry += r*ay;
    rz += r*az;
  }
  if (sl > 0) {
    ax = sif*cog;
    ay = sif*sig;
    az = cof;
    r = sl*WrkRndNrm();
    rx += r*ax;
    ry += r*ay;
    rz += r*az;
  }
  r = sqrt(rx*rx + ry*ry + rz*rz);
  *pvs = r;
  if (r > 0) {
    *prx = rx/r;
    *pry = ry/r;
    *prz = rz/r;
    }
  return 1;
eX_1:
  eMSG(_no_source_data_);
  }

//===================================================================ClcPtlLoc
//
static int ClcPtlLoc(     /* Teilchen im Netz einordnen, evtl. reflektieren */
PTLREC *pa,               /* Pointer auf alte Teilchen-Koordinaten          */
PTLREC *pp,               /* Pointer auf neue Teilchen-Koordinaten          */
LOCREC *pl,               /* Pointer auf Index-Werte                        */
int h2z )                 /* h-Wert nach z-Wert umwandeln                   */
  {
//  dP(ClcPtlLoc);
  int i, j, k, flg, m;
  float r, x, y, z, dz, xi, eta, zeta;
  float zg;
  flg = GRD_REFBOT;
  z = (pa) ? pa->z : 0;                                         //-2002-04-10
  if ((pa) && (Zref) && (z<=Zref)) flg |= GRD_REFTOP;           //-2002-04-10
  zeta = 0;
  if (h2z == WRK_USE_H) { z = pp->h;  flg |= GRD_PARM_IS_H; }
  else {
    z = pp->z;
    if (h2z == WRK_USE_AZ)  flg |= GRD_USE_ZETA;
    zeta = pl->az;
    }

  x = pp->x;
  i = -1;
  if (x < Xmin) {
    if (bPerx) {
      x = Xmax - fmod(Xmin-x, Gx);                //-2001-03-23 lj
      pp->x = x;
      }
    else  i = 0;
    }
  else if (x > Xmax) {
    if (bPerx) {
      x = Xmin + fmod(x-Xmax, Gx);                //-2001-03-23 lj
      pp->x = x;
      }
    else  i = 0;
    }

  y = pp->y;
  j = -1;
  if (y < Ymin) {
    if (bPery) {
      y = Ymax - fmod(Ymin-y, Gy);                //-2001-03-23 lj
      pp->y = y;
      }
    else  j = 0;
    }
  else if (y > Ymax) {
    if (bPery) {
      y = Ymin + fmod(y-Ymax, Gy);                //-2001-03-23 lj
      pp->y = y;
      }
    else  j = 0;
    }
  xi =  (x - Xmin)/Dd;
  eta = (y - Ymin)/Dd;
  pl->i = 0;
  pl->j = 0;
  k = pl->k;
  pl->k = 0;
  if (bDdmet) {
    if (i==0 || j==0) { pp->flag |= SRC_GOUT;  return 0; }
    m = GrdLocate(Pma, xi, eta, &z, flg, Zref, &i, &j, &k, &zg, &zeta);
    if (m & GRD_REFTOP)  pp->refl |= WRK_REFT;
    if (m & GRD_REFBOT)  pp->refl |= WRK_REFB;
    if (!(m & GRD_INSIDE))  pp->flag |= SRC_GOUT;
    }
  else {
    zg = 0;
    if (i==0 || j==0) {
      if (x<=Xbmin || x>=Xbmax)  pp->flag |= SRC_XOUT;
      if (y<=Ybmin || y>=Ybmax)  pp->flag |= SRC_YOUT;
      }
    else {
      i = 1 + xi;   if (i > Nx)  i = Nx;
      j = 1 + eta;  if (j > Ny)  j = Ny;
      }
    if (z < 0) { z = -z;  pp->refl |= WRK_REFB; }
    if (flg&GRD_REFTOP && z>Zref) { z = 2*Zref-z;  pp->refl |= WRK_REFT; }
    for (k=1; k<=Nz; k++)
      if (z <= Sk[k]) {                                               //-2002-04-10
        dz = Sk[k] - Sk[k-1];
        zeta = (z - Sk[k-1])/dz;
        break;
        }
    if (k > Nz) {
      k = 0;  pp->flag |= SRC_ZOUT; }
    }
  if (i < 0)  i = 0;
  if (j < 0)  j = 0;
  pl->i = i;
  pl->j = j;
  pl->k = k;
  pp->z = z;
  if ((i) && (j))  pp->h = z - zg;
  if (pp->flag & SRC_GOUT)  return 0;
  pl->ax = xi - (i-1);
  pl->ay = eta - (j-1);
  pl->az = zeta;

  if (bDdmet) {
    r = WrkRnd01();
    if (r > pl->ax)  pl->is = i-1;
    else  pl->is = i;
    r = WrkRnd01();
    if (r > pl->ay)  pl->js = j-1;
    else  pl->js = j;
    pl->ks = k;
    }
  else {
    pl->is = i;
    pl->js = j;
    pl->ks = k;
    }
  return 1;
  }

//================================================================== ClcLocMod
//
static int ClcLocMod(   /* Feld am Ort des Teilchens berechnen          */
LOCREC *pl,             /* Pointer auf Index-Werte                      */
MD3REC *p3,             /* Pointer auf Feld-Werte                       */
int clceta )            /* <=0, wenn Eta nicht interpoliert werden soll */
  {
  dP(ClcLocMod);
  MD2REC *p2a, *p2b;
  MD3REC *p3a, *p3b;
  MD3REC *p000, *p001, *p010, *p100, *p011, *p101, *p110, *p111;
  float a000, a001, a010, a100, a011, a101, a110, a111;
  float ax, bx, ay, by, az, bz;
  int i, j, k, is, js;
  float a, b;
  i = pl->i;
  j = pl->j;
  k = pl->k;
  is = pl->is;
  js = pl->js;
  if (k == 0)                   eX(1);
  if (bDdmet) {
    if (i==0 || j==0)           eX(2);
    p3a = (MD3REC*) AryPtr(Pma, i, j, k-1);  if (!p3a)             eX(4);
    if (p3a->vz <= WND_VOLOUT)  return 0;    /* Innerhalb eines Koerpers */
    p3b = (MD3REC*) AryPtr(Pma, i, j, k  );  if (!p3b)             eX(5);
    if (p3b->ths <= WND_VOLOUT)  return 0;   /* Innerhalb eines Koerpers */
    if (p3b->wz  <= WND_VOLOUT)  return 0;   /* Innerhalb eines Koerpers */
    memset(p3, 0, sizeof(MD3REC));         /* Zuerst alles auf 0 setzen */
    if (!OnlyAdvec) {
      p3->ths = p3b->ths;         /* Gradient der potentiellen Temperatur */
      p3->wx = p3b->wx;                     /* Kompensation, x-Komponente */
      p3->wy = p3b->wy;                     /* Kompensation, y-Komponente */
      p3->wz = p3b->wz;                     /* Kompensation, z-Komponente */
    }
    if (DefMode & GRD_3LIN) {             /* Geschwindigkeiten 3*linear */
      p000 = (MD3REC*) AryPtr(Pma, i-1, j-1, k-1);  if (!p000)     eX(10);
      p001 = (MD3REC*) AryPtr(Pma, i-1, j-1, k  );  if (!p001)     eX(11);
      p010 = (MD3REC*) AryPtr(Pma, i-1, j  , k-1);  if (!p010)     eX(12);
      p100 = (MD3REC*) AryPtr(Pma, i  , j-1, k-1);  if (!p100)     eX(13);
      p011 = (MD3REC*) AryPtr(Pma, i-1, j  , k  );  if (!p011)     eX(14);
      p101 = (MD3REC*) AryPtr(Pma, i  , j-1, k  );  if (!p101)     eX(15);
      p110 = (MD3REC*) AryPtr(Pma, i  , j  , k-1);  if (!p110)     eX(16);
      p111 = (MD3REC*) AryPtr(Pma, i  , j  , k  );  if (!p111)     eX(17);
      bx = pl->ax;
      ax = 1.0 - bx;
      by = pl->ay;
      ay = 1.0 - by;
      bz = pl->az;
      az = 1.0 - bz;
      a000 = ax*ay*az;
      a001 = ax*ay*bz;
      a010 = ax*by*az;
      a100 = bx*ay*az;
      a011 = ax*by*bz;
      a101 = bx*ay*bz;
      a110 = bx*by*az;
      a111 = bx*by*bz;
      p3->vx = a000*p000->vx + a001*p001->vx + a010*p010->vx + a100*p100->vx +
             a011*p011->vx + a101*p101->vx + a110*p110->vx + a111*p111->vx;
      p3->vy = a000*p000->vy + a001*p001->vy + a010*p010->vy + a100*p100->vy +
             a011*p011->vy + a101*p101->vy + a110*p110->vy + a111*p111->vy;
      p3->vz = a000*p000->vz + a001*p001->vz + a010*p010->vz + a100*p100->vz +
             a011*p011->vz + a101*p101->vz + a110*p110->vz + a111*p111->vz;
      }
    else {
      p3a = (MD3REC*) AryPtr(Pma, i-1, j, k);  if (!p3a)           eX(20);
      p3->vx = (1-pl->ax)*p3a->vx + pl->ax*p3b->vx;   /* Vx interpolieren */
      p3a = (MD3REC*) AryPtr(Pma, i, j-1, k);  if (!p3a)           eX(21);
      p3->vy = (1-pl->ay)*p3a->vy + pl->ay*p3b->vy;   /* Vy interpolieren */
      p3a = (MD3REC*) AryPtr(Pma, i, j, k-1);  if (!p3a)           eX(22);
      p3->vz = (1-pl->az)*p3a->vz + pl->az*p3b->vz;   /* Vz interpolieren */
      }
    if (PrfMode == 3) {                /* Anisotrope Modellierung     */
      a = 1 - pl->az;
      b = pl->az;
      p3a = (MD3REC*) AryPtr(Pma, is, js, k-1);  if (!p3a)         eX(23);
      p3b = (MD3REC*) AryPtr(Pma, is, js, k  );  if (!p3b)         eX(24);
      p3->tau = a*p3a->tau + b*p3b->tau;
      if (OnlyAdvec)  return 1;
      if (clceta < 0)  return 1;
      p3->pxx = a*p3a->pxx + b*p3b->pxx;
      p3->pxy = a*p3a->pxy + b*p3b->pxy;
      p3->pxz = a*p3a->pxz + b*p3b->pxz;
      p3->pyx = a*p3a->pyx + b*p3b->pyx;
      p3->pyy = a*p3a->pyy + b*p3b->pyy;
      p3->pyz = a*p3a->pyz + b*p3b->pyz;
      p3->pzx = a*p3a->pzx + b*p3b->pzx;
      p3->pzy = a*p3a->pzy + b*p3b->pzy;
      p3->pzz = a*p3a->pzz + b*p3b->pzz;
      p3->lxx = a*p3a->lxx + b*p3b->lxx;
      p3->lyx = a*p3a->lyx + b*p3b->lyx;
      p3->lyy = a*p3a->lyy + b*p3b->lyy;
      p3->lzx = a*p3a->lzx + b*p3b->lzx;
      p3->lzy = a*p3a->lzy + b*p3b->lzy;
      p3->lzz = a*p3a->lzz + b*p3b->lzz;
      if (clceta == 0)  return 1;
      p3->exx = a*p3a->exx + b*p3b->exx;
      p3->eyx = a*p3a->eyx + b*p3b->eyx;
      p3->eyy = a*p3a->eyy + b*p3b->eyy;
      p3->ezx = a*p3a->ezx + b*p3b->ezx;
      p3->ezy = a*p3a->ezy + b*p3b->ezy;
      p3->ezz = a*p3a->ezz + b*p3b->ezz;
      return 1;
      }
    else {                                        /* Isotrope Modellierung */
      a = 1 - pl->az;
      b = pl->az;
      p2a = (MD2REC*) AryPtr(Pma, is, js, k-1);  if (!p2a)         eX(25);
      p2b = (MD2REC*) AryPtr(Pma, is, js, k  );  if (!p2b)         eX(26);
      p3->tau = a*p2a->tau + b*p2b->tau;
      if (OnlyAdvec)  return 1;
      if (clceta < 0)  return 1;
      p3->pxx = a*p2a->pvv + b*p2b->pvv;
      p3->pyy = p3->pxx;
      p3->pzz = a*p2a->pww + b*p2b->pww;
      p3->lxx = a*p2a->lvv + b*p2b->lvv;
      p3->lyy = p3->lxx;
      p3->lzz = a*p2a->lww + b*p2b->lww;
      if (clceta == 0)  return 1;
      p3->exx = a*p2a->evv + b*p2b->evv;
      p3->eyy = p3->exx;
      p3->ezz = a*p2a->eww + b*p2b->eww;
      return 1;
      }
    }
  else {                                   /* 1-dimensionale Meteorologie */
    if ((k <= Nz) && (k > 0)) {
      memset(p3, 0, sizeof(MD3REC));       /* Zuerst alles auf 0 setzen   */
      a = 1 - pl->az;
      b = pl->az;
      if (PrfMode == 3) {                      /* Anisotrope Modellierung */
        p3b = (MD3REC*) AryPtr(Pma, k  );  if (!p3b)               eX(27);
        p3a = (MD3REC*) AryPtr(Pma, k-1);  if (!p3a)               eX(28);
        p3->vx = a*p3a->vx + b*p3b->vx;
        p3->vy = a*p3a->vy + b*p3b->vy;
        p3->vz = a*p3a->vz + b*p3b->vz;
        p3->tau = a*p3a->tau + b*p3b->tau;
        if (OnlyAdvec)  return 1;
        p3->wz = p3b->wz;
        p3->ths = p3b->ths;
        if (clceta < 0)  return 1;
        p3->pxx = a*p3a->pxx + b*p3b->pxx;
        p3->pxy = a*p3a->pxy + b*p3b->pxy;
        p3->pxz = a*p3a->pxz + b*p3b->pxz;
        p3->pyx = a*p3a->pyx + b*p3b->pyx;
        p3->pyy = a*p3a->pyy + b*p3b->pyy;
        p3->pyz = a*p3a->pyz + b*p3b->pyz;
        p3->pzx = a*p3a->pzx + b*p3b->pzx;
        p3->pzy = a*p3a->pzy + b*p3b->pzy;
        p3->pzz = a*p3a->pzz + b*p3b->pzz;
        p3->lxx = a*p3a->lxx + b*p3b->lxx;
        p3->lyx = a*p3a->lyx + b*p3b->lyx;
        p3->lyy = a*p3a->lyy + b*p3b->lyy;
        p3->lzx = a*p3a->lzx + b*p3b->lzx;
        p3->lzy = a*p3a->lzy + b*p3b->lzy;
        p3->lzz = a*p3a->lzz + b*p3b->lzz;
        if (clceta == 0)  return 1;
        p3->exx = a*p3a->exx + b*p3b->exx;
        p3->eyx = a*p3a->eyx + b*p3b->eyx;
        p3->eyy = a*p3a->eyy + b*p3b->eyy;
        p3->ezx = a*p3a->ezx + b*p3b->ezx;
        p3->ezy = a*p3a->ezy + b*p3b->ezy;
        p3->ezz = a*p3a->ezz + b*p3b->ezz;
        return 1; }
      else {                                      /* Isotrope Modellierung */
        p2b = (MD2REC*) AryPtr(Pma, k  );  if (!p2b)               eX(29);
        p2a = (MD2REC*) AryPtr(Pma, k-1);  if (!p2a)               eX(30);
        p3->vx = a*p2a->vx + b*p2b->vx;
        p3->vy = a*p2a->vy + b*p2b->vy;
        p3->vz = a*p2a->vz + b*p2b->vz;
        p3->tau = a*p2a->tau + b*p2b->tau;
        if (OnlyAdvec)  return 1;
        p3->wz  = p2b->wz;
        p3->ths = p2b->ths;
        if (clceta < 0)  return 1;
        p3->pxx = a*p2a->pvv + b*p2b->pvv;
        p3->pyy = p3->pxx;
        p3->pzz = a*p2a->pww + b*p2b->pww;
        p3->lxx = a*p2a->lvv + b*p2b->lvv;
        p3->lyy = p3->lxx;
        p3->lzz = a*p2a->lww + b*p2b->lww;
        if (clceta == 0)  return 1;
        p3->exx = a*p2a->evv + b*p2b->evv;
        p3->eyy = p3->exx;
        p3->ezz = a*p2a->eww + b*p2b->eww;
        return 1; }
      }
    else                                        eG(31);      /* k > Nz */
    }
  return 1;
eX_1:  eX_2:  eX_4:  eX_5:
eX_10: eX_11: eX_12: eX_13: eX_14: eX_15: eX_16: eX_17:
eX_20: eX_21: eX_22: eX_23: eX_24: eX_25: eX_26: eX_27: eX_28: eX_29:
eX_30: eX_31:
  eMSG(_loc_$_, pl->i, pl->j, pl->k, pl->is, pl->js, pl->ax, pl->ay, pl->az);
  }

//================================================================= Try2Escape
//
static int Try2Escape(  /* In einem Gebaeude erzeugte Partikel herausdraengen */
PTLREC *pa,
LOCREC *pl ) {
  dP(WRK:Try2Escape);
  double x = pa->x;                                  // Anfangs-Position merken
  double y = pa->y;
  double h = pa->h;
  double d = 1;
  int ii[9] = { 0,  1, -1,  0,  0,  1,  1, -1, -1 };
  int jj[9] = { 0,  0,  0,  1, -1,  1, -1,  1, -1 };
  int n, i, k, kk;
  for (n=1; n<=10; n++) {                            // 10 Distanzen pruefen
    d = n*0.1*Dd;
    for (kk=0; kk<=2; kk++) {
      k = (kk == 2) ? -1 : kk;
      pa->h = h + k*d;
      for (i=(k==0); i<9; i++) {
        pa->x = x + ii[i]*d;
        pa->y = y + jj[i]*d;
        ClcPtlLoc(NULL, pa, pl, WRK_USE_H);  eG(2);  // neu einordnen
        if (!INSOLID(pl->i,pl->j,pl->k)) {           // geschafft!
          d = 0;
          break;
        }
      } // for i
      if (d == 0)  break;
    }
    if (d == 0)  break;
  }
  if (d > 0)                                          eX(3);
  return 0;
eX_2: eMSG(_cant_locate_);  //-2004-12-23
eX_3: eMSG(_cant_shift_); //-2004-12-23
}

//===================================================================== RunPtl
//
static int RunPtl(
void )
  {
  dP(RunPtl);
  long rc;
  static int totals = 0;
  static int steps = 0;
  static int retries = 0;
  PTLREC *pp, *pa, *pe;
#define  a  (*pa)
#define  e  (*pe)
  LOCREC l, ll;
  MD3REC m, mm;
  PPMREC *pr;
  SRCREC *ps;
  float dg, gsum, r1, r2, r3, vsed, vdep, *pf, *pw, dafu;
  float wdep, v2, tau, vred, hred, dzp, afufac;
  float wdeps[60];
  double tp;
  int handle, retry;
  int icmp, jcmp, cmpnum, cmpoff, nzdos, np, kk, k, srcind;
  int deposition, discard, created, wdep_unknown;
  int washout, dilute_plume;                                      //-2011-11-21
  int predict, predicting;                                        //-2001-09-22
  //
  vLOG(5)("WRK:RunPtl()  starting");
  pa = ALLOC(PtlRecSize);  if (!pa)                  eX(30);
  pe = ALLOC(PtlRecSize);  if (!pe)                  eX(30);
  memset(&l, 0, sizeof(l));            /* Strukturen zu 0 initialisieren */
  memset(&m, 0, sizeof(m));
  memset(&e, 0, PtlRecSize);
  nzdos = Pda->bound[2].hgh;
  handle = PtlStart(Ppa);                               eG(1);
  NumRun = 0;
  predict = (NoPredict) ? 0 : (bDdmet != 0);                      //-2001-09-22
  dilute_plume = ((MI.flags & FLG_NODILUTE) == 0);                //-2011-11-21
  for (np=0; ; ) {  /*+++++++++++++++++ Fuer alle Teilchen: ++++++++++++++++*/
    pp = PtlNext(handle);            /* Neues Teilchen anfordern           */
    if (pp == NULL)  break;          /* aufhoeren, wenn keins mehr da ist   */
    totals++;
    if (pp->flag & SRC_REMOVE)       /* naechstes nehmen, wenn aussortiert  */
      continue;
    if (!Valid) {                     // Keine gueltige Meteorologie:
      pp->flag |= SRC_REMOVE;         // Alle vorhandenen Partikel werden
      NumRemove++;                    // geloescht.
      continue;
    }
    memcpy(&a, pp, PtlRecSize);      /* umspeichern                        */
    if (a.t >= T2)   continue;       /* naechstes nehmen, wenn abgearbeitet */
    if (a.t < T1)           eX(2);
    tp = a.t;                     /* Zeit separat merken */
    created = (a.flag & SRC_CREATED) != 0;  /* neu erzeugt, falls Flag gesetzt  */ //-2003-02-21
    a.flag = SRC_EXIST;
    WrkRndSetSeed(a.rnd);         // Zufallszahlen initialisieren                 //-2003-02-21
    cmpoff = a.offcmp;            /* Komponenten-Offset im Dosis-Feld      */
    cmpnum = a.numcmp;            /* Zahl der Komponenten                  */
    srcind = a.srcind - 1;        /* Index der zugehoerigen Quelle          */
    ps = (SRCREC*) AryPtr(Psa, srcind);    // Pointer auf die zugehoerige Quelle
    if (!ps)              eX(13);
    afufac = ps->fr;              // Faktor bei Reflektion
    pr = (PPMREC*) Ppp->start + cmpoff;
    deposition = 0;
    washout = 0;                                                  //-2011-11-21
    for (icmp=0; icmp<cmpnum; icmp++) {         // dry or wet deposition?
      if (pr[icmp].wdep > 0)  deposition = 1;
      if (pr[icmp].rwsh > 0)  washout = 1;                        //-2011-11-21
    }
    if (Pwa)  deposition = 1;
    wdep_unknown = 0;
    if (a.vg > 0) {               /* Falls individuelles Vsed              */
      vsed = a.vg;
      deposition = 1;
      wdep_unknown = 1;
      }
    else  vsed = pr->vsed;        /* Sedimentations-Geschwindigkeit        */
    vred = pr->vred;              /* Reduktion der vert. Startgeschwindigk.*/
    hred = pr->hred;              /* Reduktion der hor. Startgeschwindigk. */
    np++;                         /* Teilchenzaehler hochzaehlen ...         */
    NumRun = np;                  /* ... und extern zur Verfuegung stellen  */
    ClcPtlLoc(NULL, &a, &l, WRK_USE_H);  eG(3);   /* Teilchen einordnen    */
    if (a.flag & SRC_EOUT) {      /* Falls ausserhalb des Netzes, ...       */
      if (created) a.flag |= SRC_CREATED; //-2005-01-23
      pp->flag = a.flag;          /* ... vermerken und naechstes nehmen     */
      continue; }
    if (INSOLID(l.i,l.j,l.k)) {
      if (created) {
        Try2Escape(&a, &l);                            eG(14); //-2004-10-10
      }
      else {
        Try2Escape(&a, &l);                            eG(15); //-2004-12-09
      }
    }
    kk = (a.z > Zref) ? -1 : created;           /* Oberhalb Zref ?       */
    ClcLocMod(&l, &m, kk);  eG(4);              /* Felder interpolieren  */
    if (created) {                /* Bei einem neuen Teilchen ...        */
      r1 = WrkRnd();              /* ... Geschwindigkeiten initalisieren */
      r2 = WrkRnd();
      r3 = WrkRnd();
      vred = pr->vred;            /* Reduktion der vert. Startgeschwindigk.*/
      hred = pr->hred;            /* Reduktion der hor. Startgeschwindigk. */
      a.u = hred*m.exx*r1;
      a.v = hred*(m.eyx*r1 + m.eyy*r2);
      a.w = vred*(m.ezx*r1 + m.ezy*r2 + m.ezz*r3);
      if (!ExactTau)  m.tau *= 0.5 + WrkRnd01();            //-2001-09-23
      v2 = m.vx*m.vx + m.vy*m.vy;
      ClcAfu(ps, v2, Stab, &a.afuhgt, &a.afutsc);                   eG(5);
      if (a.afutsc > 0) {
        float vs;
        vs = a.afuhgt/a.afutsc;
        ClcAfuRxyz(ps, &vs, &a.afurx, &a.afury, &a.afurz);
        a.afuhgt = vs*a.afutsc;
      }
      else {
        a.afurx = 0;  a.afury = 0;  a.afurz = 1; }
    }
    retry = 0;                    /* Zaehler fuer Schrittversuche setzen   */
    memcpy(&e, &a, PtlRecSize);   /* Alles uebernehmen (Massen g[] !)     */
    predicting = predict;
    while (tp < T2) {   /*---------- Zeitschritte durchfuehren -----------*/
      tau = m.tau;
      if (predicting) {                   //-2001-09-22
        predicting = 0;
        e.x = a.x + tau*m.vx;             // Nur horizontale ...
        e.y = a.y + tau*m.vy;             // ... Advektion durchfuehren
        e.z = a.z;
        ll = l;
        while (1) {
          ClcPtlLoc(&a, &e, &ll, WRK_USE_AZ);  eG(6);  // Position berechnen
          if (e.flag & SRC_EOUT)
            break;
          e.z += tau*(m.vz - vsed);             // Vertikale Advektion
          ClcPtlLoc(&a, &e, &ll, 0);  eG(9);    // Wieder Position berechnen
          if (e.flag & SRC_EOUT)
            break;
          rc = ClcLocMod(&ll, &mm, -1); eG(7);  // vx, vy, vz berechnen
          if (rc <= 0)                          // in verbotenem Gebiet
            break;
          m.vx = 0.5*(m.vx + mm.vx);
          m.vy = 0.5*(m.vy + mm.vy);
          m.vz = 0.5*(m.vz + mm.vz);
          break;
        }
        memcpy(&e, &a, PtlRecSize);             // Position zuruecksetzen
        continue;
      }
      steps++;
      if (TrcMode) trcPtl(np,&e,tp,m);
      CountStep++;
      if (!ShowProgress && CountStep>=CountMax) {
        CountStep = 0;
        CountPhase++;
        if (CountPhase >= 4)  CountPhase = 0;
        printf("%c\b", PhaseSymbols[CountPhase]);  fflush(stdout);
      }
      e.refl = 0;
      if (a.z > Zref) {                    /* Oberhalb Zref ...          */
        e.u = 0;  e.v = 0;  e.w = 0; }     /* ... ohne Diffusion rechnen */
      else {
        r1 = WrkRnd();
        r2 = WrkRnd();
        r3 = WrkRnd();
        e.u = m.pxx*a.u + m.pxy*a.v + m.pxz*a.w
              + m.wx + m.lxx*r1;
        e.v = m.pyx*a.u + m.pyy*a.v + m.pyz*a.w
              + m.wy + m.lyx*r1 + m.lyy*r2;
        e.w = m.pzx*a.u + m.pzy*a.v + m.pzz*a.w
              + m.wz + m.lzx*r1 + m.lzy*r2 + m.lzz*r3;
        }
      e.x = a.x + tau*m.vx;             /* Nur horizontale ...          */
      e.y = a.y + tau*m.vy;             /* ... Advektion durchfuehren    */
      e.z = a.z;
      dzp = tau*(m.vz + e.w - vsed);    /* Vertikalen Transport merken  */
      if (retry > MinRetry && vsed > 0) dzp += tau*vsed;        //-2005-02-24
      if (e.afuhgt > 0) {               /* Abgasfahnen-Ueberhoeung       */
        if ((e.afutsc <= m.tau) || (ABS(e.afuhgt) < 1.00)) {
          dafu = e.afuhgt;
          dzp += dafu*e.afurz;                   /* Rest aufaddieren */
          e.y += dafu*e.afury;
          e.x += dafu*e.afurx;
          e.afuhgt = 0; }
        else {                                 /* Teil aufaddieren */
          dafu = e.afuhgt*tau/e.afutsc;
          dzp += dafu*e.afurz;                   /* im z-System */
          e.y += dafu*e.afury;
          e.x += dafu*e.afurx;
          e.afuhgt -= dafu; }
        }
      if (retry == 0) {                     /* Beim ersten Durchlauf ...    */
        ll = l;                             /* alte gueltige Position merken */
        e.flag = SRC_EXIST;                 /* ... und Flags zuruecksetzen.  */
        ClcPtlLoc(&a, &e, &l, WRK_USE_AZ);  eG(6);  /* Position berechnen   */
        if (e.flag & SRC_EOUT) {            /* Rechengebiet verlassen,      */
          a.flag = e.flag;                  /* vermerken ...                */
          break; }                          /* ... und aufhoeren             */
        if (INSOLID(l.i,l.j,l.k)) {         /* Innerhalb eines Koerpers ?    */
          e.x = a.x;  e.y = a.y;            /* Ort zuruecksetzen             */
          l = ll;                           /* Alte Position uebernehmen     */
          m.vx = 0;  m.vy = 0;              /* Advektion ausschalten        */
          }
        }
      e.x += tau*e.u;                       /* turbulenter Transport        */
      e.y += tau*e.v;
      e.z += dzp;
      ClcPtlLoc(&a, &e, &l, 0);  eG(9);     /* Wieder Position berechnen    */
      if (e.flag & SRC_EOUT) {              /* Rechengebiet verlassen,      */
        a.flag = e.flag;                    /* vermerken ...                */
        break; }                            /* ... und aufhoeren             */
      kk = (e.z > Zref) ? -1 : 0;
      if (INSOLID(l.i,l.j,l.k))  rc = 0;    /* Innerhalb eines Koerpers ?    */
      else {
        rc = ClcLocMod(&l, &m, kk);  eG(7);      /* Felder interpolieren    */
        }
      if (rc == 0) {                             /* In verbotenem Gebiet    */
        retry++;                                 /* Anzahl Versuche pruefen  */
        retries++;
        if (retry > MaxRetry) {
          a.flag |= SRC_REMOVE;
          fprintf(MsgFile,
            "*** %5d: %6.2f (%5.3lf,%5.3lf,%5.3lf) (%5.3f,%5.3f,%5.3f)"
            " F(%5.3f,%5.3f,%5.3f)\n",
            np, m.tau, a.x, a.y, a.z, a.u, a.v, a.w, m.vx, m.vy, m.vz);
          break; }
        if (ll.k > l.k) e.refl |= WRK_REFB; /* Kommt von oben : Deposition ! */
        if (retry < MinRetry) {             /* Bei den ersten Malen          */
          a.u=-a.u; a.v=-a.v; a.w=-a.w; }   /* ... Teilchen umdrehen,        */
        else {                              /* sonst                         */
          a.u = 0;  a.v = 0;  a.w = 0;      /* Eigengeschwindigkeit loeschen, */
          m.wx = 0; m.wy = 0; m.wz = 0;     /* Driftgeschwindigkeit loeschen, */
          m.vx = 0; m.vy = 0; m.vz = 0; }   /* Windgeschwindigkeit loeschen,  */
        continue;                           /* ... und noch einmal versuchen.*/
      }
      //-2011-10-13
      if (Pxfr) {                           // Chemical reactions
        XFRREC *prec;
        int ir, ixfr;
        for (icmp=0; icmp<cmpnum; icmp++) { // for all components:
          ir = icmp + cmpoff;               // get the row index
          ixfr = Ixfr[ir];                  // get the first list index
          if (ixfr < 0)                     // no reactions for this substance
            continue;
          gsum = a.g[2*icmp];               // get the old mass
          while (ixfr < Nxfr) {             // scan the reaction list
            prec = Pxfr + ixfr;             // get a reaction record
            if (prec->ir != ir)             // check the row index
              break;
            jcmp = prec->ic - cmpoff;       // index of reaction partner
            if (jcmp < 0 || jcmp >= cmpnum)                           eX(8);
            gsum += tau*a.g[2*jcmp]*prec->val; // add contribution
            ixfr++;
          }
          if (gsum < 0)                                              eX(16);                               
          e.g[2*icmp] = gsum;               // ... new mass
        }
      }
      //
      if (e.refl & (WRK_REFB | WRK_REFT)) { /* Teilchen wurde reflektiert    */
        if (retry == 0) {                   /* Falls am Boden reflektiert,   */
          e.u = -e.u;                       /* alle Komponenten umkehren.    */
          e.v = -e.v;
          e.w = -e.w /* - 2*m.wz */;
        }
        if (e.afuhgt > 0) {
            if (afufac >= 0)  e.afuhgt *= afufac;
            else {
                e.afuhgt *= -afufac;
                e.afurz *= -1;
            }
        }
        if (deposition && e.refl&WRK_REFB && l.i>0 && l.j>0) {
          k = 0;
          pf = AryPtr(Pda, l.i, l.j, k, cmpoff);  if (!pf)              eX(20);
          if (Pwa) {
            pw = AryPtr(Pwa, l.i, l.j, cmpoff);  if (!pw)               eX(21);
            }
          if (wdep_unknown) {
            ClcLocMod(&l, &m, 1);   eG(7);              /* ezz berechnen */
            for (icmp=0; icmp<cmpnum; icmp++) {
              if (Pwa)  vdep = 0.8*m.ezz*pw[icmp]/(2 - pw[icmp]);
              else  vdep = pr[icmp].vdep;
              PpmClcWdep(m.ezz, vdep+vsed, vsed, wdeps+icmp);
              }
            wdep_unknown = 0;
            }
          for (icmp=0; icmp<cmpnum; icmp++) { /* Fuer alle Komponenten:   */
            if (e.vg > 0)  wdep = wdeps[icmp];
            else  wdep = (Pwa) ? pw[icmp] : pr[icmp].wdep;
            if (wdep < 0)                                                eX(55); //-2013-07-08
            if (wdep > 1)  wdep = 1;
            dg = e.g[2*icmp]*wdep;            /* abgelagerte Masse ...   */
            e.g[2*icmp] -= dg;                /* fehlt beim Teilchen ... */
            pf[icmp]  += dg; }                /* und liegt am Boden.     */
          }
        }
      if (l.i>0 && l.j>0 && l.k>0 && l.k<=nzdos) {
        pf = AryPtr(Pda, l.i, l.j, l.k, cmpoff);  if (!pf)               eX(12);
        for (icmp=0; icmp<cmpnum; icmp++)     /* Fuer alle Komponenten ... */
          pf[icmp] += e.g[2*icmp]*tau;        /* ... Dosis aufaddieren    */
      }
      //-------------------------------------------------------------2011-11-21
      if (washout && l.i>0 && l.j>0) {                          /* Washout */
        k = -1;
        pf = AryPtr(Pda, l.i, l.j, k, cmpoff);  if (!pf)                eX(10);
        for (icmp=0; icmp<cmpnum; icmp++) {
          float g = e.g[2*icmp];
          float fac;                                              //-2011-10-17
          fac = pr[icmp].rwsh*tau;
          if (fac > 1)  fac = 1;                                  //-2011-10-17
          dg = g*fac;                                             //-2010-10-17
          if (dilute_plume) {
            g -= dg;
            e.g[2*icmp] = g;
          }
          pf[icmp]  += dg;
        }
      }
      //-----------------------------------------------------------------------
      discard = 1;                          // Noch weiter brauchbar?
      for (icmp=0; icmp<cmpnum; icmp++) {   // Alle Komponenten pruefen
        if (pr[icmp].mmin == 0                                    //-2012-04-09
        		|| e.g[2*icmp] > pr[icmp].mmin*e.g[2*icmp+1])
          discard = 0;                      // Nicht abgereicherte weiter verwenden
      }
      if (discard) {                        // Leeres Teilchen als nicht ...
        a.flag |= SRC_REMOVE;               // ... mehr verwendbar markieren.
        Ndsc++;
        break; }                            /* und abbrechen.           */
      retry = 0;                         /* Versuchs-Zaehler zuruecksetzen  */
      tp += tau;                         /* Zeit weitersetzen    */
      memcpy(&a, &e, PtlRecSize);        /* Anfangswerte setzen  */
      a.flag = SRC_EXIST;                /* Flags zuruecksetzen   */
      a.refl = 0;
      predicting = predict;
      }  /*-------------------------------- Ende der Zeitschleife ---------*/
    a.rnd = WrkRndGetSeed();             /* Zufallszahl merken   */
    a.t = tp + 0.5;                      /* Endzeit eintragen (gerundet)   */
    memcpy(pp, &a, PtlRecSize);          /* Werte in der Tabelle eintragen */
    if (TrcMode) trcPtl(np,&a,tp,m);
    }  /*++++++++++++++++++++++++++++++++++ Teilchen fertig +++++++++++++++*/
  PtlEnd(handle);
  FREE(pa);
  FREE(pe);
  vLOG(5)("WRK:RunPtl()  returning, n=%d", NumRun);
  return NumRun;
eX_30:
  eMSG(_no_memory_);
eX_1:
  eMSG(_no_handle_);
eX_2:
  if (MsgFile != NULL)
    fprintf(MsgFile, _current_interval_$$_, T1, T2);
  prnPtl(MsgFile, "PTL", np+1, &a, (double)T1);     //-2004-01-12
  eMSG(_forgotten_particle_);
eX_3:  eX_6:  eX_9:
  prnPtl(MsgFile, "PTL", np, &a, (double)T1);       //-2004-01-12
  eMSG(_invalid_position_);
eX_4:  eX_7:
  eMSG(_cant_interpolate_);
eX_5:
  eMSG(_no_plume_rise_);
eX_8:
  eMSG(_no_transfer_data_);
eX_10:
eX_12:
eX_20:
  eMSG(_index_$$$_, l.i, l.j, k);
eX_21:
  eMSG(_index_$$_, l.i, l.j);
eX_13:
  eMSG(_invalid_source_count_$_, srcind);
eX_14:
  eMSG(_inside_building_$$$_, pp->x, pp->y, pp->h);
eX_15:
  prnPtl(stdout, "PTL", np, &a, (double)T1);
  eMSG(_moved_inside_);
eX_16:
  eMSG(_chemical_reactions_too_fast_);
eX_55:
		eMSG(_negative_wdep_);
}
#undef  a
#undef  e

/*================================================================= SetSimParm
*/
static void show_progress( long t ) {                           //-2001-07-10
  long p;
  p = (1000.0*(t-MI.start))/(MI.end-MI.start);
  if (p != Progress) {
    printf(_progress_$_, 0.1*p);  //!!!!!!!!!!!!!!!!!!!!!!!!!
    fflush(stdout);
    Progress = p;
  }
}

static long SetSimParm(      /* Simulations-Parameter festlegen.            */
  void )
  {
  dP(SetSimParm);
  int modlen, i, j, k;
  long igp, ima, ipi, iga, isa, ida, ipp, ixa, iwa, usage, ibn;
  ARYDSC *pa;
  char name[256], t1s[40], t2s[40];			//-2004-11-26
  TXTSTR hdr = { NULL, 0 };
  strcpy(t1s, TmString(&T1));
  strcpy(t2s, TmString(&T2));
  vLOG(5)("WRK:SetSimParm()  [%s,%s] ", t1s, t2s);
  TrcFile = MsgFile;
  strcpy(name, NmsName(StdIdent));
  igp = IDENT(GRDpar, 0, Gl, Gi);
  pa =  TmnAttach(igp, NULL, NULL, 0, NULL);                    eG(1);
  if (!pa)                                                      eX(2);
  Pgp = (GRDPARM*) pa->start;
  Nx = Pgp->nx;
  Ny = Pgp->ny;
  Nz = Pgp->nz;
  if (MI.flags & FLG_GAMMA)  Nzm = MIN(Pgp->nzmap, Nz);
  else  Nzm = MIN(Pgp->nzdos, Nz);
  Dd = Pgp->dd;
  Xmin = Pgp->xmin;
  Gx   = Nx*Dd;
  Xmax = Xmin + Gx;
  Ymin = Pgp->ymin;
  Gy   = Ny*Dd;
  Ymax = Ymin + Gy;
  bPerx = (0 != (Pgp->bd & GRD_PERX));
  if (bPerx) {
    Xbmin = Xmin;
    Xbmax = Xmax; }
  else {
    Xbmin = Xmin - Pgp->border;
    Xbmax = Xmax + Pgp->border; }
  bPery = (0 != (Pgp->bd & GRD_PERY));
  if (bPery) {
    Ybmin = Ymin;
    Ybmax = Ymax; }
  else {
    Ybmin = Ymin - Pgp->border;
    Ybmax = Ymax + Pgp->border; }
  DefMode = Pgp->defmode;
  PrfMode = Pgp->prfmode;
  switch (PrfMode) {
    case 2:   modlen = sizeof(MD2REC);
              break;
    case 3:   modlen = sizeof(MD3REC);
              break;
    default:                                                    eX(20);
    }
  Zscl = Pgp->zscl;
  //-2005-01-12
  if (Zscl != 0)                                                eX(90);
  Zref = Pgp->hmax;
  iga = IDENT(GRDarr, 0, 0, 0);
  pa =  TmnAttach(iga, NULL, NULL, 0, NULL);                    eG(4);
  if (!pa)                                                      eX(5);
  Sk = (float*) pa->start;
  TmnDetach(iga, NULL, NULL, 0, NULL);                          eG(6);

  ipi = IDENT(PRMpar, 0, 0, 0);
  PrmT2 = T1;
  pa = TmnAttach(ipi, &T1, &PrmT2, 0, NULL);                    eG(7);
  if (!pa)                                                      eX(8);
  if (PrmT2 < T2)  T2 = PrmT2;
  if (T2 <= T1)                                                 eX(41);
  if (ShowProgress)  show_progress(T1);
  Ppi = (PRMINF*) pa->start;
  Nc = Ppi->sumcmp;
  Flags = Ppi->flags;
  TrcMode =   (0 != (Flags & FLG_TRACING));                 //-2002-01-05 lj
  OnlyAdvec = (0 != (Flags & FLG_ONLYADVEC));
  NoPredict = (0 != (Flags & FLG_NOPREDICT));
  ExactTau =  (0 != (Flags & FLG_EXACTTAU));
  TmnDetach(ipi, NULL, NULL, 0, NULL);                          eG(9);

  ModT2 = T1;
  ima = IDENT(MODarr, 0, Gl, Gi);
  Pma = TmnAttach(ima, &T1, &ModT2, 0, &hdr);                   eG(10);  //-2011-06-29
  if (!Pma)                                                     eX(11);
  bDdmet= (Pma->numdm == 3);
  if (bDdmet) {
    AryAssert(Pma, modlen, 3, 0, Nx, 0, Ny, 0, Nz);             eG(22);
    DmnDatAssert("Delta|Dd|Delt", hdr.s, Dd);                   eG(23); //-2011-06-29
    DmnDatAssert("Xmin", hdr.s, Xmin);                          eG(24); //-2011-06-29
    DmnDatAssert("Ymin", hdr.s, Ymin);                          eG(25); //-2011-06-29
    ibn = IDENT(GRDarr, GRD_ISLD, Gl, Gi);
    Pbn = TmnAttach(ibn, NULL, NULL, TMN_MODIFY, NULL);  if (!Pbn)  eX(50);
    for (i=1; i<=Nx; i++)
      for (j=1; j<=Ny; j++)
        for (k=1; k<=Nz; k++)
          if (((MD3REC*)AryPtrX(Pma,i,j,k-1))->vz <= WND_VOLOUT)
            *(long*)AryPtrX(Pbn,i,j,k) |= GRD_VOLOUT;
    }
  else {
    AryAssert(Pma, modlen, 1, 0, Nz);                           eG(26);
    Pbn = NULL;
    }
  TmnInfo(ima, NULL, NULL, &usage, NULL, NULL);               //-2001-06-29
  Valid = (0 == (usage & TMN_INVALID));                       //-2001-06-29
  if (!Valid && DropT1 != T1) {                               //-2011-07-04
    InterDrop += (T2 - T1);
    AverDrop += (T2 - T1);
    TotalDrop += (T2 - T1);
    DropT1 = T1;
  }
  DmnGetFloat(hdr.s, "KL|KM", "%f", &Stab, 1);
  Hm = 0;
  DmnGetFloat(hdr.s, "HM",  "%f", &Hm, 1);
  if (bDdmet) {
    if ( (Pgp->sscl == Sk[Nz])
      && (Pgp->bd & GRD_REFZ)
      && (Zref == Zscl)
      && (Zscl > 0));
    else  Zref = 9999;
  }
  else {
    if (Zref <= 0) {
      if (Pgp->bd & GRD_REFZ)  Zref = Hm;
      if (Zref <= 0)  Zref = 9999;
      }
    else {
      if (Zref<Hm && 0==(Pgp->bd&GRD_REFZ))  Zref = Hm;
      }
    if (Zref <= Pgp->zmax)                                      eX(51);
  }
  TmnDetach(igp, NULL, NULL, 0, NULL);                          eG(3);
  if (ModT2 < T2)  T2 = ModT2;
  if (T2 <= T1)                                                 eX(42);
  PpmT2 = T1;
  ipp = IDENT(PPMarr, 0, Gl, Gi);
  Ppp = TmnAttach(ipp, &T1, &PpmT2, 0, NULL);                   eG(12);
  if (!Ppp)                                                     eX(13);
  if (PpmT2 < T2)  T2 = PpmT2;
  if (T2 <= T1)                                                 eX(43);
  if (bDdmet) {
    WdpT2 = T1;
    iwa = IDENT(WDParr, 0, Gl, Gi);
    Pwa = TmnAttach(iwa, &T1, &WdpT2, 0, &hdr);                 eG(16);
    if (!Pwa)                                                   eX(17);
    if (WdpT2 < T2)  T2 = WdpT2;
    if (T2 <= T1)                                               eX(44);
    AryAssert(Pwa, sizeof(float), 3, 1, Nx, 1, Ny, 0, Nc-1);    eG(27);
    DmnDatAssert("Delta|Delt|Dd", hdr.s, Dd);                   eG(28); //-2011-06-29
    DmnDatAssert("Xmin", hdr.s, Xmin);                          eG(29); //-2011-06-29
    DmnDatAssert("Ymin", hdr.s, Ymin);                          eG(30); //-2011-06-29
  }
  else  Pwa = NULL;
  if (Flags & FLG_CHEM) {
    float *pf;
    XfrT2 = T1;      
    ixa = IDENT(CHMarr, 0, 0, 0);                     //-2001-04-25 uj
    Pxa = TmnAttach(ixa, &T1, &XfrT2, 0, NULL);                 eG(31);
    if (!Pxa)                                                   eX(32);
    if (XfrT2 < T2)  T2 = XfrT2;
    if (T2 <= T1)                                               eX(45);
    AryAssert(Pxa, sizeof(float), 2, 0, Nc-1, 0, Nc-1);         eG(33);
    //                                                              -2011-10-13
    pf = (float*)Pxa->start;
    Nxfr = 0;
    for (i=0; i<Nc; i++)
      for (j=0; j<Nc; j++) {
        if (*pf)
          Nxfr++;
        pf++;
      }
    if (Nxfr > 0) {
      XFRREC *pxfr;
      int nr=0;
      Pxfr = ALLOC(Nxfr*sizeof(XFRREC));
      Ixfr = ALLOC(Nc*sizeof(int));
      pxfr = Pxfr;
      pf = (float*)Pxa->start;
      for (i=0; i<Nc; i++) {
        Ixfr[i] = -1;
        for (j=0; j<Nc; j++) {
          if (*pf) {
            pxfr->ir = i;
            pxfr->ic = j;
            pxfr->val = *pf;
            if (Ixfr[i] < 0)
              Ixfr[i] = nr;
            pxfr++;
            nr++;
          }
          pf++;
        }
      }
    }
  }
  else {
    Pxa = NULL;
    Pxfr = NULL;
    Nxfr = 0;
  }
  SrcT2 = T1;
  isa = IDENT(SRCtab, 0, 0, 0);
  Psa = TmnAttach(isa, &T1, &SrcT2, 0, NULL);                   eG(34);
  if (!Psa)                                                     eX(35);
  if (SrcT2 < T2)  T2 = SrcT2;
  if (T2 <= T1)                                                 eX(46);
  ida = IDENT(DOSarr, Gp, Gl, Gi);
  strcpy(name, NmsName(ida));
  Pda = NULL;
  if (TmnInfo(ida, &DosT1, &DosT2, &usage, NULL, NULL)) {
    if (usage & TMN_DEFINED) {
      if (DosT1>T1 || DosT2<T1)                                 eX(38);
      Pda = TmnAttach(ida, NULL, NULL, TMN_MODIFY, NULL);       eG(36);
      if (!Pda)                                                 eX(39);
      }
    }
  if (!Pda) {
    Pda = TmnCreate(ida, sizeof(float),
             4, 1, Nx, 1, Ny, -1, Nzm, 0, Nc-1);                eG(37);
    DosT1 = T1;
    DosT2 = T2;
    }
  TxtClr(&hdr);
  if (TrcMode)  trcInit(Gp, Gl, Gi, T1, T2);
  NumRemove = 0;
  Ndsc = 0;
  vLOG(5)("WRK:SetSimParm()  returning");
  return 0;
eX_1:  eX_2:  eX_3:
  eMSG(_no_grid_data_);
eX_4:  eX_5:  eX_6:
  eMSG(_no_vertical_grid_);
eX_7:  eX_8:  eX_9:
  eMSG(_no_general_parameters_);
eX_10: eX_11:
  strcpy(name, NmsName(ima));
  eMSG(_no_model_field_$_, name);
eX_12: eX_13:
  strcpy(name, NmsName(ipp));
  eMSG(_no_particle_parameters_$_, name);
eX_16: eX_17:
  strcpy(name, NmsName(iwa));
  eMSG(_no_depo_prob_$_, name);
eX_20:
  eMSG(_invalid_mode_$_, PrfMode);
eX_22: eX_23: eX_24: eX_25: eX_26:
  strcpy(name, NmsName(ima));
  eMSG(_improper_model_$_, name);
eX_28: eX_29: eX_30:
  if (hdr.s)  nMSG("hdr=%s<<<", hdr.s);
eX_27:
  strcpy(name, NmsName(iwa));
  eMSG(_improper_structure_$_, name);
eX_31: eX_32:
  strcpy(name, NmsName(ixa));
  eMSG(_no_transfer_$_, name);
eX_33:
  strcpy(name, NmsName(iwa));
  eMSG(_improper_transfer_$_, name);
eX_34: eX_35:
  eMSG(_cant_get_plume_rise_);
eX_36: eX_37: eX_39:
  eMSG(_no_dose_$_, name);
eX_38:
  strcpy(t1s, TmString(&T1));
  strcpy(t2s, TmString(&T2));
  eMSG(_dose_$$$_parm_$$_, name, t1s, TmString(&DosT2), t1s, t2s);
eX_41: eX_42: eX_43: eX_44: eX_45: eX_46:
  strcpy(t1s, TmString(&T1));
  strcpy(t2s, TmString(&T2));
  eMSG(_invalid_interval_$$_, t1s, t2s);
eX_50:
  eMSG(_no_boundaries_);
eX_51:
  eMSG(_reflection_$_$$$_, Zref, Gl, Gi, Pgp->zmax);
eX_90:
  eMSG(_zscl_sscl_0_);
  }

//================================================================ FreeSimParm
//
static long FreeSimParm(      /* Simulations-Parameter freigeben.        */
  void )
  {
  dP(FreeSimParm);
  long mode;
  TmnDetach(IDENT(MODarr, 0, Gl, Gi), NULL, NULL, 0, NULL);             eG(41);
  Pma = NULL;
  TmnDetach(IDENT(PPMarr, 0, Gl, Gi), NULL, NULL, 0, NULL);             eG(42);
  Ppp = NULL;
  TmnDetach(IDENT(SRCtab, 0,  0,  0), NULL, NULL, 0, NULL);             eG(43);
  Psa = NULL;
  if (Pwa) {
    TmnDetach(IDENT(WDParr, 0, Gl, Gi), NULL, NULL, 0, NULL);           eG(44);
    Pwa = NULL;
    }
  if (Pxa) {
    TmnDetach(IDENT(CHMarr, 0,  0,  0), NULL, NULL, 0, NULL);           eG(45);
    Pxa = NULL;
    if (Pxfr) {
      FREE(Pxfr);                                                 //-2011-10-13
      Pxfr = NULL;
    }
    if (Ixfr) {
      FREE(Ixfr);                                                 //-2011-10-13
      Ixfr = NULL;
    }
  }
  mode = TMN_MODIFY | ((Valid) ? TMN_SETVALID : TMN_INVALID);     //-2001-06-29
  TmnDetach(IDENT(DOSarr, Gp, Gl, Gi), &DosT1, &T2, mode, NULL);        eG(46);
  Pda = NULL;
  if (Pbn) {
    TmnDetach(IDENT(GRDarr, GRD_ISLD, Gl, Gi), NULL, NULL, TMN_MODIFY, NULL);    eG(47);
    Pbn = NULL;
    }
  if (NumRemove)
    vLOG(4)("%d particles from inside bodies removed", NumRemove);
  if (Ndsc)
    vLOG(4)("%d particles discarded", Ndsc);
  return 0;
eX_41: eX_42: eX_43: eX_44: eX_45: eX_46: eX_47:
  eMSG(_cant_detach_);
  }

//================================================================== WrkHeader
//
#define CAT(a,b)  {TxtPrintf(&txt,(a),(b)); TxtCat(&hdr,txt.s);}
char *WrkHeader(                /* the header (global storage)  */
  long id,                      /* identification               */
  long *pt1,                    /* start of the validity time   */
  long *pt2 )                   /* end of validity time         */
  {
  dP(WrkHeader);
  char name[256], t1s[40], t2s[40], dts[40], format[80], vldf[40];  //-2008-12-04 //-2011-04-13
  int gl, gi, iga, igp, k, ict, nz, nc, seq;
  char atype;
  float *sk;
  ARYDSC *pa, *pct, *pga;
  GRDPARM *pgp;
  CMPREC *pcmp;
  enum DATA_TYPE dt;
  TXTSTR txt = { NULL, 0 };
  TXTSTR hdr = { NULL, 0 };
  strcpy(name, NmsName(id));
  dt = XTR_DTYPE(id);
  gl = XTR_LEVEL(id);
  gi = XTR_GRIDN(id);
  switch (dt) {
    case DOSarr: atype = 'D';
                 break;
    case SUMarr: atype = 'D';
                 break;
    case DOStab: atype = 'D';
                 break;
    case CONtab: atype = 'C';
                 break;
    case GAMarr: atype = 'G';
                 break;
    default:     atype = '?';
    }
  igp = IDENT(GRDpar, 0, gl, gi);
  pa =  TmnAttach(igp, NULL, NULL, 0, NULL);                    eG(1);
  if (!pa)                                                      eX(2);
  pgp = (GRDPARM*) pa->start;
  iga = IDENT(GRDarr, 0, 0, 0);
  pga =  TmnAttach(iga, NULL, NULL, 0, NULL);                   eG(3);
  if (!pga)                                                     eX(4);
  sk = (float*) pga->start;
  nz = pga->bound[0].hgh + 1;
  ict = IDENT(CMPtab, 0, 0, 0);
  pct = TmnAttach(ict, NULL, NULL, 0, NULL);                    eG(5);
  if (!pct)                                                     eX(6);
  pcmp = pct->start;
  nc = pct->bound[0].hgh + 1;
  strcpy(t1s, TmString(pt1));
  strcpy(t2s, TmString(pt2));
  strcpy(dts, TmString(&MI.cycle));
  strcpy(format, "Con%10.2e");
  strcpy(vldf, "V");
  if (MI.flags & FLG_SCAT) { 
    strcat(format, "Dev%(*100)5.1f");                            //-2011-06-29
    strcat(vldf, "V");
  }
  seq = MI.cycind;
  if (MI.average > 1)  seq = 1 + (seq-1)/MI.average;
  //
  // dmna header                                                 //-2011-06-29
  TxtCpy(&hdr, "\n");
//  sprintf(s, "TALWRK_%d.%d.%s", StdVersion, StdRelease, StdPatch);
  CAT("prgm  \"%s\"\n", TalLabel);                              //-2011-12-05
  CAT("idnt  \"%s\"\n", MI.label);
  CAT("artp  \"%c\"\n", atype);
  CAT("axes  \"%s\"\n", "xyzs");
  CAT("vldf  \"%s\"\n", vldf);
  CAT("file  \"%s\"\n", name);
  CAT("form  \"%s\"\n", format);
  CAT("t1    \"%s\"\n", t1s);
  CAT("t2    \"%s\"\n", t2s);
  CAT("dt    \"%s\"\n", dts);
  CAT("index  %d\n", seq);
  CAT("groups %d\n", MI.numgrp);
  CAT("refx   %d\n", pgp->refx);
  CAT("refy   %d\n", pgp->refy);   
  if (pgp->refx > 0 && *pgp->ggcs)                                //-2008-12-04
    CAT("ggcs  \"%s\"\n", pgp->ggcs);                             //-2008-12-04
  if (*MI.refdate)                                                //-2008-12-04
    CAT("rdat  \"%s\"\n", MI.refdate);                            //-2008-12-04                                               //-2008-12-04
  CAT("xmin  %s\n", TxtForm(pgp->xmin, 6));
  CAT("ymin  %s\n", TxtForm(pgp->ymin, 6));
  CAT("delta %s\n", TxtForm(pgp->dd, 6));
  CAT("zscl  %s\n", TxtForm(pgp->zscl, 6));
  CAT("sscl  %s\n", TxtForm(pgp->sscl, 6));
  TxtCat(&hdr, "sk  ");
  for (k=0; k<nz; k++)
    CAT(" %s", TxtForm(sk[k], 6));
  TxtCat(&hdr, "\n");
   TxtCat(&hdr, "name  ");
  for (k=0; k<nc; k++) 
    CAT(" \"%s\"", pcmp[k].name);
  TxtCat(&hdr, "\n");
  TxtCat(&hdr, "unit  ");
  for (k=0; k<nc; k++) 
    CAT(" \"%s\"", pcmp[k].unit);
  TxtCat(&hdr, "\n");
  TxtCat(&hdr, "refc  ");
  for (k=0; k<nc; k++) 
    CAT(" %10.2e", pcmp[k].refc);
  TxtCat(&hdr, "\n");
  TxtCat(&hdr, "refd  ");
  for (k=0; k<nc; k++) 
    CAT(" %10.2e", pcmp[k].refd);
  TxtCat(&hdr, "\n");
  CAT("valid %1.6f\n", WrkValid(VALID_AVER));                       //-2011-07-04
  TmnAttach(id, NULL, NULL, TMN_MODIFY, NULL);              eG(7);  //-2001-06-29
  TmnDetach(id, NULL, NULL, TMN_MODIFY|TMN_SETVALID, NULL); eG(13);
  TmnDetach(igp, NULL, NULL, 0, NULL);                      eG(10);
  TmnDetach(iga, NULL, NULL, 0, NULL);                      eG(11);
  TmnDetach(ict, NULL, NULL, 0, NULL);                      eG(12);
  TxtClr(&txt);
  return hdr.s;
eX_1:  eX_2:  eX_3:  eX_4:  eX_5:  eX_6:  eX_7:
  nMSG(_cant_attach_);
  return NULL;
eX_10: eX_11: eX_12: eX_13:
  nMSG(_cant_detach_);
  return NULL;
  }
#undef CAT

//=================================================================== WrkValid
float WrkValid( int type ) {                                     //-2011-07-04
  dP(WrkValid);
  float valid;
  if (type == VALID_TOTAL) {
    if (TotalTime <= 0)                                                  eX(1);
    valid = (TotalTime - TotalDrop)/((float)TotalTime);
  }
  else if (type == VALID_AVER) {
    if (AverTime <= 0)                                                   eX(2);
    valid = (AverTime - AverDrop)/((float)AverTime);
  }
  else if (type == VALID_INTER) {
    if (InterTime != MI.cycle)                                           eX(3);
    valid = (InterTime - InterDrop)/((float)InterTime);
  }
  else                                                                   eX(4);
  if (valid < 0)                                                         eX(5);
  if (valid > 1)                                                         eX(6);
  return valid;
eX_1: eX_2: eX_3: eX_4: eX_5: eX_6:
  eMSG(_error_calculating_valid_);
  return 0;
}

//==================================================================== WrkInit
//
long WrkInit(           /* initialize server    */
long flags,             /* action flags         */
char *istr )            /* server options       */
  {
  dP(WrkInit);
  long iddos, mask;
  char *jstr, *ps, s[200];
  int vrb, dsp;
  if (StdStatus & STD_INIT)  return 0;
  if (istr) {
    jstr = istr;
    ps = strstr(istr, "-v");
    if (ps) sscanf(ps+2, "%d", &StdLogLevel);
    ps = strstr(istr, "-p");
    if (ps) sscanf(ps+2, "%d", &ShowProgress);
    ps = strstr(istr, "-y");
    if (ps) sscanf(ps+2, "%d", &StdDspLevel);
    ps = strstr(istr, "-d");
    if (ps)  strcpy(AltName, ps+2);
    }
  else  jstr = "";
  vLOG(3)("WRK_%d.%d.%s (%08lx,%s)", StdVersion, StdRelease, StdPatch, flags, jstr);
  StdStatus |= flags;
  iddos = IDENT(DOSarr, 0, 0, 0);
  mask = ~(NMS_GROUP|NMS_GRIDN|NMS_LEVEL);
  TmnCreator(iddos, mask, TMN_UNIQUE, "", WrkServer, WrkHeader);        eG(1);
  vrb = StdLogLevel;
  dsp = StdDspLevel;
  sprintf(s, " GRD -v%d -y%d -d%s", vrb, dsp, AltName);
  GrdInit(flags, s);                                                    eG(2);
  sprintf(s, " PRM -v%d -y%d -d%s", vrb, dsp, AltName);
  PrmInit(flags, s);                                                    eG(3);
  sprintf(s, " PTL -v%d -y%d -d%s", vrb, dsp, AltName);
  PtlInit(flags, s);                                                    eG(4);
  sprintf(s, " SRC -v%d -y%d -d%s", vrb, dsp, AltName);
  SrcInit(flags, s);                                                    eG(5);
  sprintf(s, " MOD -v%d -y%d -d%s", vrb, dsp, AltName);
  ModInit(flags, s);                                                    eG(6);
  sprintf(s, " PPM -v%d -y%d -d%s", vrb, dsp, AltName);
  PpmInit(flags, s);                                                    eG(7);
  StdStatus |= STD_INIT;
  return 0;
eX_1:
  eMSG(_creator_$_, NmsName(iddos));
eX_2:  eX_3:  eX_4:  eX_5:  eX_6:  eX_7:
  eMSG(_cant_init_servers_);
  }

//================================================================= WrkServer
//
long WrkServer(         /* server routine for DOSarr    */
  char *s )             /* calling option               */
  {
  dP(WrkServer);
  char t1s[40], t2s[40], name[256];				//-2004-11-26
  int ida, ipa, mask;
  if (StdArg(s))  return 0;
  if (*s) {
    switch (s[1]) {
      case 'd': strcpy(AltName, s+2);
                break;
      case 'J': sscanf(s+2, "%d", &TrcMode);
                break;
      case 'n': if (s[2]) {                                      //-2011-07-04
                  sscanf(s+2, "%d,%d", &AverTime, &AverDrop);
                  if (AverTime < 0) {
                    AverTime = 0;
                    AverDrop = 0;
                    TotalTime = 0;
                    TotalDrop = 0;
                  }
                }
                else {
                  TotalTime += MI.cycle;
                  AverTime += MI.cycle;
                }
                DropT1 = -1;
                InterTime = MI.cycle;
                InterDrop = 0;
                break;
      default:                                                  eX(17);
      }
    return 0;
    }
  if (!StdIdent)                                                eX(20);
  if ((StdStatus & STD_INIT) == 0) {
    WrkInit(0, "");                                             eG(20);
    }
  Gl = XTR_LEVEL(StdIdent);
  Gi = XTR_GRIDN(StdIdent);
  Gp = XTR_GROUP(StdIdent);
  if (StdStatus & STD_TIME)  T1 = StdTime;
  else                                                          eX(1);
  strcpy(t1s, TmString(&T1));

  ida = IDENT(DOSarr, 0, 0, 0);
  mask = ~(NMS_GROUP|NMS_GRIDN|NMS_LEVEL);
  TmnCreator(ida, mask, 0, "", NULL, NULL);                     eG(2);
  PtlT2 = T1;
  ipa = IDENT(PTLarr, Gp, Gl, Gi);
  Ppa = TmnAttach(ipa, &T1, &PtlT2, TMN_MODIFY, NULL);          eG(10);
  if (!Ppa)                                                     eX(11);
  T2 = PtlT2;  if (T2 <= T1)                                    eX(3);
  Nptl = PtlCount(ipa, SRC_EXIST, Ppa);                         eG(12);

  SetSimParm();                                                 eG(13);
  strcpy(t2s, TmString(&T2));
  strcpy(name, NmsName(StdIdent));
  vDSP(2)(_moving_$$$_, name, t1s, t2s);
  RunPtl();                                                     eG(14);
  vDSP(2)("");
  vLOG(4)("WRK: %d particles moved for %s [%s,%s]",
    NumRun, name, t1s, t2s);
  FreeSimParm();                                                eG(15);

  TmnDetach(ipa, &T1, &PtlT2, TMN_MODIFY, NULL);                eG(16);
  TmnCreator(ida, mask, TMN_UNIQUE, "", WrkServer, WrkHeader);  eG(21);
  return 0;
eX_20:
  eMSG(_missing_data_);
eX_1:
  eMSG(_missing_time_);
eX_2:  eX_21:
  strcpy(name, NmsName(ida));
  eMSG(_cant_change_creator_$_, name);
eX_3:
  strcpy(name, NmsName(ipa));
  strcpy(t2s, TmString(&T2));
  eMSG(_invalid_times_$$$_, name, t1s, t2s);
eX_10: eX_11: eX_12:
  strcpy(name, NmsName(ipa));
  eMSG(_no_particles_$_, name);
eX_13: eX_15:
  eMSG(_no_parameters_);
eX_14:
  strcpy(name, NmsName(ida));
  strcpy(t2s, TmString(&T2));
  eMSG(_cant_build_$$$_, name, t1s, t2s);
eX_16:
  strcpy(name, NmsName(ipa));
  strcpy(t1s, TmString(&T2));
  strcpy(t2s, TmString(&PtlT2));
  eMSG(_cant_update_$$$_, name, t1s, t2s);
 eX_17: 
  eMSG("unknown option %s!", s+1);
}

//==========================================================================
//
//  history:
//
// 2002-06-21 lj 0.13.0 final test version
// 2002-09-24 lj 1.0.0  final release candidate
// 2002-12-10 lj 1.0.4  source parameter Tt
// 2003-02-21 lj 1.1.2  internal random number generator
// 2003-06-16 lj 1.1.7  variabel vsed
// 2004-01-12 lj 1.1.14 error message more precise
// 2004-10-01 uj 2.1.0  version number upgraded
// 2004-10-10 uj 2.1.1  Try2Escape
// 2004-11-26 lj 2.1.7  string length for names = 256
// 2004-12-09 uj 2.1.8  shift particle outside body also if not created
// 2005-01-12 uj 2.1.9  check if Zscl = 0
// 2005-01-23 uj 2.1.12 re-mark particle created outside an inner grid
// 2005-02-24 uj 2.1.15 disable sedimentation for retry > MinRetry
// 2005-03-17 uj 2.2.0  version number upgrade
// 2006-10-26 lj 2.3.0  external strings
// 2008-08-04 lj 2.4.3  writes "valid" using 6 decimals
// 2008-12-04 lj 2.4.5  writes "refx", "refy", "ggcs", "rdat"
// 2011-04-13 uj 2.4.9  handle title with 256 chars in header
// 2011-06-29 uj 2.5.0  adjusted to dmna output
// 2011-07-04 uj        handling of "valid" extended
// 2011-10-13 uj 2.5.2  optimization for chemical reactions
// 2011-11-21 lj 2.6.0  wet deposition
// 2013-07-08 uj 2.6.8  check for wdep<0
//
//==========================================================================

