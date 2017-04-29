// ============================================================= TalDMKutil.h
//
// Utility functions and classes for DMK
// =====================================
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
// last change: 2004-12-13 uj
//
//==========================================================================

#define BDY_RECTANGLE 1
#define BDY_CIRCLE    2
#define BDY_POLYGON   3
#define BDY_MINPINCELL 6
#define I4PI          0.079577471

#define BND_PNTBND   0x0001
#define BND_PNTOUT   0x0002
#define BND_PNTMSK   0x0003
#define BND_PNTSHIFT 0   
#define BND_ARXBND   0x0010  // boundary of computational volume
#define BND_ARXOUT   0x0020  // outside of computational volume
#define BND_ARXSET   0x0040  // boundary value set on this face
#define BND_ARXPOS   0x0080  // comput. volume to the right of this face
#define BND_ARXMSK   0x00f0
#define BND_ARXSHIFT 4
#define BND_ARYBND   0x0100
#define BND_ARYOUT   0x0200
#define BND_ARYSET   0x0400
#define BND_ARYPOS   0x0800
#define BND_ARYMSK   0x0f00
#define BND_ARYSHIFT 8
#define BND_ARZBND   0x1000
#define BND_ARZOUT   0x2000
#define BND_ARZSET   0x4000
#define BND_ARZPOS   0x8000
#define BND_ARZMSK   0xf000
#define BND_ARZSHIFT 12
#define BND_VOLBND   0x10000
#define BND_VOLOUT   0x20000
#define BND_VOLMSK   0x30000
#define BND_VOLSHIFT 16
#define BND_BOUNDARY 1
#define BND_OUTSIDE  2
#define BND_FIXED    4
#define BND_POSITIVE 8


typedef struct {
  float x;
  float y;
  float z;
} VEC3D;

typedef struct {
  float  xx, xy, xz, yx, yy, yz, zx, zy, zz;
} MAT3D;

typedef struct {
  float ax, ay, az;
  int i, j, k;
} POSITION;

typedef struct {
  VEC3D a, b;
  float charge;
} LINE;

typedef struct {
  VEC3D a, b, c, d, e;
  VEC3D v1, v2, v3, m;
  MAT3D rot;
  float length, width, area, granular, charge;
} RECTANGLE;

typedef struct {
  int constantSpacing;
  float xmin;
  float xmax;
  float dx;
  int nx;
  ARYDSC xx; 
} GRID1D;
          
typedef struct {
  GRID1D gridX, gridY;
  int constantSpacing;
  int nx, ny;
  float xmin, xmax, dx, ymin, ymax, dy, zmin, zmax;
  ARYDSC xx, yy, zz;
} GRID2D;

typedef struct {
  GRID2D gridXY;
  GRID1D gridZ;
  int constantSpacing;
  int nx, ny, nz;
  ARYDSC hh; 
  float xmin, xmax, dx, ymin, ymax, dy;
} GRID3D;


typedef struct {
  GRID3D grid;
  int nx, ny, nz, completed;
  ARYDSC bb;
} BOUND3D;

typedef struct {
  BOUND3D bound;
  GRID3D grid;
  GRID2D gridXY;
  GRID1D gridZ;
  int nx, ny, nz;
  float dx, dy, dz, dmin, omega;
  float areaX, areaY, areaZ;
  ARYDSC hh, zp, zg, vx, vy, vz;
} WIND3D;

typedef struct {
  int type, np;
  char *name;
  float floor, top, x0, y0, x1, y1;  //-2004-12-14
  float *xp, *yp;
} BODY;

//--------------------------------------------------------------------------

float Vec3D_abs(VEC3D);
float Vec3D_dot(VEC3D, VEC3D);
VEC3D Vec3D_add(VEC3D, VEC3D);
VEC3D Vec3D_sub(VEC3D, VEC3D);
VEC3D Vec3D_mul(VEC3D, float);
VEC3D Vec3D_div(VEC3D, float);
VEC3D Vec3D_cross(VEC3D, VEC3D);
VEC3D Vec3D_norm(VEC3D);
VEC3D Vec3D(float, float, float);
char *Vec3D_toString(VEC3D, char *);

MAT3D Mat3D(VEC3D, VEC3D, VEC3D);
VEC3D Mat3D_dot(MAT3D, VEC3D);
MAT3D Mat3D_trans(MAT3D);
MAT3D Mat3D_mul(MAT3D, float);

LINE Line(VEC3D, VEC3D);
int Line_field(LINE, VEC3D *, VEC3D);
int Line_base(LINE, VEC3D *, VEC3D);
int Line_base2(VEC3D, VEC3D, VEC3D, VEC3D *);

RECTANGLE Rectangle(VEC3D, VEC3D, VEC3D);
int Rectangle_field(RECTANGLE, VEC3D *, VEC3D);   
int Rectangle_base(RECTANGLE, VEC3D *, VEC3D);
VEC3D Rectangle_transform(RECTANGLE, VEC3D);
float Rectangle_getDistance(RECTANGLE, VEC3D);

int Grid1D(GRID1D *, float, float, int);   
int Grid1Dz(GRID1D *, int, float *);
int Grid1D_getPoint(GRID1D, VEC3D *, int);
int Grid1D_getXx(GRID1D, ARYDSC *);
float Grid1D_getX(GRID1D, int);

int Grid2D(GRID2D *, GRID1D, GRID1D);
int Grid2Dz(GRID2D *, GRID1D, GRID1D, ARYDSC);
int Grid2Dxyz(GRID2D *, ARYDSC, ARYDSC, ARYDSC);
float Grid2D_getX(GRID2D, int, int);
float Grid2D_getY(GRID2D, int, int);
float Grid2D_getZ(GRID2D, int, int);
int Grid2D_getXx(GRID2D, ARYDSC *);      
int Grid2D_getYy(GRID2D, ARYDSC *);
int Grid2D_getZz(GRID2D, ARYDSC *);
int Grid2D_getPoint(GRID2D, VEC3D *, int, int);
int Grid2D_getMidPoint(GRID2D, VEC3D *, int, int);
int Grid2D_isFlat(GRID2D);

int Grid3D(GRID3D *, GRID2D, GRID1D);
float Grid3D_getAreaX(GRID3D, int, int, int);
float Grid3D_getAreaY(GRID3D, int, int, int);
float Grid3D_getAreaZ(GRID3D, int, int, int);
int Grid3D_getPoint(GRID3D, VEC3D *, int, int, int);    
int Grid3D_getMidPoint(GRID3D, VEC3D *, int, int, int);
int Grid3D_getPosition(GRID3D, POSITION *, VEC3D); 
int Grid3D_getIntPosition(GRID3D, int *, int *, int *, VEC3D);
int Grid3D_isOutside(GRID3D, float, float, float);

int Bound3D(BOUND3D *, GRID3D);
int Bound3D_copy(BOUND3D *, BOUND3D);
int Bound3D_complete(BOUND3D *);
int Bound3D_setVolout(BOUND3D *, int, int, int, int);
int Bound3D_getPointType(BOUND3D, int, int, int);
int Bound3D_getVolumeType(BOUND3D, int, int, int);
int Bound3D_getAreaXType(BOUND3D, int, int, int);
int Bound3D_getAreaYType(BOUND3D, int, int, int);
int Bound3D_getAreaZType(BOUND3D, int, int, int);
int Bound3D_setFixedX(BOUND3D *, int, int, int, int); 
int Bound3D_setFixedY(BOUND3D *, int, int, int, int);
int Bound3D_setFixedZ(BOUND3D *, int, int, int, int);
int Bound3D_isInside(BOUND3D, int, int, int);

int Wind3D(WIND3D *, BOUND3D);
int Wind3D_clearBoundaryValues(WIND3D *);
int Wind3D_add(WIND3D *, WIND3D, float);
int Wind3D_divergence(WIND3D, int, int, int, float *);
int Wind3D_maxDivergence(WIND3D, float *);
int Wind3D_printZ(WIND3D, int, FILE *);
int Wind3D_writeDMNA(WIND3D, FILE *, char *);
int Wind3D_getVector(WIND3D, VEC3D *, VEC3D);
VEC3D Wind3D_vector(WIND3D, int, int, int);

void Grid1D_free(GRID1D *);
void Grid2D_free(GRID2D *);
void Grid3D_free(GRID3D *);
void Bound3D_free(BOUND3D *);
void Wind3D_free(WIND3D *);

int Wind3D_init(WIND3D *, float, float, float, float, float, float, float, float);
float GetU(float, float, float, float, float);
float GetRa(float, float, float, float, float, float, float);

int BodyRectangle(BODY *, float, float, float, float, float, float, float);
int BodyCircle(BODY *, float, float, float, float, float);
int BodyPolygon(BODY *, float, float);
int Body_setPoints(BODY *, int, float *, float *);
int Body_isInside(BODY, float, float, float);

int DMKutil_B2BM(ARYDSC *, int, int, int, float, float, float, float *, ARYDSC);
int DMKutil_BM2BM(ARYDSC *, int, int, int, float, float, float, float *,
  ARYDSC, float, float, float, float);

