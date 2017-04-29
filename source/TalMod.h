//=================================================================== TalMod.h
//
// Handling of model fields
// ========================
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
// last change: 2002-09-24 lj
//
//==========================================================================

#ifndef  TALMOD_INCLUDE
#define  TALMOD_INCLUDE

typedef struct {
   float za;
   float vx, vy, vz;
   float tau, ths, eww;
   } MODREC;

typedef struct {
  float za, vx, vy, vz, tau, ths, eww, wx, wy, wz, evv, pvv, pww, lvv, lww;
  } MD2REC;

typedef struct {
  float za, vx, vy, vz, tau, ths, ezz, wx, wy, wz;
  float pxx, pxy, pxz, pyx, pyy, pyz, pzx, pzy, pzz;
  float lxx, lyy, lzz, lyx, lzx, lzy;
  float exx, eyx, eyy, ezx, ezy;
  } MD3REC;

long ModInit(     /* initialize server  */
  long flags,   /* action flags   */
  char *istr )    /* server options */
  ;
long ModServer(         /* server routine for MOD */
  char *t )   /* calling option   */
  ;
/*==========================================================================*/
#endif
