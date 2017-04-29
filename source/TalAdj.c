// ================================================================ TalAdj.c
//
// ADI method used by AUSTAL2000
// =============================
//
// Copyright (C) Umweltbundesamt, Dessau-Roﬂlau, Germany, 2002-2005
// Copyright (C) Janicke Consulting, 88662 ‹berlingen, Germany, 2002-2005
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
// last change: 2006-10-26 lj
//
//==========================================================================

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

static char *eMODn = "TalAdj";

#include "IBJmsg.h"
#include "IBJary.h"
#include "TalAdj.h"
#include "TalAdj.nls"

#define  OFF(i,j,k)  (*(int*)AryPtrX(Poff,(i),(j),(k)))
#define  MAT(l)      (*(MATREC*)AryPtrX(Tmx,(l)))

#define  NC     15

typedef struct matrec {
  int i, j, k; int ii[NC]; float aa[NC];
  } MATREC;

static ARYDSC *Poff, *Tmx;
static int Ni, Nj, Nk, Nv;
static int *Ll;
static float *Xx, *Rs, *X0, *Xi, *Xj, *Xk;

/*====================================================================== SlvTri
*/
static int SlvTri(int n, float *aa, float *bb, float *cc, float *dd, float *xx)
  {
  int i;
  float f;
  if (n < 1)  return 0;
  for (i=1; i<n; i++) {
    if (aa[i-1] == 0.0)  return -1;
    f = cc[i-1]/aa[i-1];
    aa[i] -= f*bb[i];
    dd[i] -= f*dd[i-1];
    }
  if (aa[n-1] == 0.0)  return -1;
  xx[n-1] = -dd[n-1]/aa[n-1];
  for (i=n-2; i>=0; i--)
    xx[i] = -(dd[i] + bb[i+1]*xx[i+1])/aa[i];
  return 0; 
  }

/*====================================================================== ExtMxI
*/
static int ExtMxI(int j, int k, float *aa, float *bb, float *cc, float *dd )
  {
  int i, c, lc, l0, l1, lm, m, mm, *pl;
  float wm, *pa;
  MATREC *pm;
  memset(bb, 0, Ni*sizeof(float));
  memset(cc, 0, Ni*sizeof(float));
  for (mm=0, i=1; i<=Ni; i++) {
    lm = OFF(i,j,k);
    if (lm >= 0)  Ll[mm++] = lm;
    }
  Ll[mm] = -2;
  lm = -2;
  l1 = Ll[0];
  for (m=0; m<mm; m++) {
    l0 = lm;  lm = l1;  l1 = Ll[m+1];
    pm = &MAT(lm);
    pl = pm->ii;
    pa = pm->aa;
    dd[m] = -Rs[lm];
if (pl[0] != lm) {                                               //-2006-05-30
  printf(_$_row_start_$$_, "ExtMxI", lm, pl[0]);  
  return -1; 
}
    aa[m] = pa[0];
    wm = 0;
    for (c=1; c<NC; c++) {
      lc = pl[c];
      if (lc < 0)  break;
      if      (lc == l0)  cc[m-1] = pa[c];
      else if (lc == l1)  bb[m+1] = pa[c];
      else                wm += pa[c]*Xx[lc];
      }
    dd[m] += wm;
    }
  return mm;
  }

/*====================================================================== ExtMxJ
*/
static int ExtMxJ(int i, int k, float *aa, float *bb, float *cc, float *dd )
  {
  int j, c, lc, l0, l1, lm, m, mm, *pl;
  float wm, *pa;
  MATREC *pm;
  memset(bb, 0, Nj*sizeof(float));
  memset(cc, 0, Nj*sizeof(float));
  for (mm=0, j=1; j<=Nj; j++) {
    lm = OFF(i,j,k);
    if (lm >= 0)  Ll[mm++] = lm;
    }
  Ll[mm] = -2;
  lm = -2;
  l1 = Ll[0];
  for (m=0; m<mm; m++) {
    l0 = lm;  lm = l1;  l1 = Ll[m+1];
    pm = &MAT(lm);
    pl = pm->ii;
    pa = pm->aa;
    dd[m] = -Rs[lm];
if (pl[0] != lm) {                                            //-2006-05-30
  printf(_$_row_start_$$_, "ExtMxJ", lm, pl[0]); 
  return -1; 
}
    aa[m] = pa[0];
    wm = 0;
    for (c=1; c<NC; c++) {
      lc = pl[c];
      if (lc < 0)  break;
      if      (lc == l0)  cc[m-1] = pa[c];
      else if (lc == l1)  bb[m+1] = pa[c];
      else                wm += pa[c]*Xx[lc];
      }
    dd[m] += wm;
    }
  return mm;
  }

/*====================================================================== ExtMxK
*/
static int ExtMxK(int i, int j, float *aa, float *bb, float *cc, float *dd )
  {
  int k, c, lc, l0, l1, lm, m, mm, *pl;
  float wm, *pa;
  MATREC *pm;
  memset(bb, 0, Nk*sizeof(float));
  memset(cc, 0, Nk*sizeof(float));
  for (mm=0, k=1; k<=Nk; k++) {
    lm = OFF(i,j,k);
    if (lm >= 0)  Ll[mm++] = lm;
    }
  Ll[mm] = -2;
  lm = -2;
  l1 = Ll[0];
  for (m=0; m<mm; m++) {
    l0 = lm;  lm = l1;  l1 = Ll[m+1];
    pm = &MAT(lm);
    pl = pm->ii;
    pa = pm->aa;
    dd[m] = -Rs[lm];
if (pl[0] != lm) {                                              //-2006-05-30
  printf(_$_row_start_$$_, "ExtMxK", lm, pl[0]); 
  return -1; 
}
    aa[m] = pa[0];
    wm = 0;
    for (c=1; c<NC; c++) {
      lc = pl[c];
      if (lc < 0)  break;
      if      (lc == l0)  cc[m-1] = pa[c];
      else if (lc == l1)  bb[m+1] = pa[c];
      else                wm += pa[c]*Xx[lc];
      }
    dd[m] += wm;
    }
  return mm;
  }

/*======================================================================== AdiIterate
*/
int AdiIterate( ARYDSC *poff, ARYDSC *pmat, float *pr, float *px, float omega,
int ni, int nj, int nk )
  {
  dP(AdiIterate);
  int i, j, k, l, nn, n, m, r;
  float *aa, *bb, *cc, *dd, *xx;
  Poff = poff;
  Tmx = pmat;
  Rs = pr;
  X0 = px;
  Ni = ni;
  Nj = nj;
  Nk = nk;
  nn = MAX(Ni,Nj);
  nn = MAX(Nk,nn);
  nn++;
  Nv = pmat->bound[0].hgh + 1;
  aa = ALLOC(nn*sizeof(float));
  bb = ALLOC(nn*sizeof(float));
  cc = ALLOC(nn*sizeof(float));
  dd = ALLOC(nn*sizeof(float));
  xx = ALLOC(nn*sizeof(float));
  Ll = ALLOC(nn*sizeof(int));
  Xi = ALLOC(Nv*sizeof(float));
  Xj = ALLOC(Nv*sizeof(float));
  Xk = ALLOC(Nv*sizeof(float));
  Xx = X0;
  for (i=1; i<=Ni; i++)
    for (j=1; j<=Nj; j++) {
      n = ExtMxK(i, j, aa, bb, cc, dd);
      if (n < 0)   eX(21);
      if (n > Nk)  eX(11);
      r = SlvTri(n, aa, bb, cc, dd, xx);  if (r < 0)      eX(1);
      for (m=0; m<n; m++)  Xk[Ll[m]] = xx[m]; 
      }
  Xx = Xk;
  for (j=1; j<=Nj; j++)
    for (k=1; k<=Nk; k++) {
      n = ExtMxI(j, k, aa, bb, cc, dd);
      if (n < 0)   eX(22);
      if (n > Ni)  eX(12);
      r = SlvTri(n, aa, bb, cc, dd, xx);  if (r < 0)      eX(2);
      for (m=0; m<n; m++)  Xi[Ll[m]] = xx[m]; 
      }
  Xx = Xi;
  for (i=1; i<=Ni; i++)
    for (k=1; k<=Nk; k++) {
      n = ExtMxJ(i, k, aa, bb, cc, dd);
      if (n < 0)   eX(23);
      if (n > Nj)  eX(13);
      r = SlvTri(n, aa, bb, cc, dd, xx);  if (r < 0)      eX(3);
      for (m=0; m<n; m++)  Xj[Ll[m]] = xx[m]; 
      }
  Xx = Xj;
  for (l=0; l<Nv; l++)
    X0[l] += omega*(Xx[l] - X0[l]);
  FREE(Xk);
  FREE(Xj);
  FREE(Xi);
  FREE(Ll);
  FREE(xx);
  FREE(dd);
  FREE(cc);
  FREE(bb);
  FREE(aa);
  return 0;
eX_1: eX_2: eX_3:
eX_11: eX_12: eX_13:
  eMSG(_singular_matrix_);
eX_21: eX_22: eX_23:
  MsgListAlloc(stderr);
  eMSG(_internal_error_);
}
//======================================================================

