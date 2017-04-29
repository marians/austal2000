//=================================================================== TalPrm.c
//
// Definition of parameters for AUSTAL2000
// =======================================
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

#define  STDMYMAIN  PrmMain
static char *eMODn = "TalPrm";

#include <ctype.h>
#include <math.h>
#include "IBJmsg.h"
#include "IBJary.h"
#include "IBJtxt.h"
#include "IBJstd.h"

//==================================================================

STDPGM(tstprm, PrmServer, 2, 6, 11);

//==================================================================

#include "genio.h"
#include "genutl.h"

#include "TalTmn.h"
#include "TalNms.h"
#include "TalPrm.h"
#include "TalPrm.nls"
#include "TalIo1.h"
#include "TalGrd.h"
#include "TalRnd.h"

#define DEFDATE   "2000-01-01"
#define DEFTIME   "00:00:00"
#define DEFSTART  0
#define DEFSTEP   60
#define DEFCYCLE  3600    // AUSTAL2000
#define DEFGROUPS 9
#define DEFRATE   1.0
#define DEFGAM0   2.00
#define DEFGAM1   0.50
#define DEFGAM2   0.50
#define DEFGAM3   1.00
#define DEFGAM4   1.00
#define DEFKHMIN  1.0
#define DEFKVMIN  1.0
#define DEFGMIN   0.001
#define DEFWAIT   2

#define  GET(a,b,c,d)  GetData((a),(b),(c),(d)); strcpy(nn,(a)); eG(100)
#define  DEF(a,b,c,d,e,f)  {DefParm((a),(b),(c),(d),(e),(f)); \
                           strcpy(nn,(a)); eG(200);}

extern char DataPath[];    /* in GENIO  */
extern char InputPath[];   /* in GENIO  */
extern FILE *InputFile;    /* in GENIO  */

PRMINF *PMI;
ARYDSC *PrmPasl, *PrmPcmp, *PrmPsrc, *PrmPems, *PrmPchm;
ARYDSC *PrmPsrg, *PrmPemg;
VARTAB *PrmPvpp;

static char DefName[120];

static int NumSrcDat = 31;
static float StdSrcDat[] = {
   0.0,  0.0, 0.0, 100.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 1.0,-999,-999,
   -1.e33, 0.0, 0.0, -999, -1.e33,-1.e33,-1.e33,-1.e33,-1.e33,-1.e33,
  -999,  0.0,  0.0,  0.0, 0.0, 0.0, 0.0, 0.0 };
static char *SrcNames[]  = {
  "XQ", "YQ", "RQ", "HQ","TQ","LQ","BQ","WQ","QQ","VQ","DQ","OQ","PQ",
  "AQ", "CQ", "FQ", "GQ", "X1", "Y1", "H1", "X2", "Y2", "H2",
  "TS", "SH", "SV", "SL", "FR", "RH", "LW", "TT", NULL };

static int NumAslDat = 8;
static float StdAslDat[] = {  0.0,   0.0,   0.0,   1.0,   0.0,   1.0,   1.0,
  -1.0 };
static char *AslNames[]  = { "VDEP","FWSH","RFAK","REXP","MPTL","REFC","REFD",
  "WDEP", NULL };

char InputName[80] = "PARAM\b\b\b\b\b?????";
static char **CmpNames;

  /*============================ prototypes ================================*/

/* File PARAM.DEF einlesen und Werte setzen.  */
static long PrmReadParam( void );
  
/* read source definitions      */
static long PrmReadSrc( void );

/* File mit Definition der Stoffart einlesen.      */
static long PrmReadAsl( void );


static long PrmCmpUpdate( void );

/* Quellstaerken STAERKE.DEF einlesen.   */
static long PrmReadEms( void );

/* Chemische Reaktionen CHEMIE.DEF einlesen.  */
static long PrmReadChm( void );

/* Variable Parameter aus Zeitreihe lesen.  */
static long PrmReadVar( void );

/*================================================================== PrmHeader
*/
#define CAT(a, b)  TxtPrintf(&txt, a, b), TxtCat(&hdr, txt.s);
char *PrmHeader(                /* the header (global storage)  */
  long id,                      /* identification               */
  long *pt1,                    /* start of the validity time   */
  long *pt2 )                   /* end of validity time         */
  {
  char t1s[32], t2s[32], name[256], s[256];
  static TXTSTR hdr = { NULL, 0 };
  TXTSTR txt = { NULL, 0 };
  strcpy(name, NmsName(id));
  strcpy(t1s, TmString(pt1));
  strcpy(t2s, TmString(pt2));
  TxtCpy(&hdr, "\n");
  sprintf(s, "PRM_%d.%d.%s", StdVersion, StdRelease, StdPatch);
  CAT("prgm  \"%s\"\n", s);
  CAT("idnt  \"%s\"\n", MI.label);
  CAT("file  \"%s\"\n", name);
  CAT("t1    \"%s\"\n", t1s);
  CAT("t2    \"%s\"\n", t2s);
  TxtClr(&txt);
  return hdr.s;
}
#undef  CAT

/*================================================================== CmpHeader
*/
#define CAT(a, b)  TxtPrintf(&txt, a, b), TxtCat(&hdr, txt.s);
char *CmpHeader(                /* the header (global storage)  */
  long id,                      /* identification               */
  long *pt1,                    /* start of the validity time   */
  long *pt2 )                   /* end of validity time         */
  {
  char s[512], t1s[32], t2s[32], name[256];
  static TXTSTR hdr = { NULL, 0 };   
  TXTSTR txt = { NULL, 0 };
  strcpy(name, NmsName(id));
  strcpy(t1s, TmString(&MI.dost1));
  strcpy(t2s, TmString(&MI.dost1));
  TxtCpy(&hdr, "\n");
  sprintf(s, "PRM_%d.%d.%s", StdVersion, StdRelease, StdPatch);
  CAT("prgm  \"%s\"\n", s);
  CAT("idnt  \"%s\"\n", MI.label);
  CAT("file  \"%s\"\n", name);
  CAT("t1    \"%s\"\n", t1s);
  CAT("t2    \"%s\"\n", t2s);
  TxtClr(&txt);
  TxtCat(&hdr, "form  \"");
  sprintf(s, "%s%s",
    "name%40cunit%20csi%4dsx%4dgmin%5.3fvsed%5.3fvdep%5.3ffwsh%12.4erfak%12.4e",
    "rexp%5.3fmptl%12.4erefc%12.4erefd%12.4ewdep%12.4evred%12.4ehred%12.4easlcmp%4d");
  TxtCat(&hdr, s);
  TxtCat(&hdr, "\"\n");
  return hdr.s;
}
#undef  CAT

  /*============================================================= PrmReadParam
  */
static long PrmReadParam(  /* File PARAM.DEF einlesen und Werte setzen.  */
  void )
  {
  dP(PrmReadParam);
  char flags[256], timestr[80];
  long rc;
  char nn[20], fname[256];
  strcpy(fname, "param.def");
  vLOG(3)(_reading_$_, fname );
  if (0 > OpenInput(fname, MI.defname))                         eX(1);
  strcpy( MI.input, fname);
  rc = GetLine('.', Buf, BUFLEN);  if (rc <= 0)                 eX(2);
  flags[0] = 0;
  MI.titel[0] = 0;
  strcpy(MI.label, "TEST");
  GET("IDENT|KENNUNG", Buf, "%255s", MI.label);                   //-2011-11-23
  GET("TITLE|TITEL",   Buf, "%255s", MI.titel);                   //-2011-11-23
  GET("FLAGS",   Buf, "%255s", flags);
  if (flags[0] != '\0') {
    SetFlag( flags, "ECHO",   &MI.flags, FLG_ECHO );
    SetFlag( flags, "CHECK",  &MI.flags, FLG_CHECK );
    SetFlag( flags, "VERB",   &MI.flags, FLG_VERB );
    SetFlag( flags, "DISP",   &MI.flags, FLG_DISP );
    SetFlag( flags, "RESULT", &MI.flags, FLG_RESULT );
    SetFlag( flags, "WRTMOD", &MI.flags, FLG_WRTMOD );
    SetFlag( flags, "SCAT",   &MI.flags, FLG_SCAT );
    SetFlag( flags, "CHEM",   &MI.flags, FLG_CHEM );
    SetFlag( flags, "MNT",    &MI.flags, FLG_MNT );
    SetFlag( flags, "MAXIMA", &MI.flags, FLG_MAXIMA );
    SetFlag( flags, "RNDSET", &MI.flags, FLG_RNDSET );
    SetFlag( flags, "QUIET",  &MI.flags, FLG_QUIET );
    SetFlag( flags, "ONLYADVEC", &MI.flags, FLG_ONLYADVEC );
    SetFlag( flags, "NOPREDICT", &MI.flags, FLG_NOPREDICT );
    SetFlag( flags, "EXACTTAU",  &MI.flags, FLG_EXACTTAU );
    SetFlag( flags, "TRACING",   &MI.flags, FLG_TRACING );
    SetFlag( flags, "ODOR",      &MI.flags, FLG_ODOR );
    SetFlag( flags, "ODMOD",     &MI.flags, FLG_ODMOD );
    SetFlag( flags, "NODILUTE",  &MI.flags, FLG_NODILUTE );       //-2011-11-21
    SetFlag( flags, "WRITESFC", &MI.flags, FLG_WRITESFC );      //-2012-10-30
    SetFlag( flags, "READSFC",  &MI.flags, FLG_READSFC );       //-2012-10-30
  }
  MI.disproc[0] = 0;
  MI.disparm[0] = 0;
  strcpy(MI.refdate, DEFDATE);
  GET("REFDATE|REFDATUM", Buf, "%39s", MI.refdate);             //-2011-11-23
  strcpy(MI.reftime, DEFTIME);
  GET("REFTIME|REFZEIT", Buf, "%39s", MI.reftime);              //-2011-11-23
  MI.cycle = DEFCYCLE;
  rc = GET("INTERVAL|INTERVALL", Buf, "%78s", timestr);         //-2011-11-23
  if (rc > 0)  MI.cycle = TmValue(timestr);
  MI.gamma[0] = DEFGAM0;
  MI.gamma[1] = DEFGAM1;
  MI.gamma[2] = DEFGAM2;
  MI.gamma[3] = DEFGAM3;
  MI.gamma[4] = DEFGAM4;
  MI.khmin = DEFKHMIN;
  MI.kvmin = DEFKVMIN;
  MI.average = 0;
  GET("AVERAGE", Buf, "%ld", &MI.average);
  MI.step = DEFSTEP;
  rc = GET("TAU", Buf, "%s", timestr);
  if (rc > 0) {
    if (strchr(timestr, ':'))  MI.step = TmValue(timestr);
    else  sscanf(timestr, "%f", &(MI.step));          // 98-08-16
  }
  MI.start = DEFSTART;
  rc = GET("START", Buf, "%s", timestr);
  if (rc > 0)  MI.start = TmValue(timestr);
  MI.end = MI.start + MI.cycle;
  rc = GET("END|ENDE", Buf, "%s", timestr);                     //-2011-11-23
  if (rc > 0)  MI.end = TmValue(timestr);
  MI.cycind = 1;
  strcpy(MI.asllist, "");                                       /*-08jul97-*/
  MI.numasl = 0;
  *MI.srglist = 0;
  MI.numgrp = DEFGROUPS;
  GET("GROUPS|GRUPPEN", Buf, "%ld", &MI.numgrp);                //-2011-11-23
  MI.prcp = -999;
  MI.hmax = 0;
  MI.timing = 0.0;
  MI.lastrfac = 1.0;
  MI.usedrfac = 1.0;
  MI.numshuf = 0;
  MI.seed = 11111;
  GET("SEED",    Buf, "%ld", &MI.seed);
  MI.count = 0;
  MI.special = 0;
  GET("ODORTHR", Buf, "%f", &MI.threshold);                     //-2005-03-11
  vDSP(1)("");
  CloseInput();                                                 eG(3);
  *MI.input = 0;
  vLOG(3)(_$_closed_, fname);
  if ((PrmPvpp) && (PrmPvpp->num > 0))  MI.status |= PRM_VARPRM;
  return 0;
eX_1: eX_2: eX_3: eX_100:
  eMSG(_internal_error_);
  }

  /*=============================================================== PrmReadAsl
  */
static long PrmReadAsl(  /* File mit Definition der Stoffart einlesen.      */
  void )
  {
  dP(PrmReadAsl);
  char nn[40], fname[256], cname[40], hdr[102], *pc, *pb;
  char datname[40], aslname[40];
  int i, ii, j, k, ndat, ncmp, nspc, nasl, varasl, nl, iasl, icmp, mode;
  long rc, idasl, idcmp, t1, t2;
  CMPREC *pcmp, *pm;
  ASLREC *pasl, *ps;
  float *pdat, minmptl, maxmptl, sum, total, mptl;
  strcpy(fname, "substances.def");                                //-2011-11-23
  vLOG(3)(_reading_$_, fname);
  varasl = 0;
  if (0 > OpenInput(fname, MI.defname))                         eX(1);
  strcpy(MI.input, fname);

  pb = NULL;
  ncmp = GenCntLines('K', &pb);  if (ncmp < 1)                  eX(7);
  nasl = 0;
  for (pc=pb; (*pc); pc++)
    if (*pc == '.')  nasl++;
  if (nasl < 1)                                                 eX(42);
  MI.numasl = nasl;
  MI.sumcmp = ncmp;
  idasl = IDENT(ASLarr, 0, 0, 0);
  PrmPasl = TmnCreate(idasl, sizeof(ASLREC), 1, 0, nasl-1);     eG(101);
  idcmp = IDENT(CMPtab, 0, 0, 0);
  mode = TMN_UNIQUE | TMN_MODIFY | TMN_FIXED;
  PrmPcmp = TmnAttach(idcmp, NULL, NULL, mode, NULL);           eG(120);
  if (PrmPcmp) {
    if (PrmPcmp->bound[0].hgh-PrmPcmp->bound[0].low+1 != ncmp)  eX(121);
  }
  else {
    TmnCreator(idcmp, -1, 0, "", NULL, CmpHeader);              eG(102);
    PrmPcmp = TmnCreate(idcmp, sizeof(CMPREC), 1, 0, ncmp-1);   eG(102);
  }
  iasl = -1;
  icmp = -1;
  nspc = 0;
  ndat = -1;
  pasl = NULL;
  ii = 0;

  for (pc=pb; (*pc); pc++) {
    if (pc[0] == '.') {
      pasl = AryPtr(PrmPasl, ++iasl);  if (!pasl)               eX(103);
      rc = GetLine('.', Buf, BUFLEN);  if (rc < 0)              eX(2);
      ii++;
      *aslname = 0;
      GET("NAME", Buf, "%18s", aslname);
      if (CheckName(aslname))                                   eX(92); //-2001-11-08
      for (i=0; i<iasl; i++) {
        ps = AryPtr(PrmPasl, i);  if (!ps)                      eX(104);
        if (!CisCmp(ps->name, aslname))                         eX(31);
        }
      strcpy(pasl->name, aslname);
      if (*aslname) {
        strcpy(aslname, ".");
        strcat(aslname, pasl->name);
        }
      strcpy(pasl->unit, "g");
      GET("UNIT|EINHEIT", Buf, "%18s", pasl->unit);
      pasl->vsed = 0.0;
      GET("VSED", Buf, "%f", &pasl->vsed);
      if (IsUndef(&pasl->vsed)) {
        varasl = 1;
        strcpy(nn, "VSED");
        strcat(nn, aslname);
        DefVar(nn, "%f", &pasl->vsed, 1.0, &PrmPvpp);            eG(100);
        }
      pasl->gmin = DEFGMIN;
      GET("THRESHOLD|SCHWELLE", Buf, "%f", &pasl->gmin);          //-2011-11-23
      pasl->rate = DEFRATE;
      GET("RATE", Buf, "%f", &pasl->rate);
      if (IsUndef(&pasl->rate)) {
        varasl = 1;
        strcpy(nn, "RATE");
        strcat(nn, aslname);
        DefVar(nn, "%f", &pasl->rate, 1.0, &PrmPvpp);            eG(100);
        }
      pasl->vred = 1.0;
      GET("VRED", Buf, "%f", &pasl->vred);
      pasl->hred = 1.0;
      GET("HRED", Buf, "%f", &pasl->hred);
      nl = GET("DIAMETER", Buf, "%[20]f", pasl->adlimits);
      if (nl > 0) {
        pasl->nadclass = nl;
        for (i=1; i<nl; i++)
          if (pasl->adlimits[i] <= pasl->adlimits[i-1])           eX(20);
        nl = GET("PROBABILITY", Buf, "%[20]f", pasl->addistri);
        if (nl != pasl->nadclass)                                 eX(21);
        total = 0;
        for (i=0; i<nl; i++)  total += pasl->addistri[i];
        sum = 0;
        for (i=0; i<nl; i++) {
          sum += pasl->addistri[i];
          pasl->addistri[i] = sum/total;
          }
        pasl->addistri[nl-1] = 1;
        pasl->admean = 0;
        pasl->slogad = 0;
        }
      else {
        pasl->nadclass = 0;
        nl = GET("MEANDIAM", Buf, "%f", &pasl->admean);
        if (nl > 0) {
          if (pasl->admean < 0)    pasl->admean = 0;
          if (pasl->admean > 200)  pasl->admean = 200;
          nl = GET("SIGLOGD", Buf, "%f", &pasl->slogad);
          if (nl < 1)  pasl->slogad = 1.5;
          else if (pasl->slogad < 0)  pasl->slogad = 0;
          else if (pasl->slogad > 4)  pasl->slogad = 4;
          }
        else {
          pasl->admean = 0;
          pasl->slogad = 0;
          }
        }
      nl = GET("DENSITY", Buf, "%f", &pasl->density);
      if (nl < 1)  pasl->density = 1;
      else if (pasl->density < 0.1)  pasl->density = 0.1;
      else if (pasl->density > 20 )  pasl->density = 20;
      pasl->offcmp = icmp + 1;
      pasl->specident = 'a' + iasl;
      pasl->specindex = iasl;
      pasl->id = 0;
      pasl->pa = NULL;
      minmptl = 1;
      maxmptl = 0;
      nspc = 0;
      ndat = 0;
      continue;
      }
    if (pc[0] == '!') {
      if (!pasl)                                                eX(3);
      rc = GetLine('!', hdr, 100);  if (rc <= 0)                eX(4);
      ii++;
      ndat = EvalHeader(AslNames, hdr, DatPos, DatScale);       eG(5);
      nspc = 0;
      continue;
      }
    if (!ndat || !pasl)                                         eX(6);
    rc = GetLine('K', Buf, BUFLEN);  if (rc <= 0)               eX(9);
    ii++;
    rc = GetList(Buf, cname, 18, DatVec, MAXVAL);  if (rc < 0)  eX(91);
    if (rc != ndat)                                             eX(10);
    pcmp = AryPtrX(PrmPcmp, ++icmp);
    nspc++;
    pasl->numcmp = nspc;
    strcpy(pcmp->name, pasl->name);
    if (*(pasl->name))  strcat(pcmp->name, ".");
    strcat(pcmp->name, cname);
    for (i=0; i<icmp; i++) {
      pm = AryPtr(PrmPcmp, i);  if (!pm)                        eX(105);
      if (!CisCmp(pm->name, pcmp->name))                        eX(30);
      }
    strcpy(pcmp->unit, pasl->unit);
    pcmp->specident = pasl->specident;
    pcmp->specindex = pasl->specindex;
    pcmp->gmin = pasl->gmin;
    pcmp->vsed = pasl->vsed;
    pcmp->vred = pasl->vred;
    pcmp->hred = pasl->hred;
    pcmp->aslcmp = iasl;
    pdat = &(pcmp->vdep);
    mptl = pcmp->mptl;                     /* 99-08-06 UJ */
    for (j=0; j<NumAslDat; j++)
      pdat[j] = StdAslDat[j];
    for (j=0; j<ndat; j++) {
      k = DatPos[j];
      if (k < 0)  continue;
      if (IsUndef(&DatVec[j])) {
        varasl = 1;
        strcpy(datname, AslNames[k]);
        strcat(datname, ".");
        strcat(datname, pcmp->name);
        DefVar(datname, "%f", &pdat[k], DatScale[j], &PrmPvpp);    eG(13);
      }
      else
        pdat[k] = DatVec[j]*DatScale[j];
    }
    if (pcmp->mptl > maxmptl)  maxmptl = pcmp->mptl;
    if (pcmp->mptl < minmptl)  minmptl = pcmp->mptl;
    if (mptl > 0)  pcmp->mptl = mptl;                       //-00-02-21 lj
    if (pc[1] != 'K') {
      if (minmptl > 0)  pasl->rate = 0;
      else  if (maxmptl > 0) {
        vLOG(3)("value of MPTL in file %s ignored!", fname);
      }
    }
  }
  MI.maxcmp = 0;
  for (iasl=0; iasl<nasl; iasl++) {
    pasl = AryPtr(PrmPasl, iasl);  if (!pasl)                      eX(108);
    if (pasl->numcmp > MI.maxcmp)  MI.maxcmp = pasl->numcmp;
  }
  if (varasl) {
    MI.status |= PRM_VARPRM|PRM_VARASL;
    t1 = TmMin();
    t2 = t1;
  }
  else {
    t1 = MI.start;
    t2 = TmMax();
  }
  TmnDetach(idasl, &t1, &t2, TMN_MODIFY|TMN_FIXED, NULL);       eG(106);
  TmnDetach(idcmp, &t1, &t2, TMN_MODIFY|TMN_FIXED, NULL);       eG(107);
  vDSP(1)("");
  CloseInput();                                                 eG(12);
  *MI.input = 0;
  FREE(pb);
  vLOG(3)(_$_species_$_, ncmp, nasl);
  return 0;
eX_1: eX_2: eX_3: eX_42: eX_4: eX_5: eX_6: eX_7: eX_9: eX_10:
eX_12: eX_13: eX_20: eX_21: eX_30: eX_31:
eX_91: eX_92:
eX_100: eX_101: eX_102: eX_103: eX_104: eX_105: eX_106: eX_107: eX_108:
eX_120:eX_121:
  eMSG(_internal_error_);
  }

  /*============================================================= PrmCmpUpdate
  */
static long PrmCmpUpdate(
  void )
  {
  dP(PrmCmpUpdate);
  static long t1, t2, idasl, idcmp;
  ASLREC *pasl;
  CMPREC *pcmp;
  char t1s[40], t2s[40], nmasl[256], nmcmp[256];		//-2004-11-26
  int iasl, icmp;
  idasl = IDENT(ASLarr,0,0,0);
  strcpy(nmasl, NmsName(idasl));
  if (!TmnInfo(idasl, &t1, &t2, NULL, NULL, NULL))      eX(1);
  strcpy(t1s, TmString(&t1));
  strcpy(t2s, TmString(&t2));
  idcmp = IDENT(CMPtab,0,0,0);
  strcpy(nmcmp, NmsName(idcmp));
  for (icmp=0; icmp<MI.sumcmp; icmp++) {
    pcmp = AryPtr(PrmPcmp, icmp);  if (!pcmp)              eX(2);
    iasl = pcmp->specindex;
    pasl = AryPtr(PrmPasl, iasl);  if (!pasl)              eX(3);
    pcmp->vsed = pasl->vsed;
    }
  return 0;
eX_1: eX_2:  eX_3:
  eMSG(_internal_error_);
  }

  /*=============================================================== PrmReadSrc
  */
static long PrmReadSrc( /* read source definitions      */
  void )
  {
  dP(PrmReadSrc);
  SRCREC *psrc, *ps;
  SRGREC *pg, *ph;
  char name[40], fname[256], hdr[402], datname[80], srgname[40], srcname[40];
  char *pc, *pb;
  int i, ii, j, k, ndat, numgrdsrc, varsrc;
  long rc, t1, t2, idgrd, idsrg, idsrc;
  float *pdat, doarc, dparc;
  int nsrg, nsrc, isrg, isrc;

  strcpy(fname, "sources.def");                                 //-2011-11-23
  vLOG(3)(_reading_$_, fname);
  if (0 > OpenInput(fname, MI.defname))                         eX(1);
  strcpy(MI.input, fname);
  nsrc = GenCntLines('Q', &pb);  if (nsrc < 1)                  eX(4);
  nsrg = 0;
  for (pc=pb; (*pc); pc++)
    if (*pc == '.')  nsrg++;
  if (nsrg < 1)                                                 eX(3);
  FREE(pb);
  MI.numsrg = nsrg;
  MI.numsrc = nsrc;
  idsrg = IDENT(SRCarr, 0, 0, 0);
  PrmPsrg = TmnCreate(idsrg, sizeof(SRGREC), 1, 0, nsrg-1);     eG(5);
  idsrc = IDENT(SRCtab, 0, 0, 0);
  PrmPsrc = TmnCreate(idsrc, sizeof(SRCREC), 1, 0, nsrc-1);     eG(6);
  varsrc = 0;
  numgrdsrc = 0;
  isrg = -1;
  isrc = -1;
  ii = 0;

new_group:
  rc = GetLine('.', Buf, BUFLEN);  if (rc < 0)                  eX(2);
  ii++;
  *name = 0;
  rc = GetData("NAME", Buf, "%39s", name);                      eG(21);
  if (CheckName(name))                                          eX(92); //-2001-11-08
  pg = AryPtr(PrmPsrg, ++isrg);  if (!pg)                       eX(31);
  strcpy(pg->name, name);
  pg->fq = 1.0;
  for (i=0; i<isrg; i++) {
    ph = AryPtr(PrmPsrg, i);  if (!ph)                          eX(36); //-2014-06-26
    if (!CisCmp(ph->name, name))                                eX(22); //-2014-06-26
    }
  strcpy(srgname, name);
  if (*srgname)  strcat(srgname, ".");

new_tab: /*--- Einlesen einer Tabelle mit Header ---*/
  rc = GetLine('!', hdr, 400);  if (rc <= 0)            eX(7);
  ii++;
  ndat = EvalHeader(SrcNames, hdr, DatPos, DatScale);   eG(8);
  for ( ; ; ) {
    rc = GetLine('Q', Buf, BUFLEN);                     eG(9);
    if (rc <= 0)  break;
    ii++;
    psrc = AryPtr(PrmPsrc, ++isrc);  if (!psrc)               eX(34);
    rc = GetList(Buf, name, 39, DatVec, MAXVAL);  if (rc < 0) eX(91);
    if (ndat != rc)                                           eX(10);
    strcpy(psrc->name, srgname);
    psrc->idgrp = isrg;
    if (name[0] == '@') {
      strcpy(srcname, name+1);
      numgrdsrc++;
      idgrd = TmnIdent();
      psrc->idgrd = idgrd;
      TmnCreator(idgrd, -1, 0, srcname, NULL, NULL);    eG(15);
      }
    else {
      strcpy(srcname, name);
      }
    if (strlen(psrc->name)+strlen(srcname) > 39)        eX(20);
    strcat(psrc->name, srcname);
    for (j=0; j<isrc; j++) {
      ps = AryPtr(PrmPsrc, j);  if (!ps)                eX(37);
      if (!CisCmp(ps->name, psrc->name))                eX(17);
      }

    pdat = &(psrc->x);
    for (j=0; j<NumSrcDat; j++)
      pdat[j] = StdSrcDat[j];
    for (j=0; j<ndat; j++) {
      k = DatPos[j];
      if (k < 0) continue;
      if (IsUndef(&DatVec[j])) {
        varsrc = 1;
        strcpy(datname, SrcNames[k]);
        strcat(datname, ".");
        strcat(datname, psrc->name);
        DefVar(datname, "%f", &pdat[k], DatScale[j], &PrmPvpp);            eG(11);
        }
      else
        pdat[k] = DatVec[j]*DatScale[j];
      }
    if (MI.flags & FLG_GEOGK)
    if (psrc->oq != -999) {
        if (psrc->pq != -999) {
          doarc = GrdPprm->radfak*(psrc->oq - GrdPprm->oref);
          dparc = GrdPprm->radfak*(psrc->pq - GrdPprm->pref);
          psrc->x = 1000*GrdPprm->radius*doarc*(GrdPprm->cosref
                    - GrdPprm->sinref*dparc);
          psrc->y = 1000*GrdPprm->radius*(dparc
                    + 0.5*doarc*doarc*GrdPprm->cosref*GrdPprm->sinref);
        }
        else                                                            eX(12);
    }
  }
  if (isrc < nsrc-1) {
    if (rc == -1) {
      if (Buf[0] == '!')  goto new_tab;
      if (Buf[0] == '.')  goto new_group;
      }
    eX(35);
    }

  if (varsrc) {
    MI.status |= PRM_VARPRM|PRM_VARSRC;
    t1 = TmMin();
    t2 = t1;
    }
  else {
    t1 = MI.start;
    t2 = TmMax();
    }
  TmnDetach(idsrc, NULL, NULL, TMN_MODIFY|TMN_FIXED, NULL);     eG(32);
  TmnDetach(idsrg, NULL, NULL, TMN_MODIFY|TMN_FIXED, NULL);     eG(33);
  vDSP(1)("");
  CloseInput();                                                 eG(14);
  *MI.input = 0;
  vLOG(3)(_$_sources_$_, nsrc, nsrg);
  return nsrc;
eX_31: eX_32: eX_33: eX_34: eX_35: eX_36: eX_37:
eX_1: eX_2: eX_3: eX_4: eX_5: eX_6: eX_7: eX_8: eX_9:
eX_10: eX_11: eX_12: eX_14: eX_15: eX_17:
eX_20: eX_21: eX_22: eX_91: eX_92:
  eMSG(_internal_error_);
  }

  /*=============================================================== PrmReadEms
  */
static long PrmReadEms(    /* Quellst√§rken STAERKE.DEF einlesen.   */
  void )
  {
  dP(PrmReadEms);
  int i, ig, ic, j, k, n, idef, numdef, varems, isrc, isrg, icmp;
  int ns, ng, nc, sz, ndat, nhdr;
  int writesfc = 0;
  char name[80], *t, datname[80], fname[40], nn[80];
  long  rc, t1, t2, idems, idemg;
  float *pf;
  CMPREC *pcmp;
  SRCREC *psrc;
  SRGREC *psrg;
  strcpy(fname, "emissions.def");                                 //-2011-11-23
  vLOG(3)(_reading_$_, fname);
  if ((!PrmPcmp) || (!PrmPsrc) || (!PrmPsrg))                   eX(99);
  writesfc = (MI.flags & FLG_WRITESFC);
  varems = 0;
  ns = MI.numsrc;
  ng = MI.numsrg;
  nc = MI.sumcmp;
  idems = IDENT(EMSarr, 0, 0, 0);
  PrmPems = TmnCreate(idems, sizeof(float), 2, 0, ns-1, 0, nc-1);       eG(20);
  idemg = IDENT(EMSarr, 1, 0, 0);
  PrmPemg = TmnCreate(idemg, sizeof(float), 2, 0, ng-1, 0, nc-1);       eG(30);
  for (ig=0; ig<ng; ig++)
    for (ic=0; ic<nc; ic++) {
      pf = AryPtr(PrmPemg, ig, ic);  if (!pf)                   eX(31);
      *pf = 1.0;
      }
  if (0 > OpenInput(fname, MI.defname))                         eX(1);
  strcpy(MI.input, fname);
  rc = GetLine('.', Buf, BUFLEN);  if (rc < 0)                  eX(2);
  MI.emisfac = 1.0;
  GET("EMISFAC", Buf, "%f", &MI.emisfac);
  if (IsUndef(&MI.emisfac)) {
    strcpy(datname, "emisfac");
    DefVar("EMISFAC", "%f", &MI.emisfac, 1.0, &PrmPvpp);           eG(3);
    varems = 1;
    }
  for (ig=0; ig<ng; ig++) {
    psrg = AryPtr(PrmPsrg, ig);  if (!psrg)                        eX(32);
    strcpy(datname, "FQ");
    if (*psrg->name) {
      strcat(datname, ".");
      strcat(datname, psrg->name);
      }
    GET(datname, Buf, "%f", &psrg->fq);
    if (IsUndef(&psrg->fq)) {
      DefVar(datname, "%f", &psrg->fq, 1.0, &PrmPvpp);             eG(33);
      varems = 1;
      }
    }
  rc = CntLines('=', Buf, BUFLEN);  if (rc <= 1)       eX(24);  //-2001-04-29 lj
  n = strlen(Buf);
  numdef = 0;
  for (i=0; i<n; i++) {
    if (Buf[i] == '!')  continue;
    if (Buf[i] == 'E')  numdef++;
    else if (Buf[i] == '.')  break;                             //-2001-04-29 lj
    else                                                        eX(25);
    }
  if (numdef < 1)                                               eX(26);
  idef = 0;
  pcmp = (CMPREC*) PrmPcmp->start;  if (!pcmp)                  eX(6);
  sz = (MI.sumcmp+1)*sizeof(char*);
  CmpNames = ALLOC(sz);  if (!CmpNames)                         eX(7);
  for (i=0; i<MI.sumcmp; i++)
    CmpNames[i] = pcmp[i].name;
  CmpNames[MI.sumcmp] = NULL;

newtab: /*-------------- new tab header ------------------*/
  rc = GetLine('!', Buf, BUFLEN);  if (rc <= 0)                 eX(4);
  t = GetWord(Buf, name, 18);
  ToCap(name);
  if (CisCmp(name, "QUELLE") && CisCmp(name, "SOURCE"))         eX(5);
  nhdr = EvalHeader(CmpNames, Buf, DatPos, DatScale);           eG(8);
  for (i=0; i<nhdr; i++)
    if (DatPos[i] < 0)
      vLOG(1)("WARNING: unknown species in tab header at position %d!", i+1);
  for ( ; idef<numdef; ) {
    rc = GetLine('E', Buf, BUFLEN);
    if (rc <= 0) {
      if ((rc == -1) && (Buf[0] == '!'))  goto newtab;
      else                                                          eX(9);
      }
    idef++;
    ndat = GetList(Buf, name, 78, DatVec, MAXVAL);  if (ndat < 0)   eX(10);
    if (ndat != nhdr)                                               eX(11);
    isrg = -1;
    psrc = (SRCREC*) PrmPsrc->start;
    for (k=0; k<ns; k++, psrc++)
      if (0 == CisCmp(name, psrc->name))  break;
    isrc = k;
    if (isrc >= ns) {
      isrc = -1;
      psrg = (SRGREC*) PrmPsrg->start;
      for (k=0; k<ng; k++, psrg++)
        if (0 == CisCmp(name, psrg->name))  break;
      if (k >= ng)                                              eX(12);
      isrg = k;
    }
    for (j=0; j<ndat; j++) {
      k = DatPos[j];
      if (k < 0) continue;
      icmp = k;
      if (isrc >= 0) {
        pf = AryPtr(PrmPems, isrc, icmp);  if (!pf)                eX(13);
        strcpy(datname, "EQ.");
      }
      else {
        pf = AryPtr(PrmPemg, isrg, icmp);  if (!pf)                eX(34);
        strcpy(datname, "FQ.");
      }
      if (writesfc) {                             //-2012-10-30
        *pf = 0.0; 		
      }
      else if (IsUndef(&DatVec[j])) {
        varems = 1;
        strcat(datname, name);
        strcat(datname, ".");
        strcat(datname, CmpNames[k]);
        DefVar(datname, "%f", pf, DatScale[j], &PrmPvpp);          eG(14);
      }
      else {
        *pf = DatVec[j]*DatScale[j];
      }
    }
  }
  if (varems) {
    MI.status |= PRM_VARPRM|PRM_VAREMS;
    t1 = TmMin();
    t2 = t1;
    }
  else {
    t1 = MI.start;
    t2 = TmMax();
    }
  FREE(CmpNames);
  CmpNames = NULL;
  TmnDetach(idems, &t1, &t2, TMN_MODIFY|TMN_FIXED, NULL);       eG(15);
  TmnDetach(idemg, &t1, &t2, TMN_MODIFY|TMN_FIXED, NULL);       eG(35);
  vDSP(1)("");
  CloseInput();                                                 eG(16);
  *MI.input = 0;
  vLOG(3)(_$_emissions_, numdef);
  return numdef;
eX_99: eX_6: eX_1: eX_2: eX_3: eX_14: eX_33: eX_4: eX_24:
eX_5: eX_7: eX_8: eX_9:
eX_10: eX_11: eX_12: eX_13: eX_15: eX_16: eX_20: eX_30:
eX_25: eX_26: eX_31: eX_32: eX_34: eX_35: eX_100:
  eMSG(_internal_error_);
  }

  /*=============================================================== PrmReadChm
  */
static long PrmReadChm(   /* Chemische Reaktionen CHEMIE.DEF einlesen.  */
  void )
  {
  dP(PrmReadChm);
  long rc, ndat, id, t1, t2, sz;
  int nc, i, j, k, idef, numdef, varchm;
  char name[80], datname[80], fname[256], *pc;
  float *pf;
  CMPREC *pcmp;
  strcpy(fname, "chemics.def");                                 //-2011-11-23
  if ((MI.flags & FLG_CHEM) == 0)  return 0;
  nc = MI.sumcmp;
  vLOG(3)(_reading_$_, fname);
  if (0 > OpenInput(fname, MI.defname))                         eX(1);
  strcpy(MI.input, fname);
  rc = GetLine('.', Buf, BUFLEN);  if (rc < 0)                  eX(2);
  rc = CntLines('=', Buf, BUFLEN);  if (rc <= 1)                eX(3);
  pc = strchr(Buf, '.');
  if (pc)  numdef = pc - Buf -1; //-2001-05-30 uj
  else  numdef = rc - 1;
  rc = GetLine('!', Buf, BUFLEN);  if (rc <= 0)                 eX(4);
  pcmp = (CMPREC*) PrmPcmp->start;  if (!pcmp)                  eX(6);
  sz = (MI.sumcmp+1)*sizeof(char*);
  CmpNames = ALLOC(sz);  if (!CmpNames)                         eX(7);
  for (i=0; i<MI.sumcmp; i++)
    CmpNames[i] = pcmp[i].name;
  CmpNames[MI.sumcmp] = NULL;
  ndat = EvalHeader(CmpNames, Buf, DatPos, DatScale);           eG(8);
  id = IDENT(CHMarr, 0, 0, 0);
  PrmPchm = TmnCreate(id, sizeof(float), 2, 0, nc-1, 0, nc-1);  eG(9);
  varchm = 0;
  for (idef=0; idef<numdef; idef++) {
    rc = GetLine('C', Buf, BUFLEN);  if (rc <= 0)               eX(10);
    rc = GetList(Buf, name, 78, DatVec, MAXVAL);  if (rc < 0)   eX(91);
    if (ndat != rc)                                             eX(11);
    i = NamePos(name, CmpNames);  if (i < 0)                    eX(12);
    for (k=0; k<ndat; k++) {
      j = DatPos[k];
      if (j < 0)  continue;
      pf = AryPtr(PrmPchm, i, j);  if (!pf)                     eX(13);
      if (IsUndef(&DatVec[k])) {
        varchm = 1;
        sprintf(datname, "%s-%s", CmpNames[i], CmpNames[j]);
        DefVar(datname, "%f", pf, DatScale[k], &PrmPvpp);          eG(14);
        }
      else
        *pf = DatVec[k]*DatScale[k];
      }
    }
  if (varchm) {
    MI.status |= PRM_VARCHM|PRM_VARPRM;
    t1 = TmMin();
    t2 = t1;
    }
  else {
    t1 = MI.start;
    t2 = TmMax();
    }
  FREE(CmpNames);
  CmpNames = NULL;
  TmnDetach(id, &t1, &t2, TMN_MODIFY|TMN_FIXED, NULL);          eG(15);
  vDSP(1)("");
  CloseInput();                                                 eG(16);
  *MI.input = 0;
  vLOG(3)(_$_reactions_, numdef);
  return numdef;
eX_1: eX_2: eX_3: eX_4: eX_6: eX_7: eX_8: eX_9:
eX_10: eX_11: eX_12: eX_13: eX_14: eX_15: eX_16: eX_91:
  eMSG(_internal_error_);
  }

  /*=============================================================== PrmReadVar
  */
static long PrmReadVar(    /* Variable Parameter aus Zeitreihe lesen.  */
  void )
  {
  dP(PrmReadVar);
  long t1, t2, rc;
  static long position = 0;
  if (MI.status & PRM_VARPRM) {
    rc = 0;
    if (position == 0) {
      rc = OpenInput(MI.input, MI.defname);                             eG(1);
      if (rc > 0)  strcpy(MI.input, GenInputName);            //-2001-05-04 lj
      }
    t1 = StdTime;
    TmSetUndef(&t2);
    rc = ReadZtr(MI.input,NULL,&t1,&t2,StdDspLevel,&position,PrmPvpp);   eG(2);
    if (rc <= 0)                                                         eX(4);
    MI.ztrt1 = t1;
    if (TmIsUndef(&t2))  MI.ztrt2 = MI.end;
    else  MI.ztrt2 = t2;
    CloseInput();                                                       eG(3);
    }
  else {
    MI.ztrt1 = MI.start;
    MI.ztrt2 = MI.end;
    rc = 1;
    }
  return rc;
eX_1: eX_2: eX_3: eX_4:
  eMSG(_internal_error_);
  }


  /*================================================================== PrmInit
  */
long PrmInit(           /* initialize server    */
  long flags,           /* action flags         */
  char *istr )          /* server options       */
  {
  dP(PrmInit);
  long id, mask, t1, t2, mode, *pt1, *pt2, seed;
  int cycind, average, count;
  char *jstr, *ps;
  ARYDSC *pa;
  if (StdStatus & STD_INIT)  return 0;
  if (istr) {
    jstr = istr;
    ps = strstr(istr, "-v");
    if (ps) sscanf(ps+2, "%d", &StdLogLevel);
    ps = strstr(istr, "-y");
    if (ps) sscanf(ps+2, "%d", &StdDspLevel);
    ps = strstr(istr, "-d");
    if (ps) sscanf(ps+2, "%s", DefName);
    }
  else  jstr = "";
  vLOG(3)("PRM_%d.%d.%s (%08lx,%s)", StdVersion, StdRelease, StdPatch, flags, jstr);
  StdStatus |= flags;
  mask = -1;
  id = IDENT(PRMpar, 0, 0, 0);
  mode = TMN_MODIFY | TMN_FIXED | TMN_UNIQUE;
  pa = TmnAttach(id, NULL, NULL, mode|TMN_READ, NULL);  eG(10);
  if (pa) {
    PMI = pa->start;
    MI.status |=  PRM_CONTINUE;
    MI.status &= ~PRM_BREAK;
    cycind  = MI.cycind;
    average = MI.average;
  }
  else {
    pa = TmnCreate(id, sizeof(PRMINF), 1, 0, 0);        eG(7);
    PMI = pa->start;
  }
  MI.flags |= FLG_SCAT;
  strcpy(MI.dpath, StdPath);
  strcpy(MI.defname, DefName);
  strcpy(MI.cltime, TimePtr());
  strcpy(MI.cldate, DatePtr());
  count = MI.count;                        /* 99-08-06 UJ */
  seed = MI.seed;
  PrmReadParam();                                       eG(1);
  if (count > MI.count) MI.count = count;
  RndSetSeed(MI.seed);
  if (MI.numgrp < 5)  MI.flags &= ~FLG_SCAT;
  PrmReadAsl();                                         eG(2);
  PrmCmpUpdate();                                       eG(3);
  PrmReadSrc();                                         eG(4);
  PrmReadEms();                                         eG(5);
  if (MI.flags & FLG_CHEM)  PrmReadChm();               eG(6);
  if (MI.status & PRM_VARPRM) {
    t1 = TmMin();
    t2 = t1;
  }
  else {
    t1 = MI.start;
    t2 = TmMax();
  }
  MI.ztrt1 = t1;                //-99-03-26
  MI.ztrt2 = t2;                //-99-03-26
  pt1 = &t1;                    //-99-03-26
  pt2 = &t2;                    //-99-03-26
  MI.dost1 = MI.start;
  strcpy(MI.input, "variable.def");                               //-2011-11-23
  TmnDetach(id, pt1, pt2, mode, NULL);                          eG(8);
  TmnCreator(id, mask, TMN_FIXED, istr, PrmServer, PrmHeader);  eG(9);
  StdStatus |= STD_INIT;
  return 0;
eX_1:  eX_2:  eX_3:  eX_4:  eX_5:  eX_6:
eX_7:  eX_8:  eX_10:
eX_9:
  eMSG(_internal_error_);
  }

  /*================================================================= PrmServer
  */
long PrmServer(
  char *ss )
  {
  dP(PrmServer);
  long id, mask, t1, t2;
  if (StdArg(ss))  return 0;
  if (*ss) {
    switch (ss[1]) {
      case 'd':  strcpy(DefName, ss+2);
                 break;
      default:   ;
      }
    return 0;
    }
  if ((StdStatus & STD_INIT) == 0) {
    PrmInit(0, "");                                     eG(1);
    }
  id = IDENT(PRMpar, 0, 0, 0);
  mask = -1;
  TmnCreator(id, mask, TMN_FIXED, "", NULL, PrmHeader);  eG(2);
  PrmReadVar();                                         eG(3);
  t1 = MI.ztrt1;
  t2 = MI.ztrt2;
  TmnAttach(id, NULL, NULL, TMN_MODIFY, NULL);          eG(4);
  TmnDetach(id, &t1, &t2, TMN_MODIFY, NULL);            eG(5);
  if (MI.status & PRM_VARASL) {
    id = IDENT(ASLarr, 0, 0, 0);
    TmnAttach(id, NULL, NULL, TMN_MODIFY, NULL);        eG(14);
    TmnDetach(id, &t1, &t2, TMN_MODIFY, NULL);          eG(15);
    id = IDENT(CMPtab, 0, 0, 0);
    TmnAttach(id, NULL, NULL, TMN_MODIFY, NULL);        eG(24);
    TmnDetach(id, &t1, &t2, TMN_MODIFY, NULL);          eG(25);
    PrmCmpUpdate();                                     eG(7);
    }
  if (MI.status & PRM_VAREMS) {
    id = IDENT(EMSarr, 0, 0, 0);
    TmnAttach(id, NULL, NULL, TMN_MODIFY, NULL);        eG(34);
    TmnDetach(id, &t1, &t2, TMN_MODIFY, NULL);          eG(35);
    id = IDENT(EMSarr, 1, 0, 0);
    TmnAttach(id, NULL, NULL, TMN_MODIFY, NULL);        eG(44);
    TmnDetach(id, &t1, &t2, TMN_MODIFY, NULL);          eG(45);
    }
  if (MI.status & PRM_VARSRC) {
    id = IDENT(SRCtab, 0, 0, 0);
    TmnAttach(id, NULL, NULL, TMN_MODIFY, NULL);        eG(54);
    TmnDetach(id, &t1, &t2, TMN_MODIFY, NULL);          eG(55);
    id = IDENT(SRCarr, 0, 0, 0);
    TmnAttach(id, NULL, NULL, TMN_MODIFY, NULL);        eG(64);
    TmnDetach(id, &t1, &t2, TMN_MODIFY, NULL);          eG(65);
    }
  if (MI.status & PRM_VARCHM) {
    id = IDENT(CHMarr, 0, 0, 0);
    TmnAttach(id, NULL, NULL, TMN_MODIFY, NULL);        eG(74);
    TmnDetach(id, &t1, &t2, TMN_MODIFY, NULL);          eG(75);
    }
  id = IDENT(PRMpar, 0, 0, 0);
  TmnCreator(id, mask, TMN_FIXED, "", PrmServer, PrmHeader); eG(6);
  return 0;
eX_1: eX_2: eX_6: eX_3:
eX_4:  eX_5:
eX_14: eX_15:
eX_24: eX_25:
eX_34: eX_35:
eX_44: eX_45:
eX_54: eX_55:
eX_64: eX_65:
eX_74: eX_75:
eX_7:
  eMSG(_internal_error_);
  }

//==========================================================================
//
// history:
//
// 2002-09-24 lj 1.0.0  final release candidate
// 2002-12-10 lj 1.0.4  SRCREC.tmp 
// 2003-02-22 lj 1.1.2  new random numbers
// 2004-06-29 lj 2.0.0  flag ODOR
// 2004-10-04 lj 2.0.3  flag ODMOD
// 2004-11-26 lj 2.1.7  string length for names = 256
// 2005-03-11 lj 2.1.16 param.def:Schwelle replaced by OdorThr
// 2005-03-17 uj 2.2.0  version number upgrade
// 2006-02-25 uj        Kennung in quotation marks
// 2006-10-26 lj 2.3.0  external strings
// 2011-06-29 uj 2.5.0  DMNA header
// 2011-11-21 lj 2.6.0  flag NODILUTE
// 2011-11-23 lj        english names
// 2012-10-30 uj        set emission 0 if writesfc
// 2014-06-26 uj 2.6.11 eG/eX adjusted
//
//==========================================================================

