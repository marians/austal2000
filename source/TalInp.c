//============================================================ TalInp.c
//
// Read Input for AUSTAL2000
// =========================
//
// All data given by TA Luft are defined in this module
// =====================================================
//
// Copyright (C) Umweltbundesamt, Dessau-Ro�lau, Germany, 2002-2012
// Copyright (C) Janicke Consulting, 88662 �berlingen, Germany, 2002-2014
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
// last change: 2014-06-26 uj
//
//==========================================================================

char *TalInpVersion = "2.6.11";
static char *eMODn = "TalInp";
static int CHECK = 0;
static int StdLogLevel = 3;                               //-2006-10-20
static int StdDspLevel = 2;                               //-2006-10-20

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <math.h>
#include <locale.h>

#ifdef __linux__
  #include <sys/types.h>
  #include <sys/wait.h>
  #include <unistd.h>
  #define  AUSTAL  "austal2000"
  #define  AUSTALN  "austal2000n"
  #define  TALDIA  "taldia"
  #define  VDISP   "vdisp"
#else
  #include <process.h>
  #define  AUSTAL  "austal2000.exe"
  #define  AUSTALN  "austal2000n.exe"
  #define  TALDIA  "taldia.exe"
  #define  VDISP   "vdisp.exe"
#endif

#include "IBJmsg.h"
#include "IBJary.h"
#include "IBJtxt.h"
#include "IBJdmn.h"
#include "IBJgk.h"
#include "IBJntr.h"
#include "IBJnls.h"                                               //-2008-12-19
#include "TalAKT.h"
#include "TalAKS.h"
#include "TalUtl.h"
#include "TalRgl.h"
#include "TalDef.h"
#include "TalZet.h"
#include "TalDMK.h"
#include "TalStt.h"
#include "TalInp.h"
#include "TalInp.nls"
#include "TalMat.h"
#include "TalCfg.h"

//////////////////////////////////////////////////////////////
// default CRC codes, status: 2014-01-21
//////////////////////////////////////////////////////////////
#define RGL_CRC_GK        "3b0d22a5"
#define RGL_CRC_UTM       "7e0adae7"
#define SETTINGS_CRC      "fdd2774f"
#define SETTINGS_A2KN_CRC "c076e87d"
//////////////////////////////////////////////////////////////

#define  vLOG(a)  MsgQuiet=2*(StdDspLevel<(a))-1, MsgVerbose=(StdLogLevel>=(a)), vMsg

#define TE(i)  ((AKTREC*)AryPtrX(&TIPary, i))->t
#define RA(i)  ((AKTREC*)AryPtrX(&TIPary, i))->iRa
#define UA(i)  ((AKTREC*)AryPtrX(&TIPary, i))->fUa
#define KL(i)  ((AKTREC*)AryPtrX(&TIPary, i))->iKl

typedef struct {
  int iLine;        // line number in input file
  char *_name;      // parameter name + buffer for tokens
  int nTokens;      // number of tokens
  char **_tokens;   // array of pointers to tokens
  int type;         // type of tokens (TIP_STRING or TIP_NUMBER)
  double *_dd;      // array of values or NULL
} TIPREC;


// these unit specifications must be applied in the settings file
#define U_GM3   "g/m3"                //-2008-07-22
#define U_UGM3  "ug/m3"               //-2008-07-22
#define U_MGM3  "mg/m3"               //-2008-07-22
#define U_GM2D  "g/(m2*d)"            //-2008-07-22
#define U_MGM2D  "mg/(m2*d)"          //-2008-07-22
#define U_UGM2D  "ug/(m2*d)"          //-2008-07-22
#define U_KGHAA  "kg/(ha*a)"          //-2008-07-22
#define U_PERC   "%"                  //-2008-07-22

double TipEpsilon = 1.e-4;
int TipMaxIter = 200;

int cSO2, cNO, cNO2, cNOX, cODOR;
int TipTrbExt = 0;

static char *VarPrm = "vq,qq,sq,hm,tq,lq,rq,ri";                                 //-2011-11-23
static double Hh[] = { 0, 3, 6, 10, 16, 25, 40, 65, 100, 150, 200, 300, 400, 500,
                        600, 700, 800, 1000, 1200, 1500 };

static double BodyDda[] = { 32, 16,  8,  4,  2 };
static double BodyDza[] = {  6,  4,  3,  3,  2 };
static double BodyDd, BodyDz, BodyHmax;
static double BodyRatio = 5;  //-2004-07
static double TurbRatio = 10; //-2004-07
static ARYDSC BM;
static double BMx0, BMy0, BMdd, BMdz;

static double DdMin = 16.0; //-2004-11-02
static double RadMin = 1000;
static double RadRatio = 50;
static int NxyMax = 300;

static int nz0TAL=9, iZ0;
static double z0TAL[9] = { 0.01, 0.02, 0.05, 0.10, 0.20, 0.50, 1.0, 1.5, 2.0 };
static double lmTAL[6][9] = {
 {    7,    9,   13,   17,   24,   40,   65,   90,  118 },
 {   25,   31,   44,   60,   83,  139,  223,  310,  406 },
 {99999,99999,99999,99999,99999,99999,99999,99999,99999 },
 {  -25,  -32,  -45,  -60,  -81, -130, -196, -260, -326 },
 {  -10,  -13,  -19,  -25,  -34,  -55,  -83, -110, -137 },
 {   -4,   -5,   -7,  -10,  -14,  -22,  -34,  -45,  -56 } }; 

static int Xflags;                                                //-2008-08-08
static TXTSTR LogCheck = { NULL, 0 };

//-----------------------------------------------------------------------------

TIPDAT TI;
ARYDSC TipVar;
ARYDSC TIPary;
FILE *TipMsgFile;
int TalMode;
static int PrmMode;
static TXTSTR TxtUser, TxtSystem;
static char Path[256], Home[256], Locale[256];
static int Write_z=0, Write_l=0;
static int nVarParm, nUsrParm;												            //-2011-12-02
static DMNFRMREC *TipFrmTab;

//================================================================= xcodeUnits

static void xcodeUnits() {                                        //-2008-07-22
  STTSPCREC *tsr;                                                 //-2011-11-23
  char *uc, *un;
  int i;
  for (i=0; i<SttSpcCount; i++) {
    tsr = SttSpcTab + i;
    if (*tsr->name == 0)
      break;
    uc = tsr->uc;
    un = tsr->un;
    if (!strcmp(uc, U_GM3)) strcpy(uc, _U_GM3_);
    else if (!strcmp(uc, U_MGM3)) strcpy(uc, _U_MGM3_);
    else if (!strcmp(uc, U_UGM3)) strcpy(uc, _U_UGM3_);
    else if (!strcmp(uc, U_PERC)) strcpy(uc, _U_PERC_);
    if (!strcmp(un, U_GM2D)) strcpy(un, _U_GM2D_);
    else if (!strcmp(un, U_MGM2D)) strcpy(un, _U_MGM2D_);
    else if (!strcmp(un, U_UGM2D)) strcpy(un, _U_UGM2D_);
    else if (!strcmp(un, U_KGHAA)) strcpy(un, _U_KGHAA_);
    else if (!strcmp(un, U_PERC)) strcpy(un, _U_PERC_);
  }
}

//================================================================= isUndefined
static int isUndefined( double d ) {
  return (d == HUGE_VAL);
}

//=============================================================== TipBlmVersion
//
float TipBlmVersion( void ) 
{
  dP(TipBlmVersion);
  float vrs = 2.6;
  char *pc_blm, *pc_noshear, *pc_spread, *pc_prfmod;
  pc_blm  = strstr(TI.os, "Blm=");                              //-2006-02-13
  pc_noshear = strstr(TI.os, "NOSHEAR");
  pc_spread = strstr(TI.os, "SPREAD");
  pc_prfmod = strstr(TI.os, "PRFMOD");                          //-2011-07-07
  if (!NOSTANDARD) {
   if (pc_blm || pc_noshear || pc_spread || pc_prfmod)                   eX(1);
  }
  else {  
    if (pc_blm && (pc_noshear || pc_spread || pc_prfmod))                eX(2);
    if (pc_spread && pc_prfmod)                                          eX(3);
    if (pc_blm) 
      sscanf(pc_blm+4, "%f", &vrs);
    else if (pc_prfmod)
      vrs = (pc_noshear) ? 4.8 : 4.6;
    else if (pc_spread)
      vrs = (pc_noshear) ? 3.8 : 3.6;
    else if (pc_noshear)
      vrs = 2.8;   
  }
  return vrs;
eX_1: 
  eMSG(_blm_nostandard_required_);
eX_2:
  eMSG(_blm_explicit_);
eX_3:
  eMSG(_blm_overdefined_);
}

//================================================================== TipZ0index
//
int TipZ0index( double z0 ) {
  int i;
  for (i=0; i<nz0TAL-1; i++) {
    if (z0 < 0.5*(z0TAL[i] + z0TAL[i+1]))  break;
  }
  return i;
}

//================================================================== TipKMclass
//
int TipKMclass( double z0, double lm ) {
  int k;
  int iz = TipZ0index(z0);
  double rlm = 1/lm;
  for (k=0; k<5; k++) {
    if (rlm > 0.5*(1/lmTAL[k][iz] + 1/lmTAL[k+1][iz]))  break;
  }
  return k+1;
}

//====================================================================== TipMOvalue
//
double TipMOvalue( double z0, int kl ) {
  int iz = TipZ0index(z0);
  int ik = kl-1;
  if (ik<0 || ik>5)  ik = 2;
  return lmTAL[ik][iz];
}

//-----------------------------------------------------------------------------------

static int check_tab( void ) {
  int i, emitted;
  STTSPCREC *p;
  for (i=0; i<SttSpcCount; i++) {
    emitted = TipSpcEmitted(i);
    p = SttSpcTab + i;
    if (TalMode & TIP_SCINOTAT) {
      p->dy = -1;
      p->dd = -1;
      p->dh = -1;
      p->dn = -1;
    }
    if (emitted) {
        if (!strcmp(p->unit, "Bq"))  TalMode |= TIP_GAMMA;
        if (p->de > 0)  TalMode |= TIP_DECAY;
        if (StdLogLevel > 3)
          fprintf(MsgFile, "%-5s: vd=%6.4f m/s, wf=%10.2e 1/s, we=%3.1f\n", p->name, p->vd, p->wf, p->we);
    }
  }
  if (StdLogLevel > 3) {
  	 char s[8];
  	 sprintf(s, " 1234u");
  		for (i=1; i<=5; i++)
  				fprintf(MsgFile, "-%c   : vd=%6.4f m/s, wf=%10.2e 1/s, we=%3.1f, vs=%6.4f m/s\n", 
  						s[i], SttVdVec[i], SttWfVec[i], SttWeVec[i], SttVsVec[i]);
    fprintf(MsgFile, "\n");
  }
  return i;
}

//============================================================== getCmpIndex
static int getCmpIndex( char *n ) {
  int i;
  for (i=0; i<SttCmpCount; i++) {
    if (!strcmp(n, SttCmpNames[i]))
      return i;
  }
  return -1;                                                      //-2011-11-23
}

//================================================================= init_index
static void init_index( void ) {
  cSO2  = getCmpIndex("so2");
  cNO2  = getCmpIndex("no2");
  cNO   = getCmpIndex("no");
  cNOX  = getCmpIndex("nox");
  cODOR = getCmpIndex("odor");
}

//================================================================= parse
static int parse( char *line, TIPREC *pp ) {
  dP(parse);
  int i, n, l;
  char *p1, c, tk[256];
  if (!pp)                                    eX(2);
  pp->_name = NULL;
  pp->nTokens = 0;
  pp->_tokens = NULL;
  pp->type = TIP_NUMBER;
  pp->_dd = NULL;
  if (line==NULL || !*line)  return 0;
  l = strlen(line);
  for (i=l-1; i>0; i--)
    if (line[i] <= ' ')  line[i] = 0;
    else  break;
  pp->_name = ALLOC(l+1);  if (!pp->_name)    eX(1);
  strcpy(pp->_name, line);
  for (p1=pp->_name; (isalnum(*p1) || *p1=='-' || *p1 == '_'); p1++); //-2008-03-10
  if (!*p1)  return 0;
  *p1++ = 0;
  if (!*pp->_name)  return 0;
  pp->_tokens = _MsgTokens(p1, " ;\t\r\n=");
  if (!pp->_tokens)  return 0;
  for (n=0; ; n++)
    if (!pp->_tokens[n])  break;
  if (n == 0)  return 0;
  pp->nTokens = n;
  for (i=0; i<n; i++) {
    c = *pp->_tokens[i];
    if (!isdigit(c) && c!='-' && c!='+' && c!='?')
      pp->type = TIP_STRING;
  }
  if (pp->type == TIP_NUMBER) {
    char *tail;
    pp->_dd = ALLOC(n*sizeof(double));
    for (i=0; i<n; i++) {
      strncpy(tk, pp->_tokens[i], 255);                         //-2003-07-07
      tk[255] = 0;
      if (tk[0] == '?') {
        pp->_dd[i] = HUGE_VAL;
      }
      else {
        for (p1=tk; (*p1); p1++)  if (*p1 == ',')  *p1 = '.';   //-2003-07-07
        pp->_dd[i] = strtod(tk, &tail);
        if (*tail) {
          FREE(pp->_dd);
          pp->type = TIP_STRING;
          break;
        }
      }
    }
  }
  if (pp->type == TIP_STRING) {
    for (i=0; i<n; i++)
      MsgUnquote(pp->_tokens[i]);                                 //-2001-09-04
  }
  return pp->type;
eX_1: eX_2:
  eMSG(_internal_error_);
}

static int TipIsConstant( TIPREC *tr ) {
  int i;
  if (!tr || tr->type==TIP_STRING || !tr->_dd)  return 1;
  for (i=0; i<tr->nTokens; i++)
    if (tr->_dd[i] == HUGE_VAL)  return 0;
  return 1;
}

static int TipMustBeConstant( char *name ) {                             //-2001-12-27
  int i;
  if (strstr(VarPrm, name))  return 0;
  for (i=0; i<SttCmpCount; i++) {
    if (!strcmp(SttCmpNames[i], name))  return 0;                 //-2002-01-06
  }
  return 1;
}

static char *missing_value( char *msg ) {
  sprintf(msg, "%s", _missing_value_);
  return msg;
}

static char *too_many_values( char *msg ) {
  sprintf(msg, "%s", _too_many_values_);
  return msg;
}

static char *size_exceeded( char *msg ) {
  sprintf(msg, "%s", _string_too_long_);
  return msg;
}

static char *unknown_name( char *msg ) {
  sprintf(msg, "%s", _unknown_name_);
  return msg;
}

static char *not_a_valid_number( char *msg ) {
  sprintf(msg, "%s", _not_a_valid_number_);
  return msg;
}

static char *must_be_constant( char *msg ) {
  sprintf(msg, "%s", _must_be_constant_);
  return msg;
}

static char *improper_count( char *msg, int n, int m ) {
  sprintf(msg, _improper_count_$$_, n, m);
  return msg;
}

static char *set_value( double *pv, TIPREC t, char *msg ) {
  dQ(set_value);
  if      (t.nTokens < 1)  missing_value(msg);
  else if (t.nTokens > 1)  too_many_values(msg);
  else if (t.type != TIP_NUMBER)  not_a_valid_number(msg);
  else *pv = t._dd[0];
  if (t._dd)  FREE(t._dd);
  return msg;
}

static char *set_integer( int *pi, TIPREC t, char *msg ) {
  dQ(set_integer);
  if      (t.nTokens < 1)  missing_value(msg);
  else if (t.nTokens > 1)  too_many_values(msg);
  else if (t.type != TIP_NUMBER)  not_a_valid_number(msg);
  else *pi = t._dd[0];
  if (t._dd) {
    FREE(t._dd);
    t._dd = NULL;
  }
  return msg;
}

static char *set_string( char *s, TIPREC t, char *msg ) {
  if      (t.nTokens < 1)  missing_value(msg);
  else if (t.nTokens > 1)  too_many_values(msg);
  else if (strlen(t._tokens[0]) > 255)  size_exceeded(msg);
  else strcpy(s, t._tokens[0]);
  return msg;
}

static char *set_vector( double **ppv, TIPREC t, int *pn, char *msg ) {
  if ((pn) && *pn==0)  *pn = t.nTokens;
  if      (t.nTokens < 1)  missing_value(msg);
  else if (t.nTokens != *pn)  improper_count(msg, *pn, t.nTokens);
  else if (t.type != TIP_NUMBER)  not_a_valid_number(msg);
  else *ppv = t._dd;
  return msg;
}

static char *set_ivector( int **ppi, TIPREC t, int *pn, char *msg ) {
  dQ(set_ivector);
  int i;
  if ((pn) && *pn==0)  *pn = t.nTokens;
  if      (t.nTokens < 1)  missing_value(msg);
  else if (t.nTokens != *pn)  improper_count(msg, *pn, t.nTokens);
  else if (t.type != TIP_NUMBER)  not_a_valid_number(msg);
  else {
    *ppi = ALLOC(t.nTokens*sizeof(int));
    for (i=0; i<t.nTokens; i++)  (*ppi)[i] = (int) t._dd[i];
  }
  if (t._dd) {
    FREE(t._dd);
    t._dd = NULL;
  }
  return msg;
}

static int checkvar( int check, double *pd, int n, char *name, int *pi ) {
  int k, j;
  TIPVAR *ptv;
  char *pc, grp[40], nm[40];
  if (!pd)
    return 0;
  for (k=0; k<n; k++) {
    if (!isUndefined(pd[k]))
      continue;
    if (check) {                                      //-2005-09-27
      (*pi)++;                                      
      continue;
    }
    ptv = AryPtrX(&TipVar, *pi);
    (*pi)++;                                          //-2007-04-23              
    strcpy(nm, name);
    sprintf(ptv->name, "%02d.%s", k+1, nm);
    if (strstr(VarPrm, nm)) {
      if      (!strcmp(nm, "sq"))  strcpy(nm, "ts");
      else if (!strcmp(nm, "rq"))  strcpy(nm, "rh");  //-2002-12-10
      else if (!strcmp(nm, "lq"))  strcpy(nm, "lw");  //-2002-12-10
      else if (!strcmp(nm, "tq"))  strcpy(nm, "tt");  //-2002-12-10
      sprintf(ptv->lasn, "%s.%02d", nm, k+1);
    }
    else {
      strcpy(grp, "gas");
      pc = strchr(nm, '-');
      if (pc) {                                       // dust component
        for (j=1; j<6; j++)                           //-2005-09-27
          if (!strcmp(pc, SttGrpXten[j])) {
            strcpy(grp, SttGroups[j]);
            break;
          }
      }
      sprintf(ptv->lasn, "Eq.%02d.%s.%s", k+1, grp, nm);
    }
    ptv->p = pd+k;
  }
  return (check != 0);                                //-2005-09-27
}

//============================================================== analyse
static char *analyse( TIPREC t ) {
  static char msg[256];
  int i;
  *msg = 0;
  if (CHECK) {
    vMsg("NAME=%s, TYPE=%d", t._name, t.type);
    for (i=0; i<t.nTokens; i++)  vMsg("%s;\\", t._tokens[i]);
    vMsg("");
  }
  if      (!strcmp(t._name, "ti"))  set_string(TI.ti, t, msg);
  else if (!strcmp(t._name, "as"))  set_string(TI.as, t, msg);
  else if (!strcmp(t._name, "az"))  set_string(TI.az, t, msg);
  else if (!strcmp(t._name, "os"))  set_string(TI.os, t, msg);
  else if (!strcmp(t._name, "gh"))  set_string(TI.gh, t, msg);
  /* 2014-01-21 removed as it may result in internal parsing inconsistencies
  else if (!strcmp(t._name, "lc")) {
    if (!*TI.lc)  set_string(TI.lc, t, msg);
  }
  */
  else if (!strcmp(t._name, "gx"))  set_value(&TI.gx, t, msg);
  else if (!strcmp(t._name, "gy"))  set_value(&TI.gy, t, msg);
  else if (!strcmp(t._name, "ux"))  set_value(&TI.ux, t, msg);
  else if (!strcmp(t._name, "uy"))  set_value(&TI.uy, t, msg);
  else if (!strcmp(t._name, "nx"))  set_ivector(&TI.nx, t, &TI.nn, msg);
  else if (!strcmp(t._name, "ny"))  set_ivector(&TI.ny, t, &TI.nn, msg);
  else if (!strcmp(t._name, "nz"))  set_ivector(&TI.nz, t, &TI.nn, msg);
  else if (!strcmp(t._name, "dd"))  set_vector(&TI.dd, t, &TI.nn, msg);
  else if (!strcmp(t._name, "x0"))  set_vector(&TI.x0, t, &TI.nn, msg);
  else if (!strcmp(t._name, "x1"))  set_vector(&TI.x1, t, &TI.nn, msg);
  else if (!strcmp(t._name, "x2"))  set_vector(&TI.x2, t, &TI.nn, msg);
  else if (!strcmp(t._name, "x3"))  set_vector(&TI.x3, t, &TI.nn, msg);
  else if (!strcmp(t._name, "y0"))  set_vector(&TI.y0, t, &TI.nn, msg);
  else if (!strcmp(t._name, "y1"))  set_vector(&TI.y1, t, &TI.nn, msg);
  else if (!strcmp(t._name, "y2"))  set_vector(&TI.y2, t, &TI.nn, msg);
  else if (!strcmp(t._name, "y3"))  set_vector(&TI.y3, t, &TI.nn, msg);
  else if (!strcmp(t._name, "z0"))  set_value(&TI.z0, t, msg);
  else if (!strcmp(t._name, "d0"))  set_value(&TI.d0, t, msg);
  else if (!strcmp(t._name, "xa"))  set_value(&TI.xa, t, msg);
  else if (!strcmp(t._name, "ya"))  set_value(&TI.ya, t, msg);
  else if (!strcmp(t._name, "ha"))  set_value(&TI.ha, t, msg);
  else if (!strcmp(t._name, "ua"))  set_value(&TI.ua, t, msg);
  else if (!strcmp(t._name, "ra"))  set_value(&TI.ra, t, msg);
  else if (!strcmp(t._name, "lm"))  set_value(&TI.lm, t, msg);
  else if (!strcmp(t._name, "hm"))  set_value(&TI.hm, t, msg);
  else if (!strcmp(t._name, "ri") && CfgWet)                      //-2014-01-21
    set_value(&TI.ri, t, msg);  
  else if (!strcmp(t._name, "ri") && (PrmMode & TIP_TALDIA))      //-2014-06-26
  		; // ignore "ri" if input file is read by TALdia
  else if (!strcmp(t._name, "ie"))  set_value(&TI.ie, t, msg);
  else if (!strcmp(t._name, "mh"))  set_value(&TI.mh, t, msg);
  else if (!strcmp(t._name, "im"))  set_integer(&TI.im, t, msg);
  else if (!strcmp(t._name, "qs"))  set_integer(&TI.qs, t, msg);
  else if (!strcmp(t._name, "qb"))  set_integer(&TI.qb, t, msg);
  else if (!strcmp(t._name, "sd"))  set_integer(&TI.sd, t, msg);
  else if (!strcmp(t._name, "hh"))  set_vector(&TI.hh, t, &TI.nhh, msg);
  else if (!strcmp(t._name, "xp"))  set_vector(&TI.xp, t, &TI.np, msg);
  else if (!strcmp(t._name, "yp"))  set_vector(&TI.yp, t, &TI.np, msg);
  else if (!strcmp(t._name, "hp"))  set_vector(&TI.hp, t, &TI.np, msg);
  else if (!strcmp(t._name, "aq"))  set_vector(&TI.aq, t, &TI.nq, msg);
  else if (!strcmp(t._name, "bq"))  set_vector(&TI.bq, t, &TI.nq, msg);
  else if (!strcmp(t._name, "cq"))  set_vector(&TI.cq, t, &TI.nq, msg);
  else if (!strcmp(t._name, "dq"))  set_vector(&TI.dq, t, &TI.nq, msg);
  else if (!strcmp(t._name, "hq"))  set_vector(&TI.hq, t, &TI.nq, msg);
  else if (!strcmp(t._name, "lq"))  set_vector(&TI.lq, t, &TI.nq, msg);
  else if (!strcmp(t._name, "qq"))  set_vector(&TI.qq, t, &TI.nq, msg);
  else if (!strcmp(t._name, "rq"))  set_vector(&TI.rq, t, &TI.nq, msg);
  else if (!strcmp(t._name, "sq"))  set_vector(&TI.sq, t, &TI.nq, msg);
  else if (!strcmp(t._name, "tq"))  set_vector(&TI.tq, t, &TI.nq, msg);
  else if (!strcmp(t._name, "vq"))  set_vector(&TI.vq, t, &TI.nq, msg);
  else if (!strcmp(t._name, "wq"))  set_vector(&TI.wq, t, &TI.nq, msg);
  else if (!strcmp(t._name, "xq"))  set_vector(&TI.xq, t, &TI.nq, msg);
  else if (!strcmp(t._name, "yq"))  set_vector(&TI.yq, t, &TI.nq, msg);
  else if (!strcmp(t._name, "xb"))  set_vector(&TI.xb, t, &TI.nb, msg);
  else if (!strcmp(t._name, "yb"))  set_vector(&TI.yb, t, &TI.nb, msg);
  else if (!strcmp(t._name, "ab"))  set_vector(&TI.ab, t, &TI.nb, msg);
  else if (!strcmp(t._name, "bb"))  set_vector(&TI.bb, t, &TI.nb, msg);
  else if (!strcmp(t._name, "cb"))  set_vector(&TI.cb, t, &TI.nb, msg);
  else if (!strcmp(t._name, "wb"))  set_vector(&TI.wb, t, &TI.nb, msg);
  else if (!strcmp(t._name, "rb"))  set_string(TI.bf, t, msg);
  else {
    for (i=0; i<SttCmpCount; i++) {
      if (!strcmp(t._name, SttCmpNames[i])) {
        set_vector(&TI.cmp[i], t, &TI.nq, msg);
        break;
      }
    }
    if (i >= SttCmpCount)  unknown_name(msg);
  }
  if (!*msg && TipMustBeConstant(t._name) && !TipIsConstant(&t)) {  //-2002-01-06
    must_be_constant(msg);
  }
  return msg;
}

//=============================================================== TipSpcIndex
int TipSpcIndex( char *cmp_name ) {
  char s[16], *pc;
  int ks;
  strcpy(s, cmp_name);
  pc = strchr(s, '-');
  if (pc)  *pc = 0;
  for (pc=s; (*pc); pc++)  *pc = tolower(*pc);
  for (ks=0; ks<SttSpcCount; ks++) {
    if (!strcmp(SttSpcTab[ks].name, s))  return ks;
  }
  return -1;
}

//============================================================= TipSpcEmitted
int TipSpcEmitted( int ks )
{
  int ic, l;
  char name[40];
  if (ks<0 || ks>=SttSpcCount)  return 0;
  strcpy(name, SttSpcTab[ks].name);
  if (!*name)  return 0;
  for (ic=0; ic<SttCmpCount; ic++) {
    if (!strcmp(SttCmpNames[ic], name) && (TI.cmp[ic]))  return 1;
  }
  strcat(name, "-");
  l = strlen(name);
  for (ic=0; ic<SttCmpCount; ic++) {
    if (!strncmp(SttCmpNames[ic], name, l) && (TI.cmp[ic]))  return 1;
  }
  return 0;
}

//=================================================================== TipRead
static int TipRead( char *path )
{
  dP(TipRead);
  FILE *f, *m;
  int n=0, scanning=0, buflen=32000;
  TIPREC t;
  char *buf, *msg, name[256];
  if (CHECK) vMsg("TipRead(%s) ...", path);
  m = (MsgFile) ? MsgFile : stdout;
  sprintf(name, "%s/%s", path, "austal2000.txt");
  buf = ALLOC(buflen);  if (!buf)        eX(3);
  f = fopen(name, "rb");
  if (!f) {
    vMsg(_cant_read_file_$_, name);
    exit(2);
  }
  fprintf(m, "%s", _start_input_);
  while (fgets(buf, buflen, f)) {
    n++;
    if (strlen(buf) >= buflen-2)            eX(1);
    if (!isalpha(*buf))  continue;
    if (0 > parse(buf, &t))                 eX(2);
    fprintf(m, "> %s\n", buf);                      //-2001-06-09
    MsgLow(t._name);
    msg = analyse(t);
    FREE(t._name);
    FREE(t._tokens);
    if (*msg) {
      vMsg(_error_input_line_$$$_, n, buf, msg);
      scanning = 1;
    }
  }
  fclose(f);
  fprintf(m, "%s", _end_input_);
  FREE(buf);
  fprintf(m, "\n");
  if (scanning) return -1;
  return n;
eX_1:
  eMSG(_buffer_overflow_$_, n);
eX_2: eX_3:
  eMSG(_internal_error_);
}

//========================================================= TipInitialize
//
static int TipInitialize( void ) {
  dP(TipInitialize);
  if (CHECK)  vMsg("TipInitialize() ...");
  TI.interval = 3600;
  TI.average = 24;
  TI.qb = 0;
  TI.gx = 0;
  TI.gy = 0;
  TI.ux = 0;
  TI.uy = 0;
  TI.dd = NULL;
  TI.x0 = NULL;
  TI.x1 = NULL;
  TI.x2 = NULL;
  TI.x3 = NULL;
  TI.y0 = NULL;
  TI.y1 = NULL;
  TI.y2 = NULL;
  TI.y3 = NULL;
  TI.z0 = HUGE_VAL;
  TI.d0 = HUGE_VAL;
  TI.aq = NULL;
  TI.bq = NULL;
  TI.cq = NULL;
  TI.dq = NULL;
  TI.hq = NULL;
  TI.qq = NULL;
  TI.sq = NULL;
  TI.vq = NULL;
  TI.wq = NULL;
  TI.ha = 0;                                  //-2002-04-16
  TI.hm = 0;                            
  TI.sc = 0;                                  //-2011-11-23
  TI.ri = -HUGE_VAL;                          //-2014-01-21
  TI.xa = HUGE_VAL;
  TI.ya = HUGE_VAL;
  TI.sd = 11111;
  TI.im = TipMaxIter;                                             //-2008-10-20
  TI.ie = TipEpsilon;                                             //-2008-10-20
  TI.mh = HUGE_VAL;
  TI.npmax = 20;                             //-2007-02-03
  //-2004-07
  TI.xb = NULL;
  TI.yb = NULL;
  TI.ab = NULL;
  TI.bb = NULL;
  TI.cb = NULL;
  TI.wb = NULL;
  TI.xbmin = NULL;
  TI.xbmax = NULL;
  TI.ybmin = NULL;
  TI.ybmax = NULL;
  TI.dmk = ALLOC(9*sizeof(double)); if (!TI.dmk) return -1;
  TI.dmk[0] = 6.0;   // strength of recirculation
  TI.dmk[1] = 1.0;   // weighting of wind direction
  TI.dmk[2] = 0.3;   // strength of recirculation
  TI.dmk[3] = 0.05;  // Cut-Off
  TI.dmk[4] = 0.7;   // reduction of z-component
  TI.dmk[5] = 1.2;   // vertical relative extension
  TI.dmk[6] = 15.0;  // opening angle
  TI.dmk[7] = 0.5;   // strength of additional circulation
  TI.dmk[8] = 0.3;   // strength of additional turbulence
  strcpy(TI.lc, "C");                                             //-2014-01-21
  /*
  if (*Locale) {
    strcpy(TI.lc, Locale);                                        //-2008-10-17
  }
  */
  xcodeUnits();                                                   //-2008-07-22
//  init_tab();
  TI.cmp = (double**)ALLOC(SttCmpCount*sizeof(double*));          //-2005-08-25
  init_index();
  return 0;
}

//============================================================= TipCorine
//
static double TipCorine( int nq, double *xq, double *yq, double *hq, double *cq )
{ dP(TipCorine);
  FILE *m;
  double z0, z0m, z0g, x, y, h;
  char *_buf=NULL; 
  char crc[16];
  int n;
  if (CHECK)  vMsg("TipCorine(%d, ...) ...", nq);
  m = (MsgFile) ? MsgFile : stdout;
  z0m = 0;
  z0g = 0;
  n = TrlReadHeader(Home, TI.ggcs);
  fprintf(m, "\n");
  sprintf(crc, "%08x", RglCRC);
  if ((strstr(RglFile, "gk") && !strcmp(crc, RGL_CRC_GK)) ||        //-2011-06-29
      (strstr(RglFile, "utm") && !strcmp(crc, RGL_CRC_UTM)))
    fprintf(m, _standard_register_$$_, RglFile, RglCRC);
  else  
    vMsg(_nonstandard_register_$$_, RglFile, RglCRC);
  vLOG(4)(_roughness2_$$_, RglGGCS, RglDelta);
  vLOG(4)(_roughness3_$$$$_, RglXmin, RglXmax, RglYmin, RglYmax);
  if (n)
    return n;
  for (n=0; n<nq; n++) {
    x = TI.gx + xq[n];
    y = TI.gy + yq[n];
    h = hq[n] + 0.5*cq[n];
    if (h < 10.) h = 10.;                                              //-2008-09-05
    z0 = TrlGetZ0(TI.ggcs, x, y, 10*h, &_buf);
    if (RglMrd > 0 && MsgFile) {
      fprintf(MsgFile, _gkconverted_$$$$$$_, RglMrd, n+1, x, y, RglX, RglY);    //-2006-11-21
    }
    if (z0 <= 0) {
      TrlReadHeader(NULL, NULL);
      return z0;
    }
    vLOG(4)(_roughness4_$$$$_, n+1, x, y, h);
    vLOG(4)(_roughness5_$$$$_, RglA, RglB, _buf, z0);
    z0m += z0*h*h;
    z0g += h*h;
  }
  TrlReadHeader(NULL, NULL);
  if (z0g > 0)  z0m /= z0g;
  return z0m;
}

//============================================================== checkSurface
//
#define  ZZ(i,j)  *(float*)AryPtrX(&dsc, i, j)

static float getMaxDif( ARYDSC dsc, int n ) {
  int i, j, i1, i2, j1, j2;
  float dz, dzmax;
  if (dsc.numdm != 2)  return -1;
  if (n < 1)  n = 1;
  i1 = dsc.bound[0].low;
  i2 = dsc.bound[0].hgh;
  j1 = dsc.bound[1].low;
  j2 = dsc.bound[1].hgh;
  dzmax = 0;
  for (i=i1; i<=i2; i++) {
    for (j=j1+n; j<=j2; j++) {
      dz = ZZ(i,j)-ZZ(i,j-n);
      dz = (dz < 0) ? -dz : dz;
      if (dz > dzmax)  dzmax = dz;
    }
  }
  for (j=j1; j<=j2; j++) {
    for (i=i1+n; i<=i2; i++) {
      dz = ZZ(i,j)-ZZ(i-n,j);
      dz = (dz < 0) ? -dz : dz;
      if (dz > dzmax)  dzmax = dz;
    }
  }
  return dzmax;
}

static float getMeanHeight( ARYDSC dsc ) {
  int i, j, i1, i2, j1, j2;
  float h;
  if (dsc.numdm != 2)  return -9999;
  i1 = dsc.bound[0].low;
  i2 = dsc.bound[0].hgh;
  j1 = dsc.bound[1].low;
  j2 = dsc.bound[1].hgh;
  h = 0;
  for (i=i1; i<=i2; i++)
    for (j=j1; j<=j2; j++)
      h += ZZ(i,j);
  h /= (i2-i1+1)*(j2-j1+1);
  return h;
}

static int checkSurface( int n ) {
  dP(checkSurface);
  char fn[256], gn[256];
  char *ggcs=NULL;                                                //-2008-12-11
  TXTSTR usr = { NULL, 0 };
  TXTSTR sys = { NULL, 0 };
  int nx, ny;
  double x0, y0, dd, d1, d2, gx, gy;
  ARYDSC dsc;
  sprintf(fn, "%s/zg%02d.dmna", Path, n+(TI.nn>1));
  if (!TutFileExists(fn)) {
    if (*TI.gh == '*')                                      eX(10);
    if (TutMakeName(fn, Path, TI.gh) < 0)                   eX(10);  //-2001-11-23
    if (!TutFileExists(fn))                                 eX(10);
    return 0;
  }
  sprintf(gn, "%s/zg%02d", Path, n+(TI.nn>1));
  memset(&dsc, 0, sizeof(ARYDSC));
  DmnRead(gn, &usr, &sys, &dsc);                            eG(1);
  if (dsc.numdm != 2)                                       eX(2);
  if (dsc.bound[0].low!=0 || dsc.bound[1].low!=0)           eX(3);
  nx = dsc.bound[0].hgh;
  ny = dsc.bound[1].hgh;
  if (1 > DmnGetDouble(usr.s, "xmin|x0", "%lf", &x0, 1))        eX(4);
  if (1 > DmnGetDouble(usr.s, "ymin|y0", "%lf", &y0, 1))        eX(5);
  if (1 > DmnGetDouble(usr.s, "dd|delt|delta", "%lf", &dd, 1))  eX(6);
  if (1 > DmnGetDouble(usr.s, "gakrx|refx", "%lf", &gx, 1))  gx = 0;  //-2008-12-11
  if (1 > DmnGetDouble(usr.s, "gakry|refy", "%lf", &gy, 1))  gy = 0;  //-2008-12-11
  if (1 > DmnGetString(usr.s, "ggcs", &ggcs, 0))  ggcs = NULL;        //-2008-12-11
  if (TI.gx != gx)                                              eX(16);
  if (TI.gy != gy)                                              eX(17);
  if (ggcs && strcmp(TI.ggcs, ggcs))                            eX(18); //-2008-12-11
  if (TI.nx[n] != nx)                                           eX(11);
  if (TI.ny[n] != ny)                                           eX(12);
  if (TI.dd[n] != dd)                                           eX(13);
  if (TI.x0[n] != x0)                                           eX(14);
  if (TI.y0[n] != y0)                                           eX(15);
  if (isUndefined(TI.mh))  TI.mh = getMeanHeight(dsc);
  d1 = getMaxDif(dsc, 1)/dd;
  d2 = getMaxDif(dsc, 2)/(2*dd);
  if (MsgFile) {
    fprintf(MsgFile, "%s", _maximum_steepness_);
    if (TI.nn > 1)  fprintf(MsgFile, _within_grid_$_, n+1);
    fprintf(MsgFile, _steepness_is_$$_, d1, d2);
  }
  AryFree(&dsc);
  return 1;
eX_10:
  eMSG(_no_surface_file_$_, fn);           //-2003-06-24
eX_1:
  eMSG(_cant_read_file_$_, fn);
eX_2: eX_3:
  eMSG(_improper_file_structure_$_, fn);
eX_4: eX_5: eX_6:
  eMSG(_missing_grid_parameters_$_, fn);
eX_11: eX_12: eX_13: eX_14: eX_15: eX_16: eX_17: eX_18:           //-2008-12-11
  eMSG(_inconsistent_grid_parameters_$_, fn);
}
#undef ZZ


//-------------------------------------------------------- checkBodiesAndSources
static int checkBodiesAndSources(void) {
  dP(checkBodiesAndSources);
  int i, j, k, n, nq, rc=0;
  float x, y, h, r, d, xb, yb, rb, hb, hd, co, si;
  if (!TI.nb) return 0;
  for (nq=0; nq<TI.nq; nq++) {
    x = TI.xm[nq];
    y = TI.ym[nq];
    r = 0.5*sqrt(TI.aq[nq]*TI.aq[nq] + TI.bq[nq]*TI.bq[nq]);
    h = TI.hq[nq] + 0.5*TI.cq[nq];
    hd = h;                //-2005-01-10
    if (hd < 10) hd = 10;  //-2005-01-10
    if (TI.nb > 0) {
      for (n=0; n<TI.nb; n++) {
        co = cos(TI.wb[n]/RADIAN);  //-2004-12-14
        si = sin(TI.wb[n]/RADIAN);  //-2004-12-14
        xb = TI.xb[n] + 0.5*(TI.ab[n]*co - TI.bb[n]*si);
        yb = TI.yb[n] + 0.5*(TI.bb[n]*co + TI.ab[n]*si);
        rb = 0.5*sqrt(TI.ab[n]*TI.ab[n] + TI.bb[n]*TI.bb[n]);
        hb = TI.cb[n];
        d = sqrt((xb-x)*(xb-x) + (yb-y)*(yb-y));
        if (d-r-rb < 6*hd) {
          if (h < 1.2*hb) {
            if (rc == 0)
              vMsg(_low_source_$$_, nq+1, n+1);
            rc--;
          }
        }
      }
    }
    else if (BM.start) {
      rb = 0.5*sqrt(2.)*BMdd;
      for (i=BM.bound[0].low; i<=BM.bound[0].hgh; i++) {
        for (j=BM.bound[1].low; j<=BM.bound[1].hgh; j++) {
          k = *(int *)AryPtr(&BM, i, j);
          if (k == 0) continue;
          xb = BMx0 + (i-BM.bound[0].low+0.5)*BMdd;
          yb = BMy0 + (j-BM.bound[1].low+0.5)*BMdd;
          hb = k * BMdz;
          d = sqrt((xb-x)*(xb-x) + (yb-y)*(yb-y));
          if (d-r-rb < 6*hd) {
            if (h < 1.2*hb) {
              if (rc == 0)
                vMsg(_low_source_$$$$_, nq+1, i, j);
              rc--;
            }
          }
        }
      }
    }
  }
  if (rc < 0) {
    if (rc < -1)
      vMsg(_additional_cases_$_, -rc-1);
    vMsg("");
  }
  return rc;
}

//============================================================== makeSurface
//
static int makeSurface( int n ) {
  dP(makeSurface);
  char fn[256];
  NTRREC nrec, irec;
  double rx0, rx3, ry0, ry3, gx0, gx3, gy0, gy3;
  float d1, d2;
  int meridr, meridg;
  char locale[256]="C";
  if (TutMakeName(fn, Path, TI.gh) < 0)           eX(10);     //-2001-11-23
  if (NtrReadFile(fn) < 0)                        eX(1);      //-2001-10-01
  irec = NtrGetInRec();
  meridr = (TI.gx+TI.x0[n])/1000000;
  meridg = (irec.gkx+irec.xmin)/1000000;
  if (meridr != meridg)                           eX(3);
  rx0 = TI.gx + TI.x0[n];
  rx3 = TI.gx + TI.x3[n];
  ry0 = TI.gy + TI.y0[n];
  ry3 = TI.gy + TI.y3[n];
  gx0 = irec.gkx + irec.xmin;
  gx3 = irec.gkx + irec.xmax;
  gy0 = irec.gky + irec.ymin;
  gy3 = irec.gky + irec.ymax;
  if (rx0<gx0 || rx3>gx3 || ry0<gy0 || ry3>gy3)   eX(4);
  nrec.nx = TI.nx[n]+1;
  nrec.ny = TI.ny[n]+1;
  nrec.xmin = TI.x0[n];
  nrec.ymin = TI.y0[n];
  nrec.delta = TI.dd[n];
  nrec.gkx = TI.gx;
  nrec.gky = TI.gy;
  strcpy(nrec.ggcs, TI.ggcs);                                     //-2008-12-11
  NtrSetOutRec(nrec);
  sprintf(fn, "%s/zg%02d", Path, n+(TI.nn>1));
  //
  if (*TI.lc) {                                               //-2003-07-07
    strcpy(locale, setlocale(LC_NUMERIC, NULL));
    setlocale(LC_NUMERIC, TI.lc);
  }
  NtrWriteFile(fn);                               eG(2);
  if (*TI.lc)  setlocale(LC_NUMERIC, locale);                 //-2003-07-07
  //
  if (isUndefined(TI.mh))  TI.mh = getMeanHeight(NtrDsc);
  d1 = getMaxDif(NtrDsc, 1)/TI.dd[n];
  d2 = getMaxDif(NtrDsc, 2)/(2*TI.dd[n]);
  if (MsgFile) {
    fprintf(MsgFile, "%s", _maximum_steepness_);
    if (TI.nn > 1)  fprintf(MsgFile, _within_grid_$_, n+1);
    fprintf(MsgFile, _steepness_is_$$_, d1, d2);
  }
  AryFree(&NtrDsc);
  return 0;
eX_10: eX_1:
  eMSG(_cant_read_surface_file_$_, fn);
eX_2:
  eMSG(_cant_write_surface_profile_$_, fn);
eX_3:
  eMSG(_inconsistent_systems_);
eX_4:
  if (MsgFile) {
    fprintf(MsgFile, _computational_area_$$$$_, rx0, rx3, ry0, ry3);
    fprintf(MsgFile, _surface_area_$$$$_, gx0, gx3, gy0, gy3);
  }
  eMSG(_not_inside_surface_);
}

//============================================================== getNesting
//
static int getNesting( int nq, double *xm, double *ym, double *hq,
                       double *aq, double *bq, double *cq,
                       int nb, double *xb0, double *xb3, double *yb0,
                       double *yb3, double ddb, double gd[10][5] ) {
  int i, j, n, k, kstart, gdset;
  double d, dmin, xmin, xmax, ymin, ymax, x0, x3, y0, y3, h;
  double a, b, r, x, y;
  double rfac = 20;
  double ddq = gd[0][0];
  if (nq < 1)  return -1;
  xmin = gd[0][1];
  xmax = gd[0][2];
  ymin = gd[0][3];
  ymax = gd[0][4];
  dmin = (nb) ? ddb : ddq;
  if (dmin <= 0)  return -2;
  xmin /= dmin;
  xmax /= dmin;
  ymin /= dmin;
  ymax /= dmin;
  d = 1;
  kstart = 0;
  gdset = -1;
  if (nb) {
    for (kstart=0; kstart<=1; kstart++) {
      gd[kstart][0] = d;
      d *= 2;
      gd[kstart][1] = d*floor((xb0[kstart]/dmin)/d);
      gd[kstart][2] = d*ceil((xb3[kstart]/dmin)/d);
      gd[kstart][3] = d*floor((yb0[kstart]/dmin)/d);
      gd[kstart][4] = d*ceil((yb3[kstart]/dmin)/d);
      gdset = kstart;
      if (d*dmin > ddq) {
        d /= 2;
        rfac *= ddq/(d*dmin);
        break;
      }
    }
  }
  for (k=kstart; k<10; k++) {
    x0 = x3 = xm[0]/dmin;
    y0 = y3 = ym[0]/dmin;
    for (n=0; n<nq; n++) {
      x = xm[n]/dmin;
      y = ym[n]/dmin;
      a = aq[n]/dmin;
      b = bq[n]/dmin;
      r = 0.5*sqrt(a*a+b*b);
      h = (hq[n] + 0.5*cq[n])/dmin;
      if (d <= 0.5*h)  continue;
      a = rfac*d + r;
      if (x-a < x0)  x0 = x-a;
      if (x+a > x3)  x3 = x+a;
      if (y-a < y0)  y0 = y-a;
      if (y+a > y3)  y3 = y+a;
    }
    if (k <= gdset) {                                        //-2004-11-10
      if (gd[k][1] < x0) x0 = gd[k][1];
      if (gd[k][2] > x3) x3 = gd[k][2];
      if (gd[k][3] < y0) y0 = gd[k][3];
      if (gd[k][4] > y3) y3 = gd[k][4];
    }
    /*
    if (k > 0) {                                             //-2004-11-12
      if (gd[k-1][1] < x0) x0 = gd[k-1][1];
      if (gd[k-1][2] > x3) x3 = gd[k-1][2];
      if (gd[k-1][3] < y0) y0 = gd[k-1][3];
      if (gd[k-1][4] > y3) y3 = gd[k-1][4];
    }
    */
    gd[k][0] = d;
    d *= 2;
    x0 = d*floor(x0/d);
    x3 = d*ceil(x3/d);
    y0 = d*floor(y0/d);
    y3 = d*ceil(y3/d);
    if (x0 <= xmin+d)  x0 = d*floor(xmin/d);
    if (x3 >= xmax-d)  x3 = d*ceil(xmax/d);
    if (y0 <= ymin+d)  y0 = d*floor(ymin/d);
    if (y3 >= ymax-d)  y3 = d*ceil(ymax/d);
    if (k > 0) {                                           //-2004-11-30
      if (gd[k-1][1] < x0) x0 = d*floor(gd[k-1][1]/d);
      if (gd[k-1][2] > x3) x3 = d*ceil(gd[k-1][2]/d);
      if (gd[k-1][3] < y0) y0 = d*floor(gd[k-1][3]/d);
      if (gd[k-1][4] > y3) y3 = d*ceil(gd[k-1][4]/d);
    }
    gd[k][1] = x0;
    gd[k][2] = x3;
    gd[k][3] = y0;
    gd[k][4] = y3;
    if (x0<=xmin && x3>=xmax && y0<=ymin && y3>=ymax)  break;
  }
  if (k <= 9)  k++;
  for (i=0; i<k; i++)
    for (j=0; j<5; j++)
      gd[i][j] *= dmin;
  //
  // add guard cells for inner grids
  //
  for (i=0; i<k-1; i++) {                                         //-2004-10-25
    d = gd[i][0];                                                 //-2004-10-25
    gd[i][1] -= 2*d;
    gd[i][2] += 2*d;
    gd[i][3] -= 2*d;
    gd[i][4] += 2*d;
  }
  //
  // check and possibly adjust or remove outer grid
  //
  d = 0.01;
  n = 0;
  if (k > 1) {                                                    //-2004-10-25
    if (gd[k-1][1] + d >= gd[k-2][1]) {
      gd[k-1][1] = gd[k-2][1];
      n++;
    }
    if (gd[k-1][2] - d <= gd[k-2][2]) {
      gd[k-1][2] = gd[k-2][2];
      n++;
    }
    if (gd[k-1][3] + d >= gd[k-2][3]) {
      gd[k-1][3] = gd[k-2][3];
      n++;
    }
    if (gd[k-1][4] - d <= gd[k-2][4]) {
      gd[k-1][4] = gd[k-2][4];
      n++;
    }
    if (n == 4)  k--;
  }
  return k;
}

//============================================================= checkGrid
//
static int overlap( int i, int j ) {
  double xi1, xi2, yi1, yi2;
  double xj1, xj2, yj1, yj2;
  xi1 = TI.x1[i];
  xi2 = TI.x2[i];
  yi1 = TI.y1[i];
  yi2 = TI.y2[i];
  xj1 = TI.x1[j];
  xj2 = TI.x2[j];
  yj1 = TI.y1[j];
  yj2 = TI.y2[j];
  if (xi2 <= xj1)  return 0;
  if (xi1 >= xj2)  return 0;
  if (yi2 <= yj1)  return 0;
  if (yi1 >= yj2)  return 0;
  return 1;
}

static int checkGrid( void ) {
  dP(checkGrid);
  int n, nn, m, nx, ny, i, j;
  double ddmin, ddmax, dd, x, y;
  nn = TI.nn;
  if (nn < 1)                                 eX(1);
  if (!TI.dd)                                 eX(2);
  TI.gl = ALLOC(nn*sizeof(int));  if (!TI.gl) eX(42);
  TI.gi = ALLOC(nn*sizeof(int));  if (!TI.gi) eX(43);
  //
  // check spacing: smallest mesh width first
  //
  ddmax = ddmin = TI.dd[0];                   //-2002-03-26
  for (n=0; n<nn; n++) {
    dd = TI.dd[n];
    if (dd == ddmax)  continue;
    if (dd < ddmax)                           eX(3);
    if (dd != 2*ddmax)                        eX(4);
    ddmax = dd;
  }
  dd = ddmax;
  i = (nn > 1);
  if (i && TI.dd[nn-2]==ddmax)                eX(44); //-2002-09-21
  m = i;
  for (n=nn-1; n>=0; n--) {
    if (TI.dd[n] < dd) {
      m++;
      i = 1;
      dd = TI.dd[n];
    }
    TI.gl[n] = m;
    TI.gi[n] = i++;
  }
  if (!TI.x0)                                 eX(5);
  if (!TI.y0)                                 eX(6);
  //
  // check alignment: lower left border
  //
  for (n=0; n<nn-1; n++) {                          //-2001-11-03
    dd = 2*TI.dd[n];
    for (m=n+1; m<nn; m++) {
      if (TI.dd[m] > dd)  break;
      if (fmod(TI.x0[n]-TI.x0[m], TI.dd[m]) != 0)   eX(7);
      if (fmod(TI.y0[n]-TI.y0[m], TI.dd[m]) != 0)   eX(8);
    }
  }
  //
  // check alignment: right border
  //
  if (TI.nx) {
    for (n=0; n<nn; n++) {
      if (TI.nx[n] < 1)                               eX(10);
      dd = TI.dd[n];
      if (dd<ddmax && (TI.nx[n]%2 != 0))              eX(11);
    }
  }
  if (TI.x3) {
    for (n=0; n<nn; n++) {
      dd = TI.dd[n];
      if (TI.nx) {
        if (TI.x3[n] != TI.x0[n]+TI.nx[n]*dd)         eX(12);
      }
      else {
        if (fmod(TI.x3[n], dd) != 0)                  eX(13); //-2001-11-03
        for (m=n+1; m<nn; m++) {                              //-2001-11-03
          if (TI.dd[m] > 2*dd)  break;
          if (fmod(TI.x3[n]-TI.x0[m], TI.dd[m]) != 0) eX(14);
        }
      }
    }
  }
  else {  // set right border
    if (!TI.nx)                                       eX(15);
    TI.x3 = ALLOC(nn*sizeof(double)); if (!TI.x3)     eX(16);
    for (n=0; n<nn; n++)
      TI.x3[n] = TI.x0[n] + TI.nx[n]*TI.dd[n];
  }
  if (!TI.nx) { // set number of x-intervals
    TI.nx = ALLOC(nn*sizeof(int));  if (!TI.nx)       eX(17);
    for (n=0; n<nn; n++)
      TI.nx[n] = (int)((TI.x3[n]-TI.x0[n])/TI.dd[n]+0.5);
  }
  // check number of x-intervals
  for (n=0; n<nn; n++)  if (TI.nx[n] > NxyMax)        eX(51);
  //
  if (TI.x1) {  // check inner left border
    for (n=0; n<nn; n++)
      if (fmod(TI.x1[n]-TI.x0[n], TI.dd[n]) != 0)     eX(18);   //-2005-05-12
  }
  else {        // set inner left border
    TI.x1 = ALLOC(nn*sizeof(double));  if (!TI.x1)    eX(19);
    for (n=0; n<nn; n++)  TI.x1[n] = TI.x0[n];
  }
  if (TI.x2) {  // check inner right border
    for (n=0; n<nn; n++)
      if (fmod(TI.x3[n]-TI.x2[n], TI.dd[n]) != 0)     eX(20);   //-2005-05-12
  }
  else {        // set inner right border
    TI.x2 = ALLOC(nn*sizeof(double));  if (!TI.x2)    eX(21);
    for (n=0; n<nn; n++)  TI.x2[n] = TI.x3[n];
  }
  //
  // check alignment: upper border
  //
  if (TI.ny) {
    for (n=0; n<nn; n++) {
      if (TI.ny[n] < 1)                               eX(30);
      dd = TI.dd[n];
      if (dd<ddmax && (TI.ny[n]%2 != 0))              eX(31);
    }
  }
  if (TI.y3) {
    for (n=0; n<nn; n++) {
      dd = TI.dd[n];
      if (TI.ny) {
        if (TI.y3[n] != TI.y0[n]+TI.ny[n]*dd)         eX(32);
      }
      else {
        if (fmod(TI.y3[n], dd) != 0)                  eX(33); //-2001-11-03
        for (m=n+1; m<nn; m++) {                              //-2001-11-03
          if (TI.dd[m] > 2*dd)  break;
          if (fmod(TI.y3[n]-TI.y0[m], TI.dd[m]) != 0) eX(34);
        }
      }
    }
  }
  else {      // set upper border
    if (!TI.ny)                                       eX(35);
    TI.y3 = ALLOC(nn*sizeof(double)); if (!TI.y3)     eX(36);
    for (n=0; n<nn; n++)
      TI.y3[n] = TI.y0[n] + TI.ny[n]*TI.dd[n];
  }
  if (!TI.ny) { // set number of y-intervals
    TI.ny = ALLOC(nn*sizeof(int));  if (!TI.ny)       eX(37);
    for (n=0; n<nn; n++)
      TI.ny[n] = (int)((TI.y3[n]-TI.y0[n])/TI.dd[n]+0.5);
  }
  // check number of y-intervals
  for (n=0; n<nn; n++)  if (TI.ny[n] > NxyMax)        eX(52);
  //
  if (TI.y1) {  // check lower inner border
    for (n=0; n<nn; n++)
      if (fmod(TI.y1[n]-TI.y0[n], TI.dd[n]) != 0)     eX(38);   //-2005-05-12
  }
  else {        // set lower inner border
    TI.y1 = ALLOC(nn*sizeof(double));  if (!TI.y1)    eX(39);
    for (n=0; n<nn; n++)  TI.y1[n] = TI.y0[n];
  }
  if (TI.y2) {  // check upper inner border
    for (n=0; n<nn; n++)
      if (fmod(TI.y3[n]-TI.y2[n], TI.dd[n]) != 0)     eX(40);   //-2005-05-12
  }
  else {        // set upper inner border
    TI.y2 = ALLOC(nn*sizeof(double));  if (!TI.y2)    eX(41);
    for (n=0; n<nn; n++)  TI.y2[n] = TI.y3[n];
  }
  //
  // check overlap of inner regions
  //
  for (n=0; n<nn; n++) {
    dd = TI.dd[n];
    for (m=n+1; m<nn; n++) {
      if (dd < TI.dd[m])  break;
      if (overlap(n, m)) {
        vMsg(_no_overlap_$$_, n+1, m+1);
      }
    }
  }
  //
  // check inclusion of finer grids
  //
  for (n=0; n<nn; n++) {
    dd = TI.dd[n];
    if (dd == ddmax)  break;
    nx = TI.nx[n];
    ny = TI.ny[n];
    for (i=0; i<nx; i++) {
      x = TI.x0[n] + (i+0.5)*dd;
      for (j=0; j<ny; j++) {
        y = TI.y0[n] + (j+0.5)*dd;
        for (m=n+1; m<nn; m++) {
          if (TI.dd[m] == dd)  continue;
          if (TI.dd[m] > 2*dd)                                      eX(50);
          if (x>TI.x0[m] && x<TI.x3[m] && y>TI.y0[m] && y<TI.y3[m])  break;
        }
        if (m >= nn)                                                eX(50); //-2003-10-02
      }
    }
  }
  return 0;
  //
eX_16: eX_17: eX_19: eX_21: eX_36: eX_37: eX_39: eX_41: eX_42: eX_43:
  eMSG(_internal_error_);
eX_1: eX_2:
  eMSG(_no_grid_);
eX_3:
  eMSG(_increasing_mesh_width_required_);
eX_4:
  eMSG(_factor_2_required_);
eX_5:
  eMSG(_no_western_border_);
eX_6:
  eMSG(_no_southern_border_);
eX_7:
  eMSG(_improper_western_border_$_, n+1);
eX_8:
  eMSG(_improper_southern_border_$_, n+1);
eX_10:
  eMSG(_invalid_nx_$_, n+1);
eX_11:
  eMSG(_even_nx_required_$_, n+1);
eX_12: eX_13:
  eMSG(_improper_eastern_border_$_, n+1);
eX_14:
  eMSG(_invalid_eastern_border_$_, n+1);
eX_15:
  eMSG(_missing_nx_);
eX_18:
  eMSG(_improper_x1_$_, n+1);
eX_20:
  eMSG(_improper_x2_$_, n+1);
eX_30:
  eMSG(_invalid_ny_$_, n+1);
eX_31:
  eMSG(_even_ny_required_$_, n+1);
eX_32: eX_33:
  eMSG(_improper_northern_border_$_, n+1);
eX_34:
  eMSG(_invalid_northern_border_$_, n+1);
eX_35:
  eMSG(_missing_ny_);
eX_38:
  eMSG(_improper_y1_$_, n+1);
eX_40:
  eMSG(_improper_y2_$_, n+1);
eX_44:
  eMSG(_only_one_coarse_grid_);
eX_50:
  eMSG(_$_not_within_coarse_grid_, n+1);
eX_51:
  eMSG(_nx_too_large_$$_, n+1, NxyMax);
eX_52:
  eMSG(_ny_too_large_$$_, n+1, NxyMax);
}

//============================================================== TipCheck
//
static TIPVAR *get_varptr( char *name ) {                         //-2005-09-23
  TIPVAR *ptv = NULL;
  int i;
  if (!TipVar.start)
    return NULL;
  for (i=0; i<nVarParm; i++) {
    ptv = AryPtrX(&TipVar, i);
    if (!ptv)
      return NULL;
    if (!strcmp(name, ptv->name))
      break;
    ptv = NULL;                                                   //-2006-12-13
  }
  return ptv;
}

int TipLogCheck(char *id, int sum) {
		if (id != NULL) {
				TXTSTR st = { NULL, 0 };
				TxtPrintf(&st, "%s %-8s %08x\n", _checksum_, id, sum);
				TxtCat(&LogCheck, st.s);
		}
		return 0;
}

static int TipCheck( char *path, int write_z ) {
  dP(TipCheck);
  char name[256], s[1024], elnm[40];
  char locale[256]="C";
  int i, j, i1, i2, k, n, l, emission, d=0, kpmax, rc, check;		//-2011-11-23
  int make_surface=0, check_surface=0, series_exists=0;
  int make_grid=0, make_nz=0, round_z0=1;
  double x, y, z, xmin, ymin, xmax, ymax, z0, h, dz, r, co, si;
  double f1, f2, f3, f4, f5, f6, f7, f8, f9;
  double dd, dh, gd[10][5];
  double hh[100];
  DMNFRMREC *pfr;
  TIPVAR *ptv;
  TXTSTR syshdr = { NULL, 0 };
  TXTSTR usrhdr = { NULL, 0 };
  if (CHECK) vMsg("TipCheck(%s, %d) ...", path, write_z);
  //
  // check values
  //
  strcpy(locale, setlocale(LC_NUMERIC, NULL));                  //-2008-10-17
  if (*TI.os) {                                                 //-2004-07-09
    char *ps;
    for (ps=TI.os; (*ps); ps++)  if (*ps == ',')  *ps = '.';    //-2003-07-07
    if (strstr(TI.os, "SCINOTAT")) {                            //-2003-02-21
      TalMode |= TIP_SCINOTAT;
    }
    if (strstr(TI.os, "NOSTANDARD") || strstr(TI.os, "NONSTANDARD"))  //-2008-08-27
      TalMode |= TIP_NOSTANDARD;
  }
  check_tab();                                                   //-2005-08-25
  if (!*TI.lc)  strcpy(TI.lc, "C");                             //-2003-07-07
  else if (strcmp(TI.lc, "german") && strcmp(TI.lc, "C"))       eX(242);
  MsgSetLocale(TI.lc);                                            //-2008-10-17
  if (NOSTANDARD) {
    char *pc;
    int kref, kmax;
    int d;
    vMsg(_nostandard_);
    setlocale(LC_NUMERIC, "C");                                   //-2008-10-17
    if (strstr(TI.os, "LIB2"))   TalMode |= TIP_LIB2;
    else if (strstr(TI.os, "LIB36"))  TalMode |= TIP_LIB36;
    pc = strstr(TI.os, "Kref=");
    if (pc) {
      kref = 1;
      sscanf(pc+5, "%d", &kref);
      if (kref < 0)  kref = 1;
      if (TI.kp < kref)  TI.kp = kref;
    }
    pc = strstr(TI.os, "Kmax=");
    if (pc) {
      kmax = 1;
      sscanf(pc+5, "%d", &kmax);
      if (kmax <= 0)  kmax = 1;
      if (TI.kp < kmax)  TI.kp = kmax;
    }
    if (CfgWet) {                                                 //-2014-01-21
      if (strstr(TI.os, "NODILUTE")) {                            //-2011-12-01
        TalMode |= TIP_NODILUTE;
      }
    }
    if (strstr(TI.os, "SORRELAX")) { 
    		TalMode |= TIP_SORRELAX;                                  //-2006-02-06
    }
    pc = strstr(TI.os, "BS=");                                  //-2004-06-10
    if (pc)  sscanf(pc+3, "%lf", &SttOdorThreshold);
    pc = strstr(TI.os, "Interval=");
    if (pc) {                                                   //-2007-02-03
      d = TI.interval;
      sscanf(pc+9, "%d", &d);
      if (d > 0) TI.interval = d;
    }
    pc = strstr(TI.os, "Average=");                             //-2007-02-03
    if (pc) {
      d = TI.average;
      sscanf(pc+8, "%d", &d);
      if (d > 0) TI.average = d;
    }
    pc = strstr(TI.os, "MntMax=");                             //-2007-02-03
    if (pc) {
      d = TI.npmax;
      sscanf(pc+7, "%d", &d);
      if (d > 200) d = 200;
      if (d > TI.npmax) TI.npmax = d;
    }
    pc = strstr(TI.os, "DMKp=");
    if (pc) {
      n = sscanf(pc+5, "{%lf;%lf;%lf;%lf;%lf;%lf;%lf;%lf;%lf}",
        &f1, &f2, &f3, &f4, &f5, &f6, &f7, &f8, &f9);
      if (n != 9)                                                     eX(300);
      TI.dmk[0] = f1;
      TI.dmk[1] = f2;
      TI.dmk[2] = f3;
      TI.dmk[3] = f4;
      TI.dmk[4] = f5;
      TI.dmk[5] = f6;
      TI.dmk[6] = f7;
      TI.dmk[7] = f8;
      TI.dmk[8] = f9;
    }
    MsgSetLocale(TI.lc);                                          //-2008-10-17
    if (!isUndefined(TI.hm) && TI.hm != 0) {
      vMsg(_$_not_time_dependent_, "hm");                         //-2011-12-02
      TI.hm = 0;
    } 
  }
  else {                                                          //-2002-03-26
    int not_allowed = 0;                                          //-2011-12-16
    if (TI.hm != 0) {
      vMsg(_$_ignored_, "hm");                                    //-2008-10-20
      TI.hm = 0;
      not_allowed++;
    }   
    if (TI.ie != TipEpsilon) {
      vMsg(_$_ignored_, "ie");                                    //-2008-10-20
      TI.ie = TipEpsilon;
      not_allowed++;
    }
    if (TI.im != TipMaxIter) {
      vMsg(_$_ignored_, "im");                                    //-2008-10-20
      TI.im = TipMaxIter;
      not_allowed++;
    }
    if (TI.mh != HUGE_VAL) {
      vMsg(_$_ignored_, "mh");                                    //-2008-10-20
      TI.mh = HUGE_VAL;
      not_allowed++;
    }
    if (TI.x1 != NULL) {
      vMsg(_$_ignored_, "x1");                                    //-2008-10-20
      FREE(TI.x1);
      TI.x1 = NULL;
      not_allowed++;
    }
    if (TI.x2 != NULL) {
      vMsg(_$_ignored_, "x2");                                    //-2008-10-20
      FREE(TI.x2);
      TI.x2 = NULL;
      not_allowed++;
    }
    if (TI.x3 != NULL) {
      vMsg(_$_ignored_, "x3");                                    //-2008-10-20
      FREE(TI.x3);
      TI.x3 = NULL;
      not_allowed++;
    }
    if (TI.y1 != NULL) {
      vMsg(_$_ignored_, "y1");                                    //-2008-10-20
      FREE(TI.y1);
      TI.y1 = NULL;
      not_allowed++;
    }
    if (TI.y2 != NULL) {
      vMsg(_$_ignored_, "y2");                                    //-2008-10-20
      FREE(TI.y2);
      TI.y2 = NULL;
      not_allowed++;
    }
    if (TI.y3 != NULL) {
      vMsg(_$_ignored_, "y3");                                    //-2008-10-20
      FREE(TI.y3);
      TI.y3 = NULL;
      not_allowed++;
    }
    if (TI.hh != NULL) {
      vMsg(_$_ignored_, "hh");                                    //-2012-04-06
      FREE(TI.hh);
      TI.hh = NULL;
      not_allowed++;
    }
    if (not_allowed)                                              //-2011-12-16
      return -1;
  }
  if (TI.ri != -HUGE_VAL && !isUndefined(TI.ri)) {                //-2014-01-21
    vMsg(_ri_time_dependent_);
    return -1;
  }
  //
  // coordinate system
  //
  if ((TI.gx!=0 || TI.gy!=0) && (TI.ux!=0 || TI.uy!=0)) {
    vMsg(_mixed_systems_);
    return -1;
  }
  else if ((TI.gx==0 && TI.gy==0) && (TI.ux==0 && TI.uy==0))
    strcpy(TI.ggcs, "");
  else if ((TI.gx==0 && TI.gy==0) && (TI.ux!=0)) {                //-2008-12-04
    strcpy(TI.ggcs, "UTM");
    TI.gx = TI.ux;
    TI.gy = TI.uy;
    if (TI.gx < 1000000)  TI.gx += 32000000;                      //-2008-12-07
  }
  else if ((TI.gx!=0 && TI.gy!=0) && (TI.ux==0 && TI.uy==0))
    strcpy(TI.ggcs, "GK");
  else {
    vMsg(_incomplete_coordinates_);
    return -1;
  }
  //
  if (isUndefined(TI.z0)) {
    if (strlen(TI.ggcs) == 0) {
      vMsg(_missing_z0_);
      return -1;
    }
  }
  else if (TI.z0 < 0) {
    vMsg(_invalid_z0_);
    return -1;
  }
  //
  if ((TI.qs>4 || TI.qs<-4) && !NOSTANDARD) {                           //-2002-03-12
    vMsg(_invalid_qs_);
    return -1;
  }
  if (TI.nq < 1) {
    vMsg(_no_sources_);
    return -1;
  }
  TI.xm = ALLOC(TI.nq*sizeof(double));  if (!TI.xm)   eX(1);
  TI.ym = ALLOC(TI.nq*sizeof(double));  if (!TI.ym)   eX(2);
  if (!TI.hq) {
    vMsg(_no_stack_height_);
    return -1;
  }
  if (!TI.aq) {
    TI.aq = ALLOC(TI.nq*sizeof(double));  if (!TI.aq) eX(3);
  }
  if (!TI.bq) {
    TI.bq = ALLOC(TI.nq*sizeof(double));  if (!TI.bq) eX(4);
  }
  if (!TI.cq) {
    TI.cq = ALLOC(TI.nq*sizeof(double));  if (!TI.cq) eX(5);
  }
  if (!TI.dq) {
    TI.dq = ALLOC(TI.nq*sizeof(double));  if (!TI.dq) eX(6);
  }
  if (!TI.lq) {
    TI.lq = ALLOC(TI.nq*sizeof(double));  if (!TI.lq) eX(7);
  }
  if (!TI.qq) {
    TI.qq = ALLOC(TI.nq*sizeof(double));  if (!TI.qq) eX(7);
  }
  if (!TI.rq) {
    TI.rq = ALLOC(TI.nq*sizeof(double));  if (!TI.rq) eX(7);
  }
  if (!TI.sq) {
    TI.sq = ALLOC(TI.nq*sizeof(double));  if (!TI.sq) eX(7);
    for (n=0; n<TI.nq; n++)  TI.sq[n] = -1;                 //-2002-01-03
  }
  if (!TI.tq) {
    TI.tq = ALLOC(TI.nq*sizeof(double));  if (!TI.tq) eX(7);
  }
  if (!TI.vq) {
    TI.vq = ALLOC(TI.nq*sizeof(double));  if (!TI.vq) eX(8);
  }
  if (!TI.wq) {
    TI.wq = ALLOC(TI.nq*sizeof(double));  if (!TI.wq) eX(9);
  }
  if (!TI.xq) {
    TI.xq = ALLOC(TI.nq*sizeof(double));  if (!TI.xq) eX(10);
  }
  if (!TI.yq) {
    TI.yq = ALLOC(TI.nq*sizeof(double));  if (!TI.yq) eX(11);
  }
  if (cNO2 >= 0 && cNO >= 0 && !TI.cmp[cNO2] && TI.cmp[cNO]) {		//2011-11-23
    TI.cmp[cNO2] = ALLOC(TI.nq*sizeof(double));  if (!TI.cmp[cNO2]) eX(12);
  }
  for (n=0; n<TI.nq; n++) {
    if (fabs(TI.xq[n]) > TIP_MAXLENGTH) { strcpy(s, "xq");          eX(240); }
    if (fabs(TI.yq[n]) > TIP_MAXLENGTH) { strcpy(s, "yq");          eX(240); }
    if (TI.hq[n]<0 || TI.hq[n]>500) {                              //-2011-06-17
      vMsg(_invalid_stack_height_$_, n+1);
      return -1;
    }
    if (TI.hq[n] < 10) {
      vMsg(_low_stack_height_$_, n+1);
    }
    if (TI.aq[n] < 0) {
      vMsg(_invalid_aq_$_, n+1);
      return -1;
    }
    if (TI.bq[n] < 0) {
      vMsg(_invalid_bq_$_, n+1);
      return -1;
    }
    if (TI.cq[n]<0 || TI.cq[n]+TI.hq[n]>500) {                     //-2011-06-17
      vMsg(_invalid_cq_$_, n+1);
      return -1;
    }
    if (TI.dq[n] < 0) {
      vMsg(_invalid_dq_$_, n+1);
      return -1;
    }
    if (TI.vq[n] < 0) {
      vMsg(_invalid_vq_$_, n+1);
      return -1;
    }
    if (TI.wq[n]<-360 || TI.wq[n]>360) {
      vMsg(_invalid_wq_$_, n+1);
      return -1;
    }
    co = cos(TI.wq[n]/RADIAN);
    si = sin(TI.wq[n]/RADIAN);
    TI.xm[n] = TI.xq[n] + 0.5*(TI.aq[n]*co - TI.bq[n]*si);  //-2001-09-04
    TI.ym[n] = TI.yq[n] + 0.5*(TI.bq[n]*co + TI.aq[n]*si);  //-2001-09-04
  }
  ///////////////////////////////////////////////////////////////////////////
  ///////////////////////////////////////////////////////////////////////////
  // check for buildings //-2004-07
  //
  if (TI.nb > 0 && strlen(TI.bf) > 0)                                eX(312);
  if (TI.nb > 0 || strlen(TI.bf) > 0) {
    TI.xbmin = ALLOC(2*sizeof(double));  if (!TI.xbmin)              eX(301);
    TI.xbmax = ALLOC(2*sizeof(double));  if (!TI.xbmax)              eX(301);
    TI.ybmin = ALLOC(2*sizeof(double));  if (!TI.ybmin)              eX(301);
    TI.ybmax = ALLOC(2*sizeof(double));  if (!TI.ybmax)              eX(301);
    BodyHmax = 0;
  }
  if (!TI.xb) {
    TI.xb = ALLOC(TI.nb*sizeof(double));  if (!TI.xb)                eX(301);
  }
  if (!TI.yb) {
    TI.yb = ALLOC(TI.nb*sizeof(double));  if (!TI.yb)                eX(301);
  }
  if (!TI.ab) {
    TI.ab = ALLOC(TI.nb*sizeof(double));  if (!TI.ab)                eX(301);
  }
  if (!TI.bb) {
    TI.bb = ALLOC(TI.nb*sizeof(double));  if (!TI.bb)                eX(301);
  }
  if (!TI.cb) {
    TI.cb = ALLOC(TI.nb*sizeof(double));  if (!TI.cb)                eX(301);
  }
  if (!TI.wb) {
    TI.wb = ALLOC(TI.nb*sizeof(double));  if (!TI.wb)                eX(301);
  }
  for (n=0; n<TI.nb; n++) {
   if (fabs(TI.xb[n]) > TIP_MAXLENGTH) { strcpy(s, "xb");           eX(240); }
    if (fabs(TI.yb[n]) > TIP_MAXLENGTH) { strcpy(s, "yb");           eX(240); }
    if (TI.ab[n] < 0 ) {
      vMsg(_invalid_ab_$_, n+1);
      return -1;
    }
    /*
    if (TI.bb[n] < 4 && TI.bb[n] > 0) {
      vMsg("Die Breite des Gebäudes %d hat einen unzulässigen Wert (<4) !",n+1);
      return -1;
    }
    */
    if (TI.bb[n] < 0 && TI.ab[n] != 0) {
      vMsg(_improper_ab_$_, n+1);
      return -1;
    }
    if (TI.cb[n] < 0) {
      vMsg(_invalid_cb_$_, n+1);
      return -1;
    }
    co = cos(TI.wb[n]/RADIAN); //-2004-12-14
    si = sin(TI.wb[n]/RADIAN); //-2004-12-14
    if (TI.bb[n] > 0) {
      x = TI.xb[n] + 0.5*(TI.ab[n]*co - TI.bb[n]*si);
      y = TI.yb[n] + 0.5*(TI.bb[n]*co + TI.ab[n]*si);
      z = 0.5*sqrt(TI.ab[n]*TI.ab[n] + TI.bb[n]*TI.bb[n]);
    }
    else {
      x = TI.xb[n];
      y = TI.yb[n];
      z = -0.5*TI.bb[n];
    }
    if (TI.cb[n] > BodyHmax) BodyHmax = TI.cb[n];
    for (l=0; l<=1; l++) {
      if (l == 0)
        r = z + BodyRatio*TI.cb[n];
      else
        r = z + TurbRatio*TI.cb[n];
      xmin = x - r;
      xmax = x + r;
      ymin = y - r;
      ymax = y + r;
      if (n == 0) {
        TI.xbmin[l] = xmin;
        TI.xbmax[l] = xmax;
        TI.ybmin[l] = ymin;
        TI.ybmax[l] = ymax;
      }
      else {
        if (xmin < TI.xbmin[l]) TI.xbmin[l]= xmin;
        if (xmax > TI.xbmax[l]) TI.xbmax[l] = xmax;
        if (ymin < TI.ybmin[l]) TI.ybmin[l] = ymin;
        if (ymax > TI.ybmax[l]) TI.ybmax[l] = ymax;
      }
    }
  }
  memset(&BM, 0, sizeof(ARYDSC));
  if (strlen(TI.bf) > 0) {
    TI.nb = -1;
    if (TutMakeName(name, Path, TI.bf) < 0)                          eX(10);
    strcpy(TI.bf, name);                                        //-2008-01-25
    rc = DmnRead(name, &usrhdr, &syshdr, &BM); if (rc != 1)          eX(320);
    if (BM.numdm != 2 || BM.elmsz != sizeof(int))                    eX(321);
    if (DmnFrmTab->dt != inDAT)                                      eX(327);
    rc = DmnGetDouble(usrhdr.s, "dz", "%lf", &BMdz, 1);
    if (rc != 1 || BMdz < 1)                                         eX(322);
    rc = DmnGetDouble(usrhdr.s, "x0|xmin", "%lf", &BMx0, 1);
    if (rc != 1)                                                     eX(323);
    rc = DmnGetDouble(usrhdr.s, "y0|ymin", "%lf", &BMy0, 1);
    if (rc != 1)                                                     eX(324);
    rc = DmnGetDouble(usrhdr.s, "dd|delta|delt", "%lf", &BMdd, 1);
    if (rc != 1 || BMdz < 1)                                         eX(325);
    if (fabs(BMx0) > TIP_MAXLENGTH || fabs(BMy0) > TIP_MAXLENGTH)    eX(326);
    n = 0;
    for (i=BM.bound[0].low; i<=BM.bound[0].hgh; i++)
      for (j=BM.bound[1].low; j<=BM.bound[1].hgh; j++) {
        k = *(int *)AryPtr(&BM, i, j);
        if (k == 0) continue;
        x = BMx0 + (i-BM.bound[0].low+0.5)*BMdd;
        y = BMy0 + (j-BM.bound[1].low+0.5)*BMdd;
        z = k * BMdz;
        if (z > BodyHmax) BodyHmax = z;
        for (l=0; l<=1; l++) {
          r = (l==0) ? BMdd/2 + BodyRatio*z : BMdd/2 + TurbRatio*z;
          xmin = x - r;
          xmax = x + r;
          ymin = y - r;
          ymax = y + r;
          if (n == 0) {
            TI.xbmin[l] = xmin;
            TI.xbmax[l] = xmax;
            TI.ybmin[l] = ymin;
            TI.ybmax[l] = ymax;
          }
          else {
            if (xmin < TI.xbmin[l]) TI.xbmin[l] = xmin;
            if (xmax > TI.xbmax[l]) TI.xbmax[l] = xmax;
            if (ymin < TI.ybmin[l]) TI.ybmin[l] = ymin;
            if (ymax > TI.ybmax[l]) TI.ybmax[l] = ymax;
          }
        }
        n++;
      }
    TI.nb = -n;
  }
  if (TI.nb) {
    vMsg(_maximum_height_$_, BodyHmax);
    if (BodyHmax == 0)
      TI.nb = 0;
    else {
      TalMode |= TIP_BODIES;
      TalMode |= TIP_NESTING;
      checkBodiesAndSources();
      if (TI.qb>1 || TI.qb<-3) {
        vMsg(_invalid_qb_);
        return -1;
      }
      BodyDd = BodyDda[3+TI.qb];
      BodyDz = BodyDza[3+TI.qb];
    }
  }
  if (strstr(TI.os, "-NESTING"))
    TalMode &= ~TIP_NESTING;
  else if (strstr(TI.os, "NESTING"))
    TalMode |= TIP_NESTING;
  ///////////////////////////////////////////////////////////////////////////
  ///////////////////////////////////////////////////////////////////////////
  //
  // check and/or define grid
  //
  if (TI.nn > 0) {                                  //-2001-10-24
    for (n=0; n<TI.nn; n++) {                       //-2002-08-15
      if ((TI.x0) && (fabs(TI.x0[n])>TIP_MAXLENGTH)) { strcpy(s, "x0");  eX(240); }
      if ((TI.y0) && (fabs(TI.y0[n])>TIP_MAXLENGTH)) { strcpy(s, "y0");  eX(240); }
    }
    checkGrid();                                                    eG(213);
    if (*TI.gh)  check_surface = 1;
    if (!TI.nz) {
      TI.nz = ALLOC(TI.nn*sizeof(int));  if (!TI.nz)                eX(234);
    }
  }
  else {
    make_grid = 1;
    dd = TI.hq[0] + 0.5*TI.cq[0];                   //-2001-10-18
    for (n=1; n<TI.nq; n++) {
      dh = TI.hq[n] + 0.5*TI.cq[n];                 //-2001-10-18
      if (dd > dh)  dd = dh;
    }
    dd = floor(dd);
    if (dd < DdMin)  dd = DdMin;
    xmin = TI.xm[0];                          //-2002-07-13
    xmax = TI.xm[0];
    ymin = TI.ym[0];
    ymax = TI.ym[0];
    for (n=0; n<TI.nq; n++) {
      double x, y, r;
      r = RadRatio*TI.hq[n];                  //-2002-07-13
      if (r < RadMin)  r = RadMin;            //-2002-07-13
      x = TI.xm[n] - r;
      if (x < xmin)  xmin = x;
      x = TI.xm[n] + r;
      if (x > xmax)  xmax = x;
      y = TI.ym[n] - r;
      if (y < ymin)  ymin = y;
      y = TI.ym[n] + r;
      if (y > ymax)  ymax = y;
    }
    if (!(TalMode & TIP_NESTING) && (TalMode & TIP_BODIES)) {
      dd = BodyDd;
      if (TI.xbmin[1] < xmin) xmin = TI.xbmin[1];
      if (TI.xbmax[1] > xmax) xmax = TI.xbmax[1];
      if (TI.ybmin[1] < ymin) ymin = TI.ybmin[1];
      if (TI.ybmax[1] > ymax) ymax = TI.ybmax[1];
    }
    xmin = dd*floor(xmin/dd+0.001);
    xmax = dd*ceil(xmax/dd-0.001);
    ymin = dd*floor(ymin/dd+0.001);
    ymax = dd*ceil(ymax/dd-0.001);
    gd[0][0] = dd;
    gd[0][1] = xmin;
    gd[0][2] = xmax;
    gd[0][3] = ymin;
    gd[0][4] = ymax;
    if (TalMode & TIP_NESTING) {
      TI.nn = getNesting(TI.nq, TI.xm, TI.ym, TI.hq, TI.aq, TI.bq, TI.cq,
                         TI.nb, TI.xbmin, TI.xbmax, TI.ybmin, TI.ybmax,
                         BodyDd, gd);
    }
    else TI.nn = 1;
    TI.dd = ALLOC(TI.nn*sizeof(double));  if (!TI.dd)         eX(220);
    TI.x0 = ALLOC(TI.nn*sizeof(double));  if (!TI.x0)         eX(221);
    TI.x1 = ALLOC(TI.nn*sizeof(double));  if (!TI.x1)         eX(222);
    TI.x2 = ALLOC(TI.nn*sizeof(double));  if (!TI.x2)         eX(223);
    TI.x3 = ALLOC(TI.nn*sizeof(double));  if (!TI.x3)         eX(224);
    TI.y0 = ALLOC(TI.nn*sizeof(double));  if (!TI.y0)         eX(225);
    TI.y1 = ALLOC(TI.nn*sizeof(double));  if (!TI.y1)         eX(226);
    TI.y2 = ALLOC(TI.nn*sizeof(double));  if (!TI.y2)         eX(227);
    TI.y3 = ALLOC(TI.nn*sizeof(double));  if (!TI.y3)         eX(228);
    TI.gl = ALLOC(TI.nn*sizeof(int));  if (!TI.gl)            eX(229);
    TI.gi = ALLOC(TI.nn*sizeof(int));  if (!TI.gi)            eX(230);
    TI.nx = ALLOC(TI.nn*sizeof(int));  if (!TI.nx)            eX(231);
    TI.ny = ALLOC(TI.nn*sizeof(int));  if (!TI.ny)            eX(232);
    TI.nz = ALLOC(TI.nn*sizeof(int));  if (!TI.nz)            eX(233);
    for (n=0; n<TI.nn; n++) {
      TI.dd[n] = gd[n][0];
      TI.x0[n] = gd[n][1];
      TI.x1[n] = gd[n][1];
      TI.x2[n] = gd[n][2];
      TI.x3[n] = gd[n][2];
      TI.y0[n] = gd[n][3];
      TI.y1[n] = gd[n][3];
      TI.y2[n] = gd[n][4];
      TI.y3[n] = gd[n][4];
      TI.nx[n] = (int)((TI.x3[n]-TI.x0[n])/TI.dd[n] + 0.5);
      TI.ny[n] = (int)((TI.y3[n]-TI.y0[n])/TI.dd[n] + 0.5);
      TI.nz[n] = 0;
      TI.gl[n] = TI.nn - n - (TI.nn==1);
      TI.gi[n] = (TI.nn > 1);
    }
    if (*TI.gh)  check_surface = 1;                                         //-2003-10-12
  }
  TipTrbExt = (TalMode & TIP_NESTING) && (TalMode & TIP_BODIES);
  if (TipTrbExt
    && TI.nn > 1
    && TI.x0[0] <= TI.xbmin[1]                                  //-2005-03-16
    && TI.x3[0] >= TI.xbmax[1]                                  //-2005-03-16
    && TI.y0[0] <= TI.ybmin[1]                                  //-2005-03-16
    && TI.y3[0] >= TI.ybmax[1]) TipTrbExt = 0;                  //-2005-03-16
  if (CHECK) for (n=0; n<TI.nn; n++) printf(
    "n=%d (%d,%d), dd=%1.1lf, x0=%1.0lf, x3=%1.0lf, nx=%d, y0=%1.0lf, y3=%1.0lf, ny=%d\n",
    n, TI.gl[n], TI.gi[n], TI.dd[n], TI.x0[n], TI.x3[n], TI.nx[n], TI.y0[n], TI.y3[n], TI.ny[n]);
  //
  // set vertical grid //-2004-07
  if (TI.hh && NOSTANDARD) {                                    //-2002-08-02
    if (TI.nhh < 2) {
      vMsg(_no_vertical_grid_);
      return -1;
    }
    if (TI.hh[0] != 0) {
      vMsg(_invalid_footpoint_);
      return -1;
    }
    for (k=1; k<TI.nhh; k++) {
      if (TI.hh[k] <= TI.hh[k-1]) {
        vMsg(_invalid_vertical_grid_);
        return -1;
      }
    }
  }
  else {
    if (TI.hh) {                                                //-2002-08-02
      FREE(TI.hh);
      vMsg(_hh_ignored_);
    }
    if (!TI.nb) {
      TI.nhh = sizeof(Hh)/sizeof(double);
      TI.hh = ALLOC(sizeof(Hh));  if (!TI.hh)                        eX(20);
      memcpy(TI.hh, Hh, sizeof(Hh));
    }
    else {
      // (almost) equal intervals in the lower part
      hh[0] = 0;
      hh[1] = 3;
      k = 1;
      z = BodyDz * (int)(2.0*BodyHmax/BodyDz + 0.5);
      while (hh[k]<z && k<99) {
        ++k;
        hh[k] = hh[k-1]+BodyDz;
      }
      // check for next valid height of the default vertical grid
      n = sizeof(Hh)/sizeof(double);
      for (i=0; i<n; i++) if (Hh[i] >= hh[k]) break;
      if (i >= n-1)                                                  eX(400);
      if (Hh[i]-hh[k] < BodyDz) {
        ++k;
        ++i;
        hh[k] = hh[k-1]+BodyDz;
      }
      // fill gap from equal spacing to default vertical grid
      dz = floor(BodyDz*1.5);
      while (Hh[i]-(hh[k]+dz) >= dz && k<99) {
        ++k;
        hh[k] = hh[k-1]+dz;
        dz = floor(dz*1.5);
      }
      // continue with default vertical grid
      k++;
      while (i<n && k<99) {
        hh[k] = Hh[i];
        ++k;
        ++i;
      }
      if (i!=n)                                                      eX(401);
      TI.nhh = k;
      TI.hh = ALLOC(k*sizeof(double));  if (!TI.hh)                  eX(20);
      *s = 0;
      for (i=0; i<TI.nhh; i++) {
        TI.hh[i] = hh[i];
        sprintf(s, "%s %6.1f", s, hh[i]);
        if ((i+1)%10 == 0) strcat(s, "\n");
      }
      if (i%10 != 0) strcat(s, "\n");
      fprintf(MsgFile, _vertical_grid_$_, s);
      fprintf(MsgFile, "----------------------------------------------------------------------\n");
    }
  }
  // set vertical grid extent
  TI.nzmax = TI.nhh - 1;
  if (!TI.nb || TI.nn == 1) {
    for (n=0; n<TI.nn; n++)
      TI.nz[n] = TI.nzmax;
  }
  else {
    if (TI.nz[0] <= 0) {
      make_nz = 1;                                              //-2005-03-16
      for (i=0; i<TI.nhh; i++)
        if (TI.hh[i] >= 2*BodyHmax) break;
      if (i >= TI.nhh)                                               eX(404);
      TI.nz[0] = i;
    }
    for (i=0; i<TI.nn; i++) {
      if (TI.nz[i] <= 0)
        TI.nz[i] = TI.nzmax;
    }
    // check
    if (TI.nz[1] < TI.nzmax)                                         eX(408);  //-2006-02-11
    for (i=0; i<TI.nn; i++) {
      if (TI.nz[i] > TI.nzmax)                                       eX(405);
      if (i > 0 && TI.nz[i] < TI.nz[i-1])                            eX(406);
      if (i == 0 && TI.hh[TI.nz[i]] < 2*BodyHmax)                    eX(407);
    }
    // set user defined nzmax
    TI.nzmax = 0;
    for (i=0; i<TI.nn; i++)
      if (TI.nz[i] > TI.nzmax)
        TI.nzmax = TI.nz[i];
  }
  if (TI.kp > TI.nzmax)
    TI.kp = TI.nzmax;
  if (make_grid || make_nz) {                                   //-2005-03-16
    fprintf(MsgFile, "%s", _computational_grid_);
    fprintf(MsgFile, "dd");
    for (n=0; n<TI.nn; n++)  fprintf(MsgFile, " %6.0f", TI.dd[n]);
    fprintf(MsgFile, "\n");
    fprintf(MsgFile, "x0");
    for (n=0; n<TI.nn; n++)  fprintf(MsgFile, " %6.0f", TI.x0[n]);
    fprintf(MsgFile, "\n");
    fprintf(MsgFile, "nx");
    for (n=0; n<TI.nn; n++)  fprintf(MsgFile, " %6d", TI.nx[n]);
    fprintf(MsgFile, "\n");
    fprintf(MsgFile, "y0");
    for (n=0; n<TI.nn; n++)  fprintf(MsgFile, " %6.0f", TI.y0[n]);
    fprintf(MsgFile, "\n");
    fprintf(MsgFile, "ny");
    for (n=0; n<TI.nn; n++)  fprintf(MsgFile, " %6d", TI.ny[n]);
    fprintf(MsgFile, "\n");
    fprintf(MsgFile, "nz");
    for (n=0; n<TI.nn; n++)  fprintf(MsgFile, " %6d", TI.nz[n]);
    fprintf(MsgFile, "\n");
    fprintf(MsgFile, "--");
    for (n=0; n<TI.nn; n++)  fprintf(MsgFile, "-------");
    fprintf(MsgFile, "\n");
    checkGrid();                                                eG(313);
  }
  // end set vertical grid
  //
  // check anemometer position          -2002-01-12, 2005-03-16 moved to here
  //
  if (TI.xa==HUGE_VAL || TI.ya==HUGE_VAL) {
    if (*TI.gh || TI.nb) {                                       eX(214); }
    else {
      TI.xa = TI.x0[0];
      TI.ya = TI.y0[0];
    }
  }
  else {
    if (fabs(TI.xa) > TIP_MAXLENGTH)   { strcpy(s, "xa");       eX(240); }
    if (fabs(TI.ya) > TIP_MAXLENGTH)   { strcpy(s, "ya");       eX(240); }
  }
  for (n=0; n<TI.nn; n++) {
    if (TI.xa>=TI.x0[n] && TI.xa<=TI.x3[n]
     && TI.ya>=TI.y0[n] && TI.ya<=TI.y3[n])  break;
  }
  if (n >= TI.nn)                                               eX(215);
  //
  if (*TI.gh) {   // complex terrain: all surfaces defined?
    if (check_surface && !make_surface) {
      for (n=0; n<TI.nn; n++) {
        k = checkSurface(n);                                    eG(210);
        if (!k) {
          if (n == 0)  make_surface = 1;
          else if (!make_surface)                               eX(212);
        }
      }
    }
    if (make_surface) {
      for (n=0; n<TI.nn; n++) {
        makeSurface(n);                                         eG(211);
      }
    }
    else {
    		if (TI.nn == 1)
    		  vMsg(_use_existing_zg_);
    		else if (TI.nn > 1)
    				vMsg(_use_existing_zgs_);
    }
  }
  if (isUndefined(TI.mh))  TI.mh = 0;
  for (n=0; n<TI.nq; n++) {
    if (CHECK) printf("source %d, xm=%1.0lf, ym=%1.0lf\n", n+1, TI.xm[n], TI.ym[n]);
    for (i=0; i<TI.nn; i++)
      if (TI.xm[n]>TI.x1[i] && TI.xm[n]<TI.x2[i] && TI.ym[n]>TI.y1[i] && TI.ym[n]<TI.y2[i]) break;
    if (i >= TI.nn) {
      vMsg(_source_outside_$_, n+1);
      return -1;
    }
  }
  if (isUndefined(TI.z0)) {
    TI.z0 = TipCorine(TI.nq, TI.xm, TI.ym, TI.hq, TI.cq);
    if (TI.z0 <= 0) {
      vMsg(_no_z0_reason_);
      if (TI.z0 == RGLE_FILE)
        vMsg(_z0_file_not_found_$_, RglFile);
      else if (TI.z0 == RGLE_XMIN)
        vMsg(_z0_no_xmin_$_, RglFile);
      else if (TI.z0 == RGLE_YMIN)
        vMsg(_z0_no_ymin_$_, RglFile);
      else if (TI.z0 == RGLE_DELT)
        vMsg(_z0_no_delt_$_, RglFile);
      else if (TI.z0 == RGLE_GGCS)
        vMsg(_z0_no_ggcs_$_, RglFile);
      else if (TI.z0 == RGLE_DIMS)
        vMsg(_z0_not_2d_$_, RglFile);
      else if (TI.z0 == RGLE_ILOW)
        vMsg(_z0_invalid_low_bound_j_$_, RglFile);
      else if (TI.z0 == RGLE_JIND)
        vMsg(_z0_invalid_bounds_i_$_, RglFile);
      else if (TI.z0 == RGLE_SEQN)
        vMsg(_z0_no_sequ_$_, RglFile);
      else if (TI.z0 == RGLE_SEQU)
        vMsg(_z0_invalid_sequ_$_, RglFile);
      else if (TI.z0 == RGLE_CSNE)
        vMsg(_z0_improper_ggcs_$$$_, RglFile, RglGGCS, TI.ggcs);
      else if (TI.z0 == RGLE_GKGK)
        vMsg(_z0_invalid_source_$$$_, RglX, RglY, RglMrd);
      else if (TI.z0 == RGLE_POUT) {
        vMsg(_z0_source_outside_$$$_, RglX, RglY, RglFile);
        vMsg(_z0_allowed_range_$$$$_, RglXmin, RglXmax, RglYmin, RglYmax);
      }
      else if (TI.z0 == RGLE_SECT)
        vMsg(_z0_error_$_, RglFile);
      else
        vMsg(_z0_unspecified_error_);
      vMsg("");
      return -1;
    }
    if (MsgFile)
      fprintf(MsgFile, _z0_average_z0_$_, TI.z0);
    else
      vMsg(_z0_average_z0_$_, TI.z0);
  }
  else round_z0 = !NOSTANDARD;
  iZ0 = TipZ0index(TI.z0);                                                  //-2001-06-27
  z0 = z0TAL[iZ0];                                                          //
  if (z0!=TI.z0 && round_z0) {
    vMsg(_z0_rounded_$_, z0);                                               //
    TI.z0 = z0;                                                             //
  }                                                                         //
  if (isUndefined(TI.d0))  TI.d0 = 6*TI.z0;
  else if (TI.d0 < 0) {
    vMsg(_invalid_d0_);
    return -1;
  }
  //
  // check definition of odor emissions                         //-2008-03-10
  //
  emission = 0;
  if (cODOR >= 0) {
    for (i=0; i<TIP_ADDODOR; i++) {
      if (TI.cmp[cODOR+i+1] != NULL)
        emission++;
    }
    if (emission > 0) {
      double* oe = TI.cmp[cODOR];
      if (oe != NULL) {
        vMsg(_source_strength_ignored_);
      }
      else {
        oe = ALLOC(TI.nq*sizeof(double));
        TI.cmp[cODOR] = oe;
      }
      for (n=0; n<TI.nq; n++)
        oe[n] = 0;
      TalMode |= TIP_ODORMOD;
    }
  }
  //
  // check variable parameters
  //
  AryFree(&TipVar);
  nVarParm = 0;
  nUsrParm = 0;
  emission = 0;
  for (check=1; check>=0; check--) {                                            //-2005-09-23
    int nv = 0;                                                   //-2006-12-13
    checkvar(check, TI.vq, TI.nq, "vq", &nv);                     //-2006-12-13
    checkvar(check, TI.qq, TI.nq, "qq", &nv);                     //-2006-12-13
    checkvar(check, TI.rq, TI.nq, "rq", &nv);                     //-2006-12-13
    checkvar(check, TI.lq, TI.nq, "lq", &nv);                     //-2011-11-23
    checkvar(check, TI.sq, TI.nq, "sq", &nv);                     //-2006-12-13
    checkvar(check, TI.tq, TI.nq, "tq", &nv);                     //-2006-12-13
    for (n=0; n<SttCmpCount; n++) {
      strcpy(name, SttCmpNames[n]);
      k = checkvar(check, TI.cmp[n], TI.nq, name, &nv);           //-2006-12-13
      if (check && k) {
        emission++;
      }
    }
    if (check) {
      if (!emission) {
        vMsg(_no_emissions_);
        return -1;
      }
      nUsrParm = nv;
      if (isUndefined(TI.hm))
        nv++;
      if (isUndefined(TI.ri))
        nv++;
      nVarParm = nv;                                              //-2006-12-13
      AryCreate(&TipVar, sizeof(TIPVAR), 1, 0, nVarParm-1);       //-2011-12-02
                                                                  eG(21); 
    }
    else {  // optional parameters                                  -2011-12-08
      if (isUndefined(TI.hm)) {
        ptv = AryPtrX(&TipVar, nv);
        strcpy(ptv->name, "hm");
        ptv->p = &TI.hm;
        nv++;
      }
      if (isUndefined(TI.ri)) {
        ptv = AryPtrX(&TipVar, nv);
        strcpy(ptv->name, "ri");
        ptv->p = &TI.ri;
        nv++;
      }
    }
  }
  if (nUsrParm > 0)  TalMode |= TIP_VARIABLE;
  //
  // check monitor points
  //
  if (TI.np > 0) {
    if (TI.np > TI.npmax)                                             eX(241); //-2006-10-17
    if (TI.xp && TI.yp) {
      for (i=0; i<TI.np; i++) {
        for (n=0; n<TI.nn; n++)
          if (TI.xp[i]>TI.x0[n] && TI.xp[i]<TI.x3[n] && TI.yp[i]>TI.y0[n] && TI.yp[i]<TI.y3[n])
            break;
        if (n >= TI.nn) {
          vMsg(_monitor_outside_$_, i+1);
          return -1;                                                  //-2002-02-26
        }
       }
    }
    else {
      vMsg(_missing_xy_);
      return -1;
    }
    if (!TI.hp) {
      TI.hp = ALLOC(TI.np*sizeof(double));
      for (i=0; i<TI.np; i++)  TI.hp[i] = 1.5;
    }
    // check maximum height of monitor points (for nzdos)  2001-08-03
    //
    kpmax = 1;
    for (i=0; i<TI.np; i++) {
      h = TI.hp[i];
      for (k=0; k<=TI.nzmax; k++)   if (h < TI.hh[k])  break;
      if (k > TI.nzmax) {
        vMsg(_monitor_above_$_, i+1);
        return -1;
      }
      if (k > kpmax)  kpmax = k;
    }
    if (TI.kp < kpmax)  TI.kp = kpmax;
  }
  //
  // read and check time series
  //
  if (TipFrmTab)  FREE(TipFrmTab);
  TipFrmTab = NULL;
  strcpy(name, path);
  strcat(name, "/");
  strcat(name, CfgSeriesString);                                  //-2008-07-28
  strcat(name, ".dmna");
  if (write_z) {
    if (!*TI.az) {
      vMsg(_missing_akterm_);
      return -1;
    }
    TalMode |= TIP_SERIES;                                      //-2002-01-02
  }
  else {
    series_exists = TutFileExists(name);
    if (series_exists || *TI.az)  TalMode |= TIP_SERIES;
    else
      if (*TI.as)  TalMode |= TIP_STAT;
      else {
        vMsg(_missing_meteo_);
        return -1;
      }
    if (TalMode & TIP_VARIABLE) {
      if ((TalMode&TIP_SERIES) && !series_exists) {
        vMsg(_missing_series_$_, name);
        return -1;
      }
      if (TalMode & TIP_STAT) {
        char fname[256];
        int n, k=0, quiet, verbose;
        TIPVAR *ptv;
        ARYDSC *pa;
        for (n=0; n<nVarParm; n++) {
          ptv = AryPtrX(&TipVar, n);
          sprintf(fname, "%s/%s.dmna", path, ptv->name);
          if (!TutFileExists(fname)) {
            vMsg(_missing_table_$$_, ptv->name, fname);
            k++;
          }
        }
        if (k)
          return -1;
        for (n=0; n<nVarParm; n++) {
          ptv = AryPtrX(&TipVar, n);
          sprintf(fname, "%s/%s.dmna", path, ptv->name);
          quiet = MsgQuiet;       MsgQuiet = 1;
          verbose = MsgVerbose;   MsgVerbose = 1;
          pa = &(ptv->dsc);
          DmnRead(fname, NULL, NULL, pa);
          MsgQuiet = quiet;
          MsgVerbose = verbose;
          if (MsgCode < 0) {
            vMsg(_cant_read_file_$_, fname);
            vMsg(_see_log_);
            MsgCode = 0;
            return -2;
          }
          if (pa->numdm!=2 || pa->elmsz!=4
            || pa->bound[0].low!=1 || pa->bound[0].hgh!=6
            || pa->bound[1].low!=1 || pa->bound[1].hgh!=9) {
            vMsg(_invalid_structure_$_, fname);
            return -2;
          }
        } // for n
      }
    }
  }
  if (TalMode & TIP_SERIES) {    // ------------ work with time series --------
    TatArgument(path);
    /* TatArgument("-u0.7"); */ // 2013-01-11 (does not work with german locale)
    sprintf(s, "-f%s", TI.az);
    TatArgument(s);
    sprintf(s, "-z%1.3lf,%1.3lf", TI.z0, TI.d0);
    TatArgument(s);
    sprintf(s, "-a%1.0lf,%1.0lf,%1.1lf", TI.xa, TI.ya, TI.ha);      //-2002-04-16
    TatArgument(s);
    TatArgument(NULL);
  }
  if (!series_exists && (TI.interval != 3600))                 eX(130);  //-2007-02-03
  if (series_exists) {  // --------- time series provided by user -------------
    int quiet, verbose, ostd;
    double t1, t2;
    int uses_hm = 0;
    int uses_ri = 0;
    int off_hm, off_ri;
    char *p;
    //
    quiet = MsgQuiet;
    MsgQuiet = 1;
    verbose = MsgVerbose;
    MsgVerbose = 1;
    DmnRead(name, &TxtUser, &TxtSystem, &TIPary);
    MsgQuiet = quiet;
    MsgVerbose = verbose;
    if (MsgCode < 0) {
      vMsg(_cant_read_time_series_$_, name);
      vMsg(_see_log_);
      MsgCode = 0;
      return -2;
    }
    vMsg(_using_series_$_, name);
    TipLogCheck("SERIES", TutMakeCrc(TIPary.start, TIPary.ttlsz)); //-2011-12-07
    TipFrmTab = DmnFrmTab;  if (!TipFrmTab)                     eX(26);
    DmnFrmTab = NULL;
    if (TIPary.numdm != 1)                                      eX(105);
    i1 = TIPary.bound[0].low;
    i2 = TIPary.bound[0].hgh;
    if (i2-i1+1 > MNT_NUMVAL) {
      vMsg(_series_too_long_$_, MNT_NUMVAL);
      return -3;
    }
    ostd = 0;
    //
    // mandatory parameters
    //
    strcpy(elnm, "te");
    pfr = DmnFrmPointer(TipFrmTab, elnm);  if (!pfr)            eX(101);
    if (pfr->offset != ostd)                                    eX(111);
    if (pfr->dt != ltDAT)                                       eX(121);
    ostd += sizeof(double);
    strcpy(elnm, "ra");
    pfr = DmnFrmPointer(TipFrmTab, elnm);  if (!pfr)            eX(102);
    if (pfr->offset != ostd)                                    eX(112);
    if (pfr->dt != flDAT)                                       eX(122);
    ostd += sizeof(float);
    strcpy(elnm, "ua");
    pfr = DmnFrmPointer(TipFrmTab, elnm);  if (!pfr)            eX(103);
    if (pfr->offset != ostd)                                    eX(113);
    if (pfr->dt != flDAT)                                       eX(123);  //-2001-06-27
    ostd += sizeof(float);
    strcpy(elnm, "lm");
    pfr = DmnFrmPointer(TipFrmTab, elnm);  if (!pfr)            eX(104);
    if (pfr->offset != ostd)                                    eX(114);
    if (pfr->dt != flDAT)                                       eX(124);
    ostd += sizeof(float);
    //
    // optional parameters                                          -2011-12-02
    //
    off_hm = -1;
    strcpy(elnm, "hm");
    pfr = DmnFrmPointer(TipFrmTab, elnm);
    if (pfr) {
      if (pfr->dt != flDAT)                                     eX(116);
      off_hm = pfr->offset;
    }
    off_ri = -1;
    strcpy(elnm, "ri");
    pfr = DmnFrmPointer(TipFrmTab, elnm);  
    if (pfr) {
      if (pfr->dt != flDAT)                                     eX(116);
      off_ri = pfr->offset;
    }
    if (off_hm > 0 || off_ri > 0) {
      for (i=i1; i<=i2; i++) {
        p = AryPtrX(&TIPary, i);
        if (off_hm > 0) {
          float hm = *(float*)(p + off_hm);
          if (hm > 0)
            uses_hm = 1;
        }
        if (off_ri > 0) {
          float ri = *(float*)(p + off_ri);
          if (ri >= 0)
            uses_ri = 1;
        }
      } // for i
    }
    //                                                              -2011-12-02
    if (isUndefined(TI.hm)) {   // values expected
      if (!uses_hm)                                               eX(250)
      else
        TalMode |= TIP_HM_USED;
    }
    //else if (uses_hm)
    //  vMsg(_HM_in_$_ignored_, "\"time series\"");
    //                                                              -2011-12-02
    if (isUndefined(TI.ri)) {   // values expected
      if (!uses_ri)                                               eX(251)
      else
        TalMode |= TIP_RI_USED;
    }
    else if (CfgWet && uses_ri)
      vMsg(_RI_in_$_ignored_, "DMNA");
    //
    // variable parameters
    //
    for (i=0; i<nVarParm; i++) {
      ptv = AryPtrX(&TipVar, i);
      strcpy(elnm, ptv->name);
      pfr = DmnFrmPointer(TipFrmTab, ptv->name);  if (!pfr)     eX(106);
      if (pfr->dt != flDAT)                                     eX(116);
      ptv->o = pfr->offset;
      ptv->i = (pfr->offset - ostd)/sizeof(float);                //-2011-12-19
    }
    //
    if (TI.ha <= 0) {
      float havec[9];
      i = DmnGetFloat(TxtUser.s, "ha", "%f", havec, 9);
      if (i == 9) {
        TI.ha = havec[iZ0];
        vMsg(_using_ha_$_, TI.ha);
      }
    }
    if (*TI.az)  vMsg(_ignoring_az_$_, TI.az);
    *TI.az = 0;
    if (*TI.as)  vMsg(_ignoring_as_$_, TI.as);   //-2001-12-14
    *TI.as = 0;
    t1 = *(double*) AryPtrX(&TIPary, i1);
    for (i=i1+1; i<=i2; i++) {                                //-2001-12-14
      t2 = *(double*) AryPtrX(&TIPary, i);
      n = (int)(MsgDateSeconds(t1, t2) + 0.5);
      if (n != TI.interval)                                     eX(120);  //-2007-02-03
      t1 = t2;
    }
  }
  else if (*TI.az) {  // ------------------ AKTerm provided by user ------------------
    int nn, nt, elmsz, akt;
    akt = TatReadAKTerm();
    if (akt < 0)
      return -1;
    nt = AKTary.bound[0].hgh;
    if (nt > MNT_NUMVAL) {
      vMsg(_akterm_too_long_$_, MNT_NUMVAL);
      return -3;
    }
    //                                                              -2011-12-02
    if (isUndefined(TI.hm)) {   // values expected
      if (0 == (akt & 0x01))                                    eX(260)
      else
        TalMode |= TIP_HM_USED;
    }
    //else if (akt & 0x01)
    //  vMsg(_HM_in_$_ignored_, "\"AKTerm\"");
    //                                                              -2011-12-02
    if (isUndefined(TI.ri)) {   // values expected
      if (0 == (akt & 0x02))                                    eX(261)
      else
        TalMode |= TIP_RI_USED;
    }
    else if (CfgWet && (akt & 0x02))
      vMsg(_RI_in_$_ignored_, "AKTerm");
    //
    nn = TatCheckZtr();
    if (TI.ha <= 0) {                                       //-2002-04-16
      TI.ha = TatGetHa(iZ0);
      vMsg(_using_ha_$_, TI.ha);
    }
    vMsg(_availability_$_, 100 - (nn*100.0)/nt);
    { // extend the transformed AKTerm
      char *_frm;
      char **forms;
      char s[40];
      char form[40];                                              //-2011-12-08
      char form_hm[40];                                           //-2011-12-08
      char form_ri[40];                                           //-2011-12-08
      AKTREC *psrc;
      TMSREC *pdst;
      int nf, ihm=-1, iri=-1;                                     //-2011-11-22
      int l4 = AKTary.elmsz - sizeof(int) - sizeof(float);        //-2011-12-08
      forms = ALLOC(6*sizeof(char*));                             //-2011-12-08
      elmsz = l4 + nVarParm*sizeof(float);                        //-2011-12-08
      AryCreate(&TIPary, elmsz, 1, 1, nt);                    eG(22);
      nf = DmnGetString(AKTheader, "form", forms, 6);             //-2011-11-22
      if (nf != 6)                                            eX(23);
      n = 4;
      for (i=0; i<nf-2; i++)                                      //-2011-12-08
        n += strlen(forms[i])+3;
      n += 22*nVarParm;
      _frm = ALLOC(n);  if (!_frm)                            eX(24); //-2014-06-26
      for (i=0; i<nf; i++) {                                      //-2011-12-08
        strcpy(form, forms[i]);
        FREE(forms[i]);                                           //-2011-12-08
        if (!strncmp(form, "hm%", 3)) {
          strcpy(form_hm, form);
          continue;
        }
        if (!strncmp(form, "ri%", 3)) {
          strcpy(form_ri, form);
          continue;
        }
        strcat(_frm, " \"");
        if (!strncmp(form, "kl%", 3))
          strcat(_frm, "lm%7.1f");
        else
          strcat(_frm, form);
        strcat(_frm, "\"");
      }
      FREE(forms);
      for (i=0; i<nVarParm; i++) {
        ptv = AryPtrX(&TipVar, i);
        strcpy(s, " \"");
        if (!strcmp(ptv->name, "hm")) {                           //-2011-12-08
          strcat(s, form_hm);
          ihm = i;
        }
        else if (!strcmp(ptv->name, "ri")) {                      //-2011-12-08
          strcat(s, form_ri);
          iri = i;
        }
        else {
          strcat(s, ptv->name);
          strcat(s, "%10.3e");
        }
        strcat(s, "\"");
        strcat(_frm, s);
        ptv->i = i;
        ptv->o = l4 + i*sizeof(float);
      }
      if (CHECK) vMsg("New format:%s<<<\n", _frm);
      TipFrmTab = DmnAnaForm(_frm, &TIPary.elmsz);  if (!TipFrmTab) eX(28);
      TxtCpy(&TxtSystem, "form  ");                               //-2014-01-28
      TxtCat(&TxtSystem, _frm);
      TxtCat(&TxtSystem, "\n");
      FREE(_frm);
      for (i=1; i<=nt; i++) {
        pdst = AryPtrX(&TIPary, i);
        psrc = AryPtrX(&AKTary, i);
        pdst->t = psrc->t;
        pdst->fRa = psrc->fRa;
        pdst->fUa = psrc->fUa;
        k = psrc->iKl - 1;
        pdst->fLm = (k<0 || k>5) ? 0 : lmTAL[k][iZ0];             //-2001-07-12
        if (ihm >= 0)
          pdst->fPrm[ihm] = psrc->fHm;                            //-2011-12-08
        if (iri >= 0)
          pdst->fPrm[iri] = psrc->fPr;                            //-2011-12-08
      }
      AryFree(&AKTary);
    }
    if (write_z) {  // write generated time series
      char s[256], t[256];                                        //-2008-10-17
      sprintf(t, "locl  \"%s\"\n", TI.lc);                        //-2008-10-17 -2014-01-28
      TxtCat(&TxtSystem, t);                                      //-2008-10-17
      strcpy(name, path);
      strcat(name, "/");
      strcat(name, CfgSeriesString);
      TxtCat(&TxtSystem, "mode  \"text\"\n");                     //-2014-01-28
      strcpy(s, "ha");
      for (i=0; i<nz0TAL; i++) {
        sprintf(t, "  %1.1f", TatGetHa(i));                       //-2014-01-28
        strcat(s, t);
      }
      strcat(s, "\n");
      TxtCat(&TxtSystem, s);
      sprintf(s, "z0  %3.2lf\n", TI.z0);  TxtCat(&TxtSystem, s);  //-2002-06-19 -2014-01-28
      sprintf(s, "d0  %3.2lf\n", TI.d0);  TxtCat(&TxtSystem, s);  //-2014-01-28
      sprintf(s, "artp  \"ZA\"\n");  TxtCat(&TxtSystem, s);       //-2008-12-04 -2014-01-28
      DmnWrite(name, TxtSystem.s, &TIPary);                   eG(100);
      vMsg(_series_written_$_, name);
      TxtClr(&TxtSystem);
    }
  }
  if (TI.ha <= 0) {                                               //-2002-04-16
    TI.ha = 10 + TI.d0;
    vMsg(_using_ha_$_, TI.ha);
  }
  if (TalMode & TIP_STAT) {  //--------------- AKS provided by user -----------
    TalAKS(Path);
    sprintf(s, "-f%s", TI.as);
    TalAKS(s);
    sprintf(s, "-z%1.3lf,%1.3lf", TI.z0, TI.d0);
    TalAKS(s);
    sprintf(s, "-a%1.0lf,%1.0lf,%1.1lf", TI.xa, TI.ya, TI.ha);    //-2002-04-16
    TalAKS(s);
    if (cNO >= 0 && TI.cmp[cNO])  TalAKS("-n");
    d = TasRead();
    if (d > 0) {                                                  //-2011-12-20
      TalMode |= TIP_RI_USED;
      d = 0;
    }
  }
  if (TalMode & TIP_SERIES) {
    i1 = TIPary.bound[0].low;
    i2 = TIPary.bound[0].hgh;
    TI.t1 = *(double*) AryPtrX(&TIPary, i1);
    TI.t2 = *(double*) AryPtrX(&TIPary, i2);
    TI.t1 = MsgDateIncrease(TI.t1, -TI.interval);                 //-2007-02-03
  }
  setlocale(LC_NUMERIC, locale);                                  //-2008-10-17
  return d;

eX_1: eX_2: eX_3: eX_4: eX_5: eX_6: eX_7: eX_8: eX_9: eX_10: eX_11: eX_12:
eX_20: eX_21: eX_22: eX_23: eX_24: eX_26: eX_28:
eX_220: eX_221: eX_222: eX_223: eX_224: eX_225: eX_226: eX_227: eX_228:
eX_229: eX_230: eX_231: eX_232: eX_233: eX_234:
eX_301:
  eMSG(_internal_error_);
eX_100:
  vMsg(_akterm_not_written_$_, name);
  MsgCode = 0;
  return -100;
eX_105:
  eMSG(_invalid_structure_series_$_, name);
eX_101: eX_102: eX_103: eX_104:
  vMsg(_missing_element_$_, elnm);
  DmnPrnForm(TipFrmTab, name);
  return MsgSource=ePGMs, MsgCode;
eX_111: eX_112: eX_113: eX_114:
  vMsg(_wrong_position_$_, elnm);
  DmnPrnForm(TipFrmTab, name);
  return MsgSource=ePGMs, MsgCode;
eX_121: eX_122: eX_123: eX_124:
  vMsg(_wrong_type_$_, elnm);
  DmnPrnForm(TipFrmTab, name);
  return MsgSource=ePGMs, MsgCode;
eX_106:
  eMSG(_missing_parameter_$_, elnm);
eX_116:
  eMSG(_float_expected_$_, elnm);
eX_120:
  eMSG(_invalid_interval_$$_, i-i1+1, TI.interval);
eX_130:
  eMSG(_interval_requires_series_);
eX_210:
  eMSG(_improper_profile_$_, n+1);
eX_211:
  eMSG(_profile_not_created_$_, n+1);
eX_212:
  eMSG(_incomplete_profiles_);
eX_213: eX_313:
  eMSG(_error_grid_);
eX_214:
  eMSG(_anemometer_required_);
eX_215:
  eMSG(_anemometer_outside_);
eX_240:
  eMSG(_no_absolute_value_$_, s);
eX_241:
  eMSG(_too_many_points_);
eX_242:
  eMSG(_invalid_lc_);
eX_250:
  eMSG(_no_valid_HM_in_$_, "DMNA");
eX_251:
  eMSG(_no_valid_RI_in_$_, "DMNA");
eX_260:
  eMSG(_no_valid_HM_in_$_, "AKTerm");
eX_261:
  eMSG(_no_valid_RI_in_$_, "AKTerm");
eX_300:
  eMSG(_invalid_dmkp_);
eX_312:
  eMSG(_duplicate_definition_);
eX_320:
  eMSG(_no_raster_$_, TI.bf);
eX_321:
  eMSG(_raster_dimension_);
eX_327:
  eMSG(_raster_integer_);
  eX_322:
  eMSG(_raster_dz_);
eX_323: eX_324: eX_325: eX_326:
  eMSG(_raster_x0_);
eX_400: eX_401:
  eMSG(_vertical_grid_size_);
eX_404:
  eMSG(_vertical_grid_low_);
eX_405:
  eMSG(_invalid_nz_);
eX_406:
  eMSG(_small_nz_);
eX_407:
  eMSG(_finest_grid_low_);
eX_408:
  eMSG(_coarse_grid_low_);
}

//================================================================== TipMain
//
int TipMain( char *s ) {
  dP(TipMain);
  char fname[512];
  char sttname[32];
  int n, pid;
  static int create_lib=0;
  if (!MsgFile) {
    if (TipMsgFile)  MsgFile = TipMsgFile;
    MsgVerbose = 1;
    MsgBreak = '\'';
  }
  if (CHECK) vMsg("TipMain(%s)", (s) ? s : "NULL");
  if (s && *s) {
    if (s[0] == '-')
      switch (s[1]) {
      		/* 2014-01-21 removed as it may result in internal parsing inconsistencies     		 
        case 'A': if (!strcmp(s+2, "german") || !strcmp(s+2, "C"))  //-2008-10-17
                    strcpy(Locale, s+2);
                  break;
        */
        case 'H': strcpy(Home, s+2);
                  if (!*Home)  strcpy(Home, ".");
                  break;
        case 'l': create_lib = 1;
                  sscanf(s+2, "%d", &create_lib);
                  if (create_lib > 0)  TalMode |=  TIP_LIBRARY;
                  else                 TalMode &= ~TIP_LIBRARY;
                  break;
        case 'M': sscanf(s+2, "%x", &PrmMode);                    //-2014-06-26
                  break;
        case 'v': sscanf(s+2, "%d,%d", &StdDspLevel, &StdLogLevel); //-2006-10-20
                  break;
        case 'w': Write_l = 1;
                  sscanf(s+2, "%d", &Write_l);
                  if (Write_l)  TalMode |=  TIP_WRITE_L;
                  else          TalMode &= ~TIP_WRITE_L;
                  break;
        case 'x': Xflags = 0;                                     //-2008-08-08
                  sscanf(s+2, "%x", &Xflags);
                  break;
        case 'z': Write_z = 1;
                  create_lib = -1;                          //-2005-03-18
                  sscanf(s+2, "%d", &Write_z);
                  if (Write_z)  TalMode |=  TIP_WRITE_Z;
                  else          TalMode &= ~TIP_WRITE_Z;
                  break;
        default:  ;
      }
    else {
      if (strlen(s) > 240)                                    eX(20);
      strcpy(Path, s);
      MsgCheckPath(Path);
    }
    return 0;
  }
  //                                                                -2011-12-07
  sprintf(fname, "%s/%s", Home, (CfgWet) ? AUSTALN : AUSTAL);     //-2014-02-19
  TipLogCheck("AUSTAL", TutGetCrc(fname));
  sprintf(fname, "%s/%s", Home, TALDIA);
  TipLogCheck("TALDIA", TutGetCrc(fname));
  sprintf(fname, "%s/%s", Home, VDISP);
  TipLogCheck("VDISP", TutGetCrc(fname));
  //
  strcpy(sttname, (CfgWet) ? "austal2000n" : "austal2000");       //-2014-01-21
  sprintf(fname, "%s/%s.settings", Home, sttname);
  n = TutGetCrc(fname);
  TipLogCheck("SETTINGS", n);
  sprintf(fname, "%08x", n);
  if (strcmp(fname, (CfgWet) ? SETTINGS_A2KN_CRC : SETTINGS_CRC)) //-2014-01-21
  		vMsg(_nostandard_settings_$_, sttname);
  //
  n = SttRead(Home, sttname);                         //-2011-11-23 -2014-01-21
  if (n < 0)
    return -1;
  //
  TipInitialize();
  n = TipRead(Path);
  if (n < 0)
    return -1;
  if ((*TI.bf || TI.xb || *TI.gh) && create_lib>=0) {    // complex terrain
    sprintf(fname, "%s/lib", Path);
    if (TutDirExists(fname)) {
      if (TalMode & TIP_LIBRARY) {
        n = TutDirClearOnDemand(fname);  if (n < 0)           eX(10);
      }
      else vMsg(_using_library_);
    }
    else {
      n = TutDirMake(fname);  if (n < 0)                      eX(11);
      create_lib = 1;
    }
    if (create_lib > 0) {         // make library
      char gname[256] = "\"";
      char gpath[256] = "\"";
      char *lan, lanopt[256];                                     //-2008-12-19
      fflush(stdout);
      if (*NlsLanOption) {                                        //-2008-12-19
        sprintf(lanopt, "--language=%s", NlsLanOption);
        lan = lanopt;
      }
      else                                                        //-2008-12-19
        lan = NULL;
      sprintf(fname, "%s/%s", Home, TALDIA);	          //-2002-07-03
      strcat(gname, fname);
      strcat(gname, "\"");
      strcat(gpath, Path);
      strcat(gpath, "\"");
#ifdef __linux__
      n = fork();
      if (n == 0)  execl(fname,
          fname, Path, "-B~../lib", "-w30000", lan, NULL);        //-2008-12-19
      if (n != -1)  pid = wait(&n);
#else
      n = spawnl(P_WAIT, fname,
          gname, gpath, "-B~../lib", "-w30000", lan, NULL);       //-2008-12-19
#endif
      if (n) {
        sprintf(fname, "%s/lib", Path);                 //-2006-11-06
        TutDirClear(fname);
        TutDirRemove(fname);
        if (n > 0)                              eX(12);
                                                eX(13);
      }
      if (TalMode & TIP_LIBRARY) {
        vMsg(_library_created_);
        return 1;
      }
    }
  }
  else if (*TI.bf==0 && !TI.xb && *TI.gh==0 && create_lib>0)
    return 2;
  if (Write_l) {
    strcpy(fname, Path);
    strcat(fname, "/work");
    n = TutDirMake(fname);  if (n < 0)          eX(1);
    if (n > 0 && (Xflags & 0x20) == 0) {                          //-2008-08-08
      n = TutDirClear(fname);  if (n < 0)       eX(2);
    }
  }
  n = TipCheck(Path, Write_z);
  fprintf(MsgFile, "\n%s", LogCheck.s);
  if (n < 0)
    return -1;
  if (n > 0)
    return  1;                                   //-2002-03-28
  if (Write_z) {                                           //-2001-05-09 lj
    if (!nVarParm) vMsg(_no_variable_parameters_);
    return 1;
  }
  if (Write_l) {
    n = TdfWrite(Path, &TIPary, TipFrmTab, TalMode);
    if (n < 0)                                 eX(4);     //-2007-02-03
    if (n>=0 && (*TI.gh>0)) {
      n = TdfCopySurface(Path);   if (n <= 0)  eX(3);     //-2001-09-15
    }
  }
  if (TipFrmTab)  FREE(TipFrmTab);
  TipFrmTab = NULL;
  return 0;
eX_1:
  eMSG(_directory_$_not_created_, fname);
eX_2:
  eMSG(_not_cleared_$_, fname);
eX_3:
  eMSG(_missing_profiles_);
eX_4:
  eMSG(_cant_write_defs_);
eX_10:
  eMSG(_library_not_cleared_);
eX_11:
  eMSG(_libdir_not_created_);
eX_12:
  eMSG(_library_not_created_$_, n);
eX_13:
  eMSG(_taldia_not_run_);
eX_20:
  eMSG(_pathname_too_long_);
}

#ifdef MAIN  //##########################################################
//================================================================== main
int main( int argc, char *argv[] ) {
  char lfile[256];
  int n;
  if (argc > 1) {
    for (n=1; n<argc; n++)  TipMain(argv[n]);
    strcpy(lfile, Path);
    strcat(lfile, "/LasTal.log");
    MsgFile = fopen(lfile, "w");
  }
  MsgVerbose = 1;
  MsgBreak = '\'';
  vMsg("LasTal Version %s %s", TalInpVersion, IBJstamp(__DATE__, __TIME__));
  if (argc < 2) {
    vMsg("usage: LasTal <path> {-z | -w}");
    exit(0);
  }
  n = TipMain(NULL);
  if (n < 0) vMsg("Program LasTal aborted!");
  else       vMsg("Program LasTal finished.");
  if (MsgFile) fclose(MsgFile);
  return 0;
}
#endif  //#################################################################
/*=========================================================================

 history:

 2001-04-15 0.1.0 lj created
 2001-05-01 0.2.1 lj AKS
 2001-05-03 0.2.2 lj monitor points
 2001-05-04 0.2.3 lj format string of AKTerm
 2001-05-09 0.2.4 lj return(1) if option "-z"
 2001-05-11 0.2.5 lj write ha+d0 in wetter.def
 2001-05-24 0.2.6 lj definition of not emittable components
 2001-05-30 0.3.0 lj new definition of species names and components
 2001-06-09 0.5.0 lj logging of input file
 2001-06-27 0.5.3 lj ua represented as float
 2001-06-28 0.5.4 lj lm instead of kl in zeitreihe.dmna
 2001-06-29 0.5.5 lj hq<10 allowed (warning only), nh3 added
 2001-07-09 0.6.0 lj release candidate
 2001-07-12 0.6.1 lj set Lm=0 for missing data
 2001-07-14 0.6.2 lj check on missing zeitreihe.dmna
 2001-07-20 0.6.3 lj determination of z0 using CORINE
 2001-08-03 0.6.4 lj parameter TI.kp (for nzdos)
 2001-08-14 0.7.0 lj conversion of Gauß-Krüger coordinates
 2001-09-04 0.7.1 lj MsgUnquote() instead of MsgDequote() for input strings
                     calculation of source center corrected
 2001-09-17 0.8.0 lj working with complex terrain
 2001-10-18 0.8.1 lj computational area at least 2000 m squared
 2001-10-30 0.8.2 lj nested grids
 2001-11-02 0.8.3 lj component nox
 2001-11-03 0.8.4 lj grid alignment relaxed
 2001-11-05 0.9.0 lj release candidate
 2001-11-23 0.9.1 lj absolute path for "gh" allowed
 2001-12-14 0.9.2 lj time interval length checked
 2002-01-03 0.10.2 lj default value of sq[] = -1
 2002-01-12 0.10.4 lj check anemometer position (and other positions)
 2002-02-26 0.10.5 lj counting of monitor points corrected
 2002-03-11 0.11.1 lj new input parameter : mixing height hm
 2002-03-28 0.11.2 lj return from input check corrected
 2002-04-16 0.12.0 lj ha not incremented by d0
 2002-07-03 0.13.1 lj modifications for linux
 2002-07-13 0.13.2 lj calculation of nested grids corrected
 2002-07-30 0.13.3 lj number of decimals for deposition of pm = 4
 2002-08-01 0.13.4 lj time dependent hm; hh requires NOSTANDARD
 2002-08-15 0.13.5 lj setting of x0 and y0 checked
 2002-09-21 0.13.6 lj checking of nested grids improved
 2002-09-23 0.13.7 lj input parameters lq, rq, tq
 2002-09-24 1.0.0  lj final release candidate
 2002-12-06 1.0.3  lj check of Np (<= 10)
 2002-12-10 1.0.4  lj parameter lq, rq, tq allowed to vary with time
 2003-01-22 1.0.5  lj blanks in file names allowed
 2003-02-21 1.1.1  lj optional scientific notation, species "xx"
 2003-06-24 1.1.7  lj error message corrected
 2003-07-07 1.1.8  lj localisation
 2003-10-02 1.1.11 lj check of inclusion of finer grids corrected
                   lj nesting of grids corrected, check added
 2003-10-12 1.1.12 lj don't "make" a surface if zg00 is given
 2003-12-05 1.1.13 lj check on length of time series (<=MNT_NUMVAL) added
 2004-07    2.1.1  uj buildings included
 2004-09-10 2.1.2  lj check of grid nesting
 2004-10-25 2.1.4  lj guard cells for grids (1 for outer, 2 for inner)
                   lj z0 from CORINE always rounded
                   lj optional factors Ftv
 2004-11-02 2.1.5  uj min dd without buildings changed from 15m to 16m
 2004-11-12 2.1.6  uj additional check included in getNesting()
 2004-11-30 2.1.8  uj additional check included in getNesting()
                      explicit reference to TA Luft for warning h < 1.2hq
 2004-12-14 2.1.9  uj wb, da in degree
 2005-01-10 2.1.10 uj checkBodiesandSources: hq min. 10 m for distance
 2005-02-21 2.1.11 uj unused makePlainSurface() removed
 2005-03-16 2.1.12 uj check xa/ya moved, check TrbExt corrected
 2005-03-17 2.2.0  uj version number upgrade
 2005-03-18        lj no wind lib created with option -z
 2005-04-13 2.2.1  lj option -z for flat terrain corrected
 2006-02-06 2.2.7  uj nonstandard option SORRELAX
 2006-02-11        uj check if user-defined nz extends to hh_max
 2006-02-15 2.2.9  lj freeing of strings
 2006-10-11 2.2.15 lj TI.im defaults to 200
 2006-10-12 2.2.16 uj number of monitor points increased to 20
                      extension to UTM coordinates
 2006-10-20 2.2.17 uj verbose modus
 2006-10-20 2.3.0  lj externalization of strings
 2006-11-06        uj clear lib corrected
 2006-11-21 2.3.3  uj write out GK-converted source coordinates
 2007-02-03 2.3.5  uj time interval revised, error if WriteDefs fails
                      maximum np adjustable from outside
 2007-03-08        uj minimum h for z0 calculation
 2008-01-25 2.3.7  uj allow absolute path for rb
 2008-03-10 2.4.0  lj evaluation of rated odor frequencies
 2008-04-17 2.4.1  lj merged with 2.3.x
 2008-08-27 2.4.3  lj NONSTANDARD optionally
 2008-09-23        lj 2 additional odor substances
 2008-10-17        lj uses MsgSetLocale()
 2008-10-20 2.4.4  lj nonstandard: hm, ie, im, mh, x1, x2, x3, y1, y2, y3
 2008-12-04 2.4.5  lj lq actually treated as variable parameter
                      writes "artp" into header of time series
 2008-12-07        lj UTM uses zone 32 if ux has no prefix
 2008-12-11        lj parameter "ggcs" in NTRREC
 2011-06-17 2.4.10 uj maximum stack height increased to 500
 2011-06-29        uj check and report if standard roughness register is used
 2011-07-07 2.5.0  uj version number upgrade
 2011-09-12 2.5.1  uj TipBlmVersion
 2011-11-23 2.6.0  lj settings from ARTM
 2011-12-07        lj check sums
 2011-12-14        lj "ri" requires NOSTANDARD
 2011-12-16        lj abort if required option NOSTANDARD is missing
 2012-04-06 2.6.3  uj complain if NOSTANDARD is missing for "hh"
 2012-10-30 2.6.5  uj read/write superposition factors
 2013-01-11        uj invalid umin parsing removed
 2014-01-21 2.6.9  uj CfgWet
 2014-06-26 2.6.11 uj accept "ri" if called by TALdia, eX/eG adjusted
 ==========================================================================*/
