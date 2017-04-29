//======================================================== IBJntr.c
//
// Net transformations for surface definition files
// ================================================
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
//
// last change:  2014-01-21 uj
//
//==================================================================

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <math.h>
#include <locale.h>

#include "IBJtxt.h"
#include "IBJmsg.h"
#include "IBJary.h"
#include "IBJdmn.h"
#include "IBJntr.h"
#include "IBJntr.nls"

char  *NtrVersion = "2.1.3";
char  *NtrPgmName = "IBJgrid";
ARYDSC NtrDsc;

static char *eMODn = "IBJntr";
static NTRREC ri, ro;
static float Dd;
static int   FileType, idX=0, idY=1, idZ=2;
static float *Data, *data;
static char  infile[256], seq[32] = "j-,i+", mode[32]="text";
static char  Separators[32] = " ,;\t\r\n";

static float getData(int i, int j);
static float getdata(int i, int j);
static int   setData(int i, int j, float f);
static int   setdata(int i, int j, float f);
static int   getPrm(FILE *fp, char *name, double *f);
static int   getArray(FILE *fp, float *zz, int n);


//------------------------------------------------------------ NtrGetInRec
/** returns net informations of the input file */
NTRREC NtrGetInRec( void )
{
  return ri;
}

//----------------------------------------------------------- NtrGetOutRec
/** returns net informations of the output file */
NTRREC NtrGetOutRec( void )
{
  return ro;
}

//----------------------------------------------------------- NtrSetOutRec
/** sets net informations for the output file */
void  NtrSetOutRec(NTRREC f) { ro = f; }

//--------------------------------------------------------- NtrSetSequence
/** sets the index sequence for the output file */
void  NtrSetSequence(char *s) { strcpy(seq, s); }

//------------------------------------------------------------- NtrSetMode
/** sets the write mode for the output file (text or binary) */
void  NtrSetMode(char *s) { strcpy(mode, s); }

//------------------------------------------------------- NtrSetXYZIndices
/** sets the data column indices for input files of type XYZ */
void  NtrSetXYZIndices(int i, int j, int k) {
  idX = i-1;
  idY = j-1;
  idZ = k-1;
}

//---------------------------------------------------------------- getData
static float getData(int i, int j) {
  if (i>=ri.nx || i<0 || j>=ri.ny || j<0 || Data==NULL) return NTR_UNDEF;
  else return Data[i*ri.ny+j];
}

//-----------------------------------------------------------------getdata
static float getdata(int i, int j) {
  if (i>=ro.nx || i<0 || j>=ro.ny || j<0 || data==NULL) return NTR_UNDEF;
  else return data[i*ro.ny+j];
}

//---------------------------------------------------------------- setData
static int setData(int i, int j, float f) {
  if (i>=ri.nx || i<0 || j>=ri.ny || j<0 || Data==NULL) return -1;
  else  Data[i*ri.ny+j] = f;
  return 0;
}

//---------------------------------------------------------------- setdata
static int setdata(int i, int j, float f) {
  if (i>=ro.nx || i<0 || j>=ro.ny || j<0 || data==NULL) return -1;
  else  data[i*ro.ny+j] = f;
  return 0;
}

//----------------------------------------------------------------- getPrm
static int getPrm(FILE *fp, char *name, double *f) {
    char s[256], *pc;
    *f = NTR_UNDEF;
    if (!fgets(s, 256, fp)) return -1;
    if (strncmp(s, name, strlen(name))) return -2;
    for (pc=s+strlen(name); (*pc); pc++)  if (*pc == ',')  *pc = '.';
    if (1 != sscanf(s+strlen(name), "%lf", f)) return -3;
    return 0;
}

//--------------------------------------------------------------- getArray
static int getArray(FILE *fp, float *zz, int n) {
  dP(getArray);
  int i, l, bufsize;
  char *buf, *pc, *pn;
  float f;
  bufsize = 40000;
  buf = TxtAlloc(bufsize);
  if (!fgets(buf, bufsize, fp)) {
    FREE(buf);
    return -1;
  }
  pn = buf;
  for (pc=buf; (*pc); pc++)  if (*pc == ',')  *pc = '.';
  for (i=0; i<n; i++) {
    if (NULL == (pc=MsgTok(pn, &pn, " ;\t\n", &l)))  return -2;
    if (1 != sscanf(pc, "%f", &f))  return -3;
    zz[i] = f;
  }
  FREE(buf);
  return 0;
}

//--------------------------------------------------------- NtrReadArcFile
/** reads in a file of type ArcInfo */
int NtrReadArcFile(char *name) {
  dP(NtrReadArcFile);
  FILE *fp;
  int i, j;
  double ncols, nrows, xll, yll, size, nodata;
  float *zz;
  char locale[256]="C";
  strcpy(locale, setlocale(LC_NUMERIC, NULL));
  setlocale(LC_NUMERIC, "C");
  FileType = NTR_TYPE_ARC;
  if (NULL == (fp=fopen(name, "rb")))                           eX(1);
  getPrm(fp, "ncols", &ncols); if (ncols == NTR_UNDEF)          eX(10);
  getPrm(fp, "nrows", &nrows); if (nrows == NTR_UNDEF)          eX(10);
  getPrm(fp, "xllcorner",&xll);if (xll == NTR_UNDEF)            eX(10);
  getPrm(fp, "yllcorner",&yll);if (yll == NTR_UNDEF)            eX(10);
  getPrm(fp, "cellsize",&size);if (size == NTR_UNDEF)           eX(10);
  ri.nx = (int)ncols;
  ri.ny = (int)nrows;
  ri.xmin = xll + 0.5*size;
  ri.ymin = yll + 0.5*size;
  ri.delta = size;
  ri.gkx = 0;
  ri.gky = 0;
  *ri.ggcs = 0;                                                   //-2008-12-11
  ri.xmax = ri.xmin + (ri.nx-1)*ri.delta;
  ri.ymax = ri.ymin + (ri.ny-1)*ri.delta;
  getPrm(fp, "NODATA_value", &nodata);
  Data = ALLOC((ri.nx*ri.ny)*sizeof(float));  if (!Data)        eX(2);
  zz = ALLOC(ri.nx*sizeof(float));      if (!zz)                eX(3);
  for (j=0; j<ri.ny; j++) {
        if (0 > getArray(fp, zz, ri.nx))                        eX(4);
        for (i=0; i<ri.nx; i++) {
          if (zz[i] == nodata)                                  eX(6);  //-2004-05-12
          if (setData(i, ri.ny-1-j, zz[i]) < 0)                 eX(5);
        }
  }
  fclose(fp);
  FREE(zz);
  setlocale(LC_NUMERIC, locale);
  return 0;
eX_1:      eMSG(_cant_open_$_, name);
eX_2:eX_3: eMSG(_cant_allocate_);
eX_4:      eMSG(_cant_read_line_$_, j+1);
eX_5:      eMSG(_cant_set_data_);
eX_6:      eMSG(_nodata_$$_, j+1, i+1);
eX_10:     eMSG(_cant_read_header_);
}

//--------------------------------------------------------- NtrReadXYZFile
/** reads in a file of type XYZ */
int NtrReadXYZFile(char *name) {
  dP(NtrReadXYZFile);
  FILE *fp;
  int i, j, n, l, zone;
  char s[256], *pc, *pn;
  double xmin, ymin, xm, ym, x, y, z, xold, d, dd;
  char locale[256]="C";
  strcpy(locale, setlocale(LC_NUMERIC, NULL));
  setlocale(LC_NUMERIC, "C");
  FileType = NTR_TYPE_XYZ;
  xmin = 1.e12;
  ymin = 1.e12;
  xm = -1.e12;
  ym = -1.e12;
  dd = 1.e12;                                                  
  // test
  n = 0;
  fp = fopen(name, "rb"); if (!fp)                              eX(1);
  while (fgets(s, 256, fp)) {
    for (pc=s; (*pc); pc++)  if (*pc == ',')  *pc = '.';
    i = 0;
    xold = x;
    x = NTR_DUNDEF;                                                 //-2014-01-21
    y = NTR_DUNDEF;
    z = NTR_DUNDEF;
    pn = s;
    if (strlen(s) < 3) continue;
    while (NULL != (pc=MsgTok(pn, &pn, Separators, &l))) {
      if (i == idX) if (1 != sscanf(pc, "%lf", &x))              eX(11);
      if (i == idY) if (1 != sscanf(pc, "%lf", &y))              eX(12);
      if (i == idZ) if (1 != sscanf(pc, "%lf", &z))              eX(13);
      i++;
    }
    if (x==NTR_DUNDEF || y==NTR_DUNDEF || z==NTR_DUNDEF)         eX(14); //-2014-01-21
    if (x < xmin) xmin = x;
    if (y < ymin) ymin = y;
    if (x > xm) xm = x;
    if (y > ym) ym = y;
    if (n > 0) {                                                //-2004-04-27
      d = (xold > x) ? xold-x : x-xold;
      if (d > 0.1 && d < dd) dd = d;
    }
    n++;
  }
  fclose(fp);
  if (dd <= 0.1 || dd == NTR_DUNDEF)                            eX(2); //-2014-01-21
  ri.delta = dd;
  ri.nx = (int)((xm-xmin)/dd+0.5) + 1;
  ri.ny = (int)((ym-ymin)/dd+0.5) + 1;
  if (ri.nx*ri.ny != n)                                         eX(3);
  ri.gkx = xmin;
  ri.gky = ymin;
  zone = ri.gkx/1000000;                                          //-2008-12-11
  if (zone > 0) {
    if (zone >= 2 && zone <=5)
      strcpy(ri.ggcs, "GK");
    else
      strcpy(ri.ggcs, "UTM");
  }
  ri.xmin = 0;
  ri.ymin = 0;
  ri.xmax = ri.xmin + (ri.nx-1)*ri.delta;
  ri.ymax = ri.ymin + (ri.ny-1)*ri.delta;
  // read
  Data = ALLOC((ri.nx*ri.ny)*sizeof(float));
  fp = fopen(name, "rb"); if (!fp)                              eX(1);
  while (fgets(s, 256, fp)) {
    for (pc=s; (*pc); pc++)  if (*pc == ',')  *pc = '.';
    i = 0;
    pn = s;
    if (strlen(s) < 3) continue;
    while (NULL != (pc=MsgTok(pn, &pn, Separators, &l))) {
      if (i == idX) if (1 != sscanf(pc, "%lf", &x))              eX(11);
      if (i == idY) if (1 != sscanf(pc, "%lf", &y))              eX(12);
      if (i == idZ) if (1 != sscanf(pc, "%lf", &z))              eX(13);
      i++;
    }
    i = (int)((x-xmin)/dd+0.5);
    j = (int)((y-ymin)/dd+0.5);
    setData(i, j, z);
  }
  fclose(fp);
  setlocale(LC_NUMERIC, locale);
  return 0;
eX_1: eMSG(_cant_open_$_, name);
eX_11:eX_12:eX_13:eX_14: eMSG(_cant_read_floats_$$$$_, n+1, idX, idY, idZ);
eX_2: eMSG(_cant_set_delta_);
eX_3: eMSG(_inconsistent_number_lines_$_, n, ri.nx*ri.ny, dd, xmin, xm, ymin, ym);
}

//--------------------------------------------------------- NtrReadDmnFile
/** reads in a file of type DMN */
int NtrReadDmnFile(char *name) {
  dP(NtrReadDmnFile);
  int i, j, i1, i2, j1, j2;
  double delta, xmin, ymin, gkx, gky;
  char *ggcs = NULL;                                            //-2008-12-11
  ARYDSC dsc;
  TXTSTR usrhdr = { NULL, 0 };
  TXTSTR syshdr = { NULL, 0 };
  FileType = NTR_TYPE_DMN;
  memset(&dsc, 0, sizeof(ARYDSC));
  if (0>DmnRead(name, &usrhdr, &syshdr, &dsc))                    eX(1);
  if (dsc.numdm != 2)                                             eX(2);
  if (dsc.elmsz != sizeof(float))                                 eX(3);
  if (1 != DmnGetDouble(usrhdr.s, "delt|delta","%lf", &delta, 1)) eX(4);
  if (1 != DmnGetDouble(usrhdr.s, "xmin|x0", "%lf", &xmin, 1))    eX(5);
  if (1 != DmnGetDouble(usrhdr.s, "ymin|y0", "%lf", &ymin, 1))    eX(6);
  if (1 != DmnGetDouble(usrhdr.s, "gakrx|refx", "%lf", &gkx, 1)) gkx = 0; //-2006-11-08
  if (1 != DmnGetDouble(usrhdr.s, "gakry|refy", "%lf", &gky, 1)) gky = 0; //-2006-11-08
  if (1 != DmnGetString(usrhdr.s, "ggcs", &ggcs, 0)) ggcs = NULL; //-2008-12-11
  i1 = dsc.bound[0].low;
  i2 = dsc.bound[0].hgh;
  j1 = dsc.bound[1].low;
  j2 = dsc.bound[1].hgh;
  ri.delta = delta;
  ri.xmin = xmin;
  ri.ymin = ymin;
  ri.gkx = gkx;
  ri.gky = gky;
  if (!ggcs || !*ggcs) {                                          //-2008-12-11
    int zone;
    zone = gkx/1000000;
    if (zone > 0) {
      if (zone >= 2 && zone <=5)
        strcpy(ri.ggcs, "GK");
      else
        strcpy(ri.ggcs, "UTM");
    }
  }
  else
    strcpy(ri.ggcs, ggcs);
  ri.xmin += i1*ri.delta;
  ri.ymin += j1*ri.delta;
  ri.nx = i2-i1+1;
  ri.ny = j2-j1+1;
  ri.xmax = ri.xmin + (ri.nx-1)*ri.delta;
  ri.ymax = ri.ymin + (ri.ny-1)*ri.delta;
  Data = ALLOC((ri.nx*ri.ny)*sizeof(float));  if (!Data)        eX(7);
  for (i=i1; i<=i2; i++)
    for (j=j1; j<=j2; j++)
      setData(i-i1, j-j1, *(float *)AryPtr(&dsc, i, j));

  return 0;
eX_1: eMSG(_cant_read_file_$_, name);
eX_2: eMSG(_file_$_not_2d_, name);
eX_3: eMSG(_must_be_float_);
eX_4: eMSG(_missing_delta_);
eX_5: eMSG(_missing_xmin_);
eX_6: eMSG(_missing_ymin_);
eX_7: eMSG(_cant_allocate_);
}

//------------------------------------------------------------ interpolate
static float interpolate(double x, double y) {
  float h, g;
  double ximin, yimin, ximax, yimax, x1, y1, xi, eta, d;
  int i, j, i1, i2, j1, j2;
  d = ri.delta;
  ximin = ri.gkx + ri.xmin;
  yimin = ri.gky + ri.ymin;
  ximax = ri.gkx + ri.xmax;
  yimax = ri.gky + ri.ymax;
  if (x < ximin) {
    if (x < ximin-d)  return NTR_UNDEF;
    x = ximin;
  }
  if (x > ximax) {
    if (x > ximax+d)  return NTR_UNDEF;
    x = ximax;
  }
  if (y < yimin) {
    if (y < yimin-d)  return NTR_UNDEF;
    y = yimin;
  }
  if (y > yimax) {
    if (y > yimax+d)  return NTR_UNDEF;
    y = yimax;
  }
  i1 = (int)floor((x-Dd-ximin)/d);
  i2 = (int)floor((x+Dd-ximin)/d);
  j1 = (int)floor((y-Dd-yimin)/d);
  j2 = (int)floor((y+Dd-yimin)/d);
  h = 0;
  g = 0;
  for (i=i1; i<=i2; i++) {
     if (i < 0)  continue;
     if (i >= ri.nx)  break;
     x1 = ximin + i*d;
     xi = 1 - ((x1 > x) ? (x1-x)/Dd : (x-x1)/Dd);
     if (xi < 0)  continue;
     for (j=j1; j<=j2; j++) {
        if (j < 0)  continue;
        if (j >= ri.ny)  break;
        y1 = yimin + j*d;
        eta = 1 - ((y1 > y) ? (y1-y)/Dd : (y-y1)/Dd);
        if (eta < 0)  continue;
        g += xi*eta;
        h += xi*eta*getData(i,j);
     }
  }
  if (g > 0)  h /= g;
  return h;
}

//----------------------------------------------------------- NtrWriteFile
/** writes out data as DMN file */
int NtrWriteFile(char *name) {
  dP(NtrWriteFile);
  TXTSTR header = { NULL, 0 };
  int i, j;
  char s[256], *cp;
  float f, zgmn;                                              //-2005-02-21
  double x, y;
  char locale[256] = "C";
  strcpy(locale, setlocale(LC_NUMERIC, NULL));                //-2003-07-07
  cp = strrchr(name, '.');
  if (cp) {
    if (!strchr(cp, MSG_PATHSEP))  *cp = 0;                   //-2002-04-28
  }

  // interpolate
  Dd = (ri.delta > ro.delta) ? ri.delta : ro.delta;
  data = ALLOC((ro.nx*ro.ny)*sizeof(float)); if (!data)         eX(2);
  zgmn = 0.;
  for (i=0; i<ro.nx; i++) {
    for (j=0; j<ro.ny; j++) {
      x = ro.gkx + ro.xmin + i*ro.delta;
      y = ro.gky + ro.ymin + j*ro.delta;
      f = interpolate(x, y);
      setdata(i, j, f);
      if (getdata(i, j) == NTR_UNDEF)                           eX(3);
      zgmn += f;
    }
  }
  zgmn /= (ro.nx * ro.ny);
  // write out
  AryFree(&NtrDsc);
  if (0 > AryCreate(&NtrDsc, sizeof(float), 2, 0,ro.nx-1, 0,ro.ny-1))  eX(4);
  for (i=0; i<ro.nx; i++) {
    for (j=0; j<ro.ny; j++) {
      *(float *)AryPtr(&NtrDsc, i, j) = getdata(i, j);
    }
  }
  FREE(data);
  data = NULL;                     // tabulator replaced by blank //-2014-01-28
  sprintf(s, "- original file: %s\n", infile); TxtCpy(&header, s);
  sprintf(s, "locl  \"%s\"\n", locale);        TxtCat(&header, s);  //-2003-07-07
  sprintf(s, "xmin  %1.1lf\n", ro.xmin);       TxtCat(&header, s);
  sprintf(s, "ymin  %1.1lf\n", ro.ymin);       TxtCat(&header, s);
  sprintf(s, "delta %1.1lf\n", ro.delta);     TxtCat(&header, s);
  sprintf(s, "refx  %1.1lf\n", ro.gkx);        TxtCat(&header, s);
  sprintf(s, "refy  %1.1lf\n", ro.gky);        TxtCat(&header, s);
  if (*ro.ggcs) {                                                   //-2008-12-11
    sprintf(s, "ggcs  \"%s\"\n", ro.ggcs);     TxtCat(&header, s);
  }
  sprintf(s, "zgmn  %1.1lf\n",  zgmn);         TxtCat(&header, s); //-2005-02-21
  sprintf(s, "form  \"%s\"\n", "%7.1f");       TxtCat(&header, s);
  sprintf(s, "artp  \"%s\"\n", "T");           TxtCat(&header, s);
  sprintf(s, "vldf  \"%s\"\n", "P");           TxtCat(&header, s);
  sprintf(s, "sequ  %s\n", seq);               TxtCat(&header, s);
  if (mode != NULL && strstr(mode, "binary") != NULL)
    sprintf(s, "mode  \"binary\"\n");
  else
    sprintf(s, "mode  \"text\"\n");
  TxtCat(&header, s);
  if (0 > DmnWrite(name, header.s, &NtrDsc))                         eX(5);
  TxtClr(&header);
  return 0;
eX_2: eMSG(_cant_allocate_);
eX_3: eMSG(_no_output_);
eX_4: eMSG(_cant_create_array_);
eX_5: eMSG(_cant_write_file_$_, name);
}

//------------------------------------------------------------ NtrReadFile
/** checks the file type and reads in the data */
int NtrReadFile(char *name) {
  dQ(NtrReadFile);
  int rc;
  double nc;
  FILE *fp;
  if (Data)  FREE(Data);
  if (data)  FREE(data);
  Data = NULL;
  data = NULL;
  FileType = NTR_TYPE_UNDEF;
  rc = -1;
  strcpy(infile, name);
  if (NULL != strstr(name, ".dmn"))
    rc = NtrReadDmnFile(name);
  else {
    fp = fopen(name, "rb"); if (!fp) return -1;
    getPrm(fp, "ncols", &nc);
    fclose(fp);
    if (nc == NTR_UNDEF)
      rc = NtrReadXYZFile(name);
    else
      rc = NtrReadArcFile(name);
  }
  if (rc < 0)  return rc;
  ro.nx = ri.nx;
  ro.ny = ri.ny;
  ro.xmin = ri.xmin;
  ro.ymin = ri.ymin;
  ro.delta = ri.delta;
  ro.gkx = ri.gkx;
  ro.gky = ri.gky;
  strcpy(ro.ggcs, ri.ggcs);
  if (ro.xmin > 1000000.) {
    ro.gkx += ro.xmin;
    ro.xmax = (ro.xmax - ro.xmin);                                //-2011-06-09
    ro.xmin = 0;    
  }
  if (ro.ymin > 1000000.) {
    ro.gky += ro.ymin;
    ro.ymax = (ro.ymax - ro.ymin);                               //-2011-06-09
    ro.ymin = 0;
  }
  return FileType;
}

#ifdef MAIN /*###############################################################*/

#include "IBJstamp.h"
#include "IBJnls.h"

#define  ISOPTION(a)  ('-' == (a))

//------------------------------------------------------------------- Help
static void Help( void ) {
  /*
Net transformations for surface definition files\n
usage: %s <file> [options]\n
<file>  input file of type XYZ/DMN/ArcInfo\n
options:\n
  -i              information about <file> (no transformation)\n
  -j<i,j,k>       column indices for input of type XYZ (default:1,2,3)\n
  -x<number>      new xmin (absolute or relative to refx, m)\n
  -y<number>      new ymin (absolute or relative to refy, m)\n
  -g<refx>,<refy>[,<ggcs>] new reference point (m)\n
  -d<delta>       new delta (m)\n
  -n<nx,ny>       new number of grid cells\n
  -o<file>        output file name (default: srfa000.dmna in input dir)\n
  -s<sequ>        output sequence (default: j-:i+)\n
  -m<mode>        output mode (binary/text, default: text)\n
  */
  printf(_help1_);
  printf(_help2_$_, NtrPgmName);
  printf(_help3_);
  printf(_help4_);
  printf(_help5_);
  printf(_help6_);
  printf(_help7_);
  printf(_help8_);
  printf(_help9_);
  printf(_help10_);
  printf(_help11_);
  printf(_help12_);
  printf(_help13_);
  printf(_help14_);
  printf("\n");
  exit(0);
}

//------------------------------------------------------------------- main
int main( int argc, char *argv[] ) {
  int i, j, ix, iy, iz, info=0, nx, ny;
  double xmin, ymin, delta, gkx, gky;
  char *pc, out[256], path[256], ggcs[32];
  NTRREC ri, ro;
  if (argc>1 && !strncmp(argv[1], "--language=", 11)) {           //-2006-10-26
    char *lan = argv[1] + 11;
    if (*lan) {
      if (NlsRead(argv[0], "IBJntr", lan))  printf(_problems_nls_$_, lan);
    }
  }
  MsgVerbose = 1;
  *ggcs = 0;
  strcpy(out, "");
  printf(_main1_$$$_, NtrPgmName, NtrVersion, IBJstamp(__DATE__, __TIME__));
  printf(_main2_);
  printf(_main3_);
  printf(_$_main4_, NtrPgmName);
  printf(_$_main5_, NtrPgmName);
  if (argc < 2) Help();
  for (i=2; i<argc; i++) {
    pc = argv[i];
    if (ISOPTION(*pc)) {
      switch (pc[1]) {
        case 'i': info = 1;
                  break;
        case 'j': j=sscanf(pc+2, "%d,%d,%d", &ix, &iy, &iz);
                  if (j != 3) Help();
                  NtrSetXYZIndices(ix, iy, iz);
        default:  break;
      }
    }
  }
  for (i=strlen(argv[1])-1; i>=0; i--)
    if (argv[1][i] == '\\' || argv[1][i] == '/') break;
  *out = 0;
  if (i >= 0) {
    strncpy(out, argv[1], i+1);
    out[i+1] = 0;                                               //-2005-06-13
    strcat(out, "srfa000.dmna");
  }
  else
    strcpy(out, "~");
  if (0 > NtrReadFile(argv[1])) {
    printf(_input_$_not_read_, argv[1]);
    exit(-99);
  }
  ri = NtrGetInRec();
  ro = NtrGetOutRec();
  for (i=2; i<argc; i++) {
    pc = argv[i];
    if (ISOPTION(*pc)) {
      switch (pc[1]) {
        case 'x': j=sscanf(pc+2, "%lf", &xmin);                   //-2011-06-09
                  if (j != 1) Help();
                  ro.xmin = xmin;
                  break;
        case 'y': j=sscanf(pc+2, "%lf", &ymin);                   //-2011-06-09
                  if (j != 1) Help();
                  ro.ymin = ymin;
                  break;
        case 'g': j=sscanf(pc+2, "%lf,%lf,%s", &gkx, &gky, ggcs); //-2011-06-09
                  if (j < 2) Help();                              //-2008-12-11
                  ro.gkx = gkx;
                  ro.gky = gky;
                  if (j < 3) {                                    //-2008-12-11
                    int zone;
                    zone = gkx/1000000;
                    if (zone > 0) {
                      if (zone >= 2 && zone <= 5)
                        strcpy(ggcs, "GK");
                      else
                        strcpy(ggcs, "UTM");
                    }
                  }
                  strcpy(ro.ggcs, ggcs);                          //-2008-12-11
                  break;
        case 'n': j=sscanf(pc+2, "%d,%d", &nx, &ny);
                  if (j != 2) Help();
                  ro.nx = nx+1;
                  ro.ny = ny+1;
                  break;
        case 'd': j=sscanf(pc+2, "%lf", &delta);                  //-2011-06-09
                  if (j != 1) Help();
                  ro.delta = delta;
                  break;
        case 'o': strcpy(out, pc+2);
                  break;
        case 's': NtrSetSequence(pc+2);
                  break;
        case 'm': NtrSetMode(pc+2);
                  break;
        case 'i': break;
        case 'j': break;
        default:  break;
      }
    }
    else Help();
  }
  printf(_input_$_, argv[1]);
  if (FileType == NTR_TYPE_XYZ) printf("(XYZ)\n");
  if (FileType == NTR_TYPE_ARC) printf("(ArcInfo)\n");
  if (FileType == NTR_TYPE_DMN) printf("(DMN)\n");
  printf("xmin  = %1.1f\n", ri.xmin);
  printf("ymin  = %1.1f\n", ri.ymin);  //-2004-12-28
  printf("delta = %1.1f\n", ri.delta);
  printf("refx  = %1.0f\n", ri.gkx);  //-2004-12-28
  printf("refy  = %1.0f\n", ri.gky);  //-2004-12-28
  printf("ggcs  = %s\n", ri.ggcs);    //-2008-12-11
  printf("nx    = %d\n", ri.nx-1);
  printf("ny    = %d\n", ri.ny-1);
  if (!info) {
    printf(_output_$_, out);
    printf("xmin  = %1.1f\n", ro.xmin);
    printf("ymin  = %1.1f\n", ro.ymin);
    printf("delta = %1.1f\n", ro.delta);
    printf("refx  = %1.0f\n", ro.gkx);  //-2004-12-28
    printf("refy  = %1.0f\n", ro.gky);  //-2004-12-28
    printf("ggcs  = %s\n", ro.ggcs);    //-2008-12-11
    printf("nx    = %d\n", ro.nx-1);
    printf("ny    = %d\n", ro.ny-1);
    printf("\n");
    NtrSetOutRec(ro);
    NtrWriteFile(out);
  }
  printf("", NtrPgmName);
  exit(0);
}
#endif  /*###################################################################*/

/*=========================================================================

 history:

 2001-09-15 1.0.0  uj  created
 2004-12-28 1.3.9  uj  parameter names in main-output corrected
 2005-02-21 1.3.10 uj  write out zgmn to header
 2005-06-13 1.3.12 uj  default output name corrected
 2006-10-20 1.3.13 uj  Gauss-Krueger replaced by Absolute
 2006-10-26 2.0.0  lj  external strings
 2006-11-08 2.0.1  uj  alternative to gakrx/y
 2008-12-11 2.1.1  lj  parameter ggcs
 2011-06-09 2.1.2  uj  scan format corrected
                       set ro.xmax/ro.ymax just in case
 2014-01-21 2.1.3  uj  allow -999 as coordinate in NtrReadXYZ()

==========================================================================*/

