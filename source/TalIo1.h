// =================================================================== TalIo1.h
//
// Handling of input/output
// ========================
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

#ifndef  TALIO1_INCLUDE
#define  TALIO1_INCLUDE

#include <stdio.h>

#define  MAXVPP   100
#define  MAXVNL    40
#define  MAXVFL    12
#define  BUFLEN 32000	
#define  MAXVAL   100

typedef struct {
   char name[MAXVNL];
   char form[MAXVFL];
   float f;
   void *pv;
   int column;
   } VARREC;

typedef struct {
   int num, max;
   VARREC tab[MAXVPP];
   } VARTAB;

extern char Buf[BUFLEN];
extern float DatVec[MAXVAL];
extern int DatPos[MAXVAL];
extern float DatScale[MAXVAL];

/*=============================== PROTOTYPES TALIO1 ========================*/
 
long DefVar(          /* Namen als VARIABEL in einer Liste registrieren.    */
char n[],             /* Name, der gemerkt werden soll.                     */
char f[],             /* Format zur Umwandlung der zugeordneten Eingabe.    */
void *pv,             /* Adresse, wo der Wert abzuspeichern ist.            */
float fac,            /* Faktor zur Skalierung des Wertes.                  */
VARTAB **pps );       /* Adresse des Pointers auf die Merktabelle.          */

long PrnVar(          /* Test-Ausdruck der gemerkten Namen.                 */
char t[],             /* Auszudruckende Überschrift.                        */
VARTAB *ps,           /* Pointer auf die Merktabelle.                       */
FILE *logfile );      /* Ausgabe-File.                                      */

VARREC *FndVar(       /* In der Merktabelle nach einem Namen suchen.        */
char n[],             /* Name, nach dem gesucht werden soll.                */
VARTAB *ps );         /* Pointer auf die Merktabelle.                       */
                      /* RETURN: Pointer auf den zugehörigen Record.        */

long DefParm(         /* Rechenparameter einlesen oder Standardwert setzen. */
char *name,           /* Name des einzulesenden Parameters.                 */
char *string,         /* Zeichenkette mit Wertzuweisungen.                  */
char *form,           /* Format zur Umwandlung der Eingabe.                 */
void *ptr,            /* Adresse, wo der Wert abzuspeichern ist.            */
char *value,          /* Zeichenkette mit Standardwert für diesen Parameter.*/
VARTAB **ppvp )       /* Adresse des Pointers auf die Merktabelle           */
  ;                   /* RETURN: = 1, wenn der Name gefunden wurde.         */

char *GetWord(        /* Wort (Token) aus einer Zeichenkette holen.         */
char *s,              /* Zeichenkette, die durchsucht wird.                 */
char*n,               /* Zeichenkette, die das Wort aufnimmt.               */
int ln );             /* Maximale Länge der aufnehmenden Zeichenkette.      */
                      /* RETURN: Pointer auf das nächste Wort.              */   

long ReadZtr(         /* Zeitreihen-File einlesen.                          */
char *fname,          /* Name des Zeitreihen-Files.                         */
char *dname,          /* Alternativer Name des Zeitreihen-Files.            */
long *pt1,            /* Pointer auf Standardwert Anfangszeit.              */
long *pt2,            /* Pointer auf Standardwert Endzeit.                  */
int vrb,              /* verbose level                                      */
long *ppos,           /* Vorgegebene Leseposition.                          */
VARTAB *pvpp )        /* Adresse der Tabelle variabler Parameter            */
  ;
long NamePos(         /* Position eines Namens in einer Liste feststellen   */
char *orgname,        /* Name, nach dem gesucht werden soll.                */
char *namelist[] )    /* Liste, die durchsucht werden soll.                 */
                      /* RETURN: Position in der Liste (0, ... ), oder -1   */
  ;
long EvalHeader(      /* Namen im Tabellen-Header auswerten.                */
char *names[],        /* Liste der möglichen Namen.                         */
char *header,         /* Header der Tabelle.                                */
int *position,        /* Positionen der Header-Namen bzgl. names[].         */
float *scale )        /* Gefundene Faktoren zur Umskalierung ( = 1.0 )      */
                      /* RETURN: Anzahl der Namen im Tabellen-Header.       */
  ;
#endif

/*==========================================================================*/

