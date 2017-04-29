//================================================================== TalPlm.c
//
// Calculation of plume rise
// =========================
//
// Copyright (C) Umweltbundesamt, Dessau-Roﬂlau, Germany, 2002-2004
// Copyright (C) Janicke Consulting, 88662 ‹berlingen, Germany, 2002-2004
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
// last change: 2004-12-22 uj
//
//==========================================================================

#include <stdio.h>
#include <math.h>
#include "TalPlm.h"
#include "TalVsp.h"

#define MAXVSP 200

typedef struct vsprec {
float tm, hq, uq, cl, vq, dq, rhy, lwc, dh, xe; } VSPREC;

char *PlmMsg;
FILE *PlmLog;                                             //-2003-03-06

int TalPlm( 	//  plume rise
int type, 		//  model type
float *pdh, 	//  calculated rise height [m]
float *pxe,		//  calculated rise distance [m]
float qq,		  //  heat emission [MW]
float hq,		  //  source height [m]
float uq,		  //  wind speed at source height [m/s]
float class,	//  stability class [Klug/Manier] 
float vq,		  //  gas velocity [m/s]
float dq, 		//  stack diameter [m]
float rhy,    //  relative humidity [%]
float lwc,    //  liquid water content [kg/kg]
float tmp )   //  gas temperature [Celsius]                       //-2002-12-10
  {
  int kl;
  static float HmLabil = 1100, HmNeutral = 800;
  double u, q, h, v, d, r, l, t, logq, dhl, dhn, dhs, dhk=0, dhf=0, xe, dh;
  double R=0;   // volume flux (standard conditions)
  u = (uq < 1.0) ? 1.0 : uq;		//-2004-12-22
  q = (qq < 0) ? 0 : qq;
  h = (hq < 0) ? 0 : hq;
  v = (vq < 0) ? 0 : vq;
  d = (dq < 0) ? 0 : dq;
  t = (tmp < 10) ? 10 : tmp;                                      //-2002-12-10
  r = rhy = (rhy < 0) ? 0 : rhy;
  l = lwc = (lwc < 0) ? 0 : lwc;
  R = 0.25*3.1415926*d*d*v*273.15/(273.15+t);                     //-2003-03-28
  if (r>0 || l>0)  type = PLM_VDI3784_2;
  //
  //============================== VDI 3784/2 =================================
  //
  if (type == PLM_VDI3784_2) {                                    //-2002-09-23
    static VSPREC vsplist[MAXVSP];
    static int Nvsp=0, filled=0;
    int i, i0, i1, part;
    float du, hh, xh, f;
    volatile float tm;                                            //-2003-06-16
    VSPREC *pv;
    if (R>0 && q>0) {                                             //-2004-11-30
      f = 1 - q/(1.36e-3*R*(273.15+t));
      tm = (f != 0) ? (10 + 273.15)/f - 273.15 : 10;
      if (tm < 10) tm = 10;
    }
    else 
      tm = t;
    for (part=0; part<=filled; part++) {
      if (part == 0) {
        i1 = Nvsp-1;
        i0 = 0;
      }
      else {
        i1 = MAXVSP-1;
        i0 = Nvsp;
      }
      for (i=i1; i>=i0; i--) {
        pv = vsplist + i;
        du = 2*(pv->uq - uq)/(pv->uq + uq);
        if (du>0.1 || du<-0.1)
          continue;     // uq within 10 %
        if (tm==pv->tm && hq==pv->hq && class==pv->cl && vq==pv->vq 
          && dq==pv->dq && rhy==pv->rhy && lwc==pv->lwc)
          break;
      }
      if (i >= i0)
        break;
    }
    if (i >= i0) {
      dh = pv->dh;
      xe = pv->xe;
    }
    else {
      PlmMsg = VspCallVDISP("vdisp", dq, hq, vq, tm, rhy, lwc, uq, class, &hh, &xh);
      if (PlmMsg)  // error not to be ignored
        return -1;
      dh = 2*hh;
      xe = xh/(0.4*log(2));
      if (PlmLog)  fprintf(PlmLog,                                          //-2003-03-06
        "VDISP(%5.1f, %3.0f, %4.1f, %5.1f, %3.0f, %6.4f, %4.1f, %3.1f) = (%4.0lf, %4.0lf)\n",
        dq, hq, vq, tm, rhy, lwc, uq, class, dh, xe);
      pv = vsplist + Nvsp;
      pv->tm = tm;
      pv->uq = uq;
      pv->hq = hq;
      pv->cl = class;
      pv->vq = vq;
      pv->dq = dq;
      pv->rhy = rhy;
      pv->lwc = lwc;
      pv->dh = dh;
      pv->xe = xe;
      Nvsp++;
      if (Nvsp >= MAXVSP) {
        Nvsp = 0;
        filled = 1;
      }
    }
    *pdh = dh;
    *pxe = xe;
    return 0;
  }
  //
  //============================== VDI 3782/3 =================================
  //
  kl = class*10 + 0.001;
  if (q<=0 && tmp>10)  q = (tmp-10)*1.36e-3*R;              //-2003-03-28
  //
  if (q > 1.4) {  /***************** Warme Quellen *********************/
    logq = log(q);
    if (kl >= 40) {
      if (h >= HmLabil) {
        *pdh = 0;
        *pxe = 0;
        return 0;
      }
      if (q > 6.0)  dhl = 146.0*exp(0.6*logq)/u;
      else  dhl = 112.0*exp(0.75*logq)/u;
      if ((h + dhl) > HmLabil)  dhl = HmLabil - h;
      xe = exp((log(dhl*u/3.34)-0.333*logq)/0.667);
      dh = dhl;
    }
    else {
      if (h >= HmNeutral) {
        *pdh = 0;
        *pxe = 0;
        return 0;
      }
      if (q > 6.0)  dhn = 102.0*exp(0.6*logq)/u;
      else  dhn = 78.4*exp(0.75*logq)/u;
      if ((h + dhn) > HmNeutral)  dhn = HmNeutral - h;
      if (kl > 20) {
        xe = exp((log(dhn*u/2.84)-0.333*logq)/0.667);
        dh = dhn; 
      }
      else {
        dhs = exp(0.333*(logq-log(u)))*( (kl == 10) ? 74.4 : 85.2 );
        dh = (dhs > dhn) ? dhn : dhs;
        xe = exp((log(dh*u/3.34)-0.333*logq)/0.667);
      }
    }
    *pdh = dh;  *pxe = xe;  
    return 0;
  }
  /***************************** Kalte Quellen **************************/
  if (q > 0) { 
    dhk = 84.0*sqrt(q);  
    xe = 209.8*exp(0.522*log(q)); 
  }
  else { 
    dhk = 0;  
    xe = 0; 
  }
  dhk = (dhk + 0.35*v*d)/u;
  dhf = 3.0*v*d/u;
  dh = (dhf > dhk) ? dhf : dhk;
  *pdh = dh;  *pxe = xe;
  return 0;
}

//======================================================================
//
// history:
//
// 2002-09-24 lj  final release candidate
// 2002-12-10 lj  temperature as optional parameter
// 2003-03-06 lj  optional printing of calculated plume rise by vdisp
// 2003-03-28 lj  volume flux (standard conditions) used for heat emission
// 2003-06-16 lj  parameter "tm" as volatile (for correct comparison)
// 2004-11-30 uj  derivation of tm from q and R corrected
// 2004-12-22 uj  minimum uq changed form 0.5 to 1.0 m/s
//
//======================================================================

