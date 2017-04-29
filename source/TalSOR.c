// ================================================================= TalSOR.c
//
// SOR solver for DMK
// ==================
//
// Copyright (C) Umweltbundesamt, Dessau-Roﬂlau, Germany, 2004-2005
// Copyright (C) Janicke Consulting, 88662 ‹berlingen, Germany, 2004-2011
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
// last change: 2007-07-20 uj
//
//==========================================================================


/**
 * Solve the potential equation using successive overrelaxation. The 
 * main scope is the diagnostic windfield model where the divergence
 * of the windfield has to be removed. Odd-even ordering and Chebyshev
 * acceleration are applied. The value of the Jacobi radius may be
 * prescribed or is chosen by the program according to a theoretical
 * relation. The method <code>findRj()</code> is provided to 
 * search for the optimum value of <code>rj</code>.
 * 
 * The solution requires the following steps:
 * <br>
 * 1. Define grid and boundary conditions as Bound3D. The vertical
 * grid may have varying spacing.
 * <br>
 * 2. Build the raw wind field <code>U</code> with correct 
 * boundary conditions.
 * <br>
 * 3. Setup the SOR for the boundary conditions.
 * <br>
 * 4. Set <code>rr</code> to the divergence of <code>U</code>.
 * <br>
 * 5. Call <code>removeDiv(U)</code> to remove the divergence. 
 * The values in <code>U</code> are replaced by the values of the solution.
 * <br>
 */ 
 
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <math.h>

#include "IBJmsg.h"
#include "IBJary.h"
#include "TalDMKutil.h"
#include "TalSOR.h"
#include "TalSOR.nls"
#include "TalMat.h"   //-2004-12-14

static char *TalSORVersion = "2.6.0";

static char *eMODn = "TalSOR";
static int CHECK = 0; 
static char Options[4000] = ";";                                  //-2006-10-04

static int Sor_maxDev(SOR, ARYDSC, float *);
static int Sor_setMatrix(SOR *);
static int Sor_iterate(SOR *, int, int, float *);
static int Sor_getVx(SOR, ARYDSC *);
static int Sor_getVy(SOR, ARYDSC *);
static int Sor_getVz(SOR, ARYDSC *);
static int Sor_divergence(SOR, WIND3D, ARYDSC *);   
static int Sor_divergenceXYZ(SOR, ARYDSC, ARYDSC, ARYDSC, ARYDSC *);
static int Sor_setField(SOR, WIND3D *, float, float, float);
static void Sor_print(SOR, ARYDSC);


#define  XXX(a,b,c,d) (*(float*)AryPtr(&(a),(b),(c),(d)))
#define  X(a,b)       (*(float*)AryPtr(&(a),(b)))
#define  FFF(a,b,c,d) (*(float*)((char*)(a).start + (a).virof    \
                       + (b)*(a).bound[0].mul                    \
                       + (c)*(a).bound[1].mul                    \
                       + (d)*(a).bound[2].mul ))                   
#define  F(a,b)       (*(float*)((char*)(a).start+(a).virof      \
                       + (b)*(a).bound[0].mul ))    
#define  S          (*_sor)       
#define  W          (*_w) 


//----------------------------------------------------------------- SorOption

char *SorGetOptionString(void) {                                  //-2011-07-20
 return Options;
}

int SorOption(char *option) {                                     //-2006-10-04
  strcat(Options, option);
  strcat(Options, ";");
// printf("SOR %s\n", option);
  return 0;
}

static int GetOption(char *key, char *value) {                    //-2006-10-04
  char *pk, *ps, k[400];
  if (!key || !value) return 0;
  *value = 0;
  strcpy(k, ";");
  strcat(k, key);
  strcat(k, "=");
  pk = strstr(Options, k);
  if (!pk) return 0;
  pk += strlen(k);
  ps = strchr(pk, ';');
  if (ps)
    strncat(value, pk, ps-pk);
  else
    strcpy(value, pk);
  return 1;
}

//---------------------------------------------------------------------- Sor
/**
 * Set up a successive overrelaxation scheme for a given boundary
 * value problem.
 * @param _sor  pointer to the SOR object
 * @param bound boundary definition
 */
int Sor(SOR *_sor, BOUND3D bound) {
  dP(Sor);
  int k, rc;
  char t[100];                                                    //-2006-10-04
  if (!_sor)                                                         eX(10);
  if (S.a111.start || S.a011.start || S.a211.start || S.a101.start ||
      S.a121.start || S.a110.start || S.a112.start)                  eX(1);
  if (S.uu.start || S.rr.start)                                      eX(2);
  if (S.dz.start || S.dzs.start || S.zz.start)                       eX(3);
  S.b3d =  bound;
  S.g3d = S.b3d.grid;
  S.nx = S.g3d.nx;
  S.ny = S.g3d.ny;
  S.dx = S.g3d.dx;
  S.dy = S.g3d.dy;
  S.nz = S.g3d.nz;
  S.g1d = S.g3d.gridZ; 
  S.fdivmax = 15;
  S.ndivmax = 15;
  S.rj = -1;
  S.eps = SOR_EPS;
  S.maxIter = SOR_MAXITER;
  S.omega = 0;
  S.curIter = 0;
  S.ux = 1;
  S.uy = 0;
  S.uz = 0;
  S.check_rj = 0;
  S.nphi = 0;
  S.phi0 = 0;
  S.dphi = 0;
  S.rjmin = 0.8;
  S.rjmax = 0;
  S.qiter = 1.5;
  S.log = 3;                                                      //-2007-02-12
  S.dsp = 3;                                                      //-2007-02-12
  //------------------------------------------------------------- //-2006-10-04
  if (GetOption("utest", t))
    sscanf(t, "%f,%f,%f", &S.ux, &S.uy, &S.uz);
  if (GetOption("phi", t))
    sscanf(t, "%f,%f,%d", &S.phi0, &S.dphi, &S.nphi);
  if (GetOption("fdivmax", t))
    sscanf(t, "%f", &S.fdivmax);
  if (GetOption("ndivmax", t))
    sscanf(t, "%d", &S.ndivmax);
  if (GetOption("eps", t))
    sscanf(t, "%f", &S.eps);
  if (GetOption("rj", t))
    sscanf(t, "%f", &S.rj);
  if (GetOption("rjmin", t))
    sscanf(t, "%f", &S.rjmin);
  if (GetOption("qiter", t))
    sscanf(t, "%f", &S.qiter);
  if (GetOption("maxiter", t))
    sscanf(t, "%d", &S.maxIter);
  if (GetOption("check_rj", t))
    sscanf(t, "%d", &S.check_rj);
  if (GetOption("niter", t))
    sscanf(t, "%d", &S.niter);
  if (GetOption("ntry", t))
    sscanf(t, "%d", &S.ntry);
  if (GetOption("log", t))
    sscanf(t, "%d", &S.log);
  if (GetOption("dsp", t))
    sscanf(t, "%d", &S.dsp);
  //---------------------------------------------------------------------------
  rc = Grid1D_getXx(S.g1d, &S.zz); if (rc)                           eX(4);
  AryCreate(&S.dz, sizeof(float), 1, 0, S.nz);                       eG(5);
  AryCreate(&S.dzs, sizeof(float), 1, 0, S.nz);                      eG(5);
  for (k=1; k<=S.nz; k++)
    X(S.dzs, k) = X(S.zz, k) - X(S.zz, k-1);
  X(S.dz, 0) = X(S.dzs, 1);
  X(S.dz, S.nz) = X(S.dzs, S.nz);      
  for (k=1; k<S.nz; k++)
    X(S.dz, k) = 0.5 * (X(S.dzs, k) + X(S.dzs, k+1));
  AryCreate(&S.a111, sizeof(float), 3, 0, S.nx, 0, S.ny, 0, S.nz);   eG(6);
  AryCreate(&S.a011, sizeof(float), 3, 0, S.nx, 0, S.ny, 0, S.nz);   eG(6);
  AryCreate(&S.a211, sizeof(float), 3, 0, S.nx, 0, S.ny, 0, S.nz);   eG(6);
  AryCreate(&S.a101, sizeof(float), 3, 0, S.nx, 0, S.ny, 0, S.nz);   eG(6);
  AryCreate(&S.a121, sizeof(float), 3, 0, S.nx, 0, S.ny, 0, S.nz);   eG(6);
  AryCreate(&S.a110, sizeof(float), 3, 0, S.nx, 0, S.ny, 0, S.nz);   eG(6);
  AryCreate(&S.a112, sizeof(float), 3, 0, S.nx, 0, S.ny, 0, S.nz);   eG(6);
  rc = Sor_setMatrix(_sor); if (rc)                                  eX(7);
  return 0;
eX_10: eMSG(_null_SOR_);
eX_1: eMSG(_not_clean_);
eX_2: eMSG(_not_clean_);
eX_3: eMSG(_not_clean_);
eX_4: eMSG(_cant_read_xx_$_, rc);
eX_5: eMSG(_cant_allocate_1d_);
eX_6: eMSG(_cant_allocate_3d_);
eX_7: eMSG(_cant_set_matrix_);
}

void Sor_free(SOR *_sor) {
  if (!_sor) return;
  AryFree(&(_sor->a111));
  AryFree(&(_sor->a011));
  AryFree(&(_sor->a211));
  AryFree(&(_sor->a101));
  AryFree(&(_sor->a121));
  AryFree(&(_sor->a110));   
  AryFree(&(_sor->a112));
  AryFree(&(_sor->zz));
  AryFree(&(_sor->dz));
  AryFree(&(_sor->dzs));
  AryFree(&(_sor->uu));
  AryFree(&(_sor->rr));
}

//------------------------------------------------------------ Sor_setMatrix
/**
 * Set the values of the coefficient matrix. Actually the 
 * sparse matrix is stored as 7 3d-field.
 *
 * @param _sor  pointer to the SOR object
 */
static int Sor_setMatrix(SOR *_sor) {
  int i, j, k, m;
  float gz2, gz0, gx, gy;
  if (!_sor) return -1;
  gx = 1/(S.g3d.dx * S.g3d.dx);
  gy = 1/(S.g3d.dy * S.g3d.dy);
  for (k=1; k<=S.nz; k++) {
    gz2 = 1/(X(S.dz, k) * X(S.dzs, k));
    gz0 = 1/(X(S.dz, k-1) * X(S.dzs, k));
    for (j=1; j<=S.ny; j++) {
      for (i=1; i<=S.nx; i++) {
        m = Bound3D_getVolumeType(S.b3d, i, j, k);
        if ((m & BND_OUTSIDE) != 0) {  // out of computational area
          XXX(S.a111, i, j, k) = 0;
          continue;
        }
        if ((m & BND_BOUNDARY) == 0) { // no boundary condition
          XXX(S.a111, i, j, k) = -2*gx - 2*gy - gz0 - gz2;
          XXX(S.a011, i, j, k) = gx;
          XXX(S.a211, i, j, k) = gx;
          XXX(S.a101, i, j, k) = gy;
          XXX(S.a121, i, j, k) = gy;
          XXX(S.a110, i, j, k) = gz0;
          XXX(S.a112, i, j, k) = gz2;
          continue;
        }
        XXX(S.a111, i, j, k) = 0;
        //
        m = Bound3D_getAreaXType(S.b3d, i, j, k);
        if ((m & BND_BOUNDARY) != 0) {
          if ((m & BND_FIXED) == 0) 
            XXX(S.a111, i, j, k) -= 2*gx;
          XXX(S.a211, i, j, k) = 0;
        }
        else {
          XXX(S.a111, i, j, k) -= gx;
          XXX(S.a211, i, j, k) = gx;
        }
        m = Bound3D_getAreaXType(S.b3d, i-1, j, k);
        if ((m & BND_BOUNDARY) != 0) {
          if ((m & BND_FIXED) == 0) 
            XXX(S.a111, i, j, k) -= 2*gx;
          XXX(S.a011, i, j, k) = 0;
        }
        else {
          XXX(S.a111, i, j, k) -= gx;
          XXX(S.a011, i, j, k) = gx;
        }
        //
        m = Bound3D_getAreaYType(S.b3d, i, j, k);
        if ((m & BND_BOUNDARY) != 0) {
          if ((m & BND_FIXED) == 0) 
            XXX(S.a111, i, j, k) -= 2*gy;
          XXX(S.a121, i, j, k) = 0;
        }
        else {
          XXX(S.a111, i, j, k) -= gy;
          XXX(S.a121, i, j, k) = gy;
        }
        m = Bound3D_getAreaYType(S.b3d, i, j-1, k);
        if ((m & BND_BOUNDARY) != 0) {
          if ((m & BND_FIXED) == 0) 
            XXX(S.a111, i, j, k) -= 2*gy;
          XXX(S.a101, i, j, k) = 0;
        }
        else {
          XXX(S.a111, i, j, k) -= gy;
          XXX(S.a101, i, j, k) = gy;
        }
        //
        m = Bound3D_getAreaZType(S.b3d, i, j, k);
        if ((m & BND_BOUNDARY) != 0) {
          if ((m & BND_FIXED) == 0) 
            XXX(S.a111, i, j, k) -= 2*gz2;
          XXX(S.a112, i, j, k) = 0;
        }
        else {
          XXX(S.a111, i, j, k) -= gz2;
          XXX(S.a112, i, j, k) = gz2;
        }
        m = Bound3D_getAreaZType(S.b3d, i, j, k-1);
        if ((m & BND_BOUNDARY) != 0) {
          if ((m & BND_FIXED) == 0) 
            XXX(S.a111, i, j, k) -= 2*gz0;
          XXX(S.a110, i, j, k) = 0;
        }
        else {
          XXX(S.a111, i, j, k) -= gz0;
          XXX(S.a110, i, j, k) = gz0;
        }
      } // for i
    } // for j
  } // for k
  return 0;
}

          
//-------------------------------------------------------------- Sor_iterate         
/**
 * Successive overrelaxation using <code>rr</code> as residual and 
 * <code>uu</code> as initial
 * value and solution. Odd-even ordering and Chebyshev acceleration is
 * applied.
 * The iteration stops, if one of the following
 * conditions is met:
 * <br>
 * 1. The maximum number of iterations is reached
 * <br>
 * 2. The maximum residual is less then <code>dlimit</code>.
 * <br>
 * 3. For <code>ndivmax</code> iterations divergence is observed.
 * <br>
 * 4. The maximum residual has increased by more than <code>fdivmax</code>.
 * <br>
 * The value of omega is iterated in such a way, that this method
 * may be used repeatedly.
 *
 * @param   _sor  pointer to the SOR object
 * @param   nmax  maximum number of iterations
 * @param   ndev  size of float array pointed to by _dev
 * @param   dev   extreme values of the residual from the last iteration:
 *                <code>{ dabs, dsum, dmin, dmax }</code>
 * @return number of iterations or <0 in case of divergence or error
 */
static int Sor_iterate(SOR *_sor, int nmax, int ndev, float *dev) {
  dP(Sor_iterate);
  int n=0, nu=0, ndiv=0, mdiv=0, m, i, j, k, k0;
  float d, dsum=0, dmin=0, dabs=0, dmax=0, dref=0, fdiv=0;
  if (!_sor)                                                         eX(1);
  if (!S.rr.start)                                                   eX(2);
  if (!S.uu.start) {
    AryCreate(&S.uu, sizeof(float), 3, 0, S.nx, 0, S.ny, 0, S.nz);   eG(3);
    memset(S.uu.start, 0, sizeof(S.uu.ttlsz));
  }
  if (S.omega == 0)
    S.omega = 1;
  if (S.rj < 0)
    S.rj = 0;
  for (n=1; n<=nmax; n++) {
    nu = 0;
    dabs = 0;
    dsum = 0;
    dmin = 0;
    dmax = 0;
    for (m=0; m<=1; m++) { // odd-even ordering
      if (CHECK)
        printf("Sor_iterate: n=%d, m=%d, omega=%f\n", n, m, S.omega);
      for (i=1; i<=S.nx; i++) {
        for (j=1; j<=S.ny; j++) {
          k0 = 1 + (m+i+j) % 2;
          for (k=k0; k<=S.nz; k+=2) {
            if (FFF(S.a111, i,j,k) == 0) continue;
            d = FFF(S.a111, i,j,k)*FFF(S.uu, i,j,k) - FFF(S.rr, i,j,k);
            if (i > 1)    d += FFF(S.a011, i,j,k)*FFF(S.uu, i-1,j,k);
            if (i < S.nx) d += FFF(S.a211, i,j,k)*FFF(S.uu, i+1,j,k);
            if (j > 1)    d += FFF(S.a101, i,j,k)*FFF(S.uu, i,j-1,k);
            if (j < S.ny) d += FFF(S.a121, i,j,k)*FFF(S.uu, i,j+1,k);
            if (k > 1)    d += FFF(S.a110, i,j,k)*FFF(S.uu, i,j,k-1);
            if (k < S.nz) d += FFF(S.a112, i,j,k)*FFF(S.uu, i,j,k+1);
            XXX(S.uu, i,j,k) -= S.omega * d / FFF(S.a111, i,j,k);
             //
            if (d < dmin)  dmin = d;
            if (d > dmax)  dmax = d;
            d = fabs(d);
            if (d > dabs)  dabs = d;
            dsum += d;
            nu++;
          } // for k
        } // for j
      } // for i
      if (S.omega == 1)  
        S.omega = 1/(1 - 0.5*S.rj*S.rj);
      else 
        S.omega = 1/(1 - 0.25*S.omega*S.rj*S.rj);
    } // for m
    dsum /= nu;
    
    if (CHECK) 
      printf("Sor_iterate: n=%d, dmin=%e, dmax=%e, dsum=%e\n", n, dmin, dmax, dsum);
    if (CHECK) 
      printf("      dref=%e, dabs=%e, ndiv=%d\n", dref, dabs, ndiv);
    S.curIter++;
    if (dabs < S.dlimit)
      break;
    //
    if (dref > 0 && dabs > dref) {
      ndiv++;
      if (ndiv > mdiv)  
        mdiv = ndiv;
      if (dabs/dref > fdiv) 
        fdiv = dabs/dref;
    } 
    else {
      dref = dabs;
      ndiv = 0;
    }
    if (ndiv > S.ndivmax || dabs > S.fdivmax*dref) {
      n = -n;
      break;
    }
  } // for n  
  if (dev) {
    dev[0] = dabs;
    dev[1] = dsum;
    dev[2] = dmin;
    dev[3] = dmax;
    if (ndev > 4) 
      dev[4] = dref;
    if (ndev > 6) {
      dev[5] = mdiv;
      dev[6] = fdiv;
    }
  } 
  return n;
eX_1: eMSG(_null_SOR_);
eX_2: eMSG(_rr_not_initialized_);
eX_3: eMSG(_no_memory_);
}
  
//---------------------------------------------------------------- Sor_getVx
/**
 * Calculate the x-component of the gradient of <code>uu</code>. 
 * The boundary conditions are applied.
 * @param  s     SOR object
 * @param  _v    pointer to the array descriptor
 */
static int Sor_getVx(SOR s, ARYDSC *_a) {
  dP(Sor_getVx);
  int i, j, k, m;
  if (!_a || _a->start)                                             eX(1);
  AryCreate(_a, sizeof(float), 3, 0, s.nx, 0, s.ny, 0, s.nz);       eG(2);
  for (j=1; j<=s.ny; j++) {
    for (k=1; k<=s.nz; k++) {
      for (i=0; i<=s.nx; i++) {
        m = Bound3D_getAreaXType(s.b3d, i, j, k);
        if ((m & (BND_OUTSIDE | BND_FIXED)) != 0) {
          XXX(*(_a), i,j,k) = 0;
          continue;
        }
        if ((m & BND_BOUNDARY) != 0) {
          if (i == 0)
            XXX(*(_a), i,j,k) = -2 * XXX(s.uu, 1,j,k)/s.dx;
          else if (i == s.nx)
            XXX(*(_a), i,j,k) =  2 * XXX(s.uu, s.nx,j,k)/s.dx;
          else                                                      eX(3);
          continue;
        }
        XXX(*(_a), i,j,k) = -(XXX(s.uu, i+1,j,k)-XXX(s.uu, i,j,k))/s.dx;
      }
    }
  }
  return 0;
eX_1: eMSG(_null_array_);
eX_2: eMSG(_no_memory_); 
eX_3: eMSG(_invalid_boundary_$$$_, i, j, k);
}
   
//---------------------------------------------------------------- Sor_getVy
/**
 * Calculate the y-component of the gradient of <code>uu</code>. 
 * The boundary conditions are applied.
 * @param  s     SOR object
 * @param  _v    pointer to the array descriptor
 */
static int Sor_getVy(SOR s, ARYDSC *_a) {
  dP(Sor_getVy);
  int i, j, k, m;
  if (!_a || _a->start)                                             eX(1);
  AryCreate(_a, sizeof(float), 3, 0, s.nx, 0, s.ny, 0, s.nz);       eG(2);
  for (i=1; i<=s.nx; i++) {      
    for (k=1; k<=s.nz; k++) {
      for (j=0; j<=s.ny; j++) {
        m = Bound3D_getAreaYType(s.b3d, i, j, k);
        if ((m & (BND_OUTSIDE | BND_FIXED)) != 0) {
          XXX(*(_a), i,j,k) = 0;
          continue;
        }
        if ((m & BND_BOUNDARY) != 0) {
          if (j == 0)
            XXX(*(_a), i,j,k) = -2 * XXX(s.uu, i,1,k)/s.dy;
          else if (j == s.ny)
            XXX(*(_a), i,j,k) =  2 * XXX(s.uu, i,s.ny,k)/s.dy;
          else                                                      eX(3);
          continue;
        }
        XXX(*(_a), i,j,k) = -(XXX(s.uu, i,j+1,k)-XXX(s.uu, i,j,k))/s.dy;
      }
    }
  }
  return 0;
eX_1: eMSG(_null_array_);
eX_2: eMSG(_no_memory_); 
eX_3: eMSG(_invalid_boundary_$$$_, i, j, k);
}
  

//---------------------------------------------------------------- Sor_getVz
/**
 * Calculate the z-component of the gradient of <code>uu</code>. 
 * The boundary conditions are applied.
 * @param  s     SOR object
 * @param  _v    pointer to the array descriptor
 */
static int Sor_getVz(SOR s, ARYDSC *_a) {
  dP(Sor_getVz);
  int i, j, k, m;    
  if (!_a || _a->start)                                             eX(1);
  AryCreate(_a, sizeof(float), 3, 0, s.nx, 0, s.ny, 0, s.nz);       eG(2);
  for (j=1; j<=s.ny; j++) {
    for (i=1; i<=s.nx; i++) {      
      for (k=0; k<=s.nz; k++) {   
        m = Bound3D_getAreaZType(s.b3d, i, j, k);
        if ((m & (BND_OUTSIDE | BND_FIXED)) != 0) {
          XXX(*(_a), i,j,k) = 0;
          continue;
        }
        if ((m & BND_BOUNDARY) != 0) {
          if (k == 0)
            XXX(*(_a), i,j,k) = -2 * XXX(s.uu, i,j,1)/X(s.dz, k);
          else if (k == s.nz)
            XXX(*(_a), i,j,k) =  2 * XXX(s.uu, i,j,s.nz)/X(s.dz, k);
          else                                                      eX(3);
          continue;
        }
        XXX(*(_a),i,j,k) = -(XXX(s.uu,i,j,k+1)-XXX(s.uu,i,j,k))/X(s.dz,k);
      }
    }
  }
  return 0;
eX_1: eMSG(_null_array_);
eX_2: eMSG(_no_memory_); 
eX_3: eMSG(_invalid_boundary_$$$_, i, j, k);
}

//-------------------------------------------------------- Sor_divergenceXYZ
/**
 * Calculate the divergence of a wind field with given components.
 * @param s   SOR object
 * @param vx  x-component
 * @param vy  y-component
 * @param vz  z-component
 * @param _a  array pointer
 */
static int Sor_divergenceXYZ(SOR s, ARYDSC vx, ARYDSC vy, ARYDSC vz, ARYDSC *_a) {
  dP(Sor_divergenceXYZ);
  int i, j, k;
  float d;
  if (!_a || _a->start)                                              eX(1);
  AryCreate(_a, sizeof(float), 3, 0, s.nx, 0, s.ny, 0, s.nz);        eG(2);
  for (i=1; i<=s.nx; i++) {
    for (j=1; j<=s.ny; j++) {
      for (k=1; k<=s.nz; k++) {
        d = 0;
        d += (XXX(vx, i,j,k) - XXX(vx,i-1,j,k))*s.dy*X(s.dzs, k);
        d += (XXX(vy, i,j,k) - XXX(vy,i,j-1,k))*s.dx*X(s.dzs, k);
        d += (XXX(vz, i,j,k) - XXX(vz,i,j,k-1))*s.dx*s.dy;
        XXX(*(_a), i,j,k) = d/(s.dx*s.dy*X(s.dzs, k));
      }
    }
  }
  return 0;
eX_1: eMSG(_null_array_);
eX_2: eMSG(_no_memory_);   
}

//------------------------------------------------------------Sor_divergence
/**
 * Calculate the divergence of a wind field with given components.
 * @param s   SOR object
 * @param w   wind field
 * @param _a  array pointer
 */
static int Sor_divergence(SOR s, WIND3D w, ARYDSC *_a) {
  dP(Sor_divergence);
  int i, j, k;
  float d, dxy, dzs;
  if (!_a || _a->start)                                              eX(1);
  if (s.nx != w.nx || s.ny != w.ny || s.nz != w.nz)                  eX(2);
  if (s.dx != w.dx || s.dy != w.dy)                                  eX(3);
  dxy = s.dx * s.dy;
  for (k=0; k<=w.nz; k++)
    if (F(s.zz, k) != F(w.hh, k))                                    eX(4);
  AryCreate(_a, sizeof(float), 3, 0, s.nx, 0, s.ny, 0, s.nz);        eG(5);
  for (i=1; i<=s.nx; i++) {
    for (j=1; j<=s.ny; j++) {
      for (k=1; k<=s.nz; k++) {
        dzs = F(s.dzs, k);
        d = 0;
        d += (FFF(w.vx, i,j,k) - FFF(w.vx,i-1,j,k))*s.dy*dzs;
        d += (FFF(w.vy, i,j,k) - FFF(w.vy,i,j-1,k))*s.dx*dzs;
        d += (FFF(w.vz, i,j,k) - FFF(w.vz,i,j,k-1))*dxy;
        /*
        printf("i=%d, j=%d, k=%d, vx1=%f, vx2=%f, vy1=%f, vy2=%f, vz1=%f, vz2=%f\n",
        i, j, k, FFF(w.vx, i,j,k), FFF(w.vx, i-1,j,k), FFF(w.vy, i,j,k), FFF(w.vy, i,j-1,k), FFF(w.vz, i,j,k), FFF(w.vz, i,j,k-1));
        printf("    d=%e\n", d);
        */
        XXX(*(_a), i,j,k) = d/(dxy*dzs);
      }
    }
  }
  return 0;
eX_1: eMSG(_null_array_);
eX_2: eMSG(_inconsistent_cell_count_);
eX_3: eMSG(_inconsistent_cell_size_);
eX_4: eMSG(_inconsistent_vertical_grid_);
eX_5: eMSG(_no_memory_);   
}

//--------------------------------------------------------------- Sor_maxDev
static int Sor_maxDev(SOR s, ARYDSC rr, float *f) {
  float d, dmin=0, dmax=0, dabs=0, dsum=0;
  int i, j, k, nu=0;
  if (!rr.start) return -1;
  for (i=1; i<=s.nx; i++) {
    for (j=1; j<=s.ny; j++) {
      for (k=1; k<=s.nz; k++) {
        if (XXX(s.a111, i, j, k) == 0)  continue;
        d = -XXX(rr, i, j, k); 
        if (d < dmin)  dmin = d;
        if (d > dmax)  dmax = d;
        d = fabs(d);
        if (d > dabs)  dabs = d;
        dsum += d;
        nu++;
      }
    }
  }
  dsum /= nu;
  f[0] = dabs;
  f[1] = dsum;
  f[2] = dmin;
  f[3] = dmax;
  return 0;
}

//-------------------------------------------------------------------- print
static void Sor_print(SOR s, ARYDSC u) {
  int i, j, k;
  for (k=1; k<=s.nz; k++) {
    printf("k=%d\n", k);
    for (j=s.ny; j>=1; j--) {
      for (i=1; i<=s.nx; i++) 
        printf(" %9.6f", XXX(u, i, j, k));
      printf("\n");
    }
  }
}

//-------------------------------------------------------- Sor_theoreticalRj
/**
 * Estimate the value of <code>rj</code> according to an extension 
 * of the formula given in Numerical Recipies.
 * @return rj
 */
float Sor_theoreticalRj(SOR s) {
  float pi = PI;
  float ax = 1/(s.dx*s.dx);
  float ay = 1/(s.dy*s.dy);
  float az = 1/(X(s.zz, 1)*X(s.zz, 1));
  float rj = (ax*cos(pi/s.nx) + ay*cos(pi/s.ny) + az*cos(pi/s.nz))
         /(ax + ay + az);
  return rj;
}
  
//--------------------------------------------------------------- Sor_findRj
/**
 * Find the optimum value of <code>rj</code> with a standard value for the
 * number of iterations and the number of test runs.
 * @return optimum rj
 */
float Sor_findRj(SOR *_sor) {
  return Sor_findRj_2(_sor, 0, 0, NULL);
}

//------------------------------------------------------------- Sor_findRj_1
/**
 * Find the optimum value of <code>rj</code>.
 * A number of test runs is performed with varying values
 * of <code>rj</code> to find that value of <code>rj</code> which 
 * yields the least error (<code>dabs</code>) after a certain 
 * number of iterations. A homogeneous wind
 * field is used for the test runs. Therefore, the value of 
 * <code>rj</code> found depends on the boundary object only.
 * 
 * The number of iterations chosen should approximate the actual 
 * value of the final calculation. A first guess for eps=1.e-3
 * is half the average number of intervalls per axis for open
 * boundary conditions (with respect to the wind field). If the 
 * normal component of the wind field is prescribed the number of
 * iterations should be doubled.
 * 
 * If the optimum value of <code>rj</code> has to be determined 
 * with good accuracy
 * (for wind field libraries e.g.) the procedure can be iterated: 
 * <br>
 * 1. find <code>rj</code> with a guessed number of iterations.
 * <br>
 * 2. run a calculation with this value of <code>rj</code>.
 * <br>
 * 3. find <code>rj</code> again with the observed number of iterations.
 * <br>
 * A useful number of test runs is between 10 and 15.
 * <br>
 * @param _sor  pointer to SOR object
 * @param niter number of iterations
 * @param ntry number of test runs
 * @return optimum rj
 */
 
float Sor_findRj_1(SOR *_sor, int niter, int ntry) {
  return Sor_findRj_2(_sor, niter, ntry, NULL);
}

float Sor_findRj_2(SOR *_sor, int niter, int ntry, WIND3D *_win) {
  dP(Sor_findRj_2);
  int n, i, rc, check = 0, scan = 1;
  float tstRj[400], tstDd[400], dev[7];
  float estRj = Sor_theoreticalRj(S);
  float dr = 1 - estRj;
  float rj0=0, rj1=0, rj2=0, dd=0, dd0=0, dd1=0, dd2=0, rjmax=0;  //-2006-10-04
  WIND3D w3d;
  check = S.check_rj;                                             //-2006-10-04
  if (niter <= 0)  niter = (S.nx + S.ny + S.nz)/6;
  if (ntry <= 0)   ntry = 12;
  memset(&w3d, 0, sizeof(WIND3D));
  rc = Wind3D(&w3d, S.b3d); if (rc)                                   eX(1);
  if (!_win) {
    rc = Sor_setField(S, &w3d, S.ux, S.uy, S.uz); if (rc)             eX(2);
  }
  else {
    Wind3D_add(&w3d, *_win, 1.0);                                     eG(5);
  }           
  rc = Sor_divergence(S, w3d, &S.rr); if (rc)                         eX(3);
  S.rj = estRj;
  if (check)
    fprintf(MsgFile, "SOR: estimated rj=%8.8f, niter=%d, ntry=%d\n", estRj, niter, ntry);
  for (n=0; n<ntry; n++) {
    if (_sor->dsp > 0) {                                          //-2007-02-12
      fprintf(stdout, "T %2d\r", ntry-n);                         //-2006-10-04
      fflush(stdout);
    }
    S.omega = 1;
    S.eps = 1.e-6;
    S.curIter = 0;
    tstRj[n] = S.rj;
    memset(&S.uu, 0, sizeof(ARYDSC));
    i = Sor_iterate(_sor, niter, 7, dev);                 eG(4);  //-2006-10-04
    if (i > 0 && S.rj > rjmax)  rjmax = S.rj;                     //-2006-10-04
    dd = dev[0];
    if (check)
      fprintf(MsgFile, "SOR %3d: %9.6f (%10.5e) %9.6f (%10.5e) %9.6f (%10.5e) "
        "-> %9.6f (%10.5e) %d/%4d (%2.0f,%5.2f)\n", 
        n, rj2, dd2, rj1, dd1, rj0, dd0, S.rj, dd, scan, i, dev[5], dev[6]);
    tstDd[n] = dd;
    if (n == 0) {
      rj0 = S.rj;
      dd0 = tstDd[0];
      S.rj -= dr;
    }
    else if (n == 1) {
      rj1 = S.rj;
      dd1 = dd;
      if (dd < dd0) {
        S.rj -= 2*dr;
      }
      else {
        S.rj = 0.5*(rj0 + 1);
      }
    }
    else {  // n > 1
      if (scan) {
        if (S.rj < rj1) {
          if (dd < dd1) {
            rj0 = rj1;
            dd0 = dd1;
            rj1 = S.rj;
            dd1 = dd;
            if (rj1 < 0.5)  S.rj = 0.5*rj1;
            else S.rj = tstRj[n] - 2*(tstRj[n-1] - tstRj[n]);
            if (S.rj < 0)  S.rj = 0;
          }
          else {
            scan = 0;
            rj2 = S.rj;
            dd2 = dd;
            S.rj = 0.5*(rj2 + rj1);
          }
        }
        else if (S.rj > rj0) {
          if (dd > dd0) {
            scan = 0;
            rj2 = rj1;
            dd2 = dd1;
            rj1 = rj0;
            dd1 = dd0;
            rj0 = S.rj;
            dd0 = dd;
            S.rj = 0.5*(rj2 + rj1);
          }
          else {
            rj1 = rj0;
            dd1 = dd0;
            rj0 = S.rj;
            dd0 = dd;
            S.rj = 0.5*(rj0 + 1);
          }
        }
        else {  // rj1 < rj < rj0
          if (dd < dd0) {
            scan = 0;
            rj2 = rj1;
            dd2 = dd1;
            rj1 = S.rj;
            dd1 = dd;
            S.rj = 0.5*(rj0 + rj1);
          }
          else {
            rj1 = S.rj;
            dd1 = dd;
            S.rj = 0.5*(rj0 + rj1);
          }
        }
      }
      else {  // no scan
        if (S.rj < rj1) {
          if (dd < dd1) {
            rj0 = rj1;
            dd0 = dd1;
            rj1 = S.rj;
            dd1 = dd;
            S.rj = 0.5*(rj2 + rj1);
          }
          else { 
            rj2 = S.rj;
            dd2 = dd;
            S.rj = 0.5*(rj0 + rj1);
          }
        }
        else {  // rj > rj1
          if (dd < dd1) {
            rj2 = rj1;
            dd2 = dd1;
            rj1 = S.rj;
            dd1 = dd;
            S.rj = 0.5*(rj2 + rj1);
          }
          else {
            rj0 = S.rj;
            dd0 = dd;
            S.rj = 0.5*(rj2 + rj1);
          }
        }
      } // if else scan
    } // n>1
    AryFree(&S.uu);                                               //-2005-01-11
  } // for n    
  if (_sor->dsp > 0) {                                            //-2007-02-12
    fprintf(stdout, "    \r");                                    //-2006-10-04
    fflush(stdout);
  }
  if (check) {
    fprintf(MsgFile, "SOR: rj=%9.6f\n", S.rj);
    for (n=0; n<ntry; n++)
      fprintf(MsgFile, "SOR: %2d %9.6f %10.5e\n", n, tstRj[n], tstDd[n]);
  }
  Wind3D_free(&w3d);
  S.rj = rj1;                                                     //-2006-10-04
  S.rjmax = rjmax;                                                //-2006-10-04
  return rj1;
eX_1: eMSG(_cant_init_wind_);
eX_2: eX_5: eMSG(_cant_set_wind_);
eX_3: eMSG(_cant_calculate_div_);  
eX_4: eMSG(_cant_iterate_);
}

//------------------------------------------------------------ Sor_removeDiv
/**
 * Remove the divergence of the wind field. The value of <code>rj</code> 
 * is set to <code>theoreticalRj()</code> if not set by the user. The value 
 * of <code>eps</code> is used
 * to calculate dlimit from the starting residual. <code>nmax</code> is set 
 * to <code>2*(nx+ny+nz)</code>.
 *
 * @param _sor  pointer to SOR object
 * @param _w    pointer to wind field
 * @param cp    pointer to info string
 */
int Sor_removeDiv(SOR *_sor, WIND3D *_w, char *cp) {
  dP(Sor_removeDiv);
  int i, j, k, n, nmax, rc;
  float dm[4], dd[7];
  ARYDSC uu;
  if (!_sor || !_w)                                                  eX(1);
  if (!cp)                                                           eX(2);
  if (S.nx != W.nx || S.ny != W.ny || S.nz != W.nz)                  eX(3);
  if (S.dx != W.dx || S.dy != W.dy)                                  eX(4);
  for (k=0; k<=W.nz; k++)
    if (X(S.zz, k) != X(W.hh, k))                                    eX(5);
  rc = Sor_divergence(S, W, &S.rr); if (rc)                          eX(6);
  Sor_maxDev(S, S.rr, dm); //   if (rc)                              eX(7);
  S.dlimit = S.eps * dm[0];
  if (S.omega <= 0) {
    S.omega = 1;
    if (S.rj < 0) 
      S.rj = Sor_theoreticalRj(S);
  }
  nmax = S.maxIter;
  if (nmax <= 0) 
    nmax = 2 * (S.nx + S.ny + S.nz);
  //
  S.curIter = 0;
  n = Sor_iterate(_sor, nmax, 7, dd);                     eG(8);  //-2006-10-04
  sprintf(cp, "%s rj=%7.5f; n=%2d; dabs=%10.3e; mdiv=%2d; fdiv=%7.5f",
    (n<0) ? "*** " : "", S.rj, n, dd[0], (int)dd[5], dd[6]);
  //
  if (n < 0)  return n;                                           //-2006-10-10
  //
  memset(&uu, 0, sizeof(ARYDSC));
  rc = Sor_getVx(S, &uu); if (rc)                                    eX(10);
  for (i=0; i<=S.nx; i++)
    for (j=1; j<=S.ny; j++) 
      for (k=1; k<=S.nz; k++)
        XXX(W.vx, i, j, k) += XXX(uu, i, j, k); 
  AryFree(&uu);                                                   //-2005-01-11
  rc = Sor_getVy(S, &uu); if (rc)                                    eX(11);
  for (i=1; i<=S.nx; i++)
    for (j=0; j<=S.ny; j++) 
      for (k=1; k<=S.nz; k++)
        XXX(W.vy, i, j, k) += XXX(uu, i, j, k); 
  AryFree(&uu);                                                   //-2005-01-11
  rc = Sor_getVz(S, &uu); if (rc)                                    eX(12);
  for (i=1; i<=S.nx; i++)
    for (j=1; j<=S.ny; j++) 
      for (k=0; k<=S.nz; k++)
        XXX(W.vz, i, j, k) += XXX(uu, i, j, k); 
  AryFree(&uu);  
  return n;                                                       //-2006-10-04
eX_1: eMSG(_null_SOR_);
eX_2: eMSG(_null_string_);
eX_3: eMSG(_inconsistent_cell_count_);
eX_4: eMSG(_inconsistent_cell_size_);
eX_5: eMSG(_inconsistent_vertical_grid_);
eX_6: eMSG(_cant_get_div_);  
// eX_7: eMSG("can't get maxDiv");  
eX_8: eMSG(_cant_iterate_);  
eX_10: eMSG(_no_vx_);
eX_11: eMSG(_no_vy_);
eX_12: eMSG(_no_vz_);
}


//------------------------------------------------------------- Sor_setField
static int Sor_setField(SOR s, WIND3D *_w, float ux, float uy, float uz) {
  dP(Sor_setField);
  int i, j, k, m;
  if (!_w)                                                           eX(1);
  if (s.nx != W.nx || s.ny != W.ny || s.nz != W.nz)                  eX(2);
  if (s.dx != W.dx || s.dy != W.dy)                                  eX(3);
  for (k=0; k<=W.nz; k++)
    if (X(s.zz, k) != X(W.hh, k))                                    eX(4);
  for (i=0; i<=s.nx; i++) {
    for (j=1; j<=s.ny; j++) {
      for (k=1; k<=s.nz; k++) {
        m = Bound3D_getAreaXType(s.b3d, i, j, k);
        if (i == 0 || i == s.nx) m &= ~BND_FIXED;
        if ((m & (BND_OUTSIDE | BND_FIXED)) == 0)
          XXX(W.vx, i, j, k) = ux;
        else 
          XXX(W.vx, i, j, k) = 0;
      }
    }
  }
  for (i=1; i<=s.nx; i++) {
    for (j=0; j<=s.ny; j++) {
      for (k=1; k<=s.nz; k++) {
        m = Bound3D_getAreaYType(s.b3d, i, j, k);
        if (j == 0 || j == s.ny) m &= ~BND_FIXED;
        if ((m & (BND_OUTSIDE | BND_FIXED)) == 0)
          XXX(W.vy, i, j, k) = uy;
        else 
          XXX(W.vy, i, j, k) = 0;
      }
    }
  }
  for (i=1; i<=s.nx; i++) {
    for (j=1; j<=s.ny; j++) {
      for (k=0; k<=s.nz; k++) {
        m = Bound3D_getAreaZType(s.b3d, i, j, k);
        if (k == s.nz)  m &= ~BND_FIXED;
        if ((m & (BND_OUTSIDE |BND_FIXED)) == 0) 
          XXX(W.vz, i, j, k) = uz;
        else 
          XXX(W.vz, i, j, k) = 0;
      }
    }
  }  
  return 0;
eX_1: eMSG(_null_wio3d_);
eX_2: eMSG(_inconsistent_cell_count_);
eX_3: eMSG(_inconsistent_cell_size_);
eX_4: eMSG(_inconsistent_vertical_grid_);
}

/*###################################################################*/
#ifdef MAIN

static void Sor_test1() {
  dP(test1);
  int i, j, n, ni, nx=5, ny=5, nz=5;
  float dx=2, dy=2, dz=2;
  float zz[] = { 0, 2, 4, 8, 10, 20 };
  float dev[5], de[4];
  GRID1D gx, gy, gz;
  GRID2D gxy;
  GRID3D g;
  BOUND3D b;
  ARYDSC vx, vy, vz, dd;
  SOR sor;
  memset(&dd, 0, sizeof(ARYDSC));     
  memset(&vx, 0, sizeof(ARYDSC));     
  memset(&vy, 0, sizeof(ARYDSC));
  memset(&vz, 0, sizeof(ARYDSC));
  memset(&gx, 0, sizeof(GRID1D));
  memset(&gy, 0, sizeof(GRID1D));
  memset(&gz, 0, sizeof(GRID1D));
  memset(&gxy, 0, sizeof(GRID2D));
  memset(&g, 0, sizeof(GRID3D));
  memset(&b, 0, sizeof(BOUND3D));
  memset(&sor, 0, sizeof(SOR));
  Grid1D(&gx, 0, dx, nx);
  Grid1D(&gy, 0, dy, ny);
  //Grid1D(&gz, 0, dz, nz);    
  Grid1Dz(&gz, 6, zz);
  Grid2D(&gxy, gx, gy);
  Grid3D(&g, gxy, gz);
  Bound3D(&b, g);
  Bound3D_complete(&b);
  Sor(&sor, b);
  AryCreate(&sor.rr, sizeof(float), 3, 0, nx, 0, ny, 0, nz);     
  for (i=1; i<=nx; i++)
    for (j=1; j<=ny; j++)
      XXX(sor.rr, i, j, 1) = -1/dz;
  Sor_maxDev(sor, sor.rr, de);
  sor.eps = 1.e-3;
  sor.dlimit = sor.eps*de[0];
  printf("limit=%f\n", sor.dlimit);
  sor.rj = 0.81;
  for (n=0,ni=0; n<20; n++) {
    ni = Sor_iterate(&sor, 1, 5, dev);
    printf("%3d : %9.6f %9.6f %9.6f %9.6f\n",
      n+1, dev[0], dev[1], dev[2], dev[3]);
    if (ni <= 0) break;
  }
  printf("n=%d, ni=%d\n", n, ni);
  Sor_print(sor, sor.uu);
  Sor_getVx(sor, &vx);
  Sor_getVy(sor, &vy);
  Sor_getVz(sor, &vz);
  Sor_divergenceXYZ(sor, vx, vy, vz, &dd);
  printf("divergence:\n");
  Sor_print(sor, dd);
}

static void Sor_test2() {
  dP(test2);
  int i, j, n, ni, nx=5, ny=5, nz=5;
  float dx=2, dy=2, dz=2, d;
  float dev[5], de[4];
  char msg[256];
  GRID1D gx, gy, gz;
  GRID2D gxy;
  GRID3D g;
  BOUND3D b;
  ARYDSC vx, vy, vz, dd, dl;
  WIND3D w3d;
  SOR sor;
  memset(&w3d, 0, sizeof(WIND3D));     
  memset(&dd, 0, sizeof(ARYDSC));     
  memset(&dl, 0, sizeof(ARYDSC));     
  memset(&vx, 0, sizeof(ARYDSC));     
  memset(&vy, 0, sizeof(ARYDSC));
  memset(&vz, 0, sizeof(ARYDSC));
  memset(&gx, 0, sizeof(GRID1D));
  memset(&gy, 0, sizeof(GRID1D));
  memset(&gz, 0, sizeof(GRID1D));
  memset(&gxy, 0, sizeof(GRID2D));
  memset(&g, 0, sizeof(GRID3D));
  memset(&b, 0, sizeof(BOUND3D));
  memset(&sor, 0, sizeof(SOR));
  Grid1D(&gx, 0, dx, nx);
  Grid1D(&gy, 0, dy, ny);
  Grid1D(&gz, 0, dz, nz);
  Grid2D(&gxy, gx, gy);
  Grid3D(&g, gxy, gz);
  Bound3D(&b, g);
  Bound3D_setVolout(&b, 3, 3, 3, BND_FIXED);
  Bound3D_complete(&b);
  Wind3D(&w3d, b); 
  Sor(&sor, b);
  Sor_setField(sor, &w3d, 1, 1, 0);
  memset(&sor.rr, 0, sizeof(ARYDSC));
  Sor_divergence(sor, w3d, &sor.rr);
  Sor_print(sor, sor.rr);
  //
  sor.eps = 1.e-3;
  memset(&sor.rr, 0, sizeof(ARYDSC));
  Sor_removeDiv(&sor, &w3d, msg);
  printf("msg=%s\n", msg);  
  Wind3D_maxDivergence(w3d, &d);
  printf("maxDiv=%e\n", d);
  //
  printf("lambda:\n");
  Sor_print(sor, sor.uu);
  //
  Sor_getVx(sor, &vx);
  printf("vx:\n");
  Sor_print(sor, vx);
  Sor_getVy(sor, &vy);
  printf("vy:\n");
  Sor_print(sor, vy);
  Sor_getVz(sor, &vz);
  printf("vz:\n");
  Sor_print(sor, vz);
  Sor_divergenceXYZ(sor, vx, vy, vz, &dl);
  printf("divergence lambda:\n");
  Sor_print(sor, dl);
  //
  Sor_divergence(sor, w3d, &dd);
  printf("divergence:\n");
  Sor_print(sor, dd);
  //
  printf("%s\n", msg);
}
  

static void Sor_test4(int fix) {
  dP(test4);
  int i, j, k, n, nx=20, ny=20, nz=14, rc;
  float zz[] = { 0, 3, 6, 9, 12, 15, 18, 21, 24, 27, 30, 35, 40, 50, 60 };
  float dx=5, dy=5, d, dda[7];
  char msg[256];
  GRID1D gx, gy, gz;
  GRID2D gxy;
  GRID3D g;
  BOUND3D b;
  ARYDSC vx, vy, vz, dd, dl;
  WIND3D w3d;
  SOR sor;
  FILE *fp;
  memset(&w3d, 0, sizeof(WIND3D));     
  memset(&dd, 0, sizeof(ARYDSC));     
  memset(&dl, 0, sizeof(ARYDSC));     
  memset(&vx, 0, sizeof(ARYDSC));     
  memset(&vy, 0, sizeof(ARYDSC));
  memset(&vz, 0, sizeof(ARYDSC));
  memset(&gx, 0, sizeof(GRID1D));
  memset(&gy, 0, sizeof(GRID1D));
  memset(&gz, 0, sizeof(GRID1D));
  memset(&gxy, 0, sizeof(GRID2D));
  memset(&g, 0, sizeof(GRID3D));
  memset(&b, 0, sizeof(BOUND3D));
  memset(&sor, 0, sizeof(SOR));
  rc = 0;
  rc += Grid1D(&gx, 0, dx, nx);
  rc += Grid1D(&gy, 0, dy, ny);
  rc += Grid1Dz(&gz, 15, zz);
  rc += Grid2D(&gxy, gx, gy);
  rc += Grid3D(&g, gxy, gz);
  rc += Bound3D(&b, g);  
  
  for (i=8; i<=12; i++) {
    for (j=8; j<=12; j++) {
      for (k=1; k<=5; k++) {
        rc += Bound3D_setVolout(&b, i, j, k, BND_FIXED);
      }
    }
  }
  if (fix) {
    for (j=1; j<=ny; j++) {
      for (k=1; k<=nz; k++) {
        rc += Bound3D_setFixedX(&b, 0, j, k, 1);
        rc += Bound3D_setFixedX(&b, nx, j, k, 1);
      }
    }
    for (i=1; i<=nx; i++) {
      for (k=1; k<=nz; k++) {
        rc += Bound3D_setFixedY(&b, i,  0, k, 1);
        rc += Bound3D_setFixedY(&b, i, ny, k, 1);
      }
    }
  }
  rc += Bound3D_complete(&b);
  //
  rc += Sor(&sor, b);
  sor.eps = 1.e-3;
  sor.maxIter = 200;
  for (n=0; n<20; n++) {
    sor.rj = 0.98 + n*0.001;
    sor.omega = 1;
    Wind3D_free(&w3d);
    memset(&w3d, 0, sizeof(WIND3D));
    rc += Wind3D(&w3d, b);
    Sor_setField(sor, &w3d, 1, 0, 0);
    AryFree(&sor.rr);
    AryFree(&sor.uu);
    rc += Sor_removeDiv(&sor, &w3d, msg);
    printf("%d rj=%f, %s", n, sor.rj, msg);
  }
  printf("msg=%s", msg);
  rc += Wind3D_maxDivergence(w3d, &d);
  printf("maxDiv=%e\n", d);
  memset(&dd, 0, sizeof(ARYDSC));
  rc += Sor_divergence(sor, w3d, &dd);
  printf("divergence:\n");
  Sor_print(sor, dd);
  Wind3D_printZ(w3d, 1, NULL);
  fp = fopen("/java/csor-test4-wind.dmna", "wb");
  rc += Wind3D_writeDMNA(w3d, fp, "%7.3f");
  printf("-- fin: rc=%d\n", rc);
  
}
  
#undef XXX
#undef X
#undef F
#undef S
#undef W

int main( int argc, char *argv[] ) {
  //Sor_test1();
  //Sor_test4(1); 
  Sor_test2();
  return 0;
}

#endif  /*###################################################################*/

//============================================================================
//
// history:
//
// 2005-01-11 uj 2.1.1  release memory
// 2005-03-10 uj 2.1.15 control flush when calculating rj
// 2005-03-17 uj 2.2.0  version number upgrade
// 2006-10-04 lj 2.2.15 external access to iteration parameters
//                      test field changed for calculation of rj
// 2006-10-26 lj 2.3.0  external strings
// 2006-10-02 lj 2.3.1  display switched by S.dsp
// 2011-07-07 uj 2.5.0  version number upgrade
// 2011-07-20           SorGetOptionString
// 2012-04-06 uj 2.6.0  version number upgrade
//
//=============================================================================


