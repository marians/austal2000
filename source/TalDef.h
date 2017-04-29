//=============================================================== TalDef.h
//
// Write DEF-Files for LASAT
// =========================
//
// Copyright (C) Umweltbundesamt, Dessau-Roﬂlau, Germany, 2002
// Copyright (C) Janicke Consulting, 88662 ‹berlingen, Germany, 2002-2005
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
// last change: 2002-09-24 lj
//
//==========================================================================

#ifndef TALDEF_INCLUDE
#define TALDEF_INCLUDE

#include <stdio.h>

#ifndef IBJARY_INCLUDE
  #include "IBJary.h"
#endif

#ifndef IBJDMN_INCLUDE
  #include "IBJdmn.h"
#endif

extern char *TalDefVersion;
extern char TalVersion[];
extern char TalLabel[];

int TdfCopySurface( // copy zg??.dmna to work/srfa??.arr
char *path )        // working directory
;

int TdfWriteGrid( FILE *f )
;

int TdfWriteBodies( FILE *f )
;

int TdfWriteMetlib( // write metlib.def for generating a wind library
int mode )          // TIP_COMPLEX: 2 base fields for each class
;                   // RETURN: number of entries

int TdfWrite(       // write def-files
char *path,         // working directory
ARYDSC *pary,       // time series or statistics
DMNFRMREC *frmtab,  // format table for array record
int mode )          // working mode (TIP_SERIES, TIP_STAT, TIP_VARIABLE)
;
#endif  //=============================================================
