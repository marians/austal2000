// ================================================================ TalMat.h
//
// Mathematical functions used by AUSTAL2000
// =========================================
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
// last change: 2002-06-26 lj
//
//==========================================================================

#ifndef  TALMAT_INCLUDE
#define  TALMAT_INCLUDE

#define  SQRT2   1.414213562
#define  SQRTPI  1.772453851
#define  SQRT_PI 0.564189583
#define  RADIAN  57.2957795130823208768

#ifndef  PI
  #define  PI   3.14159265358979323846
#endif

typedef struct {
  float x, y, z;
  }  VECXYZ;
  
typedef float VECTOR[3];
typedef VECTOR* MATRIX;

typedef struct {
  float xx, xy, xz, yx, yy, yz, zx, zy, zz;
  } MAT;

  /*=================================================================== MatMul
  */
int MatMul(     /* Multiplication of two 3x3 matrices   */
  MATRIX x,     /* lefthand factor                      */
  MATRIX y,     /* righthand factor                     */
  MATRIX z )    /* product                              */
  ;
  /*=================================================================== MatTrn
  */
int MatTrn(     /* Transpose a matrix   */
  MATRIX x,     /* source               */
  MATRIX y )    /* transposed matrix    */
  ;
  /*=================================================================== MatChl
  */
int MatChl(     /* Choleski separation of a 3x3 matrix          */
  MATRIX a,     /* original matrix                              */
  MATRIX l )    /* Choleski separation (lower left triangle)    */
  ;
  /*=================================================================== MatSet
  */
int MatSet(     /* set all elements of a matrix to zero */
  float w,      /* value of the diagonal elements       */
  MATRIX m )    /* matrix                               */
  ;
  /*==================================================================== MatEqu
  */
int MatEqu(     /* assign source to destination         */
  MATRIX s,     /* source                               */
  MATRIX d )    /* destination                          */
  ;
  /*====================================================================== Erfc
  */
float Erfc(       /* complementary error function = 1 - erf */
  float x )
  ;
VECXYZ vXYZ( float x, float y, float z ) 
  ;
VECXYZ vMUL( float a, VECXYZ v ) 
  ;  
VECXYZ vADD( VECXYZ p, VECXYZ q )
  ;
VECXYZ vSUB( VECXYZ p, VECXYZ q )
  ;
float vDOT( VECXYZ p, VECXYZ q )
  ;  
/*===========================================================================*/
#endif
