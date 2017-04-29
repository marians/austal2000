// ====================================================== verif23.c
//
// Evaluation of verification 23
// =============================
//
// Copyright (C) Umweltbundesamt, Dessau-Roﬂlau, Germany, 2011-2012
// Copyright (C) Janicke Consulting, 88662 ‹berlingen, Germany, 2011-2012
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
// last change: 2011-12-09 lj
//
//================================================================

#include <string.h>
#include <stdio.h>
#include <math.h>
#include <stdlib.h>

#include "verif.h"                                                //-2008-09-30
#include "verif23.nls"

static char *Path = "verif/23";
static char FileCnc[64], FileWet[64];
static char FileCdv[64], FileWdv[64];
static double Cnc[100], Cdv[100], Wet[100], Wdv[100];
//
static int Nx=25, Ny=5, Nz=1;
static double ri = 10;
static double wf = 1.e-4;
static double we = 0.8;
static double hmax = 250;

static int readLine(char *name, double *values, int dims) {
  FILE *f;
  char fn[512];
  char buf[4000]="", *pc;
  int i, j, nx=0, ny=0, nz=0;
  //
  sprintf(fn, "%s/%s", Path, name);
  f = fopen(fn, "r");
  if (!f) {
    vMsg(_cant_read_$_, fn);
    exit(1);
  }
  while(fgets(buf, 4000, f)) {
    if (!strncmp(buf,"hghb", 4)) {
      sscanf(buf+4, "%d%d%d", &nx, &ny, &nz);
      if (dims == 2)
        nz = Nz;
      if (nx!=Nx || ny!=Ny || nz!=Nz) {
        vMsg(_invalid_structure_$_, fn);
        exit(10);
      }
    }
    if (*buf == '*')
      break;
  }
  if (*buf != '*') {
    vMsg(_data_not_found_$_, fn);
    exit(2);
  }
  for (j=1; j<=(Ny+1)/2; j++) {
    if (!fgets(buf, 4000, f)) {
      vMsg(_eof_$_, j);
      exit(3);
    }
  }
  pc = strtok(buf, " \t\r\n");
  for (i=1; i<=Nx; i++) {
    if (!pc || 1 != sscanf(pc, "%lf", values+i-1))  {
      vMsg(_cant_read_element_$_, i);
      exit(4);
    }
    pc = strtok(NULL, " \t\r\n");
  }
  fclose(f);
  f = NULL;
  return 0;
}

int main( int argc, char *argv[] ) {
  int i;
  double c, d, cmin, cmax, w, wmin, wmax, fac;
  //
  if (init(argc, argv, "VRF") < 0)
    vMsg(_problems_nls_, NlsLanguage);
  sprintf(FileCnc, "xx-%s00%s.dmna", CfgYearString, CfgAddString);
  sprintf(FileCdv, "xx-%s00%s.dmna", CfgYearString, CfgDevString);
  sprintf(FileWet, "xx-wet%s.dmna", CfgAddString);
  sprintf(FileWdv, "xx-wet%s.dmna", CfgDevString);
  //
  fprintf(Out, "%s", _title1_);
  readLine(FileCnc, Cnc, 3);
  readLine(FileCdv, Cdv, 3);
  readLine(FileWet, Wet, 2);
  readLine(FileWdv, Wdv, 2);
  //
  fprintf(Out, "%s", _header_);
  fac = hmax*86400*wf*pow(ri, we);
  for (i=0; i<25; i++) {
    c = Cnc[i]*fac;
    d = 0.01*Cdv[i];
    cmin = c*(1-2*d);
    cmax = c*(1+2*d);
    w = Wet[i];
    d = 0.01*Wdv[i];
    wmin = w*(1-2*d);
    wmax = w*(1+2*d);
    fprintf(Out, "%2d   %7.1lf %7.1lf %7.1lf   %7.1lf %7.1lf %7.1lf\n",
        i+1, cmin, c, cmax, wmin, w, wmax);
  }
  fprintf(Out, "\n");
  vMsg("@verif23 finished");
  return 0;
}
