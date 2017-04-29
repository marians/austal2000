//========================================================= IBJntr.h
//
// Net transformations for surface definition files
// ================================================
//
// Copyright (C) Umweltbundesamt, Dessau-Roﬂlau, Germany, 2008
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
// history:
//
// 2014-01-21 uj
//
//==================================================================

#ifndef IBJNTR_INCLUDE
#define IBJNTR_INCLUDE

#ifndef IBJARY_INCLUDE
  #include "IBJary.h"
#endif

#define  NTR_UNDEF        -999.
#define  NTR_DUNDEF       -1.e20                                  //-2014-01-21
#define  NTR_TYPE_ARC     0
#define  NTR_TYPE_DMN     1
#define  NTR_TYPE_XYZ     2
#define  NTR_TYPE_UNDEF   -1

typedef struct {
  int nx, ny;
  double gkx, gky, xmin, xmax, ymin, ymax, delta;
  char ggcs[32];                                                  //-2008-12-11
}  NTRREC;

extern char *NtrVersion;
extern ARYDSC NtrDsc;

//------------------------------------------------------------ NtrGetInRec
/** returns net informations of the input file */
NTRREC NtrGetInRec( void );

//----------------------------------------------------------- NtrGetOutRec
/** returns net informations of the output file */
NTRREC NtrGetOutRec( void );

//----------------------------------------------------------- NtrSetOutRec
/** sets net informations for the output file */
void  NtrSetOutRec(NTRREC f);

//--------------------------------------------------------- NtrSetSequence
/** sets the index sequence for the output file */
void  NtrSetSequence(char *s);

//------------------------------------------------------------- NtrSetMode
/** sets the write mode for the output file (text or binary) */
void  NtrSetMode(char *s);

//------------------------------------------------------- NtrSetXYZIndices
/** sets the data column indices for input files of type XYZ */
void  NtrSetXYZIndices(int i, int j, int k);

//--------------------------------------------------------- NtrReadArcFile
/** reads in a file of type ARcInfo */
int NtrReadArcFile(char *name);

//--------------------------------------------------------- NtrReadXYZFile
/** reads in a file of type XYZ */
int NtrReadXYZFile(char *name);

//--------------------------------------------------------- NtrReadDmnFile
/** reads in a file of type DMN */
int NtrReadDmnFile(char *name);

//----------------------------------------------------------- NtrWriteFile
/** writes out data as DMN file */
int NtrWriteFile(char *name);

//------------------------------------------------------------ NtrReadFile
/** checks the file type and reads in the data */
int NtrReadFile(char *name);

#endif
