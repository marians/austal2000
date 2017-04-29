//================================================================== TalPlm.h
//
// Calculation of plume rise
// =========================
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
// last change: 2002-12-10 lj
//
//==========================================================================

#define  PLM_VDI3782_3	1
#define  PLM_M9440		  2
#define  PLM_PLURIS		  3
#define  PLM_VDI3784_2  4

#include <stdio.h>

extern char *PlmMsg;
extern FILE *PlmLog;

int TalPlm( 	//  plume rise  
int type, 		//  model type
float *pdh, 	//  calculated rise height [m]
float *pxe,		//  calculated rise distance [m]    
float qq,		  //  heat emission [MW] 
float hq,		  //  source height [m]
float uq,		  //  wind speed at source height [m/s] 
float class,	//  stability class [Klug/Manier]
float vq,		  //  gas velocity [m/s] 
float dq, 		//  stack diameter [m]
float rhy,    //  relative humidity [%]
float lwc,    //  liquid water content [kg/kg]
float tmp )   //  gas temperature [Celcius]
;

//=============================================================================
