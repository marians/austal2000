//=================================================================== genio.c
//
// Basic I/O-functions
// ===================
//
// Copyright (C) Umweltbundesamt, Dessau-Ro�lau, Germany, 2002-2005
// Copyright (C) Janicke Consulting, 88662 �berlingen, Germany, 2002-2005
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
// last change 2004-10-15 uj
//
//==================================================================

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <ctype.h>
#include <errno.h>

#include "IBJmsg.h"
#include "IBJary.h"
static char *eMODn = "GenIO";

#include "genutl.h"
#include "genio.h"

char InputPath[256] = "";
char DataPath[256] = "";
char InputBuf[INBUFSIZE] = "";
int  InBufLen = 0;
FILE *InputFile = NULL;
long InputPos;
char GenInputName[256];
int  GenHdrLen = GENHDRLEN;

static char PCcode[] = { 0x84,0x94,0x81,0x8E,0x99,0x9A,0xE6,0xE1,0xFD,0xFC,0x00 };
static char NXcode[] = { 0xD9,0xF0,0xF6,0x85,0x96,0x9A,0x9D,0xFB,0xC9,0xCC,0x00 };
static char ANcode[] = { 0xE4,0xF6,0xFC,0xC4,0xD6,0xDC,0xB5,0xDF,0xB2,0xB3,0x00 };
static char *SRCcode = ANcode;
static char *DSTcode = ANcode;

static int DEBUG = 0;

//============================================================= GenCheckPath
void GenCheckPath( char *path, int append )
{
  char *p;
  for (p=path; (*p); p++)  if (*p == '\\')  *p = '/';
  if (!append)  return;
  if (strlen(path)>0 && p[-1]!=':' && p[-1]!='/')
    strcpy(p, "/");
}

/*============================================================== GenDefCode
*/
void GenDefCode( char *conversion )
  {
  char *pc;
  pc = strchr(conversion, '2');
  if (pc) {
    *pc = 0;
    if      (!strcmp(conversion,"an"))  SRCcode = ANcode;
    else if (!strcmp(conversion,"pc"))  SRCcode = PCcode;
    else if (!strcmp(conversion,"nx"))  SRCcode = NXcode;
    pc++;
    }
  else  pc = conversion;
  if      (!strcmp(pc, "an"))  DSTcode = ANcode;
  else if (!strcmp(pc, "pc"))  DSTcode = PCcode;
  else if (!strcmp(pc, "nx"))  DSTcode = NXcode;
  }

/*============================================================= GenTranslate
*/
int GenTranslate( char *s )
  {
  int i, l;
  char *pc;
  if (SRCcode == DSTcode)  return 0;
  l = strlen(s);
  for (i=0; i<l; i++) {
    pc = strchr(SRCcode, s[i]);
    if (pc)  s[i] = DSTcode[pc-SRCcode];
    }
  return l;
  }

/*=================================================================== Printf
*/
int Printf(             /* Ausgabe mit Umsetzung der Umlaute */
char *format,           /* Formatstring der Meldung          */
... )                   /* Variable Teile der Meldung        */
  {
  char t[4000];                 /*-05feb96-*/
  int n;
  va_list argptr;
  va_start(argptr, format);
  vsprintf(t, format, argptr);
  va_end(argptr);
  if (SRCcode != DSTcode)  GenTranslate(t);
  n = printf("%s", t);  fflush(stdout);
  return n;
  }

/*=================================================================== fPrintf
*/
int fPrintf(           /* Ausgabe mit Umsetzung der Umlaute */
FILE *stream,           /* Ausgabe-File                         */
char *format,           /* Formatstring der Meldung          */
... )                   /* Variable Teile der Meldung        */
  {
  char t[4000];                 /*-05feb96-*/
  int n;
  va_list argptr;
  va_start(argptr, format);
  vsprintf(t, format, argptr);
  va_end(argptr);
  n = GenTranslate(t);
  n = fprintf(stream, "%s", t);  fflush(stream);
  return n;
  }

/*====================================================================== Disp
*/
long Disp(         /*  Text s in der Display-Zeile ausgeben.      */
char *s )
  {
  char t[80];
  int l;
  l = strlen(s);
  if (l > 79)  l = 79;
  memset(t, ' ', 79);  t[79] = 0;
  strncpy(t, s, l);
  Printf("\r%s\r", t);  fflush(stdout);
  return 0;
  }

/*=============================================================== SetDataPath
*/
long SetDataPath(     /* Pfad fuer Ein- und Ausgabe merken.             */
char *pfad )          /* Pfad.                                         */
  {
  strcpy( DataPath, pfad );
  return 0; }

/*================================================================= SeekInput
*/
long SeekInput(       /* Im Eingabe-File Position aufsuchen.           */
long pos )            /* Aufzusuchende Lese-Position.                  */
                      /* RETURN: pos                                   */
  {
  dP(SeekInput);
  long rc;
  if (!InputFile)                                       eX(1);
  rc = fseek(InputFile, pos, SEEK_SET);  if (rc)        eX(2);
  InputPos = pos;
  InputBuf[0] = 0;
  InBufLen = 0;
  return pos;
eX_1:
  eMSG("no input file opened!");
eX_2:
  eMSG("improper seek request!");
  }

/*================================================================= OpenInput
*/
long OpenInput(       /*  Eingabefile auf dem gemerkten Pfad öffnen.   */
char *name,           /*  Name des Eingabe-Files.                      */
char *altn )          /*  Alternativer Name für Verbund-File           */
  {                   /*  RETURN: Start-Position im File               */
  dP(OpenInput);
  int l, irc;
  char sn[80], *pc;
  if (InputFile != NULL) {
    irc = fclose(InputFile);  if (irc == EOF)                   eX(1);
    *InputPath = 0;  *InputBuf = 0;  InBufLen=0;
    }
  if (strchr(name,MSG_PATHSEP) || strchr(name,':'))  *InputPath = 0;
  else  strcpy(InputPath, DataPath);
  strcat(InputPath, name);
  *GenInputName = 0;                                      //-2001-05-04 lj
  InputFile = fopen(InputPath, "rb");
  InputPos = 0;
  pc = InputBuf;
  if (InputFile) {
    strcpy(GenInputName, name);                           //-2001-05-04 lj
    while (fgets(pc, INBUFSIZE, InputFile)) {             //-2001-04-29 lj
      if (*pc!='-' && *pc!='=') {                         //
        fseek(InputFile, InputPos, SEEK_SET);             //
        return InputPos;                                  //
      }                                                   //
      InputPos = ftell(InputFile);                        //
    }                                                     //
    fclose(InputFile);                                    //
  }
  if ((!altn) || (!*altn))  return -1;
  strcpy(InputPath, DataPath);
  strcat(InputPath, altn);
  InputFile = fopen(InputPath, "rb");  if (!InputFile)    eX(3);
  strcpy(sn, "- ");
  strcat(sn, name);
  while (fgets(InputBuf, INBUFSIZE, InputFile)) {
    if (*InputBuf!='-' && *InputBuf!='=')  continue;      //-2001-04-29 lj
    for (l=strlen(InputBuf)-1; InputBuf[l]<=' '; l--)
      InputBuf[l] = 0;
    l = strlen(InputBuf) - strlen(sn);
    if (l < 0)  continue;
    *sn = *InputBuf;                                      //-2001-04-29 lj
    if (CisCmp(sn, InputBuf+l))  continue;
    InputPos = ftell(InputFile);
    *InputBuf = 0;
    strcpy(GenInputName, altn);                           //-2001-05-04 lj
    return InputPos;
    }
  *InputBuf = 0;
  fclose(InputFile);
  InputFile = NULL;
  eX(10);
eX_10:
  eMSG("section %s not found in alternate input file %s!", name, InputPath);
eX_1:
  eMSG("error in closing %s!", InputPath);
eX_3:
  eMSG("can't open alternate input file %s!", InputPath);
  }

/*=================================================================== CntLines
*/
long CntLines(        /* Anzahl der Eingabe-Sätze feststellen.         */
char c,               /* Kenn-Zeichen für Abbruch.                     */
char *s,              /* Zeichenkette zur Aufnahme der Kenn-Zeichen.   */
int ls )              /* Maximale Länge der Zeichenkette.              */
  {
  dP(CntLines);
  char *pc;
  long rc;
  int n;
  if (!InputFile)                                               eX(1);
  InBufLen = 0;
  *InputBuf = 0;
  rc = fseek(InputFile, InputPos, 0);  if (rc != 0)             eX(2);
  for (n=0; n<ls-1; ) {
    pc = fgets(InputBuf, INBUFSIZE, InputFile);
    if (pc == NULL)  break;
    if (*InputBuf == c)  break;
    if (*InputBuf == 0x1A)  break;  /* ^Z */
    if (NULL != strchr( " -\t\r\n", *InputBuf ))  continue;
    s[n] = *InputBuf;
    n++; }
  s[n] = 0;
  rc = fseek( InputFile, InputPos, 0 );  if (rc != 0)           eX(3);
  *InputBuf = 0;
  return n;
eX_1:
  eMSG("no input file!");
eX_2: eX_3:
  eMSG("unable to position file %s at %ld!", InputPath, InputPos);
  }

/*================================================================ GenCntLines
*/
long GenCntLines(     /* Anzahl der Eingabe-Saetze feststellen.         */
char c,               /* Ausgewaehltes Kennzeichen                      */
char **ps )           /* Zeichenkette zur Aufnahme der Kennzeichen.     */
  {
  dP(GenCntLines);
  char *s, *t, *pc, k;
  long rc;
  int i, n, l;
  if (!InputFile)                                               eX(1);
  n = 0;
  i = 0;
  l = INBUFSIZE;
  t = ALLOC(l);  if (!t)                                        eX(4);
  *InputBuf = 0;
  rc = fseek(InputFile, InputPos, 0);  if (rc != 0)             eX(2);
  for (n=0; ;) {
    pc = fgets(InputBuf, INBUFSIZE, InputFile);
    if (pc == NULL)  break;
    k = InputBuf[0];
    if (k == 0x1A)  break;  /* ^Z */
    if (strchr(" -\t\r\n", k))  continue;               /*-10feb98-*/
    if (i >= l-1) {                                     // 2001-11-12 uj
      t[i] = 0;
      l += INBUFSIZE;
      s = ALLOC(l);  if (!s)                                 eX(5);
      strcpy(s, t);
      FREE(t);
      t = s;
      s = NULL;
      }
    if (k=='.' || k=='!')  t[i++] = k;
    else if (k == c) {
      t[i++] = k;
      n++;
      }
    else  break;
    }
  t[i] = 0;
  rc = fseek(InputFile, InputPos, 0);  if (rc != 0)             eX(3);
  *InputBuf = 0;
  for (i--; i>=0; i--)
    if (t[i]=='.' || t[i]=='!')  t[i] = 0;
    else  break;
  *ps = t;
  return n;
eX_1:
  eMSG("no input file!");
eX_2: eX_3:
  eMSG("unable to position file %s at %ld!", InputPath, InputPos);
eX_4: eX_5:
  eMSG("unable to allocate %d bytes!", l);
  }


static int cutline( char *s )
  {
  int i, j, inside;
  inside = 0;
  for (i=0; s[i]!='\0'; i++) {
    if      (s[i] == '\"')  inside = 1 - inside;
    else if (s[i] == '\t')  s[i] = ' ';
    else if (s[i] == '\\') {
      if (s[i+1] == '\'')  for (j=i; s[j]!=0; j++)  s[j] = s[j+1];
      }
    else if ((s[i] == '\'') && (!inside)) {
      s[i] = 0;
      break;
      }
    }
  if ((i > 0) && (s[i-1] == '\n')) {
    i--;
    if ((i > 0) && (s[i-1] == '\r')) i--;
    s[i] = 0; }
  return i; }

/*=================================================================== GetLine
*/
long GetLine(         /* Satz vom Eingabe-File einlesen.               */
char c,               /* Gefordertes Kenn-Zeichen des Satzes.          */
char *s,              /* Zeichenkette zur Aufnahme des Satzes.         */
int n )               /* Maximale Länge der Zeichenkette.              */
                      /* RETURN: Anzahl der übertragenen Zeichen.      */
  {
  dP(GetLine);
  int m, irc;
  char *pc;
  s[0] = 0;
  if (!InputFile)                                                       eX(1);
  m = 0;              /* Zähler für die Zeichen in s                */
  if (InBufLen == 0)  /* Eingabe-Puffer füllen,                     */
    while (1) {       /* Kommentarzeilen (Beginn mit "-") überlesen */
      InputPos = ftell( InputFile );
      pc = fgets(InputBuf, INBUFSIZE, InputFile);
      if (!pc) {
        irc = feof(InputFile);  if (!irc)                               eX(2);
        return 0;
        }
      if (InputBuf[0] == 0x1A) {
        InputBuf[0] = 0;
        return 0;                         /* File-Ende bei Ctrl-Z     */
        }
      if (NULL == strchr(" -\t\r\n", InputBuf[0]))  break;
      }
  InBufLen = cutline(InputBuf);
  if (InputBuf[0] == c) {    /*  gewünschtes Anfangszeichen gefunden  */
    if (InBufLen-1 > n)                                                 eX(3);
    strcpy(s, &InputBuf[1]);
    m = InBufLen-1;
    InputBuf[0] = '\0';  InBufLen = 0; }
  else {                                /*  falsches Anfangszeichen,  */
    s[0] = InputBuf[0];  s[1] = '\0';   /*  wird übertragen.          */
    return -1; }                        /*  : keine gültigen Zeichen  */
  while (1) {
    InputPos = ftell(InputFile);
    pc = fgets(InputBuf, INBUFSIZE, InputFile);
    if (!pc) {
      irc = feof(InputFile);  if (!irc)                                 eX(4);
      return m;
      }
    if (InputBuf[0] == 0x1A) {
      InputBuf[0] = 0;
      return m;                           /* File-Ende bei Ctrl-Z     */
      }
    if (InputBuf[0] == '-') continue;     /* Kommentarzeile überlesen */
    InBufLen = cutline(InputBuf);
    if (InputBuf[0] == 0)  continue;      /* Zeile ohne Inhalt        */
    if (InputBuf[0] != ' ') return m;     /* Keine Fortsetzungszeile  */
    if (m+InBufLen > n)                                               eX(5);
    strcpy(s+m, InputBuf);                //-99-12-12 lj
    m += InBufLen;
    InputBuf[0] = '\0';  InBufLen = 0;
    }
eX_1:
  eMSG("no input file!");
eX_2:  eX_4:
  eMSG("read error in file %s!", InputPath);
eX_3:  eX_5:
  eMSG("buffer overflow in file %s!", InputPath);
  }

/*================================================================ CloseInput
*/
long CloseInput(      /* Daten-Eingabefile schließen.                  */
void )                /* RETURN: Aktuelle Lese-Position.               */
  {
  dP(CloseInput);
  int irc;
  if (InputFile != NULL) {
    irc = fclose(InputFile);  if (irc == EOF)                   eX(1);
    }
  InputPath[0] = '\0';  InputBuf[0] = '\0';  InBufLen=0;
  InputFile = NULL;
  return InputPos;
eX_1:
  eMSG("error in closing input file %s!", InputPath);
  }

/*================================================================== ArrExist
*/
long ArrExist(      /* Feststellen, ob File existiert */
char *name )        /* File-Name                      */
  {
  FILE *file;
  char fname[256];
  if ((strchr(name, MSG_PATHSEP)) || (strchr(name, ':')))
      *fname = 0;
  else  strcpy(fname, DataPath);
  strcat(fname, name);
  file = fopen(fname, "rb");  if (!file)  return 0;
  fclose(file);
  return 1;
  }

/*=================================================================== ArrRead
*/
long ArrRead(       /* Array aus einem Array-File (binär) lesen.          */
char *name,         /* File-Name (Pfad wird davorgesetzt).                */
int mode,           /* 0: Header, 1: +Deskriptor, 2: +Daten, 3: selektiv. */
char *header, ... ) /* Zeichenkette zur Aufnahme des Headers.             */
/* ARYDSC *pa,         Pointer auf Array-Deskriptor (wird ausgefüllt).    */
  {
  dP(ArrRead);
  va_list argptr;
  ARYDSC *pa;
  long rc;
  FILE *datafile;
  datafile = NULL;
  va_start(argptr, header);
  if (mode > 0)  pa = va_arg(argptr, ARYDSC*);
  else  pa = NULL;
  va_end(argptr);
  rc = ArrReadMul(name, mode, header, pa, &datafile);
  if (datafile != NULL) {
    if (EOF == fclose(datafile))                                eX(1);
    }
  return rc;
eX_1:
  eMSG("can't close file %s!", name);
  }

/*================================================================ ArrReadMul
*/
long ArrReadMul(    /* Mehrere Arrays aus einem Array-File lesen.         */
char *name,         /* File-Name (Pfad wird davorgesetzt).                */
int mode,           /* 0: Header, 1: +Deskriptor, 2: +Daten, 3: selektiv. */
char *header,       /* Zeichenkette zur Aufnahme des Headers.             */
ARYDSC *pa,         /* Pointer auf Array-Deskriptor (wird ausgefüllt).    */
FILE **pfile )      /* Pointer auf File-Pointer (wird beim OPEN gesetzt). */
  {
  dP(ArrReadMul);
  long total, ll, l1, l2, len, dim;
  int i;
  int nseq=1, iseq;                                           //-00-03-09
  FILE *datafile;
  char combname[256];
  int hdrlen, oldpos, datpos, newpos;
  if (*pfile == NULL) {  /* Filenamen bilden und File eröffnen */
    if ((strchr(name, MSG_PATHSEP)) || (strchr(name, ':')))  *combname = 0;
    else  strcpy(combname, DataPath);
    strcat(combname, name);
    i = strlen(combname) - 1;  if (i < 0)                       eX(1);
    if (combname[i] == ']') {
      for (i-- ; i>0; i--)  if (combname[i] == '[')  break;
      if (i < 1)                                                eX(2);
      combname[i] = 0;
      if (combname[i+1] == ']')  nseq = 0;                      //-00-03-09
      else {
        sscanf(combname+i+1, "%d", &nseq);  if (nseq < 1)       eX(3);
      }
    }
    datafile = fopen(combname, "rb");  if (!datafile)           eX(4);
    *pfile = datafile;
    }
  else datafile = *pfile;
  iseq = 1;                                                     //-00-03-09

  while (1) {
    total = 0;
    oldpos = ftell(datafile);
    if (1 != fread(&ll,sizeof(ll),1,datafile))  goto no_data;
    if (ll == 0)  goto no_data;
    if (ll & 0xFFFF0000)  goto no_data;
    hdrlen = (ll+1) & 0xFFFFFFFE;
    if (hdrlen >= GenHdrLen-1) {
      if (oldpos > 0)  goto no_data;
                                                                eX(6);
    }
    if (1 != fread(header, hdrlen, 1, datafile))                eX(7);
    header[hdrlen] = 0;
    if (mode < 1)  return 1;

    if (pa->start)                                              eX(8);
    AryDefine(pa, 0);
    if (1 != fread(&len, sizeof(len), 1, datafile))             eX(10);
    pa->elmsz = len;
    if (len == 0)  return 2;
    if (len < 0)                                                eX(9);
    if (1 != fread(&dim, sizeof(dim), 1, datafile))             eX(11);
    if (dim>ARYMAXDIM || dim<0)                                 eX(20);
    pa->numdm = dim;
    for (i=0; i<dim; i++) {
      if (1 != fread(&l1, sizeof(l1), 1, datafile))             eX(12);
      pa->bound[i].low  = l1;
      if (1 != fread(&l2, sizeof(l2), 1, datafile))             eX(13);
      pa->bound[i].hgh = l2;
      }
    datpos = ftell(datafile);
    AryDefine(pa, -1);                                          eG(21);
    total = pa->ttlsz;
    if (iseq != nseq) {                                         //-00-03-09
      if (fseek(datafile, total, SEEK_CUR))                     eX(22);
      }
    newpos = ftell(datafile);
    if (iseq == nseq)  break;                                   //-00-03-09
    iseq++;                                                     //-00-03-09
    }

  if (total == 0)  return 0;
  if (mode < 2)  return total;          /*-06feb96-*/

  AryDefine(pa, -1);                                          eG(32);
  total = pa->ttlsz;
  pa->start = ALLOC(total);  if (!pa->start)                  eX(33);
  if (1 != fread(pa->start, total, 1, datafile))              eX(34);
  return 0;

no_data:
  fseek(datafile, oldpos, SEEK_SET);                        //-00-06-14
  return -iseq;                                             //-00-06-14

eX_1: eX_2: eX_3:
  eMSG("invalid file name %s!", combname);
eX_4:
  eMSG("can't open file %s!", combname);
eX_6:
  eMSG("header size %d (at %d) too large!", hdrlen, oldpos);
eX_7:
  eMSG("can't read header of length %d!", hdrlen);
eX_8:
  eMSG("array descriptor already used!");
eX_9: eX_10: eX_11: eX_12: eX_13:
  eMSG("can't read array descriptor!");
eX_20:
  eMSG("numdim=%ld out of range!", dim);
eX_21:  eX_32:
  eMSG("invalid array descriptor!");
eX_22:
  eMSG("can't position input file!");
eX_33:
  eMSG("can't allocate array of size %ld!", pa->ttlsz);
eX_34:
  eMSG("can't read array with %ld bytes!", total);
  }

/*================================================================== ArrWrite
*/
long ArrWrite(      /* Array in einen Array-File (binaer) ausschreiben.    */
char *name,         /* File-Name (Pfad wird davor gesetzt).               */
char *header,       /* Header zur Beschreibung der Daten.                 */
ARYDSC *pa )        /* Pointer auf Array-Deskriptor (wird mit uebertragen).*/
                    /* RETURN: = TRUE, wenn der File schon existierte.    */
  {
  dP(ArrWrite);
  FILE *datafile;
  long rc;
  int irc;
  datafile = NULL;
  rc = ArrWriteMul( name, header, pa, &datafile );
  if (rc < 0)  return rc;
  irc = fclose( datafile );  if (irc == EOF)            eX(1);
  return rc;
eX_1:
  eMSG("can't close file %s!", name);
  }

/*=============================================================== ArrWriteMul
*/
long ArrWriteMul(   /* Array an einen Array-File (binaer) anhaengen.        */
char *name,         /* File-Name (Pfad wird davor gesetzt).                 */
char *header,       /* Header zur Beschreibung der Daten.                   */
ARYDSC *pa,         /* Pointer auf Array-Deskriptor (wird mit uebertragen). */
FILE **pfile )      /* Pointer auf File-Pointer (wird beim OPEN gesetzt).   */
                    /* RETURN: = TRUE, wenn der File schon existierte.      */
  {
  dP(ArrWriteMul);
  static FILE *datafile;
  long total, ll, oldpos, newpos, datpos, elmpos;
  int dim, old=0, len, i, irc, lhdr;
  int cmplen;
  static char combname[256];
  char *pc, *ps;
  if (pa->ttlsz) {
    if (!pa->start)                                     eX(1);
    total = pa->elmsz;
    dim = pa->numdm;
    for (i=0; i<dim; i++)
      total *= 1 + pa->bound[i].hgh - pa->bound[i].low;
    if (total != pa->ttlsz)                             eX(2);
    }
  else { total = 0;  dim = 0; }

  ps = strrchr(name, MSG_PATHSEP);
  if (!ps)  ps = name;
  pc = strrchr(ps, '.');
  cmplen = 0;

  if (*pfile == NULL) {          /* Filenamen erzeugen und File eröffnen  */
    if ((strchr(name, MSG_PATHSEP)) || (strchr(name, ':')))  *combname = 0;
    else  strcpy(combname, DataPath);
    strcat(combname, name);
    old = 0;
    datafile = fopen(combname, "rb");
    if (datafile != NULL) {
      old = 1;
      irc = fclose(datafile);  if (irc == EOF)          eX(3);
      }
    else  errno = 0;
    datafile = fopen(combname, "wb+");  if (!datafile)  eX(4);
    }
  else {
    if (datafile && (datafile!=*pfile))  *combname = 0;
    datafile = *pfile;
    old = 1;
    }

  oldpos = ftell(datafile);
  lhdr = strlen(header);
  len = (lhdr + 4) & 0xFFFC;
  ll = len;
  if (1 != fwrite(&ll, 4, 1, datafile))                 eX(21);
  if (1 != fwrite(header, lhdr, 1, datafile))           eX(22);
  i = 0;
  if (len-lhdr != fwrite(&i, 1, len-lhdr, datafile))    eX(22);
  elmpos = ftell(datafile);
  if (DEBUG) printf("length(hdr)=%d (%d),  elmpos=%lx\n", lhdr, len, elmpos);

  ll = pa->elmsz;
  if (1 != fwrite(&ll, 4, 1, datafile))                 eX(23);
  if (pa->elmsz == 0) goto done;
  ll = dim;
  if (1 != fwrite(&ll, 4, 1, datafile))                 eX(24);
  for (i=0; i<dim; i++) {
    ll = pa->bound[i].low;
    if (1 != fwrite(&ll, 4, 1, datafile))               eX(25);
    ll = pa->bound[i].hgh;
    if (1 != fwrite(&ll, 4, 1, datafile))               eX(26);
    }
  fflush(datafile);
  datpos = ftell(datafile);
  if (DEBUG) printf("datpos=%lx, total=%ld\n", datpos, total);
  total = pa->ttlsz;
  if (1 != fwrite(pa->start, total, 1, datafile))       eX(5);
  fflush(datafile);
done:
  if (*pfile == NULL)  *pfile = datafile;
  newpos = ftell(datafile);
  if (DEBUG) printf("ends at %lx\n", newpos);
  return old;
eX_1:
  eMSG("no data for file %s!", name);
eX_2:
  eMSG("inconsistent array size for file %s!", name);
eX_3:
  eMSG("error closing file %s!", combname);
eX_4: 
  eMSG("can't write to file %s!", combname);        
eX_5: //-2004-10-15
  eMSG("can't write to file %s!", name);
eX_21: eX_22: eX_23: eX_24: eX_25: eX_26:
  eMSG("error writing data descriptor!");
  }

/*================================================================= PrnArrDsc
*/
long PrnArrDsc(     /* Array-Deskriptor ausdrucken (zum Testen).          */
ARYDSC *pa )        /* Pointer auf Array-Deskriptor.                      */
  {
  int i;
  if (pa == NULL) {
    Printf("ARRAY: ArrDsc nicht definiert!\n");
    return 0; }
  Printf("ARRAY: Start bei %08lx, elmsz=%ld, maxdm=%ld, numdm=%ld\n",
          pa->start, pa->elmsz, pa->maxdm, pa->numdm);
  Printf("       Offset =  %8ld, ttlsz=%ld, usrcnt=%ld\n",
          pa->virof, pa->ttlsz, pa->usrcnt);
  for (i=0; i<pa->numdm; i++)
    Printf("    %d: low=%ld, hgh=%ld, mul=%ld\n",
            i, pa->bound[i].low, pa->bound[i].hgh, pa->bound[i].mul);
  return 0; }

/*================================================================= GenSepFsp
*/
long GenSepFsp(       /* Zerlegen einer File-Bezeichnung in Komponenten. */
FILESPEC *fsp )       /* Pointer auf Struktur mit Bezeichnung .spec und  */
                      /* Komponenten .driv, .path, .name, .extn, .sequ.  */
  {
  dP(GenSepFsp);
  int l;          /* Laenge einer Komponente */
  char *p0,  /* Pointer zum Scannen der File-Bezeichnung */
       *p1, *p2;
  p0 = fsp->spec;
  l = strlen(p0);
  for (l--; l>=0; l--)  if (p0[l]=='\\')  p0[l] = MSG_PATHSEP;
  *fsp->driv = 0;   /* Initialisierung */
  *fsp->path = 0;
  *fsp->name = 0;
  *fsp->extn = 0;
  fsp->sequ = 0;
  if (p0[l-1] == ']') {
    p2 = strrchr( p0, '[' );  if (!p2)      eX(1);
    *p2 = 0;
    sscanf( p2+1, "%d", &(fsp->sequ) ); }
  else  p2 = NULL;
  l = strlen(p0);  if (l > MAXSPEC)         eX(2);
  p1 = strchr(p0, ':');
  if (p1 != NULL) {
    l = p1 - p0;  if (l > MAXDRIV)          eX(3);
    strncpy( fsp->driv, p0, l );
    fsp->driv[l] = 0;
    p0 = p1+1; }
  p1 = strrchr( p0, MSG_PATHSEP);
  if (p1 != NULL) {
    l = 1 + (p1 - p0);  if (l > MAXPATH)    eX(4);
    strncpy( fsp->path, p0, l );
    fsp->path[l] = 0;
    p0 = p1+1; }
  p1 = strrchr( p0, '.' );
  if (p1 != NULL) {
    l = p1 - p0;  if (l > MAXNAME)          eX(5);
    strncpy( fsp->name, p0, l );
    fsp->name[l] = 0;
    p0 = p1+1;
    l = strlen( p0 );  if (l > MAXEXTN)     eX(6);
    strcpy( fsp->extn, p0 ); }
  else {
    l = strlen( p0 );  if (l > MAXNAME)     eX(7);
    strcpy( fsp->name, p0 ); }
  if (p2)  *p2 = '[';
  return 0;
eX_1: eX_2: eX_3: eX_4: eX_5: eX_6: eX_7:
  eMSG("invalid file name \"%s\"!");
  }

/*================================================================= GenConFsp
*/
long GenConFsp(       /* Erzeugen einer File-Bezeichnung aus Komponenten.*/
FILESPEC *fsp )       /* Pointer auf Struktur mit Bezeichnung .spec und  */
                      /* Komponenten .driv, .path, .name, .extn, sequ.   */
  {
  dP(GenConFsp);
  int ld, lp, ln, le, lf, l;
  char *p0, t[10];
  ld = strlen(fsp->driv);  if (ld) ld++;
  lp = strlen(fsp->path);
  ln = strlen(fsp->name);
  le = strlen(fsp->extn);  if (le) le++;
  lf = ld + lp + ln + le;  if (lf > MAXSPEC)    eX(1);
  p0 = fsp->spec;
  *p0 = 0;
  if (ld) {
    strcat( p0, fsp->driv );
    l = strlen( p0 );
    p0[l] = ':';  p0[l+1] = 0; }
  strcat( p0, fsp->path );
  strcat( p0, fsp->name );
  if (le) {
    l = strlen( p0 );
    p0[l] = '.';
    strcpy( p0+l+1, fsp->extn ); }
  if (fsp->sequ) {
    if ((fsp->sequ < 0) || (fsp->sequ > 9999))  eX(2);
    sprintf( t, "%d", fsp->sequ );
    strcat( p0, t ); }
  return 0;
eX_1:
  eMSG("file name too long!");
eX_2:
  eMSG("invalid file sequence number!");
  }

/*================================================================== GenEvlHdr
*/
long GenEvlHdr(         /* Namen im Tabellen-Header auswerten.              */
char *names[],          /* Liste der möglichen Namen.                       */
char *header,           /* Header der Tabelle.                              */
int *position,          /* Positionen der Header-Namen bzgl. names[].       */
float *scale,           /* Gefundene Faktoren zur Umskalierung ( = 1.0 )    */
int maxval )            /* Maximale Anzahl erfaßbarer Werte                 */
                        /* RETURN: Anzahl der Namen im Tabellen-Header.     */
  {
  dP(GenEvlHdr);
  char *name;
  char hdr[GENTABLEN], *pc;
  int i, n;
  strcpy( hdr, header );  header = hdr;
  pc =  strpbrk( header, ":|" );
  if (pc)  header = pc+1;
  for (n=0; ; n++, position++, scale++) {
    name = strtok( header, " ,\n\t" );
    if (name == NULL) break;
    if (n >= maxval)                                            eX(1);
    header = NULL;
    for (i=0; name[i]!=0; i++)  name[i] = toupper(name[i]);
    for (i=0; names[i]!=NULL; i++)
      if (0 == strcmp((char*)name, names[i])) break;
    if (names[i] == NULL)  { *position = -1;  *scale = 1.0; }
    else { *position = i;  *scale = 1.0; }
    }
  return n;
eX_1:
  eMSG("maximum number of tab-columns (%d) exceeded!", maxval);
  }

//=================================================================================
// history:
// 2004-10-14  uj  ArrWriteMul(): error message 5 corrected
//
