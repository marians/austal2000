// ================================================================ TalBlm.c
//
// Meteorologisches Grenzschichtmodell
// ===================================
//
// Copyright (C) Umweltbundesamt, Dessau-Roﬂlau, Germany, 2002-2005
// Copyright (C) Janicke Consulting, 88662 ‹berlingen, Germany, 2002-2012
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
// last change: 2012-11-05 uj
//
//==========================================================================

#include <math.h>

#define  STDMYMAIN  BlmMain
static char *eMODn = "TalBlm";
#include "IBJmsg.h"
#include "IBJary.h"
#include "IBJtxt.h"
#include "IBJstd.h"    

//============================================================================

STDPGM(tstblm, BlmServer, 2, 6, 5);

//============================================================================

#include "genutl.h"
#include "genio.h"

#include "TalTmn.h"
#include "TalNms.h"
#include "TalGrd.h"
#include "TalIo1.h"
#include "TalBlm.h"
#include "TalBlm.nls"

#ifndef PI
  #define PI       3.1415926
#endif

#define  TP_VAL  0x0001
#define  TS_VAL  0x0002
#define  SP_VAL  0x0004
#define  ST_VAL  0x0008
#define  SW_VAL  0x0010

BLMPARM *BlmPprm;
ARYDSC *BlmParr;
VARTAB *BlmPvpp;

static int   nz0TAL = 9;
static float z0TAL[9] = { 0.01, 0.02, 0.05, 0.10, 0.20, 0.50, 1.0, 1.5, 2.0 };

static float lmTAL[6][9] =
          { {      7,     9,   13,   17,   24,   40,   65,   90,  118 },
            {     25,    31,   44,   60,   83,  139,  223,  310,  406 },
            {  99999, 99999,99999,99999,99999,99999,99999,99999,99999 },
            {    -25,   -32,  -45,  -60,  -81, -130, -196, -260, -326 },
            {    -10,   -13,  -19,  -25,  -34,  -55,  -83, -110, -137 },
            {    -4,    -5,   -7,  -10,  -14,  -22,  -34,  -45,  -56  } };

static float TLhMax = 1200,
             TLvMax = 1200,
             TlMin  = 0;

static float Hm10 =  -1,
             Hm20 =  -1,
             Hm31 =  -1,
             Hm32 =  800, /*  konsistent mit VDI 3782/3  */
             Hm40 = 1100, /*  konsistent mit VDI 3782/3  */
             Hm50 = 1100; /*  konsistent mit VDI 3782/3  */

static char DefName[256] = "param.def";
static char InpName[256] = "meteo.def";                           //-2011-11-23
static long MetT1, MetT2, ZtrT1, ZtrT2, PosZtr;
static int PrfMode;
static GRDPARM *Pgp;
static ARYDSC  *Pga;
static int Gl, Gi;
static float vKk=0.4, rvKk=2.5, TuMax, TvMax, TwMax;
static float Suf=-999, Svf=-999, Swf=-999;                       //-2011-07-07
static int PrfMod = 0;                                           //-2011-07-07

/*==========================================================================*/

static int TalRe( float h, float z0, float d0, float ust, float lmo, float hm, float *pre );

static int TalUu( float h, float z0, float d0, float ust, float lmo, float hm, float *puu );

static int TalSw( float h, float z0, float d0, float ust, float lmo, float hm, float *psw );

static int TalTw( float h, float z0, float d0, float ust, float lmo, float hm, float *ptw );

static int TalSv( float h, float z0, float d0, float ust, float lmo, float hm, float *psv );

static int TalTv( float h, float z0, float d0, float ust, float lmo, float hm, float *ptv );

static int TalSu( float h, float z0, float d0, float ust, float lmo, float hm, float *psu );

static int TalTu( float h, float z0, float d0, float ust, float lmo, float hm, float *ptu );

static int TalRa( float h, float z0, float d0, float ha, float da, float lmo, float hm, float *pra );


static int is_equal(float x, float y) {
  float xa, ya, q, eps;
  if (x == 0 && y == 0)
    return 1;
  eps = 1.e-5;
  xa = (x < 0) ? -x : x;
  ya = (y < 0) ? -y : y;
  q = (x-y)/(xa+ya);
  if (q < 0) q = -q;
  if (q < eps)
    return 1;
  return 0;
}

//===================================================================== TalRe
//
static int TalRe( float h, float z0, float d0, float ust, float lmo, float hm,
float *pre )
{
  float a, z, rwsus3, f1, f2;
  z = (h < d0+6*z0) ? 6*z0 : h-d0;
  if (hm <= 0)  hm = 800;
  a = z/hm;
  f1 = 2.5*vKk*a;
  if (a < 1)  f1 += (1-a)*(1-a);
  if (lmo < 0) {
    rwsus3 = -hm/(vKk*lmo);
    f2 = 1.5 - 1.3*pow(a, 1./3.);
    if (f2 > 0)  f1 += rwsus3*a*vKk*f2;
    if (f1 < 1.) f1 = 1.;
  }
  else {
    f1 = 1. + 4*z/lmo;
  }
  *pre = vKk*z/(ust*ust*ust*f1);
  return 0;
}

/*====================================================================== TalUu
*/
static int TalUu( float h, float z0, float d0, float ust, float lmo, float hm,
float *puu )
  {
  float z, zm, zm0, psi, psi0, zeta, u1, hh;
  if (z0 <= 0) {
    *puu = 1;
    return 0;
  }
  hh = d0 + 6*z0;
  z = (h < hh) ? hh-d0 : h-d0;
  z -= z0;
  if (lmo > 0.0) {          // Windprofil fuer stabile Schichtung
    zm = (z+z0)/lmo;
    zm0 = z0/lmo;
    if      (zm < 0.5) psi = log(1+z/z0) + 5*(zm-zm0);
    else if (zm < 10)  psi = 8*log(2*zm)+4.25/zm-1/(2*zm*zm)-log(2*zm0)-5*zm0 - 4;
    else               psi = 0.7585*zm+8*log(20)-11.165-log(2*zm0)-5*zm0;
    *puu = rvKk*ust*psi;
  }
  else {                    // Windprofil fuer konvektive Schichtung
    zeta = (z+z0)/lmo;
    psi  = pow(1.-15.*zeta, 0.25);
    psi0 = pow(1.-15.*z0/lmo, 0.25);
    u1 = log(((psi-1.)*(psi0+1.))/((psi+1.)*(psi0-1.)));
    *puu = rvKk*ust*(2.*(atan(psi)-atan(psi0))+u1);
  }
  if (h < hh)  *puu *= h/hh;
  return 0;
}

/*====================================================================== TalSw
*/
static int TalSw( float h, float z0, float d0, float ust, float lmo, float hm,
float *psw )
  {
  float z, a, sw, rwsus3, b, c3, corr;
  z = (h < d0+6*z0) ? 6*z0 : h-d0;
  corr = (PrfMod == 1) ? 0.3 : 1.;
  a = (hm > 0) ? z/hm : 0;
  b = 1 - 0.8*a;
  c3 = (b > 0) ? pow(b, 3) : 0;
  rwsus3 = (lmo < 0) ? -hm/(vKk*lmo) : 0;
  sw = pow(exp(-3*corr*a) + a*c3*rwsus3, 0.333333);
  *psw = 1.3*ust*sw;
  if (!is_equal(Swf, 1.3))  *psw *= (Swf/1.3);                    //-2011-07-07
  return 0;
}

//====================================================================== TalTw
//
static int TalTw( float h, float z0, float d0, float ust, float lmo, float hm,
float *ptw )
  {
  float tw, sw, re;
  TalRe(h, z0, d0, ust, lmo, hm, &re);
  TalSw(h, z0, d0, ust, lmo, hm, &sw);
  tw = 2*sw*sw*re/5.7;
  *ptw = tw;
  return 0;
}

//====================================================================== TalSv
//
static int TalSv( float h, float z0, float d0, float ust, float lmo, float hm,
float *psv )
  {
  float z, a, sv, rwsus3, corr;
  z = (h < d0+6*z0) ? 6*z0 : h-d0;
  corr = (PrfMod == 1) ? 0.3 : 1.;
  a = (hm > 0) ? z/hm : 0;
  rwsus3 = (lmo < 0) ? -hm/(vKk*lmo) : 0;
  sv = exp(-corr*a)*pow(1. + 0.03522*rwsus3, 0.333333);
  *psv = 1.8*ust*sv;
  if (!is_equal(Svf, 1.8))  *psv *= Svf/1.8;                      //-2011-07-07
  return 0;
}

//====================================================================== TalTv
//
static int TalTv( float h, float z0, float d0, float ust, float lmo, float hm,
float *ptv )
{
  float tv, sv, re;
  TalRe(h, z0, d0, ust, lmo, hm, &re);
  TalSv(h, z0, d0, ust, lmo, hm, &sv);
  tv = 2*sv*sv*re/5.7;
  *ptv = tv;
  return 0;
}

//====================================================================== TalSu
//
static int TalSu( float h, float z0, float d0, float ust, float lmo, float hm,
float *psu )
  {
  float z, a, su, rwsus3, corr;
  z = (h < d0+6*z0) ? 6*z0 : h-d0;
  corr = (PrfMod == 1) ? 0.3 : 1.;
  a = (hm > 0) ? z/hm : 0;
  rwsus3 = (lmo < 0) ? -hm/(vKk*lmo) : 0;
  su = exp(-corr*a)*pow(1. + 0.01486*rwsus3, 0.333333);
  *psu = 2.4*ust*su; 
  if (!is_equal(Suf, 2.4))  *psu *= Suf/2.4;                      //-2011-07-07
  return 0;
}

//====================================================================== TalTu
//
static int TalTu( float h, float z0, float d0, float ust, float lmo, float hm,
float *ptu )
{
  float tu, su, re;
  TalRe(h, z0, d0, ust, lmo, hm, &re);
  TalSu(h, z0, d0, ust, lmo, hm, &su);
  tu = 2*su*su*re/5.7;
  *ptu = tu;
  return 0;
}

/*====================================================================== TalRa
*/
static int TalRa( float h, float z0, float d0, float ha, float da, float lmo, float hm,
float *pra )
{
  float zp, za, dmax, dh, hol;
  dmax = 45;
  if (lmo == 0.) { *pra = da; return -1; }
  hol = hm/lmo;
  if (hol < -10.)    dh = 0;
  else if (hol < 0.) dh = dmax*(1 + 0.1*hol);
  else               dh = dmax;
  if (h > hm) h = hm;
  zp = (h  < d0+6*z0) ? 6*z0 : h-d0;
  za = (ha < d0+6*z0) ? 6*z0 : ha-d0;
  *pra = da + 1.23*dh*(exp(-1.75*za/hm) - exp(-1.75*zp/hm));
  return 0;
}


//================================================================= BlmProfile
//
long BlmProfile(      /* Berechnung des Grenzschicht-Profils.               */
BLMPARM *p,           /* Eingabe-Daten (Grenzschicht-Parameter).            */
BLMREC *v )           /* Ausgabe-Daten (Wind-Varianzen und Korr.-Zeiten).   */
  {
  dP(BlmProfile);
  int vers, vers1, vers2, k;
  float z, z0, d0, lmo, hm, ha, ua, da, sua, sva, swa;
  float ust, u, a, d;
  float fcor;
  float xu, zs, zu, zg;
  float usg, su, sv, sw;
  float hp, hma, hmr;
  float tu, tv, tw, ftv;
  int no_shear=0;
  int spread_plume=0;
  float fvmin = 0.035;
  float fxmin = 10;
  float tvmin;
  vLOG(5)("BLM:BlmProfile(...)");
  if ((!p) || (!v))                                             eX(1);
  z    = v->z;          /* absolute z */
  zg   = p->AnmZpos;
  hp   = z - zg;
  if (hp < 0)                                                   eX(2);
  z0   = p->RghLen;
  d0   = p->ZplDsp;
  vers1= p->MetVers;
  PrfMod = 0;
  if (vers1 != 26 && vers1 != 28 &&               // default with/without shear 
      vers1 != 36 && vers1 != 38 &&               // option SPREAD with/without shear 
      vers1 != 46 && vers1 != 48 &&               // option PRFMOD with/without shear 
      vers1 !=  5 && vers1 !=  7 && vers1 !=  1)  // test versions for verification               
                                                                   eX(6);
  if (vers1 == 36 || vers1 == 38) {
    spread_plume = 1;
    vers1 -= 10;
  }
  else if (vers1 == 46 || vers1 == 48) {                          //-2011-07-07
    PrfMod = 1;
    vers1 -= 20;
  }
  if (vers1 == 28) {
    no_shear = 1;
    vers1 = 26;
  }
  vers2 = vers1 % 10;  vers1 = vers1 / 10;
  vers = 16*vers1 | vers2;
  lmo  = p->lmo;  if (lmo == 0)                                 eX(4);  //-2002-04-19
  hma  = p->hm;    /* absolute: above z=0    */
  hmr  = hma - zg; /* relative: above ground */
  hm   = (lmo < 0) ? hmr : hma;
  ust  = p->Ustar;
  usg  = p->Ustar;
  if (ust < 0) { ust = p->UstCalc;  usg = p->UsgCalc; }
  ha  = p->AnmHeight;
  ua  = p->WndSpeed;
  da  = p->WndDir;
  sua = p->SigmaU;
  sva = p->SigmaV;
  swa = p->SigmaW;
  TuMax = TLhMax;
  TvMax = TLhMax;
  TwMax = TLvMax;
  fcor = 1.00e-4;
  ftv = 1;
  Suf = 2.4;
  Svf = (PrfMod == 1) ? 2.0 : 1.8;
  Swf = 1.3;
  if (p->SvFac > 0)  
    Svf = p->SvFac;                                              //-2007-01-30
  if (vers1 < 1)  
   goto test_model;

  // ------------------- Standard-Modell -------------------------------
  
  k = (int)(0.001 + p->cl + (p->cl > 3.11)) - 1;            //-2004-10-25
  if (k>=0 && k<6 && p->Ftv[k]>0)  ftv = p->Ftv[k];
  //
  if (usg < 0.0) {
    TalUu(ha, z0, d0, 1, lmo, hm, &u);
    usg =  ua/u;
    if (usg <= 0)                                               eX(5); //-2006-02-07
  }
  if (ust < 0.0) {
    if (swa > 0.0)  ust = swa/1.3;
    if (ust < 0.0)  ust = usg;
  }
  if (hma <= 0) {
    if (lmo > 0) {
      hm = 0.3*ust/fcor;
      a = sqrt(fcor*lmo/ust);
      if (a < 1)  hm *= a;
      if (hm > 800)  hm = 800;                                  //-2002-03-16
      hmr = (hma = hm) - zg;
    }
    else  hma = (hm = hmr = 800.) + zg;
    p->hm = hma;
  }
  if (lmo<0 && ha>hmr)                                           eX(3);
  p->UsgCalc = usg;
  p->UstCalc = ust;
  p->us = ust;

  // Windgeschwindigkeit
  TalUu(hp, z0, d0, usg, lmo, hm, &u);
  v->u = u;

  // Windrichtung
  if (no_shear)  d = da;
  else TalRa(hp, z0, d0, ha, da, lmo, hm, &d);
  v->d = d;
  
  if (hp > d0+2*hm)  hp = d0+2*hm;                     //-2002-05-13

  TalSw(hp, z0, d0, ust, lmo, hm, &sw);
  v->sw = sw;

  TalSv(hp, z0, d0, ust, lmo, hm, &sv);
  if (spread_plume && sv < fvmin * u) sv = fvmin * u;  //-2006-02-13
  v->sv = sv;

  TalSu(hp, z0, d0, ust, lmo, hm, &su);
  v->su = su;
  v->suw = 0;
  TlMin = z0/ust;                                      //-2001-06-05
  TalTu(hp, z0, d0, ust, lmo, hm, &tu);
  TalTv(hp, z0, d0, ust, lmo, hm, &tv);
  tv *= ftv;
  if (spread_plume) {                                  //-2006-02-13
    tvmin = fvmin*fvmin*fxmin*hp*u/(sv*sv);
    if (tv < tvmin) tv = tvmin;
  }
  TalTw(hp, z0, d0, ust, lmo, hm, &tw);
  if (tu < TlMin)  tu = TlMin;
  if (tv < TlMin)  tv = TlMin;
  if (tw < TlMin)  tw = TlMin;
  if (tu > TuMax)  tu = TuMax;
  if (tv > TvMax)  tv = TvMax;
  if (tw > TwMax)  tw = TwMax;
  v->tu = tu;
  v->tv = tv;
  v->tw = tw;
  v->ths = p->ThetaGrad;
/*
printf("A: ha=%14.7e, ua=%14.7e, da=%14.7e\n", ha, ua, da);
printf("A: us=%14.7e, lm=%14.7e, hm=%14.7e\n", ust, lmo, hm);
printf("A: hp=%14.7e, u =%14.7e, d =%14.7e\n", hp, u, d);
printf("A: su=%14.7e, sv=%14.7e, sw=%14.7e\n", su, sv, sw);
printf("A: tu=%14.7e, tv=%14.7e, tw=%14.7e\n", tu, tv, tw);
*/
  return 0;

test_model:  //------------------ vers1 < 1 -----------------------

  if (ha <= 0)  ha = 10;
  if (da < 0)   da = 270;
  if (ust < 0) {
    ust = z0*ua/ha;
    p->UstCalc = ust;
    p->UsgCalc = ust;
  }
  TlMin = z0/ust;
  v->u = ua;
  v->d = da;

  if (vers2 == 5) {
    xu = 0.3;
    k = 0.001 + 10.0*p->Class;
    if (hp == 0.0) {
      zu = 0.0;
      zs = 1.e-12;
    }
    else {
      zu = pow(hp/ha, xu);
      zs = hp/ha;
    }
    v->su = 1.e-6;
    v->sv = 1.e-6;
    v->sw = swa*sqrt(zs);
    v->u  = ua*zu;
    v->tu = TlMin;
    v->tv = TlMin;
    v->tw = TlMin;
    return 0;
  }
  if (vers2 == 7) {
    if (hm <= 0)  hm = 200;
    v->su = sua;
    v->sv = sva;
    v->sw = swa*(1 - (z0/ha)*sin(0.5*PI*hp/hm));
    v->tu = 20*TlMin;
    v->tv = 20*TlMin;
    v->tw = TlMin*(1 + 20*sin(0.5*PI*hp/hm));
    return 0;
    }
  if (vers2 == 1) {
    v->su = sua;
    v->sv = sva;
    v->sw = swa;
    v->tu = 100*TlMin;
    v->tv = 100*TlMin;
    if (lmo > 9000)  v->tw = 10*TlMin;
    else             v->tw = TlMin*(1.0 + hp/ABS(lmo));
    v->suw = 0;
    return 0;
    }

eX_1:
  eMSG(_undefined_arguments_);
eX_2:
  eMSG(_negative_height_);
eX_3:
  eMSG(_anemometer_too_high_);
eX_4:
  eMSG(_undefined_stability_);
eX_5:
  eMSG(_no_ua_);
eX_6:
  eMSG(_unknown_model_$_, vers1/10.);
}


/*================================================================ KTA1508T7i2
*/
static float KTA1508T7i2(   /* Tmp.-Gradient nach KTA1508 9/88 Tab. 7-2  */
float u10,                  /* Windgeschwindigkeit [m/s] in 10 m H√∂he    */
int kl )                    /* Stabilit√§tsklasse                         */
  {
  int i, i10, k1, k2;
  float dt100;
  static float tsg[9][5] = {
    { -1.13, -1.03, -0.91, -0.37, 0.78 },
    { -1.18, -1.05, -0.91, -0.22, 1.12 },
    { -1.39, -1.18, -0.97, -0.16, 1.25 },
    { -1.61, -1.33, -1.00, -0.10, 1.32 },
    { -1.82, -1.48, -1.04, -0.04, 1.39 },
    {  NOTV, -1.62, -1.08,  0.02, 1.46 },
    {  NOTV, -1.77, -1.16,  0.08, NOTV },
    {  NOTV,  NOTV, -1.25,  NOTV, NOTV },
    {  NOTV,  NOTV, -1.40,  NOTV, NOTV } };
  if (u10 <0.0 || kl<=0)  return -1.0;
  i10 = ((int)(10*u10+0.5))/10;
  if (i10 >= 9)  i10 = 8;
  k2 = (kl < 6) ? kl-1 : 4;
  k1 = (kl > 1) ? kl-2 : 0;
  for (i=i10; i>0; i--)
    if (tsg[i][k1]>NOTV || tsg[i][k2]>NOTV)  break;
  if (tsg[i][k1] <= NOTV)  dt100 =  tsg[i][k2] - 0.05;
  else  if (tsg[i][k2] <= NOTV)  dt100 = tsg[i][k1] + 0.05;
        else {
          dt100 = 0.5*(tsg[i][k1] + tsg[i][k2]);
          if (k1 == 4)  dt100 += 0.05;
          if (k2 == 0)  dt100 -= 0.05;
          }
  return dt100; 
}

//=============================================================== BlmStability
//
long BlmStability(    // Berechnung von Mischungsschichth√∂he und Stab.-Klasse
BLMPARM *p )          // Parameter der Grenzschicht.
  {
  dP(BlmStability);
  float lmo, hm, f1, f2, z0, d0, u10, ha, ua, z1, z2;
  int cl, kl=0, k, kk;
  vLOG(4)("BLM:BlmStability(...)");
  if (!p)                                         eX(1);
  lmo = p->MonObLen;
  hm  = p->MixDpt;
  z0  = p->RghLen;
  d0  = p->ZplDsp;
  ha  = p->AnmHeight;
  ua  = p->WndSpeed;
  cl  = 0.001+10*p->Class;
  if (lmo==0 && cl<=0) {     // missing data    -2002-04-19
    p->cl  = 3.1;
    p->lmo = 99999;
    p->hm  = 800;
    p->kta = 4;
    p->ths = 0;
    return 0;
  }
  if (lmo == 0.0) {          // Monin-Obukhov-Laenge nicht bekannt
    if (cl!=50 && cl!=40 && cl!=32 && cl!=31 && cl!=20 && cl!=10)
      cl = 31;
    kk = (cl + 10*(cl>31))/10;
    if (z0 <= z0TAL[0]) lmo = lmTAL[kk-1][0];
    else {
      for (k=1; k<nz0TAL; k++)
        if (z0>z0TAL[k-1] && z0<=z0TAL[k]) break;
      if (k > nz0TAL-1) lmo = lmTAL[kk-1][nz0TAL-1];
      if (lmo == 0.0) {
        z1 = log(z0TAL[k-1]);
        z2 = log(z0TAL[k]);
        f1 = 1./lmTAL[kk-1][k-1];
        f2 = 1./lmTAL[kk-1][k];
        lmo = 1./(f1 + (log(z0)-z1)/(z2-z1)*(f2-f1));
      }
    }
  }
  if (cl <= 0) {            // Stabilitaetsklasse nicht bekannt
    if (z0 <= z0TAL[0]) k = 0;
    else if (z0 > z0TAL[nz0TAL-1]) k = nz0TAL-1;
    else
      for (k=1; k<nz0TAL; k++)
        if (z0>z0TAL[k-1] && z0<=z0TAL[k]) break;
    if (lmo == 0) cl = 31;
    else {
      f1 = 1./lmo;
      for (kk=5; kk>0; kk--) {
        f2 = (lmTAL[kk][k]+lmTAL[kk-1][k])/(2*lmTAL[kk][k]*lmTAL[kk-1][k]);
        if (f1 < f2) break;
      }
      switch (kk) {
        case 0:  cl = 10;  break;
        case 1:  cl = 20;  break;
        case 2:  cl = 31;  break;
        case 3:  cl = 32;  break;
        case 4:  cl = 40;  break;
        case 5:  cl = 50;  break;
        default: cl = 31;
      }
    }
  }
  if (kl <= 0)  kl = 7 - (cl + 10*(cl>31))/10;

  if (hm<=0.0 && lmo<0) {               // Mischungsschichthoehe nicht bekannt
    switch ( cl ) {
      case 10:  hm = Hm10;  break;
      case 20:  hm = Hm20;  break;
      case 31:  hm = Hm31;  break;
      case 32:  hm = Hm32;  break;
      case 40:  hm = Hm40;  break;
      case 50:  hm = Hm50;  break;
      default:  hm = Hm31;
      }
    }
  u10 = ua*log(10/z0)/log((ha-d0)/z0);
  p->lmo = lmo;
  p->cl  = 0.1*cl;
  p->kta = kl;
  p->hm  = hm;
  if (p->PtmpGrad > NOTV) {
    p->ThetaGrad = p->PtmpGrad;
  }
  else {
    p->ThetaGrad = 0.01*(1.0 + KTA1508T7i2(u10, kl));
  }
  p->U10 = u10;
  return 0;
eX_1:
  eMSG(_undefined_parameters_);
  }

//==================================================================== BlmRead
//
#define GETDATA(n,f,a)  {rc=GetData(#n,buf,#f,a); if(rc<0){strcpy(name,#n); eX(99);}}

static long BlmRead(   /* File WETTER.DEF einlesen und Daten uebernehmen. */
char *altname )
  {
  dP(BlmRead);
  long rc, id, pos;
  float v, za, vv[6];
  char name[40], *buf;
  ARYDSC *pa;
  vLOG(4)("BLM:BlmRead(%s)", altname);
  if ((!Pgp) || (!Pga))                                                 eX(1);
  buf = ALLOC(GENHDRLEN);  if (!buf)                                    eX(21);
  id = IDENT(BLMpar, 0, 0, 0);
  pa = TmnCreate(id, sizeof(BLMPARM), 1, 0, 0);                         eG(2);
  BlmPprm = (BLMPARM*) pa->start;
  TmnDetach(id, NULL, NULL, TMN_MODIFY|TMN_FIXED, NULL);                eG(13);
  PrfMode = Pgp->prfmode;
  vLOG(3)(_reading_$_, InpName);
  OpenInput(InpName , altname );                                        eG(3);
  rc = GetLine('.', buf, GENHDRLEN);  if (rc <= 0)                      eX(4);
  pos = CloseInput();
  vLOG(3)(_$_closed_, InpName);
  BlmPprm->MetVers = BLMDEFVERS;
  GETDATA(Version, %f, &v);
  if (rc > 0) BlmPprm->MetVers = 0.001 + 10.0*v;
  BlmPprm->ZplDsp = 0.0;
  GETDATA(D0, %f, &BlmPprm->ZplDsp);
  BlmPprm->AnmXpos = 0.0;
  GETDATA(XA, %f, &BlmPprm->AnmXpos);
  BlmPprm->AnmYpos = 0.0;
  GETDATA(YA, %f, &BlmPprm->AnmYpos);
  rc = GrdBottom(BlmPprm->AnmXpos, BlmPprm->AnmYpos, &za);
  if (rc < 0)                                                           eX(9);
  BlmPprm->AnmZpos = za;
  BlmPprm->AnmGridNumber = rc;
  strcpy(name, "HmMean");
  rc = GetData("HmMean", buf, "%[6]f", vv);  if (rc < 0)                eX(99);
  if (rc == 6) {
    Hm10 = vv[0];
    Hm20 = vv[1];
    Hm31 = vv[2];
    Hm32 = vv[3];
    Hm40 = vv[4];
    Hm50 = vv[5];
    }
  else if (rc > 0)                                                      eX(99);
  //-2004-10-25
  rc = GetData("FTV", buf, "%[6]f", BlmPprm->Ftv);  if (rc < 0)         eX(99);
  //                                       
  GETDATA(HA, %f, &BlmPprm->AnmHeight);
  DefParm( "HW",  buf, "%f", &BlmPprm->AnmHeightW, "-999", &BlmPvpp ); //-2012-11-05
  DefParm( "UA",  buf, "%f", &BlmPprm->WndSpeed, "-999", &BlmPvpp );
  DefParm( "RA",  buf, "%f", &BlmPprm->WndDir,   "-999", &BlmPvpp );
  DefParm( "Us",  buf, "%f", &BlmPprm->Ustar,    "-999", &BlmPvpp );
  DefParm( "LM",  buf, "%f", &BlmPprm->MonObLen,  "0.0", &BlmPvpp );
  DefParm( "Z0",  buf, "%f", &BlmPprm->RghLen,   BLMSZ0, &BlmPvpp );
  DefParm( "D0",  buf, "%f", &BlmPprm->ZplDsp,    "0.0", &BlmPvpp );
  DefParm( "MS",  buf, "%f", &BlmPprm->RezMol,   "-999", &BlmPvpp );
  DefParm( "HM",  buf, "%f", &BlmPprm->MixDpt,    "0.0", &BlmPvpp );
  DefParm( "KL",  buf, "%f", &BlmPprm->Class,     "0.0", &BlmPvpp );
  DefParm("PREC", buf, "%f", &BlmPprm->Precep,   "-1.0", &BlmPvpp ); //-2011-11-21
  DefParm( "SG",  buf, "%f", &BlmPprm->StatWeight,"1.0", &BlmPvpp );
  DefParm( "SU",  buf, "%f", &BlmPprm->SigmaU,   "-999", &BlmPvpp );
  DefParm( "SV",  buf, "%f", &BlmPprm->SigmaV,   "-999", &BlmPvpp );
  DefParm( "SW",  buf, "%f", &BlmPprm->SigmaW,   "-999", &BlmPvpp );
  DefParm( "TS",  buf, "%f", &BlmPprm->PtmpGrad, "-999", &BlmPvpp ); 
  DefParm( "SVF", buf, "%f", &BlmPprm->SvFac,    "-999", &BlmPvpp );  
  GETDATA(WindLib,  %s, BlmPprm->WindLib);
  MsgCheckPath(BlmPprm->WindLib);
  DefParm("WIND", buf, "%d", &BlmPprm->Wind, "-999", &BlmPvpp);
  DefParm("INIT", buf, "%d", &BlmPprm->Wini, "-999", &BlmPvpp);
  DefParm("DIFF", buf, "%d", &BlmPprm->Diff, "-999", &BlmPvpp);
  DefParm("TURB", buf, "%d", &BlmPprm->Turb, "-999", &BlmPvpp);         eG(11);
  strcpy(buf, "@");
  if (BlmPprm->WindLib[0] == '~') {
    strcat(buf, StdPath);
    strcat(buf, "/");                                 //-2002-04-28
    strcat(buf, BlmPprm->WindLib+1);
  }
  else strcat(buf, BlmPprm->WindLib);
  MsgCheckPath(buf);                                  //-2002-04-28    
  strcat(buf, "/");                                   //-2002-04-28
  vLOG(5)("wind library in %s", buf);
  NmsSeq(buf, 0);
  FREE(buf);
  buf = NULL;
  return pos;
eX_99:
  eMSG(_error_definition_$_, name);
eX_1:
  eMSG(_no_grid_);
eX_2:  eX_13:
  eMSG(_cant_create_blmpar_);
eX_3:
  eMSG(_cant_open_$_, InpName);
eX_4:
  eMSG(_invalid_structure_$_, InpName);
eX_9:
  eMSG(_anemometer_$$_outside_, BlmPprm->AnmXpos, BlmPprm->AnmYpos);
eX_11:
  eMSG(_syntax_error_);
eX_21:
  eMSG(_cant_allocate_);
  }

#undef  GETDATA

//================================================================= BlmSrfFac
//
float BlmSrfFac(        /* factor to impose surface layer */
float z0,               /* roughness length               */
float d0,               /* zero plane displacement        */
float h )               /* height above ground            */
  {
  float uh, z, u, hh;
  if (z0<=0 || d0<0)    return 1;
  if (h<0 || h>200)    return 1;
  hh = d0 + 6*z0;
  z = 200-d0;                                                 //-2002-04-02
  uh = log(z/z0);
  z = (h < hh) ? hh-d0 : h-d0;
  u = log(z/z0)/uh;
  if (h < hh)  u *= h/hh;
  return u;
  }

//=================================================================== Clc1dPrf
//
static long Clc1dPrf(   // Grenzschicht-Profil berechnen.
long t1, long t2 )
  {
  dP(Clc1dPrf);
  int k, nk, old_gl, old_gi, anm_gn;
  BLMREC *pm;
  float s, z0, d0, d, h, g, xa, ya, za;
  TXTSTR hdr = { NULL, 0 };                                       //-2011-06-29
  TXTSTR t = { NULL, 0 };
  long id;
  vLOG(4)("BLM:Clc1dPrf(...)");
  if ((!BlmPprm) || (!Pgp) || (!Pga))                                 eX(1);
  nk = Pga->bound[0].hgh;
  id = IDENT(BLMarr, 0, 0, 0);
  BlmParr = TmnCreate(id, sizeof(BLMREC), 1, 0, nk);                  eG(6);
  TxtCpy(&hdr, "\n");
  TxtPrintf(&t, "prgm  \"TALBLM_%d.%d.%s\"\n", StdVersion, StdRelease, StdPatch);
  TxtCat(&hdr, t.s);
  TxtCat(&hdr, "form  \"Z%6.1fU%6.2fG%6.2fD%5.0fSu%[3]5.2fTu%[3]5.0fSuw%5.2fThs%7.3f\"\n");
  old_gl = Pgp->level;
  old_gi = Pgp->index;
  anm_gn = BlmPprm->AnmGridNumber;
  xa = BlmPprm->AnmXpos;
  ya = BlmPprm->AnmYpos;
  za = BlmPprm->AnmZpos;
  if (BlmPprm->MonObLen==0 && BlmPprm->RezMol!=-999 && BlmPprm->RezMol!=0)
    BlmPprm->MonObLen = 1/BlmPprm->RezMol;
  if (BlmPprm->Class<=0 && BlmPprm->MonObLen==0) {                  //-2001-06-29
    TmnDetach(id, &t1, &t2, TMN_MODIFY|TMN_INVALID, &hdr);          eG(4);
    TxtClr(&hdr);
    TxtClr(&t);
    return 0;
  }
  BlmStability(BlmPprm);
  BlmPprm->UsgCalc = -999;
  BlmPprm->UstCalc = -999;
  z0 = BlmPprm->RghLen;
  d0 = BlmPprm->ZplDsp;
  GrdSetNet(anm_gn);                                                eG(7);
  for (k=0; k<=nk; k++) {
    s = *(float*) AryPtrX(Pga, k);
    h = GrdHh(xa, ya, s);
    pm = AryPtr(BlmParr, k);  if (!pm)                              eX(3);
    pm->z = h + za;
    }
  GrdSet(old_gl, old_gi);                                           eG(8);
  for (k=0; k<=nk; k++) {
    pm = AryPtr(BlmParr, k);  if (!pm)                              eX(3);
    h = pm->z - za;
    BlmProfile(BlmPprm, pm);                                        eG(5);
    if (k > 0)  pm->g = pm->u/BlmSrfFac(z0, d0, h);
    if (k == 1) {
      g = pm->g;
      d = pm->d;
      pm = AryPtrX(BlmParr, 0);
      pm->g = g;
      pm->d = d;
      }
    }
  TmnDetach(id, &t1, &t2, TMN_MODIFY|TMN_SETVALID, &hdr);           eG(4);
  TxtClr(&hdr);
  TxtClr(&t);
  return 0;
eX_1:
  eMSG(_undefined_grid_);
eX_3:  eX_4:  eX_6:
  eMSG(_cant_create_profile_);
eX_5:
  eMSG(_cant_calculate_profile_);
eX_7:  eX_8:
  eMSG(_cant_switch_grids_);
  }

//==================================================================== BlmInit
//
long BlmInit(           /* initialize server    */
long flags,             /* action flags         */
char *istr )            /* server options       */
  {
  dP(BlmInit);
  long id, mask;
  char *jstr, *ps;
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
    }
  else  jstr = "";
  vLOG(3)("BLM_%d.%d.%s (%08lx,%s)", StdVersion, StdRelease, StdPatch, flags, jstr);
  StdStatus |= flags;
  mask = ~(NMS_LEVEL | NMS_GRIDN);
  id = IDENT(BLMarr, 0, 0, 0);
  TmnCreator(id, mask, 0        , istr, BlmServer, NULL);       eG(1);
  id = IDENT(BLMpar, 0, 0, 0);
  TmnCreator(id, mask, TMN_FIXED, istr, BlmServer, NULL);       eG(2);
  StdStatus |= STD_INIT;
  return 0;
eX_1:  eX_2:
  eMSG(_not_initialized_);
  }

/*=================================================================== BlmServer
*/
long BlmServer(
char *ss )
  {
  dP(BlmServer);
  long mask, rc, idpar, idarr, igrda, igrdp;
  int read_ztr, read_parm;
  ARYDSC *pa;
  enum DATA_TYPE dt;
  if (StdArg(ss))  return 0;
  if (*ss) {
    switch (ss[1]) {
      case 'd':  strcpy(DefName, ss+2);
                 break;
      default:   ;
      }
    return 0;
    }
  dt = XTR_DTYPE(StdIdent);
  Gl = XTR_LEVEL(StdIdent);
  Gi = XTR_GRIDN(StdIdent);
  igrdp = IDENT(GRDpar, 0, Gl, Gi);
  pa = TmnAttach(igrdp, NULL, NULL, 0, NULL);  if (!pa)         eX(10); //-2014-06-26
  Pgp = (GRDPARM*) pa->start;
  igrda = IDENT(GRDarr, 0, 0, 0);
  Pga = TmnAttach(igrda, NULL, NULL, 0, NULL);  if (!Pga)       eX(11); //-2014-06-26
  idarr = IDENT(BLMarr, 0, 0, 0);

  if (dt == BLMarr) {
    TmnDelete(TmMax(), idarr, 0);                               eG(24);
    }
  if (!BlmPprm) {
    MetT1 = TmMin();
    MetT2 = MetT1;
    ZtrT1 = MetT1;
    ZtrT2 = MetT1;
    }
  if (StdStatus & STD_TIME)  MetT1 = StdTime;
  read_parm = (MetT1 >= MetT2);
  rc = 1;
  if (read_parm) {
    MetT2 = TmMax();
    mask = -1;
    idpar = IDENT( BLMpar, 0, 0, 0 );
    TmnCreator(idpar, mask, TMN_FIXED, NULL, NULL, NULL);       eG(1);
    if (!BlmPprm) {
      PosZtr = BlmRead(DefName);                                eG(3);
      if (!BlmPprm)                                             eX(4);
      if (!BlmPvpp)  ZtrT2 = TmMax();
      }
    read_ztr = (BlmPvpp) && (ZtrT2 <= MetT1);
    ZtrT2 = MetT1;
    if (read_ztr) {
      rc = ReadZtr(InpName, DefName, &MetT1, &ZtrT2,
                  StdDspLevel, &PosZtr, BlmPvpp);
      if (rc < 0)                                                       eX(6);
      if (MetT2 > ZtrT2)  MetT2 = ZtrT2;
    }
    if (!rc) {
      MetT2 = MetT1;
      vLOG(3)(_no_data_after_$_, TmString(&MetT1));
    }
    TmnAttach(idpar,   NULL,   NULL, TMN_MODIFY, NULL);                 eG(20);
    TmnDetach(idpar, &MetT1, &MetT2, TMN_MODIFY, NULL);                 eG(21);
    TmnCreator(idpar, mask, TMN_FIXED, NULL, BlmServer, NULL);          eG(9);
    }

  dt = XTR_DTYPE(StdIdent);
  if (dt == BLMarr) {
    mask = ~(NMS_LEVEL | NMS_GRIDN);
    if (rc) {
      Clc1dPrf(MetT1, MetT2);                                           eG(8);
      }
    else {
      TmnCreate(idarr, 0, 0);                                           eG(7);
      TmnDetach(idarr, &MetT1, &MetT2, TMN_MODIFY, NULL);               eG(7);
      }
    }
  TmnDetach(igrdp, NULL, NULL, 0, NULL);                                eG(10);
  TmnDetach(igrda, NULL, NULL, 0, NULL);                                eG(11);
  return 0;
eX_1:  eX_9:
  eMSG(_cant_redefine_);
eX_3:  eX_4:
  eMSG(_cant_get_parameters_);
eX_6:
  eMSG(_error_reading_$_, InpName);
eX_7:
  eMSG(_cant_create_null_);
eX_8:
  eMSG(_error_model_);
eX_10: eX_11:
  eMSG(_no_grid_);
eX_20: eX_21:
  eMSG(_cant_set_times_);
eX_24:
  eMSG(_cant_delete_$_, NmsName(idarr));
  }

#ifdef MAIN  /*############################################################*/

#include <signal.h>

static void MyExit( void ) {
  if (MsgCode < 0) {
    vMsg("@%s error exit", StdMyProg);
    TmnList(MsgFile);
  }
  else vMsg("@%s finished", StdMyProg);
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
  MsgSetQuiet(0);
  vMsg("@%s aborted", StdMyProg);
  if (sig_number != 4) {
    prn = (MsgFile) ? MsgFile : stderr;
    fflush(prn);
    fprintf(prn, "\nABORT, SIGNAL %x\n", sig_number);
  }
  if (MsgFile) {
    fprintf(MsgFile, "\n\n");
    fclose(MsgFile);
  }
  MsgFile = NULL;
  MsgSetQuiet(1);
  exit(0);
  }

static long BlmMain(void)
  {
  dP(BlmMain);
  ARYDSC *pa;
  GRDPARM *pp;
  long id, m, t1, t2;
  char istr[80];
  TXTSTR hdr = { NULL, 0 };
  MsgSetVerbose(1);
  MsgSetQuiet(0);
  MsgBreak = '\'';
  if ((StdStatus&STD_ANYARG) == 0) {
    printf("usage: TSTblm <path>\n");
    exit(0);
  }
  atexit(MyExit);
  signal(SIGSEGV, MyAbort);
  signal(SIGINT,  MyAbort);
  signal(SIGTERM, MyAbort);
  signal(SIGABRT, MyAbort);
  if (StdStatus & STD_TIME)  t1 = StdTime;
  else  t1 = TmMin();
  t2 = t1;
  m = TMN_DISPLAY + TMN_LOGALLOC + TMN_LOGACTION + TMN_DONTZERO;
  TmnInit( StdPath, NmsName, NULL, m, MsgFile );                        eG(1);
  sprintf(istr, " lstgrd -v%d -y%d -d%s", StdLogLevel, StdDspLevel, DefName);
  GrdInit(0, istr);                                                     eG(2);
  sprintf(istr, " lstblm -v%d -y%d -d%s", StdLogLevel, StdDspLevel, DefName);
  BlmInit(0, istr);                                                     eG(3);
  id = IDENT(GRDpar, 0, 0, 0);
  pa = TmnAttach(id, NULL, NULL, 0, &hdr );                             eG(4);
  if (!pa)                                              eX(11);
  pp = (GRDPARM*) pa->start;
  TmnDetach(id, NULL, NULL, 0, NULL);
  id = IDENT(BLMarr, 0, 0, 0);
  TmnAttach(id, &t1, &t2, 0, &hdr );                    eG(5);
  TmnDetach(id, NULL, NULL, 0, NULL);                   eG(6);
  TmnArchive(id, "blmtest.dmna", 0);                    eG(20);
  TmnList(MsgFile);                                     eG(9);
  t1 = t2;
  TmnAttach(id, &t1, &t2, 0, &hdr );                    eG(7);
  TmnDetach(id, NULL, NULL, 0, NULL);                   eG(8);
  TmnArchive(id, "blmtest.dmna", 1);                    eG(21);
  TmnList(MsgFile);                                     eG(9);
  TxtClr(&hdr);
  return 0;
eX_1:  eX_2:  eX_3:  eX_4:  eX_5:  eX_6:  eX_7:  eX_8:  eX_9:
eX_11:
eX_20: eX_21:
  TmnList(stdout);
  eMSG("error!");
  }
#endif  /*##################################################################*/

//  history:
//
// 2002-09-24 lj 1.0.0  final release candidate
// 2004-10-25 lj 2.0.4  optional factors Ftv  
// 2005-03-17 uj 2.2.0  version number upgrade
// 2006-02-07 uj 2.2.7  error if ua<=0 for determination of u*
// 2006-02-13 uj 2.2.8  new svmin, tvmin (blm versions 3.6, 3.8)
//                      error if unknown blm version encountered
// 2006-10-26 lj 2.3.0  external strings
// 2007-01-30 uj 2.3.4  Svf
// 2011-06-29 uj 2.5.0  DMN header
// 2011-07-07 uj        profile versions 4.6 and 4.8 (A2K option PRFMOD)
// 2011-11-21 lj 2.6.0  precipitation
// 2012-11-05 uj 2.6.5  anemometer height hw for base field superposition
// 2014-06-26 uj 2.6.11 eG/eX adjusted
//
//============================================================================

