// ================================================================= TalDtb.h
//
// Calculation of concentration statistics
// =======================================
//
// Copyright (C) Umweltbundesamt, Dessau-Roﬂlau, Germany, 2002-2005
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
// last change: 2011-07-04 uj
//
//==========================================================================

#define DTB_NUMVAL     40           //-2005-03-17
#define DTB_NUMMAX     28
#define DTB_DIVISION   10.0
#define DTB_RANGE      10000.0      //-2005-03-17

#ifndef  TALDTB_INLUDE
#define  TALDTB_INCLUDE

typedef struct {
  float valmax;
  float frqnull, frqsub, frq[DTB_NUMVAL];
  float realmax;
  float realdev;
  long t1, t2;
  }  DTBREC;

typedef struct {
  float max[DTB_NUMMAX];
  float dev[DTB_NUMMAX];
  }  DTBMAXREC;

long DtbMntSet(long t1, int nc) 
;
long DtbMntRead( void ) 
;
long DtbMntStore( void ) 
;
long DtbRegister(       /* register in concentration distribution */
long idtb )
  ;
char *DtbHeader(                /* the header (global storage)  */
  long id,                      /* identification               */
  long *pt1,                    /* start of the validity time   */
  long *pt2 )                   /* end of validity time         */
  ;

long DtbInit(     /* initialize server  */
long flags,       /* action flags   */
char *istr )      /* server options */
  ;
long DtbServer(         /* server routine for DTBarr  */
  char *s )             /* calling option   */
  ;
/*==========================================================================*/
#endif
