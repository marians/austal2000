// ======================================================== TalWnd.h
//
// Diagnostic wind field model for AUSTAL2000
// ==========================================
//
// Copyright (C) Umweltbundesamt, Dessau-Roﬂlau, Germany, 2002-2004
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
// last change:  2002-06-21  lj
//
// ==================================================================

#ifndef   TALWND_INCLUDE
#define   TALWND_INCLUDE

#define  WND_BASIC      5 // "f" initial field without Prandtl layer
#define  WND_TERRAIN    4 // "e" adjusted to terrain & divergence removed
#define  WND_WAKE       3 // "d" dummy (to keep "d" in file names)
#define  WND_SURFACE    2 // "c" Prandtl layer inserted 
#define  WND_BUILDING   1 // "b" divergence removed (ready for buildings)
#define  WND_FINAL      0 // "a" buildings inserted and divergence removed

#define  WND_BASE1      6
#define  WND_BASE2      7

#define  WND_VXSET   0x01000000
#define  WND_VYSET   0x02000000
#define  WND_VZSET   0x04000000
#define  WND_VASET   0x07000000
#define  WND_VSSET   0x08000000
#define  WND_VXEQ0   0x10000000
#define  WND_VYEQ0   0x20000000
#define  WND_VZEQ0   0x40000000
#define  WND_VAEQ0   0x70000000
#define  WND_VSEQ0   0x80000000

#define  WND_WRTWND    0x0002
#define  WND_SETUNDEF  0x0004
#define  WND_WRTLMD    0x0008
#define  WND_SLIPFLOW  0x0010
#define  WND_SETBND    0x0020
#define  WND_SEARCH    0x0040

#define  WND_VOLOUT   -99.0

typedef struct { float z, vx, vy, vz; } WNDREC;

typedef struct { float su, sv, sw, th; } TRBREC;

typedef struct { float kh, kv; } DFFREC;

int TwnWriteDef( void )
;
long TwnServer( char *s )
  ;
long TwnInit(           /* initialize server    */
long flags,             /* action flags         */
char *istr )            /* server options       */
  ;
/*===========================================================================*/
#endif
