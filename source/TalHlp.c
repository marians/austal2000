// =========================================================== TalHlp.c
//
// Help Function for AUSTAL2000
// ============================
//
// Copyright (C) Umweltbundesamt, Dessau-Roﬂlau, Germany, 2002-2006
// Copyright (C) Janicke Consulting, 88662 ‹berlingen, Germany, 2002-2014
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
// last change:  2014-01-21 uj
//
//========================================================================

#include <stdio.h>
#include <string.h>

#include "IBJmsg.h"
#include "TalHlp.h"
#include "TalCfg.h"
#include "TalHlp.nls"

void TalHelp(char *cmd) {
  int i;
  char *p;
  if (!cmd) return;
  for (i=0; ; i++) {
  		if (CfgWet && i == 0) {                                       //-2014-01-21
  				char *cp = strstr(TalHlp_NLS[i], "austal2000 ");
  				if (cp) *(cp+10) = 'n';
  		}			
    p = TalHlp_NLS[i];
    if (!p) break;
    vMsg("%s", p);                                                //-2008-09-26
    if (i == 7 && strcmp(cmd, "help")) break;                     //-2008-08-15
  }
}
