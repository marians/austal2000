// ================================================================= TalDMK.c
//
// Diagnostic microscale wind field model DMK for bodies
// =====================================================
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
// last change: 2011-07-20 uj
//
//==========================================================================

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <math.h>
#include <locale.h>                                               //-2008-10-20

#include "IBJmsg.h"
#include "IBJary.h"
#include "IBJtxt.h"
#include "IBJdmn.h"
#include "TalDMKutil.h"
#include "TalSOR.h"
#include "TalWnd.h"
#include "TalDMK.h"
#include "TalDMK.nls"
#include "TalMat.h"  //-2004-12-14
#include "IBJstd.h"

#define PRM_F2           0.4
#define PRM_GRANULAR     0.5
#define SHADOW_SHIFT     0.3
#define SHADOW_MINSV     3
#define SHADOW_CORE      2
#define SHADOW_EXT       1

//============================================================================

STDPGM(DMK, DMKserver, 2, 6, 0);

//============================================================================

//char *DMKVERS = StdVString;                                     //-2006-01-05
float DMKfdivmax=-1, DMKndivmax=-1;                             //-2005-11-09

static char *eMODn = "TalDMK";
static DMKDAT P;
static WIND3D Winit, Winit4Shadow, Wind, Rfield;
static ARYDSC Shadow, Visible, Sfield, Kfield;
static BOUND3D Bound;

static int setRfield(void);
static int cropRfield(void);
static int setVisible(RECTANGLE);
static short getVisibility(VEC3D, int, int, int, float);
static int getFaces(ARYDSC *);
static int addRfield(VEC3D, VEC3D, float);
static int setBoundaries(ARYDSC *, int);
static int setWinit(ARYDSC *);
static int setShadow(BOUND3D, int);
static int setTfields(ARYDSC, ARYDSC);
static float getMeanBuildingHeight(void);                       //-2005-05-25

#define  X(a,b)    (*(float*)AryPtr(&(a),(b)))
#define  F(a,b)    (*(float*)((char*)(a).start+(a).virof                \
                      + (b)*(a).bound[0].mul ))
#define  FF(a,b,c)   (*(float*)((char*)(a).start + (a).virof    \
                       + (b)*(a).bound[0].mul +                  \
                       + (c)*(a).bound[1].mul ))
#define  XXX(a,b,c,d) (*(float*)AryPtr(&(a),(b),(c),(d)))
#define  FFF(a,b,c,d) (*(float*)((char*)(a).start + (a).virof           \
                       + (b)*(a).bound[0].mul +                         \
                       + (c)*(a).bound[1].mul +                         \
                       + (d)*(a).bound[2].mul ))
#define  III(a,b,c,d) (*(int*)((char*)(a).start + (a).virof             \
                       + (b)*(a).bound[0].mul +                         \
                       + (c)*(a).bound[1].mul +                         \
                       + (d)*(a).bound[2].mul ))
#define  SHADOW(a,b,c) (*(short*)((char*)Shadow.start + Shadow.virof    \
                       + (a)*Shadow.bound[0].mul +                      \
                       + (b)*Shadow.bound[1].mul +                      \
                       + (c)*Shadow.bound[2].mul ))
#define  VISIBLE(a,b,c) (*(short*)((char*)Visible.start + Visible.virof \
                       + (a)*Visible.bound[0].mul +                     \
                       + (b)*Visible.bound[1].mul +                     \
                       + (c)*Visible.bound[2].mul ))
#define  INBODY(a,b,c) ( ((III(Bound.bb, (a), (b), (c)) & BND_VOLMSK) \
                          >> BND_VOLSHIFT) & BND_OUTSIDE )

//-------------------------------------------------------------------- DMKinit
long DMKinit(long flags, char *istr) {
  char *jstr, *ps, t[400];
  StdDspLevel = 1;                                               //-2005-02-15
  if (StdStatus & STD_INIT)  return 0;
  if (istr) {
    jstr = istr;
    ps = strstr(istr, " -v");
    if (ps) sscanf(ps+3, "%d", &StdLogLevel);
    ps = strstr(istr, " -y");
    if (ps) sscanf(ps+3, "%d", &StdDspLevel);
//    ps = strstr(istr, " -f");                                    //-2005-02-15
//    if (ps) sscanf(ps+3, "%ld", &MsgFile);
  }
  vLOG(3)("DMK_%d.%d.%s (%08lx,%s)",                             //-2007-02-12
    StdVersion, StdRelease, StdPatch, flags, istr);
  sprintf(t, "dsp=%d", StdDspLevel);                             //-2007-02-12
  SorOption(t);                                                  //-2007-02-12
  return 0;
}

//------------------------------------------------------ DMK_calculateWField
/**
 * @param p   structure with input parameters to be used
 * @param _wi pointer to the initial wind field
 * @param _bi pointer to the body matrix
 * @param _wo allocated pointer to which the diagnostic wind field is written
 * @param _ko allocated pointer to which the add. velocity fluct. are written
 * @param _so allocated pointer to which the add. diffusion coeff. are written
 * @return 0 if calculation succeeded, <0 otherwise
 **/
int DMK_calculateWField(DMKDAT *_p, ARYDSC *_wi, ARYDSC *_bi,
ARYDSC *_wo, ARYDSC *_ko, ARYDSC *_so, int fixed) {
  dP(calculateWField);
  int i, j, k, m, rc;
  float maxdiv;
  char msg[256];
  BOUND3D binit;
  GRID3D grd;
  SOR sor;
  P = *_p;
  vLOG(4)("DMK: a1=%1.2f,a2=%1.2f,a3=%1.2f,a4=%1.2f,a5=%1.2f,da=%1.0f,fk=%1.2f,fs=%1.2f,hs=%1.2f",
  P.a1, P.a2, P.a3, P.a4, P.a5, P.da, P.fk, P.fs, P.hs); //-2005-05-06
  if (!_wi)                                                         eX(50);
  if (!_bi)                                                         eX(51);
  if (_wi->elmsz != sizeof(WNDREC) || _bi->elmsz != sizeof(int))    eX(55);
  if (_bi->numdm != _wi->numdm)                                     eX(55);
  for (i=0; i<_bi->numdm; i++)
    if ((_bi->bound[i].low != _wi->bound[i].low) ||
        (_bi->bound[i].hgh != _wi->bound[i].hgh))                   eX(55);
  memset(&Bound, 0, sizeof(BOUND3D));
  memset(&Winit, 0, sizeof(WIND3D));
  memset(&Winit4Shadow, 0, sizeof(WIND3D));
  memset(&Wind, 0, sizeof(WIND3D));
  memset(&Rfield, 0, sizeof(WIND3D));
  memset(&Shadow, 0, sizeof(ARYDSC));
  memset(&Visible, 0, sizeof(ARYDSC));
  memset(&Sfield, 0, sizeof(ARYDSC));
  memset(&Kfield, 0, sizeof(ARYDSC));
  //
  // copy input array to Winit and set boundaries
  rc = setWinit(_wi); if (rc)                                        eX(1);
  binit = Winit.bound;
  rc = Bound3D_copy(&Bound, binit); if (rc)                          eX(43);
  rc = setBoundaries(_bi, fixed); if (rc)                            eX(2);
  rc = Wind3D(&Wind, Bound); if (rc)                                 eX(3);
  rc = Wind3D(&Rfield, Bound); if (rc)                               eX(3);
  grd = Bound.grid;
  AryCreate(&Shadow, sizeof(short),3,0,grd.nx,0,grd.ny,0, grd.nz);   eG(4);
  AryCreate(&Visible, sizeof(short),3,0,grd.nx,0,grd.ny,0, grd.nz);  eG(4);
  AryCreate(&Sfield, sizeof(float),3,0,grd.nx,0,grd.ny,0, grd.nz);   eG(4);
  AryCreate(&Kfield, sizeof(float),3,0,grd.nx,0,grd.ny,0, grd.nz);   eG(4);
  //
  // initialize Wind
  for (i=0; i<=grd.nx; i++)
    for (j=0; j<=grd.ny; j++)
      for (k=0; k<=grd.nz; k++) {
        if (j>0 && k>0)                                           //-2011-06-29
          XXX(Wind.vx, i, j, k) = FFF(Winit.vx, i, j, k);
        else
          XXX(Wind.vx, i, j, k) = 0;
        if (i>0 && k>0)
          XXX(Wind.vy, i, j, k) = FFF(Winit.vy, i, j, k);
        else
          XXX(Wind.vy, i, j, k) = 0;
        if (i>0 && j>0)
          XXX(Wind.vz, i, j, k) = FFF(Winit.vz, i, j, k);
        else
          XXX(Wind.vz, i, j, k) = 0;
      }
  //
  // insert recirculation field
  rc = setShadow(Bound, 0); if (rc)                                  eX(20);
  rc = setRfield(); if (rc)                                          eX(21);
  if (!_wo) goto save;
  rc = Wind3D_add(&Wind, Rfield, 1); if (rc)                         eX(40);
  rc = Wind3D_clearBoundaryValues(&Wind); if(rc)                     eX(5);
  //
  // calculate diagnostic wind field
  rc = Wind3D_maxDivergence(Wind, &maxdiv); if (rc)                  eX(44);
  vLOG(3)(_starting_with_divergence_$_, maxdiv);
  memset(&sor, 0, sizeof(SOR));
  rc = Sor(&sor, Bound); if (rc)                                     eX(10);
  vLOG(4)(_sor_options_$_, SorGetOptionString());                //-2011-07-20
  if (DMKfdivmax > 0) sor.fdivmax = DMKfdivmax;                  //-2005-11-09
  if (DMKndivmax > 0) sor.ndivmax = DMKndivmax;                  //-2005-11-09
  if (sor.rj >= 0)  _p->rj = sor.rj;                             //-2006-10-04
  if (_p->rj < 0) {
    float rj = 0;
    sor.rj = Sor_theoreticalRj(sor);
    sor.omega = 0;
    if (sor.niter <= 0)  sor.niter = (grd.nx + grd.ny + grd.nz)/sor.qiter;
    if (sor.niter > sor.maxIter)  sor.niter = sor.maxIter;        //-2006-10-04
    if (sor.ntry <= 0)   sor.ntry = 15;                           //-2006-10-04
    vLOG(3)(_estimated_rj_$$$_, sor.rj, sor.niter, sor.ntry);
    if (sor.nphi > 0) {
      int ip=0;
      float phi = 0;
      for (ip=0; ip<sor.nphi; ip++) {
        phi = sor.phi0 + ip*sor.dphi;
        sor.ux = cos(phi/RADIAN);
        sor.uy = sin(phi/RADIAN);
        sor.uz = 0;
        rj = Sor_findRj_2(&sor, sor.niter, sor.ntry, NULL);           eG(13);
        fprintf(MsgFile, "DMK: rj'(%7.4f,%7.4f) = %8.8f  (%8.8f)\n",
          sor.ux, sor.uy, 1-rj, 1-sor.rjmax);
        AryFree(&sor.uu);
        AryFree(&sor.rr);
      }
      exit(0);
    }
    rj = Sor_findRj_2(&sor, sor.niter, sor.ntry, NULL);               eG(13);
    sor.rj = 1 - 1.5*(1 - sor.rj);                                //-2006-10-04
    if (rj < sor.rjmin)  rj = sor.rjmin;                          //-2004-12-14
    _p->rj = rj;
    {                                                             //-2008-10-20
      char locale[256];
      if (*MsgLocale) {
        strcpy(locale, setlocale(LC_NUMERIC, NULL));
        setlocale(LC_NUMERIC, MsgLocale);
      }
      fprintf(MsgFile, _found_rj_$$_, rj, sor.rjmax);
      //fprintf(MsgFile, "DMK- Rj=%10.8f\n", rj);                   //-2007-02-12 commented 2011-07-20
      if (*MsgLocale)
        setlocale(LC_NUMERIC, locale);
    }
    AryFree(&sor.uu);
    AryFree(&sor.rr);
  }
  sor.rj = _p->rj;
  sor.ndivmax += 5;                                               //-2006-10-04
  for (m=1; ; m++) {  //---------------------------------------------2006-10-04
    sor.omega = 0;
    rc = Sor_removeDiv(&sor, &Wind, msg);                             eG(11);
    vLOG(3)("DMK: (%s)", msg);
    if (rc > 0)  break;
    if (sor.rj <= sor.rjmin || m == 10)  break;
    sor.rj = 1 - 1.5*(1 - sor.rj);
    sor.omega = 0;
    if (sor.rj <= sor.rjmin)  sor.rj = sor.rjmin;
    fprintf(MsgFile, _repeated_with_$_, sor.rj);
    AryFree(&sor.uu);
    AryFree(&sor.rr);
  }
  if (rc <= 0)                                                        eX(12);
  Sor_free(&sor);
  rc = Wind3D_maxDivergence(Wind, &maxdiv); if (rc)                   eX(45);
  vLOG(3)(_finished_with_$_, maxdiv);

save:

  //
  // copy Wind to output array
  if (_wo) {
    if (_wo->ttlsz != (grd.nx+1)*(grd.ny+1)*(grd.nz+1)*sizeof(WNDREC)) eX(52);
    for (i=0; i<=grd.nx; i++)
      for (j=0; j<=grd.ny; j++)
        for (k=0; k<=grd.nz; k++) {
          ((WNDREC *)AryPtr(_wo, i, j, k))->z  =
            ((WNDREC *)AryPtr(_wi, i, j, k))->z;
          ((WNDREC *)AryPtr(_wo, i, j, k))->vx = FFF(Wind.vx, i, j, k);
          ((WNDREC *)AryPtr(_wo, i, j, k))->vy = FFF(Wind.vy, i, j, k);
          ((WNDREC *)AryPtr(_wo, i, j, k))->vz = FFF(Wind.vz, i, j, k);
          if (i>0 && j>0 && k < grd.nz && INBODY(i, j, k+1))
            ((WNDREC *)AryPtr(_wo, i, j, k))->vz = WND_VOLOUT;
        }
  }
  if (_so) {
    if (_so->ttlsz != (grd.nx+1)*(grd.ny+1)*(grd.nz+1)*sizeof(TRBREC)) eX(53);
    for (i=0; i<=grd.nx; i++)
      for (j=0; j<=grd.ny; j++)
        for (k=0; k<=grd.nz; k++) {
          ((TRBREC *)AryPtr(_so, i, j, k))->su  = FFF(Sfield, i, j, k);
          ((TRBREC *)AryPtr(_so, i, j, k))->sv  = FFF(Sfield, i, j, k);
          ((TRBREC *)AryPtr(_so, i, j, k))->sw  = FFF(Sfield, i, j, k);
          ((TRBREC *)AryPtr(_so, i, j, k))->th  = 0;
        }
  }
  if (_ko) {
    if (_ko->ttlsz != (grd.nx+1)*(grd.ny+1)*(grd.nz+1)*sizeof(DFFREC)) eX(54);
    for (i=0; i<=grd.nx; i++)
      for (j=0; j<=grd.ny; j++)
        for (k=0; k<=grd.nz; k++) {
          ((DFFREC *)AryPtr(_ko, i, j, k))->kh  = FFF(Kfield, i, j, k);
          ((DFFREC *)AryPtr(_ko, i, j, k))->kv  = FFF(Kfield, i, j, k);
        }
  }
  Wind3D_free(&Winit);
  Wind3D_free(&Wind);
  Wind3D_free(&Rfield);
  Wind3D_free(&Winit4Shadow);
  Grid3D_free(&binit.grid);                //-2005-01-11
  Grid2D_free(&binit.grid.gridXY);         //-2005-01-11
  Grid1D_free(&binit.grid.gridXY.gridX);   //-2005-01-11
  Grid1D_free(&binit.grid.gridXY.gridY);   //-2005-01-11
  Grid1D_free(&binit.grid.gridZ);          //-2005-01-11
  Bound3D_free(&Bound);
  Bound3D_free(&binit);
  AryFree(&Shadow);
  AryFree(&Visible);
  AryFree(&Sfield);
  AryFree(&Kfield);
  return 0;
eX_40: eX_43: eX_44: eX_45: eMSG(_internal_error_);
eX_1: eMSG(_error_creating_init_$_, rc);
eX_2: eMSG(_error_setting_boundaries_$_, rc);
eX_3: eMSG(_error_creating_recirculation_$_, rc);
eX_4: eMSG(_error_allocation_);
eX_5: eMSG(_error_clearing_boundaries_$_, rc);
eX_10: eMSG(_error_init_sor_$_, rc);
eX_11: eMSG(_error_removing_divergence_$_, rc);
eX_12: eMSG(_no_convergence_);
eX_13: eMSG(_cant_calculate_rj_);
eX_20: eMSG(_error_defining_shadow_$_, rc);
eX_21: eMSG(_error_definig_recirculation_$_, rc);
eX_50: eX_51: eMSG(_null_pointer_);
eX_52: eMSG(_size_wnd_);
eX_53: eMSG(_size_trb_);
eX_54: eMSG(_size_dff_);
eX_55: eMSG(_inconsistent_dimensions_);
}

//------------------------------------------------------------ setRfield
static int setRfield(void) {
  dP(setRfield);
  GRID3D grd = Rfield.grid;
  ARYDSC ra;
  RECTANGLE rct;
  VEC3D vn, uu, rm;
  int i, j, k, fi, rc, nf, n;
  float weight, abs;
  char s[3][64];
  rc = 0;
  memset(&ra, 0, sizeof(ARYDSC));
  nf = getFaces(&ra); if (nf<0)                                      eX(1);
  for (i=0; i<=grd.nx; i++)
    for (j=0; j<=grd.ny; j++)
      for (k=0; k<=grd.nz; k++) {
        XXX(Rfield.vx, i, j, k) = 0;
        XXX(Rfield.vy, i, j, k) = 0;
        XXX(Rfield.vz, i, j, k) = 0;
      }
  n=0;
  for (fi=0; fi<nf; fi++) {
    if (!(fi%10)) {
      if (StdDspLevel > 0) {                                    //-2007-02-12
        fprintf(stdout, "%4.0f\r", (1.-(float)fi/nf)*100);
        fflush(stdout);
      }
    }
    rct = *(RECTANGLE *)AryPtr(&ra, fi);
    vn = rct.v3;
    rm = rct.m;
    rc = Wind3D_getVector(Winit, &uu, rm); if (rc)                   eX(10);
    uu.z = 0;                                             //-2004-12-06
    abs = Vec3D_abs(uu); if (abs == 0)                               eX(11);
    weight = Vec3D_dot(uu, vn)/abs;                       //-2004-12-06
    if (weight > 0.02) {                                  //-2004-12-06
      n++;
      vLOG(4)("DMK: face %3d %s %s %s -> w=%5.3f", fi,
        Vec3D_toString(rct.a, s[0]), Vec3D_toString(rct.c, s[1]),
        Vec3D_toString(rct.v3, s[2]), weight);
      rc = setVisible(rct); if (rc)                              eX(2);
      rc = addRfield(rct.a, rct.c, weight); if (rc)              eX(3);
    }
  }
  if (StdDspLevel > 0) {                                        //-2007-02-12
    fprintf(stdout, "%4.0f\r", 0.);
    fflush(stdout);
  }
  vLOG(3)(_active_faces_$$_, n, nf);
  rc = cropRfield(); if (rc)                                     eX(4);
  AryFree(&ra);
  return 0;
eX_1: eMSG(_cant_set_faces_$_, rc);
eX_2: eMSG(_cant_set_visible_$_, rc);
eX_3: eMSG(_cant_set_rfield_$_, rc);
eX_4: eMSG(_cant_crop_rfield_$_, rc);
eX_10: eX_11: eMSG(_internal_error_);
}

//----------------------------------------------------------- cropRfield
static int cropRfield(void) {
  dP(cropRfield);
  GRID3D grd = Bound.grid;
  int i, j, k, m, rc;
  VEC3D rm, uu, um;
  volatile VEC3D vp;
  ARYDSC uma, zma;
  WIND3D w;
  float abs, prj, sum, umabs, zm, dz, z0;
  //
  // weight and crop the field
  //
  rc = 0;
  for (i=1; i<=grd.nx; i++) {
    for (j=1; j<=grd.ny; j++) {
       for (k=1; k<=grd.nz; k++) {
         vp = Wind3D_vector(Rfield, i, j, k);
         abs = Vec3D_abs(vp);
         if (abs == 0) continue;
         // reduce z component
         vp.z *= (1 - fabs(P.a5));
         // weight with projection
         abs = Vec3D_abs(vp);
         rc += Grid3D_getMidPoint(grd, &rm, i, j, k);
         rc += Wind3D_getVector(Winit, &uu, rm);
         prj = abs * Vec3D_abs(uu);
         if (prj > 0) prj = Vec3D_dot(vp, uu)/prj;
         if (prj <= 0) {
           vp.x = 0;
           vp.y = 0;
           vp.z = 0;
         }
         else
           vp = Vec3D_mul(vp, pow(prj, P.a2/abs));
         // crop
         abs = Vec3D_abs(vp);
         if (abs > P.a3 && abs > 0)
           vp = Vec3D_mul(vp, P.a3/abs);
         XXX(Rfield.vx, i, j, k) = vp.x;
         XXX(Rfield.vy, i, j, k) = vp.y;
         XXX(Rfield.vz, i, j, k) = vp.z;
       }
    }
  }
  if (rc) return -1;
  //
  // reset shadow cells with additional rfield information
  //
  rc = setShadow(Bound, 1); if (rc) return -2;
  //
  // set vertically averaged wind velocities
  //
  rc = 0;
  memset(&uma, 0, sizeof(ARYDSC));
  memset(&zma, 0, sizeof(ARYDSC));
  AryCreate(&uma, sizeof(VEC3D), 2, 0, grd.nx, 0, grd.ny);           eG(1);
  if (P.flags & DMK_USEHM) {
    for (i=1; i<=grd.nx; i++) {
      for (j=1; j<=grd.ny; j++) {
        sum = 0;
        um = Vec3D(0, 0, 0);
        for (k=1; k<=grd.nz; k++) {
          if (INBODY(i, j, k) || !SHADOW(i, j, k))
            continue;
          abs = Vec3D_abs(Wind3D_vector(Rfield, i, j, k));
          rc += Grid3D_getMidPoint(grd, &rm, i, j, k);
          rc += Wind3D_getVector(Winit, &uu, rm);
          uu = Vec3D_mul(uu, abs);
          um = Vec3D_add(um, uu);
          sum += abs;
        }
        if (sum > 0) {
          um = Vec3D_div(um, sum);
        }
        *(VEC3D *)AryPtr(&uma, i, j) = um;
      }
    }
  }
  else  {                                                       //-2005-02-15
    AryCreate(&zma, sizeof(float), 2, 0, grd.nx, 0, grd.ny);         eG(2);
    for (i=1; i<=grd.nx; i++) {
      for (j=1; j<=grd.ny; j++) {
        um = Vec3D(0, 0, 0);
        zm = 0;
        sum = 0;
        z0 = P.hh[0];
        for (k=1; k<=grd.nz; k++) {
          if (INBODY(i, j, k)) {
            z0 = P.hh[k];
            if (k>1 && !INBODY(i, j, k-1))                           eX(3);
            continue;
          }
          /* // removed because of possible wind shear!     //-2005-03-11
          if (!SHADOW(i, j, k)) break;
          */
          abs = Vec3D_abs(Wind3D_vector(Rfield, i, j, k));
          if (abs < P.a4*P.a4*0.01) continue;  // cosmetics
          dz = P.hh[k] - P.hh[k-1];
          rc += Grid3D_getMidPoint(grd, &rm, i, j, k);
          rc += Wind3D_getVector(Winit, &uu, rm);
          uu = Vec3D_mul(uu, abs*dz);
          um = Vec3D_add(um, uu);
          zm += 0.5*abs*dz*(P.hh[k]-z0 + P.hh[k-1]-z0);
          sum += abs*dz;
        }
        if (sum > 0) {
          um = Vec3D_div(um, sum);
          zm /= sum;
        }
        *(VEC3D *)AryPtr(&uma, i, j) = um;
        *(float *)AryPtr(&zma, i, j) = zm;
      }
    }
  }
  if (rc) return -3;
  if (StdLogLevel >= 4 && MsgFile) {                            //-2005-02-15
    fprintf(MsgFile, "DMK: mean velocities:\n");
    fprintf(MsgFile, " j/i:");
    for (i=1; i<=grd.nx; i++)
      fprintf(MsgFile, " %4d", i);
    fprintf(MsgFile, "\n");
    for (j=grd.ny; j>0; j--) {
      fprintf(MsgFile, "%4d:", j);
      for (i=1; i<=grd.nx; i++)
        fprintf(MsgFile, " %4.1f", Vec3D_abs(*(VEC3D *)AryPtr(&uma, i, j)));
      fprintf(MsgFile, "\n");
    }
    if (!(P.flags & DMK_USEHM)) {
      fprintf(MsgFile, "DMK: mean heights:\n");
      fprintf(MsgFile, " j/i:");
      for (i=1; i<=grd.nx; i++)
      fprintf(MsgFile, " %4d", i);
      fprintf(MsgFile, "\n");
      for (j=grd.ny; j>0; j--) {
        fprintf(MsgFile, "%4d:", j);
        for (i=1; i<=grd.nx; i++)
          fprintf(MsgFile, " %4.1f", *(float *)AryPtr(&zma, i, j));
        fprintf(MsgFile, "\n");
      }
    }
  }
  //
  // set turbulence fields
  //
  rc = setTfields(uma, zma);                                    //-2005-02-15
  if (rc) return -4;
  //
  // scale with effective wind velocity; replace in the recirculation zones
  // the initial field with a vertically homogeneous one.
  //
  rc = 0;
  for (i=1; i<=grd.nx; i++) {
    for (j=1; j<=grd.ny; j++) {
      um = *(VEC3D *)AryPtr(&uma, i, j);
      umabs = Vec3D_abs(um);
      for (k=1; k<=grd.nz; k++) {
        vp = Wind3D_vector(Rfield, i, j, k);
        abs = Vec3D_abs(vp);
        if (SHADOW(i, j, k) != SHADOW_CORE || abs < P.a4) {
          vp = Vec3D(0, 0, 0);
        }
        else {
          vp = Vec3D_mul(vp, -P.a1 * umabs);
          rc += Grid3D_getMidPoint(grd, &rm, i, j, k);
          rc += Wind3D_getVector(Winit, &uu, rm);
          vp = Vec3D_sub(vp, uu);
          vp = Vec3D_add(vp, um);
        }
        XXX(Rfield.vx, i, j, k) = vp.x;
        XXX(Rfield.vy, i, j, k) = vp.y;
        XXX(Rfield.vz, i, j, k) = vp.z;
      }
    }
  }
  if (rc) return -5;
  //
  // project to ARAKAWA-C
  //
  memset(&w, 0, sizeof(WIND3D));
  rc = Wind3D(&w, Bound); if (rc) return -5;
  for (i=0; i<=grd.nx; i++) {
    for (j=0; j<=grd.ny; j++) {
      for (k=0; k<=grd.nz; k++) {
        XXX(w.vx, i,j,k) = 0;
        XXX(w.vy, i,j,k) = 0;
        XXX(w.vz, i,j,k) = 0;
        if (j>0 && k>0) {
          m = Bound3D_getAreaXType(Bound, i,j,k);
          if ((m & BND_OUTSIDE) || (m & BND_FIXED))
            ;
          else if (i==0)
            XXX(w.vx, i,j,k) = FFF(Rfield.vx, 1,j,k);
          else if (i==grd.nx)
            XXX(w.vx, i,j,k) = FFF(Rfield.vx, grd.nx,j,k);
          else
            XXX(w.vx, i,j,k) =
              0.5*(FFF(Rfield.vx, i,j,k) +FFF(Rfield.vx, i+1,j,k));
        }
        if (i>0 && k>0) {
          m = Bound3D_getAreaYType(Bound, i,j,k);
          if ((m & BND_OUTSIDE) || (m & BND_FIXED))
            ;
          else if (j==0)
            XXX(w.vy, i,j,k) = FFF(Rfield.vy, i,1,k);
          else if (j==grd.ny)
            XXX(w.vy, i,j,k) = FFF(Rfield.vy, i,grd.ny,k);
          else
            XXX(w.vy, i,j,k) =
              0.5*(FFF(Rfield.vy, i,j,k) +FFF(Rfield.vy, i,j+1,k));
        }
        if (i>0 && j>0) {
          m = Bound3D_getAreaZType(Bound, i,j,k);
          if ((m & BND_OUTSIDE) || (m & BND_FIXED))
            ;
          else if (k==0)
            XXX(w.vz, i,j,k) = FFF(Rfield.vz, i,j,1);
          else if (k==grd.nz)
            XXX(w.vz, i,j,k) = FFF(Rfield.vz, i,j,grd.nz);
          else
            XXX(w.vz, i,j,k) =
              0.5*(FFF(Rfield.vz, i,j,k) +FFF(Rfield.vz, i,j,k+1));
        }
      }
    }
  }
  for (i=0; i<=grd.nx; i++)
    for (j=0; j<=grd.ny; j++)
      for (k=0; k<=grd.nz; k++) {
        XXX(Rfield.vx, i, j, k) = FFF(w.vx, i, j, k);
        XXX(Rfield.vy, i, j, k) = FFF(w.vy, i, j, k);
        XXX(Rfield.vz, i, j, k) = FFF(w.vz, i, j, k);
      }
  Wind3D_free(&w);
  AryFree(&uma);
  AryFree(&zma);
  if (StdLogLevel >= 4  && MsgFile) {
    fprintf(MsgFile, "DMK: final recirculation field:\n");
    Wind3D_printZ(Rfield, 1, MsgFile);
  }
  return 0;
eX_1: eX_2: eMSG(_cant_allocate_);
eX_3: eMSG(_internal_error_);
}

//----------------------------------------------------------- setTfields
static int setTfields(ARYDSC uma, ARYDSC zma) {
  dP(setTfields);
  GRID3D grd = Bound.grid;
  int i, j, k, ii, jj, kk, nrm, num, topi;
  float rm, um, zm, hm, rdc, z, h, topz, rx, ry, rz, f;
  ARYDSC rfsqrt, top;
  memset(&rfsqrt, 0, sizeof(ARYDSC));
  memset(&top, 0, sizeof(ARYDSC));
  AryCreate(&rfsqrt, sizeof(float), 3, 0, grd.nx,0,grd.ny,0,grd.nz); eG(1);
  AryCreate(&top, sizeof(int), 3, 0, grd.nx, 0, grd.ny, 0, 0);       eG(1);
  hm = getMeanBuildingHeight();
  if (hm < 0)                                                        eX(2);
  // set top values for extrapolation
  for (i=1; i<=grd.nx; i++) {
    for (j=1; j<=grd.ny; j++) {
      for (kk=0,k=1; k<=grd.nz; k++)
        if (SHADOW(i, j, k)) kk = k;
      *(int *)AryPtr(&top, i, j, 0) = kk;
      for (k=1; k<=grd.nz; k++) {
        rx = FFF(Rfield.vx, i, j, k);
        ry = FFF(Rfield.vy, i, j, k);
        rz = FFF(Rfield.vz, i, j, k);
        f = pow(rx*rx+ry*ry+rz*rz, 0.25);
        if (f > P.a3) f = P.a3;
        if (f < P.a4) f = 0;
        *(float *)AryPtr(&rfsqrt, i, j, k) = f;
      }
    }
  }
  // set turbulence fields on grid knots
  for (i=0; i<=grd.nx; i++) {
    for (j=0; j<=grd.ny; j++) {
      for (k=0; k<=grd.nz; k++) {
        rm = 0;
        um = 0;
        zm = 0;
        nrm = 0;
        num = 0;
        for (ii=i; ii<=i+1; ii++) {
          for (jj=j; jj<=j+1; jj++) {
            if (ii<1 || ii>grd.nx || jj<1 || jj>grd.ny)
              continue;
            if (III(top, ii, jj, 0) <= 0)
              continue;
            um += Vec3D_abs(*(VEC3D *)AryPtr(&uma, ii, jj));
            if (zma.start) zm += (*(float *)AryPtr(&zma, ii, jj));
            num++;
            topi = III(top, ii, jj, 0);
            topz = F(grd.hh, topi);
            for (kk=k; kk<=k+1; kk++) {
              if (kk<1 || kk>grd.nz)
                continue;
              if (INBODY(ii, jj, kk))
                continue;
              if (kk <= topi)
                rm += FFF(rfsqrt, ii, jj, kk);
              else {
                z = 0.5*(F(grd.hh, kk) + F(grd.hh, kk-1));
                if (P.hs > 1)
                  rdc = (P.hs*topz - z)/(P.hs*topz - topz);
                else
                  rdc = 1;
                if (rdc > 0)
                  rm += rdc * FFF(rfsqrt, ii, jj, topi);
              }
              nrm++;
            }
          }
        }
        if (nrm == 0 || num == 0)
          continue;
        rm /= nrm;
        um /= num;
        zm /= num;
        XXX(Sfield, i, j, k) = rm * um * P.fs;
        h = (zma.start) ? 2*zm : hm;                            //-2005-02-15
        XXX(Kfield, i, j, k) = h * FFF(Sfield, i, j, k) * P.fk;
      }
    }
  }
  AryFree(&top);
  AryFree(&rfsqrt);
  return 0;
eX_1: eMSG(_cant_allocate_);
eX_2: eMSG(_invalid_building_height_);
}

//------------------------------------------------ getMeanBuildingHeight
static float getMeanBuildingHeight(void) {                      //-2005-05-25
  float h1, h2, hm;
  int i, j, k, n;
  hm = 0;
  n = 0;
  for (i=1; i<=Bound.grid.nx; i++) {
    for (j=1; j<=Bound.grid.ny; j++) {
      h1 = -1;
      h2 = -1;
      for (k=1; k<=Bound.grid.nz; k++) {
        if (INBODY(i, j, k)) {
          h2 = F(Bound.grid.hh, k);
          if (h1 < 0) h1 = F(Bound.grid.hh, k-1);
        }
      }
      if (h1 >= 0 && h2 > 0) {
        hm += (h2-h1);
        n++;
      }
    }
  }
  if (n > 0) hm /= n;
  return hm;
}

//----------------------------------------------------------- addRfield
static int addRfield(VEC3D v1, VEC3D v2, float weight) {
  VEC3D rp, vf1, vf2, a, b, e;
  RECTANGLE rs, rsm;
  int i, j, k, rc;
  if (weight == 0) return 0;
  a = Vec3D(v1.x, v1.y, v1.z);
  b = Vec3D(v2.x, v2.y, v1.z);
  e = Vec3D(0, 0, v2.z-v1.z);
  rs = Rectangle(a, b, e);
  a = Vec3D(v1.x, v1.y, -v2.z);
  b = Vec3D(v2.x, v2.y, -v2.z);
  e = Vec3D(0, 0, v2.z-v1.z);
  rsm = Rectangle(a, b, e);
  if (rs.area <= 0 || rsm.area <= 0) return -1;
  rs.granular = PRM_GRANULAR;
  rsm.granular = rs.granular;
  rs.charge = 2 * rs.area * weight;
  rsm.charge = rs.charge;
  for (i=1; i<=Bound.grid.nx; i++) {
    for (j=1; j<=Bound.grid.ny; j++) {
       for (k=1; k<=Bound.grid.nz; k++) {
         if (!VISIBLE(i, j, k) || INBODY(i, j, k))
           continue;
         Grid3D_getMidPoint(Bound.grid, &rp, i, j, k);
         rc = Rectangle_field(rs, &vf1, rp); if (rc) return -2;
         rc = Rectangle_field(rsm, &vf2, rp); if (rc) return -3;
         FFF(Rfield.vx, i, j, k) += (vf1.x+vf2.x);
         FFF(Rfield.vy, i, j, k) += (vf1.y+vf2.y);
         FFF(Rfield.vz, i, j, k) += (vf1.z+vf2.z);
       }
    }
  }
  return 0;
}

//----------------------------------------------------------- setVisible
static int setVisible(RECTANGLE r) {
  GRID3D grd = Bound.grid;
  VEC3D uu, rm;
  POSITION p;
  int i, j, k, i1, i2, j1, j2, k1, k2, m, free;
  float ddx, dx;
  dx = (grd.dx < grd.dy) ? grd.dx : grd.dy;
  ddx = PRM_F2 * dx;
  rm = r.m;
  Wind3D_getVector(Winit, &uu, rm);
  uu.z = 0;                                              //-2004-12-06
  uu = Vec3D_norm(uu);
  // start point at half a mesh width in front of cell
  rm = Vec3D_add(r.m, Vec3D_mul(uu, 0.5*dx));
  Grid3D_getPosition(grd, &p, rm);
  if (INBODY(p.i, p.j, p.k)) {
    rm = Vec3D_add(r.m, Vec3D_mul(r.v3, 0.5*dx));
    Grid3D_getPosition(grd, &p, rm);
    if (INBODY(p.i, p.j, p.k)) {
      return -1;
    }
  }
  memset(Visible.start, 0, Visible.ttlsz);                        //-2010-08-26
  for (m=0; m<8; m++) {                                  //-2005-01-11
    if (0 != (m & 0x1)) { i1 = 1;   i2 = p.i;    }
    else                { i1 = p.i; i2 = grd.nx; }
    if (0 != (m & 0x2)) { j1 = 1;   j2 = p.j;    }
    else                { j1 = p.j; j2 = grd.ny; }
    if (0 != (m & 0x4)) { k1 = 1;   k2 = p.k;    }
    else                { k1 = p.k; k2 = grd.nz; }
    free = 1;
    for (i=i1; i<=i2; i++)
      for (j=j1; j<=j2; j++)
        for (k=k1; k<=k2; k++)
          if (INBODY(i, j, k)) {
            free = 0;
            break;
          }
    for (i=i1; i<=i2; i++)
      for (j=j1; j<=j2; j++)
        for (k=k1; k<=k2; k++)
          if (SHADOW(i, j, k))
            *(short *)AryPtr(&Visible, i, j, k) =
              (free) ? 1 : getVisibility(rm, i, j, k, ddx);
  }
  /*
  memset(Visible.start, 0, Visible.ttlsz);
  for (i=1; i<=grd.nx; i++) {
    for (j=1; j<=grd.ny; j++) {
      for (k=1; k<=grd.nz; k++) {
        if (SHADOW(i, j, k))
          *(short *)AryPtr(&Visible, i, j, k) =
            getVisibility(rm, i, j, k, ddx);
      }
    }
  }
  */
  if (StdLogLevel >= 5 && MsgFile) {
    fprintf(MsgFile, "DMK: visibility for k=1:\n");
    k = 1;
    for (j=grd.ny; j>0; j--) {
      for (i=1; i<=grd.nx; i++)
        fprintf(MsgFile, "%1d", (int)VISIBLE(i, j, k));
      fprintf(MsgFile, "\n");
    }
  }
  return 0;
}

//------------------------------------------------------- getVisibility
static short getVisibility(VEC3D rm, int ai, int aj, int ak, float ddx) {
  VEC3D v, dv, a;
  float l, f, h, len, ax, ay;
  int kk, rc, pi, pj, pk;
  GRID3D g = Bound.grid;
  // get mid point
  a.x = 0.25*(FF(g.gridXY.xx, ai-1, aj-1) + FF(g.gridXY.xx, ai, aj-1) +
              FF(g.gridXY.xx, ai-1, aj)   + FF(g.gridXY.xx, ai, aj));
  a.y = 0.25*(FF(g.gridXY.yy, ai-1, aj-1) + FF(g.gridXY.yy, ai, aj-1) +
              FF(g.gridXY.yy, ai-1, aj)   + FF(g.gridXY.yy, ai, aj));
  a.z = 0;
  if (g.gridXY.zz.start)
    a.z = 0.25*(FF(g.gridXY.zz, ai-1, aj-1) + FF(g.gridXY.zz, ai, aj-1) +
                FF(g.gridXY.zz, ai-1, aj) + FF(g.gridXY.zz, ai, aj));
  a.z += 0.5 * (F(g.hh, ak-1) + F(g.hh, ak));
  //Grid3D_getMidPoint(Bound.grid, &a, ai, aj, ak);
  //
  dv.x = a.x - rm.x;
  dv.y = a.y - rm.y;
  dv.z = a.z - rm.z;
  v.x = rm.x;
  v.y = rm.y;
  v.z = rm.z;
  len = sqrt(dv.x*dv.x + dv.y*dv.y + dv.z*dv.z);
  if (len == 0)
    return 1;
  if (ddx < 0.1)
    ddx = 0.1;
  f = ddx/len;
  dv.x *= f;
  dv.y *= f;
  dv.z *= f;
  l = 0;
  while (l < len) {
    // get position
    rc = 0;
    ax = (v.x - g.xmin)/g.dx;
    if (ax < 0 || ax > g.nx)
      rc = -2;
    else {
      pi = (int)ax;
      if (pi == g.nx) pi--;
      ax -= pi++;
      ay = (v.y - g.ymin)/g.dy;
      if (ay < 0 || ay > g.ny)
       rc = -3;
      else {
        pj = (int)ay;
        if (pj == g.ny) pj--;
        ay -= pj++;
        h = v.z;
        for (kk=1; kk<=g.nz; kk++)
          if (F(g.hh, kk) >= h) break;
        if (kk < 0 || kk > g.nz)
          rc = -4;
        else
         pk = kk;
      }
    }
    //rc = Grid3D_getIntPosition(Bound.grid, &pi, &pj, &pk, v);
    if (rc)
      return 1;
    else if (INBODY(pi, pj, pk))
      return 0;
    v.x += dv.x;
    v.y += dv.y;
    v.z += dv.z;
    l += ddx;
  }
  return 1;
}


//------------------------------------------------------------- getFaces
static int getFaces(ARYDSC *rp) {
  dP(getFaces);
  int i, j, k, m, n;
  GRID3D grd = Rfield.grid;
  VEC3D r1, r2, r3;
  float x, y, z1, z2, dx, dy;
  if (!rp) return -1;
  dx = grd.dx;
  dy = grd.dy;
  for (m=0; m<=1; m++) {
    if (m) {
      AryCreate(rp, sizeof(RECTANGLE), 1, 0, n-1);                   eG(1);
    }
    n = 0;
    for (i=1; i<=grd.nx; i++) {
      x = grd.xmin + (i-1)*grd.dx;
      for (j=1; j<=grd.ny; j++) {
        y = grd.ymin + (j-1)*grd.dy;
        for (k=1; k<=grd.nz; k++) {
          if (!INBODY(i, j, k))
            continue;
          z1 = F(grd.hh, k-1);
          z2 = F(grd.hh, k);
          if (i<=grd.nx-1 && !INBODY(i+1, j, k)) { // right side
            if (!m) n++;
            else {
              r1 = Vec3D(x+dx, y, z1);
              r2 = Vec3D(x+dx, y+dy, z1);
              r3 = Vec3D(0, 0, z2-z1);
              *(RECTANGLE *)AryPtr(rp, n++) = Rectangle(r1, r2, r3);
            }
          }
          if (j<=grd.ny-1 && !INBODY(i, j+1, k)) { // top side
            if (!m) n++;
            else {
              r1 = Vec3D(x+dx, y+dy, z1);
              r2 = Vec3D(x, y+dy, z1);
              r3 = Vec3D(0, 0, z2-z1);
              *(RECTANGLE *)AryPtr(rp, n++) = Rectangle(r1, r2, r3);
            }
          }
          if (i>1 && !INBODY(i-1, j, k)) { // left side
            if (!m) n++;
            else {
              r1 = Vec3D(x, y+dy, z1);
              r2 = Vec3D(x, y, z1);
              r3 = Vec3D(0, 0, z2-z1);
              *(RECTANGLE *)AryPtr(rp, n++) = Rectangle(r1, r2, r3);
            }
          }
          if (j>1 && !INBODY(i, j-1, k)) { // bottom side
            if (!m) n++;
            else {
              r1 = Vec3D(x, y, z1);
              r2 = Vec3D(x+dx, y, z1);
              r3 = Vec3D(0, 0, z2-z1);
              *(RECTANGLE *)AryPtr(rp, n++) = Rectangle(r1, r2, r3);
            }
          }
        }
      }
    }
  }
  return n;
eX_1: eMSG(_cant_allocate_);
}


//-------------------------------------------------------- setBoundaries
static int setBoundaries(ARYDSC *_bi, int fixed) {
  GRID3D grd = Bound.grid;
  int i, j, k, rc=0;
  for (i=1; i<=grd.nx; i++)
    for (j=1; j<grd.ny; j++)
      for (k=1; k<=grd.nz; k++)
        if (III(*_bi, i, j, k))
          rc += Bound3D_setVolout(&Bound, i, j, k, BND_FIXED);
  // fix border values
  vLOG(4)("DMK: %s boundaries", (fixed) ? "fixed" : "open");    //-2005-05-06
  if (fixed) {
    rc = 0;
    for (j=1; j<=grd.ny; j++) {
      for (k=1; k<=grd.nz; k++) {
        rc += Bound3D_setFixedX(&Bound, 0, j, k, 1);
        rc += Bound3D_setFixedX(&Bound, grd.nx, j, k, 1);
      }
    }
    for (i=1; i<=grd.nx; i++) {
      for (k=1; k<=grd.nz; k++) {
        rc += Bound3D_setFixedY(&Bound, i,  0, k, 1);
        rc += Bound3D_setFixedY(&Bound, i, grd.ny, k, 1);
      }
    }
    /*
    for (i=1; i<=grd.nx; i++) {
      for (j=1; j<=grd.ny; j++) {
        rc += Bound3D_setFixedZ(&Bound, i, j, grd.nz, 1);
      }
    }
    */
  }
  Bound3D_complete(&Bound);
  return rc;
}

//------------------------------------------------------------- setWinit
static int setWinit(ARYDSC *_w) {
  int i, j, k, rc;
  float dtau, dx;
  VEC3D v, r;
  GRID1D gx, gy, gz;
  GRID2D gxy;
  GRID3D g;
  BOUND3D b;
  memset(&gx, 0, sizeof(GRID1D));
  memset(&gy, 0, sizeof(GRID1D));
  memset(&gz, 0, sizeof(GRID1D));
  memset(&gxy, 0, sizeof(GRID2D));
  memset(&g, 0, sizeof(GRID3D));
  memset(&b, 0, sizeof(BOUND3D));
  rc = 0;
  rc += Grid1D(&gx, P.x0, P.dd, P.nx);
  rc += Grid1D(&gy, P.y0, P.dd, P.ny);
  rc += Grid1Dz(&gz, P.nz+1, P.hh);
  rc += Grid2D(&gxy, gx, gy);
  rc += Grid3D(&g, gxy, gz);
  rc += Bound3D(&b, g);
  rc += Bound3D_complete(&b);
  rc += Wind3D(&Winit, b);
  rc += Wind3D(&Winit4Shadow, b);
  for (i=0; i<=P.nx; i++) {
    for (j=0; j<=P.ny; j++) {
      for (k=0; k<=P.nz; k++) {
        XXX(Winit.vx, i, j, k) = ((WNDREC *)AryPtr(_w, i, j, k))->vx;
        XXX(Winit.vy, i, j, k) = ((WNDREC *)AryPtr(_w, i, j, k))->vy;
        XXX(Winit.vz, i, j, k) = ((WNDREC *)AryPtr(_w, i, j, k))->vz;
      }
    }
  }
  dx = (g.dx < g.dy) ? g.dx : g.dy;
  dtau = PRM_F2 * dx;
  if (dtau < 0.1) dtau = 0.1;
  for (i=1; i<=P.nx; i++) {
    for (j=1; j<=P.ny; j++) {
      for (k=1; k<=P.nz; k++) {
        rc += Grid3D_getMidPoint(g, &r, i, j, k);
        rc += Wind3D_getVector(Winit, &v, r);
        v.z = 0;
        if (v.x == 0 && v.y == 0)
          return -99;
        v = Vec3D_norm(v);
        XXX(Winit4Shadow.vx, i, j, k) = v.x*dtau;
        XXX(Winit4Shadow.vy, i, j, k) = v.y*dtau;
        XXX(Winit4Shadow.vz, i, j, k) = v.z*dtau;
      }
    }
  }
  return rc;
}


//------------------------------------------------------------ setShadow
static int setShadow(BOUND3D b, int userf) {
  //dP(setShadow);
  int iter, rc, a, da, i, j, k, l, m, pi, pj, pk, smallvalue, leftbody;
  int oi, oj, ok;
  float a42, abs2, dtau, dx, ds, dc, ux, uy, uz, vx, vy, vz;
  GRID3D grd = b.grid;
  VEC3D ez, rr, shift, rm, uu;
  short *sp;
  dx = (grd.dx < grd.dy) ? grd.dx : grd.dy;
  dtau = PRM_F2 * dx;
  if (dtau < 0.1) dtau = 0.1;
  for (i=1; i<=grd.nx; i++)
    for (j=1; j<=grd.ny; j++)
      for (k=1; k<=grd.nz; k++)
        SHADOW(i,j,k) = 0;
  ez = Vec3D(0, 0, SHADOW_SHIFT*dx);
  rc = 0;
  da = (int)(P.da+0.5);           //-2004-12-14
  a42 = P.a4*P.a4;
  for (m=0; m<=2*da; m++) {
    a = m;
    if (a > da) a = da - a;
    vLOG(5)("DMK: shadow direction %d", m);
    ds = sin(a/RADIAN);           //-2004-12-14
    dc = cos(a/RADIAN);           //-2004-12-14
    uu.z = 0;
    for (i=1; i<=grd.nx; i++) {
      for (j=1; j<=grd.ny; j++) {
        for (k=1; k<=grd.nz; k++) {
          if (!INBODY(i, j, k))
            continue;
          rc = Grid3D_getMidPoint(grd, &rm, i, j, k);
          if (rc < 0) return rc-1000;                             //-2005-08-17
          rc = Wind3D_getVector(Winit, &uu, rm);
          if (rc < 0) return rc-2000;                             //-2005-08-17
          uu = Vec3D_norm(uu);
          shift = Vec3D_cross(uu, ez);
          for (l=0; l<3; l++) {
            rr.x = rm.x;
            rr.y = rm.y;
            rr.z = rm.z;
            if (l ==1 ) {
              rr.x += shift.x;
              rr.y += shift.y;
              rr.z += shift.z;
            }
            else if (l == 2) {
              rr.x -= shift.x;
              rr.y -= shift.y;
              rr.z -= shift.z;
            }
            if (rr.x<=grd.xmin || rr.x>=grd.xmax ||       //-2005-08-17
                rr.y<=grd.ymin || rr.y>=grd.ymax)
              continue;
            rc = Grid3D_getIntPosition(grd, &pi, &pj, &pk, rr);
            if (rc < 0) return rc-3000;                           //-2005-08-17
            ux = FFF(Winit4Shadow.vx, pi, pj, pk);
            uy = FFF(Winit4Shadow.vy, pi, pj, pk);
            uz = FFF(Winit4Shadow.vz, pi, pj, pk);
            uu.x = ux*dc + uy*ds;
            uu.y = uy*dc - ux*ds;
            uu.z = 0;
            smallvalue = 0;
            leftbody = 0;
            iter = 0;
            pi = -1;
            pj = -1;
            pk = -1;
            while (++iter) {
              rr.x += uu.x;
              rr.y += uu.y;
              rr.z += uu.z;
              if (rr.x<=grd.xmin || rr.x>=grd.xmax ||     //-2005-08-17
                  rr.y<=grd.ymin || rr.y>=grd.ymax)
                break;
              oi = pi;
              oj = pj;
              ok = pk;
              rc = Grid3D_getIntPosition(grd, &pi, &pj, &pk, rr);
              if (rc < 0) return rc-4000;                         //-2005-08-17
              if (iter>1 && pi==oi && pj==oj && pk==ok)
                continue;
              if (!leftbody && !INBODY(pi, pj, pk))
                leftbody = 1;
              if (!leftbody)
                continue;
              if (INBODY(pi, pj, pk)) {
                break;
              }
              if (userf) {
                vx = FFF(Rfield.vx, pi, pj, pk);
                vy = FFF(Rfield.vy, pi, pj, pk);
                vz = FFF(Rfield.vz, pi, pj, pk);
                abs2 = vx*vx + vy*vy + vz*vz;
                if (abs2 < a42)
                  smallvalue++;
              }
              sp = (short *)AryPtr(&Shadow, pi, pj, pk);
              if (m == 0) {
                if (smallvalue >= SHADOW_MINSV && *sp != SHADOW_CORE)
                  *sp = SHADOW_EXT;
                else
                  *sp = SHADOW_CORE;
              }
              else if (*sp != SHADOW_CORE)
                *sp = SHADOW_EXT;
            } // while
          } // l (shift of start point)
        } // k
      } // j
    } // i
  } // m (direction)
  if (StdLogLevel >= 4 && MsgFile) {
    for (k=1; k<=1; k++) {
      fprintf(MsgFile, "DMK: shadow for k=%d:\n", k);
      for (j=grd.ny; j>0; j--) {
        for (i=1; i<=grd.nx; i++)
          fprintf(MsgFile, "%1d", (int)SHADOW(i, j, k));
        fprintf(MsgFile, "\n");
      }
    }
  }
  return rc;
}

#undef X
#undef XXX
#undef FFF
#undef FF
#undef F
#undef III
#undef SHADOW
#undef BLOCKED
#undef INBODY

//=============================================================================
//
// history:
//
// 2004-12-06 uj 2.1.2  uz=0, check for weight >0.02 in setRField()
// 2004-12-14 uj 2.1.3  include TalMat.h
// 2005-01-11 uj 2.1.4  getVisibility/setVisible: speed improvement
// 2005-02-15 uj 2.1.14 use zm instead of hm for additional diffusion
// 2005-03-11 lj 2.1.16 shadowing modified
// 2005-03-17 uj 2.2.0  version number upgrade
// 2005-05-06 uj 2.2.1  log levels adjusted
// 2005-08-17 uj 2.2.2  avoid rounding problem for non-integer grid coordinates
// 2005-11-09 uj 2.2.3  external access to SOR.fdivmax and SOR.ndivmax
// 2006-10-04 lj 2.2.15 relaxed constraints for SOR
// 2006-10-26 lj 2.3.0  external strings
// 2007-01-05 lj 2.3.1  version string modified
// 2007-02-12 lj        display switched by StdDspLevel (SOR: S.dsp)
// 2008-10-20 lj 2.4.4  using MsgLocale
// 2010-12-27 uj 2.4.5  SetVisible() corrected
// 2011-06-29 uj 2.4.6  "&" corrected into "&&"
// 2011-07-07 uj 2.5.0  version number upgrade
// 2011-07-20 uj        log info adjusted
// 2012-04-06 uj 2.6.0  version number upgrade
//=============================================================================

