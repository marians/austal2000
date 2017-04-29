//=================================================================== TalPpm.c
//
// Handling of particle parameters
// ===============================
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
// last change: 2013-07-08
//
//==========================================================================

#include <math.h>

#define  STDMYMAIN  PpmMain
#include "IBJmsg.h"
#include "IBJary.h"
#include "IBJtxt.h"
#include "IBJdmn.h"
#include "IBJstd.h"
static char *eMODn = "TalPpm";

/*=========================================================================*/

STDPGM(tstppm, PpmServer, 2, 6, 8);

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
#include "TalPpm.h"
#include "TalPpm.nls"

static PRMINF *Ppi;
static GRDPARM *Pgp;
static ARYDSC *Pppm, *Pmod, *Pwdp, *Pvdp, *Pcmp;
static int Nx, Ny, Nc;
static int GridLevel, GridIndex;
static char AltName[120] = "param.def";
static float *Sk, Dd, Prcp;
static long T1, ModT2, CmpT2, PpmT2, VdpT2, WdpT2;

/*============================== internal functions ========================*/

static long ClcPpmArr(  /* calculate particle parameters */
  void )
  ;
static long ClcWdpArr(  /* calculate 2dim deposition velocity */
  void )
  ;
static char *PpmHeader( /* generate header for PPMarr, WDParr   */
  long id,              /* tab identification                   */
  long *pt1,            /* start of validity time               */
  long *pt2 )           /* end of validity time                 */
  ;

  /*================================================================== PpmVsed
  */
float PpmVsed(          /* Berechnung der Sedimentations-Geschwindigkeit */
  float dm,             /* Durchmesser (in mu)          */
  float rho )           /* Dichte (in g/cm3)            */
  {
  float w, logw, re, a, vg;
  static float aa[7] = {
    -0.318657e1,
     0.992696e0,
    -0.153193e-2,
    -0.987059e-3,
    -0.578878e-3,
     0.855176e-4,
    -0.327815e-5 };
  int i;
  w = 5.222e-5*dm*dm*dm*rho;
  if (w < 0.003)  vg = 0;
  else {
    if (w < 0.24)  re = w/24;
    else {
      logw = log(w);
      a = aa[6];
      for (i=5; i>=0; i--)  a = aa[i] + a*logw;
      re = exp(a);
      if (re > 300)  re = 300;
      }
    vg = 14.174*re/dm;
    }
  return vg;
  }

  /*=============================================================== PpmClcWdep
  */
long PpmClcWdep(        /* Berechnung der Depositions-Wahrscheinlichkeit */
  float sw,             /* Sigma-W am Erdboden                           */
  float vdep,           /* Depositions-Geschwindigkeit.                  */
  float vsed,           /* Sedimentations-Geschwindigkeit.               */
  float *pwdep )        /* Pointer auf Depositions-Wahrscheinlichkeit.   */
  {
  float ga, fn, wdep;
  if (sw <= 0.0) {
    *pwdep = 1.0;
    return 0;
    }
  ga = vsed/(SQRT2*sw);
  fn = exp(-ga*ga)/(2 - Erfc(ga));                         /*-19feb93-*/
  wdep = 2*vdep/(vdep + vsed + (SQRT2/SQRTPI)*sw*fn);      /*-19feb93-*/
  *pwdep = wdep;
  return 0;
  }

  /*================================================================ ClcPpmArr
  */
static long ClcPpmArr(  /* calculate particle parameters */
  void )
  {
  dP(ClcPpmArr);
  int n;
  long t1, t2, usage;
  PPMREC *pm;
  CMPREC *pc;
  MODREC *pmd0;
  float wdep, rwsh, prcp, sw;                                     //-2011-11-21
  char t1s[40], t2s[40], m1s[40], m2s[40];
  Pppm = NULL;
  if (TmnInfo(StdIdent, &t1, &t2, &usage, NULL, NULL)) {
    if (usage & TMN_DEFINED) {
      if (t2 != T1)                                             eX(50);
      Pppm = TmnAttach(StdIdent, NULL, NULL, TMN_MODIFY, NULL); eG(51);
      if (!Pppm)                                                eX(52);
      }
    }
  if (!Pppm) {
    Pppm = TmnCreate(StdIdent, sizeof(PPMREC), 1, 0, Nc-1);     eG(1);
    }
  if (Pmod->numdm == 1) {
    pmd0 = AryPtr(Pmod, 0);  if (!pmd0)                            eX(2);
    sw = pmd0->eww;
    }
  else sw = -1;
  prcp = Prcp;
  if (prcp < 0)  prcp = 0;
  for (n=0; n<Nc; n++) {
    pm = AryPtr(Pppm, n);  if (!pm)                                eX(4);
    pc = AryPtr(Pcmp, n);  if (!pc)                                eX(5);
    pm->mmin = pc->gmin /* *pc->mptl */;                        //-2001-07-02
    pm->vdep = pc->vdep;
    pm->vsed = pc->vsed;
    pm->vred = pc->vred;
    pm->hred = pc->hred;
    if (pc->wdep >= 0)  wdep = pc->wdep;
    else if (sw >= 0) {
      PpmClcWdep(sw, pc->vdep, pc->vsed, &wdep);                eG(6);
    }
    else  wdep = -1;
    if (wdep > 1.0) {
      vLOG(2)("Wdep[%d] = %2.2f ! (Sw=%3.3f, Vd=%3.3f, Vs=%3.3f)",
        n, wdep, pmd0->eww, pc->vdep, pc->vsed);
      wdep = 1.0;
    }
    pm->wdep = wdep;
    rwsh = (prcp <= 0) ? 0 : pc->rfak*pow(prcp, pc->rexp);        //-2011-11-21
    pm->rwsh = rwsh;                                              //-2002-07-07
    if (pm->vsed < 0)                                                  eX(10); //-2013-07-08
    if (pm->vdep < 0)                                                  eX(11);
    if (pm->vred < 0)                                                  eX(12);
    if (pm->hred < 0)                                                  eX(13);
    if (pc->rfak < 0)                                                  eX(14);
    if (pc->rexp < 0)                                                  eX(15);
  }
  TmnDetach(StdIdent, &T1, &PpmT2, TMN_MODIFY, NULL);           eG(7);
  return 0;
eX_1:  eX_7:
  eMSG(_cant_create_table_);
eX_2:  eX_4:  eX_5:
  eMSG(_no_data_);
eX_10: eX_11: eX_12: eX_13: eX_14: eX_15:
		eMSG(_negative_parm_);
eX_6:
  eMSG(_cant_calculate_);
eX_50:
  strcpy(t1s, TmString(&t1));
  strcpy(t2s, TmString(&t2));
  strcpy(m1s, TmString(&T1));
  strcpy(m2s, TmString(&PpmT2));
  eMSG(_cant_update_$_$$_$$_, NmsName(StdIdent), t1s, t2s, m1s, m2s);
eX_51: eX_52:
  eMSG(_cant_attach_$_, NmsName(StdIdent));
  }

  /*================================================================ ClcWdpArr
  */
static long ClcWdpArr(  /* calculate 2dim deposition velocity */
  void )
  {
  dP(ClcWdpArr);
  int i, j, n, nxcdd;
  long ivd, iwd, t1, t2, usage;
  CMPREC *pc;
  MODREC *pm000, *pm100, *pm010, *pm110;
  float wdep, vdep, *pw, *pv, sw;
  char name[256], t1s[40], t2s[40], m1s[40], m2s[40];	//-2004-11-26
  char *_hdr;
  TXTSTR header = { NULL, 0 };                                   //-2011-06-29
  WdpT2 = PpmT2;
  //if (Ppi->flags & FLG_2DWDP)                                           eX(20);
  Pvdp = NULL;  
  if (Ppi->flags & FLG_2DVDP) {
    VdpT2 = T1;
    ivd = IDENT(VDParr, 0, GridLevel, GridIndex);
    Pvdp = TmnAttach(ivd, &T1, &VdpT2, 0, &header);                     eG(1);
    if (!Pvdp)                                                          eX(2);
    _hdr = header.s;
    AryAssert(Pvdp, sizeof(float), 3, 1, Nx, 1, Ny, 0, Nc-1);           eG(3);
    DmnDatAssert("Xmin", _hdr, Pgp->xmin);                              eG(31); //-2011-06-29
    DmnDatAssert("Ymin", _hdr, Pgp->ymin);                              eG(32); //-2011-06-29
    DmnDatAssert("Delta|Dd|Delt", _hdr, Pgp->dd);                       eG(33); //-2011-06-29
    if (VdpT2 < WdpT2)  WdpT2 = VdpT2;
    TxtClr(&header);
  }
  iwd = IDENT(WDParr, 0, GridLevel, GridIndex);
  Pwdp = NULL;
  if (TmnInfo(iwd, &t1, &t2, &usage, NULL, NULL)) {
    if (usage & TMN_DEFINED) {
      if (t2 != T1)                                                     eX(50);
      Pwdp = TmnAttach(iwd, NULL, NULL, TMN_MODIFY, NULL);              eG(51);
      if (!Pwdp)                                                        eX(52);
      }
    }
  if (!Pwdp) {
    Pwdp = TmnCreate(iwd, sizeof(float), 3, 1, Nx, 1, Ny, 0, Nc-1);     eG(4);
    }
  nxcdd = 0;
  for (i=1; i<=Nx; i++)
    for (j=1; j<=Ny; j++) {
      if (Pmod->numdm == 3) {
        pm000 = AryPtr(Pmod, i-1, j-1, 0);  if (!pm000)                    eX(11);
        pm100 = AryPtr(Pmod, i  , j-1, 0);  if (!pm100)                    eX(12);
        pm010 = AryPtr(Pmod, i-1, j  , 0);  if (!pm010)                    eX(13);
        pm110 = AryPtr(Pmod, i  , j  , 0);  if (!pm110)                    eX(14);
        sw = 0.25*(pm000->eww + pm100->eww + pm010->eww + pm110->eww);
        }
      else {
        pm000 = AryPtr(Pmod, 0);  if (!pm000)                              eX(15);
        sw = pm000->eww;
        }
      for (n=0; n<Nc; n++) {
        pc = AryPtr(Pcmp, n);  if (!pc)                                    eX(5);
        pw = AryPtr(Pwdp, i, j, n);  if (!pw)                              eX(6);
        if (Pvdp) {
          pv = AryPtr(Pvdp, i, j, n);  if (!pv)                            eX(7);
          vdep = *pv;
          if (vdep < 0)                                                    eX(16); //-2013-07-08
        }
        else vdep = pc->vdep;
        if (pc->wdep >= 0)  wdep = pc->wdep;
        else {
          PpmClcWdep(sw, vdep, pc->vsed, &wdep);                        eG(8);
        }
        if (wdep > 1.0) {
          nxcdd++;
          wdep = 1.0;
        }
        *pw = wdep;
      }
    }
  if (nxcdd > 0)
    vLOG(1)(_$_wdep_, nxcdd);             //-00-04-13
  _hdr = PpmHeader(iwd, &T1, &WdpT2);
  TxtCpy(&header, _hdr);
  FREE(_hdr);
  TmnDetach(iwd, &T1, &WdpT2, TMN_MODIFY, &header);                     eG(9);
  TxtClr(&header);
  if (Pvdp) {
    TmnDetach(ivd, NULL, NULL, 0, NULL);                                eG(10);
  }
  return 0;
//eX_20:
//  eMSG(_no_depo_probabilities_);
eX_1:  eX_2:  eX_10:
  eMSG(_no_depo_velocities_);
eX_3:  eX_31: eX_32: eX_33:
  strcpy(name, NmsName(ivd));
  eMSG(_improper_structure_$_, name);
eX_4:  eX_9:
  eMSG(_cant_create_probabilities_);
eX_5:  eX_6:  eX_7:
  eMSG(_missing_data_$$$_, i, j, n);
eX_8:
  eMSG(_cant_calculate_probability_$$$_, i, j, n);
eX_15:
  i = 0;  j = 0;
eX_11: eX_12: eX_13: eX_14:
  eMSG(_range_error_$$_, i, j);
eX_16:
		eMSG(_negative_vdep_);
eX_50:
  strcpy(t1s, TmString(&t1));
  strcpy(t2s, TmString(&t2));
  strcpy(m1s, TmString(&T1));
  strcpy(m2s, TmString(&WdpT2));
  eMSG(_cant_update_$_$$_$$_, NmsName(iwd), t1s, t2s, m1s, m2s);
eX_51: eX_52:
  eMSG(_cant_attach_$_, NmsName(iwd));
  }

  /*================================================================ PpmHeader
  */
static char *PpmHeader( /* generate header for PPMarr, WDParr   */
  long id,              /* tab identification                   */
  long *pt1,            /* start of validity time               */
  long *pt2 )           /* end of validity time                 */
  {
//  dP(PPM:PpmHeader);
  char t[256], prgm[64], t1s[40], t2s[40];
  int gp;
  enum DATA_TYPE dt;
  TXTSTR hdr = { NULL, 0 };
  TxtCpy(&hdr, "\n");
  dt = XTR_DTYPE(id);
  gp = XTR_GROUP(id);
  sprintf(prgm, "TALPPM_%d.%d.%s", StdVersion, StdRelease, StdPatch);
  switch (dt) {
    case PPMarr:
      sprintf(t, "prgm  \"%s\"\n", prgm);  TxtCat(&hdr, t);
      sprintf(t, "artp  \"%s\"\n", "R");   TxtCat(&hdr, t);
      sprintf(t, "form  \"%s\"\n", "Mmin%10.2eVsed%6.3fRwsh%10.2eWdep%6.3f");
      TxtCat(&hdr, t);
      break;
    case WDParr:
      sprintf(t, "prgm  \"%s\"\n", prgm);              TxtCat(&hdr, t);
      sprintf(t, "artp  \"%s\"\n", "F");               TxtCat(&hdr, t);
      sprintf(t, "form  \"%s\"\n", "Wdep%6.3f");       TxtCat(&hdr, t);
      sprintf(t, "xmin  %s\n", TxtForm(Pgp->xmin,6));  TxtCat(&hdr, t);
      sprintf(t, "ymin  %s\n", TxtForm(Pgp->ymin,6));  TxtCat(&hdr, t);
      sprintf(t, "delta %s\n", TxtForm(Pgp->dd,6));    TxtCat(&hdr, t);
      sprintf(t, "zscl  %s\n", TxtForm(Pgp->zscl,6));  TxtCat(&hdr, t);     
      break;
    default:
      sprintf(t, "prgm  \"%s\"\n", prgm);  TxtCat(&hdr, t);
      sprintf(t, "artp  \"%s\"\n", "?");   TxtCat(&hdr, t);
      return hdr.s;
    }
  strcpy(t1s, TmString(pt1));
  strcpy(t2s, TmString(pt2));
  sprintf(t, "t1    \"%s\"\n", t1s);  TxtCat(&hdr, t);
  sprintf(t, "t2    \"%s\"\n", t2s);  TxtCat(&hdr, t);
  return hdr.s;
  }

  /*================================================================== PpmInit
  */
long PpmInit(           /* initialize server    */
  long flags,           /* action flags         */
  char *istr )          /* server options       */
  {
  dP(PpmInit);
  long id, mask;
  char *jstr, *ps;
  if (StdStatus & STD_INIT)  return 0;
  if (istr) {
    jstr = istr;
    ps = strstr(istr, " -y");
    if (ps) sscanf(ps+3, "%d", &StdDspLevel);
    ps = strstr(istr, " -v");
    if (ps) sscanf(ps+3, "%d", &StdLogLevel);
    }
  else  jstr = "";
  vLOG(3)("PPM_%d.%d.%s (%08lx,%s)", StdVersion, StdRelease, StdPatch, flags, jstr);
  StdStatus |= flags;
  sprintf(istr, " GRD -v%d -d%s", StdLogLevel, AltName);
  GrdInit(0, istr);                                                     eG(11);
  sprintf(istr, " PRF -v%d -d%s", StdLogLevel, AltName);
  PrfInit(0, istr);                                                     eG(12);
  sprintf(istr, " PRM -v%d -d%s", StdLogLevel, AltName);
  PrmInit(0, istr);                                                     eG(13);
  sprintf(istr, " MOD -v%d -d%s", StdLogLevel, AltName);
  ModInit(0, istr);                                                     eG(14);
  mask = ~(NMS_LEVEL | NMS_GRIDN);
  id = IDENT(PPMarr, 0, 0, 0);
  TmnCreator(id, mask, 0, istr, PpmServer, PpmHeader);                  eG(1);
  id = IDENT(WDParr, 0, 0, 0);
  TmnCreator(id, mask, 0, istr, PpmServer, PpmHeader);                  eG(2);
  StdStatus |= STD_INIT;
  return 0;
eX_1:  eX_2:
eX_11: eX_12: eX_13: eX_14:
  eMSG(_not_initialized_);
  }

  /*================================================================= PpmServer
  */
long PpmServer(         /* server routine for PPMarr, WDParr    */
  char *s )             /* calling option                       */
  {
  dP(PpmServer);
  enum DATA_TYPE dt;
  long igp, ima, ipi, iga, ict, t1, t2;
  ARYDSC *pa;
  char name[256], t1s[40], t2s[40];				//-2004-11-26
  TXTSTR hdr = { NULL, 0 };
  if (StdArg(s))
    return 0;
  if (*s) {
    switch (s[1]) {
      case 'd': strcpy(AltName, s+2);
                break;
      default:  ;
    }
    return 0;
  }
  if (!StdIdent)                                                eX(20);
  strcpy(name, NmsName(StdIdent));
  GridLevel = XTR_LEVEL(StdIdent);
  GridIndex = XTR_GRIDN(StdIdent);
  dt = XTR_DTYPE(StdIdent);
  if (StdStatus & STD_TIME)  t1 = StdTime;
  else  t1 = TmMin();
  t2 = t1;
  T1 = t1;
  igp = IDENT(GRDpar, 0, GridLevel, GridIndex);
  pa =  TmnAttach(igp, NULL, NULL, 0, NULL);                    eG(1);
  if (!pa)                                                      eX(2);
  Pgp = (GRDPARM*) pa->start;
  Nx = Pgp->nx;
  Ny = Pgp->ny;
  Dd = Pgp->dd;
  TmnDetach(igp, NULL, NULL, 0, NULL);                          eG(3);
  iga = IDENT(GRDarr, 0, 0, 0);
  pa =  TmnAttach(iga, NULL, NULL, 0, NULL);                    eG(4);
  if (!pa)                                                      eX(5);
  Sk = (float*) pa->start;
  TmnDetach(iga, NULL, NULL, 0, NULL);                          eG(6);
  ipi = IDENT(PRMpar, 0, 0, 0);
  pa = TmnAttach(ipi, &t1, &t2, 0, NULL);                       eG(7);
  if (!pa)                                                      eX(8);
  Ppi = (PRMINF*) pa->start;
  PpmT2 = t2;
  if (PpmT2 <= T1)                                              eX(41);
  Nc = Ppi->sumcmp;
  TmnDetach(ipi, NULL, NULL, 0, NULL);                          eG(9);
  ModT2 = t1;
  ima = IDENT(MODarr, 0, GridLevel, GridIndex);
  Pmod = TmnAttach(ima, &t1, &ModT2, 0, &hdr);                  eG(10);
  if (!Pmod)                                                    eX(11);
  if (ModT2 < PpmT2)  PpmT2 = ModT2;
  if (PpmT2 <= T1)                                              eX(42);
  Prcp = -1;
  DmnGetFloat(hdr.s, "NI|PREC", "%f", &Prcp, 1);                eG(24);
  CmpT2 = t1;
  ict = IDENT(CMPtab, 0, 0, 0);
  Pcmp = TmnAttach(ict, &t1, &CmpT2, 0, NULL);                  eG(12);
  if (!Pcmp)                                                    eX(13);
  if (CmpT2 < PpmT2)  PpmT2 = CmpT2;
  if (PpmT2 <= T1)                                              eX(43);
  switch (dt) {
    case PPMarr: ClcPpmArr();                                   eG(14);
                 break;
    case WDParr: ClcWdpArr();                                   eG(15);
                 break;
    default:                                                    eX(16);
    }
  TmnDetach(ima, NULL, NULL, 0, NULL);                          eG(25);
  TmnDetach(ict, NULL, NULL, 0, NULL);                          eG(26);
  TxtClr(&hdr);
  return 0;
eX_20:
  eMSG(_no_data_specified_);
eX_1:  eX_2:  eX_3:
  eMSG(_no_grid_data_);
eX_4:  eX_5:  eX_6:
  eMSG(_no_vertical_grid_);
eX_7:  eX_8:  eX_9:
  eMSG(_no_parameters_);
eX_10: eX_11:
  strcpy(name, NmsName(ima));
  eMSG(_no_model_$_, name);
eX_12: eX_13:
  strcpy(name, NmsName(ict));
  eMSG(_no_components_$_, name);
eX_14:
  eMSG(_cant_calculate_parameters_);
eX_15:
  eMSG(_cant_calculate_2d_);
eX_16:
  strcpy(name, NmsName(StdIdent));
  eMSG(_improper_request_$_, name);
eX_24: eX_25: eX_26:
  eMSG(_no_boundary_);
eX_41: eX_42: eX_43:
  strcpy(t1s, TmString(&T1));
  strcpy(t2s, TmString(&PpmT2));
  eMSG(_invalid_time_$$_, t1s, t2s);
  }

//==========================================================================
//
// history:
//
// 2002-09-24 lj        final release candidate
// 2004-11-26 lj        string length for names = 256
// 2005-03-17 uj 2.2.0  version number upgrade
// 2006-10-26 lj 2.3.0  external strings
// 2011-06-29 uj 2.5.0  DMNA header
// 2011-11-21 lj 2.6.0  wet deposition
// 2013-07-08 uj 2.6.8  check for negative substance parameters
//
//==========================================================================

