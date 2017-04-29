// ======================================================== TalWnd.c
//
// Diagnostic wind field model for AUSTAL2000
// ==========================================
//
// Copyright (C) Umweltbundesamt, Dessau-Roﬂlau, Germany, 2002-2007
// Copyright (C) Janicke Consulting, 88662 ‹berlingen, Germany, 2002-2014
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
// last change:  2014-06-26
//
// ==================================================================

#include <math.h>
#include <time.h>
#include <signal.h>
static char *eMODn = "TALdia";

#define  STDMYMAIN    TwnMain
#define  STDLOGLEVEL  1
#define  STDDSPLEVEL  0
#define  STDARG       1

#include "IBJmsg.h"
#include "IBJary.h"
#include "IBJtxt.h"

#include "TalWnd.nls"
#define STDNLS "DIA"
#define NLS_DEFINE_ONLY
#include "IBJstd.h"
#include "IBJstd.nls"

static int CHECK=0;

/*=========================================================================*/

STDPGM(TALdia, TwnServer, 2, 6, 5);

/*=========================================================================*/

#include "genutl.h"
#include "genio.h"

#include "TalTmn.h"
#include "TalNms.h"
#include "TalGrd.h"
#include "TalBds.h"
#include "TalBlm.h"
#include "TalAdj.h"
#include "IBJtxt.h"
#include "IBJdmn.h"
#include "IBJstamp.h"  //-2005-03-10
#include "TalInp.h"
#include "TalDef.h"
#include "TalUtl.h"
#include "TalAKT.h"
#include "TalAKS.h"
#include "TalWnd.h"
#include "TalDMKutil.h"
#include "TalDMK.h"
#include "TalMat.h"   //-2004-12-14
#include "TalSOR.h"   //-2006-10-04
#include "TalCfg.h"                                               //-2008-08-08

#define  NC  15
#define  MAXRELDIV 0.5

ARYDSC   Aoff, *Tmx;
#define  OFF(i,j,k)  (*(int*)AryPtrX(&Aoff,(i),(j),(k)))
#define  MAT(l)      (*(MATREC*)AryPtrX(Tmx,(l)))

#define  NOT_DEFINED  -999

#define iZP(a,b,c)  (*(float*)AryPtrX(Pzp,(a),(b),(c)))
#define iDZ(a,b,c)  (iZP(a,b,c)-iZP(a,b,c-1))
#define iZM(a,b,c)  (*(float*)AryPtrX(Pzm,(a),(b),(c)))
#define iVM(a,b,c)  (*(float*)AryPtrX(Pvm,(a),(b),(c)))
#define iAN(a,b,c)  (*(float*)AryPtrX(Pan,(a),(b),(c)))
#define iBN(a,b,c)  (*(float*)AryPtrX(Pbn,(a),(b),(c)))
#define iCN(a,b,c)  (*(float*)AryPtrX(Pcn,(a),(b),(c)))
#define iMX(a,b)    (*(float*)AryPtrX(Pmx,(a),(b)))
#define iIN(a)      (*(float*)AryPtrX(Pin,(a)))
#define iLD(a)      (*(float*)AryPtrX(Pld,(a)))
#define iLX(a)      (*(float*)AryPtrX(Plx,(a)))

#define  iWND(a,b,c)  (*(WNDREC*)AryPtrX(Pwnd,(a),(b),(c)))
#define  iWIN(a,b,c)  (*(WNDREC*)AryPtrX(Pwin,(a),(b),(c)))
#define  iWKF(a,b,c)  (*(WNDREC*)AryPtrX(Pwkf,(a),(b),(c)))
#define  iLMD(a,b,c)  (*(float*)AryPtrX(Plmd,(a),(b),(c)))
#define   SLD(a,b,c)  (*(long*)AryPtrX(Psld,(a),(b),(c)))

/*--------------------------------------------------------------------------*/

typedef struct matrec {
  int i, j, k; int ii[NC]; float aa[NC];
  } MATREC;

/*----------------------------------------------------------------------------*/

static int DefGrid( int level );
static int WndW2R( ARYDSC *pwnd, long mask );
static long MakeInit( char *ss );
static int ReadInit( int step );
static int ReadTurb( void );
static int UvwInit( int step );
static int ClcMxIn( long mask );
static int ClcUvw( int step, int set_volout );
static void ClrArrays( int step );
static int WrtHeader(TXTSTR *hdr, char *valdef );
static int ClcDMK(ARYDSC *, int, int);
static float InterpolateVv(double *, double *, double, int);
static int ClcUref(float *, float *);
static int DefBodies( void );
static int PrepareBodies( void );


#ifdef MAIN
static void TwnHelp( char* );                                     //-2008-08-15
#endif
/*-------------------------------------------------------------------------*/
static float *Yy, *Cc;
static float Xmin, Ymin, Xmax, Ymax, Dd, Zd, Sd, Dzm;
static float Xa, Ya, Za, Ha, Ua, Da;
static int Nx, Ny, Nz, Nv;
static int Nc = NC;
static ARYDSC *Psld, *Ptrb, *Pwin, *Pwnd, *Pzp, *Pzm, *Pvm, *Pdff;
static ARYDSC *Pan, *Pbn, *Pcn, *Pin, *Pld, *Pba, *Pbld;
static float Aver, Uref, Dref, Tchar, Uini, AvMean, AvMax=10;
static float Omega, Epsilon;
static int MaxIter;
static int SolidTop, GridLevel, GridIndex;
static char DefName[256] = "param.def";
static char InpName[256] = "metlib.def";
static char InpNewName[256] = "";
static char TalPath[256];
static char WindLib[256] = "~../lib"; //-2004-08-31 (f√ºr AKS)
static long MetT1, MetT2, Itrb, Iwin, Isld, Iwnd, Izp, Izm, Ivm, Idff;
static long Isrf0, Isrf1, Ibld;
static long Ian, Ibn, Icn, Imx, Iin, Ild;
static long Iba, Ibt, Ibp, Igt, Igp, Iga;
static float CharHb, CharHg, CharLg;
static int LowestLevel, HighestLevel;

static const float Grav = 9.81;
static const float AvTmp = 283;
static int Nret = 2;
static int WrtMask=0x30000, LastStep;
static char OptString[4000];
static int ZeroDiv = -1;
static int UseCharLg = 1;                                     //-2002-02-04
static int NumLibMembers=0;
static char WndVersion[256];

static ARYDSC Bin, Bout;
static double BmInX0, BmInY0, BmInDd, BmInDz;
static int Nb;
static double Gx, Gy;

static DMKDAT DMKpar;
static int LstMode=0;
static int WriteBinary=1;
static int GridOnly = 0;                                        //-2005-08-14
static int DefsOnly = 0;                                        //-2006-03-24
static int ClearLib = 0;                                        //-2006-10-04

static float MaxDiv = 0;                                        //-2007-03-07
static int   MaxDivWind = -1;                                   //-2007-03-07

/*-------------------------------------------------------------------------*/


/*===================================================================== DefGrid
*/
static int DefGrid( int level )
  {
  dP(DefGrid);
  int i, j, k, m, isld, setbnd, inside;
  float z1, z2;
  ARYDSC *pa, *psld;
  GRDPARM *pg;
  vLOG(4)("WND:DefGrid(%d)", level);
  GrdSet(GridLevel, GridIndex);                                         eG(1);
  inside = (GridLevel > LowestLevel);
  setbnd = (0 != (StdStatus & WND_SETBND));

  Igp = IDENT(GRDpar, 0, GridLevel, GridIndex);
  Igt = IDENT(GRDtab, 0, 0, 0);
  Iga = IDENT(GRDarr, 0, 0, 0);
  Isrf0 = IDENT(SRFarr, 0, GridLevel, GridIndex);
  Isrf1 = IDENT(SRFarr, 1, GridLevel, GridIndex);
  pa = TmnAttach(Igp, NULL, NULL, 0, NULL);  if (!pa)                   eX(11);
  pg = (GRDPARM*) pa->start;
  if (pg->defmode)                                                      eX(10);
  Nx = pg->nx;
  Ny = pg->ny;
  Nz = pg->nz;
  Xmin = pg->xmin;
  Ymin = pg->ymin;
  Dd = pg->dd;
  Zd = pg->zscl;
  Sd = pg->sscl;
  MaxIter = pg->itermax;
  Epsilon = pg->itereps;
  Omega   = pg->iterrad;
  if (Omega < 0.9)  Omega = 0;
  TmnDetach(Igp, NULL, NULL, 0, NULL);                              eG(12);
  Xmax = Xmin + Nx*Dd;
  Ymax = Ymin + Ny*Dd;
  if (level < 1)  return 1;

  /* Absolute z-Werte der Gitterpunkte */

  Izp  = IDENT(GRDarr, GRD_IZP,  GridLevel, GridIndex);
  Pzp  = TmnAttach(Izp, NULL, NULL, 0, NULL);                       eG(13);
  Izm  = IDENT(GRDarr, GRD_IZM,  GridLevel, GridIndex);
  Pzm  = TmnAttach(Izm, NULL, NULL, 0, NULL);                       eG(14);
  Ivm  = IDENT(GRDarr, GRD_IVM,  GridLevel, GridIndex);
  Pvm  = TmnAttach(Ivm, NULL, NULL, 0, NULL);                       eG(15);

  /* Mittlere vertikale Maschenweite */

  Dzm = 0;
  for (i=0; i<=Nx; i++)
    for (j=0; j<=Ny; j++) {
      z1 = iZP(i,j,0);
      z2 = iZP(i,j,Nz);
      Dzm += z2 - z1;
      }
  Dzm /= (Nx+1)*(Ny+1)*Nz;

  /* Geometrie-Daten erg‰nzen */

  SolidTop = 0;
  if ( (Sd == *(float*)AryPtrX(GrdParr,Nz))                         //-00-02-18
    && (GrdPprm->bd & GRD_REFZ)
    && (GrdPprm->hmax == Zd)
    && (Zd > 0))  SolidTop = 1;
  if (SolidTop) {
    vLOG(3)("WND: solid top in grid (%d,%d)", GridLevel, GridIndex);
    }
  if (Isld)                                                         eX(2);
  isld = IDENT(GRDarr, GRD_ISLD, GridLevel, GridIndex);
  psld = TmnAttach(isld, NULL, NULL, 0, NULL);  if (!psld)          eX(3);
  Isld = TmnIdent();
  Psld = TmnCreate(Isld, sizeof(long), 3, 0,Nx, 0,Ny, 0,Nz);        eG(5);
  if (Psld->ttlsz != psld->ttlsz)                                   eX(6);
  memcpy(Psld->start, psld->start, psld->ttlsz);
  TmnDetach(isld, NULL, NULL, 0, NULL);                             eG(7);

  if (setbnd || inside) {
    m = GRD_ARXSET;
    for (j=1; j<=Ny; j++)
      for (k=1; k<=Nz; k++) {
        SLD( 0,j,k)  |= m;
        SLD(Nx,j,k)  |= m;
        }
    m = GRD_ARYSET;
    for (i=1; i<=Nx; i++)
      for (k=1; k<=Nz; k++) {
        SLD(i, 0,k)  |= m;
        SLD(i,Ny,k)  |= m;
        }
    }

  m = GRD_ARZBND | GRD_ARZSET;
  for (i=1; i<=Nx; i++)
    for (j=1; j<=Ny; j++)  SLD(i,j,0) |= m;

  m = 0;
  if (SolidTop)  m =  GRD_ARZBND | GRD_ARZSET;
  if (inside)  m |= GRD_ARZSET;
  if (m)
    for (i=1; i<=Nx; i++)
      for (j=1; j<=Ny; j++)  SLD(i,j,Nz) |= m;

  TmnDetach(Isld, NULL, NULL, TMN_MODIFY, NULL);                        eG(4);
  Psld = NULL;
  return 1;
eX_1:
  eMSG(_cant_set_$$_, GridLevel, GridIndex);
eX_10:
  eMSG(_defmode0_);
eX_11: eX_12: eX_13: eX_14: eX_15:
  eMSG(_no_grid_);
eX_2:  eX_3:  eX_4:  eX_5:  eX_6:  eX_7:
  eMSG(_no_solid_);
  }

/*====================================================================== WndW2R
*/
#define WND(i,j,k)  (*(WNDREC*)AryPtrX(pwnd,(i),(j),(k)))
#define  VX(i,j,k)  (((WNDREC*)AryPtrX(pwnd,(i),(j),(k)))->vx)
#define  VY(i,j,k)  (((WNDREC*)AryPtrX(pwnd,(i),(j),(k)))->vy)
#define  VZ(i,j,k)  (((WNDREC*)AryPtrX(pwnd,(i),(j),(k)))->vz)
#define  ZZ(i,j,k)  (((WNDREC*)AryPtrX(pwnd,(i),(j),(k)))->z)

static int WndW2R( ARYDSC *pwnd, long mask )
  {
  dP(WndW2R);
  int i, j, k;
  int c12 = 0;                                                    //-2012-04-06
  float phi, chi, z1, z2, zm, vx, vy, vx1, vx2, vy1, vy2;
  vLOG(4)("WND:WndW2R(..., %08x)", mask);
  Psld = TmnAttach(Isld, NULL, NULL, 0, NULL);  if (!Psld)      eX(1);
  for (i=1; i<=Nx; i++)
    for (j=1; j<=Ny; j++)
      for (k=0; k<=Nz; k++) {
        if (SLD(i,j,k) & (GRD_ARZOUT|GRD_ARZBND) & mask) {
          VZ(i,j,k) = 0;
          continue;
          }
        phi = (ZZ(i,j-1,k) + ZZ(i,j,k) - ZZ(i-1,j-1,k) - ZZ(i-1,j,k))/(2*Dd);
        chi = (ZZ(i-1,j,k) + ZZ(i,j,k) - ZZ(i-1,j-1,k) - ZZ(i,j-1,k))/(2*Dd);
        vx = 0;
        vy = 0;
        z1 = 0;
        c12 = 0;                                                  //-2012-04-06
        if (k>0 && 0==(SLD(i,j,k)&GRD_VOLOUT&mask)) {
          vx = vx1 = 0.5*(VX(i-1,j,k) + VX(i,j,k));
          vy = vy1 = 0.5*(VY(i,j-1,k) + VY(i,j,k));
          zm = (ZZ(i,j-1,k) + ZZ(i,j,k) + ZZ(i-1,j-1,k) + ZZ(i-1,j,k))/4;
          z1 = (ZZ(i,j-1,k-1) + ZZ(i,j,k-1)
               + ZZ(i-1,j-1,k-1) + ZZ(i-1,j,k-1))/4;
          z1 = (z1 + zm)/2;
          c12++;                                                  //-2012-04-06
        }
        z2 = 0;
        if (k<Nz && 0==(SLD(i,j,k+1)&GRD_VOLOUT&mask)) {
          vx = vx2 = 0.5*(VX(i-1,j,k+1) + VX(i,j,k+1));
          vy = vy2 = 0.5*(VY(i,j-1,k+1) + VY(i,j,k+1));
          zm = (ZZ(i,j-1,k) + ZZ(i,j,k) + ZZ(i-1,j-1,k) + ZZ(i-1,j,k))/4;
          z2 = (ZZ(i,j-1,k+1) + ZZ(i,j,k+1)
               + ZZ(i-1,j-1,k+1) + ZZ(i-1,j,k+1))/4;
          z2 = (z2 + zm)/2;
          c12++;                                                  //-2012-04-06
        }
        if (c12 == 2) {                                           //-2012-04-06
          vx = ((z2-zm)*vx1 + (zm-z1)*vx2)/(z2-z1);
          vy = ((z2-zm)*vy1 + (zm-z1)*vy2)/(z2-z1);
        }
        VZ(i,j,k) -= vx*phi + vy*chi;
      }
  TmnDetach(Isld, NULL, NULL, 0, NULL);                         eG(2);
  Psld = NULL;
  return 0;
eX_1: eX_2:
  eMSG(_no_grdarr2_);
  }
#undef WND
#undef  VX
#undef  VY
#undef  VZ
#undef  ZZ

/*============================================================= InsertBoundary
*/
static int InsertBoundary( ARYDSC *pwnd, GRDPARM *pgrdp, int step )
  {
  dP(InsertBoundary);
  NETREC *pn;
  ARYDSC *qwnd, *pgt;
  WNDREC *pw, *qw;
  float xmin, xmax, ymin, ymax, dd, d;
  float xm, ym, dm;
  int i, j, k, igt, m;
  int gl, nx, ny, nz, net, ll, lowerlevel, ii, jwnd;
  int mx, my, mz, im, jm;
  vLOG(4)("WND:InsertBoundary(step %d)", step);
  //if (step == WND_TERRAIN)  step = WND_FINAL;
  gl = pgrdp->level;
  nx = pgrdp->nx;
  ny = pgrdp->ny;
  nz = pgrdp->nz;
  xmin = pgrdp->xmin;
  xmax = pgrdp->xmax;
  ymin = pgrdp->ymin;
  ymax = pgrdp->ymax;
  dd = pgrdp->dd;
  d = 0.01*dd;
  lowerlevel = -1;
  if (step == WND_FINAL) {
    Psld = TmnAttach(Isld, NULL, NULL, 0, NULL);  if (!Psld)    eX(1);
    }
  else  Psld = NULL;

  if (gl > LowestLevel) {
    igt = IDENT(GRDtab, 0, 0, 0);
    pgt = TmnAttach(igt, NULL, NULL, 0, NULL);  if (!pgt)       eX(20);
    for (net=pgrdp->numnet; net>0; net--) {
      pn = AryPtr(pgt, net);  if (!pn)                          eX(21);
      ll = pn->level;
      if (ll >= gl)  continue;
      if (lowerlevel < 0)  lowerlevel = ll;
      else if (ll < lowerlevel)  break;
      if (pn->ntyp == FLAT1D)  break;
      if (pn->x1 > xmax+d)  continue;
      if (pn->x2 < xmax-d)  continue;
      if (pn->y1 > ymax+d)  continue;
      if (pn->y2 < ymax-d)  continue;
      mx = pn->nx;
      my = pn->ny;
      mz = pn->nz;
      xm = pn->xmin;
      ym = pn->ymin;
      dm = pn->delta;
      ii = pn->index;
      jwnd = IDENT(WNDarr, step, ll, ii);
      qwnd = TmnAttach(jwnd, &MetT1, &MetT2, 0, NULL);  if (!qwnd)  eX(22);
      for (i=1; i<=nx; i++) {
        im = floor(1.01 + (xmin + (i-0.5)*dd - xm)/dm);             //-2003-03-11
        if (im<1 || im>mx)  continue;
        for (j=0; j<=ny; j+=ny) {
          jm = floor(0.01 + (ymin + j*dd - ym)/dm);                 //-2003-03-11
          if (jm<0 || jm>my)  continue;
          for (k=1; k<=nz; k++) {
            if (k > mz)  break;
            pw = AryPtr(pwnd, i, j, k);  if (!pw)                      eX(23);
            qw = AryPtr(qwnd, im, jm, k);  if (!qw)                    eX(24);
            pw->vy = qw->vy;
            if (Psld) {
              m = SLD(i,j,k);
              if (m & (GRD_ARYBND|GRD_ARYOUT))  pw->vy = 0;
              }
            }
          }
        }
      for (j=1; j<=ny; j++) {
        jm = floor(1.01 + (ymin + (j-0.5)*dd - ym)/dm);             //-2003-03-11
        if (jm<1 || jm>my)  continue;
        for (i=0; i<=nx; i+=nx) {
          im = floor(0.01 + (xmin + i*dd - xm)/dm);                 //-2003-03-11
          if (im<0 || im>mx)  continue;
          for (k=1; k<=nz; k++) {
            if (k > mz)  break;
            pw = AryPtr(pwnd, i, j, k);  if (!pw)                      eX(25);
            qw = AryPtr(qwnd, im, jm, k);  if (!qw)                    eX(26);
            pw->vx = qw->vx;
            if (Psld) {
              m = SLD(i,j,k);
              if (m & (GRD_ARXBND|GRD_ARXOUT))  pw->vx = 0;
              }
            }
          }
        }
      for (i=1; i<=nx; i++) {
        im = floor(1.01 + (xmin + (i-0.5)*dd - xm)/dm);             //-2003-03-11
        if (im<1 || im>mx)  continue;
        for (j=1; j<=ny; j++) {
          jm = floor(1.01 + (ymin + (j-0.5)*dd - ym)/dm);           //-2003-03-11
          if (jm<1 || jm>my)  continue;
          pw = AryPtr(pwnd, i, j, nz);  if (!pw)                      eX(27);
          qw = AryPtr(qwnd, im, jm, nz);  if (!qw)                    eX(28);
          pw->vz = qw->vz;
          if (Psld) {
            m = SLD(i,j,nz);
            if (m & (GRD_ARZBND|GRD_ARZOUT))  pw->vz = 0;
            }
          }
        }
      TmnDetach(jwnd, NULL, NULL, 0, NULL);                        eG(29);
      qwnd = NULL;
      }  /* for net */
    TmnDetach(igt, NULL, NULL, 0, NULL);                           eG(30);
    pgt = NULL;
    }  /* if gl */

  if (Psld) {
    TmnDetach(Isld, NULL, NULL, 0, NULL);                          eG(2);
    Psld = NULL;
    }

  return 0;
eX_1: eX_2:
eX_20: eX_21: eX_22: eX_23: eX_24: eX_25: eX_26: eX_27: eX_28: eX_29: eX_30:
  eMSG(_cant_insert_boundary_);
  }

/*=================================================================== MakeInit
*/
#define  ZM(i,j,k)  (*(float*)AryPtrX(pzm,(i),(j),(k)))
#define  ZP(i,j,k)  (*(float*)AryPtrX(pzp,(i),(j),(k)))

static long MakeInit( char *ss )
  {
  dP(MakeInit);
  ARYDSC *pwnd, *pzp, *pzm, *pgp, *pbp, *pba;
  GRDPARM *pgrdp;
  BLMPARM *pp;
  BLMREC *pb1, *p1, *p2;
  float vx, vy, hb1, h, r, u, si, co, zg, zb0;                //-2003-02-24
  WNDREC *pw;
  int i, j, k, iba, ibp, nk, izm, izp, igp;
  int gl, gi, nx, ny, nz;
  static long id, t1, t2;
  float h1, h2, r1, r2, u1, u2, vx1, vx2, vy1, vy2, a1, a2;   //-2003-02-24
  int k1, k2;
  char name[256], t1s[40];				//-2004-11-26
  TXTSTR hdr = { NULL , 0 };
  vLOG(4)("WND:MakeInit(%s)", ss);
  if (*ss) {
    if (*ss == '-')
      switch(ss[1]) {
        case 'N':  sscanf(ss+2, "%08lx", &id);
                   break;
        case 'T':  strcpy(t1s, ss+2);
                   t1 = TmValue(t1s);
                   break;
        default:   ;
      }
    return 0;
  }
  if (!id)                                                      eX(30);
  t2 = t1;
  strcpy(name, NmsName(id));
  vLOG(4)("WND: creating %s", name);
  gl = XTR_LEVEL(id);
  gi = XTR_GRIDN(id);
  igp = IDENT(GRDpar, 0, gl, gi);
  pgp = TmnAttach(igp, NULL, NULL, 0, NULL);  if (!pgp)         eX(1);
  pgrdp = pgp->start;
  nx = pgrdp->nx;
  ny = pgrdp->ny;
  nz = pgrdp->nz;
  izm = IDENT(GRDarr, GRD_IZM, gl, gi);
  pzm = TmnAttach(izm, NULL, NULL, 0, NULL);  if (!pzm)         eX(1);
  izp = IDENT(GRDarr, GRD_IZP, gl, gi);
  pzp = TmnAttach(izp, NULL, NULL, 0, NULL);  if (!pzp)         eX(1);
  ibp = IDENT(BLMpar, 0, 0, 0);
  pbp = TmnAttach(ibp, &t1, &t2, 0, NULL);  if (!pbp)           eX(2);
  pp = (BLMPARM*) pbp->start;
  if (pp->Wini >= 0)                                            eX(9);
  if (pp->lmo == 0 && pp->cl == 0)                             eX(52);
  iba = IDENT(BLMarr, 0, 0, 0);
  pba = TmnAttach(iba, &t1, &t2, 0, NULL);  if (!pba)           eX(10);
  nk = pba->bound[0].hgh;
  pwnd = TmnCreate(id, sizeof(WNDREC), 3, 0, nx, 0, ny, 0, nz); eG(4);

  /*  Grenzschicht-Profil einsetzen: */

  for (i=0; i<=nx; i++)
    for (j=0; j<=ny; j++)
      for (k=0; k<=nz; k++)
        ((WNDREC*)AryPtrX(pwnd,i,j,k))->z = ZP(i,j,k);

  pb1 = AryPtr(pba,0);  if (!pb1)                                  eX(11);
  zb0 = pb1->z;
  pb1 = AryPtr(pba,1);  if (!pb1)                                  eX(11);
  hb1 = pb1->z - zb0;
  for (i=1; i<=nx; i++) {
    for (j=1; j<=ny; j++) {
      zg = 0.25*(ZP(i,j,0) + ZP(i-1,j,0) + ZP(i,j-1,0) + ZP(i-1,j-1,0)); //-2004-10-19
      for (k=1; k<=nz; k++) {
        h = ZM(i,j,k) - zg;                                       //-2003-02-24
        k1 = 0;
        if (h <= hb1) {
          r = pb1->d;
          u = pb1->g;
        }
        else {
          for (k2=2; k2<=nk; k2++) {
            p2 = AryPtr(pba, k2);  if (!p2)                        eX(12);
            h2 = p2->z - zb0;                                      //-2003-02-24
            if (h <= h2)  break;
          }
          if (k2 > nk) {
            r = p2->d;
            u = p2->g;
          }
          else {
            k1 = k2 - 1;
            p1 = AryPtr(pba, k1);  if (!p1)                        eX(14);
            h1 = p1->z - zb0;                                      //-2003-02-24
            r1 = p1->d;  u1 = p1->g;
            r2 = p2->d;  u2 = p2->g;
            vx1 = -sin(r1*PI/180)*u1;
            vx2 = -sin(r2*PI/180)*u2;
            vy1 = -cos(r1*PI/180)*u1;
            vy2 = -cos(r2*PI/180)*u2;
            a1 = (h - h1)/(h2 - h1);                               //-2003-02-24
            a2 = 1 - a1;
            vx = a2*vx1 + a1*vx2;
            vy = a2*vy1 + a1*vy2;
          }
        }
        if (k1 == 0) {
          si = sin(r*PI/180);
          co = cos(r*PI/180);
          vx = -si*u;
          vy = -co*u;
        }
        pw = (WNDREC*) AryPtrX(pwnd, i, j, k);
        pw->vx += vx;
        pw->vy += vy;
        pw = (WNDREC*) AryPtrX(pwnd, i-1, j, k);
        pw->vx += vx;
        pw = (WNDREC*) AryPtrX(pwnd, i, j-1, k);
        pw->vy += vy;
      }
    }
  }

  for (i=1; i<=nx-1; i++)
    for (j=1; j<=ny; j++)
      for (k=1; k<=nz; k++)
        ((WNDREC*)AryPtrX(pwnd,i,j,k))->vx *= 0.5;

  for (i=1; i<=nx; i++)
    for (j=1; j<=ny-1; j++)
      for (k=1; k<=nz; k++)
        ((WNDREC*)AryPtrX(pwnd,i,j,k))->vy *= 0.5;
        
  WndW2R(pwnd, 0);                                              eG(13);
  InsertBoundary(pwnd, pgrdp, WND_TERRAIN);                     eG(20);
  TmnDetach(ibp, NULL, NULL, 0, NULL);                          eG(2);
  TmnDetach(iba, NULL, NULL, 0, NULL);                          eG(5);
  TmnDetach(izp, NULL, NULL, 0, NULL);                          eG(15);
  TmnDetach(izm, NULL, NULL, 0, NULL);                          eG(16);
  TmnDetach(igp, NULL, NULL, 0, NULL);                          eG(17);
  WrtHeader(&hdr, "PXYS");                                      eG(7);
  TmnDetach(id, &t1, &t2, TMN_MODIFY, &hdr);                    eG(8);
  TxtClr(&hdr);
  /* // this bit taken for code archiving from LJ
  if (WrtMask & (1<<WND_BASIC)) {                                 //-2010-09-10
    int option = 0;
    if (StdStatus & WND_SETUNDEF)  option = TMN_SETUNDEF;
    strcpy(name, NmsName(id));
    TmnArchive(id, name, option);                                eG(44);
    vLOG(3)(_$_written_, name);
  }
  */
  return 0;
eX_30:
  eMSG(_no_init_);
eX_1:
  eMSG(_no_grid_);
eX_2: eX_5:
  eMSG(_no_meteo_);
eX_4:  eX_8:
  eMSG(_cant_create_init_$_, name);
eX_7: eX_10: eX_11: eX_12: eX_13: eX_14: eX_15: eX_16: eX_17:
  eMSG(_internal_error_);
eX_9:
  eMSG(_init_$_not_found_, name);
eX_20:
  eMSG(_no_boundary_$$$_, gl, gi, WND_TERRAIN);
eX_52:
  eMSG(_no_stability_);
  }
#undef  ZM
#undef  ZP

/*==================================================================== ReadInit
*/
static int ReadInit( int step )
  {
  dP(ReadInit);
  float xm, ym, dd;
  char fname[256], format[256], valdef[40];		                     //-2011-06-29
  int n, i, j, set0;
  TXTSTR hdr = { NULL, 0 };
  vLOG(4)("WND:ReadInit()");
  strcpy(fname, NmsName(Iwin));
  vLOG(4)("WND: looking for %s", fname);
  Pwin = TmnAttach(Iwin, &MetT1, &MetT2, 0, &hdr);  if (!Pwin)    eX(50);
  if (0 > DmnGetFloat(hdr.s, "Uref", "%f", &Uini, 1))             eX(51); //-2014-06-26
  if (0 > DmnGetFloat(hdr.s, "Xmin", "%f", &xm, 1))               eX(52);
  if (0 > DmnGetFloat(hdr.s, "Ymin", "%f", &ym, 1))               eX(53);
  if (0 > DmnGetFloat(hdr.s, "Delta|Dd|Delt", "%f", &dd, 1))      eX(54);
  if (0 != DmnCpyString(hdr.s, "form|format", format, 256))       eX(70);
  if (0 != DmnCpyString(hdr.s, "ValDef|vldf", valdef, 40))        eX(72);
  if (strlen(valdef) != 4)                              eX(73);
  if (ISNOTEQUAL(xm,GrdPprm->xmin))                     eX(55);
  if (ISNOTEQUAL(ym,GrdPprm->ymin))                     eX(56);
  if (ISNOTEQUAL(dd,GrdPprm->dd))                       eX(57);
  AryAssert(Pwin, sizeof(WNDREC), 3, 0, Nx, 0, Ny, 0, Nz);  eG(58);
  if (valdef[3] != 'S') {
    if (valdef[3] != 'Z')                               eX(74);
    n = WndW2R(Pwin, 0);  if (n < 0)                    eX(71);
    vLOG(4)(_init_converted_);
    }
  set0 = SolidTop;
  for (i=1; i<=Nx; i++)
    for (j=1; j<=Ny; j++) {
      iWIN(i,j,0).vz = 0;
      if (set0)  iWIN(i,j,Nz).vz = 0;
      }
  TmnDetach(Iwin, NULL, NULL, 0, NULL);                 eG(59);
  Pwin = NULL;
  TxtClr(&hdr);
  return 0;
eX_59:
  eMSG(_internal_error_);
eX_50: eX_51: eX_52:  eX_53:  eX_54:  eX_70:  eX_72:
  eMSG(_error_header_$_, fname);
eX_73:
  eMSG(_valdef_required_$_, fname);
eX_55: eX_56: eX_57:
  eMSG(_parameter_$_inconsistent_, fname);
eX_58:
  eMSG(_structure_$_inconsistent_, fname);
eX_71: eX_74:
  eMSG(_cant_convert_);
  }

/*==================================================================== ReadTurb
*/
static int ReadTurb( void )
  {
  dP(ReadTurb);
  float xm, ym, dd;
  char fname[256];					//-2004-11-26
  TXTSTR hdr = { NULL, 0 };                                      //-2011-06-29
  vLOG(4)("ReadTurb() ...");
  if (BlmPprm->Turb <= 0) {
    Ptrb = NULL;
    return 0;
  }
  NmsSeq( "index", BlmPprm->Turb);
  Itrb = IDENT(VARarr, 0, GridLevel, GridIndex);
  strcpy(fname, NmsName(Itrb));
  Ptrb = TmnAttach(Itrb, &MetT1, &MetT2, 0, &hdr);      eG(50);
  if (!Ptrb)                                            eX(51);
  if (0 > DmnGetFloat(hdr.s, "Xmin|X0", "%f", &xm, 1))  eX(52);   //-2011-06-29
  if (0 > DmnGetFloat(hdr.s, "Ymin|Y0", "%f", &ym, 1))  eX(53);   //-2011-06-29
  if (0 > DmnGetFloat(hdr.s, "Delta|Dd|Delt", "%f", &dd, 1)) eX(54);
  if (ISNOTEQUAL(xm,Xmin))                              eX(55);
  if (ISNOTEQUAL(ym,Ymin))                              eX(56);
  if (ISNOTEQUAL(dd,Dd))                                eX(57);
  AryAssert(Ptrb, sizeof(TRBREC), 3, 0, Nx, 0, Ny, 0, Nz);  eG(58);
  return 0;
eX_50: eX_51:
  eMSG(_cant_read_$_, fname);
eX_52:  eX_53:  eX_54:
  eMSG(_error_header_$_, fname);
eX_55: eX_56: eX_57:
  eMSG(_parameter_$_inconsistent_, fname);
eX_58:
  eMSG(_structure_$_inconsistent_, fname);
  }

//====================================================================== ClcUref
static int ClcUref(float *uref, float *dref) {
  dP(WND:ClcUref);
  long iwnd;
  int k, na, nh, nx, ny, nz, nzmax=99, gl, gi, ja, ia;
  double uu[100], zz[100];
  float da, db, z00, z01, z10, z11, v0, v1, vx, vy, xmin, ymin, dd;
  ARYDSC *pwnd;
  na = GrdBottom(Xa, Ya, NULL);  if (na < 0)                            eX(1);
  if (na > 0) {
    GrdSetNet(na);                                                      eG(2);
  }
  gl = GrdPprm->level;
  gi = GrdPprm->index;
  xmin = GrdPprm->xmin;
  ymin = GrdPprm->ymin;
  nx = GrdPprm->nx;
  ny = GrdPprm->ny;
  nz = GrdPprm->nz;
  dd = GrdPprm->dd;
  iwnd = IDENT(WNDarr, WND_FINAL, gl, gi);
  pwnd = TmnAttach(iwnd, NULL, NULL, 0, NULL);  if (!pwnd)              eX(3);
  ia = 1 + (Xa-xmin)/dd;  if (ia > nx)  ia = nx;
  da = (Xa-xmin)/dd - (ia-1);
  ja = 1 + (Ya-ymin)/dd;  if (ja > ny)  ja = ny;
  db = (Ya-ymin)/dd - (ja-1);
  nh = (nz <= nzmax) ? nz : nzmax;
  for (k=0; k<=nh; k++) {
    z00 = (*(WNDREC*)AryPtrX(pwnd, ia-1, ja-1, k)).z;
    z10 = (*(WNDREC*)AryPtrX(pwnd, ia  , ja-1, k)).z;
    z01 = (*(WNDREC*)AryPtrX(pwnd, ia-1, ja  , k)).z;
    z11 = (*(WNDREC*)AryPtrX(pwnd, ia  , ja  , k)).z;
    zz[k] = z00 + da*(z10-z00) + db*(z01-z00) + da*db*(z00+z11-z10-z01);
  }
  for (k=1; k<=nh; k++)
    if (zz[k]-zz[0] >= Ha)  break;
  if (k > nh)                                                          eX(4);
  for (k=1; k<=nh; k++) {
    v0 = (*(WNDREC*)AryPtrX(pwnd, ia-1, ja, k)).vx;
    v1 = (*(WNDREC*)AryPtrX(pwnd, ia  , ja, k)).vx;
    uu[k] = v0 + da*(v1-v0);
  }
  vx = InterpolateVv(uu, zz, Ha, nh);
  for (k=1; k<=nh; k++) {
    v0 = (*(WNDREC*)AryPtrX(pwnd, ia, ja-1, k)).vy;
    v1 = (*(WNDREC*)AryPtrX(pwnd, ia, ja  , k)).vy;
    uu[k] = v0 + db*(v1-v0);
  }
  vy = InterpolateVv(uu, zz, Ha, nh);
  *uref = sqrt(vx*vx + vy*vy);
  *dref = (180/PI)*atan2(-vx, -vy);
  if (*dref < 0)  *dref += 360;
  if (*uref < 0.5)                                                     eX(6);
  TmnDetach(iwnd, NULL, NULL, 0, NULL);                                eG(5);
  return 0;
eX_1: eX_2: eX_3: eX_4: eX_5:
  eMSG(_no_uref_);
eX_6:
  eMSG(_improper_field_$$_, NmsName(iwnd), *uref);
}


/*==================================================================== ClcCharHL
*/
static int ClcCharHL( float *phb, float *phg, float *plg )
  {
  float zg, zgm, zg2m, hgc, z00, z01, z10, z11, zm, g2m;
  float zgmin, zgmax, fx, fy, lgc;
  int i, j;
  vLOG(4)("WND:ClcCharHL(...)");
  zgm = 0;
  zgmax = iBOT(0,0);
  zgmin = zgmax;
  for (i=0; i<=Nx; i++) {                 //-2002-01-23
    fx = (i==0 || i==Nx) ? 0.5 : 1;
    for (j=0; j<=Ny; j++) {
      fy = (j==0 || j==Ny) ? 0.5 : 1;
      zg = iBOT(i,j);
      if (zg < zgmin)  zgmin = zg;
      if (zg > zgmax)  zgmax = zg;
      zgm += fx*fy*zg;
      }
    }
  zgm /= Nx*Ny;
if (CHECK) printf("WND: zgm=%1.1f\n", zgm);
  zg2m = 0;
  for (i=0; i<=Nx; i++) {                 //-2002-01-23
    fx = (i==0 || i==Nx) ? 0.5 : 1;
    for (j=0; j<=Ny; j++) {
      fy = (j==0 || j==Ny) ? 0.5 : 1;
      zg = iBOT(i,j);
      zg2m += fx*fy*(zg-zgm)*(zg-zgm);
      }
    }
  zg2m /= Nx*Ny;
if (CHECK) printf("WND: zg2m=%1.1f\n", zg2m);
  g2m = 0;
  for (i=1; i<=Nx; i++)
    for (j=1; j<=Ny; j++) {
      z11 = iBOT(i,j);
      z00 = iBOT(i-1,j-1);
      z01 = iBOT(i-1,j);
      z10 = iBOT(i,j-1);
      zm = 0.25*(z00+z01+z10+z11);
      g2m += z11*z11 + z01*z01 + z10*z10 + z00*z00 - 4*zm*zm;
    }
  g2m /= Nx*Ny*Dd*Dd;
if (CHECK) printf("WND: Dd=%1.1f  g2m=%1.3f\n", Dd, g2m);
  hgc = 4*sqrt(zg2m);                                   //-00-02-27
  lgc = (g2m > 0) ? 0.5*hgc/sqrt(g2m) : hgc;
if (CHECK) printf("WND: hgc=%1.2f  lgc=%1.2f\n", hgc, lgc);
  if (phg)  *phg = hgc;
  if (plg)  *plg = lgc;
  if (phb)  *phb = 0;
  return 0;
  }

/*===================================================================== UvwInit
*/
static int UvwInit( int step )
  {
  dP(UvwInit);
  int i, j, k, nk, ka;
  float ah, av, h, avsum;
  float zg, ts, z, ts0, za;
  float h1, th, th1, sr2, tcc;
  TRBREC *pt;
  BLMREC *pm;
  float hc, vc, lc;
  char name[256];					//-2004-11-26
  TXTSTR hdr = { NULL, 0 };
  vLOG(4)("WND:UvwInit(%d)", step);
  vc = Ua;
  hc = 0;
  lc = 0;
  tcc = 0;

  Pwin = TmnAttach(Iwin, &MetT1, &MetT2, 0, NULL);                  eG(10);
  Pwnd = TmnCreate(Iwnd, sizeof(WNDREC), 3, 0, Nx, 0, Ny, 0, Nz);   eG(20);
  strcpy(name, NmsName(Iwnd));
  if (Pwin->ttlsz != Pwnd->ttlsz)                                   eX(21);
  memcpy(Pwnd->start, Pwin->start, Pwin->ttlsz);
  WrtHeader(&hdr, "PXYS");                                          eG(23);
  TmnDetach(Iwnd, &MetT1, &MetT2, TMN_MODIFY, &hdr);                eG(24);
  Pwnd = NULL;
  TxtClr(&hdr);
  TmnDetach(Iwin, NULL, NULL, 0, NULL);                             eG(25);
  Pwin = NULL;

  Ian = TmnIdent();
  Pan = TmnCreate(Ian, sizeof(float), 3, 0, Nx, 0, Ny, 0, Nz);      eG(11);
  Ibn = TmnIdent();
  Pbn = TmnCreate(Ibn, sizeof(float), 3, 0, Nx, 0, Ny, 0, Nz);      eG(12);
  Icn = TmnIdent();
  Pcn = TmnCreate(Icn, sizeof(float), 3, 0, Nx, 0, Ny, 0, Nz);      eG(13);
  if (Aver <= 0.0) {
    ts0 = NOT_DEFINED;
    if (Tchar <= 0) {
      hc = CharHg;                                          //-2002-02-04
      lc = (UseCharLg) ? CharLg : hc;                       //-2002-02-04
    }
    av = 1;
    ah = 1;
    if (!Ptrb)  ReadTurb();                                         eG(3);
  }
  else {
    Ptrb = NULL;
    ts0 = 0;
    av = Aver;
    ah = 1/av;
  }
  vLOG(4)("WND:UvwInit  Av=%1.2f Tchar=%1.0f hc=%1.0f lc=%1.0f vc=%1.2f",
    Aver, Tchar, hc, lc, vc);
  nk = Pba->bound[0].hgh;
  pm = AryPtrX(Pba,0);
  za = pm->z;
  avsum = 0;
  for (i=0; i<=Nx; i++)
    for (j=0; j<=Ny; j++) {
      zg = iZP(i,j,0);
      for (k=0; k<=Nz; k++) {
        ts = ts0;
        z = iZP(i,j,k);
        h = z - zg;
        // vc = ClcCharVel( k );
        pm = AryPtr(Pba,k);  if (!pm)                              eX(50);
        vc = pm->g;
        if (vc < 1)  vc = 1;
        if (ts == NOT_DEFINED) {
          if (Ptrb) {
            pt = (TRBREC*) AryPtr(Ptrb, i, j, k);  if (!pt)        eX(7);
            th = pt->th;
            if (k > 0)  ts = (th-th1)/(h-h1);
            th1 = th;
          }
          else {
            ka = z - za + 0.5;
            if (ka < 1)   ka = 1;
            if (ka > nk)  ka = nk;
            pm = AryPtr(Pba,k);  if (!pm)                          eX(51);
            ts = pm->ths;
          }
        }  /* if (ts == NOT_DEFINED) */
        h1 = h;
        /* if (k == 0)  continue; */
        if ((Aver<=0.0) && (ts!=NOT_DEFINED)) {
          if (ts < 0.0)  ts = 0;
          if (Tchar > 0.0)  tcc = Tchar*Tchar;
          else  tcc = lc*hc/(vc*vc);                    //-2002-02-04
          sr2 = tcc*Grav*ts/(2*AvTmp);
          av = sr2 + sqrt(1+sr2*sr2);
          if (av > AvMax)  av = AvMax;
          ah = 1/av;
        }
        if ((MsgFile) && (StdLogLevel>=7)) fprintf(MsgFile,
          "(%d,%d,%d): ts=%1.3f ah=%1.2f av=%1.2f\n", i, j, k, ts, ah, av);
        iAN(i,j,k) = ah;
        iBN(i,j,k) = ah;
        iCN(i,j,k) = av;
        avsum += av;
      }  /* for (k.. ) */
    }  /* for (j.. ) */
  avsum /= (Nx+1)*(Ny+1)*(Nz+1);
  vLOG(3)("WND: <av>=%1.2f", avsum);
  if (step == WND_TERRAIN)  AvMean = avsum;

  for (i=Nx; i>=0; i--)
    for (j=Ny; j>0; j--)
      for (k=Nz; k>0; k--)
        iAN(i,j,k) = 0.25*(iAN(i,j,k) + iAN(i,j-1,k)
                     + iAN(i,j,k-1) + iAN(i,j-1,k-1));
  for (i=Nx; i>0; i--)
    for (j=Ny; j>=0; j--)
      for (k=Nz; k>0; k--)
        iBN(i,j,k) = 0.25*(iBN(i,j,k) + iBN(i-1,j,k)
                     + iBN(i,j,k-1) + iBN(i-1,j,k-1));
  for (i=Nx; i>0; i--)
    for (j=Ny; j>0; j--)
      for (k=Nz; k>0; k--)
        iCN(i,j,k) = 0.25*(iCN(i,j,k) + iCN(i-1,j,k)
                     + iCN(i,j-1,k) + iCN(i-1,j-1,k));
  if (Ptrb) {
    TmnDetach(Itrb, NULL, NULL, 0, NULL);               eG(52);
    Ptrb = NULL;
  }
  return 0;
eX_3:
  eMSG(_cant_read_turb_);
eX_7:
  eMSG(_index_error_);
eX_10:
  eMSG(_cant_get_init_);
eX_11: eX_12: eX_13:
  eMSG(_cant_create_array_);
eX_20:
  eMSG(_cant_create_field_$_, name);
eX_21:
  eMSG(_inconsistent_size_);
eX_23:
  eMSG(_cant_write_header_);
eX_24: eX_25:
eX_50: eX_51: eX_52:
  eMSG(_internal_error_);
}

/*=============================================================== ClcVelX
*/
static int ClcVelX( int i, int j, int k, int *ll, float *ff )
  {
  dP(ClcVelX);
  int n, nn;
  int l0m, l00, l01, l1m, l10, l11;
  float f0m, f00, f01, f1m, f10, f11;
  float z0m, z00, z01, z1m, z10, z11;
  float an, z0, zp, z1, g0, g1;
  if (i<0 || i>Nx || j<1 || j>Ny || k<1 || k>Nz)        eX(1);
  if (SLD(i,j,k) & GRD_ARXSET)  return 0;
  zp = 0.25*(iZP(i,j,k)+iZP(i,j-1,k)+iZP(i,j,k-1)+iZP(i,j-1,k-1));
  n = 0;
  an = iAN(i,j,k);
  l0m =-1;  l1m =-1;  l00 =-1;  l01 =-1;  l10 =-1;  l11 =-1;
  g0 = 0;  f0m = 0;
  g1 = 0;  f1m = 0;
  if (i > 0) {
    z0 = 0.25*(iZP(i-1,j,k)+iZP(i-1,j-1,k)+iZP(i-1,j,k-1)+iZP(i-1,j-1,k-1));
    g0 = (zp-z0)/Dd;
    l0m = OFF(i,j,k);  if (l0m < 0)                     eX(2);
    z0m = iZM(i,j,k);
    l00 = OFF(i,j,k-1);
    if (l00 >= 0) {
      z00 = iZM(i,j,k-1);
      f0m += 1/(z0m-z00);
      f00 = -1/(z0m-z00);
      }
    l01 = OFF(i,j,k+1);
    if (l01 >= 0) {
      z01 = iZM(i,j,k+1);
      f0m -= 1/(z01-z0m);
      f01 = 1/(z01-z0m);
      if (l00 >= 0) {
        f01 -= 1/(z01-z00);
        f00 += 1/(z01-z00);
        }
      }
    }
  if (i < Nx) {
    z1 = 0.25*(iZP(i+1,j,k)+iZP(i+1,j-1,k)+iZP(i+1,j,k-1)+iZP(i+1,j-1,k-1));
    g1 = (z1-zp)/Dd;
    l1m = OFF(i+1,j,k);  if (l1m < 0)                     eX(3);
    z1m = iZM(i+1,j,k);
    l10 = OFF(i+1,j,k-1);
    if (l10 >= 0) {
      z10 = iZM(i+1,j,k-1);
      f1m += 1/(z1m-z10);
      f10 = -1/(z1m-z10);
      }
    l11 = OFF(i+1,j,k+1);
    if (l11 >= 0) {
      z11 = iZM(i+1,j,k+1);
      f1m -= 1/(z11-z1m);
      f11 = 1/(z11-z1m);
      if (l10 >= 0) {
        f11 -= 1/(z11-z10);
        f10 += 1/(z11-z10);
        }
      }
    }
  if ((l00>=0 || l01>=0) && (l10>=0 || l11>=0)) {
    g0 *= 0.5;
    g1 *= 0.5;
    }
  if (l0m >= 0) { ll[n] = l0m;  ff[n] = (-1/Dd - g0*f0m)/an;  n++; }
  if (l00 >= 0) { ll[n] = l00;  ff[n] = -g0*f00/an;  n++; }
  if (l01 >= 0) { ll[n] = l01;  ff[n] = -g0*f01/an;  n++; }
  if (l1m >= 0) { ll[n] = l1m;  ff[n] = (1/Dd - g1*f1m)/an;  n++; }
  if (l10 >= 0) { ll[n] = l10;  ff[n] = -g1*f10/an;  n++; }
  if (l11 >= 0) { ll[n] = l11;  ff[n] = -g1*f11/an;  n++; }
  nn = n;
  return nn;
eX_1:
  eMSG(_index_$$$_, i, j, k);
eX_2: eX_3:
  eMSG(_singular_$$$_, i, j, k);
  }

/*=============================================================== ClcVelY
*/
static int ClcVelY( int i, int j, int k, int *ll, float *ff )
  {
  dP(ClcVelY);
  int n, nn;
  int l0m, l00, l01, l1m, l10, l11;
  float f0m, f00, f01, f1m, f10, f11;
  float z0m, z00, z01, z1m, z10, z11;
  float bn, z0, zp, z1, g0, g1;
  if (i<1 || i>Nx || j<0 || j>Ny || k<1 || k>Nz)        eX(1);
  if (SLD(i,j,k) & GRD_ARYSET)  return 0;
  zp = 0.25*(iZP(i,j,k)+iZP(i-1,j,k)+iZP(i,j,k-1)+iZP(i-1,j,k-1));
  n = 0;
  bn = iBN(i,j,k);
  l0m =-1;  l1m =-1;  l00 =-1;  l01 =-1;  l10 =-1;  l11 =-1;
  g0 = 0;  f0m = 0;
  g1 = 0;  f1m = 0;
  if (j > 0) {
    z0 = 0.25*(iZP(i,j-1,k)+iZP(i-1,j-1,k)+iZP(i,j-1,k-1)+iZP(i-1,j-1,k-1));
    g0 = (zp-z0)/Dd;
    l0m = OFF(i,j,k);  if (l0m < 0)                     eX(2);
    z0m = iZM(i,j,k);
    l00 = OFF(i,j,k-1);
    if (l00 >= 0) {
      z00 = iZM(i,j,k-1);
      f0m += 1/(z0m-z00);
      f00 = -1/(z0m-z00);
      }
    l01 = OFF(i,j,k+1);
    if (l01 >= 0) {
      z01 = iZM(i,j,k+1);
      f0m -= 1/(z01-z0m);
      f01 = 1/(z01-z0m);
      if (l00 >= 0) {
        f01 -= 1/(z01-z00);
        f00 += 1/(z01-z00);
        }
      }
    }
  if (j < Ny) {
    z1 = 0.25*(iZP(i,j+1,k)+iZP(i-1,j+1,k)+iZP(i,j+1,k-1)+iZP(i-1,j+1,k-1));
    g1 = (z1-zp)/Dd;
    l1m = OFF(i,j+1,k);  if (l1m < 0)                     eX(2);
    z1m = iZM(i,j+1,k);
    l10 = OFF(i,j+1,k-1);
    if (l10 >= 0) {
      z10 = iZM(i,j+1,k-1);
      f1m += 1/(z1m-z10);
      f10 = -1/(z1m-z10);
      }
    l11 = OFF(i,j+1,k+1);
    if (l11 >= 0) {
      z11 = iZM(i,j+1,k+1);
      f1m -= 1/(z11-z1m);
      f11 = 1/(z11-z1m);
      if (l10 >= 0) {
        f11 -= 1/(z11-z10);
        f10 += 1/(z11-z10);
        }
      }
    }
  if ((l00>=0 || l01>=0) && (l10>=0 || l11>=0)) {
    g0 *= 0.5;
    g1 *= 0.5;
    }
  if (l0m >= 0) { ll[n] = l0m;  ff[n] = (-1/Dd - g0*f0m)/bn;  n++; }
  if (l00 >= 0) { ll[n] = l00;  ff[n] = -g0*f00/bn;  n++; }
  if (l01 >= 0) { ll[n] = l01;  ff[n] = -g0*f01/bn;  n++; }
  if (l1m >= 0) { ll[n] = l1m;  ff[n] = (1/Dd - g1*f1m)/bn;  n++; }
  if (l10 >= 0) { ll[n] = l10;  ff[n] = -g1*f10/bn;  n++; }
  if (l11 >= 0) { ll[n] = l11;  ff[n] = -g1*f11/bn;  n++; }
  nn = n;
  return nn;
eX_1:
  eMSG(_index_$$$_, i, j, k);
eX_2:
  eMSG(_singular_$$$_, i, j, k);
  }

/*================================================================== AddLform
*/
static int AddLform( int *li, float *fi, int ni,
float f, int *lj, float *fj, int nj )
  {
  dP(AddLform);
  int i, j, l;
  for (j=0; j<nj; j++) {
    l = lj[j];
    if (l < 0)  continue;
    for (i=0; i<ni; i++)
      if (li[i] == l) {
        fi[i] += f*fj[j];
        break;
        }
    if (i < ni)  continue;
    for (i=0; i<ni; i++)
      if (li[i] < 0) {
        li[i] = l;
        fi[i] = f*fj[j];
        break;
        }
    if (i >= ni)                eX(1);
    }
  for (i=ni-1; i>=0; i--)
    if (li[i] >= 0)  break;
  return i+1;
eX_1:
  eMSG(_overflow_equations_);
  }

/*=================================================================== ClcVelS
*/
static int ClcVelS( int i, int j, int k, int *ll, float *ff )
  {
  dP(ClcVelS);
  float xx[10], yy[10], zz[10], gg[10];
  float zp, x2m, y2m, z2m, xm, ym, zm, xzm, yzm, gsum, dx2, dy2, dz2;
  float g, ax, ay, az, hx, hy, hz;
  int n, nx, ny, nz;
  int llx[10], lly[10], llz[10];
  float ffx[10], ffy[10], ffz[10];
  if (i<1 || i>Nx || j<1 || j>Ny || k<0 || k>Nz)        eX(1);
  if (SLD(i,j,k) & GRD_ARZSET)  return 0;
  zp = 0.25*(iZP(i,j,k)+iZP(i,j-1,k)+iZP(i-1,j,k)+iZP(i-1,j-1,k));
  n = 0;
  ax = 0;
  ay = 0;
  if (k > 0) {
    ax += iAN(i-1,j,k) + iAN(i,j,k);
    ay += iBN(i,j-1,k) + iBN(i,j,k);
    n += 2;
    }
  if (k < Nz) {
    ax += iAN(i-1,j,k+1) + iAN(i,j,k+1);
    ay += iBN(i,j-1,k+1) + iBN(i,j,k+1);
    n += 2;
    }
  ax /= n;
  ay /= n;
  az = iCN(i,j,k);
  hx = (iZP(i,j-1,k)+iZP(i,j,k)-iZP(i-1,j-1,k)-iZP(i-1,j,k))/(2*Dd*ax);
  hy = (iZP(i-1,j,k)+iZP(i,j,k)-iZP(i-1,j-1,k)-iZP(i,j-1,k))/(2*Dd*ay);
  hz = 1/az;

  n = 0;
  if (k == 0) {
    llz[n] = -1;  zz[n] = -(iZM(i,j,1)-zp);  gg[n++] = 1;
    }
  else {
    llz[n] = OFF(i,j,k);  zz[n] = iZM(i,j,k)-zp;  gg[n++] = 1;
    }
  if (k == Nz) {
    llz[n] = -1;  zz[n] = -(iZM(i,j,Nz)-zp);  gg[n++] = 1;
    }
  else {
    llz[n] = OFF(i,j,k+1);  zz[n] = iZM(i,j,k+1)-zp;  gg[n++] = 1;
    }
  nz = n;
  zm = 0.5*(zz[0] + zz[1]);
  z2m = 0.5*(zz[0]*zz[0] + zz[1]*zz[1]);
  z2m -= zm*zm;
  for (n=0; n<nz; n++)
    if (llz[n] < 0)  ffz[n] = 0;
    else ffz[n] = 0.5*(zz[n]-zm)/z2m;

  n = 0;
  if (k > 0) {
    llx[n] = OFF(i-1,j,k);
    if (llx[n] >= 0) { xx[n] =-Dd;  zz[n] = iZM(i-1,j,k)-zp;  gg[n++] = 1; }
    llx[n] = OFF(i,j,k);
    if (llx[n] >= 0) { xx[n] = 0;   zz[n] = iZM(i,j,k)-zp;    gg[n++] = 1; }
    llx[n] = OFF(i+1,j,k);
    if (llx[n] >= 0) { xx[n] = Dd;  zz[n] = iZM(i+1,j,k)-zp;  gg[n++] = 1; }
    }
  if (k < Nz) {
    llx[n] = OFF(i-1,j,k+1);
    if (llx[n] >= 0) { xx[n] =-Dd;  zz[n] = iZM(i-1,j,k+1)-zp;  gg[n++] = 1; }
    llx[n] = OFF(i,j,k+1);
    if (llx[n] >= 0) { xx[n] = 0;   zz[n] = iZM(i,j,k+1)-zp;    gg[n++] = 1; }
    llx[n] = OFF(i+1,j,k+1);
    if (llx[n] >= 0) { xx[n] = Dd;  zz[n] = iZM(i+1,j,k+1)-zp;  gg[n++] = 1; }
    }
  nx = n;
  xm = 0;  zm = 0;  xzm = 0;  x2m = 0;  z2m = 0;  gsum = 0;
  for (n=0; n<nx; n++) {
    dx2 = xx[n]*xx[n];
    dz2 = zz[n]*zz[n];
    g = gg[n];
    gsum += g;
    x2m += g*dx2;
    z2m += g*dz2;
    xzm += g*xx[n]*zz[n];
    xm  += g*xx[n];
    zm  += g*zz[n];
    }
  if (gsum == 0)                        eX(2);
  x2m /= gsum;
  z2m /= gsum;
  xzm /= gsum;
  xm  /= gsum;
  zm  /= gsum;
  x2m -= xm*xm;
  z2m -= zm*zm;
  xzm -= xm*zm;
  if (x2m > 0) {
    for (n=0; n<nx; n++) {
      g = gg[n]/gsum;
      if (llx[n] < 0)  ffx[n] = 0;
      else  ffx[n] = g*(xx[n]-xm)/x2m;
      }
    AddLform(llx, ffx, 10, -xzm/x2m, llz, ffz, 2);          eG(3);
    }
  else  nx = 0;

  n = 0;
  if (k > 0) {
    lly[n] = OFF(i,j-1,k);
    if (lly[n] >= 0) { yy[n] =-Dd;  zz[n] = iZM(i,j-1,k)-zp;  gg[n++] = 1; }
    lly[n] = OFF(i,j,k);
    if (lly[n] >= 0) { yy[n] = 0;   zz[n] = iZM(i,j,k)-zp;    gg[n++] = 1; }
    lly[n] = OFF(i,j+1,k);
    if (lly[n] >= 0) { yy[n] = Dd;  zz[n] = iZM(i,j+1,k)-zp;  gg[n++] = 1; }
    }
  if (k < Nz) {
    lly[n] = OFF(i,j-1,k+1);
    if (lly[n] >= 0) { yy[n] =-Dd;  zz[n] = iZM(i,j-1,k+1)-zp;  gg[n++] = 1; }
    lly[n] = OFF(i,j,k+1);
    if (lly[n] >= 0) { yy[n] = 0;   zz[n] = iZM(i,j,k+1)-zp;    gg[n++] = 1; }
    lly[n] = OFF(i,j+1,k+1);
    if (lly[n] >= 0) { yy[n] = Dd;  zz[n] = iZM(i,j+1,k+1)-zp;  gg[n++] = 1; }
    }
  ny = n;
  ym = 0;  zm = 0;  yzm = 0;  y2m = 0;  z2m = 0;  gsum = 0;
  for (n=0; n<ny; n++) {
    dy2 = yy[n]*yy[n];
    dz2 = zz[n]*zz[n];
    g = gg[n];
    gsum += g;
    y2m += g*dy2;
    z2m += g*dz2;
    yzm += g*yy[n]*zz[n];
    ym  += g*yy[n];
    zm  += g*zz[n];
    }
  if (gsum == 0)                        eX(4);
  y2m /= gsum;
  z2m /= gsum;
  yzm /= gsum;
  ym  /= gsum;
  zm  /= gsum;
  y2m -= ym*ym;
  z2m -= zm*zm;
  yzm -= ym*zm;
  if (y2m > 0) {
    for (n=0; n<ny; n++) {
      g = gg[n]/gsum;
      if (lly[n] < 0)  ffy[n] = 0;
      else  ffy[n] = g*(yy[n]-ym)/y2m;
      }
    AddLform(lly, ffy, 10, -yzm/y2m, llz, ffz, 2);          eG(5);
    }
  else  ny = 0;

  for (n=0; n<10; n++) { ll[n] = -1;  ff[n] = 0; }
  n = AddLform(ll, ff, 10, hz, llz, ffz, 2);                 eG(6);
  n = AddLform(ll, ff, 10,-hx, llx, ffx, nx);                eG(7);
  n = AddLform(ll, ff, 10,-hy, lly, ffy, ny);                eG(8);
  return n;
eX_1:
  eMSG(_index_$$$_, i, j, k);
eX_2:  eX_4:
  eMSG(_singular_$$$_, i, j, k);
eX_3: eX_5: eX_6: eX_7: eX_8:
  eMSG(_error_matrix_);
  }

/*=============================================================== MakeTmatrix
*/
static int MakeTmatrix( char *ss )
  {
  dP(MakeTmatrix);
  MATREC *pm;
  static long id, t1, t2, mask;
  char name[256], t1s[40];
  int i, j, k, l, m;
  int ll[15], n;
  float ff[15], a, a0;
  TXTSTR hdr = { NULL, 0 };
  vLOG(4)("WND:MakeTmatrix(%s)", ss);
  if (*ss) {
    if (*ss == '-')
      switch(ss[1]) {
        case 'M':  sscanf(ss+2, "%lx", &mask);
                   break;
        case 'N':  sscanf(ss+2, "%08lx", &id);
                   break;
        case 'T':  strcpy(t1s, ss+2);
                   t1 = TmValue(t1s);
                   break;
        default:   ;
        }
    return 0;
    }
  if (!id)                                                      eX(2);
  t2 = t1;
  strcpy(name, NmsName(id));
  vLOG(4)("WND: creating %s", name);
  DefGrid(0);                                                   eG(1);
  Psld = TmnAttach(Isld, NULL, NULL, 0, NULL);  if (!Psld)      eX(10);
  AryCreate(&Aoff, sizeof(int), 3, 0, Nx+1, 0, Ny+1, 0, Nz+1);  eG(8);
  for (i=0; i<=Nx+1; i++)
    for (j=0; j<=Ny+1; j++)
      for (k=0; k<=Nz+1; k++)  OFF(i,j,k) = -1;
  l = 0;
  for (i=1; i<=Nx; i++)
    for (j=1; j<=Ny; j++)
      for (k=1; k<=Nz; k++)
        if (0 == (SLD(i,j,k)&GRD_VOLOUT&mask))  OFF(i,j,k) = l++;
  Nv = l;

  Tmx = TmnCreate(id, sizeof(MATREC), 1, 0, Nv-1);       eG(20);
  for (i=1; i<=Nx; i++)
    for (j=1; j<=Ny; j++)
      for (k=1; k<=Nz; k++) {
        l = OFF(i,j,k);
        if (l >= 0) {
          MAT(l).i = i;
          MAT(l).j = j;
          MAT(l).k = k;
          }
        }
  for (l=0; l<Nv; l++) {
    pm = &MAT(l);
    for (m=0; m<Nc; m++) {
      pm->ii[m] = -1;
      pm->aa[m] = 0;
      }
    i = pm->i;
    j = pm->j;
    k = pm->k;
    if (!(SLD(i-1,j,k) & GRD_ARXSET)) {
      a = iZP(i-1,j-1,k) + iZP(i-1,j,k) - iZP(i-1,j-1,k-1) - iZP(i-1,j,k-1);
      a *= 0.5*Dd;
      n = ClcVelX(i-1, j, k, ll, ff);                   eG(101);
      if (n > 0) {
        AddLform(pm->ii, pm->aa, Nc,-a, ll, ff, n);     eG(102);
        }
      }
    if (!(SLD(i,j,k) & GRD_ARXSET)) {
      a = iZP(i,j-1,k) + iZP(i,j,k) - iZP(i,j-1,k-1) - iZP(i,j,k-1);
      a *= 0.5*Dd;
      n = ClcVelX(i, j, k, ll, ff);                                 eG(103);
      if (n > 0) {
        AddLform(pm->ii, pm->aa, Nc, a, ll, ff, n);                 eG(104);
        }
      }
    if (!(SLD(i,j-1,k) & GRD_ARYSET)) {
      a = iZP(i-1,j-1,k) + iZP(i,j-1,k) - iZP(i-1,j-1,k-1) - iZP(i,j-1,k-1);
      a *= 0.5*Dd;
      n = ClcVelY(i, j-1, k, ll, ff);                               eG(201);
      if (n > 0) {
        AddLform(pm->ii, pm->aa, Nc,-a, ll, ff, n);                 eG(202);
        }
      }
    if (!(SLD(i,j,k) & GRD_ARYSET)) {
      a = iZP(i-1,j,k) + iZP(i,j,k) - iZP(i-1,j,k-1) - iZP(i,j,k-1);
      a *= 0.5*Dd;
      n = ClcVelY(i, j, k, ll, ff);                     eG(203);
      if (n > 0) {
        AddLform(pm->ii, pm->aa, Nc, a, ll, ff, n);     eG(204);
        }
      }
    if (!(SLD(i,j,k-1) & GRD_ARZSET)) {
      a = Dd*Dd;
      n = ClcVelS(i, j, k-1, ll, ff);                   eG(301);
      if (n > 0) {
        AddLform(pm->ii, pm->aa, Nc,-a, ll, ff, n);     eG(302);
        }
      }
    if (!(SLD(i,j,k) & GRD_ARZSET)) {
      a = Dd*Dd;
      n = ClcVelS(i, j, k, ll, ff);                     eG(303);
      if (n > 0) {
        AddLform(pm->ii, pm->aa, Nc, a, ll, ff, n);     eG(304);
        }
      }
    if (pm->ii[0] != l) {
      a0 = pm->aa[0];
      for (m=1; m<Nc; m++) {
        n = pm->ii[m];
        if (n == l) {
          pm->aa[0] = pm->aa[m];
          pm->aa[m] = a0;
          pm->ii[m] = pm->ii[0];
          pm->ii[0] = l;
          break;
          }
        }
      if (m >= Nc)                                      eX(21);
      }
    a = pm->aa[0];
    if (a == 0)                                         eX(22);
    if (!(l%10000)) {
      if (MsgQuiet <= 0) {
        fprintf(stdout, "%4d\r", l/10000);
        fflush(stdout);
      }
    }
  }
  if (StdDspLevel>1) printf("MATRIX(%4d)    \n", l);
  WrtHeader(&hdr, "a");                                         eG(7);
  TmnDetach(id, NULL, NULL, TMN_MODIFY, &hdr);                  eG(8);
  Tmx = NULL;
  TxtClr(&hdr);
  TmnDetach(Isld, NULL, NULL, 0, NULL);                         eG(11);
  Psld = NULL;
  return 0;

eX_2: eX_1: eX_7: eX_8:
eX_10: eX_11: eX_20:
eX_101: eX_102: eX_103: eX_104:
eX_201: eX_202: eX_203: eX_204:
eX_301: eX_302: eX_303: eX_304:
  eMSG(_internal_error_);
eX_21:
  eMSG(_no_diagonal_$_, l);
eX_22:
  eMSG(_zero_diagonal_$_, l);
  }

/*===================================================================== ClcMxIn
*/
static int ClcMxIn( long mask )
  {
  dP(ClcMxIn);
  char t[120];
  int i, j, k, l;
  float a, d, fx0, fx1, fy0, fy1, fz0, fz1;
  TXTSTR hdr = { NULL, 0 };
  vLOG(4)("WND:ClcMxIn()");
  if (!Imx) {
    vPRN(5)("WND: creating Matrix ...\n");
    Imx = TmnIdent();
    sprintf(t, "-N%08lx", Imx);   MakeTmatrix(t);
    sprintf(t, "-M%08lx", mask);  MakeTmatrix(t);
    MakeTmatrix("");                                                    eG(20);
    }
  Pwnd= TmnAttach(Iwnd, NULL, NULL, 0, NULL);  if (!Pwnd)               eX(10);
  Tmx = TmnAttach(Imx, NULL, NULL, 0, NULL);                            eG(1);
  Iin = TmnIdent();
  Pin = TmnCreate(Iin, sizeof(float), 1, 0, Nv-1);                      eG(2);
  for (l=0; l<Nv; l++) {
    i = MAT(l).i;
    j = MAT(l).j;
    k = MAT(l).k;
    d = 0;
    a = iZP(i-1,j-1,k) + iZP(i-1,j,k) - iZP(i-1,j-1,k-1) - iZP(i-1,j,k-1);
    a *= 0.5*Dd;
    fx0 = a*iWND(i-1,j,k).vx;
    a = iZP(i,j-1,k) + iZP(i,j,k) - iZP(i,j-1,k-1) - iZP(i,j,k-1);
    a *= 0.5*Dd;
    fx1 = a*iWND(i,j,k).vx;
    a = iZP(i-1,j-1,k) + iZP(i,j-1,k) - iZP(i-1,j-1,k-1) - iZP(i,j-1,k-1);
    a *= 0.5*Dd;
    fy0 = a*iWND(i,j-1,k).vy;
    a = iZP(i-1,j,k) + iZP(i,j,k) - iZP(i-1,j,k-1) - iZP(i,j,k-1);
    a *= 0.5*Dd;
    fy1 = a*iWND(i,j,k).vy;
    a = Dd*Dd;
    fz0 = a*iWND(i,j,k-1).vz;
    fz1 = a*iWND(i,j,k).vz;
    d = fx1 - fx0 + fy1 - fy0 + fz1 - fz0;
    iIN(l) = d;
    }
  TmnDetach(Iin,  NULL, NULL, TMN_MODIFY, NULL);                eG(7);
  TmnDetach(Imx,  NULL, NULL, 0, NULL);                         eG(8);
  TmnDetach(Iwnd, NULL, NULL, 0, NULL);                         eG(11);
  Ild = TmnIdent();
  Pld = TmnCreate( Ild, sizeof(float), 1, 0, Nv-1 );            eG(3);
  WrtHeader(&hdr, "V");                                         eG(5);
  TmnDetach(Ild, NULL, NULL, TMN_MODIFY, &hdr);                 eG(6);
  TxtClr(&hdr);
  return 0;
eX_20: eX_1:  eX_2:  eX_3:  eX_5:  eX_6:  eX_7:  eX_8:
eX_10: eX_11:
  eMSG(_internal_error_);
  }

/*============================================================== CheckSolution
*/
static int CheckSolution( float *psum, float *pmin, float *pmax )
  {
  float *aa, *xx, *rs;
  float d, dmin, dmax, dsum, g, gsum;
  MATREC *pm;
  int c, n, l, *ii;
  rs = Pin->start;
  xx = Pld->start;
  dmin = 0;
  dmax = 0;
  gsum = 0;
  dsum = 0;

  for (l=0; l<Nv; l++) {
    pm = &MAT(l);
    aa = pm->aa;
    ii = pm->ii;
    d = rs[l];
    for (c=0; c<Nc; c++) {
      n = ii[c];
      if (n < 0)  break;
      d -= aa[c]*xx[n];
      }
    g = iVM(pm->i, pm->j, pm->k);
    d /= g;
    if (d < dmin)  dmin = d;
    if (d > dmax)  dmax = d;
    dsum += g*d*d;
    gsum += g;
    }
  dsum /= gsum;
  if (psum)  *psum = sqrt(dsum);
  if (pmin)  *pmin = dmin;
  if (pmax)  *pmax = dmax;
  return 0;
  }

/*================================================================ SolveSystem
*/
static int SolveSystem( int step )
  {
  dP(SolveSystem);
  float dsum, dmin, dmax;
  float asum, amin, amax, epsilon, lsum;
  int iter, maxiter, setbnd, inside, nret, use_adi;
  int ndiv, search;
  char method[40];
  use_adi = 1;
  search = 0;
  strcpy(method, "ADI");
  vLOG(4)("WND:SolveSystem(step %d) %s", step, method);
  setbnd = (0 != (StdStatus & WND_SETBND));
  inside = (GridLevel > LowestLevel);
  epsilon =  Epsilon;
  maxiter =  MaxIter;
  Tmx = TmnAttach(Imx, NULL, NULL, 0, NULL);            eG(1);
  Pin = TmnAttach(Iin, NULL, NULL, 0, NULL);            eG(2);
  Pld = TmnAttach(Ild, NULL, NULL, TMN_MODIFY, NULL);   eG(3);
  Yy = ALLOC(Nv*sizeof(float));  if (!Yy)               eX(4);
  if (Omega <= 0)  Omega = 1.5;
  vLOG(4)("WND: grid=(%d,%d)  step=%d  omega=%1.3f  %s",
    GridLevel, GridIndex, step, Omega, method);

searching:

  if (search)  Omega = 1;
  nret = Nret;

  CheckSolution(&asum, &amin, &amax);
  vLOG(4)("%3d:  sum=%10.2e  min=%10.2e  max=%10.2e", 0, asum, amin, amax);
  dsum = -1;
  ndiv = 0;
  for (iter=0; iter<maxiter; iter++) {
    AdiIterate(&Aoff, Tmx, Pin->start, Pld->start, Omega, Nx, Ny, Nz);  eG(9);
    CheckSolution(&dsum, &dmin, &dmax);
    if (iter == 0)  lsum = dsum;
    vLOG(5)("%3d:  sum=%15.7e  min=%15.7e  max=%15.7e",
            iter+1, dsum, dmin, dmax);
    if (StdDspLevel>1 && StdDspLevel<5) {
      fprintf(stdout, "%3d:  sum=%10.2e  min=%10.2e  max=%10.2e", iter+1, dsum, dmin, dmax);
      fflush(stdout);
    }
    if (MsgQuiet <= 0) {
      fprintf(stdout, "%4d\r", iter);
      fflush(stdout);
    }
    if (dmin>-epsilon && dmax<epsilon)  break;
    if (dsum > lsum)  ndiv++;
    else  ndiv = 0;
    lsum = dsum;
    if (dsum>10*asum || ndiv>2) {
      vLOG(3)(_abort_$_, iter);
      iter = maxiter+1;
      break;
      }
    }
  if (StdDspLevel>1 && StdDspLevel<5)  fprintf(stdout, "\n");
  if (iter >= maxiter) {
    if (iter == maxiter) {
      vLOG(3)(_$_not_sufficient_$$$_, iter, GridLevel, GridIndex, step);
      }
    if (dsum>asum  || ndiv>2) {
      if (search)                                       eX(10);
      search = 1;
      goto searching;
      }
    }
  else vLOG(3)(_$_iterations_, iter);

  FREE(Yy);
  TmnDetach(Imx, NULL, NULL, 0, NULL);                  eG(6);
  Tmx = NULL;
  TmnDetach(Iin, NULL, NULL, 0, NULL);                  eG(7);
  Pin = NULL;

  TmnDetach(Ild, NULL, NULL, TMN_MODIFY, NULL);         eG(8);
  Pld = NULL;
  return 0;
eX_1: eX_2: eX_3: eX_4: eX_6: eX_7: eX_8: eX_9:
  eMSG(_internal_error_);
eX_10:
  eMSG(_no_convergence_$$$_, GridLevel, GridIndex, step);
  }

/*====================================================================== ClcUvw
*/
static int ClcUvw( int step, int set_volout )
  {
  dP(ClcUvw);
  int i, j, k, l, m, n, ll[10], zero_div;
  long mm;
  float ff[10], vx, vy, vz, a, d, dsum, dmin, dmax, g, gsum, vz0, vz1;
  WNDREC *pw;
  char name[256];						//-2004-11-26
  TXTSTR hdr = { NULL, 0 }, *phdr;                                //-2011-06-29
  vLOG(4)("WND:ClcUvw(step %d)", step);
  if (step == WND_FINAL) { //-2004-07
    phdr = &hdr;
    zero_div = 0;
  }
  else {
    phdr = NULL;
    zero_div = 0;
  }
  NmsSeq("index", BlmPprm->Wind);
  strcpy(name, NmsName(Iwnd));

  if (step != WND_FINAL) { //-2004-07
    Pwnd = TmnAttach(Iwnd, NULL, NULL, TMN_MODIFY, phdr);               eG(10);
    Psld = TmnAttach(Isld, NULL, NULL, 0, NULL);                        eG(20);
    Pld  = TmnAttach(Ild, NULL, NULL, 0, NULL);                         eG(2);
    for (i=0; i<=Nx; i++) {
      for (j=0; j<=Ny; j++)
        for (k=0; k<=Nz; k++) {
          mm = SLD(i,j,k);
          pw = &iWND(i,j,k);
          if (j>0 && k>0 && !(mm & GRD_ARXSET)) {
            n = ClcVelX(i, j, k, ll, ff);              eG(3);
            vx = 0;
            for (m=0; m<n; m++) {
              l = ll[m];
              if (l >= 0)  vx += ff[m]*iLD(l);
              }
            pw->vx -= vx;
            }
          if (i>0 && k>0 && !(mm & GRD_ARYSET)) {
            n = ClcVelY(i, j, k, ll, ff);              eG(4);
            vy = 0;
            for (m=0; m<n; m++) {
              l = ll[m];
              if (l >= 0)  vy += ff[m]*iLD(l);
              }
            pw->vy -= vy;
            }
          if (i>0 && j>0 && !(mm & GRD_ARZSET)) {
            n = ClcVelS(i, j, k, ll, ff);              eG(5);
            vz = 0;
            for (m=0; m<n; m++) {
              l = ll[m];
              if (l >= 0)  vz += ff[m]*iLD(l);
              }
            pw->vz -= vz;
            }
          }
      }
  }
  else {
    Pwnd = TmnAttach(Iwnd, NULL, NULL, 0, phdr);                        eG(10);
    Psld = TmnAttach(Isld, NULL, NULL, 0, NULL);                        eG(20);
    set_volout = 0;
  }
  dsum = 0;
  gsum = 0;
  dmin = 0;
  dmax = 0;
  for (i=1; i<=Nx; i++)
    for (j=1; j<=Ny; j++) {
      vz0 = 0;
      for (k=1; k<=Nz; k++) {
        if (SLD(i,j,k) & GRD_VOLOUT)
          if (set_volout) iWND(i,j,k-1).vz = WND_VOLOUT;
        if (iWND(i,j,k-1).vz == WND_VOLOUT)
          continue;
        d = 0;
        a = iZP(i-1,j-1,k) + iZP(i-1,j,k) - iZP(i-1,j-1,k-1) - iZP(i-1,j,k-1);
        a *= 0.5*Dd;
        d -= a*iWND(i-1,j,k).vx;
        a = iZP(i,j-1,k) + iZP(i,j,k) - iZP(i,j-1,k-1) - iZP(i,j,k-1);
        a *= 0.5*Dd;
        d += a*iWND(i,j,k).vx;
        a = iZP(i-1,j-1,k) + iZP(i,j-1,k) - iZP(i-1,j-1,k-1) - iZP(i,j-1,k-1);
        a *= 0.5*Dd;
        d -= a*iWND(i,j-1,k).vy;
        a = iZP(i-1,j,k) + iZP(i,j,k) - iZP(i-1,j,k-1) - iZP(i,j,k-1);
        a *= 0.5*Dd;
        d += a*iWND(i,j,k).vy;
        a = Dd*Dd;
        vz1 = iWND(i,j,k).vz;
        if (zero_div && !(SLD(i,j,k) & GRD_ARZSET))
          iWND(i,j,k).vz = iWND(i,j,k-1).vz - d/a;
        d -= a*vz0;
        d += a*vz1;
        vz0 = vz1;
        g = iVM(i,j,k);
        d /= g;
        if (d > dmax)  dmax = d;
        if (d < dmin)  dmin = d;
        dsum += g*d*d;
        gsum += g;
        }
      }
  dsum /= gsum;
  dsum = sqrt(dsum);
  vLOG(3)("WND: mean=%10.2e  min=%10.2e  max=%10.2e ", dsum, dmin, dmax);
  if (step == WND_FINAL) {
    if (dmax < -dmin)  dmax = -dmin;
    if (dmax*Dd/Ua > MaxDiv) {
      MaxDiv = dmax*Dd/Ua;
      MaxDivWind = BlmPprm->Wind;
    }
    if (GrdPprm->numnet > 0)
      vLOG(1)(_divergence_$$$$_,
        dmax*Dd/Ua, BlmPprm->Wind, GridLevel, GridIndex);
    else
      vLOG(1)(_divergence_$$_, dmax*Dd/Ua, BlmPprm->Wind);
    if (dmax*Dd/Ua > MAXRELDIV)                                 eX(30);
  }
  if (step != WND_FINAL) { //-2004-07
    TmnDetach(Iwnd, NULL, NULL, TMN_MODIFY, phdr);              eG(14);
  }
  else {
    TmnDetach(Iwnd, NULL, NULL, 0, phdr);                       eG(14);
  }
  Pwnd = NULL;
  TmnDetach(Isld, NULL, NULL, 0, NULL);                         eG(21);
  Psld = NULL;
  if (step != WND_FINAL) { //-2004-07
    TmnDetach(Ild, NULL, NULL, 0, NULL);                        eG(22);
    Pld = NULL;
  }
  TxtClr(&hdr);
  return 0;
eX_2:  eX_3:  eX_4:  eX_5:
  eMSG(_internal_error_);
eX_10:
  eMSG(_cant_get_field_$_, name);
eX_14: eX_20: eX_21: eX_22:
  eMSG(_internal_error_);
eX_30:
  eMSG(_divergence_too_large_$_, MAXRELDIV);
  }

/*================================================================= InsertLayer
*/
static int InsertLayer( void )
  {
  dP(InsertLayer);
  int i, j, k;
  float z0, d0, rghlen, zpldsp, h;
  float t0, t1, b0, b1, g0, g1, gm, z00, z01, z10, z11;
  ARYDSC *psrf;
  SRFREC *ps;
  vLOG(4)("WND:InsertLayer() ");

  rghlen = BlmPprm->RghLen;
  zpldsp = BlmPprm->ZplDsp;
  Pwnd = TmnAttach(Iwnd, NULL, NULL, TMN_MODIFY, NULL);  if (!Pwnd)     eX(2);
  psrf = TmnAttach(Isrf1, NULL, NULL, 0, NULL);  if (!psrf)             eX(10);

  for (i=0; i<=Nx; i++) {
    for (j=1; j<=Ny; j++) {
      t0 = g0 = iWND(i,j-1,0).z;
      t1 = g1 = iWND(i,j,0).z;
      gm = 0.5*(g0 + g1);
      ps = AryPtr(psrf, i, j);  if (!ps)                                   eX(11);
      if (ps->z0 > 0) {
        z0 = ps->z0;
        d0 = ps->d0;
        ps = AryPtr(psrf, i, j-1);  if (!ps)                               eX(12);
        z0 = 0.5*(z0 + ps->z0);
        d0 = 0.5*(d0 + ps->d0);
        }
      else { z0 = rghlen;  d0 = zpldsp; }
      for (k=1; k<=Nz; k++) {
        b0 = t0;  t0 = iWND(i,j-1,k).z;
        b1 = t1;  t1 = iWND(i,j,k).z;
        h = 0.25*(b0 + b1 + t0 + t1) - gm;
        iWND(i,j,k).vx *= BlmSrfFac(z0, d0, h);
        }
      }
    }

  for (j=0; j<=Ny; j++) {
    for (i=1; i<=Nx; i++) {
      t0 = g0 = iWND(i-1,j,0).z;
      t1 = g1 = iWND(i,j,0).z;
      gm = 0.5*(g0 + g1);
      ps = AryPtr(psrf, i, j);  if (!ps)                                   eX(13);
      if (ps->z0 > 0) {
        z0 = ps->z0;
        d0 = ps->d0;
        ps = AryPtr(psrf, i-1, j);  if (!ps)                               eX(14);
        z0 = 0.5*(z0 + ps->z0);
        d0 = 0.5*(d0 + ps->d0);
        }
      else { z0 = rghlen;  d0 = zpldsp; }
      for (k=1; k<=Nz; k++) {
        b0 = t0;  t0 = iWND(i-1,j,k).z;
        b1 = t1;  t1 = iWND(i,j,k).z;
        h = 0.25*(b0 + b1 + t0 + t1) - gm;
        iWND(i,j,k).vy *= BlmSrfFac(z0, d0, h);
        }
      }
    }

  for (i=1; i<=Nx; i++) {
    for (j=1; j<=Ny; j++) {
      z00 = iWND(i-1,j-1,0).z;
      z01 = iWND(i-1,j,0).z;
      z10 = iWND(i,j-1,0).z;
      z11 = iWND(i,j,0).z;
      gm = 0.25*(z00 + z01 + z10 + z11);
      ps = AryPtr(psrf, i, j);  if (!ps)                                   eX(15);
      if (ps->z0 > 0) {
        z0 = ps->z0;  d0 = ps->d0;
        ps = AryPtr(psrf, i, j-1);  if (!ps)                               eX(16);
        z0 += ps->z0;  d0 += ps->d0;
        ps = AryPtr(psrf, i-1, j);  if (!ps)                               eX(17);
        z0 += ps->z0;  d0 += ps->d0;
        ps = AryPtr(psrf, i-1, j-1);  if (!ps)                             eX(18);
        z0 = 0.25*(z0 + ps->z0);
        d0 = 0.25*(d0 + ps->d0);
        }
      else { z0 = rghlen;  d0 = zpldsp; }
      for (k=1; k<=Nz; k++) {
        z00 = iWND(i-1,j-1,k).z;
        z10 = iWND(i,j-1,k).z;
        z01 = iWND(i-1,j,k).z;
        z11 = iWND(i,j,k).z;
        h = 0.25*(z00+z01+z10+z11) - gm;
        iWND(i,j,k).vz *= BlmSrfFac(z0, d0, h);
        }
      }
    }
  InsertBoundary(Pwnd, GrdPprm, WND_FINAL);                     eG(4); 
  TmnDetach(Isrf1, NULL, NULL, 0, NULL);                        eG(19);
  TmnDetach(Iwnd, NULL, NULL, TMN_MODIFY, NULL);                eG(3);
  Pwnd = NULL;

  return 0;
eX_2: eX_3: eX_4:
  eMSG(_cant_get_field_);
eX_10: eX_11: eX_12: eX_13: eX_14: eX_15: eX_16: eX_17: eX_18: eX_19:
  eMSG(_no_surface_info_);
  }

/*=================================================================== ClrArrays
*/
static void ClrArrays( int step )
  {
  dP(ClrArrays);
  char name[256];
  vLOG(4)("ClrArrays(step %d)", step);
  TmnDetach(Ian, NULL, NULL, TMN_MODIFY, NULL);                 eG(40);
  TmnDetach(Ibn, NULL, NULL, TMN_MODIFY, NULL);                 eG(41);
  TmnDetach(Icn, NULL, NULL, TMN_MODIFY, NULL);                 eG(42);
  if (StdCheck & 0x10) {
    sprintf(name, "ian%d.dmna", step);  TmnArchive(Ian, name, 0);    eG(30);
    sprintf(name, "ibn%d.dmna", step);  TmnArchive(Ibn, name, 0);    eG(31);
    sprintf(name, "icn%d.dmna", step);  TmnArchive(Icn, name, 0);    eG(32);
    sprintf(name, "mat%d.dmna", step);  TmnArchive(Imx, name, 0);    eG(34);
    }
  TmnDelete(TmMax(), Ian, Ibn, Icn, Imx, Iin, TMN_NOID);            eG(4);
  Ian = 0;  Ibn = 0;  Icn = 0;  Imx = 0;  Iin = 0;
  if (Cc)  FREE(Cc);
  Cc = NULL;

  if (Iwin) {
    if ((step==WND_TERRAIN) && (WrtMask & (1 << WND_BASIC))) {
      if (!STDSILENT) printf("%s", _writing_init_);               //-2011-11-21
      TmnArchive(Iwin, "", 0);                                  eG(33);
      }
    }

  TmnDelete(TmMax(), Ild, TMN_NOID);                            eG(5);
  Ild = 0;
  AryFree(&Aoff);                                               eG(7);

  if (Psld)  TmnDetach(Isld, NULL, NULL, TMN_MODIFY, NULL);     eG(3);
  Psld = NULL;

  return;
eX_3:  eX_4:  eX_5:  eX_7:
eX_30: eX_31: eX_32: eX_33: eX_34:
eX_40: eX_41: eX_42:
  nMSG(_internal_error_);
  return;
  }

//================================================================== writeDmna
//
static int writeDmna( ARYDSC *pa, char *hdr, char *name, char *valdef ) {
  dP(writeDmna);
  char fn[256], path[256], *pc, *pn, s[256], dmk[256];
  int i, j, k, rc, nx, ny, nz;
  ARYDSC dsc;
  WNDREC *pw;
  float *pf;
  TXTSTR usr = { NULL, 0 };
  nx = pa->bound[0].hgh;
  ny = pa->bound[1].hgh;
  nz = pa->bound[2].hgh;
  strcpy(path, StdPath);
  pn = path;                                                 //-2007-02-03
  while (strstr(pn, "/work")) {
    pc = strstr(pn, "/work");
    pn++;
  }
  if (pc)  pc[1] = 0;
  strcat(path, "lib");
  if (0 > TutDirMake(path))                                   eX(10);
  pn = strrchr(name, '/');
  if (pn)  pn++;
  else pn = name;
  TxtCpy(&usr, hdr);                                              //-2011-06-29
  DmnRplValues(&usr, "mode", "\"text\"");
  DmnRplName(&usr, "valdef", "vldf");
  DmnRplName(&usr, "format", "form");
  DmnRplName(&usr, "arrtype", "artp");
  sprintf(s, "%1.1f", Gx);                               //-2003-10-12
  DmnRplValues(&usr, "refx", s);
  sprintf(s, "%1.1f", Gy);
  DmnRplValues(&usr, "refy", s);
  if (Gx > 0 && *TI.ggcs) {                                       //-2008-12-11
    DmnRplValues(&usr, "ggcs", TxtQuote(TI.ggcs));                //-2008-12-11
  }
  DmnRplValues(&usr, "axes", "\"xyz\"");                          //-2005-08-06
  if (!strcmp(valdef, "P")) {
    //
    // zg
    //
    memset(&dsc, 0, sizeof(ARYDSC));
    DmnRplValues(&usr, "form", "zg%7.2f");
    DmnRplValues(&usr, "vldf", "\"P\"");
    DmnRplValues(&usr, "artp", "\"T\"");
    DmnRplValues(&usr, "axes", "\"xy\"");
    AryCreate(&dsc, sizeof(float), 2, 0, nx, 0, ny);            eG(1);
    for (i=0; i<=nx; i++)
      for (j=0; j<=ny; j++) {
        pw = AryPtr(pa, i, j, 0);  if (!pw)                     eX(2);
        pf = AryPtr(&dsc, i, j, 0);  if (!pf)                   eX(3);
        *pf = pw->z;
      }
    sprintf(fn, "%s/%s%s", path, "zg", pn+6);
    pc = strrchr(fn, '.');
    if (pc)  *pc = 0;
    rc = DmnWrite(fn, usr.s, &dsc);  if (rc < 0)                eX(4);
    AryFree(&dsc);
    //
    // zp
    //
    memset(&dsc, 0, sizeof(ARYDSC));
    DmnRplValues(&usr, "form", "zp%7.2f");
    DmnRplValues(&usr, "vldf", "\"P\"");
    DmnRplValues(&usr, "artp", "\"G\"");
    DmnRplValues(&usr, "axes", "\"xyz\"");                        //-2005-08-06
    AryCreate(&dsc, sizeof(float), 3, 0, nx, 0, ny, 0, nz);     eG(11);
    for (i=0; i<=nx; i++)
      for (j=0; j<=ny; j++)
        for (k=0; k<=nz; k++) {
          pw = AryPtr(pa, i, j, k);  if (!pw)                      eX(12);
          pf = AryPtr(&dsc, i, j, k);  if (!pf)                    eX(13);
          *pf = pw->z;
        }
    sprintf(fn, "%s/%s%s", path, "zp", pn+6);
    pc = strrchr(fn, '.');
    if (pc)  *pc = 0;
    rc = DmnWrite(fn, usr.s, &dsc);  if (rc < 0)                   eX(14);
    AryFree(&dsc);
    return 0;
  }
  if (WrtMask & 0x20000) {                                      //-2001-10-22
    if (Nb && GridLevel >= HighestLevel-1) {                    //-2005-03-23
      strcpy(dmk, "");                                          //-2005-03-10
      for (i=1; i<=9; i++) {
        sprintf(s, " %s", GenForm(BdsDMKp[i],6));
        strcat(dmk, s);
      }
      DmnRplValues(&usr, "dmkp", dmk);
      if (DMKpar.flags & DMK_USEHM)
        DmnRplValues(&usr, "dmkf", "\"DMKHM\"");
    }
    //
    if (!strcmp(valdef, "PXYS")) {
      //
      // WNDREC
      //
      DmnRplValues(&usr, "mode", WriteBinary ? "\"binary\"" : "\"text\""); //-2005-02-11
      DmnRplValues(&usr, "form", "Zp%7.2fVx%7.3fVy%7.3fVs%7.3f");
      DmnRplValues(&usr, "vldf", "\"PXYS\"");
      DmnRplValues(&usr, "artp", "\"W\"");
      sprintf(fn, "%s/%s%s", path, "w", pn+1);
      pc = strrchr(fn, '.');
      if (pc)  *pc = 0;
      rc = DmnWrite(fn, usr.s, pa);  if (rc < 0)                    eX(15);
      TxtClr(&usr);
    }
    else if (!strcmp(valdef, "PPPP")) {
      //
      // TRBREC
      //
      DmnRplValues(&usr, "mode", WriteBinary ? "\"binary\"" : "\"text\""); //-2005-02-11
      DmnRplValues(&usr, "form", "Su%[3]6.2fTh%6.2f");
      DmnRplValues(&usr, "vldf", "\"PPPP\"");
      DmnRplValues(&usr, "artp", "\"V\"");
      sprintf(fn, "%s/%s%s", path, "v", pn+1);
      pc = strrchr(fn, '.');
      if (pc)  *pc = 0;
      rc = DmnWrite(fn, usr.s, pa);  if (rc < 0)                    eX(16);
      TxtClr(&usr);
    }
    else if (!strcmp(valdef, "PP")) {
      //
      // DFFREC
      //
      DmnRplValues(&usr, "mode", WriteBinary ? "\"binary\"" : "\"text\""); //-2005-02-11
      DmnRplValues(&usr, "form", "Kh%6.2fKv%6.2f");
      DmnRplValues(&usr, "vldf", "\"PP\"");
      DmnRplValues(&usr, "artp", "\"K\"");       //-2004-12-06
      sprintf(fn, "%s/%s%s", path, "k", pn+1);
      pc = strrchr(fn, '.');
      if (pc)  *pc = 0;
      rc = DmnWrite(fn, usr.s, pa);  if (rc < 0)                    eX(17);
      TxtClr(&usr);
    }
    return 0;
  }
  //
  // vx
  //
  memset(&dsc, 0, sizeof(ARYDSC));
  DmnRplValues(&usr, "mode", "\"text\"");
  DmnRplValues(&usr, "form", "vx%7.3f");
  DmnRplValues(&usr, "vldf", "\"X\"");
  DmnRplValues(&usr, "artp", "\"W\"");
  AryCreate(&dsc, sizeof(float), 3, 0, nx, 1, ny, 1, nz);     eG(21); //-2002-01-12
  for (i=0; i<=nx; i++)
    for (j=1; j<=ny; j++)
      for (k=1; k<=nz; k++) {
        pw = AryPtr(pa, i, j, k);  if (!pw)                      eX(22);
        pf = AryPtr(&dsc, i, j, k);  if (!pf)                    eX(23);
        *pf = pw->vx;
      }
  sprintf(fn, "%s/%s%s", path, "vx", pn+1);
  pc = strrchr(fn, '.');
  if (pc)  *pc = 0;
  rc = DmnWrite(fn, usr.s, &dsc);  if (rc < 0)                   eX(24);
  AryFree(&dsc);
  //
  // vy
  //
  memset(&dsc, 0, sizeof(ARYDSC));
  DmnRplValues(&usr, "form", "vy%7.3f");
  DmnRplValues(&usr, "vldf", "\"Y\"");
  DmnRplValues(&usr, "artp", "\"W\"");
  AryCreate(&dsc, sizeof(float), 3, 1, nx, 0, ny, 1, nz);     eG(31); //-2002-01-12
  for (i=1; i<=nx; i++)
    for (j=0; j<=ny; j++)
      for (k=1; k<=nz; k++) {
        pw = AryPtr(pa, i, j, k);  if (!pw)                      eX(32);
        pf = AryPtr(&dsc, i, j, k);  if (!pf)                    eX(33);
        *pf = pw->vy;
      }
  sprintf(fn, "%s/%s%s", path, "vy", pn+1);
  pc = strrchr(fn, '.');
  if (pc)  *pc = 0;
  rc = DmnWrite(fn, usr.s, &dsc);  if (rc < 0)                  eX(34);
  AryFree(&dsc);
  //
  // vs
  //
  memset(&dsc, 0, sizeof(ARYDSC));
  DmnRplValues(&usr, "form", "vs%7.3f");
  DmnRplValues(&usr, "vldf", "\"S\"");
  DmnRplValues(&usr, "artp", "\"W\"");
  AryCreate(&dsc, sizeof(float), 3, 1, nx, 1, ny, 0, nz);     eG(41); //-2002-01-12
  for (i=1; i<=nx; i++)
    for (j=1; j<=ny; j++)
      for (k=0; k<=nz; k++) {
        pw = AryPtr(pa, i, j, k);  if (!pw)                      eX(42);
        pf = AryPtr(&dsc, i, j, k);  if (!pf)                    eX(43);
        *pf = pw->vz;
      }
  sprintf(fn, "%s/%s%s", path, "vs", pn+1);
  pc = strrchr(fn, '.');
  if (pc)  *pc = 0;
  rc = DmnWrite(fn, usr.s, &dsc);  if (rc < 0)                   eX(44);
  AryFree(&dsc);
  //
  TxtClr(&usr);
  return 0;
eX_10:
eX_1:  eX_2:  eX_3:
eX_11: eX_12: eX_13:
eX_21: eX_22: eX_23:
eX_31: eX_32: eX_33:
eX_41: eX_42: eX_43:
  eMSG(_internal_error_);
eX_4:
eX_14: eX_15: eX_16: eX_17:
eX_24:
eX_34:
eX_44:
  eMSG(_cant_write_file_$_, fn);
}

//=============================================================== PrepareBodies
static int PrepareBodies( void ) {
  dP(PrepareBodies);
  int i, j, k, rc, net;
  char s[1024], name[256];
  float *p;
  FILE *fp;
  rc = DefBodies(); if (rc < 0)                                      eX(11);
  if (Nb == 0)                                                       eX(12);
  AryCreate(&Bout, sizeof(ARYDSC), 1, 0, GrdPprm->numnet);           eG(13);
  // set body matrices and write out volout
  net = (GrdPprm->numnet > 0) ? 1 : 0;
  do {
    if (net)  GrdSetNet(net);                                        eG(15);
    GridLevel = GrdPprm->level;
    GridIndex = GrdPprm->index;
    if (GridLevel < HighestLevel-1) {
      net++;
      continue;
    }
    DefGrid(0);                                                      eG(16);
    rc = 0;
    // set body matrix
    if (Nb > 0)
      rc = DMKutil_B2BM((ARYDSC *)AryPtr(&Bout, net), Nx, Ny, Nz, Xmin,
        Ymin, Dd, (float *)GrdParr->start, Bin);
    else if (Nb < 0)
      rc = DMKutil_BM2BM((ARYDSC *)AryPtr(&Bout, net), Nx, Ny, Nz, Xmin,
        Ymin, Dd, (float *)GrdParr->start, Bin, BmInX0, BmInY0, BmInDd, BmInDz);
    if (rc < 0)                                                      eX(17);
    // write out volout
    if (LstMode)
      sprintf(name, "%s/%s%1d%1d.dmna", TalPath, "volout",
      GridLevel, GridIndex);
    else
      sprintf(name, "%s/%s0%1d.dmna", TalPath, "volout",
        (HighestLevel == 0) ? 0 : HighestLevel-GridLevel+1);
    fp = fopen(name, "wb"); if (!fp)                                 eX(18);
    fprintf(fp, "----  volout array\n");
    fprintf(fp, "xmin  %s\n", GenForm(Xmin, 6));                  //-2014-01-28
    fprintf(fp, "ymin  %s\n", GenForm(Ymin, 6));
    fprintf(fp, "delt  %s\n", GenForm(Dd,6 ));
    fprintf(fp, "refx  %d\n", GrdPprm->refx);                     //-2008-12-11
    fprintf(fp, "refy  %d\n", GrdPprm->refy);                     //-2008-12-11
    if (GrdPprm->refx > 0 && *GrdPprm->ggcs) {                    //-2008-12-11
      fprintf(fp, "ggcs  \"%s\"\n", GrdPprm->ggcs);               //-2008-12-11
    }
    strcpy(s, "hh  ");
    p = (float *)GrdParr->start;
    for (k=0; k<=Nz; k++) sprintf(s, "%s %1.1f", s, p[k]);
    fprintf(fp, "%s\n", s);
    fprintf(fp, "vldf  \"V\"\n");
    fprintf(fp, "dims  3\n");
    fprintf(fp, "size  4\n");
    fprintf(fp, "form  \"%s\"\n", "%2.0f");
    fprintf(fp, "mode  \"text\"\n");
    fprintf(fp, "lowb  1  1  1\n");
    fprintf(fp, "hghb  %d  %d  %d\n", Nx, Ny, Nz);
    fprintf(fp, "sequ  \"k+,j-,i+\"\n");
    fprintf(fp, "*\n");
    for (k=1; k<=Nz; k++) {
      for (j=Ny; j>=1; j--) {
        for (i=1; i<=Nx; i++)
          fprintf(fp, "%2d", *(int *)AryPtr((ARYDSC *)AryPtr(&Bout,net),i,j,k));
        fprintf(fp, "\n");
      }
      fprintf(fp, "\n");
    }
    fprintf(fp, "***\n");
    fclose(fp);
    // check for body-free boundary cells 2004-12-09
    if (GridLevel == HighestLevel) {
      for (i=1; i<=Nx; i++) {
        for (j=1; j<=Ny; j++) {
          if (j != 1 && j != Ny)
            if (i != 1 && i != Nx) continue;
          if (i != 1 && i != Nx)
            if (j != 1 && j != Ny) continue;
          for (k=1; k<=Nz; k++) {
            if (0 != *(int *)AryPtr((ARYDSC *)AryPtr(&Bout,net),i,j,k)) eX(20);
          }
        }
      }
    }
    net++;
  } while (net <= GrdPprm->numnet);
  return 0;
eX_11: eX_12: eX_13: eX_15: eX_16: eX_17: eX_18:
  eMSG(_error_bodies_);
eX_20:
  eMSG(_improper_grid_$$$$_, i, j, k, name);
}


/*=================================================================== ClcWnd
*/
static int ClcWnd(    /* Calculate windfields for all grids  */
void )
{
  dP(ClcWnd);
  int alldone, net, inside, option, id;
  char name[256], s[64];
  float hg, lg, uref, dref;
  ARYDSC *pa=NULL;
  static char *NoYes[3] = { "no", "yes", NULL };
  TXTSTR hdr = { NULL, 0 };   
  TXTSTR hhdr = { NULL, 0 };
  alldone = 0;
  option = 0;
  if (StdStatus & WND_SETUNDEF)  option = TMN_SETUNDEF;
  net = (GrdPprm->numnet > 0) ? 1 : 0;
  do {                                                          //-2005-03-23
    if (net)  GrdSetNet(net);                                        eG(21);
    GridLevel = GrdPprm->level;
    GridIndex = GrdPprm->index;
    if (net <= 1)  LowestLevel = GridLevel;
    if (GridLevel > HighestLevel) HighestLevel = GridLevel;
    net++;
  } while (net <= GrdPprm->numnet);
  if ((GrdPprm->flags & GRD_BODIES) && !Nb) {
    PrepareBodies();                                                 eG(20);
  }
  net = (GrdPprm->numnet > 0) ? 1 : 0;
  do {  /***************** for all grids ************************/
    if (alldone)  break;
    if (net)  GrdSetNet(net);                                        eG(21);
    GridLevel = GrdPprm->level;
    GridIndex = GrdPprm->index;
    if (net <= 1)  LowestLevel = GridLevel;
    inside = (GridLevel > LowestLevel);
    DefGrid(1);                                                      eG(22);
    Aver = 0;
    ClcCharHL(&CharHb, &hg, &lg);
    if (GridLevel <= LowestLevel) {   //-2004-08-23               //-2014-06-26
      CharHg = hg;
      CharLg = lg;
    }
    inside = (GridLevel > LowestLevel);
    vLOG(2)(_grid_$$_ground_$$$_,
      GridLevel, GridIndex, CharHg, CharLg, NoYes[SolidTop]);

    Iwin = IDENT(WNDarr, WND_BASIC, GridLevel, GridIndex);
    Iwnd = IDENT(WNDarr, WND_TERRAIN, GridLevel, GridIndex);
    pa = TmnAttach(Iwnd, &MetT1, &MetT2, 0, NULL);                   eG(70);
    if (pa) {
      vLOG(1)(_$_found_, NmsName(Iwnd));
      TmnDetach(Iwnd, NULL, NULL, 0, NULL);                          eG(80);
    }
    else {
      ReadInit(WND_TERRAIN);                                         eG(1);
      if (CharHg>0 || (WND_TERRAIN==LastStep)) {
        UvwInit(WND_TERRAIN);                                        eG(2);
        ClcMxIn(0);                                                  eG(3);
        SolveSystem(WND_TERRAIN);                                    eG(4);
        ClcUvw(WND_TERRAIN, 0);                                      eG(5);
        ClrArrays(WND_TERRAIN);                                      eG(6);
      }
      else {
        TmnRename(Iwin, Iwnd);                                       eG(31);
        Iwin = 0;
      }
    }
    if (WrtMask & (1<<WND_TERRAIN)) {
      TmnArchive(Iwnd, "", option);                                  eG(41);
      vLOG(1)(_$_written_, NmsName(Iwnd));
    }
    if (WND_BUILDING < LastStep) goto clear_step;
    Iwin = IDENT(WNDarr, WND_TERRAIN, GridLevel, GridIndex);
    Iwnd = IDENT(WNDarr, WND_BUILDING, GridLevel, GridIndex);
    pa = TmnAttach(Iwnd, &MetT1, &MetT2, 0, NULL);                   eG(71);
    if (pa) {
      vLOG(1)(_$_found_, NmsName(Iwnd));
      TmnDetach(Iwnd, NULL, NULL, 0, NULL);                          eG(81);
    }
    else {
      Aver = 1;
      UvwInit(WND_BUILDING);                                         eG(12);
      InsertLayer();                                                 eG(18);
      if (WrtMask & (1<<WND_SURFACE)) {
        id = IDENT(WNDarr, WND_SURFACE, GridLevel, GridIndex);
        strcpy(name, NmsName(id));
        TmnArchive(Iwnd, name, 0);                                   eG(42);
      }
      ClcMxIn(0);                                                    eG(13); //-2005-05-06
      SolveSystem(WND_BUILDING);                                     eG(14);
      ClcUvw(WND_BUILDING, 0);                                       eG(15);
      ClrArrays(WND_BUILDING);                                       eG(16);
    }
    if (WrtMask & (1<<WND_BUILDING)) {
      TmnArchive(Iwnd, "", option);                                  eG(43);
      vLOG(1)(_$_written_, NmsName(Iwnd));
    }
    if (WND_FINAL < LastStep) goto clear_step;
    Iwin = IDENT(WNDarr, WND_BUILDING, GridLevel, GridIndex);
    Iwnd = IDENT(WNDarr, WND_FINAL, GridLevel, GridIndex);
    pa = TmnAttach(Iwnd, &MetT1, &MetT2, 0, NULL);                   eG(72);
    if (pa) {
      vLOG(1)(_$_found_, NmsName(Iwnd));
      TmnDetach(Iwnd, NULL, NULL, 0, NULL);                          eG(82);
    }
    else {
      if (Nb && GridLevel == HighestLevel) {
        ClcDMK((ARYDSC *)AryPtr(&Bout, net), 1, inside);             eG(51);
        ClcUvw(WND_FINAL, 0);                                        eG(25);
      }
      else {
        if (Nb && BdsTrbExt && GridLevel == HighestLevel-1) {
          ClcDMK((ARYDSC *)AryPtr(&Bout, net), 0, inside);           eG(52);
        }
        TmnRename(Iwin, Iwnd);                                       eG(32);
        ClcUvw(WND_FINAL, 0);                                        eG(35);
      }
    }
clear_step:
    TmnDetach(Ivm, NULL, NULL, 0, NULL);                             eG(83);
    Pvm = NULL;
    TmnDetach(Izm, NULL, NULL, 0, NULL);                             eG(84);
    Pzm = NULL;
    TmnDetach(Izp, NULL, NULL, 0, NULL);                             eG(85);
    Pzp = NULL;
    TmnDelete(TmMax(), Isld, TMN_NOID);                              eG(86);
    Isld = 0;
    net++;
  } while (net <= GrdPprm->numnet);  /***** for all grids *********/
  if (LastStep>WND_FINAL)  return 1;

  ClcUref(&uref, &dref);                                             eG(91);
  alldone = 0;
  net = (GrdPprm->numnet > 0) ? 1 : 0;
  do {  /**************** for all grids *******************/
    if (alldone)  break;
    if (net)  GrdSetNet(net);                                        eG(23);
    GridLevel = GrdPprm->level;
    GridIndex = GrdPprm->index;
    TxtCpy(&hhdr, "\n");
    sprintf(s, "xmin  %s\n", TxtForm(GrdPprm->xmin, 6));
    TxtCat(&hhdr, s);
    sprintf(s, "ymin  %s\n", TxtForm(GrdPprm->ymin, 6));
    TxtCat(&hhdr, s);
    sprintf(s, "delta  %s\n", TxtForm(GrdPprm->dd, 6));
    TxtCat(&hhdr, s);    
    Iwnd = IDENT(WNDarr, WND_FINAL, GridLevel, GridIndex);
    Pwnd = TmnAttach(Iwnd, NULL, NULL, TMN_MODIFY, &hdr);
    if (!Pwnd)                                                       eX(203);
    sprintf(s, "%1.3f", uref);
    DmnRplValues(&hdr, "uref", s);                                   eG(204); //-2011-06-29
    sprintf(s, "%1.1f", dref);
    DmnRplValues(&hdr, "dref", s);                                   eG(205); //-2011-06-29
    //
    if (WrtMask & 0x10000) {
      writeDmna(Pwnd, hhdr.s, NmsName(Iwnd), "P");                   eG(64); //-2005-03-23
      if (!LstMode) {
        writeDmna(Pwnd, hdr.s, NmsName(Iwnd), "PXYS");               eG(61);
      }
      Itrb = IDENT(VARarr, WND_WAKE, GridLevel, GridIndex);
      Ptrb = TmnAttach(Itrb, NULL, NULL, TMN_MODIFY, &hhdr);         eG(77);
      if (Ptrb) {
        if (!LstMode) {
          writeDmna(Ptrb, hhdr.s, NmsName(Itrb), "PPPP");             eG(62);
        }
        TmnDetach(Itrb, NULL, NULL, TMN_MODIFY, &hhdr);              eG(87);
        if (WrtMask & (1<<WND_FINAL)) {
          strcpy(name, NmsName(Itrb));
          TmnArchive(Itrb, name, option);                            eG(46);
          vLOG(3)(_$_written_, name);
        }
        Ptrb = NULL;
      }
      Idff = IDENT(KFFarr, WND_WAKE, GridLevel, GridIndex);
      Pdff = TmnAttach(Idff, NULL, NULL, TMN_MODIFY, &hhdr);         eG(78);
      if (Pdff) {
        if (!LstMode) {
          writeDmna(Pdff, hhdr.s, NmsName(Itrb), "PP");               eG(63);
        }
        TmnDetach(Idff, NULL, NULL, TMN_MODIFY, &hhdr);              eG(88);
        if (WrtMask & (1<<WND_FINAL)) {
          strcpy(name, NmsName(Idff));
          TmnArchive(Idff, name, option);                            eG(47);
          vLOG(3)(_$_written_, name);
        }
        Pdff = NULL;
      }
    }
    if (WrtMask & (1<<WND_FINAL)) {
      TmnDetach(Iwnd, NULL, NULL, TMN_MODIFY, &hdr);                 eG(89);
      strcpy(name, NmsName(Iwnd));
      TmnArchive(Iwnd, name, option);                                eG(44);
      vLOG(3)(_$_written_, name);
    }
    else {
      TmnDetach(Iwnd, NULL, NULL, TMN_MODIFY, &hdr);                 eG(891);
    }
    Pwnd = NULL;
    Ptrb = NULL;
    TxtClr(&hdr);
    TxtClr(&hhdr);    
    Iwnd = IDENT(WNDarr, WND_BASIC, GridLevel, GridIndex);
    TmnDelete(TmMax(), Iwnd, 0);
    Iwnd = IDENT(WNDarr, WND_TERRAIN, GridLevel, GridIndex);
    TmnDelete(TmMax(), Iwnd, 0);
    Iwnd = IDENT(WNDarr, WND_BUILDING, GridLevel, GridIndex);
    TmnDelete(TmMax(), Iwnd, 0);
    net++;
  } while (net <= GrdPprm->numnet);
  return 1;
eX_1:
  eMSG(_cant_get_init_);
eX_21: eX_23:
  eMSG(_grid_undefined_);
eX_22:
  eMSG(_cant_define_grid_);
eX_2: eX_12:
  eMSG(_cant_initialize_);
eX_3: eX_13:
  eMSG(_cant_define_equations_);
eX_4: eX_14:
  eMSG(_cant_solve_);
eX_5: eX_15: eX_25: eX_35:
  eMSG(_error_calculation_);
eX_6: eX_16:
  eMSG(_error_clearing_);
eX_31: eX_32:
  eMSG(_cant_rename_);
eX_41: eX_42: eX_43: eX_44:
  eMSG(_cant_write_wind_$_, NmsName(Iwnd));
eX_46:
  eMSG(_cant_write_trb_$_, NmsName(Itrb));
eX_47:
  eMSG(_cant_write_dff_$_, NmsName(Idff));
eX_61:
  eMSG(_cant_write_dmna_$_, NmsName(Iwnd));
eX_62:
  eMSG(_cant_write_dmna_trb_$_, NmsName(Itrb));
eX_63:
  eMSG(_cant_write_dmna_dff_$_, NmsName(Idff));
eX_64:
  eMSG(_cant_write_zg_);
eX_18:
  eMSG(_cant_insert_);
eX_91:
  eMSG(_cant_set_uref_);
eX_70: eX_71: eX_72: eX_77: eX_78:
  eMSG(_cant_attach_);
eX_80: eX_81: eX_82: eX_83: eX_84: eX_85: eX_86: eX_87: eX_88: eX_89: eX_891:
  eMSG(_cant_detach_);
eX_51: eX_52:
  eMSG(_cant_calculate_dmk_);
eX_203: eX_204: eX_205:
  eMSG(_internal_error_);
eX_20:
  eMSG(_cant_prepare_bodies_);
}

//--------------------------------------------------------------------- Bds2Bds
static int DefBodies(void) {
  dP(DefBodies);
  int n, i, j, k, ii, n0, n1, nb, np, rc;
  int *ip;
  BDSBXREC *pb;
  BDSP0REC *p0;
  BDSP1REC *p1;
  BDSMXREC *pm;
  float *px, *py;
  if (Nb || !(GrdPprm->flags & GRD_BODIES))                             eX(1);
  Ibld = IDENT(SLDarr, 1, 0, 0);
  Pbld = TmnAttach(Ibld, NULL, NULL, 0, NULL);                          eG(21);
  if (!Pbld)                                                            eX(99);
  pb = (BDSBXREC*)AryPtr(Pbld, Pbld->bound[0].low);
  if (pb->btype == BDSMX) {
    pm = (BDSMXREC*)pb;
    AryCreate(&Bin, sizeof(int), 2, 1, pm->nx, 1, pm->ny);             eG(22);
    BmInX0 = pm->x0;
    BmInY0 = pm->y0;
    BmInDd = pm->dd;
    BmInDz = pm->dz;
    ip = &(pm->nzstart);
    for (i=0; i<pm->nx; i++) {
      for (j=0; j<pm->ny; j++) {
        k =  ip[i*pm->ny+j];
        *(int *)AryPtr(&Bin, i+1, j+1) = k;
        if (k) Nb--;
      }
    }
  }
  else {
    n0 = Pbld->bound[0].low;
    n1 = Pbld->bound[0].hgh;
    for (n=n0; n<=n1; n++) {
      pb = (BDSBXREC*)AryPtr(Pbld,n);
      if (pb->btype == BDSBOX || pb->btype == BDSPOLY0)
        Nb++;
    }
    AryCreate(&Bin, sizeof(BODY), 1, 0, Nb);                            eG(2);
    nb = 0;
    for (n=n0; n<=n1; n++) {
      pb = &(*(BDSBXREC*)AryPtr(Pbld,n));
      if (pb->btype == BDSBOX) {
        BodyRectangle((BODY *)AryPtr(&Bin, nb),
          pb->xb, pb->yb, pb->ab, pb->bb, 0., pb->cb, pb->wb);  //-2004-12-14
        nb++;
      }
      else if (pb->btype == BDSPOLY0) {
        p0 = (BDSP0REC*)pb;
        if (p0->btype != BDSPOLY0)                                      eX(11);
        np = p0->np;
        px = ALLOC((np)*sizeof(float)); if (!px)                        eX(3);
        py = ALLOC((np)*sizeof(float)); if (!py)                        eX(4);
        for (i=0; i<2; i++) {
          px[i] = p0->p[i].x;
          py[i] = p0->p[i].y;
        }
        p1 = (BDSP1REC*) p0;
        for (i=2; i<np; i++) {
          ii = (i+4)%6;
          if (!ii) {
            p1++;
            if (p1->btype != BDSPOLY1)                                  eX(12);
          }
          if (ii >= p1->np)                                             eX(13);
          px[i] = p1->p[ii].x;
          py[i] = p1->p[ii].y;
        }
        BodyPolygon((BODY *)AryPtr(&Bin, nb), 0, p0->cb);
        rc = Body_setPoints((BODY *)AryPtr(&Bin, nb), np, px, py);
        nb++;
        if (rc < 0)                                                    eX(14);
        FREE(px);
        FREE(py);
      }
      else if (pb->btype == BDSPOLY1)
        continue;
      else {
        eX(30);
      }
    }
  }

  if (Nb < 0)
    vLOG(3)(_body_matrix_$_, -Nb);
  else
    vLOG(3)(_$_bodies_, Nb);
  return 0;
eX_1: eX_2: eX_3: eX_4: eX_11: eX_12: eX_13: eX_21: eX_22: eX_30: eX_99:
  eMSG(_internal_error_);
eX_14:
  eMSG(_cant_set_points_$_, rc);
}



//---------------------------------------------------------------------- ClcDMK
static int ClcDMK(ARYDSC *pbm, int store_wind, int fix_bnd) { //-2004-07
  dP(ClcDMK);
  int rc;
  TXTSTR hdr = { NULL, 0 };
  if (Zd != 0 || Sd != 0)                                            eX(1);
  if (!pbm || !pbm->start)                                           eX(2);
  Idff = IDENT(KFFarr, WND_WAKE, GridLevel, GridIndex);
  Pdff = TmnCreate(Idff, sizeof(DFFREC), 3, 0, Nx, 0, Ny, 0, Nz);    eG(31);
  Itrb = IDENT(VARarr, WND_WAKE, GridLevel, GridIndex);
  Ptrb = TmnCreate(Itrb, sizeof(TRBREC), 3, 0, Nx, 0, Ny, 0, Nz);    eG(32);
  TmnClear(MetT1, Iwnd, Idff, Itrb, TMN_NOID);                       eG(33);
  if (store_wind) {
    Pwnd = TmnCreate(Iwnd, sizeof(WNDREC), 3, 0, Nx, 0, Ny, 0, Nz);  eG(34);
  }
  else
    Pwnd = NULL;
  Pwin = TmnAttach(Iwin, &MetT1, &MetT2, 0, NULL);                   eG(35);
  //
  // calculate DMK wind field
  //
  DMKpar.a1 = BdsDMKp[1];
  DMKpar.a2 = BdsDMKp[2];
  DMKpar.a3 = BdsDMKp[3];
  DMKpar.a4 = BdsDMKp[4];
  DMKpar.a5 = BdsDMKp[5];
  DMKpar.hs = BdsDMKp[6];
  DMKpar.da = BdsDMKp[7];
  DMKpar.fs = BdsDMKp[8];
  DMKpar.fk = BdsDMKp[9];
  DMKpar.x0 = Xmin;
  DMKpar.y0 = Ymin;
  DMKpar.dd = Dd;
  DMKpar.nx = Nx;
  DMKpar.ny = Ny;
  DMKpar.nz = Nz;
  DMKpar.hh = GrdParr->start;
  DMKpar.flags = 0;
  if (GrdPprm->flags & GRD_DMKHM) DMKpar.flags |= DMK_USEHM;    //-2005-02-15
  rc = DMK_calculateWField(&DMKpar, Pwin, pbm, Pwnd, Pdff, Ptrb, fix_bnd);
  if (rc)                                                           eX(3);
  // store
  TmnDetach(Iwin, NULL, NULL, 0, NULL);                             eG(23);
  if (store_wind) {
    WrtHeader(&hdr, "PXYS");                                        eG(24);
    TmnDetach(Iwnd, NULL, NULL, TMN_MODIFY, &hdr);                  eG(25);
  }
  WrtHeader(&hdr, "PP");
  TmnDetach(Idff, NULL, NULL, TMN_MODIFY, &hdr);                    eG(26);
  WrtHeader(&hdr, "PPPP");
  TmnDetach(Itrb, NULL, NULL, TMN_MODIFY, &hdr);                    eG(27);
  TxtClr(&hdr);
  Ptrb = NULL;
  Pdff = NULL;
  return 0;
eX_1:
  eMSG(_not_applicable_);
eX_2:
  eMSG(_undefined_bmatrix_);
eX_3:
  eMSG(_cant_calculate_dmk_);
eX_31: eX_32: eX_33: eX_34: eX_35:
  eMSG(_cant_attach_);
eX_23: eX_24: eX_25: eX_26: eX_27:
  eMSG(_cant_detach_);
}



/*=================================================================== WrtHeader
*/
#define CAT(a, b) TxtPrintf(&txt, a, b), TxtCat(phdr, txt.s);
static int WrtHeader(TXTSTR *phdr, char *valdef)
  {
  /* dP(WrtHeader); */
  char s[256], frm[256], arrtype;
  TXTSTR txt = { NULL, 0 };                                       //-2011-06-29
  int k;
  int type;
  if (!strcmp(valdef, "PXYS")) {
    type = 0;
    arrtype = 'W';
    strcpy(frm, "Z%6.1fVx%[2]7.2fVs%7.2f");
  }
  else if (!strcmp(valdef, "PXYZ")) {
    type = 1;
    arrtype = 'W';
    strcpy(frm, "Z%6.1fVx%[2]7.2fVz%7.2f");
  }
  else if (!strcmp(valdef, "a")) {
    type = 2;
    arrtype = 'Z';
    strcpy(frm, "i%[3]3ldLa%[15]3ldAa%[15]10.2e");
  }
  else if (!strcmp(valdef, "V")) {
    type = 3;
    arrtype = 'Z';
    strcpy(frm, "%10.2e");
  }
  else if (!strcmp(valdef, "PP")) {
    type = 4;
    arrtype = 'K';
    strcpy(frm, "Kh%6.1fKv%6.1f");
  }
  else if (!strcmp(valdef, "PPPP")) {
    type = 5;
    arrtype = 'V';
    strcpy(frm, "Su%[3]6.2fTh%6.2f");
  }
  vLOG(4)("WND:WrtHeader(...,%s)", valdef);
  TxtCpy(phdr, "\n");
  sprintf(s, "TALdia_%d.%d.%s", StdVersion, StdRelease, StdPatch);
  CAT("prgm  \"%s\"\n", s);
  CAT("artp  \"%c\"\n", arrtype);
  CAT("vldf  \"%s\"\n", valdef);
  CAT("avmean  %3.3f\n", AvMean);
  CAT("form  \"%s\"\n", frm);
  CAT("xmin  %s\n", TxtForm(Xmin, 6));
  CAT("ymin  %s\n", TxtForm(Ymin, 6));
  CAT("delta %s\n", TxtForm(Dd, 6));
  CAT("zscl  %s\n", TxtForm(Zd, 6));
  CAT("sscl  %s\n", TxtForm(Sd, 6));
  CAT("refx  %d\n", GrdPprm->refx);
  CAT("refy  %d\n", GrdPprm->refx);
  if (GrdPprm->refx > 0 && *GrdPprm->ggcs)                        //-2008-12-11
    CAT("ggcs  \"%s\"\n", GrdPprm->ggcs);
  TxtCat(phdr, "sk   ");
  for (k=0; k<=Nz; k++)
    CAT(" %s", TxtForm(iSK(k),6));
  TxtCat(phdr, "\n");
  TxtClr(&txt);
  return 0;
}
#undef CAT

//============================================================= TwnWriteDef
//
int TwnWriteDef( void ) {
  dP(TwnWriteDef);
  FILE *f;
  char fn[256];
  int n, mode;
  sprintf(fn, "%s/grid.def", StdPath);
  f = fopen(fn, "w");   if (!f)                                    eX(1);  //-2014-01-21
  TdfWriteGrid(f);
  fclose(f);
  if (Nb) {
    sprintf(fn, "%s/bodies.def", StdPath);
    f = fopen(fn, "w");   if (!f)                                  eX(1);  //-2014-01-21
    TdfWriteBodies(f);
    fclose(f);
  }
  if (TalMode & TIP_SERIES) {
    // TatArgument(("-l2"); //-2004-08-13 not used
    TatArgument(StdPath);
    TatArgument("-wkr");
    sprintf(fn, "-B%s", WindLib);
    TatArgument(fn);
    mode = TIP_LIBRARY | TIP_COMPLEX;
    if (TalMode & TIP_BODIES) mode |= TIP_BODIES;
    if (TalMode & TIP_LIB2) mode |= TIP_LIB2;
    else if (TalMode & TIP_LIB36) mode |= TIP_LIB36;
    n = TdfWrite(StdPath, &TIPary, NULL, mode);  if (n < 0)         eX(2);
    NumLibMembers = n;
  }
  else if (TalMode & TIP_STAT) {
    TalAKS(StdPath);
    TalAKS((TalMode & TIP_BODIES) ? "-l36" : "-l2");
    TalAKS("-wkr");
    sprintf(fn, "-B%s", WindLib);
    TalAKS(fn);
    n = TasWriteWetlib(); if (n < 0)                               eX(3);
    NumLibMembers = n;
  }
  return 0;
eX_1: eX_2: eX_3:
  eMSG(_internal_error_);
}

//============================================================ TwnInterpolateVv
static float InterpolateVv( double *uu, double *zz, double ha, int nh ) {
  double d, d1, d2, h1, h2, h3, p1, p2, p3, u1, u2;
  int ka;
  if (nh < 1)  return 0;
  for (ka=1; ka<=nh; ka++)  if (zz[ka]-zz[0] >= ha)  break;
  if (ka > nh)  return 0;
  p2 = uu[ka];
  d2 = zz[ka] - zz[ka-1];
  h2 = 0.5*(zz[ka-1]+zz[ka]) - zz[0];
  d = (ha - zz[ka-1] + zz[0])/d2;
  if (ka == 1) {
    h1 = -h2;
    d1 = d2;
    p1 = -p2;
  }
  else {
    h1 = 0.5*(zz[ka-2]+zz[ka-1]) - zz[0];
    d1 = zz[ka-1] - zz[ka-2];
    p1 = uu[ka-1];
  }
  u1 = p1 + (p2-p1)*(ha-h1)/(h2-h1);
  if (ka == nh)  return u1;
  p3 = uu[ka+1];
  h3 = 0.5*(zz[ka]+zz[ka+1]) - zz[0];
  u2 = p2 + (p3-p2)*(ha-h2)/(h3-h2);
  return (1-d)*u1 + d*u2;
}

/*==================================================================== TwnInit
*/
long TwnInit(           /* initialize server    */
long flags,             /* action flags         */
char *istr )            /* server options       */
  {
  dP(TwnInit);
  long id, mask;
  char *jstr, *ps, t[400];
  int rc;
  if (StdStatus & STD_INIT)  return 0;
  if (istr) {
    jstr = istr;
    ps = strstr(istr, " -v");
    if (ps) sscanf(ps+3, "%d", &StdLogLevel);
    ps = strstr(istr, " -y");
    if (ps) sscanf(ps+3, "%d", &StdDspLevel);
    ps = strstr(istr, " -d");
    if (ps)  strcpy(DefName, ps+3);
    ps = strstr(istr, " -i");
    if (ps)  strcpy(InpName, ps+3);
    ps = strstr(istr, " -w");
    if (ps) {
      if (ps[3] == '0') StdStatus &= ~WND_WRTWND;
      else  StdStatus |= WND_WRTWND;
      }
    }
  else  jstr = "";
  vLOG(3)("WND_%d.%d.%s (%08lx,%s)", StdVersion, StdRelease, StdPatch, flags, jstr);
  StdStatus |= flags;
  if (!LstMode) {
    TwnWriteDef();                                                       eG(4);
    if (TI.gh[0]) {
      rc = TdfCopySurface(TalPath);                                      eG(5);
      if (rc <= 0) {
        vMsg(_no_surface_found_);
        return -1;
      }
    }
  }
  sprintf(t, " GRD -v%d -y%d -d%s", StdLogLevel, StdDspLevel, DefName);
  GrdInit(flags&STD_GLOBAL, t);                                         eG(12);
  sprintf(t, " BLM -v%d -y%d -d%s -i%s", StdLogLevel, StdDspLevel, DefName, InpName);
  BlmInit(flags&STD_GLOBAL, t);                                         eG(14);
  sprintf(t, " DMK -v%d -y%d", StdLogLevel, StdDspLevel);
  DMKinit(flags&STD_GLOBAL, t);                                         eG(15);
  DMKpar.rj = -1;
  mask = ~(NMS_LEVEL | NMS_GRIDN);
  id = IDENT(WNDarr, WND_FINAL, 0, 0);
  TmnCreator(id, mask, 0, istr, TwnServer, NULL);     eG(1);
  id = IDENT(WNDarr, WND_BUILDING, 0, 0);
  TmnCreator(id, mask, 0, istr, TwnServer, NULL);     eG(2);
  id = IDENT(WNDarr, WND_BASIC, 0, 0);
  TmnCreator(id, mask, 0, istr, MakeInit, NULL);      eG(3);
  StdStatus |= STD_INIT;
  return 0;
eX_1:  eX_2:  eX_3:  eX_4:  eX_5:
eX_12: eX_14: eX_15:
  eMSG(_not_initialized_);
  }

/*=================================================================== TwnServer
*/
long TwnServer(
char *ss )
  {
  dP(TwnServer);
  int id, option, mask;
  int rc;
  //
  if (!strncmp(ss, "-h", 2)) {                                    //-2008-08-15
    sprintf(WndVersion, "%d.%d.%s", StdVersion, StdRelease, StdPatch);
#ifdef MAKE_LABEL
    strcat(WndVersion, "-" MAKE_LABEL);
#endif
    TwnHelp(ss+1);                                                //-2008-08-15
  }
  //
  strcpy(TI.cset, NlsGetCset());                                  //-2008-07-22
  //
  if (StdArg(ss))  return 0;
  if (*ss) {
    switch (ss[1]) {
      case 'A': TipMain(ss);
                break;
      case 'B': strcpy(WindLib, ss+2);
                break;
#ifdef MAIN
      case 'h': TwnHelp(ss+1);
                break;
      /*                                                         //-2014-02-18/uj unused, commented just in case
      case 'l': LstMode = 1;
                if (!*InpNewName) strcpy(InpName, "meteo.def");
                break;
      */
#endif
      case 'i': strcpy(InpName, ss+2);
                strcpy(InpNewName, ss+2);
                break;
      case 'o': strcat(OptString, ss+2);
                strcat(OptString, ";");                         //-2006-10-04
                if (!strncmp(ss+2, "SOR.", 4)) {                //-2006-10-04
                  SorOption(ss+6);
                  break;
                }
                if      (strstr(OptString, "-NODIV"))  ZeroDiv = -1;
                else if (strstr(OptString, "NODIV"))   ZeroDiv = 1;
                if      (strstr(OptString, "-USECHARLG"))  UseCharLg = 0;
                else if (strstr(OptString, "USECHARLG"))   UseCharLg = 1;
                if      (strstr(OptString, "GRIDONLY"))    GridOnly = 1; //-2005-08-14
                if      (strstr(OptString, "DEFSONLY"))    DefsOnly = 1; //-2006-03-24
                if      (strstr(OptString, "CLEAR"))       ClearLib = 1; //-2006-10-04
                break;
      case 's': StdStatus |= WND_SLIPFLOW;
                break;
      case 'u': StdStatus |= WND_SETUNDEF;
                break;
      case 't': WriteBinary = 0;
                break;
      case 'w': WrtMask = 0x30000;
                sscanf(ss+2, "%x", &WrtMask);
                if (WrtMask & 0x01)  StdStatus |= WND_WRTWND;
                else  StdStatus &= ~WND_WRTWND;
                break;
      default:  ;
      }
    return 0;
    }
  if (TalMode & TIP_SORRELAX) {                                 //-2006-02-06
    vLOG(3)(_applying_sorrelax_);
    DMKfdivmax = 10;
    DMKndivmax = 30;
  }
  vLOG(3)(_calculating_wind_);
  if (!GrdPprm)                                                         eX(10);
  if (StdStatus & STD_TIME)  MetT1 = StdTime;
  else  MetT1 = TmMin();
  MetT2 = MetT1;
  Ibp= IDENT(BLMpar, 0, 0, 0);
  TmnAttach(Ibp, &MetT1, &MetT2, 0, NULL);                              eG(13);
  TmnDetach(Ibp, NULL, NULL, 0, NULL);                                  eG(14);
  if (MetT1 == MetT2)  return 0;
  NmsSeq("index", BlmPprm->Wind);
  LastStep = WND_FINAL;
  mask = ~(NMS_LEVEL | NMS_GRIDN);
  id = IDENT(WNDarr, WND_FINAL, 0, 0);
  TmnCreator(id, mask, 0, NULL, NULL, NULL);                            eG(60);
  id = IDENT(WNDarr, WND_BUILDING, 0, 0);
  TmnCreator(id, mask, 0, NULL, NULL, NULL);                            eG(61);
  Ibt = IDENT(BLMtab, 0, 0, 0);
  Iba = IDENT(BLMarr, 0, 0, 0);
  Pba = TmnAttach(Iba, &MetT1, &MetT2, 0, NULL);  if (!Pba)             eX(15);
  Ha  = BlmPprm->AnmHeight;
  Ua  = BlmPprm->WndSpeed;
  Da  = BlmPprm->WndDir;
  Xa  = BlmPprm->AnmXpos;
  Ya  = BlmPprm->AnmYpos;
  Za  = BlmPprm->AnmZpos;
  Uref = 0;
  Dref = 0;
  option = 0;
  if (StdStatus & WND_SETUNDEF)  option = TMN_SETUNDEF;
  rc = ClcWnd();                                                        eG(65);
// clean_up:

  TmnDetach(Iba, NULL, NULL, 0, NULL);                                  eG(64);
  Pba = NULL;
  mask = ~(NMS_LEVEL | NMS_GRIDN);
  id = IDENT(WNDarr, WND_FINAL, 0, 0);
  TmnCreator(id, mask, 0, NULL, TwnServer, NULL);                       eG(62);
  id = IDENT(WNDarr, WND_BUILDING, 0, 0);
  TmnCreator(id, mask, 0, NULL, TwnServer, NULL);                       eG(63);
  return 1;
eX_10: eX_13: eX_14: eX_15:
eX_60: eX_61: eX_62: eX_63: eX_64: eX_65:
  eMSG(_internal_error_);
}


#ifdef MAIN  /*#############################################################*/

//==================================================================== TwnHelp
//
static void TwnHelp( char *s )                                    //-2008-08-15
  {
  char lan[256];
  sprintf(lan, "%s [%s]", IBJstamp(NULL, NULL), NlsLanguage);     //-2008-08-27
  vMsg(_help_1_$$$_, "TALDIA", WndVersion, lan);                  //-2008-10-01
  vMsg(_help_2_);                                                 //-2008-10-01
  vMsg(_help_3_);                                                 //...
  vMsg(_help_4_);
  vMsg(_help_5_);
  if (LstMode) {
    vMsg(_help_6_$_, WindLib);
    vMsg(_help_7_$_, InpName);
    vMsg(_help_8_);
  }
  if (s[1] == 'l') {                                              //-2008-08-15
    vMsg("NLS:%s\n", CfgString());
  }
  StdStatus = 0;
  exit(0);
  }

static void MyExit( void ) {
  if (MsgCode < 0) {
    vMsg(_$_exit_error_, StdMyProg);
    TmnList(MsgFile);
  }
  else if (MsgCode == 99) {
    vMsg(_$_exit_input_, StdMyProg);
  }
  else vMsg(_$_exit_, StdMyProg);
  TmnFinish();
  if (MsgFile) {
    fprintf(MsgFile, "\n\n\n");
    fclose(MsgFile);
  }
  MsgFile = NULL;
  MsgSetQuiet(1);
}

static void MyAbort( int sig_number )
  {
  FILE *prn;
  char *pc;
  static int interrupted=0;
  if (interrupted) exit(0);                             //-2002-09-21
  MsgSetQuiet(0);
  switch (sig_number) {
    case SIGINT:  pc = _interrupted_;  interrupted = 1;  break;
    case SIGSEGV: pc = _segmentation_violation_;  break;
    case SIGTERM: pc = _termination_request_;  break;
    case SIGABRT: pc = _abnormal_termination_;  break;
    default:      pc = _unknown_signal_;
  }
  vMsg("@%s (%s)", _wnd_aborted_, pc);
  if (sig_number != SIGINT) {
    prn = (MsgFile) ? MsgFile : stdout;
    fflush(prn);
    fprintf(prn, _abort_signal_, sig_number);
  }
  if (MsgFile) {
    fprintf(MsgFile, "\n\n");
    fclose(MsgFile);
  }
  MsgFile = NULL;
  MsgSetQuiet(1);
  if (!LstMode) {
    TutDirClear(StdPath);                                   //-2002-09-21
    TutDirRemove(StdPath);
  }
  exit(sig_number);                                               //-2008-08-08
  }

/*==================================================================== TwnMain
*/
static long TwnMain( void )
  {
  dP(TwnMain);
  long igp, rc, tflags, ivar, ikff, t1, t2;
  char istr[80], t[80], t1s[32], t2s[32];
  int tset, mask, n=0, k;
  char path[256], *pc, **ll;
  int iLibMember = 0;
  ARYDSC *pa;
  GRDPARM *pgp;
  //
  CfgInit(0);
  sprintf(WndVersion, "%d.%d.%s", StdVersion, StdRelease, StdPatch);
#ifdef MAKE_LABEL
  strcat(WndVersion, "-" MAKE_LABEL);
#endif
  if ((StdStatus & STD_ANYARG) == 0) {
    MsgSetVerbose(1);
    MsgSetQuiet(0);
    MsgDspLevel = 0;
    TwnHelp("h");
  }
  MsgSetVerbose(1);
  MsgSetQuiet(0);
  MsgBreak = '\'';
  StdArgDisplay = 9;
  atexit(MyExit);
  signal(SIGSEGV, MyAbort);
  signal(SIGINT,  MyAbort);
  signal(SIGTERM, MyAbort);
  signal(SIGABRT, MyAbort);
  tflags = StdCheck & 0x07;
  if (!WrtMask)  TwnServer("-w");
  vMsg("");
  vMsg(_$$_calculation_libs_, StdMyProg, WndVersion);
  if (!MsgFile) {
    vMsg(_cant_work_$_, StdPath);
    MsgCode = 99;
    exit(1);
  }
  if (strlen(StdPath) == 0)
    TwnHelp("h");                                                 //-2008-08-15
  fprintf(MsgFile, _created_$_,  IBJstamp(NULL, NULL));                                        //-2006-08-24
  pc = getenv("HOSTNAME");
  if (!pc)  pc = getenv("COMPUTERNAME");
  if (pc)  fprintf(MsgFile, _host_is_$_, pc);
  fflush(MsgFile);
  //
  MsgCheckPath(StdPath);
  strcpy(TalPath, StdPath);
  if (!LstMode) {
    strcat(StdPath, "/work");
    if (!TutDirExists(TalPath))                                        eX(31);
    sprintf(path, "%s/%s", TalPath, "austal2000.txt");
    if (!TutFileExists(path))                                          eX(32);
    strcpy(path, StdPath);
    if (0 > TutDirMake(path))                                          eX(33);
    TutDirClear(path);
    sprintf(path, "%s/%s", TalPath, "lib");                       //-2004-08
    if (ClearLib) {                                               //-2006-10-04
      TutDirClear(path);
    }
    else {
      if (0 > TutDirClearOnDemand(path))                               eX(35);
    }
    TipMain(TalPath);
    sprintf(t, "-M%02x", TIP_TALDIA);
    TipMain(t);                                                   //-2014-06-26
    sprintf(path, "-H%s", StdMyName);
    pc = strrchr(path, '/');
    if (pc)  *pc = 0;
    TipMain(path);
    TipMain("-l-1");    // don't worry about library
    n = TipMain(NULL);  // read input for austal2000
    Gx = TI.gx;  // GK reference point x
    Gy = TI.gy;  // GK reference point y
    Nb = TI.nb;  // Nb != 0: buildings are defined
  }
  else {
    // write out dmna files
    WrtMask |= 0x00001;
    // remove existing grd****.dmna and sld****.dmna
    ll = _TutGetFileList(TalPath);
    for (k=0; (ll[k]); k++) {
      if ((strlen(ll[k]) == 12) && !strncmp(ll[k]+7, ".dmna", 5) &&
          (!strncmp(ll[k], "grd", 3) || !strncmp(ll[k], "sld", 3))) {
        TutMakeName(path, TalPath, ll[k]);
        rc = remove(path); if (rc)                                eX(21);
      }
    }
  }
  if (n) {
    if (n < 0)  MsgCode = -1;
    else if (n == 2) {
      vMsg(_library_complex_);
    }
    else if (n == 1)                        eX(20);
    exit(1);
  }
  //
  SetDataPath(StdPath);
  //
  TmnInit(StdPath, NmsName, NULL, tflags, MsgFile);                     eG(1);
  sprintf(istr, " WND -v%d -y%d -i%s", StdLogLevel, StdDspLevel, InpName);
  rc = TwnInit(StdStatus&STD_GLOBAL, istr);
  if (DefsOnly)  return 0;                                        //-2006-03-24
  if (!LstMode && (NumLibMembers<=0 || rc<0)) {
    vMsg(_cant_create_library_);
    MsgCode = -1;
    exit(2);
  }
  tset = ((StdStatus & STD_TIME) != 0);
  sprintf(t, "-w%04x", WrtMask);
  GrdServer(t);
  BdsServer(t);
  Nb = 0;
  igp = IDENT(GRDpar, 0, 0, 0);
  pa =  TmnAttach(igp, NULL, NULL, 0, NULL);  if (!pa)          eX(10);
  pgp = (GRDPARM*) pa->start;
  TmnDetach(igp, NULL, NULL, 0, NULL);                          eG(11);
  StdIdent = IDENT(WNDarr, WND_FINAL, 0, 0) | (StdIdent & 0xff);
  mask = ~(NMS_GROUP | NMS_LEVEL | NMS_GRIDN);
  StdStatus |= TMN_SETUNDEF;
  while (!GridOnly) {  //*********** time series ************** //-2005-08-14
    if (MsgQuiet <= 0) {
      if (!LstMode) {
        fprintf(stdout, _progress_$_, iLibMember*100.0/NumLibMembers);
        fflush(stdout);
      }
      else {
        if (StdStatus & WND_SETUNDEF) { t1 = TmMin();  t2 = TmMax(); }
        else { t1 = MetT1;  t2 = MetT2; }
        strcpy(t1s, TmString(&t1));
        strcpy(t2s, TmString(&t2));
        fprintf(stdout, "     [%s,%s]             \r", t1s, t2s );
        fflush(stdout);
      }
    }
    TmnClearAll(MetT1, mask, StdIdent, TMN_NOID);               eG(17);
    rc = TwnServer("");                                         eG(14);
    if (!rc)  break;
    ivar = IDENT(VARarr, 0, 0, 0);
    ikff = IDENT(KFFarr, 0, 0, 0);
    TmnClearAll(TmMax(), mask, StdIdent, ivar, ikff, TMN_NOID); eG(16);
    if ((tset) || (MetT2 == TmMax()))  break;
    MetT1 = MetT2;
    StdStatus |= STD_TIME;
    StdTime = MetT1;
    iLibMember++;
    if (LstMode)
      fprintf(stdout, "     [%s,%s]             \n", t1s, t2s );
  }
  if (!LstMode) {
    if (GridOnly) {                                             //-2005-08-14
      int net;
      char *hhdr;
      net = (GrdPprm->numnet > 0) ? 1 : 0;
      do {  /***************** for all grids ************************/
        hhdr = ALLOC(GENHDRLEN);  if (!hhdr)                      eX(80);
        if (net)  GrdSetNet(net);                                 eG(81);
        GridLevel = GrdPprm->level;
        GridIndex = GrdPprm->index;
        Izp  = IDENT(GRDarr, GRD_IZP,  GridLevel, GridIndex);
        Pzp  = TmnAttach(Izp, NULL, NULL, 0, NULL);               eG(82);
        Iwnd = IDENT(WNDarr, WND_FINAL, GridLevel, GridIndex);
        writeDmna(Pzp, hhdr, NmsName(Iwnd), "P");                 eG(83);
        TmnDetach(Izp, NULL, NULL, 0, NULL);                      eG(85);
        Pzp = NULL;
        FREE(hhdr);
        net++;
      } while (net <= GrdPprm->numnet);
      vMsg(_grid_file_created_);
    }
    else
      vMsg(_lib_$_created_, NumLibMembers);
    if (MaxDivWind >= 0)
      vMsg(_max_div_$$_, MaxDiv, MaxDivWind);
    if (StdCheck & 0x20)  TmnList(MsgFile);
    if ((WrtMask&0x0fff) == 0) {    //-2001-09-27
      TutDirClear(StdPath);
      TutDirRemove(StdPath);
    }
  }
  return 0;
eX_1: eX_10: eX_11: eX_14: eX_16: eX_17: eX_20: eX_21:
  vMsg(_internal_error_);
  exit(-MsgCode);
eX_31:
  vMsg(_no_work_$_, TalPath);
  exit(31);
eX_32:
  vMsg(_no_input_$_, TalPath);
  exit(32);
eX_33:
  vMsg(_cant_make_$_, path);
  exit(33);
eX_35:
  vMsg(_$_not_deleted_, path);
  exit(35);
eX_80: eX_81: eX_82: eX_83: eX_85:
  vMsg(_internal_error_);
  exit(80);
  }
#endif  /*###################################################################*/

//=============================================================================
//
// history:
//
// 2002-06-21 lj 0.13.0 final test version
// 2002-09-21 lj 0.13.4 working directory cleared if aborted
// 2002-09-24 lj 1.0.0  formatted wind fields, final release candidate
// 2003-02-24 lj 1.1.3  "h" used for initializing windfield
// 2003-03-11 lj 1.1.5  improved rounding for index calculation
// 2003-10-12 lj 1.1.12 gx and gy written to header
// 2004-08-23 uj 1.1.13 use CharHg and CharLg from largest grid
// 2004-10-19 uj 2.0.1  MakeInit: zg corrected
// 2004-10    uj 2.1.0  Buildings (integration of DMK)
// 2004-10-25 lj 2.1.4  INP ZET UTI guard cells for grids
//                      INP value of z0 from CORINE always rounded
//                      BLM INP DEF optional factors Ftv  extended to TALdia
// 2004-11-12 uj 2.1.5  INP additional check in getNesting()
//                      DMKutil Body_isInside corrected
//                      TwnWriteDef: NumLibMembers for AKS corrected
// 2004-11-26 lj 2.1.7  string length for names = 256
// 2004-11-30 uj 2.1.8  INP getNesting() check
//                      VSP check for VDISP abort
// 2004-12-06 uj 2.1.9  DMK uz=0, check for weight >0.02 in setRField()
// 2004-12-09 uj 2.1.10 WND PrepareBodies: check for body-free boundary cells
// 2004-12-14 uj 2.1.11 WND DMKutil SOR DMK use TalMath.h for PI, RADIAN
//                      WND DMKutil INP DEF wb, da in degree
//                      DMKutil include body border
// 2005-01-04 uj        DMKutil B2BM, BM2BM: loops reordered, speed improvements
// 2005-01-11 uj        DMK getVisibility/setVisible: speed improvement
//                      DMK SOR release memory
// 2005-02-11 uj 2.1.13 WND binary output by default (option -t: text output)
// 2005-02-15 uj 2.1.14 WND DMK GRD DEF use zm instead of hm for add. diffusion
// 2005-03-10 uj 2.1.15 SOR control flush, WND log creation date & computer
// 2005-03-17 uj 2.2.0  version number upgrade
// 2005-03-23 uj        set of Lowest/HighestLevel merged, dmkp output reduced
// 2005-05-06 uj 2.2.1  ClcWnd: UvwInit mask corrected (no consequences)
// 2005-08-06 uj 2.2.2  WriteDmna: write parameter axes to file header
// 2005-08-08 uj 2.2.3  DMN reading of parameter buff corrected
// 2005-08-17 uj 2.2.4  DMK avoid rounding problem for non-int grid coordinates
//                      option GRIDONLY
// 2006-02-06 uj 2.2.7  INP nonstandard option SORRELAX
//                      DEF local TmString, TimeStr
//                      BLM error if ua=0
// 2006-02-13 uj 2.2.8  DEF AKS BLM nonstandard option SPREAD
// 2006-02-15 lj 2.2.9  MSG UTL TMN new allocation check
// 2006-03-24 lj 2.2.11 Option DEFSONLY
// 2006-05-30 uj 2.2.12 TalAdj: catch unexpected error
// 2006-08-24 lj 2.2.13 modified IBJstamp()
// 2006-08-25 lj 2.2.14 BDS workaround for compiler error (SSE2 in I9.1)
// 2006-10-18 lj 2.3.0  externalization of strings
// 2007-02-03 uj 2.3.1  path setting corrected
// 2007-03-07 uj 2.3.2  write out maximum divergence at the end
// 2008-01-25 uj 2.3.7  INP DEF allow absolute path for rb
// 2008-07-22 lj 2.4.2  WND native language support
// 2008-08-12 lj 2.4.3  Display of version number
// 2008-08-15 lj        CFG NLS-info in help()
// 2008-08-27 lj        NLS display selected language
// 2008-10-01 lj        Setting of MsgTranslate removed (-> IbjStdReadNls())
// 2008-10-20 lj 2.4.4  Version update
// 2008-12-04 lj 2.4.5  Version update
// 2008-12-11 lj        Header parameter modified
// 2008-12-19 lj 2.4.6  Version update
// 2008-12-29 lj 2.4.7  Version update
// 2011-04-06 uj 2.4.8  PRF check in case of 1 base field
// 2011-06-29 uj 2.5.0  version number upgrade
// 2011-11-21 lj 2.6.0  minor adjustments
// 2012-04-06 uj 2.6.4  interpolation corrected for z<0
// 2014-06-26 uj 2.6.5  INP accept "ri" if called by TALdia
//                      WND global setting of hc/lc corrected, eX/eG adjusted
//=============================================================================

