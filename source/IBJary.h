//========================================================= IBJary.h
//
// Array functions for IBJ-programs
// ================================
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
//==================================================================

#ifndef  IBJARY_INCLUDE
#define  IBJARY_INCLUDE

#define  ARYMAXDIM  5
#define  ChrPtr      (char  *) AryPtrX
#define  IntPtr      (int   *) AryPtrX
#define  LngPtr      (long  *) AryPtrX
#define  FltPtr      (float *) AryPtrX
#define  DblPtr      (double*) AryPtrX

#ifndef  ABS
	#define  ABS(a)  ((a)<0 ? -(a) : (a))
#endif
#ifndef  MIN
	#define  MIN(a,b)  ((a)<(b) ? (a) : (b))
#endif
#ifndef  MAX
	#define  MAX(a,b)  ((a)>(b) ? (a) : (b))
#endif

extern char *IBJaryVersion;

typedef struct {                /* Array descriptor                     */
        void *start;            /* address of first data byte           */
        int usrcnt;             /* user count                           */
        int elmsz;              /* element size                         */
        int maxdm;              /* maximum number of dimensions         */
        int numdm;              /* actual number of dimensions          */
        int ttlsz;              /* total size of data                   */
        int virof;              /* virtual offset                       */
        struct { int low;       /* lower bound                          */
                 int hgh;       /* upper bound                          */
                 int mul;       /* multiplier                           */
                 } bound[ARYMAXDIM];
        } ARYDSC;

int AryAssert(      /* check dimensions of array *pa:                   */
ARYDSC *pa,         /* pointer to array descriptor                      */
int l,              /* length of data element                           */
... )               /* number of dimensions                             */
                    /* lower and upper bound (as integer) of each dim   */
  ;
//================================================================= AryDefine
//
long AryDefine(     /* initialize array descriptor          */
ARYDSC *pa,         /* pointer to array descriptor          */
int l,              /* l>0: length of data element          */
                    /* l=0: set descriptor elements to 0.   */
                    /* l<0: recalculate descriptor data     */
... )               /* number of dimensions                 */
                    /* limits (lower and upper bounds)      */
;
int AryCreate(      /* create an array                                  */
ARYDSC *pa,         /* pointer to array descriptor                      */
int l,              /* length of data element                           */
... )               /* number of dimensions                             */
                    /* lower and upper bound (as integer) of each dim   */
  ;
int AryDefAry(      /* define subarray                                  */
ARYDSC *psrc,       /* pointer to source array                          */
ARYDSC *pdst,       /* pointer to destination array                     */
ARYDSC *porg,       /* actual source of destination data            */
char *def )         /* mapping, e.g.: j=3..1/0,i=0..5,k=2               */
  ;
int AryFree(        /* free memory used by array        */
ARYDSC *pa )        /* pointer to array descriptor      */
  ;
void *AryPtr(       /* address of array element */
ARYDSC *pa,         /* pointer array descriptor */
... )               /* index list               */
  ;
void *AryPtrX(      /* address of array element */
ARYDSC *pa,         /* pointer array descriptor */
... )               /* index list               */
  ;
void *AryOPtr(      // address of array element 
int offset,         // offset within record
ARYDSC *pa,         // pointer to array descriptor 
... )               // index list   
  ;
void *AryOPtrX(     // address of array element 
int offset,         // offset within record
ARYDSC *pa,         // pointer to array descriptor 
... )               // index list   
  ;
#endif  /*##################################################################*/
