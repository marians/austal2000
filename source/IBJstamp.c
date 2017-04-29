//==================================================== IBJstamp.c
//
// Provide a time stamp for the time of compilation
// ================================================
//
// Copyright (C) Umweltbundesamt, Dessau-Roﬂlau, Germany, 2002
// Copyright (C) Janicke Consulting, 88662 ‹berlingen, Germany, 2002-2012
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
// last change:  2012-09-04 uj
//
//==================================================================

#include <stdio.h>
#include <string.h>
#include <locale.h>
#include <stdlib.h>

#include "IBJstamp.h"

static char *Names = "JanFebMarAprMayJunJulAugSepOctNovDec";
static char date[256];
static int ParseChecked = 0;

char *IBJstamp( char *sd, char *st ) { 
  char d[256], t[256], *pm;
  int month=0, day=0, year=0;
  //////////////////////////////////////////////////////////////////////////////
  // make a general check if numbers are parsed correctly
  // (for Windows this check is useful as the user may change the decimal 
  //  separator in the systems settings)
  if (!ParseChecked) {
    float f;
    char locale[64], s[8];
    strcpy(locale, setlocale(LC_NUMERIC, NULL));
    if (strcmp(locale, "C")) {
    		printf("*** IBJstamp: default locale is not C.\n    Program aborted.\n\n");
  		  exit(-99);
    }
    sscanf("3.5", "%f", &f);
    sprintf(s, "%1.1f", 3.5);
    setlocale(LC_NUMERIC, locale);
    if (f != 3.5 || s[1] != '.') {
    		printf("*** IBJstamp: invalid float parsing.\n    Program aborted.\n\n");
  		  exit(-99);
    }
    ParseChecked = 1;
  }
  ////////////////////////////////////////////////////////////////////////////// 
  strcpy(d, (sd) ? sd : __DATE__);
  d[3] = 0;
  strcpy(t, (st) ? st : __TIME__);
  pm = strstr(Names, d);
  if (pm) {
    month = 1 + (pm - Names)/3;
  }
  sscanf(d+4, "%d %d", &day, &year);
  sprintf(date, "%04d-%02d-%02d %s", year, month, day, t);
  return date; 
}

