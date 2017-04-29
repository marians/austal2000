//============================================================ TalUtl.c
//
// Utility functions for AUSTAL2000
// ================================
//
// Copyright (C) Umweltbundesamt, Dessau-Roﬂlau, Germany, 2002
// Copyright (C) Janicke Consulting, 88662 ‹berlingen, Germany, 2002-2007
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
// last change:  2007-01-02  lj
//
//==================================================================

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#if defined __WATCOMC__
  #include <sys\stat.h>       // Posix 1003.1
  #include <sys\utime.h>      // Posix 1003.1
  #include <direct.h>         // Posix 1003.1
  #define  TUT_DIREC  DIR*
  #define  TUT_FINFO  struct dirent
  #define  TUTVALID(a)  ((a) != NULL)
  #define  TUTFNAME(a)  (a.d_name)
  #define  TUTFATTR(a)  ((unsigned char) (a.d_attr))
  #define  TUTMKDIR(a)  mkdir(a)
#elif defined __LCC__
  #include <sys\stat.h>       // Posix 1003.1
  #include <sys\utime.h>      // Posix 1003.1
  #include <io.h>
  #include <direct.h>
  #define  utime      _utime
  #define  TUT_DIREC  int
  #define  TUT_FINFO  struct _finddata_t
  #define  TUTVALID(a)  ((a) > 0)
  #define  TUTFNAME(a)  (a.name)
  #define  TUTFATTR(a)  ((unsigned char) (a.attrib))
  #define  TUTMKDIR(a)  mkdir(a)
#elif defined __linux__       // gcc, icc
  #include <sys/stat.h>       // Posix 1003.1
  #include <utime.h>          // Posix 1003.1
  #include <sys/io.h>
  #include <dirent.h>
  #include <unistd.h>
  #define  TUT_DIREC  DIR*
  #define  TUT_FINFO  struct dirent
  #define  TUTVALID(a)  ((a) > 0)
  #define  TUTFNAME(a)  (a.d_name)
  #define  TUTFATTR(a)  ((unsigned char) (a.d_type))
  #define  TUTMKDIR(a)  mkdir(a, S_IRWXU)
#elif defined _M_IX86         // MS C
  #include <sys\stat.h>       // Posix 1003.1
  #include <sys\utime.h>      // Posix 1003.1
  #include <io.h>
  #include <direct.h>
  #define  utime      _utime
  #define  utimbuf    _utimbuf
  #define  TUT_DIREC  int
  #define  TUT_FINFO  struct _finddata_t
  #define  TUTVALID(a)  ((a) > 0)
  #define  TUTFNAME(a)  (a.name)
  #define  TUTFATTR(a)  ((unsigned char) (a.attrib))
  #define  TUTMKDIR(a)  mkdir(a)
#elif defined __GNUC__        // MS windows
  #include <sys\stat.h>       // Posix 1003.1
  #include <sys\utime.h>      // Posix 1003.1
  #include <io.h>
  #include <direct.h>
  #define  utime      _utime
  #define  TUT_DIREC  int
  #define  TUT_FINFO  struct _finddata_t
  #define  TUTVALID(a)  ((a) > 0)
  #define  TUTFNAME(a)  (a.name)
  #define  TUTFATTR(a)  ((unsigned char) (a.attrib))
  #define  TUTMKDIR(a)  mkdir(a)
  #define utimbuf _utimbuf
  #define utimt   _utime
#endif

#include "IBJmsg.h"   // for ALLOC                            //-2006-02-15
#include "TalUtl.h"
#include "TalUtl.nls"

static char *eMODn = "TalUtl";                                //-2006-02-15

//==================================================== TutFindClose
static int TutFindClose( TUT_DIREC d ) {
#if defined __WATCOMC__ || defined __linux__
  if (d) closedir(d);
#else
  if (d > 0) _findclose(d);
#endif
  return 0;
}

//===================================================== TutFindNext
static int TutFindNext( TUT_DIREC d, TUT_FINFO *fip ) {
#if defined __WATCOMC__ || defined __linux__
  TUT_FINFO *direntp;
  direntp = readdir(d);
  if (!direntp) {
    TutFindClose(d);
    return -1;
  }
  *fip = *direntp;
  return 0;
#else
  if (_findnext(d, fip)) {
    TutFindClose(d);
    return -1;
  }
  return 0;
#endif
}

//========================================================= TutFindFirst
static TUT_DIREC TutFindFirst( char *path, TUT_FINFO *fip ) {
  TUT_DIREC d;
#if defined __WATCOMC__ || defined __linux__
  int n;
  d = opendir(path);
  if (!d)  return NULL;
  n = TutFindNext(d, fip);
  return (n) ? NULL : d;
#else
  char fn[256];
  strcpy(fn, path);
  strcat(fn, "/*");
  d = _findfirst(fn, fip);
  return d;
#endif
}

//=========================================================== TutIsDir
//
static int TutIsDir( int a ) {
  #ifdef __linux__
    return (a & DT_DIR) != 0;
  #else
    return (a & _A_SUBDIR) != 0;
  #endif
}

//======================================================== TutGetFileList
//
char **_TutGetFileList( char *path ) {
  dQ(TutGetFileList);
  char **_ss, *p0;
  int n, r, i;
  TUT_DIREC d;
  TUT_FINFO fi;
  d = TutFindFirst(path, &fi);
  r = TUTVALID(d);
  if (!r)  return NULL;
  n = 0;
  while (r) {
    if (!TutIsDir(TUTFATTR(fi))) n++;
    r = !TutFindNext(d, &fi);
  }
  _ss = ALLOC((n+1)*sizeof(char*) + n*256);
  p0 = (char*)(_ss + (n+2));
  d = TutFindFirst(path, &fi);
  r = TUTVALID(d);
  if (!r)  return _ss;
  for (i=0; i<n; ) {
    if (!TutIsDir(TUTFATTR(fi))) {
      _ss[i] = p0 + i*256;
      strcpy(_ss[i], TUTFNAME(fi));
      i++;
    }
    if (TutFindNext(d, &fi))  break;
  }
  return _ss;
}

//======================================================== TutGetDirList
//
char **_TutGetDirList( char *path ) {
  dQ(TutGetDirList);
  char **_ss, *p0, name[256];
  int n, r, i;
  TUT_DIREC d;
  TUT_FINFO fi;
  d = TutFindFirst(path, &fi);
  r = TUTVALID(d);
  if (!r)  return NULL;
  n = 0;
  while (r) {
    strcpy(name, TUTFNAME(fi));
    if (TutIsDir(TUTFATTR(fi)) && *name!='.') n++;
    r = !TutFindNext(d, &fi);
  }
  _ss = ALLOC((n+1)*sizeof(char*) + n*256);
  p0 = (char*)(_ss + (n+2));
  d = TutFindFirst(path, &fi);
  r = TUTVALID(d);
  if (!r)  return _ss;
  for (i=0; i<n; ) {
    strcpy(name, TUTFNAME(fi));
    if (TutIsDir(TUTFATTR(fi)) && *name!='.') {
      _ss[i] = p0 + i*256;
      strcpy(_ss[i], TUTFNAME(fi));
      i++;
    }
    if (TutFindNext(d, &fi))  break;
  }
  return _ss;
}

//=========================================================== TutDirExists
//
int TutDirExists( // check, if a directory exists
char *path )      // directory name
{                 // RETURN : 1 if exists
  TUT_DIREC d;
  TUT_FINFO fi;
  d = TutFindFirst(path, &fi);
  TutFindClose(d);
  return TUTVALID(d);
}

//=========================================================== TutFileExists
//
int TutFileExists(  // check, if a file exists (and can be read)
char *path )        // file name
{                   // RETURN : 1 if exists
  FILE *pfile;
  pfile = fopen(path, "r");
  if (pfile)  fclose(pfile);
  return (pfile != NULL);
}

//=============================================================== TutDirMake
//
int TutDirMake(   // create a directory (recursive)
char *path )      // directory name
{                 // RETURN : 0 if created, 1 if exists, -1 on error
  char *ps, part[256];
  int n;
  if (!path)  return 0;
  if (!*path)  return -1;
  ps = path;
  if (!strncmp(ps, "//", 2)) {
    ps = strchr(ps+2, '/');
    if (!ps)  return -2;
    ps = strchr(ps+1, '/');
    if (!ps)  return -3;
    ps++;
  }
  else {
    if (ps[1] == ':')  ps += 2;
    if (*ps == '/')  ps++;
  }
  for (;;) {
    ps = strchr(ps, '/');
    if (ps) {
      *part = 0;
      strncat(part, path, ps-path);
    }
    else  strcpy(part, path);
    n = TutDirExists(part);
    if (!n) {
      if (TUTMKDIR(part))  return -4;
    }
    if (!ps)  return n;
    ps++;
  }
}

//========================================================== TutFileCopy
//
int TutFileCopy(    // copy a file
char *srcname,      // name of source file
char *dstname )     // name of destination file
  {                 // RETURN : number of bytes copied
  dQ(TutFileCopy);
  FILE *src, *dst;
  struct stat statbuf;
  struct utimbuf timbuf;
  int n, m, total;
  char *buf;
  buf = ALLOC(0x8000);
  if (!buf)  return -1;
  src = NULL;
  dst = NULL;
  total = 0;
  src = fopen(srcname, "rb");  if (!src) goto error;
  dst = fopen(dstname, "wb");  if (!dst) goto error;
  do {
    n = fread(buf, 1, 32000, src);
    if (n <= 0)  break;
    m = fwrite(buf, 1, n, dst);
    if (m != n)  goto error;
    total += n;
    } while (n == 32000);
  fclose(dst);
  fclose(src);
  free(buf);
  if (!stat(srcname, &statbuf)) {
    timbuf.actime = statbuf.st_atime;
    timbuf.modtime = statbuf.st_mtime;
    utime(dstname, &timbuf);
  }
  return total;
error:
  if (src)  fclose(src);
  if (dst)  fclose(dst);
  if (buf)  FREE(buf);
  return -1;
  }

//=========================================================== TutDirCopy
//
int TutDirCopy(   // copy a directory with its files
char *srcpath,    // name of source directory
char *dstpath )   // name of destination directory
{                 // RETURN : number of files copied
  char src[256], dst[256], name[256];
  int n, a, r;
  int nn = 0;
  TUT_DIREC d;
  TUT_FINFO fi;
  d = TutFindFirst(srcpath, &fi);
  for (r=TUTVALID(d); (r); r=!TutFindNext(d,&fi)) {
    strcpy(name, TUTFNAME(fi));
    sprintf(src, "%s/%s", srcpath, name);
    sprintf(dst, "%s/%s", dstpath, name);
    a = TUTFATTR(fi);
    if (TutIsDir(a)) {                 // directory
      if (*name == '.')  continue;
      n = TutDirMake(dst);
      if (n < 0) {
        nn = -2;
        break;
      }
      n = TutDirCopy(src, dst);
      if (n < 0) {
        nn = n;
        break;
      }
      nn += n;
    }
    else {         // File
      n = TutFileCopy(src, dst);
      if (n < 0) {
        nn = -1;
        break;
      }
      nn++;
    }
  }
  return nn;
}

//========================================================== TutDirClear
//
int TutDirClearOnDemand(  // delete the files in a directory
char *path )      // name of the directory
{                 // RETURN : number of files deleted or -1 if error/exit
  TUT_DIREC d;
  TUT_FINFO fi;
  char c, src[256], fn[256];
  int a, r;
  if (!TutDirExists(path)) return 0;
  d = TutFindFirst(path, &fi);
  r = TUTVALID(d);
  while (r) {
    strcpy(fn, TUTFNAME(fi));
    if (strcmp(fn, ".") && strcmp(fn, "..")) {
      sprintf(src, "%s/%s", path, fn);
      a = TUTFATTR(fi);
      if (!TutIsDir(a)) break;
    }
    r = !TutFindNext(d, &fi);
  }
  if (!r) return 0;
  printf(_clear_all_$_, path); 
  fflush(stdout); 
  c = getchar(); 
  if (NULL == strchr(_approve_, c))                               //-2007-01-02
    return -1;
  else
    return TutDirClear(path); 
}


//========================================================== TutDirClear
//
int TutDirClear(  // delete the files in a directory
char *path )      // name of the directory
{                 // RETURN : number of files deleted
  char src[256], fn[256];
  int n=0, a, r, nn=1;
  TUT_DIREC d;
  TUT_FINFO fi;
  d = TutFindFirst(path, &fi);
  r = TUTVALID(d);
  while (r) {
    strcpy(fn, TUTFNAME(fi));
    if (strcmp(fn, ".") && strcmp(fn, "..")) {
      sprintf(src, "%s/%s", path, fn);
      a = TUTFATTR(fi);
      if (TutIsDir(a)) {   // directory
      }
      else {               // File
        if (remove(src)) {
          nn = -1;
          // break;
        }
        else n++;
      }
    }
    r = !TutFindNext(d, &fi);
  }
  return nn*n;
}

//===================================================== TutDirRemove
//
int TutDirRemove( // delete the directory
char *path )      // name of the directory
{                 // RETURN : error
  return rmdir(path);
}

//====================================================== TutCheckName
//
int TutCheckName(   // replace backslash and remove trailing slash
  char *fn )        // file name
{
  char *pc;
  if (!fn)  return -1;
  for (pc=fn; (*pc); pc++)  if (*pc == '\\')  *pc = '/';
  if (*fn && pc[-1]=='/')  pc[-1] = 0;
  return 0;
}

//======================================================= TutMakeName
//
int TutMakeName(  // construct absolute file name
  char *fn,       // result (buffer > 255)
  char *path,     // path
  char *name )    // file name
{
  char pa[256], na[256];
  *fn = 0;
  if (path) {
    if (strlen(path) > 255)  return -1;
    strcpy(pa, path);
    TutCheckName(pa);
  }
  else *pa = 0;
  if (name) {
    if (strlen(name) > 255)  return -1;
    strcpy(na, name);
    TutCheckName(na);
  }
  else *na = 0;
  if (*na=='/' || strchr(na, ':'))  strcpy(fn, na);
  else {
    if (strlen(na)+strlen(pa) >= 255)  return -2;
    sprintf(fn, "%s/%s", pa, na);
  }
  return 0;
}

//======================================================================
/* table of crc's of all 8-bit messages */
static unsigned int crc_table[256];

/* make the table for a fast crc */
static void make_crc_table(void) {
  unsigned int c;
  int n, k;
  for (n = 0; n < 256; n++) {
    c = (unsigned int)n;
    for (k = 0; k < 8; k++)
      c = c & 1 ? 0xedb88320 ^ (c >> 1) : c >> 1;
    crc_table[n] = c;
  }
}

/* update a running crc with the bytes buf[0..len-1]--the crc should be
   initialized to all 1's, and the transmitted value is the 1's complement
   of the final running crc (see the crc() routine below))
 */
static unsigned int update_crc(unsigned int crc, unsigned char *buf, int len) {
  unsigned int c = crc;
  unsigned char *p = buf;
  int n = len;
  if (n > 0) do {
    c = crc_table[(c ^ (*p++)) & 0xff] ^ (c >> 8);
  } while (--n);
  return c;
}

/* return the crc of the bytes buf[0..len-1] */
unsigned int TutMakeCrc(unsigned char *buf, int len) {
  if (crc_table[1] != 0x77073096) {
    make_crc_table();
  }
  return ~update_crc(0xffffffff, buf, len);
}

unsigned int TutGetCrc(char *fn) {
  dQ(TutGetCrc);
  FILE *f;
  unsigned int crc = 0;
  unsigned char *_buffer = NULL;
  int buflen=4096, n=0;
  f = fopen(fn, "rb");
  if (f == NULL)
    return 0;
  _buffer = ALLOC(buflen);
  if (!_buffer) {
      fclose(f);
      return 0;
  }
  crc = TutMakeCrc(NULL, 0);
  do {
      n = fread(_buffer, 1, buflen, f);
      if (n <= 0)
        break;
      crc = ~update_crc(~crc, _buffer, n);
  } while (n == buflen);
  fclose(f);
  FREE(_buffer);
  return crc;
}

//=======================================================================
#ifdef MAIN  //#########################################################

int list( char *path, int l ) {
  TUT_DIREC d;
  TUT_FINFO fi;
  char name[256], dn[256];
  int r;
  printf("LIST %s (%d)\n", path, l);
  d = TutFindFirst(path, &fi);
  r = TUTVALID(d);
  while (r) {
    strcpy(name, TUTFNAME(fi));
    printf("%2d: %02x %s\n", l, TUTFATTR(fi), name);
    if ((l) && TutIsDir(TUTFATTR(fi)) && (*name!='.')) {
      sprintf(dn, "%s/%s", path, name);
      list(dn, l+1);
    }
    r = !TutFindNext(d, &fi);
  }
  return 0;
}


void help( void ) {
  printf("commands:\n");
  printf("c <src> <dst>  DirCopy\n");
  printf("d <src>        DirClear\n");
  printf("e <src>        Exists\n");
  printf("f <src> <dst>  FileCopy\n");
  printf("h              help\n");
  printf("l <src>        list\n");
  printf("m <dst>        DirMake\n");
}

int main( int argc, char *argv[] ) {
  char s[400], src[256], dst[256], c, **_ll;
  int i, k, n;
  printf("TEST of TalTut\n");
  help();
  while (fgets(s, 400, stdin)) {
    if (*s < ' ')  break;
    c = 0;
    *src = 0;
    *dst = 0;
    i = sscanf(s, "%c %s %s", &c, src, dst);
    if (i <= 0)  break;
    switch (c) {
      case 'c': if (i != 3)  continue;
                n = TutDirCopy(src, dst);
                printf("DirCopy = %d\n", n);
                break;
      case 'd': if (i != 2)  continue;
                n = TutDirClear(src);
                printf("DirClear = %d\n", n);
                break;
      case 'e': if (i != 2)  continue;
                n = TutFileExists(src);
                printf("FileExists = %d\n", n);
                n = TutDirExists(src);
                printf("DirExists = %d\n", n);
                break;
      case 'f': if (i != 3)  continue;
                n = TutFileCopy(src, dst);
                printf("FileCopy = %d\n", n);
                break;
      case 'h': help();
                break;
      case 'l': if (i != 2)  continue;
                list(src, 0);
                break;
      case 'L': if (i != 2)  continue;
                list(src, 1);
                break;
      case 'm': if (i != 2)  continue;
                n = TutDirMake(src);
                printf("MakeDir = %d\n", n);
                break;
      case 'D': if (i != 2)  continue;
                _ll = _TutGetDirList(src);
                for (k=0; (_ll[k]); k++)  printf("D%02d  %s\n", k+1, _ll[k]);
                FREE(_ll);
                break;
      case 'F': if (i != 2)  continue;
                _ll = _TutGetFileList(src);
                for (k=0; (_ll[k]); k++)  printf("F%02d  %s\n", k+1, _ll[k]);
                FREE(_ll);
                break;
      default:  printf("unknown command\n");
    }
  }
  return 0;
}

#endif  //##############################################################

//=======================================================================
//
// history:
//
// 2001-04-27 lj new
// 2001-08-09 lj find-functions
// 2001-11-23 lj path construction
// 2002-09-24 lj final release candidate
// 2006-10-26 lj external strings
// 2007-01-02 lj NLS for user reply
//
//======================================================================

