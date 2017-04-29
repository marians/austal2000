// ================================================================= TalBds.c
//
// Handling of bodies
// ==================
//
// Copyright (C) Umweltbundesamt, Dessau-Roﬂlau, Germany, 2004-2006
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

#include <math.h>
#include <ctype.h>
#include <signal.h>

#define  STDMYMAIN  BdsMain
#include "IBJmsg.h"  
#include "IBJdmn.h"
#include "IBJary.h"   
#include "IBJtxt.h"
#include "IBJstd.h"
static char *eMODn = "TalBds";

/*=========================================================================*/

STDPGM(lprbds, BdsServer, 2, 3, 0);

/*=========================================================================*/

#include "genio.h"
#include "genutl.h"

#include "TalTmn.h"
#include "TalNms.h"
#include "TalMat.h"
#include "TalBds.h"
#include "TalIo1.h"
#include "TalBds.nls"

float BdsDMKp[10] = {
  0.0,
  6.0,   // a1, Staerke der Rezirkulation
  1.0,   // a2, Gewichtung mit der Windrichtung
  0.3,   // a3, Staerke der Rezirkulation
  0.05,  // a4, Cut-Off
  0.7,   // a5, Reduktion der z-Komponente
  1.2,   // hs, Vertikale relative Ausdehung
  15.0,  // da, √ñffnungswinkel                        // 2005-02-01
  0.5,   // fs, Staerke der Zusatzfluktuationen
  0.3    // fk, Staerke der Zusatzturbulenz
};

int  BdsTrbExt = 0;

static int NumBoxDat = 7;
static float StdBoxDat[] = {
    0.0,  0.0,  0.0,  0.0,  0.0,  0.0,  0.0 };
static char *BoxNames[] = {
    "XB", "YB", "HB", "AB", "BB", "CB", "WB", NULL };
static int NumTwrDat = 6;
static float StdTwrDat[] = {
    0.0,  0.0,  0.0,  0.0,  0.0,  0.0 };
static char *TwrNames[] = {
    "XB", "YB", "HB", "DB", "CB", "QB", NULL };
static int NumPlyDat = 2;
static float StdPlyDat[] = {
    0.0,  0.0 };
static char *PlyNames[] = {
    "XB", "YB", NULL };
                                                                
// static float DatVec[MAXVAL], DatScale[MAXVAL];             -2006-06-21
// static int DatPos[MAXVAL];
static char *Bff, DefName[120] = "param.def";

static long Ibld;                 
static ARYDSC *Pbld;              
#define  BOX(a)  (*(BDSBXREC*)AryPtr(Pbld,(a)))
                                                        
static int WrtMask = 0x0200;    

static long BdsReadMatrix( char *);
static long BdsRead( char * ); 

//=============================================================== BdsReadMatrix
static long BdsReadMatrix( char *name ) {
  dP(BDS:BdsReadMatrix);
  TXTSTR syshdr = { NULL, 0 };
  TXTSTR usrhdr = { NULL, 0 };
  ARYDSC bm;
  BDSMXREC *pb;
  int *ip;
  int i, j, k, nx, ny, rc, n;
  float dd, dz, x0, y0;
  char buf[256];
  if (*name == '~') {
    strcpy(buf, StdPath);
    strcat(buf, "/");                                
    strcat(buf, name+1);
  }
  else strcpy(buf, name);
  memset(&bm, 0, sizeof(ARYDSC));
  rc = DmnRead(buf, &usrhdr, &syshdr, &bm); if (rc != 1)             eX(1);
  if (bm.numdm != 2 || bm.elmsz != sizeof(int))                      eX(2);
  if (DmnFrmTab->dt != inDAT)                                        eX(11);
  rc = DmnGetFloat(usrhdr.s, "dz", "%f", &dz, 1); 
  if (rc != 1 || dz < 1)                                             eX(3);
  rc = DmnGetFloat(usrhdr.s, "x0|xmin", "%f", &x0, 1); 
  if (rc != 1)                                                       eX(4);  
  rc = DmnGetFloat(usrhdr.s, "y0|ymin", "%f", &y0, 1);   
  if (rc != 1)                                                       eX(5);
  if (fabs(x0) > 200000 || fabs(y0) > 200000)                        eX(6);
  rc = DmnGetFloat(usrhdr.s, "dd|delta|delt", "%f", &dd, 1); 
  if (rc != 1 || dd < 1)                                             eX(7);
  nx = bm.bound[0].hgh- bm.bound[0].low + 1;
  ny = bm.bound[1].hgh- bm.bound[1].low + 1;
  n = (sizeof(BDSMXREC) + nx*ny*sizeof(int))/sizeof(BDSBXREC) + 1;
  if (!Pbld) {
    Ibld = IDENT(SLDarr, 1, 0, 0);
    Pbld = TmnCreate(Ibld, sizeof(BDSBXREC), 1, 1, n);              eG(20);
  }
  else {
    TmnResize(Ibld, n);                                             eG(21);
  }  
  pb = (BDSMXREC*)AryPtr(Pbld, 1);
  pb->btype = BDSMX;
  strncpy(pb->fullname, buf, 255);
  pb->x0 = x0;
  pb->y0 = y0;
  pb->dd = dd;
  pb->dz = dz;
  pb->nx = nx;
  pb->ny = ny;
  ip = &(pb->nzstart);
  n = 0;
  for (i=0; i<nx; i++) {
    for (j=0; j<ny; j++) {
      k = *(int *)AryPtr(&bm, i+bm.bound[0].low, j+bm.bound[1].low);
      if (k < 0 || k > 200)                                        eX(8);
      if (k > 0) n++;
      ip[i*ny+j] = k;
    }
  }
  AryFree(&bm); 
  return n;
eX_1:
  eMSG(_cant_read_raster_$_, buf);
eX_2: 
  eMSG(_invalid_raster_format_);
eX_11:
  eMSG(_invalid_raster_format_);
eX_3:
  eMSG(_cant_read_dz_);
eX_4:
  eMSG(_cant_read_x0_);
eX_5:
  eMSG(_cant_read_y0_);
eX_6:
  eMSG(_improper_x0_y0_);
eX_7:
  eMSG(_cant_read_dd_);
eX_8:
  eMSG(_invalid_value_raster_);
eX_20: eX_21:
  eMSG(_internal_error_);
}

//====================================================================== BdsRead
static long BdsRead( char *altname )         
  {
  dP(BDS:BdsRead);
  long rc, nd;
  char *hdr, name[256], bt[80];
  int i, ii, j, k, l, n, nb, nrec, irec;
  float hb, cb, a;
  float dmkp[10];
  BDSBXREC *pb;
  BDSP0REC *p0;
  BDSP1REC *p1;
  float *pdat;
  vLOG(3)("BDS: BdsRead(%s)", altname);
  vLOG(2)(_reading_bodies_);
  if (0 > OpenInput("bodies.def", altname))                         eX(1);
  Bff = ALLOC(GENTABLEN);  if (!Bff)                                eX(6);
  hdr = ALLOC(GENTABLEN);  if (!hdr)                                eX(16);
  nb = 0;
  nrec = 0;
  BdsTrbExt = 0;
  Ibld = IDENT(SLDarr, 1, 0, 0);
  Pbld = NULL;
  
read_header:
  rc = GetLine('.', Bff, GENTABLEN);                                eG(30);
  if (rc < 1) {
    if (nb == 0)                                                    eX(2);
    goto all_done;
  }

  rc = GetData("DMKp", Bff, "%[9]f", dmkp+1);                       eG(17);
  if (rc == 9) {
    for (i=1; i<=9; i++) {
      BdsDMKp[i] = dmkp[i];  //-2004-12-14
    }
  }
  for (i=1; i<=9; i++) {
    sprintf(name, "DMKp[%d]", i);
    rc = GetData(name, Bff, "%f", &a);                              eG(17);
    if (rc > 0) {
      BdsDMKp[i] = a;  //-2004-12-14
    }
  }
  i = -1;
  rc = GetData("TrbExt", Bff, "%d", &i);
  if (rc > 0) BdsTrbExt = (i) ? 1 : 0;
  rc = GetData("RFile", Bff, "%s", name);  
  if (rc > 0) {
    if (Pbld) TmnResize(Ibld, 0); 
    nb = BdsReadMatrix(name);
    goto all_done;
  }

  rc = CntLines('.', hdr, GENTABLEN);                               eG(31);
  if (rc < 1) {
    if (nb == 0)                                                    eX(20);
    goto all_done;
  }
  if (*hdr != '!')                                                  eX(21);
  n = strspn(hdr+1, "B");
  if (n < 1) {
    if (nb == 0)                                                    eX(22);
    goto all_done;
  }
  rc = GetData("BTYP|BTYPE|Bt", Bff, "%s", bt);  if (rc < 0)        eX(32);
  if (rc > 0) {
    for (i=strlen(bt)-1; i>=0; i--)  bt[i] = toupper(bt[i]);
    if      (!strcmp(bt, "BOX"))   goto is_box;
    else if (!strcmp(bt, "POLY"))  goto is_poly;
    else if (!strcmp(bt, "TOWER")) goto is_tower;
  }
  eX(7);
                            
is_box:
  irec = nrec + 1;
  nrec += n;
  if (!Pbld) {
    Ibld = IDENT(SLDarr, 1, 0, 0);
    Pbld = TmnCreate(Ibld, sizeof(BDSBXREC), 1, 1, nrec);           eG(23);
  }
  else {
    TmnResize(Ibld, nrec);                                          eG(34);
    }
  rc = GetLine('!', hdr, 200);  if (rc <= 0)                        eX(24);
  nd = EvalHeader(BoxNames, hdr, DatPos, DatScale); if (nd<1)       eX(25);
  for (i=0; i<n; i++) {
    pb = (BDSBXREC*) AryPtr(Pbld, irec+i);  if (!pb)                eX(36);
    rc = GetLine('B', Bff, GENTABLEN);  if (rc <= 0)                eX(27);
    rc = GetList(Bff, name, 63, DatVec, MAXVAL);  if (rc < 0)       eX(71);
    if (rc != nd)                                                   eX(28);
    strcpy(pb->name, name);
    pb->btype = BDSBOX;
    pb->np = nb + i + 1;
    pdat = &(pb->xb);
    for (k=0; k<NumBoxDat; k++)  pdat[k] = StdBoxDat[k];
    for (j=0; j<nd; j++) {
      k = DatPos[j];
      if (k < 0)  continue;
      if (IsUndef(&DatVec[j]))                                      eX(29);
      pdat[k] = DatVec[j]*DatScale[j];
      }
    }
  nb += n;
  goto read_header;

is_poly:
  if (n < 3)                                                        eX(37);
  irec = nrec;
  nrec += (n+9)/6;
  hb = 0;
  GetData("HB", Bff, "%f", &hb);                                    eG(38);
  if (hb < 0)                                                       eX(39);
  cb = 0;
  GetData("CB", Bff, "%f", &cb);                                    eG(40);
  if (cb <= 0)                                                      eX(41);
  if (!Pbld) {
    Ibld = IDENT(SLDarr, 1, 0, 0);
    Pbld = TmnCreate(Ibld, sizeof(BDSBXREC), 1, 1, nrec);           eG(53);
    }
  else {
    TmnResize(Ibld, nrec);                                          eG(64);
    }
  rc = GetLine('!', hdr, 200);  if (rc <= 0)                        eX(54);
  nd = EvalHeader(PlyNames, hdr, DatPos, DatScale); if (nd<1)       eX(55);
  for (i=0; i<n; i++) {    
    rc = GetLine('B', Bff, GENTABLEN);  if (rc <= 0)                eX(57);
    rc = GetList(Bff, name, 63, DatVec, MAXVAL);  if (rc < 0)       eX(72);
    if (rc != nd)                                                   eX(58);
    if (i < 2) {
      ii = i;
      if (!ii)  irec++;
      p0 = (BDSP0REC*)AryPtr(Pbld, irec);  if (!p0)                   eX(66);
      strcpy(p0->name, name);
      p0->btype = BDSPOLY0;
      p0->np = n;
      pdat = &(p0->p[ii].x);
      }
    else {
      ii = (i+4)%6;
      if (!ii)  irec++;
      p1 = (BDSP1REC*)AryPtr(Pbld, irec);  if (!p1)                   eX(67);
      strcpy(p1->name, name);
      p1->btype = BDSPOLY1;
      p1->np = ii+1;
      pdat = &(p1->p[ii].x);
      }
    for (k=0; k<NumPlyDat; k++)  pdat[k] = StdPlyDat[k];
    for (j=0; j<nd; j++) {
      k = DatPos[j];
      if (k < 0)  continue;
      if (IsUndef(&DatVec[j]))                                      eX(59);
      pdat[k] = DatVec[j]*DatScale[j];
      }
    }
  p0->hb = hb;
  p0->cb = cb;
  nb += 1;
  goto read_header;

is_tower:
  irec = nrec;
  nrec += 3*n;
  if (!Pbld) {
    Ibld = IDENT(SLDarr, 1, 0, 0);
    Pbld = TmnCreate(Ibld, sizeof(BDSBXREC), 1, 1, nrec);           eG(23);
    }
  else {
    TmnResize(Ibld, nrec);                                          eG(34);
    }
  rc = GetLine('!', hdr, 200);  if (rc <= 0)                        eX(24);
  nd = EvalHeader(TwrNames, hdr, DatPos, DatScale); if (nd<1)       eX(55); 
  for (l=0; l<n; l++) {
    BDSTWREC twrec;
    double xp[36], yp[36], phi;                                   //-2006-08-25
    memset(twrec.name, 0, sizeof(BDSTWREC));
    pdat = &(twrec.xt);
    rc = GetLine('B', Bff, GENTABLEN);  if (rc <= 0)                eX(57);
    rc = GetList(Bff, name, 63, DatVec, MAXVAL);  if (rc < 0)       eX(73);
    if (rc != nd)                                                   eX(58);
    for (k=0; k<NumTwrDat; k++)  pdat[k] = StdTwrDat[k];
    for (j=0; j<nd; j++) {
      k = DatPos[j];
      if (k < 0)  continue;
      if (IsUndef(&DatVec[j])) {                                    eX(59); }
      else  pdat[k] = DatVec[j]*DatScale[j];
    }
    for (i=0; i<12; i++) {
      phi = (i-0.5)*30;
      xp[i] = twrec.xt + 0.5*twrec.dt*cos(phi/RADIAN); //-2004-12-14
      yp[i] = twrec.yt + 0.5*twrec.dt*sin(phi/RADIAN); //-2004-12-14
      }
    for (i=0; i<12; i++) {
      if (i < 2) {
        ii = i;
        if (!ii)  irec++;
        p0 = (BDSP0REC*) AryPtr(Pbld, irec);  if (!p0)                eX(66);
        strcpy(p0->name, name);
        p0->btype = BDSPOLY0;
        p0->np = 12;
        p0->p[ii].x = xp[i];
        p0->p[ii].y = yp[i];
        if (!i)  p0->flags = twrec.flags;
        }
      else {
        ii = (i+4)%6;
        if (!ii)  irec++;
        p1 = (BDSP1REC*) AryPtr(Pbld, irec);  if (!p1)                eX(67);
        strcpy(p1->name, name);
        p1->btype = BDSPOLY1;
        p1->np = ii+1;
        p1->p[ii].x = xp[i];
        p1->p[ii].y = yp[i];
        }                                                      
      }
    p0->hb = twrec.ht;
    p0->cb = twrec.ct;
    p0->qb = twrec.qt;
    }
  nb += n;
  goto read_header;   
  
all_done:
  CloseInput();
  if (nb < 0) 
    vLOG(2)(_bodies_closed_matrix_$_, name);
  else 
    vLOG(2)(_bodies_closed_$_, nb);
  TmnDetach(Ibld, NULL, NULL, TMN_MODIFY, NULL);                    eG(26);
  Pbld = NULL;     
  FREE(Bff);
  FREE(hdr);   
  return nb;   
eX_1:
  eMSG(_bodies_not_found_);
eX_2:
  eMSG(_bodies_incomplete_);
eX_6: eX_16:
  eMSG(_cant_allocate_buffer_);
eX_7:
  eMSG(_invalid_body_type_$_, bt);
eX_17:
  eMSG(_cant_read_dmkp_);
eX_20:
  eMSG(_cant_read_boxes_);
eX_21: eX_24: eX_27: eX_54: eX_57:
  eMSG(_invalid_structure_$_, nb+1);
eX_22:
  eMSG(_empty_list_);
eX_23: eX_26: eX_53:
  eMSG(_cant_create_sldarr_);
eX_25: eX_55:
  eMSG(_cant_read_header_$_, nb+1);
eX_28: eX_58:
  eMSG(_incosistent_count_$$$_, nb+1, rc, nd);
eX_29: eX_59:
  eMSG(_undefines_value_body_$_, nb+1);
eX_30: eX_31:
  eMSG(_cant_analyse_bodies_);
eX_32:
  eMSG(_cant_read_btype_);
eX_34: eX_64:
  eMSG(_cant_resize_sldarr_);
eX_36: eX_66: eX_67:
  eMSG(_internal_error_);
eX_37:
  eMSG(_invalid_polygon_);
eX_38:
  eMSG(_cant_read_hb_);  
eX_39:
  eMSG(_invalid_hb_);
eX_40:
  eMSG(_cant_read_cb_);
eX_41:
  eMSG(_invalid_cb_);
eX_71: eX_72: eX_73:
  eMSG(_cant_read_name_);
}

//================================================================== BdsInit
long BdsInit( long flags, char *istr ) {
  dP(BDS:BdsInit);
  long mask;
  char *jstr, *ps;
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
  vLOG(3)("BDS_%d.%d.%s (%08lx,%s)", 
    StdVersion, StdRelease, StdPatch, flags, jstr);
  StdStatus |= flags;
  Ibld = IDENT(SLDarr, 1, 0, 0);
  StdStatus |= STD_INIT;
  mask = ~(NMS_GROUP | NMS_LEVEL | NMS_GRIDN);
  TmnCreator(Ibld, mask, 0, istr, BdsServer, NULL);                     eG(1);
  return 0;
eX_1:
 eMSG(_not_initialized_);
}


//=================================================================== BdsServer
long BdsServer( char *ss ) {
  dP(BDS:BdsServer);
  if (StdArg(ss))  return 0;
  if (*ss) {
    switch (ss[1]) {
      case 'd':  strcpy(DefName, ss+2);
                 break;
      case 'w':  WrtMask = 0x0300;
                 sscanf(ss+2, "%x", &WrtMask);
                 break;
      default:   ;
    }
    return 0;
  }
  BdsRead(DefName);                                                  eG(1);
  return 0;
eX_1:
  eMSG(_cant_read_bodies_);
}

//==========================================================================
//
//  history:
//
//  2004-12-14 uj 2.1.2  angles in degree
//  2005-03-17 uj 2.2.0  version number upgrade
//  2006-06-21 lj 2.2.12 static declaration of DatVec[] etc. removed
//  2006-08-25 lj 2.2.14 float -> double in vectorized loop (SSE2 in I9.1)
//  2006-10-26 lj 2.3.0  external strings
//
//==========================================================================


