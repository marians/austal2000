//======================================================== IBJtxt.c
//
// Dynamically allocated strings for IBJ-programs
// ==============================================
//
// Copyright (C) Umweltbundesamt, Dessau-Roﬂlau, Germany, 2002-2006
// Copyright (C) Janicke Consulting, 88662 ‹berlingen, Germany, 2002-2006
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
// last change: 2011-06-29 uj
//
//==================================================================

char *IBJtxtVersion = "2.1.0";
static char *eMODn = "IBJtxt";

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>
#include <signal.h>

#include "IBJtxt.h"
#include "IBJmsg.h"
#include "IBJtxt.nls"

//=============================================================== TxtUnquote
char *TxtUnquote( // unquote a string if necessary
char* s )         // the string
{
  char *p;
  int i, l, start;
  l = strlen(s); if (s[l-1] == '\"') l--;
  for (i=0; i<l; i++) if (s[i] != ' ') break; //-2001-07-09 uj
  start = (s[i] == '\"') ? i+1 : i;
  p = s;
  for (i=start; i<l; i++) *p++ = s[i];
  *p = 0;
  return s;
}

//================================================================= TxtQuote
char *TxtQuote(   // quote a string if necessary
char *s )         // the string
{
  static char ss[512];                                  //-2001-04-21 lj
  char *p;
  int i, l;
  l = strlen(s);
  if (l >= 510)  return s;                              //-2001-04-21 lj
  if (s[0] == '\"') p = ss;
  else { ss[0] = '\"'; p = ss+1; }
  for (i=0; i<l; i++) *p++ = s[i];
  if (s[l-1] != '\"') *p++ = '\"';
  *p = 0;
  return ss;  
}

//=================================================================== TxtAlloc
char *TxtAlloc( // allocate string space
int n )     // number of bytes
{
  dQ(TxtAlloc);
  char *s;
  s = ALLOC(n);
  if (!s) {
    fprintf(stderr, _allocation_failed_$_, n);  //-2004-08-30
    MsgCode = -1;
    raise(SIGSEGV);
  }
  memset(s, 0, n);
  return s;
}

//==================================================================== TxtCat
int TxtCat(   // concatenate string to text
TXTSTR *pa,   // text (first part)
char *b )   // string (second part)
{
  dQ(TxtCat);
  int lb;
  TXTSTR c;
  if (!pa || !b)  return 0;
  lb = strlen(b);
  c.l = pa->l + lb;
  c.s = TxtAlloc(c.l+1);
  if (pa->s) { strcpy(c.s, pa->s);  FREE(pa->s);  pa->s = NULL;  pa->l = 0; }
  strcat(c.s, b);
  *pa = c;
  return 0;
  }

//=================================================================== TxtNCat
int TxtNCat(  // concatenate at most n bytes of string to text
TXTSTR *pa,   // text (first part)
char *b,    // string (second part)
int n )     // number of bytes to add
{
  dQ(TxtNCat);
  int lb;
  TXTSTR c;
  if (!pa || !b)  return 0;
  lb = strlen(b);
  if (n > lb)  n = lb;
  c.l = pa->l + n;
  c.s = TxtAlloc(c.l+1);
  if (pa->s) { strcpy(c.s, pa->s);  FREE(pa->s);  pa->s = NULL;  pa->l = 0; }
  strncat(c.s, b, n);
  c.s[c.l] = 0;
  *pa = c;
  return 0;
  }

//======================================================================== TxtCpy
int TxtCpy(   // copy string to text
TXTSTR *pa,   // text (set)
char *b )   // string (copied)
{
  dQ(TxtCpy);
  int lb;
  if (!pa)  return 0;
  if (!b) {
    pa->s = NULL;
    pa->l = 0;
    return 0;
    }
  lb = strlen(b);
  if (pa->s) { FREE(pa->s);  pa->s = NULL;  pa->l = 0; }
  pa->s = TxtAlloc(lb+1);
  strcpy(pa->s, b);
  pa->l = lb;
  return 0;
  }

//======================================================================== TxtNCpy
int TxtNCpy(  // copy at most n bytes of string to text
TXTSTR *pa,   // text (set)
char *b,    // string (copied)
int n )     // number of bytes
  {
  dQ(TxtNCpy);
  int lb;
  if (!pa || !b)  return 0;
  lb = strlen(b);
  if (pa->s) { FREE(pa->s);  pa->s = NULL;  pa->l = 0; }
  pa->s = TxtAlloc(n+1);
  strncpy(pa->s, b, n);
  pa->l = n;
  return 0;
  }

//========================================================================== TxtClr
int TxtClr(   // clear text space
TXTSTR *pa )  // text
  {
  dQ(TxtClr);
  if (!pa)  return 0;
  if (pa->s)  FREE(pa->s);
  pa->s = NULL;
  pa->l = 0;
  return 0;
  }
  
//==================================================================== TxtForm
char *TxtForm(          // ASCII transformation of a number
double x,               // number to be transformed
int n)                  // significant places
  {
  static char rr[10][80];
  static int ir;
  char t[80], *pe, *pp, *r;
  int iexp;
  r = rr[ir];
  ir++;
  if (ir > 9)  ir = 0;
  if (n < 1)  n = 1;
  if (n > 16)  n = 16;
  sprintf(t, "%24.*le", n-1, x);                                  //-2010-09-14
  pp = strchr(t, '.');
  pe = strchr(t, 'e');
  iexp = atoi(pe+1);
  if ((iexp <= n+2) && (iexp >= n-1)) {
    sprintf(r, "%1.0lf", x);                                      //-2010-09-14
    return r; }
  if ((iexp <= n-2) && (iexp >= -3)) {
    sprintf(r, "%1.*lf", n-1-iexp, x);                            //-2010-09-14
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
  
//===================================================================== TxtPrn
char *TxtPrn(           // printed pattern of a number
double x,               // number to be transformed
char *frm)              // print format
{
  static char rr[10][80];
  static int ir;
  char *r;
  r = rr[ir];
  ir++;
  if (ir > 9)  ir = 0;
  sprintf(r, frm, x);
  return r;
}

//========================================================================= TxtPCpy
int TxtPCpy(  // copy string to text at position pos
TXTSTR *pa,   // text (set)
int pos,      // position within text
char *b )     // string to be copied
  {
  dQ(TxtPCpy);
  int lb;
  TXTSTR c;
  if (!pa || !b)  return 0;
  if (pos < 0)  pos = 0;
  if (pos > pa->l)  pos = pa->l;
  lb = strlen(b);
  c.l = pos + lb;
  c.s = TxtAlloc(c.l+1);
  if (pos > 0)  strncpy(c.s, pa->s, pos);
  strcpy(c.s+pos, b);
  if (pa->s)  FREE(pa->s);
  *pa = c;
if (c.l != strlen(c.s)) printf("%s", _inconsistent_length_);
  return 0;
  }

//========================================================================= TxtPIns
int TxtPIns(  // insert string in text at position pos
TXTSTR *pa,   // text (set)
int pos,    // position within text
char *b )   // string to be inserted
  {
  dQ(TxtPIns);
  int lb;
  TXTSTR c;
  if (!pa || !b)  return 0;
  if (pos < 0)  pos = 0;
  if (pos > pa->l)  pos = pa->l;
  lb = strlen(b);
  c.l = pa->l + lb;
  c.s = TxtAlloc(c.l+1);
  if (pos > 0)  strncpy(c.s, pa->s, pos);
  strcpy(c.s+pos, b);
  if (pa->s) {
      strcat(c.s, pa->s+pos);
      FREE(pa->s);
      }
  *pa = c;
if (c.l != strlen(c.s)) printf("%s", _inconsistent_length_);
  return 0;
  }

//======================================================================= TxtPNDel
int TxtPNDel( // delete n bytes at position pos
TXTSTR *pa,   // text
int pos,      // position within text
int n )       // number of bytes to delete
  {
  dQ(TxtPNDel);
  TXTSTR c;
  if (!pa)  return 0;
  if (pos < 0)  pos = 0;
  if (pos > pa->l)  pos = pa->l;
  if (n < 0)  n = 0;
  if (pos+n > pa->l)  c.l = pos;
  else                c.l = pa->l - n;
  c.s = TxtAlloc(c.l+1);
  if (pos > 0)  strncpy(c.s, pa->s, pos);
  if (pos >= c.l)  c.s[c.l] = 0;
  if (pa->s) {
      strcpy(c.s+pos, pa->s+pos+n);
      FREE(pa->s);
      }
  *pa = c;
if (c.l != strlen(c.s)) printf("%s", _inconsistent_length_);
  return 0;
  }

//========================================================================= TxtRpl
int TxtRpl(   // replace string within text
TXTSTR *pa,   // text
char *old,    // string pattern to be replaced
char *new )   // replacing string
  {
  dQ(TxtRpl);
  int lold, lnew, n;
  char *p, *q;
  TXTSTR c;
  if (!pa || !old || !new)  return 0;
  lold = strlen(old);
  lnew = strlen(new);
  if (lold == 0)  return 0;
  if (!pa->l)  return 0;
  if (lold == lnew) {
      p = pa->s;
      while (NULL != (p = strstr(p, old))) {
          strncpy(p, new, lnew);
          p += lnew; }
      return 0; }
  n = 0;
  p = pa->s;
  while (NULL != (p = strstr(p, old))) {
      n++;
      p += lold; }
  if (n == 0)  return 0;
  c.l = pa->l + n*(lnew-lold);
  c.s = TxtAlloc(c.l+1);
  p = pa->s;
  while (NULL != (q = strstr(p, old))) {
      strncat(c.s, p, q-p);
      strcat(c.s, new);
      p = q + lold; }
  strcat(c.s, p);
  FREE(pa->s);
  *pa = c;
if (c.l != strlen(c.s)) printf("%s", _inconsistent_length_);
  return 0;
  }
  
int TxtPrintf(  // printf() into a TXTSTR buffer
TXTSTR *pa,     // pointer to the buffer
char *format,   // print format
...)            // parameters
{
  dQ(TxtPrintf);
  va_list argptr;
  int nb;
  va_start(argptr, format);
  nb = TxtVPrintf(pa, format, argptr);
  va_end(argptr);
  return nb;
}

int TxtVPrintf( // printf() into a TXTSTR buffer
TXTSTR *pa,     // pointer to the buffer
char *format,   // print format
va_list argptr) // parameters
{
  dQ(TxtVPrintf);
  int i, nb=-1;
  int bufsize = 0;
  char *ptr;
  if (!pa || !format)
    return 0;
  if (pa->s) {
    FREE(pa->s);
    pa->s = NULL;
    pa->l = 0;
  }
#if defined __linux__
  nb = vasprintf(&ptr, format, argptr);
  if (nb >= 0) {
    TxtCat(pa, ptr);
    free(ptr);
  }
#elif defined _WIN32
  for (i=0; i<20; i++) {
    bufsize += MSG_MAXNMLEN;
    pa->s = TxtAlloc(bufsize);
    nb = _vsnprintf(pa->s, bufsize, format, argptr);
    pa->l = nb;
    if (nb >= 0 && nb < bufsize)
      break;
    FREE(pa->s);
    pa->s = NULL;
    pa->l = 0;
    nb = -1;
  }
#endif
  return nb;
}



/*==============================================================
//
// history:
//
// 2000-07-27 uj  extended for IBJodor
// 2001-04-27 uj  TxtQuote(), TxtUnquote()
// 2006-10-26 lj 2.0.0  external strings
// 2011-06-29 uj 2.1.0  additional routines TxtPrintf, TxtVPrintf, TxtForm
//
//=============================================================*/
