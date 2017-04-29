//========================================================== genutl.h
//
// Utility functions
// =================
//
// Copyright (C) Umweltbundesamt, Dessau-Ro�lau, Germany, 2002-2005
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
// last change 2002-07-06 lj
//
//==================================================================

#ifndef  GENUTL_H
#define  GENUTL_H

#define  TM_INSIDE  1
#define  TM_CROSS   2
#define  TM_FOLLOW  4
#define  TM_ADJUST  8

/*=============================== time functions =========================*/

  /*================================================================== TmMax
  */
long TmMax( void )         /* return maximum value of time */
  ; 
  /*================================================================== TmMin
  */
long TmMin( void )         /* return minimum value of time */
  ; 
  /*=============================================================== TmString
  */
char *TmString(            /* return string representation of time t */
  long *pt )               /* pointer to time t                      */
  ;
  /*================================================================ TmValue
  */
long TmValue(               /* return binary representation of time */
  char *ts )                /* string representation of time        */
  ;
  /*============================================================== TmRelation
  */
int TmRelation(         /* 1, if data are valid within [*pt1,...]       */
long t1, long t2,       /* start and end of validity time interval      */
long *pt1, long *pt2 )  /* start and end of requested time interval     */
  ;
char *TimePtr(        /* Zeichenkette mit der aktuellen Uhrzeit        */
void );

char *DatePtr(        /* Zeichenkette mit dem aktuellen Datum          */
void );

int TmSetUndef(     /* Markieren einer Zeit als "nicht definiert"   */
long *ptr );        /* Pointer auf die zu markierende Zeit          */

int TmIsUndef(      /* Abfrage, ob eine Zeit "nicht definiert" ist  */
long *ptr );        /* Pointer auf die zu prüfende Zeit             */

int SetUndef(        /* Markieren einer Größe als "nicht definiert"   */
float *ptr );        /* Pointer auf die zu markierende Größe          */

int IsUndef(         /* Abfrage, ob eine Größe "nicht definiert" ist  */
float *ptr );        /* Pointer auf die zu prüfende Größe             */

int CisCmp(           /* Vergleich ohne R³cksicht auf Großschreibung   */
char *p1,        			/* Zeichenkette                                  */
char *p2 );      			/* Zeichenkette                                  */
                      /* RETURN: 0, wenn beide gleich, sonst 1         */

void TimeStr(         /* Umwandlung einer Sekundenzahl n in eine Zeichen- */
long n,               /* kette t[] der Form  ddd.hh:mm:ss;                */
char *t );            /* f³hrende Nullen werden fortgelassen.             */

long Seconds(         /* Umwandlung einer Zeitangabe t[] von der Form    */
char *t,              /* ddd.hh:mm:ss in die Anzahl der Sekunden. Wenn   */
long r );             /* die Zeichenkette mit '+' beginnt, wird die Se-  */
                      /* kundenzahl zum zweiten Argument r addiert. Zu-  */
                      /* r³ckgegeben wird die Anzahl der Sekunden.       */

long GenTimRel(   /* Relation zweier Zeitintervalle feststellen */
long t1,          /* Beginn des gegebenen Intervalls            */
long t2,          /* Ende des gegebenen Intervalls              */
long *pt1,        /* Beginn des gew³nschten Intervalls          */
long *pt2 )       /* Ende des gew³nschten Intervalls            */
  ;               /* RETURN: =0, wenn nicht enthalten.          */

long SetFlag(         /* Suche Zeichenkette und setze Flag wenn gefunden */
char *source,         /* Zeichenkette, die durchsucht wird.              */
char *tag,            /* Zeichenkette, nach der gesucht wird.            */
long *pflags,         /* Pointer auf LongInt, wo ein Bit gesetzt wird.   */
long flag );          /* LongInt, die das zu setzende Bit enthält.       */

long RptChr(           /* Die Zeichenkette d[] wird mit der l-fachen     */
register char c,       /* Wiederholung des Buchstabens c aufgef³llt.     */ 
register long l,       /* Anschließend wird die Kette mit '\0' beendet.  */
register char *d );

long ToCap(      /* Buchstaben der Zeichenkette s[] in Grobuchstaben  */
char *s );       /* (vgl. CapCode()! ) umwandeln. Zur³ckgegeben wird   */
                 /* die Länge von s[].                                 */

char *FindName(                 /* Das erste Auftreten des Namens name in */
char *name,                     /* der Zeichenkette source wird gesucht.  */
char *source );                 /* Zur³ckgegeben wird die Adresse des     */
                                /* gefundenen Namens oder NULL.           */

int CheckName(  // return 1 if <name> is not a valid name, 0 otherwise
  char *name )  // name to be checked
;
long GetData(           /* Einlesen von Daten in der Form NAME=WERT       */
char *n,                /* Name, nach dem gesucht wird.                   */
char *s,                /* Zeichenkette, die durchsucht wird.             */
char *f,                /* Format, in dem der Wert eingelesen wird.       */
void *p );              /* Adresse, an welcher der Wert gespeichert wird. */

long GetAllData(      /* Reihe von Daten ³ber GetData() einlesen.           */
char *names,          /* Zeichenkette mit Namen, durch Blank getrennt.      */
char *src,            /* Zeichenkette mit Wertzuweisungen.                  */
char *format,         /* Format zum Einlesen der Daten.                     */
float *dst );         /* Startadresse zum Abspeichern der Float-Werte.      */
                      /* RETURN: Anzahl der ³bertragenen Daten.             */
                      
long RplData(   /* Wert bei einer Zuweisung (skalar) ersetzen  */
char *n,        /* Variablen-Name                              */
char *s,        /* Zeichenkette mit Zuweisung                  */
char *w,        /* neuer Wert                                  */
char *t )       /* neue Zeichenkette                           */
  ;
long GetList(   /* Einlesen einer Reihe von Werten aus einer Zeichenkette.*/
char *s,        /* Zeichenkette, welche die Daten (Float-Zahlen) enthält. */
char *n,        /* Name vor der Werteliste, durch : oder | abgetrennt.    */
int ln,         /* Maximale Länge f³r den Namen.                          */
float *v,       /* Float-Vektor, der die Werte aufnimmt.                  */
int lv );       /* Maximale Anzahl der Werte.                             */
                /* RETURN: Anzahl der eingelesenen Werte.                 */

void Trim(               /* Blanks am Anfang und Ende beseitigen  */
unsigned char *t );      /* zu trimmende Zeichenkette             */

void StrSort(    /*  Quicksort f³r Zeichenketten         */
char *v[],       /*  Pointer-Array auf die Zeichenketten */
int left,        /*  Kleinster Index                     */
int right );     /*  Größter Index                       */

char *GenForm(          /* Ascii-Umwandlung einer Zahl  */
double x,               /* Umzuwandelnde Zahl           */
int n)                  /* Anzahl signifikanter Stellen */
  ;
long DatAssert( /* Verify value of GETDATA-Parameter    */
char *name,     /* name of the variable                 */
char *hdr,      /* string containing assignments        */
float val )     /* required value                       */
  ;
/*========================================================================*/
#endif

