//=========================================================== IBJgk.h
//
// Gauss-Krueger Routines for IBJ-programs
// =======================================
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
// last change 2001-08-14 lj 
//
//===================================================================
//
#ifndef IBJGK_INCLUDE
#define IBJGK_INCLUDE

typedef struct {
  double rechts;
  double hoch;
} GKpoint;

typedef struct {
  double lon;
  double lat;
} GEOpoint;

double GKgetLatitude(   // get geographical latitude from Gauss-Krueger
  double rechts,        // Gauss-Krueger Rechts-value
  double hoch )         // Gauss-Krueger Hoch-value
  ;
double GKgetLongitude(  // get geographical longitude from Gauss-Krueger 
  double rechts,        // Gauss-Krueger Rechts-value 
  double hoch )         // Gauss-Krueger Hoch-value 
  ;
GEOpoint GKgetGEOpoint( // get geographical point from Gauss-Krueger 
  double rechts,        // Gauss-Krueger Rechts-value 
  double hoch )         // Gauss-Krueger Hoch-value 
  ;
double GKgetRechts(     // get Gauss-Krueger Rechts from geographical coord.
  double lon,           // geographical longitude (eastern)
  double lat )          // geographical latitude
  ;
double GKgetHoch(       // get Gauss-Krueger Hoch from geographical coord. 
  double lon,           // geographical longitude (eastern) 
  double lat )          // geographical latitude
  ;
GKpoint GKgetGKpoint(   // get Gauss-Krueger point from geographical coord.  
  double lon,           // geographical longitude (eastern) 
  double lat )          // geographical latitude
  ;
GKpoint GKxGK(          // transform Gauss-Krueger between zones
  int newz,             // new zone index
  double rechts,        // Gauss-Krueger Rechts-value  
  double hoch )         // Gauss-Krueger Hoch-value 
  ;
char *GKgetString(      // get string representation (-'") of degree value
  double d )            // degree value
  ;
double GKgetDegree(     // get degree value from string representation
  char *s )             // string representation (-'")
  ;
#endif //###################################################################
