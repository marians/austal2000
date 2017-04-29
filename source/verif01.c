// ====================================================== verif01.c
//
// Evaluation of verification 01
// =============================
//
// Copyright (C) Umweltbundesamt, Dessau-Roﬂlau, Germany, 2008
// Copyright (C) Janicke Consulting, 88662 ‹berlingen, Germany, 2008
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
// last change: 2008-09-30 lj
//
//================================================================

#include <string.h>
#include <stdio.h>
#include <math.h>
#include <stdlib.h>

#include "verif.h"                                                //-2008-09-30
#include "verif01.nls"

static char *Path = "verif/01";
static char FileCncVal[64];
static char FileCncDev[64];
static char FileCncZtr[64];
static char FileOdrVal[64];
static char FileOdrDev[64];
static char FileOdrZtr[64];
static int Nx=10, Ny=10, Nz=1, Np=10, Nt=240;
static double Cmean = 1800;
static double Cnc[240][10], Frq[240][10];

int main( int argc, char *argv[] ) {
  char fn[256], buf[4000]="", *pc, name[256];
  int i, j, n;
  double c, d, h, sumv, sumq, cnc, dev, sct, frq, nh;
  FILE *f;
  if (init(argc, argv, "VRF") < 0)                                //-2008-09-30
    vMsg(_problems_nls_, NlsLanguage);                            //-2008-09-30
  sprintf(FileCncVal, "xx-%%03d%s.dmna", CfgAddString);
  sprintf(FileCncDev, "xx-%%03d%s.dmna", CfgDevString);
  sprintf(FileCncZtr, "xx-%s.dmna", CfgMntAddString);
  sprintf(FileOdrVal, "odor-%%03d%s.dmna", CfgAddString);
  sprintf(FileOdrDev, "odor-%%03d%s.dmna", CfgDevString);
  sprintf(FileOdrZtr, "odor-%s.dmna", CfgMntAddString);

  fprintf(Out, "%s", _title1_);                                   //-2008-09-30
  fprintf(Out, "%s", _title2_);                                   //-2008-09-30
  fprintf(Out, "%s", _header_);                                   //-2008-09-30

  for (n=1; n<=10; n++) { //---------------------------------------------------

  sprintf(name, FileCncVal, n);
  sprintf(fn, "%s/%s", Path, name);
  f = fopen(fn, "r");
  if (!f) {
    vMsg(_cant_read_$_, fn);                                      //-2008-09-30
    exit(1);
  }
  while(fgets(buf, 4000, f)) {
    if (!strncmp(buf,"hghb", 4))  sscanf(buf+4, "%d%d%d", &Nx, &Ny, &Nz);
    if (*buf == '*')  break;
  }
  if (*buf != '*') {
    vMsg(_data_not_found_$_, name);                               //-2008-09-30
    exit(2);
  }
  sumv = 0;
  sumq = 0;
  for (j=Ny; j>0; j--) {
    if (!fgets(buf, 4000, f)) {
      vMsg(_eof_$$_, fn, j);                                      //-2008-09-30
      exit(3);
    }
    pc = strtok(buf, " \t\r\n");
    for (i=1; i<=Nx; i++) {
      if (pc) {
        if (1 != sscanf(pc, "%lf", &c))  pc = NULL;
      }
      if (!pc) {
        vMsg(_cant_read_$$$_, i, j, fn);                          //-2008-09-30
        exit(4);
      }
      // c *= 1.e6;
      sumv += c;
      sumq += c*c;
      pc = strtok(NULL, " \t\r\n");
    }
  }
  fclose(f);
  f = NULL;
  sumv /= Nx*Ny;
  sumq /= Nx*Ny;
  cnc = sumv;
  sct = 100*sqrt(sumq - sumv*sumv)/cnc;
  //
  sprintf(name, FileCncDev, n);
  sprintf(fn, "%s/%s", Path, name);
  f = fopen(fn, "r");
  if (!f) {
    vMsg(_cant_read_$_, name);                                    //-2008-09-30
    exit(5);
  }
  while(fgets(buf, 4000, f)) {
    if (!strncmp(buf,"hghb", 4))  sscanf(buf+4, "%d%d%d", &Nx, &Ny, &Nz);
    if (*buf == '*')  break;
  }
  if (*buf != '*') {
    vMsg(_data_not_found_$_, fn);                                 //-2008-09-30
    exit(6);
  }
  sumq = 0;
  for (j=Ny; j>0; j--) {
    if (!fgets(buf, 4000, f)) {
      vMsg(_eof_$$_, fn, j);                                      //-2008-09-30
      exit(7);
    }
    pc = strtok(buf, " \t\r\n");
    for (i=1; i<=Nx; i++) {
      if (pc) {
        if (1 != sscanf(pc, "%lf", &d))  pc = NULL;
      }
      if (!pc) {
        vMsg(_cant_read_$$$_, i, j, fn);                          //-2008-09-30
        exit(8);
      }
      sumq += d*d;
      pc = strtok(NULL, " \t\r\n");
    }
  }
  fclose(f);
  f = NULL;
  dev = sqrt(sumq/(Nx*Ny));
  fprintf(Out, "%2d %9.6lf %9.2f %9.2f\n", n, cnc, sct, dev);     //-2008-09-30

  }  // for n -------------------------------------------------

  fprintf(Out, "\n");                                             //-2008-09-30

  //============================================================================

  sprintf(fn, "%s/%s", Path, FileCncZtr);
  f = fopen(fn, "r");
  if (!f) {
    vMsg(_cant_read_$_, fn);                                      //-2008-09-30
    exit(1);
  }
  while(fgets(buf, 4000, f)) {
    int nt=0, np=0;
    if (!strncmp(buf,"hghb", 4)) {
      sscanf(buf+4, "%d%d", &nt, &np);
      if (nt!=Nt || np!=Np) {
        vMsg(_improper_dimensions_$_, fn);                        //-2008-09-30
        exit(3);
      }
    }
    if (*buf == '*')  break;
  }
  if (*buf != '*') {
    vMsg(_data_not_found_$_, name);                               //-2008-09-30
    exit(4);
  }
  for (j=0; j<Nt; j++) {
    if (!fgets(buf, 4000, f)) {
      vMsg(_eof_$$_, fn, j);                                      //-2008-09-30
      exit(3);
    }
    pc = strtok(buf, " \t\r\n");
    for (i=0; i<Np; i++) {
      if (pc) {
        if (1 != sscanf(pc, "%lf", &c))  pc = NULL;
      }
      if (!pc) {
        vMsg(_cant_read_$$$_, i, j, fn);                          //-2008-09-30
        exit(4);
      }
      Cnc[j][i] = c;
      pc = strtok(NULL, " \t\r\n");
    }
  }
  fclose(f);
  f = NULL;
  //
  fprintf(Out, "%s", _time_series_);                              //-2008-09-30
  fprintf(Out, "%s", _mean_observed_);                            //-2008-09-30
  for (i=0; i<Np; i++) {
    fprintf(Out, "%2d:", i+1);                                    //-2008-09-30
    sumv = 0;
    sumq = 0;
    for (j=24; j<Nt; j++) {
      c = Cnc[j][i];
      sumv += c;
      sumq += c*c;
    }
    sumv /= Nt-24;
    sumq /= Nt-24;
    cnc = sumv;
    sct = 100*sqrt(sumq - sumv*sumv)/cnc;
    fprintf(Out, "%9.5lf %9.3lf\n", cnc, sct);                    //-2008-09-30
  }
  fprintf(Out, "\n");                                             //-2008-09-30

  //================================ Geruch ====================================

  fprintf(Out, "%s", _daily_average_);                            //-2008-09-30
  fprintf(Out, "%s", _mean_observed_estimated_);                  //-2008-09-30

  for (n=1; n<=10; n++) { //---------------------------------------------------

  sprintf(name, FileOdrVal, n);
  sprintf(fn, "%s/%s", Path, name);
  f = fopen(fn, "r");
  if (!f) {
    vMsg(_cant_read_$_, fn);                                      //-2008-09-30
    exit(1);
  }
  while(fgets(buf, 4000, f)) {
    if (!strncmp(buf,"hghb", 4))  sscanf(buf+4, "%d%d%d", &Nx, &Ny, &Nz);
    if (*buf == '*')  break;
  }
  if (*buf != '*') {
    vMsg(_data_not_found_$_, name);                               //-2008-09-30
    exit(2);
  }
  sumv = 0;
  sumq = 0;
  for (j=Ny; j>0; j--) {
    if (!fgets(buf, 4000, f)) {
      vMsg(_eof_$$_, fn, j);                                      //-2008-09-30
      exit(3);
    }
    pc = strtok(buf, " \t\r\n");
    for (i=1; i<=Nx; i++) {
      if (pc) {
        if (1 != sscanf(pc, "%lf", &h))  pc = NULL;
      }
      if (!pc) {
        vMsg(_cant_read_$$$_, i, j, fn);                          //-2008-09-30
        exit(4);
      }
      sumv += h;
      sumq += h*h;
      pc = strtok(NULL, " \t\r\n");
    }
  }
  fclose(f);
  f = NULL;
  sumv /= Nx*Ny;
  sumq /= Nx*Ny;
  frq = sumv;
  sct = sqrt(sumq - sumv*sumv);
  //
  sprintf(name, FileOdrDev, n);
  sprintf(fn, "%s/%s", Path, name);
  f = fopen(fn, "r");
  if (!f) {
    vMsg(_cant_read_$_, name);                                    //-2008-09-30
    exit(5);
  }
  while(fgets(buf, 4000, f)) {
    if (!strncmp(buf,"hghb", 4))  sscanf(buf+4, "%d%d%d", &Nx, &Ny, &Nz);
    if (*buf == '*')  break;
  }
  if (*buf != '*') {
    vMsg(_data_not_found_$_, fn);                                 //-2008-09-30
    exit(6);
  }
  sumq = 0;
  for (j=Ny; j>0; j--) {
    if (!fgets(buf, 4000, f)) {
      vMsg(_eof_$$_, fn, j);
      exit(7);
    }
    pc = strtok(buf, " \t\r\n");
    for (i=1; i<=Nx; i++) {
      if (pc) {
        if (1 != sscanf(pc, "%lf", &d))  pc = NULL;
      }
      if (!pc) {
        vMsg(_cant_read_$$$_, i, j, fn);                          //-2008-09-30
        exit(8);
      }
      sumq += d*d;
      pc = strtok(NULL, " \t\r\n");
    }
  }
  fclose(f);
  f = NULL;
  dev = sqrt(sumq/(Nx*Ny));
  fprintf(Out, "%2d %9.2lf %9.2f %9.2f\n", n, frq, sct, dev);     //-2008-09-30

  }  // for n -------------------------------------------------

  fprintf(Out, "\n");                                             //-2008-09-30


  //============================================================================

  sprintf(fn, "%s/%s", Path, FileOdrZtr);
  f = fopen(fn, "r");
  if (!f) {
    vMsg(_cant_read_$_, fn);                                      //-2008-09-30
    exit(1);
  }
  while(fgets(buf, 4000, f)) {
    int nt=0, np=0;
    if (!strncmp(buf,"hghb", 4)) {
      sscanf(buf+4, "%d%d", &nt, &np);
      if (nt!=Nt || np!=Np) {
        vMsg(_improper_dimensions_$_, fn);                        //-2008-09-30
        exit(3);
      }
    }
    if (*buf == '*')  break;
  }
  if (*buf != '*') {
    vMsg(_data_not_found_$_, name);                               //-2008-09-30
    exit(4);
  }
  for (j=0; j<Nt; j++) {
    if (!fgets(buf, 4000, f)) {
      vMsg(_eof_$$_, fn, j);                                      //-2008-09-30
      exit(3);
    }
    pc = strtok(buf, " \t\r\n");
    for (i=0; i<Np; i++) {
      if (pc) {
        if (1 != sscanf(pc, "%lf", &c))  pc = NULL;
      }
      if (!pc) {
        vMsg(_cant_read_$$$_, i, j, fn);                          //-2008-09-30
        exit(4);
      }
      Frq[j][i] = c;
      pc = strtok(NULL, " \t\r\n");
    }
  }
  fclose(f);
  f = NULL;
  //
  fprintf(Out, "%s", _time_series_odor_);                         //-2008-09-30
  fprintf(Out, "%s", _mean_observed_counted_);                    //-2008-09-30
  for (i=0; i<Np; i++) {
    fprintf(Out, "%2d:", i+1);                                    //-2008-09-30
    sumv = 0;
    sumq = 0;
    nh = 0;
    for (j=24; j<Nt; j++) {
      h = Frq[j][i];
      sumv += h;
      sumq += h*h;
      c = Cnc[j][i];
      if (c > 0.25)  nh += 1;
    }
    sumv /= Nt-24;
    sumq /= Nt-24;
    nh   /= Nt-24;
    frq = sumv;
    sct = sqrt(sumq - sumv*sumv);
    fprintf(Out, "%9.2lf %9.2lf %9.2lf\n", frq, sct, 100*nh);     //-2008-09-30
  }
  fprintf(Out, "\n");                                             //-2008-09-30
  vMsg("@verif01  finished");                                     //-2008-09-30
  return 0;
}
