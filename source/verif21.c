// ====================================================== verif21.c
//
// Evaluation of verification 21
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
#include "verif21.nls"

static char *Path = "verif/21";
static char FileCnc[64];
static char FileDev[64];
static int Nx=1, Ny=1, Nz=20;
static double Cnc[100], Dev[100];

int main( int argc, char *argv[] ) {
  char fn[256], buf[4000]="", *pc, name[256];
  int k, n;
  double c, d, cmin, cmax, z, cth, vd=0.1, kz=1;
  FILE *f;
  if (init(argc, argv, "VRF") < 0)                                //-2008-09-30
    vMsg(_problems_nls_, NlsLanguage);                            //-2008-09-30
  sprintf(FileCnc, "xx-%%03d%s.dmna", CfgAddString);
  sprintf(FileDev, "xx-%%03d%s.dmna", CfgDevString);

  fprintf(Out, "%s", _title1_);                                   //-2008-09-30

  n = 10;

  sprintf(name, FileCnc, n);
  sprintf(fn, "%s/%s", Path, name);
  f = fopen(fn, "r");
  if (!f) {
    vMsg(_cant_read_$_, fn);                                      //-2008-09-30
    exit(1);
  }
  while(fgets(buf, 4000, f)) {
    if (!strncmp(buf,"hghb", 4)) {
      sscanf(buf+4, "%d%d%d", &Nx, &Ny, &Nz);
      if (Nx!=1 || Ny!=1 || Nz>100) {
        vMsg(_invalid_structure_$_, name);                        //-2008-09-30
        exit(10);
      }
    }
    if (*buf == '*')  break;
  }
  if (*buf != '*') {
    vMsg(_data_not_found_$_, name);                               //-2008-09-30
    exit(2);
  }
  for (k=1; k<=Nz; ) {
    if (!fgets(buf, 4000, f)) {
      vMsg(_eof_$_, k);                                           //-2008-09-30
      exit(3);
    }
    pc = strtok(buf, " \t\r\n");
    if (!pc)  continue;
    if (1 != sscanf(pc, "%lf", &c))  {
      vMsg(_cant_read_element_$_, k);                             //-2008-09-30
      exit(4);
    }
    c *= 1.e6;
    Cnc[k-1] = c;
    k++;
  }
  fclose(f);
  f = NULL;
  //
  sprintf(name, FileDev, n);
  sprintf(fn, "%s/%s", Path, name);
  f = fopen(fn, "r");
  if (!f) {
    vMsg(_cant_read_$_, name);                                    //-2008-09-30
    exit(1);
  }
  while(fgets(buf, 4000, f)) {
    if (!strncmp(buf,"hghb", 4)) {
      sscanf(buf+4, "%d%d%d", &Nx, &Ny, &Nz);
      if (Nx!=1 || Ny!=1 || Nz>100) {
        vMsg(_invalid_structure_$_, name);                        //-2008-09-30
        exit(10);
      }
    }
    if (*buf == '*')  break;
  }
  if (*buf != '*') {
    vMsg(_data_not_found_$_, name);                               //-2008-09-30
    exit(2);
  }
  for (k=1; k<=Nz; ) {
    if (!fgets(buf, 4000, f)) {
      vMsg(_eof_$_, k);                                           //-2008-09-30
      exit(3);
    }
    pc = strtok(buf, " \t\r\n");
    if (!pc)  continue;
    if (1 != sscanf(pc, "%lf", &d))  {
      vMsg(_cant_read_element_$_, k);                             //-2008-09-30
      exit(4);
    }
    Dev[k-1] = d;
    k++;
  }
  fclose(f);
  f = NULL;
  fprintf(Out, "%s", _header_);                                   //-2008-09-30
  for (k=0; k<Nz; k++) {
    z = 10*(k+0.5);
    cth = 1/vd + z/kz;
    c = Cnc[k];
    d = 0.01*Dev[k];
    cmin = c*(1-2*d);
    cmax = c*(1+2*d);
    fprintf(Out, "%2d %7.1lf %7.1lf %7.1lf %7.1lf\n",             //-2008-09-30
        k+1, cmin, c, cmax, cth);
  }
  fprintf(Out, "\n");                                             //-2008-09-30
  vMsg("@verif21  finished");                                     //-2008-09-30
  return 0;
}
