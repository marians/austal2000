//======================================================== TALuti.h
//
// Utilities for AUSTAL2000
// ========================
//
// Copyright (C) Umweltbundesamt, Dessau-Roﬂlau, Germany, 2008
// Copyright (C) Janicke Consulting, 88662 ‹berlingen, Germany, 2002-2008
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
// last change:  2008-12-07  lj
//
//==================================================================

int TutLogMax(    // write maximum values into log-file
char *path,       // working directory
char *file,       // file name base
int k,            // layer index
int ii,           // sequence count
int nn )          // number of grids
;
int TutLogMon(    // write values at monitor points into log-file
char *path,       // working directory
char *file,       // file name base
int prec,         // number of decimal positions
int ii )          // sequence count
;
float TutCalcRated( // calculate rated odor hour frequency according to GIRL2008
    float hs,       // odor hour frequency for sum of odor components
    int na,         // number of additional (rated) odor components
    float *fa,      // rating factors
    float *fr)      // odor hour frequency for rated component
;
int TutEvalOdor(    // evaluate rated odor components
    char *path,     // working directory
    int na,         // number of additional odor components
    char *names[],  // component names
    char *dst,      // name of the resulting component
    int nn)         // number of grids
;
//=======================================================================
