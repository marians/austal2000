// ======================================================== TalRnd.c
//
// Random numbers for AUSTAL2000
// =============================
//
// Copyright (C) Umweltbundesamt, Dessau-Roﬂlau, Germany, 2002-2003
// Copyright (C) Janicke Consulting, 88662 ‹berlingen, Germany, 2002-2003
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

#include <math.h>
#include "TalRnd.h"            

static unsigned long RndSeed = 123456;
static int RndIset=0;

//==================================================================== RndULong
unsigned long RndULong( // Gleich verteilt als "unsigned long"
void ) {  
  RndSeed = 1664525*RndSeed + 1013904223;
  return RndSeed;
}

//===================================================================== RndEqu01
float RndEqu01(       // Gleich verteilt zwischen 0 und 1.
void )
{
  unsigned long r = RndULong();
  float f = ((r>>8) & 0x00ffffff)/((float)0x01000000);
  return f;
}

/*===================================================================== RndEqu
*/
float RndEqu(         // Gleich verteilt mit Varianz = 1. 
void )
{
  float r;
  r = RndEqu01();
  return 3.4641016*(r - 0.5);
}

/*===================================================================== RndNrm
*/
float RndNrm()         // Normal verteilt ( Mittel = 0,  Varianz = 1 ).
{
  static float gset;
  float fac, r, v1, v2;
  if (RndIset == 0) {
    do {
      r = RndEqu01();
      v1 = 2.0*r - 1.0;
      r = RndEqu01();
      v2 = 2.0*r - 1.0;
      r = v1*v1 + v2*v2;
      } while (r >= 1.0);
    fac = sqrt( -2.0*log(r)/r );
    gset = v1*fac;
    RndIset = 1;
    return v2*fac;
    }
  else {
    RndIset = 0;
    return gset;
  }
}

//============================================================= RndGetSeed
//
unsigned long RndGetSeed(   // Seed-Parameter liefern.
void ) { 
  return RndSeed; 
}

//============================================================= RndSetSeed
//
void RndSetSeed(            // Seed-Parameter setzen
unsigned long seed) {
  if (seed != 0)  RndSeed = seed;
  else            RndSeed = 123456;
  RndIset = 0;
}

//============================================================================
//
// history:
//
// 2003-02-21 1.1.2 lj  revised version
//
//============================================================================

