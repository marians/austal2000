// ======================================================== austal2000.c
//
// Dispersion model AUSTAL2000
// ===========================
//
// Copyright (C) Umweltbundesamt, Dessau-Roﬂlau, Germany, 2002-2013
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

#ifdef AUSTAL2000N                                                //-2014-01-21
static char *eMODn = "AUSTAL2000N";
#else
static char *eMODn = "AUSTAL2000";
#endif

#include <math.h>
#include <signal.h>
#include <errno.h>
#include <time.h>
#include <stdlib.h>
#include <locale.h>

#ifdef __linux__
  #include <unistd.h>
#endif

#define  STDDSPLEVEL  -1
#define  STDLOGLEVEL   1
#define  STDMYMAIN    TalMain
#define  STDLOGOPEN   "a"
#define  STDARG       1

#include "IBJmsg.h"
#include "IBJary.h"

#include "genutl.h"
#include "genio.h"
#include "TalRnd.h"

#ifdef AUSTAL2000N                                                //-2014-01-21
#include "austal2000n.nls"
#define STDNLS "A2KN"
#else
#include "austal2000.nls"
#define STDNLS "A2K"
#endif
#define NLS_DEFINE_ONLY
#include "IBJstd.h"
#include "IBJstd.nls"                                             //-2011-12-01

/*=========================================================================*/
#ifdef AUSTAL2000N                                                //-2014-01-21
STDPGM(austal2000n, TalServer, 2, 6, 11);
#else
STDPGM(austal2000,  TalServer, 2, 6, 11);
#endif
/*=========================================================================*/

#include "TalStt.h"                                               //-2011-11-23
#include "TalDos.h"
#include "TalDtb.h"
#include "TalGrd.h"
#include "TalIo1.h"
#include "TalMod.h"
#include "TalNms.h"
#include "TalPlm.h"
#include "TalPrm.h"
#include "TalPrf.h"
#include "TalPtl.h"
#include "TalSrc.h"
#include "TalTmn.h"
#include "TalWrk.h"
#include "TalVsp.h"
//-2004-11-30

#include "TalInp.h"
#include "TalDef.h"
#include "TalMtm.h"
#include "TalQtl.h"
#include "TalMon.h"
#include "TALuti.h"
#include "TalUtl.h"
#include "IBJstamp.h"
#include "TalHlp.h"
#include "TalCfg.h"

#include "austal2000.h"

static char DefName[120] = "param.def";
static char T1s[40], T2s[40], TalPath[256], *ClrPath;
static long T1, T2;

static int Xflags = 0x0f;                                         //-2008-07-22
// 1234|1234
// ..1.|....  : don't delete work/ at start
// ...1|....  : don't delete work/ at end
// ....|...1  : write def files
// ....|..1.  : do dispersion calculation
// ....|.1..  : write result files
// ....|1...  : evaluate results

  /*================================================================== TalInit
  */
long TalInit(           /* initialize server    */
  long flags,           /* action flags         */
  char *istr )          /* server options       */
{
  dP(TalInit);
  int vrb, dsp;
  char *jstr, *ps, s[200];
  if (StdStatus & STD_INIT)  return 0;
  if (istr) {
    jstr = istr;
    ps = strstr(istr, " -v");
    if (ps) sscanf(ps+3, "%d", &StdLogLevel);
    ps = strstr(istr, " -y");
    if (ps) sscanf(ps+3, "%d", &StdDspLevel);
    ps = strstr(istr, " -d");
    if (ps)  strcpy(DefName, ps+3);
    }
  else  jstr = "";
  StdStatus |= flags;
  vrb = StdLogLevel;
  dsp = StdDspLevel;
  lMSG(3,3)("TAL_%d.%d.%s (%08lx,%s)", StdVersion, StdRelease, StdPatch, flags, jstr);
  sprintf(s, " GRD -v%d -y%d -d%s", vrb, dsp, DefName);
  GrdInit(flags, s);                                                    eG(2);
  TmnVerify(IDENT(GRDpar,0,0,0), NULL, NULL, 0);                        eG(3);
  PrmServer(StdPath);
  DtbServer(StdPath);
  sprintf(s, " PRM -v%d -y%d -d%s", vrb, dsp, DefName);
  PrmInit(flags, s);                                                    eG(4);
  MI.vlevel = StdLogLevel;
  MI.dlevel = StdDspLevel;
  sprintf(s, " PRF -v%d -y%d -d%s", vrb, dsp, DefName);
  PrfInit(flags, s);                                                    eG(10);
  sprintf(s, " PTL -v%d -y%d -d%s", vrb, dsp, DefName);
  PtlInit(flags, s);                                                    eG(5);
  sprintf(s, " SRC -v%d -y%d -d%s", vrb, dsp, DefName);
  SrcInit(flags, s);                                                    eG(6);
  sprintf(s, " MOD -v%d -y%d -d%s", vrb, dsp, DefName);
  ModInit(flags, s);                                                    eG(7);
  sprintf(s, " WRK -v%d -y%d -p1 -d%s", vrb, dsp, DefName);             //-2001-07-10
  WrkInit(flags, s);                                                    eG(8);
  sprintf(s, " DOS -v%d -y%d -d%s", vrb, dsp, DefName);
  DosInit(flags, s);                                                    eG(9);
  sprintf(s, " DTB -v%d -y%d -d%s", vrb, dsp, DefName);
  DtbInit(flags, s);                                                    eG(11);
  if (MI.flags & FLG_WRTMOD)  ModServer("-w");
  NmsSeq("average", MI.average);
  if (MI.average > 0)  MI.cycind = 1 + (MI.cycind-1)*MI.average;
  NmsSeq("sequence", MI.cycind);
  StdStatus |= STD_INIT;
  return 0;
eX_2:  eX_3:  eX_4:  eX_5:  eX_6:  eX_7:  eX_8:  eX_9:  eX_10:
eX_11:
  eMSG(_internal_error_);
}

  /*================================================================= TalServer
  */
long TalServer(         /* server routine for A2K       */
  char *s )             /* calling option               */
  {
  dP(TalServer);
  static long status=0;
  static long pflags=0, nflags=0, print=0;
  char t[80];
  long mask0, mask1, idos0, idos1, idos2, iprf0, imod0;           //-2006-02-15
  long icon0, idtb0, ixtr0, isum0, igam0, iptl0, igrdp;
  //
  if (!strncmp(s, "-h", 2)) {                                     //-2008-08-12
    sprintf(TalVersion, "%d.%d.%s", StdVersion, StdRelease, StdPatch);
#ifdef MAKE_LABEL
    strcat(TalVersion, "-" MAKE_LABEL);
#endif
    printf("%s, %s %s %s [%s]\n", eMODn, _version_, TalVersion,
      IBJstamp(NULL, NULL), NlsLanguage);                         //-2008-08-27
#ifdef AUSTAL2000N
      CfgInit(1);
#else
      CfgInit(0);
#endif
    TalHelp(s+1);
    if (s[2] == 'l') {                                            //-2008-08-15
      printf("NLS:%s\n", CfgString());
    }
    StdStatus = 0;
    exit(0);
  }
  //
  if (StdArg(s))
    return 0;
  if (*s) {
    TipMain(s);
    if (*s == '-')  switch (s[1]) {                               //-2006-10-13
      case 'a': Xflags = 8;                                       //-2006-02-16
                break;
      case 'J': if (strchr(s+2, 't'))  pflags |= FLG_TRACING;     //-2002-01-07
                else                   nflags |= FLG_TRACING;
                if (strchr(s+2, 'e'))  pflags |= FLG_EXACTTAU;
                else                   nflags |= FLG_EXACTTAU;
                if (strchr(s+2, 'o'))  pflags |= FLG_ONLYADVEC;
                else                   nflags |= FLG_ONLYADVEC;
                if (strchr(s+2, 'n'))  pflags |= FLG_NOPREDICT;
                else                   nflags |= FLG_NOPREDICT;
                break;
      case 'l': StdStatus |= FLG_LIBRARY;
                break;
      case 'p': print = 1;                              //-2003-10-12
                PrfServer(s);
                break;
      case 'x': sscanf(s+2, "%x", &Xflags);      		        
                break;
      case 'z': TipMain(s);
                Xflags = 0;
                break;
      default:  ;
    }
    return 0;
  }
  if (0 == (StdStatus & STD_INIT)) {
    TalInit(StdStatus&STD_GLOBAL, " TAL");              eG(2);
  }
  MI.status |= status;
  MI.flags  |= pflags;
  MI.flags  &= ~nflags;
  status = 0;
  igrdp = IDENT(GRDpar, 0, 0, 0);
  icon0 = IDENT(CONtab, 0, 0, 0);
  idtb0 = IDENT(DTBarr, 0, 0, 0);
  idos0 = IDENT(DOStab, 0, 0, 0);
  idos1 = IDENT(DOStab, 1, 0, 0);
  idos2 = IDENT(DOStab, 2, 0, 0);
  igam0 = IDENT(GAMarr, 0, 0, 0);
  ixtr0 = IDENT(XTRarr, 0, 0, 0);
  iptl0 = IDENT(PTLarr, 0, 0, 0);
  isum0 = IDENT(SUMarr, 0, 0, 0);
  iprf0 = IDENT(PRFarr, 0, 0, 0);
  imod0 = IDENT(MODarr, 0, 0, 0);
  mask0 = ~(NMS_GROUP | NMS_GRIDN | NMS_LEVEL);
  mask1 = ~(NMS_GRIDN | NMS_LEVEL);
  TmnVerify(igrdp, NULL, NULL, 0);                      eG(4);
  if (Xflags < 0)   // drop execution of AUSTAL2000
    return 0;
  WrkServer("-n0,0");
  DtbServer("-n0,0");
  for (T1=MI.dost1; T1<MI.end; T1+=MI.cycle) {
    MI.dost1 = T1;
    T2 = T1 + MI.cycle;
    strcpy(T1s, TmString(&T1));
    strcpy(T2s, TmString(&T2));
    WrkServer("-n");                                          //-2001-06-29
    DtbServer("-n");
    NmsSeq("sequence", MI.cycind);
    sprintf(t, "-N%08lx", idos0);       DosServer(t);
    sprintf(t, "-T%s", T1s);            DosServer(t);
    DosServer("");                                      eG(3);
    if (MI.average > 0) {
      sprintf(t, "-N%08lx", idtb0);     DtbServer(t);
      DtbServer("");                                    eG(5);
    }
    if ((MI.average < 1) || (0 == (MI.cycind % MI.average))) {
      sprintf(t, "-N%08lx", icon0);    
      DtbServer(t);                                               //-2011-07-04
      DtbServer("");   
      TmnStoreAll(    mask0, icon0, TMN_NOID);          eG(8);    //-2011-07-04
      TmnClearAll(T2, mask0, icon0, TMN_NOID);          eG(9);    //-2011-07-04
      WrkServer("-n0,0");                                         //-2002-02-19
      TmnClearAll(T2, mask0, idos0, TMN_NOID);          eG(10);   //-2011-07-04
    }
    TmnClearAll(T2, mask1, idos1, TMN_NOID);            eG(11);
    TmnClearAll(T2, mask1, idos2, TMN_NOID);            eG(12);
    TmnClearAll(T2, mask0, ixtr0, TMN_NOID);            eG(13);
    TmnClearAll(T2, mask0, isum0, TMN_NOID);            eG(14);
    TmnClearAll(T2, mask0, iprf0, TMN_NOID);            eG(33);   //-2006-02-15
    TmnClearAll(T2, mask0, imod0, TMN_NOID);            eG(34);   //-2006-02-15
    strcpy(MI.output, NmsName(idos0));
    vDSP(1)("");
    MI.cycind++;
    if (StdCheck & 0x000080) {                                    //-2006-02-15
#ifdef MSGALLOC
      char form[256];
      sprintf(form, _msgalloc_bytes_, "%s", MSG_I64DFORM,        //-2011-07-18
        MSG_I64DFORM, "%d");
      fprintf(MsgFile, form, T2s, MsgGetNumBytes(), MsgGetMaxBytes(),
        MsgGetNumAllocs());
#endif
      fprintf(MsgFile, _tmncount_particles_,
      TmnList(NULL), TmnMemory(NULL), TmnParticles());
    }
  }
  printf("                                            \r");     //-2003-06-26
  TmnStoreAll(mask1, idtb0, TMN_NOID);                  eG(15);
  TmnClearAll(TmMax(), mask0, iptl0, TMN_NOID);         eG(19); //-2006-02-15
  TmnClearAll(TmMax(), mask1, idtb0, TMN_NOID);         eG(20); //-2006-02-15
  DtbServer("-ms");                                     eG(35); // 2001-04-26 uj
  if (StdCheck & 0x000080) {                                    //-2006-02-15
#ifdef MSGALLOC
      char form[256];
      sprintf(form, _msgalloc_bytes_, "%s", MSG_I64DFORM,       //-2011-07-18
        MSG_I64DFORM, "%d");
      fprintf(MsgFile, form, T2s, MsgGetNumBytes(), MsgGetMaxBytes(),
        MsgGetNumAllocs());
#endif
      fprintf(MsgFile, _tmncount_particles_,
      TmnList(NULL), TmnMemory(NULL), TmnParticles());
      TmnList(MsgFile);
  }
  return 0;
eX_2: eX_3: eX_4:
eX_5:
eX_8: eX_9:
eX_10: eX_11: eX_12:
eX_13:
eX_14:
eX_15: eX_19: eX_20:
eX_33: eX_34: eX_35:
  eMSG(_internal_error_);
}

#ifdef  MAIN  /*############################################################*/

static int Interactive;

static void MyExit( void )
{
  MsgQuiet = -1;                                      //-2002-07-03
  if (MsgCode < 0) {
    vMsg("@%s", _a2k_error_);
    raise(SIGABRT);   //&&&
  }
  else if (MsgCode == 99) {
    vMsg("@%s", _a2k_stopped_);
  }
  else if (MsgFile) {
    vMsg("@%s", _a2k_finished_);
  }
  TmnFinish();
  if (MsgFile) {
    fprintf(MsgFile, "\n\n\n");
    fclose(MsgFile);
  }
  MsgFile = NULL;
  if (Interactive) {
    int i;
    vMsg(_press_return_);                                //-2006-03-23
    i = getchar();
  }
  MsgSetQuiet(1);
  if (0 == (Xflags & 0x10) && ClrPath) {                        //-2006-03-23
    TutDirClear(ClrPath);
    TutDirRemove(ClrPath);
  }
}

static void MyAbort( int sig_number )
{
  FILE *prn;
  char *pc;
  static int interrupted=0;
  if (interrupted)
    exit(0);                             //-2002-08-29
  MsgSetQuiet(0);
  switch (sig_number) {
    case SIGINT:  pc = _interrupted_;  interrupted = 1;  break;
    case SIGSEGV: pc = _segmentation_violation_;  break;
    case SIGTERM: pc = _termination_request_;  break;
    case SIGABRT: pc = _abnormal_termination_;  break;
    default:      pc = _unknown_signal_;
  }
  vMsg("@%s (%s)", _a2k_aborted_, pc);
  if (sig_number != SIGINT) {
    prn = (MsgFile) ? MsgFile : stdout;
    fflush(prn);
    fprintf(prn, _abort_signal_, sig_number);
  }
  if (MsgFile) {
    fprintf(MsgFile, "\n\n");
    fclose(MsgFile);
  }
  MsgFile = NULL;
  MsgSetQuiet(1);
  Interactive = 0;
  exit(sig_number);                                               //-2008-08-08
}

static long TalMain( void )
{
  dP(TalMain);
  int n, r;
  char path[256], t[64], *pc;
  int standard_int_av = 1, ri_used = 0;
  if (!StdCheck) {
    signal(SIGSEGV, MyAbort);
    signal(SIGABRT, MyAbort);
  }
  signal(SIGTERM, MyAbort);                           //-2001-03-28 lj
  signal(SIGINT,  MyAbort);
  //
  strcpy(TI.cset, NlsGetCset());                                //-2008-07-22
  //
  if (StdCheck & 0x1000)  MsgFile = NULL;
  MsgSetVerbose(1);
  MsgSetQuiet(0);
  MsgDspLevel = 2;
  MsgLogLevel = 2;
  MsgBreak = '\'';
  atexit(MyExit);
  //
#ifdef AUSTAL2000N
  CfgInit(1);
#else
  CfgInit(0);
#endif
  setlocale(LC_NUMERIC, "C"); /* use decimal point for in and out, should not be changed */
  sprintf(TalVersion, "%d.%d.%s", StdVersion, StdRelease, StdPatch);
#ifdef MAKE_LABEL
  strcat(TalVersion, "-" MAKE_LABEL);
#endif
  sprintf(TalLabel, "%s_%s", eMODn, TalVersion);           //-2011-12-01
  //
  if (!(StdStatus & STD_ANYARG)) {
    char **_tl;
    int i;
    Interactive = 1;
    StdArgDisplay = 1;
    *path = 0;
    vMsg(_austal2000_version_, TalVersion);
    vMsg(_set_work_directory_);
    fgets(path, 256, stdin);                        //-2002-07-03
    if (*path < ' ')
      exit(0);
    _tl = _MsgTokens(path, " \t\r\n");
    if (!_tl || !_tl[0])
      exit(0);
    for (i=0; (_tl[i]); i++) {
        MsgUnquote(_tl[i]);                         //-2003-01-22
        TalServer(_tl[i]);
    }
    FREE(_tl);
    MsgSetQuiet(0);
    MsgDspLevel = 2;
  }
  StdArgDisplay = 3;
  strcpy(TalPath, StdPath);
  VspSetWork(StdPath);                                            //-2011-12-21
  strcat(StdPath, "/work");
  ClrPath = StdPath;                                              //-2006-11-06
  //
  lMSG(0,0)("");
  lMSG(0,0)(__austal2000_version_, TalVersion);
  lMSG(0,0)(_copyright_uba_);
  lMSG(0,0)(_copyright_ibj_);
  if (CfgWet) {                                                   //-2014-01-21
  		lMSG(0,0)("");
  		lMSG(0,0)(_no_taluft_);
  }
  lMSG(0,0)("");
  lMSG(0,0)(_work_directory_, TalPath);
  if (Xflags != 0x0f && Xflags != 0 && Xflags != 8)
    lMSG(0,0)(_xflags_applied_);
  //
  if (!MsgFile) {
    vMsg(_cant_work_);
    *StdPath = 0;                                                 //-2006-03-23
    exit(1);
  }
  fprintf(MsgFile, _pgm_created_,
    IBJstamp(NULL, NULL));                                        //-2006-08-24
#ifdef __linux__                                                  //-2008-07-30
  gethostname(t, 64);
  pc = t;
#else
  pc = getenv("HOSTNAME");                                        //-2002-07-03
  if (!pc)  pc = getenv("COMPUTERNAME");
#endif
  if (pc)  fprintf(MsgFile, _host_is_, pc);
  fflush(MsgFile);
  //
  TipMain(TalPath);
  sprintf(t, "-M%02x", TIP_AUSTAL);
  TipMain(t);                                                     //-2014-06-26
  sprintf(path, "-H%s", StdMyName);
  pc = strrchr(path, '/');
  if (pc)  *pc = 0;
  TipMain(path);
  sprintf(path, "-v%d,%d", StdDspLevel, StdLogLevel);             //-2006-10-20
  TipMain(path);
  sprintf(t, "-x%02x", Xflags);
  TipMain(t);                                                     //-2008-08-08
  if (Xflags & 0x01)  TipMain("-w");
  else if (Xflags & 0x02)                                         //-2008-10-06
    lMSG(0,0)(_no_def_files_created_);
  //
  // read input for austal2000
  n = TipMain(NULL); 
  //
  if (NOSTANDARD && strstr(TI.os, "WRITESFC")) {                 //-2012-10-30
    Xflags &= ~0x0c; // no results, no evaluation
    PrfServer("-fwritesfc");
  }
  else if  (NOSTANDARD && strstr(TI.os, "READSFC")) {            //-2012-10-30
    PrfServer("-freadsfc");
  }
  if (n) {                                                  //-2001-05-09 lj
    if (n == 1) { // library created
      sprintf(path, "%s/lib", StdPath);
      TutDirRemove(path);
    }
    else if (n == 2) {
      vMsg(_lib_complex_only_);
    }
    if (n<0 && MsgCode>=0)  MsgCode = 99;                   //-2002-04-20
    exit(-MsgCode);
  }
  if (TI.interval != 3600 || TI.average != 24) {              //-2007-02-03
    standard_int_av = 0;
  }
  if ((Xflags & 0x0e) == 0)
    goto CLEAR;                                                 //-2008-08-11
  //
  SetDataPath(StdPath);
  if (TI.os && strstr(TI.os, "CHECKVDISP"))  PlmLog = MsgFile;  //-2003-03-06
  if (Xflags & 0x02) {                                          //-2006-02-16
    // StdLogLevel = 5;                                         //-2001-09-25
    TmnInit(StdPath, NmsName, NULL, StdCheck, MsgFile);      eG(1);
    StdDspLevel = 0;                                        //-2001-07-10
    MsgSetQuiet(1);
    TalServer("");                                           eG(3);
    StdLogLevel = 1;
    MsgSetQuiet(0);
    lMSG(1,1)("");
  }

  if (VspAbortCount > 0) {                                  //-2004-11-30
    vMsg(_stopped_by_vdisp_);
    vMsg(">>> %s", VspAbortMsg);
    vMsg(_plume_rise_0_);
    if (VspAbortCount > 1)
      vMsg(_additional_cases_, VspAbortCount-1);
    vMsg("");
  }
  //
  // --- Evaluation ---
  //
  n = 0;
  if (TalMode & (TIP_SERIES | TIP_STAT)) {
    char s[400], fn[256];
    int i, m=0, nfiles, kref=-1, kmax=-1;
    //if (TalMode & TIP_SERIES)  ndays = floor(TI.t2 - TI.t1);
    //else  ndays = 1;
    if (TalMode & TIP_SERIES)
      nfiles = floor((MsgDateSeconds(TI.t1, TI.t2)+0.5)/(TI.interval * TI.average));  //-2014-03-04
    else  
    		nfiles = 1;   
    if (TalMode & TIP_NODAY)  nfiles = 1;                   //-2005-10-25
    if (NOSTANDARD) {                                       //-2002-03-11
      char *pk;
      pk = strstr(TI.os, "Kref=");
      if (pk) {
        sscanf(pk+5, "%d", &kref);
        if (kref > TI.nz[0])  kref = TI.nz[0];
      }
      pk = strstr(TI.os, "Kmax=");                          //-2002-04-09
      if (pk) {
        sscanf(pk+5, "%d", &kmax);
        if (kmax > TI.nz[0])  kmax = TI.nz[0];
      }
    }
    TmtMain(TalPath);
    TqlMain(TalPath);
    TmoMain(TalPath);
    if (*TI.lc) {                                           //-2003-07-07
      sprintf(s, "-A%s", TI.lc);
      TmtMain(s);
      TmoMain(s);
      TqlMain(s);
    }
    if (0 == (Xflags & 0x04))
      goto EVALUATE;
    fprintf(MsgFile, "%s", _break_line_);
    for (i=0; i<SttSpcCount; i++) {                               //-2011-11-23
      if (SttSpcTab[i].rn<=0 &&                                   //-2012-04-06
      		  SttSpcTab[i].ry<=0 && 
      		  SttSpcTab[i].rd<=0 && 
      		  SttSpcTab[i].rh<=0)
        continue;
      if (TipSpcEmitted(i)) {
        char gn[32]="", fn[256];
        TmtMain("-0");
        sprintf(s, "-n%d", nfiles);
        TmtMain(s);
        if (kref >= 0) {
          sprintf(s, "-k%d", kref);                           //-2002-03-11
          TmtMain(s);
        }
        if (kmax > 0) {
          sprintf(s, "-m%d", kmax);                           //-2002-04-09
          TmtMain(s);
        }
        TmtMain("-b1");
        sprintf(s, "-s%d", i);                                  //-2005-08-25
        TmtMain(s);
        if (!standard_int_av)                                 //-2007-02-03
          TmtMain("-a");                                      // only long-time average
        for (n=0; n<TI.nn; n++) {
          if (TI.nn > 1) {
            sprintf(gn, "-g%02d", n+1);
            TmtMain(gn);
          }
          sprintf(fn, "-f%s%d%d", "work/c%04da", TI.gl[n], TI.gi[n]); //-2011-12-14
          TmtMain(fn);
          r = TmtMain("");  if (r < 0)                                eX(4);
        }
      }
    }
    TmtPrintWorker();
    if (standard_int_av) {                                        //-2007-02-03
      for (i=0; i<SttSpcCount; i++) {                             //-2011-11-23
        if (SttSpcTab[i].rh>0 && TipSpcEmitted(i)) {
				  char fn[256], gn[32]="";
					  TqlMain("-0");
		        sprintf(s, "-s%d", i);
		        TqlMain(s);
            for (n=0; n<TI.nn; n++) {
              if (TI.nn > 1) {
                sprintf(gn, "-g%02d", n+1);
                TqlMain(gn);
              }
              sprintf(fn, "-i%s%d%d.dmna", "work/dtba0", TI.gl[n], TI.gi[n]);  //!!!!!!!!!!
              TqlMain(fn);
              if (TalMode & TIP_SERIES)  TqlMain("-m");
              else                       TqlMain("-p");
              r = TqlMain("");  if (r < 0)                                eX(5);
              if (TalMode & TIP_SERIES)  TqlMain("-m0");                  //-2001-07-06
              else                       TqlMain("-p-2");
              r = TqlMain("");  if (r < 0)                                eX(7);
					  }
				  }
			 }
	 	}
    if ((TalMode & TIP_SERIES) && TI.np>0) {
      for (i=0; i<SttSpcCount; i++) {                             //-2011-11-23     		
        if (SttSpcTab[i].ry<=0 && SttSpcTab[i].rd<=0 && SttSpcTab[i].rh<=0)
            continue;
        if (TipSpcEmitted(i)) {
          TmoMain("-0");
          sprintf(s, "-n%d", TI.np);
          TmoMain(s);
          sprintf(s, "-s%d", i);                                  //-2011-11-23
          TmoMain(s);
          n = TmoMain("");  if (n < 0)                              eX(6);
        }
      }
    }
    //
EVALUATE: // log maximum values
    //
    if (0 == (Xflags & 0x08) || !standard_int_av)
      goto CLEAR;                                                 //-2007-02-03
    //
    ri_used = (0 != (TalMode & TIP_RI_USED));                     //-2011-12-15
    /* 2014-01-21 removed as it may result in internal parsing inconsistencies
    MsgSetLocale(TI.lc);                                          //-2008-10-17
    */
    if (TalMode & TIP_ODORMOD) {                                  //-2008-03-10
      char *names[TIP_ADDODOR + 2];
      for (i=0; i<=TIP_ADDODOR; i++)
        names[i] = SttCmpNames[cODOR + i];
      names[TIP_ADDODOR + 1] = NULL;
      TutEvalOdor(TalPath, TIP_ADDODOR, names, "odor_mod", TI.nn);    eG(8);
    }
    //
    fprintf(MsgFile, "%s", _break_line_);
    fprintf(MsgFile, "%s", _evaluation_t_);
    fprintf(MsgFile, "%s", _evaluation_u_);
    fprintf(MsgFile, "%s", _eval_dep_);
    if (ri_used) {                                                //-2011-12-15
      fprintf(MsgFile, "%s", _eval_dry_);
      fprintf(MsgFile, "%s", _eval_wet_);
    }
    fprintf(MsgFile, "%s", _eval_j00_);
    fprintf(MsgFile, "%s", _eval_tnn_);
    fprintf(MsgFile, "%s", _eval_snn_);
    for (i=0; i<TI.nq; i++)
      if (TI.hq[i] < 10)
        break;
    if (i < TI.nq) {                                              //-2002-02-26
      fprintf(MsgFile, "%s", _warning_src_1_);
      fprintf(MsgFile, "%s", _warning_src_2_);
      fprintf(MsgFile, "%s", _warning_src_3_);
    }
    //
    // evaluate deposition
    //
    m = 0;
    for (i=0; i<SttSpcCount; i++) {
      STTSPCREC *prec = SttSpcTab + i;
      int show_wet = ri_used && (prec->wf > 0 || prec->grps[2] > '0');
      int show_dry = ri_used && (prec->vd > 0 || prec->grps[2] > '0');
      int show_dep = (show_wet || prec->vd > 0 || prec->grps[2] > '0');  //-2011-12-19
      //
      if (!TipSpcEmitted(i))  // no emission for this substance
        continue;
      if (prec->rn <= 0)      // deposition not requested for this substance
        continue;
      if (show_dep) {                                             //-2011-12-19
        sprintf(fn, "%s-%s", prec->name, CfgDepString);           //-2011-12-15
        n = TutLogMax(TalPath, fn, 0, m, TI.nn);
        if (n < 0)  MsgCode = 0;
        else  m++;
      }
      if (show_dry) {
        sprintf(fn, "%s-%s", prec->name, CfgDryString);           //-2011-12-15
        n = TutLogMax(TalPath, fn, 0, m, TI.nn);
        if (n < 0)  MsgCode = 0;
        else  m++;
      }
      if (show_wet) {
        sprintf(fn, "%s-%s", prec->name, CfgWetString);           //-2011-12-15
        n = TutLogMax(TalPath, fn, 0, m, TI.nn);
        if (n < 0)  MsgCode = 0;
        else  m++;
      }
    } // for i
    if (m)  fprintf(MsgFile, "%s", _break_line_);
    //
    // evaluate concentration
    //
    m = 0;
    for (i=0; i<SttSpcCount; i++) {
      STTSPCREC *prec = SttSpcTab + i;
      if (!TipSpcEmitted(i))  // no emission for this substance
        continue;
      if (prec->ry > 0) {
        sprintf(fn, "%s-%s00", prec->name, CfgYearString);
        if (!strcmp(prec->name, "odor"))
          m = 0;                                                  //-2008-03-10
        n = TutLogMax(TalPath, fn, 1, m, TI.nn);
        if (n < 0)  MsgCode = 0;
        else  m++;
      }
      if (prec->rd > 0) {
        sprintf(fn, "%s-%s%02d", prec->name, CfgDayString, prec->nd);
        n = TutLogMax(TalPath, fn, 1, m, TI.nn);
        if (n < 0)  MsgCode = 0;
        else  m++;
        if (prec->nd > 0) {
          sprintf(fn, "%s-%s%02d", prec->name, CfgDayString, 0);
          n = TutLogMax(TalPath, fn, 1, m, TI.nn);
          if (n < 0)  MsgCode = 0;
          else  m++;
        }
      }
      if (prec->rh > 0) {
        sprintf(fn, "%s-%s%02d", prec->name, CfgHourString, prec->nh);
        n = TutLogMax(TalPath, fn, 1, m, TI.nn);
        if (n < 0)  MsgCode = 0;
        else  m++;
        if (prec->nh > 0) {
          sprintf(fn, "%s-%s%02d", prec->name, CfgHourString, 0);
          n = TutLogMax(TalPath, fn, 1, m, TI.nn);
          if (n < 0)  MsgCode = 0;
          else  m++;
        }
      }
    }  // for i
    //if (m)  fprintf(MsgFile, "%s", _break_line_);
    //
    // evaluate odor
    //
    //m = 0;
    if (TalMode & TIP_ODORMOD) {                           //-2008-03-10
      sprintf(fn, "odor_mod-%s00", CfgYearString);							  //-2008-04-14
      n = TutLogMax(TalPath, fn, 1, m, TI.nn);
      if (n < 0)  MsgCode = 0;
      else  m++;
    }
    if (m)  fprintf(MsgFile, "%s", _break_line_);
    //
    // evaluate time series for monitor points
    //
    if ((TalMode & TIP_SERIES) && TI.np>0) {
      int ii;
      for (ii=0; ii<2; ii++) {
        m = 0;
        for (i=0; i<SttSpcCount; i++) {
          STTSPCREC *prec = SttSpcTab + i;
          int show_wet = ri_used && (prec->wf > 0 || prec->grps[2] > '0');
          int show_dry = ri_used && (prec->vd > 0 || prec->grps[2] > '0');
          int show_dep = (show_wet || prec->vd > 0 || prec->grps[2] > '0');  //-2011-12-19
          if (!TipSpcEmitted(i))
            continue;
          if (ii==0 && prec->rn>0) {                              //-2002-08-28
            if (show_dep) {                                       //-2011-12-19
              sprintf(fn, "%s-%s", prec->name, CfgDepString);     //-2011-11-25
              n = TutLogMon(TalPath, fn, prec->dn, m);
              if (n < 0)  MsgCode = 0;
              if (n > 0)  m += n;
            }
            if (show_dry) {
              sprintf(fn, "%s-%s", prec->name, CfgDryString);
              n = TutLogMon(TalPath, fn, prec->dn, m);
              if (n < 0)  MsgCode = 0;
              if (n > 0)  m += n;
            }
            if (show_wet) {
              sprintf(fn, "%s-%s", prec->name, CfgWetString);
              n = TutLogMon(TalPath, fn, prec->dn, m);
              if (n < 0)  MsgCode = 0;
              if (n > 0)  m += n;
            }
          }
          TmoMain("-0");
          sprintf(s, "-s%d", i);
          TmoMain(s);
          sprintf(s, "-e%d", m);
          TmoMain(s);
          if (ii)  TmoMain("-g");
          n = TmoMain("");  if (n < 0)                                eX(11);
          m += n;
          if (m > 1)  m = 2;                                    //-2008-03-10
        }  // for i
        if (TalMode & TIP_ODORMOD) {                              //-2008-09-24
          i = TipSpcIndex("odor");
          TmoMain("-0");
          sprintf(s, "-s%d", i);
          TmoMain(s);
          sprintf(s, "-e%d", m);
          TmoMain(s);
          sprintf(s, "-r%d", i);
          TmoMain(s);
          if (ii)  TmoMain("-g");
          n = TmoMain("");  if (n < 0)                                  eX(12);
          if (n < 0)  MsgCode = 0;
        }
        fprintf(MsgFile, "%s", _break_line_);
      }
    }
    if ((TalMode & TIP_STAT) && TI.np>0) {
      m = 0;
      for (i=0; i<SttSpcCount; i++) {
        STTSPCREC *prec = SttSpcTab + i;
        int show_wet = ri_used && (prec->wf > 0 || prec->grps[2] > '0');  //-2012-04-06
        int show_dry = ri_used && (prec->vd > 0 || prec->grps[2] > '0');
        int show_dep = (show_wet || prec->vd > 0 || prec->grps[2] > '0');
        if (!TipSpcEmitted(i))
          continue;
        if (prec->rn>0) {
          if (show_dep) {
            sprintf(fn, "%s-%s", prec->name, CfgDepString);
            n = TutLogMon(TalPath, fn, prec->dn, m);
            if (n < 0)  MsgCode = 0;
            if (n > 0)  m += n;
          }
          if (show_dry) {
            sprintf(fn, "%s-%s", prec->name, CfgDryString);
            n = TutLogMon(TalPath, fn, prec->dn, m);
            if (n < 0)  MsgCode = 0;
            if (n > 0)  m += n;
          }
          if (show_wet) {
            sprintf(fn, "%s-%s", prec->name, CfgWetString);
            n = TutLogMon(TalPath, fn, prec->dn, m);
            if (n < 0)  MsgCode = 0;
            if (n > 0)  m += n;
          }
        }      
        if (SttSpcTab[i].ry > 0) {
          sprintf(fn, "%s-%s00", SttSpcTab[i].name, CfgYearString);
          n = TutLogMon(TalPath, fn, SttSpcTab[i].dy, m);
          if (n < 0)  MsgCode = 0;
          if (n)  m += 2;
        }
        if (SttSpcTab[i].rh > 0) {
          sprintf(fn, "%s-%s%02d", SttSpcTab[i].name, CfgHourString, SttSpcTab[i].nh);
          n = TutLogMon(TalPath, fn, SttSpcTab[i].dh, m);
          if (n < 0)  MsgCode = 0;
          if (n)  m += 2;
          if (SttSpcTab[i].nh > 0) {
            sprintf(fn, "%s-%s%02d", SttSpcTab[i].name, CfgHourString, 0);
            n = TutLogMon(TalPath, fn, SttSpcTab[i].dh, m);
            if (n < 0)  MsgCode = 0;
            if (n)  m += 2;
          }
        }
        if (m > 1)  m = 2;
      }  // for i
      if (TalMode & TIP_ODORMOD) {                                //-2008-03-10
        i = TipSpcIndex("odor");
        sprintf(fn, "odor_mod-%s00", CfgYearString);							//-2008-04-17
        n = TutLogMon(TalPath, fn, SttSpcTab[i].dy, m);
        if (n < 0)  MsgCode = 0;
      }
      fprintf(MsgFile, "%s", _break_line_);
    }  // if STAT
  }  // if SERIES or STAT
  //
CLEAR:
  //                                                              //-2006-03-23
  if (StdCheck & 0x000080) {                                      //-2006-02-15
#ifdef MSGALLOC
      char form[256];
      sprintf(form, _msgalloc_bytes_, "%s", MSG_I64DFORM,
        MSG_I64DFORM);
      fprintf(MsgFile, form, _main_done_, MsgGetNumBytes(), MsgGetMaxBytes());
      MsgListAlloc(MsgFile);
#endif
      fprintf(MsgFile, _tmncount_particles_,
      TmnList(NULL), TmnMemory(NULL), TmnParticles());
      TmnList(MsgFile);
  }
  TmnFinish();                                                    //-2006-02-15
#ifdef MSGALLOC
  if (StdCheck & 0x000080) {
      char form[256];
      sprintf(form, _msgalloc_bytes_, "%s", MSG_I64DFORM,
        MSG_I64DFORM);
      fprintf(MsgFile, form, _main_finished_, MsgGetNumBytes(), MsgGetMaxBytes());
      MsgListAlloc(MsgFile);
  }
#endif
  //
  return 0;
eX_1: eX_3: eX_4: eX_5: eX_6: eX_7: eX_8: eX_11: eX_12:					  //-2008-09-24
  eMSG(_internal_error_);
  }

#endif  //##################################################################

/*==========================================================================
 *
 * history:
 *
 * 2001-05-09 lj 0.1.2 option "-z": exit after input processing
 * 2001-06-10 lj 0.5.4 user interaction to get the working directory
 * 2001-06-21 uj 0.5.5 LM-values corrected
 * 2001-06-27 lj 0.5.6 DtSubMin in TalAKS
 * 2001-06-29 lj 0.5.7 informing Worker on index for averaging
 * 2001-07-06 lj 0.5.8 evaluation of supremum
 * 2001-07-07 lj 0.5.9 german abbreviations for file names
 * 2001-07-08 lj 0.5.10 option "-a": evaluation only
 * 2001-07-09 lj 0.6.0  release candidate
 * 2001-07-10 lj 0.6.1  progress report in worker
 * 2001-08-14 lj 0.7.0  release candidate
 * 2001-09-04 lj 0.7.1  log file opened as "append"
 * 2001-09-27 lj 0.8.0  release candidate
 * 2001-10-01 lj 0.8.1  time stamp
 * 2001-11-05 lj 0.9.0  release candidate
 * 2001-12-27 lj 0.10.0
 * 2002-01-02 lj 0.10.1 error in TipCheck() for option "-z" corrected
 * 2002-01-03 lj        default value of sq in TipCheck()
 * 2002-01-12 lj 0.10.4 TalInp, TalWnd, TalPrf changed
 * 2002-02-19 lj 0.10.5 handling of invalid times corrected
 * 2002-02-25 lj 0.10.6 values at monitor points with AKS
 * 2002-02-26 lj 0.11.0 release cadidate
 * 2002-03-16 lj 0.11.1 LstBlm: hm<=800 for lmo>0
 * 2002-03-28 lj 0.11.2 adapted to IBJstd.h
 * 2002-05-13 lj 0.12.4 BLM modified
 * 2002-05-24 lj 0.12.5 MTM modified (usage of "valid")
 * 2002-06-21 lj 0.13.0 INP extended to "Cd", AKT revised
 * 2002-07-03 lj 0.13.1 yearly average at monitor points for AKTerm, LINUX
 * 2002-07-13 lj 0.13.2 INP calculation of nesting corrected
 * 2002-07-15 lj 0.13.3 PRF external meteorological fields
 * 2002-08-01 lj 0.13.4 INP time dependent hm
 * 2002-08-28 lj 0.13.5 DEP printed for monitor points
 * 2002-09-23 lj 0.13.6 VDISP (plume rise for cooling towers)
 * 2002-09-24 lj 1.0.0  final release candidate, no run time restriction
 * 2002-09-28 lj 1.0.1  MTM corrected for nh==0
 * 2002-10-21 lj 1.0.2  PRF check on negative zg removed in Clc3dMet()
 * 2002-10-28 lj 1.0.3  DTB DtbMntRead() corrected
 * 2002-12-10 lj 1.0.4  PRM WRK PLM INP DEF time series for lq, rq, tq
 * 2003-01-22 lj 1.0.5  INP A2K blanks in file names allowed
 * 2003-01-30 lj 1.0.6  PRF buffer size for file name increased
 * 2003-02-21 lj 1.1.1  MTM QTL INP MON scientific notation, species "xx"
 * 2003-02-22 lj 1.1.2  RND SRC WRK PRM new random numbers
 * 2003-02-24 lj 1.1.3  WND initial wind field
 * 2003-03-06 lj 1.1.4  option CHECKVDISP
 * 2003-03-11 lj 1.1.5  WND improved rounding for index calculation
 * 2003-03-27 lj 1.1.6  UTL GetList(), GetData(): reading of dirty "0" improved
 * 2003-03-28 lj        PLM volume flux (standard conditions) used for heat emission
 * 2003-06-16 lj 1.1.7  PLM parameter "tm" as volatile (for correct comparison)
 *                      INP DEF WRK option SPECTRUM
 *                      PRF check of va in library file and of vz
 * 2003-07-07 lj 1.1.8  DMN MTM QTL localisation
 * 2003-08-29 lj 1.1.9  SRC bit pattern for selection corrected
 * 2003-09-23 lj 1.1.10 DEF avoid lost of numerical significance (AKS)
 * 2003-10-02 lj 1.1.11 INP check and construction of nested grids corrected
 * 2003-10-12 lj 1.1.12 INP check of surface
 *                      WND gx an gy written to header
 *                      PRF listing of wind library, option "-p"
 * 2003-12-05 lj 1.1.13 DTB error message more specific
 *                      INP check on length of time series and AKTerm
 * 2004-01-12 lj 1.1.14 WRK error message more precise
 * 2004-04-27 lj 1.1.15 IBJntr undefined variable corrected (B.Petersen)
 * 2004-05-11 lj 1.1.16 PRF error message corrected
 * 2004-05-12 lj 1.1.17 NTR error message if NODATA found
 * 2004-06-08 lj 1.1.18 DEF write vd using %10.3e
 * 2004-06-08 lj 2.0.0  AUSTAL2000G
 * 2004-08-14 lj 2.0.1  -> AUSTAL2000
 * 2004-09-01 uj 2.1.0  TALdia
 * 2004-09-10 lj 2.1.2  INP check of grid nesting
 * 2004-10-04 lj 2.1.3  DEF PRM DOS option ODMOD
 * 2004-10-25 lj 2.1.4  INP ZET UTI guard cells for grids
 *                      INP value of z0 from CORINE always rounded
 *                      BLM INP DEF optional factors Ftv
 * 2004-11-02 uj 2.1.5  INP min dd without buildings changed from 15m to 16m
 * 2004-11-12 uj 2.1.6  INP additional check in getNesting()
 * 2004-11-26 lj 2.1.7  DOS DTB GRD MOD PPM PRM PTL SRC WND WRK ZET
 *                      string length for names = 256
 * 2004-11-30 uj 2.1.8  INP getNesting() check
 *                      INP explicite reference to TA Luft for warning h < 1.2hq
 *                      A2K VSP check for VDISP abort
 *                      PLM derivation of tm from q and R corrected
 * 2004-12-09 uj 2.1.9  WRK shift particle outside body also if not created
 *                      WND check for body-free boundary cells
 *                      MTM artp for odor: FHP<BS>, EHP
 * 2004-12-14 uj 2.1.10 INP BDS DEF wb, da in degree, SOR include TalMath
 * 2004-12-22 uj 2.1.11 PLM minimum u changed from 0.5 to 1 m/s
 * 2004-12-23 uj        PRF WRK warning message corrected
 * 2005-01-23 uj 2.1.12 WRK re-mark particles created outside an inner grid
 * 2005-01-25 lj        DMN conversion to arr-header modified
 * 2005-02-11 uj 2.1.13 WND binary output by default (option -t: text output)
 * 2005-02-10 lj 2.1.14 PRF additional fields for nested grids corrected
 * 2005-03-10 uj 2.1.15 SOR control flush, WND log creation date & computer
 * 2005-03-11 lj 2.1.16 ZET relation of monitor points to grids
 *                      DEF output to *.def with 1 fractional digit
 *                      DEF PRM param.def:Schwelle replaced by OdorThr
 *                      QTL array type changed
 *                      DMK shadowing modified
 * 2005-03-16 lj 2.1.17 DOS odstep as standard
 * 2005-03-17 uj 2.2.0  version number upgrade
 * 2005-03-18 lj        INP no wind lib created with option -z
 * 2005-04-13 lj 2.2.1  INP option -z for flat terrain corrected
 * 2005-05-12 lj 2.2.2  INP check of nesting corrected
 *                      DMKutil work around for compiler error
 * 2005-08-08 uj 2.2.3  PRF control printout of 1D profiles with option -p
 *                      WND write out parameter axes to file headers
 *                      NTR small extensions
 *                      DMN reading of parameter buff corrected
 *                      MTM rounding of double returned by MsgDateSeconds()
 * 2005-08-17 uj 2.2.4  DMK avoid rounding problem for non-int grid coordinates
 * 2005-08-14 lj        WND option GRIDONLY
 * 2005-09-07 lj 2.2.5  PRF tolerated surface difference increased to 0.2
 * 2006-01-23 uj 2.2.6  PRF try to avoid extrapolation of library fields
 * 2006-02-06 uj 2.2.7  INP nostandard option SORRELAX
 *                      DEF local TmString, TimeStr
 *                      BLM error if ua=0
 * 2006-02-13 uj 2.2.8  DEF AKS BLM nostandard option SPREAD
 * 2006-02-15 lj 2.2.9  MSG UTL TMN new allocation check
 * 2006-02-16           Xflags
 * 2006-02-22           DOS SetOdourHour: getting sg
 * 2006-02-25 uj        PRM DTB quotation marks for Kennung added
 * 2006-02-27 uj        MyExit Xflags corrected
 * 2006-03-13 lj 2.2.10 std.h: option of type "--name=value"
 *                      MSG MsgReadOption(), dynamic size of allocation tables
 * 2006-03-24 lj 2.2.11 MST deleting work directory corrected
 *                      VSP maximum uh increased to 100 m/s
 * 2006-07-13 uj 2.2.12 MTM case gruppen<=4 corrected
 * 2006-08-22 lj        QTL ZET string length for "Kennung" increased
 * 2006-08-24 lj 2.2.13 modified IBJstamp()
 * 2006-08-25 lj 2.2.14 BDS work around for compiler error (SSE2 in I9.1)
 * 2006-10-04 lj 2.2.15 SOR DMK DMKutil iteration modified
 * 2006-10-11 lj        INP TI.im defaults to 200
 * 2006-10-18 lj 2.3.0  NLS HLP native language support, help function
 * 2006-10-31 lj 2.3.1  MST CFG configuration data
 * 2006-11-03 uj 2.3.2  DEF qq with 3 decimal places
 *                      INP TIP_MAXLENGTH, RGL corrections
 * 2006-11-06 uj        INP clear lib corrected, A2K ClrPath def moved
 *                      corrections in *.text
 * 2006-11-21 2.3.3  uj INP write out GK-converted source coordinates
 * 2007-01-30 2.3.4  uj DEF BLM define Svf via option string
 * 2007-02-03 2.3.5  uj DEF INP MTM time interval revised
 *                      INP maximum np adjustable from outside
 * 2007-03-07 2.3.6  uj AKT DEF calculation of uamean corrected
 * 2008-01-25 2.3.7  uj INP DEF allow absolute path for rb
 * 2008-02-12        lj DMK display switched by StdDspLevel (SOR: S.dsp)
 * 2008-03-10 lj 2.4.0  DOS SRC MTM MON INP DEF UTI rated odor
 * 2008-04-17 lj 2.4.1  2.3.x updated
 * 2008-04-23 lj        UTI IBJ-mode as standard for odor rating
 * 2008-07-22 lj 2.4.2  DTB MTM QTL NLS external strings, parameter cset
 * 2008-07-30 lj        using gethostname() with Linux
 * 2008-08-04 lj 2.4.3  MTM UTI MON reading dmna-files, logging scatter
 *                      MTM DTB WRK write "valid" using 6 decimals
 * 2008-08-12 lj        WND Display of version number
 * 2008-08-15 lj        WND CFG NLS-info in help()
 *                      MTM worker version logged
 * 2008-08-27 lj        NLS STD filter language file, display selected language
 * 2008-09-05 uj        INP use a minimum height of 10m in z0 calculation
 * 2008-09-15 uj        Xflags DEF warning only if dispersion calculation
 * 2008-09-23 lj        INP 2 additional odor substances
 *                      no transcoding for windows (use cp1252)
 * 2008-09-24 lj        TUT MON update for rated odor frequencies
 * 2008-09-26 lj        NLS NlsRead(): read from nls/LAN/
 * 2008-09-29 lj        Option "-X" evaluated by IbjStdReadNls()
 *                      (translation latin1->cp850 for screen output in Windows)
 * 2008-10-07 lj        TMT scatter of deposition corrected
 * 2008-10-17 lj        INP MON QTL MTM MSG DMN uses MsgSetLocale()
 * 2008-10-20 lj 2.4.4  INP nonstandard: hm, ie, im, mh, x1, x2, x3, y1, y2, y3
 * 2008-10-27 lj        TUT rated odor hour frequency limited to <=100%
 * 2008-10-27 lj        TUT odor values at monitor points with AKS
 * 2008-12-04 lj 2.4.5  TUT odor scatter at monitor points with AKS corrected
 *                      MON writes modified t1, t2, artp; renamed dt, rdat
 *                      WRK DTB MON GRD DEF use "refx", "refy", "ggcs"
 *                      INP "lq" actually treated as variable parameter
 *                      MTM renamed "pgm", "ident"
 * 2008-12-07 lj        INP RGL "UTM" uses zone 32 if no prefix is given
 *                      STD UTI header complemented
 * 2008-12-11 lj        GRD DMN NTR WND INP header parameter modified
 *                      NTR "ggcs" added to record
 * 2008-12-19 lj 2.4.6  UTI "n.v." internationalized
 *                      AKS-i18n header for metlib.def corrected
 *                      NLS INP pass language option to taldia
 * 2008-12-29 lj 2.4.7  AKS writing of metlib.def modified
 * 2009-02-03 uj        PRF header allocation moved
 * 2011-04-06 uj 2.4.8  PRF check in case of 1 base field    
 * 2011-04-13 uj 2.4.9  WRK DTB handle title with 256 chars in header
 * 2011-06-03 uj 2.4.10 UTI upper bound of dev corrected for odor
 * 2011-06-09 uj        ZET grid assignment for monitor points corrected
 *                      MTM MON dmn header entries adjusted
 *                      MON TmoEvalSeries: use absolute dev for odorants
 *                      DEF dummy flag RATEDODOR
 *                      INP maximum emission height increased to 500
 * 2011-06-18 uj        MON QTL SpcName length increased to 16
 * 2011-06-20 uj        TUT log odor scatter with one decimal position
 * 2011-06-29 uj 2.5.0  TMN WRK DTB GRD DEF MOD PRF ZET SRC BLM PRM: 
 *                         dmn files/headers in "work", tmn routines with TXTSTR
 *                      INP check if standard roughness register is used
 * 2011-07-04 uj        A2K WRK DTB concentration in "work" instead of dose
 *                      GRD don't write grd files by default
 * 2011-07-07 uj        DEF BLM option PRFMOD (blm versions 4.6/4.8)
 * 2011-07-29 uj        ZET positioning of monitor points improved
 * 2011-09-12 uj 2.5.1  INP DEF AKS AKT setting of blm version unified
 * 2011-10-13 uj 2.5.2  WRK optimization for chemical reactions
 * 2011-11-21 lj 2.6.0  WRK PRF BLM MOD PRM PPM precipitation, settings
 * 2011-12-01 lj        IO1 MSG DMN RGL DMK GRD TXT print format for pointer
 * 2011-12-12 lj        output refinement
 * 2011-12-15 lj        evaluation of DEP, DRY, WET
 * 2011-12-19 lj        evaluation corrected
 * 2011-12-21 lj 2.6.1  VSP uses work directory
 * 2012-01-26 uj 2.6.2  PRF error message corrected
 * 2012-04-06 uj 2.6.3  INP complain if NOSTANDARD is missing for "hh"
 *                      AKT detecting precipitation corrected
 *                      WND interpolation corrected for z<0
 * 2012-09-04 uj 2.6.4  TMN gencnv.h excluded (not used)
 * 2012-10-30 uj 2.6.5  A2K INP PRF PRM DEF read/write superposition factors
 * 2012-11-05           BLM DEF anemometer height hw for base field superposition
 * 2013-01-11 uj        INP AKT invalid umin parsing removed
 * 2013-04-08 uj 2.6.6  DEF AKS definition of HmMean without ","
 * 2013-06-13 uj 2.6.7  AKT warning for inconsistent definition of calms
 * 2013-07-08 uj 2.6.8  WRK PPM check for negative substance parameters
 * 2014-01-21 uj 2.6.9  A2K INP allow only decimal point for output
 *                      NTR allow -999 as coordinate in NtrReadXYZ()
 *                      A2K HLP CFG INP AKS DEF rules.make austal2000/n.text:
 *                          split into austal2000 and austal2000n
 * 2014-01-24 uj        PRF skip lmo for setting akl
 * 2014-01-28 uj        DMN NTR AKT INP MON MTM QTL WND use blank instead of tab
 * 2014-01-29 uj        AKS DEF precipitation info
 * 2014-01-30 uj        MSG allow 'T' as time separator
 * 2014-02-27 uj        A2K TalMode init in TalMain removed again
 * 2014-03-04 uj 2.6.10 A2K Writing for Average != 24 reactivated
 * 2014-06-26 uj 2.6.11 A2K WND INP accept "ri" if called by TALdia
 *                      MNT write cset to output header of daily files
 *                      BLM DOS INP MTM PRM PRF PTL SRC WND ZET eX/eG adjusted
 ==========================================================================*/
 
