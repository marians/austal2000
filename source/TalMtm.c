// ======================================================== TalMtm.c
//
// Calculation of daily averages for AUSTAL2000
// ============================================
//
// Copyright (C) Umweltbundesamt, Dessau-Roﬂlau, Germany, 2002-2007
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
// last change:  2014-06-26
//
//========================================================================
//
//  Berechnung des maximalen Tagesmittels aus einzelnen Tagesmittelwerten
//  Nur Tagesmittel aus mindestens 12 Stundenmitteln werden verwendet
//
//=========================================================================

static char *TalMtmVersion = "2.6.11";
static char *eMODn = "TalMtm";
static int CHECK = 0;

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <locale.h>

#include "genutl.h"                                               //-2008-12-11
#include "IBJmsg.h"
#include "IBJary.h"
#include "IBJtxt.h"
#include "IBJdmn.h"
#include "TalUtl.h"
#include "TalStt.h"
#include "TalInp.h"
#include "TalCfg.h"    //-2006-10-31
#include "TalMtm.h"
#include "TalMtm.nls"

static ARYDSC CurDos, VecDos, VecDay, MaxCon, MaxDev, MaxDay, CncDsc, DevDsc;
static ARYDSC SumDos, SumCon, SumDev, SumDep, SumDps, SupCon, SupDev, SupDay;
static ARYDSC DryDep, DryDps, WetDep, WetDps, TtlDep, TtlDps;
static long Folge=1;
static long Anzahl, Dt, Ia, ElmSz, Nsum, Nval;
static char Eingabe[256];
static char DosName[80] = "c%04da00";                             //-2011-12-14
static char GrdName[80] = "";
static char Path[256];
static int Nx, Ny, Nz, Nc, Nh, Ns, Ng;
static int Maxima, Concentration, AverageOnly;
static int Deposition, DryDeposition, WetDeposition;              //-2011-12-14
static int Scatter=0, WriteHeader=1, Kref=-999, Kmax=-999;
static TXTSTR UsrHeader = { NULL, 0 };
static TXTSTR SysHeader = { NULL, 0 };
static int CmpIndex[8], CmpIsGas[8];
static int CmpDayNum, CmpHourNum;
static char CmpName[16], CmpFamily[16], CmpGroups[16];
static char CmpConForm[32], CmpDayForm[32], CmpHourForm[32];
static char CmpDepForm[32], CmpDryForm[32], CmpWetForm[32];       //-2011-12-14
static char CmpConUnit[32], CmpDepUnit[32];
static float CmpConFac, CmpDepFac, CmpWetFac;
static float CmpConRef, CmpDayRef, CmpDepRef, CmpHourRef;
static char **Names;
static float Sk[201], Dd;                                         //-2008-08-04
static double T1, T2;
static float SumValid, MinValid=0.5, RefDays=365;
static char locl[256]="";
static char worker[256];                                          //-2008-08-15

//================================================================= WriteDmna
//
static int WriteDmna( int day, float valid ) {                    //-2002-08-09
  dP(WriteDmna);
  int i, j, k, k1, k2, l, ii, odor, gas;                     //-2008-10-07
  float *pcurdos, *pcnc, *pdev;
  float dev, cnc, var, c, d;                                      //-2008-10-07
  char *pc, name[40], fname[256], gname[256], srefv[40], t[80];
  char src, dst, ot[40], of[40];
  char locale[256];
  float bs = SttOdorThreshold;
  //
  strcpy(locale, setlocale(LC_NUMERIC, NULL));                    //-2008-10-17
  if (CurDos.numdm != 4)                                            eX(10);
  k2 = CurDos.bound[2].hgh;
  if (Kref >= 0) {
    if (Kref > k2)                                                   eX(1);
    k1 = Kref;
    k2 = Kref;
  }
  else if (Kmax > 0) {
    if (Kmax > k2)                                                   eX(1);
    k1 = 1;
    k2 = Kmax;
  }
  else return 0;
  //
  strcpy(name, CmpFamily);
  odor = (0 == strncmp(name, "odor", 4));                         //-2011-06-09
  if (odor) {
    sprintf(ot, "\"FHP%4.2f\"", bs);
    sprintf(of, "\"frq%s", CmpConForm+4);
  }
  //
  if (k2 > 0) {
    AryCreate(&CncDsc, sizeof(float), 3, 1,Nx, 1,Ny, k1,k2);         eG(11);
    if (Scatter) {
      AryCreate(&DevDsc, sizeof(float), 3, 1,Nx, 1,Ny, k1,k2);       eG(12);
    }
  }
  else {
    AryCreate(&CncDsc, sizeof(float), 2, 1,Nx, 1,Ny);                eG(13);
    if (Scatter) {
      AryCreate(&DevDsc, sizeof(float), 2, 1,Nx, 1,Ny);              eG(14);
    }
  }
  for (k=k1; k<=k2; k++) {
    for (i=1; i<=Nx; i++) {
      for (j=1; j<=Ny; j++) {
        l = CmpIndex[0];
        if (l < 0)  continue;
        if (valid == 0) {                                         //-2008-08-04
          cnc = -1;
          dev = -1;
          var = -1;                                               //-2008-10-07
        }
        else {
          cnc = 0;
          dev = 0;
          var = 0;                                                //-2008-10-07
          for (ii=0; ii<8; ii++) {
            l = CmpIndex[ii];
            gas = CmpIsGas[ii];                                   //-2008-10-07
            if (l < 0)
              break;
            if (k>0 && !gas)
              continue;
            pcurdos = AryPtr(&CurDos, i, j, k, l);  if (!pcurdos)     eX(15);
            c = pcurdos[0];                                       //-2008-10-07
            cnc += c;
            if (Scatter) {
              d = c*pcurdos[1];
              if (gas)
                dev += d;                                         //-2008-10-07
              else                                                //-2008-10-07
                var += d*d;                                       //-2008-10-07
            }
            if (cnc < 0) {                                        //-2008-08-04
              dev = -1;
              var = -1;                                           //-2008-08-04
              break;
            }
          } // for ii
          if (var > 0)                                            //-2008-10-07
            dev = sqrt(var + dev*dev);                            //-2008-10-07
        }
        pcnc = AryPtr(&CncDsc, i, j, k);  if (!pcnc)                eX(16);
        *pcnc = cnc*CmpConFac;
        if (Scatter) {
          if (cnc > 0)  {                                         //-2008-08-04
            if (odor)
              dev *= CmpConFac;
            else
              dev /= cnc;
          }
          pdev = AryPtr(&DevDsc, i, j, k);  if (!pdev)              eX(17);
          *pdev = dev;                                            //-2008-08-04
        }
      } // for j
    } // for i
  } // for k
  DmnRplValues(&UsrHeader, "cset", NULL);                         //-2014-06-26
  sprintf(t, "\n%s  \"%s\"", "cset", TI.cset);                    //-2014-06-26
  TxtPIns(&UsrHeader, 0, t);                                      //-2014-06-26 
  sprintf(t, "\"%s\"", name);
  for (pc=t; (*pc); pc++)  *pc = toupper(*pc);
  DmnRplValues(&UsrHeader, "name", t);
  DmnRplValues(&UsrHeader, "vldf", "\"V\"");
  DmnRplValues(&UsrHeader, "axes", "\"xyz\"");
  DmnRplValues(&UsrHeader, "sequ", "k+,j-,i+");
  DmnRplValues(&UsrHeader, "refc", NULL);
  DmnRplValues(&UsrHeader, "refd", NULL);
  DmnRplValues(&UsrHeader, "unit", NULL);
  //
  if (*locl) {
    char s[256];
    sprintf(s, "\"%s\"", locl);
    DmnRplValues(&UsrHeader, "locl", s);
    MsgSetLocale(locl);                                           //-2008-10-17
  }
  else {
    DmnRplValues(&UsrHeader, "locl", "C");
    setlocale(LC_NUMERIC, "C");                                   //-2008-10-17
  }
  if (!strcmp(locl, "german"))  { src = '.';  dst = ','; }
  else                          { src = ',';  dst = '.'; }
  DmnRplChar(&UsrHeader, "xmin", src, dst);
  DmnRplChar(&UsrHeader, "ymin", src, dst);
  DmnRplChar(&UsrHeader, "valid", src, dst);
  DmnRplChar(&UsrHeader, "sk|zk", src, dst);
  DmnRplChar(&UsrHeader, "dd|delta", src, dst);
  //
  if (k2 > 0) {
    sprintf(gname, "%s-%03d%s%s", name, day, CfgAddString, GrdName); //-2006-10-31
    DmnRplValues(&UsrHeader, "file", TxtQuote(gname));
    sprintf(fname, "%s%s-%03d%s%s", Path, name, day, CfgAddString, GrdName); //-2006-10-31
    DmnRplValues(&UsrHeader, "form", (odor) ? of : CmpConForm);
    DmnRplValues(&UsrHeader, "unit", CmpConUnit);
    sprintf(srefv, "%10.3e", CmpConRef);
    DmnRplValues(&UsrHeader, "refv", srefv);
    DmnRplValues(&UsrHeader, "artp", (odor) ? ot : "\"C\"");
    DmnWrite(fname, UsrHeader.s, &CncDsc);                          eG(18);
    vMsg(_file_$_written_, fname);
    if (Scatter) {
      sprintf(fname, "\"%s-%03d%s%s\"", name, day, CfgDevString, GrdName);  //-2006-10-31
      DmnRplValues(&UsrHeader, "file", fname);
      sprintf(fname, "%s%s-%03d%s%s", Path, name, day, CfgDevString, GrdName); //-2006-10-31
      if (odor) {                                                   //-2004-06-10
        DmnRplValues(&UsrHeader, "form", "\"sct%5.1f\"");
        DmnRplValues(&UsrHeader, "unit", CmpConUnit);
        DmnRplValues(&UsrHeader, "refv", "1.0");
        DmnRplValues(&UsrHeader, "artp", "\"EHP\"");              //-2011-06-09
      }
      else {
        DmnRplValues(&UsrHeader, "form", "\"dev%(*100)5.1f\"");
        DmnRplValues(&UsrHeader, "unit", NULL);
        DmnRplValues(&UsrHeader, "refv", "0.03");
        DmnRplValues(&UsrHeader, "artp", "\"E\"");
      }
      DmnWrite(fname, UsrHeader.s, &DevDsc);                        eG(19);
      vMsg(_file_$_written_, fname);
    }
  }
  else {
    char *pt = (k2 < 0) ? "wet" : "dry";
    DmnRplValues(&UsrHeader, "sequ", "j-,i+");
    DmnRplValues(&UsrHeader, "axes", "\"xy\"");
    sprintf(gname, "%s-%s%03d%s%s", name, pt, day, CfgAddString, GrdName); //-2011-11-25
    DmnRplValues(&UsrHeader, "file", TxtQuote(gname));
    sprintf(fname, "%s%s-%s%03d%s%s", Path, name, pt, day, CfgAddString, GrdName);  //-2011-11-25
    DmnRplValues(&UsrHeader, "form", (k2 < 0) ? CmpWetForm : CmpDryForm);
    DmnRplValues(&UsrHeader, "unit", CmpDepUnit);
    sprintf(srefv, "%10.3e", CmpDepRef);
    DmnRplValues(&UsrHeader, "refv", srefv);
    DmnRplValues(&UsrHeader, "artp", "\"XD\"");                     //-2002-07-24
    DmnWrite(fname, UsrHeader.s, &CncDsc);                          eG(20);
    vMsg(_file_$_written_, fname);
    if (Scatter) {
      sprintf(fname, "\"%s-%s%03d%s%s\"", name, pt, day, CfgDevString, GrdName); //-2011-11-25
      DmnRplValues(&UsrHeader, "file", fname);
      sprintf(fname, "%s%s-%s%03d%s%s", Path, name, pt, day, CfgDevString, GrdName); //-2011-11-25
      DmnRplValues(&UsrHeader, "form", "\"dev%(*100)5.1f\"");
      DmnRplValues(&UsrHeader, "unit", NULL);
      DmnRplValues(&UsrHeader, "refv", "0.03");
      DmnRplValues(&UsrHeader, "artp", "\"E\"");
      DmnWrite(fname, UsrHeader.s, &DevDsc);                        eG(21);
      vMsg(_file_$_written_, fname);
    }
  }
  AryFree(&CncDsc);
  AryFree(&DevDsc);
  setlocale(LC_NUMERIC, locale);                                  //-2008-10-17
  return 0;
eX_1:
  eMSG(_invalid_kref_);
eX_10: eX_11: eX_12: eX_13: eX_14: eX_15: eX_16: eX_17: eX_18: eX_19:
eX_20: eX_21:
  eMSG(_internal_error_);
}

static long TmtDos2Con(ARYDSC *pa, TXTSTR *ph) {
  dP(TmtDos2Con);
  int i, j, k, l, nx, ny, nz, nc, sz, scatter, ng;
  long t1, t2;
  char *_t1s, *_t2s;
  float area, f, sk[201], dd, scale, *pd, rgf, dos, cnc, dsq, dev;
  float zd, valid;
  //
  nx = pa->bound[0].hgh;
  ny = pa->bound[1].hgh;
  nz = pa->bound[2].hgh;
  nc = pa->bound[3].hgh + 1;
  sz = pa->elmsz;
  if (1 != DmnGetFloat(ph->s, "delta|dd", "%f", &dd, 1))                eX(1);
  if (1 != DmnGetFloat(ph->s, "zscl", "%f", &zd, 1))                    eX(2);
  if (1 != DmnGetInt(ph->s, "gruppen|groups", "%d", &ng, 1))            eX(3);
  if (nz > 200)                                                         eX(4);
  if (nz+1 > DmnGetFloat(ph->s, "sk|zk", "%f", sk, 201))                eX(5);
  if (1 != DmnGetString(ph->s, "t1", &_t1s, 1))                         eX(6);
  if (1 != DmnGetString(ph->s, "t2", &_t2s, 1))                         eX(7);
  if (zd > 0)                                                           eX(8);
  if (1 != DmnGetFloat(ph->s, "Valid", "%f", &valid, 1))                eX(9);
  if (valid < 0)  valid = 0;
  if (valid > 1)  valid = 1;
  t1 = TmValue(_t1s);
  t2 = TmValue(_t2s);
  scatter = (sz == 2*sizeof(float));
  rgf = (ng > 1) ? 1.0/(ng-1.0) : 1.0;
  area = dd*dd*(t2 - t1);
  for (i=1; i<=nx; i++) {
    for (j=1; j<=ny; j++) {
      scale = 1;
      for (k=-1; k<=nz; k++) {
        if (k <= 0)
          f = area;
        else
          f = area*(sk[k] - sk[k-1])*scale;
        pd = AryPtr(pa, i, j, k, 0);  if (!pd)                          eX(10);
        for (l=0; l<nc; l++) {
          dos = *pd;
          cnc = (dos < 0 || valid == 0) ? -1 : dos/(f*valid);
          *pd++ = cnc;
          if (scatter) {
            if (dos <= 0.0 || valid == 0) {
              dev = 0;
            }
            else {
              dsq = *pd;
              dev = (ng*dsq/(dos*dos)-1.0)*rgf;
              dev = (dev < 0) ? 0.0 : sqrt(dev);
            }
            *pd++ = dev;
          }
        }  // for l
      } // for k
    } // for j
  } // for i
  //
  DmnRplName(ph, "pgm", "prgm");                  //-2008-12-04
  DmnRplName(ph, "arrtype", "artp");
  DmnRplName(ph, "einheit", "unit");
  DmnRplName(ph, "kennung", "idnt");              //-2008-12-04
  DmnRplName(ph, "gruppen", "groups");
  DmnRplName(ph, "format", "form");
  DmnRplName(ph, "valdef", "vldf");
  DmnRplName(ph, "folge", "index");
  DmnRplName(ph, "gakrx", "refx");
  DmnRplName(ph, "gakry", "refy");
  DmnRplValues(ph, "artp", "\"C\"");
  //
  FREE(_t1s);
  FREE(_t2s);
  return 0;
eX_1: eX_2: eX_3: eX_4: eX_5: eX_6: eX_7: eX_8: eX_9: eX_10:
  eMSG(_internal_error_);
}

//================================================================== ReadArray
//

static void print_array(ARYDSC *pa, char *comment) {
  int nx, ny, i, j, sz;
  float *pf;
  printf("\n%s\n", comment);
  sz = pa->elmsz;
  nx = pa->bound[0].hgh;
  ny = pa->bound[1].hgh;
  for (j=ny; j>0; j--) {
    for (i=1; i<=nx; i++) {
      pf = (float*)AryPtr(pa, i, j, 1, 0);
      if (sz > 4)
        printf("| %10.4e %10.4e ", pf[0], pf[1]);
      else
        printf("| %10.4e ", pf[0]);
    }
    printf("|\n");
  }

}

static long ReadArray( void )
  {
  dP(ReadArray);
  long n;
  char name[256], fn[256];                                        //-2008-08-04
  char *_s = NULL;
  int i;
  //
  if (CurDos.start)  AryFree(&CurDos);
  n = Folge + Ia;
  sprintf(name, DosName, n);
  sprintf(Eingabe, "%s%s", Path, name);
  strcpy(fn, Eingabe);                                            //-2008-08-04
  strcat(fn, ".dmna");                                            //-2011-06-29
  if (!TutFileExists(fn))                                               eX(7);
  DmnRead(Eingabe, &UsrHeader, &SysHeader, &CurDos);                    eG(3);
  if (!*worker) {                                                 //-2008-08-15
    i = DmnGetString(UsrHeader.s, "prgm|pgm", &_s, 1);            //-2008-12-04
    if (i == 1) {
      strcpy(worker, _s);
      FREE(_s);
      _s = NULL;
    }
  }
  i = DmnGetString(UsrHeader.s, "artp|arrtype", &_s, 1);  if (i != 1)   eX(4);
  if (!strcmp(_s, "D")) {
    TmtDos2Con(&CurDos, &UsrHeader);                                    eG(5);
  }
  else if (strcmp(_s, "C"))                                             eX(6); //-2014-06-26
  MsgQuiet = 0;
  if (MsgCode < 0) {
    MsgCode = 0;
    printf("   \r");  fflush(stdout);
    return 0;
  }
  printf("%3ld\r", n);  fflush(stdout);
  if (!ElmSz) {
    ElmSz = CurDos.elmsz;
    if (ElmSz > sizeof(float)) {
      Scatter = 1;
      if (ElmSz != 2*sizeof(float))                                     eX(2);
    }
    else  Scatter = 0;                                            //-2002-08-09
  }
  else if (ElmSz > CurDos.elmsz)                                        eX(1);
  if (_s)  FREE(_s);
  return 1;
eX_1: eX_2:
  eMSG(_inconsistent_data_);
eX_3: eX_7:
  eMSG(_cant_read_$_, Eingabe);
eX_4: eX_5: eX_6:
  eMSG(_internal_error_);
}

/*================================================================ CompareArray
*/
static long CompareArray( void )
  {
  dP(CompareArray);
  int n, i, ii, i1, i2, j, j1, j2, k, k1, k2, l, l1, l2, m, dm;
  int k0;                                                         //-2011-11-23
  int nh, ni, ic;
  float *pcurdos, *pvecdos, *psumdos, *pfs, *pfd;
  float cnc, dev, csum, dsum, vsum;                            //-2008-10-07
  short day, *pvecday, *pis, *pid;
  char t1s[40], t2s[40], *_pt1s=NULL, *_pt2s=NULL;
  char name[40];
  double t1, t2;
  float valid = 1;
  int not_used=0, gas;                                            //-2008-10-07
  //
  DmnGetFloat(UsrHeader.s, "Valid", "%f", &valid, 1);
  if (valid < 0) valid = 0;                                        //-2006-07-13
  if (valid > 1) valid = 1;                                        //-2006-07-13
  if (valid < MinValid)  not_used = 1;                             //-2001-06-29
  else  SumValid += valid;        //-2002-08-09   //-2002-05-24    //-2001-06-29
  *t1s = 0;
  *t2s = 0;
  if (1 != DmnGetString(UsrHeader.s, "T1", &_pt1s, 1))                  eX(7);
  strcpy(t1s, _pt1s);  FREE(_pt1s);
  if (1 != DmnGetString(UsrHeader.s, "T2", &_pt2s, 1))                  eX(8);
  strcpy(t2s, _pt2s);  FREE(_pt2s);
  t1 = MsgDateVal(t1s);
  t2 = MsgDateVal(t2s);
  if (CurDos.numdm != 4)                                                eX(1);
  i1 = CurDos.bound[0].low;  i2 = CurDos.bound[0].hgh;
  j1 = CurDos.bound[1].low;  j2 = CurDos.bound[1].hgh;
  k1 = CurDos.bound[2].low;  k2 = CurDos.bound[2].hgh;
  l1 = CurDos.bound[3].low;  l2 = CurDos.bound[3].hgh;
  if (i1!=1 || j1!=1 || k1>-1 || l1!=0 )                                eX(2);
  dm = 1 + Scatter;
  k0 = 1;                                                         //-2005-04-12
  if (Deposition)  k0 = 0;                                        //-2011-12-19
  if (WetDeposition)  k0 = -1;                                    //-2011-12-14
  if (SumDos.start == NULL) {
    Nsum = 0;
    Nval = 0;
    T1 = t1;
    T2 = t2;
    Nx = i2;
    Ny = j2;
    Nz = k2;                                                    //-2002-03-25
    Nc = l2 + 1;
    Dt = MsgDateSeconds(t1, t2) + 0.5;                          //-2005-08-06
    if (Dt <= 0)                                                        eX(3);
    Names = ALLOC((Nc+1)*sizeof(char*));
    n = DmnGetString(UsrHeader.s, "name", Names, Nc);  if (n != Nc)     eX(9);
    n = DmnGetFloat(UsrHeader.s, "delta", "%f", &Dd, 1);  if (n != 1)   eX(14);
    n = DmnGetFloat(UsrHeader.s, "sk", "%f", Sk, 201);  if (n < Nz+1)   eX(15); //-2008-08-04
    n = DmnGetInt(UsrHeader.s, "groups", "%d", &Ng, 1); if (n != 1)     eX(30); //-2008-08-04
    //
    // get the indices for the requested species
    //
    strcpy(name, "gas.");
    strcat(name, CmpName);
    for (ii=0; ii<8; ii++) {
      CmpIndex[ii] = -1;
      CmpIsGas[ii] = 0;
    }
    ii = 0;
    for (ic=0; ic<6; ic++) {
      sprintf(name, "%s.%s%s", SttGroups[ic], CmpName, SttGrpXten[ic]);
      for (l=0; l<Nc; l++) {
        if (!strcmp(Names[l], name)) {
          CmpIndex[ii] = l;
          CmpIsGas[ii] = (ic < 3);
          ii++;
          break;
        }
      }
    }
    if (ii == 0)                                                    eX(16);
    for (n=0; ; n++) 
      if (!Names[n])
        break;
      else FREE(Names[n]);   									//-2001-11-02
    FREE(Names);
    Names = NULL;
    Nh = CmpDayNum;
    if (Anzahl < 2)  Maxima = 0;
    //
    // create the arrays
    //
    if (Ng <= 4) {
      Scatter = 0;
      ElmSz = 4;
      dm = 1;                                                       //-2006-07-13
    }
    AryCreate(&SumDos, ElmSz, 3, 1,Nx, 1,Ny, k0,Nz);                eG(5);
    if (Maxima) {
      AryCreate(&VecDos, ElmSz, 4, 1,Nx, 1,Ny, 1,Nz, 0,Nh);         eG(4);
      AryCreate(&VecDay, sizeof(short), 4, 1,Nx, 1,Ny, 1,Nz, 0,Nh); eG(6);
    }
  }
  if (i2!=Nx || j2!=Ny || k2<Nz || l2!=Nc-1 )                       eX(11); //-2001-08-03
  if ((long)(MsgDateSeconds(t1, t2) + 0.5) != Dt)                   eX(13); //-2005-08-06
  T2 = t2;
  day = Folge + Ia - 1;     // first value of Ia is 1 at this point
  if (Kref>=0 || Kmax>=0) {                                       //-2002-04-09
    WriteDmna(day, valid);                                         eG(28);  //-2002-08-09
  }
  nh = CmpDayNum;
  for (k=k0; k<=Nz; k++) {                                        //-2011-11-23
    if (not_used)  break;                                         //-2008-08-04
    for (i=1; i<=Nx; i++) {
      for (j=1; j<=Ny; j++) {
        l = CmpIndex[0];
        if (l < 0)
          continue;
        csum = 0;                                                 //-2008-08-04
        dsum = 0;
        vsum = 0;                                                 //-2008-10-07
        for (ii=0; ii<8; ii++) {
          l = CmpIndex[ii];
          gas = CmpIsGas[ii];                                     //-2008-10-07
          if (l < 0)
            break;
          if (k>0 && !gas)                                        //-2008-10-07
            continue;
          pcurdos = AryPtr(&CurDos, i, j, k, l);  if (!pcurdos)     eX(23);
          cnc = pcurdos[0];
          csum += cnc;
          if (Scatter) {
            dev = cnc*pcurdos[1];                                 //-2008-10-07
            if (gas)                                              //-2008-10-07
              dsum += dev;                                        //-2008-10-07
            else                                                  //-2008-10-07
              vsum += dev*dev;                                    //-2008-10-07
          }
        }
        if (vsum > 0) {                                           //-2008-10-07
          dsum = sqrt(vsum + dsum*dsum);                          //-2008-10-07
        }                                                         //-2008-10-07
        //
        psumdos = AryPtr(&SumDos, i, j, k);  if (!psumdos)       eX(27);
        psumdos[0] += valid*csum;                                 //-2008-08-04
        if (Scatter) {
          psumdos[1] += valid*valid*dsum*dsum;                    //-2008-08-04
        }
        //
        if (csum > 0)  dsum /= csum;
        cnc = csum;
        dev = dsum;
        if (k>0 && Maxima) {
          pvecdos = AryPtr(&VecDos, i, j, k, 0);  if (!pvecdos)  eX(24);
          pvecday = AryPtr(&VecDay, i, j, k, 0);  if (!pvecday)  eX(25);
          for (pfs=pvecdos, m=0; m<=nh; m++, pfs+=dm)
            if (cnc >= *pfs)
              break;
          if (m <= nh) {
            pfd = pvecdos + dm*nh + Scatter;
            pfs = pfd - dm;
            pid = pvecday + nh;
            pis = pid - 1;
            ni = nh - m;
            for (ii=ni; ii>0; ii--) {
              *pfd-- = *pfs--;
              if (Scatter)  *pfd-- = *pfs--;
              *pid-- = *pis--;
            }
            pfd = pvecdos + dm*m;
            pid = pvecday + m;
            pfd[0] = cnc;
            if (Scatter)  pfd[1] = dev;
            pid[0] = day;                                         //-2001-06-09
          }  // if (m <= nh)
        }  // if (k > 0)
      }  // for j
    }  // for i
  }  // for k
  Nsum++;
  if (!not_used)  Nval++;
  return not_used;
eX_1:  eX_2:  eX_3:  eX_7: eX_8: eX_9: eX_11: eX_13: eX_14: eX_15:
  eMSG(_invalid_array_);
eX_4:  eX_5:  eX_6:
  eMSG(_cant_create_array_);
eX_16:
  eMSG(_no_results_for_$_, CmpName);
eX_23: eX_24: eX_25: eX_27: eX_28:
  eMSG(_internal_error_);
eX_30:
  eMSG(_gruppen_not_found_);
  }

/*================================================================= WriteResult
*/
static long WriteResult( void )
  {
  dP(WriteResult);
  char fname[256], gname[256], t[80], name[40], srefv[40], ot[40], of[40], oe[40];
  char locale[256];                                               //-2008-10-17
  char *pc, src, dst;
  int i, j, k, k1, nh, odor;
  float *pvecdos, *pmaxcon, *pmaxdev, *psumdos, dev, cnc, csum, dsum;
  float *psupcon, *psupdev;
  float *psumcon, *psumdev, *psumdep, *psumdps, *pttldep, *pttldps;
  float xdep, xcon, valid, sumval;
  float bs = SttOdorThreshold;
  short *pvecday, *pmaxday, *psupday;
  //
  strcpy(locale, setlocale(LC_NUMERIC, NULL));                    //-2008-10-17
  DmnRplValues(&UsrHeader, "cset", NULL);                         //-2014-06-26
  sprintf(t, "\n%s  \"%s\"", "cset", TI.cset);                    //-2008-07-22 -2014-01-28
  TxtPIns(&UsrHeader, 0, t);                                      //-2008-07-22 
  sprintf(t, "%ld", Folge+Ia-1);                                  //-2011-06-29
  DmnRplValues(&UsrHeader, "index", t);                           //-2008-08-04
  nh = CmpDayNum;
  strcpy(name, CmpFamily);                                    //-2001-11-02
  odor = !strncmp(name, "odor", 4);                           //-2008-03-10
  if (odor) {
    sprintf(ot, "\"FHP%4.2f\"", bs);                          //-2004-12-09
    sprintf(oe, "\"EHP\"");                                   //-2004-12-09
    sprintf(of, "\"frq%s", CmpConForm+4);
  }
  xcon = CmpConFac;
  xdep = CmpDepFac;
  valid = (SumValid > 0) ? SumValid/Nsum : 1;                 //-2001-06-29
  sumval = (SumValid > 0) ? SumValid : 1;
  if (Nval < 0.9*RefDays) {                                   //-2002-08-10
    nh = nh*Nval/RefDays;                                     //-2002-08-10
    if (Maxima) vMsg(_exceedings_changed_$_, nh);
  }
  k1 = SumDos.bound[2].low;
  if (Concentration) {
    AryCreate(&SumCon, sizeof(float), 3, 1,Nx, 1,Ny, 1,Nz);   eG(20);
    if (Scatter) {
      AryCreate(&SumDev, sizeof(float), 3, 1,Nx, 1,Ny, 1,Nz); eG(21);
    }
  }
  if (DryDeposition || Deposition) {                              //-2012-04-06
    AryCreate(&DryDep, sizeof(float), 2, 1,Nx, 1,Ny);         eG(22);
    if (Scatter) {
      AryCreate(&DryDps, sizeof(float), 2, 1,Nx, 1,Ny);       eG(23);
    }
  }
  if (WetDeposition) {                                            //-2011-12-14
    AryCreate(&WetDep, sizeof(float), 2, 1,Nx, 1,Ny);         eG(24);
    if (Scatter) {
      AryCreate(&WetDps, sizeof(float), 2, 1,Nx, 1,Ny);       eG(25);
    }
  }
  if (Deposition) {                                               //-2011-12-19
    AryCreate(&TtlDep, sizeof(float), 2, 1,Nx, 1,Ny);         eG(26);
    if (Scatter) {
      AryCreate(&TtlDps, sizeof(float), 2, 1,Nx, 1,Ny);       eG(27);
    }
  }
  if (Maxima) {
    AryCreate(&MaxCon, sizeof(float), 3, 1,Nx, 1,Ny, 1,Nz);   eG(4);
    AryCreate(&SupCon, sizeof(float), 3, 1,Nx, 1,Ny, 1,Nz);   eG(4);
    if (Scatter) {
      AryCreate(&SupDev, sizeof(float), 3, 1,Nx, 1,Ny, 1,Nz); eG(5);
      AryCreate(&MaxDev, sizeof(float), 3, 1,Nx, 1,Ny, 1,Nz); eG(5);
    }
    AryCreate(&MaxDay, sizeof(short), 3, 1,Nx, 1,Ny, 1,Nz);   eG(6);
    AryCreate(&SupDay, sizeof(short), 3, 1,Nx, 1,Ny, 1,Nz);   eG(6);
  }
  //
  for (k=k1; k<=Nz; k++) {
    for (i=1; i<=Nx; i++) {
      for (j=1; j<=Ny; j++) {
        psumdos = AryPtr(&SumDos, i, j, k);  if (!psumdos)          eX(15);
        if (SumValid == 0) {
          psumdos[0] = -1;
          if (Scatter)  psumdos[1] = -1;
        }
        else {
          csum = psumdos[0];
          psumdos[0] = csum/sumval;
          if (Scatter) {
            dsum = psumdos[1];
            if (dsum > 0)  dsum = sqrt(dsum);
            if (csum > 0)  dsum /= csum;
            psumdos[1] = dsum;
          }
        }
      } // for j
    } // for i
  } // for k
  //
  for (k=k1; k<=Nz; k++) {
    for (i=1; i<=Nx; i++) {
      for (j=1; j<=Ny; j++) {
        if (k <= 0) {
          pttldep = AryPtrX(&TtlDep, i, j);
          if (Scatter) {
            pttldps = AryPtrX(&TtlDps, i, j);
          }
        }
        psumdos = AryPtr(&SumDos, i, j, k);  if (!psumdos)          eX(15);
        cnc = psumdos[0];
        dev = (Scatter) ? psumdos[1] : 0;
        if (k == -1) {
          if (WetDeposition) {                                    //-2011-12-19
            psumdep = AryPtr(&WetDep, i, j);  if (!psumdep)         eX(16);
            *psumdep = cnc*xdep;                                  //-2008-08-04
            *pttldep = cnc*xdep;                                  //-2011-12-14
            if (Scatter) {
              psumdps = AryPtr(&WetDps, i, j);  if (!psumdps)       eX(17);
              *psumdps = dev;                                     //-2008-08-04
              *pttldps = dev;                                     //-2011-12-14
            }
          }
        }
        else if (k == 0) {
          if (DryDeposition || Deposition) {                      //-2012-04-06
            float wv = *pttldep;        // wet value (if any)
            psumdep = AryPtr(&DryDep, i, j);  if (!psumdep)         eX(46);
            *psumdep = cnc*xdep;                                  //-2008-08-04
            *pttldep += cnc*xdep;                                 //-2011-12-14
            if (Scatter) {
              psumdps = AryPtr(&DryDps, i, j);  if (!psumdps)       eX(47);
              *psumdps = dev;                                     //-2008-08-04
              if (wv > 0) {                                       //-2011-12-14
                float dv = *psumdep;    // dry value;
                float ds = dev;         // dry scatter
                float ws = *pttldps;    // wet scatter
                float var = dv*dv*ds*ds + wv*wv*ws*ws;    // variance
                *pttldps = sqrt(var)/(*pttldep);
              }
              else {
                *pttldps = dev;
              }
            }
          }
        }
        else {
          if (Concentration) {
            psumcon = AryPtr(&SumCon, i, j, k);  if (!psumcon)      eX(18);
            *psumcon = cnc*xcon;                                  //-2008-08-04
            if (Scatter) {
              psumdev = AryPtr(&SumDev, i, j, k);  if (!psumdev)    eX(19);
              *psumdev = dev;                                     //-2008-08-04
              if (odor)  *psumdev *= cnc*xcon;                    //-2008-08-04
            }
          }
          if (Maxima) {
            pmaxcon = AryPtr(&MaxCon, i, j, k);  if (!pmaxcon)        eX(10);
            pmaxday = AryPtr(&MaxDay, i, j, k);  if (!pmaxday)        eX(11);
            pvecdos = AryPtr(&VecDos, i, j, k, nh);  if (!pvecdos)    eX(12);
            pvecday = AryPtr(&VecDay, i, j, k, nh);  if (!pvecday)    eX(13);
            cnc = pvecdos[0];
            *pmaxcon = cnc*xcon;
            if (Scatter) {
              pmaxdev = AryPtr(&MaxDev, i, j, k);  if (!pmaxdev)      eX(14);
              dev = pvecdos[1];
              *pmaxdev = dev;
             }
            *pmaxday = *pvecday;
            if (nh >= 0) {                                        //-2002-09-28
              psupcon = AryPtr(&SupCon, i, j, k);  if (!psupcon)        eX(10);
              psupday = AryPtr(&SupDay, i, j, k);  if (!psupday)        eX(11);
              pvecdos = AryPtr(&VecDos, i, j, k, 0);  if (!pvecdos)     eX(12);
              pvecday = AryPtr(&VecDay, i, j, k, 0);  if (!pvecday)     eX(13);
              cnc = pvecdos[0];
              *psupcon = cnc*xcon;
              if (Scatter) {
                psupdev = AryPtr(&SupDev, i, j, k);  if (!psupdev)      eX(14);
                dev = pvecdos[1];
                *psupdev = dev;
               }
              *psupday = *pvecday;
            } // if nh
          } // if maxima
        } // k>0
      } // for j
    } // for i
  } // for k
  sprintf(t, "\"%s\"", name);
  for (pc=t; (*pc); pc++)  *pc = toupper(*pc);
  DmnRplValues(&UsrHeader, "name", t);
  DmnRplValues(&UsrHeader, "vldf", "\"V\"");
  DmnRplValues(&UsrHeader, "axes", "\"xyz\"");
  DmnRplValues(&UsrHeader, "sequ", "k+,j-,i+");
  DmnRplValues(&UsrHeader, "refc", NULL);
  DmnRplValues(&UsrHeader, "refd", NULL);
  DmnRplValues(&UsrHeader, "unit", NULL);
  DmnRplValues(&UsrHeader, "t1", TxtQuote(MsgDateString(T1)));    //-2008-12-11
  DmnRplValues(&UsrHeader, "t2", TxtQuote(MsgDateString(T2)));    //-2008-12-11
  if (*locl) {
    char s[256];
    sprintf(s, "\"%s\"", locl);
    DmnRplValues(&UsrHeader, "locl", s);
    MsgSetLocale(locl);                                           //-2008-10-17
  }
  else {
    DmnRplValues(&UsrHeader, "locl", "\"C\"");
    setlocale(LC_NUMERIC, "C");                                   //-2008-10-17
  }
  if (!strcmp(locl, "german"))  { src = '.';  dst = ','; }
  else                          { src = ',';  dst = '.'; }
  DmnRplChar(&UsrHeader, "xmin", src, dst);
  DmnRplChar(&UsrHeader, "ymin", src, dst);
  DmnRplChar(&UsrHeader, "valid", src, dst);
  DmnRplChar(&UsrHeader, "sk|zk", src, dst);
  DmnRplChar(&UsrHeader, "dd|delta", src, dst);
  sprintf(t, "%1.6f", valid);                                     //-2008-08-04
  DmnRplValues(&UsrHeader, "valid", t);
  if (Concentration) {
    sprintf(gname, "%s-%s00%s%s",
        name, CfgYearString, CfgAddString, GrdName);              //-2006-10-31
    DmnRplValues(&UsrHeader, "file", TxtQuote(gname));
    sprintf(fname, "%s%s-%s00%s%s",
        Path, name, CfgYearString, CfgAddString, GrdName);        //-2006-10-31
    DmnRplValues(&UsrHeader, "form", (odor) ? of : CmpConForm);
    DmnRplValues(&UsrHeader, "unit", CmpConUnit);
    sprintf(srefv, "%10.3e", CmpConRef);
    DmnRplValues(&UsrHeader, "refv", (odor) ? "0" : srefv);       //-2004-12-09
    DmnRplValues(&UsrHeader, "artp", (odor) ? ot : "\"C\"");
    DmnWrite(fname, UsrHeader.s, &SumCon);                          eG(31);
    vMsg(_file_$_written_, fname);
    if (Scatter) {
      sprintf(fname, "\"%s-%s00%s%s\"",
          name, CfgYearString, CfgDevString, GrdName);            //-2006-10-31
      DmnRplValues(&UsrHeader, "file", fname);
      sprintf(fname, "%s%s-%s00%s%s",
          Path, name, CfgYearString, CfgDevString, GrdName);      //-2006-10-31
      if (odor) {                                                 //-2004-06-10
        DmnRplValues(&UsrHeader, "form", "\"sct%5.1f\"");         //-2011-06-09
        DmnRplValues(&UsrHeader, "unit", CmpConUnit);
        DmnRplValues(&UsrHeader, "refv", "0.0");                  //-2004-12-09
        DmnRplValues(&UsrHeader, "artp", oe);                     //-2004-12-09
      }
      else {
        DmnRplValues(&UsrHeader, "form", "\"dev%(*100)5.1f\"");
        DmnRplValues(&UsrHeader, "unit", "\"1\"");                //-2011-06-09
        DmnRplValues(&UsrHeader, "refv", "0.03");
        DmnRplValues(&UsrHeader, "artp", "\"E\"");
      }
      DmnWrite(fname, UsrHeader.s, &SumDev);                        eG(32);
      vMsg(_file_$_written_, fname);
    }
  }
  if (Maxima) {
    int ii, nn;
    ARYDSC *pa;
    DmnRplValues(&UsrHeader, "valid", NULL);                      //-2002-08-10
    for (ii=0; ii<2; ii++) {
      nn = (ii) ? 0: CmpDayNum;
      sprintf(gname, "%s-%s%02d%s%s",
          name, CfgDayString, nn, CfgAddString, GrdName);         //-2006-10-31
      DmnRplValues(&UsrHeader, "file", TxtQuote(gname));
      sprintf(fname, "%s%s-%s%02d%s%s",
          Path, name, CfgDayString, nn, CfgAddString, GrdName);   //-2006-10-31
      DmnRplValues(&UsrHeader, "form", CmpDayForm);
      DmnRplValues(&UsrHeader, "unit", CmpConUnit);
      sprintf(srefv, "%10.3e", CmpDayRef);
      DmnRplValues(&UsrHeader, "refv", srefv);
      sprintf(t, "\"CD%02d\"", (ii) ? 0 : nh);                    //-2002-08-10
      DmnRplValues(&UsrHeader, "artp", t);                        //-2002-07-24
      sprintf(t, "%d", (ii) ? 0 : nh);                            //-2002-08-10
      DmnRplValues(&UsrHeader, "exceed", t);
      pa = (ii) ? &SupCon : &MaxCon;                              //-2001-07-11
      DmnWrite(fname, UsrHeader.s, pa);                               eG(33);
      vMsg(_file_$_written_, fname);
      if (Scatter) {
        sprintf(fname, "\"%s-%s%02d%s%s\"",
            name, CfgDayString, nn, CfgDevString, GrdName);       //-2006-10-31
        DmnRplValues(&UsrHeader, "file", fname);
        sprintf(fname, "%s%s-%s%02d%s%s",
            Path, name, CfgDayString, nn, CfgDevString, GrdName); //-2006-10-31
        DmnRplValues(&UsrHeader, "form", "\"dev%(*100)5.1f\"");
        DmnRplValues(&UsrHeader, "unit", NULL);
        DmnRplValues(&UsrHeader, "refv", "0.30");
        DmnRplValues(&UsrHeader, "artp", "\"E\"");
        pa = (ii) ? &SupDev : &MaxDev;                            //-2001-07-11
        DmnWrite(fname, UsrHeader.s, pa);                             eG(34);
        vMsg(_file_$_written_, fname);
      }
      sprintf(fname, "\"%s-%s%02d%s%s\"",
          name, CfgDayString, nn, CfgIndString, GrdName);         //-2006-10-31
      DmnRplValues(&UsrHeader, "file", fname);
      sprintf(fname, "%s%s-%s%02d%s%s",
          Path, name, CfgDayString, nn, CfgIndString, GrdName);   //-2006-10-31
      DmnRplValues(&UsrHeader, "form", "\"day%4hd\"");
      DmnRplValues(&UsrHeader, "unit", NULL);
      DmnRplValues(&UsrHeader, "refv", NULL);
      DmnRplValues(&UsrHeader, "artp", "\"N\"");
      pa = (ii) ? &SupDay : &MaxDay;                              //-2001-07-11
      DmnWrite(fname, UsrHeader.s, pa);                               eG(35);
      vMsg(_file_$_written_, fname);
      if (nn == 0)
        break;                                                    //-2001-07-11
      nn = 0;                                                     //-2001-07-11
    }
  }
  if (k1 < 1)                                                     //-2011-12-19
    k1 = -2;
  for (k=k1; k<=0; k++) {																					//-2011-11-23
    char *px;
    char *pt;
    char *pf;
    ARYDSC *pa, *ps;
    if (k == 0 && DryDeposition) {                                //-2011-12-19
      pt = CfgDryString;
      px = "\"XD\"";
      pa = &DryDep;
      ps = &DryDps;
      pf = CmpDryForm;
    }
    else if (k == -1 && WetDeposition) {                          //-2011-12-19
      pt = CfgWetString;
      px = "\"XW\"";
      pa = &WetDep;
      ps = &WetDps;
      pf = CmpWetForm;
    }
    else if (k == -2 && Deposition) {                             //-2011-12-19
      pt = CfgDepString;
      px = "\"X\"";
      pa = &TtlDep;
      ps = &TtlDps;
      pf = CmpDepForm;
    }
    else                                                          //-2011-12-19
      continue;
    sprintf(t, "%1.6f", valid);                                   //-2008-08-04
    DmnRplValues(&UsrHeader, "valid", t);                         //-2002-08-10
    DmnRplValues(&UsrHeader, "sequ", "j-,i+");
    DmnRplValues(&UsrHeader, "axes", "\"xy\"");
    sprintf(gname, "%s-%s%s%s",
        name, pt, CfgAddString, GrdName);                         //-2011-11-23
    DmnRplValues(&UsrHeader, "file", TxtQuote(gname));
    sprintf(fname, "%s%s-%s%s%s",
        Path, name, pt, CfgAddString, GrdName);                   //-2011-11-23
    DmnRplValues(&UsrHeader, "form", pf);                         //-2011-12-14
    DmnRplValues(&UsrHeader, "unit", CmpDepUnit);
    sprintf(srefv, "%10.3e", CmpDepRef);
    DmnRplValues(&UsrHeader, "refv", srefv);
    DmnRplValues(&UsrHeader, "artp", px);                         //-2011-12-14
    DmnWrite(fname, UsrHeader.s, pa);                                 eG(36);
    vMsg(_file_$_written_, fname);
    if (Scatter) {
      sprintf(fname, "\"%s-%s%s%s\"",
          name, pt, CfgDevString, GrdName);                       //-2011-11-23
      DmnRplValues(&UsrHeader, "file", fname);
      sprintf(fname, "%s%s-%s%s%s",
          Path, name, pt, CfgDevString, GrdName);                 //-2011-11-23
      DmnRplValues(&UsrHeader, "form", "\"dev%(*100)5.1f\"");
      DmnRplValues(&UsrHeader, "unit", NULL);
      DmnRplValues(&UsrHeader, "refv", "0.03");
      DmnRplValues(&UsrHeader, "artp", "\"E\"");
      DmnWrite(fname, UsrHeader.s, ps);                               eG(37);
      vMsg(_file_$_written_, fname);
    }
  }
  TxtClr(&UsrHeader);                                             //-2006-02-15
  AryFree(&SumCon);
  AryFree(&SumDev);
  AryFree(&SumDep);
  AryFree(&SumDps);
  AryFree(&DryDep);
  AryFree(&DryDps);
  AryFree(&WetDep);
  AryFree(&WetDps);
  AryFree(&TtlDep);
  AryFree(&TtlDps);
  AryFree(&MaxCon);
  AryFree(&MaxDev);
  AryFree(&MaxDay);
  AryFree(&SupCon);
  AryFree(&SupDev);
  AryFree(&SupDay);
  AryFree(&VecDos);
  AryFree(&VecDay);
  AryFree(&SumDos);
  AryFree(&CurDos);
  setlocale(LC_NUMERIC, locale);
  return 0;
eX_4: eX_5: eX_6:
eX_20: eX_21: eX_22: eX_23: eX_24: eX_25: eX_26: eX_27:
eX_31: eX_32: eX_33: eX_34: eX_35: eX_36: eX_37:
  eMSG(_cant_write_array_);
eX_10: eX_11: eX_12: eX_13: eX_14: eX_15: eX_16: eX_17: eX_18: eX_19:
eX_46: eX_47:
  eMSG(_internal_error_);
  }

/*======================================================================= main
*/
static void define_component( STTSPCREC rec ) {             //-2005-08-25
  char format[40], units[40];
  char locale[256]="C";
  strcpy(locale, setlocale(LC_NUMERIC, NULL));      //-2003-07-07
  setlocale(LC_NUMERIC, "C");
  strcpy(CmpName, rec.name);
  strcpy(CmpFamily, rec.name);
  strcpy(CmpGroups, rec.grps);                              //-2005-09-29
  CmpConFac = rec.fc;
  CmpConRef = rec.ry;
  if (rec.dy < 0)  strcpy(format, "\"con%10.3e\"");         //-2003-02-21
  else             sprintf(format, "\"con%%5.%df\"", rec.dy);
  strcpy(CmpConForm, format);
  sprintf(units, "\"%s\"", rec.uc);
  strcpy(CmpConUnit, units);
  CmpDayNum = rec.nd;
  CmpDayRef = rec.rd;
  if (rec.rd <= 0)  rec.nd = -1;
  if (rec.dd < 0)  strcpy(format, "\"con%10.3e\"");         //-2003-02-21
  else             sprintf(format, "\"con%%5.%df\"", rec.dd);
  strcpy(CmpDayForm, format);
  CmpHourNum = rec.nh;
  CmpHourRef = rec.rh;
  if (rec.rh <= 0)  rec.nh = -1;
  if (rec.dh < 0)  strcpy(format, "\"con%10.3e\"");         //-2003-02-21
  else             sprintf(format, "\"con%%5.%df\"", rec.dh);
  strcpy(CmpHourForm, format);
  CmpDepFac = rec.fn;
  CmpDepRef = rec.rn;
  CmpWetFac = rec.wf;
  if (rec.dn < 0)  strcpy(format, "%10.3e\"");         //-2011-11-25
  else             sprintf(format, "%%5.%df\"", rec.dn);
  strcpy(CmpDepForm, "\"dep");
  strcat(CmpDepForm, format);
  strcpy(CmpDryForm, "\"dry");
  strcat(CmpDryForm, format);
  strcpy(CmpWetForm, "\"wet");
  strcat(CmpWetForm, format);
  sprintf(units, "\"%s\"", rec.un);
  strcpy(CmpDepUnit, units);
  //                                                                -2011-12-14
  Deposition =  (CmpDepRef > 0);
  DryDeposition =  Deposition && (rec.vd>0 || CmpGroups[2]>'0')
      && (TalMode & TIP_RI_USED);
  WetDeposition =  Deposition && (rec.wf>0 || CmpGroups[2]>'0')
      && (TalMode & TIP_RI_USED);
  //                                                                -2012-04-06
  // if the reference value is >0 only because of wet deposition, but
  // no rain is used, then revise Deposition:
  if (Deposition && !WetDeposition && !(rec.vd>0) && !(CmpGroups[2]>'0'))
  		Deposition = 0;    
  //
  if (CHECK) vMsg(
    "MTM Name=%s, fc=%10.3e, ry=%10.3e, mc=%s, uc=%s, nd=%d, rd=%10.3e, md=%s, "
    "fn=%10.3e, wf=%10.3e, rn=%10.3e, mn=%s, un=%s",
    CmpName, CmpConFac, CmpConRef, CmpConForm, CmpConUnit, CmpDayNum,
    CmpDayRef, CmpDayForm, 
    CmpDepFac, CmpWetFac, CmpDepRef, CmpDepForm, CmpDepUnit);
  setlocale(LC_NUMERIC, locale);
}

char *TmtPrintWorker() {
  if (*worker)
    vMsg(_worker_$_, worker);
  return worker;
}

int TmtMain( char *s )
  {
  dP(TmtMain);
  int i, l;
  int ninvalid = 0;
  long rc;
  char *pc;
  if (CHECK) vMsg("TmtMain(%s)", s);
  if (*s) {
    pc = s;
    if (*pc == '-')
      switch (pc[1]) {
        case '0': Nh=0; Ns=0; Ng=0; Anzahl=0; Folge=1; Deposition=0;
                  strcpy(DosName, "c%04da00");                    //-2011-12-14
                  *GrdName = 0;
                  *CmpName = 0;
                  WriteHeader = 1;
                  Kref = -999;
                  Kmax = -999;
                  break;
        case 'A': strcpy(locl, pc+2);                             //-2003-07-07
                  break;
        case 'a': AverageOnly = 1;                                //-2007-02-03
                  break;
        case 'b': sscanf(pc+2, "%ld", &Folge);
                  break;
        case 'f': strcpy(DosName, pc+2);
                  break;
        case 'g': strcpy(GrdName, pc+2);
                  break;
        case 'k': Kref = 1;
                  sscanf(pc+2, "%d", &Kref);
                  break;
        case 'm': Kmax = 1;
                  sscanf(pc+2, "%d", &Kmax);
                  break;
        case 'n': sscanf(pc+2, "%ld", &Anzahl);
                  break;
        case 'q': MsgQuiet = 1;
                  sscanf(pc+2, "%d", &MsgQuiet);
                  break;
        case 's': sscanf(pc+2, "%d", &i);                         //-2011-11-23
                  define_component(SttSpcTab[i]);
                  break;
        default:  vMsg(_unknown_option_$_, pc);
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
  if (WriteHeader) vMsg(_evaluation_$_, CmpName);
  Nh = 0;
  Ng = 0;
  Nsum = 0;
  Concentration = (CmpConRef > 0);
  //Deposition    = (CmpDepRef > 0);                        -2012-04-06
  
  Maxima        = (CmpDayRef > 0 && !AverageOnly);          //-2007-02-03
  SumValid = 0;
  for (Ia=0; ; ) {
    rc = ReadArray();                                                   eG(1);
    if (rc==0 && Anzahl>0 && Ia<Anzahl)                                 eX(2);
    if (rc == 0)
      break;
    Ia++;
    rc = CompareArray();                                                eG(3);
    if (rc > 0)  ninvalid++;
    AryFree(&CurDos);
    if (Anzahl>0 && Ia>=Anzahl)
      break;
    }
  if (Anzahl>1 && WriteHeader>0)
    vMsg(_$_daily_averages_$_, Ia, ninvalid);
  WriteHeader = 0;
  WriteResult();                                                        eG(4);
  return 0;
eX_1:
  eMSG(_cant_read_$_, Eingabe);
eX_2:
  eMSG(_only_$_$_found_, Ia, Anzahl);
eX_3:
  eMSG(_error_maximum_);
eX_4:
  eMSG(_error_writing_);
  }

//=============================================================================
//
// history:
//
// 2002-06-21 lj 0.13.0  final test version
// 2002-07-24 lj 0.13.1  "artp" corrected
// 2002-08-10 lj 0.13.2  use of "valid" corrected, setting of "exceed", RefDays
// 2002-09-24 lj 1.0.0   final release candidate
// 2002-09-28 lj 1.0.1   write highest values even if nh==0
// 2003-02-21 lj 1.1.1   optional scientific notation
// 2003-07-07 lj 1.1.8   localisation
// 2004-06-10 lj 2.0.0   odor
// 2004-12-09 uj 2.1.1   artp for odor: FHP<BS>, EHP
// 2005-03-17 uj 2.2.0   version number upgrade
// 2005-08-06 uj 2.2.1   rounding of double returned by MsgDateSeconds()
// 2006-02-15 lj 2.2.9   TxtClr inserted in WriteResult
// 2006-07-13 uj 2.2.12  case gruppen<=4 corrected
// 2006-10-18 lj 2.3.0   externalization of strings
// 2006-10-31 lj 2.3.1   configuration data
// 2007-02-03 uj 2.3.5   option -a (AverageOnly)
// 2008-03-10 lj 2.4.0   evaluation of rated odor frequencies
// 2008-04-17 lj 2.4.1   merged with 2.3.x
// 2008-07-22 lj 2.4.2   parameter "cset" in dmna-header
// 2008-08-04 lj 2.4.3   reading dmna-files, writes "valid" using 6 decimals
// 2008-10-07 lj         scatter of deposition corrected
// 2008-10-17 lj         uses MsgSetLocale()
// 2008-12-04 lj 2.4.5   renamed "pgm", "ident"
// 2008-12-11 lj         header "genutl.h" included
// 2011-06-09 uj 2.4.10  DMN header entries adjusted
// 2011-07-07 uj 2.5.0   version number upgrade
// 2011-11-23 lj 2.6.0   wet deposition, settings
// 2011-12-14 lj         total deposition
// 2011-12-19 lj         output corrected
// 2014-06-26 uj 2.6.11  write cset to output header of daily files, eX/eG adjusted
//
//=============================================================================

