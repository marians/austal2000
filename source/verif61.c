// ====================================================== verif61.c
//
// Evaluation of verification 61
// =============================
//
// Copyright (C) Umweltbundesamt, Dessau-Roﬂlau, Germany, 2003-2008
// Copyright (C) Janicke Consulting, 88662 ‹berlingen, Germany, 2003-2008
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

#include "verif.h"
#include "verif61.nls"

static char *Path = "verif/61";
static char *FileLog = LogName;
static int Nn=120;
static double U=3.6645, R=70;

//==================================================================== main

int main( int argc, char *argv[] ) {
  char fn[256], buf[4000]="";
  int n, l;
  double tp, x, y, xp, yp, t0, a;
  FILE *f;
  if (init(argc, argv, "VRF") < 0)                                //-2008-09-30
    vMsg(_problems_nls_, NlsLanguage);                            //-2008-09-30
  fprintf(Out, "%s", _title1_);                                   //-2008-09-30

  sprintf(fn, "%s/%s", Path, FileLog);
  f = fopen(fn, "r");
  if (!f) {
    vMsg(_cant_read_$_, fn);                                      //-2008-09-30
    exit(1);
  }
  l = 0;
  while(fgets(buf, 4000, f)) {
    l++;
    if (!strncmp(buf,"#### 0 0 0 [82800,86400]", 14)) break;
  }
  fprintf(Out, "%s", _header_);
  for (n=0; n<=Nn; n++) {
    if (!fgets(buf, 4000, f)) {
      vMsg(_eof_$_$_, n, l);                                      //-2008-09-30
      exit(3);
    }
    l++;
    if (3 != sscanf(buf+16, "%lf |%lf %lf", &tp, &xp, &yp)) {
      vMsg(_read_error_$_, n);                                    //-2008-09-30
      exit(5);
    }
    if (n == 0)  t0 = tp;
    tp -= t0;
    a = U*tp/R;
    x = -R*sin(a);
    y = R*cos(a);
    if (n%5 == 0)
      fprintf(Out, " %3.0lf %7.2lf %7.2lf %7.2lf %7.2lf\n",       //-2008-09-30
          tp, xp, x, yp, y);
  }
  fclose(f);
  f = NULL;
  fprintf(Out, "\n");                                             //-2008-09-30
  vMsg("@verif61  finished");                                     //-2008-09-30
  return 0;
}
