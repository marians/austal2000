// ======================================================== TalPrf.c
//
// Generate boundary layer profile for AUSTAL2000
// ==============================================
//
// Copyright (C) Umweltbundesamt, Dessau-Roﬂlau, Germany, 2002-2012
// Copyright (C) Janicke Consulting, 88662 ‹berlingen, Germany, 2002-2014
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
// last change:  2014-06-26 uj
//
//========================================================================

#include <math.h>
#include <signal.h>
#include <ctype.h>

#define  STDMYMAIN  PrfMain
static char *eMODn = "TalPrf";

#include "IBJmsg.h"
#include "IBJary.h"
#include "IBJtxt.h"
#include "IBJdmn.h"
#include "IBJstd.h"

//=========================================================================

STDPGM(TalProfil, PrfServer, 2, 6, 11);

//=========================================================================

#include "genutl.h"
#include "genio.h"

#include "TalTmn.h"
#include "TalNms.h"
#include "TalGrd.h"
#include "TalIo1.h"
#include "TalBlm.h"
#include "TalUtl.h"
#include "TalWnd.h"
#include "TalPrf.h"
#include "TalMat.h"
#include "TalPrf.nls"

#define  PRF_WRTWIND   0x00000001
#define  PRF_PRNDEF    0x00000002
#define  PRF_3DMET     0x00000004
#define  PRF_BODIES    0x00000008
#define  PRF_NOBASE    0x00000010
#define  PRF_WRITESFC  0x00000100
#define  PRF_READSFC   0x00000200

#define  PRFLIB_ZG  0x00000001
#define  PRFLIB_ZP  0x00000002
#define  PRFLIB_IB  0x00000010
#define  PRFLIB_NA  0x00000020
#define  PRFLIB_VX  0x00000100
#define  PRFLIB_VY  0x00000200
#define  PRFLIB_VS  0x00000400
#define  PRFLIB_KU  0x00001000
#define  PRFLIB_KV  0x00002000
#define  PRFLIB_KW  0x00004000
#define  PRFLIB_SU  0x00010000
#define  PRFLIB_SV  0x00020000
#define  PRFLIB_SW  0x00040000
#define  PRFLIB_TH  0x00080000
#define  PRFLIB_WW  0x01000000
#define  PRFLIB_KK  0x02000000
#define  PRFLIB_KA  0x20000000
#define  PRFLIB_TT  0x04000000
#define  PRFLIB_TA  0x40000000
#define  PRFLIB_AR  0xff000000
#define  PRFLIB_DA  0x00ffff00

typedef struct {
  char name[8];
  int akl;
  float vxa, vya, lmo;
  int parts[1];
} LIBREC;

static int LibRecSize = sizeof(LIBREC);
static double VaMin = 0.5;
static double VzMax = 50;                                       //-2003-10-12
static int *LibLvls, *LibInxs;                                  //-2005-02-10

/*=============================== PROTOTYPES TALPRF ========================*/

static long Check3D( char *name, char *header, ARYDSC *pa, int elmsz );
static long Clc1dMet( void )  /* Meteorologie-Profil berechnen.    */;
static long Clc3dMet( void );
static long CrtHeader(  /* Header fuer Modell-Feld erzeugen         */
TXTSTR *phdr );         /* Header fuer PRFarr                       */

static long PrnData( char *s );

/*--------------------------------------------------------------------------*/

static long MetT1, MetT2, Iba, Iwnd, Iprf, Itrb, Idff, Jtrb, Jdff;
static int GridLevel, GridIndex;
static float WndUref = BLMDEFUREF;

static char DefName[120] = "param.def";
static char InpName[120] = "meteo.def";
static int PrfMode, DefMode;
static int PrfRecLen;
static char PrnCmd[80];

static GRDPARM *Pgp;
static BLMPARM *Pbp;
static ARYDSC  *Pga, *Pba, *Ptrb, *Pdff, *Pprf, *Pwnd, *Qtrb, *Qdff;
static char ValDef[80];
static FILE *sfcFile;
static int sfcFirst;

static int isuneq( float a, float b, float c )
  {
  return (ABS((a)-(b)) > c);                   //-2002-09-26
  }

static int isequal( float a, float b, float c )
  {
  return (ABS((a)-(b)) <= c);                  //-2002-09-26
  }

//============================================================== Check3dGrid
//
static long Check3dGrid( GRDPARM *pgp, ARYDSC *pa, char *prm, int check_zp ) {
  dP(Check3dGrid);
  int i, j, k, nx, ny, nz, gl, gi, sz;
  double dd, xmin, ymin, d, x, y;
  float *pf, *pg;
  long iza;
  ARYDSC *pza=NULL;
  gl = pgp->level;
  gi = pgp->index;
  nx = pgp->nx;
  ny = pgp->ny;
  nz = pgp->nz;
  dd = pgp->dd;
  xmin = pgp->xmin;
  ymin = pgp->ymin;
  iza = IDENT(GRDarr, GRD_IZP, gl, gi);
  pza = TmnAttach(iza, NULL, NULL, 0, NULL);  if (!pza)           eX(1);
  sz = pa->elmsz;
  AryAssert(pa, sz, 3, 0, nx, 0, ny, 0, nz);                      eG(2);
  if (1 != DmnGetDouble(prm, "dd|delt|delta", "%lf", &d, 1))      eX(3);
  if (isuneq(dd, d, 0.02))                                        eX(4);
  if (1 != DmnGetDouble(prm, "xmin|x0", "%lf", &x, 1))            eX(5);
  if (isuneq(xmin, x, 0.02))                                      eX(6);
  if (1 != DmnGetDouble(prm, "ymin|y0", "%lf", &y, 1))            eX(7);
  if (isuneq(ymin, y, 0.02))                                      eX(8);
  if (check_zp) {
    for (i=0; i<=nx; i++)
      for (j=0; j<=ny; j++)
        for (k=0; k<=nz; k++) {
          pf = AryPtr(pa, i, j, k);  if (!pf)      eX(10);
          pg = AryPtr(pza, i, j, k);  if (!pg)     eX(11);
          if (isuneq(*pf, *pg, 0.2))               eX(12);  //-2005-09-07
        }                                        
  }                                              
  TmnDetach(iza, NULL, NULL, 0, NULL);             eG(13);
  pza = NULL;
  return 0;
eX_1: eX_10: eX_11: eX_13:
  eMSG(_internal_error_);
eX_2:
  eMSG(_improper_dimensions_);
eX_3: eX_5: eX_7:
  eMSG(_incomplete_header_);
eX_4: eX_6: eX_8:
  eMSG(_improper_grid_);
eX_12:
  eMSG(_found_$$$_$_expected_$_, i, j, k, *pf, *pg);
}

//================================================================== CalcVz
//
static int CalcVz( ARYDSC *pa, float dd ) {
  dP(CalcVz);
  WNDREC *p000, *p001, *p010, *p100, *p110, *p011, *p101, *p111;
  float a, f, g;
  int i, j, k, nx, ny, nz;
  if (pa->elmsz<sizeof(WNDREC) || pa->numdm!=3)                     eX(1);
  nx = pa->bound[0].hgh;
  ny = pa->bound[1].hgh;
  nz = pa->bound[2].hgh;
  a = dd*dd;
  for (i=1; i<=nx; i++)
    for (j=1; j<=ny; j++) {
      p111 = AryPtr(pa, i, j, 0);
      p011 = AryPtr(pa, i-1, j, 0);
      p101 = AryPtr(pa, i, j-1, 0);
      p001 = AryPtr(pa, i-1, j-1, 0);
      if (!p111 || !p011 || !p101 || !p001)                         eX(2);
      f = p111->vz;
      for (k=1; k<=nz; k++) {
        p110 = p111;
        p010 = p011;
        p100 = p101;
        p000 = p001;
        p111 = AryPtr(pa, i, j, k);
        p011 = AryPtr(pa, i-1, j, k);
        p101 = AryPtr(pa, i, j-1, k);
        p001 = AryPtr(pa, i-1, j-1, k);
        if (!p111 || !p011 || !p101 || !p001)                       eX(3);
        if (p110->vz > WND_VOLOUT && p111->vz <= WND_VOLOUT)        eX(4); //-2012-01-26
        if (p110->vz <= WND_VOLOUT) {
          if (f > WND_VOLOUT)                                       eX(5); //- never reached
          continue;
        }
        else if (f <= WND_VOLOUT) {
          f = p110->vz;
          continue;
        }
        g = 0;
        g += 0.5*(p001->z - p000->z + p011->z - p010->z)*p011->vx;
        g -= 0.5*(p101->z - p100->z + p111->z - p110->z)*p111->vx;
        g += 0.5*(p001->z - p000->z + p101->z - p100->z)*p101->vy;
        g -= 0.5*(p011->z - p010->z + p111->z - p110->z)*p111->vy;
        f += g/dd;
        p111->vz = f;
      }
    }
  return 0;
eX_1: eX_2: eX_3:
  eMSG(_internal_error_);
eX_4: eX_5:
  eMSG(_invalid_wind_field_);
}

//============================================================== LibWndServer
//
static long LibWndServer( // make wind field from library files
char *ss )                // calling arguments
{
  dP(LibWndServer);
  static char libpath[256], path[256], name[256], fn[256], line[256];
  static char t1s[40], t2s[40];
  static long tmvalue, iwnd, itrb, jtrb, idff, jdff;
  ARYDSC *plt=NULL, *pa, *pwnd, *ptrb, *qtrb, *pdff, *qdff;
  ARYDSC Ww1Dsc, Ww2Dsc, ZpDsc, Vx1Dsc, Vx2Dsc, Vy1Dsc, Vy2Dsc;
  ARYDSC Tt1Dsc, Tt2Dsc, Ta1Dsc, Ta2Dsc;
  ARYDSC Kk1Dsc, Kk2Dsc, Ka1Dsc, Ka2Dsc;
  LIBREC *pl=NULL, *pl1=NULL, *pl2=NULL;
  TXTSTR usr = { NULL, 0 };
  TXTSTR sys = { NULL, 0 };
  BLMPARM *pbp=NULL;
  GRDPARM *pgp=NULL;
  WNDREC *pw, *pw1, *pw2;
  TRBREC *pt, *pt1, *pt2;
  DFFREC *pk, *pk1, *pk2;
  int gl, gi, dt, i, j, k, n, parts, nx, ny, nz, ns, ni, nn;    //-2005-02-10
  int kl, klsfc, found;
  long igp, ibp, ilt;
  double dd;
  float det, f1, f2, ua, ra, si, co, sia, coa, vxa, vya, uasfc, rasfc;
  float vx, vy, va, vx1, vy1, vx2, vy2, du1, du2, dr, dr1, dr2, rot, sq;
  float *p1, *p2, *pz;
  int n1, n2;
  char *name1, *name2, **tokens=NULL;
  TXTSTR wndhdr = { NULL, 0 };
  TXTSTR kffhdr = { NULL, 0 };
  TXTSTR trbhdr = { NULL, 0 };
  vLOG(4)("PRF: LibWndServer(%s) ...", ss);
  if ((ss) && (*ss)) {
    if (*ss != '-') {
      strcpy(path, ss);
      return 0;
    }
    switch (ss[1]) {
      case 'l':  strcpy(name, ss+2);
                 break;
      case 'N':  sscanf(ss+2, "%08lx", &iwnd);
                 break;
      case 'T':  tmvalue = TmValue(ss+2);
                 break;
      default:   ;
    }
    return 0;
  }
  if (!iwnd)                                                  eX(1);
  //
  TxtCpy(&wndhdr,"\n"
    "option  NOCHECK\n"
    "uref  0\n"
    "vldf  \"PXYS\"\n"
    "form  \"Zp%7.2fVx%6.2fVy%6.2fVs%6.2f\"\n");                  //-2011-07-04
  TxtCpy(&kffhdr,"\n"
    "option  NOCHECK\n"
    "uref  0\n"
    "vldf  \"PP\"\n"
    "form  \"Kh%6.2fKv%6.2f\"\n");
  TxtCpy(&trbhdr,"\n"
    "option  NOCHECK\n"
    "uref  0\n"
    "vldf  \"PPP\"\n"
    "form  \"Su%6.2fSv%6.2fSw%6.2f\"\n");       
  //   
  dt = XTR_DTYPE(iwnd);
  gl = XTR_LEVEL(iwnd);
  gi = XTR_GRIDN(iwnd);
  if (dt != WNDarr)  return 0;
  itrb = IDENT(VARarr, WND_FINAL, gl, gi);
  jtrb = IDENT(VARarr, WND_WAKE,  gl, gi);
  idff = IDENT(KFFarr, WND_FINAL, gl, gi);
  jdff = IDENT(KFFarr, WND_WAKE,  gl, gi);
  igp  = IDENT(GRDpar, 0, gl, gi);
  pa =  TmnAttach(igp, NULL, NULL, 0, NULL);  if (!pa)        eX(2);
  pgp = (GRDPARM*) pa->start;
  if (!pgp)                                                   eX(3);
  ibp = IDENT(BLMpar, 0, 0, 0);
  pa =  TmnAttach(ibp, NULL, NULL, 0, NULL);  if (!pa)        eX(4);
  pbp = (BLMPARM*) pa->start;
  if (!pbp)                                                   eX(5);
  MsgCheckPath(path);
  MsgCheckPath(name);
  if (!*name)  strcpy(name, "~lib");
  if (*name == '~')  sprintf(libpath, "%s/%s", path, name+1);
  else  strcpy(libpath, name);
  nx = pgp->nx;
  ny = pgp->ny;
  nz = pgp->nz;
  dd = pgp->dd;
  ua = pbp->WndSpeed;
  ra = pbp->WndDir;
  kl = 7 - pbp->kta;
  sia = sin(ra/RADIAN);
  coa = cos(ra/RADIAN);
  vxa = -ua*sia;
  vya = -ua*coa;
  ilt = IDENT(LIBtab, 0, 0, 0);
  plt = TmnAttach(ilt, NULL, NULL, 0, NULL);  if (!plt)       eX(6);
  ns = plt->bound[0].hgh + 1;  
  //--------------------------------------------------------------2005-02-10
  nn = GrdPprm->numnet;
  if (nn > 0) {
      for (ni=0; ni<nn; ni++)
          if ((LibLvls[ni] == gl) && (LibInxs[ni] == gi))  break;
      if (ni >= nn)                                             eX(500);
  }
  else  ni = 0;
  //-------------------------------------------------------------------------
  n1 = -1;
  n2 = -1;
  if (StdStatus & PRF_READSFC) {                                  //-2012-10-30
  		//
  		// read factors from log file
  		strcpy(t1s, TmString(&MetT1));
    strcpy(t2s, TmString(&MetT2));
  	 found = 0;
  		if (!sfcFile) {
  				sprintf(fn, "%s/austal2000.log", libpath);
  				sfcFile = fopen(fn, "rb");
  				if (!sfcFile)                                             eX(150);
  				lMSG(0,0)(_sfc_read_$_, fn);
  		}
  		while (fgets(line, 255, sfcFile))
  		  if (!strncmp(line, "SFC ", 4)) {
  		  		tokens = _MsgTokens(line+4, " \t");
        for (n=0; ; n++)
          if (!tokens[n])  break;
        if (n != 9)                                             eX(152);
        if (!strcmp(tokens[0], t1s)) {
  						  found = 1;
  					   break;
  					 }
  				}  				
  		if (!found)                                                 eX(151);	 
    uasfc = 0;
    rasfc = 0;
    klsfc = 0;
    n = 0;
    n += sscanf(tokens[2], "%f", &uasfc);
    n += sscanf(tokens[3], "%f", &rasfc);
    n += sscanf(tokens[4], "%d", &klsfc);
    name1 = tokens[5];
    name2 = tokens[6];
    n += sscanf(tokens[7], "%f", &f1);
    n += sscanf(tokens[8], "%f", &f2);
    if (n != 5 || strcmp(t2s, tokens[1]) || 
    		  uasfc != ua || rasfc != ra || klsfc != kl)              eX(153);
    pl1 = AryPtr(plt, 0);  if (!pl1)                            eX(10);
  }
  else {
    //
    // interpolate                                           //-2006-01-23
    du1 = 100;
    du2 = 100;
    dr1 = 5;
    dr2 = 5;
    for (n=0; n<ns; n++) {
      pl = AryPtr(plt, n);  if (!pl)                            eX(7);
      if (pl->akl != kl)  continue;
      vx = pl->vxa;
      vy = pl->vya;
      va = sqrt(vx*vx+vy*vy);
      if (va == 0)                                              eX(8);
      si = -vx/va;   // meteorol. definition            //-2002-07-14
      co = -vy/va;                                      //-2002-07-14
      dr = (si-sia)*(si-sia) + (co-coa)*(co-coa);
      rot = vxa*vy - vya*vx;
      if (rot <= 0 && dr < dr1) {
        du1 = (ua-va)*(ua-va);
        dr1 = dr;
        n1 = n;
    }
      else if (rot > 0 && dr < dr2) {
        du2 = (ua-va)*(ua-va);
        dr2 = dr;
        n2 = n;
      }
    }
    if (ns == 1 && (n2 == 0 || n1 == 0)) {                           //-2011-04-06
      n1 = 0;
      n2 = 0;
    }
    //
    // extrapolate if interpolation failed
    if (n1 < 0 || n2 < 0) { 
      du1 = 100;
      du2 = 100;
      dr1 = 5;
      dr2 = 5;
      for (n=0; n<ns; n++) {
        pl = AryPtr(plt, n);  if (!pl)                            eX(7);
        if (pl->akl != kl)  continue;
        vx = pl->vxa;
        vy = pl->vya;
        va = sqrt(vx*vx+vy*vy);
        if (va == 0)                                              eX(8);
        si = -vx/va;   // meteorol. definition            //-2002-07-14
        co = -vy/va;                                      //-2002-07-14
        dr = (si-sia)*(si-sia) + (co-coa)*(co-coa);
        if (dr < dr1) {
          du2 = du1;
          dr2 = dr1;
          n2 = n1;
          du1 = (ua-va)*(ua-va);
          dr1 = dr;
          n1 = n;
        }
        else if (dr < dr2) {
          du2 = (ua-va)*(ua-va);
          dr2 = dr;
          n2 = n;
        }
      }
    }
    if (n1 < 0 || n2 < 0)                                                eX(430); //-2011-04-06
    pl1 = AryPtr(plt, n1);  if (!pl1)                                    eX(10);
    name1 = pl1->name;
    vx1 = pl1->vxa;
    vy1 = pl1->vya;
    pl2 = AryPtr(plt, n2);  if (!pl2)                                    eX(11);
    name2 = pl2->name;
    vx2 = pl2->vxa;
    vy2 = pl2->vya;
    det = vx1*vy2 - vx2*vy1;
    if (det == 0) {
      double p11, p22, p10, p20;
      if (ns > 1)  vMsg("PRF: \"%s\" and \"%s\" are not independent!", name1, name2);
      p11 = vx1*vx1 + vy1*vy1;
      p22 = vx2*vx2 + vy2*vy2;
      p10 = vx1*vxa + vy1*vya;
      p20 = vx2*vxa + vy2*vya;
      if      (p11>=p22 && p11>0) { f1 = p10/p11;  f2 = 0; }
      else if (p22>=p11 && p22>0) { f1 = 0;  f2 = p20/p22; }
      else                        { f1 = 0.5;  f2 = 0.5; }
    }
    else {
      f1 = (vy2*vxa - vx2*vya)/det;
      f2 = (vx1*vya - vy1*vxa)/det;
    }
  }
  sq = f1*f1 + f2*f2;
  if (sq > 100 || sq < 0.0025)                                        eX(12); //-2006-02-23 
  vLOG(3)("PRF: adding wind fields \"%s\" and \"%s\" with f1=%1.4f, f2=%1.4f",
    name1, name2, f1, f2);
  //
  if (StdStatus & PRF_WRITESFC) {                                 //-2012-10-30
  		if (sfcFirst) {
  				lMSG(0,0)(_sfc_write_);
  				sfcFirst = 0;
  		}
    strcpy(t1s, TmString(&MetT1));
    strcpy(t2s, TmString(&MetT2));
    vMsg("SFC %12s %12s %6.3f %4.0f %1d %8s %8s %14.6e %14.6e",
    		t1s, t2s, ua, ra, kl, name1, name2, f1, f2);
  }
  //
  parts = pl1->parts[ni];                                       //-2005-02-10
  if (parts & PRFLIB_WW) {
    memset(&Ww1Dsc, 0, sizeof(ARYDSC));
    sprintf(fn, "%s/w%sa%1x%1x", libpath, name1, gl, gi);
    if (0 > DmnRead(fn, &usr, &sys, &Ww1Dsc))                       eX(13);
    Check3dGrid(pgp, &Ww1Dsc, usr.s, 1);                            eG(14);
    TxtClr(&usr);
    TxtClr(&sys);
    memset(&Ww2Dsc, 0, sizeof(ARYDSC));
    sprintf(fn, "%s/w%sa%1x%1x", libpath, name2, gl, gi);
    if (0 > DmnRead(fn, &usr, &sys, &Ww2Dsc))                       eX(15);
    Check3dGrid(pgp, &Ww2Dsc, usr.s, 1);                            eG(16);
    TxtClr(&usr);
    TxtClr(&sys);
    pwnd = TmnCreate(iwnd, sizeof(WNDREC), 3, 0,nx, 0,ny, 0,nz);
    if (!pwnd)                                                      eX(17);
    if (parts & PRFLIB_KK) {
      memset(&Kk1Dsc, 0, sizeof(ARYDSC));
      sprintf(fn, "%s/k%sa%1x%1x", libpath, name1, gl, gi);
      if (0 > DmnRead(fn, &usr, &sys, &Kk1Dsc))                       eX(113);
      Check3dGrid(pgp, &Kk1Dsc, usr.s, 0);                            eG(114);
      TxtClr(&usr);
      TxtClr(&sys);
      memset(&Kk2Dsc, 0, sizeof(ARYDSC));
      sprintf(fn, "%s/k%sa%1x%1x", libpath, name2, gl, gi);
      if (0 > DmnRead(fn, &usr, &sys, &Kk2Dsc))                       eX(115);
      Check3dGrid(pgp, &Kk2Dsc, usr.s, 0);                            eG(116);
      TxtClr(&usr);
      TxtClr(&sys);
      pdff = TmnCreate(idff, sizeof(DFFREC), 3, 0,nx, 0,ny, 0,nz);
      if (!pdff)                                                      eX(117);
      vLOG(5)("PRF: idff=%08x created", idff);
    }
    else  pdff = NULL;
    
    if (parts & PRFLIB_KA) {
      memset(&Ka1Dsc, 0, sizeof(ARYDSC));
      sprintf(fn, "%s/k%sd%1x%1x", libpath, name1, gl, gi);
      if (0 > DmnRead(fn, &usr, &sys, &Ka1Dsc))                       eX(213);
      Check3dGrid(pgp, &Ka1Dsc, usr.s, 0);                            eG(214);
      TxtClr(&usr);
      TxtClr(&sys);
      memset(&Ka2Dsc, 0, sizeof(ARYDSC));
      sprintf(fn, "%s/k%sd%1x%1x", libpath, name2, gl, gi);
      if (0 > DmnRead(fn, &usr, &sys, &Ka2Dsc))                       eX(215);
      Check3dGrid(pgp, &Ka2Dsc, usr.s, 0);                            eG(216);
      TxtClr(&usr);
      TxtClr(&sys);
      qdff = TmnCreate(jdff, sizeof(DFFREC), 3, 0,nx, 0,ny, 0,nz);
      vLOG(5)("PRF: jdff=%08x created", jdff);
      if (!qdff)                                                      eX(217);
    }
    else  qdff = NULL;
    
    if (parts & PRFLIB_TT) {
      memset(&Tt1Dsc, 0, sizeof(ARYDSC));
      sprintf(fn, "%s/v%sa%1x%1x", libpath, name1, gl, gi);
      if (0 > DmnRead(fn, &usr, &sys, &Tt1Dsc))                       eX(313);
      Check3dGrid(pgp, &Tt1Dsc, usr.s, 0);                            eG(314);
      TxtClr(&usr);
      TxtClr(&sys);
      memset(&Tt2Dsc, 0, sizeof(ARYDSC));
      sprintf(fn, "%s/v%sa%1x%1x", libpath, name2, gl, gi);
      if (0 > DmnRead(fn, &usr, &sys, &Tt2Dsc))                       eX(315);
      Check3dGrid(pgp, &Tt2Dsc, usr.s, 0);                            eG(316);
      TxtClr(&usr);
      TxtClr(&sys);
      ptrb = TmnCreate(itrb, sizeof(TRBREC), 3, 0,nx, 0,ny, 0,nz);
      if (!ptrb)                                                      eX(317);
      vLOG(5)("PRF: itrb=%08x created", itrb);
    }
    else  ptrb = NULL;
    
    if (parts & PRFLIB_TA) {
      memset(&Ta1Dsc, 0, sizeof(ARYDSC));
      sprintf(fn, "%s/v%sd%1x%1x", libpath, name1, gl, gi);
      if (0 > DmnRead(fn, &usr, &sys, &Ta1Dsc))                       eX(413);
      Check3dGrid(pgp, &Ta1Dsc, usr.s, 0);                            eG(414);
      TxtClr(&usr);
      TxtClr(&sys);
      memset(&Ta2Dsc, 0, sizeof(ARYDSC));
      sprintf(fn, "%s/v%sd%1x%1x", libpath, name2, gl, gi);
      if (0 > DmnRead(fn, &usr, &sys, &Ta2Dsc))                       eX(415);
      Check3dGrid(pgp, &Ta2Dsc, usr.s, 0);                            eG(416);
      TxtClr(&usr);
      TxtClr(&sys);
      qtrb = TmnCreate(jtrb, sizeof(TRBREC), 3, 0,nx, 0,ny, 0,nz);
      if (!qtrb)                                                      eX(417);
      vLOG(5)("PRF: jtrb=%08x created", jtrb);
    }
    else  qtrb = NULL;
      
    for (i=0; i<=nx; i++)
      for (j=0; j<=ny; j++)
        for (k=0; k<=nz; k++) {
          pw1 = AryPtr(&Ww1Dsc, i, j, k);  if (!pw1)                eX(20);
          pw2 = AryPtr(&Ww2Dsc, i, j, k);  if (!pw2)                eX(21);
          pw  = AryPtr(pwnd, i, j, k);  if (!pw)                    eX(22);
          pw->z = pw1->z;
          pw->vx = f1*pw1->vx + f2*pw2->vx;
          pw->vy = f1*pw1->vy + f2*pw2->vy;
          if (pw1->vz>WND_VOLOUT && pw2->vz>WND_VOLOUT)
            pw->vz = f1*pw1->vz + f2*pw2->vz;
          else  pw->vz = WND_VOLOUT;
          if (pdff) {
            pk1 = AryPtr(&Kk1Dsc, i, j, k);  if (!pk1)              eX(120);
            pk2 = AryPtr(&Kk2Dsc, i, j, k);  if (!pk2)              eX(121);
            pk  = AryPtr(pdff, i, j, k);  if (!pk)                  eX(122);
            pk->kh = f1*pk1->kh + f2*pk2->kh;
            pk->kv = f1*pk1->kv + f2*pk2->kv;
          }
          if (qdff) {
            pk1 = AryPtr(&Ka1Dsc, i, j, k);  if (!pk1)              eX(220);
            pk2 = AryPtr(&Ka2Dsc, i, j, k);  if (!pk2)              eX(221);
            pk  = AryPtr(qdff, i, j, k);  if (!pk)                  eX(222);
            pk->kh = f1*pk1->kh + f2*pk2->kh;
            pk->kv = f1*pk1->kv + f2*pk2->kv;
          }
          if (ptrb) {
            pt1 = AryPtr(&Tt1Dsc, i, j, k);  if (!pt1)              eX(320);
            pt2 = AryPtr(&Tt2Dsc, i, j, k);  if (!pt2)              eX(321);
            pt  = AryPtr(ptrb, i, j, k);  if (!pt)                  eX(322);
            pt->su = f1*pt1->su + f2*pt2->su;
            pt->sv = f1*pt1->sv + f2*pt2->sv;
            pt->sw = f1*pt1->sw + f2*pt2->sw;
          }
          if (qtrb) {
            pt1 = AryPtr(&Ta1Dsc, i, j, k);  if (!pt1)              eX(420);
            pt2 = AryPtr(&Ta2Dsc, i, j, k);  if (!pt2)              eX(421);
            pt  = AryPtr(qtrb, i, j, k);  if (!pt)                  eX(422);
            pt->su = f1*pt1->su + f2*pt2->su;
            pt->sv = f1*pt1->sv + f2*pt2->sv;
            pt->sw = f1*pt1->sw + f2*pt2->sw;
          }
        }
    CalcVz(pwnd, pgp->dd);                                          eG(45);
    TmnDetach(iwnd, NULL, NULL, TMN_MODIFY, &wndhdr);               eG(23);
    pwnd = NULL;
    AryFree(&Ww1Dsc);
    AryFree(&Ww2Dsc);
    if (pdff) {
      AryFree(&Kk1Dsc);
      AryFree(&Kk2Dsc);
      TmnDetach(idff, NULL, NULL, TMN_MODIFY, &kffhdr);             eG(123);
      pdff = NULL;
    }
    if (qdff) {
      AryFree(&Ka1Dsc);
      AryFree(&Ka2Dsc);
      TmnDetach(jdff, NULL, NULL, TMN_MODIFY, &kffhdr);             eG(223);
      qdff = NULL;
    }
    if (ptrb) {
      AryFree(&Tt1Dsc);
      AryFree(&Tt2Dsc);
      TmnDetach(itrb, NULL, NULL, TMN_MODIFY, &trbhdr);             eG(323);
      ptrb = NULL;
    }
    if (qtrb) {
      AryFree(&Ta1Dsc);
      AryFree(&Ta2Dsc);
      TmnDetach(jtrb, NULL, NULL, TMN_MODIFY, &trbhdr);             eG(423);
      qtrb = NULL;
    }
  }
  else if (parts & PRFLIB_VX) {
    memset(&ZpDsc, 0, sizeof(ARYDSC));
    sprintf(fn, "%s/zp%1x%1x", libpath, gl, gi);
    if (0 > DmnRead(fn, &usr, &sys, &ZpDsc))                        eX(30);
    Check3dGrid(pgp, &ZpDsc, usr.s, 1);                             eG(31);
    TxtClr(&usr);
    TxtClr(&sys);
    memset(&Vx1Dsc, 0, sizeof(ARYDSC));
    sprintf(fn, "%s/vx%sa%1x%1x", libpath, name1, gl, gi);
    if (0 > DmnRead(fn, &usr, &sys, &Vx1Dsc))                       eX(32);
    TxtClr(&usr);
    TxtClr(&sys);
    memset(&Vx2Dsc, 0, sizeof(ARYDSC));
    sprintf(fn, "%s/vx%sa%1x%1x", libpath, name2, gl, gi);
    if (0 > DmnRead(fn, &usr, &sys, &Vx2Dsc))                       eX(33);
    TxtClr(&usr);
    TxtClr(&sys);
    memset(&Vy1Dsc, 0, sizeof(ARYDSC));
    sprintf(fn, "%s/vy%sa%1x%1x", libpath, name1, gl, gi);
    if (0 > DmnRead(fn, &usr, &sys, &Vy1Dsc))                       eX(34);
    TxtClr(&usr);
    TxtClr(&sys);
    memset(&Vy2Dsc, 0, sizeof(ARYDSC));
    sprintf(fn, "%s/vy%sa%1x%1x", libpath, name2, gl, gi);
    if (0 > DmnRead(fn, &usr, &sys, &Vy2Dsc))                       eX(35);
    TxtClr(&usr);
    TxtClr(&sys);
    pa = TmnCreate(iwnd, sizeof(WNDREC), 3, 0,nx, 0,ny, 0,nz);
    if (!pa)                                                        eX(36);
    for (i=0; i<=nx; i++)
      for (j=0; j<=ny; j++)
        for (k=0; k<=nz; k++) {
          pw  = AryPtr(pa, i, j, k);  if (!pw)                eX(37);
          pz  = AryPtr(&ZpDsc, i, j, k);  if (!pz)            eX(38);
          pw->z = *pz;
          if (k>0 && j>0) {
            p1 = AryPtr(&Vx1Dsc, i, j, k);  if (!p1)          eX(39);
            p2 = AryPtr(&Vx2Dsc, i, j, k);  if (!p2)          eX(40);
            pw->vx = f1*(*p1) + f2*(*p2);
          }
          if (k>0 && i>0) {
            p1 = AryPtr(&Vy1Dsc, i, j, k);  if (!p1)          eX(41);
            p2 = AryPtr(&Vy2Dsc, i, j, k);  if (!p2)          eX(42);
            pw->vy = f1*(*p1) + f2*(*p2);
          }
        }
    AryFree(&ZpDsc);
    AryFree(&Vx1Dsc);
    AryFree(&Vx2Dsc);
    AryFree(&Vy1Dsc);
    AryFree(&Vy2Dsc);
    CalcVz(pa, pgp->dd);                                            eG(43);
    TmnDetach(iwnd, NULL, NULL, TMN_MODIFY, &wndhdr);               eG(44);
  }
  TmnDetach(igp, NULL, NULL, 0, NULL);                              eG(24);
  pgp = NULL;
  TmnDetach(ibp, NULL, NULL, 0, NULL);                              eG(25);
  pbp = NULL;
  TmnDetach(ilt, NULL, NULL, 0, NULL);                              eG(26);
  plt = NULL;
  TxtClr(&wndhdr);
  TxtClr(&kffhdr);
  TxtClr(&trbhdr);
  if (tokens)
    FREE(tokens);
  return 0;
eX_1: eX_2: eX_3: eX_4: eX_5: eX_7:
eX_10: eX_11: eX_17:
eX_20: eX_21: eX_22: eX_23: eX_24: eX_25: eX_26:
eX_36: eX_37: eX_38: eX_39:
eX_40: eX_41: eX_42: eX_43: eX_44: eX_45:
eX_117: eX_123:
eX_217: eX_223:
eX_317: eX_323:
eX_417: eX_423:
eX_120: eX_121: eX_122:
eX_220: eX_221: eX_222:
eX_320: eX_321: eX_322:
eX_420: eX_421: eX_422:
eX_430:
eX_500:
  eMSG(_internal_error_);
eX_6:
  eMSG(_no_lib_);
eX_8:
  eMSG(_no_wind_$_, pl->name);
eX_12:
  eMSG(_improper_base_$$_$$_, name1, name2, f1, f2);  //-2004-12-23
eX_13: eX_15: eX_30: eX_32: eX_33: eX_34: eX_35:
eX_113: eX_115:
eX_213: eX_215:
eX_313: eX_315:
eX_413: eX_415:
  eMSG(_cant_read_$_, fn);
eX_14: eX_16: eX_31:
eX_114: eX_116:
eX_214: eX_216:
eX_314: eX_316:
eX_414: eX_416:
  eMSG(_improper_grid_$_, fn);
eX_150:
		eMSG(_sfc_cant_read_$_, fn);
eX_151:
		eMSG(_sfc_no_line_);
eX_152:
		eMSG(_sfc_invalid_item_number_$_);
eX_153:
		eMSG(_sfc_line_doesnt_match_$$$$$_$$$$$_,
				t1s, t2s, ua, ra, kl, tokens[0], tokens[1], uasfc, rasfc, klsfc);
}

static float InterpolateVv( double *uu, double *zz, double ha, int nh ) {
  double d, d1, d2, h1, h2, h3, p1, p2, p3, u1, u2;
  int ka;
  if (nh < 1)  return 0;
  for (ka=1; ka<=nh; ka++)  if (zz[ka]-zz[0] >= ha)  break;
  if (ka > nh)  return 0;
  p2 = uu[ka];
  d2 = zz[ka] - zz[ka-1];
  h2 = 0.5*(zz[ka-1]+zz[ka]) - zz[0];
  d = (ha - zz[ka-1] + zz[0])/d2;
  if (ka == 1) {
    h1 = -h2;
    d1 = d2;
    p1 = -p2;
  }
  else {
    h1 = 0.5*(zz[ka-2]+zz[ka-1]) - zz[0];
    d1 = zz[ka-1] - zz[ka-2];
    p1 = uu[ka-1];
  }
  u1 = p1 + (p2-p1)*(ha-h1)/(h2-h1);
  if (ka == nh)  return u1;
  p3 = uu[ka+1];
  h3 = 0.5*(zz[ka]+zz[ka+1]) - zz[0];
  u2 = p2 + (p3-p2)*(ha-h2)/(h3-h2);
  return (1-d)*u1 + d*u2;
}

//============================================================== LibTabServer
//
static int PrintLibTab( void ) {
  dP(PrintLibTab);
  long ilt;
  int n, ns, ni, nn;
  LIBREC *pl;
  ARYDSC *ptab;
  ilt = IDENT(LIBtab, 0, 0, 0);
  ptab = TmnAttach(ilt, NULL, NULL, 0, NULL);                         eG(1);
  if (!ptab) {
    vMsg("PRF: no information on library catalog available!");
    return -1;
  }
  ns = ptab->bound[0].hgh + 1;
  nn = (ptab->elmsz - sizeof(LIBREC))/sizeof(int);
  vMsg("Katalog der Windfeldbibliothek:");
  vMsg(" Nr.    ident    Vx(A)    Vy(A) Kl      Lmo");
  for (n=0; n<ns; n++) {
    pl = AryPtr(ptab, n);  if (!pl)  break;
    vMsg("%3d: %8s  %7.2f  %7.2f  %d  %7.1f\\",
      n+1, pl->name, pl->vxa, pl->vya, pl->akl, pl->lmo);
    for (ni=0; ni<nn; ni++)  vMsg(" %08x\\", pl->parts[ni]);
    vMsg("");
    }
  TmnDetach(ilt, NULL, NULL, 0, NULL);                                eG(2);
  return 0;
eX_1: eX_2:
  eMSG(_internal_error_);
}

static long LibTabServer( // make list of library files
char *ss )                // calling arguments
{
  dP(LibTabServer);
  static char libpath[256], path[256], name[256];
  static long tmvalue, itab, print=0;
  ARYDSC *parr=NULL, *plib=NULL;
  ARYDSC ZpDsc, VxDsc, VyDsc, WwDsc;
  LIBREC *pl=NULL;
  TXTSTR usr = { NULL, 0 };
  TXTSTR sys = { NULL, 0 };
  BLMPARM *pbp;
  int gl, gi, l, i, j, k, n, glevel, gindex, nx, ny, nz, ns, nh;
  int ia, ja, akl, parts, netparts[20];
  char **_ll=NULL, *_t=NULL, fn[256], gn[256]="", s[256], *pc;
  double xa, ya, ha, xmin, ymin, xmax, ymax, dd, da, db, va;
  double d, x, y;
  double zz[100], uu[100];
  int nzmax = 99;
  double z00, z01, z10, z11, lmo, v0, v1, va_invalid=-1;
  int izg=0, izp=0;
  int nn=0, ni=0;
  long ibp, igt;
  TXTSTR libhdr = { NULL, 0 };
  vLOG(4)("PRF: LibTabServer(%s) ...", ss);
  if ((ss) && (*ss)) {
    if (*ss != '-') {
      strcpy(path, ss);
      return 0;
    }
    switch (ss[1]) {
      case 'l':  strcpy(name, ss+2);
                 break;
      case 'N':  sscanf(ss+2, "%08lx", &itab);
                 break;
      case 'p':  print = 1;
                 break;
      case 'T':  tmvalue = TmValue(ss+2);
                 break;
      default:   ;
    }
    return 0;
  }
  //
  // make list of grids
  //   
  nn = GrdPprm->numnet;
  if (nn == 0)  nn = 1;
  LibRecSize = sizeof(LIBREC) + nn*sizeof(int);
  LibLvls = ALLOC(nn*sizeof(int));
  LibInxs = ALLOC(nn*sizeof(int));
  if (GrdPprm->numnet > 0) {
    NETREC *pn;
    ARYDSC *pa;
    igt = IDENT(GRDtab, 0, 0, 0);
    pa = TmnAttach(igt, NULL, NULL, 0, NULL);  if (!pa)       eX(70);
    for (ni=0; ni<nn; ni++) {
      pn = (NETREC*) AryPtr(pa, ni+1);  if (!pn)              eX(71);
      LibLvls[ni] = pn->level;                                  //-2005-02-10
      LibInxs[ni] = pn->index;                                  //-2005-02-10
    }
    TmnDetach(igt, NULL, NULL, 0, NULL);                      eG(72);
  }

  MsgCheckPath(path);
  MsgCheckPath(name);
  if (!*name)  strcpy(name, "~lib");
  if (*name == '~')  sprintf(libpath, "%s/%s", path, name+1);
  else  strcpy(libpath, name);
  _ll = _TutGetFileList(libpath);  if (!_ll)                      eX(2);
  n = 0;
  for (i=0; (_ll[i]); i++)  n++;
  //
  // construct string of situation indices
  //
  _t = ALLOC(1+5*n);  if (!_t)                                    eX(3);
  for (i=0; (_ll[i]); i++) {
    strcpy(fn, _ll[i]);
    if (!strcmp(fn, ".") || !strcmp(fn, ".."))  continue;         //-2002-07-03
    strcpy(gn, fn);
    l = strlen(fn);
    for (j=0; j<l; j++)  fn[j] = tolower(fn[j]);
    if (l==13 && !strcmp(fn+8, ".dmna")) {                   //-2001-10-22
      // e.g. w0001a00.dmna
      strcpy(s, "/");
      strncat(s, gn+1, 4);
      if (!strstr(_t, s))  strcat(_t, s);
    }
    else if (l==13 && !strcmp(fn+8, ".dmnb"))  continue;
    else if (l==14 && !strcmp(fn+9, ".dmna")) {
      // e.g. vx0001a00.dmna
      strcpy(s, "/");
      strncat(s, gn+2, 4);
      if (!strstr(_t, s))  strcat(_t, s);
    }
    else if (l==14 && !strcmp(fn+9, ".dmnb"))  continue;
    else if (l==9 && !strcmp(fn+4, ".dmna")) {
      // e.g. zg00.dmna
      if      (!strncmp(fn, "zg", 2))  izg = 1;
      else if (!strncmp(fn, "zp", 2))  izp = 1;
    }
    else if (l==9 && !strcmp(fn+4, ".dmnb"))  continue;
    else if (l > 4 && !strcmp(fn+l-4, ".log")) continue;          //-2012-10-30
    else                                                          eX(7);
  }
  ns = strlen(_t)/5;
  if (ns == 0)                                                    eX(8); //-2002-01-09
  //
  // construct list of situations
  //
  plib = TmnCreate(itab, LibRecSize, 1, 0, ns-1);                 eG(9);
  for (i=0; (_ll[i]); i++) {  // construct the listing
    strcpy(fn, _ll[i]);
    strcpy(gn, fn);
    l = strlen(fn);
    for (j=0; j<l; j++)  fn[j] = tolower(fn[j]);
    if (l==13 && !strcmp(fn+8, ".dmna")) {                   //-2001-10-22
      // e.g. w0001a00.dmna
      if (2 != sscanf(fn+6, "%1x%1x", &gl, &gi))                  eX(10);
      strcpy(s, "/");
      strncat(s, gn+1, 4);
    }
    else if (l==14 && !strcmp(fn+9, ".dmna")) {
      // e.g. vx0001a00.dmna
      if (2 != sscanf(fn+7, "%1x%1x", &gl, &gi))                  eX(11);
      strcpy(s, "/");
      strncat(s, gn+2, 4);
    }
    else  continue;
    for (ni=0; ni<nn; ni++)
      if (gl==LibLvls[ni] && gi==LibInxs[ni]) break;
    if (ni >= nn)                                                 eX(78);
    pc = strstr(_t, s);  if (!pc)                                 eX(12);
    n = (pc - _t)/5;
    pl = AryPtr(plib, n);  if (!pl)                               eX(13);
    if (l==12 || l==13) {                                         //-2001-10-22
      if (!*(pl->name))  strncat(pl->name, gn+1, 4);
      if      (*fn == 'w')  pl->parts[ni] |= PRFLIB_WW;
      else if (*fn == 'v') {
        if      (fn[5] == 'a')  pl->parts[ni] |= PRFLIB_TT;       //-2002-07-14
        else if (fn[5] == 'd')  pl->parts[ni] |= PRFLIB_TA;
      }
      else if (*fn == 'k') {
        if      (fn[5] == 'a')  pl->parts[ni] |= PRFLIB_KK;       //-2002-07-14
        else if (fn[5] == 'd')  pl->parts[ni] |= PRFLIB_KA;
      }
    }
    else if (l == 14) {
      if (!*(pl->name))  strncat(pl->name, gn+2, 4);
      if      (!strncmp(fn, "vx", 2))  pl->parts[ni] |= PRFLIB_VX;
      else if (!strncmp(fn, "vy", 2))  pl->parts[ni] |= PRFLIB_VY;
      else if (!strncmp(fn, "ku", 2))  pl->parts[ni] |= PRFLIB_KU;
      else if (!strncmp(fn, "kv", 2))  pl->parts[ni] |= PRFLIB_KV;
      else if (!strncmp(fn, "kw", 2))  pl->parts[ni] |= PRFLIB_KW;
      else if (!strncmp(fn, "su", 2))  pl->parts[ni] |= PRFLIB_SU;
      else if (!strncmp(fn, "sv", 2))  pl->parts[ni] |= PRFLIB_SV;
      else if (!strncmp(fn, "sw", 2))  pl->parts[ni] |= PRFLIB_SW;
      else if (!strncmp(fn, "th", 2))  pl->parts[ni] |= PRFLIB_TH;
      if (izg)  pl->parts[ni] |= PRFLIB_ZG;
      if (izp)  pl->parts[ni] |= PRFLIB_ZP;
    }
  }
  FREE(_ll);  _ll = NULL;
  FREE(_t);   _t  = NULL;
  //
  // check if the library is complete and of single type
  //
  // fields required for all nets and situations
  parts = 0;
  for (n=0; n<ns; n++) {
    pl = AryPtr(plib, n);  if (!pl)                            eX(61);
    for (ni=0; ni<nn; ni++)  parts |= pl->parts[ni];
  }
  // fields required for all situations and a specified grid
  for (ni=0; ni<nn; ni++) { //-2004-08
    netparts[ni] = 0;
    for (n=0; n<ns; n++) {
      pl = AryPtr(plib, n);  if (!pl)                          eX(61);
      netparts[ni] |= pl->parts[ni];
    }
  }   
  if (!(parts & (PRFLIB_VX | PRFLIB_VY | PRFLIB_WW)))          eX(19);
  l = 0;
  for (n=0; n<ns; n++) {
    pl = AryPtr(plib, n);  if (!pl)                            eX(61);
    strcpy(s, pl->name);
    for (ni=0; ni<nn; ni++) {
      gl = LibLvls[ni];
      gi = LibInxs[ni];
      k = pl->parts[ni];
      if (parts & PRFLIB_WW) {
        if (!(k & PRFLIB_WW)) {
          vMsg(_no_wind_$$$_, s, gl, gi);
          l++;
        }
        if ((parts & PRFLIB_KK) && !(k & PRFLIB_KK)) {
          vMsg(_no_diff_a_$$$_, s, gl, gi);
          l++;
        }
        if ((netparts[ni] & PRFLIB_KA) && !(k & PRFLIB_KA)) {
          vMsg(_no_diff_d_$$$_, s, gl, gi);
          l++;
        }
        if ((parts & PRFLIB_TT) && !(k & PRFLIB_TT)) {
          vMsg(_no_turb_a_$$$_, s, gl, gi);
          l++;
        }
        if ((netparts[ni] & PRFLIB_TA) && !(k & PRFLIB_TA)) {
          vMsg(_no_turb_d_$$$_, s, gl, gi);
          l++;
        }
      }
      else if (parts & (PRFLIB_VX | PRFLIB_VY)) {
        if (!(k & PRFLIB_VX)) {
          vMsg(_no_vx_$$$_, s, gl, gi);
          l++;
        }
        if (!(k & PRFLIB_VY)) {
          vMsg(_no_vy_$$$_, s, gl, gi);
          l++;
        }
        if (n == 0) {
          if (!(k & PRFLIB_ZG)) {
            vMsg(_no_zg_$$_, gl, gi);
            l++;
          }
          if (!(k & PRFLIB_ZP)) {
            vMsg(_no_zp_$$_, gl, gi);
            l++;
          }
        }
      }
      if (l > 0)                                                 eX(14);
    }
  }
  
  if ((parts & PRFLIB_KK) || (parts & PRFLIB_KA) || (parts & PRFLIB_TT) || (parts & PRFLIB_TA))
  		vMsg("");

  if (parts & PRFLIB_KK) 
    vMsg(_lib_new_k_);
  if (parts & PRFLIB_KA) { //-2004-08
    strcpy(s, _lib_additional_k_);
    if (nn > 1) {    
      strcat(s, _grids_);
      for (ni=nn-1; ni>=0; ni--) 
        if (netparts[ni] & PRFLIB_KA) 
          sprintf(s, "%s%1d,", s, nn-ni);
      s[strlen(s)-1] = 0; strcat(s, ")");
    }
    strcat(s, ".");
    vMsg(s);
  }
  if (parts & PRFLIB_TT) 
    vMsg(_lib_new_sig_);
  if (parts & PRFLIB_TA) { //-2004-08
    strcpy(s, _lib_additional_sig_);
    if (nn > 1) {    
      strcat(s, _grids_);
      for (ni=nn-1; ni>=0; ni--) 
        if (netparts[ni] & PRFLIB_KA) 
          sprintf(s, "%s%1d,", s, nn-ni);
      s[strlen(s)-1] = 0; strcat(s, ")");
    }
    strcat(s, ".");
    vMsg(s);
  }
  //
  // get vxa and vya for each situation
  //
  ibp = IDENT(BLMpar, 0, 0, 0);
  parr =  TmnAttach(ibp, NULL, NULL, 0, NULL);  if (!parr)       eX(20);
  pbp = (BLMPARM*) parr->start;
  xa = pbp->AnmXpos;
  ya = pbp->AnmYpos;
  ha = pbp->AnmHeight;
  if (pbp->AnmHeightW > 0) {                                      //-2012-11-05
  		ha = pbp->AnmHeightW;
  		vMsg(_using_hw_$$_, ha, pbp->AnmHeight); 
  }
  TmnDetach(ibp, NULL, NULL, 0, NULL);
  pbp = NULL;
  memset(&ZpDsc, 0, sizeof(ARYDSC));
  memset(&VxDsc, 0, sizeof(ARYDSC));
  memset(&VyDsc, 0, sizeof(ARYDSC));
  memset(&WwDsc, 0, sizeof(ARYDSC));
  if (parts & PRFLIB_VX) {
    for (ni=nn-1; ni>=0; ni--) {
      glevel = LibLvls[ni];
      gindex = LibInxs[ni];
      sprintf(fn, "%s/zp%1x%1x", libpath, glevel, gindex);
      if (0 > DmnRead(fn, &usr, &sys, &ZpDsc))                      eX(21);
      if (ZpDsc.numdm != 3)                                         eX(22);
      if (ZpDsc.bound[0].low!=0
       || ZpDsc.bound[1].low!=0
       || ZpDsc.bound[2].low!=0)                                    eX(23);
      nx = ZpDsc.bound[0].hgh;
      ny = ZpDsc.bound[1].hgh;
      nz = ZpDsc.bound[2].hgh;
      if (1 != DmnGetDouble(usr.s, "dd|delt|delta", "%lf", &dd, 1)) eX(24);
      if (1 != DmnGetDouble(usr.s, "xmin|x0", "%lf", &xmin, 1))     eX(25);
      xmax = xmin + nx*dd;
      if (1 != DmnGetDouble(usr.s, "ymin|y0", "%lf", &ymin, 1))     eX(26);
      ymax = ymin + ny*dd;
      if (xa>=xmin && xa<=xmax && ya>=ymin && ya<=ymax) break;
      AryFree(&ZpDsc);
    }
    if (ni < 0)                                                     eX(27);
    ia = 1 + (xa-xmin)/dd;  if (ia > nx)  ia = nx;
    da = (xa-xmin)/dd - (ia-1);
    ja = 1 + (ya-ymin)/dd;  if (ja > ny)  ja = ny;
    db = (ya-ymin)/dd - (ja-1);
    nh = (nz <= nzmax) ? nz : nzmax;
    for (k=0; k<=nh; k++) {
      z00 = *(float*)AryPtrX(&ZpDsc, ia-1, ja-1, k);
      z10 = *(float*)AryPtrX(&ZpDsc, ia  , ja-1, k);
      z01 = *(float*)AryPtrX(&ZpDsc, ia-1, ja  , k);
      z11 = *(float*)AryPtrX(&ZpDsc, ia  , ja  , k);
      zz[k] = z00 + da*(z10-z00) + db*(z01-z00) + da*db*(z00+z11-z10-z01);
    }
    for (k=1; k<=nh; k++)
      if (zz[k]-zz[0] >= ha)  break;
    if (k > nh)                                                       eX(28);
    for (n=0; n<ns; n++) {
      pl = AryPtr(plib, n);  if (!pl)                                 eX(29);
      sprintf(fn, "%s/vx%sa%1x%1x", libpath, pl->name, glevel, gindex);
      if (0 > DmnRead(fn, &usr, &sys, &VxDsc))                        eX(30);
      if (0 > AryAssert(&VxDsc, sizeof(float), 3, 0,nx, 1,ny, 1,nz))  eX(31);
      if (1 != DmnGetDouble(usr.s, "dd|delt|delta", "%lf", &d, 1))    eX(32);
      if (1 != DmnGetDouble(usr.s, "xmin|x0", "%lf", &x, 1))          eX(33);
      if (1 != DmnGetDouble(usr.s, "ymin|y0", "%lf", &y, 1))          eX(34);
      if (d!=dd || x!=xmin || y!=ymin)                                eX(35);
      akl = 0;
      DmnGetInt(usr.s, "akl", "%d", &akl, 1);
      lmo = 0;
      DmnGetDouble(usr.s, "lmo", "%lf", &lmo, 1);
       if (akl == 0) {                                             //-2014-01-24
        akl = pl->name[0] - '0';
        if (akl<1 || akl>6)                                           eX(36);
      }
      pl->akl = akl;
      pl->lmo = lmo;
      for (k=1; k<=nh; k++) {
        v0 = *(float*)AryPtrX(&VxDsc, ia-1, ja, k);
        v1 = *(float*)AryPtrX(&VxDsc, ia  , ja, k);
        uu[k] = v0 + da*(v1-v0);
      }
      pl->vxa = InterpolateVv(uu, zz, ha, nh);
      AryFree(&VxDsc);
      TxtClr(&sys);
      TxtClr(&usr);
      sprintf(fn, "%s/vy%sa%1x%1x", libpath, pl->name, glevel, gindex);
      if (0 > DmnRead(fn, &usr, &sys, &VyDsc))                        eX(37);
      if (0 > AryAssert(&VyDsc, sizeof(float), 3, 1,nx, 0,ny, 1,nz))  eX(38);
      if (1 != DmnGetDouble(usr.s, "dd|delt|delta", "%lf", &d, 1))    eX(39);
      if (1 != DmnGetDouble(usr.s, "xmin|x0", "%lf", &x, 1))          eX(40);
      if (1 != DmnGetDouble(usr.s, "ymin|y0", "%lf", &y, 1))          eX(41);
      if (d!=dd || x!=xmin || y!=ymin)                                eX(42);
      for (k=1; k<=nh; k++) {
        v0 = *(float*)AryPtrX(&VyDsc, ia, ja-1, k);
        v1 = *(float*)AryPtrX(&VyDsc, ia, ja  , k);
        uu[k] = v0 + db*(v1-v0);
      }
      pl->vya = InterpolateVv(uu, zz, ha, nh);
      va = sqrt(pl->vxa*pl->vxa + pl->vya*pl->vya);
      if (va < VaMin) {                                               //-2004-05-11
        va_invalid = va;
        strcpy(gn, fn);       
      }
      AryFree(&VyDsc);
      TxtClr(&sys);
      TxtClr(&usr);
    }
  }
  else if (parts & PRFLIB_WW) {
    n = 0;
    for (ni=nn-1; ni>=0; ni--) {
      glevel = LibLvls[ni];
      gindex = LibInxs[ni];
      pl = AryPtr(plib, n);  if (!pl)                                 eX(43);
      sprintf(fn, "%s/w%sa%1x%1x", libpath, pl->name, glevel, gindex);
      if (0 > DmnRead(fn, &usr, &sys, &WwDsc))                        eX(44);
      if (WwDsc.numdm != 3)                                           eX(45);
      if (1 != DmnGetDouble(usr.s, "dd|delt|delta", "%lf", &dd, 1))   eX(47);
      if (1 != DmnGetDouble(usr.s, "xmin|x0", "%lf", &xmin, 1))       eX(48);
      if (1 != DmnGetDouble(usr.s, "ymin|y0", "%lf", &ymin, 1))       eX(49);
      nx = WwDsc.bound[0].hgh;
      ny = WwDsc.bound[1].hgh;
      nz = WwDsc.bound[2].hgh;
      xmax = xmin + nx*dd;
      ymax = ymin + ny*dd;
      AryFree(&WwDsc);
      TxtClr(&sys);
      TxtClr(&usr);
      if (xa>=xmin && xa<=xmax && ya>=ymin && ya<=ymax) break;
    }
    if (ni < 0)                                                       eX(27);
    for (n=0; n<ns; n++) {
      pl = AryPtr(plib, n);  if (!pl)                                 eX(43);
      sprintf(fn, "%s/w%sa%1x%1x", libpath, pl->name, glevel, gindex);
      if (0 > DmnRead(fn, &usr, &sys, &WwDsc))                        eX(44);
      if (WwDsc.numdm != 3)                                           eX(45);
      if (0 > AryAssert(&WwDsc, sizeof(WNDREC), 3, 0,nx, 0,ny, 0,nz)) eX(46);
      if (1 != DmnGetDouble(usr.s, "dd|delt|delta", "%lf", &d, 1))    eX(47);
      if (1 != DmnGetDouble(usr.s, "xmin|x0", "%lf", &x, 1))          eX(48);
      if (1 != DmnGetDouble(usr.s, "ymin|y0", "%lf", &y, 1))          eX(49);
      akl = 0;
      DmnGetInt(usr.s, "akl", "%d", &akl, 1);
      lmo = 0;
      DmnGetDouble(usr.s, "lmo", "%lf", &lmo, 1);
      if (akl == 0) {                                             //-2014-01-24
        akl = pl->name[0] - '0';
        if (akl<1 || akl>6)                                           eX(36);
      }
      pl->akl = akl;
      pl->lmo = lmo;
      if (d!=dd || x!=xmin || y!=ymin)                                eX(50); //-2014-06-26
      ia = 1 + (xa-xmin)/dd;  if (ia > nx)  ia = nx;
      da = (xa-xmin)/dd - (ia-1);
      ja = 1 + (ya-ymin)/dd;  if (ja > ny)  ja = ny;
      db = (ya-ymin)/dd - (ja-1);
      nh = (nz <= nzmax) ? nz : nzmax;
      for (k=0; k<=nh; k++) {
        z00 = (*(WNDREC*)AryPtrX(&WwDsc, ia-1, ja-1, k)).z;
        z10 = (*(WNDREC*)AryPtrX(&WwDsc, ia  , ja-1, k)).z;
        z01 = (*(WNDREC*)AryPtrX(&WwDsc, ia-1, ja  , k)).z;
        z11 = (*(WNDREC*)AryPtrX(&WwDsc, ia  , ja  , k)).z;
        zz[k] = z00 + da*(z10-z00) + db*(z01-z00) + da*db*(z00+z11-z10-z01);
      }
      for (k=1; k<=nh; k++)
        if (zz[k]-zz[0] >= ha)  break;
      if (k > nh)                                                   eX(52);
      for (k=1; k<=nh; k++) {
        v0 = (*(WNDREC*)AryPtrX(&WwDsc, ia-1, ja, k)).vx;
        v1 = (*(WNDREC*)AryPtrX(&WwDsc, ia  , ja, k)).vx;
        uu[k] = v0 + da*(v1-v0);
      }
      pl->vxa = InterpolateVv(uu, zz, ha, nh);
      for (k=1; k<=nh; k++) {
        v0 = (*(WNDREC*)AryPtrX(&WwDsc, ia, ja-1, k)).vy;
        v1 = (*(WNDREC*)AryPtrX(&WwDsc, ia, ja  , k)).vy;
        uu[k] = v0 + db*(v1-v0);
      }
      pl->vya = InterpolateVv(uu, zz, ha, nh);
      va = sqrt(pl->vxa*pl->vxa + pl->vya*pl->vya);
      if (va < VaMin) {                                                       //-2004-05-11
        va_invalid = va; 
        strcpy(gn, fn); 
      }
      AryFree(&WwDsc);
      TxtClr(&sys);
      TxtClr(&usr);
    } // for n
  }
  else                                                              eX(53);
  AryFree(&ZpDsc);
  AryFree(&VxDsc);
  AryFree(&VyDsc);
  AryFree(&WwDsc);
  TxtCpy(&libhdr, "\nform  \"Name%8sParts%08xAkl%2dVxa%5.2fVya%5.2fLmo%7.1f\"\n"); 
  TmnDetach(itab, NULL, NULL, TMN_MODIFY, &libhdr);                 eG(54);
  if (va_invalid >= 0)  print = 1;                                          //-2003-10-12
  if (print)  PrintLibTab();                                                //-2003-10-12
  if (va_invalid >= 0)                                              eX(55); //-2003-10-12
  TxtClr(&libhdr);
  return 0;
eX_3: eX_9: eX_12: eX_13: eX_20: eX_29: eX_43: eX_53: eX_54:
eX_61:
eX_70: eX_71: eX_72:
  eMSG(_internal_error_);
eX_2:
  eMSG(_lib_$_not_found_, libpath);
eX_10: eX_11:
  eMSG(_invalid_name_$_, gn);
eX_7:
  eMSG(_file_$_not_registered_, gn);
eX_8:
  eMSG(_empty_lib_$_, libpath);
eX_14:
  eMSG(_missing_fields_$_$_, libpath, l);
  //eMSG("%d files missing in library \"%s\"!", l, libpath);
eX_19:
  eMSG(_no_wind_fields_$_, libpath);
eX_21: eX_30: eX_37: eX_44:
  eMSG(_file_$_not_found_, fn);
eX_22: eX_23: eX_24: eX_25: eX_26:
eX_31: eX_32: eX_33: eX_34: eX_35: eX_36:
eX_38: eX_39: eX_40: eX_41: eX_42:
eX_45: eX_46: eX_47: eX_48: eX_49: eX_50:
  eMSG(_improper_structure_$_, fn);
eX_27:
  eMSG(_anemometer_$$_outside_, xa, ya);
eX_28: eX_52:
  eMSG(_anemometer_height_);
eX_55:
  eMSG(_improper_wind_field_$$_, gn, va_invalid);            //-2004-05-11
eX_78:
  eMSG(_strange_field_$_, fn);
}

//=================================================================== Check3D
//
static long Check3D( char *name, char *header, ARYDSC *pa, int elmsz )
  {
  dP(Check3D);
  int rco, rcx, rcy, rcd, rcz, rcs;
  int k;
  float xmin, ymin, delta, zscl, sscl, zk[101], *pf;
  char option[256];
  vLOG(4)("PRF: Check3D(%s, ...)", name);
  if (!pa)                                      eX(99);
  if (pa->elmsz != elmsz)                       eX(1);
  if (pa->numdm != 3 )                          eX(2);
  if (pa->bound[0].low != 0)                    eX(3);
  if (pa->bound[1].low != 0)                    eX(4);
  if (pa->bound[2].low != 0)                    eX(5);
  if (pa->bound[0].hgh != Pgp->nx )             eX(6);
  if (pa->bound[1].hgh != Pgp->ny )             eX(7);
  if (pa->bound[2].hgh != Pgp->nz )             eX(8);
  rco = DmnCpyString(header, "option", option, 256);              //-2011-06-29
  if (rco == 0 && NULL != strstr(option, "NOCHECK"))
    return 0;
  rcx = 0;  rcy = 0;  rcd = 0;  rcz = 0;
  rcx = DmnGetFloat(header, "xmin|x0", "%f", &xmin, 1);           //-2011-06-29
  if ((rcx > 0) && isuneq(xmin, Pgp->xmin, 0.02))               eX(9);
  rcy = DmnGetFloat(header, "ymin|y0", "%f", &ymin, 1);           //-2011-06-29
  if ((rcy > 0) && isuneq(ymin ,Pgp->ymin, 0.02))               eX(10);
  rcs = DmnGetFloat(header, "zscl", "%f", &zscl, 1);              //-2011-06-29
  if ((rcs > 0) && isuneq(zscl, Pgp->zscl, 0.02))               eX(11);
  rcs = DmnGetFloat(header, "sscl", "%f", &sscl, 1);              //-2011-06-29
  if ((rcs > 0) && isuneq(sscl, Pgp->sscl, 0.02))               eX(15);
  rcd = DmnGetFloat(header, "dd|delta|delt", "%f", &delta, 1);    //-2011-06-29
  if ((rcd > 0) && isuneq(delta, Pgp->dd, 0.02))                eX(12);
  rcz = DmnGetFloat(header, "sk|zk|hh", "%f", zk, 100+1);         //-2011-06-29
  if (rcz < Pgp->nz+1)                                          eX(13);
  pf = (float*) Pga->start;
  for (k=0; k<=Pgp->nz; k++)  if (isuneq(zk[k], pf[k], 0.02))   eX(14);
  return 0;
eX_99:
  eMSG(_$_undefined_, name);
eX_1:
  eMSG(_$_element_size_$$_, name, pa->elmsz, elmsz);
eX_2:
  eMSG(_$_rank_$_, name, pa->numdm);
eX_3:  eX_6:
  eMSG(_$_i_range_$$_$$_, name, pa->bound[0].low, pa->bound[0].hgh, 0, Pgp->nx);
eX_4:  eX_7:
  eMSG(_$_j_range_$$_$$_, name, pa->bound[1].low, pa->bound[1].hgh, 0, Pgp->ny);
eX_5:  eX_8:
  eMSG(_$_k_range_$$_$$_, name, pa->bound[2].low, pa->bound[2].hgh, 0, Pgp->nz);
eX_9:
  eMSG(_$_xmin_$$_, name, xmin, Pgp->xmin);
eX_10:
  eMSG(_$_ymin_$$_, name, ymin, Pgp->ymin);
eX_11:
  eMSG(_$_zscl_$$_, name, zscl, Pgp->zscl);
eX_12:
  eMSG(_$_delt_$$_, name, delta, Pgp->dd);
eX_13:
  eMSG(_$_count_sk_$_, name, Pgp->nz+1);
eX_14:
  eMSG(_$_sk_$_$$_, name, k, zk[k], pf[k]);
eX_15:
  eMSG(_$_sscl_$$_, name, sscl, Pgp->sscl);
  }

//================================================================= Clc1dMet
//
static long Clc1dMet( void )  /* Meteorologie-Profil berechnen.       */
  {
  dP(Clc1dMet);
  float h1;
  void *p0;
  PR1REC *p1;
  PR2REC *p2;
  PR3REC *p3;
  BLMREC *pm;
  int k, nx, ny, nz;
  long iba, usage;
  double a;
  vLOG(4)("PRF: Clc1dMet() ...");
  nx = Pgp->nx;
  ny = Pgp->ny;
  nz = Pgp->nz;
  if (!Pprf) {
    Pprf = TmnCreate( Iprf, PrfRecLen, 1, 0, nz );              eG(2);
    }
  iba = IDENT(BLMarr, 0, 0, 0);                                 //-2001-06-29
  TmnInfo(iba, NULL, NULL, &usage, NULL, NULL);                 //
  if (usage & TMN_INVALID)  return 0;                           //
  for (k=0; k<=nz; k++) {
    pm = (BLMREC*) AryPtrX(Pba, k);
    p0 = AryPtrX(Pprf, k);
    p1 = (PR1REC*) p0;
    p2 = (PR2REC*) p0;
    p3 = (PR3REC*) p0;
    a = pm->d/RADIAN;
    p1->h  = pm->z;
    p1->vx = -pm->u*sin(a);
    p1->vy = -pm->u*cos(a);
    p1->vz = 0.0;
    h1  = pm->z;
    switch (PrfMode) {
      case 1:  p1->kw = pm->sw*pm->sw*pm->tw;
               p1->kv = pm->sv*pm->sv*pm->tv;
               break;
      case 2:  p2->kv = pm->sv*pm->sv*pm->tv;
               p2->kw = pm->sw*pm->sw*pm->tw;
               p2->sv = pm->sv;
               p2->sw = pm->sw;
               p2->ts = pm->ths;
               break;
      case 3:  p3->ku = pm->su*pm->su*pm->tu;
               p3->kv = pm->sv*pm->sv*pm->tv;
               p3->kw = pm->sw*pm->sw*pm->tw;
               p3->kc = 0;
               p3->su = pm->su;
               p3->sv = pm->sv;
               p3->sw = pm->sw;
               p3->sc = pm->suw;
               p3->ts = pm->ths;
               break;
      default:                                                          eX(3);
      }
    }
  vLOG(4)("PRF:Clc1dMet returning");
  return 0;
eX_2:
  eMSG(_cant_create_profile_);
eX_3:
  eMSG(_invalid_mode_$_, PrfMode);
  }

//=================================================================== Clc3dMet
//
static long Clc3dMet( void )
  {
  dP(Clc3dMet);
  float ti, ufac, sfac, kfac, a1, a2, h, zg, zmg;
  float tu, tv, tw, su, sv, sw, sc, ts, ku, kv, kw, du, dv, dw;
  float tmin, kmin, th1, th2, z1, z2;
  volatile float vz;
  int mask;
  void *p0;
  char name[256];                                                 //-2003-01-30
  TXTSTR header = { NULL, 0 };                                    //-2011-06-29
  PR1REC *p1;
  PR2REC *p2;
  PR3REC *p3;
  BLMREC *pm1, *pm2;
  WNDREC *pw;
  DFFREC *pd, *qd;
  TRBREC *pt, *qt;
  int i, j, k, nx, ny, nz, l, ba3d, nk;
  long usage;
  vLOG(4)("PRF: Clc3dMet() ...");
// time(&ct);
// printf("START: clock=%d (%d) %s\n", clock(), CLK_TCK, ctime(&ct));
  
  Iwnd = IDENT(WNDarr, WND_FINAL, GridLevel, GridIndex);
  Itrb = IDENT(VARarr, WND_FINAL, GridLevel, GridIndex);
  Jtrb = IDENT(VARarr, WND_WAKE,  GridLevel, GridIndex);
  Idff = IDENT(KFFarr, WND_FINAL, GridLevel, GridIndex);
  Jdff = IDENT(KFFarr, WND_WAKE,  GridLevel, GridIndex);
  mask = ~(NMS_GROUP | NMS_LEVEL | NMS_GRIDN);
  TmnClearAll(MetT1, mask, Iwnd, Itrb, Jtrb, Idff, Jdff, TMN_NOID); eG(40);
  nx = Pgp->nx;
  ny = Pgp->ny;
  nz = Pgp->nz;
  if (!Pprf) {
    Pprf = TmnCreate(Iprf, PrfRecLen, 3, 0, nx, 0, ny, 0, nz);      eG(9);
  }
  TmnInfo(Iba, NULL, NULL, &usage, NULL, NULL);                     //-2001-06-29
  if (usage & TMN_INVALID)  return 0;                               //
  TmnDetach(Iba, NULL, NULL, 0, NULL);                              eG(61);
  Pba = NULL;
  strcpy(name, NmsName(Iwnd));
  Pwnd = TmnAttach(Iwnd, &MetT1, &MetT2, 0, &header);  if (!Pwnd)   eX(2);
  Check3D(name, header.s, Pwnd, sizeof(WNDREC));                    eG(3);
  vLOG(5)("PRF: File %s (%08x) found and checked", name, Iwnd);    //-2005-02-10
  if (DmnCpyString(header.s, "valdef|vldf", ValDef, 80))            eX(11);  //-2011-06-29
  if (!*ValDef)                                                     eX(12);
  
  strcpy(name, NmsName(Itrb));
  Ptrb = TmnAttach(Itrb, &MetT1, &MetT2, 0, &header);         eG(115);
  if (Ptrb) {
    Check3D(name, header.s, Ptrb, sizeof(TRBREC));            eG(116);
    vLOG(5)("PRF: File %s (%08x) found and checked", name, Itrb);  //-2005-02-10
  }
  else vLOG(5)("PRF: File %s (%08x) not found", name, Itrb);      //-2005-02-10
  strcpy(name, NmsName(Jtrb));
  Qtrb = TmnAttach(Jtrb, &MetT1, &MetT2, 0, &header);         eG(125);
  if (Qtrb) {
    Check3D(name, header.s, Qtrb, sizeof(TRBREC));            eG(126);
    vLOG(5)("PRF: File %s (%08x) found and checked", name, Jtrb);  //-2005-02-10
  }
  else vLOG(5)("PRF: File %s (%08x) not found", name, Jtrb);      //-2005-02-10
  strcpy(name, NmsName(Idff));
  Pdff = TmnAttach(Idff, &MetT1, &MetT2, 0, &header);         eG(135);
  if (Pdff) {
    Check3D(name, header.s, Pdff, sizeof(DFFREC));            eG(136);
    vLOG(5)("PRF: File %s (%08x) found and checked", name, Idff);  //-2005-02-10
  }
  else vLOG(5)("PRF: File %s (%08x) not found", name, Idff);      //-2005-02-10
  strcpy(name, NmsName(Jdff));
  Qdff = TmnAttach(Jdff, &MetT1, &MetT2, 0, &header);         eG(145);
  if (Qdff) {                                                   //-2005-02-10
    Check3D(name, header.s, Qdff, sizeof(DFFREC));            eG(146);
    vLOG(5)("PRF: File %s (%08x) found and checked", name, Jdff);  //-2005-02-10
  }
  else vLOG(5)("PRF: File %s (%08x) not found", name, Jdff);      //-2005-02-10
  
  sfac = 1;
  kfac = 1;
  ti = 3600;
  tmin = 0;
  kmin = 0;
  ufac = 1;
  Pba = TmnAttach(Iba, &MetT1, &MetT2, 0, NULL);  if (!Pba)   eX(62);
  if (Pba->numdm == 3) {
    ba3d = 1;
    nk = Pba->bound[2].hgh;
    }
  else {
    ba3d = 0;
    nk = Pba->bound[0].hgh;
    }
  pd = NULL;  
  qd = NULL;
  pt = NULL;  
  qt = NULL;
  for (i=0; i<=nx; i++)
    for (j=0; j<=ny; j++)
      for (k=0; k<=nz; k++) {
        pw = (WNDREC*) AryPtr(Pwnd, i, j, k);  if (!pw)                    eX(21);
        pd = (Pdff) ? (DFFREC*) AryPtrX(Pdff, i, j, k) : NULL;
        pt = (Ptrb) ? (TRBREC*) AryPtrX(Ptrb, i, j, k) : NULL;
        qd = (Qdff) ? (DFFREC*) AryPtrX(Qdff, i, j, k) : NULL;
        qt = (Qtrb) ? (TRBREC*) AryPtrX(Qtrb, i, j, k) : NULL;
                                                                           eG(32);
        if (k == 0)  zg = pw->z;
        p0 = AryPtr(Pprf, i, j, k);  if (!p0)                              eX(22);
        p1 = (PR1REC*) p0;
        p2 = (PR2REC*) p0;
        p3 = (PR3REC*) p0;
        p1->h  = pw->z;
        p1->vx = ufac*pw->vx;
        p1->vy = ufac*pw->vy;      
        vz = pw->vz;
        if (vz == WND_VOLOUT)  p1->vz = vz;                             //-2003-10-12
        else if (vz<-VzMax || vz>VzMax) {                       eX(4);  //-2003-06-16
        }
        else  p1->vz = ufac*vz;                                         //-2003-10-12
        h = pw->z - zg;
        a1 = 1.0;
        a2 = 0.0;
        pm1 = (BLMREC*) ((ba3d) ? AryPtr(Pba, i, j, 0) : AryPtr(Pba, 0));
        if (!pm1)                                                       eX(20);
        zmg = pm1->z;
        for (l=0; l<=nk; l++) {
          pm2 = (BLMREC*) ((ba3d) ? AryPtr(Pba, i, j, l) : AryPtr(Pba, l));
          if (!pm2)                                                     eX(20);
          if (pm2->z-zmg >= h)  break;
          pm1 = pm2;
          }
        if ((l > 0)  && (l <= nk)) {
          a1 = (pm2->z-zmg - h)/(pm2->z - pm1->z);
          a2 = 1.0 - a1;
          }
        tu = a1*pm1->tu + a2*pm2->tu;
        tv = a1*pm1->tv + a2*pm2->tv;
        tw = a1*pm1->tw + a2*pm2->tw;
        if (pt) {
          su = ufac*pt->su;
          sv = ufac*pt->sv;
          sw = ufac*pt->sw;
          sc = 0;
          tu /= ufac;
          tv /= ufac;
          tw /= ufac;
          if (k == 0) {
            th1 = pt->th;
            z1 = p1->h;
            ts = 0; }
          else {
            th2 = pt->th;
            z2 = p1->h;
            ts = (th2-th1)/(z2-z1);
            th1 = th2;
            z1 = z2; }
          }
        else {
          su = a1*pm1->su + a2*pm2->su;
          sv = a1*pm1->sv + a2*pm2->sv;
          sw = a1*pm1->sw + a2*pm2->sw;
          sc = a1*pm1->suw + a2*pm2->suw;
          ts = a1*pm1->ths + a2*pm2->ths;
          }
        ku = su*su*tu;
        kv = sv*sv*tv;
        kw = sw*sw*tw;
        if (qt) {
          du = sfac*ufac*qt->su;  su = sqrt(su*su + du*du);
          dv = sfac*ufac*qt->sv;  sv = sqrt(sv*sv + dv*dv);
          dw = sfac*ufac*qt->sw;  sw = sqrt(sw*sw + dw*dw);
          }
        if (pd) {
          ku = ufac*pd->kh;
          kv = ufac*pd->kh;
          kw = ufac*pd->kv;
          }
        if (qd) {
          ku += kfac*ufac*qd->kh;
          kv += kfac*ufac*qd->kh;
          kw += kfac*ufac*qd->kv;
          }
        if (ku < kmin)  ku = kmin;
        if (su*su*tmin > ku)  su = sqrt(ku/tmin);
        if (kv < kmin)  kv = kmin;
        if (sv*sv*tmin > kv)  sv = sqrt(kv/tmin);
        if (kw < kmin)  kw = kmin;
        if (sw*sw*tmin > kw)  sw = sqrt(kw/tmin);

        switch (PrfMode) {
          case 1:  p1->kw = kw;
                   p1->kv = kv;
                   break;
          case 2:  p2->kv = kv;
                   p2->kw = kw;
                   p2->sv = sv;
                   p2->sw = sw;
                   p2->ts = ts;
                   break;
          case 3:  p3->ku = ku;
                   p3->kv = kv;
                   p3->kw = kw;
                   p3->kc = 0;
                   p3->su = su;
                   p3->sv = sv;
                   p3->sw = sw;
                   p3->sc = sc;
                   p3->ts = ts;
                   break;
          default:                                      eG(10);
          }
        }
  TmnDetach(Iwnd, NULL, NULL, 0, NULL);                 eG(13);
  mask = ~NMS_GROUP;
  TmnClearAll(TmMax(), mask, Iwnd, TMN_NOID);           eG(15);
  Pwnd = NULL;
  if (Ptrb) {
    TmnDetach(Itrb, NULL, NULL, 0, NULL);               eG(17);
    TmnDelete(TmMax(), Itrb, TMN_NOID);                 eG(17);
    Ptrb = NULL;
    }
  if (Qtrb) {
    TmnDetach(Jtrb, NULL, NULL, 0, NULL);               eG(17);
    TmnDelete(TmMax(), Jtrb, TMN_NOID);                 eG(17);
    Qtrb = NULL;
    }
  if (Pdff) {
    TmnDetach(Idff, NULL, NULL, 0, NULL);               eG(18);
    TmnDelete(TmMax(), Idff, TMN_NOID);                 eG(18);
    Pdff = NULL;
    }
  if (Qdff) {
    TmnDetach(Jdff, NULL, NULL, 0, NULL);               eG(18);
    TmnDelete(TmMax(), Jdff, TMN_NOID);                 eG(18);
    Qdff = NULL;
    }
  TxtClr(&header);
// time(&ct);
// printf("END: clock=%d %s\n", clock(), ctime(&ct));
  return 0;
eX_2:
  eMSG(_no_wind_field_$_, name);
eX_3:
  eMSG(_inconsistent_wind_$_, name);
eX_4:
  eMSG(_improper_wind_$_, VzMax);    //-2003-10-12
eX_115: eX_125: eX_135: eX_145: 
  eMSG(_cant_read_$_, name);
eX_116: eX_126: eX_136: eX_146: 
  eMSG(_inconsistent_field_$_, name);
eX_9:
  eMSG(_cant_create_profile_);
eX_10:
  eMSG(_invalid_mode_$_);
eX_11: eX_12:
  eMSG(_no_valdef_);
eX_13: eX_15: eX_17: eX_18:
eX_20: eX_21: eX_22:
eX_32:
eX_40:
eX_61: eX_62:
  eMSG(_internal_error_);
}

/*================================================================== CrtHeader
*/
#define CAT(a, b)  TxtPrintf(&txt, a, b), TxtCat(phdr, txt.s);
static long CrtHeader(  /* Header f¸r Modell-Feld erzeugen                  */
TXTSTR *phdr)           /* Header f¸r PRFarr                                */
  {
  dP(CrtHeader);
  char t1s[20], t2s[20], t[256];
  TXTSTR txt = { NULL, 0 };
  int k, valid;
  float *pf;
  if (phdr == NULL)
    return 0;
  TimeStr( MetT1, t1s );
  TimeStr( MetT2, t2s );
  TxtCpy(phdr, "\n");
  sprintf(t, "TALPRF_%d.%d.%s", StdVersion, StdRelease, StdPatch);
  switch (PrfMode) {
    case 1:
      CAT("artp  \"%s\"\n", "B1");     
      CAT("prgm  \"%s\"\n", t);
      CAT("form  \"%s\"\n", "Z%6.1fVx%[2]7.2fVs%7.2fKv%[2]7.1f");     
      break;
    case 2:
      CAT("artp  \"%s\"\n", "B2");
      CAT("prgm  \"%s\"\n", t);
      CAT("form  \"%s\"\n", "Z%6.1fVx%[2]7.2fVs%7.2fKv%[2]7.1fSv%[2]6.2fTs%7.3f");   
      break;
    case 3:
      CAT("artp  \"%s\"\n", "B3");
      CAT("prgm  \"%s\"\n", t);
      CAT("form  \"%s\"\n", "Z%6.1fVx%[2]7.2fVs%7.2fKv%[2]7.1fSv%[2]6.2fTs%7.3f"
        "Ku%7.1fKc%7.1fSu%6.2fSc%6.2f");
      break;
    default:                                                            eX(1);
    }
  valid = (Pbp->Class >= 0);
  CAT("valid  %d\n", valid);
  CAT("typ   %d\n", PrfMode);
  CAT("z0    %s\n", TxtForm(Pbp->RghLen, 6));
  CAT("us    %s\n", TxtForm(Pbp->us, 6));
  CAT("hm    %s\n", TxtForm(Pbp->hm, 6));
  CAT("lm    %s\n", IsUndef(&Pbp->lmo) ? "?" : TxtPrn(Pbp->lmo, "%1.0f"));
  CAT("kl    %s\n", IsUndef(&Pbp->cl) ? "?" : TxtPrn(Pbp->cl, "%1.1f"));
  CAT("prec  %2.4f\n", Pbp->Precep);                              //-2011-11-21
  CAT("sg    %s\n", TxtForm(Pbp->StatWeight, 6));
  TxtCat(phdr, "sk   ");
  pf = (float*) Pga->start;
  for (k=0; k<=Pgp->nz; k++, pf++)
    CAT(" %s", TxtForm(*pf, 6));
  TxtCat(phdr, "\n");
  if (Pgp->ntyp > FLAT1D) {
    CAT("xmin  %s\n", TxtForm(Pgp->xmin, 10));                    //-2011-05-09
    CAT("ymin  %s\n", TxtForm(Pgp->ymin, 10));                    //-2011-05-09
    CAT("delta  %s\n", TxtForm(Pgp->dd, 6));
    CAT("defmode  %d\n", DefMode);
    if (Pgp->ntyp == COMPLEX) {
      CAT("zscl  %1.1f\n", Pgp->zscl);
      CAT("sscl  %1.1f\n", Pgp->sscl);
    }
    if (*ValDef)
      CAT("vldf  \"%s\"\n", ValDef);
  }
  TxtClr(&txt);
  return 0;
eX_1:
  eMSG(_invalid_mode_$_, PrfMode);
}
#undef CAT

//==================================================================== PrfInit
//
long PrfInit(           /* initialize server    */
  long flags,           /* action flags         */
  char *istr )          /* server options       */
  {
  dP(PrfInit);
  long id, mask;
  char *jstr, *ps, t[256];
  if (StdStatus & STD_INIT)  return 0;
  if (istr) {
    jstr = istr;
    ps = strstr(istr, "-v");
    if (ps) sscanf(ps+2, "%d", &StdLogLevel);
    ps = strstr(istr, "-y");
    if (ps) sscanf(ps+2, "%d", &StdDspLevel);
    }
  else  jstr = "";
  vLOG(3)("PRF_%d.%d.%s (%08lx,%s)", StdVersion, StdRelease, StdPatch, flags, jstr);
  StdStatus |= flags;
  sprintf(t, " GRD -v%d -y%d -d%s", StdLogLevel, StdDspLevel, DefName);
  GrdInit(flags&STD_GLOBAL, t);                                         eG(12);
  sprintf(t, " BLM -v%d -y%d -d%s", StdLogLevel, StdDspLevel, DefName);
  TmnVerify(IDENT(GRDpar,0,0,0), NULL, NULL, 0);                        eG(3);
  BlmInit(flags&STD_GLOBAL, t);                                         eG(13);
  mask = ~(NMS_LEVEL | NMS_GRIDN);
  id = IDENT( LIBtab, 0, 0, 0 );
  TmnCreator( id, mask, 0, istr, LibTabServer, NULL );                  eG(2);
  id = IDENT( WNDarr, 0, 0, 0 );
  TmnCreator( id, mask, 0, istr, LibWndServer, NULL );                  eG(4);
  id = IDENT( PRFarr, 0, 0, 0 );
  TmnCreator( id, mask, 0, istr, PrfServer, NULL );                     eG(1);
  StdStatus |= STD_INIT;
  if (sfcFile)
  		fclose(sfcFile);
  sfcFile = NULL;
  sfcFirst = 1;
  return 0;
eX_1:  eX_2:  eX_3:  eX_4:  eX_12:  eX_13:
  eMSG(_not_initialized_);
  }

//=================================================================== PrfServer
//
long PrfServer(         /* server routine for PRF       */
  char *s )             /* calling option               */
  {
  dP(PrfServer);
  int r;
  long igp, iga, ibp, t1, t2, usage, valid, mode;
  ARYDSC *pa;
  char t1s[40], t2s[40], m1s[40], m2s[40], ss[256];
  TXTSTR hdr = { NULL, 0 };
  if (StdArg(s))  return 0;
  if (*s) {
    switch (s[1]) {
      case 'd': strcpy(DefName, s+2);
                break;
      case 'f': if (strstr(s+2, "NOBASE"))
                  StdStatus |= PRF_NOBASE;          //-2001-03-24 lj
                else if (strstr(s+2, "writesfc"))
                		StdStatus |= PRF_WRITESFC;
                else if (strstr(s+2, "readsfc"))
                		StdStatus |= PRF_READSFC;
                break;
      case 'i': strcpy(InpName, s+2);
                break;
      case 'p': StdStatus |= PRF_PRNDEF;
                strcpy(PrnCmd, s+2);
                LibTabServer(s);
                break;
      case 'w': GrdServer(s);
                break;
      default:  ;
      }
    return 0;
    }
  if (!StdIdent)                                                eX(99);
  r = 0;
  Iprf = StdIdent;
  GridLevel = XTR_LEVEL(StdIdent);
  GridIndex = XTR_GRIDN(StdIdent);
  if (StdStatus & STD_TIME)  t1 = StdTime;
  else  t1 = TmMin();
  t2 = t1;
  igp = IDENT(GRDpar, 0, GridLevel, GridIndex);
  pa =  TmnAttach(igp, NULL, NULL, 0, NULL);  if (!pa)          eX(1);
  Pgp = (GRDPARM*) pa->start;
  iga = IDENT(GRDarr, 0, 0, 0);
  Pga = TmnAttach(iga, NULL, NULL, 0, NULL);  if (!Pga)         eX(2);
  ibp = IDENT(BLMpar, 0, 0, 0);
  pa =  TmnAttach(ibp, &t1, &t2, 0, NULL);  if (!pa)            eX(3);
  if (t1 == t2) {
    TmnClear(t1, Iprf, 0);                                      eG(13);
    goto done;
    }
  r = 1;
  Pbp = (BLMPARM*) pa->start;
  if (*Pbp->WindLib) {
    sprintf(ss, "-l%s", Pbp->WindLib);
    LibTabServer(ss);
    if (StdStatus & PRF_PRNDEF)  LibTabServer("-p");
    LibWndServer(ss);
  }
  Iba = IDENT(BLMarr, 0, 0, 0);
  Pba = TmnAttach(Iba, &t1, &t2, 0, NULL);  if (!Pba)           eX(4);
  TmnInfo(Iba, NULL, NULL, &usage, NULL, NULL);
  valid = (0 == (usage & TMN_INVALID));                         //-2001-06-29
  MetT1 = t1;
  MetT2 = t2;
  PrfMode = Pgp->prfmode;
  DefMode = Pgp->defmode;
  switch (PrfMode) {
    case 1:   PrfRecLen = sizeof(PR1REC);
              break;
    case 2:   PrfRecLen = sizeof(PR2REC);
              break;
    case 3:   PrfRecLen = sizeof(PR3REC);
              break;
    default:                                                    eX(5);
    }
  if (TmnInfo(StdIdent, &t1, &t2, &usage, NULL, NULL)) {
    if (usage & TMN_DEFINED) {
      if (t2 != MetT1)                                          eX(30);
      Pprf = TmnAttach(StdIdent, NULL, NULL, TMN_MODIFY, NULL); eG(31);
      if (!Pprf)                                                eX(32);
      }
    else  Pprf = NULL;
    }
  *ValDef = 0;
  switch (Pgp->ntyp) {
    case FLAT1D:  Clc1dMet();                                   eG(11);
                  break;
    case FLAT3D:
    case COMPLEX: Clc3dMet();                                   eG(14);
                  break;
    default:                                                    eX(15);
    }
  CrtHeader(&hdr);                                              eG(16);
  mode = TMN_MODIFY | ((valid) ? TMN_SETVALID : TMN_INVALID);  //-2001-06-29
  TmnDetach(Iprf, &MetT1, &MetT2, mode, &hdr);                  eG(17);
  TxtClr(&hdr);
  TmnDetach(Iba, NULL, NULL, 0, NULL);                          eG(18);
  Pba = NULL;
done:
  TmnDetach( ibp, NULL, NULL, 0, NULL );                        eG(19);
  TmnDetach( iga, NULL, NULL, 0, NULL );                        eG(20);
  TmnDetach( igp, NULL, NULL, 0, NULL );                        eG(21);
  if (StdStatus & PRF_PRNDEF) {
    PrnData((*PrnCmd) ? PrnCmd : "pk");                  
  }
  vLOG(4)("PRF:PrfServer returning");
  return r;
eX_99:
  eMSG(_undefined_profile_);
eX_1:
  eMSG(_no_grid_parameter_);
eX_2:
  eMSG(_no_grid_array_);
eX_3:
  eMSG(_no_boundary_parameter_);
eX_4:
  eMSG(_no_boundary_array_);
eX_5:
  eMSG(_invalid_mode_$_, PrfMode);
eX_11: eX_14:
  eMSG(_cant_calculate_profile_);
eX_13:
  eMSG(_cant_clear_profile_);
eX_15:
  eMSG(_invalid_grid_type_$_, Pgp->ntyp);
eX_16:
  eMSG(_cant_create_header_);
eX_17: eX_18: eX_19: eX_20: eX_21:
  eMSG(_cant_detach_);
eX_30:
  strcpy(t1s, TmString(&t1));
  strcpy(t2s, TmString(&t2));
  strcpy(m1s, TmString(&MetT1));
  strcpy(m2s, TmString(&MetT2));
  eMSG(_cant_update_$_$$_$$_, NmsName(StdIdent), t1s, t2s, m1s, m2s);
eX_31: eX_32:
  eMSG(_cant_attach_$_, NmsName(StdIdent));
  }
  
  
static long PrnData(  /* Kontrollausgabe von Daten.                         */
  char *s )           /* Druck-Kommando.                                    */
  {
  dP(PRF:PrnData);
  char c, t1s[40], t2s[40];
  long iba, ipa;
  int k;
  float  *pf;
  BLMREC *pm;
  PR1REC *p1;
  PR2REC *p2;
  PR3REC *p3;
  ARYDSC *pba, *ppa;
  FILE *f;
  f = MsgFile;
  if (!f)  return 0;
  for ( ; *s != '\0'; s++)
  {
  c = *s;
  switch ( c ) {
    case 'G':
      fprintf(f, "- Vertikales Netz");
      pf = (float*) Pga->start;
      if (pf == NULL) {
        fprintf(f, "\n--- nicht definiert!\n");
        break; }
      for (k=0; k<=Pgp->nz; k++, pf++) {
        if (0 == k%10) printf("\n");
        fprintf(f, "%7.1f", *pf); }
      fprintf(f, "\n");
      break;

    case 'p':
      fprintf(f, "- Profil-Parameter\n");
      fprintf(f, ". Version=%4.1f,   Typ=%d,  Z0=%6.3f m,  D0=%4.2f m \n",
        0.1*Pbp->MetVers, PrfMode, Pbp->RghLen, Pbp->ZplDsp);
      fprintf(f, "  Ha=%5.1f m,     Ua=%7.2f m/s, Ra=%4.0f grd\n",
        Pbp->AnmHeight, Pbp->WndSpeed, Pbp->WndDir);
      fprintf(f, "  Su=%5.3f m/s,   Sv=%5.3f m/s,   Sw=%5.3f m/s\n",
        Pbp->SigmaU, Pbp->SigmaV, Pbp->SigmaW );
      fprintf(f, "  Lm=%8.1f m,  Hm=%5.0f m,     Kl=%4.1f Klug/Manier\n",
        Pbp->lmo, Pbp->hm, Pbp->cl);                            /*-20sep93-*/
      fprintf(f, "  Us=%5.3f m/s\n",
        Pbp->us );                      /*-20sep93-*/
      strcpy(t1s, TmString(&MetT1));
      strcpy(t2s, TmString(&MetT2));
      fprintf(f, "  T1=%s   T2=%s \n", t1s, t2s );
      /* PrnVar( "VARIABLE PROFILE PARAMETERS", &PrfVpp, f );  */
      break;

    case 'P':
      fprintf(f, "- Grenzschicht-Profil\n");
      iba = IDENT( BLMarr, 0, GridLevel, GridIndex );
      pba = TmnAttach( iba, NULL, NULL, 0, NULL );                      eG(4);
      fprintf(f,
"  k      Z     U      D    Su    Sv    Sw    Tu    Tv    Tw   Suw    Ths\n");
      for (k=0; k<=Pgp->nz; k++) {
        pm = (BLMREC*)  AryPtr( pba, k );
        fprintf( f,
" %2d %6.1f %5.2f %6.1f %5.2f %5.2f %5.2f %5.1f %5.1f %5.1f %5.2f %6.3f\n",
               k, pm->z, pm->u, pm->d, pm->su, pm->sv, pm->sw,
               pm->tu, pm->tv, pm->tw, pm->suw, pm->ths );
        }
      TmnDetach( iba, NULL, NULL, 0, NULL);                             eG(5);
      break;

    case 'M':  case 'm':
      fprintf(f, "- Modell-Feld\n");
      ipa = IDENT( PRFarr, 0, GridLevel, GridIndex );
      ppa = TmnAttach( ipa, NULL, NULL, 0, NULL );                      eG(6);
      if (ppa->numdm > 1) {
        fprintf(f, "Profil-Feld hat mehr als 1 Dimension!\n");
        break; }
      switch (PrfMode) {
        case 1:
          fprintf( f,
          "  k      H      Vx      Vy      Vz      Kv      Kw\n");
          for (k=0; k<=Pgp->nz; k++ ) {
            p1 = (PR1REC*) AryPtr( ppa, k );
            fprintf(f, " %2d %6.1f %7.2f %7.2f %7.2f %7.2f %7.2f\n",
                k, p1->h, p1->vx, p1->vy, p1->vz, p1->kv, p1->kw );
            }
          break;
        case 2:
          fprintf( f,
      "  k      H     Vx     Vy     Vz     Kv     Kw    Sv    Sw    Ths\n" );
          for (k=0; k<=Pgp->nz; k++) {
          p2 = (PR2REC*) AryPtr( ppa, k );
          fprintf( f,
      " %2d %6.1f %6.2f %6.2f %6.2f %6.2f %6.2f %5.2f %5.2f %6.3f\n",
            k, p2->h, p2->vx, p2->vy, p2->vz, p2->kv, p2->kw,
            p2->sv, p2->sw, p2->ts );
            }
          break;
        case 3:
          fprintf( f, "  k      H     Vx     Vy     Vz     Ku     Kv     Kw"
            "   Su   Sv   Sw     Kc   Suw    Ths\n");
          for (k=0; k<=Pgp->nz; k++) {
            p3 = (PR3REC*) AryPtr( ppa, k );
            fprintf( f,
              " %2d %6.1f %6.2f %6.2f %6.2f %6.2f %6.2f %6.2f"
              " %4.2f %4.2f %4.2f %6.2f %5.2f %6.3f\n",
              k, p3->h, p3->vx, p3->vy, p3->vz, p3->ku, p3->kv, p3->kw,
              p3->su, p3->sv, p3->sw, p3->kc, p3->sc, p3->ts );
            }
          TmnDetach(ipa, NULL, NULL, 0, NULL);                          eG(7);
          break;
        default: fprintf( f, " --- Mode %d nicht implementiert!\n", PrfMode);
        }
      break;

    case 'K':  case 'k':
      fprintf(f, "- Profil und Modellfeld\n");
      iba = IDENT(BLMarr, 0, GridLevel, GridIndex);
      pba = TmnAttach(iba, NULL, NULL, 0, NULL);
      if (!pba) {
        if (!GridLevel && !GridIndex)                                   eX(8);
        iba = IDENT(BLMarr, 0, 0, 0);
        pba = TmnAttach(iba, NULL, NULL, 0, NULL);  if (!pba)           eX(12);
        }
      ipa = IDENT(PRFarr, 0, GridLevel, GridIndex);
      ppa = TmnAttach(ipa, NULL, NULL, 0, NULL);   if (!ppa)            eX(9);
      if ((pba->numdm > 1) || (ppa->numdm > 1)) {
        fprintf(f, "--- sind dreidimensional!\n");
        TmnDetach(iba, NULL, NULL, 0, NULL);                            eG(10);
        TmnDetach(ipa, NULL, NULL, 0, NULL);                            eG(11);
        break; }
      fprintf(f,
      "!  k|      Z     U    D     Vx     Vy   Su   Sv   Sw   Tu   Tv   Tw"
      "     Kh     Kv\n" );
      fprintf(f,
      "----+--------------------------------------------------------------"
      "--------------\n" );
      for (k=0; k<=Pgp->nz; k++) {
        pm = (BLMREC*) AryPtr( pba, k );
        p1 = (PR1REC*) AryPtr( ppa, k );
        fprintf(f, "V %2d| %6.1f %5.2f %4.0f %6.2f %6.2f %4.2f %4.2f %4.2f"
              " %4.0f %4.0f %4.0f %6.2f %6.2f\n",
              k, pm->z, pm->u, pm->d, p1->vx, p1->vy, pm->su, pm->sv, pm->sw,
              pm->tu, pm->tv, pm->tw, p1->kv, p1->kw );
        }
      fprintf(f,
      "-------------------------------------------------------------------"
      "------------\n" );
      TmnDetach(iba, NULL, NULL, 0, NULL);                              eG(10);
      TmnDetach(ipa, NULL, NULL, 0, NULL);                              eG(11);
      break;

    default:
      fprintf(f, "--- Unbekannter Druck-Befehl!\n");
    }
  }
  return 0;
eX_4:  eX_5:  eX_6:  eX_7:  eX_8:  eX_9:  eX_10: eX_11: eX_12:
  return vMsg(_cant_print_);
  }


//=================================== history =================================
//
// history:
//
// 2002-06-21 lj 0.13.0 final test version
// 2002-07-03 lj 0.13.1 modifications for linux
// 2002-07-14 lj 0.13.3 selection of base fields corrected
// 2002-09-24 lj 1.0.0  final release candidate
// 2002-10-21 lj 1.0.1  check on negative zg removed in Clc3dMet()
// 2003-01-30 lj 1.0.6  buffer size for file name in Clc3dMet() increased
// 2003-06-16 lj 1.1.7  check of va in library file and of vz
// 2003-10-12 lj 1.1.12 option "-p": print library catalog
//                      scaling of Vz corrected
// 2004-05-11 lj 1.1.16 error message corrected
// 2005-02-10 lj 2.1.14 additional fields for nested grids corrected
// 2005-03-17 uj 2.2.0  version number upgrade
// 2005-05-20 uj 2.2.1  option -p to print 1D profiles
// 2005-09-07 lj 2.2.5  tolerated surface difference increased to 0.2
// 2006-01-23 uj 2.2.6  try to avoid extrapolation of library fields
// 2006-10-26 lj 2.3.0  external strings
// 2009-02-03 uj        header allocation moved
// 2011-04-06 uj 2.3.1  check in case of 1 base field
// 2011-06-29 uj 2.5.0  DMNA header, TmnAttach
// 2011-11-21 lj 2.6.0  precipitation
// 2012-01-26 uj 2.6.2  error message corrected
// 2012-10-30 uj 2.6.3  exclude log files from lib table
// 2012-10-30 uj 2.6.5  write/read superposition factors
// 2012-11-05           anemometer height hw for base field superposition
// 2014-01-24 uj 2.6.9  WLB: skip lmo for setting akl
// 2014-06-26 uj 2.6.11 eX/eG adjusted
//
//=============================================================================


