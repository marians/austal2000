//=================================================================== genio.h
//
// Basic I/O-functions
// ===================
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

#ifndef  GENIO_H
#define  GENIO_H

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#ifndef  IBJARY_INCLUDE
	#include "IBJary.h"
#endif

#define  INBUFSIZE  4000
#define  GENHDRLEN  4000
#define  GENTABLEN  4000

#define  MAXSPEC  256
#define  MAXDRIV    1
#define  MAXPATH  256
#define  MAXNAME  120
#define  MAXEXTN  120
#define  SEPDRIV  ':'

typedef struct {
  int  sequ;
  char spec[MAXSPEC+1];
  char driv[MAXDRIV+1];
  char path[MAXPATH+1];
  char name[MAXNAME+1];
  char extn[MAXEXTN+1];
  } FILESPEC;

extern char GenInputName[];

void GenCheckPath( char *path, int append )
;
void GenDefCode( char *conversion )
  ;
int GenTranslate( char *s )
  ;
int Printf(             /* Ausgabe mit Umsetzung der Umlaute */
char *format,           /* Formatstring der Meldung          */
... )                   /* Variable Teile der Meldung        */
  ;
int fPrintf(            /* Ausgabe mit Umsetzung der Umlaute */
FILE *stream,           /* Ausgabe-File                      */
char *format,           /* Formatstring der Meldung          */
... )                   /* Variable Teile der Meldung        */
  ;
long Disp(         /*  Text s in der Display-Zeile ausgeben.      */
char *s );

long Dump(            /* Hex-Dump ausgeben (nur für Testzwecke).       */
void *pv );           /* Start-Adresse des Dumps.                      */

long SetDataPath(     /* Pfad f³r Ein- und Ausgabe merken.             */
char *pfad );         /* Pfad.                                         */

long SeekInput(       /* Im Eingabe-File Position aufsuchen.           */
long pos );           /* Aufzusuchende Lese-Position.                  */
                      /* RETURN: pos                                   */

long OpenInput(       /*  Eingabefile auf dem gemerkten Pfad öffnen.   */
char *name,           /*  Name des Eingabe-Files.                      */
char *altn );         /*  Alternativer Name f³r Verbund-File           */
                      /*  RETURN: Start-Position im File               */

long CntLines(        /* Anzahl der Eingabe-Sätze feststellen.         */
char c,               /* Kenn-Zeichen für Abbruch.                     */
char *s,              /* Zeichenkette zur Aufnahme der Kenn-Zeichen.   */
int ls );             /* Maximale Länge der Zeichenkette.              */

long GenCntLines(     /* Anzahl der Eingabe-Sätze feststellen.         */
char c,               /* Ausgewähltes Kennzeichen                      */
char **ps )           /* Zeichenkette zur Aufnahme der Kennzeichen.    */
  ;
long GetLine(         /* Satz vom Eingabe-File einlesen.               */
char c,               /* Gefordertes Kenn-Zeichen des Satzes.          */
char *s,              /* Zeichenkette zur Aufnahme des Satzes.         */
int n );              /* Maximale Länge der Zeichenkette.              */
                      /* RETURN: Anzahl der übertragenen Zeichen.      */

long CloseInput(      /* Daten-Eingabefile schließen.                  */
void );               /* RETURN: Aktuelle Lese-Position.               */

long ArrExist(      /* Feststellen, ob File existiert */
char *name )        /* File-Name                      */
  ;
long ArrRead(       /* Array aus einem Array-File (binär) lesen.          */
char *name,         /* File-Name (Pfad wird davorgesetzt).                */
int mode,           /* 0: Header, 1: +Deskriptor, 2: +Daten, 3: selektiv. */
char *header, ... );/* Zeichenkette zur Aufnahme des Headers.             */
/* ARRDSC *pa,         Pointer auf Array-Deskriptor (wird ausgefüllt).    */

long ArrReadMul(    /* Mehrere Arrays aus einem Array-File lesen.         */
char *name,         /* File-Name (Pfad wird davorgesetzt).                */
int mode,           /* 0: Header, 1: +Deskriptor, 2: +Daten, 3: selektiv. */
char *header,       /* Zeichenkette zur Aufnahme des Headers.             */
ARYDSC *pa,         /* Pointer auf Array-Deskriptor (wird ausgefüllt).    */
FILE **pfile);      /* Pointer auf File-Pointer (wird beim Open gesetzt). */

long ArrWrite(      /* Array in einen Array-File (binär) ausschreiben.    */
char *name,         /* File-Name (Pfad wird davor gesetzt).               */
char *header,       /* Header zur Beschreibung der Daten.                 */
ARYDSC *pa );       /* Pointer auf Array-Deskriptor (wird mit ³bertragen).*/
                    /* RETURN: = TRUE, wenn der File schon existierte.    */

long ArrWriteMul(   /* Array an einen Array-File (binär) anhängen.        */
char *name,         /* File-Name (Pfad wird davor gesetzt).               */
char *header,       /* Header zur Beschreibung der Daten.                 */
ARYDSC *pa,         /* Pointer auf Array-Deskriptor (wird mit übertragen).*/
FILE **pfile );     /* Pointer auf File-Pointer (wird beim OPEN gesetzt). */
                    /* RETURN: = TRUE, wenn der File schon existierte.    */

long PrnArrDsc(     /* Array-Deskriptor ausdrucken (zum Testen).          */
ARYDSC *pa );       /* Pointer auf Array-Deskriptor.                      */

long GenSepFsp(       /* Zerlegen einer File-Bezeichnung in Komponenten. */
FILESPEC *fsp );      /* Pointer auf Struktur mit Bezeichnung .spec und  */
                      /* Komponenten .driv, .path, .name und .extn.      */

long GenConFsp(       /* Erzeugen einer File-Bezeichnung aus Komponenten.*/
FILESPEC *fsp );      /* Pointer auf Struktur mit Bezeichnung .spec und  */
                      /* Komponenten .driv, .path, .name und .extn.      */

long GenEvlHdr(         /* Namen im Tabellen-Header auswerten.              */
char *names[],          /* Liste der möglichen Namen.                       */
char *header,           /* Header der Tabelle.                              */
int *position,          /* Positionen der Header-Namen bzgl. names[].       */
float *scale,           /* Gefundene Faktoren zur Umskalierung ( = 1.0 )    */
int maxval )            /* Maximale Anzahl erfaßbarer Werte                 */
                        /* RETURN: Anzahl der Namen im Tabellen-Header.     */
  ;
/*========================================================================*/
#endif

