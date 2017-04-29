// ================================================================= TalAKT.h
//
// Read AKTerm for AUSTAL2000
// ==========================
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
// last change:  2011-12-02 lj
//
// =========================================================================

#ifndef TALAKT_INCLUDE
#define TALAKT_INCLUDE

#ifndef IBJARY_INCLUDE
  #include "IBJary.h"
#endif

typedef struct {
  double t;       // end time of the 1 hour interval (GMT+1)
  float fRa;      // wind direction in degree
  float fUa;      // wind velocity in m/s
  int iKl;        // stability class (Klug/Manier)
  float fHm;      // mixing height (m)                            //-2011-12-02
  float fPr;      // precipitation in mm/h                        //-2011-11-22
} AKTREC;

extern char *TalAKTVersion;
extern ARYDSC AKTary;
extern char AKTheader[];
extern float TatUmin, TatUref;

int TatReadAKTerm( void )
;
int TatCheckZtr( void )
;
float TatGetHa( int iZ0 )
;
int TatArgument( char *pc )
;
#endif
