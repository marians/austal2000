//======================================================== IBJdmn.c
//
// Data Manager for IBJ-programs
// =============================
//
// Copyright (C) Umweltbundesamt, Dessau-Roﬂlau, Germany, 2002-2006
// Copyright (C) Janicke Consulting, 88662 ‹berlingen, Germany, 2002-2013
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
// last change: 2013-10-14 uj
//
//==================================================================

char *IBJdmnVersion = "2.2.3";
static char *eMODn = "IBJdmn";

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <fcntl.h>
#include <limits.h>
#include <locale.h>
#include <math.h>

#include <errno.h>

#include "IBJtxt.h"
#include "IBJmsg.h"
#include "IBJary.h"
#include "zlib.h"
#include "IBJdmn.h"
#include "IBJdmn.nls"

#if defined __WIN32__   // MinGW
  #define LSPEC "I64"
#else                   // GNU-C, MSC
  #define LSPEC "ll"
#endif

#define DAYMAX 24800                                              //-2013-10-14

#define  STDEXT  ".dmna"
#define  BINEXT  ".dmnb"
#define  TXTEXT  ".dmnt"
#define  CMPEXT  ".gz"                                        //-2005-11-07
#define  LISTSEP ";"
#define  STDFRM  " %11.3e"
#define  CR      "\r"
#define  LF      "\n"
#define  CRLF    "\r\n"
#define  SEPCHR  " ;\t"

typedef struct drop { // drop condition for reading text data
  int equal;
  int any;
  int count;
  char letters[64];
} DROP;

DMNFRMREC *DmnFrmTab;

static char *DimParms =
"lowb,hghb,size,dims,fact,mode,file,sequ,form,lsbf,locl,cmpr,data,tmzn,drop,";
static int DatLen[] = { 0, 1, 1, 2, 4, 4, 4, 8, 8, 8 };

//=================================== Time ================================

static long TmMax( void )
;
static long TmMin( void )
;
static char *TmString( long* )
;
static long TmValue( char* )
;
static void TimeStr( long, char* )
;
static long Seconds( char*, long )
;

static const char TmMinString[] = "-inf";
static const char TmMaxString[] = "+inf";
static const long TmMinValue = LONG_MIN;
static const long TmMaxValue = LONG_MAX;

static FILE *ModErr; //-2004-08-20

/*================================================================== TmMax
*/
static long TmMax( void )  /* return maximum value of time */
  {
  return TmMaxValue; }

/*================================================================== TmMin
*/
static long TmMin( void )   /* return minimum value of time */
  {
  return TmMinValue; }

/*=============================================================== TmString
*/
static char *TmString(     /* return string representation of time t */
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
static long TmValue(        /* return binary representation of time */
  char *ts )                /* string representation of time        */
  {
  if (!strcmp(ts,TmMinString))  return TmMinValue;
  if (!strcmp(ts,TmMaxString))  return TmMaxValue;
  if ((!strcmp(ts,"UNDEF")) || (!strcmp(ts,"undef")) || (*ts == '?'))
    return TmMinValue;
  return Seconds( ts, 0 );
  }

/*=================================================================== TimeStr
*/
static void TimeStr(  /* Umwandlung einer Sekundenzahl n in eine Zeichen- */
long n,               /* kette t[] der Form  ddd.hh:mm:ss;                */
char *t )             /* fuehrende Nullen werden fortgelassen.            */
  {
  int l = 0;
  long k;
  char b[10];
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
  return; }

/*=================================================================== Seconds
*/
static long Seconds(  /* Umwandlung einer Zeitangabe t[] von der Form    */
char *t,              /* ddd.hh:mm:ss in die Anzahl der Sekunden. Wenn   */
long r )              /* die Zeichenkette mit '+' beginnt, wird die Se-  */
                      /* kundenzahl zum zweiten Argument r addiert. Zu-  */
                      /* r√ºckgegeben wird die Anzahl der Sekunden.       */
  {
  char *d, s[20];
  long z, x, m;
  strncpy(s, t, 18);  s[18] = 0;
  if (t[0] == 0)  return 0;
  if (t[0] == '+') { s[0] = ' ';  z = r;  m = 1; }
  else {
    z = 0;
    if (t[0] == '-') { s[0] = ' ';  m = -1; }
    else m = 1;
  }
  d = strrchr(s, ':');
  if (d != NULL)  {
    x = atol(d+1);  z +=  m*x;
    *d = 0;
    m *= 60;
    d = strrchr(s, ':');
    if (d != NULL) {
      x = atol(d+1);  z += m*x;
      *d = 0;
      m *= 60;
      d = strrchr(s, '.');
      if (d != NULL) {
        x = atol(d+1);  z += m*x;
        *d = 0;
        m *= 24;
      }
    }
  }
  x = atol(s);
  if (x > DAYMAX) return TmMinValue;                              //-2013-10-14 
  z += m*x;
  return z;
}

static double get_shift(char *tmzn) {  // time shift in days
  double d = 0;
  int l, n, hours=0, minutes=0;
  if (tmzn == NULL)  return 0;
  l = strlen(tmzn);
  if (l < 5)  return 0;
  if (strncmp(tmzn, "GMT", 3))  return 0;
  if (!strchr("+-", tmzn[3]))  return 0;
  if (strchr(tmzn+4, ':'))                                      //-2006-01-19
    n = sscanf(tmzn+4, "%2d:%2d", &hours, &minutes);
  else
    n = sscanf(tmzn+4, "%2d%2d", &hours, &minutes);
  if (n < 1)  return 0;
  if (n < 2)  minutes = 0;
  d = hours*60 + minutes;
  if (tmzn[3] == '-') d = -d;
  d /= 24*60;
  return d;
}

static DROP* get_drop(char *s) {
  static DROP drop;
  char *pc;
  drop.equal = -1;
  drop.any = 0;
  drop.count = 0;
  drop.letters[0] = 0;
  if (!s || !*s)  return NULL;
  if (isdigit(*s)) {
    sscanf(s, "%d", &drop.count);
    pc = strchr(s, ';');                                          //-2005-12-11
    if (!pc)  return &drop;
    s = pc + 1;
  }
  if (*s == '=')
    drop.equal = 1;
  else if (*s == '!')
    drop.equal = 0;
  else return NULL;
  if (s[1] == '[') {
    if (!s[2])  return NULL;
    drop.any = strlen(s) - 3;
    strcpy(drop.letters, s+2);
    drop.letters[drop.any] = 0;
  }
  else {
    drop.any = 0;
    strcpy(drop.letters, s+1);
  }
  return &drop;
}

static int check_drop(DROP *pd, char *s) {
  if (pd->count > 0) {
    pd->count--;
    return 1;
  }
  if (pd->equal < 0)   return 0;
  if (pd->any) {
    if (pd->equal) {
      if (strchr(pd->letters, '@') && isalpha(*s)) return 1;
      if (strchr(pd->letters, *s))  return 1;
    }
    else {
      if (strchr(pd->letters, '@') && !isalpha(*s)) return 1;
      if (!strchr(pd->letters, *s))  return 1;
    }
  }
  else {
    int ll = strlen(pd->letters);
    if (strlen(s) < ll) return -1;
    if (strncmp(s, pd->letters, ll))  return (pd->equal == 0);
    else return (pd->equal != 0);
  }
  return 0;
}

//===============================================================================

char *DmnGetPath( char *fn )
{
  static char path[256];
  char *pc;
  *path = 0;
  if (!fn)  return path;
  strcpy(path, fn);
  for (pc=path; (*pc); pc++)  if (*pc == '\\')  *pc = '/';
  pc = strrchr(path, '/');
  if (pc) {
    pc[1] = 0;
    return path;
  }
  pc = strchr(path, ':');
  if (pc) {
    pc[1] = 0;
    return path;
  }
  strcpy(path, "./");
  return path;
}

char *DmnGetFile( char *fn )
{
  static char file[256];
  char *pc;
  *file = 0;
  if (!fn)  return file;
  strcpy(file, fn);
  for (pc=file; (*pc); pc++)  if (*pc == '\\')  *pc = '/';
  pc = strrchr(file, '/');
  if (pc)  return pc+1;
  pc = strchr(file, ':');
  if (pc)  return pc+1;
  return file;
}

int DmnFileExist( char *fn )
{
  FILE *f;
  f = fopen(fn, "r");
  if (f) {
    fclose(f);
    return 1;
  }
  return 0;
}

//===============================================================================

void DmnReplaceEscape(  // replace ansi escape codes
char *s )               // source and destination string
{
  char *p1, *p2;
  for (p1=s, p2=s; (*p2); ) {
    if (*p2 == '\\') {
      switch (*++p2) {
        case 'b':   *p1 = '\b';  p2++;
                    break;
        case '\\':  *p1 = '\\';  p2++;
                    break;
        case '~':   *p1 = ' ';   p2++;
                    break;
        case 't':   *p1 = '\t';  p2++;
                    break;
        case 'r':   *p1 = '\r';  p2++;
                    break;
        case 'n':   *p1 = '\n';  p2++;
                    break;
        case 'f':   *p1 = '\f';  p2++;
                    break;
        case 'v':   *p1 = '\v';  p2++;
                    break;
        case '\'':  *p1 = '\'';  p2++;
                    break;
        case '\"':  *p1 = '\"';  p2++;
                    break;
        default:    *p1 = '\\';
      }
      p1++;
    }
    else  *p1++ = *p2++;
  }
  *p1 = 0;
}

//==================================================================== DmnAnaForm
DMNFRMREC *DmnAnaForm(  // analyse format combination
char *frm,              // extended format
int *psize )            // record size
  {
  dP(DmnAnaForm);
  enum DAT_TYPE dt;
  char *pp, *pa, *pb, *pc, *ps;
  char name[40], option[40], lopt[40], lfield[20], lspec, fspec, fm[256];
  int i, n, nn, np, m, l, width, prcsn, sz, nform;
  double fac;
  DMNFRMREC *frmtab=NULL;                                       //-2001-04-29 lj
  nform = 0;
  pa = frm;
  ps = pa;
  n = 0;
  nn = 0;
  while (NULL != (pp = strchr(pa, '%'))) {
    l = 1;
    if (pp[1] == '[')  sscanf(pp+2, "%d", &l);
    n++;
    nn += l;
    pa = pp + 1;
    }
  pa = frm;
  np = n;
  frmtab = ALLOC((nn+1)*sizeof(DMNFRMREC));  if (!frmtab)       eX(1);
  nform = nn;
  nn = 0;
  sz = 0;
  for (n=0; n<np; n++) {
    for (pp=pa; *pp!='%'; pp++)  if (!*pp)                      eX(2);
    *name = 0;
    l = pp - pa;
    if (l > 39) l = 39;
    strncat(name, pa, l);
    m = 1;
    ps = pp;
    if (ps[1] == '[') {
      pb = strchr(ps+2, ']');  if (!pb)                         eX(3);
      if (1 != sscanf(ps+2, "%d", &m))                          eX(4);
      if (m < 0)                                                eX(5);
      ps = pb;
    }
    fac = 0;
    *option = 0;
    if (ps[1] == '(') {
      pb = strchr(ps+2, ')');  if (!pb)                         eX(6);
      strncat(option, ps+2, pb-ps-2);                           //-2001-04-26 lj
      if (ps[2] == '*') {
        sprintf(lopt, "fac %s", option+1);
        if (1 != DmnGetDouble(lopt, "fac", "%lf", &fac, 1))     eX(8);  //-2005-06-28
      }
      else if (ps[2] == '+') {                                  //-2001-04-26 lj
        fac = MsgDateVal(option);
      }
      else                                                      eX(7);
      ps = pb;
      }
    ps++;
    l = strspn(ps, "+-.0123456789");  if (l > 19)               eX(9);
    *lfield = 0;
    width = -1;
    prcsn = 0;
    if (l) {
      strncat(lfield, ps, l);
      if (1 != sscanf(lfield, "%d", &width))                    eX(10);
      pc = strchr(lfield, '.');
      if (pc) {
        if (1 != sscanf(pc+1, "%d", &prcsn))                    eX(10);
      }
      ps += l;
      }
    if (strchr("bhlL", *ps)) {                                  //-2005-12-08
      lspec = *ps;
      ps++;
      }
    else  lspec = 0;
    fspec = *ps;
    if (fspec == 's') {
      dt = chDAT;
      if (prcsn <= 0) {
        prcsn = width;
        if (prcsn < 0)  prcsn = -prcsn;
        //else                                                  eX(21);
        sprintf(lfield, "%d.%d", width, prcsn);
      }
    }
    else if (strchr("dioux", fspec)) {
      if      (lspec == 'b')  dt = btDAT;                       //-2001-03-22 lj /uj
      else if (lspec == 'h')  dt = siDAT;
      else if (lspec == 'L')  dt = liDAT;
      else                    dt = inDAT;
      }
    else if (strchr("efg", fspec)) {
      if (lspec == 'l')  dt = lfDAT;
      else               dt = flDAT;
      }
    else if (fspec == 't') {
      if (lspec == 'l')  dt = ltDAT;
      else               dt = tmDAT;
      if (width < 0)  width = (dt == ltDAT) ? 19 : 12;
      }
    else                                                        eX(11);
    for (i=0; i<m; i++) {
      l = strlen(name);
      strcpy(frmtab[nn].name, name);
      frmtab[nn].dt = dt;
      if (dt == chDAT) {                                        //-2001-04-30 uj
        frmtab[nn].ld = DatLen[dt]*prcsn;
      }
      else
        frmtab[nn].ld = DatLen[dt];
      frmtab[nn].lf = width;
      frmtab[nn].offset = sz;
      frmtab[nn].fac = fac;
      frmtab[nn].fspc = fspec;
      strcpy(frmtab[nn].opt, option);
      //
      strcpy(fm, "%");
      strcat(fm, lfield);
      strncat(fm, &lspec, 1);
      strncat(fm, &fspec, 1);
      strcpy(frmtab[nn].oform, fm);
      //
      if (fspec == 't') {
        if (width) sprintf(fm, "%%%ds", width);
        else strcpy(fm, "%s");
      }
      else if (fspec == 's') {
        sprintf(fm, "\"%s\"", frmtab[nn].oform);
      }
      else {
        strcpy(fm, "%");
        strcat(fm, lfield);
        if (lspec != 'b') {
          if (lspec == 'L')
           strcat(fm, LSPEC);
          else
            strncat(fm, &lspec, 1);
        }
        strncat(fm, &fspec, 1);
      }
      strcpy(frmtab[nn].pform, fm);
      //
      strcpy(fm, "%");
      if (lspec != 'b' && fspec != 't') {
        if (lspec == 'L')
          strcat(fm, LSPEC);
        else
          strncat(fm, &lspec, 1);
      }
      if (fspec == 't')  strcat(fm, "s");
      else {
        strncat(fm, &fspec, 1);
      }
      strcpy(frmtab[nn].iform, fm);
      //
      if (l > 0)  name[l-1]++;
      sz += frmtab[nn].ld;
      nn++;
      }
    pa = ps + 1;
    }
  if (!*psize)  *psize = sz;
  else if (*psize != sz)                eX(20);
  return frmtab;
eX_1:
  nMSG(_cant_allocate_format_table_$_, nn);
  return NULL;
eX_2: eX_3: eX_4: eX_5: eX_6: eX_7: eX_8: eX_9: eX_10: eX_11:
  nMSG(_invalid_format_string_);
  vMsg("%s", frm);
  vMsg("%*s%*s", pa-frm+1, "|", ps-pa, "^");
  return NULL;
eX_20:
  DmnPrnForm(frmtab, _in_input_file_);
  nMSG(_inconsistent_element_size_$$_, sz, *psize);
  return NULL;
  }

//============================================================= DmnPrnForm
//
int DmnPrnForm(   // print table of formats
DMNFRMREC *pf,    // pointer to table of formats
char *s )         // comment
  {
  int n;
  if (!pf)  return 0;
  printf(_print1_table_$_, s);
  printf("%s", _print2_table_);
  for (n=1; (pf->dt); pf++, n++)
    printf(_print3_table_$$$$$$$$$_, n, pf->fspc, pf->name, pf->ld,
      pf->lf, pf->fac, pf->oform, pf->iform, pf->pform);
  return n;
  }

//=============================================================== DmnFrmPointer
//
DMNFRMREC *DmnFrmPointer( // get record pointer of element
DMNFRMREC *pfrm,          // format table for the record
char *name )              // name of the element (case ignored)
{                         // RETURN: pointer or NULL, if not found
  char n1[40], n2[40];
  if (!pfrm)  return NULL;
  if (strlen(name) >= 40)  return NULL;
  strcpy(n1, name);
  MsgLow(n1);
  while (pfrm->dt) {
    strcpy(n2, pfrm->name);
    MsgLow(n2);
    if (!strcmp(n1, n2))  return pfrm;
    pfrm++;
  }
  return NULL;
}


//================================================================== DmnPrintR
int DmnPrintR( // print data record to a file
void *f,       // file pointer
int (*prn)(void*, const char*, ...),  // print routine
DMNFRMREC *pf, // format table
double fact,   // overal factor
double shift,  // time shift in days
void *pr)      // pointer to data record
{              // RETURN: number of items written out
  int n;
  char c, *cp;                                           //-2001-04-30 uj
  double d,  fc;
  if (fact <= 0)  fact = 1;
  n = 0;

  while (pf->dt) {
    if (pf->lf != 0) {                                          //-2005-12-06
      prn(f, " ");
      fc = (pf->fac != 0) ? pf->fac : fact;
      switch (pf->dt) {
        case chDAT: cp = (char*)pr;                             //-2001-04-30 uj
                    prn(f, pf->pform, cp);
                    break;
        case btDAT: c = *(char*)pr;                             //-2001-04-30 uj
                    prn(f, pf->pform, c);
                    break;
        case siDAT: prn(f, pf->pform, *(short int*)pr);
                    break;
        case inDAT: prn(f, pf->pform, *(int*)pr);
                    break;
        case liDAT: prn(f, pf->pform, *(long long*)pr);
                    break;
        case flDAT: prn(f, pf->pform, fc*(*(float*)pr));
                    break;
        case lfDAT: prn(f, pf->oform, fc*(*(double*)pr));
                    break;
        case tmDAT: if (pf->fac > 0) {
                      d = MsgDateIncrease(pf->fac, *(int*)pr);
                      prn(f, pf->pform, MsgDateString(d));
                    }
                    else  prn(f, pf->pform, TmString((long*)pr));
                    break;
        case ltDAT: prn(f, pf->pform, MsgDateString(*(double*)pr + shift));
                    break;
        default:    ;
      }
      n++;
    }
    pr = (char*)pr + pf->ld;
    pf++;
  }
  return n;
}

/*================================================================ DmnFindName
*/
char *DmnFindName(  // Find a parameter name in the header
char *buf,          // header (lines separated by \n)
char *name )        // parameter name
  {                 // RETURN: pointer to name string
  char *s, *t, white[] = " =\t\n\r", nn[256], *pn, *ps;
  int l, i;
  if ((!buf) || (!name))  return NULL;
  if (strlen(name) > 255)  return NULL;
  strcpy(nn, name);
  pn = nn;
  while (pn) {
    ps = strchr(pn, '|');                                       //-2001-09-10
    if (ps)  *ps = 0;
    l = strlen(pn);
    if (!l)  return NULL;
    for (s=buf; (*s); s++) {
      t = s;
      for (i=0; i<l; i++, s++)
        if (tolower(*s) != tolower(pn[i]))  break;
      if ((i >= l) && (strchr(white, *s)))  break;
      if (!*s) break;
      s = strchr(s, '\n');
      if (!s) break;
      }
    if ((s) && (*s))  return t;
    pn = ps;
    if (pn)  pn++;
  }
  return NULL;
}

//=============================================================== DmnGetValues
char *DmnGetValues(   // Get the value string of a parameter
char *hdr,            // header
char *name,           // name of the parameter
int *pl )             // set to length of the value string
  {                   // RETURN: pointer to value string
  char *s, *e, *c, white[] = " =\t\n\r";
  int ic=-1;
  *pl = 0;
  s = DmnFindName(hdr, name);
  if (!s)  return NULL;
  for ( ; (*s); s++)  if (strchr(white, *s))  break;
  if (!*s)  return s;
  for ( ; (*s); s++)  if (!strchr(white, *s))  break;
  if (!*s)  return s;
  //-2006-10-20
  c = s;
  while (*c && *c != '\n') {
    if (*c == '\"') ic *= -1;
    else if (ic<0 && *c=='\'') break;
    c++;
  }
  if (*c == '\'')
    e = c-1;
  //
  else {
    e = strchr(s, '\n');
    if (!e)  e = s + strlen(s);
  }
  while (*e <= ' ')  e--;
  *pl = e - s + 1;
  return s;
  }

//=============================================================== DmnRplValues
int DmnRplValues(   // Replace the value string of a parameter
TXTSTR *phdr,       // header
char *name,         // name of the parameter
char *values )      // new values (or NULL to delete)
{                   // RETURN: 1 = added, 2 = replaced or deleted
  char *pn, *pc;
  int r = 0;
  TXTSTR newhdr = { NULL, 0 };
  pc = phdr->s;
  pn = DmnFindName(pc, name);
  if (pn) {
    TxtNCat(&newhdr, pc, pn-pc);
    if (values) {
      TxtCat(&newhdr, name);
      TxtCat(&newhdr, "  ");                                      //-2014-01-28
      TxtCat(&newhdr, values);
    }
    pn = strchr(pn, '\n');
    if (values) {
      if (pn)  TxtCat(&newhdr, pn);
      else     TxtCat(&newhdr, "\n");
    }
    else {
      if (pn && *(++pn) != 0) TxtCat(&newhdr, pn);
    }
    TxtClr(phdr);
    *phdr = newhdr;
    r = 2;
  }
  else if (values) {
    TxtCat(phdr, name);
    TxtCat(phdr, "  ");                                           //-2014-01-28
    TxtCat(phdr, values);
    TxtCat(phdr, "\n");
    r = 1;
  }
  return r;
}

//=============================================================== DmnRplName
int DmnRplName(     // Replace the name string of a parameter
TXTSTR *phdr,       // header
char *oldname,      // old name of the parameter
char *newname )     // new name (or NULL to delete)
{                   // RETURN: 0 = not found, 1 = replaced, 2 = deleted
  char *pn, *pc;
  int r = 0;
  TXTSTR newhdr = { NULL, 0 };
  pc = phdr->s;
  pn = DmnFindName(pc, oldname);
  if (!pn)  return 0;
  TxtNCat(&newhdr, pc, pn-pc);
  if (newname) {
    TxtCat(&newhdr, newname);
    TxtCat(&newhdr, pn+strlen(oldname));
    r = 1;
  }
  else {
    pc = strchr(pn, '\n');
    if (pc)  TxtCat(&newhdr, pc+1);
    r = 2;
  }
  TxtClr(phdr);
  *phdr = newhdr;
  return r;
}

//=============================================================== DmnRplChar
int DmnRplChar(     // Replace a character in the value string
TXTSTR *phdr,       // header
char *name,         // parameter name
char old,           // old character
char new )          // new character
{                   // RETURN: number of replacements
  char *s, *pc;
  int i, n, l;
  if (!phdr)  return 0;
  s = phdr->s;
  if (!s)  return 0;
  pc = DmnGetValues(s, name, &l);
  if (!pc)  return 0;
  n = 0;
  for (i=0; i<l; i++) {
    if (pc[i] == old) {
      pc[i] = new;
      n++;
    }
  }
  return n;
}

//================================================================= DmnGetCount
int DmnGetCount(  // Get the number of tokens of a parameter
char *hdr,        // header
char *name )      // name of the parameter
{                 // RETURN: number of tokens for this parameter
  int i, l;
  char *s, *pc, *pn;
  TXTSTR t = { NULL, 0 };                                         //-2011-06-29
  s = DmnGetValues(hdr, name, &l);
  if (!s)  return -1;
  t.s = NULL;
  TxtNCpy(&t, s, l);
  pc = strtok(t.s, SEPCHR);
  for (i=0; ; i++) {
    pc = MsgTok(pn, &pn, SEPCHR, &l);
    if (!pc)  break;
  }
  TxtClr(&t);
  return i;
}

//================================================================= DmnGetInt
int DmnGetInt(  // Get integer values of a parameter
char *hdr,      // header
char *name,     // name of the parameter
char *frm,      // format for reading
int *ii,        // integer vector
int n )         // maximum number of positions in ii[]
{               // RETURN: number of values read
  int i, l;
  char *s, *pc;
  TXTSTR t = { NULL, 0 };                                         //-2011-06-29
  s = DmnGetValues(hdr, name, &l);
  if (!s)  return -1;
  TxtNCpy(&t, s, l);
  pc = strtok(t.s, SEPCHR);
  for (i=0; i<n; i++) {
    if (!pc)  break;
    sscanf(pc, frm, ii+i);
    pc = strtok(NULL, SEPCHR);
  }
  TxtClr(&t);
  return i;
}

//================================================================= DmnPutInt
int DmnPutInt(  // Put integer values of a parameter into header
char *hdr,      // header
int len,        // maximum length of header string
char *name,     // name of the parameter
char *frm,      // format for writing (with separator!)
int *ii,        // integer vector
int n )         // number of values to write
{               // RETURN: number of values written
  int i;
  char buf[80];
  if (strlen(hdr)+strlen(name)+1 > len)  return -1;
  strcat(hdr, name);
  for (i=0; i<n; i++) {
    sprintf(buf, frm, ii[i]);
    if (strlen(hdr)+strlen(buf)+1 > len)  return -1;
    strcat(hdr, buf);
  }
  return i;
}

//================================================================= DmnGetFloat
int DmnGetFloat(  // Get float values of a parameter
char *hdr,        // header
char *name,       // name of the parameter
char *frm,        // format for reading
float *ff,        // float vector
int n )           // maximum number of positions in ff[]
{                 // RETURN: number of values read
  int i, l=0;
  char *s, *pc, *ps, locale[256];
  TXTSTR t = { NULL, 0 };                                         //-2011-06-29
  s = DmnGetValues(hdr, name, &l);
  if (!s || l<=0)
    return 0;																								                          			//-2011-06-29
  TxtNCpy(&t, s, l);
  strcpy(locale, setlocale(LC_NUMERIC, NULL));
  setlocale(LC_NUMERIC, "C");
  pc = strtok(t.s, SEPCHR);
  for (i=0; i<n; i++) {
    if (!pc)  break;
    for (ps=pc; (*ps); ps++)  if (*ps == ',')  *ps = '.';
    if (1 != sscanf(pc, frm, ff+i))													                  //-2011-06-29
      return -1;
    pc = strtok(NULL, SEPCHR);
  }
  TxtClr(&t);
  setlocale(LC_NUMERIC, locale);
  return i;
}

//================================================================= DmnPutFloat
int DmnPutFloat(  // Put float values of a parameter into header
char *hdr,        // header
int len,          // maximum length of header string
char *name,       // name of the parameter
char *frm,        // format for writing (with separator!)
float *ff,        // float vector
int n )           // number of values to write
{                 // RETURN: number of values written
  int i;
  char buf[80];
  if (strlen(hdr)+strlen(name)+1 > len)  return -1;
  strcat(hdr, name);
  for (i=0; i<n; i++) {
    sprintf(buf, frm, ff[i]);
    if (strlen(hdr)+strlen(buf)+1 > len)  return -1;
    strcat(hdr, buf);
  }
  return i;
}

//================================================================= DmnGetDouble
int DmnGetDouble( // Get double values of a parameter
char *hdr,        // header
char *name,       // name of the parameter
char *frm,        // format for reading
double *ff,       // double vector
int n )           // maximum number of positions in ff[]
{                 // RETURN: number of values read
  int i, l;
  char *s, *pc, *ps, locale[256];
  TXTSTR t = { NULL, 0 };                                         //-2011-06-29
  s = DmnGetValues(hdr, name, &l);
  if (!s)  return -1;
  TxtNCpy(&t, s, l);
  strcpy(locale, setlocale(LC_NUMERIC, NULL));
  setlocale(LC_NUMERIC, "C");
  pc = strtok(t.s, SEPCHR);
  for (i=0; i<n; i++) {
    if (!pc)  break;
    for (ps=pc; (*ps); ps++)  if (*ps == ',')  *ps = '.';
    sscanf(pc, frm, ff+i);
    pc = strtok(NULL, SEPCHR);
  }
  TxtClr(&t);
  setlocale(LC_NUMERIC, locale);
  return i;
}

//================================================================= DmnPutDouble
int DmnPutDouble( // Put double values of a parameter into header
char *hdr,        // header
int len,          // maximum length of header string
char *name,       // name of the parameter
char *frm,        // format for writing (with separator!)
double *ff,       // double vector
int n )           // number of values to write
{                 // RETURN: number of values written
  int i;
  char buf[80];
  if (strlen(hdr)+strlen(name)+1 > len)  return -1;
  strcat(hdr, name);
  for (i=0; i<n; i++) {
    sprintf(buf, frm, ff[i]);
    if (strlen(hdr)+strlen(buf)+1 > len)  return -1;
    strcat(hdr, buf);
  }
  return i;
}

static void trim( char *s ) {
  char *p1, *p2;
  for (p1=s; (*p1); p1++)
    if (*p1 > ' ')  break;
  if (!*p1) {
    *s = 0;
    return;
  }
  for (p2=s+strlen(s)-1; p2>p1; p2--)
    if (*p2 > ' ')  break;
  *(++p2) = 0;
  p2 = p1;
  p1 = s;
  for ( ; (*p2); )  *p1++ = *p2++;
  *p1 = 0;
}

//================================================================= DmnGetString
int DmnGetString( // Get string values of a parameter
char *hdr,        // header
char *name,       // name of the parameter
char **ss,        // string vector
int n )           // maximum number of positions in ss[]
{                 // RETURN: number of values read
  dP(DmnGetString);
  int i, l;
  char *s, *pc, *pn;
  TXTSTR t = { NULL, 0 };
  TXTSTR b = { NULL, 0 };
  *ss = NULL;
  s = DmnGetValues(hdr, name, &l);
  if (!s)  return -1;
  if (n == 0) {     // concatenate all elements to one string
    TxtNCat(&b, s, l);
    pn = b.s;
    for (i=0; ; i++) {
      pc = MsgTok(pn, &pn, SEPCHR, &l);
      if (!pc)  break;
      if (pn && *pn)  *pn++ = 0;
      MsgDequote(pc);
      TxtNCat(&t, pc, l);
    }
    *ss = t.s;
    TxtClr(&b);
    return 0;
  }
  TxtNCpy(&t, s, l);
  pn = t.s;
  for (i=0; i<n; i++) {
    pc = MsgTok(pn, &pn, SEPCHR, &l);
    if (!pc)  break;
    if (ss[i]) FREE(ss[i]);
    ss[i] = ALLOC(l+1);  if (ss[i] == NULL)                eX(1); //-2006-02-15
    strncpy(ss[i], pc, l);
    MsgDequote(ss[i]);
    trim(ss[i]);
  }
  TxtClr(&t);
  return i;
eX_1:
  eMSG(_cant_allocate_$_, l+1);
}

//================================================================= DmnCpyString
int DmnCpyString( // Copy a string value from header
char *hdr,        // header
char *name,       // name of the parameter
char *dst,        // destination string
int n )           // maximum number of characters
{                 // RETURN: 0 on success
  dQ(DmnCpyString);
  char *_f = NULL;
  *dst = 0;
  if (n <= 0)  return -1;
  DmnGetString(hdr, name, &_f, 0);
  if (_f) {
    strncpy(dst, _f, n);
    dst[n-1] = 0;
    FREE(_f);
  }
  else return 1;
  return 0;
}

//================================================================= DmnPutString
int DmnPutString( // Write string values of a parameter into header
char *hdr,        // header
int len,          // maximum length of header string
char *name,       // name of the parameter
char *frm,        // format for writing (with separator and \"...\" !)
char **ss,        // string vector
int n )           // number of strings to write
{                 // RETURN: number of strings written
  int i;
  char buf[4000];
  if (strlen(hdr)+strlen(name)+1 > len)  return -1;
  strcat(hdr, name);
  for (i=0; i<n; i++) {
    sprintf(buf, frm, ss[i]);
    if (strlen(hdr)+strlen(buf)+1 > len)  return -1;
    strcat(hdr, buf);
  }
  return i;
}

//================================================= DmnConvertHeader
static char *find_eos( char *s )
  {
  while (*s) {
    s++;
    if (*s == '"') {
      if (s[1] != '"')  return s;
      s++;
      }
    }
  return s;
  }

static char *find_eob( char *s )
  {
  int n;
  char a, b;
  if ((!s) || (!*s))  return s;
  a = *s;
  switch (a) {
    case '(':  b = ')';  break;
    case '[':  b = ']';  break;
    case '{':  b = '}';  break;
    default :  return s+1;
    }
  n = 0;
  for (s++ ; (*s); s++) {
    if (*s == '"') {
      s = find_eos(s);
      if (!*s)  break;
      }
    else if (*s == b)
      if (!n)  return s;
      else  n--;
    else if (*s == a)  n++;
    }
  return s;
  }

static char *_newString( char *source, int len ) {
  dQ(newString);
  char *s;
  s = ALLOC(len+1);
  strncat(s, source, len);
  return s;
}

int DmnCnvHeader( // convert array header to user header
char *arrhdr,     // array header
TXTSTR *ptxt )    // user header
{
  dQ(DmnCnvHeader);
  int n;
  char *pe, *p0, *p1, *p2;
  char *_key, *_val;
  char white[] = " \t\r\n";
  TxtCpy(ptxt, "\n");
  n = 0;
  p0 = arrhdr;
  for (pe=arrhdr; (*pe); pe++) {
    if (*pe != '=') {
      if (*pe == '"') {
        pe = find_eos(pe);
        if (!*pe)  break;
      }
      continue;
    }
    for (p2=pe-1; p2>=p0; p2--)
      if (!strchr(white, *p2))  break;
    if (p2 < p0)  continue;
    for (p1=p2-1; p1>=p0; p1--)
      if (!isalnum(*p1))  break;
    p1++;
    if (p1 > p2)  continue;
    if (p2-p1+2 > 80) continue;
    _key = _newString(p1, p2-p1+1);
    for (p1=pe+1; (*p1); p1++)
      if (!strchr(white, *p1))  break;
    if (*p1 == '{') {
      p2 = find_eob(p1);
      _val = _newString(p1+1, p2-p1-1);
    }
    else if (*p1 == '\"') {
      p2 = find_eos(p1);
      _val = _newString(p1, p2-p1+1);
    }
    else {
      for (p2=p1+1; (*p2); p2++)
        if (strchr(white, *p2))  break;
      p2--;
      _val = _newString(p1, p2-p1+1);
    }
    trim(_val);
    TxtCat(ptxt, _key);
    TxtCat(ptxt, "  ");                                           //-2014-01-28
    TxtCat(ptxt, _val);
    TxtCat(ptxt, "\n");
    FREE(_key);  _key = NULL;
    FREE(_val);  _val = NULL;
    pe = p2;
    n++;
  }
  return n;
}

//====================================================== DmnArrHeader
//
int DmnArrHeader(   // convert user header to array header
char *s,            // user header
TXTSTR *ptxt )      // array header
{
  dQ(DmnArrHeader);
  char *pc, *pn, *ps, *_ss;
  _ss = ALLOC(strlen(s)+1);  if (!_ss)  return 0;
  strcpy(_ss, s);
  pc = _ss;
  while (NULL != (pn=strchr(pc, '\n'))) {
    if (!isalpha(*pc)) {                                //-2004-01-25
      pc = pn + 1;
      continue;
    }
    *pn = 0;
    ps = strrchr(pc, '\'');  if (ps)  *ps = 0;
    ps = strrchr(pc, '\r');  if (ps)  *ps = 0;
    for (ps=pc; ((*ps) && !isspace(*ps));  ps++);
    if (*ps) {                                          //-2005-01-25
      *ps = 0;
      //TxtCat(ptxt, " ");                             -2006-05-29
      TxtCat(ptxt, pc);
      TxtCat(ptxt, "={");
      TxtCat(ptxt, ps+1);
      TxtCat(ptxt, "} \n");                            //-2006-05-29
    }
    pc = pn+1;
  }
  FREE(_ss);
  return 0;
}

//===================================================================== DmnWrite
static int makerecord( char *psrc, char *pdst, DMNFRMREC *pfrm, int swap ) {
  int l, n, i, reverse;
  n = 0;
  while (pfrm->dt) {
    l = pfrm->ld;
    if (pfrm->lf != 0) {
      reverse = (swap) && (pfrm->dt!=chDAT);
      if (reverse)
        for (i=0; i<l; i++)  pdst[l-i-1] = psrc[i];
      else memcpy(pdst, psrc, l);
      pdst += l;
      n += l;
    }
    psrc += l;
    pfrm++;
  }
  return n;
}

static int endswith( char *body, char *end ) {
  int lb, le;
  if (body && end) {
    lb = strlen(body);
    le = strlen(end);
    if ((lb >= le) && !strcmp(body+lb-le, end))  return 1;
  }
  return 0;
}

int DmnWrite(   // Write data into a file
char *fname,    // file name (with path, without extension)
char *hdr,      // header string
ARYDSC *pa )    // address of array descriptor
{
  dP(DmnWrite);
  ARYDSC a, b;
  TXTSTR header = { NULL, 0 };
  TXTSTR oform =  { NULL, 0 };
  int osize, drop, swap, lsbf, noheader=0, nodata=0;
  FILE *f;
  gzFile g;
  DMNFRMREC *pf;
  char *pb;
  char *mode=NULL, *sequ=NULL, *form=NULL, *data=NULL, *pc, *frm;
  char *locl=NULL, *vldf=NULL, *tmzn=NULL;
  char fna[256], fnb[256], fn[256], buf[80], format[80], s[256], t[80];
  char oldsel[80], newsel[80], fxd[80], valdef[80];
  int i, j, k, n, dims, v[ARYMAXDIM], nd;
  int compressed, binary, reorder, opened, binfile;           //-2005-11-07
  double fact, time_shift = 0;
  void *pv;
  char locale[256] = "C";
  if (!pa->start)  return 0;
  strcpy(fn, fname);
  compressed = 0;
  DmnGetInt(hdr, "cmpr", "%d", &compressed, 1);              //-2006-05-26
  if (endswith(fn, ".dmna"))  fn[strlen(fn)-5] = 0;               //-2006-01-18
  if (!ModErr) {
#ifdef __linux__
    ModErr = stderr;
#else
    setmode(2, O_BINARY);
    ModErr = fdopen(2, "wb");
#endif
  }
  if (!ModErr)                                                eX(99);

  if (*fn=='-' || *fn=='~') {
    if (fn[1] != 0) {
      noheader = 1;
      nodata = 1;
      if (strchr(fn, 'h'))  noheader = 0;
      if (strchr(fn, 'd'))  nodata = 0;
    }
    strcpy(fn, (*fn=='-') ? "-" : "~");
  }
  TxtCpy(&header, hdr);
  if (compressed<0 || compressed>9) {
    compressed = 0;
    DmnRplValues(&header, "cmpr", "0");
  }
  DmnGetString(header.s, "sequ", &sequ, 1);
  DmnGetString(header.s, "mode", &mode, 1);
  DmnGetString(header.s, "data", &data, 1);
  DmnGetString(header.s, "form", &form, 0);             //-2001-04-29 lj
  DmnGetString(header.s, "vldf", &vldf, 1);             //-2000-03-03
  DmnGetString(header.s, "locl", &locl, 1);             //-2003-07-07
  DmnGetString(header.s, "tmzn", &tmzn, 1);
  time_shift = get_shift(tmzn);
  strcpy(locale, setlocale(LC_NUMERIC, NULL));                    //-2008-10-17
  //
  if ((locl) && (*locl) && strcmp(locl, "C")) {                   //-2012-09-04	
  		MsgLow(locl);
    if (!strcmp(locl, "de")) {
    		FREE(locl);
    		locl = ALLOC(8);
      strcpy(locl, "german");
      DmnRplValues(&header, "locl", TxtQuote(locl));              //-2012-09-04	
    }
    else if (!strcmp(locl, "en")) {
    		strcpy(locl, "C");
    		DmnRplValues(&header, "locl", TxtQuote(locl));              //-2012-09-04	
    }
    if (MsgSetLocale(locl))                             eX(10);   //-2008-10-17
  }
  else {
    DmnRplValues(&header, "locl", "\"C\"");                       //-2008-12-11
    MsgSetLocale("C");                                            //-2008-10-17
  }
  //
  if (vldf) {
    strcpy(valdef, vldf);
    FREE(vldf);
    vldf = NULL;
  }
  else  *valdef = 0;
  if ((data) && (*data=='*' || *data==0)) {
    FREE(data);
    data = NULL;
  }
  lsbf = 1;
  DmnGetInt(header.s, "lsbf", "%d", &lsbf, 1);
  fact = 0;
  DmnGetDouble(header.s, "fact", "%lf", &fact, 1);
  if (fact <= 0)  fact = 1;
  if (sequ) {
    strcpy(oldsel, sequ);
    AryDefAry(pa, &a, &b, oldsel);                  eG(1);
    strcpy(newsel, oldsel);
  }
  else { a = *pa;  b = a; *newsel = 0; }
  reorder = (a.usrcnt < 0);
  dims = b.numdm;

  *fxd = 0;
  if (sequ) {           // remove fixed dimensions
    strcat(newsel, ",");
    for (n=0; n<dims; n++) {
      t[0] = 'i' + n;
      strcpy(t+1, "=");
      pc = strstr(newsel, t);
      if (pc) {
        for (; (*pc); pc++) {
          if (*pc == ',') { *pc = ' ';   break; }
          *pc = ' ';
        }
        strncat(fxd, t, 1);
      }
    }
    for (i=strlen(fxd)-1; i>=0; i--) {
      for (j=strlen(newsel)-1; j>=0; j--) {
        if (!strchr("ijklm", newsel[j]))  continue;
        if (newsel[j] > fxd[i])  newsel[j]--;
      }
    }
    for (i=0, j=0; i<strlen(newsel); i++)
      if (newsel[i] > ' ')  newsel[j++] = newsel[i];
    newsel[j] = 0;
    if (j > 0)  newsel[j-1] = 0;
  }
  else  strncat(newsel, "i,j,k,l,m", 2*dims-1);
  DmnRplValues(&header, "sequ", TxtQuote(newsel));
  nd = dims - strlen(fxd);
  binary = (mode) ? (0==strcmp(mode, "binary")) : (form == NULL);
  drop = 0;                                                       //-2006-03-20
  if (form) {
    frm = form;
    DmnReplaceEscape(frm);
    if (DmnFrmTab)  FREE(DmnFrmTab);                              //-2001-04-29 lj
    DmnFrmTab = DmnAnaForm(frm, &a.elmsz);                            eG(9);
    if (!DmnFrmTab)                                                   eX(21);
    drop = 0;
    osize = 0;
    pf = DmnFrmTab;                                               //-2001-04-29 lj
    i = 0;
    while (pf->dt) {
      if (pf->lf != 0) {              // 2001-04-30 uj ?? //-2005-12-06
        osize += pf->ld;
        TxtCat(&oform, "\"");
        TxtCat(&oform, pf->name);
        TxtCat(&oform, "%");
        if (strlen(pf->opt)>0) {
          TxtCat(&oform, "(");
          TxtCat(&oform, pf->opt);
          TxtCat(&oform, ")");
        }
        TxtCat(&oform, pf->oform+1);
        TxtCat(&oform, "\" ");
      }
      else {
        drop++;
        if (i < strlen(valdef))  valdef[i] = ' ';
      }
      i++;
      pf++;
    }
    DmnRplValues(&header, "form", oform.s);
    TxtClr(&oform);                                              //-2006-02-15
    if (drop) {
      if (*valdef) {                                             //-2004-11-15 uj
        for (i=0, k=0; k<strlen(valdef); k++)
          if (valdef[k] != ' ')  valdef[i++] = valdef[k];
        valdef[i] = 0;
        DmnRplValues(&header, "vldf", TxtQuote(valdef));
      }
    }
  }
  else {
    osize = a.elmsz;
    if (!binary) {
      int nf = a.elmsz/sizeof(float);
      if (nf*sizeof(float) != osize)                                  eX(20);
      sprintf(format, "\"A%%[%d]13.5e\"", nf);                    //-2008-12-11
      DmnRplValues(&header, "form", format);
    }
  }
  //
  DmnRplValues(&header, "dims", NULL);                            //-2006-07-10
  DmnRplValues(&header, "size", NULL);
  DmnRplValues(&header, "lowb", NULL);
  DmnRplValues(&header, "hghb", NULL);
  //
  sprintf(buf, "dims  %d\n", nd);       TxtCat(&header, buf);     //-2014-01-28
  sprintf(buf, "size  %d\n", osize);    TxtCat(&header, buf);     //-2014-01-28
  strcpy(buf, "lowb");
  for (n=0; n<dims; n++) {
    if (strchr(fxd, 'i'+n))  continue;
    sprintf(s, "  %d", b.bound[n].low);                           //-2014-01-28
    strcat(buf, s);
  }
  strcat(buf, "\n");            TxtCat(&header, buf);
  strcpy(buf, "hghb");
  for (n=0; n<dims; n++) {
    if (strchr(fxd, 'i'+n))  continue;
    sprintf(s, "  %d", b.bound[n].hgh);                           //-2014-01-28
    strcat(buf, s);
  }
  strcat(buf, "\n");            TxtCat(&header, buf);

  opened = 0;
  binfile = 0;
  *fna = 0;
  *fnb = 0;
  if (!strncmp(fn, "LOG", 3)) {
    if (binary)  goto done;
    if (!MsgFile)  goto done;
    if (data)  goto done;
    f = MsgFile;
    fprintf(f, "%s", fn+3);
    }
  else if (!strcmp(fn,"stdout") || !strcmp(fn,"stderr") || *fn=='-' || *fn=='~') {
    f = (!strcmp(fn,"stderr")) ? stderr : stdout;
    if (!noheader) {
      if (1 != fwrite(header.s, header.l, 1, f))      eX(3);
    }
    fprintf(f, "*\n");
    if (*fn == '~')
      f = ModErr;
  }
  else {
    strcpy(fna, fn);
    for (pc=fna; (*pc); pc++)  if (*pc == '\\')  *pc = '/';
    strcpy(fnb, fna);
    //
    strcat(fnb, BINEXT);  remove(fnb);
    strcat(fnb, CMPEXT);  remove(fnb);
    strcpy(fnb, fna);
    strcat(fnb, TXTEXT);  remove(fnb);
    strcat(fnb, CMPEXT);  remove(fnb);
    strcpy(fnb, fna);
    //
    strcat(fna, STDEXT);
    f = fopen(fna, "w");  if (!f)                     eX(2);
    opened = 1;
    if (1 != fwrite(header.s, header.l, 1, f))        eX(3);
    fprintf(f, "*\n");
    if (binary)  binfile = 1;
    }
  TxtClr(&header);
  if (data || binfile || compressed) {                          //-2005-11-07
    if (opened) {
      fclose(f);
    }

    if (data) {
      pc = strrchr(fnb, '/');
      if (pc)  pc++;
      else  pc = fnb;
      *pc = 0;
      strcat(fnb, data);
    }
    strcat(fnb, (binary) ? BINEXT : TXTEXT);
    if (compressed) {                                           //-2005-11-07
      char gzmode[32];
      sprintf(gzmode, "wb%d", compressed);
      strcat(fnb, CMPEXT);
      g = gzopen(fnb, gzmode);  if (g == NULL)                      eX(14);
    }
    else {
      f = fopen(fnb, "wb");  if (!f)                                eX(4);
    }
    opened = 1;
  }

  if (nodata)  goto done;
  if (binary) {   // unformatted
    n = 1;
    pb = (char*) &n;
    swap = (pb[0] != lsbf);
    if (drop || reorder || swap) {
      pb = ALLOC(a.elmsz);
      for (n=0; n<dims; n++)  v[n] = a.bound[n].low;
      do {
        pc = AryPtr(&a, v[0], v[1], v[2], v[3], v[4]);  if (!pc)    eX(5);
        if (drop || swap) {
          if (!DmnFrmTab)                                eX(26);  //-2006-03-20
          n = makerecord(pc, pb, DmnFrmTab, swap);
          if (compressed) {                                     //-2005-11-07
            if (n != gzwrite(g, pb, n))                             eX(16);
          }
          else {
            if (1 != fwrite(pb, n, 1, f))                           eX(6);
          }
        }
        else {
          n = a.elmsz;
          if (compressed) {                                     //-2005-11-07
            if (n != gzwrite(g, pc, n))                             eX(16);
          }
          else {
            if (1 != fwrite(pc, n, 1, f))                           eX(6);
          }
        }
        for (n=a.numdm-1; n>=0; n--) {
          v[n]++;
          if (v[n]>a.bound[n].hgh && n>0)  v[n] = a.bound[n].low;
          else  break;
        }
        fflush(ModErr);
      } while (v[0] <= a.bound[0].hgh);
      FREE(pb);
    }
    else {
      n = a.ttlsz;
      if (compressed) {                                         //-2005-11-07
        if (n != gzwrite(g, a.start, n))                            eX(17);
      }
      else {
        if (1 != fwrite(a.start, n, 1, f))                          eX(7);
      }
    }
  }
  else {  // formatted
    int (*prn)(void*, const char*, ...);
    void *fp;
    if (compressed) {
      fp = g;
      prn = (int (*)(void*, const char*, ...)) gzprintf;
    }
    else {
      fp = f;
      prn = (int (*)(void*, const char*, ...)) fprintf;
    }
    for (n=0; n<dims; n++)  v[n] = a.bound[n].low;
    do {
      pv = AryPtr(&a, v[0], v[1], v[2], v[3], v[4]);  if (!pv)      eX(8);
      n = DmnPrintR(fp, prn, DmnFrmTab, fact, time_shift, pv);
      if (1 < n)  prn(fp, "\n");
      for (n=a.numdm-1; n>=0; n--) {
        v[n]++;
        if (v[n]>a.bound[n].hgh && n>0) {
          v[n] = a.bound[n].low;
          prn(fp, "\n");
        }
        else  break;
      }
    } while (v[0] <= a.bound[0].hgh);
    prn(fp, "\n");
    if (opened)  prn(fp, "***\n");
  }
  if (opened) {
    if (compressed) {
      gzclose(g);
    }
    else {
      fclose(f);
    }
  }
done:
  if (mode)  FREE(mode);
  if (data)  FREE(data);
  if (form)  FREE(form);
  if (sequ)  FREE(sequ);
  if (locl)  FREE(locl);
  if (tmzn)  FREE(tmzn);
  setlocale(LC_NUMERIC, locale);
  return 1;
eX_99:
  eMSG(_cant_reopen_);
eX_1:
  eMSG(_cant_get_subarray_$_, sequ);
eX_2:
  printf("stderr=%d, EMFILE=%d, err=%s\n", errno, EMFILE, strerror(errno));
  eMSG(_cant_open_descriptor_$_, fna);
eX_3:
  eMSG(_cant_write_header_$_, fna);
eX_4: eX_14:
  eMSG(_cant_open_data_$_, fnb);
eX_5: eX_8:
  eMSG(_index_error_$_, fnb);
eX_6: eX_7: eX_16: eX_17: eX_26:
  eMSG(_cant_write_data_$_, fnb);
eX_9:
  eMSG(_invalid_format_$_, frm);
eX_10:
  eMSG(_unknown_locale_$_, locl);
eX_20:
  eMSG(_cant_write_without_format_$_, osize);
eX_21:
  eMSG(_cant_create_format_table_);
}

static char *next_token(  // tokenize (destructive) buffer
char *buffer,             // buffer (saved as static) or NULL
char *separator,          // separator characters or NULL
int *plen,                // number of unused characters or -1 for error
DROP *pdrop)              // drop condition
{                         // RETURN: token or NULL
  // each token in buffer must be followed by a separator character
  static char *buf, *sep, *p1, *p2;
  static DROP drop;
  static int new_line=0, check=0, dropping=0;
  int i, l;
  *plen = 0;
  if (buffer) {
    buf = buffer;
    sep = separator;
    if (sep==NULL || *sep==0)  sep = " \t\r\n";
    p2 = NULL;
    if (pdrop) {
      drop = *pdrop;
// printf("drop: count=%d, equal=%d, any=%d, letters=%s<<<\n",
// drop.count, drop.equal, drop.any, drop.letters);
      check = 1;
    }
    else  check = 0;
    new_line = 1;
  }
  if (buf == NULL) {
    *plen = -1;
    return NULL;
  }
  p1 = (p2) ? p2 + 1 : buf;
  if (dropping) {
drop_line:
    for ( ; (*p1); p1++) {
      if (*p1 == '\n')  {
        dropping = 0;
        new_line = 0;
        break;
      }
    }
  }
  for (; (*p1); p1++) {
    if (new_line && check) {    // check drop condition
      int doit = check_drop(&drop, p1);
// printf("check_drop(,>>>%s<<<)=%d\n", p1, doit);
      if (doit > 0) {           // drop line
        dropping = 1;
        for ( ; (*p1); p1++) {
          if (*p1 == '\n')  {
            dropping = 0;
            new_line = 0;
            break;
          }
        }
        if (!*p1)  break;
      }
      else if (doit < 0) {      // refill buffer
        l = strlen(p1);
        for (i=0; i<l; i++)  buf[i] = p1[i];
        buf[l] = 0;
        *plen = l;
        p2 = NULL;
        return NULL;
      }
    }
    if (!strchr(sep, *p1))  break;
    new_line = (*p1 == '\n');
  }
  if (*p1 == '\'') {  // start of comment                         //-2006-02-15
    dropping = 1;
    goto drop_line;
  }
  if (!*p1){
    *buf = 0;
    p2 = NULL;
    *plen = 0;
    return NULL;
  }
  if (*p1 == '"') {
    p2 = find_eos(p1);
    if (*p2) p2++;
  }
  else {
    for (p2=p1+1; (*p2); p2++)
      if (strchr(sep, *p2))  break;
  }
  if (!*p2) { // move the remaining characters to the beginning of the buffer
    l = p2 - p1;
    for (i=0; i<l; i++)  buf[i] = p1[i];
    buf[l] = 0;
    *plen = l;
    p2 = NULL;
    return NULL;
  }
  new_line = (*p2 == '\n');
  *p2 = 0;
  return p1;
}

static void prn_dsc(char *title, ARYDSC *pa) {
  int i;
  printf("%s  @%p  %d\n", title, pa->start, pa->virof);
  for (i=0; i<pa->numdm; i++)
    printf("%2d: %3d ... %3d, *%d\n", i+1, pa->bound[i].low, pa->bound[i].hgh,
    pa->bound[i].mul);
}

//===================================================================== DmnRead
int DmnRead(    // Read data from a file
char *fn,       // file name (with path, without extension)
TXTSTR *pu,     // user header text
TXTSTR *ps,     // system header text
ARYDSC *pa )    // address of array descriptor
{
  dP(DmnRead);
  ARYDSC a, b;
  FILE *f;
  gzFile g;
  char *mode=NULL, *sequ=NULL, *form=NULL, *data=NULL, *tmzn=NULL, *locl=NULL;
  char *buf=NULL, *drpc=NULL, *str=NULL;
  char *pc, *pn, *frm, *cp, *pss;
  char fna[256], fnb[256], name[80], format[32], seps[20], squs[80];
  int n, nn, l, binary, reorder, dims, v[ARYMAXDIM], bufsize, i;
  int size;
  int ii[2][ARYMAXDIM], msgbreak, compressed;
  double fact, fc;
  double d, time_shift=0;
  DMNFRMREC *pf;
  DROP *pdrp;
  void *pv;
  union {
    char c;
    char *cp;
    short int h;
    int i;
    long long l;
    float f;
    double d;
  } u;
  TXTSTR syshdr = { NULL, 0 };
  TXTSTR usrhdr = { NULL, 0 };
  char locale[256] = "C";
  //
  strcpy(locale, setlocale(LC_NUMERIC, NULL));                //-2003-07-07
  strcpy(seps, " ;\t\n\r");
  if (!ps)  ps = &syshdr;
  if (!pu)  pu = &usrhdr;
  TxtClr(pu);
  TxtClr(ps);
  if (pa->start) {                                               //-2011-07-21
    AryFree(pa);                                                        eG(1);
  }
  memset(&a, 0, sizeof(ARYDSC));
  memset(&b, 0, sizeof(ARYDSC));
  for (i=0; i<ARYMAXDIM; i++) {
    ii[0][i] = 0;
    ii[1][i] = 0;
  }
  f = NULL;
  if (!strcmp(fn, "-") || !strcmp(fn, "stdin"))
  		f = stdin;
  else {
    strcpy(fna, fn);
    for (pc=fna; (*pc); pc++)  if (*pc == '\\')  *pc = '/';
    pc = strrchr(fna, '.');
    if (pc && !strcmp(pc, STDEXT))  
    		*pc = 0;
    strcpy(fnb, fna);
    strcat(fna, STDEXT);
    f = fopen(fna, "r");  
    if (!f)                                                   eX(3);
  }
  bufsize = 4000;
  buf = ALLOC(bufsize);
  msgbreak = MsgBreak;
  MsgBreak = '\'';                                          //-2001-06-07
  nn = 0;
  while (fgets(buf, bufsize, f)) {  // read header
    nn++;
    if (strlen(buf) >= bufsize-1)                             eX(20);
    if (*buf == '*')  break;
    *name = 0;
    strncat(name, buf, 4);
    MsgLow(name);
    if (NULL != strstr(DimParms, name)) TxtCat(ps, buf);
    else                                TxtCat(pu, buf);
    pc = DmnFindName(buf, "buff");
    if (pc) {
      n = bufsize;
      sscanf(pc+4, "%d", &n);                                     //-2005-08-08
      if (n > bufsize) {      // resize input buffer
        FREE(buf);
        bufsize = n;
        buf = ALLOC(bufsize);
      }
    }
  }
  DmnGetString(ps->s, "sequ", &sequ, 1);
  DmnGetString(ps->s, "mode", &mode, 1);
  DmnGetString(ps->s, "data", &data, 1);
  DmnGetString(ps->s, "form", &form, 0);
  DmnGetString(ps->s, "locl", &locl, 1);                          //-2003-07-07
  //
  if ((locl) && (*locl) && strcmp(locl, "C")) {                   //-2012-09-04	
  		MsgLow(locl);
    if (!strcmp(locl, "de")) {
    		FREE(locl);
    		locl = ALLOC(8);
      strcpy(locl, "german");
    }
    else if (!strcmp(locl, "en"))
    		strcpy(locl, "C");
    if (MsgSetLocale(locl))                               eX(22); //-2008-10-17
  }
  else {
    MsgSetLocale("C");                                            //-2008-10-17
  }
  //
  binary = (mode) ? (0==strcmp(mode, "binary")) : (form == NULL);
  DmnGetString(ps->s, "tmzn", &tmzn, 1);
  time_shift = get_shift(tmzn);
  DmnGetString(ps->s, "drop", &drpc, 1);
  pdrp = get_drop(drpc);
  size = 0;
  DmnGetInt(ps->s, "size", "%d", &size, 1);
  compressed = 0;                                                 //-2005-11-07
  DmnGetInt(ps->s, "cmpr", "%d", &compressed, 1);
  if ((data) && (*data=='*' || *data==0)) {
    FREE(data);
    data = NULL;
  }
  if ((binary) && (size<=0))                                  eX(16);
  if (form)  frm = form;
  else {
    if (size >= sizeof(float))
      sprintf(format, "A%%[%d]f", size/sizeof(float));
    else                                                          eX(9);
    frm = format;
  }
  if (DmnFrmTab)  FREE(DmnFrmTab);                                //-2001-04-29 lj
  DmnFrmTab = DmnAnaForm(frm, &size);                             eG(9);
  fact = 0;
  DmnGetDouble(ps->s, "fact", "%lf", &fact, 1);
  if (fact <= 0)  fact = 1;
  if (1 != DmnGetInt(ps->s, "dims", "%d", &dims, 1))              eX(17);
  if (dims<1 || dims>ARYMAXDIM)                                   eX(10);
  if (dims != DmnGetInt(ps->s, "lowb", "%d", ii[0], ARYMAXDIM))   eX(11);
  if (dims != DmnGetInt(ps->s, "hghb", "%d", ii[1], ARYMAXDIM))   eX(11);
  AryCreate(pa, size, dims, ii[0][0], ii[1][0], ii[0][1], ii[1][1],
    ii[0][2], ii[1][2], ii[0][3], ii[1][3], ii[0][4], ii[1][4]);  eG(12);
  if (sequ) { // remove standard sequence
    if (!*sequ)  strcpy(squs, "i+;j+;k+;l+;m+;");
    else  strcpy(squs, sequ);
    strcat(squs, ";");
    for (i=0; i<strlen(squs); i++)
      if (squs[i] == ',')  squs[i] = ';';
    if (!strncmp(squs, "i;j;k;l;m;", 2*dims)
     || !strncmp(squs, "i+;j+;k+;l+;m+;", 3*dims)) {
      FREE(sequ);
      sequ = NULL;
    }
  }
  if (sequ) { // create mapping
    if (strlen(sequ) > 79)                                          eX(13);
    strcpy(squs, sequ);                                           //-2002-06-30
    AryDefAry(pa, &a, &b, squs);                                    eG(41);
    reorder = 1;
  }
  else {
    a = *pa;
    b = a;
    reorder = 0;
  }
  dims = b.numdm;
  //
  if ((binary || (data) || compressed) && (f!=stdin)) {
    fclose(f);
    if (data) {
      pc = strrchr(fnb, '/');
      if (pc)  pc++;
      else  pc = fnb;
      *pc = 0;
      strcat(fnb, data);
    }
    else strcat(fnb, (binary) ? BINEXT : TXTEXT);
    if (compressed) {
      strcat(fnb, CMPEXT);
      g = gzopen(fnb, "rb");  if (!g)                             eX(24);
    }
    else {
      f = fopen(fnb, "rb");  if (!f)                              eX(4);
    }
    nn = 0;
  }
  if (binary) {
    //
    // read binary data
    //
    if (reorder) {
      for (n=0; n<dims; n++)  v[n] = a.bound[n].low;
      for ( ; n<ARYMAXDIM; n++)  v[n] = 0;
      do {
        pc = AryPtr(&a, v[0], v[1], v[2], v[3], v[4]);  if (!pc)  eX(5);
        n = a.elmsz;
        if (compressed) {
          if (n != gzread(g, pc, n))                              eX(26);
        }
        else {
          if (1 != fread(pc, n, 1, f))                            eX(6);
        }
        for (n=a.numdm-1; n>=0; n--) {
          v[n]++;
          if (v[n]>a.bound[n].hgh && n>0) {
            v[n] = a.bound[n].low;;
          }
          else  break;
        }
      } while (v[0] <= a.bound[0].hgh);
    }
    else {
      n = a.ttlsz;
      if (compressed) {
        if (n != gzread(g, a.start, n))                           eX(27);
      }
      else {
        if (1 != fread(a.start, n, 1, f))                         eX(7);
      }
    }
  }
  else {
    //
    // read formatted data
    //
    setlocale(LC_NUMERIC, "C");                                   //-2012-09-04   
    for (n=0; n<dims; n++)  v[n] = a.bound[n].low;
    for ( ; n<ARYMAXDIM; n++)  v[n] = 0;
    *buf = 0;
    pc = next_token(buf, seps, &l, pdrp);
    pn = buf;
    do {
      pv = AryPtr(&a, v[0], v[1], v[2], v[3], v[4]);  if (!pv)    eX(8);
      pf = DmnFrmTab;
      while (pf->dt) {
        //
        // get the next token, refill the buffer if necessary
        //
        while (NULL == (pc = next_token(NULL, NULL, &l, NULL))) { //-2005-11-09
          if (l < 0)                                              eX(35);
          if (l >= bufsize-1)                                     eX(36); //-2006-10-19
          if (compressed) {
            //if (!gzgets(g, buf+l, bufsize-l))                     eX(34);
            n = gzread(g, buf+l, bufsize-l-1);                            //-2006-10-19
            if (n <= 0)                                           eX(34);
          }
          else {
            //if (!fgets(buf+l, bufsize-l, f))                      eX(14);
            n = fread(buf+l, sizeof(char), bufsize-l-1, f);               //-2006-10-19
            if (n <= 0)                                           eX(14);
          }
          buf[l+n] = 0;
          nn++;
        }
        //
        // analyse the token
        //
        MsgDequote(pc);
        if      (pf->dt == ltDAT) {
          u.d = MsgDateVal(pc);
        }
        else if (pf->dt == tmDAT) {
          if (pf->fac > 0) {
            d = MsgDateVal(pc);
            u.i = MsgDateSeconds(d, pf->fac);
          }
          else  u.i = TmValue(pc);
        }
        else if (pf->dt == chDAT)     // do not use union for strings 2006-06-08
          ;
        else {
          if (pf->dt == flDAT || pf->dt == lfDAT)                 //-2012-09-04
        				for (pss=pc; (*pss); pss++)  if (*pss == ',')  *pss = '.';
        		if (1 != sscanf(pc, pf->iform, &u))                      eX(15);
        }
        fc = (pf->fac != 0) ? pf->fac : fact;
        switch (pf->dt) {
          case chDAT: cp = (char*)pv;
                      str = ALLOC(pf->ld+1);
                      strncpy(str, pc, pf->ld);
                      trim(str);
                      strcpy(cp, str);
                      FREE(str);
                      break;
          case btDAT: *(char*)pv = u.c;
                      break;
          case siDAT: *(short int*)pv = u.h;
                      break;
          case inDAT: *(int*)pv = u.i;
                      break;
          case liDAT: *(long long*)pv = u.l;
                      break;
          case flDAT: *(float*)pv = u.f/fc;
                      break;
          case lfDAT: *(double*)pv = u.d/fc;
                      break;
          case tmDAT: *(int*)pv = u.i;
                      break;
          case ltDAT: *(double*)pv = u.d - time_shift;
                      break;
          default:    ;
        }
        pv = (char*)pv + pf->ld;
        pf++;
      }
      for (n=a.numdm-1; n>=0; n--) {
        v[n]++;
        if (v[n]>a.bound[n].hgh && n>0) {
          v[n] = a.bound[n].low;
        }
        else  break;
      }
    } while (v[0] <= a.bound[0].hgh);
  }
  if (f != stdin) {
    if (compressed) {                                             //-2005-11-07
      gzclose(g);
    }
    else {
      fclose(f);
    }
  }
  MsgBreak = msgbreak;
  TxtClr(&usrhdr);
  TxtClr(&syshdr);
  if (mode)  FREE(mode);
  if (data)  FREE(data);
  if (form)  FREE(form);
  if (sequ)  FREE(sequ);
  if (locl)  FREE(locl);
  if (tmzn)  FREE(tmzn);
  if (drpc)  FREE(drpc);
  if (buf)   FREE(buf);
  setlocale(LC_NUMERIC, locale);                                  //-2003-07-07
  return 1;
eX_1:
  eMSG(_internal_error_);
eX_3:
  eMSG(_cant_open_descriptor_$_, fna);
eX_4: eX_24:
  eMSG(_cant_open_data_$_, fnb);
eX_5: eX_8:
  eMSG(_index_error_reading_$_, fnb);
eX_6: eX_7: eX_26: eX_27:
  eMSG(_cant_read_data_$_, fnb);
eX_9:
  eMSG(_invalid_format_$_, frm);
eX_10:
  eMSG(_invalid_number_dimension_$$_, dims, fnb);
eX_11:
  eMSG(_missing_bounds_$_, fnb);
eX_12:
  eMSG(_cant_create_array_$_, fnb);
eX_13:
  eMSG(_invalid_selection_$$_, sequ, fnb);
eX_14:
  eMSG(_eof_$_, fnb);
eX_15:
  fprintf(MsgFile, "token=%s, format=%s\n", pc, pf->iform);       //-2008-07-30
  eMSG(_cant_read_item_$$_, nn, fnb);
eX_16:
  eMSG(_missing_size_$_, fna);
eX_17:
  eMSG(_missing_dims_$_, fna);
eX_20:
  eMSG(_buffer_overflow_$$_, nn, fna);
eX_22:
  eMSG(_unknown_locale_$_file_$_, locl, fn);
eX_34:
  eMSG(_cant_read_compressed_$_, fnb);
eX_35:
  eMSG(_cant_tokenize_$$_, nn, fnb);
eX_36:
  eMSG(_small_buffer_$_, bufsize);
eX_41:
  eMSG(_cant_define_mapping_$_, fn);
}

//------------------------------------------------------------ Dmn2ArcInfo
int Dmn2ArcInfo(  // translate 2D file to ArcInfo format
char *fin,        // input file name
char *fout)       // output file name or NULL
{
  dP(Dmn2ArcInfo);
  ARYDSC ary;
  TXTSTR usrhdr = { NULL, 0 };
  TXTSTR syshdr = { NULL, 0 };
  FILE *fp;
  char form[80], name[256], *_frm;
  double delt, xmin, ymin, xref, yref;
  int i, j, i1, i2, j1, j2, k, n;
  memset(&ary, 0, sizeof(ARYDSC));
  DmnRead(fin, &usrhdr, &syshdr, &ary);                          eG(9);
  if (ary.numdm != 2) {
    if (ary.numdm == 3) {
      if (ary.bound[2].low != ary.bound[2].hgh)                  eX(1);
      k = ary.bound[2].low;
    }
    else                                                         eX(1);
  }
  _frm = NULL;                                                   //-2006-02-15
  n = DmnGetString(syshdr.s, "form", &_frm, 0);  if (n <= 0)     eX(6);
  strcpy(form, _frm);
  FREE(_frm);                                                    //-2006-02-15
  _frm = NULL;
  if (DmnFrmTab)  FREE(DmnFrmTab);
  DmnFrmTab = DmnAnaForm(form, &ary.elmsz);                      eG(2);
  if (ary.elmsz != 4)                                            eX(3);
  if (DmnFrmTab->dt != flDAT)                                    eX(4);
  if (!fout)
    sprintf(name, "%s.txt", fin);
  else
    strcpy(name, fout);
  fp = fopen(name, "wb"); if (!fp)                               eX(5);
  DmnGetDouble(usrhdr.s, "xmin|x0", "%lf", &xmin, 1);
  DmnGetDouble(usrhdr.s, "ymin|y0", "%lf", &ymin, 1);            //-2006-02-15
  DmnGetDouble(usrhdr.s, "delta|delt|dd", "%lf", &delt, 1);
  i = DmnGetDouble(usrhdr.s, "gakrx|refx", "%lf", &xref, 1);     //-2006-02-17
  if (i == 1) xmin += xref;
  i = DmnGetDouble(usrhdr.s, "gakry|refy", "%lf", &yref, 1);     //-2006-02-17
  if (i == 1) ymin += yref;
  i1 = ary.bound[0].low;
  i2 = ary.bound[0].hgh;
  j1 = ary.bound[1].low;
  j2 = ary.bound[1].hgh;
  fprintf(fp, "nrows %10d\n", j2-j1+1);
  fprintf(fp, "ncols %10d\n", i2-i1+1);
  fprintf(fp, "xllcorner %12.3f\n", xmin);
  fprintf(fp, "yllcorner %12.3f\n", ymin);
  fprintf(fp, "cellsize  %12.3f\n", delt);
  for (j=j2; j>=j1; j--) {
    for (i=i1; i<=i2; i++) {
      if (ary.numdm == 2)
        fprintf(fp, " %12.5e",  *(float*)AryPtrX(&ary, i, j));
      else
        fprintf(fp, " %12.5e",  *(float*)AryPtrX(&ary, i, j, k));
    }
    fprintf(fp, "\n");
  }
  fclose(fp);
  AryFree(&ary);
  TxtClr(&usrhdr);
  TxtClr(&syshdr);
  return 0;
eX_9:
  eMSG(_cant_read_file_$_, fin);
eX_1:
  eMSG(_improper_dimension_);
eX_2:
  eMSG(_cant_analyse_format_$_, form);
eX_3:
  eMSG(_invalid_element_size_);
eX_4:
  eMSG(_invalid_data_type_);
eX_5:
  eMSG(_cant_open_write_$_, name);
eX_6:
  eMSG(_no_format_);
}

//=============================================================== DmnDatAssert
//
int DmnDatAssert(  // verify value of a parameter
char *name,        // name of the variable  
char *hdr,         // string containing assignments 
float val )        // required value
{
  dP(DatAssert);
  float v;
  if (1 != DmnGetFloat(hdr, name, "%f", &v, 1))       eX(1);
  if (v != val)                                 			   eX(2);
  return 0;
eX_1:
  eMSG(_cant_get_value_$_, name);
eX_2:
  eMSG(_improper_value_$$$_, name, val, v);
}


#ifdef MAIN /*###############################################################*/

#include "IBJnls.h"
#include "IBJstamp.h"

#define  ISOPTION(a)  ('-' == (a))

static ARYDSC Ary;
static char Path[256];

static void Help( void )
  {
  /*
_help1_=usage: IBJdata <path> [<options>]\n
_help2_=<path>   : working directory\n"
_help3_=<options>:\n
_help4_=  -a<scale>   scaling factor\n
_help5_=  -c<cmpr>    compression level: 0..9\n
_help6_=  -d<outd>    name of output data file\n
_help7_=  -f<form>    format of output data\n
_help8_=  -i<input>   name of input file\n
_help9_=  -l<loc>    \"C\" or \"german\"\n
_help10_=  -m<mode>   \"binary\" or \"text\"\n
_help11_=  -o<out>     name of ouput file\n
_help12_=  -p          print output on screen\n
_help13_=  -s<sequ>    index order in output data\n
_help14_=  -t<zone>    time zone as GMT[+|-]hh:mm\n
_help15_=  -u<opt>     use these options [acmst] from input for output\n
  */
  printf(_help1_);
  printf(_help2_);
  printf(_help3_);
  printf(_help4_);
  printf(_help5_);
  printf(_help6_);
  printf(_help7_);
  printf(_help8_);
  printf(_help9_);
  printf(_help10_);
  printf(_help11_);
  printf(_help12_);
  printf(_help13_);
  printf(_help14_);
  printf(_help15_);
  exit(0);
  }

int main( int argc, char *argv[] )
{
  dP(DmnMain);
  int i, j, k, n, printing, list_format, arcinfo, verbose;
  double fact;
  char s[1024], *pc, *ps;
  char onam[MSG_MAXFNLEN], data[MSG_MAXFNLEN], inam[MSG_MAXFNLEN], fn[MSG_MAXFNLEN];
  char form[1024], sequ[64], mode[32], locl[32], cmpr[32], use[32], tmzn[32];
  TXTSTR usrhdr = { NULL, 0 };
  TXTSTR syshdr = { NULL, 0 };
  if (argc>1 && !strncmp(argv[1], "--language=", 11)) {           //-2006-10-26
    char *lan = argv[1] + 11;
    if (*lan) {
      if (NlsRead(argv[0], "IBJntr", lan))  printf(_problems_nls_$_, lan);
    }
  }
  printf(_main1_$$_, IBJdmnVersion, IBJstamp(__DATE__, __TIME__));
  printf(_main2_);
  printf(_main3_);
  printf(_$_main4_, "IBJdata");
  printf(_$_main5_, "IBJdata");
  if (argc < 2)  Help();
  strcpy(onam, "test");
  strcpy(cmpr, "0");
  *inam = 0;
  *data = 0;
  *form = 0;
  *mode = 0;
  *sequ = 0;
  *locl = 0;
  *tmzn = 0;
  *use = 0;
  printing = 0;
  verbose = 1;
  list_format = 0;
  arcinfo = 0;
  fact = HUGE_VAL;
  for (n=1; n<argc; n++) {
    strcpy(s, argv[n]);
    if (ISOPTION(*s)) {
      switch (s[1]) {
        case 'A': arcinfo = 1;
              break;
        case 'a': 
        						for (ps=s+2; (*ps); ps++)  if (*ps == ',')  *ps = '.'; //-2012-09-04
        						sscanf(s+2, "%lf", &fact);
              break;
        case 'c': strncpy(cmpr, s+2, 1);			                 //-2005-11-07
	            if (!isdigit(*cmpr))  *cmpr = '0';
              break;
        case 'd': strcpy(data, s+2);
              break;
        case 'f': strcpy(form, s+2);
              break;
        case 'h': Help();
              break;
        case 'i': strcpy(inam, s+2);
              break;
        case 'L': list_format = 1;
              break;
        case 'l': strcpy(locl, s+2);
              break;
        case 'm': strcpy(mode, s+2);
              break;
        case 'o': strcpy(onam, s+2);
              break;
        case 'p': printing = 1;
              break;
        case 's': strcpy(sequ, s+2);
              break;
        case 't': strcpy(tmzn, s+2);
              break;
        case 'u': strcpy(use, s+2);
              break;
        case 'v': sscanf(s+2, "%d", &verbose);
              break;
        default:  printf(_unknown_option_$_, s+1);
              exit(0);
      }
    }
    else  strcpy(Path, s);
  }
  if (*Path) {
    for (pc=Path; (*pc); pc++)  if (*pc == '\\')  *pc = '/';
    if (pc[-1] != '/') {
      pc[0] = '/';
      pc[1] = 0;
    }
  }
  else strcpy(Path, "./");
  strcpy(fn, Path);
  strcat(fn, "data.log");
  MsgFile = fopen(fn, "wb");
  if (!MsgFile) {
    printf(_cant_open_log_);
    exit(1);
  }
  MsgSetVerbose(verbose);
  MsgSetDisplay(1);
  for (i=0; i<n; i++)  fprintf(MsgFile, ">%s\n", argv[i]);
  if (*inam) {
    strcpy(fn, Path);
    strcat(fn, inam);
    DmnRead(fn, &usrhdr, &syshdr, &Ary);                    eG(10);
    if (verbose > 1) vMsg("IN  usr:\n%s", usrhdr.s);
    if (verbose > 1) vMsg("IN  sys:\n%s", syshdr.s);
    if (!*form) {
      DmnCpyString(syshdr.s, "form", form, 512);
    }
    if (!*locl) {                                           //-2005-01-25
      DmnCpyString(syshdr.s, "locl", locl, 32);
    }
    if (*use) {
      if (!*cmpr && strchr(use, 'c'))  DmnCpyString(syshdr.s, "cmpr", cmpr, 32);
      if (!*mode && strchr(use, 'm'))  DmnCpyString(syshdr.s, "mode", mode, 32);
      if (!*sequ && strchr(use, 's'))  DmnCpyString(syshdr.s, "sequ", sequ, 64);
      if (!*tmzn && strchr(use, 't'))  DmnCpyString(syshdr.s, "tmzn", tmzn, 32);
      if (fact == HUGE_VAL && strchr(use, 'a')) {
        DmnGetDouble(syshdr.s, "fact", "%lf", &fact, 1);
      }
    }
    if (list_format)  DmnPrnForm(DmnFrmTab, _in_input_file_);
  }
  else {
      AryCreate(&Ary, sizeof(float), 3, 1, 3, 2, 4, 0, 4);    eG(1);
      for (i=1; i<=3; i++)
      for (j=2; j<=4; j++)
        for (k=0; k<=4; k++)
          *(float*)AryPtrX(&Ary, i, j, k) = 100*i + 10*j + k;
      strcpy(form, "%8.2f");
  }
  if (arcinfo) {
    Dmn2ArcInfo(fn, NULL);
    vMsg(_finished_);
    return 0;
  }
  if (printing) {
    strcpy(mode, "text");
    strcpy(fn, "stdout");
  }
  else {
    strcpy(fn, Path);
    strcat(fn, onam);
  }
  if (!*locl)  strcpy(locl, "C");
  //
  if (*locl) {
    sprintf(s, "locl  \"%s\"\n", locl); TxtCat(&usrhdr, s);
  }
  if (*mode) {
    sprintf(s, "mode  \"%s\"\n", mode); TxtCat(&usrhdr, s);
  }
  if (*data) {
    sprintf(s, "data  \"%s\"\n", data); TxtCat(&usrhdr, s);
  }
  if (*sequ) {
    sprintf(s, "sequ  \"%s\"\n", sequ); TxtCat(&usrhdr, s);
  }
  if (*form) {
    sprintf(s, "form  \"%s\"\n", form); TxtCat(&usrhdr, s);
  }
  if (*cmpr) {                                                  //-2005-11-07
    sprintf(s, "cmpr  %s\n", cmpr); TxtCat(&usrhdr, s);
  }
  if (*tmzn) {                                                  //-2005-12-08
    sprintf(s, "tmzn  %s\n", tmzn); TxtCat(&usrhdr, s);
  }
  if (fact != HUGE_VAL) {
    sprintf(s, "fact  %10.3e\n", fact); TxtCat(&usrhdr, s);
  }
  if (verbose > 1) vMsg("OUT:\n%s", usrhdr.s);
  DmnWrite(fn, usrhdr.s, &Ary);             eG(2);
  if (list_format)  DmnPrnForm(DmnFrmTab, "in output file");
  vMsg("finished");
  return 0;
eX_10:
    eMSG(_main_no_input_);
eX_1:
    eMSG(_main_cant_create_date_);
eX_2:
    eMSG(_main_cant_write_file_$_, onam);
}

#endif  //##############################################################

/*======================================================================

 history:

 2000-07-27 uj  extended for IBJodor
 2001-03-22 lj  1.0.13 length specifier "b" in format
 2001-04-25 uj  1.0.14 DmnRplValues corrected
 2001-04-27 uj  1.0.15 updated with LJ
 2001-04-29 lj  1.0.16 updated with LJ
 2001-04-30 uj  1.1.0 format: strings as character arrays, e.g. %20c
 2001-05-03 lj  1.1.1 DmnFrmPointer()
 2001-06-06 lj  1.3.1 #define for AUSTAL2000
 2001-07-11 lj  1.3.3 dmna-file opended with "w"
 2001-09-05 uj  1.3.4 DmnRead() corrected for line length > bufsize
 2001-09-14 uj  1.3.5 synchronized
 2002-04-05 lj  1.4.0 adapted to modified IBJmsg
 2003-07-07 lj  1.4.2 localisation
 2004-11-15 uj  1.4.3 DmnWrite: drop of valdef entries corrected
 2005-01-24 uj  1.4.4 DmnArrHeader: account for blank lines and comments
 2005-01-25 lj  1.4.4 DmnArrHeader, main: modified
 2004-12-21 uj  1.4.4 Dmn2ArcInfo
 2005-06-28 uj  1.4.5 secure factor reading in AnaForm
 2005-08-08 uj  1.4.6 DmnRead: change of bufsize corrected
 2005-11-07 lj  1.5.0 zlib included
 2005-11-09 lj  1.5.1 DmnRead: large input lines for formatted data
 2005-11-29 lj  1.5.2 option -u
 2005-12-01 lj  1.6.0 selection in input corrected
 2005-12-11 lj  1.6.1 parameter drop
 2006-01-18 lj  1.6.2 DmnWrite() name with extension ".dmna" allowed
 2006-02-15 lj  1.6.3 Dmn2ArcInfo corrected, freeing of strings
                      Comment allowed in data lines
 2006-02-17 uj  1.6.4 Dmn2ArcInfo xllcorner/yllcorner as absolute values
 2006-03-20 lj  1.6.5 DmnWrite writing binary
 2006-05-29 uj  1.6.6 DmnArrHeader: blank at end of the line
 2006-06-08 uj  1.6.7 DmnRead: don't use union for scanning strings
 2006-07-10 uj  1.6.8 DmnWrite: avoid duplicate parameters
 2006-10-19 uj  1.6.9 DmnRead: fread instead of fgets for formatted reading
 2006-10-20 uj  1.6.10 DmnGetValues: skip comments (')
 2006-10-26 lj  2.0.0  external strings
 2008-10-17 lj  2.1.0  uses MsgSetLocal()
 2008-12-11 lj  2.1.1  header parameter modified
 2011-07-07 uj  2.2.0  support for ARR files terminated
 2011-07-21 uj  2.2.1  brackets added for eG statement
 2012-09-04 uj  2.2.2  replace ',' by '.' before parsing
                       DmnRead/DmnWrite: handle locl="de" and locl="en"
 2013-10-14 uj  2.2.3  apply/check maximum day index DAYMAX
=======================================================================*/

