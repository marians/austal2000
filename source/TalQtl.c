// ======================================================== TalQtl.c
//
// Calculation of percentiles for AUSTAL2000
// =========================================
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
// last change:  2011-11-28 lj
//
// ==================================================================

static char *TalQtlVersion = "2.6.0";
static char *eMODn = "TalQtl";
static int CHECK = 0;

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <math.h>
#include <locale.h>

#include "IBJmsg.h"
#include "IBJtxt.h"
#include "IBJary.h"
#include "IBJdmn.h"
#include "IBJstamp.h"
#include "TalDtb.h"
#include "TalInp.h"                                               //-2008-07-22
#include "TalUtl.h"
#include "TalQtl.h"
#include "TalQtl.nls"
#include "TalCfg.h"
#include "TalStt.h"

static float Qtl;
static int NumX, NumMax;
static char DtbName[256], QtlName[256], InName[256], OutName[256];
static char Kennung[256] = "unknown";                             //-2006-08-22
static char GrdName[32] = "";
static ARYDSC QtlDsc, DtbDsc, DevDsc;
static char Name[40], Unit[40], Artp[40], Refv[40], Form[40];
static int Nx, Ny, Nc, K1, K2, Nf;
static char StdPath[256];
static float Factor=-1, RefValue=-1;
static int clcMaximum, clcFrequency, clcQuantile, isHistogram, useScatter;

static int SpcDayNum, SpcHourNum;
static char SpcName[16], OldName[16];                             //-2011-06-18
static char SpcConForm[32], SpcDayForm[32], SpcDepForm[32], SpcHourForm[32];
static char SpcConUnit[32], SpcDepUnit[32];
static float SpcConFac, SpcDepFac;
static float SpcConRef, SpcDayRef, SpcDepRef, SpcHourRef;
static char CmpName[80];
static float RefHours=365*24;
static char locale[256]="C", locl[256]="";

static float QtlVal( DTBREC* pd, int nf, float q )
  {
  float sum, qsum, psum;
  int i, k;
  float w, wmin;
  double a, arg;
  sum = pd->frqnull + pd->frqsub;
  for (i=0; i<nf; i++)  sum += pd->frq[i];
  qsum = q*sum;
  if (qsum <= pd->frqnull)  return 0.0;
  if (qsum < pd->frqnull + pd->frqsub) {
    wmin = pd->valmax*pow(10.0,-nf/DTB_DIVISION);
    return -wmin;
  }
  qsum = sum - qsum;
  psum = 0;
  for (k=nf-1; k>=0; k--) {
    psum += pd->frq[k];
    if (psum >= qsum)  break;
  }
  if (k < 0) {
    wmin = pd->valmax*pow(10.0,-nf/DTB_DIVISION);
    return wmin;
  }
  a = (psum - qsum)/pd->frq[k];
  if (k == nf-1)
    arg =(a - 1)/DTB_DIVISION + a*log10(pd->realmax/pd->valmax);
  else  arg = (k + a - nf)/DTB_DIVISION;
  w = pd->valmax*pow(10.0, arg);
  return w;
}

static float FrqVal( DTBREC* pd, int nf, float w )
  {
  dP(FrqVal);
  float sum, psum;
  int i, k;
  float wmin, wmax, kw, dd;
  dd = DTB_DIVISION;
  wmax = pd->realmax;
  if (w >= wmax)  return 0.0;
  if (w < 0.0)  return 1.0;
  sum = pd->frqnull + pd->frqsub;
  for (i=0; i<nf; i++)  sum += pd->frq[i];
  wmin = pd->valmax*pow(10.0, -nf/dd);
  if (w <= wmin)  return (sum - pd->frqnull - (w/wmin)*pd->frqsub)/sum;
  kw = nf + dd*log10(w/pd->valmax);
  if (kw >= nf)  return 0;
  k = floor(kw);  if (k < 0)                    eX(1);
  psum = pd->frqnull + pd->frqsub;
  for (i=0; i<k; i++)  psum += pd->frq[i];
  if (k >= nf-1)
    psum += ((dd*log10(w/pd->valmax)+1)/(dd*log10(wmax/pd->valmax)+1))*pd->frq[k];
  else  psum += (kw - k)*pd->frq[k];
  return (sum - psum)/sum;
eX_1:
  eMSG("Index k=%d!", k);
  }

//========================================================================= Help
//
static void Help( void )
  {
  vMsg(_help_1_$$_, TalQtlVersion, IBJstamp(__DATE__, __TIME__));
  vMsg(_help_2_);
  vMsg(_help_3_);
  vMsg(_help_4_);
  vMsg(_help_5_);
  vMsg(_help_6_);
  vMsg(_help_7_);
  vMsg(_help_8_);
  vMsg(_help_9_);
  vMsg(_help_10_);
  vMsg(_help_11_);
  vMsg(_help_12_);
  MsgVerbose = 0;
  exit(0);
  }

//====================================================================== StdArg
static int StdArg( char *s ) {
  char *pc;
  if (!s || !*s)  return 0;
  if (*s == '-') {
    return 0;
  }
  else {
    strcpy(StdPath, s);
    for (pc=StdPath; (*pc); pc++)  if (*pc == '\\')  *pc = '/';
    if (*StdPath && pc[-1]=='/')  pc[-1] = 0;
   }
 return 1;
}

//===================================================================== TqlMain
static void define_component( STTSPCREC rec ) {                 //-2005-08-25
  char format[40], units[40];
  char locale[256] = "C";
  strcpy(locale, setlocale(LC_NUMERIC, NULL));      //-2003-07-07
  setlocale(LC_NUMERIC, "C");
  strcpy(SpcName, rec.name);
  MsgLow(SpcName);
  SpcConFac = rec.fc;
  SpcConRef = rec.ry;
  if (rec.dy < 0)  strcpy(format, "\"con%10.3e\"");         //-2003-02-21
  else             sprintf(format, "\"con%%5.%df\"", rec.dy);
  strcpy(SpcConForm, format);
  sprintf(units, "\"%s\"", rec.uc);
  strcpy(SpcConUnit, units);
  SpcDayNum = rec.nd;
  SpcDayRef = rec.rd;
  if (rec.rd <= 0)  rec.nd = -1;
  if (rec.dd < 0)  strcpy(format, "\"con%10.3e\"");         //-2003-02-21
  else             sprintf(format, "\"con%%5.%df\"", rec.dd);
  strcpy(SpcDayForm, format);
  SpcHourNum = rec.nh;
  SpcHourRef = rec.rh;
  if (rec.rh <= 0)  rec.nh = -1;
  if (rec.dh < 0)  strcpy(format, "\"con%10.3e\"");         //-2003-02-21
  else             sprintf(format, "\"con%%5.%df\"", rec.dh);
  strcpy(SpcHourForm, format);
  sprintf(units, "\"%s\"", rec.un);
  strcpy(SpcDepUnit, units);
  if (CHECK) vMsg(
    "Name=%s, fc=%10.3e, ry=%10.3e, mc=%s, uc=%s, nd=%d, rd=%10.3e, md=%s, "
    "fn=%10.3e, rn=%10.3e, mn=%s, un=%s,"
    "rh=%10.3e, dh=%d, mh=%s\n",
    SpcName, SpcConFac, SpcConRef, SpcConForm, SpcConUnit, SpcDayNum,
    SpcDayRef, SpcDayForm, SpcDepFac, SpcDepRef, SpcDepForm, SpcDepUnit,
    SpcHourRef, rec.dh, SpcHourForm);
  setlocale(LC_NUMERIC, locale);
}

int TqlMain(
char *ss )
  {
  dP(TqlMain);
  int i, j, k, l, n, valid_hours;
  char *pc, *_arrtype, grdx[8], fn[256], **_spc, s[80], *_kennung, name[40];
  char *_t1=NULL, *_t2=NULL, src, dst;
  TXTSTR usrhdr = { NULL, 0 };
  TXTSTR syshdr = { NULL, 0 };
  float *pqtl, *pdev, valid;
  DTBREC *pdtb;
  DTBMAXREC *p2dtb;                                                 //-2001-10-21
  if (CHECK) vMsg("TqlMain(%s)", ss);
  if (StdArg(ss))  return 0;
  if (*ss) {
      switch(ss[1]) {
        case '0':  *InName = 0;
                   *OutName = 0;
                   *SpcName = 0;
                   *GrdName = 0;
                   *OldName = 0;
                   *Form = 0;
                   *Name = 0;
                   *Kennung = 0;
                   *Artp = 0;
                   *Refv = 0;
                   *Unit = 0;
                   Factor = -1;
                   RefValue = -1;
                   clcFrequency = 0;
                   clcQuantile = 0;
                   clcMaximum = 0;
                   useScatter = 0;
                   break;
        case 'A':  strcpy(locl, ss+2);
                   break;
        case 'g':  strcpy(GrdName, ss+2);
                   break;
        case 'i':  strcpy(InName, ss+2);
                   break;
        case 'o':  strcpy(OutName, ss+2);
                   break;
        case 'f':  clcFrequency = 1;
                   RefValue = -1;
                   sscanf(ss+2, "%f", &RefValue);
                   break;
        case 'F':  strcpy(Form, TxtQuote(ss+2));
                   break;
        case 'h':  Help();
                   break;
        case 'm':  clcMaximum = 1;
                   NumX = -1;
                   sscanf(ss+2, "%d", &NumX);
                   break;
        case 'n':  strcpy(Name, ss+2);
                   break;
        case 'K':  strcpy(Kennung, ss+2);
                   break;
        case 'p':  clcQuantile = 1;
                   Qtl = -1;
                   sscanf(ss+2, "%f", &Qtl);
                   break;
        case 'r':  strcpy(Refv, ss+2);
                   break;
        case 's':  sscanf(ss+2, "%d", &i);
                   define_component(SttSpcTab[i]);                //-2011-11-28
                   break;
        case 'u':  strcpy(Unit, TxtQuote(ss+2));
                   break;
        case 'x':  sscanf(ss+2, "%f", &Factor);
                   break;
        default:   ;
        }
    return 0;
    }
  if (!*OldName) {
    vMsg(_calculation_$_, SpcName);
    strcpy(OldName, SpcName);
  }
  if (!clcFrequency && !clcQuantile && !clcMaximum)               eX(30);
  if (!*SpcName)                                                  eX(31);
  if (Factor < 0)   Factor = SpcConFac;
  if (Factor <= 0)  Factor = 1;
  strcpy(DtbName, (*InName) ? InName : "dtba000");
  pc = strrchr(DtbName, '.');
  if (pc && !strcmp(pc, ".dmna"))  *pc = 0;
  strcpy(grdx, GrdName);                                                //-2001-10-30
  strcpy(QtlName, OutName);
  strcpy(s, (*Name) ? Name : SpcName);
  if (clcFrequency) {
    if (RefValue < 0)  RefValue = SpcHourRef;
    sprintf(Artp, "F%3.3f ", RefValue);
    if (!*QtlName)  sprintf(QtlName, "%s-frq%s", s, grdx);
  }
  else if (clcQuantile) {
    if (Qtl < 0) {
      int nh = SpcHourNum;
      if (Qtl == -2) {          //-2001-07-06 calculate maximum value
        Qtl = 100;
        nh = 0;
      }
      else  Qtl = 100*(1-SpcHourNum/RefHours);
      sprintf(Artp, "Q%1.4f", Qtl);                             //-2005-03-15
      if (!*QtlName)
        sprintf(QtlName, "%s-%s%02d%s%s", s, CfgHourString, nh, CfgAddString,
            grdx);                                              //-2008-07-30
    }
    else {
      sprintf(Artp, "Q%1.4f", Qtl);
      if (!*QtlName)
        sprintf(QtlName, "%s-q%05d%s%s", s, (int)(Qtl*100+0.5), CfgAddString,
            grdx);                                              //-2008-07-30
    }
  }
  else if (clcMaximum) {
    if (NumX < 0)  NumX = SpcHourNum;
    sprintf(Artp, "CH%02d", NumX);                              //-2002-09-01
    if (!*QtlName)
      sprintf(QtlName, "%s-%s%02d%s%s", s, CfgHourString, NumX, CfgAddString,
          grdx);                                                //-2008-07-30
  }
  if (*DtbName == '/') strcpy(fn, DtbName);
  else sprintf(fn, "%s/%s", StdPath, DtbName);
  DmnRead(fn, &usrhdr, &syshdr, &DtbDsc);                               eG(1);
  TxtClr(&syshdr);
  DmnGetString(usrhdr.s, "idnt", &_kennung, 1);                 //-2011-06-29
  if (_kennung) {
    strcpy(Kennung, _kennung);
    FREE(_kennung);
  }
  valid = 1;
  DmnGetFloat(usrhdr.s, "valid", "%f", &valid, 1);                      eG(15);
  if (valid < 0)  valid = 0;
  if (valid > 1)  valid = 1;
  DmnGetString(usrhdr.s, "T1", &_t1, 1);
  DmnGetString(usrhdr.s, "T2", &_t2, 1);
  if ((_t1) && (_t2)) {                                                 //-2002-08-12
    double t1, t2;
    t1 = MsgDateVal(_t1);
    t2 = MsgDateVal(_t2);
    valid_hours = 0.5 + MsgDateSeconds(t1, t2)/3600;
  }
  else  valid_hours = RefHours;
  valid_hours = 0.5 + valid*valid_hours;                        //-2002-08-12
  if (valid_hours < 0.9*RefHours && NumX != 0) {  //-2001-06-29 //-2005-03-15
    NumX = NumX*valid_hours/RefHours;
    sprintf(Artp, "CH%02d", NumX);                              //-2005-03-15
    vMsg(_exceedings_changed_$_, NumX);
  }
  DmnRplName(&usrhdr, "arrtype", "artp");
  DmnGetString(usrhdr.s, "artp", &_arrtype, 1);
  if (!strcmp(_arrtype, "HN")) {
    isHistogram = 1;
    if (clcMaximum)                                                     eX(13);
    Nf = 0;
    DmnGetInt(usrhdr.s, "DTBNUMVAL", "%d", &Nf, 1);                     eG(10);
    if (DtbDsc.elmsz != sizeof(DTBREC))                                 eX(11);
  }
  else if (!strcmp(_arrtype, "M")) {
    isHistogram = 0;
    if (!clcMaximum)                                                    eX(14);
    DmnGetInt(usrhdr.s, "DTBNUMMAX", "%d", &NumMax, 1);                 eG(18);
    useScatter = 0;
    if (NumMax <= DtbDsc.elmsz/(2*sizeof(float)))  useScatter = 1;      //-2001-10-21
    else if (NumMax != DtbDsc.elmsz/sizeof(float))                      eX(16);
    if (NumX<0 || NumX>=NumMax)                                         eX(21);
  }
  if (_arrtype) FREE(_arrtype);
  if (_t1)  FREE(_t1);
  if (_t2)  FREE(_t2);
  if (Nf <= 0)  Nf = DTB_NUMVAL;
  if (DtbDsc.numdm != 4)                                                eX(2);
  Nx = DtbDsc.bound[0].hgh;
  Ny = DtbDsc.bound[1].hgh;
  K1 = DtbDsc.bound[2].low;
  K2 = DtbDsc.bound[2].hgh;
  Nc = DtbDsc.bound[3].hgh + 1;
  if (K1 < 1)  K1 = 1;
  _spc = ALLOC(Nc*sizeof(char*));
  n = DmnGetString(usrhdr.s, "name", _spc, Nc);  if (n != Nc)           eX(22);
  if (*Name)  strcpy(CmpName, Name);
  else {
    strcpy(CmpName, "gas.");
    strcat(CmpName, SpcName);
  }
  for (l=0; l<Nc; l++)  if (!strcmp(_spc[l], CmpName))  break;
  if (l >= Nc)                                                          eX(23);
  for (i=0; i<n; i++) {                                          //-2006-02-15
    FREE(_spc[i]);
  }
  FREE(_spc);
  AryCreate(&QtlDsc, sizeof(float), 3, 1, Nx, 1, Ny, K1, K2);           eG(3);
  if (useScatter) {
    AryCreate(&DevDsc, sizeof(float), 3, 1, Nx, 1, Ny, K1, K2);         eG(3);
  }
  for (i=1; i<=Nx; i++) {
    for (j=1; j<=Ny; j++) {
      for (k=K1; k<=K2; k++) {
        pqtl = (float*) AryPtr(&QtlDsc, i, j, k);  if (!pqtl)           eX(5);
        if (isHistogram) {
          pdtb = (DTBREC*) AryPtr(&DtbDsc, i, j, k, l);  if (!pdtb)     eX(4);
          if (clcFrequency) {
            *pqtl = FrqVal(pdtb, Nf, RefValue/Factor);                  eG(12);
          }
          else {
            if (Qtl >= 100)  *pqtl = Factor*pdtb->realmax;              //-2001-07-31
            else {
              *pqtl = Factor*QtlVal(pdtb, Nf, 0.01*Qtl);                eG(6);
            }
          }
        }
        else {
          p2dtb = AryPtr(&DtbDsc, i, j, k, l);  if (!p2dtb)             eX(4);
          *pqtl = Factor*p2dtb->max[NumX];
          if (useScatter) {
            pdev = AryPtr(&DevDsc, i, j, k, l);  if (!pdev)             eX(8);
            *pdev = p2dtb->dev[NumX];
          }
        }
      } // for k
    }
  }
  //
  sprintf(s, "\ncset  \"%s\"", TI.cset);                          //-2008-07-22 //-2014-01-28
  TxtPIns(&usrhdr, 0, s);                                         //-2008-07-22
  if (*locl) {
    char s[256];
    sprintf(s, "\"%s\"", locl);
    DmnRplValues(&usrhdr, "locl", s);
    strcpy(locale, setlocale(LC_NUMERIC, NULL));
    MsgSetLocale(locl);                                           //-2008-10-17
  }
  else DmnRplValues(&usrhdr, "locl", NULL);
  if (!strcmp(locl, "german"))  { src = '.';  dst = ','; }
  else                          { src = ',';  dst = '.'; }
  DmnRplChar(&usrhdr, "xmin", src, dst);
  DmnRplChar(&usrhdr, "ymin", src, dst);
  DmnRplChar(&usrhdr, "valid", src, dst);
  DmnRplChar(&usrhdr, "sk|zk", src, dst);
  DmnRplChar(&usrhdr, "dd|delta", src, dst);
  //
  DmnRplName(&usrhdr, "FORMAT", "form");
  DmnRplName(&usrhdr, "KENNUNG", "idnt");
  DmnRplName(&usrhdr, "einheit", NULL);
  DmnRplName(&usrhdr, "valdef", NULL);                            //-2001-07-11
  DmnRplName(&usrhdr, "refc", NULL);
  DmnRplName(&usrhdr, "refd", NULL);
  strcpy(name, SpcName);
  for (pc=name; (*pc); pc++)  *pc = toupper(*pc);
  DmnRplValues(&usrhdr, "name", TxtQuote(name));
  DmnRplValues(&usrhdr, "unit", (*Unit) ? Unit : SpcConUnit);
  if (clcFrequency) {
    DmnRplValues(&usrhdr, "form", TxtQuote((*Form) ? Form : "%(*1000)5.0f"));
    sprintf(s, "%10.3e", RefValue);
    DmnRplValues(&usrhdr, "value", s);
  }
  else {
    if (!*Refv)  sprintf(Refv, "%10.3e", SpcHourRef);
    DmnRplValues(&usrhdr, "form", TxtQuote((*Form) ? Form : SpcHourForm));
    DmnRplValues(&usrhdr, "refv", Refv);
    sprintf(s, "%d", NumX);
    if (clcMaximum) DmnRplValues(&usrhdr, "exceed", s);         //-2005-03-15
  }
  DmnRplValues(&usrhdr, "artp", TxtQuote(Artp));
  DmnRplValues(&usrhdr, "idnt", TxtQuote(Kennung));
  DmnRplValues(&usrhdr, "sequ", "\"k+,j-,i+\"");
  DmnRplValues(&usrhdr, "vldf", "\"V\"");
  DmnRplValues(&usrhdr, "file", TxtQuote(QtlName));
  if (*QtlName == '/') strcpy(fn, QtlName);
  else sprintf(fn, "%s/%s", StdPath, QtlName);
  DmnWrite(fn, usrhdr.s, &QtlDsc);                                  eG(7);
  vMsg(_file_$_written_, fn);
  if (useScatter) {
    int l = strlen(QtlName) - strlen(grdx);
    if (l > 0)  QtlName[l-1] = 's';
    if (*QtlName == '/') strcpy(fn, QtlName);
    else sprintf(fn, "%s/%s", StdPath, QtlName);
    DmnRplValues(&usrhdr, "file", TxtQuote(QtlName));
    DmnRplValues(&usrhdr, "artp", TxtQuote("E"));
    DmnRplValues(&usrhdr, "form", TxtQuote("dev%(*100)5.1f"));
    DmnRplValues(&usrhdr, "refv", "0.30");
    DmnRplValues(&usrhdr, "unit", NULL);                            //-2002-12-02
    DmnWrite(fn, usrhdr.s, &DevDsc);                                eG(17);
    vMsg(_file_$_written_, fn);
  }
  TxtClr(&usrhdr);
  AryFree(&QtlDsc);
  AryFree(&DevDsc);
  AryFree(&DtbDsc);
  if (*locl)  setlocale(LC_NUMERIC, locale);
  return 0;
eX_2: eX_11:
  eMSG(_$_no_distribution_, DtbName);
eX_1:  eX_3:  eX_10: eX_15:
  eMSG(_internal_error_);
eX_4:  eX_5:  eX_6:  eX_8:  eX_12:
  eMSG(_internal_error_at_$$$$_, i, j, k, l);
eX_7:
  eMSG(_cant_write_$_, QtlName);
eX_13:
  eMSG(_cant_get_from_histogram_);
eX_14: eX_16: eX_17: eX_18:
  eMSG(_cant_get_from_maximum_);
eX_21:
  eMSG(_only_exceedings_$_, NumMax-1);
eX_22:
  eMSG(_no_name_);
eX_23:
  eMSG(_$_not_found_, CmpName);
eX_30: eX_31:
  eMSG(_no_target_);
}

//=============================================================================
//
//  history:
//
// 2002-06-21 lj 0.13.0 final test version
// 2002-08-12 lj 0.13.1 adjustment of exceedence revised
// 2002-09-01 lj 0.13.2 value of "artp" changed
// 2002-09-24 lj 1.0.0  final release candidate
// 2002-12-02 lj 1.0.3  parameter "unit" in scatter file removed
// 2003-02-21 lj 1.1.1  optional scientific notation
// 2003-07-07 lj 1.1.8  localization
// 2005-03-15 uj 2.1.1  artp adjusted
// 2005-03-17 uj 2.2.0  version number upgrade
// 2006-02-15 lj 2.2.9  freeing of allocated strings
// 2006-08-22 lj 2.2.12 string length for "Kennung" increased
// 2006-10-18 lj 2.3.0  externalization of strings
// 2008-07-22 lj 2.4.2  parameter "cset" in dmna-header
// 2008-10-17 lj 2.4.3  uses MsgSetLocale()
// 2011-06-18 uj 2.4.10 SpcName length increased to 16
// 2011-07-07 uj 2.5.0  version number upgrade
// 2011-11-28 lj 2.6.0  define_component() revised
//
//=============================================================================

