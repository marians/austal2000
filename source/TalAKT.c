// ================================================================= TalAKT.c
//
// Read AKTerm for AUSTAL2000
// ==========================
//
// Copyright (C) Umweltbundesamt, Dessau-Roﬂlau, Germany, 2002-2012
// Copyright (C) Janicke Consulting, 88662 ‹berlingen, Germany, 2002-2013
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
// last change:  2013-06-13 uj
//
// =========================================================================
//
//  Umwandeln einer DWD-Zeitreihe von Ausbreitungsklassen in einen
//  Zeitreihen-File fuer AUSTAL2000.
// -------------------------------------------------------------------------
//  1. read AKTerm, invalid data are denoted by class=0
//  2. distribute values within steps
//  3. choose random wind direction for unsteady winds
//  4. generate wind rose for low wind velocities
//  5. fill small gaps of missing values (1 or 2 hours)
//  6. set wind direction for calm (interpolated or at random)
//
// ==========================================================================

char *TalAKTVersion = "2.6.4";
static char *eMODn = "TalAKT";

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <math.h>

#include "IBJmsg.h"
#include "IBJary.h"

#include "IBJtxt.h"
#include "IBJdmn.h"

#include "TalUtl.h"
#include "TalInp.h"
#include "TalAKT.h"
#include "TalAKT.nls"

#if defined _M_IX86 && !defined __linux__               //-2002-12-19
    static __int64 seed = 0x12345678i64;
    static __int64 fac = 0x5DEECE66DI64;
    static __int64 add = 0xBI64;
    static __int64 sgn = 0x7fffffffffffffffI64;
    static __int64 msk = (1I64 << 48) - 1;
    static long Seed = 11111;
    
    static void setSeed( __int64 new_seed ) {
      seed = (new_seed ^ fac) & msk;
    }
#else
    static long long seed = 0x12345678LL;
    static long long fac = 0x5DEECE66DLL;
    static long long add = 0xBLL;
    static long long sgn = 0x7fffffffffffffffLL;
    static long long msk = (1LL << 48) - 1;
    static long Seed = 11111;
    
    static void setSeed( long long new_seed ) {
      seed = (new_seed ^ fac) & msk;
    }
#endif

static int next( int bits ) {
  seed = (seed*fac + add) & msk;
  return (int)((seed & sgn) >> (48 - bits));
}

static float nextFloat() {
  return next(24)/((float)(1 << 24));
}
//---------------------------------------------------------------------

ARYDSC AKTary;
char AKTheader[256];
float TatUmin = 0.70;
float TatUlow = 0.75;
float TatUref = 1.25;

static float Xa, Ya, Ha=0, Z0=0.5, D0=3.0, HaVec[9];
static char WindLib[512] = "~";

static int Nterm;
static int ChkLevel=9;
static int SollMonat;
static int HaGiven = 0;
static int   Wetlib=0;
static float Du = 0.257;
static float Dr = 5.0;
static float Kn = 0.514;

static int Nkn[21];

#define TE(i)  ((AKTREC*)AryPtrX(&AKTary, i))->t
#define RA(i)  ((AKTREC*)AryPtrX(&AKTary, i))->fRa
#define UA(i)  ((AKTREC*)AryPtrX(&AKTary, i))->fUa
#define KL(i)  ((AKTREC*)AryPtrX(&AKTary, i))->iKl
#define HM(i)  ((AKTREC*)AryPtrX(&AKTary, i))->fHm    //-2011-12-02
#define PR(i)  ((AKTREC*)AryPtrX(&AKTary, i))->fPr    //-2011-11-22

//---------------------------------------------------------------------------

static char Pfad[256], DwdName[256];
static int tage_pro_monat[] = { 0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

//================================================================= TatReadAKTerm
//
int TatReadAKTerm( void ) 
{
  dP(TatReadAKTerm);
  FILE *f, *msg;
  char s[256], fname[256], *pc;
  int i, j, l, n;
  int is, iy, im, id, ih, ii, ir, iu, ik, it, iw, unknown1, unknown2;
  int ix, ip, qr, qu, qb, qc, qd, qe;															//-2011-11-22
  int format, monat, tag, stunde, jahr, station;
  char modifier[32] = "";																					//-2011-11-22
  double te;
  float dr = Dr;
  float du = Du;
  float ua, ra, ha, pr;																	          //-2011-11-22
  int uses_hm = 0;                                                //-2011-12-02
  int uses_ri = 0;                                                //-2011-12-02
  int inconsistent_calms;                                         //-2013-06-13
  //
  msg = (MsgFile) ? MsgFile : stdout;
  setSeed(Seed);
  if (*DwdName=='/' || strchr(DwdName, ':'))  strcpy(fname, DwdName);
  else sprintf(fname, "%s/%s", Pfad, DwdName);
  TipLogCheck("AKTerm", TutGetCrc(fname));                        //-2011-12-07
  f = fopen(fname, "rb");
  if (!f) {
    vMsg(_cant_read_file_$_, fname);
    return -1;
  }
  i = 0;
  format = 0;
  while (fgets(s, 256, f)) {
    if (format == 0) {
      if (*s=='*' || *s=='+') {
        format = 3;                       // DWD after April 2002
        if (*s == '*') {																					//-2011-11-22
          for (j=0; j<strlen(s); j++)
            s[j] = tolower(s[j]);
          if (strstr(s, "niederschlag") || strstr(s, "precipitation"))           
            strcpy(modifier, "N");
          continue;
        }
      }
      else {
        if (!isdigit(*s))  continue;
        l = strlen(s)-1;
        if (l < 20) {
          vMsg(_unknown_format_short_line_);
          fclose(f);
          return -1;
        }
        if ((s[5]=='1' && s[6]=='9') || (s[5]=='2' && s[6]=='0')) {
          format = 2;
          if (l < 21) {
            vMsg(_unknown_format_new_);
            fclose(f);
            return -1;
          }
          fprintf(msg, "%s", _format_after_1998_04_01_);
        }
        else {
          format = 1;
          if (l != 20) {
            vMsg(_unknown_format_old_);
            fclose(f);
            return -1;
          }
          fprintf(msg, "%s", _format_before_1998_04_01_);
        }
      }
    }
    if (format == 3) {
      if (*s == '*')
        continue;
      else if (*s == '+') {
        pc = strchr(s, ':');
        if (!pc) {
          vMsg(_invalid_anemometer_height_);
          fclose(f);
          return -1;
        }
        pc = strtok(pc+1, " \t\r\n");
        for (j=0; j<9; j++) {
          if (!pc) {
            vMsg(_insufficient_count_ha_);
            fclose(f);
            return -1;
          }
          if (1 != sscanf(pc, "%f", &ha)) {
            vMsg(_cant_read_ha_);
            fclose(f);
            return -1;
          }
          HaVec[j] = 0.1*ha;
          pc = strtok(NULL, " \t\r\n");
        }
        HaGiven = 1;
        continue;
      }
      else if (strncmp(s, "AK", 2)) {
        vMsg(_invalid_format_);
        fclose(f);
        return -1;
      }
    }
    else {
      if (!isdigit(*s))
        continue;
    }
    i++;
  }
  Nterm = i;
  fprintf(msg, _akterm_$$$_, fname, Nterm, format);
  if (Nterm < 24) {                                          //-2002-08-15
    vMsg(_short_data_);
    fclose(f);
    return -1;
  }
  AryCreate(&AKTary, sizeof(AKTREC), 1, 1, Nterm);           eG(1);
  fseek(f, 0, SEEK_SET);
  i = 1;
  n = 0;
  inconsistent_calms = 0;                                   //-2013-06-13
  while (fgets(s, 256, f)) {
    n++;
    if (*s <= ' ')
      continue;
    if (format==1 || format==2) {
      if (!isdigit(*s))
        continue;
      l = strlen(s)-1;
      for (l=strlen(s)-1; l>=0; l--) if (s[l] <= ' ')  s[l] = '0';
    }
    else if (format == 3) {
      if (strncmp(s, "AK", 2))
        continue;
    }
    ir = 0;
    iu = 0;
    ik = 0;
    ix = 0;                                                  //-2012-04-06
    if (format == 1) {
      j = sscanf( s, "%3d%2d%2d%2d%2d%1d%2d%2d%1d%1d%2d",
              &is, &iy, &im, &id, &ih, &ii, &ir, &iu, &ik, &it, &iw );
      if (j < 9) vMsg(_low_count_9_$_, n);
    }
    else if (format == 2) {
      j = sscanf( s, "%5d%4d%2d%2d%2d%1d%2d%2d%1d%1d%1d%1d",
             &is, &iy, &im, &id, &ih, &ii, &ir, &iu, &ik, &it, &unknown1, &unknown2 );
      if (j < 9) vMsg(_low_count_9_$_, n);
    }
    else {
      if (*modifier == 'N') {        													//-2011-11-22                           
        j = sscanf(s+3, "%d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d",
          &is,&iy,&im,&id,&ih,&ii, &qr,&qu,&ir,&iu,&qb,&ik,&qc,&ix,&qd,&ip,&qe);
        if (j < 17) {
          vMsg(_low_count_17_$_, n);
          fclose(f);
          return -1;
        }
      }
      else {
      	j = sscanf(s+3, "%d %d %d %d %d %d %d %d %d %d %d %d %d %d %d",
          &is,&iy,&im,&id,&ih,&ii, &qr,&qu,&ir,&iu,&qb,&ik,&qc,&ix,&qd);
        if (j < 15) {
          vMsg(_low_count_15_$_, n);
          fclose(f);
          return -1;
        }
      }
    }
    if (i == 1) {
      station = is;
      jahr = iy;
      monat = im;
      tag = id;
      stunde = 0;
      if ((jahr % 4) == 0)  tage_pro_monat[2]++;
    }
    else stunde++;
    if (stunde > 23) {
      stunde = 0;
      tag++;
    }
    if (tag > tage_pro_monat[monat]) {
      tag = 1;
      monat++;
    }
    if (monat > 12) {
      if ((jahr % 4) == 0)  tage_pro_monat[2]--;
      monat = 1;
      jahr++;
      if ((jahr % 4) == 0)  tage_pro_monat[2]++;
    }
    if (is!=station || iy!=jahr || im!=monat || id!=tag || ih!=stunde) {
      vMsg(_invalid_sequence_$$_, n, s);
      vMsg(_expected_$$$$$_, station, jahr, monat, tag, stunde);
      vMsg(_found_$$$$$_, is, iy, im, id, ih);
      return -1;
    }
    sprintf(s, "%04d-%02d-%02d.%02d:00:00", iy, im, id, ih);
    te = MsgDateVal(s);
    TE(i) = MsgDateIncrease(te, 3600);  // GMT+1
    //
    // remap stability class
    //
    if (ik > 6) {
      if (ik == 7)  ik = 3;
      else          ik = 0;
    }
    else if (ik < 0)  ik = 0;
    //
    // check calms for consistency
    if ((iu == 0 && ir != 0) ||
    		  (iu != 0 && ir == 0))
    		inconsistent_calms++;                         //-2013-06-13  
    //
    //
    // set random values within steps
    // set wind direction for unsteady winds
    //
    if (format == 3) {
      if      (qr == 0) { ra = 10*ir;  dr = 5.0; }
      else if (qr == 1) { ra =    ir;  dr = 5.0; }
      else if (qr == 2) { ra =    ir;  dr = 0.5; }
      else              { 
        ra =     0;  dr = 0.0;  ik = 0; }
    }
    else { ra = 10*ir;  dr = 5.0; }
    if (ra > 360)  ra = 360*nextFloat();
    else {
      ra += dr*(2*nextFloat() - 1);
      if      (ra < 0 )    ra += 360;
      else if (ra >= 360)  ra -= 360;
    }
    //
    if (format == 3) {
      if      (qu == 0) { ua =  Kn*iu;  du = 0.50*Kn; }
      else if (qu == 1) { ua = 0.1*iu;  du =    0.05; }
      else if (qu == 2) { ua = Kn*((int)(0.5+0.1*iu/Kn));  du = 0.5*Kn; }
      else if (qu == 3) { ua = 0.1*iu;  du =    0.50; }
      else              { ua =    0.0;  du =    0.00;  ik = 0; }
    }
    else { ua = Kn*iu;  du = 0.5*Kn; }
    if (ua > du)  ua += du*(2*nextFloat() - 1);
    //
    pr = -1;                                        //-2012-04-06
    if (*modifier == 'N' && qe == 1) {														//-2011-12-02
    		if (ip > 0 && ip < 990)
    				pr = ip;
    		else if (ip >= 990 && ip < 1000)
    				pr = 0.1*(ip - 990);
    		else
    				pr = 0;
    }
    RA(i) = ra;       // ra in Grad
    UA(i) = ua;       // ua in m/s
    KL(i) = ik;       // Klug/Manier (1..6)
    HM(i) = ix;       // mixing height in m
    PR(i) = pr;       // pr in mm/h											  			               	//-2011-11-22
    i++;
    if (ik >= 0) {
      if (iu > 20)  iu = 20;
      Nkn[iu]++;
    }
    if (ix > 0)
      uses_hm = 1;                                                //-2011-12-02
    if (pr >= 0)
      uses_ri = 1;                                                //-2011-12-02
  }
  fclose(f);
  if (inconsistent_calms)
  		vMsg(_inconsistent_calms_$_, inconsistent_calms);
  strcpy(AKTheader, "form  \"te%20lt\" \"ra%5.0f\" \"ua%5.1f\" \"kl%3d\""  //-2014-01-28
      " \"hm%5.0f\" \"ri%5.1f\"\n");
  return uses_hm + 2*uses_ri;
eX_1:
  eMSG(_allocation_);
}

//=================================================================== TatCheckZtr
//
int TatCheckZtr( void ) 
{
  int i, ir;
  float u, u1, u2, r, r1, r2;
  int ik, n0;
  float w, frq[7][36];
  for (n0=0, i=1; i<=Nterm; i++)  if (KL(i) == 0)  n0++;
  if (Ha <= 0)  Ha = 10 + D0;                               //-2002-04-16
  //
  // get wind distribution for low wind speed
  //
  for (ik=0; ik<=6; ik++)
    for (ir=0; ir<36; ir++)  frq[ik][ir] = 0;
  for (i=1; i<=Nterm; i++) {
    ik = KL(i);
    if (ik==0 || ik>6)
      continue;
    u = UA(i);
    r = RA(i);
    ir = (int)(0.5 + 0.1*r);
    if (ir > 35)  ir = 0;
    if (u>0 && u<=TatUref) {
      frq[ik][ir] += 1;
    }
  }
  for (ik=1; ik<=6; ik++) {
    for (ir=1; ir<36; ir++)  frq[ik][ir] += frq[ik][ir-1];
    if (frq[ik][35] == 0)
      for (ir=0; ir<36; ir++)  frq[ik][ir] = (ir+1)/36.0;
    else
      for (ir=0; ir<36; ir++)  frq[ik][ir] /= frq[ik][35];
  }
  //
  // fill small gaps
  //
  for (i=2; i<Nterm; i++) {
    if (KL(i) == 0) {
      if (KL(i-1) == 0)
        continue;
      r1 = RA(i-1);
      u1 = UA(i-1);
      if (KL(i+1) > 0) {
        u2 = UA(i+1);
        r2 = RA(i+1);
        UA(i) = (u1 + u2)/2.0;
        if      (u1 == 0)  r1 = r2;
        else if (u2 == 0)  r2 = r1;
        else if (r2-r1 > 180)  r1 += 360;
        else if (r1-r2 > 180)  r2 += 360;
        r = (r1 + r2)/2.0;
        if (r >= 360)  r -= 360;
        RA(i) = r;
        KL(i) = 0.5 + (KL(i-1) + KL(i+1))/2.0;
      }
      else {
        if (i >= Nterm-1)
          break;
        if (KL(i+2) == 0)
          continue;
        u2 = UA(i+2);                                     //-2001-07-13
        r2 = RA(i+2);                                     //-2001-07-13
        UA(i)   = (2*u1 + u2)/3.0;
        UA(i+1) = (u1 + 2*u2)/3.0;
        if      (u1 == 0)  r1 = r2;
        else if (u2 == 0)  r2 = r1;
        else if (r2-r1 > 180)  r1 += 360;
        else if (r1-r2 > 180)  r2 += 360;
        r = (2*r1 + r2)/3.0;
        if (r >= 360)  r -= 360;
        RA(i) = r;
        r = (r1 + 2*r2)/3.0;
        if (r >= 360)  r -= 360;
        RA(i+1) = r;
        KL(i) = 0.5 + (2*KL(i-1) + KL(i+2))/3.0;
        KL(i+1) = 0.5 + (KL(i-1) + 2*KL(i+2))/3.0;
      }
    }
  }
  //
  // wind direction for calms
  //
  for (i=1; i<=Nterm; i++) {
    ik = KL(i);
    if (ik == 0)
      continue;
    u = UA(i);
    if (u > 0)
      continue;
    r = -1;
    if (i>1 && KL(i-1)>0 && UA(i-1)>0) {
      if (i<Nterm && KL(i+1)>0 && UA(i+1)>0) {
        // interpolate for 1 hour
        r1 = RA(i-1);
        r2 = RA(i+1);
        if      (r2-r1 > 180)  r1 += 360;
        else if (r1-r2 > 180)  r2 += 360;
        r = 0.5 + (r1 + r2)/2.0;
        if (r >= 360)  r -= 360;
        RA(i) = r;
      }
      else if (i<Nterm-1 && KL(i+2)>0 && UA(i+2)>0) {
        // interpolate for 2 hours
        r1 = RA(i-1);
        r2 = RA(i+2);
        if      (r2-r1 > 180)  r1 += 360;
        else if (r1-r2 > 180)  r2 += 360;
        r = (2*r1 + r2)/3.0;
        if (r >= 360)  r -= 360;
        RA(i) = r;
        r = (r1 + 2*r2)/3.0;
        if (r >= 360)  r -= 360;
        RA(i+1) = r;
        i++;
      }
    }
    if (r < 0) {
      // set r according to low wind speed distribution
      w = nextFloat();
      for (ir=0; ir<36; ir++)
        if (frq[ik][ir] >= w)
          break;
      r = 10*(ir+1) + Dr*(2*nextFloat() - 1);
      if      (r < 0)     r += 360;
      else if (r >= 360)  r -= 360;
      RA(i) = r;
    }
  }
  //
  // check u against Umin
  // calculate mean wind speeds
  //
  for (i=1; i<=Nterm; i++) {
    ik = KL(i);
    if (ik == 0)
      continue;
    u = UA(i);
    if (u < TatUlow)  UA(i) = TatUmin;
  }
  //
  // round wind direction and wind velocities
  //
  for (i=1; i<=Nterm; i++) {
    r = (int)(0.5 + RA(i));
    if (r == 0)  r = 360;
    RA(i) = r;
    u = UA(i);
    UA(i) = 0.1*((int)(0.5 + 10*u));
  }
  return n0;
}

//================================================================== TatGetHa
//
float TatGetHa( int zkl ) {                                        //-2002-04-16
  float ha;
  if (zkl<0 || zkl>=9)
    return Ha;
  ha = HaVec[zkl];                        // given with input data
  if (ha <= 0)  ha = Ha;
  return ha;
}

//======================================================================= Help
//
static void Help( void )
{
  /*
Aufruf:  TalAKTerm <Pfad> [<Option>]\n
Option:  -a<xa>,<ya>,<ha>   Anemometerposition (m)\n
         -B<WindLib> Pfad+Name der Windfeldbibliothek\n
         -f<AKTerm>  Name der DWD-Zeitreihe\n
         -l<l>       Ausgabe von WETLIB.DEF\n
                     <l>= Anzahl der Windrichtungen (2 oder 36)\n
         -m<Monat>   Zeitreihe nur fuer den Monat <Monat> \n
         -s<seed>    Anfangszahl f√ºr Zufallszahlengenerator\n
         -u<umin>    Mindestwert der Windgeschwindigkeit, Standard: %1.1f m/s\n
         -z<z0>,<d0> Z0 und D0 (m), Standard: %1.1f, %1.1f\n
  */
  printf("%s", _help01_);
  printf("%s", _help02_);
  printf("%s", _help03_);
  printf("%s", _help04_);
  printf("%s", _help05_);
  printf("%s", _help06_);
  printf("%s", _help07_);
  printf("%s", _help08_);
  printf(_help09_$_, TatUmin);
  printf(_help10_$$_, Z0, D0);
  exit(0);
}

//================================================================== TatArgument
//
int TatArgument( char *pc ) {
  int j;
  if (pc) {
    if (*pc == '-')
      switch (pc[1]) {
        case 'a': sscanf(pc+2, "%f,%f,%f", &Xa, &Ya, &Ha);
                  break;
        case 'B': sprintf(WindLib, "%s", pc+2);
                  break;
        case 'c': sscanf( pc+2, "%d", &ChkLevel );
                  break;
        case 'f': strcpy(DwdName, pc+2);
                  break;
        case 'l': j = sscanf( pc+2, "%d", &Wetlib );
                  if (j!=1 || (Wetlib!=2 && Wetlib!=36))  Wetlib = 0;
                  break;
        case 'h': Help();
                  break;
        case 'm': sscanf( pc+2, "%d", &SollMonat );
                  break;
        case 's': sscanf(pc+2, "%ld", &Seed);
                  break;
        /* 2013-01-11
        case 'u': j = sscanf( pc+2, "%f", &TatUmin );
                  if (j != 1) TatUmin = 0.5;
                  break;
        */
        /*   2011-09-12       
        case 'V': if (pc[2])  strcpy(BlmVersion, pc+2);
                  break;
        */
        case 'z': Z0 = 0.5;                                      //-2011-09-12
                  D0 = 3.0;                                      //-2011-09-12
                  sscanf(pc+2, "%f,%f", &Z0, &D0);
                  break;
        default:  break;
      }
    else {
      strcpy(Pfad, pc);
      for (pc=Pfad; (*pc); pc++)  if (*pc == '\\')  *pc = '/';
      if (*Pfad && pc[-1]=='/')  pc[-1] = 0;
    }
  }
  else {
    if ((*Pfad == 0) || (*Pfad == '?'))  Help();
    for (pc=DwdName; (*pc); pc++)  if (*pc == '\\')  *pc = '/';
  }
  return 0;
}

//==========================================================================
//
//  history:
//
//  2001-06-21 lj 0.13.0  final test version
//  2002-09-24 lj 1.0.0   final release candidate
//  2002-12-19 lj 1.0.4   MS adaption
//  2004-06-15 lj 1.1.19  mean wind for situations not represented in AKT
//  2005-03-17 uj 2.2.0   version number upgrade
//  2006-10-26 lj 2.3.0   external strings
//  2007-03-07 uj 2.3.6   calculation of uamean deleted
//  2011-07-07 uj 2.5.0   version number upgrade
//  2011-09-12 uj 2.5.1   blm version set by TalInp
//  2011-11-22 lj 2.6.0   precipitation
//  2011-12-02 lj         monitoring definition of Hm and Ri
//  2012-04-06 uj 2.6.3   detecting precipitation corrected
//  2013-01-11 uj 2.6.5   invalid umin parsing temporarily removed
//  2013-06-13 uj 2.6.6   warning for inconsistent calm definitions
//
//==========================================================================

