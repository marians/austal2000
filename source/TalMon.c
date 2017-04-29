// ======================================================== TalMon.c
//
// Calculation of time series at monitor points for AUSTAL2000
// ===========================================================
//
// Copyright (C) Umweltbundesamt, Dessau-Roﬂlau, Germany, 2002-2005
// Copyright (C) Janicke Consulting, 88662 ‹berlingen, Germany, 2002-2011
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
// last change:  2011-11-23 lj
//
//========================================================================
//
//  Berechnung der Zeitreihen an den Beurteilungspunkten
//  Tagesmittel erfordert mindestens 12 Stundenwerte
//
//==========================================================================

static char *TalMonVersion = "2.6.0";
static char *eMODn = "TalMon";
static int CHECK = 0;

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <locale.h>

#include "IBJmsg.h"
#include "IBJary.h"
#include "IBJtxt.h"
#include "IBJdmn.h"
#include "TalCfg.h"
#include "TalStt.h"
#include "TALuti.h"
#include "TalMon.h"
#include "TalMon.nls"

//#ifdef MAIN
//  static char *TipGruppen[6] = { "gas", "gas", "gas", "pm3", "pm4", "pmu" };
//  static char *TipGrpXten[6] = {    "",  "-1",  "-2",  "-3",  "-4",  "-u" };
//#else
  #include "TalInp.h"
//#endif

static ARYDSC MntCAry, SpcCAry;
static ARYDSC MntSAry, SpcSAry;
static TXTSTR UsrHeader = { NULL, 0 };
static TXTSTR SysHeader = { NULL, 0 };
static char MntName[80] = "work/mnt%c%04ld";                      //-2008-07-30
static char Path[256];
static int SpcIndex[8], SpcIsGas[8];
static int SpcDayNum, SpcHourNum;
static char SpcName[16];                                          //-2011-06-18
static char SpcConForm[32], SpcDayForm[32], SpcDepForm[32], SpcHourForm[32];
static char SpcConUnit[32], SpcDepUnit[32], SpcUc[32];
static float SpcConFac, SpcDepFac;
static float SpcConRef, SpcDayRef, SpcDepRef, SpcHourRef;
static int SpcSequ=-1;
static char **Names, YearForm[32], DayForm[32], HourForm[32];
static double T0, T1, T2;
static int Nv, Nc, Nm, Intervall, AddMeasured;
static char **MntN;
static float *MntX, *MntY, *MntZ;
static int *MntI, *MntJ, *MntK, *GrdL, *GrdI;
static TXTSTR MntNames = { NULL, 0 };
static float RefDays = 365;
static float RefHours = 365*24;
static char locale[256], locl[256]="";
static int Scatter = 1;
//
static int EvalRated;                                             //-2008-09-24
static int NumOdor = 0;                                           //-2008-09-24
static float *_OdorValues[TIP_ADDODOR + 1];                       //-2008-09-24
static int NumPoints = 0;
static char OdorNames[TIP_ADDODOR + 1][32];

//============================================================ ReadMntValues
//
static int ReadMntValues( int i )
  {
  dP(ReadMntValues);
  int n;
  char name[120], fname[256], s[80];
  char t1s[40], t2s[40], *_ps=NULL;
  double t0, t1, t2;
  if (CHECK) vMsg("TMO: read point %d", i);
  if (MntCAry.start)  AryFree(&MntCAry);
  if (MntSAry.start)  AryFree(&MntSAry);
  sprintf(name, MntName, 'c', i);                                 //-2008-07-30
  strcpy(fname, Path);
  strcat(fname, name);
  if (CHECK) vMsg("TMO: read file %s", fname);
  if (1 > DmnRead(fname, &UsrHeader, &SysHeader, &MntCAry))    eX(20);
  //
  if (Scatter) {                                                  //-2008-07-30
    sprintf(name, MntName, 's', i);
    strcpy(fname, Path);
    strcat(fname, name);
    if (CHECK) vMsg("TMO: read file %s", fname);
    if (1 > DmnRead(fname, &UsrHeader, &SysHeader, &MntSAry)) {
      if (i > 1)                                                eX(20);
      Scatter = 0;
      if (MntSAry.start)  AryFree(&MntSAry);
    }
  }
  //
  if (MntCAry.elmsz != sizeof(float))                          eX(1);
  if (MntCAry.numdm != 2)                                      eX(2);
  n = MntCAry.bound[0].hgh - MntCAry.bound[0].low + 1;
  if (Nv) {
    if (n != Nv)                                              eX(3);
  }
  else  Nv = n;
  n = MntCAry.bound[1].hgh - MntCAry.bound[1].low + 1;
  if (Nc) {
    if (n != Nc)                                              eX(4);
  }
  else  Nc = n;
  if (!Intervall) {
    if (1 == DmnGetString(UsrHeader.s, "Interval", &_ps, 1)) {
      Intervall = MsgDateSeconds(MsgDateVal("00:00:00"), MsgDateVal(_ps)) + 0.5; //-2002-07-04
      FREE(_ps);  _ps = NULL;
    }
    else  Intervall = 3600;
    if (1 == DmnGetString(UsrHeader.s, "refdate|rdat", &_ps, 1)) {  //-2008-12-04
      T0 = MsgDateVal(_ps);
      FREE(_ps);  _ps = NULL;
    }
    if (1 != DmnGetString(UsrHeader.s, "T1", &_ps, 1))                eX(7);
    strcpy(t1s, _ps);  FREE(_ps);  _ps = NULL;
    if (1 != DmnGetString(UsrHeader.s, "T2", &_ps, 1))                eX(8);
    strcpy(t2s, _ps);  FREE(_ps);  _ps = NULL;
    t0 = MsgDateVal("00:00:00");
    t1 = MsgDateVal(t1s);
    T1 = MsgDateIncrease(T0, (int)(MsgDateSeconds(t0, t1) + 0.5));  //-2007-02-03
    t2 = MsgDateVal(t2s);
    T2 = MsgDateIncrease(T0, (int)(MsgDateSeconds(t0, t2) + 0.5));  //-2007-02-03
    Names = ALLOC((Nc+1)*sizeof(char*));
    n = DmnGetString(UsrHeader.s, "name", Names, Nc);  if (n != Nc)   eX(9);
  }
  if (1 != DmnGetString(UsrHeader.s, "mntn", &_ps, 1))                eX(18);
  sprintf(s, " %6s", TxtQuote(_ps));
  TxtCat(&MntNames, s);
  if (CHECK) vMsg("TMO: C");
  FREE(_ps);
  if (1 != DmnGetFloat(UsrHeader.s, "mntx", "%f", MntX+i-1, 1))       eX(10);
  if (1 != DmnGetFloat(UsrHeader.s, "mnty", "%f", MntY+i-1, 1))       eX(11);
  if (1 != DmnGetFloat(UsrHeader.s, "mntz", "%f", MntZ+i-1, 1))       eX(12);
  if (1 != DmnGetInt(UsrHeader.s, "mnti", "%d", MntI+i-1, 1))         eX(13);
  if (1 != DmnGetInt(UsrHeader.s, "mntj", "%d", MntJ+i-1, 1))         eX(14);
  if (1 != DmnGetInt(UsrHeader.s, "mntk", "%d", MntK+i-1, 1))         eX(15);
  if (1 != DmnGetInt(UsrHeader.s, "grdl", "%d", GrdL+i-1, 1))         eX(16);
  if (1 != DmnGetInt(UsrHeader.s, "grdi", "%d", GrdI+i-1, 1))         eX(17);
  return 0;
eX_20:
  eMSG(_cant_read_$_, fname);
eX_1: eX_2: eX_3: eX_4: eX_7: eX_8: eX_9:
eX_10: eX_11: eX_12: eX_13: eX_14: eX_15: eX_16: eX_17: eX_18:
  eMSG(_inconsistent_structure_);
  }

//================================================================= AddValues
//
static int AddValues( int n ) {
  dP(AddValues);
  int ii, i, i0, l, ic;
  char name[40];
  float *psrc, *pdst;
  if (CHECK) vMsg("AddValues(%d)", n);
  if (!SpcCAry.start) {
    AryCreate(&SpcCAry, sizeof(float), 2, 1, Nv, 1, Nm);       eG(1);
  }
  if (Scatter && !SpcSAry.start) {                                //-2008-07-30
    AryCreate(&SpcSAry, sizeof(float), 2, 1, Nv, 1, Nm);       eG(2);
  }
  strcpy(name, "gas.");
  strcat(name, SpcName);
  for (ii=0; ii<8; ii++) {
    SpcIsGas[ii] = 0;
    SpcIndex[ii] = -1;
  }
  ii = 0;
  for (ic=0; ic<3; ic++) {
    sprintf(name, "%s.%s%s", SttGroups[ic], SpcName, SttGrpXten[ic]); //-2011-11-23
    for (l=0; l<Nc; l++) {
      if (!strcmp(Names[l], name)) {
        SpcIndex[ii] = l;
        SpcIsGas[ii] = 1;
        ii++;
        break;
      }
    }
  }
  if (ii == 0)
    return 1;                                                     //-2001-07-07
  //
  i0 = MntCAry.bound[0].low;
  for (i=0; i<Nv; i++) {                                          //-2008-07-30
    float sumc = 0;
    float sums = 0;                                               //-2008-07-30
    float cval = 0;
    for (ii=0; ii<8; ii++) {
      l = SpcIndex[ii];
      if (l < 0)
        break;
      if (!SpcIsGas[ii])
        continue;
      psrc = AryPtr(&MntCAry, i0+i, l);  if (!psrc)              eX(4);
      if (*psrc >= 0) {
        cval = SpcConFac*(*psrc);
        sumc += cval;
      }
      else {
        sumc = -1;
        sums = -1;                                                //-2008-07-30
        break;
      }
      if (Scatter) {                                              //-2008-07-30
        psrc = AryPtr(&MntSAry, i0+i, l);  if (!psrc)             eX(6);
        sums += cval*(*psrc);
      }
    }
    pdst = AryPtr(&SpcCAry,  1+i, n);  if (!pdst)                 eX(3);
    *pdst = sumc;
    if (Scatter) {                                                //-2008-07-30
      pdst = AryPtr(&SpcSAry,  1+i, n);  if (!pdst)               eX(5);
      if (sumc > 0) {
        sums /= sumc;
      }
      *pdst = sums;
    }
  }
  return 0;
eX_1: eX_2: eX_3: eX_4: eX_5: eX_6:                               //-2008-07-30
  eMSG(_internal_error_);
}

//============================================================= WriteSpcValues
//
static int WriteSpcValues(char *fn, int scatter) {                //-2008-07-30
  dP(WriteSpcValues);
  char s[256], *_ps, fname[256], src, dst, odor;
  ARYDSC *pa = NULL;                                              //-2008-07-30
  float fac = 1;                                                  //-2008-07-30
  //
  if (*locl) {                                                  //-2003-07-07
    strcpy(locale, setlocale(LC_NUMERIC, NULL));
    MsgSetLocale(locl);                                         //-2008-10-17
  }
  if (!strcmp(locl, "german"))  { src = '.';  dst = ','; }
  else                          { src = ',';  dst = '.'; }
  DmnRplChar(&UsrHeader, "xmin", src, dst);
  DmnRplChar(&UsrHeader, "ymin", src, dst);
  DmnRplChar(&UsrHeader, "valid", src, dst);
  DmnRplChar(&UsrHeader, "sk|zk", src, dst);
  DmnRplChar(&UsrHeader, "dd|delta", src, dst);
  //
  DmnRplName(&UsrHeader, "kennung", "idnt");
  DmnRplName(&UsrHeader, "refdate", "rdat");                      //-2008-12-04
  DmnRplName(&UsrHeader, "interval", "dt");                       //-2008-12-04
  DmnRplValues(&UsrHeader, "name", TxtQuote(SpcName));
  DmnRplValues(&UsrHeader, "file", TxtQuote(fn));
  DmnRplValues(&UsrHeader, "einheit", NULL);
  DmnRplValues(&UsrHeader, "unit", SpcConUnit);
  DmnRplValues(&UsrHeader, "form", SpcConForm);
  odor = (0 == strncmp(SpcName, "odor", 4));
  if (*locl) {
    sprintf(s, "\"%s\"", locl);
    DmnRplValues(&UsrHeader, "locl", s);                          //-2003-07-07
  }
  else DmnRplValues(&UsrHeader, "locl", NULL);                    //-2003-07-07
  DmnRplValues(&UsrHeader, "refc", NULL);
  if (scatter) {                                                  //-2008-07-30
    pa = &SpcSAry;
    fac = 100;
    if (odor) {                                                   //-2011-06-09
      DmnRplValues(&UsrHeader, "unit", "\"%\""); 
      DmnRplValues(&UsrHeader, "form", "\"sct%5.1f\"");           //-2011-06-09
      DmnRplValues(&UsrHeader, "refv", "10.0");
      DmnRplValues(&UsrHeader, "artp", "\"EHP\"");                //-2011-06-09
    }
    else {
      DmnRplValues(&UsrHeader, "form", "\"dev%(*100)5.1f\"");     //-2011-06-09
      DmnRplValues(&UsrHeader, "unit", "\"1\"");                  //-2011-06-09
      DmnRplValues(&UsrHeader, "refv", "1.0");
      DmnRplValues(&UsrHeader, "artp", "\"EZ\"");                 //-2011-06-09
    }
   
  }
  else {
    pa = &SpcCAry;
    fac = 1;
    DmnRplValues(&UsrHeader, "unit", SpcConUnit);
    DmnRplValues(&UsrHeader, "form", SpcConForm);
    sprintf(s, YearForm, SpcConRef);
    DmnRplValues(&UsrHeader, "refv", s);
    if (odor) {
     sprintf(s, "\"FHP%4.2f\"", SttOdorThreshold);                //-2011-11-23
     DmnRplValues(&UsrHeader, "artp", s);                         //-2011-06-09
    }
    else {
     DmnRplValues(&UsrHeader, "artp", "\"CZ\"");                  //-2011-06-09
    }
  }
  DmnRplValues(&UsrHeader, "axes", "\"ti\"");
  DmnRplValues(&UsrHeader, "t1", TxtQuote(MsgDateString(T1-T0))); //-2008-12-04
  DmnRplValues(&UsrHeader, "t2", TxtQuote(MsgDateString(T2-T0))); //-2008-12-04
  DmnRplValues(&UsrHeader, "undf", "-1");
  DmnRplValues(&UsrHeader, "mntn", MntNames.s);
  _ps = ALLOC(20*Nm+4);
  if (Nm != DmnPutFloat(_ps, 20*Nm, "", " %8.1f", MntX, Nm))    eX(1);
  DmnRplValues(&UsrHeader, "mntx", _ps);
  *_ps = 0;
  if (Nm != DmnPutFloat(_ps, 20*Nm, "", " %8.1f", MntY, Nm))    eX(2);
  DmnRplValues(&UsrHeader, "mnty", _ps);
  *_ps = 0;
  if (Nm != DmnPutFloat(_ps, 20*Nm, "", " %8.1f", MntZ, Nm))    eX(3);
  DmnRplValues(&UsrHeader, "mntz", _ps);
  *_ps = 0;
  if (Nm != DmnPutInt(_ps, 20*Nm, "", " %8d", MntI, Nm))        eX(4);
  DmnRplValues(&UsrHeader, "mnti", _ps);
  *_ps = 0;
  if (Nm != DmnPutInt(_ps, 20*Nm, "", " %8d", MntJ, Nm))        eX(5);
  DmnRplValues(&UsrHeader, "mntj", _ps);
  *_ps = 0;
  if (Nm != DmnPutInt(_ps, 20*Nm, "", " %8d", MntK, Nm))        eX(6);
  DmnRplValues(&UsrHeader, "mntk", _ps);
  *_ps = 0;
  if (Nm != DmnPutInt(_ps, 20*Nm, "", " %8d", GrdL, Nm))        eX(7);
  DmnRplValues(&UsrHeader, "grdl", _ps);
  *_ps = 0;
  if (Nm != DmnPutInt(_ps, 20*Nm, "", " %8d", GrdI, Nm))        eX(8);
  DmnRplValues(&UsrHeader, "grdi", _ps);
  FREE(_ps);
  { // direct write
    FILE *f;
    int i, j;
    float *pf;
    sprintf(fname, "%s%s.dmna", Path, fn);
    f = fopen(fname, "w");  if (!f)                 eX(10);
    fprintf(f, "%s", UsrHeader.s);
    fprintf(f, "dims  2\n");                                      //-2014-01-28
    fprintf(f, "sequ  \"i+,j+\"\n");                              //-2008-12-11
    fprintf(f, "lowb  %3d  %3d\n", 1, 1);
    fprintf(f, "hghb  %3d  %3d\n", Nv, Nm);
    fprintf(f, "*\n");
    for (i=1; i<=Nv; i++) {
      for (j=1; j<=Nm; j++) {
        pf = AryPtr(pa, i, j);  if (!pf)            eX(11);
        fprintf(f, YearForm, fac*(*pf));                          //-2008-07-30
      }
      fprintf(f, " ' %s\n", MsgDateString(MsgDateIncrease(T0, i*Intervall)));
    }
    fclose(f);
  }
  if (*locl)  setlocale(LC_NUMERIC, locale);
  return 0;
eX_1: eX_2: eX_3: eX_4: eX_5: eX_6: eX_7: eX_8:
  eMSG(_internal_error_);
eX_10: eX_11:
  eMSG(_cant_write_$_, fname);
}

//============================================================== TmoEvalSeries
//
static int add( ARYDSC *psrc, ARYDSC *pdst ) {
  int i, i1, i2, j, j1, j2, js, jd;
  float fs, fd;
  if (!psrc)
    return 1;
  if (!psrc->start)
    return 2;
  if (psrc->numdm != 2)
    return 3;
  if (psrc->elmsz != sizeof(float))
    return 4;
  if (!pdst)
    return 11;
  if (!pdst->start)
    return 12;
  if (pdst->numdm != 2)
    return 13;
  if (pdst->elmsz != sizeof(float))
    return 14;
  i1 = psrc->bound[0].low;
  i2 = psrc->bound[0].hgh;
  j1 = psrc->bound[1].low;
  j2 = psrc->bound[1].hgh;
  jd = pdst->bound[1].hgh;                            //-2001-12-06
  if (i1 != pdst->bound[0].low)
    return 21;
  if (i2 != pdst->bound[0].hgh)
    return 22;
  if (j1 != pdst->bound[1].low)
    return 23;
  for (i=i1; i<=i2; i++) {
    for (j=j1; j<=jd; j++) {                          //-2001-12-06
      js = (j > j2) ? j2 : j;                         //-2001-12-06
      fs = *(float*)AryPtrX(psrc, i, js);             //-2001-12-06
      fd = *(float*)AryPtrX(pdst, i, j);
      if (fs<0 || fd<0)  fd = -1;
      else  fd += fs;
      *(float*)AryPtrX(pdst, i, j) = fd;
    }
  }
  return 0;
}

static int compare( const void *p1, const void *p2 ) {
  if      (*(float*)p1 < *(float*)p2)
    return 1;
  else if (*(float*)p1 > *(float*)p2)
    return -1;
  else
    return 0;
}

static int TmoEvalSeries( int ii ) {
  dP(TmoEvalSeries);
  int i, i1, i2, n, n1, n2, nv, nm;
  int nparts=0, odor;                                             //-2008-07-30
  char s[80], fn[256], gn[256], sep[160], *pc;
  char shour[16], syear[16], sday[16];                            //-2008-07-28
  FILE *f;
  ARYDSC clcc, msrc, clcs;                                        //-2008-07-30
  TXTSTR usr = { NULL, 0 };
  TXTSTR sys = { NULL, 0 };
  //
  if (CHECK) vMsg("TMO: TmoEvalSeries");
  memset(&clcc, 0, sizeof(ARYDSC));
  memset(&clcs, 0, sizeof(ARYDSC));                               //-2008-07-30
  memset(&msrc, 0, sizeof(ARYDSC));
  *sep = 0;
  odor = (0 == strncmp(SpcName, "odor", 4));
  //
  for (i=0; i<=strlen(CfgYearString); i++)                        //-2008-07-28
    syear[i] = toupper(CfgYearString[i]);
  for (i=0; i<=strlen(CfgDayString); i++)
    sday[i] = toupper(CfgDayString[i]);
  for (i=0; i<=strlen(CfgHourString); i++)
    shour[i] = toupper(CfgHourString[i]);
  //
  if (AddMeasured) {
    sprintf(gn, "%s%s-%s.dmna", Path, SpcName, CfgMntBckString);  //-2006-10-31
    if (!DmnFileExist(gn))
      return 0;
    DmnRead(gn, &usr, &sys, &msrc);                                eG(10);
  }
  if (Scatter) {                                                  //-2008-07-30
    sprintf(fn, "%s%s-%s.dmna", Path, SpcName, CfgMntSctString);
    if (!DmnFileExist(fn))
      return 0;
    DmnRead(fn, &usr, &sys, &clcs);                                eG(1);
  }
  sprintf(fn, "%s%s-%s.dmna", Path, SpcName, CfgMntAddString);    //-2006-10-31
  if (!DmnFileExist(fn))
    return 0;                                                     //-2002-08-28
  DmnRead(fn, &usr, &sys, &clcc);                                  eG(1);
  for (pc=SpcName; (*pc); pc++) *pc = toupper(*pc);
  //
  if (clcc.numdm != 2)                                             eX(2);
  i1 = clcc.bound[0].low;
  i2 = clcc.bound[0].hgh;
  n1 = clcc.bound[1].low;
  n2 = clcc.bound[1].hgh;
  nv = i2 - i1 + 1;
  nm = n2 - n1 + 1;
  if (!strcmp(SpcName, "ODOR")) {                                 //-2008-09-24
    NumPoints = nm;
    for (i=0; i<=TIP_ADDODOR; i++)
      _OdorValues[i] = ALLOC(nm*sizeof(float));
  }
  if (Scatter) {                                                  //-2008-07-30
    float v, *ps;
    for (i=i1; i<=i2; i++) {
      for (n=n1; n<=n2; n++) {
        v = *(float*)AryPtr(&clcc, i, n);
        ps = (float*)AryPtr(&clcs, i, n);
        if (v > 0  && !odor)                                      //-2011-06-09
          *ps = v*(*ps);
      }
    }
  }
  if (AddMeasured) {
    n = add(&msrc, &clcc); if (n)                                  eX(11);
  }
  f = (MsgFile) ? MsgFile : stdout;
  if (ii <= 0) {
    MntN = ALLOC(nm*sizeof(char*));
    MntX = ALLOC(nm*sizeof(float));
    MntY = ALLOC(nm*sizeof(float));
    MntZ = ALLOC(nm*sizeof(float));
    n = DmnGetString(usr.s, "mntn", MntN, nm);  if (n != nm)        eX(3);
    n = DmnGetFloat(usr.s, "mntx", "%f", MntX, nm);  if (n != nm)   eX(4);
    n = DmnGetFloat(usr.s, "mnty", "%f", MntY, nm);  if (n != nm)   eX(5);
    n = DmnGetFloat(usr.s, "mntz", "%f", MntZ, nm);  if (n != nm)   eX(6);
    fprintf(f, "%s", _evaluation_t_);
    strcat(sep, _evaluation_u_);
    if (AddMeasured) {
      fprintf(f, "%s\n", _total_load_t_);
      strcat(sep, _total_load_u_);
    }
    else {
      fprintf(f, "%s\n", _additional_load_t_);
      strcat(sep, _additional_load_u_);
    }
    fprintf(f, "%s\n", sep);
    fprintf(f, "%-12s", _point_);																	//-2008-04-17
    for (n=0; n<nm; n++) {
      if (Scatter) fprintf(f, "      ");                          //-2008-07-30
      fprintf(f, "%12s", MntN[n]);
    }
    fprintf(f, "\n");
    fprintf(f, "%-12s", "xp");																		//-2008-04-17
    for (n=0; n<nm; n++) {
      if (Scatter) fprintf(f, "      ");                          //-2008-07-30
      fprintf(f, "%12.0f", MntX[n]);
    }
    fprintf(f, "\n");
    fprintf(f, "%-12s", "yp");																		//-2008-04-17
    for (n=0; n<nm; n++) {
      if (Scatter) fprintf(f, "      ");                          //-2008-07-30
      fprintf(f, "%12.0f", MntY[n]);
    }
    fprintf(f, "\n");
    fprintf(f, "%-12s", "hp");																		//-2008-04-17
    for (n=0; n<nm; n++) {
      if (Scatter) fprintf(f, "      ");                          //-2008-07-30
      fprintf(f, "%12.1f", MntZ[n]);
    }
    fprintf(f, "\n");
    for (n=0; n<nm; n++)  if (MntN[n])  FREE(MntN[n]);
    FREE(MntN);
    FREE(MntX);
    FREE(MntY);
    FREE(MntZ);
    nparts++;                                                     //-2002-08-28
  }
  if (ii <= 1) {                                                  //-2002-08-28
    fprintf(f, "------------");													          //-2008-03-10
    for (n=0; n<nm; n++) {
      fprintf(f, "+-----------");
      //if (strlen(YearForm) > 6)  fprintf(f, "----");              //-2003-02-21
      if (Scatter)  fprintf(f, "------");                         //-2008-07-30
    }
    fprintf(f, "\n");
    nparts++;                                                     //-2002-08-28
  }
  if (SpcConRef > 0) {
    float c, s, csum, vsum;
    int av;
    fprintf(f, "%-8s", SpcName);                                  //-2008-03-10
    fprintf(f, " %s00", syear);                                   //-2008-07-28
    for (n=1; n<=nm; n++) {
      csum = 0;
      vsum = 0;
      av = 0;
      for (i=1; i<=nv; i++) {
        c = *(float*) AryPtrX(&clcc, i, n);
        if (c < 0)
          continue;
        csum += c;
        if (Scatter) {                                            //-2008-07-30
          s = *(float*)AryPtrX(&clcs, i, n);
          vsum += s*s;
        }
        av++;
      }
      if (av > 0)
        csum /= av;
      fprintf(f, YearForm, csum);
      if (odor) {                                                 //-2008-09-24
        _OdorValues[NumOdor][n-1] = csum;
      }
      if (Scatter) {                                              //-2008-07-30
        if (vsum > 0)
          vsum = sqrt(vsum)/av;
        if (odor) {
          if (csum < 0)
            fprintf(f, " -100 ");
          else if (vsum > 99.9)
            fprintf(f, "  100 ");
          else
            fprintf(f, " %4.1f ", vsum);
        }
        else {
          if (csum > 0)
            vsum = 100*vsum/csum;
          if (csum < 0)
            fprintf(f, " -100%%");
          else if (vsum > 99.9)
            fprintf(f, "  100%%");
          else
            fprintf(f, " %4.1f%%", vsum);
        }
      }
    }
    fprintf(f, "  %s\n", SpcUc);
    nparts++;
  }
  if (odor) {                                                     //-2008-09-24
    strcpy(OdorNames[NumOdor], SpcName);
    NumOdor++;
  }
  if (SpcDayRef>0 && nv>=24) {
    float *vv, c, v;
    int nd, k, id, av, ii, ndrop, nu;                             //-2001-07-02
    int dnum = SpcDayNum;                                         //-2001-07-02
    nd = nv/24;
    vv = ALLOC(2*nd*sizeof(float));                               //-2008-07-30
    for (ii=0; ii<2; ii++) {                                      //-2001-07-02
      dnum = (ii > 0) ? 0 : SpcDayNum;                            //-2002-08-10
      sprintf(s, "%s%02d", sday, dnum);                           //-2008-07-28
      fprintf(f, "%-8s", SpcName);                                //-2008-03-10
      fprintf(f, " %s", s);
      for (n=1; n<=nm; n++) {
        ndrop = 0;                                                //-2002-08-10
        for (i=1, id=0; id<nd; id++) {
          vv[2*id] = 0;                                           //-2008-07-30
          vv[2*id+1] = 0;                                         //-2008-07-30
          av = 0;
          for (k=0; k<24; k++, i++) {
            c = *(float*)AryPtrX(&clcc, i, n);
            if (c < 0)
              continue;
            vv[2*id] += c;                                        //-2008-07-30
            if (Scatter) {                                        //-2008-07-30
              v = *(float*)AryPtrX(&clcs, i, n);
              vv[2*id+1] += v*v;
            }
            av++;
          }
          if (av >= 12) {
            vv[2*id] /= av;                                       //-2008-07-30
            if (Scatter)                                          //-2008-07-30
              vv[2*id+1] = sqrt(vv[2*id+1])/av;
          }
          else {
            vv[2*id] = -1;                                        //-2008-07-30
            vv[2*id+1] = -1;                                      //-2008-07-30
            ndrop++;
          }
        }
        nu = nd - ndrop;
        qsort(vv, nd, 2*sizeof(float), compare);                  //-2008-07-30
        if (ii > 0)  dnum = 0;                        //-2002-08-10
        else if (nu < 0.9*RefDays)  dnum = SpcDayNum*nu/RefDays;
        i = (dnum < nd) ? dnum : nd-1;
        for ( ; i>0; i--)
          if (vv[2*i] >= 0)
            break;  // ???
        c = vv[2*i];                                              //-2008-07-30
        fprintf(f, YearForm, c);
        if (Scatter) {                                            //-2008-07-30
          v = 100*vv[2*i+1];
          if (c > 0)
            v /= c;
          if (c < 0)
            fprintf(f, " -100%%");
          else if (v > 99.9)
            fprintf(f, "  100%%");
          else
            fprintf(f, " %4.1f%%", v);
        }
      }
      if (ii==0 && dnum<SpcDayNum)  fprintf(f, "  *** %d ***", dnum);
      fprintf(f, "  %s\n", SpcUc);
      nparts++;
      if (dnum == 0)
        break;
      dnum = 0;
    } // for ii
    FREE(vv);
  }
  if (SpcHourRef > 0) {
    float *vv;
    int ii, ndrop, nu;                                            //-2001-07-02
    int hnum = SpcHourNum;                                        //-2001-07-02
    float c, v;
    vv = ALLOC(2*nv*sizeof(float));                               //-2008-07-30
    for (ii=0; ii<2; ii++) {                                      //-2001-07-02
      sprintf(s, "%s%02d", shour, hnum);                          //-2008-07-28
      fprintf(f, "%-8s", SpcName);                                //-2008-03-10
      fprintf(f, " %s", s);
      for (n=1; n<=nm; n++) {
        ndrop = 0;
        for (i=0; i<nv; i++)  {                                   //-2008-07-30
          vv[2*i] = *(float*)AryPtrX(&clcc, i+1, n);              //-2008-07-30
          if (Scatter)                                            //-2008-07-30
            vv[2*i+1] = *(float*)AryPtrX(&clcs, i+1, n);          //-2008-07-30
          if (vv[2*i] < 0)  ndrop++;                              //-2008-07-30
        }
        nu = nv - ndrop;
        qsort(vv, nv, 2*sizeof(float), compare);                  //-2008-07-30
        if (ii > 0)  hnum = 0;                                    //-2002-08-10
        else if (nu < 0.9*RefHours)  hnum = SpcHourNum*nu/RefHours;
        i = (hnum < nv) ? hnum : nv-1;
        for ( ; i>0; i--)
          if (vv[2*i] >= 0)
            break;    // ???
        c = vv[2*i];
        fprintf(f, YearForm, c);
        if (Scatter) {                                            //-2008-07-30
          v = 100*vv[2*i+1];
          if (c > 0)
            v /= c;
          if (c < 0)
            fprintf(f, " -100%%");
          else if (v > 99.9)
            fprintf(f, "  100%%");
          else
            fprintf(f, " %4.1f%%", v);
        }
      }
      if (ii==0 && hnum<SpcHourNum)  fprintf(f, "  *** %d ***", hnum);
      fprintf(f, "  %s\n", SpcUc);
      nparts++;
      if (hnum == 0)
        break;
      hnum = 0;
    }
    FREE(vv);
  }
  TxtClr(&usr);
  TxtClr(&sys);
  AryFree(&clcc);
  AryFree(&clcs);                                                 //-2008-07-30
  return nparts;                                                  //-2002-08-28
eX_1:
  eMSG(_cant_read_$_, fn);
eX_2: eX_3: eX_4: eX_5: eX_6:
  eMSG(_$_inconsistent_dimensions_, fn);
eX_10:
  eMSG(_cant_read_$_, gn);
eX_11:
  eMSG(_$_incompatible_structure_$_, gn, n);
}

//=========================================================== TmoEvalRatedOdor
//
static int TmoEvalRatedOdor( int ii ) {                           //-2008-09-24
  dP(TmoEvalRatedOdor);
  int i, n, na;
  char syear[16];
  float ofrated, ofsum;
  FILE *f;
  float fa[TIP_ADDODOR];
  float of[TIP_ADDODOR];
  //
  if (_OdorValues[0] == NULL || NumOdor < 2)
    goto clear;
  na = NumOdor - 1;
  f = (MsgFile) ? MsgFile : stdout;
  for (i=0; i<=strlen(CfgYearString); i++)
    syear[i] = toupper(CfgYearString[i]);
  fprintf(f, "%-8s", "ODOR_MOD");
  fprintf(f, " %s00", syear);
  for (i=0; i<na; i++) {
    sscanf(OdorNames[i+1]+5, "%f", &fa[i]);
    fa[i] /= 100;
  }
  for (n=1; n<=NumPoints; n++) {
    ofsum = _OdorValues[0][n-1];
    for (i=0; i<na; i++)
      of[i] = _OdorValues[i+1][n-1];
    ofrated = TutCalcRated(ofsum, na, fa, of);
    fprintf(f, YearForm, ofrated);
    if (Scatter)
      fprintf(f, "  --- ");
  }
  fprintf(f, "  %s\n", SpcUc);
clear:
  for (i=0; i<=TIP_ADDODOR; i++) {
    if (_OdorValues[i] != NULL)
      FREE(_OdorValues[i]);
    _OdorValues[i] = NULL;
  }
  EvalRated = 0;
  NumOdor = 0;
  return 0;
}

//======================================================================= main
//
static void define_component( STTSPCREC rec ) {                 //-2005-08-25
  char format[40], units[40], tp[40];
  char locale[256] = "C";
  int odor;
  strcpy(locale, setlocale(LC_NUMERIC, NULL));      //-2003-07-07
  setlocale(LC_NUMERIC, "C");
  strcpy(SpcName, rec.name);
  SpcConFac = rec.fc;
  SpcConRef = rec.ry;
  odor = (0 == strncmp(rec.name, "odor", 4));
  strcpy(tp, (odor) ? "frq" : "con");                             //-2011-06-09
  if (rec.dy < 0) {
    sprintf(format, "\"%s%%10.3f\"", tp);
    strcpy(YearForm, "  %10.3e");
  }
  else {
    sprintf(format, "\"%s%%5.%df\"", tp, rec.dy);
    sprintf(YearForm, "  %%10.%df", rec.dy);
  }
  strcpy(SpcConForm, format);
  strcpy(SpcUc, rec.uc);
  sprintf(units, "\"%s\"", rec.uc);
  strcpy(SpcConUnit, units);
  SpcDayNum = rec.nd;
  SpcDayRef = rec.rd;
  if (rec.rd <= 0)  rec.nd = -1;
  if (rec.dd < 0) {
    strcpy(format, "\"con%10.3e\"");         //-2003-02-21
    strcpy(DayForm, "  %10.3e");
  }
  else {
    sprintf(format, "\"con%%5.%df\"", rec.dd);
    sprintf(DayForm, "  %%10.%df", rec.dd);
  }
  strcpy(SpcDayForm, format);
  SpcHourNum = rec.nh;
  SpcHourRef = rec.rh;
  if (rec.rh <= 0)  rec.nh = -1;
  if (rec.dh < 0) {
    strcpy(format, "\"con%10.3e\"");         //-2003-02-21
    strcpy(HourForm, "  %10.3e");
  }
  else {
    sprintf(format, "\"con%%5.%df\"", rec.dh);
    sprintf(HourForm, "  %%10.%df", rec.dh);
  }
  strcpy(SpcHourForm, format);
//  SpcDepFac = rec.fn;
//  SpcDepRef = rec.rn;
//  if (rec.dn < 0) {
//    strcpy(format, "\"dep%10.3e\"");         //-2003-02-21
//  }
//  else {
//    sprintf(format, "\"dep%%5.%df\"", rec.dn);
//  }
//  strcpy(SpcDepForm, format);
  sprintf(units, "\"%s\"", rec.un);
  strcpy(SpcDepUnit, units);
  if (CHECK) vMsg(
    "MON Name=%s, fc=%10.3e, ry=%10.3e, mc=%s, uc=%s, nd=%d, rd=%10.3e, md=%s, "
    "fn=%10.3e, rn=%10.3e, mn=%s, un=%s\n",
    SpcName, SpcConFac, SpcConRef, SpcConForm, SpcConUnit, SpcDayNum,
    SpcDayRef, SpcDayForm, SpcDepFac, SpcDepRef, SpcDepForm, SpcDepUnit);
  setlocale(LC_NUMERIC, locale);
}

int TmoMain( char *s )
  {
  dP(TmoMain);
  int i, n, l;
  char *pc, fn[256];
  if (CHECK) vMsg("TmoMain(%s)", s);
  if (*s) {
    pc = s;
    if (*pc == '-')
      switch (pc[1]) {
        case '0': Nv=0; Nc=0; T0=0; Nm=0; Intervall=0;
                  strcpy(MntName, "work/mnt%c%04ld");
                  strcpy(YearForm, " %10.3e");
                  strcpy(DayForm, " %10.3e");
                  strcpy(HourForm, " %10.3e");
                  *SpcName = 0;
                  SpcSequ = -1;
                  EvalRated = 0;                                 //-2008-09-24
                  AddMeasured = 0;
                  Scatter = 1;
                  break;
        case 'A': strcpy(locl, pc+2);                             //-2003-07-07
                  break;
        case 'e': SpcSequ = 0;
                  sscanf(pc+2, "%d", &SpcSequ);
                  break;
        case 'g': AddMeasured = 1;
                  break;
        case 'n': sscanf(pc+2, "%d", &Nm);
                  break;
        case 'q': MsgQuiet = 1;
                  sscanf(pc+2, "%d", &MsgQuiet);
                  break;
        case 'r': EvalRated = 1;                                  //-2008-09-24
                  break;
        case 's': sscanf(pc+2, "%d", &i);
                  define_component(SttSpcTab[i]);                 //-2005-08-25
                  break;
        default:  vMsg("TalMonitor: Unknown option \"%s\"", pc);
                  break;
      }
    else {
      strcpy(Path, pc);
      l = strlen(Path);
      for (i=0; i<l; i++)  if (Path[i] == '\\')  Path[i] = '/';
      if ((l) && (Path[l-1]!='/') && (Path[l-1]!=':')) {
        Path[l] = '/';
        Path[l+1] = 0;
      }
    }
    return 0;
  }
  if (SpcSequ >= 0) {
    if (EvalRated)
      n = TmoEvalRatedOdor(SpcSequ);
    else
      n = TmoEvalSeries(SpcSequ);
    return n;
  }
  if (Nm < 1)
    return 0;
  if (SpcConRef <= 0)
    return 0;
  MntX = ALLOC(Nm*sizeof(float));
  MntY = ALLOC(Nm*sizeof(float));
  MntZ = ALLOC(Nm*sizeof(float));
  MntI = ALLOC(Nm*sizeof(int));
  MntJ = ALLOC(Nm*sizeof(int));
  MntK = ALLOC(Nm*sizeof(int));
  GrdL = ALLOC(Nm*sizeof(int));
  GrdI = ALLOC(Nm*sizeof(int));
  n = 0;
  for (i=1; i<=Nm; i++) {
    ReadMntValues(i);                                            eG(1);
    n = AddValues(i);                                            eG(3);
    if (n)
      break;
    AryFree(&MntCAry);
    }
  if (n)  vMsg(_no_concentration_$_, SpcName);
  else {                                                        //-2002-07-07
    vMsg(_time_series_$_, SpcName);
    sprintf(fn, "%s-%s", SpcName, CfgMntAddString);             //-2006-10-31
    WriteSpcValues(fn, 0);                                       eG(4);
    vMsg(_$$_written_, Path, fn);
    if (Scatter) {
      sprintf(fn, "%s-%s", SpcName, CfgMntSctString);             //-2006-10-31
      WriteSpcValues(fn, 1);                                     eG(5);
      vMsg(_$$_written_, Path, fn);
    }
  }
  TxtClr(&MntNames);
  TxtClr(&UsrHeader);
  TxtClr(&SysHeader);
  AryFree(&SpcCAry);
  for (i=0; ;i++) {
    if (!Names[i])
      break;
    FREE(Names[i]);
  }
  FREE(Names);
  FREE(MntX);
  FREE(MntY);
  FREE(MntZ);
  FREE(MntI);
  FREE(MntJ);
  FREE(MntK);
  FREE(GrdL);
  FREE(GrdI);
  return 0;
eX_1: eX_3:
  eMSG(_error_calculation_);
eX_4: eX_5:
  eMSG(_error_writing_);
  }

//=============================================================================
//
// history:
//
// 2002-06-21 lj 0.13.0  final test version
// 2002-07-04 lj 0.13.1  rounding for values of seconds
// 2002-07-07 lj 0.13.2  message, if no concentration available
// 2002-08-28 lj 0.13.3  return of nparts in TmoEvalSeries()
// 2002-09-24 lj 1.0.0   final release candidate
// 2003-02-21 lj 1.1.1   optional scientific notation
// 2004-06-10 lj 2.0.0   output format for odor
// 2005-03-17 uj 2.2.0  version number upgrade
// 2006-10-26 lj 2.3.0  external strings
// 2006-10-31 lj 2.3.1  configuration data
// 2007-02-03 uj 2.3.5  rounding of seconds corrected
// 2008-03-10 lj 2.4.0  evaluation of rated odor frequencies
// 2008-04-17 lj 2.4.1  merged with 2.3.x
// 2008-07-22 lj 2.4.2  parameter "cset" in dmna-header
// 2008-07-28 lj        use of configuration strings
// 2008-07-30 lj 2.4.3  evaluating scatter
// 2008-09-24 lj        update for rated odor frequencies
// 2008-10-17 lj        uses MsgSetLocale()
// 2008-12-04 lj 2.4.5  header: modified t1, t2, artp; renamed dt, rdat
// 2011-06-09 uj 2.4.10 DMN header entries adjusted
//                      TmoEvalSeries: use absolute dev for odorants
// 2011-06-18 uj        SpcName length increased to 16
// 2011-07-07 uj 2.5.0  version number upgrade
// 2011-11-23 lj 2.6.0  settings
//
//=============================================================================

