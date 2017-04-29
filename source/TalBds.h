// ================================================================= TalBds.h
//
// Handling of bodies
// ==================
//
// Copyright (C) Umweltbundesamt, Dessau-Roﬂlau, Germany, 2004
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
// last change: 2004-09-20 uj
//
//==========================================================================

#ifndef  TALBDS_INCLUDE
#define  TALBDS_INCLUDE

#ifndef   IBJARY_INCLUDE
  #include "IBJary.h"
#endif

#ifndef   TALMAT_INCLUDE
#include "TalMat.h"
#endif

enum BDSTYPE { BDSNOB, BDSBOX, BDSPOLY0, BDSPOLY1, BDSMX };

typedef struct bldrec {
  char name[64];
  unsigned char flags;
  short int np, btype;
  float x0, x1, y0, y1, h0, h1;
  float xb, yb, hb, ab, bb, cb, wb;
  } BDSBXREC;

typedef struct mxrec {
  char name[64];         
  unsigned char flags;
  short int np, btype;
  char fullname[256];      
  int nx, ny;
  float x0, y0, dd, dz;
  int nzstart;
  } BDSMXREC;

typedef struct twrrec {
  char name[64];
  unsigned char flags;
  short int np, btype;
  float x0, x1, y0, y1, h0, h1;
  float xt, yt, ht, dt, ct, qt;
  } BDSTWREC;

typedef struct bdsxyrec { float x, y; } BDSXYREC;

typedef struct bdsp0rec {
  char name[64];
  unsigned char flags;    
  short int np, btype;
  float x0, x1, y0, y1, h0, h1;
  float hb, cb;
  BDSXYREC p[2];
  float qb;
  } BDSP0REC;

typedef struct bdsp1rec {
  char name[64];
  unsigned char flags;
  short int np, btype;
  BDSXYREC p[6];
  float v;
  } BDSP1REC;
                                                
typedef struct rctrec {
  int type;
  short int kn, gn;
  VECXYZ pm, p11, p12, p21, p22, rn, rx, ry, s;
  float d, f, c, g, b[2];
  } RCTREC;

extern float BdsDMKp[];   
extern int BdsTrbExt;
extern char BdsMatrix[];
      
long BdsServer( char * );
long BdsInit( long, char * );

/*=========================================================================*/
#endif
