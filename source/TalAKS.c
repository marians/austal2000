//================================================================= TalAKS.c
//
// Convert AKS from DWD into a time series for AUSTAL2000
// ======================================================
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
//  last change 2014-01-29 uj
//
//==========================================================================

char *TalAKSVersion = "2.6.9";
static char *eMODn = "TalAKS";

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <ctype.h>

#include "TalUtl.h"
#include "IBJmsg.h"
#include "IBJary.h"
#include "TalStt.h"
#include "TalInp.h"
#include "TalAKS.h"
#include "TalCfg.h"
#include "TalAKS.nls"

#define  TAL_PRINT  0x0001
#define  TAL_NOX    0x0002
#define  STD_CHECK  0x0010

#define  MAXDAYS 24800                                            //-2013-10-14

#define  NKL     6
#define  NKLalt  7
#define  NWR    36  
#define  NWRalt 38
#define  NWGneu  9
#define  NWGvdi 15
#define  NWGalt 32
#define  BUFLEN  4000
#define  MAXKMP    60
#define  MAXSTAT   10

#define Stat          (pstat->data)
#define TimeStr(a,b)  strcpy((b), MsgDateString((a)/86400.0))
#define vLOG          vMsg

typedef struct {
  float data[NKLalt][NWGalt][NWRalt];
} STAT;

enum STATFORMS { SPECIAL, NEU, ALT, VDI, UNK };

typedef struct StatDef {
  STAT *pstat;
  char name[256];
  float weight, rain;
  enum STATFORMS format;
} STATDEF;

ARYDSC AKSary;

#define TE(i)  ((AKSREC*)AryPtrX(&AKSary, i))->t
#define RA(i)  ((AKSREC*)AryPtrX(&AKSary, i))->iRa
#define UA(i)  ((AKSREC*)AryPtrX(&AKSary, i))->iUa
#define KL(i)  ((AKSREC*)AryPtrX(&AKSary, i))->iKl
#define NS(i)  ((AKSREC*)AryPtrX(&AKSary, i))->ns
#define SG(i)  ((AKSREC*)AryPtrX(&AKSary, i))->sg
#define EF(i)  ((AKSREC*)AryPtrX(&AKSary, i))->ef
#define RI(i)  ((AKSREC*)AryPtrX(&AKSary, i))->ri

static char StdPath[256];
static int StdStatus;
static STATDEF Stats[MAXSTAT];

static float WgVDI[NWGvdi] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 };
static float WgTAL[NWGalt] = { 1.0,  1.5,  2.0,  3.0,  4.5, 6.0, 7.5, 9.0, 12.0 };
static float Kl[NKL] = { 1.0,  2.0,  3.1,  3.2,  4.0,  5.0 };
static float Fv[NKL] = { 1000, 10, 0.25, 0.35, 1000, 1000 };
static float Fw[NKL] = { 0.13, 0.13, 0.15, 0.13, 0.13, 0.13 };
static float NumHpA = 365*24;      /* Stunden   */
static float NumSpD = 24*3600;     /* Sekunden  */
static float Z0=0, D0=0;
static float Xa=0, Ya=0, Ha=10;
static char WindLib[512] = "~";
static char Append[512] = "";
static int Nstat;
static int Nwg;
static int Nwgs[] = { 0, 9, 32, 15 };
enum STATFORMS SrcFormat, DstFormat;
static char Hdr[5][256];
static int  ConvertStatistics=0;
static char PrintMode;

static char DefName[120] = "param.def";
static int NumTtl;
static int Nsub = 5, WndR, WndK, WndG;
static char StatName[80];
static int CreateLib;
static int DtSubMin = 20;

/*================================================================= CheckStat
*/
static void CheckStat( char *t, STATDEF *pdef )
  {
//  dP(CheckStat);
  int ikl, iwg, iwr, nwg;
  float summe, total;
  STAT *pstat;
  pstat = pdef->pstat;
  nwg = Nwgs[pdef->format];
  total = 0;
  for (ikl=0; ikl<NKLalt; ikl++) {
    summe = 0;
    for (iwg=0; iwg<nwg; iwg++)
      for (iwr=0; iwr<NWRalt; iwr++)
        summe += Stat[ikl][iwg][iwr];
    total += summe;
    vLOG(_$_class_$$_, t, ikl+1, summe);
    }
  vLOG(_$_sum_$_, t, total);
  }


/*================================================================= NormStat
*/
static float NormStat( STATDEF *pdef )
  {
//  dP(NormStat);
  int ikl, iwg, iwr, nwg;
  float summe, faktor;
  STAT *pstat;
  pstat = pdef->pstat;
  nwg = Nwgs[pdef->format];
  summe = 0;
  for (ikl=0; ikl<NKL; ikl++)
    for (iwg=0; iwg<nwg; iwg++)
      for (iwr=0; iwr<NWR; iwr++)
        summe += Stat[ikl][iwg][iwr];
  fprintf((MsgFile) ? MsgFile : stdout,
    _stat_$$_normalized_, pdef->name, summe);
  faktor = 1/summe;
  for (ikl=0; ikl<NKL; ikl++)
    for (iwg=0; iwg<nwg; iwg++)
      for (iwr=0; iwr<NWR; iwr++)
        Stat[ikl][iwg][iwr] *= faktor;
  return summe;
  }

//=============================================================== TasReadStat
//
static int TasReadStat( char *sname )
  {
  dP(TasReadStat);
  char buf[800], *pt, fname[256], name[256], *s, *pc;
  int i, j, ikl, iwg, iwr, l, n, nwg, istat;
  float a, summea, g[9], summes, sum_rain;                        //-2011-12-20
  FILE *ein;
  STAT *pstat;
  STATDEF *pdef;
  strcpy(name, sname);
  for (i=strlen(name)-1; i>=0; i--)
    if (name[i] == '\\')  name[i] = '/';
  if (*name=='/' || strchr(name, ':'))  strcpy(fname, name);
  else  sprintf(fname, "%s/%s", StdPath, name);
  TipLogCheck("AKS", TutGetCrc(fname));                           //-2011-12-07
  ein = fopen( fname, "r" );
  if (ein == NULL)                                              eX(1);
  n = 0;
  if (MsgFile)  fprintf(MsgFile, "\n");                           //-2001-09-25
  for (i=0; i<4; i++) {
    s = Hdr[i];
    if (NULL == fgets(s, 120, ein))                           eX(2);
    n++;
    s[120] = 0;
    for (l=strlen(s)-1; l>=0; l--) {
      if ((unsigned char)s[l] < ' ')  s[l] = ' ';
      s[l] = toupper(s[l]);
    }
    fprintf((MsgFile) ? MsgFile : stdout, "%d: %s\n", i+1, s);
    if (i == 2) {
      if      (strstr(s, "ALTES FORMAT"))  SrcFormat = ALT;
      else if (strstr(s, "NEUES FORMAT"))  SrcFormat = NEU;
      else if (strstr(s, "TA LUFT"))  SrcFormat = NEU;
      else if (strstr(s, "TA-LUFT"))  SrcFormat = NEU;
      else if (strstr(s, "VDI FORMAT"))  SrcFormat = VDI;
      else if (NULL != (pc=strstr(s, "SPECIAL"))) {
        SrcFormat = SPECIAL;
          nwg = sscanf(pc+8, "%f,%f,%f,%f,%f,%f,%f,%f,%f",
            g+0, g+1, g+2, g+3, g+4, g+5, g+6, g+7, g+8);
        if (nwg < 1)                      eX(17);
        for (j=0; j<nwg; j++)  WgTAL[j] = g[j];
        Nwgs[0] = nwg;
      }
      else if (Nwgs[0] > 0)  SrcFormat = SPECIAL;
      else  SrcFormat = UNK;
    }
  }
  if (SrcFormat != NEU)                                         eX(20);
  nwg = Nwgs[SrcFormat];
  //
  summes = 0;
  sum_rain = 0;                                                   //-2011-12-20
  for (istat = 0; istat<Nstat; istat++) {
    pdef = Stats + istat;
    i = 4;
    s = Hdr[i];
    if (NULL == fgets(s, 120, ein))                           eX(2);
    n++;
    s[120] = 0;
    for (l=strlen(s)-1; l>=0; l--) {
      if ((unsigned char)s[l] < ' ')  s[l] = ' ';
      s[l] = toupper(s[l]);
    }
    fprintf((MsgFile) ? MsgFile : stdout, "%d: %s\n", i+1, s);
    if (!strncmp(s, SttRiSep, strlen(SttRiSep)))                  //-2011-12-06
      Nstat = SttNstat;                                           //-2011-12-06
    else if (istat > 0)                                           eX(9);
    pstat = ALLOC(sizeof(STAT));  if (!pstat)                     eX(10);
    pdef->pstat = pstat;
    for (ikl=0; ikl<NKLalt; ikl++)
      for (iwg=0; iwg<NWGalt; iwg++)
        for (iwr=0; iwr<NWRalt; iwr++)  Stat[ikl][iwg][iwr] = 0;
    for (ikl=0; ikl<NKL; ikl++) {
      summea = 0;
      for (iwg=0; iwg<nwg; iwg++) {
        if (NULL == fgets(buf, 800, ein))                       eX(3);
        n++;
        for (l=strlen(buf)-1; l>=0; l--)
          if ((unsigned char)buf[l] < ' ')  buf[l] = ' ';
        pt = strtok( buf, " ,\t\n\r" );
        for (iwr=0; iwr<NWR; iwr++) {
          if (!pt)                                                eX(4);
          if (1 > sscanf( pt, "%f", &a ))                         eX(5);
          Stat[ikl][iwg][iwr] = a;
          summea += a;
          pt = strtok( NULL, " ,\t\n\r" );
        }
        if (pt)                                                   eX(21);
      }
      fprintf((MsgFile) ? MsgFile : stdout, _class_$_sum_$_, ikl+1, summea);
    }
    pdef->rain = (CfgWet) ? SttRiVec[istat] : 0;                  //-2014-01-21
    sum_rain += pdef->rain;                                       //-2011-12-20
    strcpy(pdef->name, sname);
    pdef->format = SrcFormat;
    a = NormStat(pdef);
    pdef->weight = a;
    summes += a;
  }
  fclose(ein);
  for (istat=0; istat<Nstat; istat++) {
    pdef = Stats + istat;
    pdef->weight /= summes;
  } 
  return (sum_rain > 0);                                          //-2011-12-20
eX_1:
  eMSG(_cant_read_$_, fname);
eX_2:
  eMSG(_eof_$$_, fname, n);
eX_3:  eX_4:  eX_5:
  vMsg(_input_$_, buf);
  eMSG(_eof_$$$$$_, fname, n, ikl, iwg, iwr);
eX_9:
  eMSG(_invalid_AKS_with_rain_);
eX_10:
  eMSG(_cant_allocate_);
eX_17:
  eMSG(_invalid_groups_);
eX_20:
  eMSG(_TAL_required_);
eX_21:
  eMSG(_format_error_);
  }

//=================================================================== TasRead
//
int TasRead( void ) {
  int n;
  int use_rain = 0;
  float sumrain, totrain;
  totrain = 0;
  TasReadStat(StatName);
  use_rain = TasRain();
  if (use_rain) {                                                 //-2014-01-29
  		totrain = 0;
    for (n=0; n<Nstat; n++) {
      sumrain = Stats[n].weight*Stats[n].rain*NumHpA;
      totrain += sumrain;
      vMsg("%d.  %12s  %5.1f %%  %4.1f mm/h --> %3.0f mm/a",
        n+1, Stats[n].name, 100*Stats[n].weight, Stats[n].rain, sumrain);
    }
    vMsg(_precipitation_$_, totrain);     
  }
  return (totrain > 0);
}

//================================================================ TasWriteStat
//
static int WriteStat( STATDEF *pdef )
  {
  dP(TasWriteStat);
  int i, j, l, ikl, iwg, iwr, nwg, jj;
  char fname[256], ss[256], *cp, ext[10];
  FILE *fp;
  STAT *pstat;
  l = strlen(pdef->name);
  for (i=l-1; i>=0; i--)
     if (pdef->name[i]=='.') break;
  if (i<0) strcpy(ss, pdef->name);
  else {
    for (j=0; j<i; j++) { ss[j] = pdef->name[j]; }
    ss[j] = '\0';
    for (j=i,jj=0; j<l; j++,jj++) { ext[jj] = pdef->name[j]; }
    ext[jj] = '\0';
    }
  sprintf(fname, "%s/%s%s", StdPath, ss, "_new");
  if (i>=0) strcat( fname, ext);
  cp = strstr(Hdr[2], "ALTES FORMAT");
  if (cp != NULL) {
    l = strlen( cp );
    strncpy( ss, Hdr[2], l-1);
    ss[l-1] = '\0';
    strcat( ss, "ALTES -> NEUES FORMAT)");
    strcpy( Hdr[2], ss);
    }
  remove( fname );
  fp = fopen( fname, "w");  if (fp == NULL)           eX(2);
  for (i=0; i<5; i++)
     fprintf(fp, "%s\n", Hdr[i]);
  nwg = Nwgs[pdef->format];
  pstat  = Stats[0].pstat;
  for (ikl=0; ikl<NKL; ikl++) {
     for (iwg=0; iwg<nwg; iwg++) {
        for (iwr=0; iwr<NWR; iwr++)
        fprintf(fp, "%5.0f ", Stat[ikl][iwg][iwr]*1.e+5);
        fprintf(fp, "\n");
        }
     }
  vLOG(_file_$_written_, fname);
  fclose(fp);
  return 0;
eX_2:
  eMSG(_cant_open_$_, fname);
  }

/*=================================================================== PrnStat
*/
static void PrnStat( STATDEF *pdef )
  {
  int ikl, iwg, iwr, nwg, frm;
  long t;
  FILE *aus;
  STAT *pstat;
  char ustr[80];
  float wg[NWGalt], f, sr[NWGalt], sg, total, w, vm;
  pstat = pdef->pstat;
  frm = pdef->format;
  nwg = Nwgs[frm];
  if (frm==NEU || frm==SPECIAL)
    for (iwg=0; iwg<nwg; iwg++)  wg[iwg] = WgTAL[iwg];
  else if (frm == VDI)
    for (iwg=0; iwg<nwg; iwg++)  wg[iwg] = WgVDI[iwg];
  else
    for (iwg=0; iwg<nwg; iwg++)  wg[iwg] = 0.5*iwg;
  aus = MsgFile;  if (!aus)  return;
  switch (PrintMode) {
    case 's': strcpy(ustr, "s/d");
              f = NumSpD*pdef->weight;
              break;
    case 'h': strcpy(ustr, "h/a");
              f = NumHpA*pdef->weight;
              break;
    default:  strcpy(ustr, "10 ppm");
              f = 1.e5*pdef->weight;
    }
  fprintf(aus, "\n");
  for (ikl=0; ikl<NKL; ikl++) {
    fprintf(aus, _list1_$$$$_, pdef->name, pdef->rain, Kl[ikl], ustr);
    fprintf(aus, "%s", _list2_);
    for (iwg=0; iwg<nwg; iwg++)  fprintf(aus, "%6.1f", wg[iwg]);
    fprintf(aus, "%s", _list3_);
    for (iwg=0; iwg<nwg; iwg++)  fprintf(aus, "------");
    fprintf(aus, "-+------\n");
    for (iwg=0; iwg<nwg; iwg++)  sr[iwg] = 0;
    for (iwr=0; iwr<NWR; iwr++) {
      fprintf(aus, "%4d |", 10*(iwr+1));
      sg = 0;
      for (iwg=0; iwg<nwg; iwg++) {
        w = Stat[ikl][iwg][iwr];
        sg += w;
        sr[iwg] += w;
        t = 0.5 + w*f;
        switch (PrintMode) {
          case 's': fprintf(aus, "%3ld:%02ld", t/60, t%60);
                    break;
          default:  fprintf(aus, "%6ld", t);
          }
        }
      t = 0.5 + sg*f;
      switch (PrintMode) {
        case 's': fprintf(aus, " |%3ld:%02ld\n", t/60, t%60);
                  break;
        default:  fprintf(aus, " |%6ld\n", t);
        }
      }
    fprintf(aus, "-----+");
    for (iwg=0; iwg<nwg; iwg++)  fprintf(aus, "------");
    fprintf(aus, "-+------\n");
    fprintf(aus, "%s", _list4_);
    total = 0;
    vm = 0;
    for (iwg=0; iwg<nwg; iwg++) {
      total += sr[iwg];
    vm += sr[iwg]*wg[iwg];
      t = 0.5 + sr[iwg]*f;
      switch (PrintMode) {
        case 's': fprintf(aus, "%3ld:%02ld", t/60, t%60);
                  break;
        default:  fprintf(aus, "%6ld", t);
        }
      };
    t = 0.5 + total*f;
    switch (PrintMode) {
      case 's': fprintf(aus, " |%3ld:%02ld\n", t/60, t%60);
                break;
      default:  fprintf(aus, " |%6ld\n", t);
      }
  if (total > 0)  vm /= total;
  fprintf(aus, _list5_$_, vm);
    }
  return;
  }

//=================================================================== TasRain
//
int TasRain( void ) {                                             //-2011-12-06
  int k, use_rain;
  use_rain = 0;
  for (k=0; k<Nstat; k++) {
    if (Stats[k].rain > 0) {
      use_rain = 1;
      break;
    }
  }
  return use_rain;
}

/*============================================================== TasWriteWetter
*/
int TasWriteWetter( void )
  {
  dP(WriteWetter);
  int ikl, iwg, iwr, i, k, n, nwg, frm, ir, iu;
  int jkl, jwr, jwg;
  long t, dt;
  float gewicht, weight, wg[NWGalt], xt, z0, d0, rain;
  STAT *pstat;
  FILE *aus;
  char t1s[40], t2s[40], fname[256];
  float blm_version = TipBlmVersion();                            //-2011-09-12
  int use_rain = 0;
  //
  use_rain = TasRain();                                           //-2011-12-06
  //
  sprintf(fname, "%s/%s", StdPath, "work/meteo.def");
  aus = fopen(fname, "w");  if (!aus)                           eX(1);
  fprintf( aus, "============================================ meteo.def\n");
  fprintf( aus, _comment_$$$_, (CfgWet) ? "AUSTAL2000N" : "AUSTAL2000", TalAKSVersion, StatName );
  fprintf( aus, ".\n" );
  fprintf( aus, "  Version = %1.1f\n", blm_version );
  fprintf( aus, "  Interval = 3600\n" );
  fprintf( aus, "  Xa=%1.1f  Ya=%1.1f  Ha=%1.1f\n",
    Xa, Ya, Ha );
  if (TI.mh > 0) {
    fprintf(aus, "  HmMean = { 0  0  0  %1.0lf  %1.0lf  %1.0lf }\n",  //-2013-04-08
            TI.mh+SttHmMean[3], TI.mh+SttHmMean[4], TI.mh+SttHmMean[5]);
  }
  z0 = Z0;
  d0 = D0;
  fprintf( aus, "  Z0 = %5.3f \n", z0 );
  fprintf( aus, "  D0 = %5.3f \n", d0 );
  fprintf( aus, "  Ua = ?     \n" );
  fprintf( aus, "  Ra = ?     \n" );
  fprintf( aus, "  Kl = ?     \n" );
  fprintf( aus, "  Sg = ?     \n" );
  if (use_rain)
    fprintf( aus, "  Prec = ?     \n" );
//  fprintf( aus, "  Wind = ?   \n" );                              -2008-12-19
  fprintf( aus,
  "  WindLib = %s  \n", WindLib );
  if (strlen(Append) > 1) fprintf( aus, "  %s\n", Append);
  fprintf(aus, "----------------------------------------------\n" );
  fprintf(aus, "-\n" );
  fprintf(aus,
"!              T1              T2     Ua      Ra    Kl        Sg" );
  if (use_rain)
    fprintf(aus, "   Prec");
  fprintf(aus, "\n");
  fprintf( aus,
"- (dddd.hh:mm:ss) (dddd.hh:mm:ss)  (m/s)  (grad) (K-M)       (1)" );
  if (use_rain)
    fprintf(aus, " (mm/h)");
  fprintf(aus, "\n");
  t = 0;
  jkl = 0;
  jwg = 0;
  jwr = 0;
  NumTtl = 0;
  for (k=0; k<Nstat; k++) {
    nwg = Nwgs[Stats[k].format];
    weight = Stats[k].weight;
    pstat  = Stats[k].pstat;
    for (ikl=0; ikl<NKL; ikl++)
      for (iwg=0; iwg<nwg; iwg++)
        for (iwr=0; iwr<NWR; iwr++) {
          xt = Stat[ikl][iwg][iwr]*weight*NumSpD;
          dt = 0.5 + xt;
          if (dt < 1)
            continue;
          NumTtl += Nsub;
        }
  }
  if (NumTtl > MAXDAYS)                                       eX(2); //-2013-10-14
  //
  // generate AKSary
  //
  AryFree(&AKSary);
  AryCreate(&AKSary, sizeof(AKSREC), 1, 1, NumTtl);           eG(100);
  //
  n = 1;
  for (k=0; k<Nstat; k++) {
    frm = Stats[k].format;
    nwg = Nwgs[frm];
    weight = Stats[k].weight;
    pstat  = Stats[k].pstat;
    rain = Stats[k].rain;
    if (frm==NEU || frm==SPECIAL)
      for (iwg=0; iwg<nwg; iwg++)  wg[iwg] = WgTAL[iwg];
    else if (frm == VDI)
      for (iwg=0; iwg<nwg; iwg++)  wg[iwg] = WgVDI[iwg];
    else
      for (iwg=0; iwg<nwg; iwg++)  wg[iwg] = 0.5*iwg;
    for (ikl=0; ikl<NKL; ikl++)
      for (iwg=0; iwg<nwg; iwg++)
        for (iwr=0; iwr<NWR; iwr++) {
          xt = Stat[ikl][iwg][iwr]*weight*NumSpD;
          dt = 0.5 + xt;
          if (dt < 1)
            continue;
          if (WndR)  jwr = iwr+1;
          if (WndG)  jwg = iwg+1;
          if (WndK)  jkl = ikl+1;
          gewicht = NumTtl*xt/(Nsub*NumSpD);
          for (i=0; i<Nsub; i++) {
            TimeStr(t, t1s);
            TE(n) = t;
            t += NumSpD;
            TimeStr(t, t2s);
            ir = 10.*(iwr+0.5)+(10./Nsub)*(i+0.5);
            iu = 10*wg[iwg] + 0.5;
            fprintf(aus, "Z  %14s  %14s  %5.1f  %6d  %4.1f  %8.5f",
              t1s, t2s, wg[iwg], ir, Kl[ikl], gewicht);
            if (use_rain)
              fprintf(aus, " %6.1f", rain);
            fprintf(aus, "\n");
            UA(n) = iu;
            RA(n) = ir;
            KL(n) = ikl+1;
            SG(n) = gewicht;
            RI(n) = rain;
            n++;
          }
        }
  }
  fclose(aus);
  return NumTtl;
eX_1:
  eMSG(_cant_write_$_, fname);
eX_2:
  eMSG(_maxsituations_exceeded_$$_, MAXDAYS, NumTtl);  
eX_100:
  eMSG(_internal_error_);
  }

/*============================================================== mean_velocity
*/
static float mean_velocity( STAT *pstat, int jkl, int jwg, int jwr, float *wg, int nwg) {
  float dt, t=0, v=0;
  int ikl, ikl1, ikl2;
  int iwg, iwg1, iwg2;
  int iwr, iwr1, iwr2;
  if (jkl < 1) { ikl1 = 0;      ikl2 = NKL-1; }
  else         { ikl1 = jkl-1;  ikl2 = ikl1; }
  if (jwg < 1) { iwg1 = 0;      iwg2 = nwg-1; }
  else         { iwg1 = jwg-1;  iwg2 = iwg1; }
  if (jwr < 1) { iwr1 = 0;      iwr2 = NWR-1; }
  else         { iwr1 = jwr-1;  iwr2 = iwr1; }
  for (ikl=ikl1; ikl<=ikl2; ikl++)
    for (iwg=iwg1; iwg<=iwg2; iwg++)
      for (iwr=iwr1; iwr<=iwr2; iwr++) {
      dt = Stat[ikl][iwg][iwr];
      t += dt;
      v += dt*wg[iwg];
      }
  if (t > 0)  v /= t;
  return v;
}

//=============================================================== TasWriteWetlib
//
int TasWriteWetlib( void )
  {
  dP(TasWriteWetlib);
  int iwg, k, nwg, frm;
  int jkl, jwr, jwg, jkl1, jkl2, jwg1, jwg2, jwr1, jwr2;
  long t;
  float wg[NWGalt], vm;
  STAT *pstat;
  FILE *aus;
  char fname[256];                                                //-2008-12-19
  float blm_version = TipBlmVersion();                            //-2011-09-12
  //
  sprintf(fname, "%s/%s", StdPath, "metlib.def");
  aus = fopen(fname, "w");  if (!aus)                           eX(1);
  fprintf( aus, "============================================= metlib.def\n" );
  fprintf( aus, _comment2_$$$_, (CfgWet) ? "AUSTAL2000N" : "AUSTAL2000", TalAKSVersion, StatName );
  fprintf( aus, "%s", _comment3a_ );                              //-2008-12-29
  fprintf( aus, ". \n");                                          //-2008-12-29
  fprintf( aus, "  Version = %1.1f  \n", blm_version );
  fprintf( aus, "  Interval = 3600\n" );
  fprintf( aus, "  Xa=%1.1f   Ya=%1.1f   Ha=%1.1f  \n", Xa, Ya, Ha );
  if (TI.mh > 0) {
    fprintf(aus, "  HmMean = { 0  0  0  %1.0lf  %1.0lf  %1.0lf }\n", //-2013-04-08
            TI.mh+SttHmMean[3], TI.mh+SttHmMean[4], TI.mh+SttHmMean[5]);
  }
  fprintf( aus, "  Z0 = %5.3f     \n", Z0 );
  fprintf( aus, "  D0 = %5.3f     \n", D0 );
  fprintf( aus, "  Ua = ?         \n" );
  fprintf( aus, "  Ra = ?         \n" );
  fprintf( aus, "  Kl = ?         \n" );
  fprintf( aus, "  Wind = ?       \n" );
  fprintf( aus,
  "  WindLib = %s    \n", WindLib );
  // if (strlen(Append) > 1) fprintf( aus, "  %s\n", Append);
  fprintf( aus,
"-----------------------------------------------------------------------\n" );
  fprintf( aus, "-\n" );
  fprintf( aus, "!     T1     T2     Ua     Ra     Kl   Wind\n" );
  fprintf( aus, "-     (s)    (s)  (m/s) (grad)  (K-M)    (1)\n" );
  t = 0;
  for (k=0; k<Nstat; k++) {
    frm = Stats[k].format;
    nwg = Nwgs[frm];
    pstat  = Stats[k].pstat;
    if (frm == NEU || frm == SPECIAL)
      for (iwg=0; iwg<nwg; iwg++)  wg[iwg] = WgTAL[iwg];
    else if (frm == VDI)
      for (iwg=0; iwg<nwg; iwg++)  wg[iwg] = WgVDI[iwg];
    else
      for (iwg=0; iwg<nwg; iwg++)  wg[iwg] = 0.5*iwg;
    if (WndK) { jkl1 = 1;  jkl2 = NKL; }
    else      { jkl1 = 0;  jkl2 = 0; }
    if (WndG) { jwg1 = 1;  jwg2 = nwg; }
    else      { jwg1 = 0;  jwg2 = 0; }
    if (CreateLib == 36) { jwr1 = 1;  jwr2 = 36; }
    else                 { jwr1 = 0;  jwr2 = 0; }
    for (jkl=jkl1; jkl<=jkl2; jkl++)
      for (jwg=jwg1; jwg<=jwg2; jwg++)
        for (jwr=jwr1; jwr<=jwr2; jwr++) {
          vm = mean_velocity(pstat, jkl, jwg, 0, wg, nwg);  // always use jwr=0 !!!
          if (vm == 0)  vm = wg[0];                         //-2004-06-15
          if (jwr2 == 0)  jwr = 27;
          fprintf(aus,
              "Z    %3ld    %3ld %6.1f %6.0f %6.1f   %d%d%02d\n",
              t, t+1, vm, 10.*jwr, Kl[jkl-1], jkl, jwg, jwr);
          t += 1;
          if (jwr2 == 0) {
            jwr = 18;
            fprintf(aus,
              "Z    %3ld    %3ld %6.1f %6.0f %6.1f   %d%d%02d\n",
              t, t+1, vm, 10.*jwr, Kl[jkl-1], jkl, jwg, jwr);
            t += 1;
          }
    }
    break;
  }
  fclose(aus);
  return t;                                              //-2004-11-15
eX_1:
  eMSG(_cant_write_$_, fname);
}

//============================================================ TasWriteStaerke
//
int TasWriteStaerke( void )
  {
  dP(TasWriteStaerke);
  int ikl, iwg, iwr, i, k, n, nwg, nox, ivar, nvar;
  long t, dt, dtsub;
  float weight, f, xt, *pf;
  STAT *pstat;
  FILE *aus;
  TIPVAR *pv;
  char t1s[40], t2s[40], fname[120];
  //
  nox = StdStatus & TAL_NOX;
  nvar = (TalMode & TIP_VARIABLE) ? TipVar.bound[0].hgh+1 : 0;    //-2011-11-23
  sprintf(fname, "%s/%s", StdPath, "work/variable.def");
  aus = fopen(fname, "w");  if (!aus)                           eX(1);
  if (nox)  fprintf(aus, "------------------------");
  fprintf(aus, "----------------------------- variable.def\n" );
  fprintf(aus, ".\n" );
  if (nox) {
    fprintf(aus, "  gas.no2-gas.no = R2\n");
    fprintf(aus, "  gas.no-gas.no = R1\n- \n");
  }
  fprintf(aus, "!       T1              T2         EmisFac");
  if (nox)  fprintf(aus, "          R2          R1");
  for (ivar=0; ivar<nvar; ivar++) {                             //-2001-12-28
    pv = AryPtrX(&TipVar, ivar);
    fprintf(aus, " %11s", pv->lasn);
  }
  fprintf(aus, "\n");
  if (nox)  fprintf(aus, "------------------------");
  for (ivar=0; ivar<nvar; ivar++) {                             //-2001-12-28
    fprintf(aus, "------------");
  }
  fprintf(aus, "------------------------------------------\n");
  t = 0;
  n = 1;
  for (k=0; k<Nstat; k++) {
    nwg = Nwgs[Stats[k].format];
    weight = Stats[k].weight;
    pstat  = Stats[k].pstat;
    for (ikl=0; ikl<NKL; ikl++)
      for (iwg=0; iwg<nwg; iwg++)
        for (iwr=0; iwr<NWR; iwr++) {
          xt = Stat[ikl][iwg][iwr]*weight*NumSpD;
          dt = 0.5 + xt;
          if (dt < 1)
            continue;
          dtsub = (dt + Nsub - 1)/Nsub;
          if (dtsub < DtSubMin)  dtsub = DtSubMin;            //-2001-06-27
          f = NumTtl*xt/(Nsub*dtsub);
          for (i=0; i<Nsub; i++) {
            TimeStr(t, t1s);
            TimeStr(t+dtsub, t2s);
            fprintf(aus, "Z %14s  %14s %9.2f", t1s, t2s, f);
            if (nox) {
              float r1, r2;
              r1 = 1/(SttNoxTimes[ikl]*3600);
              r2 = r1*46.0/30.0;
              fprintf(aus, " %11.3e %11.3e", r2, -r1);
            }
            for (ivar=0; ivar<nvar; ivar++) {                 //-2001-12-28
              pv = AryPtrX(&TipVar, ivar);
              pf = AryPtrX(&(pv->dsc), ikl+1, iwg+1);
              fprintf(aus, " %11.3e", *pf);
            }
            fprintf(aus, "\n");
            TimeStr(t+dtsub, t1s);
            t += NumSpD;
            TimeStr(t, t2s);
            fprintf(aus, "Z %14s  %14s       0.0", t1s, t2s );
            if (nox) {
              float r1, r2;
              r1 = 1/(SttNoxTimes[ikl]*3600);
              r2 = r1*46.0/30.0;
              fprintf(aus, " %11.3e %11.3e", r2, -r1);
            }
            for (ivar=0; ivar<nvar; ivar++) {                 //-2001-12-28
              pv = AryPtrX(&TipVar, ivar);
              pf = AryPtrX(&(pv->dsc), ikl+1, iwg+1);
              fprintf(aus, " %11.3e", *pf);
            }
            fprintf(aus, "\n");
            NS(n) = dtsub;
            EF(n) = f;
            n++;
            }
          }
    }
  if (nox)  fprintf(aus, "------------------------");
  for (ivar=0; ivar<nvar; ivar++) {                             //-2001-12-28
    fprintf(aus, "------------");
  }
  fprintf(aus, "------------------------------------------\n");
  fclose(aus);
  return 0;
eX_1:
  eMSG(_cant_write_$_, fname);
}

/*=================================================================== ClearAll
*/
static long ClearAll( void )
  {
  dQ(ClearAll);
  int k;
  for (k=0; k<Nstat; k++) {
    FREE(Stats[k].pstat);
    Stats[k].pstat = NULL;
  }
  Nstat = 0;
  return 0;
}

/*==================================================================== TasHelp
*/
static void TasHelp( void )
  {
  char c = '-';
  printf("usage : TalAKS <path> [options ...] \n");
  printf("option: %ca<xa%%f>,<ya%%f>,<ha%%f> \n", c);
  printf("        %cb[v|w]<class%%d>=<value%%f> ...\n", c);
  printf("        %cB<WindLib%%s> \n", c);
  printf("        %cc  (convert old to new format)\n", c);
  printf("        %cf<stat%%s>,<weight%%f:1>,<rain%%f:0 mm/h> ...\n", c);
  printf("        %cg<g[0]>,<g[1]>,...,<g[8]> \n", c);
  printf("        %cl<2|36> \n", c);
  printf("        %cp[s|h|p] \n", c);
  printf("        %cs<nsub%%d:5> \n", c);
  printf("        %cw[k][g][r] \n", c);
  printf("        %cz<z0%%f> \n", c);
  }

/*===================================================================== TalAKS
*/
long TalAKS(
char *ss )
  {
  dP(TalAKS);
  int n, k;
  float val, sumrain, g[9];
  static enum STATFORMS newform = NEU;
  char c, *pc, *fm = "spc neu alt vdi ";
  //
  if (*ss) {
    if (*ss != '-') {
      strcpy(StdPath, ss);
      for (pc=StdPath; (*pc); pc++)  if (*pc == '\\')  *pc = '/';
      if (pc[-1] == '/')  pc[-1] = 0;
      return 0;
    }
    switch (ss[1]) {
      case 'a': sscanf(ss+2, "%f,%f,%f", &Xa, &Ya, &Ha);
                break;
      case 'A': sprintf(Append, "%s", ss+2);
                break;
      case 'b': if (3 != sscanf(ss+2, "%c%d=%f", &c, &k, &val))         eX(2);
                if (k<1 || k>6)                                         eX(3);
                if (c == 'v')  Fv[k-1] = val;
                if (c == 'w')  Fw[k-1] = val;
                break;
      case 'B': sprintf(WindLib, "%s", ss+2);
                break;
      case 'c': ConvertStatistics = 1;
                break;
      case 'd': strcpy(DefName, ss+2);
                break;
      case 'f': if (Nstat >= MAXSTAT)                                   eX(1);
                strcpy(StatName, ss+2);
                Nstat = 1;
                break;
      case 'g': Nwg = sscanf(ss+2, "%f,%f,%f,%f,%f,%f,%f,%f,%f",
                  g+0, g+1, g+2, g+3, g+4, g+5, g+6, g+7, g+8);
                if (Nwg < 1)                                            eX(17);
                vMsg(_$_non_standard_, Nwg);
                for (n=0; n<Nwg; n++)  WgTAL[n] = g[n];
                Nwgs[0] = Nwg;
                break;
      case 'h': TasHelp();
                return 0;
      case 'l': CreateLib = 36;
                sscanf(ss+2, "%d", &CreateLib);
                break;
      case 'm': DtSubMin = 20;
                sscanf(ss+2, "%d", &DtSubMin);
                break;
      case 'n': StdStatus |= TAL_NOX;
                break;
      case 'p': StdStatus |= TAL_PRINT;
                PrintMode = ss[2];
                break;
      case 's': sscanf(ss+2, "%d", &Nsub);
                if (Nsub < 1)  Nsub = 1;
                break;
      case 't': pc = strstr(fm, ss+2);
                if (!pc)                                                eX(4);
                newform = (pc - fm)/4;
                break;
      case 'w': if (strchr(ss+2,'r'))  WndR = 1;
                if (strchr(ss+2,'g'))  WndG = 1;
                if (strchr(ss+2,'k'))  WndK = 1;
                break;
      case 'z': Z0 = 1.5;
                D0 = 0.0;
                sscanf(ss+2, "%f,%f", &Z0, &D0);
                break;
      default:  ;
      }
    return 0;
    }
  if ((!Nstat) || (!*StatName)) { 
    TasHelp();  
    return 0; 
  }
  vMsg(_title_$_, TalAKSVersion);
  vMsg("FILE %s", StatName);
  TasReadStat(StatName);                                                eG(10);
  for (n=0; n<Nstat; n++) {
    sumrain = Stats[n].weight*Stats[n].rain*NumHpA;
    vMsg("%d.  %12s  %5.1f %%  %3.0f mm  %4.1f mm/h ",
      n+1, Stats[n].name, 100*Stats[n].weight, sumrain, Stats[n].rain);
    }
  for (n=0; n<Nstat; n++) {
    if (StdStatus & TAL_PRINT)  PrnStat(Stats+n);
  }
  TasWriteWetter();                                                     eG(11);
  if (CreateLib==2 || CreateLib==36)  TasWriteWetlib();                 eG(11);
  TasWriteStaerke();                                                    eG(12);
  ClearAll();
  vMsg(_$_cases_, NumTtl );
  return NumTtl;
eX_1:
  nMSG(_$_files_exceeded_, MAXSTAT);
  goto done;
eX_2:
  nMSG(_invalid_option_$_, ss);
  goto done;
eX_3:
  nMSG(_invalid_class_$_, k);
  goto done;
eX_4:
  nMSG(_unknown_format_$_, ss+2);
  goto done;
eX_10:
  nMSG(_error_reading_$_, Stats[n].name);
  goto done;
eX_11:
  nMSG(_cant_write_$_, "meteo.def");
  goto done;
eX_12:
  nMSG(_cant_write_$_, "variable.def");
  goto done;
eX_17:
  nMSG(_invalid_count_velocities_);
  goto done;
done:
  ClearAll();
  return -1;
}

#ifdef MAIN   /*#########################################################*/

int main( int argc, char *argv[] )
  {
  dP(TasMain);
  int i;
  if (argc < 2) { TasHelp();  return 0; }
  for (i=1; i<argc; i++)  TalAKS(argv[i]);
  TalAKS("");                                           eG(1);
  return 0;
eX_1:
  eMSG(_error_creating_input_);
}
#endif    /*#############################################################*/
/*=========================================================================
 *
 * history:
 *
 * 2001-05-04 lj 1.1.0  adapted from LstTal
 * 2001-05-24 lj 1.2.0  Tvf and Twf dropped if version=BlmTAL
 * 2001-05-31 lj 1.2.1  NoxTimes from TalInp, reading and writing separated
 * 2001-06-05 lj 1.2.2  Messages into log-file only
 * 2001-06-27 lj 1.2.3  DtSubMin introduced (=20)
 * 2001-08-16 lj 1.2.4  TA-LUFT-format only supported
 * 2001-12-28 lj 0.10.0 Variable parameters
 * 2002-09-24 lj 1.0.0  final release candidate
 * 2004-06-15 lj 1.1.19 mean wind for situations not represented in AKS
 * 2004-11-15 uj 2.1.1  TasWriteWetlib returns number of situations
 * 2005-03-17 uj 2.2.0  version number upgrade
 * 2006-02-13 uj 2.2.8  nostandard option SPREAD
 * 2006-20-26 lj 2.3.0  exteral strings
 * 2008-12-19 lj 2.4.6  file name length = 256, i18n for metlib.def
 * 2008-12-29 lj 2.4.7  writing of metlib.def modified
 * 2011-07-07 uj 2.5.0  version number upgrade
 * 2011-09-12 uj 2.5.1  blm version from TalInp
 * 2011-11-23 lj 2.6.0  settings, precipitation like ARTM
 * 2011-12-20 lj        notification about AKS with rain
 * 2013-04-08 uj 2.6.6  definition of HmMean without ","
 * 2013-10-14 uj 2.6.7  check maximum number of situations
 * 2014-01-21 uj 2.6.9  CfgWet
 * 2014-01-29 uj        prec info
 *
 *=======================================================================*/
