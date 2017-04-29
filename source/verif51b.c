// ====================================================== verif51b.c
//
// Evaluation of verification 51b
// ==============================
//
// Copyright (C) Umweltbundesamt, Dessau-Roﬂlau, Germany, 2008
// Copyright (C) Janicke Consulting, 88662 ‹berlingenm, Germany, 2008
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
#include <stdlib.h>
#include <math.h>

#include "verif.h"                                                //-2008-09-30
#include "verif51b.nls"

#define CC(i,j,k)  Cc[((i-1)*Ny+j-1)*Nz+k-1]
#define DD(i,j,k)  Dd[((i-1)*Ny+j-1)*Nz+k-1]

static char *Path = "verif/51b";
static char FileCnc[64];
static int Nx=100, Iq=2, Ny=3, Jq=2, Nz=30, Nn=70;
static double *Cc;
static double Dx=20, Dz=10, Di=2;
static double Tw=10, Sw=0.5, H=55, Sq=40, Vq=2.5, U=6.0;

//==================================================================== main

int main( int argc, char *argv[] ) {
  char fn[256], buf[4000]="", *pc, name[256];
  int i, j, k, n, l;
  double c, x, z, csum, zsum, sz, za, szth, zath, t;
  FILE *f;
  if (init(argc, argv, "VRF") < 0)                                //-2008-09-30
    vMsg(_problems_nls_, NlsLanguage);                            //-2008-09-30
  sprintf(FileCnc, "xx-%%03d%s.dmna", CfgAddString);

  fprintf(Out, "%s", _title1_);                                   //-2008-09-30

  n = 1;

  sprintf(name, FileCnc, n);
  sprintf(fn, "%s/%s", Path, name);
  f = fopen(fn, "r");
  if (!f) {
    vMsg(_cant_read_$_, fn);                                      //-2008-09-30
    exit(1);
  }
  l = 0;
  while(fgets(buf, 4000, f)) {
    l++;
    if (!strncmp(buf,"hghb", 4)) {
      sscanf(buf+4, "%d%d%d", &Nx, &Ny, &Nz);
      if (Nx>100 || Ny>100 || Nz>100) {
        vMsg(_invalid_structure_$_, fn);                          //-2008-09-30
        exit(10);
      }
      Cc = malloc(Nx*Ny*Nz*sizeof(double));
    }
    if (*buf == '*')  break;
  }
  if (*buf != '*') {
    vMsg(_data_not_found_$_, fn);                                 //-2008-09-30
    exit(2);
  }
  for (k=1; k<=Nz; k++) {
    for (j=Ny; j>0; ) {
      if (!fgets(buf, 4000, f)) {
        vMsg(_eof_$$_, j, k);                                     //-2008-09-30
        exit(3);
      }
      l++;
      pc = strtok(buf, " \t\r\n");
      if (!pc)  continue;
      for (i=1; i<=Nx; i++) {
        if (!pc) {
          vMsg(_element_$$$_not_found_, i, j, k);                 //-2008-09-30
          exit(5);
        }
        if (1 != sscanf(pc, "%lf", &c))  {
          vMsg(_cant_read_element_$$$_, i, j, k);                 //-2008-09-30
          exit(4);
        }
        c *= 1.e6;
        CC(i,j,k) = c;
        pc = strtok(NULL, " \t\r\n");
      } // for i
      j--;
    } // for j
  } // for k
  fclose(f);
  f = NULL;

  //
  fprintf(Out, "%s", _header_);                                   //-2008-09-30
  for (i=Iq+Di; i<=Iq+Nn; i+=Di) {
    x = (i-Iq)*Dx;
    fprintf(Out, "%4.0lf", x);                                    //-2008-09-30
    j = Jq;
    csum = 0;
    zsum = 0;
    for (k=1; k<=Nz; k++) {
      if (i>Nx || j>Ny || k>Nz) {
        fprintf(Out, " -------\n");                               //-2008-09-30
        exit(6);
      }
      z = (k-0.5)*Dz;
      c = CC(i,j,k);
      csum += c;
      zsum += c*z;
    }
    if (csum > 0)  zsum /= csum;
    za = zsum;
    csum = 0;
    zsum = 0;
    for (k=1; k<=Nz; k++) {
      z = (k-0.5)*Dz - za;
      c = CC(i,j,k);
      csum += c;
      zsum += c*(z*z + Dz*Dz/12);
    }
    if (csum > 0)  zsum /= csum;
    sz = sqrt(zsum);
    t = x/U;
    zath = H + Vq*Sq*(1 - exp(-t/Sq));
    szth = Tw*Sw*sqrt(2*(t/Tw - 1 + exp(-t/Tw)));
    fprintf(Out, " %7.1f %7.1f %7.1f %7.1f\n", za, zath, sz, szth);//-2008-09-30
  }
  fprintf(Out, "\n");                                             //-2008-09-30
  vMsg("@verif51b finished");                                     //-2008-09-30
  return 0;
}
