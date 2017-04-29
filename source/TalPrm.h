//=================================================================== TalPrm.h
//
// Definition of parameters for AUSTAL2000
// =======================================
//
// Copyright (C) Umweltbundesamt, Dessau-Roﬂlau, Germany, 2002-2004
// Copyright (C) Janicke Consulting, 88662 ‹berlingen, Germany, 2002-2012
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
// last change: 2012-10-30 uj
//
//==========================================================================

#ifndef  TALPRM_INCLUDE
#define  TALPRM_INCLUDE

#ifndef   IBJARY_INCLUDE
  #include "IBJary.h"
#endif
#ifndef   TALIO1_INCLUDE
  #include "TalIo1.h"
#endif

#define FLG_ECHO      0x00000001L
#define FLG_CHECK     0x00000002L
#define FLG_VERB      0x00000004L
#define FLG_DISP      0x00000008L
#define FLG_QUIET     0x00000010L
#define FLG_MNT       0x00000020L // 2001-04-26 uj
#define FLG_MAXIMA    0x00000040L // 2001-04-26 uj
#define FLG_LIBRARY   0x00000080L //-2002-01-05 lj
#define FLG_PTLTAB    0x00000100L
#define FLG_RESULT    0x00000200L
#define FLG_WRTMOD    0x00000400L
#define FLG_SCAT      0x00000800L
#define FLG_CHEM      0x00001000L
#define FLG_GEOGK     0x00002000L
#define FLG_OENORM    0x00004000L
#define FLG_NOBASE    0x00008000L   //-2001-03-24 lj
#define FLG_GAMMA     0x00010000L
#define FLG_2DVDP     0x00020000L
//#define FLG_2DWSH     0x00040000L
//#define FLG_2DWDP     0x00080000L
#define FLG_WRITESFC  0x00040000L                                 //-2012-10-30
#define FLG_READSFC   0x00080000L                                 //-2012-10-30
#define FLG_ONLYADVEC 0x00100000    //-2002-01-05 lj
#define FLG_NOPREDICT 0x00200000
#define FLG_EXACTTAU  0x00400000
#define FLG_TRACING   0x00800000
#define FLG_MONITOR   0x01000000L
#define FLG_RNDSET    0x02000000L
#define FLG_ODOR      0x04000000L
#define FLG_ODMOD     0x08000000L
#define FLG_NODILUTE  0x10000000                                  //-2011-11-21

#define PRM_VARCHM    0x00000001
#define PRM_VAREMS    0x00000002
#define PRM_VARASL    0x00000004
#define PRM_VARSRC    0x00000008
#define PRM_VARPRM    0x00000080
#define PRM_DIRECT    0x00000100
#define PRM_CLOCK     0x00000200
#define PRM_STOP      0x00001000
#define PRM_BREAK     0x00002000
#define PRM_CONTINUE  0x00004000
#define PRM_USER      0x00008000
#define PRM_ERROR     0x80000000

#define  MI  (*PMI)

typedef struct {
   char cltime[12];
   char cldate[12];
   char label[256];                               //-2004-11-09
   char titel[256];
   char dpath[256];
   char defname[20];
   char input[80];
   char output[80];
   char asllist[40];
   char srglist[40];
   char refdate[40];
   char reftime[40];
   char disproc[256];   /* PostProc =    */
   char disparm[256];   /* PostParm =    */
   char depo[80], depobase[256], wash[80], washbase[256];
   int numasl, maxcmp, sumcmp, numsrc, numsrg, numgrp;
   int numshuf, asl, net;
   long flags, vlevel, dlevel, special, status, seed, count;
   float gamma[5];
   float khmin, kvmin, prcp, hmax, step;
   long cycind, cycle, average, wait;
   long start, end;
   long mett1, mett2, ztrt1, ztrt2, simt1, simt2, dost1, dost2;
   float wlkext;
   long  wlkdiv;
   float timing, lastrfac, usedrfac;
   long cli, cla, clb, clc, tmi, tma;
   float emisfac, quality;
   int iset, shn, inext, inextp;
   long shy, shtable[100];
   float threshold;
   } PRMINF;

typedef struct {
   char name[40], unit[20];
   int specident, specindex;
   float gmin, vsed, vdep, fwsh, rfak, rexp, mptl, refc, refd, wdep;
   float vred, hred;
   int aslcmp;
   } CMPREC;

typedef struct {
   char name[40];
   char unit[20];
   int specident, specindex;
   long id;
   float vsed, gmin, rate;
   int offcmp, numcmp;
   ARYDSC *pa;
   float admean, slogad, density;
   int nadclass;
   float adlimits[20], addistri[20];
   float vred, hred;
   } ASLREC;

typedef struct {
   char name[40];
   float fq;
   } SRGREC;

typedef struct {
   char name[40];
   float x, y, r, h, t, l, w, d, qq, vq, dq, oq, pq, aq, cq, fq, gq;
   float x1, y1, h1, x2, y2, h2, ts, sh, sv, sl, fr, rhy, lwc, tmp, si, co;
   long idgrd, idgrp;
   int ipoly1, ipoly2;
   float xmin, xmax, ymin, ymax, length;
   } SRCREC;

typedef struct {
  float xp, yp, hp;
} SRPREC;

typedef struct {
   float x, y, r, h, t, l, w, d, qq, vq, dq, oq, pq, aq, cq, fq, gq, ts, sh, sv, sl, fr;
   } AFUREC;

extern PRMINF *PMI;
extern ARYDSC *PrmPasl, *PrmPcmp, *PrmPsrc, *PrmPems, *PrmPchm;
extern ARYDSC *PrmPsrg, *PrmPemg, *PrmPsrp;
extern VARTAB *PrmPvpp;

/*=========================== PROTOTYPES LSTPRM ============================*/

char *PrmHeader(                /* the header (global storage)  */
  long id,                      /* identification               */
  long *pt1,                    /* start of the validity time   */
  long *pt2 )                   /* end of validity time         */
  ;
char *CmpHeader(                /* the header (global storage)  */
  long id,                      /* identification               */
  long *pt1,                    /* start of the validity time   */
  long *pt2 )                   /* end of validity time         */
  ;
long PrmInit(           /* initialize server    */
long flags,             /* action flags         */
char *istr )            /* server options       */
  ;
long PrmServer(
char *ss )
  ;
/*==========================================================================*/
#endif
