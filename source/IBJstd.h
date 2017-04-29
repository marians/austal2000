// =================================================================== IBJstd.h
//
// Header for standard IBJ-programs
// ================================
//
// Copyright (C) Umweltbundesamt, Dessau-Roﬂlau, Germany, 2002-2008
// Copyright (C) Janicke Consulting, 88662 ‹berlingen, Germany, 2002-2008
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
// last change: 2008-12-07 lj
//
//==========================================================================

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <limits.h>
#include <time.h>
#include <math.h>
#include <ctype.h>

#ifndef   IBJMSG_INCLUDE
  #include "IBJmsg.h"
#endif
#ifndef   IBJSTAMP_INCLUDE
  #include "IBJstamp.h"
#endif

#ifndef  STDLOGLEVEL
  #define  STDLOGLEVEL  3
#endif
#ifndef  STDDSPLEVEL
  #define  STDDSPLEVEL  2
#endif
#ifndef  STDMYMAIN
  #define  STDMYMAIN  MyMain
#endif
#ifndef  STDLOGOPEN
  #define  STDLOGOPEN  "w"
#endif
#ifndef  STDTXTSTART
  #define  STDTXTSTART  "---------------------------------------------------------"
#endif
#ifndef  STDTXTEND
  #define  STDTXTEND    "========================================================="
#endif

#ifdef STDNLS
  #include "IBJstd.nls"
  #include "IBJnls.h"
#else
  #define _ibjstd_no_log_      "can't write into file %s!"
  #define _ibjstd_no_arg_file_ "argument file %s not found!"
  #define _ibjstd_error_       "error during execution of option %s!"
#endif

#define  STD_SILENT    0x00010000L
#define  STD_VERBOSE   0x00020000L
#define  STD_NOLOG     0x00040000L
#define  STD_PRNDEF    0x00080000L
#define  STD_CHECK     0x00100000L
#define  STD_TIME      0x00200000L
#define  STD_WAIT      0x00400000L
#define  STD_DIALOG    0x00800000L
#define  STD_INIT      0x10000000L
#define  STD_ANYARG    0x20000000L
#define  STD_GLOBAL    0x0f050000L

#define  STDNAMELEN    256

#define  STDDIALOG  ((StdStatus & STD_DIALOG) != 0)
#define  STDSILENT  ((StdStatus & STD_SILENT) != 0)
#define  STDNOARG   ((StdStatus & STD_ANYARG) == 0)

#define  vDSP(a)  if (StdDspLevel>=(a) && !STDDIALOG) MsgQuiet=0, MsgVerbose=0, vMsg
#define  vPRN(a)  if (StdDspLevel >= (a))   Printf
#define  vLOG(a)  MsgQuiet=2*(StdDspLevel<(a))-1, MsgVerbose=(StdLogLevel>=(a)), vMsg

#define  STDSCD(a,b,c)  {int n,v; n=sscanf(a,b,&v); if (n==1) c=v;}

extern char *IbjStdMsg(int i);                                    //-2008-12-11
extern void IbjStdReadNls(int argc, char* argv[], char *main, char *mod, char *version);

static long StdStatus, StdIdent, StdTime, StdCheck;
static char StdPath[STDNAMELEN], StdLogName[STDNAMELEN], StdReply[STDNAMELEN];
static int StdLogLevel = STDLOGLEVEL;
static int StdDspLevel = STDDSPLEVEL;
static char StdLogOpen[32] = STDLOGOPEN;


#ifdef   MAIN
static long STDMYMAIN(void);
static char StdMyName[STDNAMELEN];
static char StdMyProg[STDNAMELEN];
static char StdMyVersion[STDNAMELEN];

#ifndef STDARG
  #define  STDARG 3
#endif
#ifndef STDHDR
  #define  STDHDR 1
#endif

#ifdef STDNLS
  #define STDREADNLS(main, mod, version)                                \
    IbjStdReadNls(argc, argv, main, mod, version);
#else
  #define STDREADNLS(main, mod, version)
#endif

#define  STDMAIN(StdPgm,StdSub,STD_VERSION,STD_RELEASE,STD_PATCH)       \
                                                                        \
static void StdExit( void )                                             \
  {                                                                     \
  if (StdStatus) { vMsg("@%s\n", STDTXTEND); }                          \
  }                                                                     \
                                                                        \
int main( int argc, char *argv[] )                                      \
  {                                                                     \
  dP(StdPgm);                                                           \
  int i, n;                                                             \
  char name[256], argline[4096], *pc;                                   \
  FILE *argfile;                                                        \
  strcpy(StdMyProg, #StdPgm);                                           \
  atexit(StdExit);                                                      \
  StdArgDisplay = STDARG;                                               \
  strcpy(StdMyName, argv[0]);                                           \
  MsgCheckPath(StdMyName);                                              \
  strcpy(StdMyVersion, #STD_VERSION "." #STD_RELEASE "." #STD_PATCH);   \
  STDREADNLS(StdMyName, STDNLS, StdMyVersion);                          \
  for (i=1; i<argc; i++) {                                              \
    if (!strncmp(argv[i], "--language=", 11))  continue;                \
    StdStatus |= STD_ANYARG;                                            \
    if (argv[i][0] == '@') {                                            \
      strcpy( name, StdPath );                                          \
      if (argv[i][1] == 0)  strcat( name, #StdPgm ".arg" );             \
      else  strcat( name, argv[i]+1 );                                  \
      if (argv[i][1] == '-')  argfile = stdin;                          \
      else  argfile = fopen( name, "rb" );  if (!argfile)       eX(1);  \
      while (fgets( argline, 4096, argfile)) {                          \
        n = strlen( argline );                                          \
        if (n == 0)  continue;                                          \
        if (*argline <= ' ')  continue;                                 \
        pc = strchr(argline, '\'');                                     \
        if (pc)  *pc = 0;                                               \
        n = strlen( argline );                                          \
        for (n--; n>0; n--)                                             \
          if (argline[n] <= ' ')  argline[n] = 0;                       \
          else  break;                                                  \
        if (strlen(argline)==0)  continue;                              \
        StdStatus |= STD_ANYARG;                                        \
        StdSub(argline);                                        eG(2);  \
        }                                                               \
      if (argfile != stdin)  fclose(argfile);                           \
      }                                                                 \
    else {                                                              \
      strcpy(argline, argv[i]);                                         \
      StdSub(argline);                                          eG(3);  \
      }                                                                 \
    }                                                                   \
  MsgLogLevel = StdLogLevel;  MsgSetVerbose(StdLogLevel>0);             \
  MsgDspLevel = StdDspLevel;  MsgSetQuiet(StdDspLevel<=0);              \
  STDMYMAIN();                                                  eG(4);  \
  return 0;                                                             \
eX_1:                                                                   \
  eMSG(_ibjstd_no_arg_file_, name);                                     \
eX_2:  eX_3:                                                            \
  eMSG(_ibjstd_error_, argline);                                        \
eX_4:                                                                   \
  return MsgCode;                                                       \
  }
#else
  #define  STDMAIN(StdPgm,StdSub,STD_VERSION,STD_RELEASE,STD_PATCH)
  #define  STDREADNLS(StdMyMain, STDNLS, lan, STDVERSION)
  #ifndef STDARG
    #define  STDARG  4
  #endif
  #ifndef STDHDR
    #define  STDHDR  5
  #endif
#endif

#define  STDPGM(StdPgm,StdSub,STD_VERSION,STD_RELEASE,STD_PATCH)        \
                                                                        \
long StdSub(char *);                                                    \
static int StdVersion=STD_VERSION;                                      \
static int StdRelease=STD_RELEASE;                                      \
static char StdPatch[]=#STD_PATCH;                                      \
static int StdArgDisplay=STDARG;                                        \
                                                                        \
static int StdArg( char *s )                                            \
  {                                                                     \
  int l, k, quiet;                                                      \
  char *t, name[256];                                                   \
  double tm;                                                            \
  k = 1;                                                                \
  if (*s == '-') {                                                      \
    t = s+1;                                                            \
    switch ( *t ) {                                                     \
      case 'C':  StdCheck = 1;                                          \
                 STDSCD(t+1, "%x", StdCheck);                           \
                 if (StdCheck)  StdStatus |= STD_CHECK;                 \
                 break;                                                 \
      case 'D':  strcpy(StdLogOpen, "w");                               \
                 return k;                                              \
      case 'G':  l = 0;                                                 \
                 sscanf(t+1, "%x", &l);                                 \
                 StdStatus |= STD_GLOBAL & (l<<24);                     \
                 break;                                                 \
      case 'I':  StdStatus |= STD_DIALOG;                               \
                 break;                                                 \
      case 'L':  strcpy( StdLogName, t+1 );                             \
                 if (*StdLogName == '-') {                              \
                   StdStatus |= STD_NOLOG;                              \
                   *StdLogName = 0; }                                   \
                 else {                                                 \
                   StdStatus &= ~STD_NOLOG;                             \
                   if (StdLogLevel <= 0) {                              \
                     StdStatus |= STD_VERBOSE;  StdLogLevel = 1; }      \
                   }                                                    \
                 break;                                                 \
      case 'N':  sscanf(t+1, "%08lx", &StdIdent);                       \
                 break;                                                 \
      case 'q':  quiet = 9;                                             \
                 STDSCD(t+1, "%d", quiet);                              \
                 StdDspLevel = 9 - quiet;                               \
                 if (StdDspLevel < 1)  StdStatus |= STD_SILENT;         \
                 return k;                                              \
      case 'T':  StdStatus |= STD_TIME;                                 \
                 tm = floor(0.5 + MsgDateSeconds(0, MsgDateVal(t+1)));  \
                 StdTime = (tm<LONG_MIN)?LONG_MIN : ((tm>LONG_MAX)?LONG_MAX:tm);  \
                 break;                                                 \
      case 'v':  StdLogLevel = 1;                                       \
                 STDSCD(t+1, "%d", StdLogLevel);                        \
                 if (StdLogLevel > 0)  StdStatus |= STD_VERBOSE;        \
                 break;                                                 \
      case 'W':  StdStatus |= STD_WAIT;                                 \
                 break;                                                 \
      case 'y':  StdDspLevel = 1;                                       \
                 STDSCD(t+1, "%d", StdDspLevel);                        \
                 if (StdDspLevel < 1)  StdStatus |= STD_SILENT;         \
                 return k;                                              \
      case '-':  strcpy(StdReply, t);                                   \
                 if (!*t)  break;                                       \
                 if (MsgReadOption(t+1));                               \
                 break;                                                 \
      case '0':  StdStatus &= STD_NOLOG;                                \
                 StdIdent = 0;                                          \
                 StdTime = 0;                                           \
                 StdDspLevel = STDDSPLEVEL;                             \
                 StdLogLevel = STDLOGLEVEL;                             \
                 break;                                                 \
      default:   k = 0;                                                 \
      }                                                                 \
    }                                                                   \
  else {                                                                \
    l = strlen(s);                                                      \
    if (l) {                                                            \
      strcpy(StdPath, s);                                               \
      MsgCheckPath(StdPath);                                            \
      }                                                                 \
    else k = 0;                                                         \
    }                                                                   \
  if (StdStatus & STD_NOLOG) {                                          \
    MsgFile = NULL;                                                     \
    }                                                                   \
  else if (!MsgFile) {                                                  \
    if (!*StdLogName) {                                                 \
      strcpy(StdLogName, #StdPgm ".log");                               \
      t = StdLogName; while (*t) { *t = tolower(*t); t++; }             \
    }                                                                   \
    *name = 0;                                                          \
    if (MsgCheckPath(StdLogName))  strcpy(name, StdLogName);            \
    else  sprintf(name, "%s/%s", StdPath, StdLogName);                  \
    MsgFile = fopen(name, StdLogOpen);                                  \
    if (!MsgFile) { vMsg(_ibjstd_no_log_, name); exit(1); }             \
    }                                                                   \
  if (*StdLogName) {                                                    \
    MsgLogLevel = StdLogLevel;                                          \
    MsgDspLevel = StdDspLevel;                                          \
    lMSG(STDHDR+1,STDHDR+1)(#StdSub "_%d.%d.%s %s",                     \
      StdVersion,StdRelease,StdPatch,IBJstamp(__DATE__,__TIME__));      \
    lMSG(STDHDR,STDHDR+1)("@%s", STDTXTSTART);                          \
    *StdLogName = 0;                                                    \
  }                                                                     \
  lMSG(StdArgDisplay,9)(#StdSub ":%s", s);                              \
  if (!*s)  *StdLogName = 0;                                            \
  return k;                                                             \
  }                                                                     \
STDMAIN(StdPgm,StdSub,STD_VERSION,STD_RELEASE,STD_PATCH)
