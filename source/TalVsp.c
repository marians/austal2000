//=========================================================== TalVsp.c
//
// Evaluation of VDISP calculations
// ================================
//
// Copyright (C) Umweltbundesamt, Dessau-Roﬂlau, Germany, 2002-2011
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
// last change:  2011-12-21 lj
//
//==================================================================

#include <stdio.h>
#include <string.h>
#include <math.h>

#include "TalVsp.h"
#include "TalVsp.nls"

#ifdef __linux__
  #define  VDISP  "vdisp"
#else
  #define  VDISP  "vdisp.exe"
#endif

#if defined _M_IX86 && !defined __linux__
    #define popen _popen
#endif

#define MAXVAL 500

int VspAbortCount = 0;
char VspAbortMsg[256];

static float Ex[6] = { 0.42, 0.37, 0.28, 0.22, 0.20, 0.09 };

static char *NmInput = "VDIIN.DAT";
static char *NmOutput = "VDIOUT.DAT";
static char *NmProgram = VDISP;

static char VspWork[1024] = ".";
static float Xv[MAXVAL], Hv[MAXVAL];
static int Nv;

static float Step = 0.01;
static float Tick = 0.3;
static float Dist = 30;

void VspSetWork(    // Set the work directory
    char *work)     // work directory
{
  strcpy(VspWork, work);
}

char *VspCallVDISP( // return: NULL or error message
char *nm,           // source name (used for file name construction)
float dm,           // source diameter (m)
float hg,           // source height (m)
float vl,           // exit velocity (m/s)
float tm,           // exit temperature (Celsius)
float hm,           // relative humidity (%)
float lq,           // liquid water content (kg/kg)
float uh,           // wind velocity at source height (m/s)
float cl,           // stability class (1.0, 2.0, 3.1, 3.2, 4.0, 5.0)
float *dhh,         // result: half height of plume rise
float *dxh )        // result: source distance where hh is reached
{
  FILE *f;
  int i, n, drop, kl;
  float x, h, hmax, xl, hl, a, ur;
  static char buf[400], msg[256];
  char InName[1024], OutName[1024], CmdString[1200];              //-2011-12-21
  //
  *dhh = 0;
  *dxh = 0;
  *buf = 0;
  *msg = 0;
  if (cl<1 || cl>5 || uh<=0 || uh>100) {                          //-2006-03-24
    sprintf(buf, _invalid_parameter_$$_, cl, uh);
    return buf;
  }
  kl = (int)(cl + (cl>3.15)) - 1;
  ur = uh/pow(hg/10.0, Ex[kl]);
  sprintf(InName, "%s/%s", VspWork, NmInput);                     //-2011-12-21
  f = fopen(InName, "w");
  if (!f) {
    sprintf(buf, _cant_open_$_, InName);
    return buf;
  }
  fprintf(f, "%20s\n", nm);
  fprintf(f, "%7.1f %7.1f %7.1f %7.1f %7.1f %9.3f\n", dm, hg, vl, tm, hm, lq);
  fprintf(f, "%7.1f %7.1f %8.2f %7.1f %6.0f\n", ur, cl, Step, Tick, Dist);
  fclose(f);
  sprintf(OutName, "%s/%s", VspWork, NmOutput);                   //-2011-12-21
  remove(OutName);
  f = fopen(NmProgram, "r");
  if (f) {
    fclose(f);
    sprintf(CmdString, "%s \"%s\"", NmProgram, VspWork);          //-2011-12-21
    f = popen(CmdString, "r");
  }
  if (!f) {
    sprintf(buf, _no_connection_$_, NmProgram);
    printf(_$_not_found_, NmProgram);
    return buf;
  }
  while (fgets(buf, 400, f)) {
    // printf("%s:%s", NmProgram, buf);
  }
  fclose(f);
  f = fopen(OutName, "r");                                        //-2011-12-21
  if (!f) {
    sprintf(buf, _cant_read_$_, OutName);
    return buf;
  }
  n = 0; 
  drop = 1;
  hmax = 0;
  while(fgets(buf, 400, f)) {
    if (strchr(buf, '*') != NULL) {       //-2004-11-30
      strcat(msg, buf);
      break;
    }
    if (drop) {
      if (strstr(buf, "DISTANCE"))  drop = 0;
      continue;
    }
    if (strlen(buf) < 10)
      continue;
    if (strstr(buf, "DISTANCE"))
      continue;
    if (2 != sscanf(buf, "%f %f", &x, &h)) {
      sprintf(buf, _read_error1_$_, n+1);
      fclose(f);
      return buf;
    }
    if (n<MAXVAL) {                                     //-2006-10-20
      Xv[n] = x;
      Hv[n] = h;
    }
    if (h > hmax)  hmax = h;
    n++;
  }
  if (drop || strlen(msg)>0) {                           //-2004-11-30
    fclose(f);
    *dhh = 0;
    *dxh = 0;
    if (VspAbortCount == 0 && strlen(msg) > 0) {
      if (msg[strlen(msg)-1] == '\n')
        msg[strlen(msg)-1] = 0;
      strcpy(VspAbortMsg, msg); 
    }
    VspAbortCount++;
    return NULL;
  }
  Nv = n;
  *dhh = hmax/2;
  fseek(f, 0, SEEK_SET);
  i = 0;
  drop = 1;
  xl = 0;
  hl = 0;
  while(fgets(buf, 400, f)) {
    if (drop) {
      if (strstr(buf, "DISTANCE"))  drop = 0;
      continue;
    }
    if (strlen(buf) < 10)
      continue;
    if (strstr(buf, "DISTANCE"))
      continue;
    if (2 != sscanf(buf, "%f %f", &x, &h)) {
      sprintf(buf, _read_error2_$_, i+1);
      fclose(f);
      return buf;
    }
    if (h > hmax/2) {
      a = (hmax/2 - hl)/(h - hl);
      *dxh = xl + a*(x-xl);
      break;
    }
    xl = x;
    hl = h;
    i++;
  }
  fclose(f);
  return NULL;
}
//=========================================================================
//
// history:
// 
// 2002-09-24 lj  final release candidate
// 2004-11-30 uj  check for VDISP abort
// 2006-03-24 uj  maximum uh increased to 100 m/s
// 2006-10-20 uj  check corrected
// 2006-10-26 lj  external strings
// 2011-12-21 uj  in and out from project directory
//
//=========================================================================

