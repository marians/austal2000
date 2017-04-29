// ================================================================== TalSrc.h
//
// Generate particles
// ==================
//
// Copyright (C) Umweltbundesamt, Dessau-Ro�lau, Germany, 2002
// Copyright (C) Janicke Consulting, 88662 �berlingen, Germany, 2002-2005
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
// last change: 2002-06-26 lj
//
//==========================================================================

#ifndef TALSRC_INCLUDE
#define TALSRC_INCLUDE

#ifndef  IBJARY_INCLUDE
  #include "IBJary.h"
#endif

#ifndef   TALPRM_INCLUDE
  #include "TalPrm.h"
#endif

#define  SRC_XOUT    0x01
#define  SRC_YOUT    0x02
#define  SRC_ZOUT    0x04
#define  SRC_GOUT    0x07
#define  SRC_TOUT    0x08
#define  SRC_CREATED 0x10
#define  SRC_SELECT  0x20
#define  SRC_REMOVE  0x40
#define  SRC_EOUT    0x47
#define  SRC_ROUT    0x4F
#define  SRC_EXIST   0x80


typedef struct {
   unsigned char flag, refl, numcmp, offcmp;
   long srcind;
   double x, y, z, h;
   long t;
   float u, v, w;
   unsigned long rnd;
   unsigned char ix, iy, iz, nr;
   float vg, afuhgt, afutsc, afurx, afury, afurz, g[2];
   } PTLREC;

long SrcNumPtl(       /*  Berechnung der Teilchenzahl für eine Zelle.  */
int np,               /*  Gesamtzahl der Teilchen für das Raster.      */
SRCREC *psrc,         /*  Pointer auf Quellen-Beschreibung.            */
ARYDSC *pdsc,         /*  Pointer auf Rasterquelle                     */
float *px0,           /*  Linke x-Koordinate der Zelle.                */
float *py0,           /*  Untere y-Koordinate der Zelle.               */
float *ph0,           /*  Höhe der Raster-Zelle.                       */
float *pfak )         /*  Korrektur-Faktor für Teilchen-Masse.         */
  ;
long SrcCrtPtl(       /* Teilchen erzeugen.                            */
int aslind,           /* Stoffarten-Index der Teilchen.                */
int part,             /* Nummer der Gruppe.                            */
long idptl )          /* Ident der Teilchen-Tabelle                    */
  ;
long SrcSlcPtl(            /* Teilchen auswählen                       */
long id,                   /* Ident der Teilchentabelle                */
int flag,                  /* Zu setzendes Flag                        */
float x1, float x2,        /* Gewünschter Bereich in x                 */
float y1, float y2,        /* Gewünschter Bereich in y                 */
float rfac )               /* Ausdünnungsfaktor                        */
  ;                        /* RETURN: Anzahl der übertragenen Teilchen */

int SrcReduceAllPtl(    /* reduce particle number       */
float rfac )            /* reduction factor             */
  ;
char *SrcHeader(                /* the header (global storage)  */
  long id,                      /* identification               */
  long *pt1,                    /* start of the validity time   */
  long *pt2 )                   /* end of validity time         */
  ;
long SrcInit(           /* initialize server    */
long flags,             /* action flags         */
char *istr )            /* server options       */
  ;
long SrcServer(         /* server for PTLarr    */
char *ss )              /* argument string      */
  ;

/*==========================================================================*/
#endif

