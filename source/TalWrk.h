// ======================================================== TalWrk.h
//
// Particle driver for AUSTAL2000
// ==============================
//
// Copyright (C) Umweltbundesamt, Dessau-Roﬂlau, Germany, 2002
// Copyright (C) Janicke Consulting, 88662 ‹berlingen, Germany, 2002-2011
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
// last change:  2011-07-04 uj
//
// ==================================================================

#ifndef  TALWRK_INCLUDE
#define  TALWRK_INCLUDE

#define VALID_INTER  1
#define VALID_AVER   2
#define VALID_TOTAL  3

/*================ function prototypes =====================================*/

char *WrkHeader(  /* the header (global storage)  */
  long id,        /* identification   */
  long *pt1,      /* start of the validity time */
  long *pt2 )     /* end of validity time   */
  ;
long WrkInit(   /* initialize server  */
long flags,     /* action flags   */
char *istr )    /* server options */
  ;
long WrkServer(   /* server routine for DOSarr  */
  char *s )       /* calling option   */
  ;
  
float WrkValid( int total );                                      //-2011-07-04

/*==========================================================================*/
#endif
