// ======================================================== TalRgl.h
//
// Calculate average roughness length for AUSTAL2000
// =================================================
//
// Copyright (C) Umweltbundesamt, Dessau-Roﬂlau, Germany, 2002
// Copyright (C) Janicke Consulting, 88662 ‹berlingen, Germany, 2002-2005
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
// last change:  2002-10-19 uj
//
//========================================================================

#define RGLE_FILE    -10
#define RGLE_XMIN    -11
#define RGLE_YMIN    -12
#define RGLE_DELT    -13
#define RGLE_GGCS    -14
#define RGLE_DIMS    -15
#define RGLE_ILOW    -16
#define RGLE_JIND    -17
#define RGLE_SEQN    -18
#define RGLE_SEQU    -19
#define RGLE_UERR    -99
#define RGLE_CSNE    -21
#define RGLE_GKGK    -22
#define RGLE_POUT    -23
#define RGLE_SECT    -24


extern double RglXmin, RglXmax, RglYmin, RglYmax, RglDelta;
extern double RglX, RglY, RglA, RglB;
extern int RglMrd;
extern char RglFile[], RglGGCS[];
extern unsigned int RglCRC;


int TrlReadHeader( 	  // read the header of the ArcInfo-File
char *path,          // directory that contains the cataster
char *ggcs)          // coordinate system
;
float TrlGetZ0( 	   // calculate the average value of z0 within a circle
char *cs,           // geographic coordinate system chosen in a2k input file    
double x, 		        // x-coordinate of the circle (GK3-Rechts)
double y, 		        // y-coordinate of the circle (GK3-Hoch)
double r, 		        // radius of the circle (m)
char **pbuf ) 		    // optionally a print image of the area is returned
;
//=====================================================================
