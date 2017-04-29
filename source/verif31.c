// ====================================================== verif31.c
//
// Evaluation of verification 31
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
#include <stdlib.h>
#include <math.h>

#include "verif.h"                                                //-2008-09-30
#include "verif31.nls"

#define CC(i,j,k)  Cc[((i-1)*Ny+j-1)*Nz+k-1]

static char *Path = "verif/31";
static char FileCnc[64];
static int Nx=61, Ny=61, Nz=41, Nn=30;
static double Sx[100], Sy[100], Sz[100], *Cc;
static double Dx=20, Dy=20, Dz=10;
static double Tu=2.e6, Tv=2.e6, Tw=2.e5, Su=8.e-5, Sv=6.e-5, Sw=4.e-5;
static double Dt=86400;

int main( int argc, char *argv[] ) {
  char fn[256], buf[4000]="", *pc, name[256];
  int i, j, k, n, l;
  double c, x, xm, y, ym, z, zm, csum, vsum, sx, sy, sz, t;
  FILE *f;
  if (init(argc, argv, "VRF") < 0)                                //-2008-09-30
    vMsg(_problems_nls_, NlsLanguage);                            //-2008-09-30
  sprintf(FileCnc, "xx-%%03d%s.dmna", CfgAddString);

  fprintf(Out, "%s", _title1_);                                   //-2008-09-30

  for (n=1; n<=Nn; n++) {

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
  xm = 0.5*Dx*Nx;
  vsum = 0;
  csum = 0;
  for (i=1; i<=Nx; i++) {
    x = (i-0.5)*Dx - xm;
    c = 0;
    for (j=1; j<=Ny; j++)
      for (k=1; k<=Nz; k++)
        c += CC(i,j,k);
    vsum += (x*x + Dx*Dx/12)*c;
    csum += c;
  }
  if (csum > 0)  vsum /= csum;
  Sx[n-1] = sqrt(vsum);
  //
  ym = 0.5*Dy*Ny;
  vsum = 0;
  csum = 0;
  for (j=1; j<=Ny; j++) {
    y = (j-0.5)*Dy - ym;
    c = 0;
    for (i=1; i<=Nx; i++)
      for (k=1; k<=Nz; k++)
        c += CC(i,j,k);
    vsum += (y*y + Dy*Dy/12)*c;
    csum += c;
  }
  if (csum > 0)  vsum /= csum;
  Sy[n-1] = sqrt(vsum);
  //
  zm = 0.5*Dz*Nz;
  vsum = 0;
  csum = 0;
  for (k=1; k<=Nz; k++) {
    z = (k-0.5)*Dz - zm;
    c = 0;
    for (j=1; j<=Ny; j++)
      for (i=1; i<=Nx; i++)
        c += CC(i,j,k);
    vsum += (z*z + Dz*Dz/12)*c;
    csum += c;
  }
  if (csum > 0)  vsum /= csum;
  Sz[n-1] = sqrt(vsum);
  //

  } // for n

  fprintf(Out, "%s", _header_);                                   //-2008-09-30
  for (n=1; n<=Nn; n++) {
    t = (n-0.5)*Dt - 1800;
    sx = Tu*Su*sqrt(2*(t/Tu - 1 + exp(-t/Tu)));
    sy = Tv*Sv*sqrt(2*(t/Tv - 1 + exp(-t/Tv)));
    sz = Tw*Sw*sqrt(2*(t/Tw - 1 + exp(-t/Tw)));
    fprintf(Out, "%2d %7.1lf %7.1lf %7.1lf %7.1lf %7.1lf %7.1lf\n",//-2008-09-30
    n, Sx[n-1], sx, Sy[n-1], sy, Sz[n-1], sz);
  }
  fprintf(Out, "\n");                                             //-2008-09-30
  vMsg("@verif31  finished");                                     //-2008-09-30
  return 0;
}
