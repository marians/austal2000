// ====================================================== verif00.c
//
// Evaluation of verification 00
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
#include "verif00.nls"

static char *Path = "verif/00";
static char FileCnc[64];
static char FileDev[64];
static int Nx=50, Ny=50, Nz=1;
static double Cmean = 1800;

int main( int argc, char *argv[] ) {
  char fn[256], buf[4000]="", *pc, name[256];
  int i, j, n;
  double c, d, sumv, sumq, cnc, dev, sct;
  FILE *f;
  if (init(argc, argv, "VRF") < 0)                                //-2008-09-30
    vMsg(_problems_nls_, NlsLanguage);                            //-2008-09-30
  sprintf(FileCnc, "xx-%%03d%s.dmna", CfgAddString);
  sprintf(FileDev, "xx-%%03d%s.dmna", CfgDevString);

  fprintf(Out, "%s", _title_);
  fprintf(Out, "%s", _header_);

  for (n=1; n<=10; n++) {

  sprintf(name, FileCnc, n);
  sprintf(fn, "%s/%s", Path, name);
  f = fopen(fn, "r");                           // open file
  if (!f) {
    vMsg(_cant_read_$_, fn);                                      //-2008-09-30
    exit(1);
  }
  while(fgets(buf, 4000, f)) {                  // ommit header
    if (!strncmp(buf,"hghb", 4))  sscanf(buf+4, "%d%d%d", &Nx, &Ny, &Nz);
    if (*buf == '*')
      break;
  }
  if (*buf != '*') {
    vMsg(_data_not_found_$_, name);                               //-2008-09-30
    exit(2);
  }
  sumv = 0;
  sumq = 0;
  for (j=Ny; j>0; j--) {
    if (!fgets(buf, 4000, f)) {                 // read lines
      vMsg(_eof_$_, j);                                           //-2008-09-30
      exit(3);
    }
    pc = strtok(buf, " \t\r\n");
    for (i=1; i<=Nx; i++) {
      if (pc) {
        if (1 != sscanf(pc, "%lf", &c))  pc = NULL;
      }
      if (!pc) {
        vMsg(_cant_read_$$_, i, j);                               //-2008-09-30
        exit(4);
      }
      c *= 1.e6;
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
  sprintf(name, FileDev, n);
  sprintf(fn, "%s/%s", Path, name);
  f = fopen(fn, "r");                           // open file
  if (!f) {
    vMsg(_cant_read_$_, name);                                    //-2008-09-30
    exit(1);
  }
  while(fgets(buf, 4000, f)) {                  // ommit header
    if (!strncmp(buf,"hghb", 4))  sscanf(buf+4, "%d%d%d", &Nx, &Ny, &Nz);
    if (*buf == '*')  break;
  }
  if (*buf != '*') {
    vMsg(_data_not_found_$_, fn);                                 //-2008-09-30
    exit(2);
  }
  sumq = 0;
  for (j=Ny; j>0; j--) {
    if (!fgets(buf, 4000, f)) {                 // read lines
      vMsg(_eof_$_, j);                                           //-2008-09-30
      exit(3);
    }
    pc = strtok(buf, " \t\r\n");
    for (i=1; i<=Nx; i++) {
      if (pc) {
        if (1 != sscanf(pc, "%lf", &d))  pc = NULL;
      }
      if (!pc) {
        vMsg(_cant_read_$$_, i, j);                               //-2008-09-30
        exit(4);
      }
      sumq += d*d;
      pc = strtok(NULL, " \t\r\n");
    }
  }
  fclose(f);
  f = NULL;
  dev = sqrt(sumq/(Nx*Ny));
  fprintf(Out, "%2d %9.1lf %9.1f %9.1f\n", n, cnc, sct, dev);     //-2008-09-30

  }  // for n

  fprintf(Out, "\n");                                             //-2008-09-30
  vMsg("@verif00  finished");                                     //-2008-09-30
  return 0;
}
