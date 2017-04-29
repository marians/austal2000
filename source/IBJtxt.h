//======================================================== IBJtxt.h
//
// Dynamically allocated strings for IBJ-programs
// ==============================================
//
// Copyright (C) Umweltbundesamt, Dessau-Roﬂlau, Germany, 2002
// Copyright (C) Janicke Consulting, 88662 ‹berlingen, Germany, 2002-2005
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
// history:
//
// last change: 2011-06-29 uj
// 2000-07-27 uj  extended for IBJodor
// 2011-06-29 uj  additional routines TxtPrintf, TxtVPrintf, TxtForm
//
//==================================================================

#ifndef  IBJTXT_INCLUDE
#define  IBJTXT_INCLUDE

#include <stdarg.h>

extern char *IBJtxtVersion;

typedef struct txtstr {
        char *s;
        int l;
        } TXTSTR;


char *TxtUnquote( // unquote a string if necessary
char* s )         // the string
  ; 

char *TxtQuote( // quote a string if necessary
char* s )       // the string
  ;
char *TxtAlloc( // allocate string space
int n )         // number of bytes
  ;
int TxtCat(   // concatenate string to text
TXTSTR *pa,   // text (first part)
char *b )     // string (second part)
  ;
int TxtNCat(  // concatenate at most n bytes of string to text
TXTSTR *pa,   // text (first part)
char *b,      // string (second part)
int n )       // number of bytes to add
  ;
char *TxtForm(          // ASCII transformation of a number
double x,               // number to be transformed
int n)                  // significant places
  ;
char *TxtPrn(           // printed pattern of a number
double x,               // number to be transformed
char *frm)              // print format
  ;
int TxtCpy(   // copy string to text
TXTSTR *pa,   // text (set)
char *b )   // string (copied)
  ;
int TxtNCpy(  // copy at most n bytes of string to text
TXTSTR *pa,   // text (set)
char *b,    // string (copied)
int n )     // number of bytes
  ;
int TxtClr(   // clear text space
TXTSTR *pa )  // text
  ;
int TxtPCpy(  // copy string to text at position pos
TXTSTR *pa,   // text (set)
int pos,    // position within text
char *b )   // string to be copied
  ;
int TxtPIns(  // insert string in text at position pos
TXTSTR *pa,   // text (set)
int pos,    // position within text
char *b )   // string to be inserted
  ;
int TxtPNDel(   // delete n bytes at position pos
TXTSTR *pa,   // text
int pos,    // position within text
int n )     // number of bytes to delete
  ;
int TxtRpl(   // replace string within text
TXTSTR *pa,   // text
char *old,    // string pattern to be replaced
char *new )   // replacing string
  ; 
int TxtPrintf(  // printf() into a TXTSTR buffer
TXTSTR *pa,     // pointer to the buffer
char *format,   // print format
...)            // parameters
;
int TxtVPrintf( // printf() into a TXTSTR buffer
TXTSTR *pa,     // pointer to the buffer
char *format,   // print format
va_list argptr) // parameters
;


#endif
