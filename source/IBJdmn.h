//======================================================== IBJdmn.h
//
// Data Manager for IBJ-programs
// =============================
//
// Copyright (C) Umweltbundesamt, Dessau-Roﬂlau, Germany, 2002-2005
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
//==================================================================

#ifndef  IBJDMN_INCLUDE
#define  IBJDMN_INCLUDE

#ifndef  IBJARY_INCLUDE
  #include  "IBJary.h"
#endif

#ifndef  IBJTXT_INCLUDE
  #include  "IBJtxt.h"
#endif

extern char *IBJdmnVersion;

enum DAT_TYPE { noDAT, chDAT, btDAT, siDAT, inDAT, flDAT, tmDAT, ltDAT, lfDAT, liDAT };

typedef struct {
  char name[40];
  enum DAT_TYPE dt;
  int ld, lf, offset;
  char opt[40], iform[40], oform[40], pform[40];
  double fac;
  char fspc;
  } DMNFRMREC;

extern DMNFRMREC *DmnFrmTab;

DMNFRMREC *DmnAnaForm(  // analyse format combination
char *frm,              // extended format
int *psize )            // record size
;                       // RETURN: format table

int DmnPrnForm(   // print table of formats
DMNFRMREC *pf,    // pointer to table of formats
char *s )         // comment
;                 // RETURN: number of format items

DMNFRMREC *DmnFrmPointer( // get record pointer of element
DMNFRMREC *pfrm,          // format table for the record
char *name )              // name of the element (case ignored)
;                         // RETURN: pointer or NULL, if not found

void DmnReplaceEscape(  // replace ansi escape codes
char *s )               // source and destination string
;

int DmnPrintR( // print data record to a file
void *f,       // file pointer
int (*prn)(void*, const char*, ...),  // print routine
DMNFRMREC *pf, // format table
double fact,   // overal factor
double shift,  // time shift in days
void *pr)      // pointer to data record
 ;             // RETURN: number of items written out


char *DmnFindName(  // Find a parameter name in the header
char *buf,          // header (lines separated by \n)
char *name )        // parameter name
  ;                 // RETURN: pointer to name string

char *DmnGetValues( // Get the value string of a parameter
char *hdr,          // header
char *name,         // name of the parameter
int *pl )           // set to length of the value string
  ;                 // RETURN: pointer to value string

int DmnRplValues(   // Replace the value string of a parameter
TXTSTR *phdr,       // header
char *name,         // name of the parameter
char *values )      // new values (or NULL)
;                   // RETURN: 0 = replaced or deleted, 1 = added

int DmnRplName(     // Replace the name string of a parameter
TXTSTR *phdr,       // header
char *oldname,      // old name of the parameter
char *newname )     // new name (or NULL to delete)
;                   // RETURN: 0 = not found, 1 = replaced, 2 = deleted

int DmnRplChar(     // Replace a character in the value string
TXTSTR *phdr,       // header
char *name,         // parameter name
char old,           // old character
char new )          // new character
;                   // RETURN: number of replacements

int DmnGetCount(  // Get the number of tokens of a parameter
char *hdr,        // header
char *name )      // name of the parameter
;                 // RETURN: number of tokens for this parameter

int DmnGetInt(    // Get integer values of a parameter
char *hdr,        // header
char *name,       // name of the parameter
char *frm,        // format for reading
int *ii,          // integer vector
int n )           // maximum number of positions in ii[]
;                 // RETURN: number of values read

int DmnPutInt(    // Put integer values of a parameter into header
char *hdr,        // header
int len,          // maximum length of header string
char *name,       // name of the parameter
char *frm,        // format for writing (with separator!)
int *ii,          // integer vector
int n )           // number of values to write
;                 // RETURN: number of values written

int DmnGetFloat(  // Get float values of a parameter
char *hdr,        // header
char *name,       // name of the parameter
char *frm,        // format for reading
float *ff,        // float vector
int n )           // maximum number of positions in ff[]
;                 // RETURN: number of values read

int DmnPutFloat(  // Put float values of a parameter into header
char *hdr,        // header
int len,          // maximum length of header string
char *name,       // name of the parameter
char *frm,        // format for writing (with separator!)
float *ff,        // float vector
int n )           // number of values to write
;                 // RETURN: number of values written

int DmnGetString( // Get string values of a parameter
char *hdr,        // header
char *name,       // name of the parameter
char **ss,        // string vector
int n )           // maximum number of positions in ss[]
;                 // RETURN: number of values read

int DmnCpyString( // Copy a string value from header
char *hdr,        // header
char *name,       // name of the parameter
char *dst,        // destination string
int n )           // maximum number of characters
;                 // RETURN: 0 on success

int DmnGetDouble( // Get double values of a parameter
char *hdr,        // header
char *name,       // name of the parameter
char *frm,        // format for reading
double *ff,       // double vector
int n )           // maximum number of positions in ff[]
;                 // RETURN: number of values read

int DmnPutDouble( // Put double values of a parameter into header
char *hdr,        // header
int len,          // maximum length of header string
char *name,       // name of the parameter
char *frm,        // format for writing (with separator!)
double *ff,       // double vector
int n )           // number of values to write
;                 // RETURN: number of values written

int DmnPutString( // Write string values of a parameter into header
char *hdr,        // header
int len,          // maximum length of header string
char *name,       // name of the parameter
char *frm,        // format for writing (with separator and \"...\" !)
char **ss,        // string vector
int n )           // number of strings to write
;                 // RETURN: number of strings written

int DmnCnvHeader( // convert array header to user header
char *arrhdr,     // array header
TXTSTR *ptxt )    // user header
;
int DmnArrHeader( 	// convert user header to array header
char *s, 						// user header
TXTSTR *ptxt ) 			// array header
;
int DmnWrite(     // Write data into a file
char *fn,         // file name (with path, without extension)
char *hdr,        // header string
ARYDSC *pa )      // address of array descriptor
;
int DmnRead(      // Read data from a file
char *fn,         // file name (with path, without extension)
TXTSTR *pu,       // user header text
TXTSTR *ps,       // system header text
ARYDSC *pa )      // address of array descriptor
;
char *DmnGetPath( char *fn )
;
char *DmnGetFile( char *fn )
;
int DmnFileExist( char *fn )
;
int Dmn2ArcInfo(  // translate 2D file to ArcInfo format
char *fin,        // input file name
char *fout)       // output file name or NULL
;
int DmnDatAssert(  // verify value of a parameter
char *name,        // name of the variable  
char *hdr,         // string containing assignments 
float val )        // required value
;


#endif  /*#################################################################*/
