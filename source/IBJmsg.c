//======================================================== IBJmsg.c
//
// Message Routines for IBJ-programs
// =================================
//
// Copyright (C) Umweltbundesamt, Dessau-Roﬂlau, Germany, 2006
// Copyright (C) Janicke Consulting, 88662 ‹berlingen, Germany, 2006-2014
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
// last change 2014-01-30 uj
//
//==================================================================

char *IBJmsgVersion = "2.1.2";
static char *eMODn = "IBJmsg";

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <math.h>
#include <time.h>
#include <locale.h>

#include "IBJmsg.h"
#include "IBJmsg.nls"

FILE *MsgFile;
int MsgCode, MsgBreak, MsgLevel=0, MsgQuiet=0, MsgVerbose=1;
int MsgLogLevel=3, MsgDspLevel=2;
int MsgTranslate=0;                               //-2002-07-03
int MsgDateHour, MsgDateMinute, MsgDateSecond;
int MsgDateYear, MsgDateMonth, MsgDateDay;
int MsgBufLen = 4000;
void *MsgPvd;
char *MsgSource;
char MsgLocale[256];
static int StdQuiet=-99;
static int StdVerbose=-99;
static int StdLevel=0;
//
//---------------------------------------------------------------//-2006-02-13

#define MINALLOC 4000

static int DEBUG = 0;
static int MaxAlloc = 0;                                          //-2006-03-13
static long long bytes_allocated;
static long long bytes_maximum;
static int *bytes;
static void **ptrs;
static time_t *times;
static int *counters;
static char **names;
static char *namebuf;
static int numalloc=0, maxalloc=0, cntalloc=0;

static void InitAlloc(int size) {                                 //-2006-03-13
  int i;                                                          //-2006-03-13
  int *by = bytes;
  void **pt = ptrs;
  time_t *ti = times;
  int *co = counters;
  char **ns = names;
  char *nf = namebuf;
  if (size < MaxAlloc)  size = MaxAlloc;
  if (numalloc >= size)  size = numalloc + MINALLOC;
  if (size <= MaxAlloc)  return;
  //
  bytes = calloc(size, sizeof(int));
  ptrs = calloc(size, sizeof(void*));
  times = calloc(size, sizeof(time_t));
  counters = calloc(size, sizeof(int));
  namebuf = calloc(1, size*MSG_MAXNMLEN*sizeof(char));
  names = calloc(size, sizeof(char*));
  if (!bytes || !ptrs || !times || !names || !namebuf) {
    fprintf(stderr, _no_allocation_tables_$_, size);
    if (MsgFile != NULL)
      fprintf(MsgFile, _no_allocation_tables_$_, size);
    exit(1);
  }
  for (i=0; i<size; i++)
    names[i] = namebuf + i*MSG_MAXNMLEN;
  //
  if (numalloc) {                                                 //-2006-03-13
    for (i=0; i<MaxAlloc; i++) {
      bytes[i] = by[i];
      ptrs[i] = pt[i];
      times[i] = ti[i];
      counters[i] = co[i];
      if (*ns[i])  strcpy(names[i], ns[i]);
    }
    free(by);
    free(pt);
    free(ti);
    free(co);
    free(nf);
    free(ns);
  }
  MaxAlloc = size;
}

void* MsgAlloc(int size, char *mod, char *name, int line) {
  void* p = NULL;
  int i;
  char id[MSG_MAXNMLEN];
  sprintf(id, "%s.%s.%d", mod, name, line);
  if (!bytes || numalloc>=MaxAlloc)  InitAlloc(0);
  p = calloc(1, size);
  if (p == NULL) {
    fprintf(stderr, _cant_allocate_$_, size);
    fprintf(stderr, MSG_I64DFORM, bytes_allocated);
    fprintf(stderr, "\n     (%s)\n", id);
    return NULL;
  }
  cntalloc++;
  //
#ifdef MSGLOGALLOC
  if (MsgFile) {
    fprintf(MsgFile, "ALLOC %08x(%d) @ %s\n", p, size, id);
    fflush(MsgFile);
  }
#endif
  for (i=0; i<MaxAlloc; i++) {
    if (ptrs[i] == NULL) {
      numalloc++;
      if (numalloc > maxalloc)  maxalloc = numalloc;
      ptrs[i] = p;
      bytes[i] = size;
      strcpy(names[i], id);
      times[i] = time(NULL);
      counters[i] = cntalloc;
      bytes_allocated += size;
      if (bytes_allocated > bytes_maximum)  bytes_maximum = bytes_allocated;
      if (DEBUG)  MsgListAlloc(NULL);
      return p;
    }
  }
  fprintf(stderr, "%s", _buffer_overflow_);
  MsgListAlloc(NULL);
  MsgCode = -2;
  exit(2);
}

void* MsgCalloc(int n, int size, char *mod, char *name, int line) {
  void* p;
  int i;
  char id[MSG_MAXNMLEN];
  sprintf(id, "%s.%s.%d", mod, name, line);
  if (!bytes || numalloc>=MaxAlloc)  InitAlloc(0);
  p = calloc(n, size);
  if (p == NULL) {
    fprintf(stderr, _cant_allocate_$_, size);
    fprintf(stderr, MSG_I64DFORM, bytes_allocated);
    fprintf(stderr, "\n     (%s)\n", id);
    return NULL;
  }
  cntalloc++;
#ifdef MSGLOGALLOC
  if (MsgFile) {
    fprintf(MsgFile, "CALLOC %08x(%d,%d) @ %s\n", p, n, size, id);
    fflush(MsgFile);
  }
#endif
  for (i=0; i<MaxAlloc; i++) {
    if (ptrs[i] == NULL) {
      numalloc++;
      if (numalloc > maxalloc)  maxalloc = numalloc;
      ptrs[i] = p;
      bytes[i] = n*size;
      strcpy(names[i], id);
      times[i] = time(NULL);
      counters[i] = cntalloc;
      bytes_allocated += n*size;
      if (bytes_allocated > bytes_maximum)  bytes_maximum = bytes_allocated;
      if (DEBUG)  MsgListAlloc(NULL);
      return p;
    }
  }
  fprintf(stderr, "%s", _buffer_overflow_);
  MsgListAlloc(NULL);
  MsgCode = -3;
  exit(3);
}

void  MsgFree(void *p, char *mod, char *name, int line) {
  int i;
  char id[MSG_MAXNMLEN];
  if (p == NULL)  return;
  sprintf(id, "%s.%s.%d", mod, name, line);
  if (!bytes || numalloc>=MaxAlloc)  InitAlloc(0);
#ifdef MSGLOGALLOC
  if (MsgFile) {
    fprintf(MsgFile, "FREE %08x @ %s\n", p, id);
    fflush(MsgFile);
  }
#endif
  for (i=0; i<MaxAlloc; i++) {
    if (p == ptrs[i]) {
      free(p);
      numalloc--;
      ptrs[i] = NULL;
      *names[i] = 0;
      times[i] = 0;
      counters[i] = 0;
      bytes_allocated -= bytes[i];
      bytes[i] = 0;
      if (DEBUG)  MsgListAlloc(NULL);
      return;
    }
  }
  fprintf(stderr, _invalid_free_$$_, p, id);
  MsgListAlloc(NULL);
  MsgCode = -4;
  exit(4);
}

void  MsgCheckAlloc(void *p, char *mod, char *name, int line) {
  int i;
  char id[MSG_MAXNMLEN];
  if (p == NULL)  return;
  sprintf(id, "%s.%s.%d", mod, name, line);
  if (!bytes || numalloc>=MaxAlloc)  InitAlloc(0);
  for (i=0; i<MaxAlloc; i++) {
    if (p == ptrs[i])
      return;
  }
  fprintf(stderr, "MSG: Invalid address p=%p\n     (%s)\n", p, id);
  MsgCode = -4;
  exit(4);
}

void* MsgRealloc(void *p, int size, char *mod, char *name, int line) {
  void *q;
  int i;
  char id[MSG_MAXNMLEN];
  sprintf(id, "%s.%s.%d", mod, name, line);
  if (!bytes || numalloc>=MaxAlloc)  InitAlloc(0);
  for (i=0; i<MaxAlloc; i++) {
    if (p == ptrs[i]) {
      q = realloc(p, size);
      if (q == NULL) {
        fprintf(stderr, _cant_allocate_$_, size);
        fprintf(stderr, MSG_I64DFORM, bytes_allocated);
        fprintf(stderr, "\n     (%s)\n", id);
        return NULL;
      }
#ifdef MSGLOGALLOC
  if (MsgFile) {
    fprintf(MsgFile, "REALLOC %08x=>%08x(%d) @ %s\n", p, q, size, id);
    fflush(MsgFile);
  }
#endif
      cntalloc++;
      if (p == NULL) {
        numalloc++;
        if (numalloc > maxalloc)  maxalloc = numalloc;
      }
      else {
        ptrs[i] = NULL;
        *names[i] = 0;
        times[i] = 0;
        bytes_allocated -= bytes[i];
        bytes[i] = 0;
      }
      ptrs[i] = q;
      bytes[i] = size;
      strcpy(names[i], id);
      times[i] = time(NULL);
      counters[i] = cntalloc;
      bytes_allocated += size;
      if (bytes_allocated > bytes_maximum)  bytes_maximum = bytes_allocated;
      if (DEBUG)  MsgListAlloc(NULL);
      return q;
    }
  }
  fprintf(stderr, _invalid_realloc_$$_, p, id);
  return NULL;
}

long long MsgGetNumBytes(void) {
  return bytes_allocated;
}

long long MsgGetMaxBytes(void) {
  return bytes_maximum;
}

int MsgGetNumAllocs() {
  return numalloc;
}

int MsgGetMaxAllocs() {
  return maxalloc;
}

int MsgListAlloc(FILE *prn) {
  int i, n;
  char ts[80];
  if (!bytes)  return -1;
  if (prn == NULL)  prn = MsgFile;
  if (prn == NULL)  prn = stderr;
  fprintf(prn, "%s", _table_allocations_);
  fprintf(prn, "%s", _table_header_);
  n = 0;
  for (i=0; i<MaxAlloc; i++) {
    if (ptrs[i] == NULL)  continue;
    n++;
    strftime(ts, 79, "%Y-%m-%d %H:%M:%S", localtime(times+i));
    fprintf(prn, "%9d:  %9d  @%8p  %s  %s\n",
      counters[i], bytes[i], ptrs[i], ts, names[i]);
  }
  fprintf(prn, "%s", _currently_allocated_);
  fprintf(prn, MSG_I64DFORM, bytes_allocated);
  fprintf(prn, "\n");
  fprintf(prn, "%s", _maximum_allocated_);
  fprintf(prn, MSG_I64DFORM, bytes_maximum);
  fprintf(prn, "\n");
  fprintf(prn, _number_allocations_$$$_, numalloc, maxalloc, MaxAlloc);
  return n;
}

//-----------------------------------------------------------------------------

/*===================================================================== MsgLow
*/
void MsgLow(    /* convert to lower case        */
char *s )       /* string to be converted       */
  {
  while (*s) {
    *s = tolower(*s);
    s++;
    }
  }

//===================================================================== MsgTok
//
char *MsgTok(   /* non-destructive tokenisation */
char *start,    /* begin of scanning            */
char **pnext,   /* start of next scan           */
char *sep,      /* separator characters         */
int *pl )       /* number of usable characters  */
  {             /* RETURN: start of token       */
  int l, quoted;
  if ((!start) || (!sep))  return NULL;
  while (*start && strchr(sep, *start))  start++;
  if (!*start)  return NULL;
  if (*start == MsgBreak)  return NULL;
  quoted = 0;
  for (l=0; ; l++) {
    if (!start[l])  break;
    if (strchr(sep, start[l]) && !quoted)  break;
    if (start[l] == '\"') {
      if (l==0 || start[l-1]!='\\')  quoted = 1-quoted;
    }
  }
  if (pnext)  *pnext = start + l;
  if (pl)  *pl = l;
  return start;
}

//================================================================= MsgTokens
//
char **_MsgTokens(  // tokenize a string
char *t,            // string to be tokenized (not changed by this routine)
char *s )           // separator characters
{                   // RETURN: allocated array of strings, ends with NULL
  dQ(_MsgTokens);
  int n, l;
  char *pt, *pn, **_ppt;
  if (!t || !s)  return NULL;
  for (pn=t, n=0; ; n++) {
    pt = MsgTok(pn, &pn, s, &l);
    if (!pt)  break;
  }
  _ppt = (char**) ALLOC((n+1)*sizeof(char*) + strlen(t) + 1); //-2001-08-15
  if (!_ppt)  return NULL;
  pt = (char*)(_ppt+n+1);                                     //-2001-08-15
  strcpy(pt, t);                                              //-2001-08-15
  for (pn=pt, n=0; ; n++) {                                   //-2001-08-15
    pt = MsgTok(pn, &pn, s, &l);
    if (!pt)  break;
    _ppt[n] = pt;
    if (*pn)  *pn++ = 0;
  }
  _ppt[n] = NULL;
  return _ppt;
}

//================================================================ MsgDequote
//
int MsgDequote( //  remove quotation marks and escape sequences
char *s )       //  string to work on
{               //  RETURN: number of replacements
  int n;
  char *psrc, *pdst;
  if (s == NULL)  return 0;
  n = 0;
  for (psrc=s, pdst=s; (*psrc); psrc++) {
    if (*psrc == '\"') {
      n++;
      continue;
    }
    if (*psrc == '\\') {
      char c=0;
      switch (psrc[1]) {
        case '\"': c = '\"';  break;
        case '\'': c = '\'';  break;
        case '\\': c = '\\';  break;
        case '?':  c = '?';   break;
        case 'a':  c = '\a';  break;
        case 'b':  c = '\b';  break;
        case 'f':  c = '\f';  break;
        case 'n':  c = '\n';  break;
        case 't':  c = '\t';  break;
        case 'r':  c = '\r';  break;
        case 'v':  c = '\v';  break;
        default: ;
      }
      if (c) {
        *pdst++ = c;
        psrc++;
        n++;
        continue;
      }
      if (psrc[1]=='x') {
        char b[16], *tail;
        *b = 0;
        strncat(b, psrc+2, 2);
        *pdst++ = strtol(b, &tail, 16);
        psrc += 3;
        n++;
        continue;
      }
      if (isdigit(psrc[1])) {
        char b[16], *tail;
        *b = 0;
        strncat(b, psrc+1, 3);
        *pdst++ = strtol(b, &tail, 8);
        psrc += 3;
        n++;
        continue;
      }
    }
    *pdst++ = *psrc;
  }
  *pdst = 0;
  return n;
}

//================================================================ MsgUnquote
//
int MsgUnquote( //  remove quotation marks
char *s )       //  string to work on
{               //  RETURN: number of replacements
  int n;
  char *psrc, *pdst;
  if (s == NULL)  return 0;
  n = 0;
  for (psrc=s, pdst=s; (*psrc); psrc++) {
    if (*psrc == '\"') {
      n++;
      continue;
    }
    *pdst++ = *psrc;
  }
  *pdst = 0;
  return n;
}

//============================================================= MsgSetVerbose
int MsgSetVerbose( int new_value )  // set standard value of MsgVerbose
{
  int old_value = StdVerbose;
// printf("MSG set verbose=%d\n", new_value);
  MsgVerbose = new_value;
  StdVerbose = new_value;
  return old_value;
}

//============================================================= MsgSetQuiet
int MsgSetQuiet( int new_value )
{
  int old_value = StdQuiet;
// printf("MSG set quiet=%d\n", new_value);
  MsgQuiet = new_value;
  StdQuiet = new_value;
  return old_value;
}

//============================================================= MsgSetLevel
int MsgSetLevel( int new_value )
{
  int old_value = StdLevel;
  MsgLevel = new_value;
  StdLevel = new_value;
  return old_value;
}

//=============================================================== MsgSetLocale
int MsgSetLocale(char *l) {                                       //-2008-10-17
  char *lcl, *pc;
  static char locale[256], syslcl[256], oldlcl[256];
  if (l == NULL || *l == 0)
    return 0;
  strcpy(oldlcl, setlocale(LC_NUMERIC, NULL));
  if (strcmp(l, "german"))  l = "C";
  lcl = setlocale(LC_NUMERIC, l);
  if (lcl != NULL || !strcmp(l, "C"))
    return 0;
  strcpy(locale, "de_DE");
  strcpy(syslcl, setlocale(LC_NUMERIC, ""));
  pc = strchr(syslcl, '.');
  if (pc)  strcat(locale, pc);
  lcl = setlocale(LC_NUMERIC, locale);
  if (lcl == NULL) {
    setlocale(LC_NUMERIC, oldlcl);
  }
  strcpy(MsgLocale, setlocale(LC_NUMERIC, NULL));
  return (lcl == NULL);
}

//====================================================================== vMsg
int vMsg(       /* print message on screen and into logfile     */
char *format,   /* printing format                              */
... )           /* parameter                                    */
{
  dQ(vMsg);
  va_list argptr;
  int verbose, display;
  char time_stamp[80]="", *_form=NULL;
  char locale[256];
  static char *s, *pc;
  static char DSTcode[] = { 0x84,0x94,0x81,0x8E,0x99,0x9A,0xE6,0xE1,0xFD,0xFC,0x00 };
  static char SRCcode[] = { 0xE4,0xF6,0xFC,0xC4,0xD6,0xDC,0xB5,0xDF,0xB2,0xB3,0x00 };
  int i, l, drop_cr=0;
  if (!format) {
    if (s)  FREE(s);
    s = NULL;
    return 0;
  }
  if (!s) {
    s = ALLOC(MsgBufLen);
    if (!s) {
      printf(_no_message_buffer_$_, MsgBufLen);
      fflush(stdout);
      exit(1);
    }
  }
  if (*format == '@') {
    strcpy(time_stamp, MsgDate());
    format++;
  }
  l = strlen(format);
  if (l>0 && format[l-1]=='\\') {
    drop_cr = 1;
    _form = ALLOC(l+1);
    strcpy(_form, format);
    _form[l-1] = 0;
  }
  else {
    drop_cr = 0;
    _form = format;
  }
  display = (MsgQuiet < 0) ? 1 : ((MsgVerbose>MsgLevel) && !MsgQuiet); //-2001-08-10
  verbose = (MsgCode < 0) ? (MsgVerbose>=0) : (MsgVerbose>MsgLevel);
  if (*MsgLocale) {                                               //-2008-10-20
    strcpy(locale, setlocale(LC_NUMERIC, NULL));
    setlocale(LC_NUMERIC, MsgLocale);
  }
  if (display) {  //-2001-08-10
#ifdef __WATCOMC__
    int n;
    va_start(argptr, format);
    n = _vbprintf(s, MsgBufLen-1, _form, argptr);
    if (n < 0)  sprintf(s, _message_buffer_overflow_);
#else
    va_start(argptr, format);
    vsprintf(s, _form, argptr);        // buffer overflow not caught
#endif
    va_end(argptr);
    if (MsgTranslate) {
      l = strlen(s);
      for (i=0; i<l; i++) {
        pc = strchr(SRCcode, s[i]);
        if (pc)  s[i] = DSTcode[pc-SRCcode];
      }
    }
    if (*time_stamp)  fprintf(stdout, "%s ", time_stamp);
    else if (MsgCode < 0)  fprintf(stdout, "*** ");
    fprintf(stdout, "%s", s);
    if (!drop_cr)  fprintf(stdout, "\n");
    if (MsgSource && *MsgSource)  fprintf(stdout, "    (%s)\n", MsgSource);
    fflush(stdout);
  }
  va_start(argptr, format);
  if (MsgFile && verbose) {
    if (MsgCode < 0)  fprintf(MsgFile, "*** ");
    if (*time_stamp)  fprintf(MsgFile, "%s ", time_stamp);
    vfprintf(MsgFile, _form, argptr);
    if (!drop_cr)  fprintf(MsgFile, "\n");
    if (MsgSource && *MsgSource)  fprintf(MsgFile, "    (%s)\n", MsgSource);
    fflush(MsgFile);
  }
  va_end(argptr);
  if (*MsgLocale) {                                               //-2008-10-20
    setlocale(LC_NUMERIC, locale);
  }
  if (drop_cr)  FREE(_form);
  MsgSource = NULL;
  MsgLevel = 0;
  if (StdQuiet != -99)    MsgQuiet = StdQuiet;
  if (StdVerbose != -99)  MsgVerbose = StdVerbose;
  if (StdLevel != -99)    MsgLevel = StdLevel;
  return MsgCode;
}

/*=============================================================== MsgDateVal
*/

static void trim( char *s )
  {
  char *p, *q;
  for (q=s; (*q<=' '); q++)  if (!*q)  break;
  for (p=s; (*q); )  *p++ = *q++;
  *p = 0;
  for (p--; p>=s; p--)
      if (*p <= ' ')  *p = 0;
      else  break;
  }

double MsgDateVal(      /* converts date string into numerical value    */
char * ss )             /* string of the form yy-mm-dd.hh:mm:ss         */
  {                     /* RETURN: days after 1899-12-30'00:00:00 +1.e6 */
  static int dayspassed[] = {
        0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334, 365 };
  char s[80], *pc, *pd, *pt;
  int year, month, day, days;
  int hour, minute, second, n;
  double mydate;
  strcpy(s, ss);  trim(s);
  if (!strcmp(s, "-inf"))  return -9999999;
  if (!strcmp(s, "+inf"))  return 9999999;
  pc = strchr(s, ' ');
  if (!pc)  pc = strchr(s, '\'');
  if (!pc)  pc = strchr(s, '.');
  if (!pc)  pc = strchr(s, 'T');                  //-2014-01-30
  if (!pc)  pc = strchr(s, ' ');                  //-2002-02-14
  if (pc) {
      *pc = 0;
      pt = pc+1;
      pd = s; }
  else {
      if (strchr(s, '-')) {
          pd = s;
          pt = ""; }
      else {
          pd = "";
          pt = s; }
      }
  pc = strchr(pd, '-');
  mydate = 0;
  if (pc) {
      *pc++ = 0;
      year = atoi(pd);
      pd = pc;
      pc = strchr(pd, '-');
      if (pc) {
          *pc++ = 0;
          month = atoi(pd);
          day = atoi(pc); }
      else {
          month = year;
          year = 0;
          day = atoi(pd); }
      if (year > 1000)  year = year - 1900;
      if (year < 0)  year = 0;
      if (month < 0)  month = 0;
      if (month > 12)  month = 12;
      days = 0;
      if (month > 0) {
          days += 1 + dayspassed[month-1];
          if (year > 0)  days += 365*year + (year-1)/4;
          if ((year>0) && (year%4==0) && (month>2))  days++; }
      days += day;
      mydate = 1000000 + days;
      }
  else  sscanf(pd, "%lf", &mydate);
  n = sscanf(pt, "%d:%d:%d", &hour, &minute, &second);
  if (n < 3) { second=minute; minute=hour; hour=0; }
  if (n < 2) { second=minute; minute=0; }
  if (n < 1) { second=0; }
  mydate += (second + 60*(minute + 60*hour))/(24.0*3600.0);
  MsgDateHour = hour;
  MsgDateSecond = second;
  MsgDateMinute = minute;
  return mydate;
  }

char *MsgDateString(    /* converts date value into string      */
double d )              /* days after 1899-12-30'00:00:00 +1.e6 */
  {                     /* RETURN: string yy-mm-dd'hh:mm:ss     */
  static char dstr[80];
  static int dayspassed[] = {
        0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334, 365 };
  int seconds, second, minute, hour;
  int days, day, month, year;
  int day1, day2, add;
  char sep = '.';
  if (d < -1.e-5)  return "-inf";                   //-2002-07-03
  if (d < 0)  d = 0;                                //-2002-07-03
  if (d >= 9999999)  return "+inf";
  days = floor(d);
  seconds = (d - days)*24*3600 + 0.5;
  second = seconds % 60;
  minute = (seconds/60) % 60;
  hour = seconds/3600;
  MsgDateHour = hour;
  MsgDateMinute = minute;
  MsgDateSecond = second;
  if (days == 0) {
      sprintf(dstr, "%02d:%02d:%02d", hour, minute, second);
      return dstr; }
  if (days < 1000002) {
      sprintf(dstr, "%d%c%02d:%02d:%02d", days, sep, hour, minute, second);
      return dstr; }
  days -= 1000000;
  day1 = 2;
  for (year=0; year<200; year++) {
      day2 = day1 + 365;
      if ((year) && (year%4==0)) day2++;
      if (days < day2)  break;
      day1 = day2; }
  days -= day1;
  add = ((year) && (year%4==0)) ? 1 : 0;
  for (month=1; month<=12; month++)
      if (days < dayspassed[month]+add*(month>1))  break;
  day = 1 + days - (dayspassed[month-1]+add*(month>2));
  /* if (year >= 100) */  year += 1900;                         //-2001-04-26 lj
  sprintf(dstr, "%02d-%02d-%02d%c%02d:%02d:%02d",
    year, month, day, sep, hour, minute, second);
  MsgDateYear = year;
  MsgDateMonth = month;
  MsgDateDay = day;
  return dstr;
  }

char *MsgDate( void )   // returns the current date and time
{
  static char buffer[80];
  time_t time_of_day = time(NULL);
  strftime(buffer, 79, "%Y-%m-%d %H:%M:%S", localtime(&time_of_day));
  return buffer;
}


double MsgDateSeconds( double t1, double t2 )
  {
  return (t2-t1)*24*3600;
  }

double MsgDateIncrease(   // increase date t by n seconds
double t,
int n )
{
  t += n/(24.0*3600.0);
  return t;
}

int MsgCheckPath(     // normalize path expression
char *s )             // path
{                     // RETURN: is absolute path
  char *pc, *p0;
  if (!s || !*s)  return 0;
  for (pc=s; (*pc); pc++)  if (*pc == MSG_ALTPSEP)  *pc = MSG_PATHSEP;
  if (pc[-1] == MSG_PATHSEP)  pc[-1] = 0;
  if (*s == MSG_PATHSEP)  return 1;
  if (!strchr(s, MSG_PATHSEP) && *s != '~') {   //-2004-08-20
    p0 = strrchr(s, ':');                       // insert current directory "./"
    if (p0)  p0++;
    else     p0 = s;
    for (pc=s+strlen(s); (pc>=p0); pc--)  pc[2] = pc[0];
    p0[0] = '.';
    p0[1] = MSG_PATHSEP;
  }
  if (strchr(s, ':'))  return 1;
  return 0;
}

int MsgReadOption(    // read an option
char *s)              // option string of type "name=value"
{                     // RETURN: option name recognized
  int l=0, n=0;
  char *t = NULL;
  if (s == NULL)  return 0;
  t = "MsgMaxAlloc=";
  l = strlen(t);
  if (!strncmp(s, t, l)) {
    l = sscanf(s+l, "%d", &n);
    if (l > 0)  InitAlloc(n);
  }
  else return 0;
  return 1;
}


#ifdef  MAIN  /*#####################################################*/

int main()
  {
  char s[80], t[80];
  double d;
  for (;;) {
      fputs(":", stdout);  gets(s);
      if (!*s) break;
      trim(s);
      d = MsgDateVal(s);
      strcpy(t, MsgDateString(d));
      printf("%15.5lf  >%s<\n", d, t);
  }
  return 0;
  }
#endif
/*==========================================================================

 history:

 2001-04-27 uj 1.2.1  update with lj-version
 2001-04-28 lj 1.2.2  update
 2001-04-28 lj 1.2.3  time format (separator)
 2001-08-10 lj 1.2.4  display in vMsg()
 2001-08-15 lj 1.2.5  _MsgTokens preserves source string
 2001-09-03 lj 1.2.6  MsgUnquote()
 2001-09-07 lj 1.2.7  synchronized
 2006-02-08 lj 1.3.0  MsgAlloc
 2006-03-13 lj 1.3.3  MsgReadOption, dynamic enlargement of allocation tables
 2006-10-26 lj 2.0.0  external strings
 2008-10-17 lj 2.1.0  MsgSetLocale()
 2008-10-20 lj 2.1.1  MsgLocale, vMsg() uses MsgLocale
 2014-01-30 uj 2.1.2  allow 'T' as date/time separator

===========================================================================*/
