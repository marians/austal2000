// ======================================================== TalRgl.c
//
// Calculate average roughness length for AUSTAL2000
// =================================================
//
// Copyright (C) Umweltbundesamt, Dessau-Roﬂlau, Germany, 2002
// Copyright (C) Janicke Consulting, 88662 ‹berlingen, Germany, 2002-2008
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
// last change:  2011-12-07 lj
//
//========================================================================

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <ctype.h>                                                //-2008-12-07

#include "TalUtl.h"                                               //-2011-12-07
#include "TalRgl.h"
#include "TalRgl.nls"
#include "IBJgk.h"                  //-2001-09-17
#include "IBJary.h"                 //-2006-10-12
#include "IBJtxt.h"                 //-2006-10-12
#include "IBJdmn.h"                 //-2006-10-12

double RglXmin, RglXmax, RglYmin, RglYmax, RglDelta;
double RglX, RglY, RglA, RglB;
int    RglMrd;
unsigned int RglCRC;
char RglFile[32];
char RglGGCS[32];

static float z0values[10] = {
  0.05, 0.01, 0.02, 0.05, 0.10, 0.20, 0.50, 1.00, 1.50, 2.00 };
static char Buf[40000];
static int ncols, nrows;
static double xll, yll, cell;
static double rr;
static char *fn = "rl.dat";
static int READ_DMNA = 1;           //-2006-10-12
static FILE *f;
static ARYDSC z0dsc;

//========================================================= TrlReadHeader
//
int TrlReadHeader(   // read the header of the ArcInfo-File
char *path,          // directory that contains the cataster
char *ggcs)          // coordinate system
{
  if (!READ_DMNA) {
    char fname[256];
    if (!path) {
      if (f)  fclose(f);
      f = NULL;
      return 0;
    }
    sprintf(fname, "%s/%s", path, fn);
    f = fopen(fname, "rb");  if (!f) return -1;
    ncols = 6780;
    nrows = 8910;
    xll = 3279000;
    yll = 5229000;
    cell = 100;
  }
  else {
    char fname[256], ext[32], *cs=NULL, *cp, *sequ;
    TXTSTR usrhdr = { NULL, 0 };
    TXTSTR syshdr = { NULL, 0 };
    if (!path) {
      AryFree(&z0dsc);
      return 0;
    }
    memset(&z0dsc, 0, sizeof(ARYDSC));
    strcpy(ext, ggcs);
    cp = ext;
    while (*cp) { *cp = tolower(*cp); cp++; }
    sprintf(RglFile, "z0-%s.dmna", ext);
    sprintf(fname, "%s/%s", path, RglFile);
    if (0 > DmnRead(fname, &usrhdr, &syshdr, &z0dsc))                return RGLE_FILE;
    if (1 != DmnGetDouble(usrhdr.s, "xmin", "%lf", &xll, 1))         return RGLE_XMIN;
    if (1 != DmnGetDouble(usrhdr.s, "ymin", "%lf", &yll, 1))         return RGLE_YMIN;
    if (1 != DmnGetDouble(usrhdr.s, "delt|delta", "%lf", &cell, 1))  return RGLE_DELT;
    if (1 != DmnGetString(usrhdr.s, "ggcs",  &cs, 1))                return RGLE_GGCS;
    if (1 != DmnGetString(syshdr.s, "sequ",  &sequ, 1))              return RGLE_SEQN;
    if (strcmp(sequ, "j-,i+"))                                       return RGLE_SEQU;
    cp = cs;
    while (*cp) { *cp = toupper(*cp); cp++; }
    strcpy(RglGGCS, cs);
    if (strcmp(ggcs, RglGGCS))                                       return RGLE_CSNE;
    if (z0dsc.numdm != 2)                                            return RGLE_DIMS;
    if (z0dsc.bound[1].low != 1)                                     return RGLE_ILOW;
    if (z0dsc.bound[0].low != z0dsc.bound[0].hgh)                    return RGLE_JIND;   
    if (z0dsc.bound[0].low != 0)                                     return RGLE_JIND; //-2011-06-29
    if (!strcmp(RglGGCS, "UTM") && (xll < 1000000))               //-2008-12-07
      xll += 32000000;                                            //-2008-12-07
    ncols = z0dsc.elmsz - 1;
    nrows = z0dsc.bound[1].hgh;
    RglXmin = xll;
    RglXmax = xll + ncols*cell;
    RglYmin = yll;
    RglYmax = yll + nrows*cell;
    RglDelta = cell;
    TxtClr(&usrhdr);
    TxtClr(&syshdr);
    RglCRC = TutMakeCrc(z0dsc.start, z0dsc.ttlsz);
  }
  return 0;
}

static float **_readSection(
double amin, int nc, double bmin, int nr, char *buf ) {
  int i1, j1, i, j, k, l, ii, m, n;
  float **z0, *data;
  char *pc;
  if (amin<0 || amin+nc>ncols)  return NULL;
  if (bmin<0 || bmin+nr>nrows)  return NULL;
  j1 = (int)amin;
  i1 = (int)bmin;
  z0 = malloc(nr*sizeof(float*) + nc*nr*sizeof(float));
  if (!z0)  return NULL;
  data = (float*)(z0 + nr);
  if (buf)  memset(buf, 0, (nc+1)*nr+1);
  if (!READ_DMNA) {
    if (!f)                           goto nof;
    if (fseek(f, 0, SEEK_SET))        goto nof;
    for (i=0; i<i1; i++) {
      m = 0;
      if (1 != fread(&m, 2, 1, f))    goto eof;    // get number of words
      if (fseek(f, 2*m, SEEK_CUR))    goto eof;    // drop words
    }
    for (l=0; l<nr; l++) {
      i = nr - l - 1;
      z0[l] = data + l*nc;
      pc = Buf;
      m = 0;
      if (1 != fread(&m, 2, 1, f))    goto eof;   // get number of words
      for (ii=0; ii<m; ii++) {                    // read words
        n = fgetc(f);  if (n == EOF)  goto eof;   // get count
        k = fgetc(f);  if (k == EOF)  goto eof;   // get class index
        for (; n>0; n--)  *pc++ = k+'0';
      }
      if (buf) {
        strncpy(buf+i*(nc+1), Buf+j1, nc);
        buf[i*(nc+1)+nc] = '\n';
      }
      for (j=0; j<nc; j++) {
        k = Buf[j1+j] - '0';
        if (k<0 || k>9)                goto invalid;
        z0[l][j] = z0values[k];
      }
    }
  }
  else {
    for (i=0; i<nr; i++) {
      z0[i] = data + i*nc;
      pc = (char *)AryPtr(&z0dsc, z0dsc.bound[0].low, 1+i1+i);
      for (j=0; j<nc; j++) {
        k = pc[j1+j] - '0';
        if (k<0 || k>9)                goto invalid;
        z0[i][j] = z0values[k];
        if (buf)
          buf[(nr-i-1)*(nc+1)+j] = k + '0';
      }
      if (buf)
        buf[(nr-i-1)*(nc+1)+nc] = '\n';
    }
  }
  return z0;
nof:
  free(z0);
  return NULL;
eof:
  printf("%s", _eof_);
  free(z0);
  return NULL;
invalid:
  if (!READ_DMNA)
    printf(_invalid_char1_$$$_, Buf[j1+j], i1+i, j1+j);
  else
    printf(_invalid_char2_$$$_, pc[j1+j], nrows-(1+i1+i), j1+j);
  free(z0);
  return NULL;
}

static void z0add( float z0, double d, double a, double b, double c,
double *pz0m, double *pz0g ) {
  int i, j, ni;
  double cq = c*c;
  if (c < d) {
    int nd = (int)ceil(d/c);
    double dd = d/nd;
    for (i=0; i<nd; i++)
      for (j=0; j<nd; j++)  z0add(z0, dd, a-i*dd, b-j*dd, c, pz0m, pz0g);
    return;
  }
  if (a+c <= 0)  return;
  if (a-c >= d)  return;
  if (b+c <= 0)  return;
  if (b-c >= d)  return;
  ni = 0;
  if ((a*a + b*b) <= cq) ni++;
  if (((a-d)*(a-d) + b*b) <= cq)  ni++;
  if ((a*a + (b-d)*(b-d)) <= cq)  ni++;
  if(((a-d)*(a-d) + (b-d)*(b-d)) <= cq)  ni++;
  if (ni == 4) {
    *pz0g += d*d;
    *pz0m += d*d*z0;
    return;
  }
  if (ni == 0)  return;
  if (d < 0.01*rr) {
    double g = 0;
    double ai, bj, dd;
    int n=5;
    dd = d/n;
    for (i=0; i<n; i++) {
      ai = a - (i+0.5)*dd;
      for (j=0; j<n; j++) {
        bj = b - (j+0.5)*dd;
        if (ai*ai+bj*bj <= cq)  g++;
      }
    }
    g *= dd*dd;
    *pz0g += g;
    *pz0m += g*z0;
    return;
  }
  d *= 0.5;
  z0add(z0, d, a,   b,   c, pz0m, pz0g);
  z0add(z0, d, a-d, b,   c, pz0m, pz0g);
  z0add(z0, d, a,   b-d, c, pz0m, pz0g);
  z0add(z0, d, a-d, b-d, c, pz0m, pz0g);
}

//=========================================================== TrlGetZ0
//
float TrlGetZ0(   // calculate the average value of z0 within a circle
char  *cs,        // geographic coordinate system chosen in a2k input file
double xc,        // x-coordinate of the circle
double yc,        // y-coordinate of the circle
double r,         // radius of the circle (m)
char **pbuf )     // optionally a print image of the area is returned
{
  double a, b, c, amin, amax, bmin, bmax;
  float **_z0;
  double z0m=0, z0g=0, x, y;
  int na, nb, i, j;
  char *_buf=NULL;
  if ((!READ_DMNA && !f) || (READ_DMNA && !z0dsc.start))   return RGLE_UERR;
  RglX = xc;
  RglY = yc;
  RglMrd = -1;
  //
  // coordinate conversions
  if (strcmp(cs, RglGGCS))                                 return RGLE_CSNE;
  if (!strcmp(cs, "GK")) {
    GKpoint gk;
    RglMrd = (int)(xll/1000000);
    if (xc<RglMrd*1000000 || xc>=(RglMrd+1)*1000000) {
      gk = GKxGK(RglMrd, xc, yc);
      if (gk.rechts <= 0 && gk.hoch <= 0)                  return RGLE_GKGK;
      x = gk.rechts;
      y = gk.hoch;
    }
    else {
      RglMrd = -1;
      x = xc;
      y = yc;
    }
  }
  else {
    x = xc;
    y = yc;
  }
  RglX = x;
  RglY = y;
  //
  // relative coordinates
  rr = r/cell;
  a = (x-xll)/cell;
  b = (y-yll)/cell;
  c = r/cell;
  if (a < 0 || a > ncols || b < 0 || b > nrows)            return RGLE_POUT;
  amin = floor(a-c);
  if (amin < 0)  amin = 0;
  amax = ceil(a+c);
  if (amax > ncols)  amax = ncols;
  bmin = floor(b-c);
  if (bmin < 0)  bmin = 0;
  bmax = ceil(b+c);
  if (bmax > nrows)  bmax = nrows;
  na = (int)(amax-amin);
  nb = (int)(bmax-bmin);
  if (na<1 || nb<1)                                        return RGLE_UERR;
  RglA = xll + amin*cell;
  RglB = yll + bmin*cell;
  if (pbuf) {
    _buf = malloc(nb*(na+1)+1);
    if (!_buf)                                             return RGLE_UERR;
  }
  _z0 = _readSection(amin, na, bmin, nb, _buf);
  if (!_z0)                                                return RGLE_SECT;
  for (i=0; i<nb; i++) {
    for (j=0; j<na; j++) {
      z0add(_z0[i][j], 1.0, a-amin-j, b-bmin-i, c, &z0m, &z0g);
    }
  }
  if (z0g > 0)  z0m /= z0g;
  free(_z0);
  if (pbuf) *pbuf = _buf;
  return z0m;
}

//=============================================================================
//
// history:
//
// 2002-06-21 lj  0.13.0  final test version
// 2002-09-24 lj          final release candidate
// 2006-10-12 uj          z0 register in dmna format
// 2006-10-26 lj          external strings
// 2006-11-03 uj          formal corrections
// 2008-12-07 lj          UTM uses zone 32 if xmin has no prefix
// 2011-12-07 lj          CRC32 shifted to TalUtl
//
//=============================================================================

