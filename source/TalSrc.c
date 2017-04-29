// ================================================================== TalSrc.c
//
// Generate particles
// ==================
//
// Copyright (C) Umweltbundesamt, Dessau-Ro�lau, Germany, 2002-2005
// Copyright (C) Janicke Consulting, 88662 �berlingen, Germany, 2002-2014
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
// last change: 2014-06-26 uj
//
//==========================================================================

#include <math.h>

#define  STDMYMAIN  SrcMain
#include "IBJmsg.h"
#include "IBJary.h"
#include "IBJtxt.h"
#include "IBJdmn.h"
#include "IBJstd.h"
static char *eMODn = "TalSrc";

//=============================================================================

STDPGM(tstsrc, SrcServer, 2, 6, 11);

//=============================================================================

#include "genutl.h"
#include "genio.h"

#include "TalRnd.h"
#include "TalTmn.h"
#include "TalNms.h"
#include "TalGrd.h"
#include "TalPpm.h"
#include "TalPrm.h"
#include "TalPtl.h"
#include "TalSrc.h"
#include "TalSrc.nls"
#include "TalMat.h"

//====================================================== random numbers
//
static int SrcRndIset = 0;

#if defined _M_IX86 && !defined __linux__               //-2003-02-21
    static __int64 sed = 0x12345678i64;
    static __int64 fac = 0x5DEECE66DI64;
    static __int64 add = 0xBI64;
    static __int64 sgn = 0x7fffffffffffffffI64;
    static __int64 msk = (1I64 << 48) - 1;
    
    static void setSeed( __int64 new_seed ) {
      sed = (new_seed ^ fac) & msk;
      SrcRndIset = 0;
    }
#else
    static long long sed = 0x12345678LL;
    static long long fac = 0x5DEECE66DLL;
    static long long add = 0xBLL;
    static long long sgn = 0x7fffffffffffffffLL;
    static long long msk = (1LL << 48) - 1;
    
    static void setSeed( long long new_seed ) {
      sed = (new_seed ^ fac) & msk;
      SrcRndIset = 0;
    }
#endif

static unsigned long next( int bits ) {
  sed = (sed*fac + add) & msk;
  return (unsigned long)((sed & sgn) >> (48 - bits));
}

static unsigned long SrcRndULong( void ) {
  return next(32);
}

static float SrcRndEqu01() {
  return next(24)/((float)(1 << 24));
}

static float SrcRndNrm()         // Normal verteilt ( Mittel = 0,  Varianz = 1 ).
  {
  static float gset;
  float fac, r, v1, v2;
  if (SrcRndIset == 0) {
    do {
      r = SrcRndEqu01();
      v1 = 2.0*r - 1.0;
      r = SrcRndEqu01();
      v2 = 2.0*r - 1.0;
      r = v1*v1 + v2*v2;
      } while (r >= 1.0);
    fac = sqrt( -2.0*log(r)/r );
    gset = v1*fac;
    SrcRndIset = 1;
    return v2*fac;
    }
  else {
    SrcRndIset = 0;
    return gset;
    }
  }

/*================================================================== SrcNumPtl
*/
long SrcNumPtl(       /*  Berechnung der Teilchenzahl für eine Zelle.  */
int np,               /*  Gesamtzahl der Teilchen für das Raster.      */
SRCREC *psrc,         /*  Pointer auf Quellen-Beschreibung.            */
ARYDSC *pdsc,         /*  Pointer auf Rasterquelle                     */
float *px0,           /*  Linke x-Koordinate der Zelle.                */
float *py0,           /*  Untere y-Koordinate der Zelle.               */
float *ph0,           /*  Höhe der Raster-Zelle.                       */
float *pfak )         /*  Korrektur-Faktor für Teilchen-Masse.         */
  {
  dP(SrcNumPtl);
  static SRCREC *qsrc;
  static ARYDSC *qdsc;
  static float *pval;
  static double summ, part, eqmin;
  static int ip, inxt, jnxt, i1, i2, j1, j2, numval;
  int i, j, n;
  double eq;
  setSeed(RndULong());
  if (np < 0) {              /* Initialisierung */
    qsrc = psrc;
    qdsc = pdsc;
    i1 = pdsc->bound[0].low;  i2 = pdsc->bound[0].hgh;
    j1 = pdsc->bound[1].low;  j2 = pdsc->bound[1].hgh;
    summ = 0.0;
    for (i=i1; i<=i2; i++)
      for (j=j1; j<=j2; j++) {
        part = *FltPtr(pdsc, i, j);  if (part < 0)      eX(1);
        summ += part; }
    part = 0.0;
    eqmin = 1.e-10*summ;
    ip = 0;  inxt = i1;  jnxt = j1;
    numval =  pdsc->elmsz/sizeof(float);
    return 0;
    }
  if ((psrc != qsrc) || (pdsc != qdsc))                 eX(2);
  pval = AryPtr(pdsc, inxt, jnxt);  if (!pval)          eX(3);
  eq = *pval;
  if (numval > 1)  *ph0 = pval[1];
  else  *ph0 = psrc->h;
  *px0 = psrc->x + (inxt-i1)*psrc->w;
  *py0 = psrc->y + (jnxt-j1)*psrc->l;
  jnxt++;
  if (jnxt > j2)  { jnxt = j1;  inxt++; }
  if (inxt > i2)  n = np - ip;
  else {
    if (eq <= eqmin) {
      *pfak = 1;
      return 0;
      }
    n = (np-ip)*eq/(summ-part) + SrcRndEqu01();
    if (n > np-ip)  n = np-ip;
    }
  *pfak = 1;
  ip += n;
  part += eq;
  return n;
eX_1:
  eMSG(_negative_emission_$$$_, psrc->name, i, j);
eX_2:
  eMSG(_inconsistent_reference_);
eX_3:
  eMSG(_range_error_grid_$$$_, psrc->name, inxt, jnxt);
  }

/*================================================================== SrcCrtPtl
*/
long SrcCrtPtl(       /* Teilchen erzeugen.                            */
int aslind,           /* Stoffarten-Index der Teilchen.                */
int part,             /* Nummer der Gruppe.                            */
long idptl )          /* Ident der Teilchen-Tabelle                    */
  {
  dP(SrcCrtPtl);
  PTLREC *pptl;
  ASLREC *pasl;
  SRCREC *psrc;
  SRGREC *psrg;
  CMPREC *pcmp;
  ARYDSC *pparr, *pgarr;
  char name[256], t1s[40], t2s[40];				//-2004-11-26
  long l, rc, t1, t2, idgrd;
  int n, np, numptl, sumptl, specid, numparts, handle;
  int src, cmp, i, k, cmp1, cmp2, ncmp, ip, sumip, ig;
  float x0, y0, dx1, dx2, dy1, dy2, r1, r2, r3, si, co;
  float x1, y1, x2, y2, aq, bq, cq, lq, wq, dq, hq, tq, xq, yq;
  double t0, dt;                                            /*-25sep95-*/
  float afuhgt, afutsc, delta;
  float xm, ym, dxm, dym, dz, h1, h2, dh;
  float fak, mptl, np_soll, np_ist, efac, *pe, *pg, fq, ef;
  float *gg, *totalems, *prelmptl;
  float admean, slogad, *adlimits, *addistri, ad, adg, vg, rho, qf;
  int nadclass;
  double a;
  int *ico, io, no;                                               //-2008-03-10
  vLOG(5)("SRC:SrcCrtPtl(%d,%d,%s)", aslind, part, NmsName(idptl));

  if ((!PrmPasl) || (!PrmPsrc) || (!PrmPems) || (!PrmPcmp)
   || (!PrmPsrg) || (!PrmPemg))                                 eX(50);
  setSeed(RndULong());
  efac = MI.emisfac;
  pasl = AryPtr(PrmPasl, aslind);  if (!pasl)                   eX(1);
  if (aslind >= MI.numasl)                                      eX(2);
  specid = pasl->specident;
  numparts = MI.numgrp;
  cmp1 = pasl->offcmp;
  ncmp = pasl->numcmp;
  /* Staub mit kontinuierlichem Spektrum */
  admean = pasl->admean;
  slogad = pasl->slogad;
  nadclass = pasl->nadclass;
  adlimits = pasl->adlimits;
  addistri = pasl->addistri;
  rho = pasl->density;
  cmp2 = cmp1 + ncmp;
  //
  // analyse odor components                                      //-2008-03-10
  //
  io = -1;
  no = 0;
  ico = NULL;
  for (cmp=cmp1; cmp<cmp2; cmp++) { // find odor components
    pcmp = AryPtr(PrmPcmp, cmp);  if (!pcmp)                eX(61);
    if (!strcmp(pcmp->name, "gas.odor"))
      io = cmp - cmp1;
    else if (!strncmp(pcmp->name, "gas.odor_", 9))
      no++;
  }
  if ((io >= 0) && (no > 0)) {  // register odor components
    ico = ALLOC((no+1)*sizeof(int));  if (!ico)             eX(62);
    no = 1;
    for (cmp=cmp1; cmp<cmp2; cmp++) {
      pcmp = AryPtr(PrmPcmp, cmp);  if (!pcmp)              eX(63);
      if (!strcmp(pcmp->name, "gas.odor"))
        ico[0] = cmp - cmp1;      // sum component
      else if (!strncmp(pcmp->name, "gas.odor_", 9))
        ico[no++] = cmp - cmp1;   // rated components
    }
  }
  //
  gg       = ALLOC(ncmp*sizeof(float));  if (!gg)            eX(3);
  totalems = ALLOC(ncmp*sizeof(float));  if (!totalems)      eX(4);
  prelmptl = ALLOC(ncmp*sizeof(float));  if (!prelmptl)      eX(5);
  t1 = MI.simt1;
  t2 = MI.simt2;
  dt = t2 - t1;
  t0 = t1;
  qf = pow(2.0, MI.quality);
  if (pasl->rate > 0) {               /* Neufestlegung von Mptl */
    for (src=0; src<MI.numsrc; src++) {
      psrc = AryPtr(PrmPsrc, src);  if (!psrc)                     eX(7);
      ig = psrc->idgrp;
      psrg = AryPtr(PrmPsrg, ig);  if (!psrg)                      eX(30);
      fq = psrg->fq;
      for (cmp=cmp1; cmp<cmp2; cmp++) {
        pg = AryPtr(PrmPemg, ig, cmp);  if (!pg)                   eX(40);
        ef = efac*fq*(*pg);
        pe = AryPtr(PrmPems, src, cmp);  if (!pe)                  eX(8);
        totalems[cmp-cmp1] += ef*(*pe);
        }
      }
    for (cmp=cmp1; cmp<cmp2; cmp++) {
      pcmp = AryPtr(PrmPcmp, cmp);  if (!pcmp)                     eX(6);
      mptl = totalems[cmp-cmp1]/(qf*pasl->rate);
      if (mptl > 0)  pcmp->mptl = mptl;           /*-05jun93-*/
      else                                        /*-05jun93-*/
        if (pcmp->mptl <= 0)  pcmp->mptl = 1;     /*-05jun93-*/
      }
    np_soll = dt*qf*pasl->rate;
    np_ist = 0;
    for (src=0; src<MI.numsrc; src++) {
      psrc = AryPtr(PrmPsrc, src);  if (!psrc)                     eX(101);
      ig = psrc->idgrp;
      psrg = AryPtr(PrmPsrg, ig);  if (!psrg)                      eX(31);
      fq = psrg->fq;
      np = 0;
      for (cmp=cmp1; cmp<cmp2; cmp++) {
        pcmp = AryPtr(PrmPcmp, cmp);  if (!pcmp)                   eX(102);
        pg = AryPtr(PrmPemg, ig, cmp);  if (!pg)                   eX(41);
        ef = efac*fq*(*pg);
        pe = AryPtr(PrmPems, src, cmp);  if (!pe)                  eX(8);
        n = ef*(*pe)*dt/pcmp->mptl;
        if (n > np)  np = n;
        }
      np_ist += np;
      }
    }
  numptl = 0;
  for (src=0; src<MI.numsrc; src++, psrc++) {
    psrc = AryPtr(PrmPsrc, src);  if (!psrc)                       eX(103);
    ig = psrc->idgrp;
    psrg = AryPtr(PrmPsrg, ig);  if (!psrg)                        eX(32);
    fq = psrg->fq;
    np = 0;                       // number of particles for this group
    for (cmp=cmp1; cmp<cmp2; cmp++) {
      pcmp = AryPtr(PrmPcmp, cmp);  if (!pcmp)                     eX(104);
      pg = AryPtr(PrmPemg, ig, cmp);  if (!pg)                     eX(42);
      ef = efac*fq*(*pg);
      mptl = pcmp->mptl/MI.usedrfac;
      pe = AryPtr(PrmPems, src, cmp);
      if (ef*(*pe) == 0.0)  n = 0;
      else {
        n = ef*(*pe)*dt/(mptl*numparts) + 0.5;                //-00-03-23
        if (n < 1)  n = 1;
      }
      if (n > np)  np = n;
      }
    numptl += np;
    }
  if (numptl <= 0) {
    FREE(gg);
    FREE(totalems);
    FREE(prelmptl);
    if (ico)  FREE(ico);                                          //-2008-03-10
    numptl = 1;
    PtlCreate(idptl, numptl, 0);                                eG(10);
    TmnDetach(idptl, &t1, &t2, TMN_MODIFY|TMN_UNIQUE, NULL);    eG(11);
    return 0;
    }
  pparr = PtlCreate(idptl, numptl, SRC_EXIST);                  eG(12);
  handle = PtlStart(pparr);                                     eG(13);
  strcpy(name, NmsName(idptl));
  strcpy(t1s, TmString(&t1));
  strcpy(t2s, TmString(&t2));
  vLOG(5)("SRC:SrcCrtPtl: %d particles for %s [%s,%s] ",
    numptl, name, t1s, t2s);
  k = 0;
  sumptl = 0;
  for (src=0; src<MI.numsrc; src++) {
    psrc = AryPtr(PrmPsrc, src);  if (!psrc)                       eX(105);
    ig = psrc->idgrp;
    psrg = AryPtr(PrmPsrg, ig);  if (!psrg)                        eX(33);
    fq = psrg->fq;
    np = 0;                      // number of particles for this group
    for (cmp=cmp1; cmp<cmp2; cmp++) {
      pcmp = AryPtr(PrmPcmp, cmp);  if (!pcmp)                     eX(106);
      pg = AryPtr(PrmPemg, ig, cmp);  if (!pg)                     eX(43);
      ef = efac*fq*(*pg);
      mptl = pcmp->mptl/MI.usedrfac;
      pe = AryPtr(PrmPems, src, cmp);
      if (ef*(*pe) == 0.0)  n = 0;
      else {
        n = ef*(*pe)*dt/(mptl*numparts) + 0.5;                  //-00-03-23
        if (n < 1)  n = 1;
      }
      if (n > np)  np = n;
      }
    if (np == 0)  continue;
    idgrd = psrc->idgrd;
    if (idgrd) {
      TXTSTR hdr = { NULL, 0 };
      pgarr = TmnAttach(idgrd, &t1, &t2, 0, &hdr);  if (!pgarr) eX(14);
      if (1 > DmnGetFloat(hdr.s, "delta|delt|dd", "%f", &delta, 1))     eX(15);
      psrc->w = delta;
      psrc->l = delta;
      TxtClr(&hdr);
      SrcNumPtl(-1, psrc, pgarr, NULL, NULL, NULL, NULL);       eG(16);
    }
    vLOG(5)("SRC:SrcCrtPtl: source %s with %d particles", psrc->name, np);
    if (np > 0)  fak = dt/(np*numparts);
    else  fak = 0;
    for (cmp=cmp1; cmp<cmp2; cmp++) {
      pg = AryPtr(PrmPemg, ig, cmp);  if (!pg)                     eX(43);
      ef = efac*fq*(*pg);
      i = cmp - cmp1;
      pe = AryPtrX(PrmPems, src, cmp);
      gg[i] = ef*(*pe)*fak;
      }
    xq = psrc->x;
    yq = psrc->y;
    x1 = psrc->x1;  y1 = psrc->y1;  h1 = psrc->h1;
    x2 = psrc->x2;  y2 = psrc->y2;  h2 = psrc->h2;
    aq = psrc->aq;  bq = psrc->w;   cq = psrc->cq;
    lq = psrc->l;
    wq = psrc->w;
    dq = psrc->d;
    hq = psrc->h;
    tq = psrc->t;
    if (x1>-1.e30 && y1>-1.e30 && x2>-1.e30 && y2>-1.e30) {
      xq = 0.5*(x1 + x2);
      yq = 0.5*(y1 + y2);
      dxm = x2 - x1;
      dym = y2 - y1;
      lq = sqrt(dxm*dxm + dym*dym);
      if (lq > 0) {
        si = dxm/lq;
        co = dym/lq; }
      else {
        si = 0;
        co = 1;
      }
      if (psrc->l > 0) {
        lq += psrc->l;
        xq += 0.5*si*psrc->l;
        yq += 0.5*co*psrc->l;
      }
      aq = -999;              //-99-08-29
    }
    else {                    //-99-08-29
      a = dq/RADIAN;
      si = sin(a);
      co = cos(a);
    }
    if (aq >= 0) {
      x0 = xq;
      y0 = yq;
      dx1 = aq*co;
      dy1 = aq*si;
      dx2 = -bq*si;
      dy2 =  bq*co;
      hq += cq;
      dz = cq;
      psrc->si = si;  // mathematical orientation
      psrc->co = co;
    }
    else {
      dx1 = lq*si;
      dy1 = lq*co;
      dx2 = wq*co;
      dy2 = -wq*si;
      xm = xq;
      ym = yq;
      x0 = xm - 0.5*(dx1+dx2);
      y0 = ym - 0.5*(dy1+dy2);
      dz = tq;
      psrc->si = co;
      psrc->co = si;
    }
    afuhgt = 0;
    afutsc = 0;
    if (h1>0  && h2>0) {
      dh = h2 - h1;
      hq = 0.5*(h1 + h2); }
    else {
      h1 = hq;
      h2 = hq;
      dh = 0; }
    if (dz > h1)  dz = h1;
    if (dz > h2)  dz = h2;

vLOG(7)("LSTSRC:  src=%d np=%ld xq=%1.0f yq=%1.0f dt=%1.1f lq=%1.0f wq=%1.0f",
      src, np, xq, yq, dt, lq, wq);
vLOG(7)("LSTSRC:  x0=%1.0f y0=%1.0f dx1=%1.0f dy1=%1.0f dx2=%1.0f dy2=%1.0f",
x0, y0, dx1, dy1, dx2, dy2);

    sumip = 0;
    do {
      if (idgrd) {
        rc = SrcNumPtl(np, psrc, pgarr, &x0, &y0, &h1, &fak);   eG(17);
        ip = rc; }
      else {
        ip = np;
        fak = 1.0; }
      if (dz > hq)  dz = hq;
      for (l=0; l<ip; l++) {
        pptl = PtlNext(handle);  if (!pptl)                     eX(18);
        k++;
        MI.count++;
        sumip++;
        sumptl++;
        r1 = SrcRndEqu01();
        r2 = SrcRndEqu01();
        r3 = SrcRndEqu01();
        pptl->rnd = SrcRndULong();             
        pptl->x = x0 + r1*dx1 + r2*dx2;
        pptl->y = y0 + r1*dy1 + r2*dy2;
        pptl->h = h1 + r1*dh - r3*dz;
        pptl->t = t0 + ((l+0.5)*dt)/ip;
        for (i=0; i<ncmp; i++) {
          pptl->g[2*i]   = gg[i]*fak;                 //-2001-07-02
          pptl->g[2*i+1] = gg[i]*fak;                 //-2001-07-02
        }
        if (ico) {  // set sum odor component         //-2008-03-10
          double so = 0;
          for (io=1; io<no; io++) 
            so += gg[ico[io]];
          io = ico[0];
          pptl->g[2*io]   = so*fak;
          pptl->g[2*io+1] = so*fak;
        }
        pptl->ix = 0;
        pptl->iy = 0;
        pptl->iz = 0;
        pptl->nr = 0;
        pptl->vg = 0;
        pptl->flag = SRC_EXIST | SRC_CREATED;         //-2003-02-21
        pptl->refl = 0;
        pptl->numcmp = ncmp;
        pptl->offcmp = cmp1;
        pptl->srcind = src + 1;
        pptl->afuhgt = afuhgt;
        pptl->afutsc = afutsc;
        if (nadclass > 0) {
          r1 = SrcRndEqu01();
          for (i=0; i<nadclass; i++)
            if (r1 <= addistri[i])  break;
          if (i == 0)  ad = r1*adlimits[0]/addistri[0];
          else {
            adg = (adlimits[i] - adlimits[i-1])/(addistri[i] - addistri[i-1]);
            ad = adlimits[i-1] + (r1 - addistri[i-1])*adg;
            }
          vg = PpmVsed(ad, rho);
          }
        else if (admean > 0) {
          do {
            ad = admean*exp(slogad*SrcRndNrm());
            }  while (ad > 200);
          vg = PpmVsed(ad, rho);
          }
        else  vg = 0;
        pptl->vg = vg;
        }
      } while (sumip < np);
    if (idgrd) {
      TmnDetach(idgrd, &t1, &t2, 0, NULL);                      eG(19);
      }
    }
  PtlEnd(handle);
  FREE(gg);
  FREE(totalems);
  FREE(prelmptl);
  if (ico)  FREE(ico);                                            //-2008-03-10
  TmnDetach(idptl, &t1, &t2, TMN_MODIFY|TMN_UNIQUE, NULL);      eG(21);
  return numptl;
eX_50:  eX_1:  eX_6:  eX_7:
  eMSG(_no_data_);
eX_2:
  eMSG(_invalid_index_$_, aslind);
eX_3:  eX_4:  eX_5:
  eMSG(_no_memory_);
eX_8:
  eMSG(_range_error_$$_, src, cmp);
eX_10: eX_12:
  eMSG(_cant_create_particles_$_, numptl);
eX_11:  eX_21:
  eMSG(_cant_detach_particles_);
eX_13:
  eMSG(_cant_start_chain_);
eX_14:
  eMSG(_no_grid_source_$_, psrc->name);
eX_15:
  eMSG(_missing_delta_$_, psrc->name);
eX_16:
  eMSG(_cant_init_grid_$_, psrc->name);
eX_17:
  eMSG(_no_particle_count_grid_$_, psrc->name);
eX_18:
  eMSG(_no_next_particle_$$_$_, l+1, ip, psrc->name);
eX_19:
  eMSG(_cant_detach_grid_$_, psrc->name);
eX_30: eX_31: eX_32: eX_33:
  eMSG(_range_error_$_, ig);
eX_40: eX_41: eX_42: eX_43:
  eMSG(_range_error_$$_, ig, cmp);
eX_61: eX_62: eX_63:
  eMSG(_cant_analyse_odor_);
eX_101: eX_102: eX_103: eX_104: eX_105: eX_106:
  eMSG(_internal_error_);
  }

/*================================================================== SrcSlcPtl
*/
long SrcSlcPtl(            /* Teilchen auswaehlen                       */
long id,                   /* Ident der Teilchentabelle                 */
int flag,                  /* Zu setzendes Flag                         */
float x1, float x2,        /* Gewuenschter Bereich in x                 */
float y1, float y2,        /* Gewuenschter Bereich in y                 */
float rfac )               /* Ausduennungsfaktor                        */
  {                        /* RETURN: Anzahl der uebertragenen Teilchen */
  dP(SrcSlcPtl);
  PTLREC *pp;
  ARYDSC *pa;
  int np, nd, nc, l, handle, l0, nn;
  float r, *gtotal, *gselect, *gfactor;
  char name[256];						//-2004-11-26
  strcpy(name, NmsName(id));
  vLOG(5)("SRC:SrcSlcPtl(%s,%02x) x(%1.0f,%1.0f) y(%1.0f,%1.0f) r=%5.3f",
    name, flag, x1, x2, y1, y2, rfac);
  setSeed(RndULong());
  nn = MI.sumcmp;
  gtotal  = ALLOC(nn*sizeof(float));
  gselect = ALLOC(nn*sizeof(float));
  gfactor = ALLOC(nn*sizeof(float));
  if (!gtotal || !gselect || !gfactor)                          eX(10); //-2014-06-26
  for (l=0; l<nn; l++) {
    gtotal[l]  = 0;
    gselect[l] = 0;
    gfactor[l] = 1;
    }
  pa = TmnAttach(id, NULL, NULL, TMN_MODIFY, NULL);             eG(1);
  handle = PtlStart(pa);                                        eG(2);
  for (np=0, nd=0; ; ) {
    pp = PtlNext(handle);                                       eG(3);
    if (!pp)  break;
    if (pp->flag & SRC_REMOVE) {
      pp->flag = 0;
      nd++;
      continue; }
    if ( (pp->x < x1) || (pp->x > x2)
      || (pp->y < y1) || (pp->y > y2) )  continue;
    l0 = pp->offcmp;
    nc = pp->numcmp;
    for (l=0; l<nc; l++)  gtotal[l0+l] += pp->g[2*l];           //-2001-07-19
    r = SrcRndEqu01();
    if (r < rfac) {
      np++;
      pp->flag |= flag;
      nc = pp->numcmp;
      for (l=0; l<nc; l++)  gselect[l0+l] += pp->g[2*l];        //-2001-07-19
      }
    else {
      pp->flag = 0;
      nd++;
      }
    }
  PtlEnd(handle);
  for (l=0; l<nn; l++)
    if (gselect[l] > 0)  gfactor[l] = gtotal[l]/gselect[l];
  handle = PtlStart(pa);                                        eG(12);
  for (np=0, nd=0; ; ) {
    pp = PtlNext(handle);                                       eG(13);
    if (!pp)  break;
    if (pp->flag & flag) {
      l0 = pp->offcmp;
      nc = pp->numcmp;
      for (l=0; l<nc; l++) {
        pp->g[2*l]   *= gfactor[l0+l];                            //-2001-07-19
        pp->g[2*l+1] *= gfactor[l0+l];                            //-2001-07-19
      }
    }
  }
  PtlEnd(handle);
  FREE(gtotal);
  FREE(gselect);
  FREE(gfactor);
  TmnDetach(id, NULL, NULL, TMN_MODIFY, NULL);                  eG(4);
  vLOG(5)("SRC:SrcSlcPtl = %d (%d) ", np, nd);
  return np;
eX_10:
  eMSG(_no_memory_);
eX_1:
  eMSG(_cant_attach_particles_$_, NmsName(id));
eX_2: eX_12:
  eMSG(_cant_init_count_$_, NmsName(id));
eX_3: eX_13:
  eMSG(_no_next_particle_$_, NmsName(id));
eX_4:
  eMSG(_cant_detach_particles_$_, NmsName(id));
  }

/*============================================================ SrcReduceAllPtl
*/
int SrcReduceAllPtl(    /* reduce particle number       */
float rfac )            /* reduction factor             */
  {
  dP(SrcReduceAllPtl);
  int gl, gi, gp, idptl, net, ng;
  PTLREC *pp;
  ARYDSC *pa;
  int np, nd, nc, l, handle, l0, nn, flag;
  float r, *gtotal, *gselect, *gfactor;
  vLOG(5)("SRC:SrcReduceAllPtl r=%5.3f", rfac);
  setSeed(RndULong());
  flag = SRC_SELECT;
  nn = MI.sumcmp;
  gtotal  = ALLOC(nn*sizeof(float));
  gselect = ALLOC(nn*sizeof(float));
  gfactor = ALLOC(nn*sizeof(float));
  if (!gtotal || !gselect || !gfactor)                          eX(10); //-2014-06-26
  net = GrdPprm->numnet;
  ng = MI.numgrp;
  do {
    if (net > 0) {
      GrdSetNet(net);                                           eG(1);
      }
    gl = GrdPprm->level;
    gi = GrdPprm->index;
    for (gp=0; gp<ng; gp++) {
      for (l=0; l<nn; l++) {
        gtotal[l]  = 0;
        gselect[l] = 0;
        gfactor[l] = 1;
        }
      idptl = IDENT(PTLarr, gp, gl, gi);
      pa = TmnAttach(idptl, NULL, NULL, TMN_MODIFY, NULL);      eG(2);
      handle = PtlStart(pa);                                    eG(3);
      for (np=0, nd=0; ; ) {
        pp = PtlNext(handle);                                   eG(4);
        if (!pp)  break;
        l0 = pp->offcmp;
        nc = pp->numcmp;
        for (l=0; l<nc; l++)  gtotal[l0+l] += pp->g[2*l];       //-2001-07-19
        r = SrcRndEqu01();
        if (r < rfac) {
          np++;
          pp->flag |= flag;
          nc = pp->numcmp;
          for (l=0; l<nc; l++)  gselect[l0+l] += pp->g[2*l];    //-2001-07-19
          }
        else {
          pp->flag = 0;
          nd++;
          }
        }
      PtlEnd(handle);
      for (l=0; l<nn; l++)
        if (gselect[l] > 0)  gfactor[l] = gtotal[l]/gselect[l];
      handle = PtlStart(pa);                                    eG(5);
      for (np=0, nd=0; ; ) {
        pp = PtlNext(handle);                                   eG(6);
        if (!pp)  break;
        if (pp->flag & flag) {
          l0 = pp->offcmp;
          nc = pp->numcmp;
          for (l=0; l<nc; l++) {
            pp->g[2*l]   *= gfactor[l0+l];                      //-2001-07-19
            pp->g[2*l+1] *= gfactor[l0+l];                      //-2001-07-19
          }
          pp->flag &= ~flag;
        }
      }
      PtlEnd(handle);
      TmnDetach(idptl, NULL, NULL, TMN_MODIFY, NULL);           eG(7);
      PtlTransfer(idptl, 0, 0, 0);                              eG(8);
      }
    net--;
    }  while (net > 0);
  FREE(gtotal);
  FREE(gselect);
  FREE(gfactor);
  return 0;
eX_10:
  eMSG(_no_memory_);
eX_1:
  eMSG(_cant_set_grid_$_, net);
eX_2:
  eMSG(_cant_attach_particles_$_, NmsName(idptl));
eX_3: eX_5:
  eMSG(_cant_init_count_$_, NmsName(idptl));
eX_4: eX_6:
  eMSG(_no_next_particle_$_, NmsName(idptl));
eX_7:
  eMSG(_cant_detach_particles_$_, NmsName(idptl));
eX_8:
  eMSG(_cant_reduce_particles_$_, NmsName(idptl));
  }

/*================================================================== SrcHeader
*/
char *SrcHeader(                /* the header (global storage)  */
  long id,                      /* identification               */
  long *pt1,                    /* start of the validity time   */
  long *pt2 )                   /* end of validity time         */
  {
  /* dP(SRC:SrcHeader); */
  char name[256], t1s[40], t2s[40], s[100+256];
  TXTSTR hdr = { NULL, 0 };
  strcpy(name, NmsName(id));
  strcpy(t1s, TmString(pt1));
  strcpy(t2s, TmString(pt2));
  TxtCpy(&hdr, "\n");
  sprintf(s, "prgm  \"SRC_%d.%d.%s\"", StdVersion, StdRelease, StdPatch); 
  TxtCat(&hdr, s);
  sprintf(s, "name  \"%s\"\n", name);
  TxtCat(&hdr, s);
  sprintf(s, "t1    \"%s\"\n", t1s);
  TxtCat(&hdr, s);
  sprintf(s, "t2    \"%s\"\n", t2s);
  TxtCat(&hdr, s);
  TxtCat(&hdr, "form  \"");  
  TxtCat(&hdr, PtlRecFormat);
  TxtCat(&hdr, "\"\n");
  return hdr.s;
  }

/*==================================================================== SrcInit
*/
long SrcInit(           /* initialize server    */
long flags,             /* action flags         */
char *istr )            /* server options       */
  {
  dP(SrcInit);
  long idptl, mask;
  char *jstr, *ps;
  if (StdStatus & STD_INIT)  return 0;
  if (istr) {
    jstr = istr;
    ps = strstr(istr, "-v");
    if (ps) sscanf(ps+2, "%d", &StdLogLevel);
    ps = strstr(istr, "-y");
    if (ps) sscanf(ps+2, "%d", &StdDspLevel);
    }
  else  jstr = "";
  vLOG(3)("SRC_%d.%d.%s (%08lx,%s)", StdVersion, StdRelease, StdPatch, flags, jstr);
  StdStatus |= flags;
  PtlRecSize = sizeof(PTLREC) + 2*(MI.maxcmp-1)*sizeof(float);          //-2001-07-02
  sprintf(PtlRecFormat, "%sga%%[%d]9.1e",
    "fl%3bxrf%3bxnc%3bdoc%3bdsrc%5ld"
    "x%[3]8.1lfh%8.1lft%9tu%[3]6.2frnd%8ld"
    "ix%[3]4bdnr%4bdrr%7.1fah%6.1fat%6.1frx%[3]6.3f",                   //-2001-03-21 lj
     2*MI.maxcmp);                                                      //-2001-07-02
  idptl = IDENT(PTLarr, 0, 0, 0);
  mask = ~(NMS_GROUP|NMS_GRIDN|NMS_LEVEL);
  TmnCreator(idptl, mask, TMN_UNIQUE, "", SrcServer, SrcHeader);        eG(1);
  StdStatus |= STD_INIT;
  return 0;
eX_1:
  eMSG(_cant_define_creator_$_, NmsName(idptl));
  }

/*================================================================== SrcServer
*/
long SrcServer(
  char *ss )
  {
  dP(SrcServer);
  long rc, id, idptl, idsrc, idgrd, mask, t1, t2, m1, m2, usage;
  int gp, gl, gi, iasl, create, info;
  PTLTAG select;                                                //-2002-12-19
  float x1, x2, y1, y2;
  char tts[40], t1s[40], t2s[40], m1s[40], m2s[40], name[256];	//-2004-11-26
  ARYDSC *pa;
  GRDPARM *pgp;
  if (StdArg(ss))  return 0;
  if (*ss) {
    switch (ss[1]) {
      default:   ;
      }
    return 0;
    }
  if (!StdIdent)                                                eX(30);
  if ((StdStatus & STD_INIT) == 0) {
    SrcInit(0, "");                                             eG(1);
    }
  if (!PMI)                                                     eX(2);
  t1 = MI.simt1;
  t2 = MI.simt2;
  strcpy(t1s, TmString(&t1));
  strcpy(t2s, TmString(&t2));
  if (StdTime<t1 || StdTime>=t2)                                eX(3);
  gp = XTR_GROUP(StdIdent);
  gl = XTR_LEVEL(StdIdent);
  gi = XTR_GRIDN(StdIdent);
  idptl = IDENT(PTLarr, 0, 0, 0);
  mask = ~(NMS_GROUP|NMS_GRIDN|NMS_LEVEL);
  TmnCreator(idptl, mask, 0, "", NULL, NULL);                   eG(4);
  if (gl) {
    idptl = IDENT(PTLarr, gp, gl, gi);
    idsrc = IDENT(XTRarr, gp, 0, 0);
    select = SRC_SELECT;                                        //-2003-08-29
    }
  else {
    idptl = IDENT(PTLarr, gp, 0, 0);
    idsrc = idptl;
    select = 0;
    }
  info = TmnInfo(idsrc, &m1, &m2, &usage, NULL, NULL);
  if ((info) && (usage & TMN_DEFINED)) {
    if (m2 == t1)  create = 1;
    else {
      if (m1!=t1 || m2!=t2)                                     eX(20);
      create = 0;
      }
    }
  else  create = 1;
  if (create) {
    id = TmnIdent();
    for (iasl=0; iasl<MI.numasl; iasl++) {
      rc=SrcCrtPtl(iasl, gp, id);                               eG(5);
      PtlTransfer(id, idsrc, PTL_ANY, PTL_SAVE);                eG(6);
      TmnDelete(t2, id, TMN_NOID);                              eG(7);
      }
    }
  if (select) {
    idgrd = IDENT(GRDpar, 0, gl, gi);
    pa = TmnAttach(idgrd, NULL, NULL, 0, NULL);                 eG(21);
    if (!pa)                                                    eX(22); //-2014-06-26
    pgp = pa->start;
    x1 = pgp->x1;
    x2 = pgp->x2;
    y1 = pgp->y1;
    y2 = pgp->y2;
    SrcSlcPtl(idsrc, select, x1, x2, y1, y2, pgp->rfac);        eG(23);
    PtlTransfer(idsrc, idptl, select, PTL_CLEAR);               eG(24);
    TmnDetach(idgrd, NULL, NULL, 0, NULL);                      eG(25);
    pgp = NULL;
    }
  TmnAttach(idptl, NULL, NULL, TMN_MODIFY, NULL);               eG(10);
  TmnDetach(idptl,  &t1,  &t2, TMN_MODIFY|TMN_UNIQUE, NULL);    eG(11);
  idptl = IDENT(PTLarr, 0, 0, 0);
  TmnCreator(idptl, mask, TMN_UNIQUE, "", SrcServer, SrcHeader);eG(8);
  return 0;
eX_30:  eX_1:
  eMSG(_cant_init_);
eX_2:
  eMSG(_prm_not_initialized_);
eX_3:
  strcpy(tts, TmString(&StdTime));
  eMSG(_improper_time_request_$_$$_, tts, t1s, t2s);
eX_4:  eX_8:
  eMSG(_cant_redefine_);
eX_5:
  eMSG(_cant_create_particles_$$_, iasl, gp);
eX_6:
  eMSG(_cant_transfer_particles_$$_, iasl, gp);
eX_7:
  eMSG(_cant_delete_particles_$$_, iasl, gp);
eX_10: eX_11:
  eMSG(_cant_define_validity_$_, NmsName(idptl));
eX_20:
  strcpy(m1s, TmString(&m1));
  strcpy(m2s, TmString(&m2));
  strcpy(name, NmsName(idsrc));
  eMSG(_expected_$$$_found_$$_, name, t1s, t2s, m1s, m2s);
eX_21: eX_22: eX_25:
  eMSG(_no_grid_$$_, gl, gi);
eX_23:
  strcpy(name, NmsName(idsrc));
  eMSG(_cant_select_particles_$$$_, name, gl, gi);
eX_24:
  strcpy(name, NmsName(idsrc));
  eMSG(_cant_transfer_particles_$$$_, name, gl, gi);
  }

//===========================================================================
//
// history:
// 
// 2002-09-24 lj 1.0.0  final release candidate
// 2003-02-21 lj 1.1.2  private random numbers
// 2003-08-29 lj 1.1.9  bit pattern for selection corrected
// 2004-11-26 lj 2.1.7  string length for names = 256
// 2005-03-17 uj 2.2.0  version number upgrade
// 2006-10-26 lj 2.3.0  external strings
// 2008-03-10 lj 2.4.0  SrcCrtPtl: evaluation of rated odor frequencies
// 2008-04-17 lj 2.4.1  merged with 2.3.x
// 2011-06-29 uj 2.5.0  DMNA header
// 2012-04-06 uj 2.6.0  version number upgrade
// 2014-06-26 uj 2.6.11 eX/eG adjusted
//
//===========================================================================

