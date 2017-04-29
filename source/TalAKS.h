//================================================================= TalAKS.h
//
// Convert AKS from DWD into a time series for AUSTAL2000
// ======================================================
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
//  last change 2011-12-06 lj
//
//==========================================================================

#ifndef TALAKS_INCLUDE
#define TALAKS_INCLUDE

typedef struct {
  double t;       // end time of the 1 hour interval
  int iRa;        // wind direction in degree
  int iUa;        // wind velocity in 0.1 m/s
  int iKl;        // stability class (Klug/Manier)
  int ns;         // number of seconds
  float sg;       // statistical weight
  float ef;       // emission factor
  float ri;       // rain intensity
} AKSREC;

extern ARYDSC AKSary;

int TasRead( void )
;
int TasWriteWetter( void )
;
int TasWriteStaerke( void )
;
int TasWriteWetlib( void )
;
int TasRain( void )
;
long TalAKS( 
char *ss )
  ;
#endif  //====================================================================
