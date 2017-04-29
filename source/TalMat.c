// ================================================================ TalMat.c
//
// Mathematical functions used by AUSTAL2000
// =========================================
//
// Copyright (C) Umweltbundesamt, Dessau-Roﬂlau, Germany, 2002
// Copyright (C) Janicke Consulting, 88662 ‹berlingen, Germany, 2002
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

#include <math.h>
#include "TalMat.h"

  /*=================================================================== MatMul
  */
int MatMul(     /* Multiplication of two 3x3 matrices   */
  MATRIX x,     /* lefthand factor                      */
  MATRIX y,     /* righthand factor                     */
  MATRIX z )    /* product                              */
  {
  int i, j, k;
  float s;
  for (i=0; i<3; i++)
    for (j=0; j<3; j++) {
      s = 0.0;
      for (k=0; k<3; k++)  s += x[i][k]*y[k][j];
      z[i][j] = s; }
  return 0; }

  /*=================================================================== MatTrn
  */
int MatTrn(     /* Transpose a matrix   */
  MATRIX x,     /* source               */
  MATRIX y )    /* transposed matrix    */
  {
  int i, j;
  float s;
  for (i=0; i<3; i++) {
    y[i][i] = x[i][i];
    for (j=0; j<i; j++) {
      s = x[j][i];
      y[j][i] = x[i][j];
      y[i][j] = s; } 
    }
  return 0; }

  /*=================================================================== MatChl
  */
int MatChl(     /* Choleski separation of a 3x3 matrix          */
  MATRIX a,     /* original matrix                              */
  MATRIX l )    /* Choleski separation (lower left triangle)    */
  {
  float x;
  int i, j, k;
  for (i=0; i<3; i++)
    for (j=i; j<3; j++) {
      x = a[i][j];
      for (k=i-1; k>=0; k--)  x -= l[j][k]*l[i][k];
      if (i==j) 
        if (x < 0.0)  return -1;
        else l[i][i] = sqrt(x);
      else { 
        if (l[i][i] == 0.0)
          if (x != 0.0)  return -1;
          else  l[j][i] = 0.0;
        else  l[j][i] = x/l[i][i];
        l[i][j] = 0.0; } 
      }
  return 0; 
  }

  /*=================================================================== MatSet
  */
int MatSet(     /* set all elements of a matrix to zero */
  float w,      /* value of the diagonal elements       */
  MATRIX m )    /* matrix                               */
  {
  int i, j;
  for (i=0; i<3; i++)
    for (j=0; j<3; j++)  m[i][j] = (i==j) ? w : 0.0;
  return 0; 
  }

  /*==================================================================== MatEqu
  */
int MatEqu(     /* assign source to destination         */
  MATRIX s,     /* source                               */
  MATRIX d )    /* destination                          */
  {
  int i;
  float *ps, *pd;
  ps = s[0];
  pd = d[0];
  for (i=0; i<9; i++)  *pd++ = *ps++;
  return 0; 
  }

  /*====================================================================== Erfc
  */
float Erfc(       /* complementary error function = 1 - erf */
  float x )
  {
  float a, t, r, s, f;
  int i;
  a = (x < 0.0) ? -x : x;
  if (a < 2.7577) {
    t = 1.0/(1.0 + 0.3275911*a);
    r = t*(  0.254829592 +
        t*( -0.284496736 +
        t*(  1.421413741 +
        t*( -1.453152027 +
        t*   1.061405429 )))); }
  else {
    s = 1.0;
    f = 1.0/(2.0*a*a);
    t = 1.0;
    for (i=0; i<6; i++) { t *= -(2*i+1)*f;  s += t; }
    r = s*SQRT_PI/a; }
  if (a < 20.0) r *= exp( -a*a );
  else  r = 0.0;
  if (x < 0.0)  r = 2.0 - r;
  return r; 
  }

VECXYZ vXYZ( float x, float y, float z ) 
  {
  VECXYZ v;
  v.x = x;  v.y = y;  v.z = z;  return v; }
  
VECXYZ vMUL( float a, VECXYZ v ) 
  {
  v.x *= a;  v.y *= a;  v.z *= a;  return v; }
  
VECXYZ vADD( VECXYZ p, VECXYZ q )
  {
  p.x += q.x;  p.y += q.y;  p.z += q.z;  return p; }
  
VECXYZ vSUB( VECXYZ p, VECXYZ q )
  {
  p.x -= q.x;  p.y -= q.y;  p.z -= q.z;  return p; }

float vDOT( VECXYZ p, VECXYZ q )
  {
  return p.x*q.x + p.y*q.y + p.z*q.z; }  
  
//===========================================================================
//
// history:
//
// 2002-09-24 lj 1.0.0  final release candidate
//
//===========================================================================

