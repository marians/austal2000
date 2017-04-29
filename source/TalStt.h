//============================================================ TalStt.h
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
// last change: 2011-12-06 lj
//
//==========================================================================

#ifndef TALSTT_INCLUDE
#define TALSTT_INCLUDE

#define STT_ARTM  0x0001

typedef struct {
  char name[16];// name of the component
  char grps[16];// aerosole groups allowed
  char unit[16];// unit of emission
  float vd;     // deposition velocity for gas in m/s             //-2005-09-12
  float wf;     // factor for wet deposition (Lambda_0) in 1/s    //-2005-09-12
  float we;     // exponent for wet deposition (kappa)            //-2005-04-12
  float de;     // decay exponent in 1/s                          //-2005-09-26
  float fr;     // fraction above 0.2 MeV                         //-2005-09-26
  //
  float fc;     // factor for conversion of concentration
  char uc[16];  // unit of concentration
  float fn;     // factor for conversion of deposition
  char un[16];  // unit of deposition
  // yearly average:
  float ry;     // reference value
  int dy;       // number of trailing decimals
  // daily average:
  int nd;       // number of allowed exceedings
  float rd;     // reference value
  int dd;       // number of trailing decimals
  // hourly average:
  int nh;       // number of allowed exceedings
  float rh;     // reference value
  int dh;       // number of trailing decimals
  // deposition:
  float rn;     // reference value
  int dn;       // number of trailing decimals
  //
} STTSPCREC;

extern int SttSpcCount, SttCmpCount, SttTypeCount, SttMode;
extern STTSPCREC *SttSpcTab;
extern char *SttGroups[6];
extern char *SttGrpXten[6];
extern double SttVdVec[6];
extern double SttVsVec[6];
extern double SttWfVec[6];
extern double SttWeVec[6];
extern double SttRiVec[6];                                        //-2011-12-06
extern double SttNoxTimes[6];
extern double SttHmMean[6];
extern double SttOdorThreshold;
extern char **SttCmpNames;
extern char SttRiSep[256];                                        //-2011-12-06
extern int SttNstat;

extern double SttGmmB1;
extern double SttGmmB2;
extern char SttGmmUnit[16];
extern double SttGmmFd;
extern double SttGmmFe;
extern double SttGmmFf;
extern double SttGmmFr;
extern double SttGmmMue;
extern double SttGmmRef;
extern double SttGmmMu[2];
extern double SttGmmDa[2][6];
extern double SttGmmBk[2][4][4];

int SttRead(        // read the settings for ARTM
char *path,         // home directory of ARTM
char *pgm )         // program name
;
STTSPCREC *SttGetSpecies( // get the species definition record
char *name)               // species name
;

#endif

