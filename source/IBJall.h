//======================================================== IBJmsg.h
//
// Allocation Routines for IBJ-programs
// ====================================
//
// Copyright (C) Umweltbundesamt, Dessau-Roﬂlau, Germany, 2006
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
// last change: 2006-02-15  lj/ibj
//
//==================================================================

#ifndef IBJALL_INCLUDE
#define IBJALL_INCLUDE

#include <stdio.h>
#include <stdlib.h>

#ifdef MSGALLOC
  // Requires the character strings
  // eMODn : module name
  // ePGMn : program name
  #define ALLOC(a)        MsgAlloc((a), eMODn, ePGMn, __LINE__)
  #define CALLOC(a,b)     MsgCalloc((a),(b), eMODn, ePGMn, __LINE__)
  #define FREE(a)         MsgFree((a), eMODn, ePGMn, __LINE__)
  #define CHKALLOC(a)     MsgCheckAlloc((a), eMODn, ePGMn, __LINE__)
  #define REALLOC(a,b)    MsgRealloc((a),(b), eMODn, ePGMn, __LINE__)
#else
  #define ALLOC(a)        calloc(1,(a))
  #define CALLOC(a,b)     calloc((a),(b))
  #define FREE(a)         free(a)
  #define CHKALLOC(a)
  #define REALLOC(a,b)    realloc((a),(b))
#endif

void* MsgAlloc(int size, char *mod, char *name, int line);
void* MsgCalloc(int n, int size, char *mod, char *name, int line);
void  MsgFree(void *p, char *mod, char *name, int line);
void* MsgRealloc(void *p, int size, char *mod, char *name, int line);
long long MsgGetNumBytes(void);
long long MsgGetMaxBytes(void);
int MsgGetNumAllocs(void);
int MsgGetMaxAllocs(void);
int MsgListAlloc(FILE *prn);
void MsgCheckAlloc(void *p, char *mod, char *name, int line);

#endif // IBJALL_INCLUDE
