//=================================================================== TalPpm.h
//
// Handling of particle parameters
// ===============================
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

#ifndef  TALPPM_INCLUDE   /*######################################################*/
#define  TALPPM_INCLUDE

typedef struct {
   float mmin;
   float vdep;
   float vsed;
   float rwsh;
   float wdep;
   float vred;
   float hred;
   } PPMREC;

float PpmVsed(          /* Berechnung der Sedimentations-Geschwindigkeit */
  float dm,             /* Durchmesser (in mu)    */
  float rho )           /* Dichte (in g/cm3)    */
  ;
long PpmClcWdep(        /* Berechnung der Depositions-Wahrscheinlichkeit */
  float sw,             /* Sigma-W am Erdboden                           */
  float vdep,           /* Depositions-Geschwindigkeit.                  */
  float vsed,           /* Sedimentations-Geschwindigkeit.               */
  float *pwdep )        /* Pointer auf Depositions-Wahrscheinlichkeit.   */
  ;
long PpmInit(     /* initialize server  */
  long flags,   /* action flags   */
  char *istr )    /* server options */
  ;
long PpmServer(         /* server routine for PPMpar, WDParr  */
  char *s )   /* calling option     */
  ;

#endif  /*##################################################################*/
