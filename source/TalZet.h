// ======================================================== TalZet.h
//
// Calculate time series for AUSTAL2000
// ====================================
//
// Copyright (C) Umweltbundesamt, Dessau-Roﬂlau, Germany, 2002
// Copyright (C) Janicke Consulting, 88662 ‹berlingen, Germany, 2002-2008
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
// last change:  2008-07-30  lj
//
// ==================================================================

#ifndef  TALZET_INCLUDE
#define  TALZET_INCLUDE

#ifndef   IBJARY_INCLUDE
  #include "IBJary.h"
#endif

#define MNT_NUMVAL     8800  // maximum number of values within a time series
#define MNT_MAX        100   // maximum number of monitor points

typedef struct {
  char np[40];
  float xp, yp, zp;
  int index;
  int nn, nl, ni, i, j, k, ic;
  ARYDSC adc;
  ARYDSC ads;
  long t1, t2;
} MONREC;


extern int NumMon;

long ZetInit(     /* initialize server  */
  long flags,   /* action flags   */
  char *istr )    /* server options */
  ;
long ZetServer(         /* server routine for ZET */
  char *s )   /* calling option   */
  ;
/*==========================================================================*/
#endif
