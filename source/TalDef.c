//============================================================ TalDef.c
//
// Write DEF-Files for LASAT
// =========================
//
// Copyright (C) Umweltbundesamt, Dessau-Roßlau, Germany, 2002-2011
// Copyright (C) Janicke Consulting, 88662 Überlingen, Germany, 2002-2014
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
// last change: 2014-01-29 uj
//
//==========================================================================

char *TalDefVersion = "2.6.9";
char TalVersion[80];
char TalLabel[80];

static char *eMODn = "TalDef";
static int CHECK = 0;

// #define  LOG_ALLOCATION

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include <math.h>

#include "genutl.h"
#include "IBJmsg.h"
#include "IBJary.h"
#include "IBJtxt.h"
#include "IBJdmn.h"
#include "TalAKT.h"
#include "TalAKS.h"
#include "TalUtl.h"
#include "TalStt.h"
#include "TalInp.h"
#include "TalMat.h"
#include "TalDef.h"
#include  "TalCfg.h"                                              //-2014-01-21
#include "TalDef.nls"

#define TE(i)  ((TMSREC*)AryPtrX(&AryDsc, i))->t
#define RA(i)  ((TMSREC*)AryPtrX(&AryDsc, i))->fRa
#define UA(i)  ((TMSREC*)AryPtrX(&AryDsc, i))->fUa
#define LM(i)  ((TMSREC*)AryPtrX(&AryDsc, i))->fLm

static char Path[256];
static int Mode;
static ARYDSC AryDsc;
static char WindLib[256] = "~../lib";

/*=================================== Time ===============================*/

static char *LocalTmString(     /* return string representation of time t */
       long *pt );              /* pointer to time t                      */
static void LocalTimeStr( long, char* );
static const char TmMinString[] = "-inf";
static const char TmMaxString[] = "+inf";
static const long TmMinValue = LONG_MIN;
static const long TmMaxValue = LONG_MAX;

/*=============================================================== LocalTmString
*/
static char *LocalTmString(     /* return string representation of time t */
  long *pt )                    /* pointer to time t                      */
  {
  static char tstr[40];
  if (!pt)  strcpy( tstr, "NOT");
  else if (*pt == TmMinValue)  strcpy( tstr, TmMinString );
  else if (*pt == TmMaxValue)  strcpy( tstr, TmMaxString );
  else  LocalTimeStr( *pt, tstr );
  return tstr;
  }

/*=================================================================== LocalTimeStr
*/
static void LocalTimeStr(  /* Umwandlung einer Sekundenzahl n in eine Zeichen- */
long n,                    /* kette t[] der Form  ddd.hh:mm:ss;                */
char *t )                  /* führende Nullen werden fortgelassen.             */
  {
  int l = 0;
  long k;
  char b[10];
  t[0] = '\0';
  if (n < 0) {  strcat( t, "-" );  n = -n; }
  k = n / 86400L;
  if (k > 0) {
    sprintf( b, "%ld.", k );  strcat( t, b );  n -= k*86400L;  l++; }
  k = n/3600L;
  sprintf(b, "%02ld:", k);
  strcat(t, b);  n -= k*3600L;  l++;
  k = n/60L;
  sprintf(b, "%02ld:", k);
  strcat(t, b);  n -= k*60L; l++;
  sprintf(b, "%02ld", n);
  strcat(t, b);
  return; }

//===============================================================================

static int endswith( char *s, char *t ) {
  int ls = strlen(s);
  int lt = strlen(t);
  if (lt<=ls && !strcmp(s+ls-lt, t))  return 1;
  return 0;
}

//=============================================================== TdfCopySurface
//
int TdfCopySurface( // copy zg??.dmna to work/srfa??.dmna
char *path )        // working directory
{
  dP(TdfCopySurface);
  int rc, n=0;
  char fn[256];
  ARYDSC dsc;
  TXTSTR usr = { NULL, 0 };
  TXTSTR sys = { NULL, 0 };
  memset(&dsc, 0, sizeof(ARYDSC));
  for (n=0; n<TI.nn; n++) {                                           //-2001-10-31
    sprintf(fn, "%s/zg%02d", path, n+(TI.nn>1));
    rc = DmnRead(fn, &usr, &sys, &dsc);  if (rc < 0)                    eX(3);
    if (dsc.numdm != 2)                                                 eX(6);
    DmnRplValues(&usr, "form", "zg%6.1f");
    DmnRplValues(&usr, "sequ", "\"j-,i+\"");
    DmnRplValues(&usr, "mode", "\"text\"");
    sprintf(fn, "%s/work", path);
    TutDirMake(fn);
    sprintf(fn, "%s/work/srfa0%d%d.dmna", path, TI.gl[n], TI.gi[n]);
    rc = DmnWrite(fn, usr.s, &dsc);  if (rc < 0)                        eX(5); //-2011-06-29
    AryFree(&dsc);
    TxtClr(&usr);
    TxtClr(&sys);
  }
  MsgCode = 1;
  goto clear;
eX_5:
  vMsg(_internal_error_);
  goto clear;
eX_3: eX_6:
  vMsg(_cant_read_surface_file_$_, fn);
clear:
  TxtClr(&usr);
  TxtClr(&sys);
  if (MsgCode < 0)  MsgSource = ePGMs;
  return MsgCode;
}

//============================================================== TdfWriteWetter
//
static int TdfWriteWetter( char *fname ) {
  dP(TdfWriteWetter);
  int i, nn, i0, i1, ihm, iri, nv;
  int ir, kl;
  int wind;
  float ua, ra, lm, ftv[6], ri, hm;
  float su=-1, sv=-1, sw=-1, us=-1, svf=-1, hw=-1;
  char buf[100];
  char *pc;
  float blm_version;
  long n1, n2;
  char t1s[40], t2s[40];
  long ri_seconds;                                                //-2014-01-29
  float ri_total;                                                 //-2014-01-29
  TIPVAR *ptv;
  TMSREC *ptms;                                                   //-2011-12-08
  FILE *tms;
  //
  for (i=0; i<6; i++)  ftv[i] = -1;
  i0 = AryDsc.bound[0].low;
  i1 = AryDsc.bound[0].hgh;
  tms = fopen(fname, "w");  if (!tms)                           eX(1);
  blm_version = TipBlmVersion();                                  //-2011-09-12
  //                                                                -2011-12-08
  ihm = -1;
  iri = -1;
  ri_seconds = 0;
  ri_total = 0;
  if (!TipVar.start)  nv = 0;
  else  nv = TipVar.bound[0].hgh+1;
  for (i=0; i<nv; i++) {
    ptv = AryPtrX(&TipVar, i);
    if (!strcmp(ptv->name, "hm"))
      ihm = ptv->i;
    else if (!strcmp(ptv->name, "ri"))
      iri = ptv->i;
  }
  if (!(TalMode & TIP_HM_USED))
    ihm = -1;
  if (!(TalMode & TIP_RI_USED))
    iri = -1;
  //
  if (NOSTANDARD) {
    pc = strstr(TI.os, "Su=");
    if (pc)  sscanf(pc+3, "%f", &su);
    pc = strstr(TI.os, "Sv=");
    if (pc)  sscanf(pc+3, "%f", &sv);
    pc = strstr(TI.os, "Sw=");
    if (pc)  sscanf(pc+3, "%f", &sw);
    pc = strstr(TI.os, "Us=");
    if (pc)  sscanf(pc+3, "%f", &us);
    pc = strstr(TI.os, "Svf=");                             //-2007-01-30
    if (pc)  sscanf(pc+4, "%f", &svf);
    pc = strstr(TI.os, "Hw=");                              //-2012-11-05
    if (pc)  sscanf(pc+3, "%f", &hw);
    pc = strstr(TI.os, "Ftv=");
    if (pc) {
      GetData("FTV", pc, "%[6]f", ftv);
    }
  }
  fprintf(tms, "- Input file created by %s %s\n", (CfgWet) ? "AUSTAL2000N" : "AUSTAL2000", TalVersion);
  fprintf(tms, "==================================================== meteo.def\n" );
  fprintf(tms, "- TalDef: Meteorological time series %s\n", TI.az );
  fprintf(tms, "-         Umin=%3.1f\n", TatUmin);
  fprintf(tms, ".\n");
  fprintf(tms, "  Version = %1.1f\n", blm_version);
  fprintf(tms, "  Z0 = %5.3lf \n", TI.z0 );
  fprintf(tms, "  D0 = %5.3lf \n", TI.d0 );
  fprintf(tms, "  Xa=%1.1lf  Ya=%1.1lf  Ha=%1.1lf \n", TI.xa, TI.ya, TI.ha );
  fprintf(tms, "  Ua = ? \n" );
  fprintf(tms, "  Ra = ? \n" );
  fprintf(tms, "  Lm = ? \n" );
  if (iri >= 0)                                                   //-2011-12-08
    fprintf(tms, "  Prec = ?     \n");
  if (NOSTANDARD) {                                               //-2002-03-17
    if (ihm >= 0)                                                 //-2011-12-08
      fprintf(tms, "  Hm = ? \n" );
    if (su >= 0)  fprintf(tms, "  Su = %1.5f \n", su);
    if (sv >= 0)  fprintf(tms, "  Sv = %1.5f \n", sv);
    if (sw >= 0)  fprintf(tms, "  Sw = %1.5f \n", sw);
    if (us >  0)  fprintf(tms, "  Us = %1.5f \n", us);
    if (svf > 0)  fprintf(tms, "  Svf = %1.5f \n", svf);          //-2007-01-30
    if (hw > 0)   fprintf(tms, "  Hw = %1.2f \n", hw);            //-2012-11-05
    if (ftv[0] >= 0) {
      fprintf(tms, "  Ftv = {");
      for (i=0; i<6; i++) fprintf(tms, " %1.1f", ftv[i]);
      fprintf(tms, " } \n");
    }
  }
  if (TI.mh > 0) {
    fprintf(tms, "  HmMean = { 0  0  0  %1.0lf  %1.0lf  %1.0lf }\n", //-2013-04-08
            TI.mh+SttHmMean[3], TI.mh+SttHmMean[4], TI.mh+SttHmMean[5]);
  }
//  fprintf(tms, "  Wind = ? \n" );															//-2008-04-17
  fprintf(tms, "  WindLib = %s \n", WindLib);
  fprintf(tms, "---------------------------------------------------\n");
  fprintf(tms, "-  \n" );
  fprintf(tms, "!      T1             T2           Ua     Ra      Lm");
  if (ihm >= 0)  fprintf(tms, "    Hm");                          //-2011-12-08
  if (iri >= 0)  fprintf(tms, "   Prec");                         //-2011-12-08
  fprintf(tms, "\n");
  fprintf(tms, "- (ddd.hh:mm:ss) (ddd.hh:mm:ss)  (m/s) (deg.)     (m)");
  if (ihm >= 0)  fprintf(tms, "   (m)");                          //-2011-12-08
  if (iri >= 0)  fprintf(tms, " (mm/h)");                         //-2011-12-08
  fprintf(tms, "\n");
  nn = 0;
  strcpy(buf, "Z                                   \n" );
  n1 = 0;
  for (i=i0; i<=i1; )  {
    ptms = AryPtrX(&AryDsc, i);
    ua = ptms->fUa;
    ra = ptms->fRa;
    lm = ptms->fLm;
    hm = (ihm >= 0) ? ptms->fPrm[ihm] : 0;
    ri = (iri >= 0) ? ptms->fPrm[iri] : 0;
    kl = TipKMclass(TI.z0, lm);
    ir = 0.5 + 0.1*ra;
    if (ir == 0)  ir = 36;
    wind = 1000*kl + ir;
    n2 = n1 + TI.interval;                                        //-2007-02-03
    strcpy(t1s, LocalTmString(&n1));
    strcpy(t2s, LocalTmString(&n2));
    fprintf(tms, "Z %13s  %13s %6.3f %6.0f %7.1f", t1s, t2s, ua, ra, lm);
    if (ihm >= 0) {                                               //-2011-12-08
      fprintf(tms, " %6.1f", hm);
    }
    if (iri >= 0) {                                               //-2011-12-08
      fprintf(tms, " %6.2f", ri);
      if (lm != 0 && ri > 0) {                                    //-2014-01-29
        ri_seconds += TI.interval;
        ri_total += ri * TI.interval;
      }
    }
    fprintf(tms, "\n");
    n1 += TI.interval;
    i++;
    nn++;
  }
  fprintf(tms, "---------------------------------------------------------\n");
  fclose(tms);
  if (iri >= 0)                                                   //-2014-01-29
    vMsg(_precipitation_$$_, ri_total/3600.0, ri_seconds/3600.0);
  return nn;
eX_1:
  eMSG(_cant_write_$_, fname);
}

//================================================================= TdfWriteMetlib
//
int TdfWriteMetlib( // write metlib.def for generating a wind library
int mode )          // TIP_COMPLEX: 2 base fields for each class
{                   // RETURN: number of entries
  dP(TdfWriteMetlib);
  int i, n, i0, i1;
  int ir, kl, ir1, ir2, irstep;
  int wind, hour, day;
  float ua, ra, lm;
  float umean1[7];
  int nmean1[7];
  char buf[100], fname[256];
  float blm_version;
  FILE *tms;
  if ((mode&TIP_COMPLEX) == 0)  return -2;
  i0 = AryDsc.bound[0].low;
  i1 = AryDsc.bound[0].hgh;
  blm_version = TipBlmVersion();                                 //-2011-09-12
  sprintf(fname, "%s/%s", Path, "metlib.def");
  tms = fopen(fname, "w");  if (!tms)                         eX(1);
  fprintf(tms, "- Input file created by %s %s\n", (CfgWet) ? "AUSTAL2000N" : "AUSTAL2000", TalVersion);
  fprintf(tms, "==================================================== metlib.def\n" );
  fprintf(tms, "- TalDef: Time series %s for library\n", TI.az );
  fprintf(tms, "-         Umin=%3.1f\n", TatUmin);
  fprintf(tms, ".\n");
  fprintf(tms, "  Version = %1.1f\n", blm_version);
  fprintf(tms, "  Z0 = %5.3f \n", TI.z0 );
  fprintf(tms, "  D0 = %5.3f \n", TI.d0 );
  fprintf(tms, "  Xa=%1.1f  Ya=%1.1f  Ha=%1.1f \n", TI.xa, TI.ya, TI.ha );
  fprintf(tms, "  Ua = ? \n" );
  fprintf(tms, "  Ra = ? \n" );
  fprintf(tms, "  Lm = ? \n" );
  if (TI.mh > 0) {
    fprintf(tms, "  HmMean = { 0  0  0  %1.0lf  %1.0lf  %1.0lf }\n", //-2013-04-08
            TI.mh+SttHmMean[3], TI.mh+SttHmMean[4], TI.mh+SttHmMean[5]);
  }
  fprintf(tms, "  Wind = ? \n" );
  fprintf(tms, "  WindLib = %s \n", WindLib);
  fprintf(tms, "---------------------------------------------------------\n");
  fprintf(tms, "-  \n" );
  fprintf(tms, "!     T1   T2     Ua     Ra      Lm Wind ");
  fprintf(tms, "\n");
  fprintf(tms, "-    (ss) (ss)  (m/s) (deg.)     (m)  (1) ");
  fprintf(tms, "\n");
  strcpy(buf, "Z                                   \n" );
  hour = 0;
  day = 0;
  for (kl=0; kl<=6; kl++) {
    umean1[kl] = 0;
    nmean1[kl] = 0;
  }
  for (i=i0; i<=i1; i++)  {
    ua = UA(i);
    lm = LM(i);
    if (lm == 0) continue;                                  //-2007-03-07
    kl = TipKMclass(TI.z0, lm);
    ir = (int)(RA(i)/10.+0.5);
    nmean1[kl] += 1;
    umean1[kl] += ua;
  }
  n = 0;
  for (kl=1; kl<=6; kl++) {
    if (nmean1[kl] == 0)  continue;
    if (mode & TIP_LIB2) {
      ir1 = (int)(RA(i0) + 0.5);
      if (ir1 <= 270)
        ir2 = ir1 + 90;
      else {
        ir2 = ir1;
        ir1 -= 90;
      }
      irstep = 90;
    }
    else if ((mode & TIP_LIB36) || (mode & TIP_BODIES)) {
      ir1 = 10;
      ir2 = 360;
      irstep = 10;
    }
    else {
      ir1 = 180;
      ir2 = 270;
      irstep = 90;
    }
    for (ir=ir1; ir<=ir2; ir+=irstep) {
      lm = TipMOvalue(TI.z0, kl);
      wind = 1000*kl + (int)(ir/10.+0.5);
      ra = ir;
      ua = umean1[kl]/nmean1[kl];
      fprintf(tms, "Z   %4d  %4d %6.3f %6.0f %7.1f %4d\n", n, n+1, ua, ra, lm, wind);
      n++;
    }
  }
  fprintf(tms, "---------------------------------------------------------\n");
  fclose(tms);
  return n;
eX_1:
  eMSG(_cant_write_$_, fname);
}


//============================================================== TdfWriteBodies
//
int TdfWriteBodies( FILE *f ) {
  int i, n, setpoint=1;
  fprintf(f, "================================================== bodies.def\n");
  fprintf(f, ". \n");
  fprintf(f, "  DMKp = { %1.3f %1.3f %1.3f %1.3f %1.3f %1.3f %1.1f "
     "%1.3f %1.3f }\n", TI.dmk[0], TI.dmk[1], TI.dmk[2], TI.dmk[3], TI.dmk[4],
     TI.dmk[5], TI.dmk[6], TI.dmk[7], TI.dmk[8]);  //-2004-12-14
  fprintf(f, "  TrbExt = %d\n", TipTrbExt);
  fprintf(f, "-\n");
  if (TI.nb < 0) {
    /*
    strcpy(name, TI.bf);
    for (pc=name; (*pc); pc++)  if (*pc == '\\')  *pc = '/';
    if (*pc == '/')
      fprintf(f, "  RFile = %s\n", name);
    else
      fprintf(f, "  RFile = ~../%s\n", name);
    */
    fprintf(f, "  RFile = \"%s\"\n", TI.bf);                      //-2008-01-25
  }
  else {
    for (n=0,i=0; i<TI.nb; i++) if (TI.bb[i] > 0) n++;
    if (n) {
      fprintf(f, "- Rectangles \n");
      if (!setpoint) fprintf(f, ".\n");
      fprintf(f, "  Btype = BOX\n");
      fprintf(f, "!  Name  |         Xb         Yb      Hb      Ab      Bb      Cb      Wb\n");
      fprintf(f, "---------+--------------------------------------------------------------\n");
      for (i=0; i<TI.nb; i++)
        if (TI.bb[i] > 0) {
          fprintf(f, "B    %02d  | %10.2f %10.2f %7.2f %7.2f %7.2f %7.2f "
          "%7.2f\n", i+1, TI.xb[i], TI.yb[i], 0., TI.ab[i], TI.bb[i], TI.cb[i],
          TI.wb[i]);  //-2004-12-14
        }
      fprintf(f, "------------------------------------------------------------------------\n");
      setpoint = 0;
    }
    for (n=0,i=0; i<TI.nb; i++) if (TI.bb[i] < 0) n++;
    if (n) {
      fprintf(f, "- Cooling towers \n");
      if (!setpoint) fprintf(f, ".\n");
      fprintf(f, "  Btype = TOWER\n");
      fprintf(f, "!  Name  |         Xb         Yb      Hb      Cb      Db\n");
      fprintf(f, "---------+----------------------------------------------\n");
      for (i=0; i<TI.nb; i++)
        if (TI.bb[i] < 0) {
          fprintf(f, "B    %02d  | %10.2f %10.2f %7.2f %7.2f %7.2f\n",
            i+1, TI.xb[i], TI.yb[i], 0., TI.cb[i], -TI.bb[i]);
        }
      fprintf(f, "--------------------------------------------------------\n");
      setpoint = 0;
    }
  }
  return 0;
}


//================================================================= TdfWriteGrid
//
int TdfWriteGrid( FILE *f ) {
  int k, n, nzd=1, periodic=0;
  char flags[256] = "";
  if (NOSTANDARD) {
    char *pc;
    pc = strstr(TI.os, "PERIODIC");
    if (pc)  periodic = 1;
  }
  fprintf(f, "==================================================== grid.def\n");
  fprintf(f, ". \n");
  fprintf(f, "  RefX = %1.0lf\n", TI.gx);                         //-2008-12-04
  fprintf(f, "  RefY = %1.0lf\n", TI.gy);                         //-2008-12-04
  if (*TI.ggcs)  fprintf(f, "  GGCS = %s\n", TI.ggcs);            //-2008-12-04
  fprintf(f, "  Sk = {");
  for (k=0; k<=TI.nzmax; k++)  fprintf(f, " %1.1lf", TI.hh[k]);
  fprintf(f, " }\n");
  nzd = 1;
  if (TI.kp > nzd)  nzd = TI.kp;                                  //-2001-08-03
  fprintf(f, "  Nzd = %d\n", nzd);
  if (TI.nn > 1)  strcat(flags, "+NESTED");
  if (TI.nb)      strcat(flags, "+BODIES");
  /*
  if (strstr(TI.os, "DMKHM") && strstr(TI.os, "NOSTANDARD"))      //-2005-03-11
      strcat(flags, "+DMKHM");
  */
  if (*flags)    fprintf(f, "  Flags = %s\n", flags);
  if (periodic) {
    fprintf(f, "  Bcx=P  Bcy=P  Bcz=R  Hmax=%1.1lf\n", TI.hh[TI.nzmax]);
  }
  if (TI.nn == 1) {
    fprintf(f, "  Xmin = %1.1lf\n", TI.x0[0]);
    fprintf(f, "  Ymin = %1.1lf\n", TI.y0[0]);
    fprintf(f, "  Delta = %1.1lf\n", TI.dd[0]);
    fprintf(f, "  Nx = %d\n", TI.nx[0]);
    fprintf(f, "  Ny = %d\n", TI.ny[0]);
    if (TI.gh[0]) {                                               //-2001-09-15
      fprintf(f, "  Ntype = COMPLEX\n");
      fprintf(f, "  Im = %d\n", TI.im);
      fprintf(f, "  Ie = %1.2le\n", TI.ie);
    }
    else if (TI.nb) {
      fprintf(f, "  Ntype = FLAT3D\n");
      fprintf(f, "  Im = %d\n", TI.im);
      fprintf(f, "  Ie = %1.2le\n", TI.ie);
    }
    else {
      fprintf(f, "  Ntype = FLAT1D\n");
      fprintf(f, "  Rand = 20\n");
    }
  }
  else {                                                          //-2001-10-30
    int nt;
    double rf;
    if (TI.gh[0]) nt = 3;
    else if (TI.nb) nt = 2;
    else nt = 1;
    fprintf(f, "-\n");
    fprintf(f, "! Nm | Nl Ni Nt Pt     Dd  Nx  Ny  Nz     Xmin     Ymin  Rf  Im       Ie\n");
    fprintf(f, "-----+------------------------------------------------------------------\n");
    for (n=TI.nn-1; n>=0; n--) {
      rf = (n > 1) ? 0.5 : 1.0;
      fprintf(f,
      "N %02d | %2d %2d %2d %2d %6.1lf %3d %3d %3d %8.1lf %8.1lf %3.1lf %3d %8.1le\n",
      n+1, TI.gl[n], TI.gi[n], nt, 3, TI.dd[n], TI.nx[n], TI.ny[n], TI.nz[n],
      TI.x0[n], TI.y0[n], rf, TI.im, TI.ie);
    }
    fprintf(f, "------------------------------------------------------------------------\n");
  }
  return 0;
}

//================================================================= TdfWriteParam
//
static char *ems( double eq ) {
  static char s[32];
  if (eq == HUGE_VAL)  return "         ?";
  sprintf(s, "%12.3e", eq);                                     //-2008-03-10
  return s;
}

static int TdfWriteParam( void )     // write param.def
{
  dP(TdfWriteParam);
  char fname[256], ts[40], flags[256], name[256], buf[256];
  int n, i, i1, i2, k, nox, odor, ks, ic, groups=0, trace=0, spectrum=0;
  int writesfc=0, readsfc=0;
  long tm;
  double t0, rate=0;
  float vd=-1, vs=-1, wf=-1, we=-1;                               //-2011-12-08
  float vdep, vsed, refc, refd, tau=0, threshold, rfak, rexp;
  int pm[] = { 0, 0, 0, 0, 0, 0 };
  FILE *f;
  threshold = SttOdorThreshold;
  if (CHECK) vMsg("TdfWriteParam() ...");
  if (NOSTANDARD) {
    char *pc;
    pc = strstr(TI.os, "Groups=");
    if (pc)  sscanf(pc+7, "%d", &groups);
    pc = strstr(TI.os, "Tau=");
    if (pc)  sscanf(pc+4, "%f", &tau);
    pc = strstr(TI.os, "Rate=");
    if (pc)  sscanf(pc+5, "%lf", &rate);
    pc = strstr(TI.os, "Vd=");
    if (pc)  sscanf(pc+3, "%f", &vd);
    pc = strstr(TI.os, "Vs=");
    if (pc)  sscanf(pc+3, "%f", &vs);
    pc = strstr(TI.os, "Wf=");
    if (pc)  sscanf(pc+3, "%f", &wf);
    pc = strstr(TI.os, "We=");
    if (pc)  sscanf(pc+3, "%f", &we);
    pc = strstr(TI.os, "TRACE");
    if (pc)  trace = 1;
    pc = strstr(TI.os, "SPECTRUM");                               //-2003-06-16
    if (pc)  spectrum = 1;
    pc = strstr(TI.os, "WRITESFC");                              //-2012-10-30
    if (pc)  writesfc = 1;
    pc = strstr(TI.os, "READSFC");                               //-2012-10-30
    if (pc)  readsfc = 1;
  }
  nox = (cNO>=0 && TI.cmp[cNO] != NULL);                          //-2011-11-23
  odor = (cODOR>=0 && TI.cmp[cODOR] != NULL);                     //-2011-11-23
  for (k=0; k<SttCmpCount; k++) {
    if (!TI.cmp[k])  continue;
    strcpy(name, SttCmpNames[k]);
    if      (endswith(name, "-3"))  pm[3] = 1;
    else if (endswith(name, "-4"))  pm[4] = 1;
    else if (endswith(name, "-u"))  pm[5] = 1;
    else  pm[2] = 1;
  }
  strcpy(fname, Path);
  strcat(fname, "/work/meteo.def");
  if (Mode & TIP_SERIES) {
    n = TdfWriteWetter(fname);  if (n < 1)    eX(3);
  }
  else if (Mode & TIP_STAT) {
    char s[512], *pc;
    sprintf(s, "-B%s", WindLib);
    TalAKS(s);
    pc = strstr(TI.os, "TAS(");                     //-2001-11-16
    if (pc) {
      if (strlen(pc) < 512) {
        strcpy(s, pc+4);
        pc = strchr(s, ')');
        if (pc)  *pc = 0;
        TalAKS(s);
      }
    }
    n = TasWriteWetter();  if (n < 0)         eX(13);
    AryDsc = AKSary;
    n = TasWriteStaerke();  if (n < 0)        eX(14);
  }
  //
  // write param.def
  //
  strcpy(fname, Path);
  strcat(fname, "/work/param.def");
  f = fopen(fname, "w");  if (!f)             eX(4);              //-2014-01-21
  fprintf(f, "- Input file created by %s %s\n", (CfgWet) ? "AUSTAL2000N" : "AUSTAL2000", TalVersion);
  fprintf(f, "==================================================== param.def\n");
  fprintf(f, ". \n");
  if (*TI.ti)
    fprintf(f, "  Ident = %s\n", TxtQuote(TI.ti));
  fprintf(f, "  Seed = %d\n", TI.sd);
  *flags = 0;
  i1 = AryDsc.bound[0].low;
  i2 = AryDsc.bound[0].hgh;
  if (Mode & TIP_SERIES) {
    int ints = i2-i1+1;
    tm = TI.interval;                                             //-2007-02-03
    fprintf(f, "  Interval = %s\n", LocalTmString(&tm));				  //-2011-11-23
    t0 = MsgDateIncrease(TE(i1), -TI.interval);
    strcpy(ts, MsgDateString(t0));
    if (!strstr(ts, "00:00:00"))                               eX(5);
    fprintf(f, "  RefDate = %s\n", ts);													  //-2011-11-23
    tm = 0;
    fprintf(f, "  Start = %s\n", LocalTmString(&tm));
    tm = ints * TI.interval;
    fprintf(f, "  End = %s\n", LocalTmString(&tm));								//-2011-11-23
    fprintf(f, "  Average = %d\n", TI.average);
    strcat(flags, "+MAXIMA");
    if (rate <= 0)  rate = 2*pow(2, TI.qs);
  }
  else if (Mode & TIP_STAT) {
    int days = i2-i1+1;
    fprintf(f, "  Start = 0\n");
    fprintf(f, "  End = %d.00:00:00\n", days);										//-2011-11-23
    //
    // to avoid lost of numerical significance                    //-2003-09-23
    //
    // if (TI.cmp[cSO2] || TI.cmp[cNO] || TI.cmp[cNO2]) {
      fprintf(f, "  Interval = 1.00:00:00\n");										//-2011-11-23
      fprintf(f, "  Average = %d\n", days);
    // }
    // else {
    //   fprintf(f, "  Intervall = %d.00:00:00\n", days);
    // }
    if (rate <= 0)  rate = 500*pow(2, TI.qs);
  }
  if (nox)    strcat(flags, "+CHEM");
  if (odor)   strcat(flags, "+ODOR");                         //-2004-08-12
  if (TalMode & TIP_ODORMOD)
              strcat(flags, "+RATEDODOR");                    //-2011-06-09
  if (TalMode & TIP_NODILUTE)
              strcat(flags, "+NODILUTE");                     //-2011-12-01
  if (trace)  strcat(flags, "+ONLYADVEC+EXACTTAU+TRACING");
  if (writesfc) strcat(flags, "+WRITESFC");                 //-2012-10-30
  if (readsfc)  strcat(flags, "+READSFC");                  //-2012-10-30
  if (TI.np && (Mode & TIP_SERIES))  strcat(flags, "+MNT");   //-2001-07-07
  if (*flags)      fprintf(f, "  Flags = %s\n", flags);
  if (tau > 0)     fprintf(f, "  Tau = %1.0f\n", tau);
  if (groups > 0)  fprintf(f, "  Groups = %d\n", groups);					//-2011-11-23
  if (odor > 0)    fprintf(f, "  OdorThr = %6.3f\n", threshold);  //-2005-03-11
  //
  // write grid.def
  //
  TdfWriteGrid(f);
  //
  // write bodies.def
  //
  if (TI.nb) TdfWriteBodies(f);
  //
  // write quellen.def
  //
  fprintf(f, "==================================================== sources.def\n");
  fprintf(f, ". \n");
  fprintf(f, "! Nr. |      Xq      Yq    Hq    Aq    Bq    Cq     Wq   Dq   Vq      Qq    Ts     Lw     Rh     Tt\n");
  fprintf(f, "------+--------------------------------------------------------------------------------------------\n");
  for (i=0; i<TI.nq; i++) {
    double dq, vq, qq, sq, lq, rq, tq;
    qq = TI.qq[i];
    sq = TI.sq[i];                                                  //-2001-12-27
    dq = TI.dq[i];
    vq = TI.vq[i];
    rq = TI.rq[i];                                                  //-2002-12-10
    tq = TI.tq[i];                                                  //-2002-12-10
    lq = TI.lq[i];                                                  //-2002-12-10
    if (sq==HUGE_VAL || sq>0) {
      qq = 0;
      tq = 0;
      lq = 0;
      rq = 0;
    }
    else {
      sq = -1;                                                  //-2002-01-03
    }
    fprintf(f, "Q  %02d |", i+1);
    fprintf(f, " %7.1lf", TI.xq[i]);
    fprintf(f, " %7.1lf", TI.yq[i]);
    fprintf(f, " %5.1lf", TI.hq[i]);
    fprintf(f, " %5.1lf", TI.aq[i]);
    fprintf(f, " %5.1lf", TI.bq[i]);
    fprintf(f, " %5.1lf", TI.cq[i]);
    fprintf(f, " %6.1lf", TI.wq[i]);                                //-2005-03-11
    fprintf(f, " %4.1lf", TI.dq[i]);
    if (vq == HUGE_VAL)  fprintf(f, "    ?");
    else                 fprintf(f, " %4.1lf", vq);
    if (qq == HUGE_VAL)  fprintf(f, "     ?");
    else                 fprintf(f, " %7.3lf", qq);                 //-2006-10-26
    if (sq == HUGE_VAL)  fprintf(f, "     ?");                      //-2001-12-27
    else                 fprintf(f, " %5.1lf", sq);
    if (lq == HUGE_VAL)  fprintf(f, "      ?");                     //-2002-12-10
    else                 fprintf(f, " %6.4lf", lq);
    if (rq == HUGE_VAL)  fprintf(f, "    ?");                       //-2002-12-10
    else                 fprintf(f, " %6.1lf", rq);                 //-2005-03-11
    if (tq == HUGE_VAL)  fprintf(f, "    ?");                       //-2002-12-10
    else                 fprintf(f, " %6.1lf", tq);                 //-2005-03-11
    fprintf(f, "\n");
  }
  fprintf(f, "------+--------------------------------------------------------------------------------------------\n");
  //
  // write stoffe.def
  //
  fprintf(f, "================================================= substances.def\n");
  for (ic=2; ic<6; ic++) {                                        //-2001-07-07
    if (!pm[ic])
      continue;
    fprintf(f, ". \n");
    fprintf(f, "  Name = %s\n", SttGroups[ic]);
    fprintf(f, "  Unit = g\n");																		//-2011-11-23
    fprintf(f, "  Rate = %1.5lf\n", rate);                        //-2005-02-01
    if (vs < 0)  vsed = SttVsVec[ic];                             //-2011-11-29
    else         vsed = vs;
    if (spectrum && ic>2) {                                       //-2003-06-16
      if      (ic == 3)  fprintf(f, "  Diameter = { 10  50 }\n");
      else if (ic == 4)  fprintf(f, "  Diameter = { 50 100 }\n");
      else               fprintf(f, "  Diameter = { 10 100 }\n");
      fprintf(f, "  Probability = { 0 1 }\n");
    }
    else {
      fprintf(f, "  Vsed = %1.4f\n", vsed);                       //-2004-06-08
    }
    fprintf(f, "-\n");
    fprintf(f, "! Substance |       Vdep       Refc       Refd       Rfak  Rexp\n");
    fprintf(f, "------------+--------------------------------------------------\n");
    for (k=0; k<SttCmpCount; k++) {                               //-2011-11-23
      if (!TI.cmp[k])
        continue;
      strcpy(name, SttCmpNames[k]);
      ks = TipSpcIndex(name);
      if (ks >= 0) {
        refc = SttSpcTab[ks].ry/SttSpcTab[ks].fc;
        refd = SttSpcTab[ks].rn/SttSpcTab[ks].fn;
      }
      else {
        refc = 0;
        refd = 0;
      }
      vdep = SttVdVec[ic];                                        //-2011-11-29
      rfak = SttWfVec[ic];                                        //-2011-12-05
      rexp = SttWeVec[ic];                                        //-2011-12-05
      if      (endswith(name, "-1")) {
        if (ic != 2)
          continue;
        vdep = SttVdVec[1];                                       //-2011-11-29
        rfak = SttWfVec[1];                                       //-2011-12-05
        rexp = SttWeVec[1];                                       //-2011-12-05
      }
      else if (endswith(name, "-2")) {
        if (ic != 2)
          continue;
      }
      else if (endswith(name, "-3")) {
        if (ic != 3)
          continue;
      }
      else if (endswith(name, "-4")) {
        if (ic != 4)
          continue;
      }
      else if (endswith(name, "-u")) {
        if (ic != 5)
          continue;
      }
      else {  // gases
        if (ic != 2)
          continue;
        if (ks >= 0) {
          rfak = SttSpcTab[ks].wf;
          rexp = SttSpcTab[ks].we;
          vdep = SttSpcTab[ks].vd;
        }
        else {
          rfak = 0;
          rexp = 1;
          vdep = 0;
        }
      }
      if (vd >= 0)  vdep = vd;
      if (wf >= 0)  rfak = wf;                                    //-2011-12-08
      if (we >= 0)  rexp = we;                                    //-2011-12-08
      else if (spectrum && ic>2)  vdep = 0.01;                    //-2003-06-16
      fprintf(f, "K  %-8s | %10.3e %10.3e %10.3e %10.3e %5.2f\n",
          name, vdep, refc, refd, rfak, rexp);                    //-2011-11-23
    }
    fprintf(f, "------------+--------------------------------------------------\n");
  }
  //
  // write chemie.def
  //
  if (nox) {
    fprintf(f, "==================================================== chemics.def\n");
    fprintf(f, ". \n");
    fprintf(f, "! created\\from |  gas.no\n");
    fprintf(f, "---------------+--------\n");
    fprintf(f, "C  gas.no2     |       ?\n");
    fprintf(f, "C  gas.no      |       ?\n");
    fprintf(f, "---------------+--------\n");
  }
  //
  // write staerke.def
  //
  fprintf(f, "==================================================== emissions.def\n");
  fprintf(f, ". \n");
  if (Mode & TIP_STAT)  fprintf(f, "  EmisFac = ?\n-\n");
  fprintf(f, "! SOURCE |");
  for (k=0; k<SttCmpCount; k++) {                                 //-2011-11-23
    if (!TI.cmp[k])
      continue;
    strcpy(name, SttCmpNames[k]);
    if      (endswith(name, "-3"))  ic = 3;
    else if (endswith(name, "-4"))  ic = 4;
    else if (endswith(name, "-u"))  ic = 5;
    else  ic = 2;
    sprintf(buf, "%s.%s", SttGroups [ic], name);                  //-2011-12-08
    fprintf(f, " %12s", buf);                                     //-2011-12-08
  }
  fprintf(f, "\n");
  fprintf(f, "---------+");
  for (k=0; k<SttCmpCount; k++) {                                 //-2011-11-23
    if (!TI.cmp[k])
      continue;
    fprintf(f, "-------------");                                  //-2008-03-10
  }
  fprintf(f, "\n");
  for (i=0; i<TI.nq; i++) {
    fprintf(f, "E     %02d |", i+1);
    for (k=0; k<SttCmpCount; k++) {                               //-2011-11-23
      if (!TI.cmp[k])
        continue;
      strcpy(name, SttCmpNames[k]);
      fprintf(f, " %s", ems(TI.cmp[k][i]));
    }
    fprintf(f, "\n");
  }
  fprintf(f, "---------+");
  for (k=0; k<SttCmpCount; k++) {                                 //-2011-11-23
    if (!TI.cmp[k])
      continue;
    fprintf(f, "-------------");                                  //-2008-03-10
  }
  fprintf(f, "\n");
  //
  // write monitor.def
  //
  if (TI.np>0) {                                                  //-2013-07-08
    fprintf(f, "==================================================== monitor.def\n");
    fprintf(f, ". \n");
    fprintf(f, "! Nr. |      Xp      Yp    Hp\n");
    fprintf(f, "------+----------------------\n");
    for (i=0; i<TI.np; i++)
      fprintf(f, "M  %02d | %7.1lf %7.1lf %5.1lf\n",
              i+1, TI.xp[i], TI.yp[i], TI.hp[i]);
    fprintf(f, "------+----------------------\n");
  }
  fprintf(f, "====================================================\n");
  fclose(f);
  return 0;
eX_3: eX_13: eX_14:
  eMSG(_cant_write_series_$_, fname);
eX_4:
  eMSG(_cant_write_$_, fname);
eX_5:
  eMSG(_no_daystart_at_00_$_, ts);
}

//========================================================== TdfWriteVariabel
//
static int TdfWriteVariabel( void )  // write variable.def
{
  dP(TdfWriteVariabel);
  char fname[256], t1s[40], t2s[40];
  int i0, i1, i, k, nv, iv, nox;
  int ll = 0;                                                     //-2008-07-30
  int lmax = 124;                                                 //-2008-07-30
  long n1, n2;
  float *pf, r1, r2;
  FILE *f;
  TIPVAR *ptv;
  if (CHECK) vMsg("TdfWriteVariabel() ...\n");
  nox = (TI.cmp[cNO] != NULL);
  //
  if (!nox && !(Mode & TIP_VARIABLE))
    return 0;
  //
  if (!TipVar.start)  nv = 0;
  else  nv = TipVar.bound[0].hgh+1;
  strcpy(fname, Path);
  strcat(fname, "/work/variable.def");
  f = fopen(fname, "w");
  if (!f) {
    vMsg(_cant_write_$_, fname);
    return -1;
  }
  i0 = AryDsc.bound[0].low;
  i1 = AryDsc.bound[0].hgh;
  fprintf(f, "- Input file created by %s %s\n", (CfgWet) ? "AUSTAL2000N" : "AUSTAL2000", TalVersion);
  fprintf(f, "==================================================== variable.def\n");
  fprintf(f, ". \n");
  if (nox) {
    fprintf(f, "  gas.no2-gas.no = R2\n");
    fprintf(f, "  gas.no-gas.no = R1\n");
  }
  for (iv=0; iv<nv; iv++) {
    ptv = AryPtr(&TipVar, iv);   if (!ptv)                                eX(10);
    if (*(ptv->lasn))  fprintf(f, "  %s = %s\n", ptv->lasn, ptv->name);
  }
  fprintf(f, "-\n");
  fprintf(f, "!           T1           T2");
  ll = 27;
  if (nox) {
    fprintf(f, "          R2          R1");
    ll += 24;
  }
  for (iv=0; iv<nv; iv++) {
    if (ll + 11 > lmax) {
      fprintf(f, "\n");
      ll = 0;
    }
    ptv = AryPtr(&TipVar, iv);   if (!ptv)                              eX(10);
    if (!strcmp(ptv->name, "hm")) continue;                       //-2012-04-09
    if (!strcmp(ptv->name, "ri")) continue;                       //-2012-04-09
    fprintf(f, " %10s", ptv->name);
    ll += 11;
  }
  fprintf(f, "\n");
  fprintf(f, "---------------------------");
  ll = 27;                                                        //-2008-07-30
  if (nox) {
    fprintf(f, "------------------------");
    ll += 24;                                                     //-2008-07-30
  }
  for (iv=0; iv<nv; iv++) {
    if (ll + 11 > lmax)                                           //-2008-07-30
      break;
    fprintf(f, "-----------");
    ll += 11;                                                     //-2008-07-30
  }
  fprintf(f, "\n");
  n1 = 0;
  for (i=i0; i<=i1; i++) {
    k = TipKMclass(TI.z0, LM(i)) - 1;
    n2 = n1 + TI.interval;                                   //-2007-02-03
    strcpy(t1s, LocalTmString(&n1));
    strcpy(t2s, LocalTmString(&n2));
    fprintf(f, "Z %12s %12s", t1s, t2s);
    ll = 27;                                                      //-2008-07-30
    if (nox) {
      r1 = -1.0/(SttNoxTimes[k]*3600);
      r2 = -r1*46.0/30.0;
      fprintf(f, " %11.3e %11.3e", r2, r1);
      ll += 24;                                                   //-2008-07-30
    }
    for (iv=0; iv<nv; iv++) {
      ptv = AryPtr(&TipVar, iv);   if (!ptv)                              eX(11);
      if (!strcmp(ptv->name, "hm")) continue;
      if (!strcmp(ptv->name, "ri")) continue;                     //-2011-11-23
      pf = AryOPtr(ptv->o, &AryDsc, i);  if (!pf)                         eX(12);
      if (ll + 11 > lmax) {
        fprintf(f, "\n");
        ll = 0;                                                   //-2008-07-30
      }
      fprintf(f, " %10.3e", *pf);
      ll += 11;                                                   //-2008-07-30
    }
    fprintf(f, "\n");
    n1 += TI.interval;
  }
  fprintf(f, "---------------------------");
  ll = 27;                                                        //-2008-07-30
  if (nox) {
    fprintf(f, "------------------------");
    ll += 24;                                                     //-2008-07-30
  }
  for (iv=0; iv<nv; iv++) {
    if (ll + 11 > lmax)                                           //-2008-07-30
      break;
    fprintf(f, "-----------");
    ll += 11;                                                     //-2008-07-30
  }
  fprintf(f, "\n");
  fclose(f);
  return 0;
eX_10: eX_11: eX_12:
  eMSG(_internal_error_);
}

//================================================================== TdfWrite
//
int TdfWrite(       // write def-files
char *path,         // working directory
ARYDSC *pary,       // time series or statistics
DMNFRMREC *frmtab,  // format table for array record
int mode )          // working mode (TIP_SERIES, TIP_STAT, TIP_VARIABLE)
{
  dP(TdfWrite);
  int n;
  if (CHECK) vMsg("TdfWrite() ...");
  if (!pary)                                      eX(11);
  strcpy(Path, path);
  AryDsc = *pary;
  AryDsc.usrcnt = -1;
  Mode = mode;
  if (mode & TIP_LIBRARY) {
    n = TdfWriteMetlib(mode);
    return n;
  }
  n = TdfWriteParam();  if (n < 0)  return n;
  if (Mode & TIP_SERIES) {
    n = TdfWriteVariabel();  if (n < 0)  return n;
  }
  return n;
eX_11:
  eMSG(_internal_error_);
}

//==============================================================================
//
// history:
//
// 2001-07-07 lj 0.5.3 monitor.def for time series only
//                     pm-3, pm-4 and pm-u
// 2001-07-09 lj 0.6.0 release candidate
// 2001-08-03 lj 0.6.1 nzd>1 if required
// 2001-09-23 lj 0.8.0 release candidate
// 2001-10-30 lj 0.8.1 grid nesting
// 2001-11-05 lj 0.9.0 release candidate
// 2001-11-16 lj 0.9.1 parameter for TalAKS form TI.os
// 2002-01-03 lj 0.10.1 default value of sq = -1
// 2002-03-17 lj 0.11.1 parameter mixing height
// 2002-03-28 lj 0.11.2 adaption to new vMsg()
// 2002-06-20 lj 0.12.2 wind direction ra as float
// 2002-09-23 lj 0.13.2 source parameter RH and LW
// 2002-09-24 lj 1.0.0  final release candidate
// 2002-12-10 lj 1.0.4  parameter lq, rq, tq allowed to vary with time
// 2003-09-23 lj 1.1.10 avoid lost of numerical significance (AKS)
// 2004-06-08 lj 1.1.18 write vd using %10.3e
// 2004-06-10 lj 2.1.1  species odor
// 2004-10-04 lj 2.1.3  no standard option ODMOD
// 2004-10-25 lj 2.1.4  factors ftv[] in NOSTANDARD
// 2004-12-14 uj 2.1.5  angle wb in degree
// 2005-02-01 uj 2.1.6  write out Rate with 5 decimal places
// 2005-02-15 uj 2.1.14 option DMKHM
// 2005-03-11 lj 2.1.16 output to *.def with 1 fractional digit, OdorThr
// 2005-03-17 uj 2.2.0  version number upgrade
// 2006-02-06 uj 2.2.7  TmString->LocalTmString, TimeStr->LocalTimeStr
//                      unused function Seconds() removed
// 2006-02-13 uj 2.2.8  nostandard option SPREAD
// 2006-10-18 lj 2.3.0  externalization of strings
// 2006-10-23 uj 2.3.1  qq with three decimal places
// 2007-01-30 uj 2.3.4  define Svf via option string
// 2007-02-03 uj 2.3.5  time interval revised
// 2007-03-07 uj 2.3.6  calculation of uamean corrected
// 2008-01-25 uj 2.3.7  allow absolute path for rb
// 2008-03-10 lj 2.4.0  evaluation of rated odor frequencies
// 2008-04-17 lj 2.4.1  merged with 2.3.x
// 2008-12-04 lj 2.4.5  writes "refx", "refy", "ggcs"
// 2011-06-09 uj 2.4.10 dummy flag RATEDODOR
// 2011-06-29 uj 2.5.0  write srfa000 as dmna file
// 2011-07-07 uj        NOSTANDARD option PRFMOD
// 2011-09-12 uj 2.5.1  blm version from TalInp
// 2011-11-23 lj 2.6.0  precipitation, settings
// 2011-11-29 lj        no re-scaling of vdep and vsed
// 2011-12-01 lj        option NODILUTE
// 2011-12-05 lj        wet deposition of dust
// 2012-11-05 uj 2.6.5  anemometer height hw for base field superposition
// 2013-04-08 uj 2.6.6  definition of HmMean without ","
// 2013-07-08 uj        write monitor.def also for AKS
// 2014-01-29 uj 2.6.9  split AUSTAL2000/AUSTAL2000N, prec info
//
//==============================================================================
