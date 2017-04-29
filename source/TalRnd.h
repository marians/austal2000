//=================================================================== TalRnd.h
//
// Random numbers for AUSTAL2000
// =============================
//
// Copyright (C) Umweltbundesamt, Dessau-Roﬂlau, Germany, 2002-2003
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
// last change:  2003-02-21  lj
//
// ==================================================================

#ifndef  TALRND_INCLUDE
#define  TALRND_INCLUDE

unsigned long RndGetSeed(   // Seed-Parameter liefern. 
void );

void RndSetSeed(            // Seed-Parameter setzen
unsigned long seed);

unsigned long RndULong(     // Gleich verteilt als "unsigned long"
void );
  
float RndEqu01(             // Gleich verteilt zwischen 0 und 1.
void );

float RndEqu(               // Gleich verteilt mit Varianz = 1. 
void );

float RndNrm(               // Normal verteilt ( Mittel = 0,  Varianz = 1 ).
void );

/*==========================================================================*/
#endif
