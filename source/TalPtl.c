//=================================================================== TalPtl.c
//
// Handling of particle arrays
// ===========================
//
// Copyright (C) Umweltbundesamt, Dessau-Roﬂlau, Germany, 2002-2005
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
// last change: 2014-06-26 uj
//
//==========================================================================

#define  STDMYMAIN  PtlMain
static char *eMODn = "TalPtl";

#include "IBJmsg.h"
#include "IBJstd.h"

//==================================================================

STDPGM(tstptl, PtlServer, 2, 6, 11);

//==================================================================

#include "TalTmn.h"
#include "TalNms.h"
#include "TalPtl.h"
#include "TalPtl.nls"

#define  LISTSIZE  10

/*--------------------------------------------------------------------------*/

int  PtlRecSize;
char PtlRecFormat[200];

static struct ARRLIST {
  ARYDSC *pa;
  int i, n;
  } ArrList[LISTSIZE];

  /*================================================================== PtlInit
  */
long PtlInit(           /* initialize server    */
  long flags,           /* action flags         */
  char *init_string )   /* server options       */
  {
  // dP(PTL:PtlInit);
  char *jstr, *ps;
  if (StdStatus & STD_INIT)  return 0;
  if (init_string) {
    jstr = init_string;
    ps = strstr(init_string, " -y");
    if (ps) sscanf(ps+3, "%d", &StdDspLevel);
    ps = strstr(init_string, " -v");
    if (ps) sscanf(ps+3, "%d", &StdLogLevel);
    }
  else  jstr = "";
  vLOG(3)("PTL_%d.%d.%s (%08lx,%s)", StdVersion, StdRelease, StdPatch, flags, jstr);
  StdStatus |= flags;
  StdStatus |= STD_INIT;
  return 0;
  }

  /*================================================================= PtlStart
  */
long PtlStart(  /* initialize sequential retrieval of ptl's     */
  ARYDSC *pa )  /* array pointer                                */
  {
  dP(PtlStart);
  int i;
  if (!pa)  return 0;
  for (i=0; i<LISTSIZE; i++)
    if (!ArrList[i].pa)  break;
  if (i >= LISTSIZE)                                    eX(1);
  if ((pa->numdm != 1) || (!pa->start))  return 0;
  ArrList[i].pa = pa;
  ArrList[i].i = pa->bound[0].low;
  ArrList[i].n = pa->bound[0].hgh;
  return i+1;
eX_1:
  eMSG(_list_full_);
  }

  /*================================================================== PtlNext
  */
void *PtlNext(  /* pointer to next ptl  */
  int handle )  /* handle               */
  {
  dP(PtlNext);
  void *pp;
  ARYDSC *pa;
  struct ARRLIST *ps;
  if ((handle < 1) || (handle > LISTSIZE)) {
    return NULL;
    }
  handle--;
  ps = ArrList + handle;
  pa = ps->pa;
  if ((!pa) || (!pa->start)) {
    ps->pa = NULL;
    return NULL;
    }
  while (ps->i <= ps->n) {
    pp = AryPtr(pa, ps->i);  if (!pp)              eX(1);
    ps->i++;
    if (*(PTLTAG*)pp)  return pp;
    }
  ps->pa = NULL;
  return NULL;
eX_1:
  nMSG(_invalid_index_$_, ps->i);
  return NULL;
  }

  /*=================================================================== PtlEnd
  */
long PtlEnd(    /* finish sequential retrieval of ptl's */
  int handle )  /* handle                               */
  {
  if ((handle < 1) || (handle > LISTSIZE))  return 0;
  handle--;
  ArrList[handle].pa = NULL;
  ArrList[handle].i = 0;
  ArrList[handle].n = 0;
  return 0;
  }

  /*================================================================= PtlCount
  */
long PtlCount(  /* count particles in table with this ...       */
  long id,      /* identification having one of these ...       */
  PTLTAG flags, /* flags set                                    */
  ARYDSC *pd )  /* pointer to particle array (opt.)             */
  {
  dP(PtlCount);
  int i, i1, i2, n;
  ARYDSC *pa;
  PTLTAG *pp;
  vLOG(4)("PTL:PtlCount(%s/%08lx, %02x)", NmsName(id), id, flags);
  if (pd)  pa = pd;
  else {
    pa = TmnAttach(id, NULL, NULL, 0, NULL);  if (!pa)          eX(1);
    }
  if ((!pa->start) || (pa->numdm != 1)) {
    if (!pd) {
      TmnDetach(id, NULL, NULL, 0, NULL);                       eG(4);
      }
    return 0;
    }
  n = 0;
  i1 = pa->bound[0].low;
  i2 = pa->bound[0].hgh;
  for (i=i1; i<=i2; i++) {
    pp = AryPtr(pa, i);  if (!pp)                               eX(2);
    if (*pp & flags)  n++;
    }
  if (!pd) {
    TmnDetach(id, NULL, NULL, 0, NULL);                         eG(3);
    }
  vLOG(4)("PTL:PtlCount = %d ", n);
  return n;
eX_1:
  eMSG(_cant_attach_$_, NmsName(id));
eX_2:
  eMSG(_index_range_$$_, i, NmsName(id));
eX_3:  eX_4:
  eMSG(_cant_detach_$_, NmsName(id));
  }

  /*================================================================ PtlCreate
  */
ARYDSC *PtlCreate(      /* create particle table        */
  long id,              /* identification               */
  int nptl,             /* number of particles          */
  PTLTAG tag )          /* init tag                     */
  {
  dP(PTL:PtlCreate);
  ARYDSC *pa;
  PTLTAG *pp;
  int i;
  if (PtlRecSize <= 0)                                  eX(1);
  if (nptl < 1)  nptl = 1;
  vLOG(5)("PTL:PtlCreate(%s/%08lx, %d, %02x)", 
    NmsName(id), id, nptl, tag);
  pa = TmnCreate(id, PtlRecSize, 1, 1, nptl);           eG(2);
  if (!pa)                                              eX(3); //-2014-06-26
  for (i=1; i<=nptl; i++) {
    pp = AryPtr(pa, i);  if (!pp)                       eX(4);
    *pp = tag;
    }
  return pa;
eX_1:
  nMSG(_undefined_recsize_);
  return NULL;
eX_2:  eX_3:
  nMSG(_cant_create_table_$_, nptl);
  return NULL;
eX_4:
  nMSG(_range_$$_, i, nptl);
  return NULL;
  }

  /*============================================================== PtlTransfer
  */
long PtlTransfer(       /* extract selected ptl's from source to dest.  */
  long srcid,           /* identification of source                     */
  long dstid,           /* identification of destination                */
  PTLTAG flags,         /* flags for selection                          */
  int clear )           /* clear flags                                  */
  {
  dP(PtlTransfer);
  int nsrc, ndst, nfree, i, i1, i2, j, j1, j2, n;
  long usage, defined, info, dstmode, dstt1, dstt2, srct1, srct2;
  ARYDSC *psrc, *pdst;
  PTLTAG *ps, *pd;
  char srcname[256], dstname[256];				//-2004-11-26
  strcpy(srcname, NmsName(srcid));
  strcpy(dstname, NmsName(dstid));
  vLOG(4)("PTL:PtlTransfer(%s/%08lx, %s/%08lx, %02x, %d)", 
    srcname, srcid, dstname, dstid, flags, clear);
  i = 0;  i1 = 0;  i2 = 0;
  j = 0;  j1 = 0;  j2 = 0;
  nsrc = PtlCount(srcid, flags, NULL);                          eG(1);
  if (!TmnInfo(srcid, &srct1, &srct2, &usage, NULL, NULL))      eX(30);
  info = TmnInfo(dstid, &dstt1, &dstt2, &usage, NULL, NULL);
  defined = (info && (usage & TMN_DEFINED));
  if (defined) {
    dstmode = TMN_MODIFY;
    pdst = TmnAttach(dstid, NULL, NULL, dstmode, NULL);         eG(2);
    if ((!pdst) || (!pdst->start) || (pdst->numdm!=1))          eX(3);
    if (pdst->elmsz != PtlRecSize)                              eX(4);
    i1 = pdst->bound[0].low;
    i2 = pdst->bound[0].hgh;
    nfree = 0;
    for (i=i1; i<=i2; i++) {
      pd = AryPtr(pdst, i);  if (!pd)                           eX(20);
      if (!*pd)  nfree++;
      }
    if (nsrc > nfree) {
      TmnResize(dstid, i2 + nsrc - nfree);                      eG(5);
      i2 = pdst->bound[0].hgh;
      }
    if (srct1 < dstt1)  dstt1 = srct1;
    if (srct2 > dstt2)  dstt2 = srct2;
    }
  else if (dstid) {
    dstmode = TMN_MODIFY | TMN_UNIQUE;
    ndst = nsrc;
    if (ndst < 1)  ndst = 1;
    pdst = TmnCreate(dstid, PtlRecSize, 1, 1, ndst);            eG(6);
    i1 = 1;
    i2 = ndst;
    dstt1 = srct1;
    dstt2 = srct2;
    }
  else  pdst = NULL;
  if (nsrc <= 0) {
    if (dstid) {
      TmnDetach(dstid, &dstt1, &dstt2, dstmode, NULL);          eG(7);
      }
    return 0;
    }
  psrc = TmnAttach(srcid, NULL, NULL, TMN_MODIFY, NULL);        eG(8);
  if ((!psrc) || (!psrc->start) || (psrc->numdm!=1))            eX(9);
  if (psrc->elmsz != PtlRecSize)                                eX(10);
  j1 = psrc->bound[0].low;
  j2 = psrc->bound[0].hgh;
  i = i1;
  if (pdst) {
    pd = AryPtr(pdst, i);  if (!pd)                                eX(11);
    }
  n = 0;
  for (j=j1; j<=j2; j++) {
    ps = AryPtr(psrc, j);  if (!ps)                                eX(12);
    if (*ps & flags) {
      if (pdst) {
        while (*pd) {
          i++;  if (i > i2)                                        eX(13);
          pd = AryPtr(pdst, i);  if (!pd)                          eX(14);
          }
        memcpy(pd, ps, PtlRecSize);
        if (clear)  *pd &= ~flags;
        }
      *ps = 0;
      n++;
      }
    }
  if (pdst)  TmnDetach(dstid, &dstt1, &dstt2, dstmode, NULL);      eG(15);
  for (i=j1, j=j2; i<j; i++, j--) {
    for ( ; i<j; j--) {
      ps = AryPtr(psrc, j);  if (!ps)                              eX(16);
      if (*ps)  break;
      }
    for ( ; i<j; i++) {
      pd = AryPtr(psrc, i);  if (!pd)                              eX(17);
      if (!*pd)  break;
      }
    if (i >= j)  break;
    memcpy(pd, ps, PtlRecSize);
    *ps = 0;
    }
  if (j < j2) {
    TmnResize(srcid, j);                                        eG(18);
    }
  TmnDetach(srcid, NULL, NULL, TMN_MODIFY, NULL);               eG(19);
  vLOG(4)("PTL:PtlTransfer = %d ", n);
  return n;
eX_30:
  eMSG(_no_$_, srcname);
eX_1:
  eMSG(_cant_count_$_, srcname);
eX_2:
  eMSG(_cant_attach_$_, dstname);
eX_3:  eX_4:
  eMSG(_structure_$_, dstname);
eX_5:
  eMSG(_cant_resize_$_, dstname);
eX_6:
  eMSG(_cant_create_$_, dstname);
eX_7:
  eMSG(_cant_detach_$_, dstname);
eX_8:
  eMSG(_cant_attach_$_, srcname);
eX_9: eX_10:
  eMSG(_structure_$_, srcname);
eX_11: eX_12: eX_13: eX_14: eX_16: eX_17: eX_20:
  eMSG(_internal_error_$_, i, i1, i2, j, j1, j2);
eX_15:
  eMSG(_cant_detach_$_, dstname);
eX_18:
  eMSG(_cant_resize_$_, srcname);
eX_19:
  eMSG(_cant_detach_$_, srcname);
  }

/*=================================================================== PtlServer
*/
long PtlServer(
char *ss )
  {
  // dP(PTL:PtlServer);
  if (StdArg(ss))  return 0;
  if (*ss) {
    switch (ss[1]) {
      default:   ;
      }
    return 0;
    }
  return 0;
  }
  
//===============================================================================
//
// history:
//
// 2002-09-24 lj 1.0.0  final release candidate
// 2004-11-26 lj 2.1.7  string length for names = 256
// 2005-03-17 uj 2.2.0  version number upgrade
// 2006-10-26 lj 2.3.0  external strings
// 2011-07-07 uj 2.5.0  version number upgrade
// 2012-04-06 uj 2.6.0  version number upgrade
// 2014-06-26 uj 2.6.11 eX/eG adjusted
//
//===============================================================================

