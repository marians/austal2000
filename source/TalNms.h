//=================================================================== TalNms.h
//
// AUSTAL2000 name server
// ======================
//
// Copyright (C) Umweltbundesamt, Dessau-Roßlau, Germany, 2002
// Copyright (C) Janicke Consulting, 88662 Überlingen, Germany, 2002-2005
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
// last change: 2002-06-26 lj
//
//==========================================================================

#ifndef  TALNMS_INCLUDE
#define  TALNMS_INCLUDE
 
#define  NMS_IDENT   0x00FFFFFFL
#define  NMS_CMMND   0x7F000000L
#define  NMS_DTYPE   0x00FF0000L
#define  NMS_GROUP   0x0000FF00L
#define  NMS_LEVEL   0x000000F0L
#define  NMS_GRIDN   0x0000000FL

#define  XTR_IDENT(a)   ((a)&NMS_IDENT)
#define  XTR_COMMAND(a) (((a)&NMS_CMMND)>>24)
#define  XTR_DTYPE(a)   (((a)&NMS_DTYPE)>>16)
#define  XTR_GROUP(a)   (((a)&NMS_GROUP)>>8)
#define  XTR_LEVEL(a)   (((a)&NMS_LEVEL)>>4)
#define  XTR_GRIDN(a)   ((a)&NMS_GRIDN)

#define  IDENT(dt,gp,nl,ni) (((long)(dt)&0xFFL)<<16|((gp)&0xFFL)<<8|\
                              ((nl)&0xFL)<<4|((ni)&0xFL))

enum DATA_TYPE { NOarr, 
  GRDpar, GRDarr, GRDtab, SRFpar, SRFarr, TOParr, SLDarr,
  PRFarr, METpar, METarr, BLMarr, BLMpar, BLMtab,
  WNDarr, VARarr, KFFarr, 
  MODarr, VRFarr, SIGarr, 
  VDParr, WDParr, XFRarr, DOSpar, DOSarr, SUMarr, DOStab,
  PPMarr, PTLarr, PTLtab, XTRarr, ADDarr, SIMpar, 
  AFUarr, GAMarr, GAMmat, CONtab, SPCarr, SPGarr, AVDarr, AVSarr, DTBarr, MNTarr,
  DTBtab, ERRmsg, WRKpar, AMNarr, PRMpar, ASLarr, CMPtab,
  SRCarr, SRCtab, EMSarr, CHMarr, MONarr, LMDarr,
  INVarr, LIBtab, ANYarr };

extern char *DataNames[];

  /*================================================================== NmsSeq
  Set parameter used in forming the name.
  */
void NmsSeq( char *name, int value )
  ;
  /*================================================================= NmsName
  */
char *NmsName(      /* file name used on disk     */
  long ident )      /* identification of the table  */
  ;

/*===========================================================================*/
#endif
