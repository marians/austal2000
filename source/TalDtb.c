//================================================================== TalDtb.c
//
// Calculation of concentration statistics
// =======================================
//
// Copyright (C) Umweltbundesamt, Dessau-Roﬂlau, Germany, 2002-2007
// Copyright (C) Janicke Consulting, 88662 ‹berlingen, Germany, 2002-2011
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
// last change: 2011-06-29 uj
//
//==========================================================================

#include <math.h>

#define  STDMYMAIN  DtbMain
#include "IBJmsg.h"
#include "IBJary.h"
#include "IBJtxt.h"
#include "IBJdmn.h"
#include "IBJstd.h"
static char *eMODn = "TalDtb";

/*=========================================================================*/

STDPGM(tstdtb, DtbServer, 2, 6, 0);

/*=========================================================================*/

#include "genutl.h"
//#include "genio.h"

#include "TalNms.h"
#include "TalTmn.h"
#include "TalGrd.h"
#include "TalInp.h"
#include "TalPrm.h"
#include "TalMod.h"
#include "TalDos.h"
#include "TalDtb.h"
#include "TalZet.h"
#include "IBJary.h"
#include "IBJdmn.h"
#include "IBJtxt.h"
#include "TalBlm.h"
#include "TalWrk.h"
#include "TalDtb.nls"

static char DefName[256];                                       //-2011-06-29
static long T1;
static int PartTotal, PartNew, PartDrop;                        //-2001-06-29

/*================================================================ DtbDos2Con
*/
static long DtbDos2Con(        /* calculate concentration      */
long idos,              /* ident of dose field          */
long icon,              /* ident of concentration field */
int k1, int k2,         /* subrange                     */
float valid )           /* valid time interval fraction */
  {
  dP(DtbDos2Con);
  int i, j, k, l, gp, gl, gi, nx, ny, nz, nc, sz, scatter, ng;
  long t1, t2, igrd, usage, ivol, invalid;
  char t1s[40], t2s[40], name[256];    //-2004-11-26
  float area, f, *sk, dd, scale, *pc, *pd, rgf, dos, dsq, dev;
  float zd, *pv;
  ARYDSC *pdos, *pcon, *pgrd, *pvol;
  GRDPARM *pgp;
  strcpy(name, NmsName(idos));
  vLOG(4)("converting dose %s to concentration", name);
  gp = XTR_GROUP(idos);
  gl = XTR_LEVEL(idos);
  gi = XTR_GRIDN(idos);
  igrd = IDENT(GRDpar, 0, gl, gi);
  pgrd = TmnAttach(igrd, NULL, NULL, 0, NULL);  if (!pgrd)              eX(1);
  pgp = pgrd->start;
  dd = pgp->dd;
  zd = pgp->zscl;                     //-98-08-22
  sk = GrdParr->start;
  if (zd > 0) {                       //-98-08-22
    ivol = IDENT(GRDarr, GRD_IVM, gl, gi);
    pvol = TmnAttach(ivol, NULL, NULL, 0, NULL);  if (!pvol)            eX(30);
    }
  else  pvol = NULL;
  TmnInfo(idos, &t1, &t2, &usage, NULL, NULL);
  if (!(usage & TMN_DEFINED))                                           eX(2);
  //invalid = usage & TMN_INVALID;
  invalid = (valid <= 0);                                         //-2011-07-04
  if (T1 > TmMin() && (t2<=T1 || t1>T1))                                eX(20);
  strcpy(t1s, TmString(&t1));
  strcpy(t2s, TmString(&t2));
  pdos = TmnAttach(idos, &t1, &t2, 0, NULL);  if (!pdos)                eX(3);
  nx = pdos->bound[0].hgh;
  ny = pdos->bound[1].hgh;
  nz = pdos->bound[2].hgh;
  nc = pdos->bound[3].hgh + 1;
  sz = pdos->elmsz;
  if (icon) {
    if (k1 < -1)  k1 = -1;
    if (k2>nz || k2<k1)  k2 = nz;
    /* sz = sizeof(float);                    -05feb96-*/
    }
  else {
    icon = IDENT(CONtab, gp, gl, gi);
    k1 = -1;
    k2 = nz;
    }
  TmnInfo(icon, NULL, NULL, &usage, NULL, NULL);
  if (usage & TMN_DEFINED) {
    pcon = TmnAttach(icon, NULL, NULL, TMN_MODIFY, NULL); if (!pcon)    eX(5);
    AryAssert(pcon, sz, 4, 1, nx, 1, ny, k1, k2, 0, nc-1);              eG(6);
    }
  else {
    pcon = TmnCreate(icon, sz, 4, 1, nx, 1, ny, k1, k2, 0, nc-1);       eG(7);
    }
  scatter = (sz == 2*sizeof(float));
  ng = MI.numgrp;
  rgf = (ng > 1) ? 1.0/(ng-1.0) : 1.0;
  area = dd*dd*(t2 - t1)*valid;                                   //-2011-07-04
  for (i=1; i<=nx; i++)
    for (j=1; j<=ny; j++) {
      scale = 1;                                        //-98-08-22
      for (k=k1; k<=k2; k++) {
        if (k <= 0)  f = area;
        else {
          if (pvol) {
            pv = AryPtr(pvol, i, j, k);  if (!pv)       eX(31);
            f = (t2 - t1)*(*pv)*valid;                            //-2011-07-04
            }
          else  f = area*(sk[k] - sk[k-1])*scale;
          }
        pd = AryPtr(pdos, i, j, k, 0);  if (!pd)        eX(10);
        pc = AryPtr(pcon, i, j, k, 0);  if (!pc)        eX(11);
        for (l=0; l<nc; l++) {
          if (scatter) {
            dos = *pd++;
            if (dos <= 0.0) {
              *pc++ = 0.0;
              *pc++ = 0.0;
              pd++;                             /*-05feb96-*/
              }
            else {
              dsq = *pd++;
              *pc++ = dos/f;
              dev = (ng*dsq/(dos*dos)-1.0)*rgf;
              dev = (dsq == 0) ? 1.0 : (ng*dsq/(dos*dos)-1.0)*rgf; //-2011-07-01
              *pc++ = (dev < 0) ? 0.0 : sqrt(dev);
              }
            }
          else  *pc++ = (*pd++)/f;
          }  /* for l */
        }
      }
  TmnDetach(igrd, NULL, NULL, 0, NULL);                 eG(13);
  TmnDetach(idos, NULL, NULL, 0, NULL);                 eG(14);
  if (!invalid)  invalid = TMN_SETVALID;                        //-2001-06-29
  TmnDetach(icon, &t1, &t2, TMN_MODIFY|invalid, NULL);  eG(15);
  if (pvol) {
    TmnDetach(ivol, NULL, NULL, 0, NULL);               eG(32);
  }
  return 0;
eX_1:
  eMSG(_cant_get_grid_$_, NmsName(igrd));
eX_2:
  eMSG(_dose_$_undefined_, name);
eX_20:
  eMSG(_dose_$$$_not_suited_$_, name, t1s, t2s, TmString(&T1));
eX_3:
  eMSG(_cant_get_dose_$$$_, name, t1s, t2s);
eX_5:
  eMSG(_cant_get_concentration_$_, NmsName(icon));
eX_6:
  eMSG(_improper_structure_$_, NmsName(icon));
eX_7:
  eMSG(_cant_create_$_, NmsName(icon));
eX_10: eX_11: eX_31:
  eMSG(_index_error_$$$_, i, j, k);
eX_13: eX_14: eX_15:
  eMSG(_cant_detach_);
eX_30: eX_32:
  eMSG(_cant_get_grid_$_, NmsName(ivol));
  }

//-------------------------------------------------------------------- DtbMntSet
long DtbMntSet(long t1, int nc)
{
  dP(DtbMntSet);
  int i, l, sz;
  long imon, imnt, ict, scatter;
  ARYDSC *pmnt, *pmon, *pct;
  MONREC *pm, *pn;
  CMPREC *pcmp;
  if (!(MI.flags & FLG_MNT) || NumMon < 1) return 0;
  scatter  = (MI.flags & FLG_SCAT) && (MI.numgrp > 4);           //-2008-07-30
  sz = sizeof(MONREC);
  imnt = IDENT(MNTarr,0,0,0);
  imon = IDENT(MONarr,0,0,0);
  pmnt = TmnCreate(imnt, sz, 2, 1, NumMon, 0, nc-1);                 eG(1);
  pmon = TmnAttach(imon, NULL, NULL, 0, NULL);                       eG(2);
  ict = IDENT(CMPtab, 0, 0, 0);
  pct = TmnAttach(ict, NULL, NULL, 0, NULL);  if (!pct)              eX(3);
  pcmp = pct->start;
  for (i=1; i<=NumMon; i++) {
    pm = AryPtr(pmon, i);  if (!pm)                                  eX(4);
    for (l=0; l<nc; l++) {
      pn = AryPtr(pmnt, i, l); if (!pn)                              eX(5);
      AryCreate(&pn->adc, sizeof(float), 1, 0, MNT_NUMVAL);          eG(6);
      if (scatter) {
        AryCreate(&pn->ads, sizeof(float), 1, 0, MNT_NUMVAL);        eG(7);
      }
      sprintf(pn->np, "%s", pm->np);
      pn->i = pm->i;   pn->j = pm->j;   pn->k = pm->k;
      pn->xp = pm->xp; pn->yp = pm->yp; pn->zp = pm->zp;
      pn->nn = pm->nn; pn->nl = pm->nl; pn->ni = pm->ni;
      pn->ic = l;
      pn->t1 = t1;
      pn->t2 = t1;
      pn->index = 0;
    }
  }
  TmnDetach(imon, NULL, NULL, 0, NULL);                              eG(8);
  TmnDetach(imnt, NULL, NULL, TMN_MODIFY, NULL);                     eG(9);
  TmnDetach(ict, NULL, NULL, 0, NULL);                   eG(10); //-2008-07-30
  return 0;
eX_1: eX_2: eX_3: eX_4: eX_5: eX_6: eX_7: eX_8: eX_9: eX_10:
  eMSG(_cant_attach_detach_);
}

//------------------------------------------------------------------ DtbMntRead
long DtbMntRead( void )
{
  dP(DtbMntRead);
  int i, n, l, nc, sz, m, scatter;                //-2008-07-30
  char *_sp, name[512], names[512];                //-2008-07-30
  float *fpi, *fpo;
  long imnt, ict, imon, t1, t2;
  ARYDSC *pmnt, *pmon, dsc, dscs, *pct;              //-2008-07-30
  MONREC *pn, *pm;
  CMPREC *pcmp;
  TXTSTR usrhdr = { NULL, 0 };
  TXTSTR syshdr = { NULL, 0 };
  if (!(MI.flags & FLG_MNT) || NumMon < 1) return 0;
  scatter  = (MI.flags & FLG_SCAT) && (MI.numgrp > 4);            //-2008-07-30
  memset(&dsc, 0, sizeof(dsc));                                   //-2002-10-28
  memset(&dscs, 0, sizeof(dscs));                 //-2008-07-30
  ict = IDENT(CMPtab,0,0,0);
  imnt = IDENT(MNTarr,0,0,0);
  imon = IDENT(MONarr,0,0,0);
  sz = sizeof(MONREC);
  if (NumMon > MNT_MAX-1)                                           eX(30); //-2002-10-28
  pct = TmnAttach(ict, NULL, NULL, 0, NULL);  if (!pct)             eX(1);
  pcmp = pct->start;
  nc = pct->bound[0].hgh-pct->bound[0].low+1;
  pmnt = TmnCreate(imnt, sz, 2, 1, NumMon, 0, nc-1);                eG(2);
  pmon = TmnAttach(imon, NULL, NULL, 0, NULL);  if (!pmon)          eX(3); //-2008-07-30
  for (m=1; m<=NumMon; m++) {
    sprintf(name, "%s/mntc%04d", StdPath, m);           //-2008-07-30
    if (0>DmnRead(name, &usrhdr, &syshdr, &dsc))                    eX(11);
    if (dsc.numdm != 2)                                             eX(21);
    n = dsc.bound[0].hgh-dsc.bound[0].low+1;
    if (dsc.bound[1].low != 0 || dsc.bound[1].hgh!=nc-1)            eX(21);
    if (scatter) {                        //-2008-07-30
      sprintf(names, "%smnts%04d", StdPath, m);
      if (0 > DmnRead(names, &usrhdr, &syshdr, &dscs))           eX(12);
      if (dscs.numdm != 2)                                           eX(13);
      if (n != (dscs.bound[0].hgh-dscs.bound[0].low+1))              eX(14);
      if (dscs.bound[1].low != 0 || dscs.bound[1].hgh!=nc-1)         eX(15);
      if (dscs.elmsz != sizeof(float))                               eX(16);
    }
    if (1 != DmnGetString(usrhdr.s, "t1", &_sp, 1))                 eX(22);
    t1 = TmValue(_sp);
    FREE(_sp);                                                    //-2006-02-15
    if (1 != DmnGetString(usrhdr.s, "t2", &_sp, 1))                 eX(23);
    t2 = TmValue(_sp);
    FREE(_sp);                                                    //-2006-02-15
    if (n != (int)((t2-t1)/MI.cycle) || t2 != MI.dost1)             eX(24);
    if (dsc.elmsz != sizeof(float))                                 eX(25);
    pm = AryPtr(pmon, m); if (!pm)                                  eX(5);
    DmnRplValues(&usrhdr, "t1", NULL);
    DmnRplValues(&usrhdr, "t2", NULL);
    for (l=0; l<nc; l++) {
      pn = AryPtr(pmnt, m, l); if (!pn)                             eX(6);
      AryCreate(&pn->adc, sizeof(float), 1, 0, MNT_NUMVAL);         eG(7);
      if (scatter) {                       //-2008-07-30
        AryCreate(&pn->ads, sizeof(float), 1, 0, MNT_NUMVAL);        eG(77);
      }
      sprintf(pn->np, "%s", pm->np);
      pn->i = pm->i;   pn->j = pm->j;   pn->k = pm->k;
      pn->xp = pm->xp; pn->yp = pm->yp; pn->zp = pm->zp;
      pn->nn = pm->nn; pn->nl = pm->nl; pn->ni = pm->ni;
      pn->ic = l;
      pn->t1 = t1;
      pn->t2 = t2;
      pn->index = n;
        for (i=0; i<n; i++) {
        fpi = (float *)AryPtrX(&dsc, i, l);
        fpo = (float *)AryPtrX(&pn->adc, i);           //-2008-07-30
        *fpo = *fpi;
        if (scatter) {                      //-2008-07-30
          fpi = (float *)AryPtr(&dscs, i, l);
          fpo = (float *)AryPtr(&pn->ads, i);
          *fpo = *fpi;
        }
      }
    }
    AryFree(&dsc);                                                //-2002-10-28
    if (scatter) AryFree(&dscs);                 //-2008-07-30
  }
  TmnDetach(imnt, NULL, NULL, TMN_MODIFY, NULL);                     eG(8);
  TmnDetach(imon, NULL, NULL, 0, NULL);                              eG(9);
  TmnDetach(ict, NULL, NULL, 0, NULL);                               eG(10);
  return 0;
eX_30:
  eMSG(_count_monitor_$_, MNT_MAX);
eX_12: eX_13: eX_14: eX_15: eX_16: eX_77:
eX_1: eX_2: eX_3: eX_5:  eX_6:  eX_7: eX_8: eX_9: eX_10:
  eMSG(_cant_get_tables_);
eX_11:
  eMSG(_cant_read_monitor_$_, name);
eX_21: eX_25:
  eMSG(_data_$_inconsistent_, name);
eX_22: eX_23: eX_24:
  eMSG(_wrong_times_$_, name);
}

#define CAT(a, b)  sprintf(t, a, b), TxtCat(&hdr, t);
//------------------------------------------------------------------ DtbMntStore
long DtbMntStore( void )
{
  dP(DtbMntStore);
  int i, l, m, nc, i1, i2, scatter;                //-2008-07-30
  char s[512], t1s[40], t2s[40], t[512];
//  char *hdr, *name, *unit, *refc;                                 //-2011-06-29
  TXTSTR hdr  = { NULL, 0 };
  TXTSTR name = { NULL, 0 };
  TXTSTR unit = { NULL, 0 };
  long imnt, ict, t1, t2;
  char *fp, *cp;
  ARYDSC *pmnt, *pct, dsc;
  MONREC *pn;
  CMPREC *pcmp;
  if (!(MI.flags & FLG_MNT) || NumMon < 1) 
    return 0;
  scatter  = (MI.flags & FLG_SCAT) && (MI.numgrp > 4);            //-2008-07-30
  ict = IDENT(CMPtab, 0, 0, 0);
  pct = TmnAttach(ict, NULL, NULL, 0, NULL);  if (!pct)              eX(1);
  pcmp = pct->start;
  imnt = IDENT(MNTarr,0,0,0);
  pmnt = TmnAttach(imnt, NULL, NULL, 0, NULL); if (!pmnt)            eX(2);
  nc = pmnt->bound[1].hgh - pmnt->bound[1].low + 1;
  if (nc != pct->bound[0].hgh - pct->bound[0].low + 1)               eX(3);

  // Header und Daten-Array erzeugen
  for (m=1; m<=NumMon; m++) {
    pn = AryPtr(pmnt, m, 0); if (!pn)                                eX(4);
    pn->adc.bound[0].hgh = pn->adc.bound[0].low + pn->index - 1;
    if (m == 1) {
      t1 = pn->t1;
      t2 = pn->t2;
      strcpy(t1s, TmString(&t1));
      strcpy(t2s, TmString(&t2));
      i1 = pn->adc.bound[0].low;
      i2 = pn->adc.bound[0].hgh;
    }
    memset(&dsc, 0, sizeof(ARYDSC));
    AryCreate(&dsc, sizeof(float), 2, i1, i2, 0, nc-1);              eG(5);
    if (pn->t1 != t1 || pn->t2 != t2)                                eX(7);
    if (pn->adc.bound[0].low != i1 || pn->adc.bound[0].hgh != i2)    eX(8);
    TxtCpy(&hdr, "\n");
    CAT("cset  \"%s\"\n", TI.cset);
    sprintf(s, "TALDTB_%d.%d.%s", StdVersion, StdRelease, StdPatch);
    CAT("prgm  \"%s\"\n", s);
    CAT("idnt  \"%s\"\n", MI.label);
    CAT("artp  \"%s\"\n", "Z");
    CAT("refx  %d\n", GrdPprm->refx);
    CAT("refy  %d\n", GrdPprm->refy);
    if (GrdPprm->refx > 0 && *GrdPprm->ggcs)
      CAT("ggcs  \"%s\"\n", GrdPprm->ggcs);
    CAT("mntn  %10s\n", TxtQuote(pn->np));                        //-2009-04-15
    CAT("mntx  %10.1f\n", pn->xp);
    CAT("mnty  %10.1f\n", pn->yp);
    CAT("mntz  %10.1f\n", pn->zp);
    CAT("mnti  %10d\n",   pn->i);
    CAT("mntj  %10d\n",   pn->j);
    CAT("mntk  %10d\n",   pn->k);
    CAT("grdl  %10d\n",   pn->nl);
    CAT("grdi  %10d\n",   pn->ni);
    CAT("undf  %d\n", -1);
    CAT("form  \"%s\"\n", "con%12.4e");
    CAT("t1    \"%s\"\n", t1s);
    CAT("t2    \"%s\"\n", t2s);
    CAT("dt    \"%s\"\n", TmString(&MI.cycle));
    CAT("rdat  \"%s\"\n", MI.refdate);
    CAT("mode  \"%s\"\n", "text");
    CAT("axes  \"%s\"\n", "ts");
    TxtCpy(&name, "name");
    TxtCpy(&unit, "unit");         
    for (l=0; l<nc; l++) {
      sprintf(s, " \"%s\"", pcmp[l].name);   TxtCat(&name, s);
      sprintf(s, " \"%s\"", pcmp[l].unit);   TxtCat(&unit, s);
    }
    TxtCat(&name, "\n");  TxtCat(&hdr, name.s);  TxtClr(&name);
    TxtCat(&unit, "\n");  TxtCat(&hdr, unit.s);  TxtClr(&unit);
    for (i=i1; i<=i2; i++) {
      for (l=0; l<nc; l++) {
        fp = AryPtrX(&dsc, i, l);
        pn = AryPtr(pmnt, m, l); if (!pn)                               eX(9);
        *(float *)fp = *(float *)AryPtrX(&pn->adc, i);
      }
    }
    sprintf(s, "%s/mntc%04d", StdPath, m);            //-2008-07-30
    if (0 > DmnWrite(s, hdr.s, &dsc))                                   eX(10);
    AryFree(&dsc);
    //
    if (scatter) {                                                //-2008-07-30
      memset(&dsc, 0, sizeof(ARYDSC));
      AryCreate(&dsc, sizeof(float), 2, i1, i2, 0, nc-1);            eG(55);
      if (pn->ads.bound[0].low != i1 || pn->ads.bound[0].hgh < i2)   eX(58);
      cp = strstr(hdr.s, "con%12.4e");
      strncpy(cp, "dev%8.4f ", 9);
      cp = strstr(hdr.s, "unit");
      *cp = 0;
      TxtCat(&hdr, "unit");
      for (l=0; l<nc; l++) {
        TxtCat(&hdr, " \"1\"");
      }
      TxtCat(&hdr, "\n");
      //
      for (i=i1; i<=i2; i++) {
        for (l=0; l<nc; l++) {
          fp = AryPtr(&dsc, i, l);
          pn = AryPtr(pmnt, m, l); if (!pn)                          eX(59);
          *(float *)fp = *(float *)AryPtr(&pn->ads, i);
        }
      }
      sprintf(s, "%s/mnts%04d", StdPath, m);
      if (0 > DmnWrite(s, hdr.s, &dsc))                             eX(60);
      AryFree(&dsc);
    }
    //
    for (l=0; l<nc; l++) {                                        //-2006-02-15
      pn = AryPtr(pmnt, m, l); if (!pn)                              eX(9);
      AryFree(&pn->adc);                                             eG(23);
      if (scatter) {
        AryFree(&pn->ads);                                           eG(24);
      }
    }
  }
  //                                                              //-2006-02-15
  TxtClr(&hdr);
  TmnDetach(imnt, NULL, NULL, 0, NULL);                             eG(20);
  TmnClear(TmMax(), imnt, TMN_NOID);                                eG(21);
  TmnDetach(ict, NULL, NULL, 0, NULL);                              eG(22);
  return 0;
eX_1: eX_2: eX_3: eX_4: eX_5: eX_7: eX_8: eX_9: eX_10:
eX_55: eX_58: eX_59: eX_60:
eX_20: eX_21: eX_22: eX_23: eX_24:
  eMSG(_cant_write_);
}
#undef CAT

/*================================================================ DtbRegister
*/
static void regval( float a, float g, DTBREC *p )
  {
  int i, n, m;
  float g0;
  if (a <= 0.0)  {
    p->frqnull += g;
    return;
  }
  if (a >= DTB_RANGE*p->valmax) {
    g0 = 0.0;
    for (i=0; i<DTB_NUMVAL; i++) {
      g0 += p->frq[i];
      p->frq[i] = 0.0; }
    p->frqsub += g0;
    n = floor(1 + DTB_DIVISION*log10(a));
    p->valmax = pow(10.0, ((float)n)/DTB_DIVISION);
    p->frq[DTB_NUMVAL-1] = g;
    return;
  }
  n = floor(DTB_NUMVAL + DTB_DIVISION*log10(a/p->valmax));
  if (n < 0)  {
    p->frqsub += g;
    return;
  }
  if (n < DTB_NUMVAL) {
    p->frq[n] += g;
    return;
  }
  m = n - DTB_NUMVAL + 1;
  g0 = 0.0;
  for (i=0; i<m; i++)  g0 += p->frq[i];
  p->frqsub += g0;
  for ( ; i<DTB_NUMVAL; i++) {
    p->frq[i-m] = p->frq[i];
    p->frq[i] = 0.0;
  }
  p->valmax *= pow(10.0, m/DTB_DIVISION);
  p->frq[DTB_NUMVAL-1] = g;
  return;
}

static void regmax(float a, float dev, DTBMAXREC *p) {
  int i, j;
  for (i=DTB_NUMMAX-1,j=-1; i>=0; i--)
    if (a > p->max[i]) j = i;
    else  break;                                                  //-2001-10-19
  if (j < 0) return;
  for (i=DTB_NUMMAX-1; i>j; i--) {
    p->max[i] = p->max[i-1];
    p->dev[i] = p->dev[i-1];
  }
  p->max[j] = a;
  p->dev[j] = dev;
}

//------------------------------------------------------------------ DtbRegister
long DtbRegister(       /* register in concentration distribution */
long idtb )
  {
  dP(DtbRegister);
  int gp, gl, gi, i, j, k, l, nx, ny, nz, nc, sz;
  char t1s[40], t2s[40], m1s[40], m2s[40], name[256]; //-2004-11-26
  long t1, t2, m1, m2, icon, idos, iblp, usage, imnt, invalid;  //-2006-02-15
  ARYDSC *pcon, *pdtb, *pblp, *pmnt;                                //-2006-02-15
  DTBREC  *pd;
  DTBMAXREC *pd2;
  MONREC  *pn;
  BLMPARM *pbp;                                                     //-2006-02-15
  float *pc, sg, a, dev, *pf, valid;
  int scatter=0;
  strcpy(name, NmsName(idtb));
  vLOG(4)("concentration distribution %s", name);
  gp = (MI.average > 1);
  gl = XTR_LEVEL(idtb);
  gi = XTR_GRIDN(idtb);
  TmnInfo(idtb, &m1, &m2, &usage, NULL, NULL);
  if (!(usage & TMN_DEFINED))  m2 = TmMin();
  icon = IDENT(CONtab, gp, gl, gi);
  idos = IDENT(DOStab, gp, gl, gi);
  imnt = IDENT(MNTarr,0,0,0);
  TmnInfo(icon, &t1, &t2, &usage, NULL, NULL);
  valid = WrkValid(VALID_INTER);                                  //-2011-07-04
  if (usage & TMN_DEFINED) {
    if ((m2 > TmMin()) && (m2 != t1))                                eX(1);
  }
  else {
    icon = TmnIdent();
    DtbDos2Con(idos, icon, 1, 0, valid);                             eG(3); //-2011-07-04
    TmnInfo(icon, &t1, &t2, &usage, NULL, NULL);
    if (!(usage & TMN_DEFINED))                                      eX(4);
  }
  invalid = (valid < 0.5);                                        //-2011-07-04
  pcon = TmnAttach(icon, &t1, &t2, 0, NULL);  if (!pcon)             eX(5);
  nx = pcon->bound[0].hgh;
  ny = pcon->bound[1].hgh;
  nz = pcon->bound[2].hgh;
  nc = pcon->bound[3].hgh + 1;
  scatter = (pcon->elmsz == 2*sizeof(float));                       //-2001-10-19
  if (!scatter && ((MI.flags & FLG_SCAT) && (MI.numgrp > 4)))        eX(90);  //-2008-07-30
  strcpy(t1s, TmString(&t1));
  strcpy(t2s, TmString(&t2));
  TmnInfo(idtb, &m1, &m2, &usage, NULL, NULL);
  if (usage & TMN_DEFINED) {
    if (m2 != t1)                                                    eX(6);
    pdtb = TmnAttach(idtb, &m1, &m2, TMN_MODIFY, NULL);  if (!pdtb)  eX(7);
  }
  else {
    sz = (MI.flags & FLG_MAXIMA) ? sizeof(DTBMAXREC) : sizeof(DTBREC);
    pdtb = TmnCreate(idtb, sz, 4, 1, nx, 1, ny, 1, nz, 0, nc-1);     eG(8);
    m1 = t1;
  }
  TmnInfo(imnt, NULL, NULL, &usage, NULL, NULL);
  if (!(usage & TMN_DEFINED)) {
      DtbMntSet(t1, nc);                                             eG(80);
  }
  //
  iblp = IDENT(BLMpar, 0, 0, 0);
  pblp =  TmnAttach(iblp, NULL, NULL, 0, NULL);  if (!pblp)          eX(11);
  pbp = (BLMPARM*) pblp->start;
  sg = pbp->StatWeight;
  TmnDetach(iblp, NULL, NULL, 0, NULL);                              eG(14);
  if (sg <= 0)                                                       eX(13);
  if ((MI.flags & FLG_MAXIMA) && sg != 1.)                           eX(20);
  if (invalid)  
    nx = 0;
  for (i=1; i<=nx; i++) {
    for (j=1; j<=ny; j++) {
      for (k=1; k<=nz; k++) {
        for (l=0; l<nc; l++) {
          pc = (float*) AryPtr(pcon, i, j, k, l);  if (!pc)          eX(15);
          a = pc[0]/sg;
          dev = (scatter) ? pc[1] : -1;                             //-2001-10-19
          if (MI.flags & FLG_MAXIMA) {
            pd2 = (DTBMAXREC*) AryPtr(pdtb, i, j, k, l);  if (!pd2)  eX(16);
            regmax(a, dev, pd2);
          }
          else {
            pd = (DTBREC*) AryPtr(pdtb, i, j, k, l);  if (!pd)       eX(16);
            if (a < 0.0)  continue;
            if (a == 0.0) {
              pd->frqnull += sg;
              continue;
            }
            if (a > pd->realmax) {                /*-04aug94-*/
              pd->realmax = a;
              pd->realdev = dev;
              pd->t1 = t1;
              pd->t2 = t2;
            }
            regval(a, sg, pd);
          }
        }
      }
    }
  }
  // get concentration at monitor points
  if (MI.flags & FLG_MNT) {
    pmnt = TmnAttach(imnt, NULL, NULL, TMN_MODIFY, NULL); if (!pmnt) eX(81);
    for (i=1; i<=NumMon; i++) {
      for (l=0; l<nc; l++) {
        pn = AryPtr(pmnt, i, l); if (!pn)                            eX(82);
        if (pn->nl != gl || pn->ni != gi) continue;
        if (t1 != pn->t2)                                            eX(83);
        pf = (float *)AryPtrX(&pn->adc, pn->index);
        if (invalid) {
          *pf = -1;
          if (scatter) {                      //-2008-07-30
            pf = (float *)AryPtr(&pn->ads, pn->index);
            *pf = -1;
          }
        }
        else {
          pc = (float*) AryPtr(pcon, pn->i, pn->j, pn->k, l);  if (!pc) eX(84);
          if (pn->index >= MNT_NUMVAL)                                  eX(86); //-2003-12-05
          *pf++ = *pc++;
          if (scatter) {                                          //-2008-07-30
            pf = (float*)AryPtr(&pn->ads, pn->index);
            *pf = *pc;
          }
        }
        pn->index = pn->index+1;
        pn->t2 = t2;
      }
    }
    TmnDetach(imnt, NULL, NULL, TMN_MODIFY, NULL);                   eG(85);
  }

  TmnDetach(idtb, &m1, &t2, TMN_MODIFY, NULL);                       eG(17);
  TmnDetach(icon, NULL, NULL, 0, NULL);                              eG(18);
  if (icon < 0) {
    TmnDelete(t2, icon, TMN_NOID);                                   eG(19);
  }
  return 0;
eX_1:  eX_6:
  strcpy(t1s, TmString(&t1));
  strcpy(t2s, TmString(&t2));
  strcpy(m1s, TmString(&m1));
  strcpy(m2s, TmString(&m2));
  strcpy(name, NmsName(icon));
  eMSG(_cant_add_$$$_$$$_, name, t1s, t2s, NmsName(idtb), m1s, m2s);
eX_3:  eX_4:  eX_5:
  strcpy(t1s, TmString(&t1));
  strcpy(t2s, TmString(&t2));
  strcpy(name, NmsName(icon));
  eMSG(_cant_get_$$$_, name, t1s, t2s);
eX_7:
  strcpy(m1s, TmString(&m1));
  strcpy(m2s, TmString(&m2));
  strcpy(name, NmsName(idtb));
  eMSG(_cant_get_$$$_, name, m1s, m2s);
eX_8:
  strcpy(name, NmsName(idtb));
  eMSG(_cant_create_$_, name);
eX_11: eX_14:
  eMSG(_cant_get_weight_);
eX_13:
  eMSG(_invalid_weight_$_, sg);
eX_15: eX_16:
  eMSG(_index_error_$$$$_, i, j, k, l);
eX_17: eX_18:
  eMSG(_cant_detach_);
eX_19:
  eMSG(_cant_clear_);
eX_20:
  eMSG(_cant_determine_maximum_);
eX_80: eX_81: eX_82: eX_83: eX_84: eX_85:
  eMSG(_cant_set_monitor_);
eX_86:
  eMSG(_time_series_length_$_, MNT_NUMVAL);
eX_90:
 eMSG(_internal_error_);
}

/*================================================================== DtbHeader
*/
#define CAT(a, b)  sprintf(t, a, b), TxtCat(&hdr, t);
char *DtbHeader(                /* the header (global storage)  */
  long id,                      /* identification               */
  long *pt1,                    /* start of the validity time   */
  long *pt2 )                   /* end of validity time         */
  {
  dP(DtbHeader);
  char name[256], t1s[40], t2s[40], dts[40], format[80], s[100+256], t[100+256];  //-2008-12-04 //-2011-04-13
  int gl, gi, gp, iga, igp, k, ict, nz, nc, folge;
  char *atype;
  float *sk;
  ARYDSC *pa, *pct, *pga;
  GRDPARM *pgp;
  CMPREC *pcmp;
  enum DATA_TYPE dt;
  TXTSTR hdr = { NULL, 0 };
  
  strcpy(name, NmsName(id));
  dt = XTR_DTYPE(id);
  gp = XTR_GROUP(id);
  gl = XTR_LEVEL(id);
  gi = XTR_GRIDN(id);
  switch (dt) {
    case CONtab: atype = "C";
                 break;
    case DTBtab:
    case DTBarr: atype = (MI.flags & FLG_MAXIMA) ? "M" : "HN";
                 break;
    default:     atype = "?";
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
  strcpy(dts, TmString(&MI.cycle));                               //-2008-12-04
  //
  // DMNA header                                                  //-2011-06-29
  sprintf(s, "TALDTB_%d.%d.%s", StdVersion, StdRelease, StdPatch);
  TxtCpy(&hdr, "\n");
  CAT("prgm  \"%s\"\n", s);
  CAT("artp  \"%s\"\n", atype);
  CAT("axes  \"%s\"\n", "xyzs");
  CAT("file  \"%s\"\n", name);
  CAT("idnt  \"%s\"\n", MI.label);
  CAT("t1    \"%s\"\n", t1s);
  CAT("t2    \"%s\"\n", t2s);
  CAT("dt    \"%s\"\n", dts);
  folge = (MI.average>0 && gp==3) ? 1+(MI.cycind-1)/MI.average : MI.cycind;
  if (dt==DTBarr || dt==DTBtab) {
    if (MI.flags & FLG_MAXIMA) {
      sprintf(s, "S00%%[10]10.2eS10%%[10]10.2eS20%%[%d]10.2e"
          "D00%%[10](*100)5.1fD10%%[10](*100)5.1fD20%%[%d](*100)5.1f",
        DTB_NUMMAX-20, DTB_NUMMAX-20);
      CAT("form  \"%s\"\n", s);
      CAT("dtbnummax  %d\n", DTB_NUMMAX);
      CAT("index  %d\n", 0);
      CAT("groups %d\n", 0);
    }
    else {
      sprintf(s,
        "Max%%10.2eN=%%[2]5.0fF0%%[10]3.0fFa%%[20]3.0fG0%%[%d]3.0f"
        "MaxVal%%10.2eMaxDev%%10.2eT1%%[2]13t",
         DTB_NUMVAL-30);
      CAT("form  \"%s\"\n", s);
      CAT("dtbnumval  %d\n", DTB_NUMVAL);
      CAT("index  %d\n", 0);
      CAT("groups %d\n", 0);
    }
  }
  else {   
    strcpy(format, "Con%12.4e");                                  //-2011-06-29
    if (MI.flags & FLG_SCAT)  strcat(format, "Dev%(*100)5.1f");   //-2011-06-29
    CAT("form  \"%s\"\n", format);
    CAT("index  %d\n", folge);
    CAT("groups %d\n", MI.numgrp);
  }
  CAT("xmin  %1.0f\n", pgp->xmin);
  CAT("ymin  %1.0f\n", pgp->ymin);
  CAT("delta %1.1f\n", pgp->dd);
  CAT("refx  %d\n", GrdPprm->refx);
  CAT("refy  %d\n", GrdPprm->refy);
  if (GrdPprm->refx > 0 && *GrdPprm->ggcs)                        //-2008-12-04
    CAT("ggcs  \"%s\"\n", GrdPprm->ggcs);
  CAT("zscl  %1.0f\n", pgp->zscl);
  CAT("sscl  %1.0f\n", pgp->sscl);
  TxtCat(&hdr, "sk ");
  for (k=0; k<nz; k++) {
    CAT(" %s", TxtForm(sk[k], 6));
  }
  TxtCat(&hdr, "\n");
  TxtCat(&hdr, "name  ");
  for (k=0; k<nc; k++) {
    CAT(" \"%s\"", pcmp[k].name);
  }
  TxtCat(&hdr, "\n");
  TxtCat(&hdr, "unit  ");
  for (k=0; k<nc; k++) {
    CAT(" \"%s\"", pcmp[k].unit);
  }
  TxtCat(&hdr, "\n");
  TxtCat(&hdr, "refc  ");
  for (k=0; k<nc; k++) {
    CAT(" %10.2e", pcmp[k].refc);
  }
  TxtCat(&hdr, "\n");
  TxtCat(&hdr, "refd  ");
  for (k=0; k<nc; k++) {
    CAT(" %10.2e", pcmp[k].refd);
  }
  TxtCat(&hdr, "\n");
  if (MI.flags & FLG_MAXIMA) {
    TxtCat(&hdr, "vldf  \"");
    for (k=0; k<DTB_NUMMAX; k++) TxtCat(&hdr, "V");
    TxtCat(&hdr, "\"\n");
  }
  TmnDetach(igp, NULL, NULL, 0, NULL);                  eG(10);
  TmnDetach(iga, NULL, NULL, 0, NULL);                  eG(11);
  TmnDetach(ict, NULL, NULL, 0, NULL);                  eG(12);
  CAT("valid  %1.6f\n",WrkValid(VALID_TOTAL));                    //-2011-07-04
  return hdr.s;
eX_1:  eX_2:  eX_3:  eX_4:  eX_5:  eX_6:
  nMSG(_cant_attach_);
  return NULL;
eX_10: eX_11: eX_12:
  nMSG(_cant_detach_);
  return NULL;
}
#undef  CAT

/*==================================================================== DtbInit
*/
long DtbInit(           /* initialize server    */
long flags,             /* action flags         */
char *istr )            /* server options       */
  {
  dP(DtbInit);
  long idtb, icon, imnt, mask;
  char *jstr, *ps, s[200];
  if (StdStatus & STD_INIT)  return 0;
  if (istr) {
    jstr = istr;
    ps = strstr(istr, "-v");
    if (ps) sscanf(ps+2, "%d", &StdLogLevel);
    ps = strstr(istr, "-y");
    if (ps) sscanf(ps+2, "%d", &StdDspLevel);
    ps = strstr(istr, "-d");
    if (ps)  strcpy(DefName, ps+2);
    }
  else  jstr = "";
  vLOG(3)("DTB_%d.%d.%s (%08lx,%s)", StdVersion, StdRelease, StdPatch, flags, jstr);
  StdStatus |= flags;
  mask = ~(NMS_GROUP|NMS_GRIDN|NMS_LEVEL);
  idtb = IDENT(DTBarr, 0, 0, 0);
  TmnCreator(idtb, mask, TMN_UNIQUE, "", DtbServer, DtbHeader);         eG(10);
  icon = IDENT(CONtab, 0, 0, 0);
  TmnCreator(icon, mask, TMN_UNIQUE, "", DtbServer, WrkHeader);         eG(1);  //-2011-07-04

  if (MI.flags & FLG_MNT) {
    sprintf(s, " ZET -v%d -d%s", StdLogLevel, DefName);
    ZetInit(flags, s);
    imnt = IDENT(MNTarr, 0, 0, 0);
    TmnCreator(imnt, -1, TMN_UNIQUE, "", DtbServer, WrkHeader);         eG(8);  //-2011-07-04
  }

  sprintf(s, " GRD -v%d -d%s", StdLogLevel, DefName);
  GrdInit(flags, s);                                                    eG(2);
  sprintf(s, " PRM -v%d -d%s", StdLogLevel, DefName);
  PrmInit(flags, s);                                                    eG(3);
  sprintf(s, " MOD -v%d -d%s", StdLogLevel, DefName);
  ModInit(flags, s);                                                    eG(4);
  sprintf(s, " DOS -v%d -d%s", StdLogLevel, DefName);
  DosInit(flags, s);                                                    eG(5);
  StdStatus |= STD_INIT;
  return 0;
eX_10:
  eMSG(_cant_set_creator_$_, NmsName(idtb));
eX_1:
  eMSG(_cant_set_creator_$_, NmsName(icon));
eX_2:  eX_3:  eX_4:  eX_5:
  eMSG(_cant_init_servers_);
eX_8:
  eMSG(_cant_read_monitor_);
}

  /*================================================================= DtbServer
  */
long DtbServer(         /* server routine for DTBarr, CONtab    */
  char *s )             /* calling option                       */
  {
  dP(DtbServer);
  char name[256];      //-2004-11-26
  int gl, gi, gp, net;
  long idos, idtb;
  enum DATA_TYPE dt;
  if (StdArg(s))  return 0;
  if (*s) {
    switch (s[1]) {
      case 'd': strcpy(DefName, s+2);
                break;
      case 'm': if (s[2] == 's')
                  DtbMntStore();
                else if (s[2] == 'r')
                  DtbMntRead();
                break;
      case 'n': if (s[2]) {                                      //-2001-06-29
                  sscanf(s+2, "%d,%d", &PartTotal, &PartDrop);
                  PartNew = 0;
                }
                else {
                  PartTotal++;
                  PartNew = 1;
                }
                break;
      default:  ;
      }
    return 0;
    }
  if (!StdIdent)                                                eX(10);
  if ((StdStatus & STD_INIT) == 0) {
    DtbInit(0, "");                                             eG(1);
    }
  dt = XTR_DTYPE(StdIdent);
  gl = XTR_LEVEL(StdIdent);
  gi = XTR_GRIDN(StdIdent);
  gp = XTR_GROUP(StdIdent);
  strcpy(name, NmsName(StdIdent));
  if (StdStatus & STD_TIME)  T1 = StdTime;
  else  T1 = TmMin();
  net = GrdPprm->numnet;
  do {
    if (net > 0) {
      GrdSetNet(net);                                           eG(5);
      }
    if ((gl) && (GrdPprm->level!=gl || GrdPprm->index != gi)) {
      net--;
      continue;
      }
    switch (dt) {
      case CONtab: idos = IDENT(DOStab, gp, GrdPprm->level, GrdPprm->index);
                   DtbDos2Con(idos, 0, 0, 0, WrkValid(VALID_AVER));  eG(2); //-2011-07-04
                   break;
      case DTBarr: idtb = IDENT(DTBarr,  0, GrdPprm->level, GrdPprm->index);
                   DtbRegister(idtb);                           eG(3);
                   break;  
      default:                                                  eX(4);
      }
    net--;
    }  while (net > 0);
  return 0;
eX_10:
  eMSG(_no_data_);
eX_1:
  eMSG(_cant_init_);
eX_2:
  strcpy(name, NmsName(idos));
  eMSG(_cant_calculate_contab_$_, name);
eX_3:
  strcpy(name, NmsName(idtb));
  eMSG(_cant_calculate_$_, name);
eX_4:
  eMSG(_invalid_request_$_, name);
eX_5:
  eMSG(_cant_set_grid_$_, net);
  }

//==============================================================================
//
// history:
//
// 2002-09-24 lj 1.0.0  final release candidate
// 2002-10-28 lj 1.0.1  DtbMntRead() corrected (undefined array descriptor)
// 2003-12-05 lj 1.1.13 DtbRegister(): error message more specific
// 2004-11-26 lj 2.1.7  string length for names = 256
// 2005-03-17 uj 2.1.8  4 decades
// 2005-03-17 uj 2.2.0  version number upgrade
// 2006-02-15 lj 2.2.9  get SG from BLMpar, clear arrays in MNTarr
//                      DtbMntRead corrected, freeing of strings
// 2006-02-25 uj        Kennung in quotation marks
// 2006-10-26 lj 2.3.0  external strings
// 2008-07-22 lj 2.4.2  parameter "cset" in dmna-header
// 2008-08-04 lj 2.4.3  writes "valid" using 6 decimals
// 2008-12-04 lj 2.4.5  writes "refx", "refy", "ggcs", "dt", "rdat", "artp"
// 2011-04-13 uj 2.4.9  handle title with 256 chars in header
// 2011-06-29 uj 2.5.0  dmna output
//                      declaration of DefName corrected, dmn header
// 2011-07-01 uj        DtbDos2Con: distinguish tiny dos and dev
// 2012-04-06 uj 2.6.0  version number upgrade
//
//==============================================================================

