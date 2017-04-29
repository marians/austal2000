//======================================================== TALuti.c
//
// Utilities for AUSTAL2000
// ========================
//
// Copyright (C) Umweltbundesamt, Dessau-Roﬂlau, Germany, 2002-2008
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
// last change:  2012-04-06 uj
//
//==================================================================

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <math.h>
#include "IBJmsg.h"
#include "IBJary.h"
#include "IBJdmn.h"
#include "IBJtxt.h"
#include "TalAKT.h"
#include "TalUtl.h"
#include "TalInp.h"
#include "TalCfg.h"
#include "TALuti.h"
#include "TALuti.nls"

char *TALutiVersion = "2.6.0";
static char *eMODn = "TALuti";

//================================================================ TutCalcRated
//
float TutCalcRated( // calculate rated odor hour frequency according to GIRL2008
    float hs,       // odor hour frequency for sum of odor components
    int na,         // number of additional (rated) odor components
    float *fa,      // rating factors
    float *fr)      // odor hour frequency for rated component
{
  float hh[TIP_ADDODOR], ff[TIP_ADDODOR], sh, sg, sf, hb, g;
  int i;
  //
  if (na < 1)
    return hs;
  sh = 0;
  for (i=0; i<na; i++) {
    hh[i] = fr[na-i-1];
    ff[i] = fa[na-i-1];
    sh += hh[i];
  }
  sg = hh[0];
  sf = ff[0]*sg;
  for (i=1; i<na; i++) {
    g = MIN(hh[i], hs - sg);
    sg += g;
    sf += ff[i]*g;
  }
  hb = (sg > 0) ? sf*hs/sg : hs;
  if (hb > 100)  hb = 100;                                        //-2008-10-27
  return hb;
  /*
    double g1=0, g2=0, g3=0, g4=0, sg=0;
    double h1=0, h2=0, h3=0, h4=0, sh=0;
    double f1=0, f2=0, f3=0, f4=0;
    double hb=h;
    if (fa[3] > 0) { h1 = *(float*)AryPtr(&aa[3], i, j, k);  f1 = fa[3]; }
    if (fa[2] > 0) { h2 = *(float*)AryPtr(&aa[2], i, j, k);  f2 = fa[2]; }
    if (fa[1] > 0) { h3 = *(float*)AryPtr(&aa[1], i, j, k);  f3 = fa[1]; }
    if (fa[0] > 0) { h4 = *(float*)AryPtr(&aa[0], i, j, k);  f4 = fa[0]; }
    sh = h1 + h2 + h3 + h4;
    g1 = h1;
    g2 = MIN(h2, h - g1);
    g3 = MIN(h3, h - g1 - g2);
    g4 = MIN(h4, h - g1 - g2 - g3);
    sg = g1 + g2 + g3 + g4;
    if (sg > 0)  hb = (h/sg)*(f1*g1 + f2*g2 + f3*g3 + f4*g4);
    *(float*)AryPtr(&ax, i, j, k) = hb;
  */
}

//================================================================= TutEvalOdor
//                                                                //-2008-03-10
int TutEvalOdor(    // evaluate rated odor components
    char *path,     // working directory
    int na,         // number of additional odor components
    char *names[],  // component names
    char *dst,      // name of the resulting component
    int nn)         // number of grids
{
  dP(TutEvalOdor);
  char fn[256], s[256], grdx[32], *pc, *name;
  ARYDSC ao, aa[TIP_ADDODOR], am, ax;                             //-2008-04-30
  float fa[TIP_ADDODOR], fr[TIP_ADDODOR];                         //-2008-09-24
  int i, i1, i2, j, j1, j2, k, k1, k2, n, ia;
  int scatter=1;                                                  //-2008-09-24
  float h, hb;                            //-2008-09-24
  TXTSTR usr = { NULL, 0 };
  TXTSTR sys = { NULL, 0 };
  if (!path || !names || !dst || !*dst)
    return 0;
  if (na != TIP_ADDODOR)                                            eX(9); //-2008-09-29
  for (n=0; n<nn; n++) {
    memset(&ao, 0, sizeof(ARYDSC));
    memset(&am, 0, sizeof(ARYDSC));
    memset(&ax, 0, sizeof(ARYDSC));
    for (ia=0; ia<na; ia++) {
      memset(&aa[ia], 0, sizeof(ARYDSC));
      fa[ia] = 0;
    }
    if (nn == 1)  *grdx = 0;
    else sprintf(grdx, "%02d", n+1);
    sprintf(fn, "%s/%s-%s00%s%s.dmna", path, names[0], CfgYearString, CfgDevString, grdx); //-2008-10-06
    if (!TutFileExists(fn))                                       //-2008-09-24
      scatter = 0;                                                //-2008-09-24
    sprintf(fn, "%s/%s-%s00%s%s.dmna", path, names[0], CfgYearString, CfgAddString, grdx); //-2008-10-06
    if (!TutFileExists(fn))                                         eX(1);
    DmnRead(fn, &usr, &sys, &ao);                                   eG(2);
    if (ao.numdm != 3)                                              eX(3);
    i1 = ao.bound[0].low;
    i2 = ao.bound[0].hgh;
    j1 = ao.bound[1].low;
    j2 = ao.bound[1].hgh;
    k1 = ao.bound[2].low;
    k2 = ao.bound[2].hgh;
    for (ia=0; ia<na; ia++) {
      name = names[1 + ia];
      sprintf(fn, "%s/%s-%s00%s%s.dmna", path, name, CfgYearString, CfgAddString, grdx);	//-2008-10-06
      if (!TutFileExists(fn))
        continue;
      pc = strchr(name, '_');
      if (!pc || 1 != sscanf(pc+1, "%f", &fa[ia]))
        continue;
      fa[ia] /= 100;
      DmnRead(fn, &usr, &sys, &aa[ia]);                               eG(4);
      if (aa[ia].numdm != 3)                                          eX(5);
      if (i1 != aa[ia].bound[0].low || i2 != aa[ia].bound[0].hgh ||
          j1 != aa[ia].bound[1].low || j2 != aa[ia].bound[1].hgh ||
          k1 != aa[ia].bound[2].low || k2 != aa[ia].bound[2].hgh)     eX(6);
    }
    AryCreate(&am, sizeof(float), 3, i1,i2, j1,j2, k1,k2);            eG(7);
    AryCreate(&ax, sizeof(float), 3, i1,i2, j1,j2, k1,k2);            eG(7);
    for (k=k1; k<=k2; k++) {
      for (j=j1; j<=j2; j++) {
        for (i=i1; i<=i2; i++) {
          h = *(float*)AryPtr(&ao, i, j, k);
          if (h <= 0)
            continue;
          for (ia=0; ia<na; ia++) {                               //-2008-09-24
            if (fa[ia] > 0) {
              float *pf;
              pf = AryPtr(&aa[ia], i, j, k);
              if (pf == NULL) {
                printf("TUT null pointer: %d, %d, %d, %d\n", ia, i, j, k);
              }
              fr[ia] = *(float*)AryPtr(&aa[ia], i, j, k);
            }
            else
              fr[ia] = 0;
          }
          hb = TutCalcRated(h, na, fa, fr);                       //-2008-09-24
          *(float*)AryPtr(&ax, i, j, k) = hb;
        }
      }
    }
    sprintf(s, "\"%s_%s\"", eMODn, TALutiVersion);
    DmnRplValues(&usr, "prgm", s);                                //-2009-02-03
    sprintf(s, "\"%s-%s00%s%s\"", dst, CfgYearString, CfgAddString, grdx);	//-2008-10-06
    DmnRplValues(&usr, "file", s);
    sprintf(s, "\"%s\"", dst);
    for (i=0; i<strlen(s); i++)  s[i] = toupper(s[i]);
    DmnRplValues(&usr, "name", s);
    DmnRplValues(&usr, "form", "\"frq%5.1f\"");
    DmnRplValues(&usr, "sequ", "\"k+,j-,i+\"");
    DmnRplValues(&usr, "mode", "\"text\"");
    DmnRplValues(&usr, "cmpr", "0");
    sprintf(fn, "%s/%s-%s00%s%s.dmna", path, dst, CfgYearString, CfgAddString, grdx);	//-2008-10-06
    DmnWrite(fn, usr.s, &ax);                                       eG(8);  //-2008-04-23
    //
    AryFree(&ax);
    AryFree(&ao);
    AryFree(&am);
    for (ia=0; ia<na; ia++) {
      AryFree(&aa[ia]);
    }
    TxtClr(&usr);
    TxtClr(&sys);
  } // for n
  return 0;
  //
eX_1: eX_2: eX_3: eX_4: eX_5: eX_6: eX_7: eX_8: eX_9:
  eMSG(_error_odor_);
}

//======================================================================= TutLogMax
//
int TutLogMax(    // write maximum values into log-file
char *path,       // working directory
char *file,       // file name base
int k,            // layer index
int ii,           // sequence count
int nn )          // number of grids
{
  dP(TutLogMax);
  char fn[256], species[40], type[40], sep[80], grdx[32];
  ARYDSC a, b;
  int i, i1, i2, j, j1, j2, k1, k2, nz, n;
  int i1s, i2s, j1s, j2s;                                 //-2004-10-20
  int imax=0, jmax=0, nmax=0, border=0, odor=0;
  char *pc, *_artp, *_unit, *_name, *_vldf, *_form, *_file;
  float vmax=0, zz[101], xmin=0, ymin=0, delta=1, v, x=0, y=0, *pf, dev;
  TXTSTR usr = { NULL, 0 };
  TXTSTR sys = { NULL, 0 };
  FILE *msg;
  if (!path || !file || !*file)
    return 0;
  msg = (MsgFile) ? MsgFile : stdout;
  memset(&a, 0, sizeof(ARYDSC));
  memset(&b, 0, sizeof(ARYDSC));
  strcpy(species, file);
  pc = strchr(species, '-');  if (!pc)                      eX(1);
  *pc = 0;
  strcpy(type, pc+1);
  for (pc=species; (*pc); pc++)
    *pc = toupper(*pc);
  for (pc=type; (*pc); pc++)
    *pc = toupper(*pc);
  odor = (0 == strncmp(species, "ODOR", 4));                      //-2008-03-10
  //
  for (n=0; n<nn; n++) {
    if (nn == 1)  *grdx = 0;
    else sprintf(grdx, "%02d", n+1);
    sprintf(fn, "%s/%s%s%s.dmna", path, file, CfgAddString, grdx); //-2006-10-31
    if (!TutFileExists(fn)) {
      fprintf(msg, "%-8s %3s : %s\n", species, type, _n_a_);      //-2008-12-19
      return -1;
    }
    DmnRead(fn, &usr, &sys, &a);                              eG(2);
    if (a.numdm == 2) {
      k1 = 0;                                                     //-2011-11-25
      k2 = 0;
    }
    else if (a.numdm == 3) {
      k1 = 1;
      k2 = a.bound[2].hgh;
    }
    else                                                      eX(3);
    if (k<k1 || k>k2)
      goto finish;
    i1 = a.bound[0].low;
    i2 = a.bound[0].hgh;
    j1 = a.bound[1].low;                                      //-2001-06-11 uj
    j2 = a.bound[1].hgh;
    if (n == nn-1) {                                           //-2004-10-20 lj
      i1s = i1;
      i2s = i2;
      j1s = j1;
      j2s = j2;
    }
    else {
      i1s = i1+2;
      i2s = i2-2;
      j1s = j1+2;
      j2s = j2-2;
    }
    DmnGetString(usr.s, "artp", &_artp, 1);
    DmnGetString(usr.s, "name", &_name, 1);
    DmnGetString(usr.s, "unit", &_unit, 1);
    DmnGetString(usr.s, "vldf", &_vldf, 1);
    DmnGetString(sys.s, "form", &_form, 1);
    DmnGetString(sys.s, "file", &_file, 1);
    DmnGetFloat(usr.s, "xmin", "%f", &xmin, 1);
    DmnGetFloat(usr.s, "ymin", "%f", &ymin, 1);
    DmnGetFloat(usr.s, "delta", "%f", &delta, 1);
    nz = DmnGetFloat(usr.s, "sk", "%f", zz, 101);
    if (n == 0) {
      imax = i1s;
      jmax = j1s;
      nmax = 0;
    }
    for (i=i1s; i<=i2s; i++)                                    //-2004-10-20
      for (j=j1s; j<=j2s; j++) {                                //-2004-10-20
        v = *(float*) AryPtrX(&a, i, j, k);
        if (v > vmax) {
          vmax = v;
          imax = i;
          jmax = j;
          border =  (i<=i1+1 || i>=i2-1 || j<=j1+1 || j>=j2-1);
          if (_vldf && *_vldf=='V') {
            x = xmin + (0.5 + imax - i1)*delta;
            y = ymin + (0.5 + jmax - j1)*delta;
          }
          else {
            x = xmin + (imax-i1)*delta;
            y = ymin + (jmax-j1)*delta;
          }
          nmax = n;
        }
      }
    if (n == nmax) {
      sprintf(fn, "%s/%s%s%s.dmna", path, file, CfgDevString, grdx); //-2006-10-31
      if (TutFileExists(fn)) {
        DmnRead(fn, &usr, &sys, &b);                              eG(4);
        pf = AryPtr(&b, imax, jmax, k);
      }
      else pf = NULL;
      dev = (pf) ? *pf : -1;                                      //-2001-10-22
      if (odor) {                                                 //-2011-06-03
        if (dev > 99.9)
         dev = 99.9;
      }
      else {
        if (dev > 0.999)
         dev = 0.999;
      }
    }
    if (n >= nn-1) {
      if (ii==0 && !odor) {
        fprintf(msg, "%s", _maximum_values_t_);
        strcpy(sep,  _maximum_values_u_);
        if (_artp) {
          if (!strcmp(_artp, "C") || *_artp=='M') {
            fprintf(msg, "%s", _concentration_t_);
            strcat(sep,  _concentration_u_);
            if (k>0 && k<=nz) {
              fprintf(msg, _at_z_$_t_, 0.5*zz[k]+zz[k-1]);
              strcat(sep,  _at_z_$_u_);
            }
          }
          else if (*_artp == 'X') {                               //-2011-12-15
            fprintf(msg, "%s", _deposition_t_);
            strcat(sep,  _deposition_u_);
          }
        }
        fprintf(msg, "\n%s\n", sep);
      }
      if (ii==0 && odor) {                                        //-2008-03-10
        fprintf(msg, "%s", _maximum_odor_t_);
        strcpy(sep,  _maximum_odor_u_);
        if (k>0 && k<=nz) {
          fprintf(msg, _at_z_$_t_, 0.5*zz[k]+zz[k-1]);
          strcat(sep,  _at_z_$_u_);
        }
        fprintf(msg, "\n%s\n", sep);
      }
      fprintf(msg, "%-8s %3s : ", species, type);                 //-2008-03-10
      if (_form) {
        pc = strchr(_form, '%');
        fprintf(msg, pc, vmax);
      }
      else  fprintf(msg, "%10.3e", vmax);
      if (_unit)  fprintf(msg, " %-5s", _unit);
      if (odor) {
        if (dev >= 0)  fprintf(msg, " (+/- %4.1f )", dev);        //-2011-06-20
        else           fprintf(msg, " (+/-  ?   )");
      }
      else {
        if (dev >= 0)  fprintf(msg, " (+/- %4.1f%%)", 100*dev);   //-2001-10-28
        else           fprintf(msg, " (+/-  ?  %%)");
      }
      if (vmax > 0) {                                             //-2002-05-03
        fprintf(msg, " %s x=%5.0f m, y=%5.0f m (", _at_, x, y);   //-2008-07-30
        if (nn > 1)  fprintf(msg, "%d:", nmax+1);                 //-2001-11-02
        fprintf(msg, "%3d,%3d)", imax, jmax);
        if (border)  fprintf(msg, "%s", _border_);                //-2011-11-23
      }
      fprintf(msg, "\n");
    }
    if (_artp)  FREE(_artp);
    if (_file)  FREE(_file);
    if (_form)  FREE(_form);
    if (_vldf)  FREE(_vldf);
    if (_name)  FREE(_name);
    if (_unit)  FREE(_unit);
  }
finish:
  AryFree(&a);
  AryFree(&b);
  TxtClr(&usr);
  TxtClr(&sys);
  return 0;
eX_1:
  eMSG(_strange_filename_$_, file);
eX_2: eX_4:
  eMSG(_cant_read_file_$_, fn);
eX_3:
  eMSG(_improper_dimension_$_, fn);
}

//======================================================================= TutLogMon
//
int TutLogMon(    // write values at monitor points into log-file
char *path,       // working directory
char *file,       // file name base
int prec,         // number of decimal positions
int ii )          // sequence count
{
  dP(TutLogMon);
  char fn[256], species[40], type[40], grdx[32], format[32], units[32];
  ARYDSC ac, as;                                                  //-2008-07-30
  int i, i1, i2, j, j1, j2, k, k1, k2, nz, n, ng, ip, np;
  int nparts=0;                                                   //-2002-08-28
  int scatter = 1;                                                //-2008-07-30
  float *values=NULL;
  char *pc, *_artp, *_unit, *_name, *_vldf, *_form, *_file;
  float zz[101], xmin=0, ymin=0, delta=1, s=0;               //-2008-07-30
  TXTSTR usr = { NULL, 0 };
  TXTSTR sys = { NULL, 0 };
  FILE *f;
  static int odor_scatter = 0;                                    //-2008-10-27
  int odor = 0;                                                   //-2008-10-27
  int insert_scatter = 0;                                         //-2008-10-27
  if (!path || !file || !*file || TI.np<1)
    return 0;
  f = (MsgFile) ? MsgFile : stdout;
  memset(&ac, 0, sizeof(ARYDSC));
  memset(&as, 0, sizeof(ARYDSC));                                 //-2008-07-30
  //
  ng = TI.nn;                                                     //-2008-08-25
  for (n=0; n<ng; n++) {                                          //-2008-07-30
    if (ng == 1)  *grdx = 0;
    else sprintf(grdx, "%02d", n+1);
    sprintf(fn, "%s/%s%s%s.dmna", path, file, CfgDevString, grdx);
    if (!TutFileExists(fn))
      scatter = 0;
  }
  if (!strncmp(file, "odor-", 5))  odor_scatter= scatter;         //-2008-10-27
  odor = !strncmp(file, "odor", 4);                               //-2008-10-27
  insert_scatter = scatter || (odor && odor_scatter);             //-2008-10-27
  //
  np = TI.np;
  values = ALLOC(2*np*sizeof(float));                             //-2008-07-30
  for (ip=0; ip<2*np; ip++)  values[ip] = -1;                     //-2008-10-06
  if (prec < 0)  strcpy(format, "  %10.3e");
  else           sprintf(format, "  %%10.%df", prec);
  *units = 0;
  if (ii < 1) {
    fprintf(f, "%s", _eval_additional_);
    fprintf(f, "%s", _sepa_additional_);
    fprintf(f,"%-12s",  _point_);																  //-2008-04-17
    for (ip=0; ip<np; ip++) {
      if (insert_scatter) fprintf(f, "      ");                   //-2008-10-27
      fprintf(f, "          %02d", ip+1);
    }
    fprintf(f, "\n");
    fprintf(f, "%-12s", "xp");																	  //-2008-04-17
    for (ip=0; ip<np; ip++) {
      if (scatter) fprintf(f, "      ");                          //-2008-07-30
      fprintf(f, "  %10.0f", TI.xp[ip]);
    }
    fprintf(f, "\n");
    fprintf(f, "%-12s", "yp");																	  //-2008-04-17
    for (ip=0; ip<np; ip++) {
      if (insert_scatter) fprintf(f, "      ");                   //-2008-10-27
      fprintf(f, "  %10.0f", TI.yp[ip]);
    }
    fprintf(f, "\n");
    fprintf(f, "%-12s", "hp");																	  //-2008-04-17
    for (ip=0; ip<np; ip++) {
      if (insert_scatter) fprintf(f, "      ");                   //-2008-10-27
      fprintf(f, "  %10.1f", TI.hp[ip]);
    }
    fprintf(f, "\n");
    nparts++;
  }
  if (ii < 2) {
    fprintf(f, "------------");																	  //-2008-04-17
    for (ip=0; ip<np; ip++) {
      fprintf(f, "+-----------");
      //if (prec < 0)  fprintf(f, "----");
      if (insert_scatter) fprintf(f, "------");                   //-2008-10-27
    }
    fprintf(f, "\n");
    nparts++;
  }
  //
  strcpy(species, file);
  pc = strchr(species, '-');  if (!pc)                      eX(1);
  *pc = 0;
  strcpy(type, pc+1);
  for (pc=species; (*pc); pc++)
    *pc = toupper(*pc);
  for (pc=type; (*pc); pc++)
    *pc = toupper(*pc);
  //
  fprintf(f, "%-8s %3s", species, type);                          //-2008-03-10
  for (n=0; n<ng; n++) {
    if (ng == 1)  *grdx = 0;
    else sprintf(grdx, "%02d", n+1);
    sprintf(fn, "%s/%s%s%s.dmna", path, file, CfgAddString, grdx); //-2006-10-31
    if (!TutFileExists(fn)) {
      fprintf(f, " %s\n", _n_a_);                                 //-2008-12-19
      goto finish;
    }
    DmnRead(fn, &usr, &sys, &ac);                              eG(2);
    if (ac.numdm == 2) {
      k1 = k2 = 0;
    }
    else if (ac.numdm == 3) {
      k1 = 1;
      k2 = ac.bound[2].hgh;
    }
    else                                                      eX(3);
    i1 = ac.bound[0].low;
    i2 = ac.bound[0].hgh;
    j1 = ac.bound[1].low;
    j2 = ac.bound[1].hgh;
    DmnGetString(usr.s, "artp", &_artp, 1);
    DmnGetString(usr.s, "name", &_name, 1);
    DmnGetString(usr.s, "unit", &_unit, 1);
    DmnGetString(usr.s, "vldf", &_vldf, 1);
    DmnGetString(sys.s, "form", &_form, 1);
    DmnGetString(sys.s, "file", &_file, 1);
    DmnGetFloat(usr.s, "xmin", "%f", &xmin, 1);
    DmnGetFloat(usr.s, "ymin", "%f", &ymin, 1);
    DmnGetFloat(usr.s, "delta", "%f", &delta, 1);
    nz = DmnGetFloat(usr.s, "sk", "%f", zz, 101);
    if (nz < k2)  k2 = nz;
    if (scatter) {                                                //-2008-07-30
      sprintf(fn, "%s/%s%s%s.dmna", path, file, CfgDevString, grdx); //-2006-10-31
      DmnRead(fn, &usr, &sys, &as);                              eG(2);
    }
    for (ip=0; ip<np; ip++) {
      if (values[2*ip] >= 0)                                      //-2008-07-30
        continue;
      i = i1 + (TI.xp[ip] - xmin)/delta;
      j = j1 + (TI.yp[ip] - ymin)/delta;
      if (ac.numdm == 3) {                                        //-2011-12-06
        for (k=k1; k<=k2; k++)
          if (TI.hp[ip] <= zz[k])
            break;
      }
      else  k = 0;
      if (i>=i1 && i<=i2 && j>=j1 && j<=j2 && k>=k1 && k<=k2) {
        values[2*ip] = *(float*) AryPtrX(&ac, i, j, k);           //-2008-07-30
        if (scatter)                                              //-2008-07-30
          values[2*ip+1] = *(float*) AryPtrX(&as, i, j, k);
        strcpy(units, _unit);
      }
    }
    if (_artp)  FREE(_artp);
    if (_file)  FREE(_file);
    if (_form)  FREE(_form);
    if (_vldf)  FREE(_vldf);
    if (_name)  FREE(_name);
    if (_unit)  FREE(_unit);
  }
  for (ip=0; ip<np; ip++)
    if (values[2*ip] < 0) {                                       //-2008-09-24
      fprintf(f, "         ---");
      if (insert_scatter)  fprintf(f, "   ---");                  //-2008-10-27
    }
    else {
      fprintf(f, format, values[2*ip]);
      if (odor && insert_scatter) {                               //-2008-10-27
        s = values[2*ip+1];                                       //-2008-12-04
        if (scatter) {
          if (s > 99.9)
            fprintf(f, "  100 ");
          else
            fprintf(f, " %4.1f ", s);
        }
        else
          fprintf(f, "   -- ");
      }
      else {
        s = 100*values[2*ip+1];                                   //-2008-07-30
        if (scatter) {                                            //-2008-07-30
          if (s > 99.9)
            fprintf(f, "  100%%");
          else
            fprintf(f, " %4.1f%%", s);
        }
        else                                                      //-2012-04-06
          fprintf(f, "   -- ");
      }
    }
  fprintf(f, "  %s\n", units);
  nparts++;
finish:
  AryFree(&ac);
  AryFree(&as);
  TxtClr(&usr);
  TxtClr(&sys);
  if (values) FREE(values);
  return nparts;
eX_1:
  eMSG(_strange_filename_$_, file);
eX_2:
  eMSG(_cant_read_file_$_, fn);
eX_3:
  eMSG(_improper_dimension_$_, fn);
}

//=============================================================================
//
// history:
//
// 2002-02-25 lj 0.10.0 extracted from TALutils
// 2002-03-28 lj 0.11.0 adapted to new vMsg()
// 2002-05-03 lj 0.12.1 coordinates of maximum only for vmax>0 printed
// 2002-07-13 lj 0.13.1 warning if maximum near border
// 2002-08-28 lj 0.13.2 adapted to TalMon (nparts)
// 2002-09-24 lj 1.0.0  final release candidate
// 2004-06-10 lj 2.0.1  output of odor hour probability
// 2004-10-20 lj 2.1.4  boundary values (2 cells) of inner grids discarded
// 2005-03-17 uj 2.2.0  version number upgrade
// 2006-10-26 lj 2.3.0  external strings
// 2006-10-31 lj 2.3.1  configuration data
// 2008-03-10 lj 2.4.0  evaluation of rated odor frequencies
// 2008-04-17 lj 2.4.1  merged with 2.3.x
// 2008-04-23 lj        IBJ-mode as standard for odor rating
// 2008-04-30 lj        fixed dimensions
// 2008-07-28 lj 2.4.2  external strings
// 2008-07-30 lj        logging scatter
// 2008-09-24 lj 2.4.3  update for rated odor frequencies (TutCalcRated())
// 2008-09-29 uj        fixed array bounds
// 2008-10-06 uj        corrections
// 2008-10-27 lj 2.4.4  rated odor hour frequency limited to <=100%
// 2008-10-27 lj        odor values at monitor points with AKS
// 2008-12-04 lj 2.4.5  odor scatter at monitor points with AKS corrected
// 2008-12-19 lj 2.4.6  "n.v." internationalized
// 2009-02-03 uj        "pgm" renamed in "prgm"
// 2011-06-03 uj 2.4.10 upper bound of dev corrected for odor
// 2011-06-20           log odor scatter with one decimal position
// 2011-07-07 uj 2.5.0  version number upgrade
// 2011-11-23 lj 2.6.0  cosmetics
// 2011-12-15 lj        total deposition
// 2012-04-06 uj        cosmetics
//
//=============================================================================
