// ================================================================= TalSOR.h
//
// SOR solver for DMK
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
// last change: 2011-07-20 uj
//
//==========================================================================

#define SOR_EPS          1.e-3
#define SOR_MAXITER      200

typedef struct {
  BOUND3D b3d;
  GRID3D g3d;
  GRID1D g1d;
  int nx, ny, nz, ndivmax, maxIter, curIter;
  float dx, dy, rj, eps, omega, fdivmax, dlimit;
  float ux, uy, uz, phi0, dphi, rjmin, rjmax, qiter;				//-2006-10-04
  int check_rj, niter, ntry, nphi;									//-2006-10-04
  ARYDSC a111, a011, a211, a101, a121, a110, a112;
  ARYDSC uu, rr;
  ARYDSC dz, dzs, zz;
  int log;	// log level											//-2007-02-12
  int dsp;	// dsp level											//-2007-02-12
} SOR;

int   Sor(SOR *, BOUND3D);
int   Sor_removeDiv(SOR *, WIND3D *, char *);
float Sor_theoreticalRj(SOR);
float Sor_findRj(SOR *);
float Sor_findRj_1(SOR *, int, int);
float Sor_findRj_2(SOR *, int, int, WIND3D *);
void  Sor_free(SOR *);
int   SorOption(char *);
char *SorGetOptionString(void);                                   //-2011-07-20

