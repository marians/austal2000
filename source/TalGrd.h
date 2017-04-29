//=================================================================== TalGrd.h
//
// Handling of computational grids
// ===============================
//
// Copyright (C) Umweltbundesamt, Dessau-Ro�lau, Germany, 2002-2005
// Copyright (C) Janicke Consulting, 88662 �berlingen, Germany, 2002-2008
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
// last change: 2008-12-04 lj
//
//==========================================================================

#ifndef  TALGRD_INCLUDE
#define  TALGRD_INCLUDE

#define  GRD_PNTBND   0x00000001
#define  GRD_PNTOUT   0x00000002
#define  GRD_PNTMSK   0x0000000f

#define  GRD_ARXBND   0x00000010
#define  GRD_ARXOUT   0x00000020
#define  GRD_ARXSET   0x00000040
#define  GRD_ARXMIX   0x00000080
#define  GRD_ARXMSK   0x000000f0

#define  GRD_ARYBND   0x00000100
#define  GRD_ARYOUT   0x00000200
#define  GRD_ARYSET   0x00000400
#define  GRD_ARYMIX   0x00000800
#define  GRD_ARYMSK   0x00000f00

#define  GRD_ARZBND   0x00001000
#define  GRD_ARZOUT   0x00002000
#define  GRD_ARZSET   0x00004000
#define  GRD_ARZMIX   0x00008000
#define  GRD_ARZMSK   0x0000f000
#define  GRD_ARAMSK   0x0000fff0

#define  GRD_VOLOUT   0x00020000
#define  GRD_VOLMIX   0x00080000
#define  GRD_VOLMSK   0x000f0000

#define  GRD_AKWC     0x00
#define  GRD_3LIN     0x01

#define  GRD_DEFREFX   0      //-2008-12-04
#define  GRD_DEFREFY   0      //-2008-12-04
#define  GRD_DEFCHI    0.0
#define  GRD_XYMAX     200000

#define  GRD_GEOGK     0x00001000L
#define  GRD_3DMET     0x00040000L
#define  GRD_SOLID     0x01000000L
#define  GRD_NESTED    0x02000000L
#define  GRD_GRID      0x04000000L
#define  GRD_BODIES    0x08000000L
#define  GRD_DMKHM     0x00100000L    //-2005-02-15

#define  GRD_IZP    1
#define  GRD_ISLD   2
#define  GRD_IZM    3
#define  GRD_IVM    4

#define  GRD_PERX    0x01
#define  GRD_PERY    0x02
#define  GRD_REFZ    0x04

#define  GRD_INSIDE     0x01
#define  GRD_PARM_IS_H  0x02
#define  GRD_USE_ZETA   0x04
#define  GRD_REFBOT     0x10
#define  GRD_REFTOP     0x20

#ifndef GRD_MACRO_ONLY //-----------------------------------------------------------

#ifndef   IBJARY_INCLUDE
  #include "IBJary.h"
#endif

#ifndef   TALMAT_INCLUDE
  #include "TalMat.h"
#endif

#define ISNOTEQUAL(a,b)  (ABS((a)-(b)) > 0.00001*(ABS(a)+ABS(b)))
#define iBOT(a,b)   ((GrdPprm->psrf)?*(float*)AryPtrX(GrdPprm->psrf,(a),(b)):0)
#define iSK(c)      (*(float*)AryPtrX(GrdParr,(c)))
#define iSLD(a,b,c) (*(long*)AryPtrX(GrdPprm->psld,(a),(b),(c)))

enum NET_TYPE { NOnet, FLAT1D, FLAT3D, COMPLEX, DEGREE };

typedef struct {
   char name[16];
   enum NET_TYPE ntyp;
   float level, index, ndim, ptyp;
   float nx, ny, nz;
   float delta;
   float xmin, x1, x2;
   float ymin, y1, y2;
   float rfac, itermax, iteranl, iterrad, itereps, o0, o2, o4;
   ARYDSC *psrf, *pgrd;
   int defmode;
   float xmax, ymax, zmin, zavr, zmax;
   } NETREC;

typedef struct {
   int nx;        /* number of grid cells in x-direction  */
   int ny;
   int nz;
   int nu;      /* number of land usage classes */
   int nzmax;
   int nzdos;
   int nzmap;
   int numnet;
   int refx, refy;
   char ggcs[16];
   long bd;        /* boundary conditions */
   float dd;
   float hmax, vspc;
   float xmin, xmax, ymin, ymax, x1, x2, y1, y2, border;
   float zmin, zavr, zmax;
   float radius, oref, pref, omin, delo, pmin, delp;
   double radfak;
   float cosref, sinref;
   float chi;
   long flags;
   int prfmode, ndim, level, index, itermax, iteranl;
   float zscl, sscl;
   float rfac, iterrad, itereps, o0, o2, o4;
   char name[16];
   enum NET_TYPE ntyp;
   ARYDSC *psrf, *pgrd;
   int defmode;
   ARYDSC *plup;
   } GRDPARM;

typedef struct { float z, z0, d0; } SRFREC;
typedef struct { float z; int luc; } SRFDEF;
typedef struct { int luc; float z0, d0; } SRFPAR;

extern GRDPARM *GrdPprm;
extern ARYDSC *GrdParr, *GrdPtab;

/*------------------------ Function Prototypes -----------------------------*/

long GrdReadSrf( NETREC *pn )
  ;
long GrdCheck( void )         /* Daten in NETarr überprüfen */
  ;
long GrdList( void )          /* Auflistung von NETarr */
  ;
char *GrdHeader(       /* File-Header mit Grid-Information erstellen */
long ident )           /* Identifizierung der Daten                  */
  ;
long GrdRead(          /* File GRID.DEF einlesen und Werte setzen.  */
char *altname )        /* Alternativer File-Name                    */
  ;
long GrdSetNet(            /* Netz auflegen */
int net )                  /* Netz-Nummer   */
  ;
long GrdSet(               /* Netz auflegen */
int netlevel,              /* Netz-Level    */
int netindex )             /* Netz-Index    */
  ;
int GrdNxtLevel(      /* Nächst niedrigeren Grid-Level heraussuchen */
int level )           /* Momentaner Level                           */
  ;
int GrdLstLevel(      /* Nächst höheren Grid-Level heraussuchen     */
int level )           /* Momentaner Level                           */
  ;
long GrdSetDefMode( int mode )
  ;
int GrdIx( float x )
  ;
int GrdJy( float y )
  ;
float GrdZb( float x, float y )
  ;
int GrdBottom( float x, float y, float *pzb )
  ;
float GrdZt( float x, float y )
  ;
float GrdZz( float x, float y, float s )
  ;
float GrdHh( float x, float y, float s )
  ;
float GrdSs( float x, float y, float z )
  ;
int GrdKs( float s )
  ;
int GrdKz( float x, float y, float z )
  ;
int GrdLocate(  /* locate position within grid          */
ARYDSC *pa,     /* array of zp-values                   */
float xi,       /* normalized x-coordinate              */
float eta,      /* normalized y-coordinate              */
float *pz,      /* height h or absolute z-value         */
int flag,       /* PARM_IS_H, REFLECT                   */
float zr,       /* reflection at z=zr                   */
int *pi,        /* index i                              */
int *pj,        /* index j                              */
int *pk,        /* index k                              */
float *pzg,     /* ground level                         */
float *paz )    /* normalized vertical position (in k)  */
  ;
long GrdInit(           /* initialize server    */
long flags,             /* action flags         */
char *istr )            /* server options       */
  ;
long GrdServer(
char *s )
  ;
#endif  //------------------------------------------------------------------

/*=========================================================================*/
#endif
