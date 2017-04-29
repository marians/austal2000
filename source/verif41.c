// ====================================================== verif41.c
//
// Evaluation of verification 41
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
#include "verif41.nls"

#define CC(i,j,k)  Cc[((i-1)*Ny+j-1)*Nz+k-1]
#define DD(i,j,k)  Dd[((i-1)*Ny+j-1)*Nz+k-1]

static char *Path = "verif/41";
static char FileCnc[64];
static char FileDev[64];
static int Nx=100, Iq=2, Ny=3, Jq=2, Nz=80, Nn=40;
static double *Cc, *Dd;
static double Dx=50, Dy=50, Dz=10;
static double Tw=2.5, Sw=2.0, H=100, Ua=6.0, Xu=0.3, Ks, Q=50;
static double Xp[4] = { 500, 1000, 2000, 4000 };

//================================================================ BesselI0
static double BesselI0(           // modified Bessel function
  double arg)                     //
  {
  double ax, ans;
  double y;
  if ((ax=fabs(arg)) < 3.75) {
    y = arg/3.75;
    y *= y;
    ans = 1.0+y*(3.5156229+y*(3.0899424+y*(1.2067492
      +y*(0.2659732+y*(0.360768e-1+y*0.45813e-2)))));
    }
  else {
    y = 3.75/ax;
    ans = (exp(ax)/sqrt(ax))*(0.39894228+y*(0.1328592e-1
      +y*(0.225319e-2+y*(-0.157565e-2+y*(0.916281e-2
      +y*(-0.2057706e-1+y*(0.2635537e-1+y*(-0.1647633e-1
      +y*0.392377e-2))))))));
    }
  return ans;
  }

//=============================================================== Berljand
static double BerljandCy( // Berljand result
  double x,               // source distance of monitor point
  double z,               // height above ground
  double h,               // source height
  double n,               // wind profile exponent
  double u,               // wind velocity at source height
  double ks)              // gradient of Kz(z)
  {
  double cy = -1., f1, f2, f3, xi, zeta;
  xi = x*(1+n)*(1+n)*ks/(h*u);
  zeta = z/h;
  f1  = (1+n)/xi;
  f2  = exp(-(1+pow(zeta,1+n))/xi);
  f3  = BesselI0(2*pow(zeta,(1+n)/2)/xi);
  cy  = f1*f2*f3/(h*u);
  return cy;
  }

//==================================================================== main

int main( int argc, char *argv[] ) {
  char fn[256], buf[4000]="", *pc, name[256];
  int i, j, k, n, l, ip, nx, ny, nz;
  double c, cth, x, z, cmin, cmax, d;
  FILE *f;
  if (init(argc, argv, "VRF") < 0)                                //-2008-09-30
    vMsg(_problems_nls_, NlsLanguage);                            //-2008-09-30
  sprintf(FileCnc, "xx-%%03d%s.dmna", CfgAddString);
  sprintf(FileDev, "xx-%%03d%s.dmna", CfgDevString);

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
  sprintf(name, FileDev, n);
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
      sscanf(buf+4, "%d%d%d", &nx, &ny, &nz);
      if (Nx!=nx || Ny!=ny || Nz!=nz) {
        vMsg(_invalid_structure_$_, fn);                          //-2008-09-30
        exit(10);
      }
      Dd = malloc(Nx*Ny*Nz*sizeof(double));
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
        if (1 != sscanf(pc, "%lf", &d))  {
          vMsg(_cant_read_element_$$$_, i, j, k);                 //-2008-09-30
          exit(4);
        }
        DD(i,j,k) = d;
        pc = strtok(NULL, " \t\r\n");
      } // for i
      j--;
    } // for j
  } // for k
  fclose(f);
  f = NULL;
  //
  for (ip=0; ip<4; ip++) {
    x = Xp[ip];
    i = Iq + x/Dx + 0.5;
    j = Jq;
    fprintf(Out, "\nX = %4.0lf:\n\n", x);                         //-2008-09-30
    fprintf(Out, "%s", _header_);                                 //-2008-09-30
    Ks = Tw*Sw*Sw/H;
    for (k=1; k<=Nn; k++) {
      z = (k-0.5)*Dz;
      if (i>Nx || j>Ny || k>Nz)  printf(" -------");
      c = CC(i,j,k)*Dy/Q;
      d = 0.01*DD(i,j,k);
      cmin = c*(1 - 2*d);
      cmax = c*(1 + 2*d);
      cth = 1.e6*BerljandCy(x, z, H, Xu, Ua, Ks);
      fprintf(Out, "%3.0lf %7.1lf %7.1lf %7.1lf %7.1lf\n",        //-2008-09-30
          z, cmin, c, cmax, cth);
    }
  }
  fprintf(Out, "\n");                                             //-2008-09-30
  vMsg("@verif41  finished");                                     //-2008-09-30
  return 0;
}

