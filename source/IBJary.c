//========================================================= IBJary.c
//
// Array functions for IBJ-programs
// ================================
//
// Copyright (C) Umweltbundesamt, Dessau-Roﬂlau, Germany, 2002
// Copyright (C) Janicke Consulting, 88662 ‹berlingen, Germany, 2002
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
// last change 2006-10-26 lj
//
//==================================================================

char *IBJaryVersion = "2.0.0";
static char *eMODn = "IBJary";

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <signal.h>                                               //-2011-12-02

#include "IBJmsg.h"
#include "IBJary.h"
#include "IBJary.nls"

/*================================================================= AryAssert
*/
int AryAssert(      /* check dimensions of array *pa:                   */
ARYDSC *pa,         /* pointer to array descriptor                      */
int l,              /* length of data element                           */
... )               /* number of dimensions                             */
                    /* lower and upper bound (as integer) of each dim   */
  {
  dP(AryAssert);
  int i, n, f, g;
  va_list argptr;
  va_start(argptr, l);
  n = va_arg(argptr, int);
  if (!pa->start)                               eX(1);
  if (pa->elmsz != l)                           eX(2);
  if (pa->numdm != n)                           eX(3);
  for (i=0; i<n; i++) {
    f = va_arg(argptr, int);
    if (pa->bound[i].low != f)                  eX(4);
    g = va_arg(argptr, int);
    if (pa->bound[i].hgh != g)                  eX(5);
    }
  va_end(argptr);
  return 0;
eX_1:
  eMSG(_no_data_);
eX_2:
  eMSG(_wrong_element_size_$$_, pa->elmsz, l);
eX_3:
  eMSG(_wrong_dimensions_$$_, pa->numdm, n);
eX_4:
  eMSG(_wrong_lower_bound_$$$_, 'i'+i, pa->bound[i].low, f);
eX_5:
  eMSG(_wrong_upper_bound_$$$_, 'i'+i, pa->bound[i].hgh, g);
  }

//================================================================= AryDefine
//
long AryDefine(     /* initialize array descriptor          */
ARYDSC *pa,         /* pointer to array descriptor          */
int l,              /* l>0: length of data element          */
                    /* l=0: set descriptor elements to 0.   */
                    /* l<0: recalculate descriptor data     */
... )               /* number of dimensions                 */
                    /* limits (lower and upper bounds)      */
{
  dP(AryDefine);
  int i, k, n, g;
  long t, s;
  va_list argptr;
  pa->start = NULL;
  if (l >= 0) {
    memset(pa, 0, sizeof(ARYDSC));
    pa->maxdm = ARYMAXDIM;
    if (l == 0)  return 0;
    va_start(argptr, l);
    n = va_arg(argptr, int);
    pa->elmsz = l;
    pa->numdm = n;
    for (i=0; i<n; i++) {
      g = va_arg(argptr, int);
      pa->bound[i].low = g;
      g = va_arg(argptr, int);
      pa->bound[i].hgh = g; 
    }
  }
  va_end(argptr);
  if (pa->numdm < 0)                                    eX(1);
  if (pa->numdm == 0) {     /* Skalar */
    pa->ttlsz = pa->elmsz;
    pa->virof = 0;
    return 0;
    }
  i = pa->numdm - 1;
  pa->bound[i].mul = pa->elmsz;
  s = pa->bound[i].low*pa->bound[i].mul;
  k = 1 + pa->bound[i].hgh - pa->bound[i].low;
  t = k;
  for (i--; i>=0; i--) {
    pa->bound[i].mul = pa->bound[i+1].mul*k;
    k = 1 + pa->bound[i].hgh - pa->bound[i].low;
    t *= k;
    s += pa->bound[i].low*pa->bound[i].mul; 
  }
  pa->ttlsz = t*pa->elmsz;
  pa->virof = -s;
  return 0;
eX_1:
  eMSG(_invalid_dimensions_);
}

/*================================================================= AryCreate
*/
int AryCreate(      /* create an array     */
ARYDSC *pa,         /* pointer to array descriptor                      */
int l,              /* length of data element                           */
... )               /* number of dimensions                             */
                    /* lower and upper bound (as integer) of each dim   */
  {
  dP(AryCreate);
  int  g, i, k, s, t, n, allocation;
  void *pv;
  va_list argptr;
  if (pa->start)                                eX(1);
  allocation = 0;
  if (l < 0)  l = -l;
  else  allocation = 1;
  if (l < 1 ) goto ALLOCATE;
  va_start(argptr, l);
  n = va_arg(argptr, int);
  if (n<0 || n>ARYMAXDIM)                       eX(3);
  pa->elmsz = l;
  pa->maxdm = ARYMAXDIM;
  pa->numdm = n;
  for (i=0; i<n; i++) {
    g = va_arg(argptr, int);
    pa->bound[i].low = g;
    g = va_arg(argptr, int);
    pa->bound[i].hgh = g;
    }
  va_end(argptr);

ALLOCATE:
  if (pa->elmsz < 0)                            eX(2);
  if (pa->numdm == 0) {     /* scalar */
    pa->ttlsz = pa->elmsz;
    pa->virof = 0;
    return 0;
    }
  i = pa->numdm - 1;
  pa->bound[i].mul = pa->elmsz;
  s = pa->bound[i].low*pa->bound[i].mul;
  k = 1 + pa->bound[i].hgh - pa->bound[i].low;
  t = k;
  for ( i--; i>=0; i--) {
    pa->bound[i].mul = pa->bound[i+1].mul*k;
    k = 1 + pa->bound[i].hgh - pa->bound[i].low;
    t *= k;
    s += pa->bound[i].low*pa->bound[i].mul; }
  pa->ttlsz = t*pa->elmsz;
  pa->virof = -s;
  if (allocation) {
    pv = ALLOC(pa->ttlsz);  if (!pv)                    eX(4);
    pa->start = pv;
    }
  pa->usrcnt = 1;
  return 0;
eX_1:
  eMSG(_descriptor_used_);
eX_2:
  eMSG(_invalid_element_size_);
eX_3:
  eMSG(_invalid_dimensions_);
eX_4:
  eMSG(_cant_allocate_$_, pa->ttlsz);
  }

/*================================================================= AryDefAry
*/
int AryDefAry(      /* define subarray                                  */
ARYDSC *psrc,       /* pointer to source array                          */
ARYDSC *pdst,       /* pointer to destination array                     */
ARYDSC *porg,       /* actual source of destination data            */
char *def )         /* mapping, e.g.: j=3..1/0,i=0..5,k=2;  modified!   */
  {
  dP(AryDefAry);
  int is, id, ns, nd, i0, i1, j0, di, offs, offd, n;
  int is0[ARYMAXDIM], is1[ARYMAXDIM], ids[ARYMAXDIM], id0[ARYMAXDIM];
  int ds[ARYMAXDIM];
  char *pa, *pn, tk[80], defstr[80], newdef[80];
  ns = psrc->numdm;
  pdst->maxdm = ARYMAXDIM;
  pdst->numdm = 0;
  pdst->usrcnt = -1;
  pdst->ttlsz = 0;
  pdst->elmsz = psrc->elmsz;
  pdst->virof = 0;
  pdst->start = psrc->start;
  for (id=0; id<ARYMAXDIM; id++) {
    pdst->bound[id].low = 0;
    pdst->bound[id].hgh = 0;
    pdst->bound[id].mul = 0;
    }
  if ((ns < 1) || (!def))                               eX(1);
  for (is=0; is<ARYMAXDIM; is++)  ds[is] = 0;
  nd = 0;
  strcpy(defstr, def);
  *newdef = 0;
  pn = defstr;
  pa = MsgTok(pn, &pn, "|:,;", &n);

  while (pa) {
    *tk = 0;  strncat(tk, pa, n);
    pa = tk;
    is = *pa - 'i';
    if (is<0 || is>=ns)                                 eX(2);
    if (ds[is] == 1) {
      pa = MsgTok(pn, &pn, "|:,;", &n);
      continue;
    }
    pa++;
    if (*pa == '=') {
      i0 = psrc->bound[is].low;
      pa++;
      sscanf(pa, "%d", &i0);
      if (i0<psrc->bound[is].low || i0>psrc->bound[is].hgh)       eX(4);
      is0[is] = i0;
      is1[is] = i0;
      ds[is] = 1;
      pa = strstr(pa, "..");
      if (pa) {
        pa += 2;
        i1 = psrc->bound[is].hgh;
        sscanf(pa, "%d", &i1);
        if (i1<psrc->bound[is].low || i1>psrc->bound[is].hgh)     eX(5);
        is1[is] = i1;
        ids[nd] = is;
        id0[nd] = 0;
        pa = strchr(pa, '/');
        if (pa) {
          pa++;
          j0 = 0;
          sscanf(pa, "%d", &j0);
          id0[nd] = j0;
        }
        nd++;
        strcat(newdef, ",");
        strncat(newdef, tk, 1);
        if (i1 < i0)  strcat(newdef, "-");
      }
      else {
        strcat(newdef, ",");
        strcat(newdef, tk);
      }
    }
    else if (*pa=='+' || *pa==0) {
      i0 = psrc->bound[is].low;
      i1 = psrc->bound[is].hgh;
      is0[is] = i0;
      is1[is] = i1;
      ds[is] = 1;
      ids[nd] = is;
      id0[nd] = i0;
      nd++;
      strcat(newdef, ",");
      strncat(newdef, tk, 1);
      strcat(newdef, "+");
    }
    else if (*pa == '-') {
      i0 = psrc->bound[is].hgh;
      i1 = psrc->bound[is].low;
      is0[is] = i0;
      is1[is] = i1;
      ds[is] = 1;
      ids[nd] = is;
      id0[nd] = i1;
      nd++;
      strcat(newdef, ",");
      strncat(newdef, tk, 1);
      strcat(newdef, "-");
    }
    else                                eX(3);
    if (nd > ns)                        eX(7);
    pa = MsgTok(pn, &pn, "|:,;", &n);
  }
  for (is=0; is<ns; is++) {
    if (ds[is])  continue;
    i0 = psrc->bound[is].low;
    i1 = psrc->bound[is].hgh;
    is0[is] = i0;
    is1[is] = i1;
    ds[is] = 1;
    ids[nd] = is;
    id0[nd] = i0;
    nd++;
  }
  if (porg) {
    *porg = *psrc;
    for (is=0; is<ns; is++) {
      i0 = is0[is];
      i1 = is1[is];
      porg->bound[is].low = MIN(i0, i1);
      porg->bound[is].hgh = MAX(i0, i1);
    }
    porg->usrcnt = -1;
  }
  pdst->numdm = nd;
  for (id=0; id<nd; id++) {
    is = ids[id];
    i0 = is0[is];
    i1 = is1[is];
    if (i1 >= i0) {
      di = i1 - i0;
      pdst->bound[id].mul = psrc->bound[is].mul;
      }
    else {
      di = i0 - i1;
      pdst->bound[id].mul = -psrc->bound[is].mul;
      }
    pdst->bound[id].low = id0[id];
    pdst->bound[id].hgh = id0[id] + di;
    }
  strcpy(def, newdef+1);
  offs = psrc->virof;
  for (is=0; is<ns; is++) {
    offs += is0[is]*psrc->bound[is].mul;
  }
  offd = 0;
  for (id=0; id<nd; id++)
    offd += id0[id]*pdst->bound[id].mul;
  pdst->virof = offs - offd;
  return nd;
eX_1:
  eMSG(_undefined_source_);
eX_2:
  eMSG(_invalid_index_$$_, *pa, ns);
eX_3:
  eMSG(_invalid_specification_$_, tk);
eX_4:  eX_5:
  eMSG(_invalid_range_$$$_, tk, psrc->bound[is].low, psrc->bound[is].hgh);
eX_7:
  eMSG(_too_many_specifications_$_, defstr);
  }

/*=================================================================== AryFree
*/
int AryFree(        /* free memory used by array        */
ARYDSC *pa )        /* pointer to array descriptor      */
{
  dQ(AryFree);
  if ((!pa) || (!pa->start))  return 0;
  if (pa->usrcnt < 0)  return 0;
  if (pa->start) FREE(pa->start);
  pa->start = NULL;
  pa->usrcnt = 0;
  return 0;
}

/*===================================================================== AryPtr
*/
void *AryPtr(       /* address of array element */
ARYDSC *pa,         /* pointer array descriptor */
... )               /* index list               */
  {
  dP(AryPtr);
  int n, k;
  char *pc;
  va_list argptr;
  if ((!pa) || (!pa->start))                            eX(1);
  pc = (char*)pa->start + pa->virof;
  va_start(argptr, pa);
  for (n=0; n<pa->numdm; n++) {
    k = va_arg(argptr, int);
    if (k<pa->bound[n].low || k>pa->bound[n].hgh)       eX(2);
    pc += k*pa->bound[n].mul; }
  va_end(argptr);
  return pc;
eX_1:
  nMSG(_undefined_data_);
  return NULL;
eX_2:
  nMSG(_$$_out_of_range_$$_, 'i'+n, k, pa->bound[n].low, pa->bound[n].hgh);
  return NULL;
  }

/*==================================================================== AryPtrX
*/
void *AryPtrX(      /* address of array element */
ARYDSC *pa,         /* pointer array descriptor */
... )               /* index list               */
  {
  dP(AryPtrX);
  int n, k;
  char *pc;
  va_list argptr;
  if ((!pa) || (!pa->start))                            eX(1);
  pc = (char*)pa->start + pa->virof;
  va_start(argptr, pa);
  for (n=0; n<pa->numdm; n++) {
    k = va_arg(argptr, int);
    if (k<pa->bound[n].low || k>pa->bound[n].hgh)       eX(2);
    pc += k*pa->bound[n].mul; }
  va_end(argptr);
  return pc;
eX_1:
  nMSG(_undefined_data_);
  raise(SIGABRT);                                                 //-2011-12-02
  exit(1);
eX_2:
  nMSG(_$$_out_of_range_$$_, 'i'+n, k, pa->bound[n].low, pa->bound[n].hgh);
  raise(SIGABRT);                                                 //-2011-12-02
  exit(1);
  }

//===================================================================== AryOPtr
//
void *AryOPtr(      // address of array element
int offset,         // offset within record
ARYDSC *pa,         // pointer to array descriptor
... )               // index list
  {
  dP(AryOPtr);
  int n, k;
  char *pc;
  va_list argptr;
  if ((!pa) || (!pa->start))                            eX(1);
  pc = (char*)pa->start + pa->virof;
  va_start(argptr, pa);
  for (n=0; n<pa->numdm; n++) {
    k = va_arg(argptr, int);
    if (k<pa->bound[n].low || k>pa->bound[n].hgh)       eX(2);
    pc += k*pa->bound[n].mul; }
  va_end(argptr);
  return pc+offset;
eX_1:
  nMSG(_undefined_data_);
  return NULL;
eX_2:
  nMSG(_$$_out_of_range_$$_, 'i'+n, k, pa->bound[n].low, pa->bound[n].hgh);
  return NULL;
  }

//===================================================================== AryOPtrX
//
void *AryOPtrX(     // address of array element
int offset,         // offset within record
ARYDSC *pa,         // pointer to array descriptor
... )               // index list
  {
  dP(AryOPtrX);
  int n, k;
  char *pc;
  va_list argptr;
  if ((!pa) || (!pa->start))                            eX(1);
  pc = (char*)pa->start + pa->virof;
  va_start(argptr, pa);
  for (n=0; n<pa->numdm; n++) {
    k = va_arg(argptr, int);
    if (k<pa->bound[n].low || k>pa->bound[n].hgh)       eX(2);
    pc += k*pa->bound[n].mul; }
  va_end(argptr);
  return pc+offset;
eX_1:
  nMSG(_undefined_data_);
  exit(1);
eX_2:
  nMSG(_$$_out_of_range_$$_, 'i'+n, k, pa->bound[n].low, pa->bound[n].hgh);
  exit(1);
  }

/*===========================================================================

 history:

 1998-07-31 lj
 2001-04-29 lj 1.2.1 AryOPtr(), AryOPtrX()
 2002-04-05 lj adapted to modified IBJmsg
 2006-10-26 lj 2.0.0  external strings

============================================================================*/
