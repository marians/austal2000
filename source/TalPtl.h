//=================================================================== TalPtl.h
//
// Handling of particle arrays
// ===========================
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
// last change: 2002-07-06 lj
//
//==========================================================================

#ifndef   IBJARY_INCLUDE
	#include "IBJary.h"
#endif

#ifndef TALPTL_INCLUDE
#define TALPTL_INCLUDE 

#define  PTL_ANY    255
#define  PTL_CLEAR  1
#define  PTL_SAVE   0

typedef unsigned char PTLTAG;

extern int  PtlRecSize;
extern char PtlRecFormat[];

  /*================================================================== PtlInit
  */
long PtlInit(         /* initialize server  */
  long flags,         /* action flags       */
  char *init_string ) /* server options     */
  ;
  /*================================================================= PtlStart
  */
long PtlStart(  /* initialize sequential retrieval of ptl's */
  ARYDSC *pa )  /* array pointer        */
  ;
  /*================================================================== PtlNext
  */
void *PtlNext(  /* pointer to next ptl  */
  int handle )  /* handle   */
  ;  
  /*=================================================================== PtlEnd
  */
long PtlEnd(  	/* finish sequential retrieval of ptl's */
  int handle )  /* handle       */
  ;  
  /*================================================================= PtlCount
  */
long PtlCount(  /* count particles in table with this ... */
  long id,  		/* identification having one of these ... */
  PTLTAG flags, /* flags set          */
  ARYDSC *pd )  /* pointer to particle array (opt.)   */
  ;
  /*================================================================ PtlCreate
  */
ARYDSC *PtlCreate(  /* create particle table  */
  long id,    			/* identification   */
  int nptl,     		/* number of particles    */
  PTLTAG tag )    	/* init tag     */
  ;
  /*============================================================== PtlTransfer
  */
long PtlTransfer( /* extract selected ptl's from source to dest.  */
  long srcid,   	/* identification of source     */
  long dstid,   	/* identification of destination    */
  PTLTAG flags,   /* flags for selection        */
  int clear )     /* clear flags                                  */
  ;
/*==========================================================================*/

#endif
