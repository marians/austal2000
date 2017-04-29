// ====================================================== verif02.c
//
// Evaluation of verification 02
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
#include "verif02.nls"

static char *Path = "verif/02";
static char *OdorSum="ODOR    ";
static char *Odor050="ODOR_050";
static char *Odor100="ODOR_100";
static char *OdorMod="ODOR_MOD";

static double FrqSum, Frq050, Frq100, FrqMod;
static double ExpSum = 70;
static double Exp050 = 50;
static double Exp100 = 30;
static double ExpMod = 50.00;   // IBJ mode

int main( int argc, char *argv[] ) {
  char fn[256], buf[4000]="";
  FILE *f;
  if (init(argc, argv, "VRF") < 0)                                //-2008-09-30
    vMsg(_problems_nls_, NlsLanguage);                            //-2008-09-30
  fprintf(Out, "%s", _title1_);                                   //-2008-09-30
  sprintf(fn, "%s/%s", Path, LogName);
  f = fopen(fn, "r");
  if (f == NULL) {
    vMsg(_cant_read_$_, fn);                                      //-2008-09-30
    exit(1);
  }
  while(fgets(buf, 4000, f)) {
    if (strlen(buf) > 22) {
      if      (!strncmp(buf, OdorSum, 8))  sscanf(buf+14, "%lf", &FrqSum);
      else if (!strncmp(buf, Odor050, 8))  sscanf(buf+14, "%lf", &Frq050);
      else if (!strncmp(buf, Odor100, 8))  sscanf(buf+14, "%lf", &Frq100);
      else if (!strncmp(buf, OdorMod, 8)) {
        sscanf(buf+15, "%lf", &FrqMod);
        break;
      }
    }
  }
  fclose(f);
  fprintf(Out, _form_label_, _label0_);                           //-2008-09-30
  fprintf(Out, "   %8s", Odor100);                                //-2008-09-30
  fprintf(Out, "   %8s", Odor050);                                //-2008-09-30
  fprintf(Out, "   %8s", OdorSum);                                //-2008-09-30
  fprintf(Out, "   %8s", OdorMod);                                //-2008-09-30
  fprintf(Out, "\n");                                             //-2008-09-30
  fprintf(Out, _form_label_, _label1_);                           //-2008-09-30
  fprintf(Out, "   %6.1lf %%", Exp100);                           //-2008-09-30
  fprintf(Out, "   %6.1lf %%", Exp050);                           //-2008-09-30
  fprintf(Out, "   %6.1lf %%", ExpSum);                           //-2008-09-30
  fprintf(Out, "   %6.1lf %%", ExpMod);                           //-2008-09-30
  fprintf(Out, "\n");                                             //-2008-09-30
  fprintf(Out, _form_label_, _label2_);                           //-2008-09-30
  fprintf(Out, "   %6.1lf %%", Frq100);                           //-2008-09-30
  fprintf(Out, "   %6.1lf %%", Frq050);                           //-2008-09-30
  fprintf(Out, "   %6.1lf %%", FrqSum);                           //-2008-09-30
  fprintf(Out, "   %6.1lf %%", FrqMod);                           //-2008-09-30
  fprintf(Out, "\n\n");                                           //-2008-09-30
  vMsg("@verif02  finished");                                     //-2008-09-30
  return 0;
}
