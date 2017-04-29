//================================================================ TalDos.c
//
// Calculation of dose field
// =========================
//
// Copyright (C) Umweltbundesamt, Dessau-Roﬂlau, Germany, 2002-2006
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

// last change: 2014-06-26
//
//==========================================================================

#include <math.h>

#define  STDMYMAIN  DosMain
#include "IBJmsg.h"
#include "IBJary.h"
#include "IBJstd.h"
static char *eMODn = "TalDos";

/*=========================================================================*/

STDPGM(tstdos, DosServer, 2, 6, 11);

/*=========================================================================*/

#include "genutl.h"
#include "genio.h"

#include "TalGrd.h"
#include "TalNms.h"
#include "TalPrm.h"
#include "TalBlm.h"
#include "TalPtl.h"
#include "TalSrc.h"
#include "TalTmn.h"
#include "TalWrk.h"
#include "TalMat.h"
#include "TalMod.h"
#include "TalDos.h"
#include "TalDos.nls"

static char DefName[120], T1s[40], T2s[40];
static long T1, T2;


/*====================================================================== DosMap
*/
long DosMap(                            /* Map dose to a different grid */
ARYDSC *pas,                            /* source dose array            */
float x1s, float y1s, float dds,        /* Xmin, Ymin, Dd of source     */
ARYDSC *pad,                            /* destination dose array       */
float x1d, float y1d, float ddd,        /* Xmin, Ymin, Dd of dest.      */
int ng )                                /* number of groups (=0: GMM)   */
  {
  dP(DosMap);
  int nxs, nys, nzs, ixs, iys, iz, ic, iz0;
  int nxd, nyd, nzd, ixd, iyd, nz, nc, nd, dix, diy;
  float alfa, alfa2, a, rng, *pfs, *pfd;
  int scatter;
  if (pas->elmsz != pad->elmsz)                         eX(1);
  if (pas->numdm!=4 || pad->numdm!=4)                   eX(2);
  scatter = (pas->elmsz == 2*sizeof(float));
  rng = 1.0/ng;
  nc = pas->bound[3].hgh;
  iz0 = pas->bound[2].low;
  if (pad->bound[3].hgh!=nc || pad->bound[2].low!=iz0)  eX(3);
  nc++;
  nxs = pas->bound[0].hgh;
  nys = pas->bound[1].hgh;
  nzs = pas->bound[2].hgh;
  nxd = pad->bound[0].hgh;
  nyd = pad->bound[1].hgh;
  nzd = pad->bound[2].hgh;
  nz = MIN(nzs, nzd);
  a = dds/ddd - 1;
  if (ABS(a) < 0.001) {      /* equal mesh size */
    dix = (x1d < x1s) ? -(int)((x1s-x1d)/dds+0.5) : (int)((x1d-x1s)/dds+0.5);
    diy = (y1d < y1s) ? -(int)((y1s-y1d)/dds+0.5) : (int)((y1d-y1s)/dds+0.5);
    ixs = MAX(1, dix+1);
    for ( ; ixs<=nxs; ixs++) {
      ixd = ixs - dix;
      if (ixd > nxd)  break;
      iys = MAX(1, diy+1);
      for ( ; iys<=nys; iys++) {
        iyd = iys - diy;
        if (iyd > nyd)  break;
        pfs = AryPtr(pas, ixs, iys, iz0, 0);  if (!pfs)    eX(10);
        pfd = AryPtr(pad, ixd, iyd, iz0, 0);  if (!pfd)    eX(11);
        for (iz=iz0; iz<=nz; iz++)
          for (ic=0; ic<nc; ic++) {
            if (scatter) {
              a = 2*(*pfs)*(*pfd)*rng;
              *pfd++ += *pfs++;
              *pfd++ += *pfs++ + a;
              }
            else  *pfd++ += *pfs++;
            }
        }
      }
    return 0;
    }
  if (dds < ddd) {         /* map into a wider grid */
    nd = ddd/dds + 0.5;
    a = ddd/dds - nd;
    if (ABS(a) > 0.001)                                 eX(4);
    dix = (x1d < x1s) ? -(int)((x1s-x1d)/dds+0.5) : (int)((x1d-x1s)/dds+0.5);
    diy = (y1d < y1s) ? -(int)((y1s-y1d)/dds+0.5) : (int)((y1d-y1s)/dds+0.5);
    ixs = MAX( 1, dix+1 );
    for ( ; ixs<=nxs; ixs++) {
      ixd = 1 + (ixs - dix - 1)/nd;
      if (ixd > nxd)  break;
      iys = MAX( 1, diy+1 );
      for ( ; iys<=nys; iys++) {
        iyd = 1 + (iys - diy - 1)/nd;
        if (iyd > nyd)  break;
        pfs = AryPtr(pas, ixs, iys, iz0, 0);  if (!pfs)    eX(20);
        pfd = AryPtr(pad, ixd, iyd, iz0, 0);  if (!pfd)    eX(21);
        for (iz=iz0; iz<=nz; iz++)
          for (ic=0; ic<nc; ic++) {
            if (scatter) {
              a = 2*(*pfs)*(*pfd)*rng;
              *pfd++ += *pfs++;
              *pfd++ += *pfs++ + a;
              }
            else  *pfd++ += *pfs++;
            }
        }
      }
    return 0;
    }
  nd = dds/ddd + 0.5;       /* map into a narrow grid */
  a = dds/ddd - nd;
  if (ABS(a) > 0.001)                                   eX(5);
  if (ng > 0)  alfa = ddd/dds;  /* mapping of dose field        */
  else  alfa = 1.0;             /* if used for gamma-submersion */
  alfa *= alfa;
  alfa2 = alfa*alfa;
  dix = (x1d < x1s) ? (int)((x1s-x1d)/ddd+0.5) : -(int)((x1d-x1s)/ddd+0.5);
  diy = (y1d < y1s) ? (int)((y1s-y1d)/ddd+0.5) : -(int)((y1d-y1s)/ddd+0.5);
  ixd = MAX( 1, dix+1 );
  for ( ; ixd<=nxd; ixd++) {
    ixs = 1 + (ixd - dix - 1)/nd;
    if (ixs > nxs)  break;
    iyd = MAX(1, diy+1);
    for ( ; iyd<=nyd; iyd++) {
      iys = 1 + (iyd - diy - 1)/nd;
      if (iys > nys)  break;
      pfs = AryPtr(pas, ixs, iys, iz0, 0);  if (!pfs)      eX(30);
      pfd = AryPtr(pad, ixd, iyd, iz0, 0);  if (!pfd)      eX(31);
      for (iz=iz0; iz<=nz; iz++)
        for (ic=0; ic<nc; ic++) {
          if (scatter) {
            a = 2*alfa*(*pfs)*(*pfd)*rng;
            *pfd++ += alfa*(*pfs++);
            *pfd++ += alfa2*(*pfs++) + a;
            }
          else  *pfd++ += alfa*(*pfs++);
          }
      }
    }
  return 0;
eX_1:  eX_2:  eX_3:  eX_4:  eX_5:
  eMSG(_incompatible_field_structure_);
eX_10: eX_11: eX_20: eX_21: eX_30: eX_31:
  eMSG(_index_range_);
  }

/*================================================================== MapAllDos
*/
static long MapAllDos(       /* mapping of dose fields */
void )
  {
  dP(MapAllDos);
  char name[256], m1s[40], m2s[40];
  int netsrc, netdst, srcnl, srcni, dstnl, dstni, nxtnl;
  int nx, ny, nz, nc, ng, nzdos, nzmap, numnet;
  int i, j, k, l;
  long srcid, sz, dstid, sumid, m1, m2, usage, mode;
  float srcxmin, srcymin, srcdd, dstxmin, dstymin, dstdd;
  float *ps, *pd;
  int do_gamma, scatter;
  ARYDSC *psrc, *pdst, *psum;
  numnet = GrdPprm->numnet;
  if (numnet < 1)  return 0;
  vLOG(4) ("DOS:MapAllDos()  mapping of concentration fields" );
  do_gamma = ((MI.flags & FLG_GAMMA) != 0);
  scatter  = (MI.flags & FLG_SCAT) && (MI.numgrp > 4);
  ng = MI.numgrp;
  nc = MI.sumcmp;
  for (netdst=1; netdst<=numnet; netdst++) {
    GrdSetNet(netdst);                                                  eG(1);
    dstnl = GrdPprm->level;
    dstni = GrdPprm->index;
    dstdd = GrdPprm->dd;
    dstxmin = GrdPprm->xmin;
    dstymin = GrdPprm->ymin;
    nx = GrdPprm->nx;
    ny = GrdPprm->ny;
    nz = GrdPprm->nz;
    nzdos = GrdPprm->nzdos;
    nzmap = GrdPprm->nzmap;
    if (do_gamma)  nz = MIN(nz, nzmap);
    else  nz = MIN(nz, nzdos);
    dstid = IDENT(SUMarr, 0, dstnl, dstni);
    sz = (1 + scatter)*sizeof(float);
    pdst = TmnCreate(dstid, sz, 4, 1, nx, 1, ny, -1, nz, 0, nc-1);      eG(2);
    for (netsrc=numnet; netsrc>=1; netsrc--) {
      GrdSetNet(netsrc);                                                eG(3);
      srcnl = GrdPprm->level;
      srcni = GrdPprm->index;
      srcdd = GrdPprm->dd;
      srcxmin = GrdPprm->xmin;
      srcymin = GrdPprm->ymin;
      if (netsrc == 1)  nxtnl = 0;
      else {
        GrdSetNet(netsrc-1);                                            eG(4);
        nxtnl = GrdPprm->level;
        }
      srcid = IDENT(SUMarr, 2, srcnl, srcni);
      psrc = TmnAttach(srcid, &T1, &T2, 0, NULL);                       eG(5);
      TmnInfo(srcid, NULL, NULL, &mode, NULL, NULL);
      mode &= TMN_INVALID;
      DosMap(psrc, srcxmin, srcymin, srcdd,
             pdst, dstxmin, dstymin, dstdd, ng);                        eG(6);
      if ((srcnl == dstnl) && (nxtnl < dstnl) && do_gamma) {
        sumid = IDENT(SUMarr, 1, dstnl, dstni);
        TmnInfo(sumid, &m1, &m2, &usage, NULL, NULL);
        if (!(usage & TMN_DEFINED)) {
          m1 = T1;
          m2 = T2;
          psum = TmnCreate(sumid, sz, 4, 1, nx, 1, ny, -1, nz, 0, nc-1);eG(7);
          memcpy(psum->start, pdst->start, pdst->ttlsz);
          }
        else {
          if (m2 != T1)                                                 eX(11);
          m2 = T2;
          for (i=1; i<=nx; i++)
            for (j=1; j<=ny; j++)
              for (k=-1; k<=nz; k++)
                for (l=0; l<nc; l++) {
                  ps = AryPtr(psum, i, j, k, l);  if (!ps)              eX(12);
                  pd = AryPtr(pdst, i, j, k, l);  if (!pd)              eX(13);
                  *ps += *pd;
                  }
          }
        TmnDetach(sumid, &m1, &m2, TMN_MODIFY|mode, NULL);              eG(8);
        }
      TmnDetach(srcid, &T1, &T2, 0, NULL);                              eG(9);
      }  /* for netsrc */
    TmnDetach(dstid, &T1, &T2, TMN_MODIFY|mode, NULL);                  eG(10);
    }  /* for netdst */
  vDSP(1) ("");
  return 0;
eX_1:
  eMSG(_cant_set_net_$_, netdst);
eX_2:  eX_10:
  strcpy(name, NmsName(dstid));
  eMSG(_cant_create_$_, name);
eX_3:
  eMSG(_cant_set_net_$_, netsrc);
eX_4:
  eMSG(_cant_set_net_$_, netsrc-1);
eX_5:  eX_9:
  strcpy(name, NmsName(srcid));
  eMSG(_$_not_available_, name);
eX_6:
  strcpy(name, NmsName(srcid));
  eMSG(_cant_map_$$_, name, NmsName(dstid));
eX_7:  eX_8:
  strcpy(name, NmsName(sumid));
  eMSG(_cant_create_$_, name);
eX_11:
  strcpy(m1s, TmString(&m1));
  strcpy(m2s, TmString(&m2));
  eMSG(_found_$$$$$_, NmsName(sumid), m1s, m2s, T1s, T2s);
eX_12: eX_13:
  eMSG(_index_range_$$$$_, i, j, k, l);
  }

/*==================================================================== SumDos
*/
static long SumDos( ARYDSC *pdos, int scatter, int invalid )
  {
  dP(SumDos);
  int i, j, k, l, nx, ny, nz, nc, nf;
  int gl, gi, gp, sz, info;
  float *ps, *pd;
  long isum, usage, t1, t2;
  char t1s[40], t2s[40], m1s[40], m2s[40], name[256];
  ARYDSC *psum;
  strcpy(t1s, TmString(&T1));
  strcpy(t2s, TmString(&T2));
  gl = GrdPprm->level;
  gi = GrdPprm->index;
  nx = GrdPprm->nx;
  ny = GrdPprm->ny;
  nz = pdos->bound[2].hgh;
  nc = MI.sumcmp;
  nf = 1 + scatter;
  gp = (gl) ? 2 : 0;
  isum = IDENT(SUMarr, gp, gl, gi);
  strcpy(name, NmsName(isum));
  vLOG(4)("DOS:SumDos()  %s[%s,%s]", name, t1s, t2s);
  sz = sizeof(float)*nf;
  info = TmnInfo(isum, &t1, &t2, &usage, NULL, NULL);
  if ((info) && (usage & TMN_DEFINED)) {
    if (t1!=T1 || t2!=T2)                                               eX(1);
    psum = TmnAttach(isum, &T1, &T2, TMN_MODIFY, NULL);                 eG(2);
    if (!psum)                                                          eX(3); //-2014-06-26
    }
  else {
    psum = TmnCreate(isum, sz, 4, 1, nx, 1, ny, -1, nz, 0, nc-1);       eG(4);
    }
  pd = pdos->start;
  ps = psum->start;
  for (i=1; i<=nx; i++)
    for (j=1; j<=ny; j++)
      for (k=-1; k<=nz; k++)
        for (l=0; l<nc; l++) {
          *ps += *pd;
          ps++;
          if (scatter)  *ps++ += (*pd)*(*pd);
          pd++;
          }
  TmnDetach(isum, &T1, &T2, TMN_MODIFY|invalid, NULL);                eG(5);
  return 0;
eX_1:
  strcpy(t1s, TmString(&T1));
  strcpy(t2s, TmString(&T2));
  strcpy(m1s, TmString(&t1));
  strcpy(m2s, TmString(&t2));
  eMSG(_searching_$$$_found_$$_, name, t1s, t2s, m1s, m2s);
eX_2:  eX_3:
  strcpy(t1s, TmString(&T1));
  strcpy(t2s, TmString(&T2));
  eMSG(_cant_attach_$$$_, name, t1s, t2s);
eX_4:  eX_5:
  eMSG(_cant_create_$_, name);
  }

/*===================================================================== ClcSum
*/
static long ClcSum(     /* sum over samples     */
void )
  {
  dP(ClcSum);
  char t1s[40], t2s[40], name[256];
  int scatter, net, numnet, gl, gi, gp;
  long iprmp, iptla, ixtra, imoda, idosa, iprfa;                  //-2006-02-15
  long t1, t2, dost2, modt2, m1, m2, usage, invalid;
  ARYDSC *pdosa;
  vLOG(4)("DOS:ClcSum() summing partial doses");
  iprmp = IDENT(PRMpar, 0, 0, 0);
  numnet = GrdPprm->numnet;
  strcpy(t1s, TmString(&T1));
  strcpy(t2s, TmString(&T2));
  t1 = T1;
  MI.simt1 = T1;
  MI.dost1 = T1;
  MI.simt2 = T2;
  MI.dost2 = T2;
  scatter = (MI.flags & FLG_SCAT) && (MI.numgrp > 4);
  do {
    strcpy(t1s, TmString(&t1));
    t2 = t1;
    TmnVerify(iprmp, &t1, &t2, 0);                              eG(4);
    if (t2 < MI.simt2)  MI.simt2 = t2;
    strcpy(t2s, TmString(&t2));
    if (MI.simt2 <= MI.simt1)                                   eX(5);
    dost2 = t1;
    modt2 = t1;
    net = numnet;
    do {
      if (net > 0) {
        GrdSetNet(net);                                         eG(6);
        }
      gl = GrdPprm->level;
      gi = GrdPprm->index;
      imoda = IDENT(MODarr, 0, gl, gi);
      iprfa = IDENT(PRFarr, 0, gl, gi);                           //-2006-02-15
      t2 = t1;
// vLOG(4)("DOS: checking MODarr");
      TmnVerify(imoda, &t1, &t2, 0);                            eG(3);
// vLOG(4)("DOS: MODarr available");
      if (modt2 == t1)  modt2 = t2;
      else {
        if (modt2 != t2)                                        eX(20); //-2014-06-26
        }
      if (t2 < MI.simt2)  MI.simt2 = t2;
      for (gp=0; gp<MI.numgrp; gp++) {
        iptla = IDENT(PTLarr, gp, gl, gi);
        TmnInfo(iptla, &m1, &m2, &usage, NULL, NULL);
        if ((usage & TMN_DEFINED) && (m1 > t1))  continue;
        TmnVerify(iptla, &t1, &MI.simt2, 0);                    eG(7);
        idosa = IDENT(DOSarr, gp, gl, gi);
        t2 = t1;
        pdosa = TmnAttach(idosa, &t1, &t2, 0, NULL);            eG(8);
        if (!pdosa)                                             eX(9);
        TmnInfo(idosa, NULL, NULL, &invalid, NULL, NULL);               //-2001-02-19 lj
        invalid &= TMN_INVALID;
        strcpy(t2s, TmString(&t2));
        if (dost2 == t1)  dost2 = t2;
        else {
          if (t2 != dost2)                                      eX(10);
          }
        if (t2 >= T2) {
          SumDos(pdosa, scatter, invalid);                      eG(11); //-2001-02-19 lj
          TmnDetach(idosa, NULL, NULL, 0, NULL);                eG(12);
          TmnClear(t2, idosa, TMN_NOID);                        eG(13);
          }
        else {
          TmnDetach(idosa, NULL, NULL, 0, NULL);                eG(14);
          }
        if (net>0 && GrdNxtLevel(gl)>0) {
          ixtra = IDENT(XTRarr, gp, 0, 0); /* ??? erst vor Beginn des n√§chsten Levels? */
          PtlTransfer(iptla, ixtra, SRC_GOUT, PTL_CLEAR);       eG(15);
          }
        PtlTransfer(iptla, 0, SRC_EOUT, 0);                     eG(16);
        if (t2 == T2) {
          TmnAttach(iptla, NULL, NULL, TMN_MODIFY, NULL);       eG(17);
          TmnDetach(iptla,  &T2,  &T2, TMN_MODIFY, NULL);       eG(18);
          }
        }
      //
      TmnClear(t2, imoda, iprfa, TMN_NOID);                     eG(23); //-2006-02-15
      //
      net--;
      }  while (net > 0);

    if (numnet > 0) {
      for (gp=0; gp<MI.numgrp; gp++) {
        ixtra = IDENT(XTRarr, gp, 0, 0);
        TmnAttach(ixtra, NULL, NULL, TMN_MODIFY, NULL);         eG(21);
        TmnDetach(ixtra, &t2,  NULL, TMN_MODIFY, NULL);         eG(22);
        }
      }

    if (t2 < T2) {
      t1 = t2;
      if (MI.simt2 == t2) {
        MI.simt1 = t1;
        MI.simt2 = T2;
        }
      }
    }  while (t2 < T2);
  if (numnet > 0)
    for (gp=0; gp<MI.numgrp; gp++) {
      ixtra = IDENT(XTRarr, gp, 0, 0);
      PtlTransfer(ixtra, 0, SRC_EXIST, 0);                      eG(19); //-2002-06-26
      }
  vLOG(4)("DOS:ClcSum returning");
  return 0;
eX_3:
  strcpy(name, NmsName(imoda));
  eMSG(_cant_provide_model_field_$$_, name, t1s);
eX_4:
  strcpy(name, NmsName(iprmp));
  eMSG(_cant_provide_parameters_$$_, name, t1s);
eX_5:
  strcpy(t1s, TmString(&MI.simt1));
  strcpy(t2s, TmString(&MI.simt2));
  eMSG(_invalid_times_$$_, t1s, t2s);
eX_6:
  eMSG(_cant_set_net_$_, net);
eX_7:
  strcpy(name, NmsName(iptla));
  strcpy(t1s, TmString(&t1));
  strcpy(t2s, TmString(&MI.simt2));
  eMSG(_cant_provide_particles_$$$_, name, t1s, t2s);
eX_8:  eX_9:
  strcpy(name, NmsName(idosa));
  strcpy(t1s, TmString(&t1));
  eMSG(_cant_get_sample_$$_, name, t1s);
eX_10:
  strcpy(name, NmsName(idosa));
  eMSG(_$_inconsistent_time_$$_, name, t2s, TmString(&dost2));
eX_11: eX_12: eX_13:
  strcpy(name, NmsName(idosa));
  eMSG(_cant_sum_$$$_, name, t1s, t2s);
eX_14:
  strcpy(name, NmsName(idosa));
  eMSG(_cant_detach_$$$_, name, t1s, t2s);
eX_15: eX_16:
  strcpy(name, NmsName(iptla));
  eMSG(_cant_extract_particles_$$$_, name, t1s, t2s);
eX_17: eX_18:
  strcpy(name, NmsName(iptla));
  eMSG(_cant_adjust_time_$$$_, name, t1s, t2s);
eX_19:
  strcpy(name, NmsName(iptla));
  eMSG(_cant_clear_particles_$$_, name, t2s);
eX_20:
  strcpy(t2s, TmString(&t2));
  strcpy(name, NmsName(imoda));
  eMSG(_$_inconsistent_time_$$_, name, t2s, TmString(&modt2));
eX_21: eX_22:
  eMSG(_cant_change_time_$_, t2s);
eX_23:
  eMSG(_cant_clear_arrays_);
 }

/*================================================================= RdcDos
*/
static long RdcDos(     /* reduce vertical size */
int ga )
  {
  dP(RdcDos);
  long idos, t1, t2, isum, mode;
  int net, gl, gi, nx, ny, nz, nc, sz, i, j, ll, nzdos;
  ARYDSC *pdos, *psum;
  void *ps, *pd;
  char name[256];
  vLOG(4)("DOS:RdcDos(%d)  reducing vertical extension", ga);
  net = GrdPprm->numnet;
  if (!net) {
    isum = IDENT(SUMarr,  0, 0, 0);
    idos = IDENT(DOStab, ga, 0, 0);
    TmnRename(isum, idos);                                      eG(1);
    return 0;
    }
  do {
    GrdSetNet(net);                                             eG(2);
    gl = GrdPprm->level;
    gi = GrdPprm->index;
    nzdos = GrdPprm->nzdos;
    isum = IDENT(SUMarr,  0, gl, gi);
    strcpy(name, NmsName(isum));
    idos = IDENT(DOStab, ga, gl, gi);
    psum = TmnAttach(isum, NULL, NULL, 0, NULL);  if (!psum)    eX(3); //-2014-06-26
    nz = psum->bound[2].hgh;
    if (nz <= nzdos) {
      TmnDetach(isum, NULL, NULL, 0, NULL);                     eG(14);
      TmnRename(isum, idos);                                    eG(4);
      }
    else {
      TmnInfo(isum, &t1, &t2, &mode, NULL, NULL);
      nx = psum->bound[0].hgh;
      ny = psum->bound[1].hgh;
      nc = psum->bound[3].hgh + 1;
      sz = psum->elmsz;
      ll = sz*nc*(nzdos+2);
      pdos = TmnCreate(idos, sz, 4, 1, nx, 1, ny, -1, nzdos, 0, nc-1);  eG(5);
      for (i=1; i<=nx; i++)
        for (j=1; j<=ny; j++) {
          ps = AryPtr(psum, i, j, -1, 0);  if (!ps)                     eX(6);
          pd = AryPtr(pdos, i, j, -1, 0);  if (!pd)                     eX(7);
          memcpy(pd, ps, ll);
          }
      mode = (mode&TMN_INVALID) | TMN_MODIFY;
      TmnDetach(idos, &t1, &t2, mode, NULL);                            eG(8);
      TmnDetach(isum, NULL, NULL, 0, NULL);                             eG(9);
      TmnClear(t2, isum, TMN_NOID);                                     eG(10);
      }
    net--;
    } while (net > 0);
  return 0;
eX_1:  eX_4:
  strcpy(name, NmsName(isum));
  eMSG(_cant_rename_$_, name);
eX_2:
  eMSG(_cant_set_net_$_, net);
eX_3:
  eMSG(_cant_get_$_, name);
eX_5:
  eMSG(_cant_create_$_, NmsName(idos));
eX_6:  eX_7:
  eMSG(_index_error_$$_, i, j);
eX_8:
  eMSG(_cant_detach_$_, NmsName(idos));
eX_9:  eX_10:
  eMSG(_cant_clear_$_, name);
eX_14:
  eMSG(_cant_detach_$_, NmsName(isum));
  }

/*================================================================= ClcAverage
*/
static long ClcAverage(         /* calculate continued average  */
long isum )
  {
  dP(ClcAverage);
  long idos, m1, m2, usage, invalid;
  int gl, gi, nx, ny, nz, nc, sz, scatter;
  int n, nn;
  char m1s[40], m2s[40], name[256];				//-2004-11-26
  ARYDSC *pdos, *psum;
  float *pd, *ps, a, rng;
  strcpy(name, NmsName(isum));
  vLOG(4)("DOS:ClcAverage(%d)  continue %s for [%s,%s]",
    isum, name, T1s, T2s);
  gl = XTR_LEVEL(isum);
  gi = XTR_GRIDN(isum);
  idos = IDENT(DOStab, 1, gl, gi);
  TmnInfo(idos, &m1, &m2, &usage, NULL, NULL);
  invalid = usage & TMN_INVALID;
  if (!(usage & TMN_DEFINED))                                   eX(2);
  if (m1!=T1 || m2!=T2)                                         eX(3);
  pdos = TmnAttach(idos, &m1, &m2, 0, NULL);  if (!pdos)        eX(4);
  sz = pdos->elmsz;
  scatter = (sz == 2*sizeof(float));
  nx = pdos->bound[0].hgh;
  ny = pdos->bound[1].hgh;
  nz = pdos->bound[2].hgh;
  nc = pdos->bound[3].hgh + 1;
  TmnInfo(isum, &m1, &m2, &usage, NULL, NULL);
  if (usage & TMN_DEFINED) {
    psum = TmnAttach(isum, &m1, &m2, TMN_MODIFY, NULL);         eG(5);
    if (m2 != T1)                                               eX(6);
    AryAssert(psum, sz, 4, 1, nx, 1, ny, -1, nz, 0, nc-1);      eG(7);
    }
  else {
    psum = TmnCreate(isum, sz, 4, 1, nx, 1, ny, -1, nz, 0, nc-1);       eG(8);
    m1 = T1;
    }
  rng = (MI.flags & FLG_SCAT) ? 1.0/MI.numgrp : 1.0;
  nn = psum->ttlsz/sz;
  ps = psum->start;
  pd = pdos->start;
  for (n=0; n<nn; n++) {
    if (scatter) {
      a = 2*(*ps)*(*pd)*rng;
      *ps++ += *pd++;
      *ps++ += *pd++ + a;
      }
    else  *ps++ += *pd++;
    }
  TmnDetach(isum, &m1, &T2, TMN_MODIFY|invalid, NULL);          eG(9);
  TmnDetach(idos, NULL, NULL, 0, NULL);                         eG(10);
  return 0;
eX_2:  eX_4:
  eMSG(_dose_$_undefined_, NmsName(idos));
eX_3:
  strcpy(m1s, TmString(&m1));
  strcpy(m2s, TmString(&m2));
  strcpy(name, NmsName(idos));
  eMSG(_expected_$$$_found_$$_, name, T1s, T2s, m1s, m2s);
eX_5:
  eMSG(_cant_get_$$$_, name, m1s, m2s);
eX_6:
  strcpy(m1s, TmString(&m1));
  strcpy(m2s, TmString(&m2));
  eMSG(_expected_$$$_found_$$_, name, m1s, T2s, m1s, m2s);
eX_7:
  eMSG(_improper_structure_$$$_, name, m1s, m2s);
eX_8:
  eMSG(_cant_create_$$$_, name, T1s, T2s);
eX_9:
  eMSG(_cant_detach_$$$_, name, T1s, T2s);
eX_10:
  strcpy(name, NmsName(idos));
  eMSG(_cant_detach_$$$_, name, T1s, T2s);
  }

/*==================================================================== DosInit
*/
long DosInit(           /* initialize server    */
long flags,             /* action flags         */
char *istr )            /* server options       */
  {
  dP(DosInit);
  long mask, idos;
  char *jstr, *ps, s[200], name[256];
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
  StdStatus |= flags;
  vLOG(3)("DOS_%d.%d.%s (%08lx,%s)", StdVersion, StdRelease, StdPatch, flags, jstr);
  idos = IDENT(DOStab, 0, 0, 0);
  mask = ~(NMS_GROUP|NMS_GRIDN|NMS_LEVEL);
  sprintf(name, "%s", NmsName(idos));                             //-2011-11-21
  TmnCreator(idos, mask, TMN_UNIQUE, "", DosServer, WrkHeader);         eG(1);
  sprintf(s, " GRD -v%d -d%s", StdLogLevel, DefName);
  GrdInit(flags, s);                                                    eG(2);
  TmnVerify(IDENT(GRDpar,0,0,0), NULL, NULL, 0);                        eG(3);
  sprintf(s, " PRM -v%d -d%s", StdLogLevel, DefName);
  PrmInit(flags, s);                                                    eG(4);
  sprintf(s, " PTL -v%d -d%s", StdLogLevel, DefName);
  PtlInit(flags, s);                                                    eG(5);
  sprintf(s, " SRC -v%d -d%s", StdLogLevel, DefName);
  SrcInit(flags, s);                                                    eG(6);
  sprintf(s, " MOD -v%d -d%s", StdLogLevel, DefName);
  ModInit(flags, s);                                                    eG(7);
  sprintf(s, " WRK -v%d -d%s", StdLogLevel, DefName);
  WrkInit(flags, s);                                                    eG(8);
  StdStatus |= STD_INIT;
  return 0;
eX_1:
  eMSG(_cant_define_creator_$_, name);
eX_2:  eX_3:  eX_4:  eX_5:  eX_6:  eX_7:  eX_8:
  eMSG(_cant_init_servers_);
  }

/*================================================================= SetOdorHour
*/
static int SetOdorHour( void ) {   // calculate odor hour probability  //-2011-07-19
  dP(SetOdorHour);
  long icmp, idos, ivol, ipnt, iblp;                            //-2006-02-22
  ARYDSC *pcmp, *pdos, *pvol, *ppnt, *pblp;                     //-2006-02-22
  int net, sz, nx, ny, nz, nc, scatter, ic, ix, iy, iz, ngp, gp;
  long m1, m2, usage, invalid;
  float bs = MI.threshold, bsmod;
  float dos, dev, dosa, deva, a, as, sa2, vol, c, sc, sc2, w2, sg;
  float *pd, *pv;
  CMPREC *pc;
  BLMPARM *pbp;                                                 //-2006-02-22
  int odstep = 0;                                               //-2005-03-16
  odstep = (MI.flags & FLG_ODMOD) == 0;                         //-2005-03-16
  w2 = sqrt(2.0);
  icmp = IDENT(CMPtab, 0, 0, 0);
  pcmp = TmnAttach(icmp, NULL, NULL, 0, NULL);                      eG(1);
  ngp = MI.numgrp;
  gp = (MI.average > 1);
  //
  iblp = IDENT(BLMpar, 0, 0, 0);                                //-2006-02-22
  pblp =  TmnAttach(iblp, NULL, NULL, 0, NULL);  if (!pblp)          eX(22);
  pbp = (BLMPARM*) pblp->start;
  sg = pbp->StatWeight;
  TmnDetach(iblp, NULL, NULL, 0, NULL);                              eG(26);
  if (sg <= 0)                                                       eX(24);
  if ((MI.flags & FLG_MAXIMA) && sg != 1.)                           eX(25);
  net = GrdPprm->numnet;
  do {
    if (net > 0) {
      GrdSetNet(net);                                               eG(2);
      }
    idos = IDENT(DOStab, gp, GrdPprm->level, GrdPprm->index);
    TmnInfo(idos, &m1, &m2, &usage, NULL, NULL);
    invalid = usage & TMN_INVALID;
    if (invalid)  return 0;
    if (!(usage & TMN_DEFINED))                                     eX(3);
    if (m1!=T1 || m2!=T2)                                           eX(4);
    pdos = TmnAttach(idos, &m1, &m2, TMN_MODIFY, NULL);  if (!pdos) eX(5);
    //
    // workaround for error in GrdServer
    //
    ipnt = IDENT(GRDarr, GRD_IZP, GrdPprm->level, GrdPprm->index);
    ppnt = TmnAttach(ipnt, NULL, NULL, 0, NULL);  if (!ppnt)        eX(16);
    TmnDetach(ipnt, NULL, NULL, 0, NULL);                           eG(17);
    //
    ivol = IDENT(GRDarr, GRD_IVM, GrdPprm->level, GrdPprm->index);
    pvol = TmnAttach(ivol, NULL, NULL, 0, NULL);  if (!pvol)        eX(6);
    sz = pdos->elmsz;
    scatter = (sz == 2*sizeof(float));
    nx = pdos->bound[0].hgh;
    ny = pdos->bound[1].hgh;
    nz = pdos->bound[2].hgh;
    nc = pdos->bound[3].hgh + 1;
    for (ic=0; ic<nc; ic++) {
      pc = AryPtr(pcmp, ic);  if (pc == NULL)                       eX(7); //-2014-06-26
      if (strcmp(pc->name, "gas.odor") && strncmp(pc->name, "gas.odor_", 9))
        continue;                                               //-2008-03-10
      for (ix=1; ix<=nx; ix++) {
        for (iy=1; iy<=ny; iy++) {
          for (iz=1; iz<=nz; iz++) {
            pd = AryPtr(pdos, ix, iy, iz, ic);  if (pd == NULL)     eX(8);
            dos = pd[0];
            pv = AryPtr(pvol, ix, iy, iz);  if (pv == NULL)         eX(9);
            vol = (*pv)*(T2 - T1);
            c = dos/(sg*vol);
            sc = 0;
            bsmod = bs;                                         //-2005-03-11
            if (scatter) {
              dev = pd[1];
              sc2 = (ngp*dev - dos*dos)/((ngp-1)*vol*vol);
              if (sc2 <= 0)  sc = 0;
              else sc = sqrt(sc2)/sg;
            }
            if (c >= bsmod) {                                   //-2005-03-11
              if (sc == 0)  a = 1;                    //-2004-10-04
              else a = 1 - 0.5*Erfc((c-bs)/(w2*sc));
              if (odstep)  as = 1;
              else as = a;
            }
            else {
              if (sc == 0)  a = 0;                    //-2004-10-04
              else a = 0.5*Erfc((bs-c)/(w2*sc));
              if (odstep)  as = 0;
              else  as = a;
            }
            if (a < 1.e-3)  a = 0;                              //-2004-08-31
            sa2 = a*(1 - a);
            dosa = as*vol*sg;
            deva = sg*sg*vol*vol*((ngp-1)*sa2 + as*as)/ngp;
            pd[0] = dosa;
            if (scatter)  pd[1] = deva;
          }
        }
      }
    }
    TmnDetach(idos, NULL, NULL, TMN_MODIFY, NULL);                  eG(10);
    net--;
    } while (net > 0);
  TmnDetach(icmp, NULL, NULL, 0, NULL);                             eG(11)
  TmnDetach(ivol, NULL, NULL, 0, NULL);                             eG(12);
  return 0;
eX_4:
{ char t1s[40], t2s[40], m1s[40], m2s[40], name[256];		//-2004-11-26
  strcpy(m1s, TmString(&m1));
  strcpy(m2s, TmString(&m2));
  strcpy(t1s, TmString(&T1));
  strcpy(t2s, TmString(&T2));
  strcpy(name, NmsName(idos));
  eMSG(_expected_$$$_found_$$_, name, t1s, t2s, m1s, m2s);
}
eX_1:  eX_2:  eX_3:  eX_5:  eX_6:  eX_7:  eX_8:  eX_9:
eX_10: eX_11: eX_12: eX_16: eX_17:
eX_22: eX_24: eX_25: eX_26:
  eMSG(_internal_error_);
}

/*================================================================= DosServer
*/
long DosServer(         /* server routine for DOStab    */
  char *s )             /* calling option               */
  {
  dP(DosServer);
  char m1s[40], m2s[40], name[256];
  int gl, gi, gp, ga, net, numnet;
  long m1, m2, mask, usage, isum, idos;
  enum DATA_TYPE dt;
  if (StdArg(s))  return 0;
  if (*s) {
    switch (s[1]) {
      case 'd': strcpy(DefName, s+2);
                break;
      case 'J': MI.flags |= FLG_TRACING;
                break;
      default:  ;
      }
    return 0;
    }
  if (!StdIdent)                                                eX(10);
  dt = XTR_DTYPE(StdIdent);
  gl = XTR_LEVEL(StdIdent);
  gi = XTR_GRIDN(StdIdent);
  gp = XTR_GROUP(StdIdent);
  if (StdStatus & STD_TIME)  T1 = StdTime;
  else                                                          eX(1);
  numnet = GrdPprm->numnet;
  strcpy(T1s, TmString(&T1));
  T2 = T1 + MI.cycle;
  strcpy(T2s, TmString(&T2));
  ga = (MI.average > 1);
  idos = IDENT(DOStab, ga, gl, gi);
  strcpy(name, NmsName(idos));
  TmnInfo(idos, &m1, &m2, &usage, NULL, NULL);
  if (usage & TMN_DEFINED) {
    if (m1!=T1 || m2!=T2)                                       eX(2);
    }
  else {
    ClcSum();                                                   eG(3);
    if (numnet > 0) {
      MapAllDos();                                              eG(4);
      if (!(MI.flags & FLG_GAMMA)) {
        mask = ~(NMS_LEVEL | NMS_GRIDN);
        isum = IDENT(SUMarr, 2, 0, 0);
        TmnClearAll(T2, mask, isum, TMN_NOID);                  eG(5);
        }
      }
    RdcDos(ga);                                                 eG(6);
    }
  if (MI.flags & FLG_ODOR) {
    SetOdorHour();                                              eG(9);
  }
  if (gp==0 && ga==1) {         /* continued average    */
    net = numnet;
    do {
      if (net > 0) {
        GrdSetNet(net);                                         eG(8);
        }
      isum = IDENT(dt, 0, GrdPprm->level, GrdPprm->index);
      ClcAverage(isum);                                         eG(7);
      net--;
      } while (net > 0);
    }
  return 0;
eX_10:
  eMSG(_no_data_);
eX_1:
  eMSG(_no_time_);
eX_2:
  strcpy(m1s, TmString(&m1));
  strcpy(m2s, TmString(&m2));
  eMSG(_expected_$$$_found_$$_, name, T1s, T2s, m1s, m2s);
eX_3:
  eMSG(_cant_calculate_$$$_, name, T1s, T2s);
eX_4:
  eMSG(_cant_map_dose_fields_);
eX_5:
  eMSG(_cant_clear_dose_fields_);
eX_6:
  eMSG(_cant_reduce_vertical_extension_);
eX_7:
  eMSG(_cant_average_dose_);
eX_8:
  eMSG(_cant_set_net_$_, net);
eX_9:
  eMSG(_cant_calculate_odour_hour_);
  }
//============================================================================
//
// history:
//
// 2002-09-24 lj 1.0.0  final release candidate
// 2004-06-10 lj 2.1.1  odor
// 2004-10-04 lj 2.1.3  odmod
// 2004-11-26 lj 2.1.7  string length for names = 256
// 2005-03-17 lj 2.1.17 odstep is standard
// 2005-03-17 uj 2.2.0  version number upgrade
// 2006-02-15 lj 2.2.9  clear unused PTLarr, MODarr
// 2006-02-22           SetOdourHour: getting sg
// 2006-10-26 lj 2.3.0  external strings
// 2008-03-10 lj 2.4.0  SetOdorHour: evaluation of rated odor frequencies
// 2008-04-17 lj 2.4.1  merged with 2.3.x
// 2011-07-07 uj 2.5.0  version number upgrade
// 2012-04-06 uj 2.6.0  version number upgrade
// 2014-06-26 uj 2.6.11 eG/eX adjusted
//
//============================================================================

