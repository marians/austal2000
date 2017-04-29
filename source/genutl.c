//========================================================== genutl.c
//
// Utility functions
// =================
//
// Copyright (C) Umweltbundesamt, Dessau-Ro�lau, Germany, 2002-2013
// Copyright (C) Janicke Consulting, 88662 �berlingen, Germany, 2002-2013
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
// last change 2013-10-14 uj
//
//==================================================================

#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <limits.h>
#include <time.h>
#include <math.h>

#include "IBJmsg.h"
static char *eMODn = "GenUtl";

#include "genutl.h"
#include "genio.h"

/*=================================== Time ===============================*/

#define DAYMAX 24800                                              //-2013-10-14

static const char TmMinString[] = "-inf";
static const char TmMaxString[] = "+inf";
static const long TmMinValue = LONG_MIN;
static const long TmMaxValue = LONG_MAX;

  /*================================================================== TmMax
  */
long TmMax( void )         /* return maximum value of time */
  {
  return TmMaxValue; }

  /*================================================================== TmMin
  */
long TmMin( void )         /* return minimum value of time */
  {
  return TmMinValue; }

  /*=============================================================== TmString
  */
char *TmString(            /* return string representation of time t */
  long *pt )               /* pointer to time t                      */
  {
  static char tstr[40];
  if (!pt)  strcpy( tstr, "NOT");
  else if (*pt == TmMinValue)  strcpy( tstr, TmMinString );
  else if (*pt == TmMaxValue)  strcpy( tstr, TmMaxString );
  else  TimeStr( *pt, tstr );
  return tstr;
  }

  /*================================================================ TmValue
  */
long TmValue(               /* return binary representation of time */
  char *ts )                /* string representation of time        */
  {
  if (!strcmp(ts,TmMinString))  return TmMinValue;
  if (!strcmp(ts,TmMaxString))  return TmMaxValue;
  if ((!strcmp(ts,"UNDEF")) || (!strcmp(ts,"undef")) || (*ts == '?'))
    return TmMinValue;
  return Seconds( ts, 0 );
  }

  /*============================================================== TmRelation
  */
int TmRelation(         /* 1, if data are valid within [*pt1,...]       */
long t1, long t2,       /* start and end of validity time interval      */
long *pt1, long *pt2 )  /* start and end of requested time interval     */
  {
  if (t2 < t1)  t2 = t1;
  if (!pt1)  return 0;
  if (*pt1 < t1)  return -1;
  if (*pt1 > t2)  return 1;
  if (*pt1 == t2) {
    if (t1 < t2)  return 1;
    if ((pt2) && (*pt2 > *pt1))  return 1;      /*-20feb95-*/
    }
  return 0;
  }

/*=================================================================== TimePtr
*/
char *TimePtr(        /* Zeichenkette mit der aktuellen Uhrzeit        */
void )
  {
  static char ts[] = "--:--:--";
  struct tm *ptm;
  time_t secs;
  time( &secs );
  ptm = localtime( &secs );
  sprintf( ts, "%02d:%02d:%02d",
           ptm->tm_hour, ptm->tm_min, ptm->tm_sec );
  return ts; }

/*=================================================================== DatePtr
*/
char *DatePtr(        /* Zeichenkette mit dem aktuellen Datum          */
void )
  {
  static char dt[] = "--.--.----";
  time_t secs;
  struct tm *ptm;
  time( &secs );
  ptm = localtime( &secs );
  sprintf( dt, "%02d.%02d.%04d",
           ptm->tm_mday,
           ptm->tm_mon+1,
           ptm->tm_year + 1900);                          //-00-02-25 uj
  return dt; }

/*================================================================== SetUndef
*/
int TmSetUndef(   /* Markieren einer Zeit als "nicht definiert" */
long *ptr )       /* Pointer auf die zu markierende Zeit        */
  {
  if (ptr) *ptr = TmMaxValue;
  return 0;
}

/*=================================================================== IsUndef
*/
int TmIsUndef(    /* Abfrage, ob eine Zeit "nicht definiert" ist  */
long *ptr )       /* Pointer auf die zu prüfende Zeit             */
{
  if (!ptr)  return 1;
  return (*ptr == TmMaxValue);
}

/*================================================================== SetUndef
*/
int SetUndef(      /* Markieren einer Größe als "nicht definiert" */
float *ptr )       /* Pointer auf die zu markierende Größe        */
  {
  if (ptr) *ptr = HUGE_VAL;
  return 0;
}

/*=================================================================== IsUndef
*/
int IsUndef(       /* Abfrage, ob eine Größe "nicht definiert" ist  */
float *ptr )       /* Pointer auf die zu prüfende Größe             */
{
  if (!ptr)  return 1;
  return (*ptr == HUGE_VAL);
}

/*==================================================================== CisCmp
*/
int CisCmp(           /* Vergleich ohne Rücksicht auf Großschreibung   */
char *p1,             /* Zeichenkette                                  */
char *p2 )            /* Zeichenkette                                  */
                      /* RETURN: 0, wenn beide gleich, sonst 1         */
{
  int result;
  char c1, c2;
  result = 0;
  do {
    c1 = *p1++;
    c2 = *p2++;
    if (toupper(c1) != toupper(c2)) { result = 1;  break; }
    } while( (c1) && (c2) );
  return result;
}

/*=================================================================== TimeStr
*/
void TimeStr(         /* Umwandlung einer Sekundenzahl n in eine Zeichen- */
long n,               /* kette t[] der Form  ddd.hh:mm:ss;                */
char *t )             /* führende Nullen werden fortgelassen.             */
{
  int l = 0;
  long k;
  char b[10];
  if (TmIsUndef(&n)) {
    strcpy( t, "UNDEF" );
    return; }
  t[0] = '\0';
  if (n < 0) {  strcat( t, "-" );  n = -n; }
  k = n / 86400L;
  if (k > 0) {
    sprintf( b, "%ld.", k );  strcat( t, b );  n -= k*86400L;  l++; }
  k = n/3600L;
  sprintf(b, "%02ld:", k);
  strcat(t, b);  n -= k*3600L;  l++;
  k = n/60L;
  sprintf(b, "%02ld:", k);
  strcat(t, b);  n -= k*60L; l++;
  sprintf(b, "%02ld", n);
  strcat(t, b);
  return;
}

/*=================================================================== Seconds
*/
long Seconds(         /* Umwandlung einer Zeitangabe t[] von der Form    */
char *t,              /* ddd.hh:mm:ss in die Anzahl der Sekunden. Wenn   */
long r )              /* die Zeichenkette mit '+' beginnt, wird die Se-  */
                      /* kundenzahl zum zweiten Argument r addiert. Zu-  */
                      /* rueckgegeben wird die Anzahl der Sekunden.      */
  {
  char *d, s[20];
  long z, x, m;
  if ((0 == strcmp(t,"UNDEF")) || (0 == strcmp(t,"undef"))) {
    TmSetUndef(&z);
    return z; }
  strncpy(s, t, 18);  s[18] = '\0';
  if (t[0] == '\0')  return 0L;
  if (t[0] == '+') { s[0] = ' ';  z = r;  m = 1L; }
  else {
    z = 0L;
    if (t[0] == '-') { s[0] = ' ';  m = -1L; }
    else m = 1L; }
  d = strrchr(s, ':');
  if (d != NULL)  {
    x = atol(d+1);  z +=  m*x;
    *d = '\0';
    m *= 60L;
    d = strrchr(s, ':');
    if (d != NULL) {
      x = atol(d+1);  z += m*x;
      *d = '\0';
      m *= 60L;
      d = strrchr(s, '.');
      if (d != NULL) {
        x = atol(d+1);  z += m*x;
        *d = '\0';
        m *= 24L; } } }
  x = atol(s);  
  if (x > DAYMAX) return TmMinValue;                              //-2013-10-14 
  z += m*x;
  return z;
}

/*================================================================== GenTimRel
*/
long GenTimRel(   /* Relation zweier Zeitintervalle feststellen */
long t1,          /* Beginn des gegebenen Intervalls            */
long t2,          /* Ende des gegebenen Intervalls              */
long *pt1,        /* Beginn des gewuenschten Intervalls          */
long *pt2 )       /* Ende des gewuenschten Intervalls            */
{                 /* RETURN: =0, wenn nicht enthalten.          */
  long rc;
  int defbound;
  defbound = 0;
  if (!TmIsUndef(&t1))  defbound |= 0x1000;
  if (!TmIsUndef(&t2))  defbound |= 0x0100;
  if (!TmIsUndef(pt1))  defbound |= 0x0010;
  if (!TmIsUndef(pt2))  defbound |= 0x0001;
  rc = 0;
  switch (defbound) {
    case 0x0000:
    case 0x0001:
    case 0x0010:
    case 0x0011:  rc = TM_INSIDE;
                  break;
    case 0x0100:  *pt2 = t2;  rc = TM_ADJUST | TM_INSIDE;
                  break;
    case 0x0101:  if (*pt2 > t2)  rc = TM_FOLLOW;
                  else  rc = TM_INSIDE;
                  break;
    case 0x0110:  if (*pt1 >= t2)  rc = TM_FOLLOW;
                  else { *pt2 = t2;  rc = TM_INSIDE | TM_ADJUST; }
                  break;
    case 0x0111:  if (*pt2 <= t2)  rc = TM_INSIDE;
                  else if (*pt1 >= t2)  rc = TM_FOLLOW;
                  break;
    case 0x1000:  *pt1 = t1;  rc = TM_ADJUST | TM_INSIDE;
                  break;
    case 0x1001:  if (*pt2 > t1) {
                    *pt1 = t1;  rc = TM_ADJUST | TM_INSIDE; }
                  break;
    case 0x1010:  if (*pt1 >= t1)  rc = TM_INSIDE;
                  break;
    case 0x1011:  if (*pt1 >= t1)  rc = TM_INSIDE;
                  break;
    case 0x1100:  *pt1 = t1;  *pt2 = t2;  rc = TM_ADJUST | TM_INSIDE;
                  break;
    case 0x1101:  if (*pt2 <= t2) {                         /*-- 10feb92 --*/
                    if (*pt2 > t1) { *pt1 = t1;  rc = TM_ADJUST | TM_INSIDE; }
                    }
                  else  rc = TM_FOLLOW;
                  break;
    case 0x1110:  if (*pt1 < t2) {                          /*-- 10feb92 --*/
                    if (*pt1 >= t1) { *pt2 = t2;  rc = TM_ADJUST | TM_INSIDE; }
                    }
                  else  rc = TM_FOLLOW;
                  break;
    case 0x1111:  if ((*pt1 >= t1) && (*pt2 <= t2))  rc = TM_INSIDE;
                  else  if (*pt1 >= t2)  rc = TM_FOLLOW;
                  break;
    }
  return rc;
}

/*=================================================================== SetFlag
*/
long SetFlag(         /* Suche Zeichenkette und setze Flag wenn gefunden */
char *source,         /* Zeichenkette, die durchsucht wird.              */
char *tag,            /* Zeichenkette, nach der gesucht wird.            */
long *pflags,         /* Pointer auf LongInt, wo ein Bit gesetzt wird.   */
long flag )           /* LongInt, die das zu setzende Bit enthält.       */
{
  char *s;
  if (NULL == (s=strstr( source, tag )) )  return 0;
  if ((*s != *source) && (s[-1] == '-'))  *pflags &= ~flag;
  else *pflags |= flag;
  return 0;
}

/*==================================================================== RptChr
*/
long RptChr(           /* Die Zeichenkette d[] wird mit der l-fachen     */
register char c,       /* Wiederholung des Buchstabens c aufgefüllt.     */
register long l,       /* Anschließend wird die Kette mit '\0' beendet.  */
register char *d )
{
  register long i;
  for (i=0; i<l; i++) *d++=c;
  *d='\0';
  return 0;
}

/*=================================================================== CapCode
*/
static long CapCode(  /* Buchstaben c in Großbuchstaben umwandeln. Falls */
char c )              /* c nicht in einem Namen (eigene Definition!)     */
                      /* erlaubt ist, wird 0 zurückgegeben.              */
{
  int i;
  char *s = "._+-0123456789$äÄöÖüÜß[]@/#";
  char *S = "._+-0123456789$ÄÄÖÖÜÜß[]@/#";
  if (isupper(c)) return (long) c;
  if (islower(c)) return (long) toupper(c);
  for (i=strlen(s)-1; i>=0; i--) if (s[i]==c) return (long) S[i];
  return 0;
}

/*===================================================================== ToCap
*/
long ToCap(      /* Buchstaben der Zeichenkette s[] in Großbuchstaben  */
char *s )        /* (vgl. CapCode()! ) umwandeln. Zurückgegeben wird   */
                 /* die Länge von s[].                                 */
{
  int i, k;
  for (i=0; s[i]!='\0'; i++) {
    k = CapCode(s[i]);
    if (k!=0) s[i]= (char) k; }
  return (long)i;
}

/*================================================================== FindName
*/
char *FindName(                 /* Das erste Auftreten des Namens name in */
char *name,                     /* der Zeichenkette source wird gesucht.  */
char *source )                  /* Zurückgegeben wird die Adresse hinter  */
                                /* dem gefundenen Namen oder NULL.        */
{
  char *n, *s, nn[80], *p1, *p2;
  long cn, cs;
  p2 = name;
next_name:
  if (!p2)  return NULL;
  p1 = p2;
  p2 = strchr(p1, '|');
  *nn = 0;
  if (p2) {
    strncat(nn, p1, p2-p1);
    p2++;
    }
  else  strcpy(nn, p1);
  s = source;
  while (*s != '\0') {
    for (n=nn; ; n++,s++) {
      cn = CapCode( *n );
      cs = CapCode( *s );
      if (cn != cs)  break;
      if (cn == 0)  return s;
      }
    while (cs != 0)  cs = CapCode(*++s);
    while (cs == 0) {
      if (*s == '\0')  goto next_name;
      if (*s=='\"')
        for (s++; *s!='\"'; s++)  if (*s == '\0')  goto next_name;
      cs = CapCode(*++s); }
    } /* while (*s!='\0') */
  goto next_name;
}

//================================================================= CheckName
//
int CheckName(  // return 1 if <name> is not a valid name, 0 otherwise
  char *name )  // name to be checked
{
  int i;
  char c, *s = "._+-0123456789$äÄöÖüÜß@/#";
  for (i=strlen(name)-1; i>=0; i--) {
    c = name[i];
    if (isupper(c))  continue;
    if (islower(c))  continue;
    if (strchr(s, c))  continue;
    return 1;
  }
  return 0;
}

/*=================================================================== GetData
*/
long GetData(           /* Einlesen von Daten in der Form NAME=WERT       */
char *n,                /* Name, nach dem gesucht wird.                   */
char *s,                /* Zeichenkette, die durchsucht wird.             */
char *f,                /* Einleseformat, %[ii]... für Vektoren.          */
void *p )               /* Adresse, an welcher der Wert gespeichert wird. */
                        /* RETURN: Anzahl der gelesenen Werte.            */
{
  dP(GetData);
  int r, found;
  char *t, *p1, *p2, *pp, format[10], formchar, nn[80];
  int vector, longitem, maxnum, i, elsize, num, l, slen, nc;    //-00-04-13
  found = 0;
  t = s;
  p1 = n;
  while (!found) {
    p2 = strchr(p1, '|');
    if (p2) {
      *nn = 0;
      l = p2 - p1;
      if (l > 79)  return 0;
      strncat(nn, p1, l);
      }
    else {
      l = strlen(p1);
      if (l > 79)  return 0;
      strcpy(nn, p1);
      }
    t = FindName(nn, t);
    if (t == NULL) {
      if (p2) {
        p1 = p2 + 1;
        t = s;
        continue;
        }
      return 0;
      }
    for ( ; *t <= ' '; t++)  if (*t == '\0')  return 0;
    if (*t=='=' || *t=='#')  found = 1;
    }
  if (f[1] == '[') {
    strcpy( format, "%" );
    vector = 1;
    maxnum = 10;
    sscanf( f+2, "%d", &maxnum );
    for ( f+=2; *f>0; f++) if (*f == ']')  break;
    if (*f == 0)                                        eX(1);
    f++;
    strcat( format, f );
    if (*f == 'l')  { longitem = 1;  f++; }
    else  longitem = 0;
    switch (*f) {
      case 'd':
      case 'x':  elsize = sizeof(int);
                 if (elsize == 2)  if (longitem)  elsize = 4;
                 break;
      case 'f':  elsize = sizeof(float);
                 if (elsize == 4)  if (longitem)  elsize = 8;
                 break;
      case 'c':  elsize = 1;
                 break;
      default:   elsize = 4;
      }
    }
  else {
    vector = 0;
    strcpy( format, f );
    maxnum = 1;
    elsize = 0;
    }
  i = strlen(format) - 1;
  formchar = format[i];
  if (i>1 && format[i-1]=='l')  longitem = 1;         //-2003-03-27
  else                          longitem = 0;
  for ( t++; *t <= ' '; t++ )  if (*t <= 0) break;
  if (*t == '{')  t++;                                  //-98-08-14
  else  maxnum = 1;

  slen = 0;
  if (formchar == 's') {                                //-00-04-13
    sscanf(format+1, "%d", &slen);
    if (slen < 0)  slen = -slen;
  }

  pp = p;
  for (num=0; num<maxnum; ) {
    for ( ; (*t<=' ')||((vector)&&(*t==',')); t++)
      if (*t <= 0) break;
    if ((*t == 0) || (*t == '}'))  break;
    pp = p;
    if ((formchar == 's') && (vector == 1))  pp = *(char**)p;
    if ((formchar != 's') &&
      ((*t=='?') || (0==strncmp(t,"UNDEF",5)) || (0==strncmp(t,"undef",5)))) {
        SetUndef( (void *)pp );  num++;
        if (*t == '?')  t++;
        else  t += 5;
      }
    else
      if ( (formchar == 's') && (*t == '\"') ) {
        nc = 0;
        for (p1=pp, p2=t+1; (*p2); p2++) {
          if (*p2 == '\"') {
            p2++;
            if (*p2 != '\"') { t = p2;  break; }
          }
          if (slen==0 || nc<slen)  *p1++ = *p2;               //-00-06-06
          nc++;                                               //-00-04-13
        }
        *p1 = 0;
        num++;
      }
      else {
        r = sscanf( t, format, pp);
        if (r <= 0)  return r;
        if (formchar == 'f') {                                //-2003-03-27
          if (longitem) {
            if (*(double*)pp == 0.0)  *(double*)pp = 0;
          }
          else {
            if (*(float*)pp == 0.0)  *(float*)pp = 0;
          }
        }
        num++;
        for ( ; *t>' '; t++) {
          if ((vector) && (*t == ',')) break;
          if (*t == '}') break; }
        }
    if (num >= maxnum) break;
    p = (void *)((char*)p + elsize);
    }
  return num;
eX_1:
  eMSG("\"%s\" not found in \"%s\"", n, f);
}

/*================================================================= GetAllData
*/
long GetAllData(      /* Reihe von Daten über GetData() einlesen.           */
char *names,          /* Zeichenkette mit Namen, durch Blank getrennt.      */
char *src,            /* Zeichenkette mit Wertzuweisungen.                  */
char *format,         /* Format zum Einlesen der Daten.                     */
float *dst )          /* Startadresse zum Abspeichern der Float-Werte.      */
                      /* RETURN: Anzahl der übertragenen Daten.             */
  {
  dP(GetAllData);
  char *p1, *p2;
  int last;
  long rc, n;
  last = 0;
  n = 0;
  for (p1=names; *p1!='\0'; p1=p2+1) {
    for (p2=p1+1; *p2>' '; p2++) ;
    if (*p2 == '\0')  last = 1;  else *p2 = '\0';
    rc = GetData( p1, src, format, dst );  if (rc < 0)      eX(1);
    if (rc <= 0) return rc;
    n++;
    if (last) break;
    dst++; }
  return n;
eX_1:
  eMSG("can't get all data!");
  }

/*=================================================================== RplData
*/
long RplData(   /* Wert bei einer Zuweisung ersetzen  */
char *n,        /* Variablen-Name                     */
char *s,        /* Zeichenkette mit Zuweisung         */
char *w,        /* neuer Wert oder NULL               */
char *t )       /* neue Zeichenkette                  */
{
  unsigned char *pn, *p0, *p1, *p2;
  *t = 0;
  p0 = (unsigned char*) s;
  p1 = (unsigned char*) FindName( n, s );
  if (p1 == NULL) {
    return 0;
  }
  pn = p1 - strlen(n);
  for ( ; *p1<=' '; p1++)  if (*p1 == 0) break;
  if (*p1 != '=')  return 0;
  for (p1++; *p1<=' '; p1++)  if (*p1 == 0)  return 0;
  if (*p1 == '{') {
    p2 = (unsigned char*) strchr((char*)(p1+1), '}');             //-2008-12-11
    if (!p2)  return 0;
    p2++; }
  else
    for (p2=p1; *p2>' '; p2++);
  if (w) {
    strncat(t, s, p1-p0);
    strcat(t, w);
  }
  else strncat(t, s, pn-p0);
  strcat(t, (char*)p2);
  return 1;
}

/*=================================================================== GetList
*/
long GetList(   /* Einlesen einer Reihe von Werten aus einer Zeichenkette.*/
char *s,        /* Zeichenkette, welche die Daten (Float-Zahlen) enth�lt. */
char *n,        /* Name vor der Werteliste, durch : oder | abgetrennt.    */
int ln,         /* Maximale L�nge f�r den Namen.                          */
float *v,       /* Float-Vektor, der die Werte aufnimmt.                  */
int lv )        /* Maximale Anzahl der Werte.                             */
                /* RETURN: Anzahl der eingelesenen Werte.                 */
    {
    dP(GetList);
    int i;
    char *pc;
    char *pd;
    while (*s == ' ') s++;
    pc = strchr( s, ':' );
    pd = strchr( s, '|' );
    if ((pc == NULL) && (pd == NULL)) *n = '\0';
    else {
      if (pc == NULL)  pc = pd;
      else
        if ((pd != NULL) && (pd < pc))  pc = pd;
      i = MIN( ln-1, pc-s);
      strncpy( n, s, i);
      for (i--; i>=0; i--)  if (n[i] != ' ') break;
      n[i+1] = '\0';
      if (CheckName(n))                                   eX(1); //-2001-11-08
      s = pc+1;
      }
    for (i=0; i<lv; i++, v++) {
      while ((*s <= ' ') && (*s > 0)) s++;
      if ((*s=='?') || (0==strncmp(s,"UNDEF",5)) || (0==strncmp(s,"undef",5)))
        SetUndef( v );
      else {
        if (1 != sscanf( s, "%f", v)) break;
        if (*v == 0.0)  *v = 0;                                   //-2003-03-27
      }
      while (*s > ' ') s++;
      }
    return (long)i;
eX_1:
    eMSG("\"%s\" is not a valid name!", n);
}

/*======================================================================= Trim
*/
void Trim(               /* Blanks am Anfang und Ende beseitigen  */
unsigned char *t )       /* zu trimmende Zeichenkette             */
{
  int l, ll, i;
  ll = strlen( (char*)t );
  for (l=ll-1; l>=0; l--)  if (t[l] != ' ')  break;
  t[l+1] = 0;
  ll = l+1;
  for (l=0; l<ll; l++)  if (t[l] > ' ') break;
  if (l > 0) {
    for (i=0; i<ll-l; i++)  t[i] = t[i+l];
    t[i] = 0; }
  return;
}

/*==================================================================== StrSort
*/
static void strswap( char *v[], int i, int j )
{
  char *temp;
  temp = v[i];  v[i] = v[j];  v[j] = temp;
  return;
}

void StrSort(    /*  Quicksort für Zeichenketten         */
char *v[],       /*  Pointer-Array auf die Zeichenketten */
int left,        /*  Kleinster Index                     */
int right )      /*  Größter Index                       */
{
  int i, last;
  if (left >= right)  return;
  strswap( v, left, (left+right)/2 );
  last = left;
  for (i=left+1; i<=right; i++)
    if (strcmp(v[i],v[left]) < 0)  strswap( v, ++last, i );
  strswap( v, left, last );
  StrSort( v, left, last-1 );
  StrSort( v, last+1, right );
  return;
}

/*=================================================================== GenForm
*/
char *GenForm(          /* Ascii-Umwandlung einer Zahl  */
double x,               /* Umzuwandelnde Zahl           */
int n)                  /* Anzahl signifikanter Stellen */
  {
  static char rr[10][40];
  static int ir;
  char t[80], *pe, *pp, *r;
  int iexp;
  r = rr[ir];
  ir++;
  if (ir > 9)  ir = 0;
  if (n < 1)  n = 1;
  if (n > 16)  n = 16;
  sprintf(t, "%24.*e", n-1, x);
  pp = strchr(t, '.');
  pe = strchr(t, 'e');
  iexp = atoi(pe+1);
  if ((iexp <= n+2) && (iexp >= n-1)) {
    sprintf(r, "%1.0f", x);
    return r; }
  if ((iexp <= n-2) && (iexp >= -3)) {
    sprintf(r, "%1.*f", n-1-iexp, x);
    n = strlen(r);
    for (n--; n>=0; n--) {
      if (r[n] == '0')  r[n] = 0;
      else  break;
      }
    if (r[n] == '.')  r[n] = 0;
    return r;
    }
  sprintf(pe+1, "%d", iexp);
  for (pp=t; *pp==' '; pp++);
  strcpy(r, pp);
  return r;
  }

/*================================================================== DatAssert
*/
long DatAssert( /* Verify value of GETDATA-Parameter    */
char *name,     /* name of the variable                 */
char *hdr,      /* string containing assignments        */
float val )     /* required value                       */
{
  dP(DatAssert);
  float v;
  if (1 != GetData(name, hdr, "%f", &v))        eX(1);
  if (v != val)                                 eX(2);
  return 0;
eX_1:
  eMSG("can't get value of %s!", name);
eX_2:
  eMSG("improper value of %s!", name);
}

/*========================================================================*/

