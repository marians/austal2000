//============================================================ TalInp.h
//
// Read Input for A2K-GRS
// ======================
//
// Copyright (C) Umweltbundesamt, Dessau-Roﬂlau, Germany, 2002-2011
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

#ifndef TALINP_INCLUDE
#define TALINP_INCLUDE

#ifndef IBJARY_INCLUDE
  #include "IBJary.h"
#endif

#define TIP_STRING 1
#define TIP_NUMBER 2
#define TIP_SERIES      0x0001
#define TIP_STAT        0x0002
#define TIP_VARIABLE    0x0010
#define TIP_COMPLEX     0x0020
#define TIP_BODIES      0x0040
#define TIP_NESTING     0x0080
#define TIP_NOSTANDARD  0x0100
#define TIP_SCINOTAT    0x0200      //-2003-02-21
#define TIP_LIB2        0x0400
#define TIP_LIB36       0x0800
#define TIP_WRITE_L     0x1000
#define TIP_WRITE_Z     0x2000
#define TIP_LIBRARY     0x4000
#define TIP_SORRELAX    0x8000      //-2006-02-06
#define TIP_ODORMOD     0x010000    //-2008-03-10
#define TIP_NODILUTE    0x020000    //-2011-12-01
#define TIP_HM_USED     0x040000    //-2011-12-02
#define TIP_RI_USED     0x080000    //-2011-12-02
#define TIP_MAXCMP      58          //-2008-09-23
#define TIP_ADDODOR     6           //-2008-09-23
#define TIP_MAXLENGTH   200000      //-2006-11-03

#define TIP_GAMMA       0x0100000
#define TIP_DECAY       0x0200000
#define TIP_NODAY       0x0400000
#define TIP_ARTM        0x1000000
#define TIP_AUSTAL      0x2000000
#define TIP_TALDIA      0x4000000                                 //-2014-06-26

#define NOSTANDARD ((TalMode&TIP_NOSTANDARD) != 0)  //-2002-03-05
#define ARTM       ((TalMode&TIP_ARTM) != 0)        //-2005-09-29

typedef struct {
  char cset[16];  // character set                                //-2008-07-22
  char ti[256];   // title of this project
  double t1, t2;  // time span for the calculation
  int interval;   // averaging interval
  int average;    // number of intervals before writing out
  double gx, gy;  // Gauss-Krueger coordinates of the origin
  double ux, uy;  // UTM coordinates of the origin
  char ggcs[32];  // geographic coordinate system (GK|UTM|<empty>)
  int nn;         // number of grids
  int npmax;      // maximum number of monitor points
  int *nx;        // number of grid cells in x-direction
  int *ny;        // number of grid cells in y-direction
  int *nz;        // number of grid cells in z-direction
  int nzmax;      // maximum number of grid cells in z-direction
  int *gl;        // grid level
  int *gi;        // grid index
  double *dd;     // horizontal mesh width
  double *x0;     // western outer border of computational grid
  double *x1;     // western inner border of computational grid
  double *x2;     // eastern inner border of computational grid
  double *x3;     // eastern outer border of computational grid
  double *y0;     // southern outer border of computational grid
  double *y1;     // southern inner border of computational grid
  double *y2;     // northern inner border of computational grid
  double *y3;     // northern outer border of computational grid
  int nhh;        // number of grid points in hh[]
  double *hh;     // vertical grid
  double z0;      // roughness length
  double d0;      // zero plane displacement
  double xa;      // anemometer x-position
  double ya;      // anemometer y-position
  double ha;      // anemometer height
  double ua;      // wind velocity
  double ra;      // wind direction
  double lm;      // Monin-Obukhov length
  double hm;      // mixing height
  double km;      // stability class (Klug/Manier)
  double sc;      // stability class (KTA 1..6)         //-2011-11-23
  double ri;      // rain intensity (mm/h)              //-2011-11-23
  double ws;      // statistical weight
  double ef;      // emission factor
  char az[256];   // file name of the meteorological time series
  char as[256];   // file name of the dispersion class statistic
  char os[256];   // option string
  char gh[256];   // file name of ground height
  char lc[256];   // locale
  int qs;         // quality level
  int qb;         // quality level for grids
  int np;               // number of monitor points
  double *xp, *yp, *hp; // coordinates of the monitor points
  int kp;               // maximum k-value for monitor points
  int sd;               // initial seed of the random number generator
  int nq;               // number of sources
  int im;               // maximum number of iterations
  double ie;            // maximum error for iterations
  double mh;            // mean surface height
  double *xq, *yq, *hq; // position and height of sources
  double *aq, *bq, *cq; // horizontal and vertical extension of the sources
  double *xm, *ym;      // midpoint of the sources
  double *wq;           // angle of rotation (ccw)
  double *dq, *vq, *qq; // diameter, exhaust velocity, heat flux (MW)
  double *rq, *lq;      // relative humidity (%), liquid water content (kg/kg)
  double *tq;           // temperature (C) for cooling towers
  double *sq;           // time scale of plume rise
  int nb;               // number of buildings (>0 with xb,...; -1 with bf)
  double *xb, *yb, *ab, *bb, *cb, *wb; // coordinates of buildings ...
  char bf[256];                        // ... or file with building raster
  double *xbmin, *xbmax, *ybmin, *ybmax;
  double *dmk;            // parameters for wind field model DMK
  double **cmp;					  // component names 												//-2011-11-23
} TIPDAT;

extern int cSO2, cNO, cNO2, cNOX, cODOR;

//typedef struct {
//  char name[16]; // name of the component                       //-2008-03-10
//  float vd;     // deposition velocity for gas in cm/s
//  float fc;     // factor for conversion of concentration
//  char uc[16];  // unit of concentration
//  float fn;     // factor for conversion of deposition
//  char un[16];  // unit of deposition
//  // yearly average:
//  float ry;     // reference value
//  int dy;       // number of trailing decimals
//  // daily average:
//  int nd;       // number of allowed exceedings
//  float rd;     // reference value
//  int dd;       // number of trailing decimals
//  // hourly average:
//  int nh;       // number of allowed exceedings
//  float rh;     // reference value
//  int dh;       // number of trailing decimals
//  // deposition:
//  float rn;     // reference value
//  int dn;       // number of trailing decimals
//  //
//  char ss[160];
//} TIPSPCREC;

typedef struct {
  char name[40];
  char lasn[40];
  void *p;
  int o;
  int i;                                                          //-2011-12-08
  ARYDSC dsc;
} TIPVAR;

typedef struct {
  double t;       // end time of the 1 hour interval (GMT+1)
  float fRa;      // wind direction in degree
  float fUa;      // wind velocity in m/s
  float fLm;      // Monin-Obukhov length
  float fPrm[1];  // first of user defined parameters
} TMSREC;

extern int TipSpcIndex( char *cmp_name )
;
extern int TipSpcEmitted( int ks )
;
extern int TipMain( char *s )
;
extern int TipZ0index( double z0 )
;
extern int TipKMclass( double z0, double lm )
;
extern double TipMOvalue( double z0, int kl )
;
extern float TipBlmVersion( void )                                //-2011-09-12
;
extern int TipLogCheck(char *id,  int sum);

extern TIPDAT TI;
extern ARYDSC TipVar;
extern ARYDSC TIPary;
extern char *TalInpVersion;
extern FILE *TipMsgFile;
extern int TalMode;
extern char TalName[];                                            //-2011-11-23
extern int TipTrbExt;

#endif
