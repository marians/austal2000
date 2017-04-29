//=========================================================== TalVsp.h
//
// Evaluation of VDISP calculations
// ================================
//
// Copyright (C) Umweltbundesamt, Dessau-Roﬂlau, Germany, 2002-2004
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
// last change:  2004-11-30 uj
//
//==================================================================

char *VspCallVDISP( // return: NULL or error message
char *nm,           // source name (used for file name construction)
float dm,           // source diameter (m)
float hg,           // source height (m)
float vl,           // exit velocity (m/s)
float tm,           // exit temperature (Celsius)
float hm,           // relative humidity (%)
float lq,           // liquid water content (kg/kg)
float uh,           // wind velocity at source height (m/s)
float cl,           // stability class (1.0, 2.0, 3.1, 3.2, ...)
float *dhh,         // result: half height of plume rise
float *dxh )        // result: source distance where hh is reached
;
void VspSetWork(    // Set the work directory
    char *work)     // work directory
;

extern int VspAbortCount;
extern char VspAbortMsg[256];
//===========================================================================

