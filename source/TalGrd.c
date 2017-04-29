// ================================================================= TalGrd.c
//
// Handling of computational grids
// ===============================
//
// Copyright (C) Umweltbundesamt, Dessau-Ro�lau, Germany, 2002-2005
// Copyright (C) Janicke Consulting, 88662 �berlingen, Germany, 2002-2011
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
// last change: 2011-07-04 uj
//
//==========================================================================

#include <math.h>
#include <ctype.h>
#include <signal.h>

#define  STDMYMAIN  GrdMain
#include "IBJstd.h"
#include "IBJmsg.h"
#include "IBJary.h"
#include "IBJtxt.h"
#include "IBJdmn.h"

static char *eMODn = "TalGrd";

/*=========================================================================*/

STDPGM(lprgrd, GrdServer, 2, 6, 0);

/*=========================================================================*/

#include "genutl.h"
#include "genio.h"

#include "TalTmn.h"
#include "TalNms.h"
#include "TalMat.h"
#include "TalBds.h"
#include "TalGrd.h"
#include "TalGrd.nls"

static int CheckSld( int gl, int gi, ARYDSC *psld )
  ;

ARYDSC *GrdParr, *GrdPtab;
GRDPARM *GrdPprm;

static int NumNetDat = 22;
static float StdNetDat[] = {
      0.0,     0.0,    1.0,    3.0, 32.0, 32.0,  0.0, 1.e9,   1.e9, 1.e9,
     1.e9,    1.e9,   1.e9,   1.e9,  1.0, 40.0,  5.0, 0.80,  1.e-4,
     0.00, 0.00, 0.00 };
static char *NetNames[] = {
     "NL",    "NI",   "NT",   "PT", "NX", "NY", "NZ", "DD", "XMIN", "X1",
     "X2",  "YMIN",   "Y1",   "Y2", "RF", "IM", "IA", "IR", "IE",
     "O0", "O2", "O4", NULL };

static float DatVec[101], DatScale[101];
static int DatPos[101];
static char *Buf, DefName[120] = "param.def";
static int WrtMask = 0x0100;                                      //-2011-07-04
static int PrnLevel, PrnIndex, PrnLayer = -1;

//=============================================================== GetGridParm
//
static int GetGridParm( int gl, int gi,
float *pxmin, float *pymin, float *pdelta,
int *pnx, int *pny, int *pnz, ARYDSC **ppsrf )
  {
  dP(GetGridParm);
  int nt, net;
  NETREC *pn;
  if (!GrdPprm)  return -1;
  if (GrdPprm->numnet > 0) {
    for (net=1; net<=GrdPprm->numnet; net++) {
      pn = AryPtr(GrdPtab, net);  if (!pn)          eX(1);
      if (pn->level==gl && pn->index==gi)  break;
      }
    if (net > GrdPprm->numnet)                      eX(2);
    if (pnx) *pnx = pn->nx;
    if (pny) *pny = pn->ny;
    if (pnz) *pnz = pn->nz;
    if (pxmin) *pxmin = pn->xmin;
    if (pymin) *pymin = pn->ymin;
    if (pdelta) *pdelta = pn->delta;
    if (ppsrf) *ppsrf = pn->psrf;
    nt = pn->ntyp;
    }
  else {
    if (pnx) *pnx = GrdPprm->nx;
    if (pny) *pny = GrdPprm->ny;
    if (pnz) *pnz = GrdPprm->nz;
    if (pxmin) *pxmin = GrdPprm->xmin;
    if (pymin) *pymin = GrdPprm->ymin;
    if (pdelta) *pdelta = GrdPprm->dd;
    if (ppsrf) *ppsrf = GrdPprm->psrf;
    nt = GrdPprm->ntyp;
    }
  return nt;
eX_1:  eX_2:
  eMSG(_grid_$$_not_found_, gl, gi);
  }

//=================================================================== MakeGrd
//
#define  ZP(a,b,c)  (*(float*)AryPtrX(pzp,(a),(b),(c)))

static long MakeGrd( char *ss )
  {
  dP(MakeGrd);
  int i, j, k, nx, ny, nz, gl, gi, gp, nt;
  static long id;
  long m, izp;
  char name[256];						//-2004-11-26
  float xmin, ymin, delta, ztop, stop, zbot, z, z1, z2, s;
  ARYDSC *pb, *psrf, *pzp;
  vLOG(4)("GRD:MakeGrd(%s)", ss);
  if (*ss) {
    if ((*ss=='-') && (ss[1] == 'N'))  sscanf(ss+2, "%08lx", &id);
    return 0;
    }
  if (!GrdPprm)                                                         eX(99);
  strcpy(name, NmsName(id));
  if (GrdPprm->flags & GRD_GRID)                                        eX(4);
  vLOG(4)("GRD: creating %s ...", name);
  gl = XTR_LEVEL(id);
  gi = XTR_GRIDN(id);
  gp = XTR_GROUP(id);
  nt = GetGridParm(gl, gi, &xmin, &ymin, &delta, &nx, &ny, &nz, &psrf);
  if (nt <= 0)                                                          eX(6);
  ztop = GrdPprm->zscl;
  stop = GrdPprm->sscl;
  if (stop <= 0)  stop = ztop;
  switch (gp) {
    case GRD_IZP:
      pb = TmnCreate(id, sizeof(float), 3, 0,nx, 0,ny, 0,nz);           eG(2);
      for (i=0; i<=nx; i++)
        for (j=0; j<=ny; j++) {
          zbot = (psrf) ? *(float*)AryPtrX(psrf,i,j) : 0;
          for (k=0; k<=nz; k++) {
            s = iSK(k);
            if (ztop <= 0)  z = zbot + s;
            else {
              if (s > stop)  z = ztop + s - stop;
              else           z = zbot + (ztop-zbot)*s/stop;
              }
            *(float*)AryPtrX(pb, i, j, k) = z;
            }
          }
      break;
    case GRD_ISLD:
      pb = TmnCreate(id, sizeof(long), 3, 0, nx, 0, ny, 0, nz);         eG(2);
      m = GRD_PNTBND;
      for (i=0; i<=nx; i++)
        for (j=0; j<=ny; j++)  *(long*)AryPtrX(pb,i,j,0) |= m;
      m = GRD_ARZBND;
      for (i=1; i<=nx; i++)
        for (j=1; j<=ny; j++)  *(long*)AryPtrX(pb,i,j,0) |= m;
      CheckSld(gl, gi, pb);                                             eG(16);
      break;
    case GRD_IZM:
      izp = IDENT(GRDarr, GRD_IZP, gl, gi);
      pzp = TmnAttach(izp, NULL, NULL, 0, NULL);  if (!pzp)             eX(10);
      pb  = TmnCreate(id, sizeof(float), 3, 1, nx, 1, ny, 1, nz);       eG(8);
      for (i=1; i<=nx; i++)
        for (j=1; j<=ny; j++)
          for (k=1; k<=nz; k++) {
            z1 = ZP(i-1,j-1,k-1)+ZP(i-1,j,k-1)+ZP(i,j-1,k-1)+ZP(i,j,k-1);
            z2 = ZP(i-1,j-1,k)  +ZP(i-1,j,k)  +ZP(i,j-1,k)  +ZP(i,j,k);
            *(float*)AryPtrX(pb,i,j,k) = (z2+z1)/8;
            }
      TmnDetach(izp, NULL, NULL, 0, NULL);                              eG(11);
      break;
    case GRD_IVM:
      izp = IDENT(GRDarr, GRD_IZP, gl, gi);
      pzp = TmnAttach(izp, NULL, NULL, 0, NULL); if (!pzp)              eX(12);
      pb  = TmnCreate(id, sizeof(float), 3, 1, nx, 1, ny, 1, nz);       eG(9);
      for (i=1; i<=nx; i++)
        for (j=1; j<=ny; j++)
          for (k=1; k<=nz; k++) {
            z1 = ZP(i-1,j-1,k-1)+ZP(i-1,j,k-1)+ZP(i,j-1,k-1)+ZP(i,j,k-1);
            z2 = ZP(i-1,j-1,k)  +ZP(i-1,j,k)  +ZP(i,j-1,k)  +ZP(i,j,k);
            *(float*)AryPtrX(pb,i,j,k) = (z2-z1)*delta*delta/4;
            }
      TmnDetach(izp, NULL, NULL, 0, NULL);                              eG(13);
      break;
    default:                                                            eX(14);
    }
  TmnDetach(id, NULL, NULL, TMN_MODIFY, NULL);                          eG(3);
  vLOG(4)("GRD: ... done");
  return 0;
eX_2:  eX_3:  eX_8:  eX_9:
eX_10: eX_11: eX_12: eX_13: eX_14: eX_16:
  eMSG(_cant_create_$_, name);
eX_4:
  eMSG(_file_$_not_found_, name);
eX_6:
  eMSG(_internal_error_);
eX_99:
  eMSG(_not_initialized_);
  }
#undef  ZP

  /*============================================================ DefLandUse
  */
static long DefLandUse( ARYDSC *pa, ARYDSC *pb, ARYDSC *pu )
#define  gAL(i,j) ((SRFDEF*)AryPtrX(pa,(i),(j)))->luc
  {
  dP(DefLandUse);
  SRFREC *ps;
  SRFDEF *pd;
  SRFPAR *pl;
  int i, j, nx, ny, l, nl, luc;
  vLOG(4)("GRD:DefLandUse(...)");
  nx = GrdPprm->nx;
  ny = GrdPprm->ny;
  if (pa->elmsz < sizeof(SRFDEF))  pu = NULL;
  if (pu)  nl = pu->bound[0].hgh;
  for (i=0; i<=nx; i++)
    for (j=0; j<=ny; j++) {
      pd = AryPtr(pa, i, j);  if (!pd)                     eX(1);
      ps = AryPtr(pb, i, j);  if (!ps)                     eX(2);
      ps->z = pd->z;
      if (pu) {
        luc = gAL(i,j);
        for (l=1; l<=nl; l++) {
          pl = AryPtr(pu, l);  if (!pl)                    eX(3);
          if (luc == pl->luc) {
            ps->z0 = pl->z0;
            ps->d0 = pl->d0;
            }
          }
        }
      }
  return 0;
eX_1:  eX_2:  eX_3:
  eMSG(_internal_error_);
  }
#undef gAL

//=================================================================== CheckSld
//
#define  ZZ(a,b,c)  (*(float*)AryPtrX(pgrd,(a),(b),(c)))
#define  BN(a,b,c)  (*(long*)AryPtrX(psld,(a),(b),(c)))
#define  VO(a,b,c)  (*(long*)AryPtrX(psld,(a),(b),(c)) & GRD_VOLOUT)

static int CheckSld( int gl, int gi, ARYDSC *psld )
  {
  dP(CheckSld);
  int i, j, k, n, nx, ny, nz, nt, igrd;
  int nins, nout, iv, jv, kv, m, highest_level;
  float xmin, ymin, delta;
  ARYDSC *pgrd;
  NETREC *pn;
  n = GrdPprm->numnet;
  if (n < 1)  highest_level = 0;
  else {
    pn = AryPtr(GrdPtab, n);  if (!pn)                                     eX(6);
    highest_level = pn->level;
    }
  nt = GetGridParm(gl, gi, &xmin, &ymin, &delta, &nx, &ny, &nz, NULL);
  if (nt <= 0)                                                          eX(9);
  if (!psld)                                                            eX(1);
  igrd = IDENT(GRDarr, GRD_IZP, gl, gi);
  pgrd = TmnAttach(igrd, NULL, NULL, 0, NULL);  if (!pgrd)              eX(2);
  for (i=1; i<=nx; i++)
    for (j=1; j<=ny; j++)
      for (k=1; k<=nz; k++) {
        if (!VO(i,j,k))  continue;
        if (i==0 || VO(i-1,j,k))  BN(i-1,j,k) |= GRD_ARXOUT;
        else  BN(i-1,j,k) |= GRD_ARXBND;
        if (i==nx || VO(i+1,j,k))  BN(i,j,k) |= GRD_ARXOUT;
        else  BN(i,j,k) |= GRD_ARXBND;
        if (j==0 || VO(i,j-1,k))  BN(i,j-1,k) |= GRD_ARYOUT;
        else  BN(i,j-1,k) |= GRD_ARYBND;
        if (j==ny || VO(i,j+1,k))  BN(i,j,k) |= GRD_ARYOUT;
        else  BN(i,j,k) |= GRD_ARYBND;
        if (k==0 || VO(i,j,k-1))  BN(i,j,k-1) |= GRD_ARZOUT;
        else  BN(i,j,k-1) |= GRD_ARZBND;
        if (k==nz || VO(i,j,k+1))  BN(i,j,k) |= GRD_ARZOUT;
        else  BN(i,j,k) |= GRD_ARZBND;
        }
  for (i=0; i<=nx; i++)
    for (j=0; j<=ny; j++)
      for (k=0; k<=nz; k++) {
        nins = 0;
        nout = 0;
        iv = i;
        jv = j;
        kv = k;
        if (iv>=1 && jv>=1 && kv>=1) {
          if (VO(iv,jv,kv))  nout++;
          else  nins++;
        }
        kv = k+1;
        if (iv>=1 && jv>=1 && kv<=nz) {
          if (VO(iv,jv,kv))  nout++;
          else  nins++;
        }
        kv = k;
        jv = j+1;
        if (iv>=1 && jv<=ny && kv>=1) {
          if (VO(iv,jv,kv))  nout++;
          else  nins++;
        }
        kv = k+1;
        if (iv>=1 && jv<=ny && kv<=nz) {
          if (VO(iv,jv,kv))  nout++;
          else  nins++;
        }
        iv = i+1;
        jv = j;
        kv = k;
        if (iv<=nx && jv>=1 && kv>=1) {
          if (VO(iv,jv,kv))  nout++;
          else  nins++;
        }
        kv = k+1;
        if (iv<=nx && jv>=1 && kv<=nz) {
          if (VO(iv,jv,kv))  nout++;
          else  nins++;
        }
        kv = k;
        jv = j+1;
        if (iv<=nx && jv<=ny && kv>=1) {
          if (VO(iv,jv,kv))  nout++;
          else  nins++;
        }
        kv = k+1;
        if (iv<=nx && jv<=ny && kv<=nz) {
          if (VO(iv,jv,kv))  nout++;
          else  nins++;
        }
        if (nout > 0)  m = (nins > 0) ? GRD_PNTBND : GRD_PNTOUT;
        else           m = 0;
        if ((m==GRD_PNTOUT) && (BN(i,j,k) & GRD_PNTBND))
          BN(i,j,k) &= ~GRD_PNTBND;
        BN(i,j,k) |= m;
      }
  TmnDetach(igrd, NULL, NULL, 0, NULL);                             eG(5);

  return 0;
eX_9: eX_1: eX_2: eX_5: eX_6:
  eMSG(_internal_error_);
  }
#undef  ZZ
#undef  VO
#undef  BN

//=============================================================== CheckSpacing
//
static int CheckSpacing( float xmin, float ymin, float delta, char *header )
  {
  dP(CheckSpacing);
  float xm, ym, dd;
  if (!header || !*header)
    return 0;
  if (1 != DmnGetFloat(header, "Xmin|X0", "%f", &xm, 1))       eX(1);  //-2011-06-29
  if (1 != DmnGetFloat(header, "Ymin|Y0", "%f", &ym, 1))       eX(2);  //-2011-06-29
  if (1 != DmnGetFloat(header, "Dd|Delt|Delta", "%f", &dd, 1)) eX(3);  //-2011-06-29
  if (ISNOTEQUAL(xm,xmin))                              eX(4);
  if (ISNOTEQUAL(ym,ymin))                              eX(5);
  if (ISNOTEQUAL(dd,delta))                             eX(6);
  return 0;
eX_1: eX_4:
  eMSG(_invalid_xmin_$_, xmin);
eX_2: eX_5:
  eMSG(_invalid_ymin_$_, ymin);
eX_3: eX_6:
  eMSG(_invalid_delta_$_, delta);
  }

//================================================================= GrdReadSrf
//
#define CAT(a, b) TxtPrintf(&txt, a, b), TxtCat(&hdr, txt.s);
long GrdReadSrf( NETREC *pn )
  {
  dP(GrdReadSrf);
  char fname[256], s[80];
  float xmin, ymin, delta;
  int nx, ny, nz, i, nu, nl, ni, mode;
  enum NET_TYPE ntyp;
  long id1, id2, idu;
  ARYDSC *pa, **ppa, **ppg, *pb, *plup;
  SRFPAR *pu;
  TXTSTR txt = { NULL, 0 };
  TXTSTR hdr = { NULL, 0 };
  TxtCpy(&hdr, "\n");
  vLOG(4)("GRD:GrdReadSrf(...)");
  if (!GrdPprm)                                                 eX(99);
  if (pn) {
    ppa = &(pn->psrf);
    ppg = &(pn->pgrd);
    nl = pn->level;
    ni = pn->index;
    }
  else {
    ppa = &(GrdPprm->psrf);
    ppg = &(GrdPprm->pgrd);
    nl = GrdPprm->level;
    ni = GrdPprm->index;
    }
  ntyp = GetGridParm(nl, ni, &xmin, &ymin, &delta, &nx, &ny, &nz, NULL);
  if (ntyp <= 0)                                                eX(98);
  *ppa = NULL;
  id1 = IDENT(SRFarr, 0, nl, ni);
  strcpy(fname, NmsName(id1));
  mode = 0;
  pa = TmnAttach(id1, NULL, NULL, 0, &hdr);             eG(1);    //-2011-06-29
  if (!pa) {
    if (ntyp == COMPLEX)                                eX(1);
    pa = TmnCreate(id1, sizeof(float), 2, 0,nx, 0,ny);  eG(5);
    sprintf(s, "GRD_%d.%d.%s", StdVersion, StdRelease, StdPatch);
    CAT("prgm  \"%s\"\n", s);
    CAT("name  \"%s\"\n", fname);
    CAT("form  \"%s\"\n", "Z%1.1f");
    CAT("xmin  %s\n", TxtForm(xmin,6));
    CAT("ymin  %s\n", TxtForm(ymin,6));
    CAT("delta %s\n", TxtForm(delta,6));
    mode = TMN_MODIFY;
  }
  else {
    CheckSpacing(xmin, ymin, delta, hdr.s);             eG(2);
    AryAssert(pa, pa->elmsz, 2, 0, nx, 0, ny);          eG(20);
  }
  if (pa->elmsz >= sizeof(SRFDEF)) {
    nu = DmnGetFloat(hdr.s, "LUC", "%f", DatVec, 101);          eG(80);
    if (nu > 100)                                               eX(92);
    if (nu > 0) {
      idu = IDENT(SRFpar, 0, nl, ni);         /*-98-07-10-*/
      plup = TmnCreate(idu, sizeof(SRFPAR), 1, 1, nu);          eG(81);
      GrdPprm->nu = nu;
      GrdPprm->plup = plup;
      TmnDetach(idu, NULL, NULL, TMN_MODIFY|TMN_FIXED, NULL);   eG(82);
      for (i=0; i<nu; i++)  {
        pu = (SRFPAR*) AryPtr(plup, i+1);  if (!pu)             eX(83);
        pu->luc = DatVec[i];
        }
      }
    nu = DmnGetFloat(hdr.s, "Z0V", "%f", DatVec, 101);          eG(84);
    if (nu > 0) {
      if (nu != GrdPprm->nu)                                    eX(85);
      for (i=0; i<nu; i++)  {
        pu = (SRFPAR*) AryPtr(plup, i+1);  if (!pu)             eX(86);
        pu->z0 = DatVec[i];
        }
      } 
    nu = DmnGetFloat(hdr.s, "D0V", "%f", DatVec, 101);          eG(87);
    if (nu > 0) {
      if (nu != GrdPprm->nu)                                    eX(88);
      for (i=0; i<nu; i++)  {
        pu = (SRFPAR*) AryPtr(plup, i+1);  if (!pu)             eX(89);
        pu->d0 = DatVec[i];
        }
      }
    }
  else  
    plup = NULL;
  id2 = IDENT(SRFarr, 1, nl, ni);
  strcpy(fname, NmsName(id2));
  pb = TmnCreate(id2, sizeof(SRFREC), 2, 0, nx, 0, ny);         eG(70);
  DefLandUse(pa, pb, plup);                                     eG(71);
  TmnDetach(id1, NULL, NULL, mode, &hdr);                       eG(91);
  DmnRplName(&hdr, "format", NULL);                             eG(72);
  DmnRplValues(&hdr, "form", "Z%6.1fZ0%6.3fD0%6.1f");           eG(73);
  TmnDetach(id2, NULL, NULL, TMN_MODIFY|TMN_FIXED, &hdr);       eG(74);
  TxtClr(&hdr);
  TxtClr(&txt);
  *ppa = pb;
  GrdPprm->psrf = pb;
  return 0;

eX_1:
  eMSG(_cant_read_$_, fname);
eX_2:
  eMSG(_invalid_header_$_, fname);
eX_5:
  eMSG(_cant_create_surface_$_, fname);
eX_20:
  eMSG(_inconsistent_dimensions_$_, fname);
eX_70:
  eMSG(_cant_create_$_, fname);
 eX_71: eX_72: eX_73: eX_74:
 eX_91: eX_92:
  eMSG(_internal_error_);
eX_80: eX_84: eX_87:
  eMSG(_cant_read_land_use_);
eX_81:
  eMSG(_cant_create_land_table_);
eX_82: eX_83: eX_86: eX_89:
  eMSG(_internal_error_);
eX_85: eX_88:
  eMSG(_inconsistent_parameter_count_$$_, nu, GrdPprm->nu);
eX_98: eX_99:
  eMSG(_not_initialized_);
}
#undef CAT

//================================================================ GrdVerify
//
static int GrdVerify( void )
  {
  dP(GrdVerify);
  char fname[256];
  int nx, ny, nz, n, nn, nl, ni, i, j;
  int id, igp;
  float xmin, ymin, delta, z, zmin, zavr, zmax;
  NETREC *pn;
  ARYDSC *pa, *pb, **ppg, *psrf;
  enum NET_TYPE ntyp;
  GRDPARM *pp;
  TXTSTR hdr = { NULL, 0 };
  vLOG(4)("GRD:GrdVerify()");
  if (!GrdPprm)                                                 eX(99);
  nn = GrdPprm->numnet;
  if (nn < 1)  nn = 1;

  for (n=1; n<=nn; n++) {  /*---------------*/

  if (GrdPprm->numnet > 0) {
    pn = AryPtr(GrdPtab, n);  if (!pn)                             eX(1);
    ppg = &(pn->pgrd);
    nl = pn->level;
    ni = pn->index;
    psrf = pn->psrf;
    }
  else {
    ppg = &(GrdPprm->pgrd);
    nl = GrdPprm->level;
    ni = GrdPprm->index;
    psrf = GrdPprm->psrf;
    pn = NULL;
    }
  ntyp = GetGridParm(nl, ni, &xmin, &ymin, &delta, &nx, &ny, &nz, NULL);
  if (ntyp <= FLAT1D)  continue;

  if (!psrf)                                                    eX(3);
  zmin = 1.e6;
  zavr = 0;
  zmax = 0;
  for (i=0; i<=nx; i++)
    for (j=0; j<=ny; j++) {
      z = *(float*)AryPtrX(psrf, i, j);
      if (z < zmin)  zmin = z;
      if (z > zmax)  zmax = z;
      zavr += z;
      }
  zavr /= (nx+1)*(ny+1);
  if (GrdPprm->zscl>0 && GrdPprm->zscl<zmax)                    eX(2);
  if (pn) {
    pn->zmin = zmin;
    pn->zavr = zavr;
    pn->zmax = zmax;
    }
  else {
    GrdPprm->zmin = zmin;
    GrdPprm->zavr = zavr;
    GrdPprm->zmax = zmax;
    }
  vLOG(3)("GRD: surface of grid (%d,%d) : %1.2f <= %1.2f <=%1.2f",
    nl, ni, zmin, zavr, zmax);
  id = IDENT(GRDarr, GRD_IZP, nl, ni);
  strcpy(fname, NmsName(id));
  pb = TmnAttach(id, NULL, NULL, TMN_FIXED,  &hdr);   if (!pb)  eX(30); //-2011-06-29
  CheckSpacing(xmin, ymin, delta, hdr.s);                       eG(31);
  AryAssert(pb, sizeof(float), 3, 0,nx, 0,ny, 0,nz);            eG(32);
  TmnDetach(id, NULL, NULL, 0, NULL);                           eG(33);
  *ppg = pb;
  GrdPprm->pgrd = pb;
  igp = IDENT(GRDpar, 0, nl, ni);
  pa =  TmnAttach(igp, NULL, NULL, TMN_MODIFY, NULL);           eG(11);
  if (!pa)                                                      eX(12);
  if (GrdPprm->numnet > 0) {
    GrdSetNet(n);                                               eG(13);
    }
  pp = (GRDPARM*) pa->start;
  *pp = *GrdPprm;
  TmnDetach(igp, NULL, NULL, TMN_MODIFY, NULL);                 eG(14);
  id = IDENT(GRDarr, GRD_ISLD, nl, ni);
  strcpy (fname, NmsName(id));
  pb = TmnAttach(id, NULL, NULL, 0, &hdr);  if (!pb)            eX(40); //-2011-06-29
  CheckSpacing(xmin, ymin, delta, hdr.s);                       eG(41);
  AryAssert(pb, sizeof(long), 3, 0,nx, 0,ny, 0,nz);             eG(42);
  TmnDetach(id, NULL, NULL, 0, NULL);                           eG(43);

  id = IDENT(GRDarr, GRD_IZM, nl, ni);
  strcpy (fname, NmsName(id));
  pb = TmnAttach(id, NULL, NULL, 0, &hdr);  if (!pb)            eX(50); //-2011-06-29
  CheckSpacing(xmin, ymin, delta, hdr.s);                       eG(51);
  AryAssert(pb, sizeof(float), 3, 1,nx, 1,ny, 1,nz);            eG(52);
  TmnDetach(id, NULL, NULL, 0, NULL);                           eG(53);

  id = IDENT(GRDarr, GRD_IVM, nl, ni);
  strcpy (fname, NmsName(id));
  pb = TmnAttach(id, NULL, NULL, 0, &hdr);  if (!pb)            eX(60); //-2011-06-29
  CheckSpacing(xmin, ymin, delta, hdr.s);                       eG(61);
  AryAssert(pb, sizeof(float), 3, 1,nx, 1,ny, 1,nz);            eG(62);
  TmnDetach(id, NULL, NULL, 0, NULL);                           eG(63);

  }  /*-------------------------*/

  TxtClr(&hdr);
  return 0;
eX_1:
  eMSG(_index_range_$_, n);
eX_2:
  eMSG(_value_zscl_$$$_, zmax, nl, ni);
eX_3:
  eMSG(_missing_surface_definition_$$_, nl, ni);
eX_11: eX_12: eX_13: eX_14:
  eMSG(_internal_error_);
eX_30: eX_40: eX_50: eX_60:
eX_33: eX_43: eX_53: eX_63:
  eMSG(_cant_get_$_, fname);
eX_31: eX_41: eX_51: eX_61:
  eMSG(_inconsistent_header_$_$_, fname, hdr.s);
eX_32: eX_42: eX_52: eX_62:
  eMSG(_inconsistent_dimensions_$_, fname);
eX_99:
  eMSG(_not_initialized_);
  }

/*=================================================================== GrdCheck
*/
long GrdCheck( void )                          /* Überprüfung von GRDtab */
  {
  dP(GrdCheck);
  int n, i, l, nz, lastnz, lvl, lastlvl, ind, lastind, nx, ny, lastntyp;
  int type_1_used;
  float xmin, xmax, ymin, ymax, dd, lastdd, lastxref, lastyref, d;
  float xMin, xMax, yMin, yMax;
  long rc, id;
  NETREC *pnet;
  ARYDSC *pa;
  GRDPARM *pp;
  vLOG(4)("GRD:GrdCheck()");
  if (!GrdPprm)                                                         eX(99);
  if (GrdPprm->numnet < 1) {
    if ((GrdPprm->ntyp != COMPLEX) && (GrdPprm->ntyp != FLAT3D))  return 0;
    rc = GrdReadSrf(NULL);
    return rc;
    }
  if (!GrdPtab)                                                 eX(1);
  if (StdStatus & 0x01000000) { /* Adjust net types to highest complexity */
    vLOG(4)("GRD: adjusting net types");
    lastntyp = -1;
    for (n=GrdPprm->numnet; n>=1; n--) {
      pnet = AryPtrX(GrdPtab, n);
      if (lastntyp > 0) {
        if (pnet->ndim > lastntyp)                              eX(43);
        pnet->ndim = lastntyp;
        }
      else  lastntyp = pnet->ndim;
      }
    }
  pnet = (NETREC*) GrdPtab->start;
  lastnz = GrdPprm->nzmax;
  nz = lastnz;
  lastlvl = -1;
  lastntyp = -1;
  lvl = -1;
  ind = -1;
  type_1_used = 0;
  //-------------------------------- translated from LASAT  2005-03-11
  xmin = 0;
  ymin = 0;
  dd = 0;
  for (n=0; n<GrdPprm->numnet; n++, pnet++) {
    lvl = -1;
    if ((pnet->level < 1) || (pnet->level > 30))                eX(2);
    lvl = pnet->level;
    ind = -1;
    if ((pnet->index < 1) || (pnet->index > 30))                eX(3);
    ind = pnet->index;
    if (lvl != lastlvl) {                                       //-2002-09-21
      lastxref = xmin;
      lastyref = ymin;
      lastdd = dd;
    }
    else if (n == 1)                                            eX(23);
    if (pnet->delta> 1.e8)                                      eX(4);
    if (pnet->delta <= 0 )                                      eX(5);
    if (pnet->xmin > 1.e8)                                      eX(6);
    xmin = pnet->xmin;                                          //-2002-09-21
    if (pnet->x1   > 1.e8)  pnet->x1 = pnet->xmin;
    if (pnet->x1 < pnet->xmin)                                  eX(7);
    if ((pnet->nx < 1) || (pnet->nx > 300))                     eX(8);
    nx = pnet->nx;
    xmax = pnet->xmin + nx*pnet->delta;
    pnet->xmax = xmax;
    if (pnet->x2   > 1.e8)  pnet->x2 = xmax;
    else {                                                      //-2002-10-05
      if (xmax < pnet->x2)                                      eX(39);
    }
    if (pnet->x2 <= pnet->x1)                                   eX(9);
    if (pnet->ymin > 1.e8)                                      eX(10);
    ymin = pnet->ymin;                                          //-2002-09-21
    if (pnet->y1   > 1.e8)  pnet->y1 = pnet->ymin;
    if (pnet->y1 < pnet->ymin)                                  eX(11);
    if ((pnet->ny < 1) || (pnet->ny > 300))                     eX(12);
    ny = pnet->ny;
    ymax = pnet->ymin + ny*pnet->delta;
    pnet->ymax = ymax;
    if (fabs(xmin)>GRD_XYMAX || fabs(xmax)>GRD_XYMAX
     || fabs(ymin)>GRD_XYMAX || fabs(ymax)>GRD_XYMAX)           eX(80);
    if (pnet->y2   > 1.e8)  pnet->y2 = ymax;
    else {                                                      //-2002-10-05
      if (ymax < pnet->y2)                                      eX(33);
    }
    if (pnet->y2 <= pnet->y1)                                   eX(13);
    if (n == 0) {                                               //-2002-09-21
      xMin = xmin-0.01;
      xMax = xmax+0.01;
      yMin = ymin-0.01;
      yMax = ymax+0.01;
    }
    else {
      if (xmin<xMin || xmax>xMax || ymin<yMin || ymax>yMax)     eX(50);
      if (nx%2 || ny%2)                                         eX(51);
    }
    if ((pnet->nz < 1) || (pnet->nz > 100))                     eX(14);
    nz = pnet->nz;
    if ((pnet->ptyp < 2) || (pnet->ptyp > 3))                   eX(15);
    if ((pnet->rfac > 1.0) || (pnet->rfac < 0))                 eX(16);
    if ((pnet->ndim < 1) || (pnet->ndim > 3))                   eX(17);
    i = pnet->ndim;
    pnet->ntyp = i;
    if (lvl != lastlvl) {
      if (lvl < lastlvl)                                        eX(18);
      if (nz > lastnz)                                          eX(19);
      dd = pnet->delta;
      if (n > 0) {
        d = 2 - lastdd/dd;
        if (ABS(d) > 0.0001)                                    eX(20);
        d = (lastxref - xmin)/lastdd;                           //-2002-09-21
        d = ABS(d);  l = d+0.00005;  d = ABS(d-l);
        if (d > 0.0001)                                         eX(21);
        d = (lastyref - ymin)/lastdd;                           //-2002-09-21
        d = ABS(d);  l = d+0.00005;  d = ABS(d-l);
        if (d > 0.0001)                                         eX(22);
        }
      if (type_1_used && i==COMPLEX)                            eX(40);
      if (lastntyp>0 && i<lastntyp)                             eX(41);
      lastnz = nz;
      lastlvl = lvl;
      lastind = ind;
      lastntyp = i;
      if (i == 1)  type_1_used = 1;
      }
    else {
      if (ind <= lastind)                                       eX(24);
      if (nz != lastnz)                                         eX(25);
      d = dd - pnet->delta;
      if (ABS(d) > 0.0001)                                      eX(26);
      d = (lastxref - pnet->xmin)/dd;
      d = ABS(d);  l = d+0.00005;  d = ABS(d-l);
      if (d > 0.0001)                                           eX(27);
      d = (lastyref - pnet->ymin)/dd;
      d = ABS(d);  l = d+0.00005;  d = ABS(d-l);
      if (d > 0.0001)                                           eX(28);
      if (lastntyp != i)                                        eX(42);
      }
    GrdSetNet(n+1);                                             eG(30);
    if ((pnet->ntyp==COMPLEX) || (pnet->ntyp==FLAT3D)) {
      GrdReadSrf(pnet);                                         eG(29);
      }
    id = IDENT( GRDpar, 0, lvl, ind );
    pa = TmnCreate(id, sizeof(GRDPARM), 1, 0, 0);  if (!pa)     eX(31);
    pp = (GRDPARM*) pa->start;
    *pp = *GrdPprm;
    TmnDetach(id, NULL, NULL, TMN_MODIFY|TMN_FIXED, NULL);      eG(32);
    }
  //-----------------------------------------------------------------
  return 0;
eX_1:
  eMSG(_cant_find_grdtab_);
eX_2:
  eMSG(_invalid_grid_level_);
eX_3:
  eMSG(_invalid_grid_index_);
eX_4:
  eMSG(_undefined_delta_$$_, lvl, ind);
eX_5:
  eMSG(_invalid_delta_$$_, lvl, ind);
eX_6:
  eMSG(_undefined_xmin_$$_, lvl, ind);
eX_7:
  eMSG(_invalid_x1_$$_, lvl, ind);
eX_8:
  eMSG(_invalid_nx_$$_, lvl, ind);
eX_9: eX_39:
  eMSG(_invalid_x2_$$_, lvl, ind);
eX_10:
  eMSG(_undefined_ymin_$$_, lvl, ind);
eX_11:
  eMSG(_invalid_y1_$$_, lvl, ind);
eX_12:
  eMSG(_invalid_ny_$$_, lvl, ind);
eX_13: eX_33:
  eMSG(_invalid_y2_$$_, lvl, ind);
eX_14:
  eMSG(_invalid_nz_$$_, lvl, ind);
eX_15:
  eMSG(_invalid_pt_$$_, lvl, ind);
eX_16:
  eMSG(_invalid_rf_$$_, lvl, ind);
eX_17:
  eMSG(_invalid_nt_$$_, lvl, ind);
eX_18:
  eMSG(_inconsistent_level_$$_, lvl, ind);
eX_19:
  eMSG(_inconsistent_nz_$$_, lvl, ind);
eX_20:
  eMSG(_inconsistent_delta_$$_, lvl, ind);
eX_21:
  eMSG(_inconsistent_xmin_$$_, lvl, ind);
eX_22:
  eMSG(_inconsistent_ymin_$$_, lvl, ind);
eX_23: eX_24:
  eMSG(_inconsistent_index_$$_, lvl, ind);
eX_25:
  eMSG(_inconsistent_nz_$$_, lvl, ind);
eX_26:
  eMSG(_inconsistent_delta_$$_, lvl, ind);
eX_27:
  eMSG(_inconsistent_xmin_$$_, lvl, ind);
eX_28:
  eMSG(_inconsistent_ymin_$$_, lvl, ind);
eX_29:
  eMSG(_error_grid_$$_, lvl, ind);
eX_30: eX_31: eX_32:
  eMSG(_cant_create_grdpar_$$_, lvl, ind);
eX_40: eX_41:
  eMSG(_no_nesting_$$$_$_, lvl, ind, i, lastntyp);
eX_42:
  eMSG(_type_$_required_$$_, lastntyp, lvl, ind);
eX_43:
  eMSG(_decrease_complexity_);
eX_50:
  eMSG(_$$_not_included_, lvl, ind);
eX_51:
  eMSG(_$$_not_aligned_, lvl, ind);
eX_80:
  eMSG(_size_exceeded_);
eX_99:
  eMSG(_not_initialized_);
  }

//==================================================================== GrdTune
//
static int GrdTune( void )
  {
  dP(GrdTune);
  int n, nn, m, nl, ll, n0, n1, n2, nv;
  int ia[32], ie[32], ja[32], je[32], imin, imax, jmin, jmax;
  int i, j, di, dj, i2, j2, nx, ny, ii, jj;
  float xm, ym, dd, zm;
  NETREC *p0, *p1, *p2;
  ARYDSC *psrf0, *psrf1, *psrf2;
  SRFREC *ps, *ps0, *ps1, *ps2;
  enum NET_TYPE nt, lt;

  nn = GrdPprm->numnet;
  if (nn < 2)  return 0;
  vLOG(4)("GRD:GrdTune()");
  n=0; n0=0; n1=0; n2=0; i=0; j=0; i2=0; j2=0;
  ll = -1;
  lt = -1;
  for (n1=1; n1<nn; n1=n2+1) {
    p1 = AryPtr(GrdPtab, n1);  if (!p1)            eX(1);
    nl = p1->level;
    nt = p1->ntyp;
    for (n2=n1+1; n2<=nn; n2++) {
      p2 = AryPtr(GrdPtab, n2);  if (!p2)          eX(2);
      if (p2->level != nl)  break;
      }
    n2--;
    if (n2 == n1)  continue;
    if (nt < COMPLEX)  continue;
    xm = p1->xmin;
    ym = p1->ymin;
    dd = p1->delta;
    imin = 0;
    jmin = 0;
    imax = p1->nx;
    jmax = p1->ny;
    ia[0] = imin;  ie[0] = imax;
    ja[0] = jmin;  je[0] = jmax;
    for (n=1; n<=n2-n1; n++) {  /* adjust within same level */
      p2 = AryPtr(GrdPtab, n1+n);  if (!p2)      eX(3);
      ia[n] = floor((p2->xmin - xm)/dd + 0.5);
      ie[n] = ia[n] + p2->nx;
      if (imax < ie[n])  imax = ie[n];
      ja[n] = floor((p2->ymin - ym)/dd + 0.5);
      je[n] = ja[n] + p2->ny;
      if (jmax < je[n])  jmax = je[n];
      }  /* for n */
    for (i=imin; i<=imax; i++)
      for (j=jmin; j<=jmax; j++) {
        nv = 0;
        zm = 0;
        for (n=0; n<=n2-n1; n++) {
          if (i<ia[n] || i>ie[n] || j<ja[n] || j>je[n])  continue;
          p2 = AryPtr(GrdPtab, n1+n);  if (!p2)                 eX(4);
          psrf2 = p2->psrf;  if (!psrf2)                        eX(5);
          ps2 = AryPtr(psrf2, i-ia[n], j-ja[n]);  if (!ps2)     eX(6);
          zm += ps2->z;
          nv++;
          }
        if (!nv)  continue;
        zm /= nv;
        for (n=0; n<=n2-n1; n++) {
          if (i<ia[n] || i>ie[n] || j<ja[n] || j>je[n])  continue;
          p2 = AryPtr(GrdPtab, n1+n);  if (!p2)                 eX(7);
          psrf2 = p2->psrf;  if (!psrf2)                        eX(8);
          ps2 = AryPtr(psrf2, i-ia[n], j-ja[n]);  if (!ps2)     eX(9);
          ps2->z = zm;
          }
        }
    }  /* for */;

  n1 = nn;
  for (n2=nn; n2>0; n2=n1-1) {  /* use average from higher levels */
    p2 = AryPtr(GrdPtab,n2);  if (!p2)                     eX(10);
    if (n2==nn && p2->ntyp<COMPLEX)  break;
    ll = p2->level;
    for (n1=n2-1; n1>0; n1--) {
      p1 = AryPtr(GrdPtab,n1);  if (!p1)                   eX(11);
      if (p1->level != ll)  break;
      }
    n1++;                       /* n1..n2 is a range of fine grids */
    if (n1 == 1)  break;
    nl = p1->level;             /* level of next coarse grid */
    p1 = AryPtr(GrdPtab,n1);  if (!p1)                     eX(12);
    xm = p1->xmin;              /* edge of first fine grid, used */
    ym = p1->ymin;              /* as an arbitrary reference     */
    dd = p1->delta;
    for (n=0; n<=n2-n1; n++) {  /* calculation of virtual index range */
      p2 = AryPtr(GrdPtab, n1+n);  if (!p2)                eX(13);
      ia[n] = floor((p2->xmin - xm)/dd + 0.5);
      ie[n] = ia[n] + p2->nx;
      ja[n] = floor((p2->ymin - ym)/dd + 0.5);
      je[n] = ja[n] + p2->ny;
      }
    for (n0=n1-1; n0>0; n0--) {         /* loop through all coarse grids */
      p0 = AryPtr(GrdPtab,n0);  if (!p0)                   eX(14);
      if (p0->level != nl)  break;      /* not the next lower level */
      di = floor((p0->xmin - xm)/dd + 0.5);     /* shift for this grid */
      dj = floor((p0->ymin - ym)/dd + 0.5);
      psrf0 = p0->psrf;  if (!psrf0)                    eX(15);
      nx = p0->nx;
      ny = p0->ny;
      for (i=0; i<=nx; i++)
        for (j=0; j<=ny; j++) {
          ii = 2*i + di;
          jj = 2*j + dj;
          for (n=0; n<=n2-n1; n++) {
            if (ii<=ia[n] || ii>=ie[n] || jj<=ja[n] || jj>=je[n])  continue;
            i2 = ii - ia[n];                                    /*-22jan98-*/
            j2 = jj - ja[n];                                    /*-22jan98-*/
            ps0 = AryPtr(psrf0, i, j);  if (!ps0)          eX(16);
            p2 = AryPtr(GrdPtab, n1+n);  if (!p2)          eX(17);
            psrf2 = p2->psrf;  if (!psrf2)                 eX(18);
            ps2 = AryPtr(psrf2, i2  , j2  );  if (!ps2)    eX(19);
            zm = ps2->z;
            ps2 = AryPtr(psrf2, i2+1, j2  );  if (!ps2)    eX(20);
            zm += 0.5*ps2->z;
            ps2 = AryPtr(psrf2, i2-1, j2  );  if (!ps2)    eX(21);
            zm += 0.5*ps2->z;
            ps2 = AryPtr(psrf2, i2  , j2+1);  if (!ps2)    eX(22);
            zm += 0.5*ps2->z;
            ps2 = AryPtr(psrf2, i2  , j2-1);  if (!ps2)    eX(23);
            zm += 0.5*ps2->z;
            ps2 = AryPtr(psrf2, i2+1, j2+1);  if (!ps2)    eX(24);
            zm += 0.25*ps2->z;
            ps2 = AryPtr(psrf2, i2-1, j2-1);  if (!ps2)    eX(25);
            zm += 0.25*ps2->z;
            ps2 = AryPtr(psrf2, i2-1, j2+1);  if (!ps2)    eX(26);
            zm += 0.25*ps2->z;
            ps2 = AryPtr(psrf2, i2+1, j2-1);  if (!ps2)    eX(27);
            zm += 0.25*ps2->z;
            ps0->z = 0.25*zm;
            break;
            }
          } /* for j, i */
      }  /* for n0 */
    }  /* for n2 */

  for (n1=1; n1<=nn; n1=n2+1) {  /* adjust boundary values */
    p1 = AryPtr(GrdPtab, n1);  if (!p1)            eX(28);
    ll = p1->level;
    for (n2=n1+1; n2<=nn; n2++) {
      p2 = AryPtr(GrdPtab, n2);  if (!p2)          eX(29);
      if (p2->level != ll)  break;
      }
    n2--;
    if (n2 == nn)  break;
    nt = p2->ntyp;
    nl = p2->level;
    if (nt == FLAT1D)  continue;
    xm = p1->xmin;
    ym = p1->ymin;
    dd = p1->delta;
    for (n=0; n<=n2-n1; n++) {
      p2 = AryPtr(GrdPtab, n1+n);  if (!p2)        eX(30);
      ia[n] = floor((p2->xmin - xm)/dd + 0.5);
      ie[n] = ia[n] + p2->nx;
      ja[n] = floor((p2->ymin - ym)/dd + 0.5);
      je[n] = ja[n] + p2->ny;
      }
    for (n0=n2+1; n0<=nn; n0++) {
      p0 = AryPtr(GrdPtab, n0);  if (!p0)          eX(31);
      if (p0->level != nl)  break;
      psrf0 = p0->psrf;  if (!psrf0)            eX(32);
      nx = p0->nx;
      ny = p0->ny;
      //---------------------------- translated from LASAT      2005-03-11
      if (nx<6 || ny<6)                                          eX(70);
      di = floor((p0->xmin - xm)/dd + 0.5);
      dj = floor((p0->ymin - ym)/dd + 0.5);
      //
      // set values near lower and upper boundary
      //
      for (j=0; j<=ny; ) {                                    //-2002-09-23
        j2 = dj + j/2;
        for (i=0; i<=nx; i+=2) {  // set values for even i
          i2 = di + i/2;
          ps = AryPtr(psrf0, i, j);  if (!ps)                    eX(33);
          for (n=0; n<=n2-n1; n++) {
            if (i2<ia[n] || i2>ie[n] || j2<ja[n] || j2>je[n])  continue;
            p2 = AryPtr(GrdPtab, n1+n);  if (!p2)                eX(34);
            psrf2 = p2->psrf;  if (!psrf2)                       eX(35);
            ps2 = AryPtr(psrf2, i2-ia[n], j2-ja[n]);  if (!ps2)  eX(36);
            ps->z = ps2->z;
          }
        }
        for (i=1; i<nx; i+=2) { // interpolate values for odd i
          ps1 = AryPtr(psrf0, i, j);  if (!ps1)                  eX(37);
          ps0 = AryPtr(psrf0, i-1, j);  if (!ps0)                eX(38);
          ps2 = AryPtr(psrf0, i+1, j);  if (!ps2)                eX(39);
          ps1->z = 0.5*(ps0->z + ps2->z);
        }
        if (j == 2)  j = ny-2;                                //-2002-09-23
        else         j += 2;
      }  // for j
      //
      // interpolate for odd j
      //
      for (j=1; j<ny; j+=ny-2) {
        for (i=0; i<=nx; i++) {
          ps1 = AryPtr(psrf0, i, j);  if (!ps1)                  eX(61);
          ps0 = AryPtr(psrf0, i, j-1);  if (!ps0)                eX(62);
          ps2 = AryPtr(psrf0, i, j+1);  if (!ps2)                eX(63);
          ps1->z = 0.5*(ps0->z + ps2->z);
        }
      }
      //
      // set values near left and right boundary
      //
      for (i=0; i<=nx; ) {                                    //-2002-09-23
        i2 = di + i/2;
        for (j=2; j<ny; j+=2) { // set values for even j
          j2 = dj + j/2;
          ps = AryPtr(psrf0, i, j);  if (!ps)                    eX(40);
          for (n=0; n<=n2-n1; n++) {
            if (i2<ia[n] || i2>ie[n] || j2<ja[n] || j2>je[n])  continue;
            p2 = AryPtr(GrdPtab, n1+n);  if (!p2)                eX(41);
            psrf2 = p2->psrf;  if (!psrf2)                       eX(42);
            ps2 = AryPtr(psrf2, i2-ia[n], j2-ja[n]);  if (!ps2)  eX(43);
            ps->z = ps2->z;
          }
        }
        for (j=1; j<ny; j+=2) { // interpolate values for odd j
          ps1 = AryPtr(psrf0, i, j);  if (!ps1)                  eX(44);
          ps0 = AryPtr(psrf0, i, j-1);  if (!ps0)                eX(45);
          ps2 = AryPtr(psrf0, i, j+1);  if (!ps2)                eX(46);
          ps1->z = 0.5*(ps0->z + ps2->z);
        }
        if (i == 2)  i = nx-2;
        else         i += 2;
      }  // for i
      //
      // interpolate for odd i
      //
      for (i=1; i<nx; i+=nx-2) {
        for (j=0; j<=ny; j++) {
          ps1 = AryPtr(psrf0, i, j);  if (!ps1)                  eX(64);
          ps0 = AryPtr(psrf0, i-1, j);  if (!ps0)                eX(65);
          ps2 = AryPtr(psrf0, i+1, j);  if (!ps2)                eX(66);
          ps1->z = 0.5*(ps0->z + ps2->z);
        }
      }
      //--------------------------------------------------------------
    }  /* for n0 */
  }  /* for n1 */

  for (n1=1; n1<=nn; n1=n2+1) { /* adjust within same level */
    p1 = AryPtr(GrdPtab, n1);  if (!p1)            eX(47);
    nl = p1->level;
    for (n2=n1+1; n2<=nn; n2++) {
      p2 = AryPtr(GrdPtab, n2);  if (!p2)          eX(48);
      if (p2->level != nl)  break;
      }
    n2--;
    if (n1 == n2)  continue;
    nt = p1->ntyp;
    if (nt == FLAT1D)  continue;
    xm = p1->xmin;
    ym = p1->ymin;
    dd = p1->delta;
    for (n=0; n<=n2-n1; n++) {
      p2 = AryPtr(GrdPtab, n1+n);  if (!p2)                eX(49);
      ia[n] = floor((p2->xmin - xm)/dd + 0.5);
      ie[n] = ia[n] + p2->nx;
      ja[n] = floor((p2->ymin - ym)/dd + 0.5);
      je[n] = ja[n] + p2->ny;
      }
    for (n=0; n<=n2-n1; n++) {
      p1 = AryPtr(GrdPtab, n1+n);  if (!p1)                eX(50);
      psrf1 = p1->psrf;  if (!psrf1)                       eX(51);
      nx = p1->nx;
      ny = p1->ny;
      for (m=0; m<=n2-n1; m++) {
        if (n == m)  continue;
        if (ia[m]>=ie[n] || ie[m]<=ia[n])  continue;
        if (ja[m]>=je[n] || je[m]<=ja[n])  continue;
        p2 = AryPtr(GrdPtab, n1+m);  if (!p2)              eX(52);
        psrf2 = p2->psrf;  if (!psrf2)                     eX(53);
        for (j=0; j<=ny; j+=ny) {
          j2 = j + ja[n] - ja[m];
          if (j2<0 || j2>p2->ny)  continue;
          for (i=0; i<=nx; i++) {
            i2 = i + ia[n] - ia[m];
            if (i2<0 || i2>p2->nx)  continue;
            ps1 = AryPtr(psrf1, i, j);  if (!ps1)          eX(54);
            ps2 = AryPtr(psrf2, i2, j2);  if (!ps2)        eX(55);
            ps2->z = ps1->z;
            }
          }
        for (i=0; i<=nx; i+=nx) {
          i2 = i + ia[n] - ia[m];
          if (i2<0 || i2>p2->nx)  continue;
          for (j=1; j<ny; j++) {
            j2 = j + ja[n] - ja[m];
            if (j2<0 || j2>p2->ny)  continue;
            ps1 = AryPtr(psrf1, i, j);  if (!ps1)          eX(56);
            ps2 = AryPtr(psrf2, i2, j2);  if (!ps2)        eX(57);
            ps2->z = ps1->z;
            }
          }
        } /* for m */
      }  /* for n */
    }  /* for n1 */
  return 0;
eX_1:  eX_2:  eX_3:  eX_4:  eX_5:  eX_6:  eX_7:  eX_8:  eX_9:  eX_10:
eX_11: eX_12: eX_13: eX_14: eX_15: eX_16: eX_17: eX_18: eX_19: eX_20:
eX_21: eX_22: eX_23: eX_24: eX_25: eX_26: eX_27: eX_28: eX_29: eX_30:
eX_31: eX_32: eX_33: eX_34: eX_35: eX_36: eX_37: eX_38: eX_39: eX_40:
eX_41: eX_42: eX_43: eX_44: eX_45: eX_46: eX_47: eX_48: eX_49: eX_50:
eX_51: eX_52: eX_53: eX_54: eX_55: eX_56: eX_57:
eX_61: eX_62: eX_63: eX_64: eX_65: eX_66:
  eMSG(_internal_error_$_, n, n0, n1, n2, i, j, i2, j2);
eX_70: eMSG(_minimum_size_);
  }


//==================================================================== GrdList
//
long GrdList( void )     /* Auflistung von NETarr */
  {
  /* dP(GRD:GrdList); */
  int n;
  NETREC *p;
  vLOG(4)("GRD:GrdList()");
  if (!GrdPtab)  return 0;
  fprintf(MsgFile, "Nr Name     Lv In Nt Pt Nx Ny Nz   " );        /*29apr92*/
  fprintf(MsgFile, "Dd   Xmin     X1     X2   Ymin     Y1     Y2 Rfac\n");
  p = (NETREC*) GrdPtab->start;
  for (n=0; n<GrdPprm->numnet; n++, p++) {
    fprintf(MsgFile, "%2d %-8s %2.0f %2.0f %2.0f %2.0f %2.0f %2.0f %2.0f",
      n+1, p->name, p->level, p->index, p->ndim, p->ptyp, p->nx, p->ny,
      p->nz );
    fprintf(MsgFile, " %4.0f %6.0f %6.0f %6.0f %6.0f %6.0f %6.0f %4.2f\n",
      p->delta, p->xmin, p->x1, p->x2, p->ymin, p->y1, p->y2, p->rfac );
    }
  return 0; }

/*================================================================= GrdHeader
*/
#define CAT(a, b)  TxtPrintf(&txt, a, b), TxtCat(&hdr, txt.s);
char *GrdHeader(       /* File-Header mit Grid-Information erstellen */
long ident )           /* Identifizierung der Daten                  */
  {
  dP(GrdHeader);
  int nl, ni, gp, net, k, nzmax;
  float xmin, ymin, delta, *pf;
  static TXTSTR hdr = { NULL, 0 };
  TXTSTR txt = { NULL, 0 };                                       //-2011-06-29
  NETREC *pn;
  vLOG(4)("GRD:GrdHeader(%08lx)", ident);
  if (!GrdPprm)                                                         eX(99);
  TxtCpy(&hdr, "\n");
  nl = XTR_LEVEL(ident);
  ni = XTR_GRIDN(ident);
  gp = XTR_GROUP(ident);
  if (GrdPprm->bd & GRD_GEOGK) {
    CAT("radius  %7.2f\n", GrdPprm->radius);
    CAT("oref    %7.2f\n", GrdPprm->oref);
    CAT("pref    %7.2f\n", GrdPprm->pref);
    CAT("oa      %7.2f\n", GrdPprm->omin);
    CAT("pa      %7.2f\n", GrdPprm->pmin);
    CAT("do      %7.2f\n", GrdPprm->delo);
    CAT("dp      %7.2f\n", GrdPprm->delp);
  }
  else {
    if (GrdPprm->numnet == 0) {
      xmin = GrdPprm->xmin;
      ymin = GrdPprm->ymin;
      delta = GrdPprm->dd; }
    else {
      if ((nl == 0) && (ni == 0)) 
        return hdr.s;
      if (!GrdPtab)                                                     eX(1);
      for (net=1; net<=GrdPprm->numnet; net++) {
        pn = (NETREC*) AryPtr( GrdPtab, net );
        if (pn == NULL)  
          return hdr.s;
        if ((pn->level == nl) && (pn->index == ni))  break;
        }
      if (net > GrdPprm->numnet)  
        return hdr.s;
      xmin = pn->xmin;
      ymin = pn->ymin;
      delta = pn->delta;
    }
    CAT("xmin  %s\n", TxtForm(xmin,6));
    CAT("ymin  %s\n", TxtForm(ymin,6));
    CAT("delta %s\n", TxtForm(delta,6));
    CAT("refx  %d\n", GrdPprm->refx);
    CAT("refy  %d\n", GrdPprm->refy);   
    if (GrdPprm->refx > 0 && *GrdPprm->ggcs)                     //-2008-12-11
      CAT("ggcs  \"%s\"\n", GrdPprm->ggcs);                      //-2008-12-11
  }
  if (!GrdParr)                                                         eX(2);
  TxtCat(&hdr, "sk " );
  pf = (float*) GrdParr->start;
  nzmax = GrdParr->bound[0].hgh;
  for (k=0; k<=nzmax; k++) {
    CAT(" %s", TxtForm(pf[k],6));
  }
  TxtCat(&hdr, "\n");
  TxtClr(&txt);
  return hdr.s;
eX_1:
  nMSG(_no_grdtab_);
  return NULL;
eX_2:
  nMSG(_no_grdarr_);
  return NULL;
eX_99:
  nMSG(_not_initialized_);
  return NULL;
}
#undef CAT

//==================================================================== GrdRead
//
long GrdRead(          /* File GRID.DEF einlesen und Werte setzen.  */
char *altname )        /* Alternativer File-Name                    */
  {
  dP(GrdRead);
  long rc, id, ndat;
  char flags[80], t[80], hdr[GENTABLEN], name[40], ntypstr[40];
  int i, j, k, n, highest_ntyp;
  ARYDSC *pa, *pt, *pp;
  enum NET_TYPE ntyp;
  NETREC *pnet;
  float *pdat;
  float *pf;
  vLOG(3)("reading grid.def ...");
  if (!GrdPprm) {
    id = IDENT(GRDpar, 0, 0, 0);
    pp = TmnCreate(id, sizeof(GRDPARM), 1, 0, 0);  if(!pp)              eX(40);
    GrdPprm = (GRDPARM*) pp->start;
    TmnDetach(id, NULL, NULL, TMN_MODIFY|TMN_FIXED, NULL);              eG(41);
    }
  if (0 > OpenInput("grid.def", altname))                               eX(1);
  Buf = ALLOC(GENTABLEN);  if (!Buf)                                    eX(16);
  rc = GetLine('.', Buf, GENTABLEN);  if (rc <= 0)                      eX(2);
  GrdPprm->refx = GRD_DEFREFX;                                    //-2008-12-04
  GetData("GAKRX|REFX", Buf, "%d", &GrdPprm->refx);       eG(50); //-2008-12-04
  GrdPprm->refy = GRD_DEFREFY;                                    //-2008-12-04
  GetData("GAKRY|REFY", Buf, "%d", &GrdPprm->refy);       eG(51); //-2008-12-04
  if (GrdPprm->refx > 0) {                                        //-2008-12-04
    int zone;
    zone = GrdPprm->refx/1000000;
    if (zone > 0) {
      if (zone>=2 && zone<=5)
        strcpy(GrdPprm->ggcs, "GK");
      else
        strcpy(GrdPprm->ggcs, "UTM");
    }
  }
  GetData("GGCS", Buf, "%15s", GrdPprm->ggcs);            eG(58); //-2008-12-04
  rc = GetData("Sk|Zk", Buf, "%[101]f", DatVec );
  if (rc < 2)                                                           eX(3);
  GrdPprm->nzmax = rc - 1;
  id = IDENT(GRDarr, 0, 0, 0);
  pa = TmnCreate(id, sizeof(float), 1, 0, GrdPprm->nzmax);              eG(4);
  GrdParr = pa;
  for (i=0, pf=pa->start; i<=GrdPprm->nzmax; i++, pf++)  *pf = DatVec[i];
  for (i=1, pf=pa->start; i<=GrdPprm->nzmax; i++) {
    if (pf[i] <= pf[i-1])                                               eX(44);
  }
  TmnDetach(id, NULL, NULL, TMN_MODIFY|TMN_FIXED, NULL);                eG(8);
  GrdPprm->nz = GrdPprm->nzmax;
  GrdPprm->nzmap = GrdPprm->nzmax;
  GetData("NZM", Buf, "%d", &GrdPprm->nzmap);                           eG(52);
  if ((GrdPprm->nzmap > GrdPprm->nzmax) || (GrdPprm->nzmap < 1))
    GrdPprm->nzmap = GrdPprm->nzmax;
  rc = GetData("NZD", Buf, "%d", &i);                                   eG(53);
  if (rc > 0) {
    if ((i > GrdPprm->nzmap) || (i < 1))                                eX(5);
    GrdPprm->nzdos = i; }
  else  GrdPprm->nzdos = GrdPprm->nzmap;
  GrdPprm->zscl = 0;
  GetData("ZSCL", Buf, "%f", &GrdPprm->zscl);                           eG(54);
  //-2005-01-12
  if (GrdPprm->zscl != 0)                                               eX(92);
  if (GrdPprm->zscl < 0)  GrdPprm->zscl = 0;
  GrdPprm->sscl = GrdPprm->zscl;
  if (GrdPprm->zscl > 0) {
    GetData("SSCL", Buf, "%f", &GrdPprm->sscl);                         eG(68);
    if (GrdPprm->sscl <= 0)  GrdPprm->sscl = GrdPprm->zscl;
    }
  GrdPprm->hmax = 0;
  GetData("HMAX|ZMAX", Buf, "%f", &GrdPprm->hmax);                      eG(67);
  GetData("FLAGS", Buf, "%78s", flags);                                 eG(55);
  if (flags[0] != 0) {
    SetFlag( flags, "GEOGK",  &GrdPprm->flags, GRD_GEOGK );
    SetFlag( flags, "NESTED", &GrdPprm->flags, GRD_NESTED );
    SetFlag( flags, "SOLID",  &GrdPprm->flags, GRD_SOLID );
    SetFlag( flags, "BODIES", &GrdPprm->flags, GRD_BODIES );
    SetFlag( flags, "GRID",   &GrdPprm->flags, GRD_GRID );
    SetFlag( flags, "DMKHM",  &GrdPprm->flags, GRD_DMKHM ); //-2005-02-15
    }
  ntyp = FLAT1D;
  rc = GetData("NTYP|NTYPE|Nt", Buf, "%38s", &ntypstr);
  if (rc > 0) {
    int nt = 0;
    ntyp = NOnet;
    rc = sscanf(ntypstr, "%d", &nt);
    ntyp = (enum NET_TYPE)nt;
    if (rc < 1) {
      for (n=0; n<strlen(ntypstr); n++)  ntypstr[n] = toupper(ntypstr[n]);
      if      (strstr(ntypstr, "FLAT1D"))  ntyp = FLAT1D;
      else if (strstr(ntypstr, "FLAT3D"))  ntyp = FLAT3D;
      else if (strstr(ntypstr, "COMPLEX")) ntyp = COMPLEX;
      else if (strstr(ntypstr, "DEGREE"))  ntyp = DEGREE;
      else                                                              eX(72);
      }
    else if (ntyp<FLAT1D || ntyp>DEGREE)                                eX(73);
    }
  if (GrdPprm->flags & GRD_GEOGK)  ntyp = DEGREE;
  if (GrdPprm->flags & GRD_NESTED) ntyp = NOnet;
  else {
    if ((GrdPprm->zscl) && (ntyp != COMPLEX))                           eX(70);
    rc = GetData("NX", Buf, "%d", &i);  if (rc <= 0)                    eX(6);
    GrdPprm->nx = i;
    rc = GetData("NY", Buf, "%d", &i);  if (rc <= 0)                    eX(7);
    GrdPprm->ny = i;
    GrdPprm->itermax = StdNetDat[15];
    GetData( "Im", Buf, "%d", &GrdPprm->itermax );                      eG(10);
    GrdPprm->iteranl = StdNetDat[16];
    GetData( "Ia", Buf, "%d", &GrdPprm->iteranl );                      eG(11);
    GrdPprm->iterrad = StdNetDat[17];
    GetData( "Ir", Buf, "%f", &GrdPprm->iterrad );                      eG(12);
    GrdPprm->itereps = StdNetDat[18];
    GetData( "Ie", Buf, "%f", &GrdPprm->itereps );                      eG(13);
    GetData( "O0", Buf, "%f", &GrdPprm->o0 );                           eG(80);
    GetData( "O2", Buf, "%f", &GrdPprm->o2 );                           eG(81);
    GetData( "O4", Buf, "%f", &GrdPprm->o4 );                           eG(82);
    }
  GrdPprm->ntyp = ntyp;
  if (ntyp == DEGREE) {
    GrdPprm->bd = GRD_REFZ | GRD_GEOGK;
    rc = GetAllData("RADIUS OREF PREF OA DO PA DP",
                    Buf, "%f", &GrdPprm->radius);                       eG(56);
    if (rc == 0)                                                        eX(14);
    GrdPprm->radfak = asin( (double)1.0 )/90;
    GrdPprm->cosref = cos(GrdPprm->radfak*GrdPprm->pref);
    GrdPprm->sinref = sin(GrdPprm->radfak*GrdPprm->pref);
    }
  GrdPprm->defmode = GRD_AKWC;
  rc = GetData("DEFMODE", Buf, "%d", &GrdPprm->defmode);                eG(57);
  GrdPprm->bd = 0;                                          /*-01feb97-*/
  i = GetData("BCZ", Buf, "%s", t );                                    eG(64);
  if (i > 0) {
    if (toupper(t[0]) == 'R')  GrdPprm->bd |=  GRD_REFZ;
    else                       GrdPprm->bd &= ~GRD_REFZ;
    }
  if ((ntyp == FLAT1D) || (ntyp == FLAT3D) || (ntyp == COMPLEX)) {
    rc = GetData("DELTA|DD", Buf, "%f", &GrdPprm->dd);  if (rc <= 0)    eX(15);
    GrdPprm->xmin = 0.0;
    rc = GetData( "XMIN", Buf, "%f", &GrdPprm->xmin );                  eG(59);
    GrdPprm->ymin = 0.0;
    rc = GetData( "YMIN", Buf, "%f", &GrdPprm->ymin );                  eG(60);
    GrdPprm->xmax = GrdPprm->xmin + GrdPprm->nx*GrdPprm->dd;
    GrdPprm->ymax = GrdPprm->ymin + GrdPprm->ny*GrdPprm->dd;
  if (fabs(GrdPprm->xmin)>GRD_XYMAX || fabs(GrdPprm->xmax)>GRD_XYMAX
   || fabs(GrdPprm->ymin)>GRD_XYMAX || fabs(GrdPprm->ymax)>GRD_XYMAX)   eX(90);
    GrdPprm->border = 0.0;
    GetData( "RAND", Buf, "%f", &GrdPprm->border );                     eG(61);
    i = GetData("BCX", Buf, "%s", t );                                  eG(62);
    if (i > 0)  if (toupper(t[0]) == 'P')  GrdPprm->bd |= GRD_PERX;
    i = GetData("BCY", Buf, "%s", t );                                  eG(63);
    if (i > 0)  if (toupper(t[0]) == 'P')  GrdPprm->bd |= GRD_PERY;
    GrdPprm->chi = GRD_DEFCHI;
    rc = GetData( "CHI", Buf, "%f", &GrdPprm->chi );                    eG(65);
    if (GrdPprm->chi < 0)  GrdPprm->chi = 0;
    if (GrdPprm->chi > 1)  GrdPprm->chi = 1;
    }
  GrdPtab = NULL;
  if (ntyp == NOnet) {                       /* Netz-Schachtelung */
    rc = CntLines( '.', Buf, GENTABLEN);  if (rc < 0)                   eX(20);
    if (*Buf != '!')                                                    eX(21);
    n = strspn( Buf+1, "N" );  if (n < 1)                               eX(22);
    id = IDENT( GRDtab, 0, 0, 0 );
    pt = TmnCreate(id, sizeof(NETREC), 1, 1, n);                        eG(23);
    GrdPtab = pt;
    TmnDetach(id, NULL, NULL, TMN_MODIFY|TMN_FIXED, NULL);              eG(26);
    GrdPprm->numnet = n;
    rc = GetLine( '!', hdr, 200 );  if (rc <= 0)                        eX(24);
    ndat = GenEvlHdr(NetNames, hdr, DatPos, DatScale, 100); if (ndat<1) eX(25);
    highest_ntyp = -1;
    pnet = (NETREC*) pt->start;
    for (i=0; i<n; i++, pnet++) {
      rc = GetLine( 'N', Buf, GENTABLEN );  if (rc <= 0)                eX(27);
      rc = GetList( Buf, name, 15, DatVec, 100 );  if (rc < 0)          eX(91);
      if (rc != ndat)                                                   eX(28);
      strcpy(pnet->name, name);
      pdat = &(pnet->level);
      for (k=0; k<NumNetDat; k++)  pdat[k] = StdNetDat[k];
      for (j=0; j<ndat; j++) {
        k = DatPos[j];
        if (k < 0)  continue;
        if (IsUndef(&DatVec[j]))                                        eX(29);
        pdat[k] = DatVec[j]*DatScale[j];
        }
      pnet->ntyp = pnet->ndim;
      if (highest_ntyp < pnet->ntyp)  highest_ntyp = pnet->ntyp;
      if ((GrdPprm->zscl) && (pnet->ntyp!=COMPLEX))                     eX(71);
      }
    if ((highest_ntyp > 1) && (StdStatus & 0x01000000))
      GrdPprm->flags |= GRD_BODIES;
    }
  else {
    GrdPprm->prfmode = 3;
    GetData("PTYP|PTYPE|Pt", Buf, "%d", &GrdPprm->prfmode );            eG(66);
    }

  CloseInput();
  vLOG(3)("... grid.def closed.");
  if (GrdPprm->flags & GRD_BODIES) {
    id = IDENT(SLDarr, 1, 0, 0);
    TmnVerify(id, NULL, NULL, 0);                                       eG(31);
  }
  GrdCheck();                                                           eG(30);
  GrdTune();                                                            eG(33);
  GrdVerify();                                                          eG(32);
  FREE(Buf);
  return 0;
eX_1:
  eMSG(_def_not_found_);
eX_2:
  eMSG(_assignment_not_found_);
eX_3:
  eMSG(_invalid_vertical_grid_);
eX_4:  eX_8:
  eMSG(_grdarr_not_created_);
eX_5:
  eMSG(_invalid_nzd_);
eX_6:
  eMSG(_undefined_nx_);
eX_7:
  eMSG(_undefined_ny_);
eX_10: eX_11: eX_12: eX_13: eX_80: eX_81: eX_82:
  eMSG(_error_iteration_);
eX_14:
  eMSG(_incomplete_data_);
eX_15:
  eMSG(_undefined_delta_);
eX_16:
  eMSG(_cant_allocate_buffer_);
eX_20:
  eMSG(_cant_read_nesting_);
eX_21: eX_24: eX_27:
  eMSG(_invalid_structure_);
eX_22:
  eMSG(_undefined_nesting_);
eX_23: eX_26:
  eMSG(_cant_create_grdtab_);
eX_25:
  eMSG(_cant_read_header_);
eX_28:
  eMSG(_inconsistent_count_);
eX_29:
  eMSG(_undefined_value_tab_);
eX_30:
  eMSG(_error_structure_);
eX_31:
  eMSG(_cant_get_buildings_);
eX_32:
  eMSG(_cant_verify_extensions_);
eX_33:
  eMSG(_cant_adjust_grids_);
eX_40: eX_41:
  eMSG(_cant_init_);
eX_44:
  eMSG(_vertical_grid_);
eX_50: eX_51: eX_52: eX_53: eX_54: eX_55: eX_56: eX_57: eX_58: eX_59:
eX_60: eX_61: eX_62: eX_63: eX_64: eX_65: eX_66: eX_67: eX_68:
  eMSG(_syntax_error_);
eX_70: eX_71:
  eMSG(_zscl_0_required_);
eX_72: eX_73:
  eMSG(_$_invalid_nt_, ntypstr);
eX_90:
  eMSG(_size_exceeded_);
eX_91:
  eMSG(_cant_read_name_);
eX_92:
  eMSG(_zscl_sscl_0_);
  }

//================================================================= GrdSetNet
//
long GrdSetNet(            /* Netz auflegen */
int net )                  /* Netz-Nummer   */
  {
  dP(GrdSetNet);
  NETREC *pn;
  vLOG(5)("GRD:GrdSetNet(%d)", net);
  if (!GrdPprm)                                                         eX(99);
  if (net==0 && GrdPprm->numnet==0)  return 0;
  if ((net < 1) || (net > GrdPprm->numnet))                             eX(1);
  if (!GrdPtab)                                                         eX(2);
  pn = (NETREC*) AryPtr(GrdPtab, net);  if (!pn)                        eX(3);
  GrdPprm->level = pn->level;
  GrdPprm->index = pn->index;
  GrdPprm->dd = pn->delta;
  GrdPprm->nx = pn->nx;
  GrdPprm->xmin = pn->xmin;
  GrdPprm->x1 = pn->x1;
  GrdPprm->x2 = pn->x2;
  GrdPprm->xmax = GrdPprm->xmin + GrdPprm->nx*GrdPprm->dd;
  GrdPprm->ny = pn->ny;
  GrdPprm->ymin = pn->ymin;
  GrdPprm->y1 = pn->y1;
  GrdPprm->y2 = pn->y2;
  GrdPprm->ymax = GrdPprm->ymin + GrdPprm->ny*GrdPprm->dd;
  GrdPprm->zmin = pn->zmin;
  GrdPprm->zavr = pn->zavr;
  GrdPprm->zmax = pn->zmax;
  GrdPprm->nz = pn->nz;
  GrdPprm->ndim = pn->ndim;
  GrdPprm->prfmode = pn->ptyp;
  GrdPprm->defmode = pn->defmode;
  GrdPprm->ntyp = pn->ntyp;
  strcpy( GrdPprm->name, pn->name );
  GrdPprm->rfac = pn->rfac;
  GrdPprm->itermax = pn->itermax;
  GrdPprm->iteranl = pn->iteranl;
  GrdPprm->iterrad = pn->iterrad;
  GrdPprm->itereps = pn->itereps;
  GrdPprm->o0 = pn->o0;
  GrdPprm->o2 = pn->o2;
  GrdPprm->o4 = pn->o4;
  GrdPprm->psrf = pn->psrf;
  GrdPprm->pgrd = pn->pgrd;
  return 0;
eX_1:
  eMSG(_invalid_index_);
eX_2:  eX_3:
  eMSG(_undefined_value_);
eX_99:
  eMSG(_not_initialized_);
  }

//==================================================================== GrdSet
//
long GrdSet(               /* Netz auflegen */
int netlevel,              /* Netz-Level    */
int netindex )             /* Netz-Index    */
  {
  dP(GrdSet);
  NETREC *pn;
  int net;
  long rc;
  vLOG(5)("GRD:GrdSet(%d,%d)", netlevel, netindex);
  if (!GrdPprm)                                                         eX(99);
  if (GrdPprm->numnet == 0) {
    GrdPprm->xmax = GrdPprm->xmin + GrdPprm->nx*GrdPprm->dd;
    GrdPprm->ymax = GrdPprm->ymin + GrdPprm->ny*GrdPprm->dd;
    GrdPprm->level = 0;
    GrdPprm->index = 0;
    return 0; }
  if (!GrdPtab)                                                         eX(1);
  for (net=1; net<=GrdPprm->numnet; net++) {
    pn = (NETREC*) AryPtr(GrdPtab, net);  if (!pn)                      eX(2);
    if ((pn->level == netlevel) && (pn->index == netindex))  break;
    }
  if (net > GrdPprm->numnet)                                            eX(3);
  rc = GrdSetNet( net );                                                eG(4);
  return rc;
eX_1:  eX_2:
  eMSG(_undefined_grdtab_);
eX_3:  eX_4:
  eMSG(_invalid_grid_);
eX_99:
  eMSG(_not_initialized_);
  }

//============================================================= GrdSetDefMode
//
long GrdSetDefMode( int mode )
  {
  dP(GrdSetDefMode);
  NETREC *pn;
  int net;
  vLOG(5)("GRD:GrdSetDefMode(%d)", mode);
  if (!GrdPprm)                                                         eX(99);
  GrdPprm->defmode = mode;
  if (GrdPprm->numnet == 0)  return 0;
  if (!GrdPtab)                                                         eX(1);
  for (net=1; net<=GrdPprm->numnet; net++) {
    pn = (NETREC*) AryPtr(GrdPtab, net);  if (!pn)                      eX(2);
    if ((pn->level==GrdPprm->level) && (pn->index==GrdPprm->index)) break;
    }
  if (net > GrdPprm->numnet)                                            eX(3);
  pn->defmode = mode;
  return 0;
eX_1:  eX_2:
  eMSG(_undefined_grdtab_);
eX_3:
  eMSG(_invalid_grid_);
eX_99:
  eMSG(_not_initialized_);
  }

//=============================================================== GrdNxtLevel
//
int GrdNxtLevel(      /* Nächst niedrigeren Grid-Level heraussuchen */
int level )           /* Momentaner Level                           */
  {
  dP(GrdNxtLevel);
  int n, l;
  NETREC *pn;
  vLOG(5)("GRD:GrdNxtLevel(%d)", level);
  if (!GrdPprm)                                                 eX(99);
  if (!GrdPtab)                                                 eX(1);
  pn = (NETREC*) GrdPtab->start;
  l = -1;
  for (n=0; n<GrdPprm->numnet; n++, pn++) {
    if (pn->level >= level)  break;
    l = pn->level; }
  return l;
eX_1:
  eMSG(_invalid_grid_level_);
eX_99:
  eMSG(_not_initialized_);
  }

//=============================================================== GrdLstLevel
//
int GrdLstLevel(      /* Nächst höheren Grid-Level heraussuchen     */
int level )           /* Momentaner Level                           */
  {
  dP(GrdLstLevel);
  int n, l;
  NETREC *pn;
  vLOG(5)("GRD:GrdLstLevel(%d)", level);
  if (!GrdPprm)                                                 eX(99);
  if (!GrdPtab)                                                 eX(1);
  pn = (NETREC*) GrdPtab->start;
  l = -1;
  for (n=0; n<GrdPprm->numnet; n++, pn++) {
    if (pn->level > level) { l = pn->level;  break; }
    }
  return l;
eX_1:
  eMSG(_invalid_grid_level_);
eX_99:
  eMSG(_not_initialized_);
  }

//====================================================================== GrdIx
//
int GrdIx( float x )
  {
  dP(GrdIx);
  int i;
  if (!GrdPprm)                                                         eX(99);
  if (x < GrdPprm->xmin)  x = GrdPprm->xmin;
  i = 1 + (x - GrdPprm->xmin)/GrdPprm->dd;
  if (i > GrdPprm->nx)  i = GrdPprm->nx;
  return i;
eX_99:
  eMSG(_not_initialized_);
  }

//====================================================================== GrdJy
//
int GrdJy( float y )
  {
  dP(GrdJy);
  int j;
  if (!GrdPprm)                                                         eX(99);
  if (y < GrdPprm->ymin)  y = GrdPprm->ymin;
  j = 1 + (y - GrdPprm->ymin)/GrdPprm->dd;
  if (j > GrdPprm->ny)  j = GrdPprm->ny;
  return j;
eX_99:
  eMSG(_not_initialized_);
  }

//====================================================================== GrdZb
//
float GrdZb( float x, float y )
  {
  dP(GrdZb);
  int i, j;
  float z, z0, z1, z00, z01, z10, z11, ax, ay;
  float dd, d, xmin, xmax, ymin, ymax;
  if (!GrdPprm)                                                         eX(99);
  if ((!GrdPprm->psrf) || (!GrdPprm->psrf->start))  return 0;
  dd = GrdPprm->dd;
  d = 0.01*dd;
  xmin = GrdPprm->xmin;
  xmax = GrdPprm->xmax;
  ymin = GrdPprm->ymin;
  ymax = GrdPprm->ymax;
  if (x<xmin-d || x>xmax+d || y<ymin-d || y>ymax+d)     eX(1);
  i = GrdIx(x);
  j = GrdJy(y);
  z00 = iBOT( i-1, j-1 );
  z10 = iBOT( i  , j-1 );
  z01 = iBOT( i-1, j   );
  z11 = iBOT( i  , j   );
  ax = (x - xmin)/dd - (i-1);
  ay = (y - ymin)/dd - (j-1);
  z0 = z00 + ay*(z01 - z00);
  z1 = z10 + ay*(z11 - z10);
  z = z0 + ax*(z1 - z0);
  return z;
eX_1:
  nMSG(_point_$$_outside_$$_, x, y, GrdPprm->level, GrdPprm->index);
  return 0;
eX_99:
  nMSG(_not_initialized_);
  return 0;
  }

//================================================================= GrdBottom
//
int GrdBottom( float x, float y, float *pzb )
  {
  dP(GrdBottom);
  int i, j, n, nx, ny;
  float z, z0, z1, z00, z01, z10, z11, ax, ay;
  float dd, d, xmin, xmax, ymin, ymax;
  ARYDSC *psrf;
  NETREC *pn;
  if (!GrdPprm)                                         eX(99);
  if (pzb)  *pzb = 0;
  if (GrdPprm->numnet < 2) {
    n = GrdPprm->numnet;
    psrf = GrdPprm->psrf;
    if (!psrf)  return n;
    dd = GrdPprm->dd;
    d = 0.01*dd;
    xmin = GrdPprm->xmin;
    xmax = GrdPprm->xmax;
    ymin = GrdPprm->ymin;
    ymax = GrdPprm->ymax;
    nx = GrdPprm->nx;
    ny = GrdPprm->ny;
    if (x<xmin-d || x>xmax+d || y<ymin-d || y>ymax+d)  return -1;
    }
  else {
    for (n=GrdPprm->numnet; n>0; n--) {
      pn = AryPtr(GrdPtab, n);  if (!pn)                eX(1);
      dd = pn->delta;
      d = 0.01*dd;
      if (x>=pn->xmin-d && x<=pn->xmax+d &&
          y>=pn->ymin-d && y<=pn->ymax+d)  break;
      }
    if (n <= 0)                                         eX(2);
    xmin = pn->xmin;
    xmax = pn->xmax;
    ymin = pn->ymin;
    ymax = pn->ymax;
    psrf = pn->psrf;
    nx = pn->nx;
    ny = pn->ny;
    }
  if (!psrf) { if (pzb) *pzb = 0;  return n;  }

  ax = (x - xmin)/dd;
  i = 1 + floor(ax);  if (i > nx)  i = nx;  else if (i < 1)  i = 1;
  ay = (y - ymin)/dd;
  j = 1 + floor(ay);  if (j > ny)  j = ny;  else if (j < 1)  j = 1;
  z00 = *(float*)AryPtrX(psrf, i-1, j-1);
  z10 = *(float*)AryPtrX(psrf, i  , j-1);
  z01 = *(float*)AryPtrX(psrf, i-1, j  );
  z11 = *(float*)AryPtrX(psrf, i  , j  );
  ax -= i-1;  if (ax > 1)  ax = 1;  else if (ax < 0)  ax = 0;
  ay -= j-1;  if (ay > 1)  ay = 1;  else if (ay < 0)  ay = 0;
  z0 = z00 + ay*(z01 - z00);
  z1 = z10 + ay*(z11 - z10);
  z = z0 + ax*(z1 - z0);
  if (pzb)  *pzb = z;
  return n;
eX_1:
  nMSG(_cant_find_$_, n);
  return 0;
eX_2:
  nMSG(_point_$$_outside_, x, y);
  return 0;
eX_99:
  nMSG(_not_initialized_);
  return 0;
  }

//====================================================================== GrdZt
//
float GrdZt( float x, float y )
  {
  dP(GrdZt);
  int i, j, nz;
  float z, z0, z1, z00, z01, z10, z11, ax, ay;
  float dd, d, xmin, xmax, ymin, ymax;
  ARYDSC *pgrd;
  if (!GrdPprm)                                                     eX(99);
  nz = GrdPprm->nz;
  pgrd = GrdPprm->pgrd;
  if ((!pgrd) || (!pgrd->start)) return *(float*)AryPtrX(GrdParr, nz);
  dd = GrdPprm->dd;
  d = 0.01*dd;
  xmin = GrdPprm->xmin;
  xmax = GrdPprm->xmax;
  ymin = GrdPprm->ymin;
  ymax = GrdPprm->ymax;
  if (x<xmin-d || x>xmax+d || y<ymin-d || y>ymax+d)           eX(1);
  i = GrdIx( x );
  j = GrdJy( y );
  z00 = *(float*)AryPtrX(pgrd, i-1, j-1, nz);
  z10 = *(float*)AryPtrX(pgrd, i  , j-1, nz);
  z01 = *(float*)AryPtrX(pgrd, i-1, j  , nz);
  z11 = *(float*)AryPtrX(pgrd, i  , j  , nz);
  ax = (x - GrdPprm->xmin)/GrdPprm->dd - (i-1);
  ay = (y - GrdPprm->ymin)/GrdPprm->dd - (j-1);
  z0 = z00 + ay*(z01 - z00);
  z1 = z10 + ay*(z11 - z10);
  z = z0 + ax*(z1 - z0);
  return z;
eX_1:
  nMSG(_point_$$_outside_$$_, x, y, GrdPprm->level, GrdPprm->index);
  return 0;
eX_99:
  nMSG(_not_initialized_);
  return 0;
  }

//====================================================================== GrdZz
//
float GrdZz( float x, float y, float s )
  {
  dP(GrdZz);
  float b, t, z;
  if (!GrdPprm)                                         eX(99);
  b = GrdZb(x, y);                                      eG(1);
  if (GrdPprm->zscl <= 0.0)  return b+s;
  t = GrdPprm->zscl;
  if (s > GrdPprm->sscl)  z = t + s - GrdPprm->sscl;
  else  z = b + (t-b)*s/GrdPprm->sscl;
  return z;
eX_1:
  nMSG(_cant_calculate_height_$$_, x, y);
  return 0;
eX_99:
  nMSG(_not_initialized_);
  return 0;
  }

//====================================================================== GrdHh
//
float GrdHh( float x, float y, float s )
  {
  dP(GrdHh);
  float b, t, h;
  if (!GrdPprm)                                 eX(99);
  if (GrdPprm->zscl <= 0.0)  return s;
  b = GrdZb(x, y);                              eG(1);
  t = GrdPprm->zscl;                                    /*-27aug97-*/
  if (s > GrdPprm->sscl)  h = t - b + s - GrdPprm->sscl;
  else  h = (t-b)*s/GrdPprm->sscl;
  return h;
eX_1:
  nMSG(_cant_calculate_height_$$_, x, y);
  return 0;
eX_99:
  nMSG(_not_initialized_);
  return 0;
  }

//====================================================================== GrdSs
//
float GrdSs( float x, float y, float z )
  {
  dP(GrdSs);
  float b, t, s;
  if (!GrdPprm)                                 eX(99);
  b = GrdZb(x, y);                              eG(1);
  if (GrdPprm->zscl <= 0.0)  return z-b;
  t = GrdPprm->zscl;
  if (z > GrdPprm->zscl)  s = GrdPprm->sscl + z - GrdPprm->zscl;
  else  s = GrdPprm->sscl*(z-b)/(t-b);
  return s;
eX_1:
  nMSG(_cant_calculate_height_$$_, x, y);
  return 0;
eX_99:
  nMSG(_not_initialized_);
  return 0;
  }

//====================================================================== GrdKs
//
int GrdKs( float s )
  {
  dP(GrdKs);
  int k;
  if (!GrdPprm)                                                         eX(99);
  for (k=1; k<=GrdPprm->nz; k++)
    if (s <= iSK(k))  break;
  return k;
eX_99:
  nMSG(_not_initialized_);
  return 0;
  }

//====================================================================== GrdKz
//
#define  ZZ(a,b,c)  (*(float*)AryPtrX(pgrd,(a),(b),(c)))

int GrdKz( float x, float y, float z )
  {
  dP(GrdKz);
  float s, ax, ay, zk, a00, a01, a10, a11;
  float dd, d, xmin, xmax, ymin, ymax;
  int i, j, k, nz;
  ARYDSC *pgrd;
  if (!GrdPprm)                                         eX(99);
  pgrd = GrdPprm->pgrd;
  if (!pgrd) {
    s = GrdSs(x, y, z);                                 eG(1);
    k = GrdKs(s);                                       eG(2);
    return k;
    }
  dd = GrdPprm->dd;
  d = 0.01*dd;
  xmin = GrdPprm->xmin;
  xmax = GrdPprm->xmax;
  ymin = GrdPprm->ymin;
  ymax = GrdPprm->ymax;
  if (x<xmin-d || x>xmax+d || y<ymin-d || y>ymax+d)     eX(3);
  i = GrdIx(x);
  j = GrdJy(y);
  ax = (x - xmin)/dd - (i-1);
  ay = (y - ymin)/dd - (j-1);
  a00 = (1-ax)*(1-ay);
  a01 = ay*(1-ax);
  a10 = ax*(1-ay);
  a11 = ax*ay;
  nz = GrdPprm->nz;
  for (k=1; k<=nz; k++)
    if (ZZ(i,j,k) > z)  break;
  if (k > nz)  k = nz;
  zk = a00*ZZ(i-1,j-1,k) + a01*ZZ(i-1,j,k) + a10*ZZ(i,j-1,k) + a11*ZZ(i,j,k);
  if (zk > z) {
    do {
      k--;
      if (k < 0)  return 0;
      zk = a00*ZZ(i-1,j-1,k)+a01*ZZ(i-1,j,k)+a10*ZZ(i,j-1,k)+a11*ZZ(i,j,k);
      } while (zk > z);
    return k+1;
    }
  do {
    k++;
    if (k > nz)  return nz+1;
    zk = a00*ZZ(i-1,j-1,k)+a01*ZZ(i-1,j,k)+a10*ZZ(i,j-1,k)+a11*ZZ(i,j,k);
    } while (zk > z);
  return k;
eX_1: eX_2:
  nMSG(_cant_calculate_height_$$_, x, y);
  return 0;
eX_3:
  nMSG(_point_$$_outside_$$_, x, y, GrdPprm->level, GrdPprm->index);
  return 0;
eX_99:
  nMSG(_not_initialized_);
  return 0;
  }

//=============================================================== GrdLocate
#define ZP(a,b,c)  (*(float*)AryPtrX(pa,(a),(b),(c)))

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
  {
  int i0, i1, j0, j1, k0, k1, nx, ny, nz, r;
  float a00, a01, a10, a11;
  float b00, b01, b10, b11, b;
  float t00, t01, t10, t11, t;
  float z, zb, zeta;
  if (!pz)  return 0;
  z = *pz;
  r = 0;
  if (pa->numdm != 3)  return 0;
  nx = pa->bound[0].hgh;
  ny = pa->bound[1].hgh;
  nz = pa->bound[2].hgh;
  k1 = *pk;
  if (k1 < 1)   k1 = 1;
  if (k1 > nz)  k1 = nz;
  if (xi<0 || xi>nx || eta<0 || eta>ny)  return 0;
  i0 = xi;   if (i0 == nx)  i0--;  i1 = i0 + 1;  xi -= i0;
  j0 = eta;  if (j0 == ny)  j0--;  j1 = j0 + 1;  eta -= j0;
  if (pi)  *pi = i1;
  if (pj)  *pj = j1;
  k0 = k1-1;
  a00 = (1-xi)*(1-eta);
  a10 = xi*(1-eta);
  a01 = (1-xi)*eta;
  a11 = xi*eta;
  b00 = ZP(i0,j0,0);  b01 = ZP(i0,j1,0);
  b10 = ZP(i1,j0,0);  b11 = ZP(i1,j1,0);
  zb = b = a00*b00 + a10*b10 + a01*b01 + a11*b11;
  if (pzg)  *pzg = zb;
  if (flag & GRD_USE_ZETA) {
    b00 = ZP(i0,j0,k0);  b01 = ZP(i0,j1,k0);
    b10 = ZP(i1,j0,k0);  b11 = ZP(i1,j1,k0);
    b = a00*b00 + a10*b10 + a01*b01 + a11*b11;
    t00 = ZP(i0,j0,k1);  t01 = ZP(i0,j1,k1);
    t10 = ZP(i1,j0,k1);  t11 = ZP(i1,j1,k1);
    t = a00*t00 + a10*t10 + a01*t01 + a11*t11;
    if (paz) {
      zeta = *paz;
      if (zeta < 0)  zeta = 0;
      if (zeta > 1)  zeta = 1;
      }
    else zeta = 0;
    if (pz) *pz = b + zeta*(t-b);
    return GRD_INSIDE;
    }
  if (flag & GRD_PARM_IS_H) {
    if (z < 0) {
      if (flag & GRD_REFBOT) { z = -z;  r |= GRD_REFBOT; }
      else  return 0;
      }
    z += zb;
    }
  else {
    if (z < zb) {
      if (flag & GRD_REFBOT) { z = 2*zb-z;  r |= GRD_REFBOT; }
      else  return 0;
      }
    }
  if ((zr) && (flag & GRD_REFTOP) && (z > zr)) {
    z = 2*zr - z;
    r |= GRD_REFTOP;
    }
  if (z < zb)  return 0;

  if (k0 > 0) {
    b00 = ZP(i0,j0,k0);  b01 = ZP(i0,j1,k0);
    b10 = ZP(i1,j0,k0);  b11 = ZP(i1,j1,k0);
    b = a00*b00 + a10*b10 + a01*b01 + a11*b11;
    }
  t = b;                      //-98-05-29
  k1 = k0;
  while (z < b) {
    t00 = b00;  t01 = b01;  t10 = b10;  t11 = b11;  t = b;
    k1 = k0--;
    if (k0 < 0)  return 0;
    b00 = ZP(i0,j0,k0);  b01 = ZP(i0,j1,k0);
    b10 = ZP(i1,j0,k0);  b11 = ZP(i1,j1,k0);
    b = a00*b00 + a10*b10 + a01*b01 + a11*b11;
    }
  if (k1 == k0) {
    k1 = k0+1;
    if (k1 > nz)  return 0;
    t00 = ZP(i0,j0,k1);  t01 = ZP(i0,j1,k1);
    t10 = ZP(i1,j0,k1);  t11 = ZP(i1,j1,k1);
    t = a00*t00 + a10*t10 + a01*t01 + a11*t11;
    }
  while (z >= t) { //-2005-02-01
    if (k1 >= nz)  return 0;
    b00 = t00;  b01 = t01;  b10 = t10;  b11 = t11;  b = t;
    k0 = k1++;
    t00 = ZP(i0,j0,k1);  t01 = ZP(i0,j1,k1);
    t10 = ZP(i1,j0,k1);  t11 = ZP(i1,j1,k1);
    t = a00*t00 + a10*t10 + a01*t01 + a11*t11;
    }
  *pk = k1;
  *pz = z;
  *paz = (z-b)/(t-b);
  return r | GRD_INSIDE;
  }
#undef ZP

//=============================================================== MakeHeader
//
#define CAT(a, b) TxtPrintf(&txt, a, b), TxtCat(&hdr, txt.s);
static char *MakeHeader(        /* the header (global storage)  */
  long id,                      /* identification               */
  long *pt1,                    /* start of the validity time   */
  long *pt2 )                   /* end of validity time         */
  {
  /* dP(GRD:MakedHeader); */
  char name[256], t1s[40], t2s[40], format[40], s[80];		//-2004-11-26
  char arrtype[40], valdef[40];
  float xmin, ymin, delta, zscl, sscl;
  TXTSTR txt = { NULL, 0 };
  static TXTSTR hdr = { NULL, 0 };
  int gp, nl, ni, nt, nx, ny, nz, k;
  enum DATA_TYPE dt;
  vLOG(4)("GRD: MakeHeader(%s, ...)", NmsName(id));
  TxtCpy(&hdr, "\n");
  zscl = GrdPprm->zscl;
  sscl = GrdPprm->sscl;
  dt = XTR_DTYPE(id);
  gp = XTR_GROUP(id);
  nl = XTR_LEVEL(id);
  ni = XTR_GRIDN(id);
  if (dt == GRDarr) {
    switch(gp) {
      case GRD_IZP:  strcpy(format, "Zp%6.2f");
                     strcpy(valdef, "P");
                     break;
      case GRD_ISLD: strcpy(format, "Bn%09lx");
                     strcpy(valdef, "M");
                     break;
      case GRD_IZM:  strcpy(format, "Zm%6.2f");
                     strcpy(valdef, "V");
                     break;
      case GRD_IVM:  strcpy(format, "Vm%8.0f");
                     strcpy(valdef, "V");
                     break;
      default:       strcpy(format, "Sk%11.3e");
                     strcpy(valdef, "S");
                     break;
      }
    sprintf(arrtype, "N%d", gp);
    }
  else if (dt == SRFarr) {
    if (gp == 1) {
      strcpy(format, "Z%6.1fZ0%6.3fD0%6.1f");
      strcpy(valdef, "PPP");
      }
    else {
      strcpy(format, "Zp%6.2f");
      strcpy(valdef, "P");
      }
    sprintf(arrtype, "S%d", gp);
    }
  else {
    strcpy(format, "X%10.3e");
    strcpy(valdef, "U");
    sprintf(arrtype, "U%d", gp);
    }
  strcpy(name, NmsName(id));
  strcpy(t1s, TmString(pt1));
  strcpy(t2s, TmString(pt2));
  sprintf(s, "GRD_%d.%d.%s", StdVersion, StdRelease, StdPatch);
  CAT("prgm  \"%s\"\n", s);
  CAT("name  \"%s\"\n", name);
  CAT("t1    \"%s\"\n", t1s);
  CAT("t2    \"%s\"\n", t2s);
  CAT("form  \"%s\"\n", format);
  CAT("artp  \"%s\"\n", arrtype);
  CAT("vldf  \"%s\"\n", valdef);
  if (dt==GRDarr && gp==0)  
    return hdr.s;
  nt = GetGridParm(nl, ni, &xmin, &ymin, &delta, &nx, &ny, &nz, NULL);
  if (nt <= 0)  
    return hdr.s;
  CAT("xmin  %s\n", TxtForm(xmin,6));
  CAT("ymin  %s\n", TxtForm(ymin,6));
  CAT("delta %s\n", TxtForm(delta,6));
  CAT("refx  %d\n", GrdPprm->refx);
  CAT("refy  %d\n", GrdPprm->refy);
  if (GrdPprm->refx > 0 && *GrdPprm->ggcs)                       //-2008-12-11
    CAT("ggcs  \"%s\"\n", GrdPprm->ggcs);
  if (dt == GRDarr) {
    CAT("zscl  %s\n", TxtForm(zscl,6));
    CAT("sscl  %s\n", TxtForm(sscl,6));
    TxtCat(&hdr, "sk ");
    for (k=0; k<=nz; k++) {
      CAT(" %s", TxtForm(iSK(k),6));
    }
    TxtCat(&hdr, "\n");
  }
  TxtClr(&txt);
  return hdr.s;
}
#undef CAT

//=================================================================== GrdInit
//
long GrdInit(           /* initialize server    */
long flags,             /* action flags         */
char *istr )            /* server options       */
  {
  dP(GrdInit);
  long id, mask;
  char *jstr, *ps, s[256];
  if (StdStatus & STD_INIT)  return 0;
  if (istr) {
    jstr = istr;
    ps = strstr(istr, " -d");
    if (ps)  strcpy(DefName, ps+3);
    ps = strstr(istr, " -y");
    if (ps) sscanf(ps+3, "%d", &StdDspLevel);
    ps = strstr(istr, " -v");
    if (ps) sscanf(ps+3, "%d", &StdLogLevel);
    }
  else  jstr = "";
  vLOG(3)("GRD_%d.%d.%s (%08lx,%s)", StdVersion, StdRelease, StdPatch, flags, jstr);
  StdStatus |= flags;
  sprintf(s, " BDS -v%d -y%d -d%s", StdLogLevel, StdDspLevel, DefName);
  BdsInit(flags&STD_GLOBAL, s);                               eG(1);
  mask = ~(NMS_GROUP | NMS_LEVEL | NMS_GRIDN);
  id = IDENT(GRDpar, 0, 0, 0);
  TmnCreator(id, mask, TMN_FIXED, istr, GrdServer, NULL);     eG(2);
  id = IDENT(GRDtab, 0, 0, 0);
  TmnCreator(id, mask, TMN_FIXED, istr, GrdServer, NULL);     eG(3);
  id = IDENT(SRFarr, 0, 0, 0);
  TmnCreator(id, mask, TMN_FIXED, istr, GrdServer, NULL);     eG(4);
  StdStatus |= STD_INIT;
  return 0;
eX_1: eX_2:  eX_3:  eX_4:
  eMSG(_grd_not_initialized_);
  }

//=================================================================== GrdServer
//
long GrdServer(
char *ss )
  {
  dP(GrdServer);
  long id, mask;
  int nl, ni;
  if (StdArg(ss))  return 0;
  if (*ss) {
    switch (ss[1]) {
      case 'd':  strcpy(DefName, ss+2);
                 break;
      case 'k':  sscanf(ss+2, "%d,%d,%d", &PrnLayer, &PrnLevel, &PrnIndex);
                 break;
      case 'w':  WrtMask = 0x0200;
                 sscanf(ss+2, "%x", &WrtMask);
                 BdsServer(ss);
                 break;
      default:   ;
      }
    return 0;
    }

  nl = XTR_GRIDN(StdIdent);
  ni = XTR_LEVEL(StdIdent);
  mask = ~(NMS_GROUP | NMS_LEVEL | NMS_GRIDN);
  id = IDENT(GRDarr, 0, 0, 0);
  TmnCreator(id, mask, 0, "", MakeGrd, MakeHeader);             eG(1);
  id = IDENT(GRDpar, 0, 0, 0);
  TmnCreator(id, mask, TMN_FIXED, "", NULL, NULL);              eG(2);
  id = IDENT(GRDtab, 0, 0, 0);
  TmnCreator(id, mask, TMN_FIXED, "", NULL, NULL);              eG(3);
  id = IDENT(SRFarr, 0, 0, 0);
  TmnCreator(id, mask, TMN_FIXED, "", NULL, NULL);              eG(4);

  if (!GrdPprm) {
    GrdRead(DefName);                                           eG(7);
    }
  if (WrtMask & 0x0200) {
    id = IDENT(GRDarr, 0, 0, 0);
    TmnStoreAll(mask, id, TMN_NOID);                            eG(8);
    //id = IDENT(SRFarr, 0, 0, 0);                                  2011-07-04
    //TmnStoreAll(mask, id, TMN_NOID);                            eG(9);
    }
  if ((nl) && (ni))  GrdSet(nl, ni);                            eG(14);
  return 0;
eX_1:  eX_2:  eX_3:  eX_4:
  eMSG(_not_initialized_);
eX_7:
  eMSG(_undefined_grid_);
eX_8:
  eMSG(_cant_store_arrays_);
eX_14:
  eMSG(_internal_error_);
  }

//============================================================================
//
// history:
//
// 2002-09-24 lj 1.0.0  final release candidate
// 2004-11-26 lj 2.1.7  string length for names = 256
// 2005-01-12 uj 2.1.8  check if Zscl = 0
// 2005-02-01 uj 2.1.9  location adjusted
// 2005-02-15 uj 2.1.14 option DMKHM
// 2005-03-11 lj 2.1.16 GrdCheck translated from LASAT
// 2005-03-17 uj 2.2.0  version number upgrade
// 2006-10-26 lj 2.3.0  external strings
// 2008-12-04 lj 2.4.5  read refx, refy, ggcs
// 2008-12-11 lj        header parameter modified
// 2011-06-29 uj 2.5.0  dmna header
// 2011-07-04 uj        don't write grd files by default
// 2012-04-06 uj 2.6.0  version number upgrade
//                      net type parsing cleaned up
//
//============================================================================
