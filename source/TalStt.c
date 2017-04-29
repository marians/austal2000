//============================================================ TalStt.c
//
// Read Settings for ARTM, re-imported to A2K
// ==========================================
//
// Copyright (C) Umweltbundesamt, Dessau-Roﬂlau, Germany, 2002-2005
// Copyright (C) Janicke Consulting, 88662 ‹berlingen, Germany, 2002-2011
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
// last change: 2011-12-14 lj
//
//==========================================================================

char *TalSttVersion = "2.6.0";
static char *eMODn = "TalStt";
static int CHECK = 0;

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <math.h>
#include <locale.h>

#include "IBJmsg.h"
#include "IBJary.h"
#include "IBJtxt.h"
#include "IBJdmn.h"
#include "TalStt.h"

typedef struct {
  char *_name;      // parameter name + buffer for tokens
  int nTokens;      // number of tokens
  char **_tokens;   // array of pointers to tokens
  double *_dd;      // array of values or NULL
} INPREC;

char **SttCmpNames; // names of emittable components
STTSPCREC *SttSpcTab;     // species defined 
STTSPCREC SttAstlDefault;
STTSPCREC SttArtmDefault;
int SttSpcCount, SttCmpCount, SttTypeCount, SttMode;
char *SttGroups[6] =  { "gas", "gas", "gas", "pm3", "pm4", "pmu" };
char *SttGrpXten[6] = {    "",  "-1",  "-2",  "-3",  "-4",  "-u" };
char SttRiSep[256] = "NIEDERSCHLAG:";                               //_2011-12-06
double SttVdVec[6] = { 0, 0.001, 0.01, 0.05, 0.20, 0.07 };          //-2005-09-12
double SttVsVec[6] = { 0, 0.000, 0.00, 0.04, 0.15, 0.06 };          //-2005-09-12
double SttWfVec[6] = { 0, 1.0e-5, 2.0e-5, 3.0e-5, 4.0e-5, 5.0e-5 }; //-2005-09-12
double SttWeVec[6] = { 0, 0.6, 0.6, 0.6, 0.6, 0.6 };                //-2005-04-12
double SttRiVec[6] = { 0, -1, -1, -1, -1, -1 };                     //-2011-12-06

double SttNoxTimes[6] = {  2.9, 2.5, 1.9, 1.3, 0.9, 0.3 };
double SttHmMean[6] = { 0, 0, 0, 800, 1100, 1100 };
double SttOdorThreshold = 0.25;
int SttNstat = 1;

double SttGmmB1 = 1.00;
double SttGmmB2 = 0.14286;
char SttGmmUnit[16] = "Bq/m≤";
double SttGmmFd = 4;
double SttGmmFe = 3;
double SttGmmFf = 10;
double SttGmmFr = 10;
double SttGmmMue = 0.0082;
double SttGmmRef = 1.e-6;

double SttGmmMu[2] = { 7.78e-3, 1.82e-2 };
double SttGmmDa[2][6] = {
  { 1.0, 0.77, 0.35, -4.00e-2, 3.20e-3, -8.20e-5 },
  { 1.0, 1.92, 1.74, -3.39e-2, 3.86e-2, -2.11e-3 }
};
double SttGmmBk[2][4][4] = {
  { {  0.485,   0.064,   1.705,  -1.179 },
    {  0.137,   1.878,  -4.817,   2.883 },
    { -0.0035, -0.8569,  2.0527, -1.2552 },
    { -0.0018,  0.0997, -0.2392,  0.1503 } },
  { {  0.279,   0.595,  -0.205,   0.622 },
    {  0.135,   0.866,  -0.716,  -0.578 },
    { -0.0131, -0.324,   0.1103,  0.2892 },
    {  0.0003,  0.0313, -0.0017, -0.0337 } }
};

static char cbreak = '#';
static int scanning = 0;

#define buflen 256

static void trim(char *buf) {
    int n, i, quote=0;
    if (!buf) return;
    n = strlen(buf);
    for (i=0; i<n; i++) {
        if (buf[i] == '"') {
            if (i > 0 && buf[i-1] == '\\') continue;
            quote = (quote == 0);
            continue;
        }
        if (quote) continue;
        if (buf[i] == cbreak) {
            buf[i] = 0;
            n = i;
            break;
        }
    }
    for (i=n-1; i>=0; i--) {
        if (buf[i] <= ' ')  buf[i] = 0;
        else  break;
    }
}

static int starts_with(char *source, char *s) {
    int rc;
    if (!source || !s)  return 0;
    rc = (0 == strncmp(source, s, strlen(s)));
    return rc;
}

static int ends_with(char *source, char *s) {
    int rc, l;
    if (!source || !s)  return 0;
    l = strlen(source) - strlen(s);
    if (l < 0)  return 0;
    rc = (0 == strcmp(source+l, s));
    return rc;
}

//================================================================= parse
static int parse( char *line, INPREC *pp ) {
  dP(parse);
  int i, n, l;
  int is_number = 1;
  char *p1, c, tk[256];
  if (!pp)                                    eX(2);
  pp->_name = NULL;
  pp->nTokens = 0;
  pp->_tokens = NULL;
  pp->_dd = NULL;
  if (line==NULL || !*line)
    return 0;
  l = strlen(line);
  pp->_name = ALLOC(l+1);  if (!pp->_name)    eX(1);
  strcpy(pp->_name, line);
  for (p1=pp->_name; isalnum(*p1) || strchr("()", *p1); p1++);
  if (!*p1)
    return 0;
  *p1++ = 0;
  if (!*pp->_name)
    return 0;
  pp->_tokens = _MsgTokens(p1, " ;\t\r\n=");
  if (!pp->_tokens)
    return 0;
  for (n=0; ; n++)
    if (!pp->_tokens[n])
      break;
  if (n == 0)
    return 0;
  pp->nTokens = n;
  for (i=0; i<n; i++) {
    c = *pp->_tokens[i];
    if (!isdigit(c) && c!='-' && c!='+')
      is_number = 0;
  }
  if (is_number) {
    char *tail;
    pp->_dd = ALLOC(n*sizeof(double));
    for (i=0; i<n; i++) {
      strncpy(tk, pp->_tokens[i], 255);                         //-2003-07-07
      tk[255] = 0;
        for (p1=tk; (*p1); p1++)  if (*p1 == ',')  *p1 = '.';   //-2003-07-07
        pp->_dd[i] = strtod(tk, &tail);
        if (*tail) {
          FREE(pp->_dd);
          pp->_dd = NULL;
          is_number = 0;
          break;
        }
    }
  }
  if (!is_number) {
    for (i=0; i<n; i++)
      MsgUnquote(pp->_tokens[i]);                                 //-2001-09-04
  }
  return is_number;
eX_1: eX_2:
  eMSG("Internal error!");
}

static char *set_string(char *s, INPREC inp) {
    static char msg[256];
if (CHECK) vMsg("setting string %s", inp._name);
    if (inp.nTokens < 1) {
        sprintf(msg, "no string data for %s", inp._name);
        return msg;
    }
    if (!inp._tokens) {
        sprintf(msg, "missing string for %s", inp._name);
        return msg;
    }
    if (strlen(inp._tokens[0]) > 15) {
        sprintf(msg, "string too long for %s", inp._name);
        return msg;
    }
    strcpy(s, inp._tokens[0]);
    return NULL;
}

static char *set_float(float *pf, INPREC inp) {
    static char msg[256];
if (CHECK) vMsg("setting float %s", inp._name);
    if (inp.nTokens < 1) {
        sprintf(msg, "no float data for %s", inp._name);
        return msg;
    }
    if (!inp._dd) {
        sprintf(msg, "missing float for %s", inp._name);
        return msg;
    }
    *pf = (float)inp._dd[0];
    return NULL;
}

static char *set_doubles(double *pf, INPREC inp, int n) {
    static char msg[256];
    int i;
if (CHECK) vMsg("setting doubles %s", inp._name);
    if (!inp._dd) {
        sprintf(msg, "missing doubles for %s", inp._name);
        return msg;
    }
    if (n > inp.nTokens)  n = inp.nTokens;
    for (i=0; i<n; i++)
        pf[i] = inp._dd[i];
    return NULL;
}

static char *set_integer(int *pi, INPREC inp) {
    static char msg[256];
if (CHECK) vMsg("setting integer %s", inp._name);
    if (inp.nTokens < 1) {
        sprintf(msg, "no integer data for %s", inp._name);
        return msg;
    }
    if (!inp._dd) {
        sprintf(msg, "missing integer for %s", inp._name);
        return msg;
    }
    *pi = (int)inp._dd[0];
    return NULL;
}

#define TRY_STRING(a) if (!strcmp(inp._name, #a))  msg = set_string(pr->a, inp)
#define TRY_FLOAT(a) if (!strcmp(inp._name, #a))  msg = set_float(&(pr->a), inp)
#define TRY_INTEGER(a) if (!strcmp(inp._name, #a))  msg = set_integer(&(pr->a), inp)
#define TRY_DOUBLES(a,b,c) if (!strcmp(inp._name, #a))  msg = set_doubles((b), inp, (c))
#define TRY_TEXT(a,b) if (!strcmp(inp._name, #a))  msg = set_string((b), inp)
static char *analyse_spec(INPREC inp, STTSPCREC *pr) {
    char *msg = NULL;
    if (CHECK) vMsg("analyse spec: name=%s, section=%s", inp._name, pr->name);
    TRY_STRING(grps);
    else TRY_STRING(unit);
    else TRY_FLOAT(vd);
    else TRY_FLOAT(wf);
    else TRY_FLOAT(we);
    else TRY_FLOAT(de);
    else TRY_FLOAT(fr);
    else TRY_FLOAT(fc);
    else TRY_STRING(uc);
    else TRY_FLOAT(fn);
    else TRY_STRING(un);
    else TRY_FLOAT(ry);
    else TRY_INTEGER(dy);
    else TRY_INTEGER(nd);
    else TRY_FLOAT(rd);
    else TRY_INTEGER(dd);
    else TRY_INTEGER(nh);
    else TRY_FLOAT(rh);
    else TRY_INTEGER(dh);
    else TRY_FLOAT(rn);
    else TRY_INTEGER(dn);
    else msg = "unknown parameter";
    return msg;
}

static char *analyse_astl(INPREC inp) {
    char *msg = NULL;
    TRY_DOUBLES(odorthreshold, &SttOdorThreshold, 1);
    else TRY_DOUBLES(noxtimes, SttNoxTimes, 6);
    else msg = "unknown astl-parameter";
    return msg;
}

static char *analyse_artm(INPREC inp) {
    char *msg = NULL;
    TRY_TEXT(gmunit, SttGmmUnit);
    else TRY_DOUBLES(gmref, &SttGmmRef, 1);
    else TRY_DOUBLES(gmfd, &SttGmmFd, 1);
    else TRY_DOUBLES(gmfe, &SttGmmFe, 1);
    else TRY_DOUBLES(gmff, &SttGmmFf, 1);
    else TRY_DOUBLES(gmfr, &SttGmmFr, 1);
    else TRY_DOUBLES(gmmu10, &SttGmmMu[0], 1);
    else TRY_DOUBLES(gmmu01, &SttGmmMu[1], 1);
    else TRY_DOUBLES(gmda10, SttGmmDa[0], 6);
    else TRY_DOUBLES(gmda01, SttGmmDa[1], 6);
    else TRY_DOUBLES(gmbk10(0), SttGmmBk[0][0], 4);
    else TRY_DOUBLES(gmbk10(1), SttGmmBk[0][1], 4);
    else TRY_DOUBLES(gmbk10(2), SttGmmBk[0][2], 4);
    else TRY_DOUBLES(gmbk10(3), SttGmmBk[0][3], 4);
    else TRY_DOUBLES(gmbk01(0), SttGmmBk[1][0], 4);
    else TRY_DOUBLES(gmbk01(1), SttGmmBk[1][1], 4);
    else TRY_DOUBLES(gmbk01(2), SttGmmBk[1][2], 4);
    else TRY_DOUBLES(gmbk01(3), SttGmmBk[1][3], 4);
    else msg = "unknown artm-parameter";
    return msg;
}

static char *analyse_system(INPREC inp) {
    char *msg = NULL;
    TRY_DOUBLES(vdvec, SttVdVec+1, 5);
    else TRY_DOUBLES(vsvec, SttVsVec+1, 5);
    else TRY_DOUBLES(wfvec, SttWfVec+1, 5);
    else TRY_DOUBLES(wevec, SttWeVec+1, 5);
    else TRY_DOUBLES(rivec, SttRiVec, 6);                         //-2011-12-06
    else TRY_DOUBLES(hmmean, SttHmMean, 6);
    else if (!strcmp(inp._name, "risep"))                         //-2011-12-06
      msg = set_string(SttRiSep, inp);
    else msg = "unknown system-parameter";    
    return msg;
}

#undef TRY_STRING
#undef TRY_FLOAT
#undef TRY_DOUBLES
#undef TRY_INTEGER
#undef TRY_TEXT


static int read_section(FILE *f, char *buf, int nn, STTSPCREC *pr) {
    dP(read_section);
    int n=0, l, i;
    char *pc, *msg, name[16];
    INPREC inp;
    if (f==NULL || buf==NULL)                               eX(1);
    l = strlen(buf);
    if (l < 2 || l > 17)                                    eX(2);
    strncpy(name, buf+1, l-2);
    MsgLow(name);
    name[l-2] = 0;
    if (pr) {
      if (SttMode) {
          memcpy(pr, &SttArtmDefault, sizeof(STTSPCREC));
      }
      else {
          memcpy(pr, &SttAstlDefault, sizeof(STTSPCREC));
      }
      strcpy(pr->name, name);
    }
    // pr->wf = -1;
    // pr->wc = -1;
    while(1) {
      *buf = 0;
      pc = fgets(buf, buflen, f);
      if (pc == NULL)
        break;
      n++;
      if (strlen(buf) >= buflen-2)              eX(3);
      trim(buf);
      if (!*buf)
        continue;
      if (*buf == '[')
        break;
      if (0 > parse(buf, &inp))                 eX(4);
      if (CHECK) vMsg("> %s", buf);                     
      MsgLow(inp._name);
      if (pr == NULL) {                                       //-2005-10-26
        msg = analyse_system(inp);
      }
      else {
        msg = analyse_spec(inp, pr);
        if (msg && starts_with(msg, "unknown parameter")) {
          if (!strcmp(pr->name, ".astl"))
            msg = analyse_astl(inp);
          else if (!strcmp(pr->name, ".artm"))
            msg = analyse_artm(inp);
        }
      }
      FREE(inp._name);
      FREE(inp._tokens);
      if (inp._dd != NULL) {
        FREE(inp._dd);
        inp._dd = NULL;
      }
      if (msg && *msg) {
        vMsg("error in line %d: %s\n%s", n+nn, buf, msg);
        scanning = 1;
      }
    }
    //
    if (pr) {
      if (!strncmp(pr->name, "no", 2))  pr->de = 0;               //-2005-09-01
      if (SttMode) {
          if (strcmp(pr->unit, "Bq"))                 eX(5);
      }
      else {
          if (!strcmp(pr->unit, "Bq"))                eX(6);
      }
    }
    else {  // .system                                            //-2011-12-06
      for (i=0; i<6; i++)
        if (SttRiVec[i] < 0)
          break;
      SttNstat = i;
      if (!SttNstat)                                  eX(7);
    }
    //
    return n;
eX_1: eX_2:
    eMSG("improper argument");
eX_3:
    eMSG("buffer overflow in line %d", n+nn);
eX_4:
    eMSG("parse error in line %d", n+nn);
eX_5:
    eMSG("unit must be \"Bq\" in artm-mode");
eX_6:
    eMSG("unit \"Bq\" allowed in artm-mode only");
eX_7:
    eMSG("no valid values in vector of rain intensities RiVec");
}

//=============================================================== SttGetSpecies
STTSPCREC *SttGetSpecies( // get the species definition record
char *name)               // species name
{
  STTSPCREC *pr;
  int i;
  if (name == NULL)  return NULL;
  pr = NULL;
  for (i=0; i<SttSpcCount; i++) {
    pr = SttSpcTab + i;
    if (!strcmp(pr->name, name))
      break;
    pr = NULL;
  }
  return pr;
}

//===================================================================== SttRead
int SttRead(     // read the settings for AUSTAL2000
char *path,      // home directory of AUSTAL2000
char *pgm )      // program name
{
  dP(SttRead);
  FILE *f;
  int n=0, ns=0, m=0, i=0, k, k1, k2;
  char buf[buflen], name[256], sec[buflen], *pc, **ppc;
  STTSPCREC *pr;
  if (CHECK) vMsg("SttRead(%s, %s) ...", path, pgm);
  *buf = 0;
  *sec = 0;
  SttMode = (0 == strncmp(pgm, "artm", 4));
  sprintf(name, "%s/%s.%s", path, pgm, "settings");
  //
  f = fopen(name, "rb");
  if (!f) {
    vMsg("can't read file \"%s\"!", name);
    exit(2);
  }
  //
  // scan the file
  //
  n = 0;
  ns = 0;
  while (fgets(buf, buflen, f)) {
    n++;
    if (strlen(buf) >= buflen-2)            eX(1);
    if (buf[0] != '[')
      continue;
    if (buf[1] == '.')
      continue;
    ns++;
  }
  if (CHECK) vMsg("%d species definitions found", ns);
  SttSpcTab = ALLOC((ns+1)*sizeof(STTSPCREC));
  SttSpcCount = ns;
  //
  // read the file
  //
  fseek(f, 0, SEEK_SET);
  n = 0;
  ns = 0;
  *buf = 0;
  while (1) {
    if (!*buf) {
      pc = fgets(buf, buflen, f);
      if (pc == NULL)
        break;
      n++;
      if (strlen(buf) >= buflen-2)            eX(2);
      trim(buf);
      if (!*buf)
        continue;
    }
    if (*buf != '[')
      continue;
    strcpy(sec, buf);
    if (sec[1] == '.') {
        if      (!strcmp(sec, "[.astl]")) {
            if (SttMode)                                  eX(10);
            pr = &SttAstlDefault;
        }
        else if (!strcmp(sec, "[.artm]")) {
            if (!SttMode)                                 eX(11);
            pr = &SttArtmDefault;
        }
        else if (!strcmp(sec, "[.system]")) {                   //-2005-10-26
            pr = NULL;
        }
        else                                              eX(4);
    }
    else {
        pr = SttSpcTab + ns;
        ns++;
    }
    m = read_section(f, buf, n, pr);
    if (m < 0)                                  eX(3);
    n += m;
  }
  fclose(f);  
  if (scanning)                                 eX(5);
  //
  // find all species names
  //
  n = 0;
  for (i=0; i<SttSpcCount; i++) {
    pr = SttSpcTab + i;
    if (!*(pr->grps)) {
      strcpy(pr->grps, "0-0");
      k1 = 0;
      k2 = 0;
    }
    else {
      pc = pr->grps;
      if (strlen(pc) != 3 || !isdigit(pc[0]) || pc[1] != '-'
          || !isdigit(pc[2]))                                           eX(6);
      k1 = pc[0] - '0';
      k2 = pc[2] - '0';
      if (k1 < 0 || k2 > 5 || k1 > k2)                                  eX(7);
    }
    n += k2 - k1 + 1;
    // check consistence of deposition parameters                   -2011-12-13
    if (pr->rn > 0) {     // logging of deposition requested
      if (k2 == 0 && pr->vd <= 0 && pr->wf <= 0)                        eX(12);
    }
    else {                // deposition not logged
      if (k2 > 0 || pr->vd > 0 || pr->wf > 0)                           eX(13);
    }
  }
  if (CHECK) vMsg("%d components found", n);
  SttCmpCount = n;
  SttCmpNames = ALLOC((n+1)*(16 + sizeof(char*)));
  ppc = SttCmpNames;
  pc = (char*)(&ppc[n+1]);
  for (i=0; i<n; i++)
      ppc[i] = pc + i*16;
  ppc[n] = NULL;
  n = 0;
  for (i=0; i<SttSpcCount; i++) {
      pr = SttSpcTab + i;
      k1 = pr->grps[0] - '0';
      k2 = pr->grps[2] - '0';
      for (k=k1; k<=k2; k++) {
          sprintf(SttCmpNames[n], "%s%s", pr->name, SttGrpXten[k]);
          n++;
      }
  }
  if (CHECK) for (i=0; i<SttCmpCount; i++)
      printf("%3d %s\n", i+1, SttCmpNames[i]);
  
  return ns;
eX_1: eX_2:
  eMSG("buffer overflow at line %d!", n);
eX_3:
  eMSG("read error at section %s (line %d)!", sec, n);
eX_4:
  eMSG("unknown section type %s (line %d)!", sec, n);
eX_5:
  eMSG("can't read settings!");
eX_6: eX_7:
  eMSG("groups \"%s\" invalid for species %s", pr->grps, pr->name);
eX_10: 
  eMSG("section [.astl] not allowed in artm-mode!");
eX_11: 
  eMSG("section [.artm] not allowed in astl-mode!");
eX_12: eX_13:
  eMSG("inconsistent deposition parameters for species %s!", pr->name);
}

#ifdef MAIN  //##########################################################
static char Path[256];
//================================================================== main
int main( int argc, char *argv[] ) {
  char lfile[256];
  int n;
  if (argc < 2) {
      printf("usage: TstStt <path>\n");
      exit(0);
  }
  strcpy(Path, argv[1]);
  strcpy(lfile, Path);
  strcat(lfile, "/TstStt.log");
  MsgFile = fopen(lfile, "w");
  MsgVerbose = 1;
  MsgBreak = '\'';
  vMsg("TstStt (%s %s)",  __DATE__, __TIME__);
  n = SttSpcRead(Path, 1);
  if (n < 0) vMsg("Programm TstStt wegen eines Fehlers abgebrochen!");
  else       vMsg("Programm TstStt normal beendet.");
  if (MsgFile) fclose(MsgFile);
  return 0;
}
#endif  //#################################################################
//=========================================================================
//
// History:
//
// 2005-09-12  2.3.2  lj vs, vd in m/s; wf in 1/s
// 2011-11-23  2.6.0  lj adapted from ARTM
// 2011-12-06         lj rain intensities added
// 2011-12-14         lj check of deposition parameters
//
//==========================================================================

