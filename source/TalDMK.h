// ================================================================= TalDMK.h
//
// Diagnostic microscale wind field model DMK for bodies
// =====================================================
//
// Copyright (C) Umweltbundesamt, Dessau-Roﬂlau, Germany, 2004-2005
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
// last change: 2005-11-09 uj
//
//==========================================================================

#define DMK_USEHM    0x0002

typedef struct {
  int   check;
  int   verb;
  int   flags;   //-2005-02-15
  float a1;
  float a2;    
  float a3;
  float a4;
  float a5;
  float da;
  float hs;
  float fs;
  float fk;
  float x0;
  float y0;
  float dd;
  int   nx;
  int   ny;
  int   nz;
  float rj;
  float *hh;
} DMKDAT;

extern char *DMKVERS;
extern float DMKfdivmax, DMKndivmax;                            //-2005-11-09

int DMK_calculateWField(DMKDAT *, ARYDSC *, ARYDSC *, ARYDSC *, ARYDSC *, ARYDSC *, int);

long DMKinit(long, char *);


