// ======================================================== TalPrf.h
//
// Generate boundary layer profile for AUSTAL2000
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
// last change:  2002-06-21  lj
//
//========================================================================

#ifndef  TALPRF_INCLUDE
#define  TALPRF_INCLUDE

typedef struct {
        float h, vx, vy, vz, kv, kw;
        }  PR1REC;

typedef struct {
        float h, vx, vy, vz, kv, kw, sv, sw, ts;
        }  PR2REC;

typedef struct {
        float h, vx, vy, vz, kv, kw, sv, sw, ts, ku, kc, su, sc;
        }  PR3REC;
        
long PrfInit(           /* initialize server    */
  long flags,           /* action flags         */
  char *istr )          /* server options       */
  ;
long PrfServer(         /* server routine for PRF       */
  char *t )             /* calling option               */
  ;
/*==========================================================================*/
#endif
