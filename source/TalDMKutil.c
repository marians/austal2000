// ============================================================= TalDMKutil.c
//
// Utility functions and classes for DMK
// =====================================
//
// Copyright (C) Umweltbundesamt, Dessau-Roﬂlau, Germany, 2004-2005
// Copyright (C) Janicke Consulting, 88662 ‹berlingen, Germany, 2004-2006
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
// last change: 2006-10-26 lj
//
//==========================================================================

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <math.h>

#include "IBJmsg.h"
#include "IBJary.h"
#include "IBJtxt.h"
#include "TalDMKutil.h"
#include "TalDMKutil.nls"
#include "TalWnd.h"
#include "TalMat.h"  //-2004-12-14

static char *eMODn = "TalDMKutil";

//--------------------------------------------------------------------------

#define  X(a,b)       (*(float*)AryPtr(&(a),(b)))
#define  XX(a,b,c)    (*(float*)AryPtr(&(a),(b),(c)))
#define  XXX(a,b,c,d) (*(float*)AryPtr(&(a),(b),(c),(d)))
#define  FFF(a,b,c,d) (*(float*)((char*)(a).start + (a).virof    \
                       + (b)*(a).bound[0].mul +                  \
                       + (c)*(a).bound[1].mul +                  \
                       + (d)*(a).bound[2].mul ))                   
#define  FF(a,b,c)   (*(float*)((char*)(a).start + (a).virof    \
                       + (b)*(a).bound[0].mul +                  \
                       + (c)*(a).bound[1].mul ))                   
#define  F(a,b)       (*(float*)((char*)(a).start+(a).virof      \
                       + (b)*(a).bound[0].mul ))    
#define  BBB(a,b,c,d) (*(int*)AryPtr(&(a),(b),(c),(d)))
#define  III(a,b,c,d) (*(int*)((char*)(a).start + (a).virof    \
                       + (b)*(a).bound[0].mul +                  \
                       + (c)*(a).bound[1].mul +                  \
                       + (d)*(a).bound[2].mul ))                   
#define  G            (*_g)
#define  B            (*_b)
#define  W            (*_w)

static int setBodyBorderValues(BODY *);                     //-2005-01-10
static int setBodyMatrix(ARYDSC *, ARYDSC, GRID3D);         //-2005-01-10

////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////

  
float Vec3D_abs(VEC3D v) {
  return sqrt(v.x*v.x + v.y*v.y + v.z*v.z);
}

float Vec3D_dot(VEC3D a, VEC3D b) {
  return (a.x*b.x + a.y*b.y + a.z*b.z);
} 

VEC3D Vec3D_add(VEC3D a, VEC3D b) {
  VEC3D v;
  v.x = a.x + b.x;
  v.y = a.y + b.y;
  v.z = a.z + b.z;
  return v;
}

VEC3D Vec3D_sub(VEC3D a, VEC3D b) {
  VEC3D v;
  v.x = a.x - b.x;
  v.y = a.y - b.y;
  v.z = a.z - b.z;
  return v;
}

VEC3D Vec3D_mul(VEC3D a, float f) {
  VEC3D v;
  v.x = a.x * f;
  v.y = a.y * f;
  v.z = a.z * f;
  return v;
}

VEC3D Vec3D_div(VEC3D a, float f) {
  VEC3D v;
  v.x = 0;
  v.y = 0;
  v.z = 0;
  if (f != 0) {
    v.x = a.x / f;
    v.y = a.y / f;
    v.z = a.z / f;
  }
  return v;
}


VEC3D Vec3D_norm(VEC3D a) {
  VEC3D v;
  float f;
  v.x = 0;
  v.y = 0;
  v.z = 0;
  f = sqrt(a.x*a.x + a.y*a.y + a.z*a.z);
  if (f != 0) {
    v.x = a.x / f;
    v.y = a.y / f;
    v.z = a.z / f;
  }
  return v;
}

VEC3D Vec3D_cross(VEC3D a, VEC3D b) {
  VEC3D v;
  v.x = a.y*b.z - a.z*b.y;
  v.y = a.z*b.x - a.x*b.z;
  v.z = a.x*b.y - a.y*b.x;
  return v;
}

VEC3D Vec3D(float x, float y, float z) {
  VEC3D v;
  v.x = x;
  v.y = y;
  v.z = z;
  return v;
}

char *Vec3D_toString(VEC3D a, char *s) {
  sprintf(s, "(%5.1f;%5.1f;%5.1f)", a.x, a.y, a.z);
  return s;
}

////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////

MAT3D Mat3D(VEC3D v1, VEC3D v2, VEC3D v3) {
  MAT3D m;
  m.xx = v1.x;  m.xy = v1.y;  m.xz = v1.z;
  m.yx = v2.x;  m.yy = v2.y;  m.yz = v2.z;
  m.zx = v3.x;  m.zy = v3.y;  m.zz = v3.z;
  return m;
}

VEC3D Mat3D_dot(MAT3D m, VEC3D v) {
  VEC3D r;
  r.x = m.xx*v.x + m.xy*v.y + m.xz*v.z;
  r.y = m.yx*v.x + m.yy*v.y + m.yz*v.z;
  r.z = m.zx*v.x + m.zy*v.y + m.zz*v.z;
  return r;
}

MAT3D Mat3D_trans(MAT3D m) {
  float a;
  a = m.xy;  m.xy = m.yx;  m.yx = a;
  a = m.xz;  m.xz = m.zx;  m.zx = a;
  a = m.zy;  m.zy = m.yz;  m.yz = a;
  return m;
}

MAT3D Mat3D_mul(MAT3D m, float f) {
  m.xx *= f;  m.xy *= f;  m.xz *= f;
  m.yx *= f;  m.yy *= f;  m.yz *= f;
  m.zx *= f;  m.zy *= f;  m.zz *= f;
  return m;
}

////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////

LINE Line(VEC3D a, VEC3D b) {
  LINE l;
  l.a = a;
  l.b = b;
  l.charge = 1;
  return l;
}

int Line_field(LINE l, VEC3D *_v, VEC3D p) {
  int rc = Line_base(l, _v, p);
  if (rc < 0) return rc;
  *_v = Vec3D_mul(*_v, l.charge);
  return 0;
}

int Line_base2(VEC3D a, VEC3D b, VEC3D p, VEC3D *_v) {
  VEC3D va, vb;
  float aa, ab, aa2, ab2, f;
  if (!_v) return -1;
  va.x = p.x - a.x;
  va.y = p.y - a.y;
  va.z = p.z - a.z;
  vb.x = p.x - b.x;
  vb.y = p.y - b.y;
  vb.z = p.z - b.z;
  aa2 = va.x*va.x + va.y*va.y + va.z*va.z;
  ab2 = vb.x*vb.x + vb.y*vb.y + vb.z*vb.z;
  aa = sqrt(aa2);
  ab = sqrt(ab2);
  va.x *= ab;
  va.y *= ab;
  va.z *= ab;
  vb.x *= aa;
  vb.y *= aa;
  vb.z *= aa;
  f = I4PI/(aa2*ab2 + va.x*vb.x + va.y*vb.y + va.z*vb.z);
  _v->x = f*(va.x + vb.x);
  _v->y = f*(va.y + vb.y);
  _v->z = f*(va.z + vb.z);
  return 0;
}

int Line_base(LINE l, VEC3D *_v, VEC3D p) {
  VEC3D va, vb, v;
  float aa, ab;
  if (!_v) return -1;
  va = Vec3D_sub(p, l.a);
  vb = Vec3D_sub(p, l.b);
  aa = Vec3D_abs(va);
  ab = Vec3D_abs(vb);
  va = Vec3D_mul(va, ab);
  vb = Vec3D_mul(vb, aa);
  v = Vec3D_add(va, vb);
  *_v = Vec3D_mul(v, I4PI / (aa*aa*ab*ab + Vec3D_dot(va, vb)));
  return 0;
}


////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////

  
RECTANGLE Rectangle(VEC3D a, VEC3D b, VEC3D e) {
  RECTANGLE r;
  float p;
  r.e = e;
  r.v1 = Vec3D_sub(b, a);
  r.length = Vec3D_abs(r.v1);
  if (r.length > 0) 
    r.v1 = Vec3D_div(r.v1, r.length);
  p = Vec3D_dot(r.e, r.v1);
  r.v2 = Vec3D_mul(r.v1, p);
  r.e = Vec3D_sub(r.e, r.v2);
  r.a = a;
  r.b = b;
  r.c = Vec3D_add(b, e);
  r.d = Vec3D_add(a, e);
  r.v2 = e;
  r.width = Vec3D_abs(e);
  if (r.width > 0) 
    r.v2 = Vec3D_div(r.v2, r.width);
  r.area = r.length * r.width;
  r.v3 = Vec3D_cross(r.v1, r.v2);
  r.m.x = 0.25*(r.a.x + r.b.x + r.c.x + r.d.x);
  r.m.y = 0.25*(r.a.y + r.b.y + r.c.y + r.d.y);
  r.m.z = 0.25*(r.a.z + r.b.z + r.c.z + r.d.z);  
  r.rot = Mat3D(r.v1, r.v2, r.v3);
  return r;
}

int Rectangle_field(RECTANGLE r, VEC3D *_v, VEC3D p) {
  int rc = Rectangle_base(r, _v, p);
  if (rc < 0) return rc;
  *_v = Vec3D_mul(*_v, r.charge);
  return 0;
}

VEC3D Rectangle_transform(RECTANGLE r, VEC3D p) {
  VEC3D q = Vec3D_sub(p, r.a);
  return Mat3D_dot(r.rot, q);
}

float Rectangle_getDistance(RECTANGLE r, VEC3D p) {
  VEC3D rs = Rectangle_transform(r, p);
  if (rs.x > 0) {
    if (rs.x > r.length)  rs.x -= r.length;
    else  rs.x = 0;
  }
  if (rs.y > 0) {
    if (rs.y > r.width)  rs.y -= r.width;
    else  rs.y = 0;
  }
  return Vec3D_abs(rs);
}
    

int Rectangle_base(RECTANGLE r, VEC3D *_v, VEC3D p) {
  float rd, ld, rmin;
  VEC3D a, b, v1, v2, vd, vv, vs, vb;
  int i, n, rc;
  if (!_v) return -1;
  rmin = Rectangle_getDistance(r, p);
  if (r.length > r.width) {
    v1 = r.a;
    v2 = r.b;
    vd = r.e;
    ld = r.width;
  }
  else {
    v1 = r.d;
    v2 = r.a;
    vd = Vec3D_sub(r.b, r.a);
    ld = r.length;
  }
  rd = (rmin > 0.001*ld) ? ld/rmin : 1;
  n = (int)ceil(rd/r.granular);
  vv.x = 0;
  vv.y = 0;
  vv.z = 0;
  for (i=0; i<n; i++) {
    vs = Vec3D_mul(vd, (i+0.5)/n);
    a.x = v1.x + vs.x;
    a.y = v1.y + vs.y;
    a.z = v1.z + vs.z;
    b.x = v2.x + vs.x;
    b.y = v2.y + vs.y;
    b.z = v2.z + vs.z;
    rc = Line_base2(a, b, p, &vb); if (rc) return -2;
    vv.x += vb.x;
    vv.y += vb.y;
    vv.z += vb.z;
  }
  *_v = Vec3D_div(vv, n);
  return 0;
}
  
 
////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////


//------------------------------------------------------------------- Grid1D
int Grid1D(GRID1D *_g, float xmin, float dx, int nx) {
  dP(Grid1D);
  int i;
  if (!_g)                                                           eX(10);
  if (G.xx.start)                                                    eX(1);
  G.xmin = xmin;
  G.dx = dx;
  G.nx = nx; 
  if (G.nx < 1 || G.dx <= 0)                                         eX(2);
  G.constantSpacing = 1;
  AryCreate(&G.xx, sizeof(float), 1, 0, G.nx);                       eG(3);
  for (i=0; i<=G.nx; i++)
    X(G.xx, i) = G.xmin + i*G.dx;
  G.xmax = X(G.xx, G.nx);
  return 0;
eX_10:  eMSG(_null_pointer_grid_);
eX_1:  eMSG(_not_clean_);
eX_2:  eMSG(_invalid_arguments_); 
eX_3:  eMSG(_cant_allocate_);     
}



//------------------------------------------------------------------ Grid1Dz
int Grid1Dz(GRID1D *_g, int nx, float *xx) {
  dP(Grid1Dz);
  int i;  
  if (!_g)                                                           eX(10);
  if (G.xx.start)                                                    eX(1);  
  G.nx = nx - 1;
  if (G.nx < 1)                                                      eX(2);
  G.xmin = xx[0];
  G.dx = 0;
  G.constantSpacing = 0;
  AryCreate(&G.xx, sizeof(float), 1, 0, G.nx);                       eG(3);
  for (i=0; i<=G.nx; i++) {
    if (i > 0 && xx[i] <= xx[i-1])                                   eX(4);
    X(G.xx, i) = xx[i];   
  }
  G.xmax = X(G.xx, G.nx);
  return 0;
eX_10:  eMSG(_null_pointer_grid_);
eX_1:  eMSG(_not_clean_);
eX_2:  eMSG(_invalid_arguments_); 
eX_3:  eMSG(_cant_allocate_);
eX_4:  eMSG(_invalid_spacing_);
}

//---------------------------------------------------------- Grid1D_getPoint
int Grid1D_getPoint(GRID1D g, VEC3D *_p, int i) {
  if (_p == NULL || i<0 || i>g.nx)
    return -1;
  _p->x = Grid1D_getX(g, i);
  _p->y = 0;
  _p->z = 0;
  return 0;
}

//-------------------------------------------------------------- Grid1D_getX
float Grid1D_getX(GRID1D g, int i) {
  return *(float *)AryPtr(&(g.xx), i);
}

//------------------------------------------------------------- Grid1D_getXx
int Grid1D_getXx(GRID1D g, ARYDSC *_a) {
  dP(Grid1D_getXx);
  int i;
  if (!_a || _a->start)                                              eX(1);
  AryCreate(_a, sizeof(float), 1, 0, g.nx);                          eG(2);
  for (i=0; i<=g.nx; i++) 
    X(*(_a), i) = X(g.xx, i);
  return 0;
eX_1:  eMSG(_null_array_);
eX_2:  eMSG(_cant_allocate_); 
}  


//------------------------------------------------------------------- Grid2D
int Grid2D(GRID2D *_g, GRID1D gridX, GRID1D gridY) {
  dP(Grid2D);
  int i, j;
  if (!_g)                                                           eX(10);
  if (G.xx.start || G.yy.start || G.zz.start)                        eX(1);
  G.gridX = gridX;
  G.gridY = gridY;
  G.nx = gridX.nx;
  G.dx = gridX.dx;  
  G.xmin = gridX.xmin;
  G.xmax = gridX.xmax;
  G.ny = gridY.nx;
  G.dy = gridY.dx;  
  G.ymin = gridY.xmin;
  G.ymax = gridY.xmax;
  G.constantSpacing = gridX.constantSpacing * gridY.constantSpacing;
  AryCreate(&G.xx, sizeof(float), 2, 0, G.nx, 0, G.ny);             eG(2);
  AryCreate(&G.yy, sizeof(float), 2, 0, G.nx, 0, G.ny);             eG(3);
  for (i=0; i<=G.nx; i++) {
    for (j=0; j<=G.ny; j++) {
      XX(G.xx, i, j) = X(gridX.xx, i);
      XX(G.yy, i, j) = X(gridY.xx, j);  
    }
  }
  return 0;
eX_10:  eMSG(_null_pointer_grid_);
eX_1:  eMSG(_array_not_clean_);
eX_2:  eMSG(_cant_allocate_xx_);
eX_3:  eMSG(_cant_allocate_yy_);  
}

//------------------------------------------------------------------ Grid2Dz
int Grid2Dz(GRID2D *_g, GRID1D gridX, GRID1D gridY, ARYDSC zz) {
  dP(Grid2Dz);
  int i, j;
  if (!_g)                                                           eX(10);
  if (Grid2D(_g, gridX, gridY) != 0)                                 eX(1);
  if (zz.start) {
    if (zz.numdm != 2 || 
        (zz.bound[0].hgh - zz.bound[0].low + 1 != G.nx+1) ||
        (zz.bound[0].hgh - zz.bound[0].low + 1 != G.nx+1))           eX(2);
    AryCreate(&G.zz, sizeof(float), 2, 0, G.nx, 0, G.ny);            eG(3);
    for (i=0; i<=G.nx; i++)
      for (j=0; j<=G.ny; j++)
        XX(G.zz, i, j) = XX(zz, i, j);
  }
  return 0;
eX_10:  eMSG(_null_pointer_grid_);
eX_1:  eMSG(_cant_init_grid_);
eX_2:  eMSG(_index_range_zz_);  
eX_3:  eMSG(_cant_allocate_zz_);  
}
  
//---------------------------------------------------------------- Grid2Dxyz
int Grid2Dxyz(GRID2D *_g, ARYDSC xx, ARYDSC yy, ARYDSC zz) {
  dP(Grid2D_3);
  int i, j;
  if (!_g)                                                           eX(10);
  if (G.xx.start || G.yy.start || G.zz.start)                        eX(1);
  G.nx = xx.bound[0].hgh - xx.bound[0].low; if (G.nx < 1)            eX(2);
  G.ny = xx.bound[1].hgh - xx.bound[1].low; if (G.ny < 1)            eX(2);
  G.constantSpacing = 0;
  AryCreate(&G.xx, sizeof(float), 2, 0, G.nx, 0, G.ny);              eG(3);
  AryCreate(&G.yy, sizeof(float), 2, 0, G.nx, 0, G.ny);              eG(3);
  for (i=0; i<=G.nx; i++) 
    for (j=0; j<=G.ny; j++) {
      XX(G.xx, i, j) = XX(xx, i, j);
      XX(G.yy, i, j) = XX(yy, i, j);
      XX(G.zz, i, j) = (zz.start) ? XX(zz, i, j) : 0;
    }
  G.dx = 0;
  G.xmin = XX(G.xx, 0, 0);
  G.xmax = G.xmin;
  for (i=0; i<=G.nx; i++) 
  for (j=0; j<=G.ny; j++) {
    if (XX(G.xx, i, j) < G.xmin) G.xmin = XX(G.xx, i, j);
    if (XX(G.xx, i, j) > G.xmax) G.xmax = XX(G.xx, i, j);      
  }
  G.dy = 0;
  G.ymin = XX(G.yy, 0, 0);
  G.ymax = G.ymin;
  for (i=0; i<=G.nx; i++) 
  for (j=0; j<=G.ny; j++) {
    if (XX(G.yy, i, j) < G.ymin) G.ymin = XX(G.yy, i, j);
    if (XX(G.yy, i, j) > G.ymax) G.ymax = XX(G.yy, i, j);      
  }  
  if (!zz.start) {
    G.zmin = 0;
    G.zmax = 0;
    return 0;
  }
  AryCreate(&G.zz, sizeof(float), 2, 0, G.nx, 0, G.ny);             eG(3);
  G.zmin = XX(G.zz, 0, 0);
  G.zmax = G.zmin;
  for (i=0; i<=G.nx; i++) 
    for (j=0; j<=G.ny; j++) {
      if (XX(G.zz, i, j) < G.zmin) G.ymin = XX(G.zz, i, j);
      if (XX(G.zz, i, j) > G.zmax) G.ymax = XX(G.zz, i, j);      
    }  
  return 0;
eX_10:  eMSG(_null_pointer_grid_);
eX_1:  eMSG(_array_not_clean_);
eX_2:  eMSG(_invalid_bounds_);       
eX_3:  eMSG(_cant_allocate_);  
}

//-------------------------------------------------------------- Grid2D_getX
float Grid2D_getX(GRID2D g, int i, int j) {
  dP(Grid2D_getX);
  if (i<0 || i>g.nx || j<0 || j>g.ny)                                eX(1);
  return FF(g.xx, i, j);
eX_1:  eMSG(_index_range_);
}   

//-------------------------------------------------------------- Grid2D_getY
float Grid2D_getY(GRID2D g, int i, int j) {
  dP(Grid2D_getY);
  if (i<0 || i>g.nx || j<0 || j>g.ny)                                eX(1);
  return FF(g.yy, i, j);
eX_1:  eMSG(_index_range_);
}  

//-------------------------------------------------------------- Grid2D_getZ
float Grid2D_getZ(GRID2D g, int i, int j) {
  dP(Grid2D_getZ);
  if (i<0 || i>g.nx || j<0 || j>g.ny)                                eX(1);
  if (!g.zz.start)
    return 0;
  else
    return FF(g.zz, i, j);
eX_1:  eMSG(_index_range_);
}  

//------------------------------------------------------------- Grid2D_getXx
int Grid2D_getXx(GRID2D g, ARYDSC *_a) {
  dP(Grid2D_getXx);
  int i, j;
  if (!_a || _a->start)                                              eX(10);                                        
  AryCreate(_a, sizeof(float), 2, 0, g.nx, 0, g.ny);                 eG(1);
  for (i=0; i<=g.nx; i++) 
    for (j=0; j<=g.ny; j++) 
      XX(*(_a), i, j) = XX(g.xx, i, j);
  return 0;
eX_10:  eMSG(_null_array_);
eX_1:  eMSG(_cant_allocate_); 
}  

//------------------------------------------------------------- Grid2D_getYy
int Grid2D_getYy(GRID2D g, ARYDSC *_a) {
  dP(Grid2D_getYy);
  int i, j;
  if (!_a || _a->start)                                              eX(10);                                        
  AryCreate(_a, sizeof(float), 2, 0, g.nx, 0, g.ny);                 eG(1);
  for (i=0; i<=g.nx; i++) 
    for (j=0; j<=g.ny; j++) 
      XX(*(_a), i, j) = XX(g.yy, i, j);
  return 0;
eX_10:  eMSG(_null_array_);
eX_1:  eMSG(_cant_allocate_); 
}  

//------------------------------------------------------------- Grid2D_getZz
int Grid2D_getZz(GRID2D g, ARYDSC *_a) {
  dP(Grid2D_getZz);
  int i, j;
  if (!_a || _a->start)                                              eX(10);                                        
  if (!g.zz.start) 
    return 0;
  AryCreate(_a, sizeof(float), 2, 0, g.nx, 0, g.ny);                 eG(1);
  for (i=0; i<=g.nx; i++) 
    for (j=0; j<=g.ny; j++) 
      XX(*(_a), i, j) = XX(g.zz, i, j);
  return 0;
eX_10:  eMSG(_null_array_);
eX_1:  eMSG(_cant_allocate_); 
}  


//---------------------------------------------------------- Grid2D_getPoint
int Grid2D_getPoint(GRID2D g, VEC3D *_p, int i, int j) {
  if (_p == NULL) return -1;
  if (i<0 || i>g.nx || j<0 || j>g.ny)
    return -1;
  _p->x = FF(g.xx, i, j);
  _p->y = FF(g.yy, i, j);
  _p->z = 0;
  return 0;
}

//------------------------------------------------------- Grid2D_getMidPoint
int Grid2D_getMidPoint(GRID2D g, VEC3D *_p, int i, int j) {
  if (_p == NULL) return -1;
  if (i<1 || i>g.nx || j<1 || j>g.ny)
    return -1;
  /*
  _p->x = 0.25*(Grid2D_getX(g, i-1, j-1) + Grid2D_getX(g, i, j-1) +
                Grid2D_getX(g, i-1, j)   + Grid2D_getX(g, i, j));
  _p->y = 0.25*(Grid2D_getY(g, i-1, j-1) + Grid2D_getY(g, i, j-1) +
                Grid2D_getY(g, i-1, j)   + Grid2D_getY(g, i, j));
  _p->z = 0;
  if (g.zz.start) 
    _p->z = 0.25*(Grid2D_getZ(g, i-1, j-1) + Grid2D_getZ(g, i, j-1) +
                  Grid2D_getZ(g, i-1, j) + Grid2D_getZ(g, i, j));
  */
  _p->x = 0.25*(FF(g.xx, i-1, j-1) + FF(g.xx, i, j-1) +
                FF(g.xx, i-1, j)   + FF(g.xx, i, j));
  _p->y = 0.25*(FF(g.yy, i-1, j-1) + FF(g.yy, i, j-1) +
                FF(g.yy, i-1, j)   + FF(g.yy, i, j));
  _p->z = 0;
  if (g.zz.start) 
    _p->z = 0.25*(FF(g.zz, i-1, j-1) + FF(g.zz, i, j-1) +
                  FF(g.zz, i-1, j) + FF(g.zz, i, j));
  return 0;
}

//------------------------------------------------------------ Grid2D_isFlat
int Grid2D_isFlat(GRID2D g) {
  volatile int rc = (g.zz.start == NULL);                       //-2005-05-12 lj
  return rc;
}

//------------------------------------------------------------------- Grid3D
int Grid3D(GRID3D *_g, GRID2D gridXY, GRID1D gridZ) {
  dP(Grid3D);
  if (!_g )                                                          eX(1);
  if (G.hh.start)                                                    eX(2);
  if (!gridXY.constantSpacing)                                       eX(3);
  if (!Grid2D_isFlat(gridXY))                                        eX(4);
  if (gridZ.xmin != 0)                                               eX(5);
  memset(_g, 0, sizeof(GRID3D));
  G.gridXY = gridXY;
  G.gridZ = gridZ;
  Grid1D_getXx(gridZ, &G.hh);
  G.constantSpacing = gridZ.constantSpacing;
  G.xmin = gridXY.xmin;
  G.xmax = gridXY.xmax;
  G.ymin = gridXY.ymin;
  G.ymax = gridXY.ymax;
  G.dx = gridXY.dx;
  G.dy = gridXY.dy; 
  G.nx = gridXY.nx;
  G.ny = gridXY.ny;
  G.nz = gridZ.nx; 
  return 0;
eX_1: eMSG(_null_pointer_grid_);
eX_2: eMSG(_hh_not_clean_);
eX_3: eMSG(_constant_spacing_required_);
eX_4: eMSG(_flat_terrain_required_);
eX_5: eMSG(_start_0_required_);
}

//---------------------------------------------------------- Grid3D_getPoint
int Grid3D_getPoint(GRID3D g, VEC3D *_p, int i, int j, int k) {
  int rc;
  if (_p == NULL) return -1;
  rc = Grid2D_getPoint(g.gridXY, _p, i, j);
  if (rc) 
    return rc;
  _p->z += F(g.hh, k);
  return 0;
}

//------------------------------------------------------- Grid3D_getMidPoint
int Grid3D_getMidPoint(GRID3D g, VEC3D *_p, int i, int j, int k) {
  int rc;
  if (k <= 0 || _p == NULL) 
    return -1;
  rc = Grid2D_getMidPoint(g.gridXY, _p, i, j);
  if (rc)
    return rc;
  _p->z += 0.5 * (F(g.hh, k-1) + F(g.hh, k));
  return 0;
}

//---------------------------------------------------------- Grid3D_getAreaX
float Grid3D_getAreaX(GRID3D g, int i, int j, int k) {
  float dz = Grid1D_getX(g.gridZ, k) - Grid1D_getX(g.gridZ, k-1);
  return g.gridXY.dx * dz;
}

//---------------------------------------------------------- Grid3D_getAreaY
float Grid3D_getAreaY(GRID3D g, int i, int j, int k) {
  float dz = Grid1D_getX(g.gridZ, k) - Grid1D_getX(g.gridZ, k-1);
  return g.gridXY.dy * dz;
}

//---------------------------------------------------------- Grid3D_getAreaZ
float Grid3D_getAreaZ(GRID3D g, int i, int j, int k) {
  return g.gridXY.dx * g.gridXY.dy;
}

//--------------------------------------------------------- Grid3D_isOutside
int Grid3D_isOutside(GRID3D g, float x, float y, float z) {
  if (x<g.xmin || x>g.xmax || y<g.ymin || y>g.ymax || 
    z<F(g.hh, 0) || z>F(g.hh, g.nz))
    return 1;
  else 
    return 0;
}

//------------------------------------------------------- Grid3D_getPosition
int Grid3D_getPosition(GRID3D g, POSITION *_p, VEC3D point) {
  int k;
  float h;
  if (_p == NULL) return -1;
  _p->ax = (point.x - g.xmin)/g.dx;
  if (_p->ax < 0 || _p->ax > g.nx) 
    return -2; 
  _p->i = (int)_p->ax;
  if (_p->i == g.nx)_p->i--;
  _p->ax -= _p->i++;
  _p->ay = (point.y - g.ymin)/g.dy;
  if (_p->ay < 0 || _p->ay > g.ny) 
    return -3; 
  _p->j = (int)_p->ay;
  if (_p->j == g.ny) _p->j--;
  _p->ay -= _p->j++;
  h = point.z;
  for (k=1; k<=g.nz; k++)
    if (F(g.hh, k) >= h) break;
  if (k < 0 || k > g.nz) 
    return -4;
  _p->az = (h - F(g.hh, k-1))/(F(g.hh, k) - F(g.hh, k-1));
  _p->k = k;
  return 0;
}    

//------------------------------------------------------- Grid3D_getPosition
int Grid3D_getIntPosition(GRID3D g, int *i, int *j, int *k, VEC3D point) {
  int kk, pi, pj;
  float h, ax, ay;
  ax = (point.x - g.xmin)/g.dx;
  if (ax < 0 || ax > g.nx)
    return -2; 
  pi = (int)ax;
  if (pi == g.nx) pi--;
  ax -= pi++;
  ay = (point.y - g.ymin)/g.dy;
  if (ay < 0 || ay > g.ny) 
    return -3; 
  pj = (int)ay;
  if (pj == g.ny) pj--;
  ay -= pj++;
  h = point.z;
  for (kk=1; kk<=g.nz; kk++)
    if (F(g.hh, kk) >= h) break;
  if (kk < 0 || kk > g.nz) 
    return -4;
  *k = kk;
  *i = pi;
  *j = pj;
  return 0;
}


////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////


//------------------------------------------------------------------ Bound3D
int Bound3D(BOUND3D *_b, GRID3D g) {
  dP(Bound3D);
  int i, j, k;
  if (!_b)                                                           eX(1);
  if (B.bb.start)                                                    eX(2);
  B.grid = g;
  B.nx = g.gridXY.nx;
  B.ny = g.gridXY.ny;
  B.nz = g.gridZ.nx;
  AryCreate(&B.bb, sizeof(int), 3, 0, B.nx, 0, B.ny, 0, B.nz);     eG(3);
  for (i=0; i<=B.nx; i++) {
    for (j=0; j<=B.ny; j++) {
      BBB(B.bb, i, j, 0) |= BND_PNTBND;
      BBB(B.bb, i, j, B.nz) |= BND_PNTBND;
      if (i > 0 && j > 0) {
        BBB(B.bb, i, j, 0) |= BND_ARZBND | BND_ARZPOS | BND_ARZSET;
        BBB(B.bb, i, j, 1) |= BND_VOLBND;
        BBB(B.bb, i, j, B.nz) |= BND_ARZBND | BND_VOLBND;     
      }
    }
  }
  for (i=0; i<=B.nx; i++) {
    for (k=0; k<=B.nz; k++) {
      BBB(B.bb, i, 0, k) |= BND_PNTBND;
      BBB(B.bb, i, B.ny, k) |= BND_PNTBND;
      if (i > 0 && k > 0) {
        BBB(B.bb, i, 0, k) |= BND_ARYBND | BND_ARYPOS;
        BBB(B.bb, i, 1, k) |= BND_VOLBND;
        BBB(B.bb, i, B.ny, k) |= BND_ARYBND | BND_VOLBND;     
      }
    }
  }  
  for (j=0; j<=B.ny; j++) {
    for (k=0; k<=B.nz; k++) {
      BBB(B.bb, 0, j, k) |= BND_PNTBND;
      BBB(B.bb, B.nx, j, k) |= BND_PNTBND;
      if (j > 0 && k > 0) {
        BBB(B.bb, 0, j, k) |= BND_ARXBND | BND_ARXPOS;
        BBB(B.bb, 1, j, k) |= BND_VOLBND;
        BBB(B.bb, B.nx, j, k) |= BND_ARXBND | BND_VOLBND;     
      }
    }
  }  
  B.completed = 0;    
  return 0;
eX_1: eMSG(_null_boundary_);
eX_2: eMSG(_bb_not_clean_);
eX_3: eMSG(_cant_allocate_);   
}

//------------------------------------------------------------- Bound3D_copy
int Bound3D_copy(BOUND3D *_b, BOUND3D source) {
  int rc, i, j, k;
  if (_b == NULL) return -1;
  rc = Bound3D(_b, source.grid);
  if (rc != 0)
    return -1;
  for (i=0; i<=B.nx; i++) 
    for (j=0; j<=B.ny; j++) 
      for (k=0; k<=B.nz; k++)
        BBB(B.bb, i, j, k) = BBB(source.bb, i, j, k);  
  return 0;
}

//--------------------------------------------------------- Bound3D_complete
int Bound3D_complete(BOUND3D *_b) {
  int i, j, k, m;
  for (i=1; i<=B.nx; i++) {
    for (j=1; j<=B.ny; j++) {
      for (k=1; k<=B.nz; k++) {
        if ((BBB(B.bb, i, j, k) & BND_VOLOUT) == 0) {
          m = 0;
          if (i == 1 || i == B.nx || 
              j == 1 || j == B.ny ||
              k == 1 || k == B.nz) m = BND_VOLBND;
          else if ((i > 1)    && ((BBB(B.bb, i-1, j, k) & BND_VOLOUT) != 0))
            m = BND_VOLBND;
          else if ((i < B.nx) && ((BBB(B.bb, i+1, j, k) & BND_VOLOUT) != 0))
            m = BND_VOLBND;
          else if ((j > 1)    && ((BBB(B.bb, i, j-1, k) & BND_VOLOUT) != 0))
            m = BND_VOLBND;
          else if ((j < B.ny) && ((BBB(B.bb, i, j+1, k) & BND_VOLOUT) != 0))
            m = BND_VOLBND;
          else if ((k > 1)    && ((BBB(B.bb, i, j, k-1) & BND_VOLOUT) != 0))
            m = BND_VOLBND;
          else if ((k < B.nz) && ((BBB(B.bb, i, j, k+1) & BND_VOLOUT) != 0))
            m = BND_VOLBND;
          BBB(B.bb, i, j, k) |= m;
        }
        else {
          m = 0;
          if (i == 1 || i == B.nx || 
              j == 1 || j == B.ny ||
              k == 1 || k == B.nz) m = BND_VOLBND;
          else if ((i > 1)    && ((BBB(B.bb, i-1, j, k) & BND_VOLOUT) == 0))
            m = BND_VOLBND;
          else if ((i < B.nx) && ((BBB(B.bb, i+1, j, k) & BND_VOLOUT) == 0))
            m = BND_VOLBND;
          else if ((j > 1)    && ((BBB(B.bb, i, j-1, k) & BND_VOLOUT) == 0))
            m = BND_VOLBND;
          else if ((j < B.ny) && ((BBB(B.bb, i, j+1, k) & BND_VOLOUT) == 0))
            m = BND_VOLBND;
          else if ((k > 1)    && ((BBB(B.bb, i, j, k-1) & BND_VOLOUT) == 0))
            m = BND_VOLBND;
          else if ((k < B.nz) && ((BBB(B.bb, i, j, k+1) & BND_VOLOUT) == 0))
            m = BND_VOLBND;
          BBB(B.bb, i, j, k) = (BBB(B.bb, i, j, k) & ~BND_VOLBND) | m;
        }
      }
    }
  }
  B.completed = 1;
  return 0;
}

//-------------------------------------------------------- Bound3D_setVolout
int Bound3D_setVolout(BOUND3D *_b, int i, int j, int k, int value) {
  int is, js, ks;
  int nins[4][4][4], nout[4][4][4];
  if (!_b || B.completed) 
    return -1;
  if (i < 1 || i > B.nx || j < 1 || j > B.ny || k < 1 || k > B.nz)
    return -2;
  value &= 0xf;
  BBB(B.bb, i, j, k) = (BBB(B.bb, i, j, k) & ~BND_VOLMSK) | BND_VOLOUT;
  //
  BBB(B.bb, i-1, j, k) &= ~BND_ARXMSK;
  if (i > 1) 
    if ((BBB(B.bb, i-1, j, k) & BND_VOLOUT) != 0)
      BBB(B.bb, i-1, j, k) |= BND_ARXOUT;
    else
      BBB(B.bb, i-1, j, k) |= BND_ARXBND | value << BND_ARXSHIFT;
  else
    BBB(B.bb, i-1, j, k) |= BND_ARXOUT;
  BBB(B.bb, i, j, k) &= ~BND_ARXMSK;
  if (i < B.nx)
    if ((BBB(B.bb, i+1, j, k) & BND_VOLOUT) != 0)
      BBB(B.bb, i, j, k) |= BND_ARXOUT;
    else
      BBB(B.bb, i, j, k) |= BND_ARXBND | BND_ARXPOS | value << BND_ARXSHIFT;
  else
    BBB(B.bb, i, j, k) |= BND_ARXOUT;
  //
  BBB(B.bb, i, j-1, k) &= ~BND_ARYMSK;
  if (j > 1) 
    if ((BBB(B.bb, i, j-1, k) & BND_VOLOUT) != 0)
      BBB(B.bb, i, j-1, k) |= BND_ARYOUT;
    else
      BBB(B.bb, i, j-1, k) |= BND_ARYBND | value << BND_ARYSHIFT;
  else
    BBB(B.bb, i, j-1, k) |= BND_ARYOUT;
  BBB(B.bb, i, j, k) &= ~BND_ARYMSK;
  if (j < B.ny)
    if ((BBB(B.bb, i, j+1, k) & BND_VOLOUT) != 0)
      BBB(B.bb, i, j, k) |= BND_ARYOUT;
    else
      BBB(B.bb, i, j, k) |= BND_ARYBND | BND_ARYPOS | value << BND_ARYSHIFT;
  else
    BBB(B.bb, i, j, k) |= BND_ARYOUT;
  //
  BBB(B.bb, i, j, k-1) &= ~BND_ARZMSK;
  if (k > 1) 
    if ((BBB(B.bb, i, j, k-1) & BND_VOLOUT) != 0)
      BBB(B.bb, i, j, k-1) |= BND_ARZOUT;
    else
      BBB(B.bb, i, j, k-1) |= BND_ARZBND | value << BND_ARZSHIFT;
  else
    BBB(B.bb, i, j, k-1) |= BND_ARZOUT;
  BBB(B.bb, i, j, k) &= ~BND_ARZMSK;
  if (k < B.nz)
    if ((BBB(B.bb, i, j, k+1) & BND_VOLOUT) != 0)
      BBB(B.bb, i, j, k) |= BND_ARZOUT;
    else
      BBB(B.bb, i, j, k) |= BND_ARZBND | BND_ARZPOS | value << BND_ARZSHIFT;
  else
    BBB(B.bb, i, j, k) |= BND_ARZOUT;
  //
  for (is=i-1; is<=i+1; is++) {
    if (is > B.nx) break;
    if (is < 1) continue;
    for (js=j-1; js<=j+1; js++) {
      if (js > B.ny) break;
      if (js < 1) continue;
      for (ks=k-1; ks<=k+1; ks++) {
        if (ks > B.nz) break;
        if (ks < 1) continue;
        if ((BBB(B.bb, is, js, ks) & BND_VOLOUT) != 0) {
          nout[is-i+1][js-j+1][ks-k+1]++;
          nout[is-i+2][js-j+1][ks-k+1]++;
          nout[is-i+1][js-j+2][ks-k+1]++;
          nout[is-i+1][js-j+1][ks-k+2]++;
          nout[is-i+2][js-j+2][ks-k+1]++;
          nout[is-i+1][js-j+2][ks-k+2]++;
          nout[is-i+2][js-j+1][ks-k+2]++;
          nout[is-i+2][js-j+2][ks-k+2]++;
        }
        else {
          nins[is-i+1][js-j+1][ks-k+1]++;
          nins[is-i+2][js-j+1][ks-k+1]++;
          nins[is-i+1][js-j+2][ks-k+1]++;
          nins[is-i+1][js-j+1][ks-k+2]++;
          nins[is-i+2][js-j+2][ks-k+1]++;
          nins[is-i+1][js-j+2][ks-k+2]++;
          nins[is-i+2][js-j+1][ks-k+2]++;
          nins[is-i+2][js-j+2][ks-k+2]++;
        }   
      }
    }
  }
  for (is=1; is<=2; is++)
    for (js=1; js<=2; js++)
      for (ks=1; ks<=2; ks++) {  
        BBB(B.bb, i+is-2, j+js-2, k+ks-2) &= ~BND_PNTMSK;
        if (nout[is][js][ks] > 0) {
          if (nins[is][js][ks] > 0) 
            BBB(B.bb, i+is-2, j+js-2, k+ks-2) |= BND_PNTBND;
          else
            BBB(B.bb, i+is-2, j+js-2, k+ks-2) |= BND_PNTOUT;
        }
      }
  return 0;
}

//-------------------------------------------------------- Bound3D_setFixedX
int Bound3D_setFixedX(BOUND3D *_b, int i, int j, int k, int set) {
  if (B.completed)
    return -1;
  if (i<0 || i>B.nx || j<1 || j>B.ny || k<1 || k>B.nz)
    return -2;
  if ((BBB(B.bb, i, j, k) & BND_ARXBND) == 0)
    return -3;
  if (set)
    BBB(B.bb, i, j, k) |= BND_ARXSET;
  else
    BBB(B.bb, i, j, k) &= ~BND_ARXSET;
  return 0;
}

//-------------------------------------------------------- Bound3D_setFixedY
int Bound3D_setFixedY(BOUND3D *_b, int i, int j, int k, int set) {
  if (B.completed)
    return -1;
  if (i<1 || i>B.nx || j<0 || j>B.ny || k<1 || k>B.nz)
    return -2;
  if ((BBB(B.bb, i, j, k) & BND_ARYBND) == 0)
    return -3;
  if (set)
    BBB(B.bb, i, j, k) |= BND_ARYSET;
  else
    BBB(B.bb, i, j, k) &= ~BND_ARYSET;
  return 0;
}

//-------------------------------------------------------- Bound3D_setFixedZ
int Bound3D_setFixedZ(BOUND3D *_b, int i, int j, int k, int set) {
  if (B.completed)
    return -1;
  if (i<1 || i>B.nx || j<1 || j>B.ny || k<0 || k>B.nz)
    return -2;
  if ((BBB(B.bb, i, j, k) & BND_ARZBND) == 0)
    return -3;
  if (set)
    BBB(B.bb, i, j, k) |= BND_ARZSET;
  else
   BBB(B.bb, i, j, k) &= ~BND_ARZSET;
  return 0;
}
  

//----------------------------------------------------- Bound3D_getPointType
int Bound3D_getPointType(BOUND3D b, int i, int j, int k) {
  int m;
  if (!b.completed) 
    return -1;
  if (i<0 || i>b.nx || j<0 || j>b.ny || k<0 || k>b.nz)
    return -2;
  m = (III(b.bb, i, j, k) & BND_PNTMSK) >> BND_PNTSHIFT;
  return m;
}
  
//---------------------------------------------------- Bound3D_getVolumeType
int Bound3D_getVolumeType(BOUND3D b, int i, int j, int k) {
  int m;
  if (!b.completed) 
    return -1;
  if (i<1 || i>b.nx || j<1 || j>b.ny || k<1 || k>b.nz)
    return -2;
  m = (III(b.bb, i, j, k) & BND_VOLMSK) >> BND_VOLSHIFT;
  return m;
}

//----------------------------------------------------- Bound3D_getAreaXType
int Bound3D_getAreaXType(BOUND3D b, int i, int j, int k) {
  int m;
  if (!b.completed) 
    return -1;
  if (i<0 || i>b.nx || j<1 || j>b.ny || k<1 || k>b.nz)
    return -2;
  m = (III(b.bb, i, j, k) & BND_ARXMSK) >> BND_ARXSHIFT;
  return m;
}

//----------------------------------------------------- Bound3D_getAreaYType
int Bound3D_getAreaYType(BOUND3D b, int i, int j, int k) {
  int m;
  if (!b.completed) 
    return -1;
  if (i<1 || i>b.nx || j<0 || j>b.ny || k<1 || k>b.nz)
    return -2;
  m = (III(b.bb, i, j, k) & BND_ARYMSK) >> BND_ARYSHIFT;
  return m;
}

//----------------------------------------------------- Bound3D_getAreaZType
int Bound3D_getAreaZType(BOUND3D b, int i, int j, int k) {
  int m;
  if (!b.completed) 
    return -1;
  if (i<1 || i>b.nx || j<1 || j>b.ny || k<0 || k>b.nz)
    return -2;
  m = (III(b.bb, i, j, k) & BND_ARZMSK) >> BND_ARZSHIFT;
  return m;
}


//------------------ -------------------------------------- Bound3D_isInside
int Bound3D_isInside(BOUND3D b, int i, int j, int k) {
  return (Bound3D_getVolumeType(b, i, j, k) & BND_OUTSIDE);
}


////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////

//------------------------------------------------------------------- Wind3D
int Wind3D(WIND3D *_w, BOUND3D bound) {
  dP(Wind3D);
  int i, j, k, rc;
  float z;
  if (!_w)                                                           eX(1);
  if (W.hh.start || W.zp.start || W.zg.start)                        eX(2);
  if (W.vx.start || W.vy.start || W.vz.start)                        eX(3);
  if (!bound.completed)                                              eX(4);
  W.bound = bound;
  W.grid = bound.grid;
  W.gridXY = W.grid.gridXY;
  W.gridZ =  W.grid.gridZ;
  W.nx = W.grid.nx;
  W.ny = W.grid.ny;
  W.nz = W.grid.nz;
  W.dx = W.gridXY.dx;
  W.dy = W.gridXY.dy;
  W.dz = (W.gridZ.constantSpacing) ? W.gridZ.dx : 0;
  W.areaX = W.dy * W.dz;
  W.areaY = W.dy * W.dz;
  W.areaZ = W.dx * W.dy;
  rc = Grid1D_getXx(W.gridZ, &W.hh); if (rc)                         eX(5);
  rc = Grid2D_getZz(W.gridXY, &W.zg); if (rc)                        eX(6);
  AryCreate(&W.zp, sizeof(float), 3, 0, W.nx, 0, W.ny, 0, W.nz);     eG(7);
  AryCreate(&W.vx, sizeof(float), 3, 0, W.nx, 0, W.ny, 0, W.nz);     eG(8);
  AryCreate(&W.vy, sizeof(float), 3, 0, W.nx, 0, W.ny, 0, W.nz);     eG(9);
  AryCreate(&W.vz, sizeof(float), 3, 0, W.nx, 0, W.ny, 0, W.nz);     eG(10);
  for (i=0; i<=W.nx; i++) {
    for (j=0; j<=W.ny; j++) {
      z = (W.zg.start) ? XX(W.zg, i, j) : 0;
      for (k=0; k<=W.nz; k++)
        XXX(W.zp, i, j, k) = z + X(W.hh, k);
    }
  }
  return 0;
eX_1: eMSG(_null_wind_);
eX_2: eMSG(_not_clean_);
eX_3: eMSG(_not_clean_);
eX_4: eMSG(_incomplete_boundary_);
eX_5: eMSG(_cant_get_hh_);
eX_6: eMSG(_cant_get_zg_);           
eX_7: eMSG(_cant_allocate_);  
eX_8: eMSG(_cant_allocate_); 
eX_9: eMSG(_cant_allocate_); 
eX_10: eMSG(_cant_allocate_); 
}

//----------------------------------------------- Wind3D_clearBoundaryValues
int Wind3D_clearBoundaryValues(WIND3D *_w) {
  int m, i, j, k;
  if (!_w || !W.vx.start || !W.vy.start || !W.vz.start) return -1;
  for (i=0; i<=W.nx; i++) 
    for (j=1; j<=W.ny; j++) 
      for (k=1; k<=W.nz; k++) {
        m = Bound3D_getAreaXType(W.bound, i, j, k); 
        if (m < 0) return m;
        if (i==0 || i==W.nx) m &= ~BND_FIXED;
        if ((m & (BND_OUTSIDE | BND_FIXED)) != 0) 
          XXX(W.vx, i, j, k) = 0;
      }
  for (i=1; i<=W.nx; i++) 
    for (j=0; j<=W.ny; j++) 
      for (k=1; k<=W.nz; k++) {
        m = Bound3D_getAreaYType(W.bound, i, j, k); 
        if (m < 0) return m;
        if (j==0 || j==W.ny) m &= ~BND_FIXED;
        if ((m & (BND_OUTSIDE | BND_FIXED)) != 0)
          XXX(W.vy, i, j, k) = 0;
      }
  for (i=1; i<=W.nx; i++) 
    for (j=1; j<=W.ny; j++) 
      for (k=0; k<=W.nz; k++) {
        m = Bound3D_getAreaZType(W.bound, i, j, k); 
        if (m < 0) return m;
        if (k==W.nz) m &= ~BND_FIXED;
        if ((m & (BND_OUTSIDE | BND_FIXED)) != 0)
          XXX(W.vz, i, j, k) = 0;
      }      
  return 0;
}

//--------------------------------------------------------------- Wind3D_add
int Wind3D_add(WIND3D *_w, WIND3D source, float f) {
  dP(Wind3D_add);
  int i, j, k;
  if (!_w)                                                           eX(1);
  if (W.bound.nx != source.bound.nx || 
      W.bound.ny != source.bound.ny || 
      W.bound.nz != source.bound.nz)                                 eX(2);
  for (i=0; i<=W.nx; i++) 
    for (j=0; j<=W.ny; j++) 
      for (k=0; k<=W.nz; k++) 
        if (XXX(W.bound.bb, i,j,k) != XXX(source.bound.bb, i,j,k))   eX(3);
  for (i=0; i<=W.nx; i++) 
    for (j=0; j<=W.ny; j++) 
      for (k=0; k<=W.nz; k++) {
        XXX(W.vx, i, j, k) += f * XXX(source.vx, i, j, k);
        XXX(W.vy, i, j, k) += f * XXX(source.vy, i, j, k);
        XXX(W.vz, i, j, k) += f * XXX(source.vz, i, j, k);
      }
  return 0;
eX_1: eMSG(_null_argument_);
eX_2: eMSG(_inconsistent_number_cells_);
eX_3: eMSG(_inconsistent_boundary_structure_);
}
      
//-------------------------------------------------------- Wind3D_divergence
int Wind3D_divergence(WIND3D w, int i, int j, int k, float *_d) {
  float d, dz;
  if (!_d) return -1;
  if (i<1 || i>w.nx || j<1 || j>w.ny || k<1 || k>w.nz)
    return -2;
  d = 0;
  dz = X(w.hh, k) - X(w.hh, k-1); 
  d += w.dy*w.dz * (XXX(w.vx, i,j,k) - XXX(w.vx, i-1,j,k));
  d += w.dx*w.dz * (XXX(w.vy, i,j,k) - XXX(w.vy, i,j-1,k));
  d += w.areaZ   * (XXX(w.vz, i,j,k) - XXX(w.vx, i,j,k-1));
  d /= (w.areaZ * w.dz);
  *(_d) = d;
  return 0;
}

//----------------------------------------------------- Wind3D_maxDivergence
int Wind3D_maxDivergence(WIND3D w, float *_d) {
  float d, dmax=0, rx=1/w.dx, ry=1/w.dy, rz;
  int i, j, k;  
  if (!_d) return -1;
  for (i=1; i<=w.nx; i++) {
    for (j=1; j<=w.ny; j++) {
      for (k=1; k<=w.nz; k++) {
        if ((BBB(w.bound.bb, i, j, k) & BND_VOLOUT) == 0) {
          rz = 1 / (X(w.hh, k) - X(w.hh, k-1));
          d = 0;
          d += rx * (FFF(w.vx, i,j,k) - FFF(w.vx, i-1,j,k));
          d += ry * (FFF(w.vy, i,j,k) - FFF(w.vy, i,j-1,k));
          d += rz * (FFF(w.vz, i,j,k) - FFF(w.vz, i,j,k-1));
          if (d < 0) 
            d = -d;
          if (d > dmax)
            dmax = d;
        }
      }
    }
  }
  *(_d) = dmax;
  return 0;
}
 

//------------------------------------------------------------ Wind3D_printZ
int Wind3D_printZ(WIND3D w, int k, FILE *_f) {
  int i, j;
  if (!_f) _f = stdout;
  if (k < 1 || k > w.nz) {
    fprintf(_f, "invalid k\n");
    return -1;
  }
  fprintf(_f, "k=%d\n", k);
  fprintf(_f, "j\\i      ");
  for (i=1; i<=w.nx; i++) 
    fprintf(_f, "  [%2d]   ", i);
  fprintf(_f, "\n");
  for (j=w.ny; j>=0; j--) {
    fprintf(_f, "         ");
    for (i=1; i<=w.nx; i++)
      fprintf(_f, "+ %6.3f ", XXX(w.vy, i, j, k));
    fprintf(_f, "+\n");
    if (j == 0) break;
    //
    fprintf(_f, "         ");
    for (i=1; i<=w.nx; i++)
      fprintf(_f, "|        ");
    fprintf(_f, "|\n");
    //
    fprintf(_f, "[%2d]  ", j);
    for (i=0; i<=w.nx; i++) 
      fprintf(_f, "%6.3f   ", XXX(w.vx, i, j, k));
    fprintf(_f, "\n");  
    //
    fprintf(_f, "         ");
    for (i=1; i<=w.nx; i++)
      fprintf(_f, "|        ");
    fprintf(_f, "|\n");
  }
  return 0;
}

//------------------------------------------------------------ Wind3D_vector
VEC3D Wind3D_vector(WIND3D w, int i, int j, int k) {
  VEC3D v;
  v = Vec3D(FFF(w.vx,i,j,k), FFF(w.vy,i,j,k), FFF(w.vz,i,j,k));
  return v;
}

//--------------------------------------------------------- Wind3D_getVector
int Wind3D_getVector(WIND3D w, VEC3D *_v, VEC3D pnt) {
  POSITION p;
  int rc;
  float v0, v1;
  if (!_v) return -1;
  rc = Grid3D_getPosition(w.grid, &p, pnt); if (rc) return -2;
  if (Bound3D_isInside(w.bound, p.i, p.j, p.k)) return -3;
  v1 = FFF(w.vx, p.i, p.j, p.k);
  v0 = FFF(w.vx, p.i-1, p.j, p.k);
  _v->x = v0 + p.ax * (v1 - v0);
  v1 = FFF(w.vy, p.i, p.j, p.k);
  v0 = FFF(w.vy, p.i, p.j-1, p.k);
  _v->y = v0 + p.ay * (v1 - v0);
  v1 = FFF(w.vz, p.i, p.j, p.k);
  v0 = FFF(w.vz, p.i, p.j, p.k-1);
  _v->z = v0 + p.az * (v1 - v0);
  return 0;
}
  

//--------------------------------------------------------- Wind3D_writeDMNA
int Wind3D_writeDMNA(WIND3D w, FILE *_f, char *frm) {
  //dP(Wind3D_writeDMNA);
  int i, j, k;
  char format[64];
  sprintf(format, "%%7.2f %s %s %s\n", frm, frm, frm);
  if (!_f) _f = stdout;
  for (i=1; i<=w.grid.nx; i++)
    for (j=1; j<=w.grid.ny; j++)
      for (k=0; k<w.grid.nz; k++)
        if (Bound3D_getVolumeType(w.bound, i,j,k+1) & BND_OUTSIDE)
          XXX(w.vz, i,j,k) = WND_VOLOUT;
  fprintf(_f, "sk    ");
  for (k=0; k<=w.nz; k++) fprintf(_f, "%1.1f ", X(w.hh, k));
  fprintf(_f, "\n");
  fprintf(_f, "hh    ");
  for (k=0; k<=w.nz; k++) fprintf(_f, "%1.1f ", X(w.hh, k));
  fprintf(_f, "\n");
  fprintf(_f, "xmin  %1.1f\n", w.grid.xmin);
  fprintf(_f, "ymin  %1.1f\n", w.grid.ymin);
  fprintf(_f, "delta %1.1f\n", w.dx);  
  fprintf(_f, "x0    %1.1f\n", w.grid.xmin);
  fprintf(_f, "y0    %1.1f\n", w.grid.ymin);
  fprintf(_f, "dd    %1.1f\n", w.dx);  
  fprintf(_f, "sscl  %1.1f\n", 0.);  
  fprintf(_f, "zscl  %1.1f\n", 0.);  
  fprintf(_f, "t1    -inf\n");  
  fprintf(_f, "t2    +inf\n"); 
  fprintf(_f, "uref  -1\n"); 
  fprintf(_f, "artp  \"W\"\n"); 
  fprintf(_f, "vldf  \"PXYS\"\n"); 
  fprintf(_f, "axes  \"xyz\"\n"); 
  fprintf(_f, "sequ  \"i+,j+,k+\"\n");     
  fprintf(_f, "lsbf  1\n"); 
  fprintf(_f, "mode  \"text\"\n"); 
  fprintf(_f, "form  \"Z%%7.2fVx%%[2]7.2fVs%%7.2f\"\n");
  fprintf(_f, "dims  3\n");  
  fprintf(_f, "lowb     0   0   0\n"); 
  fprintf(_f, "hghb  %3d %3d %3d\n", w.nx, w.ny, w.nz); 
  fprintf(_f, "***\n");
  for (i=0; i<=w.grid.nx; i++) {
    for (j=0; j<=w.grid.ny; j++) {
      for (k=0; k<=w.grid.nz; k++)
        fprintf(_f, format, FFF(w.zp, i,j,k), 
          FFF(w.vx, i,j,k), FFF(w.vy, i,j,k), FFF(w.vz, i,j,k));
      fprintf(_f, "\n");
    }
    fprintf(_f, "\n");
  }
  fprintf(_f, "***\n");
  return 0;
}

void Grid1D_free(GRID1D *_g) {
  if (!_g) return;
  AryFree(&(_g->xx));
}
    
void Grid2D_free(GRID2D *_g) {
  if (!_g) return;
  AryFree(&(_g->xx));
  AryFree(&(_g->yy));
  AryFree(&(_g->zz));
}

void Grid3D_free(GRID3D *_g) {
  if (!_g) return;
  AryFree(&(_g->hh));
}    

void Bound3D_free(BOUND3D *_b) {
  if (!_b) return;
  AryFree(&(_b->bb));
}    

void Wind3D_free(WIND3D *_w) {
  if (!_w) return;
  AryFree(&(_w->hh));
  AryFree(&(_w->zp));
  AryFree(&(_w->zg));     
  AryFree(&(_w->vx));
  AryFree(&(_w->vy));
  AryFree(&(_w->vz));
}

////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////

//------------------------------------------------------------ BodyRectangle
int BodyRectangle(BODY *_b, float x1, float y1, float dx, float dy,
float h, float c, float w) {
  dP(BodyRectangle);
  float x, y, si, co;
  if (!_b) return -1;
  B.type = BDY_RECTANGLE;
  B.floor = h;
  B.top = h+c;
  B.np = 5;
  B.xp = ALLOC(B.np*sizeof(float));  //-2005-01-11
  B.yp = ALLOC(B.np*sizeof(float));  //-2005-01-11
  x = 0;
  y = 0;
  si = sin(w/RADIAN);   //-2004-12-14
  co = cos(w/RADIAN);   //-2004-12-14
  B.xp[0] = x1;
  B.yp[0] = y1;
  x += dx;
  B.xp[1] = x1 + x*co - y*si;  
  B.yp[1] = y1 + x*si + y*co;
  y += dy;
  B.xp[2] = x1 + x*co - y*si;  
  B.yp[2] = y1 + x*si + y*co;
  x -= dx;
  B.xp[3] = x1 + x*co - y*si;  
  B.yp[3] = y1 + x*si + y*co;
  B.xp[4] = B.xp[0];      
  B.yp[4] = B.yp[0];
  setBodyBorderValues(_b);  //-2004-12-14
  return 0;
}

//----------------------------------------------------------------- BodyCircle
int BodyCircle(BODY *_b, float x, float y, float d, float h, float c) {
  dP(BodyCircle);
  float r;
  int i;
  if (!_b) return -1;
  B.type = BDY_CIRCLE;
  B.floor = h;
  B.top = h+c;
  B.np = 37;
  B.xp = CALLOC(B.np, sizeof(float));
  B.yp = CALLOC(B.np, sizeof(float));
  r = 0.5 * d;
  for (i=0; i<36; i++) {
    B.xp[i] = x - r*sin(10./RADIAN*i);   //-2004-12-14
    B.yp[i] = y + r*cos(10./RADIAN*i);   //-2004-12-14
  }
  B.xp[i] = B.xp[0];
  B.yp[i] =  B.yp[0];
  setBodyBorderValues(_b);  //-2004-12-14
  return 0;
}

//--------------------------------------------------------------------- Body
int BodyPolygon(BODY *_b, float h, float c) {
  if (!_b) return -1;
  B.type = BDY_POLYGON;
  B.floor = h;
  B.top = h+c;
  B.xp = NULL;
  B.yp = NULL;
  B.np = 0;
  return 0;
}

//-------------------------------------------------------- setBodyBorderValues
static int setBodyBorderValues(BODY *_b) {  //-2004-12-14
  int i;
  if (!_b) return -1;
  B.x0 = B.xp[0];
  B.x1 = B.xp[0];   
  B.y0 = B.yp[0];
  B.y1 = B.yp[0];
  for (i=1; i<B.np; i++) {
    if (B.xp[i] < B.x0) B.x0 = B.xp[i];     
    if (B.xp[i] > B.x1) B.x1 = B.xp[i]; 
    if (B.yp[i] < B.y0) B.y0 = B.yp[i];     
    if (B.yp[i] > B.y1) B.y1 = B.yp[i];
  }
  return 0;
}

//----------------------------------------------------------- Body_setPoints
int Body_setPoints(BODY *_b, int na, float *xa, float *ya) {
  dP(Body_setPoints);
  int np, i, j;
  float *xp, *yp, tau[2];
  float x1, y1, x2, y2, a1, b1, a2, b2, area, dx, dy, da, db, dd;
  if (!_b || !xa || !ya || na<2) return -1;
  if (B.type != BDY_POLYGON) return -2;
  // copy values
  np = na;
  if (xa[0] != xa[np-1] || ya[0] != ya[np-1])
    np++;
  xp = CALLOC(np, sizeof(float));
  yp = CALLOC(np, sizeof(float));
  B.np = 0;
  B.xp = NULL;
  B.yp = NULL;
  for (i=0; i<np; i++) {
    xp[i] = (i < na) ? xa[i] : xa[0];
    yp[i] = (i < na) ? ya[i] : ya[0];
    if (i > 0) {
      if (xp[i] == xp[i-1] && yp[i] == yp[i-1])
        return -3;
    }
  }
  // check for crossings
  for (i=1; i<np; i++) {
    x1 = xp[i-1];
    y1 = yp[i-1];
    x2 = xp[i];
    y2 = yp[i];
    for (j=i+2; j<np; j++) {
      if (j==np-1 && i==1)  break;
      a1 = xp[j-1];
      b1 = yp[j-1];
      a2 = xp[j];
      b2 = yp[j];
      if (a1 == x2 && b1 == y2) 
        continue;
      if (a2 == x1 && b2 == y1)
        continue;
      dx = x2 - x1;
      dy = y2 - y1;
      da = a2 - a1;
      db = b2 - b1;
      dd = da*dy - db*dx;
      if (dd == 0)
        continue;
      tau[0] = (da*(b1-y1) - db*(a1-x1))/dd;
      tau[1] = (dx*(b1-y1) - dy*(a1-x1))/dd;
      if (tau[0]>=0 && tau[0]<=1 && tau[1]>=0 && tau[1]<=1)
        return -8;
    }
  }    
  // sort points counterclockwise
  area = 0;
  for (i=1; i<np; i++) 
    area += (xp[i] - xp[i-1])*(yp[i] - yp[i-1]);
  B.np = np;
  B.xp = CALLOC(B.np, sizeof(float));
  B.yp = CALLOC(B.np, sizeof(float));
  if (area <= 0) {
    for (i=0; i<np; i++) {
      B.xp[i] = xp[i];
      B.yp[i] = yp[i];
    }
  }
  else {
    for (i=0; i<np; i++) {
      B.xp[i] = xp[np-i-1];
      B.yp[i] = yp[np-i-1];
    }
  }
  FREE(xp);
  FREE(yp);
  setBodyBorderValues(_b);  //-2004-12-14
  return 0;
}


//------------------------------------------------------------ Body_toString
static void Body_toString(BODY b, char *s) {
  int i;
  char ss[64];
  if (!s) return;
  if (b.name)
    sprintf(s, "body: %s\n", b.name);
  else
    sprintf(s, " ");
  for (i=0; i<b.np; i++) {
    sprintf(ss, "%2d x=%7.2f y=%7.2f\n", i, b.xp[i], b.yp[i]);
    strcat(s, ss);
  }
}
  
//------------------------------------------------------------ Body_isInside
int Body_isInside(BODY b, float x, float y, float z) {
  int n, i, l;
  float x0, y0, x1, y1;
  if (b.np <= 3 || b.xp == NULL || b.yp == NULL)
    return 0;
  n = b.np;
  // all body edges and corners belong to the body 2004-11-14
  if (z < b.floor || z > b.top)
    return 0;
  if (x<b.x0 || x>b.x1)   //-2004-12-14
    return 0;
  if (y<b.y0 || y>b.y1)   //-2004-12-14
    return 0;
  for (i=0; i<n; i++)     //-2004-12-14     
    if (x == b.xp[i] && y == b.yp[i]) 
      return 1;
  l = 0;
  for (i=1; i<n; i++) {
    x0 = b.xp[i-1];
    y0 = b.yp[i-1];
    x1 = b.xp[i];     
    y1 = b.yp[i];
    if      (x>x0 && x<=x1) {   //-2004-11-12
      if (y-y0 > (y1-y0)*(x-x0)/(x1-x0))  l++;
    }
    else if (x<=x0 && x>x1) {   //-2004-11-12
      if (y-y0 > (y1-y0)*(x-x0)/(x1-x0))  l++;
    }
  }
  if ((l % 2) == 0) {  //-2004-12-14 check opposite edges
    l = 0;
    for (i=1; i<n; i++) {
      x0 = b.xp[i-1];
      y0 = b.yp[i-1];
      x1 = b.xp[i];     
      y1 = b.yp[i];
      if      (x>=x0 && x<x1) {  
        if (y-y0 < (y1-y0)*(x-x0)/(x1-x0))  l++;
      }
      else if (x<x0 && x>=x1) {  
        if (y-y0 < (y1-y0)*(x-x0)/(x1-x0))  l++;
      }
    }
  }
  return ((l % 2) != 0);
}

//---------------------------------------------------------------- setBodyMatrix
static int setBodyMatrix(ARYDSC *_po, ARYDSC bds, GRID3D grd) { //-2005-01-04
  dP(setBodyMatrix);
  int i, j, k, l, l1, l2, nb, np, m, mx, my, mz, nx, ny, nz, n;
  BODY b;
  VEC3D pm;
  float x, y, z, dd2, x0, x1, y0, y1, z0, z1;
  nx = grd.nx;
  ny = grd.ny;
  nz = grd.nz;
  dd2 = grd.dx * 0.5;
  l1 = bds.bound[0].low;
  l2 = bds.bound[0].hgh;
  nb = l2 - l1 + 1;
  AryCreate(_po, sizeof(int), 3, 0, nx, 0, ny, 0, nz);               eG(1);
  if (nb == 0) return 0;
  b = *(BODY *)AryPtr(&bds, l1);
  x0 = b.x0;  
  x1 = b.x1;  
  y0 = b.y0;  
  y1 = b.y1; 
  z0 = b.floor;  
  z1 = b.top;  
  for (l=l1; l<=l2; l++) { 
    b = *(BODY *)AryPtr(&bds, l);
    if (b.x0 < x0) x0 = b.x0;
    if (b.x1 > x1) x1 = b.x1;
    if (b.y0 < y0) y0 = b.y0;
    if (b.y1 > y1) y1 = b.y1;
    if (b.floor < z0) z0 = b.floor;
    if (b.top > z1) z1 = b.top;
  } 
  n = 0;
  for (i=1; i<=nx; i++) {
    for (j=1; j<=ny; j++) {
      for (k=1; k<=nz; k++) { 
        np = 0;
        Grid3D_getMidPoint(grd, &pm, i, j, k);
        for (m=-1; m<8; m++) {
          if (m == -1) {
            x = pm.x;
            y = pm.y;
            z = pm.z;
          }
          else {
            mx = (0 != (m & 0x1)) ? 1 : 0;
            my = (0 != (m & 0x2)) ? 1 : 0;
            mz = (0 != (m & 0x4)) ? 1 : 0;
            x = pm.x + (0.5 - mx) * dd2;
            y = pm.y + (0.5 - my) * dd2;
            z = 0.5 * (pm.z + F(grd.hh, k-mz) - F(grd.hh, 0));
          }
          if (z < z0 || z > z1) continue;
          if (x < x0 || x > x1) continue;
          if (y < y0 || y > y1) continue;
          for (l=l1; l<=l2; l++) {
            b = *(BODY *)AryPtr(&bds, l);
            if (Body_isInside(b, x, y, z)) {
              np += (m == -1) ? 2 : 1;
              break;
            } 
          }
          if (np >= BDY_MINPINCELL) break;
        }
        *(int *)AryPtr(_po, i, j, k) = (np >= BDY_MINPINCELL);
        n += (np >= BDY_MINPINCELL);
      }
    }
  }
  return n;
eX_1: eMSG(_cant_allocate_array_);
}

//------------------------------------------------------------ DMKutil_BM2BM
int DMKutil_BM2BM(ARYDSC *_po, int nx, int ny, int nz, float x0, float y0, 
float dd, float *hh, ARYDSC ary, float x0i, float y0i, float ddi, float dzi) {
  dP(DMKutil_BM2BM);
  int i, j, k, l, n, nxi, nyi, nb, rc;
  float xb, yb;
  ARYDSC bds;
  GRID1D gx, gy, gz;
  GRID2D gxy;
  GRID3D grd;
  BODY b;
  if (!_po || _po->start)                                            eX(1);
  memset(&gx, 0, sizeof(GRID1D));
  memset(&gy, 0, sizeof(GRID1D));
  memset(&gz, 0, sizeof(GRID1D));
  memset(&gxy, 0, sizeof(GRID2D));
  memset(&grd, 0, sizeof(GRID3D));
  memset(&bds, 0, sizeof(ARYDSC));
  rc = 0;
  rc += Grid1D(&gx, x0, dd, nx); 
  rc += Grid1D(&gy, y0, dd, ny);
  rc += Grid1Dz(&gz, nz+1, hh);
  rc += Grid2D(&gxy, gx, gy);
  rc += Grid3D(&grd, gxy, gz);
  if (rc)                                                            eX(2);
  // count body cells
  nxi = ary.bound[0].hgh - ary.bound[0].low + 1;
  nyi = ary.bound[1].hgh - ary.bound[1].low + 1;
  nb = 0;
  for (i=0; i<nxi; i++) 
    for (j=0; j<nyi; j++) {
      k = *(int *)AryPtr(&ary, ary.bound[0].low+i, ary.bound[1].low+j);
      if (k) nb++;
    }
  AryCreate(&bds, sizeof(BODY), 1, 0, nb-1);                        eG(3);
  // define bodies
  l = 0;
  for (i=0; i<nxi; i++) {
    for (j=0; j<nyi; j++) {
      k = *(int *)AryPtr(&ary, ary.bound[0].low+i, ary.bound[1].low+j);
      if (k) {
        xb = x0i + i*ddi;
        yb = y0i + j*ddi; 
        rc = BodyRectangle((BODY *)AryPtr(&bds, l), 
          xb, yb, ddi, ddi, 0, k*dzi, 0);
        if (rc)                                                      eX(4);
        l++;
      }
    }
  }
  //-2005-01-04
  n = setBodyMatrix(_po, bds, grd);                                  eG(5);  
  for (i=0; i<nb; i++) {   //-2005-01-11
    b = *(BODY *)AryPtr(&bds, i);
    FREE(b.xp);
    FREE(b.yp);
  }
  AryFree(&bds);
  Grid3D_free(&grd);
  Grid2D_free(&gxy);
  Grid1D_free(&gx);
  Grid1D_free(&gy);
  Grid1D_free(&gz);
  return n;
eX_1: eX_2: eX_3: eX_4: eX_5: eMSG(_internal_error_);
}

//------------------------------------------------------------ DMKutil_B2BM
int DMKutil_B2BM(ARYDSC *_po, int nx, int ny, int nz, 
float x0, float y0, float dd, float *hh, ARYDSC bds) {
  dP(DMKutil_B2BM);
  int n, rc;
  GRID1D gx, gy, gz;
  GRID2D gxy;
  GRID3D grd;
  memset(&gx, 0, sizeof(GRID1D));
  memset(&gy, 0, sizeof(GRID1D));
  memset(&gz, 0, sizeof(GRID1D));
  memset(&gxy, 0, sizeof(GRID2D));
  memset(&grd, 0, sizeof(GRID3D));
  if (!_po || _po->start)                                            eX(1);
  rc = 0;
  rc += Grid1D(&gx, x0, dd, nx); 
  rc += Grid1D(&gy, y0, dd, ny);
  rc += Grid1Dz(&gz, nz+1, hh);
  rc += Grid2D(&gxy, gx, gy);
  rc += Grid3D(&grd, gxy, gz);
  if (rc)                                                            eX(2);
  //-2005-01-04
  n = setBodyMatrix(_po, bds, grd);                                  eG(3);
  Grid3D_free(&grd);
  Grid2D_free(&gxy);
  Grid1D_free(&gx);
  Grid1D_free(&gy);
  Grid1D_free(&gz);
  return n;
eX_1: eX_2: eX_3: eMSG(_internal_error_);
}
  
#undef X
#undef XX
#undef XXX
#undef FFF
#undef FF
#undef III
#undef F
#undef BBB
#undef G
#undef B
#undef W

//==============================================================================
//
// history:
//
//  2004-11-12 uj  Body_isInside corrected
//  2004-12-14 uj  include TalMat.h, use degree for wb, include body borders 
//  2005-01-04 uj  B2BM, BM2BM: loops reordered, speed improvements 
//  2005-01-11 uj  release memory
//  2005-05-12 lj  work around for compiler error in Grid2D_isFlat (gcc-l2)
//  2006-10-04 lj  branch eX(0) changed
//  2006-10-26 lj  external strings
//
//=============================================================================


  
