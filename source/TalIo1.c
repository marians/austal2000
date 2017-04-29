// ================================================================= TalIo1.c
//
// Handling of input/output
// ========================
//
// Copyright (C) Umweltbundesamt, Dessau-Ro�lau, Germany, 2002-2004
// Copyright (C) Janicke Consulting, 88662 �berlingen, Germany, 2002-2006
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
// last change: 2006-10-26 lj
//
//==========================================================================
 
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "IBJmsg.h"
#include "IBJary.h"
static char *eMODn = "TalIo1";

#include "genutl.h"
#include "genio.h"
#include "TalIo1.h"
#include "TalIo1.nls"

#define  vPRN(a)  MsgVerbose=(MsgLogLevel>=(a)), MsgQuiet=1, vMsg

extern FILE *InputFile;
extern long InputPos;

float DatVec[MAXVAL];
int DatPos[MAXVAL];
float DatScale[MAXVAL];
char Buf[BUFLEN];

static int VrbLevel;

/*===================================================================== DefVar
*/
long DefVar(          /* Namen als VARIABEL in einer Liste registrieren.    */
char n[],             /* Name, der gemerkt werden soll.                     */
char f[],             /* Format zur Umwandlung der zugeordneten Eingabe.    */
void *pv,             /* Adresse, wo der Wert abzuspeichern ist.            */
float fac,            /* Faktor zur Skalierung des Wertes.                  */
VARTAB **pps )        /* Adresse des Pointers auf die Merktabelle.          */
  {
  dP(DefVar);
  VARREC *pt;
  VARTAB *ps;
  int oldmax, newmax, oldnum, newsize;
  ps = *pps;
  if ((!ps) || (ps->num >= ps->max)) {
    if (ps) { oldmax = ps->max;  oldnum = ps->num; }
    else    { oldmax = 0;        oldnum = 0; }
    newmax = oldmax + MAXVPP;
    newsize = sizeof(VARTAB) + (newmax-MAXVPP)*sizeof(VARREC);
    ps = REALLOC(ps, newsize);
    if (!ps)                                                            eX(1);
    memset(ps->tab+oldmax, 0, MAXVPP*sizeof(VARREC));
    *pps = ps;
    ps->num = oldnum;
    ps->max = newmax;
    }
  if (strlen(n) >= MAXVNL)                                              eX(2);  
  pt = ps->tab + ps->num;
  strcpy(pt->name, n);  ToCap(pt->name);
  strcpy(pt->form, f);
  pt->pv = pv;
  pt->f = fac;
  pt->column = -1;
  ps->num++;
  SetUndef(pv);
  return 0;
eX_1:
  eMSG(_no_memory_);
eX_2:
  eMSG(_name_$_too_long_, n);
  }

/*===================================================================== PrnVar
*/
long PrnVar(          /* Test-Ausdruck der gemerkten Namen.                 */
char t[],             /* Auszudruckende Überschrift.                        */
VARTAB *ps,           /* Pointer auf die Merktabelle.                       */
FILE *logfile )       /* Ausgabe-File.                                      */
  {
  int i;
  FILE *lf;
  lf = (logfile) ? logfile : stderr;
  fprintf(lf, "%s\r\n", t);
  for (i=0; i<ps->num; i++)
    fprintf(lf, "%2d: %8p %12.3e %s %d (%s)\r\n",
      i, ps->tab[i].pv, ps->tab[i].f, ps->tab[i].name, 
      ps->tab[i].column, ps->tab[i].form);
  return 0; 
  }

/*===================================================================== FndVar
*/
VARREC *FndVar(       /* In der Merktabelle nach einem Namen suchen.        */
char n[],             /* Name, nach dem gesucht werden soll.                */
VARTAB *ps )          /* Pointer auf die Merktabelle.                       */
                      /* RETURN: Pointer auf den zugehörigen Record.        */
  {
  int i, l, k;
  VARREC *pt;
  char name[256];
  l = strlen(n);  if (l < 1)  return NULL;
  if (n[l-1] != '.')  l = 0;
  for (i=0, pt=ps->tab; i<ps->num; i++, pt++) {
    k = (l) ? strncmp( n, pt->name, l ) : strcmp( n, pt->name);
    if (k == 0)  return pt; }
  l = strlen(n);
  if (n[l-1] != ']') {
    strcpy( name, n );  strcat(name, "[1]");
    for (i=0, pt=ps->tab; i<ps->num; i++, pt++) 
      if (0 == strcmp( name, pt->name))  return pt; 
    }
  return NULL; 
  }

/*==================================================================== DefParm
*/
long DefParm(         /* Rechenparameter einlesen oder Standardwert setzen. */
char *name,           /* Name des einzulesenden Parameters.                 */
char *string,         /* Zeichenkette mit Wertzuweisungen.                  */
char *form,           /* Format zur Umwandlung der Eingabe.                 */
void *ptr,            /* Adresse, wo der Wert abzuspeichern ist.            */
char *value,          /* Zeichenkette mit Standardwert für diesen Parameter.*/
VARTAB **ppvp )       /* Adresse des Pointers auf die Merktabelle           */
                      /* RETURN: = 1, wenn der Name gefunden wurde.         */
  {
  dP(DefParm);
  long rc;
  char nn[80], *pc, eform[20];
  int i, l, vektor;
  void *pp;
  pc = strchr( form, '[' );
  if (pc) { 
    vektor = -1;
    sscanf(pc+1, "%d", &vektor);
    l = strlen(form);
    if ((vektor < 1) || (form[l-1] != 'f'))                     eX(1);
    strcpy( eform, "%" );
    pc = strchr(form, ']');  if (!pc)                           eX(2);
    strcat(eform, pc+1);
    }
  else { vektor = 0;  strcpy(eform, form); }
  if (vektor) 
    for (i=0; i<vektor; i++) {
      pp = ((char*)ptr) + i*sizeof(float);
      if (value == NULL)  SetUndef(pp);
       else  sscanf(value, "%f", (float*)pp);		//-2004-08-30
      }
  else {
    if (value == NULL)  SetUndef(ptr);
    else  sscanf(value, form, ptr);
    }
  rc = GetData(name, string, form, ptr);                        eG(3);
  if ((rc == 0) && (value != NULL) && (*value == 0))            eX(4);
  for (i=0; i<rc; i++) {
    pp = ((char*)ptr) + i*sizeof(float);
    if (IsUndef(pp)) {
      if (vektor)  sprintf(nn, "%s[%d]", name, i+1);
      else  strcpy( nn, name );
      DefVar(nn, eform, pp, 1.0, ppvp);                         eG(5);
      }
    }
  return rc;
eX_1:  eX_2:
  eMSG(_invalid_format_$_, form);
eX_3:
  eMSG(_cant_read_$$_, name, string);
eX_4:
  eMSG(_invalid_$_, name);
eX_5:
  eMSG(_cant_register_$_, nn);
  }

/*==================================================================== GetWord
*/
char *GetWord(        /* Wort (Token) aus einer Zeichenkette holen.         */
char *s,              /* Zeichenkette, die durchsucht wird.                 */
char *n,              /* Zeichenkette, die das Wort aufnimmt.               */
int ln )              /* Maximale Länge der aufnehmenden Zeichenkette.      */
                      /* RETURN: Pointer auf das nächste Wort.              */
  {
  int i;
  *n = '\0';
  for ( ; (*s <= ' ') || (*s == '|'); s++)  if (*s == '\0')  return s;
  for ( i=0; i<ln; i++, s++, n++ ) {
    if ((*s <= ' ') || (*s == '|')) break;
    *n = *s; }
  *n = '\0';
  for ( ; (*s > ' ') && (*s != '|'); s++) ;
  return s; }

/*==================================================================== ReadZtr
*/
long ReadZtr(         /* Zeitreihen-File einlesen.                          */
char *fname,          /* Name des Zeitreihen-Files.                         */
char *dname,          /* Alternativer Name des Zeitreihen-Files.            */
long *pt1,            /* Pointer auf Standardwert Anfangszeit.              */
long *pt2,            /* Pointer auf Standardwert Endzeit.                  */
int vrb,              /* verbose                                            */
long *ppos,           /* Vorgegebene Leseposition.                          */
VARTAB *pvpp )        /* Adresse der Merktabelle                            */
  {
  dP(ReadZtr);
  long rc, t1, t2, pos;
  int i, j, k;
  int open_input, eval_assign, eval_header, seek_position, read_line;
  VARREC *pvr;
  char n[80], n1[80], n2[80], *t;
  VrbLevel = vrb;
  if ((!pt1) || (!pt2))                                                 eX(1);
  if (pvpp->num == 0) { *pt2 = TmMax();  return 1; }
  if (!InputFile) {
    open_input = 1;
    if (ppos) {
      if (*ppos > 0) { eval_assign = 0;  seek_position = 1; }
      else {           eval_assign = 1;  seek_position = 0; }
      read_line = (*ppos >= 0);
      }
    else { eval_assign = 1;  seek_position = 0;  read_line = 1; }
    }
  else {
    open_input = 0;
    if (ppos) {
      if (*ppos > 0) { eval_assign = 0;  seek_position = 1; }
      else {           eval_assign = 1;  seek_position = 0; }
      read_line = (*ppos >= 0);
      }
    else { eval_assign = 0;  seek_position = 0;  read_line = 1; }
    }

  eval_header = 0;
  for (i=0, pvr=pvpp->tab; i<pvpp->num; i++, pvr++)
    if (pvr->column < 0) {
      eval_header = 1;
      break;
      }
  
  if (ppos)  pos = *ppos;
  else  pos = -1;
  strcpy(n1, TmString(pt1));
  strcpy(n2, TmString(pt2));
  vPRN(4)(_reading_$$_$$_, fname, pos, n1, n2);
  if (open_input) {
    OpenInput(fname, dname);                                            eG(2);
    }

  if (eval_assign) {
    rc = GetLine( '.', Buf, BUFLEN );  if (rc < 0)                      eX(3);
    if (rc > 0) {
      for (i=0, pvr=pvpp->tab; i<pvpp->num; i++, pvr++) {
        rc = GetData( pvr->name, Buf, "%s", n );
        if ((rc > 0) && (*n != '?')) {
          ToCap(n);  strcpy( pvr->name, n ); }
        }
      }
    }

  if (seek_position) {
    rc = SeekInput( *ppos );  if (rc != *ppos)                          eX(7);
    }
  if (eval_header) {
    rc = GetLine( '!', Buf, BUFLEN );  if (rc < 1)                      eX(4);
    t = Buf;
    t = GetWord( t, n, 18 );  ToCap( n );
    if (0 != strcmp(n,"T1"))                                            eX(5);
    t = GetWord( t, n, 18 );  ToCap( n );
    if (0 != strcmp(n,"T2"))                                            eX(6);
    pvr = pvpp->tab;
    for (k=0; k<pvpp->num; k++)  
      pvr[k].column = -1;
    for (i=0, k=0; k<pvpp->num; i++) {
      t = GetWord( t, n, MAXVNL );  ToCap( n );
      if (*n == 0)                                                      eX(20);
      pvr = FndVar( n, pvpp );
      if (pvr) { 
        if (pvr->column != -1)                                          eX(21);
        pvr->column = i;  
        k++; }
      }  /* for */
    }  /* if */
  while (read_line) {
    rc = GetLine('Z', Buf, BUFLEN);                                     eG(18);
    if (rc < 0)  {
      if (Buf[0] != '.')                                                eX(8);
      rc = 0;
      }
    t1 = TmMin();
    t2 = *pt2;
    if (rc > 0) {
      t = Buf;
      t = GetWord( t, n1, 18 );  if (*n1 == '\0')                       eX(9);
      t1 = TmValue(n1);
      if (*pt1 == TmMin())  *pt1 = t1;
      if (t1 > *pt1)                                                    eX(10);
      t = GetWord( t, n2, 18 );  if (*n2 == '\0')                       eX(11);
      t2 = TmValue(n2);
      if (t2 == TmMin())  t2 = TmMax();
      if (t2 <= t1)                                                     eX(13);
      if (t2 <= *pt1)  continue;
      *pt2 = t2;
      vPRN(5)("inserting values of [%s,%s]", n1, n2);
      for (i=0, k=0; k<pvpp->num; i++) {
        t = GetWord( t, n, 78 );  if (*n == 0)                          eX(14);
        for (j=0, pvr=pvpp->tab; j<pvpp->num; j++, pvr++)
          if (pvr->column == i)  break; 
        if (j >= pvpp->num)  continue;
        if (pvr->pv == NULL)                                            eX(15);
        rc = sscanf( n, pvr->form, pvr->pv );  if (rc < 1)              eX(16);
        k++; }
      rc = 1;
      pos = InputPos;
      }
    else {
      rc = 0; 
      }
    break;
    }
  if (ppos)  *ppos = pos;
  if (open_input) {
    CloseInput();                                                       eG(17); 
    vPRN(3)(_closing_$$_, fname, pos );
    }
  return rc; 
eX_20: eX_21:
  PrnVar(_time_dependent_, pvpp, MsgFile); 
  eMSG(_undefined_parameters_);
eX_1:
  eMSG(_undefined_interval_);
eX_2:
  eMSG(_cant_open_$_, fname);
eX_3:
  eMSG(_invalid_structure_);
eX_4:
  eMSG(_no_header_);
eX_5:
  eMSG(_no_t1_);
eX_6:
  eMSG(_no_t2_);
eX_7:
  eMSG(_cant_position_$$_, fname, *ppos);
eX_8:
  eMSG(_invalid_tag_, fname);
eX_9:
  eMSG(_undefined_t1_);
eX_10:
  eMSG(_invalid_t1_);
eX_11:
  eMSG(_undefined_t2_);
eX_13:
  eMSG(_invalid_t2_);
eX_14: eX_15: eX_16:
  eMSG(_undefined_parameters_);
eX_17:
  eMSG(_cant_close_$_, fname);
eX_18:
  eMSG(_cant_read_$_, fname);
  }

/*==================================================================== NamePos
*/
long NamePos(         /* Position eines Namens in einer Liste feststellen   */
char *orgname,        /* Name, nach dem gesucht werden soll.                */
char *namelist[] )    /* Liste, die durchsucht werden soll.                 */
                      /* RETURN: Position in der Liste (0, ... ), oder -1   */
  {
  dP(NamePos);
  int i, k, l, n;
  char t[80], name[80], *s;
  strcpy( name, orgname );  ToCap( name );
  l = strlen( name );  if (l == 0)                              eX(1);
  n = (name[l-1] == '.') ? l : 0;
  if (*name != '.') {
    for (i=0; ; i++) {
      if (namelist[i] == NULL) break;
      strcpy(t, namelist[i]);  ToCap( t );
      k = ( (n) ? strncmp( t, name, n ) : strcmp( t, name ) );
      if (k == 0)  return i; }
    return -1; }
  for (i=0; ; i++) {
    strcpy(t, namelist[i]);  ToCap( t );  s = t;
    while (1) {
      s = strchr( s+1, '.' );
      if (s == NULL) break;
      k = ( (n) ? strncmp( s, name, n ) : strcmp( s, name ) );
      if (k == 0)  return i;
      }
    }
eX_1:
  eMSG(_missing_name_);
  }

/*================================================================= EvalHeader
*/
long EvalHeader(      /* Namen im Tabellen-Header auswerten.                */
char *names[],        /* Liste der moeglichen Namen.                         */
char *header,         /* Header der Tabelle.                                */
int *position,        /* Positionen der Header-Namen bzgl. names[].         */
float *scale )        /* Gefundene Faktoren zur Umskalierung ( = 1.0 )      */
                      /* RETURN: Anzahl der Namen im Tabellen-Header.       */
  {
  dP(EvalHeader);
  char name[80];
  int i, n;
  header = GetWord( header, name, 78 );
  for (n=0; ; n++, position++, scale++) {
    header = GetWord( header, name, 78 );
    if (*name == 0) break;
    if (n >= MAXVAL)                                            eX(1);
    i = NamePos( name, names );
    if (i < 0)  { *position = -1;  *scale = 1.0; }
    else { *position = i;  *scale = 1.0; }
    }
  return n;
eX_1:
  eMSG(_name_count_$_, MAXVAL);
  }

//==========================================================================
//
// history:
// 
// 2002-09-24 lj  final release candidate
// 2006-10-26 lj  external strings
//
//==========================================================================
