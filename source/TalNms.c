//=================================================================== TalNms.c
//
// AUSTAL2000 name server
// ======================
//
// Copyright (C) Umweltbundesamt, Dessau-Roﬂlau, Germany, 2002
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
// last change: 2011-06-29 uj
//
//==========================================================================

#include <string.h>
#include <stdio.h>
#include "TalNms.h"

char *DataNames[] = { "noa",
  "grdp", "grda", "grdt", "srfp", "srfa", "topa", "slda",
  "prfa", "metp", "meta", "blma", "blmp", "blmt",
  "w"   , "v"   , "k"   ,
  "moda", "vrfa", "siga",
  "vdpa", "wdpa", "xfra", "dosp", "dosa", "suma", "d",
  "ppma", "ptla", "ptlt", "xtra", "adda", "simp",
  "afua", "g"   , "gamm", "c"   , "spca", "spga", "avda", "avsa", "dtba", "mnta",
  "dtbt", "errm", "wrkp", "amna", "prmp", "asla", "cmpt",
  "srca", "srct", "emsa", "chma", "mona", "l"   ,
  "inva", "liba", "anya", "" };

static int Sequence, Average, Index;
static char Windlib[256];

  /*================================================================== NmsSeq
  Set parameter Sequence and Average.
  */
void NmsSeq( char *name, int value )
  {
  switch (*name) {
    case 'a':  Average = value;
               break;
    case 'i':  Index = (value > 0) ? value : 0;
               break;
    case 's':  Sequence = value;
               break;
    case '@':  strncpy( Windlib, name+1, 255 );
               break;
    default:   ;
    }
  }

  /*================================================================= NmsName
  */
char *NmsName(      /* file name used on disk           */
  long ident )      /* identification of the table      */
  {
  int gp, nl, ni, seq;
  enum DATA_TYPE dt;
  char cgp, cnl, cni, xtn[40];
  static char filename[256], name[256];
  strcpy(xtn, "dmna");                                            //-2011-06-29
  dt = XTR_DTYPE(ident);
  if (ident<0 || dt==NOarr || dt>ANYarr) {
    sprintf(filename, "%08lx.%s", ident, xtn);                    //-2011-06-29
    return filename;
  }
  strcpy( name, DataNames[dt] );
  gp = XTR_GROUP(ident);
  if (dt==WNDarr || dt==VARarr || dt==KFFarr || dt==CONtab || dt==DOStab
   || dt==GAMarr || dt==LMDarr) {
    if (gp > 25)  gp = 26;
    cgp = "abcdefghijklmnopqrstuvwxyz-"[gp];
  }
  else {
    if (gp > 35)  gp = '-';
    else cgp = (gp > 9) ? 'a'+gp-10 : '0'+gp;
  }
  nl = XTR_LEVEL(ident);  cnl = (nl > 9) ? 'a'+nl-10 : '0'+nl;
  ni = XTR_GRIDN(ident);  cni = (ni > 9) ? 'a'+ni-10 : '0'+ni;
  if (dt==CONtab || dt==DOStab || dt==GAMarr) {
    seq = Sequence;
    if (Average > 1)  seq = 1 + (seq-1)/Average;
    sprintf(filename, "%s%04d%c%c%c.%s", name, seq, cgp, cnl, cni, xtn);
  }
  else if (dt==WNDarr || dt==VARarr || dt==KFFarr || dt==LMDarr)
    sprintf( filename, "%s%s%04d%c%c%c.%s",
      Windlib, name, Index, cgp, cnl, cni, xtn );
  else
    sprintf(filename, "%s%c%c%c.%s", name, cgp, cnl, cni, xtn);
  return filename;
}

//===========================================================================
//
// history:
//
// 2002-09-24 lj  final release candidate
// 2011-06-29 uj  DMNA files
//
//===========================================================================

