//============================================================ TalUtl.h
//
// Utility functions for AUSTAL2000
// ================================
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
// last change:  2002-09-24  lj
//
//==================================================================

#ifndef TALUTL_INCLUDE
#define TALUTL_INCLUDE

int TutDirExists( // check, if a directory exists
char *path )      // directory name
;                 // RETURN : 1 if exists

int TutFileExists(  // check, if a file exists (and can be read)
char *path )        // file name
;                   // RETURN : 1 if exists

int TutDirMake(   // create a directory (recursive)
char *path )      // directory name
;                 // RETURN : 0 if created, 1 if exists, -1 on error

int TutFileCopy(    // copy a file
char *srcname,      // name of source file
char *dstname )     // name of destination file
;                   // RETURN : number of bytes copied

int TutDirCopy(   // copy a directory with its files
char *srcpath,    // name of source directory
char *dstpath )   // name of destination directory
;                 // RETURN : number of files copied

int TutDirClearOnDemand(  // delete the files in a directory
char *path )      // name of the directory
;                 // RETURN : number of files deleted or -1 if error/exit


int TutDirClear(  // delete the files in a directory
char *path )      // name of the directory
;                 // RETURN : number of files deleted

int TutDirRemove( // delete the directory
char *path )      // name of the directory
;                 // RETURN : error

char **_TutGetFileList( // get the file names
char *path )            // directory to be searched
;                       // RETURN: listing, followed by NULL

char **_TutGetDirList(  // get the directory names
char *path )            // directory to be searched
;                       // RETURN: listing, followed by NULL

int TutCheckName(   // replace backslash and trailing slash
  char *fn )        // file name
;   
int TutMakeName(  // construct absolute file name
  char *fn,       // result (buffer > 255)
  char *path,     // path
  char *name )    // file name
;
unsigned int TutMakeCrc(  // calculate the CRC32 of a character array
    unsigned char *buf,   // character array
    int len)              // length of array
;
unsigned int TutGetCrc(   // calculate the CRC32 of a file
    char *fn)             // file name
;

//=====================================================================
#endif
