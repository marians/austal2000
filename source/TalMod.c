//=================================================================== TalMod.c
//
// Handling of model fields
// ========================
//
// Copyright (C) Umweltbundesamt, Dessau-Ro�lau, Germany, 2002-2005
// Copyright (C) Janicke Consulting, 88662 �berlingen, Germany, 2002-2011
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
// last change: 2011-11-21 lj
//
//==========================================================================

#include <math.h>

#define  STDMYMAIN  ModMain
#include "IBJmsg.h"
#include "IBJary.h"
#include "IBJtxt.h"
#include "IBJdmn.h"
#include "IBJstd.h"
static char *eMODn = "TalMod";

/*=========================================================================*/

STDPGM(tstmod, ModServer, 2, 6, 0);

/*=========================================================================*/

#include "genutl.h"
#include "genio.h"

#include "TalTmn.h"
#include "TalNms.h"
#include "TalGrd.h"
#include "TalPrf.h"
#include "TalPrm.h"
#include "TalMat.h"
#include "TalMod.h"
#include "TalMod.nls"

#define  MOD_DEFTLU  60
#define  MOD_DEFTLV  60
#define  MOD_DEFTLW  60
#define  MOD_DEFTLM 120

#define  MOD_WRITE  0x0001

typedef struct {
   float dz;
   float vx, vy, vz;
   } VRFREC;

/*========================== internal functions ===========================*/

static long InterArak(  /* Interpolate velocities on Arakawa-C-net      */
  ARYDSC *pmod,         /* array with velocities like PRF or MOD        */
  int i, int j, int k,  /* index values                                 */
  float *pvx,           /* interpolated velocities                      */
  float *pvy,
  float *pvz)
  ;
static long Clc1dInt(   /* calculate 1d interpolated velocities */
  void )
  ;
static long Clc3dInt(   /* calculate 3d interpolated velocities */
  void )
  ;
static long Clc1dVrf(   /* calculate 1d reference velocities */
  void )
  ;
static long Clc3dVrf(   /* calculate 3d reference velocities */
  void )
  ;
static long ClcTau(     /* calculate time step                  */
  void *pp,             /* pointer to profile values            */
  void *pm,             /* pointer to model field values        */
  VRFREC *pv )          /* pointer to reference velocities      */
  ;
static long ClcPsi(     /* Calculate Psi, Sigma, Lambda         */
  void *pp,             /* pointer to profile values            */
  void *pm,             /* pointer to model field values        */
  MAT *psig,            /* pointer to sigma                     */
  VRFREC *pr )          /* pointer to reference velocity        */
  ;
static long Clc1dDiv(   /* drift velocity for 1-dimensional profile */
  void )
  ;
static long Clc3dDiv(   /* drift velocity for 3-dimensional profile */
  void )
  ;
static long Clc3dMod(   /* Calculate model fields for 3d boundary layer */
  void )
  ;
static long Clc1dMod(   /* Calculate model fields for 1d boundary layer */
  void )
  ;
static char *ModHeader( /* generate header vor VRFarr, MODarr   */
  long id,              /* tab identification                   */
  long *pt1,            /* start of validity time               */
  long *pt2 )           /* end of validity time                 */
  ;
/*==========================================================================*/

static ARYDSC *Pmod, *Pprf, *Psld, *Pvrf, *Psig;
static GRDPARM *Pgp;
static PRMINF *Ppi;

static long MetT1, MetT2;
static int Nx, Ny, Nz, GridLevel, GridIndex;
static int PrfRecLen, ModRecLen, DefMode, PrfMode;
static char AltName[120] = "param.def";
static float Dd, *Sk;
static float PrfNi, PrfKl, PrfLm, PrfHm, PrfAv, PrfUs, PrfZs, PrfSg, PrfZ0;
static int PrfDm;

static int CheckOut; /*!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!*/

static void PrnMat( char *name, MAT *pm )
  {
  fprintf(MsgFile, "%s:\n", name);
  fprintf(MsgFile, "%11.3e%11.3e%11.3e\n", pm->xx, pm->xy, pm->xz);
  fprintf(MsgFile, "%11.3e%11.3e%11.3e\n", pm->yx, pm->yy, pm->yz);
  fprintf(MsgFile, "%11.3e%11.3e%11.3e\n", pm->zx, pm->zy, pm->zz);
  }

  /*=============================================================== InterArak
  */
static long InterArak(  /* Interpolate velocities on Arakawa-C-net      */
  ARYDSC *pmod,         /* array with velocities like PRF or MOD        */
  int i, int j, int k,  /* index values                                 */
  float *pvx,           /* interpolated velocities                      */
  float *pvy,
  float *pvz)
  {
  dP(InterArak);
  MODREC *pm;
  float dd[2];
  float vx, vy, vz;
  int n, ii, i1, i2, jj, j1, j2, kk, k1, k2;
  ii = i;
  jj = j;
  if (k==Nz || k==0) {
    dd[0] = 1.0;
    dd[1] = 0;
    }
  else {
    kk = k+1;
    pm = AryPtr(pmod, ii, jj, kk);  if (!pm)       eX(1);
    dd[1] = pm->za;
    kk = k;
    pm = AryPtr(pmod, ii, jj, kk);  if (!pm)       eX(2);
    dd[1] -= pm->za;
    dd[0] = pm->za;
    kk = k-1;
    pm = AryPtr(pmod, ii, jj, kk);  if (!pm)       eX(3);
    dd[0] -= pm->za;
    }
  ii = i;
  j1 = (j > 0)  ? j : j+1;
  j2 = (j < Ny) ? j+1 : j;
  k1 = (k > 0)  ? k : k+1;
  k2 = (k < Nz) ? k+1 : k;
  vx = 0;
  n = 0;
  for (jj=j1; jj<=j2; jj++) {
    for (kk=k1; kk<=k2; kk++) {
      pm = AryPtr(pmod, ii, jj, kk);  if (!pm)     eX(4);
      vx += dd[kk-k1]*pm->vx;
      }
    n++;
    }
  vx /= n*(dd[0] + dd[1]);
  i1 = (i > 0)  ? i : i+1;
  i2 = (i < Nx) ? i+1 : i;
  jj = j;
  k1 = (k > 0)  ? k : k+1;
  k2 = (k < Nz) ? k+1 : k;
  vy = 0;
  n = 0;
  for (ii=i1; ii<=i2; ii++) {
    for (kk=k1; kk<=k2; kk++) {
      pm = AryPtr(pmod, ii, jj, kk);  if (!pm)     eX(5);
      vy += dd[kk-k1]*pm->vy;
      }
    n++;
    }
  vy /= n*(dd[0] + dd[1]);
  i1 = (i > 0)  ? i : i+1;
  i2 = (i < Nx) ? i+1 : i;
  j1 = (j > 0)  ? j : j+1;
  j2 = (j < Ny) ? j+1 : j;
  kk = k;
  vz = 0;
  n = 0;
  for (ii=i1; ii<=i2; ii++)
    for (jj=j1; jj<=j2; jj++) {
      pm = AryPtr(pmod, ii, jj, kk);  if (!pm)     eX(6);
      vz += pm->vz;
      n++;
      }
  vz /= n;
  *pvx = vx;
  *pvy = vy;
  *pvz = vz;
  return 0;
eX_1: eX_2: eX_3: eX_4: eX_5: eX_6:
  eMSG(_index_error_$$$_, ii, jj, kk);
  }

  /*================================================================= Clc1dInt
  */
static long Clc1dInt(   /* calculate 1d interpolated velocities */
  void )
  {
  dP(Clc1dInt);
  int k;
  MD2REC *pm;
  VRFREC *pv;
  vLOG(6)("MOD:Clc1dInt() ...");
  for (k=0; k<=Nz; k++) {
    pm = (MD2REC*) AryPtr(Pmod, k);  if (!pm)              eX(1);
    pv = (VRFREC*) AryPtr(Pvrf, k);  if (!pv)              eX(2);
    pv->vx = pm->vx;
    pv->vy = pm->vy;
    pv->vz = pm->vz;
    pv->dz = pm->za;
    }
  vLOG(6)("MOD:Clc1dInt returning");
  return 0;
eX_1: eX_2:
  eMSG(_index_error_$_, k);
  }

  /*================================================================= Clc3dInt
  */
static long Clc3dInt(   /* calculate 3d interpolated velocities */
  void )
  {
  dP(Clc3dInt);
  int i, j, k;
  MD2REC *pm;
  VRFREC *pv;
  for (i=0; i<=Nx; i++)
    for (j=0; j<=Ny; j++)
      for (k=0; k<=Nz; k++) {
        pm = (MD2REC*) AryPtr(Pmod, i, j, k);  if (!pm)                    eX(1);
        pv = (VRFREC*) AryPtr(Pvrf, i, j, k);  if (!pv)                    eX(2);
        pv->dz = pm->za;
        if (DefMode & GRD_3LIN) {
          pv->vx = pm->vx;
          pv->vy = pm->vy;
          pv->vz = pm->vz;
          }
        else {
          InterArak(Pmod, i, j, k, &(pv->vx), &(pv->vy), &(pv->vz));    eG(3);
          }
        }
  return 0;
eX_1: eX_2:
  eMSG(_index_error_$$$_, i, j, k);
eX_3:
  eMSG(_cant_interpolate_velocities_);
  }

  /*================================================================= Clc1dVrf
  */
static long Clc1dVrf(   /* calculate 1d reference velocities */
  void )
  {
  dP(Clc1dVrf);
  int k, k1, k2, kk;
  PR2REC *pp2;
  MD2REC *pm2;
  VRFREC *pv;
  ARYDSC *pprf, *pmod, *pvrf;
  float ax, ay, vx, vy, z;
  vLOG(6)("MOD:Clc1dVrf() ...");
  pprf = Pprf;
  pmod = Pmod;
  pvrf = Pvrf;
  z = 0.0;
  for (k=0; k<=Nz; k++) {
    pp2 = (PR2REC*) AryPtr(pprf, k);  if (!pp2)            eX(2);
    pm2 = (MD2REC*) AryPtr(pmod, k);  if (!pm2)            eX(3);
    pv =  (VRFREC*) AryPtr(pvrf, k);  if (!pv)             eX(4);
    pm2->vx = pp2->vx;
    pm2->vy = pp2->vy;
    pm2->vz = pp2->vz;
    pm2->za = pp2->h;
    pm2->ths = pp2->ts;
    pv->dz = pm2->za - z;
    pv->vz = 0;
    z = pm2->za;
    }
  pm2 = (MD2REC*) AryPtr(pmod, 1);  if (!pm2)              eX(5);
  pv  = (VRFREC*) AryPtr(pvrf, 0);  if (!pv)               eX(6);
  pv->dz = pm2->za - pv->dz;
  for (k=0; k<=Nz; k++) {
    ax = 0;  vx = 0;
    ay = 0;  vy = 0;
    k1 = (k > 0)  ? k-1 : k;
    k2 = (k < Nz) ? k+1 : k;
    for (kk=k1; kk<=k2; kk++) {
      pm2 = (MD2REC*) AryPtr(pmod, kk);  if (!pm2)         eX(11);
      ax = ABS(pm2->vx);
      if (ax > vx)  vx = ax;
      ay = ABS(pm2->vy);
      if (ay > vy)  vy = ay;
      }
    pv = (VRFREC*) AryPtr(pvrf, k);  if (!pv)              eX(12);
    pv->vx = vx;
    pv->vy = vy;
    }
  vLOG(6)("MOD:Clc1dVrf returning");
  return 0;
eX_2: eX_3: eX_4: eX_12:
  eMSG(_index_error_$_, k);
eX_5: eX_6:
  eMSG(_internal_error_);
eX_11:
  eMSG(_index_error_$_, kk);
  }

  /*================================================================= Clc3dVrf
   * Die Geschwindigkeits-Komponenten, Za und Ths werden vom PRFarr zum
   * MODarr übertragen. Im VRFarr werden die Intervallhöhen und punktweise
   * definierte Geschwindigkeitsbeträge (Maximum über benachbarte Intervalle)
   * eingetragen. Diese Werte werden später zur Berechnung von Tau
   * benötigt.
   *------------------------------------------------------------------------*/
static long Clc3dVrf(   /* calculate 3d reference velocities */
  void )
  {
  dP(Clc3dVrf);
  int i, j, k;
  int i1, ii, i2, j1, jj, j2, k1, kk, k2;
  float ax, ay, az, vx, vy, vz, z;
  PR2REC *pp2;
  MD2REC *pm2;
  ARYDSC *pprf, *pmod, *pvrf;
  VRFREC *pv;
  pprf = Pprf;
  pmod = Pmod;
  pvrf = Pvrf;
  z = 0.0;
  for (i=0; i<=Nx; i++)                /* Werte umspeichern */
    for (j=0; j<=Ny; j++) {
      z = 0.0;
      for (k=0; k<=Nz; k++) {
        pp2 = (PR2REC*) AryPtr(pprf, i, j, k);  if (!pp2)          eX(2);
        pm2 = (MD2REC*) AryPtr(pmod, i, j, k);  if (!pm2)          eX(3);
        pv  = (VRFREC*) AryPtr(pvrf, i, j, k);  if (!pv)           eX(4);
        pm2->vx = pp2->vx;
        pm2->vy = pp2->vy;
        pm2->vz = pp2->vz;
        pm2->za = pp2->h;
        pm2->ths = pp2->ts;
        pv->dz = pm2->za - z;
        z = pm2->za;
        }
      pm2 = (MD2REC*) AryPtr(pmod, i, j, 1);  if (!pm2)            eX(5);
      pv  = (VRFREC*) AryPtr(pvrf, i, j, 0);  if (!pv)             eX(6);
      pv->dz = pm2->za - pv->dz;
      }
  if (DefMode & 0x01)  goto defmode_1;
  for (i=0; i<=Nx; i++)               /* Geschwindigkeiten prüfen */
    for (j=0; j<=Ny; j++)
      for (k=0; k<=Nz; k++) {
        ax = 0;  vx = 0;
        i1 = (i > 0)  ? i-1 : i;
        i2 = (i < Nx) ? i+1 : i;
        j1 = (j > 0)  ? j : j+1;
        j2 = (j < Ny) ? j+1 : j;
        k1 = (k > 0)  ? k : k+1;
        k2 = (k < Nz) ? k+1 : k;
        for (ii=i1; ii<=i2; ii++)
          for (jj=j1; jj<=j2; jj++)
            for (kk=k1; kk<=k2; kk++) {
              pm2 = (MD2REC*) AryPtr(pmod, ii, jj, kk);  if (!pm2) eX(7);
              ax = ABS(pm2->vx);
              if (ax > vx)  vx = ax;
              }
        ay = 0;  vy = 0;
        i1 = (i > 0)  ? i : i+1;
        i2 = (i < Nx) ? i+1 : i;
        j1 = (j > 0)  ? j-1 : j;
        j2 = (j < Ny) ? j+1 : j;
        k1 = (k > 0)  ? k : k+1;
        k2 = (k < Nz) ? k+1 : k;
        for (ii=i1; ii<=i2; ii++)
          for (jj=j1; jj<=j2; jj++)
            for (kk=k1; kk<=k2; kk++) {
              pm2 = (MD2REC*) AryPtr(pmod, ii, jj, kk);  if (!pm2) eX(8);
              ay = ABS(pm2->vy);
              if (ay > vy)  vy = ay;
              }
        az = 0;  vz = 0;
/*-------*/
        i1 = (i > 0)  ? i : i+1;
        i2 = (i < Nx) ? i+1 : i;
        j1 = (j > 0)  ? j : j+1;
        j2 = (j < Ny) ? j+1 : j;
        k1 = (k > 0)  ? k-1 : k;
        k2 = (k < Nz) ? k+1 : k;
        for (ii=i1; ii<=i2; ii++)
          for (jj=j1; jj<=j2; jj++)
            for (kk=k1; kk<=k2; kk++) {
              pm2 = (MD2REC*) AryPtr(pmod, ii, jj, kk);  if (!pm2) eX(9);
              az = ABS(pm2->vz);
              if (az > vz)  vz = az;
              }
/*------*/
        pv = (VRFREC*) AryPtr(pvrf, i, j, k);  if (!pv)            eX(10);
        pv->vx = vx;
        pv->vy = vy;
        pv->vz = vz;
        }
  return 0;

defmode_1:
  for (i=0; i<=Nx; i++)               /* Geschwindigkeiten prüfen */
    for (j=0; j<=Ny; j++)
      for (k=0; k<=Nz; k++) {
        ax = 0;  vx = 0;
        ay = 0;  vy = 0;
        az = 0;  vz = 0;
        i1 = (i > 0)  ? i-1 : i;
        i2 = (i < Nx) ? i+1 : i;
        j1 = (j > 0)  ? j-1 : j;
        j2 = (j < Ny) ? j+1 : j;
        k1 = (k > 0)  ? k-1 : k;
        k2 = (k < Nz) ? k+1 : k;
        for (ii=i1; ii<=i2; ii++)
          for (jj=j1; jj<=j2; jj++)
            for (kk=k1; kk<=k2; kk++) {
              pm2 = (MD2REC*) AryPtr(pmod, ii, jj, kk);  if (!pm2) eX(11);
              ax = ABS(pm2->vx);
              if (ax > vx)  vx = ax;
              ay = ABS(pm2->vy);
              if (ay > vy)  vy = ay;
              az = ABS(pm2->vz);
              if (az > vz)  vz = az;
              }
        pv = (VRFREC*) AryPtr(pvrf, i, j, k);  if (!pv)            eX(12);
        pv->vx = vx;
        pv->vy = vy;
        pv->vz = vz;
        }
  return 0;
eX_2: eX_3: eX_4: eX_10: eX_12:
  eMSG(_index_error_$$$_, i, j, k);
eX_5: eX_6:
  eMSG(_internal_error_);
eX_7: eX_8: eX_9: eX_11:
  eMSG(_index_error_$$$_, ii, jj, kk);
  }

  /*================================================================== ClcTau
   * Der Zeitschritt Tau wird so festgelegt, dass er höchstens gleich
   * dem Gamma[0]-fachen der kleinsten Lagrange-Korrel.-Zeit ist, und
   * innerhalb eines Zeitschrittes durch Diffusion hoechstens das
   * Gamma[1]-fache und durch Advektion hoechstens das Gamma[2]-fache
   * der Ausdehnung einer Gitterzelle zurueckgelegt wird.
   *------------------------------------------------------------------------*/
static long ClcTau(     /* calculate time step                  */
  void *pp,             /* pointer to profile values            */
  void *pm,             /* pointer to model field values        */
  VRFREC *pv )          /* pointer to reference velocities      */
  {
  MD2REC *pm2;
  MD3REC *pm3;
  PR3REC *pp3;
  float tau, tu, tv, tw, tm, su, sv, sw, dd, d, dx, dy, dz, sh;
  float gam0, gam1, gam2;
  gam0 = Ppi->gamma[0];
  gam1 = Ppi->gamma[1];
  gam2 = Ppi->gamma[2];
  tm = MOD_DEFTLM;              /* Maximale Korrelationszeit */
  pp3 = (PR3REC*) pp;
  sw = pp3->sw;                 /* Berechnung der kleinsten Lagr.-Kor.-Zeit */
  if (sw > 0.0) {
    tw = pp3->kw/(sw*sw);
    if (tw <= 0.0)  tw = MOD_DEFTLW;
    }
  else  tw = MOD_DEFTLW;
  if (tm > tw)  tm = tw;
  sv = pp3->sv;
  if (sv > 0.0) {
    tv = pp3->kv/(sv*sv);
    if (tv <= 0.0)  tv = MOD_DEFTLV;
    }
  else  tv = MOD_DEFTLV;
  if (tm > tv)  tm = tv;
  sh = sv;
  if (PrfMode > 2) {            /* Bei Anisotropie auch u-Komponente pruefen */
    su = pp3->su;
    if (su > 0.0)  {
      tu = pp3->ku/(su*su);
      if (tu <= 0.0)  tu = MOD_DEFTLU;
      }
    else  tu = MOD_DEFTLU;
    if (tm > tu)  tm = tu;
    if (su > sh)  sh = su;
    }
  tau = gam0*tm;                     /* Begrenzung durch Korrel.-Zeiten */
  if (tau > Ppi->step)               /* Absolute Begrenzung (Vorgabe)   */
    tau = Ppi->step;
  dd = Pgp->dd;                    /* Begrenzung durch Diffusion ...  */
  dz = pv->dz;
  if (sw*tau > gam1*dz)  tau = gam1*dz/sw;
  if (sh*tau > gam1*dd)  tau = gam1*dd/sh;
  dx = tau*pv->vx;                        /* ... und durch Advektion */
  dy = tau*pv->vy;
  d = ABS(dx) + ABS(dy);
  if (d > gam2*dd)  tau *= gam2*dd/d;
  if (PrfMode == 3) {                   /* Für anisotrope Modellierung: */
    pm3 = (MD3REC*) pm;
    pm3->tau = tau;                       /* Zeitschritt merken ...     */
    pm3->pxx = 0.5*tau/tu;                /* ... und Korrelationszeiten */
    pm3->pyy = 0.5*tau/tv;
    pm3->pzz = 0.5*tau/tw;
/*
if (CheckOut) {
fprintf(MsgFile, "ClcTau:\ntau=%14.7e, tm=%14.7e, z=%14.7e\n", tau, tm, pm3->za);
fprintf(MsgFile, " ku=%14.7e, kv=%14.7e, kw=%14.7e\n", pp3->ku, pp3->kv, pp3->kw);
fprintf(MsgFile, " tu=%14.7e, tv=%14.7e, tw=%14.7e, d=%14.7e, dd=%14.7e\n",
tu, tv, tw, d, dd);
fprintf(MsgFile, " su=%14.7e, sv=%14.7e, sw=%14.7e\n", su, sv, sw);
}
*/
    }
  else {                                  /* Für isotrope Modellierung: */
    pm2 = (MD2REC*) pm;
    pm2->tau = tau;
    pm2->pvv = 0.5*tau/tv;
    pm2->pww = 0.5*tau/tw;
    }
  return 0;
  }

  /*================================================================== ClcPsi
   * Berechnung der Tensoren Psi, Sigma und Lambda im Windsystem,
   * anschließend Drehung der Tensoren ins ortsfeste System. Sigma
   * wird im SIGarr abgespeichert, damit es später für die
   * Divergenz-Berechnung zur Verfügung steht.
   *------------------------------------------------------------------------*/
static long ClcPsi(     /* Calculate Psi, Sigma, Lambda         */
  void *pp,             /* pointer to profile values            */
  void *pm,             /* pointer to model field values        */
  MAT *psig,            /* pointer to sigma                     */
  VRFREC *pr )          /* pointer to reference velocity        */
  {
  dP(ClcPsi);
  MAT psi, sig, lam, oma, rot, trn, aux, eta;
  MATRIX m_psi, m_sig, m_lam, m_oma, m_rot, m_trn, m_aux, m_eta;
  float pu, pv, pw, q, dp, ru, rw;
  float vx, vy, vz, gg, a, f, sv, sw;
  PR2REC *pp2;
  PR3REC *pp3;
  MD2REC *pm2;
  MD3REC *pm3;
  void *ppsi;
  m_psi = (MATRIX) &psi;     /* Auf die Matrix nnn kann in der Schreibweise */
  m_sig = (MATRIX) &sig;     /* nnn.ab über die Komponenten-Namen a und b   */
  m_lam = (MATRIX) &lam;     /* zugegriffen werden, in der Schreibweise     */
  m_oma = (MATRIX) &oma;     /* m_nnn[i][j] über die Indizes i und j.       */
  m_rot = (MATRIX) &rot;
  m_trn = (MATRIX) &trn;
  m_aux = (MATRIX) &aux;
  m_eta = (MATRIX) &eta;
  MatSet( 0, m_psi );                /* Alle Werte auf 0 initialisieren */
  MatSet( 0, m_sig );
  MatSet( 0, m_lam );
  MatSet( 0, m_rot );
  MatSet( 0, m_oma );
  MatSet( 0, m_eta );
  if (PrfMode == 3) {       /* Anisotrope Modellierung */
    pp3 = (PR3REC*) pp;
    pm3 = (MD3REC*) pm;
    ppsi = &(pm3->pxx);
    sig.xx = pp3->su*pp3->su;
    sig.yy = pp3->sv*pp3->sv;
    sig.zz = pp3->sw*pp3->sw;
    sig.xz = pp3->sc;
    sig.zx = sig.xz;
    vx = pr->vx;
    vy = pr->vy;
    vz = pr->vz;
    pu = pm3->pxx;
    pv = pm3->pyy;
    pw = pm3->pzz;
    a = sig.xx*sig.zz;
    if (a > 0.0)  q = sig.xz*sig.xz/a;
    else  q = 0.0;
    if (sig.xx > 0.0)  ru = sig.xz/sig.xx;
    else  ru = 0.0;
    if (sig.zz > 0.0)  rw = sig.xz/sig.zz;
    else  rw = 0.0;
    dp = (1.0 + pu)*(1.0 + pw) - q*pu*pw;
    psi.xx = ((1.0 - pu)*(1.0 + pw) + q*pu*pw)/dp;      /* Tensor Psi */
    psi.xy = 0;
    psi.xz = -2.0*pw*rw/dp;
    psi.yx = 0.0;
    psi.yy = (1.0 - pv)/(1.0 + pv);
    psi.yz = 0.0;
    psi.zx = -2.0*pu*ru/dp;
    psi.zy = 0.0;
    psi.zz = ((1.0 + pu)*(1.0 - pw) + q*pu*pw)/dp;
    f = 4.0/(dp*dp);
    a = 1.0 + (1.0-q)*pw;
    oma.xx = f*sig.xx*(q*pw + pu*a*a);             /* Tensor Omega */
    oma.yy = 4.0*sig.yy*pv/((1.0+pv)*(1.0+pv));
    a = 1.0 + (1.0-q)*pu;
    oma.zz = f*sig.zz*(q*pu + pw*a*a);
    oma.xz = f*sig.xz*(pu + pw + 2.0*(1.0-q)*pu*pw);
    oma.zx = oma.xz;
    MatSet( 1.0, m_rot );                       /* Drehmatrix berechnen */
    gg = sqrt(vx*vx + vy*vy);
    if (gg > 0.0) {
      vx /= gg;
      vy /= gg;
      rot.xx = vx;
      rot.yy = vx;
      rot.xy = -vy;
      rot.yx = vy;
      }
    /* oma0 = oma; */
    MatTrn( m_rot, m_trn );
    MatMul( m_rot, m_psi, m_aux );                /* Tensor Psi drehen   */
    MatMul( m_aux, m_trn, m_psi );
    MatMul( m_rot, m_oma, m_aux );                /* Tensor Omega drehen */
    MatMul( m_aux, m_trn, m_oma );
    MatMul( m_rot, m_sig, m_aux );                /* Tensor Sigma drehen */
    MatMul( m_aux, m_trn, m_sig );
    if (MatChl(m_oma, m_lam))                   eX(1);
    if (MatChl(m_sig, m_eta))                   eX(2);
    memcpy(ppsi, &psi, sizeof(psi));            /* Psi nach Mod-Array   */
    memcpy(psig, &sig, sizeof(sig));            /* Sigma nach Sig-Array */
    pm3->lxx = lam.xx;              /* Lambda elementweise nach Mod-Array */
    pm3->lyy = lam.yy;
    pm3->lzz = lam.zz;
    pm3->lyx = lam.yx;
    pm3->lzx = lam.zx;
    pm3->lzy = lam.zy;
    pm3->exx = eta.xx;              /* Lambda elementweise nach Mod-Array */
    pm3->eyy = eta.yy;
    pm3->ezz = eta.zz;
    pm3->eyx = eta.yx;
    pm3->ezx = eta.zx;
    pm3->ezy = eta.zy;
    }
  else {                   /* Isotrope Modellierung */
    pp2 = (PR2REC*) pp;
    pm2 = (MD2REC*) pm;
    sv = pp2->sv;
    sw = pp2->sw;
    sig.xx = sv*sv;
    sig.yy = sv*sv;
    sig.zz = sw*sw;
    pv = pm2->pvv;
    pw = pm2->pww;
    pm2->evv = sv;
    pm2->eww = sw;
    pm2->pvv = (1.0 - pv)/(1.0 + pv);
    pm2->pww = (1.0 - pw)/(1.0 + pw);
if (CheckOut) {
fprintf(MsgFile, "ClcPsi:\npv=%12.5e, pw=%12.5e\n", pm2->pvv, pm2->pww);
}
    pm2->lvv = 2.0*sv*sqrt(pv)/(1.0+pv);
    pm2->lww = 2.0*sw*sqrt(pw)/(1.0+pw);
    memcpy(psig, &sig, sizeof(sig));
    }
  return 0;
eX_1:
  PrnMat("OMEGA", &oma);
  eMSG(_error_choleski_omega_);
eX_2:
  PrnMat("SIGMA", &sig);
  eMSG(_error_choleski_sigma_);
  }

  /*================================================================= Clc1dDiv
  */
static long Clc1dDiv(  /* drift velocity for 1-dimensional profile */
  void )
  {
  dP(Clc1dDiv);
  int k;
  float ta, tb, wx, wy, wz, dz;
  float pvva, pvvb, pwwa, pwwb;
  MAT siga, sigb, psia, psib;
  void *ppa, *ppb;
  MD2REC *pm2a, *pm2b;
  MD3REC *pm3a, *pm3b;
  ARYDSC *psig, *pmod;
  vLOG(6)("MOD:Clc1dDiv() ...");
  psig = Psig;
  pmod = Pmod;
  if (PrfMode == 3) {                        /* Anisotrope Modellierung */
    for (k=1; k<=Nz; k++) {
      ppa = AryPtr(psig, k-1);  if (!ppa)                  eX(101);
      ppb = AryPtr(psig, k  );  if (!ppb)                  eX(102);
      pm3a = (MD3REC*) AryPtr(pmod, k-1);  if (!pm3a)      eX(103);
      pm3b = (MD3REC*) AryPtr(pmod, k  );  if (!pm3b)      eX(104);
      ta = pm3a->tau;
      tb = pm3b->tau;
      memcpy( &siga, ppa, sizeof(siga) );
      memcpy( &sigb, ppb, sizeof(sigb) );
      memcpy( &psia, &(pm3a->pxx), sizeof(psia) );
      memcpy( &psib, &(pm3b->pxx), sizeof(psib) );
      dz = pm3b->za - pm3a->za;                         /* Intervall-Länge */
      wx = (sigb.zx - siga.zx)/dz;
      wy = (sigb.zy - siga.zy)/dz;
      wz = (sigb.zz - siga.zz)/dz;
      pm3b->wx = 0;
      pm3b->wy = 0;
      pm3b->wz = 0.5*( (ta*psia.zx+tb*psib.zx)*wx + (ta*psia.zy+tb*psib.zy)*wy
                   + (ta*psia.zz+tb*psib.zz)*wz );
      wx = (tb*sigb.zx - ta*siga.zx)/dz;
      wy = (tb*sigb.zy - ta*siga.zy)/dz;
      wz = (tb*sigb.zz - ta*siga.zz)/dz;
      pm3b->wz += 0.25*( (0.0-psia.zx-psib.zx)*wx + (0.0-psia.zy-psib.zy)*wy
                   + (2.0-psia.zz-psib.zz)*wz );
      }
    }
  else {
    if (PrfMode < 2)                                    eX(1);
    for (k=1; k<=Nz; k++) {
      ppa = AryPtr(psig, k-1);  if (!ppa)                  eX(11);
      ppb = AryPtr(psig, k  );  if (!ppb)                  eX(12);
      pm2a = (MD2REC*) AryPtr(pmod, k-1);  if (!pm2a)      eX(13);
      pm2b = (MD2REC*) AryPtr(pmod, k  );  if (!pm2b)      eX(14);
      ta = pm2a->tau;
      tb = pm2b->tau;
      memcpy(&siga, ppa, sizeof(siga));
      memcpy(&sigb, ppb, sizeof(sigb));
      pvva = pm2a->pvv;
      pvvb = pm2b->pvv;
      pwwa = pm2a->pww;
      pwwb = pm2b->pww;
      dz = pm2b->za - pm2a->za;
      wz = (sigb.zz - siga.zz)/dz;
      pm2b->wx = 0.0;
      pm2b->wy = 0.0;
      pm2b->wz = 0.5*(ta*pwwa + tb*pwwb)*wz;
      wz = (tb*sigb.zz - ta*siga.zz)/dz;
      pm2b->wz += 0.25*(2.0 - pwwa - pwwb)*wz;
      }
    }
  vLOG(6)("MOD:Clc1dDiv returning");
  return 0;
eX_101: eX_102: eX_103: eX_104: eX_11: eX_12: eX_13: eX_14:
  eMSG(_index_error_$_, k);
eX_1:
  eMSG(_mode_$_not_implemented_, PrfMode);
  }

/*=================================================================== Clc3dDiv
*/
static void matmul( MAT *p, float f )   /* Matrix mit Faktor multiplizieren */
  {
  p->xx *= f;  p->xy *= f;  p->xz *=f;
  p->yx *= f;  p->yy *= f;  p->yz *=f;
  p->zx *= f;  p->zy *= f;  p->zz *=f;
  }

static long Clc3dDiv(        /* drift velocity for 3-dimensional profile */
  void )
  {
  dP(Clc3dDiv);
  int i, j, k;
  MAT   sig000, sig100, sig010, sig001, sig110, sig101, sig011, sig111;
  MAT   psi000, psi100, psi010, psi001, psi110, psi101, psi011, psi111;
  MAT   fak;
  float tau000, tau100, tau010, tau001, tau110, tau101, tau011, tau111;
  void   *p00, *p10, *p01, *p11;
  MD3REC *q300, *q310, *q301, *q311;
  MD2REC *q200, *q210, *q201, *q211;
  MAT    *p200, *p210, *p201, *p211;
  ARYDSC *psig, *pmod;
  float z000, z100, z010, z001, z110, z101, z011, z111;
  float ax0x, ax1x, ay0y, ay1y, az0x, az0y, az0z, az1x, az1y, az1z, axy;
  float dd, volumen;
  float divx, divy, divz, wx, wy, wz;
  float svv000, svv100, svv010, svv001, svv110, svv101, svv011, svv111;
  float pvv000, pvv100, pvv010, pvv001, pvv110, pvv101, pvv011, pvv111;
  float sww000, sww100, sww010, sww001, sww110, sww101, sww011, sww111;
  float pww000, pww100, pww010, pww001, pww110, pww101, pww011, pww111;
  float fvv, fww;
  psig = Psig;
  pmod = Pmod;
  dd = Dd;
  if (PrfMode<2 || PrfMode>3)                                   eX(1);
  if (PrfMode == 3)  goto prfmode_3;
  for (i=1; i<=Nx; i++)                 /*------ isotropic turbulence ------*/
    for (j=1; j<=Ny; j++) {
      p200 = (MAT*) AryPtr(psig, i-1, j-1, 0);  if (!p200)         eX(11);
      p210 = (MAT*) AryPtr(psig, i  , j-1, 0);  if (!p210)         eX(12);
      p201 = (MAT*) AryPtr(psig, i-1, j  , 0);  if (!p201)         eX(13);
      p211 = (MAT*) AryPtr(psig, i  , j  , 0);  if (!p211)         eX(14);
      q200 = (MD2REC*) AryPtr(pmod, i-1, j-1, 0);  if (!q200)      eX(21);
      q210 = (MD2REC*) AryPtr(pmod, i  , j-1, 0);  if (!q210)      eX(22);
      q201 = (MD2REC*) AryPtr(pmod, i-1, j  , 0);  if (!q201)      eX(23);
      q211 = (MD2REC*) AryPtr(pmod, i  , j  , 0);  if (!q211)      eX(24);
      tau000 = q200->tau;                     /* Zeitschritt Tau unten */
      tau100 = q210->tau;
      tau010 = q201->tau;
      tau110 = q211->tau;
      z000 = q200->za;                        /* Absolute Höhe z unten */
      z100 = q210->za;
      z010 = q201->za;
      z110 = q211->za;
      for (k=1; k<=Nz; k++) {
        svv000 = p200->yy;     /*-22aug92-*/          /* Sigma.vv unten */
        svv100 = p210->yy;
        svv010 = p201->yy;
        svv110 = p211->yy;
        sww000 = p200->zz;                            /* Sigma.ww unten */
        sww100 = p210->zz;
        sww010 = p201->zz;
        sww110 = p211->zz;
        pvv000 = q200->pvv;                             /* Psi.vv unten */
        pvv100 = q210->pvv;
        pvv010 = q201->pvv;
        pvv110 = q211->pvv;
        pww000 = q200->pww;                             /* Psi.ww unten */
        pww100 = q210->pww;
        pww010 = q201->pww;
        pww110 = q211->pww;
        p200 = (MAT*) AryPtr(psig, i-1, j-1, k);  if (!p200)       eX(31);
        p210 = (MAT*) AryPtr(psig, i  , j-1, k);  if (!p210)       eX(32);
        p201 = (MAT*) AryPtr(psig, i-1, j  , k);  if (!p201)       eX(33);
        p211 = (MAT*) AryPtr(psig, i  , j  , k);  if (!p211)       eX(34);
        svv001 = p200->yy;     /*-22aug92-*/           /* Sigma.vv oben */
        svv101 = p210->yy;
        svv011 = p201->yy;
        svv111 = p211->yy;
        sww001 = p200->zz;                             /* Sigma.ww oben */
        sww101 = p210->zz;
        sww011 = p201->zz;
        sww111 = p211->zz;
        q200 = (MD2REC*) AryPtr(pmod, i-1, j-1, k);  if (!q200)    eX(41);
        q210 = (MD2REC*) AryPtr(pmod, i  , j-1, k);  if (!q210)    eX(42);
        q201 = (MD2REC*) AryPtr(pmod, i-1, j  , k);  if (!q201)    eX(43);
        q211 = (MD2REC*) AryPtr(pmod, i  , j  , k);  if (!q211)    eX(44);
        pvv001 = q200->pvv;                              /* Psi.vv oben */
        pvv101 = q210->pvv;
        pvv011 = q201->pvv;
        pvv111 = q211->pvv;
        pww001 = q200->pww;                              /* Psi.ww oben */
        pww101 = q210->pww;
        pww011 = q201->pww;
        pww111 = q211->pww;
        tau001 = q200->tau;                      /* Zeitschritt Tau oben */
        tau101 = q210->tau;
        tau011 = q201->tau;
        tau111 = q211->tau;
        z001 = q200->za;                         /* Absolute Höhe z oben */
        z101 = q210->za;
        z011 = q201->za;
        z111 = q211->za;
        ax0x = -0.5*dd*(z001 - z000 + z011 - z010);     /* Seitenflächen */
        ax1x =  0.5*dd*(z101 - z100 + z111 - z110);
        ay0y = -0.5*dd*(z001 - z000 + z101 - z100);
        ay1y =  0.5*dd*(z011 - z010 + z111 - z110);
        axy = dd*dd;
        az0x = -0.5*dd*(z000 - z100 + z010 - z110);   /* Projektion unten */
        az0y = -0.5*dd*(z000 - z010 + z100 - z110);
        az0z = -dd*dd;
        az1x =  0.5*dd*(z001 - z101 + z011 - z111);    /* Projektion oben */
        az1y =  0.5*dd*(z001 - z011 + z101 - z111);
        az1z =  dd*dd;
        volumen = 0.25*axy*(z001-z000 + z101-z100 + z011-z010 + z111-z110);
        divx = 0.25*(ax1x*(svv100 + svv110 + svv101 + svv111)
                   + ax0x*(svv000 + svv010 + svv001 + svv011)
                   + az1x*(svv001 + svv101 + svv011 + svv111)
                   + az0x*(svv000 + svv100 + svv010 + svv110));
        divy = 0.25*(ay1y*(svv010 + svv110 + svv011 + svv111)
                   + ay0y*(svv000 + svv100 + svv001 + svv101)
                   + az1y*(svv001 + svv101 + svv011 + svv111)
                   + az0y*(svv000 + svv100 + svv010 + svv110));
        divz = 0.25*(az1z*(sww001 + sww101 + sww011 + sww111)
                   + az0z*(sww000 + sww100 + sww010 + sww110));
        divx /= volumen;
        divy /= volumen;               /* = Divergenz ( Sigma ) */
        divz /= volumen;
        fvv    = ( tau000*pvv000 + tau001*pvv001
                 + tau100*pvv100 + tau101*pvv101
                 + tau010*pvv010 + tau011*pvv011
                 + tau110*pvv110 + tau111*pvv111 )/8;
        fww    = ( tau000*pww000 + tau001*pww001     /* = Mittelwert */
                 + tau100*pww100 + tau101*pww101     /*  von Tau*Psi */
                 + tau010*pww010 + tau011*pww011
                 + tau110*pww110 + tau111*pww111 )/8;
        wx = fvv*divx;
        wy = fvv*divy;        /*  2. Anteil */
        wz = fww*divz;
        svv000 *= tau000;
        svv100 *= tau100;
        svv010 *= tau010;       /* Tau*Sigma bilden ...   */
        svv001 *= tau001;
        svv110 *= tau110;
        svv011 *= tau011;
        svv101 *= tau101;
        svv111 *= tau111;
        sww000 *= tau000;
        sww100 *= tau100;
        sww010 *= tau010;
        sww001 *= tau001;
        sww110 *= tau110;       /* ... und die Divergenz: */
        sww011 *= tau011;
        sww101 *= tau101;
        sww111 *= tau111;
        divx = 0.25*(ax1x*(svv100 + svv110 + svv101 + svv111)
                   + ax0x*(svv000 + svv010 + svv001 + svv011)
                   + az1x*(svv001 + svv101 + svv011 + svv111)
                   + az0x*(svv000 + svv100 + svv010 + svv110));
        divy = 0.25*(ay1y*(svv010 + svv110 + svv011 + svv111)
                   + ay0y*(svv000 + svv100 + svv001 + svv101)
                   + az1y*(svv001 + svv101 + svv011 + svv111)
                   + az0y*(svv000 + svv100 + svv010 + svv110));
        divz = 0.25*(az1z*(sww001 + sww101 + sww011 + sww111)
                   + az0z*(sww000 + sww100 + sww010 + sww110));
        divx /= volumen;
        divy /= volumen;               /* = Divergenz ( Tau*Sigma ) */
        divz /= volumen;
        fvv    = (8 - pvv000 - pvv100 - pvv010 - pvv110
                    - pvv001 - pvv101 - pvv011 - pvv111 )/16;
        fww    = (8 - pww000 - pww100 - pww010 - pww110
                    - pww001 - pww101 - pww011 - pww111 )/16;
        wx += fvv*divx;
        wy += fvv*divy;       /*  1. Anteil */
        wz += fww*divz;
        q211->wx = wx;
        q211->wy = wy;                                  /* Ergebnis merken */
        q211->wz = wz;
        z000 = z001;  z100 = z101;  z010 = z011;  z110 = z111;
        tau000 = tau001;  tau100 = tau101;  tau010 = tau011;  tau110 = tau111;
        }
    }
  return 0;

prfmode_3:               /*---------------- anisotropic turbulence -------*/
  for (i=1; i<=Nx; i++)
    for (j=1; j<=Ny; j++) {
      p00 = AryPtr(psig, i-1, j-1, 0);  if (!p00)                  eX(51);
      p10 = AryPtr(psig, i  , j-1, 0);  if (!p10)                  eX(52);
      p01 = AryPtr(psig, i-1, j  , 0);  if (!p01)                  eX(53);
      p11 = AryPtr(psig, i  , j  , 0);  if (!p11)                  eX(54);
      q300 = (MD3REC*) AryPtr(pmod, i-1, j-1, 0);  if (!q300)      eX(61);
      q310 = (MD3REC*) AryPtr(pmod, i  , j-1, 0);  if (!q310)      eX(62);
      q301 = (MD3REC*) AryPtr(pmod, i-1, j  , 0);  if (!q301)      eX(63);
      q311 = (MD3REC*) AryPtr(pmod, i  , j  , 0);  if (!q311)      eX(64);
      tau000 = q300->tau;                      /* Zeitschritt Tau unten */
      tau100 = q310->tau;
      tau010 = q301->tau;
      tau110 = q311->tau;
      z000 = q300->za;                         /* Absolute Hoehe z unten */
      z100 = q310->za;
      z010 = q301->za;
      z110 = q311->za;
      for (k=1; k<=Nz; k++) {
        memcpy( &sig000, p00, sizeof(MAT) );       /* Tensor Sigma unten */
        memcpy( &sig100, p10, sizeof(MAT) );
        memcpy( &sig010, p01, sizeof(MAT) );
        memcpy( &sig110, p11, sizeof(MAT) );
        memcpy( &psi000, &q300->pxx, sizeof(MAT) );  /* Tensor Psi unten */
        memcpy( &psi100, &q310->pxx, sizeof(MAT) );
        memcpy( &psi010, &q301->pxx, sizeof(MAT) );
        memcpy( &psi110, &q311->pxx, sizeof(MAT) );
        p00 = AryPtr(psig, i-1, j-1, k);  if (!p00)                eX(71);
        p10 = AryPtr(psig, i  , j-1, k);  if (!p10)                eX(72);
        p01 = AryPtr(psig, i-1, j  , k);  if (!p01)                eX(73);
        p11 = AryPtr(psig, i  , j  , k);  if (!p11)                eX(74);
        memcpy( &sig001, p00, sizeof(MAT) );        /* Tensor Sigma oben */
        memcpy( &sig101, p10, sizeof(MAT) );
        memcpy( &sig011, p01, sizeof(MAT) );
        memcpy( &sig111, p11, sizeof(MAT) );
        q300 = (MD3REC*) AryPtr(pmod, i-1, j-1, k);  if (!q300)    eX(81);
        q310 = (MD3REC*) AryPtr(pmod, i  , j-1, k);  if (!q310)    eX(82);
        q301 = (MD3REC*) AryPtr(pmod, i-1, j  , k);  if (!q301)    eX(83);
        q311 = (MD3REC*) AryPtr(pmod, i  , j  , k);  if (!q311)    eX(84);
        memcpy( &psi001, &q300->pxx, sizeof(MAT) );   /* Tensor Psi oben */
        memcpy( &psi101, &q310->pxx, sizeof(MAT) );
        memcpy( &psi011, &q301->pxx, sizeof(MAT) );
        memcpy( &psi111, &q311->pxx, sizeof(MAT) );
        tau001 = q300->tau;                      /* Zeitschritt Tau oben */
        tau101 = q310->tau;
        tau011 = q301->tau;
        tau111 = q311->tau;
        z001 = q300->za;                         /* Absolute Hoehe z oben */
        z101 = q310->za;
        z011 = q301->za;
        z111 = q311->za;
        ax0x = -0.5*dd*(z001 - z000 + z011 - z010);     /* Seitenflächen */
        ax1x =  0.5*dd*(z101 - z100 + z111 - z110);
        ay0y = -0.5*dd*(z001 - z000 + z101 - z100);
        ay1y =  0.5*dd*(z011 - z010 + z111 - z110);
        axy = dd*dd;
        az0x = -0.5*dd*(z000 - z100 + z010 - z110);   /* Projektion unten */
        az0y = -0.5*dd*(z000 - z010 + z100 - z110);
        az0z = -dd*dd;
        az1x =  0.5*dd*(z001 - z101 + z011 - z111);    /* Projektion oben */
        az1y =  0.5*dd*(z001 - z011 + z101 - z111);
        az1z =  dd*dd;
        volumen = 0.25*axy*(z001-z000 + z101-z100 + z011-z010 + z111-z110);
        divx = 0.25*(ax1x*(sig100.xx + sig110.xx + sig101.xx + sig111.xx)
                   + ax0x*(sig000.xx + sig010.xx + sig001.xx + sig011.xx)
                   + ay1y*(sig010.yx + sig110.yx + sig011.yx + sig111.yx)
                   + ay0y*(sig000.yx + sig100.yx + sig001.yx + sig101.yx)
                   + az1x*(sig001.xx + sig101.xx + sig011.xx + sig111.xx)
                   + az1y*(sig001.yx + sig101.yx + sig011.yx + sig111.yx)
                   + az1z*(sig001.zx + sig101.zx + sig011.zx + sig111.zx)
                   + az0x*(sig000.xx + sig100.xx + sig010.xx + sig110.xx)
                   + az0y*(sig000.yx + sig100.yx + sig010.yx + sig110.yx)
                   + az0z*(sig000.zx + sig100.zx + sig010.zx + sig110.zx));
        divy = 0.25*(ax1x*(sig100.xy + sig110.xy + sig101.xy + sig111.xy)
                   + ax0x*(sig000.xy + sig010.xy + sig001.xy + sig011.xy)
                   + ay1y*(sig010.yy + sig110.yy + sig011.yy + sig111.yy)
                   + ay0y*(sig000.yy + sig100.yy + sig001.yy + sig101.yy)
                   + az1x*(sig001.xy + sig101.xy + sig011.xy + sig111.xy)
                   + az1y*(sig001.yy + sig101.yy + sig011.yy + sig111.yy)
                   + az1z*(sig001.zy + sig101.zy + sig011.zy + sig111.zy)
                   + az0x*(sig000.xy + sig100.xy + sig010.xy + sig110.xy)
                   + az0y*(sig000.yy + sig100.yy + sig010.yy + sig110.yy)
                   + az0z*(sig000.zy + sig100.zy + sig010.zy + sig110.zy));
        divz = 0.25*(ax1x*(sig100.xz + sig110.xz + sig101.xz + sig111.xz)
                   + ax0x*(sig000.xz + sig010.xz + sig001.xz + sig011.xz)
                   + ay1y*(sig010.yz + sig110.yz + sig011.yz + sig111.yz)
                   + ay0y*(sig000.yz + sig100.yz + sig001.yz + sig101.yz)
                   + az1x*(sig001.xz + sig101.xz + sig011.xz + sig111.xz)
                   + az1y*(sig001.yz + sig101.yz + sig011.yz + sig111.yz)
                   + az1z*(sig001.zz + sig101.zz + sig011.zz + sig111.zz)
                   + az0x*(sig000.xz + sig100.xz + sig010.xz + sig110.xz)
                   + az0y*(sig000.yz + sig100.yz + sig010.yz + sig110.yz)
                   + az0z*(sig000.zz + sig100.zz + sig010.zz + sig110.zz));
        divx /= volumen;
        divy /= volumen;               /* = Divergenz ( Sigma ) */
        divz /= volumen;
        fak.xx = ( tau000*psi000.xx + tau001*psi001.xx
                 + tau100*psi100.xx + tau101*psi101.xx
                 + tau010*psi010.xx + tau011*psi011.xx
                 + tau110*psi110.xx + tau111*psi111.xx )/8;
        fak.xy = ( tau000*psi000.xy + tau001*psi001.xy
                 + tau100*psi100.xy + tau101*psi101.xy
                 + tau010*psi010.xy + tau011*psi011.xy
                 + tau110*psi110.xy + tau111*psi111.xy )/8;
        fak.xz = ( tau000*psi000.xz + tau001*psi001.xz
                 + tau100*psi100.xz + tau101*psi101.xz
                 + tau010*psi010.xz + tau011*psi011.xz
                 + tau110*psi110.xz + tau111*psi111.xz )/8;
        fak.yx = ( tau000*psi000.yx + tau001*psi001.yx
                 + tau100*psi100.yx + tau101*psi101.yx
                 + tau010*psi010.yx + tau011*psi011.yx
                 + tau110*psi110.yx + tau111*psi111.yx )/8;
        fak.yy = ( tau000*psi000.yy + tau001*psi001.yy
                 + tau100*psi100.yy + tau101*psi101.yy
                 + tau010*psi010.yy + tau011*psi011.yy
                 + tau110*psi110.yy + tau111*psi111.yy )/8;
        fak.yz = ( tau000*psi000.yz + tau001*psi001.yz
                 + tau100*psi100.yz + tau101*psi101.yz
                 + tau010*psi010.yz + tau011*psi011.yz
                 + tau110*psi110.yz + tau111*psi111.yz )/8;
        fak.zx = ( tau000*psi000.zx + tau001*psi001.zx
                 + tau100*psi100.zx + tau101*psi101.zx
                 + tau010*psi010.zx + tau011*psi011.zx
                 + tau110*psi110.zx + tau111*psi111.zx )/8;
        fak.zy = ( tau000*psi000.zy + tau001*psi001.zy
                 + tau100*psi100.zy + tau101*psi101.zy
                 + tau010*psi010.zy + tau011*psi011.zy
                 + tau110*psi110.zy + tau111*psi111.zy )/8;
        fak.zz = ( tau000*psi000.zz + tau001*psi001.zz     /* = Mittelwert */
                 + tau100*psi100.zz + tau101*psi101.zz     /*  von Tau*Psi */
                 + tau010*psi010.zz + tau011*psi011.zz
                 + tau110*psi110.zz + tau111*psi111.zz )/8;
        wx = fak.xx*divx + fak.xy*divy + fak.xz*divz;
        wy = fak.yx*divx + fak.yy*divy + fak.xy*divz;        /*  2. Anteil */
        wz = fak.zx*divx + fak.zy*divy + fak.zz*divz;
        matmul( &sig000, tau000 );
        matmul( &sig100, tau100 );
        matmul( &sig010, tau010 );       /* Tau*Sigma bilden ...   */
        matmul( &sig001, tau001 );
        matmul( &sig110, tau110 );       /* ... und die Divergenz: */
        matmul( &sig011, tau011 );
        matmul( &sig101, tau101 );
        matmul( &sig111, tau111 );
        divx = 0.25*(ax1x*(sig100.xx + sig110.xx + sig101.xx + sig111.xx)
                   + ax0x*(sig000.xx + sig010.xx + sig001.xx + sig011.xx)
                   + ay1y*(sig010.yx + sig110.yx + sig011.yx + sig111.yx)
                   + ay0y*(sig000.yx + sig100.yx + sig001.yx + sig101.yx)
                   + az1x*(sig001.xx + sig101.xx + sig011.xx + sig111.xx)
                   + az1y*(sig001.yx + sig101.yx + sig011.yx + sig111.yx)
                   + az1z*(sig001.zx + sig101.zx + sig011.zx + sig111.zx)
                   + az0x*(sig000.xx + sig100.xx + sig010.xx + sig110.xx)
                   + az0y*(sig000.yx + sig100.yx + sig010.yx + sig110.yx)
                   + az0z*(sig000.zx + sig100.zx + sig010.zx + sig110.zx));
        divy = 0.25*(ax1x*(sig100.xy + sig110.xy + sig101.xy + sig111.xy)
                   + ax0x*(sig000.xy + sig010.xy + sig001.xy + sig011.xy)
                   + ay1y*(sig010.yy + sig110.yy + sig011.yy + sig111.yy)
                   + ay0y*(sig000.yy + sig100.yy + sig001.yy + sig101.yy)
                   + az1x*(sig001.xy + sig101.xy + sig011.xy + sig111.xy)
                   + az1y*(sig001.yy + sig101.yy + sig011.yy + sig111.yy)
                   + az1z*(sig001.zy + sig101.zy + sig011.zy + sig111.zy)
                   + az0x*(sig000.xy + sig100.xy + sig010.xy + sig110.xy)
                   + az0y*(sig000.yy + sig100.yy + sig010.yy + sig110.yy)
                   + az0z*(sig000.zy + sig100.zy + sig010.zy + sig110.zy));
        divz = 0.25*(ax1x*(sig100.xz + sig110.xz + sig101.xz + sig111.xz)
                   + ax0x*(sig000.xz + sig010.xz + sig001.xz + sig011.xz)
                   + ay1y*(sig010.yz + sig110.yz + sig011.yz + sig111.yz)
                   + ay0y*(sig000.yz + sig100.yz + sig001.yz + sig101.yz)
                   + az1x*(sig001.xz + sig101.xz + sig011.xz + sig111.xz)
                   + az1y*(sig001.yz + sig101.yz + sig011.yz + sig111.yz)
                   + az1z*(sig001.zz + sig101.zz + sig011.zz + sig111.zz)
                   + az0x*(sig000.xz + sig100.xz + sig010.xz + sig110.xz)
                   + az0y*(sig000.yz + sig100.yz + sig010.yz + sig110.yz)
                   + az0z*(sig000.zz + sig100.zz + sig010.zz + sig110.zz));
        divx /= volumen;
        divy /= volumen;               /* = Divergenz ( Tau*Sigma ) */
        divz /= volumen;
        fak.xx = (8 - psi000.xx - psi100.xx - psi010.xx - psi110.xx
                    - psi001.xx - psi101.xx - psi011.xx - psi111.xx )/16;
        fak.xy = (0 - psi000.xy - psi100.xy - psi010.xy - psi110.xy
                    - psi001.xy - psi101.xy - psi011.xy - psi111.xy )/16;
        fak.xz = (0 - psi000.xz - psi100.xz - psi010.xz - psi110.xz
                    - psi001.xz - psi101.xz - psi011.xz - psi111.xz )/16;
        fak.yx = (0 - psi000.yx - psi100.yx - psi010.yx - psi110.yx
                    - psi001.yx - psi101.yx - psi011.yx - psi111.yx )/16;
        fak.yy = (8 - psi000.yy - psi100.yy - psi010.yy - psi110.yy
                    - psi001.yy - psi101.yy - psi011.yy - psi111.yy )/16;
        fak.yz = (0 - psi000.yz - psi100.yz - psi010.yz - psi110.yz
                    - psi001.yz - psi101.yz - psi011.yz - psi111.yz )/16;
        fak.zx = (0 - psi000.zx - psi100.zx - psi010.zx - psi110.zx
                    - psi001.zx - psi101.zx - psi011.zx - psi111.zx )/16;
        fak.zy = (0 - psi000.zy - psi100.zy - psi010.zy - psi110.zy
                    - psi001.zy - psi101.zy - psi011.zy - psi111.zy )/16;
        fak.zz = (8 - psi000.zz - psi100.zz - psi010.zz - psi110.zz
                    - psi001.zz - psi101.zz - psi011.zz - psi111.zz )/16;
        wx += fak.xx*divx + fak.xy*divy + fak.xz*divz;
        wy += fak.yx*divx + fak.yy*divy + fak.xy*divz;       /*  1. Anteil */
        wz += fak.zx*divx + fak.zy*divy + fak.zz*divz;
        q311->wx = wx;
        q311->wy = wy;                                  /* Ergebnis merken */
        q311->wz = wz;
        z000 = z001;  z100 = z101;  z010 = z011;  z110 = z111;
        tau000 = tau001;  tau100 = tau101;  tau010 = tau011;  tau110 = tau111;
        }
    }
  return 0;
eX_1:
  eMSG(_mode_$_not_implemented_, PrfMode);
eX_11: eX_12: eX_13: eX_14: eX_21: eX_22: eX_23: eX_24:
eX_51: eX_52: eX_53: eX_54: eX_61: eX_62: eX_63: eX_64:
  eMSG(_index_error_$$_, i, j);
eX_31: eX_32: eX_33: eX_34: eX_41: eX_42: eX_43: eX_44:
eX_71: eX_72: eX_73: eX_74: eX_81: eX_82: eX_83: eX_84:
  eMSG(_index_error_$$$_, i, j, k);
  }

  /*================================================================= Clc3dMod
  */
static long Clc3dMod(   /* Calculate model fields for 3d boundary layer */
  void )
  {
  dP(Clc3dMod);
  int i, j, k;
  long iva, ivb, isa, ipi;
  char name[256], *_hdr=NULL;						//-2004-11-26
  void *pm, *pp;
  MD3REC *pm3;
  PR3REC *pp3;
  VRFREC *pv;
  MAT *ps;
  ARYDSC *pa;
  PRMINF *ppi;
  TXTSTR header = { NULL, 0 };
  ipi = IDENT(PRMpar, 0, 0, 0);
  pa = TmnAttach(ipi, NULL, NULL, 0, NULL);                     eG(50);
  if (!pa)                                                      eX(51);
  ppi = (PRMINF*) pa->start;
  if (ppi->flags & FLG_ONLYADVEC) {                          //-2001-09-23
    for (i=0; i<=Nx; i++)
      for (j=0; j<=Ny; j++)
        for (k=0; k<=Nz; k++) {
          pp3 = AryPtr(Pprf, i, j, k);  if (!pp3)   eX(21);    //-2002-12-19
          pm3 = AryPtr(Pmod, i, j, k);  if (!pm3)   eX(22);    //-2002-12-19
          pm3->za = pp3->h;
          pm3->vx = pp3->vx;
          pm3->vy = pp3->vy;
          pm3->vz = pp3->vz;
          pm3->tau = ppi->step;
          }
    TmnDetach(ipi, NULL, NULL, 0, NULL);                        eG(52);
    return 0;
  }
  TmnDetach(ipi, NULL, NULL, 0, NULL);                          eG(52);
  ppi = NULL;
  iva = IDENT(VRFarr, 0, GridLevel, GridIndex);
  Pvrf = TmnCreate(iva, sizeof(VRFREC), 3, 0, Nx, 0, Ny, 0, Nz);        eG(61);
  Clc3dVrf();                                                   eG(1);
  for (i=0; i<=Nx; i++)
    for (j=0; j<=Ny; j++)
      for (k=0; k<=Nz; k++) {
        pp = AryPtr(Pprf, i, j, k);  if (!pp)                      eX(21);
        pm = AryPtr(Pmod, i, j, k);  if (!pm)                      eX(22);
        pv = AryPtr(Pvrf, i, j, k);  if (!pv)                      eX(23);
        ClcTau(pp, pm, pv);                                     eG(5);
        }
  _hdr = ModHeader(iva, &MetT1, &MetT2);
  TxtCpy(&header, _hdr);
  FREE(_hdr);
  _hdr = NULL;
  TmnDetach(iva, &MetT1, &MetT2, TMN_MODIFY, &header);          eG(62);
  if (StdStatus & MOD_WRITE) {
    TmnStore(iva, TMN_NOID);                                    eG(64);
    }
  TmnClear(MetT2, iva, 0);                                      eG(63);
  ivb = IDENT(VRFarr, 1, GridLevel, GridIndex);
  Pvrf = TmnCreate(ivb, sizeof(VRFREC), 3, 0, Nx, 0, Ny, 0, Nz);        eG(81);
  isa = IDENT(SIGarr, 0, GridLevel, GridIndex);
  Psig = TmnCreate(isa, sizeof(MAT), 3, 0, Nx, 0, Ny, 0, Nz);           eG(91);
  Clc3dInt();                                                   eG(11);
  for (i=0; i<=Nx; i++)
    for (j=0; j<=Ny; j++)
      for (k=0; k<=Nz; k++) {
        pp = AryPtr(Pprf, i, j, k);  if (!pp)                      eX(31);
        pm = AryPtr(Pmod, i, j, k);  if (!pm)                      eX(32);
        ps = AryPtr(Psig, i, j, k);  if (!ps)                      eX(33);
        pv = AryPtr(Pvrf, i, j, k);  if (!pv)                      eX(34);
        ClcPsi(pp, pm, ps, pv);                                 eG(16);
        }
  Clc3dDiv();                                                   eG(17);
  _hdr = ModHeader(ivb, &MetT1, &MetT2);
  TxtCpy(&header, _hdr);
  FREE(_hdr);
  _hdr = NULL;
  TmnDetach(ivb, &MetT1, &MetT2, TMN_MODIFY, &header);          eG(82);
  if (StdStatus & MOD_WRITE) {
    TmnStore(ivb, TMN_NOID);                                    eG(84);
    }
  TmnClear(MetT2, ivb, 0);                                      eG(83);
  Pvrf = NULL;
  _hdr = ModHeader(isa, &MetT1, &MetT2);
  TxtCpy(&header, _hdr);
  FREE(_hdr);
  _hdr = NULL;
  TmnDetach(isa, &MetT1, &MetT2, TMN_MODIFY, &header);          eG(92);
  if (StdStatus & MOD_WRITE) {
    TmnStore(isa, TMN_NOID);                                    eG(94);
    }
  TmnClear(MetT2, isa, 0);                                      eG(93);
  Psig = NULL;
  TxtClr(&header);
  return 0;
eX_1:
  strcpy(name, NmsName(iva));
  eMSG(_cant_calculate_$_, name);
eX_21: eX_22: eX_23: eX_31: eX_32: eX_33: eX_34:
  eMSG(_index_error_$$$_, i, j, k);
eX_5:
  eMSG(_cant_calculate_step_$$$_, i, j, k);
eX_61: eX_62: eX_63: eX_64:
  strcpy(name, NmsName(iva));
  eMSG(_cant_create_$_, name);
eX_81: eX_82: eX_83: eX_84:
  strcpy(name, NmsName(ivb));
  eMSG(_cant_create_$_, name);
eX_91: eX_92: eX_93: eX_94:
  strcpy(name, NmsName(isa));
  eMSG(_cant_create_$_, name);
eX_11:
  eMSG(_cant_calculate_velocities_);
eX_16:
  eMSG(_cant_calculate_psi_$$$_, i, j, k);
eX_17:
  eMSG(_cant_calculate_div_);
eX_50: eX_51: eX_52:
  eMSG(_internal_error_);
  }

  /*================================================================= Clc1dMod
  */
static long Clc1dMod(   /* Calculate model fields for 1d boundary layer */
  void )
  {
  dP(Clc1dMod);
  int k;
  long iva, ivb, isa;
  char name[256], *hdr;						//-2004-11-26
  void *pm, *pp;
  VRFREC *pv;
  MAT *ps;
  TXTSTR header = { NULL, 0 };
  vLOG(4)("MOD:Clc1dMod() ...");
  iva = IDENT(VRFarr, 0, GridLevel, GridIndex);
  Pvrf = TmnCreate(iva, sizeof(VRFREC), 1, 0, Nz);      eG(61);
  Clc1dVrf();                                           eG(1);
  for (k=0; k<=Nz; k++) {
    pp = AryPtr(Pprf, k);  if (!pp)                        eX(21);
    pm = AryPtr(Pmod, k);  if (!pm)                        eX(22);
    pv = AryPtr(Pvrf, k);  if (!pv)                        eX(23);
    ClcTau(pp, pm, pv);                                 eG(5);
    }
  hdr = ModHeader(iva, &MetT1, &MetT2);
  TxtCpy(&header, hdr);
  FREE(hdr);
  hdr = NULL;
  TmnDetach(iva, &MetT1, &MetT2, TMN_MODIFY, &header);  eG(62);
  if (StdStatus & MOD_WRITE) {
    TmnStore(iva, TMN_NOID);                            eG(64);
    }
  TmnClear(MetT2, iva, 0);                              eG(63);
  ivb = IDENT(VRFarr, 1, GridLevel, GridIndex);
  Pvrf = TmnCreate(ivb, sizeof(VRFREC), 1, 0, Nz);      eG(81);
  isa = IDENT(SIGarr, 0, GridLevel, GridIndex);
  Psig = TmnCreate(isa, sizeof(MAT), 1, 0, Nz);         eG(91);
  Clc1dInt();                                           eG(11);
  for (k=0; k<=Nz; k++) {
    pp = AryPtr(Pprf, k);  if (!pp)                        eX(31);
    pm = AryPtr(Pmod, k);  if (!pm)                        eX(32);
    ps = AryPtr(Psig, k);  if (!ps)                        eX(33);
    pv = AryPtr(Pvrf, k);  if (!pv)                        eX(34);
    ClcPsi(pp, pm, ps, pv);                             eG(16);
    }
  Clc1dDiv();                                           eG(17);
  hdr = ModHeader(ivb, &MetT1, &MetT2);
  TxtCpy(&header, hdr);
  FREE(hdr);
  hdr = NULL;
  TmnDetach(ivb, &MetT1, &MetT2, TMN_MODIFY, &header);  eG(82);
  if (StdStatus & MOD_WRITE) {
    TmnStore(ivb, TMN_NOID);                            eG(84);
    }
  TmnClear(MetT2, ivb, 0);                              eG(83);
  Pvrf = NULL;
  hdr = ModHeader(isa, &MetT1, &MetT2);
  TxtCpy(&header, hdr);
  FREE(hdr);
  hdr = NULL;
  TmnDetach(isa, &MetT1, &MetT2, TMN_MODIFY, &header);  eG(92);
  if (StdStatus & MOD_WRITE) {
    TmnStore(isa, TMN_NOID);                            eG(94);
    }
  TmnClear(MetT2, isa, 0);                              eG(93);
  Psig = NULL;
  vLOG(4)("MOD:Clc1dMod returning");
  TxtClr(&header);
  return 0;
eX_1:
  strcpy(name, NmsName(iva));
  eMSG(_cant_calculate_$_, name);
eX_21: eX_22: eX_23: eX_31: eX_32: eX_33: eX_34:
  eMSG(_index_error_$_, k);
eX_5:
  eMSG(_cant_calculate_step_$_, k);
eX_61: eX_62: eX_63: eX_64:
  strcpy(name, NmsName(iva));
  eMSG(_cant_create_$_, name);
eX_81: eX_82: eX_83: eX_84:
  strcpy(name, NmsName(ivb));
  eMSG(_cant_create_$_, name);
eX_91: eX_92: eX_93: eX_94:
  strcpy(name, NmsName(isa));
  eMSG(_cant_create_$_, name);
eX_11:
  eMSG(_cant_calculate_velocities_);
eX_16:
  eMSG(_cant_calculate_psi_$_, k);
eX_17:
  eMSG(_cant_calculate_div_);
  }

  /*================================================================ ModHeader
  */
#define CAT(a, b)  TxtPrintf(&txt, a, b), TxtCat(&hdr, txt.s);
static char *ModHeader( /* generate header for VRFarr, MODarr   */
  long id,              /* tab identification                   */
  long *pt1,            /* start of validity time               */
  long *pt2 )           /* end of validity time                 */
  {
//  dP(MOD:ModHeader);
  TXTSTR hdr = { NULL, 0 };
  TXTSTR txt = { NULL, 0 };
  char t1s[40], t2s[40], pgm[100];
  int k, gp;
  enum DATA_TYPE dt;
  vLOG(4)("MOD:ModHeader(%08x, ) ...", id);
  sprintf(pgm, "TALMOD_%d.%d.%s", StdVersion, StdRelease, StdPatch);
  dt = XTR_DTYPE(id);
  gp = XTR_GROUP(id);
  switch (dt) {
    case VRFarr:
      CAT("prgm  \"%s\"\n", pgm);
      CAT("artp  \"V%d\"\n", gp);
      if (gp) {
        CAT("form  \"%s\"\n", "Z%4.0fVx%[3]7.2f");
      }
      else { 
        CAT("form  \"%s\"\n", "Dz%3.0fAx%[3]6.2f");
      }
      break;
    case SIGarr:
      CAT("prgm  \"%s\"\n", pgm);
      CAT("artp  \"%s\"\n", "M");
      CAT("form  \"%s\"\n", "Xx%[3]7.2fYx%[3]7.2fZx%[3]7.2f");
      break;
    case MODarr:
      CAT("prgm  \"%s\"\n", pgm);
      CAT("artp  \"M%d\"\n", PrfMode);
      CAT("type  %d\n", PrfMode);
      break;
    default:
      CAT("prgm  \"%s\"\n", pgm);
      CAT("artp  \"%s\"\n", "?");
      return hdr.s;
    }
  TxtCat(&hdr, "sk    ");
  for (k=0; k<=Nz; k++) {
    TxtPrintf(&txt, " %s", TxtForm(Sk[k],6));
    TxtCat(&hdr, txt.s);
  }
  TxtCat(&hdr, " \n");
  CAT("z0    %3.3f\n", PrfZ0);
  CAT("us    %3.3f\n", PrfUs);
  CAT("hm    %1.0f\n", PrfHm);
  CAT("prec  %2.4f\n", PrfNi);                                    //-2011-11-21
  CAT("sg    %7.5f\n", PrfSg);
  CAT("defmode  %d\n", DefMode);
  if (!IsUndef(&PrfKl)) {
    CAT("kl   %1.1f\n", PrfKl);
  }
  else {
    CAT("kl   %s\n", "?");
  }
  if (!IsUndef(&PrfLm)) {
    CAT("lm   %1.1f\n", PrfLm);
  }
  else {
    CAT("lm   %s\n", "?");
  }
  if (Pgp->ntyp > FLAT1D) {
    CAT("xmin  %s\n", TxtForm(Pgp->xmin,12));
    CAT("ymin  %s\n", TxtForm(Pgp->ymin,12));
    CAT("delta %s\n", TxtForm(Pgp->dd,12));
    if (Pgp->ntyp == COMPLEX) {
      CAT("zscl  %1.1f\n", Pgp->zscl);
      CAT("av    %1.3f\n", PrfAv);
    }
  }
  TxtClr(&txt);
  if (dt != MODarr) {
    vLOG(4)("MOD:ModHeader returning 1");
    return hdr.s;
  }
  switch (PrfMode) {
    case 2:
      CAT("form  \"%s\"\n",
        "Z%6.1fVx%[2]7.2fVs%7.2fTau%6.1fTs%7.3fEww%6.2fWx%[3]8.4f"
        "Evv%6.2fPvv%7.3fPww%7.3fLvv%6.2fLww%6.2f");
      break;
    case 3:
      CAT("form  \"%s\"\n",
        "Z%6.1fVx%[2]7.2fVs%7.2fTau%6.1fTs%7.3fEzz%6.2fWx%[3]8.4f"
        "Pxx%[3]7.3fPyx%[3]7.3fPzx%[3]7.3f"
        "Lxx%6.2fLyy%6.2fLzz%6.2fLyx%6.2fLzx%6.2fLzy%6.2f"
        "Exx%6.2fEyx%6.2fEyy%6.2fEzx%6.2fEzy%6.2f");
      break;
    default:
     ;
  }
  strcpy(t1s, TmString(pt1));
  strcpy(t2s, TmString(pt2));
  CAT("t1    \"%s\"\n", t1s);
  CAT("t2    \"%s\"\n", t2s);
  TxtClr(&txt);
  vLOG(4)("MOD:ModHeader returning 2");
  return hdr.s;
}
#undef CAT

  /*================================================================== ModInit
  */
long ModInit(           /* initialize server    */
  long flags,           /* action flags         */
  char *istr )          /* server options       */
  {
  dP(ModInit);
  long id, mask;
  char *jstr, *ps;
  if (StdStatus & STD_INIT)  return 0;
  if (istr) {
    jstr = istr;
    ps = strstr(istr, " -v");
    if (ps) sscanf(ps+3, "%d", &StdLogLevel);
    ps = strstr(istr, " -y");
    if (ps) sscanf(ps+3, "%d", &StdDspLevel);
    }
  else  jstr = "";
  vLOG(3)("MOD_%d.%d.%s (%08lx,%s)", StdVersion, StdRelease, StdPatch, flags, jstr);
  StdStatus |= flags;
  sprintf(istr, " GRD -v%d -d%s", StdLogLevel, AltName);
  GrdInit(0, istr);                                                     eG(11);
  sprintf(istr, " PRF -v%d -d%s", StdLogLevel, AltName);
  PrfInit(0, istr);                                                     eG(12);
  sprintf(istr, " PRM -v%d -d%s", StdLogLevel, AltName);
  PrmInit(0, istr);                                                     eG(13);
  mask = ~(NMS_LEVEL | NMS_GRIDN);
  id = IDENT(MODarr, 0, 0, 0);
  TmnCreator(id, mask, 0, istr, ModServer, ModHeader);                  eG(1);
  StdStatus |= STD_INIT;
  return 0;
eX_1: eX_11: eX_12: eX_13:
  eMSG(_not_initialized_);
  }

  /*================================================================= ModServer
  */
long ModServer(         /* server routine for MOD       */
  char *s )             /* calling option               */
  {
  dP(ModServer);
  long igp, ipa, ima, imb, ipp, iga, t1, t2, usage, valid, mode;
  ARYDSC *pa;
  char name[256], *hdr, t1s[40], t2s[40], m1s[40], m2s[40];
  TXTSTR header = { NULL, 0 };                                    //-2011-06-29
  if (StdArg(s))
    return 0;
  if (*s) {
    switch (s[1]) {
      case 'd': strcpy(AltName, s+2);
                break;
      case 'w': StdStatus |= MOD_WRITE;
                break;
      default:  ;
      }
    return 0;
    }
  if (!StdIdent)                                                eX(60);
  strcpy(name, NmsName(StdIdent));
  GridLevel = XTR_LEVEL(StdIdent);
  GridIndex = XTR_GRIDN(StdIdent);
  ima = IDENT(MODarr, 0, GridLevel, GridIndex);
  if (StdStatus & STD_TIME)  t1 = StdTime;
  else  t1 = TmMin();
  t2 = t1;
  igp = IDENT(GRDpar, 0, GridLevel, GridIndex);
  pa =  TmnAttach(igp, NULL, NULL, 0, NULL);                    eG(21);
  if (!pa)                                                      eX(22);
  Pgp = (GRDPARM*) pa->start;
  Dd = Pgp->dd;
  TmnDetach(igp, NULL, NULL, 0, NULL);                          eG(23);
  iga = IDENT(GRDarr, 0, 0, 0);
  pa =  TmnAttach(iga, NULL, NULL, 0, NULL);                    eG(41);
  if (!pa)                                                      eX(42);
  Sk = (float*) pa->start;
  TmnDetach(iga, NULL, NULL, 0, NULL);                          eG(43);
  ipp = IDENT(PRMpar, 0, 0, 0);
  pa = TmnAttach(ipp, &t1, NULL, 0, NULL);                      eG(31);
  if (!pa)                                                      eX(32);
  Ppi = (PRMINF*) pa->start;
  TmnDetach(ipp, NULL, NULL, 0, NULL);                          eG(33);
  Nx = Pgp->nx;
  Ny = Pgp->ny;
  Nz = Pgp->nz;
  Dd = Pgp->dd;
  PrfMode = Pgp->prfmode;
  DefMode = Pgp->defmode;
  ipa = IDENT(PRFarr, 0, GridLevel, GridIndex);
  Pprf = TmnAttach(ipa, &t1, &t2, 0, &header);                  eG(3);  //-2011-06-29
  if (!Pprf)                                                    eX(4);
  hdr = header.s;
  TmnInfo(ipa, NULL, NULL, &usage, NULL, NULL);         //-2001-06-29
  valid = (0 == (usage & TMN_INVALID));                 //-2001-06-29
  DmnGetFloat(hdr, "Kl|KM",   "%f", &PrfKl, 1);
  DmnGetFloat(hdr, "Us",      "%f", &PrfUs, 1);
  DmnGetFloat(hdr, "Ni|Prec", "%f", &PrfNi, 1);
  DmnGetFloat(hdr, "Lm",      "%f", &PrfLm, 1);
  DmnGetFloat(hdr, "Hm",      "%f", &PrfHm, 1);
  DmnGetFloat(hdr, "Z0",      "%f", &PrfZ0, 1);
  DmnGetFloat(hdr, "SG",      "%f", &PrfSg, 1);  
  if (Pgp->ntyp == COMPLEX) {
    DmnGetFloat(hdr, "Av",   "%f", &PrfAv, 1);
    DmnGetFloat(hdr, "Zscl", "%f", &PrfZs, 1);
    DmnGetInt(hdr, "DefMode", "%d", &PrfDm, 1);
    if (PrfDm != DefMode)                                       eX(24);
    if (PrfZs != Pgp->zscl)                                     eX(25);
  }
  MetT1 = t1;
  MetT2 = t2;
  switch (PrfMode) {
    case 2:   PrfRecLen = sizeof(PR2REC);
              ModRecLen = sizeof(MD2REC);
              break;
    case 3:   PrfRecLen = sizeof(PR3REC);
              ModRecLen = sizeof(MD3REC);
              break;
    default:                                                    eX(5);
    }
  Psld = NULL;
  if (TmnInfo(ima, &t1, &t2, &usage, NULL, NULL)) {
    if (usage & TMN_DEFINED) {
      if (t2 != MetT1)                                          eX(50);
      Pmod = TmnAttach(ima, NULL, NULL, TMN_MODIFY, NULL);      eG(51);
      if (!Pmod)                                                eX(52);
    }
    else  Pmod = NULL;
  }
  switch (Pprf->numdm) {
    case 1:  if (!Pmod) {
               Pmod = TmnCreate(ima, ModRecLen, 1, 0, Nz);              eG(6);
             }
             if (valid)  Clc1dMod();                                    eG(7);
             break;
    case 3:  if (!Pmod) {
               Pmod = TmnCreate(ima, ModRecLen, 3, 0, Nx, 0, Ny, 0, Nz);eG(8);
             }
             if (valid)  Clc3dMod();                                    eG(9);
             break;
    default:                                                            eX(10);
  }
  TmnDetach(ipa, NULL, NULL, 0, NULL);                                  eG(11);
  hdr = ModHeader(ima, &MetT1, &MetT2);                           //-2011-06-29
  TxtCpy(&header, hdr);
  FREE(hdr);
  mode = TMN_MODIFY | ((valid) ? TMN_SETVALID : TMN_INVALID);     //-2001-06-29
  TmnDetach(ima, &MetT1, &MetT2, mode, &header);                        eG(12);
  TxtClr(&header);
  if (StdStatus & MOD_WRITE) {
    imb = IDENT(MODarr, 1, GridLevel, GridIndex);
    TmnArchive(ima, NmsName(imb), 0);                                   eG(13);
  }
  return 0;
eX_60:
  eMSG(_undefined_model_field_);
eX_21: eX_22: eX_23:
  eMSG(_no_grid_parameter_);
eX_24: eX_25:
  eMSG(_inconsistent_parameters_);
eX_31: eX_32: eX_33:
  eMSG(_no_general_parameter_);
eX_41: eX_42: eX_43:
  eMSG(_no_grid_array_);
eX_3: eX_4:
  strcpy(t1s, TmString(&t1));
  strcpy(t2s, TmString(&t2));
  eMSG(_no_profile_$$$_, NmsName(ipa), t1s, t2s);
eX_5:
  eMSG(_invalid_mode_$_, PrfMode);
eX_6: eX_8:
  eMSG(_cant_create_model_field_$_, name);
eX_7: eX_9:
  eMSG(_cant_calculate_model_field_$_, name);
eX_10:
  eMSG(_invalid_dimension_count_$_, Pprf->numdm);
eX_11:
  eMSG(_cant_detach_profile_$_, NmsName(ipa));
eX_12:
  eMSG(_cant_detach_model_$_, name);
eX_13:
  eMSG(_cant_write_model_$_, name);
eX_50:
  strcpy(t1s, TmString(&t1));
  strcpy(t2s, TmString(&t2));
  strcpy(m1s, TmString(&MetT1));
  strcpy(m2s, TmString(&MetT2));
  eMSG(_cant_update_$$_$$_, NmsName(ima), t1s, t2s, m1s, m2s);
eX_51: eX_52:
  eMSG(_cant_attach_$_, NmsName(ima));
  }

//=============================================================================
//
// history:
//
// 2002-09-24 lj 1.0.0  final release candidate
// 2002-12-19 lj 1.0.4  undefined pointer corrected
// 2004-11-26 lj 2.1.7  string length for names = 256
// 2005-03-17 uj 2.2.0  version number upgrade
// 2006-10-26 lj 2.3.0  external strings
// 2011-06-29 uj 2.5.0  DMNA header, Tmn routines with TXTSTR
// 2011-11-21 lj 2.6.0  precipitation
//
//=============================================================================

