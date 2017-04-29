// ================================================================ IBJstd.c
//
// Definition of strings used by IBJstd.h
// ======================================
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
// last change:  2008-12-11 lj
//
//========================================================================

#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "IBJnls.h"
#include "IBJmsg.h"                                               //-2008-09-29
#include "IBJstd.nls"

extern char *IbjStdMsg(int i);                                    //-2008-12-11
extern void IbjStdReadNls(int argc, char* argv[], char *main, char *mod, char *version);

char *IbjStdMsg(int i) {
  char *p = "--- ---";
  switch(i) {
    case 1: p = _ibjstd_no_log_;
            break;
    case 2: p = _ibjstd_no_arg_file_;
            break;
    case 3: p = _ibjstd_error_;
            break;
    default: ;
  }
  return p;
}

void IbjStdReadNls(int argc, char* argv[], char *main, char *mod, char *version) {
  int nls = 0;
  char lan[128] = "";
  int i, trns=-1;
  for (i=1; i<argc; i++) {
    if (!strncmp(argv[i], "--language=", 11)) {
      strcpy(lan, argv[i] + 11);
    }
    else if (!strncmp(argv[i], "-X", 2)) {                        //-2008-09-29
      sscanf(argv[i]+2, "%d", &trns);
    }
  }
  if (trns >= 0)                                                  //-2008-09-29
    MsgTranslate = trns;
  nls = NlsRead(main, mod, lan, version);
#ifdef _WIN32                                                     //-2008-10-01
  if (!strcmp(NlsLanguage, "de@latin1") && (trns < 0))
    { MsgTranslate = 1; }
#endif
  if (nls == -2) {
    vMsg(_version_nls_, version);
    exit(1);
  }
  else if (nls < 0) {
    vMsg(_error_nls_, lan);
    exit(1);
  }
  else if (nls > 0)
    vMsg(_problems_nls_, lan);
}
