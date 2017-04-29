//======================================================== IBJmsg.h
//
// Message Routines for IBJ-programs
// =================================
//
// Copyright (C) Umweltbundesamt, Dessau-Roﬂlau, Germany, 2006
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
// last change: 2008-10-20  lj/ibj
//
//==================================================================

#ifndef  IBJMSG_INCLUDE
#define  IBJMSG_INCLUDE

#include <stdio.h>
#include <stdlib.h>

#include "IBJall.h"

#define  SYMOPT   "-"
#define  MSG_PATHSEP  '/'
#define  MSG_ALTPSEP  '\\'
#define  MSG_MAXFNLEN 256
#define  MSG_MAXNMLEN 256
#define  EOL      "\n"

#ifdef _M_IX86
  #define MSG_I64DFORM "%I64d"
#elif defined __WIN32__
  #define MSG_I64DFORM "%I64d"
#elif defined __linux__
  #define MSG_I64DFORM "%lld"
#endif

#ifndef dP
  #define  dP(a)   char *ePGMn = #a; static char ePGMs[MSG_MAXNMLEN]
  #define  dQ(a)   char *ePGMn = #a
  #define  eG(a)   if(MsgCode<0) eX(a)
  #define  eX(a)   {sprintf(ePGMs,"%s.%s.%s",eMODn,ePGMn,#a); MsgCode=-a; goto eX_ ## a;}
#endif

#ifndef eMSG
  #define  eMSG      return MsgSource=ePGMs, vMsg
  #define  rMSG      return vMsg
  #define  nMSG      MsgSource=ePGMs, vMsg
  #define  lMSG(a,b) MsgVerbose=(MsgLogLevel>=(a)), MsgQuiet=(MsgDspLevel<(b)), vMsg
#endif

extern char *IBJmsgVersion;
extern int MsgCode, MsgBufLen, MsgBreak, MsgVerbose, MsgQuiet;
extern int MsgLevel, MsgLogLevel,MsgDspLevel;
extern int MsgTranslate, MsgDateYear, MsgDateMonth, MsgDateDay;
extern int MsgDateHour, MsgDateMinute, MsgDateSecond;
extern int MsgTranslate;
extern FILE *MsgFile;
extern void *MsgPvd;
extern char *MsgSource;
extern char MsgLocale[];                                          //-2008-10-20

void MsgLow(    /* convert to lower case        */
char *s )       /* string to be converted       */
  ;
char *MsgTok(   /* non-destructive tokenisation */
char *start,    /* begin of scanning            */
char **pnext,   /* start of next scan           */
char *sep,      /* separator characters         */
int *pl )       /* number of usable characters  */
  ;             /* RETURN: start of token       */
char **_MsgTokens(  // tokenize a string
char *t,            // string to be tokenized (changed by this routine!)
char *s )           // separator characters
;                   // RETURN: allocated array of strings, ends with NULL
int MsgDequote( //  remove quotation marks and escape sequences
char *s )       //  string to work on
;               //  RETURN: number of replacements
int MsgUnquote( //  remove quotation marks
char *s )       //  string to work on
;               //  RETURN: number of replacements

int MsgSetVerbose( int new_value )  // set standard value of MsgVerbose
;
int MsgSetQuiet( int new_value )    // set standard value of MsgQuiet
;
int MsgSetLevel( int new_value )    // set standard value of MsgLevel
;
int MsgSetLocale(char *l)                                         //-2008-10-17
;
int vMsg(       /* print message on screen and into logfile     */
char *format,   /* printing format                              */
... )           /* parameter                                    */
  ;
double MsgDateVal(      /* converts date string into numerical value    */
char * ss )             /* string of the form yy-mm-dd'hh:mm:ss         */
  ;                     /* RETURN: days after 1899-12-30'00:00:00 +1.e6 */

char *MsgDateString(    /* converts date value into string      */
double d )              /* days after 1899-12-30'00:00:00 +1.e6 */
  ;                     /* RETURN: string yy-mm-dd'hh:mm:ss     */

char *MsgDate( void )   // returns the current date and time
;
double MsgDateSeconds(  /* converts time from t1 to t2 into seconds */
double t1,
double t2 )
  ;
double MsgDateIncrease(   // increase date t by n seconds
double t,
int n )
;
int MsgCheckPath(     // normalize path expression
char *s )             // path
;                     // RETURN: is absolute path

int MsgReadOption(    // read an option
char *s)              // option string of type "name=value"
;                     // RETURN: option name recognized

#endif  /*##################################################################*/
