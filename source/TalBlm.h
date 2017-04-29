// ================================================================ TalBlm.h
//
// Meteorologisches Grenzschichtmodell
// ===================================
//
// Copyright (C) Umweltbundesamt, Dessau-Roﬂlau, Germany, 2002
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
// last change: 2012-11-05 uj
//
//==========================================================================

#ifndef  TALBLM_INCLUDE
#define  TALBLM_INCLUDE

#ifndef   IBJARY_INCLUDE
  #include "IBJary.h"
#endif

#ifndef   TALIO1_INCLUDE
  #include "TalIo1.h"
#endif

#define  DEFUCALM  0.5
#ifndef  NOTV
  #define  NOTV  -999.0
#endif

#define  BLMDEFVERS   26
#define  BLMDEFHA   10.0
#define  BLMDEFSIG   0.3
#define  BLMDEFLMO  99999
#define  BLMDEFHM    800
#define  BLMMOSMIN  -0.1
#define  BLMMOSMAX  0.05
#define  BLMSZ0     "0.5"
#define  BLMSTUMAX  "1200"
#define  BLMSTVMAX  "1200"
#define  BLMSTWMAX  "1200"
#define  BLMDEFUREF  3.0

typedef struct {
        float z, u, g, d, su, sv, sw, tu, tv, tw, suw, ths;
        } BLMREC;

typedef struct {
        long MetVers;
        float RghLen, ZplDsp, MonObLen, RezMol, MixDpt, Precep,   //-2011-11-21
              Class, Ustar, UsgCalc, UstCalc, ThetaGrad, PtmpGrad;
        float AnmXpos, AnmYpos, AnmZpos;
        float AnmHeight, WndSpeed, WndDir, SigmaU, SigmaV, SigmaW;
        float AnmHeightW;                                         //-2012-11-05
        int AnmGridNumber;
        float StatWeight, U10;
        float SvFac;                                         //-2007-01-30
        long kta;
        float us, lmo, hm, cl, ths;
        char WindLib[256];
        int Wind, Wini, Diff, Turb;
        float HmMean[6], Ftv[6];
        } BLMPARM;

extern BLMPARM *BlmPprm;
extern ARYDSC *BlmParr;
extern VARTAB *BlmPvpp;

//=============================== PROTOTYPES TALBLM ==========================

long BlmProfile(      /* Berechnung des Grenzschicht-Profils.               */
BLMPARM *p,           /* Eingabe-Daten (Grenzschicht-Parameter).            */
BLMREC *v );          /* Ausgabe-Daten (Wind-Varianzen und Korr.-Zeiten).   */

long BlmStability(    /* Berechnung von Mischungsschichth√∂he und Stab.-Kl.  */
BLMPARM *p );         /* Parameter der Grenzschicht.                        */

float BlmSrfFac(        /* factor to impose surface layer */
float z0,               /* roughness length               */
float d0,               /* zero plane displacement        */
float h )               /* height above ground            */
  ;
long BlmInit(           /* initialize server    */
long flags,             /* action flags         */
char *istr )            /* server options       */
  ;
long BlmServer(         /* server routine for BLM       */
char *s )               /* calling option               */
  ;
//==========================================================================//
#endif
