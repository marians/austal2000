// ================================================================ IBJnls.c
//
// Native Language Support
// =======================
//
// Copyright (C) Umweltbundesamt, Dessau-Roﬂlau, Germany, 2006
// Copyright (C) Janicke Consulting, 88662 ‹berlingen, Germany, 2008
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
// last change:  2008-12-19 lj
//
//========================================================================
/*
 * Prerequisites:
 * ==============
 *
 * Directory structure relative to the current directory when IBJnls is called:
 *
 * ../source        : source code of the program PGM with modules M<i>
 * ../source/nls    : default strings for the modules in files "M<i>.text"
 * ../source/nls/LL : strings for the language LL in files "M<i>_LL.text"
 *
 *
 * Basic procedure:
 * ================
 *
 * The Native Language Support is implemented in two steps:
 *
 * 1. Externalization of the strings.
 *    All language specific strings of a module M<i> are defined in a separate
 *    file M<i>.nls as elements of a string array. The source file contains
 *    only references to this array. The file M<i>.nls is created by the NLS
 *    server from a user supplied file M<i>.text.
 *
 * 2. Language specific replacement of the strings.
 *    If the user wants PGM to use another language he specifies the language
 *    label LL as a calling parameter on invocation of PGM.
 *    Then the NLS client contained in PGM reads the language specific file
 *    PGM_LL.nls and replaces the default strings by the translated strings.
 *
 *
 * Externalization:
 * ================
 *
 * A language specific string in the source code of module M, e.g. in a
 * statement like
 *   printf("distance x is %f km\n", x);
 * is replaced by the user by a key, e.g.
 *   printf(_distance_x_, x);
 * and the meaning of the key is described in a separate file M.text
 * structured like a Java Properties file:
 *   _distance_x_=distance x is %f km\n
 *
 * This can be done in the following way:
 *
 * 1. Open the source file M.c in an editor using a split pane.
 * 2. Create the new file M.text in the second pane.
 * 3. Write a key followed by '=' into M.text.
 * 4. Cut out the content of the string in M.c and insert it in M.text.
 * 5. Replace the "" left over in M.c by the key (copy+paste).
 *
 * The file M.text containing the key/value pairs is transformed by the NLS
 * server program IBJnls into a file M.nls that has to be included into M.c:
 *
 *   IBJnls -prepare M
 *
 * In M.nls the strings are elements of a string array NLS and the keys are
 * replaced by a reference to the corresponding element:
 *
 *   #define _distance_x_  NLS[1]
 *
 *   static char *NLS[] = {
 *     " ... ",
 *     "distance x is %f km\n",
 *     ...
 *     NULL };
 *
 * The sequence of the strings is the same as in the file M.text. Therefore,
 * the index value used is the same as the line number in the text file
 * (starting with 0). Now, the print statement containing the language specific
 * format reads (after the preprocessor has done his work):
 *
 *   printf(NLS[1], x);
 *
 * In addition, the file M.nls contains the definition of a function returning
 * the array (for external access):
 *
 *   char **M_get_strings(void) { return NLS; }
 *
 * The file M.nls has to be included at the beginning of M.c. The simplest way
 * to change the language used by the program is to translate the files
 * M<i>.text and then to include the translated M<i>.nls.
 *
 *
 * Replacement:
 * ============
 *
 * To enable the user to choose the language LL at runtime, the following
 * provisions are made:
 * - A file PGM_LL.nls is provided containing the translated strings.
 * - The NLS client capable of reading this file and replacing the default
 *   strings is linked into PGM.
 *
 * The file PGM_LL.nls is created by the NLS server using the text definitions
 * M<i>.text and M<i>_LL.text associated with the modules M<i> of PGM:
 *
 *   IBJnls -collect PGM_LL M0 M1 M2 ...
 *
 * PGM_LL.nls contains all the translated strings and their index values with
 * respect to the array NLS of the corresponding module:
 *
 *   **** M0
 *   0000 string 0 of module 0
 *   0001 string 1 of module 0
 *   ...
 *   **** M1
 *   0000 string 0 of module 1
 *   0001 string 1 of module 1
 *   ...
 *
 * The index values are recalculated by checking which line in file M<i>.text
 * has the same key as found in M<i>_LL.text. The keys in the translated file
 * M<i>_LL.text form a subset of the keys in M<i>.text and may appear in a
 * different order.
 *
 * The NLS client replaces the default strings by the translated string by
 * changing the pointer values in NLS[]. Therefore, the client must know the
 * function M<i>_get_strings() returning the default definitions of module M<i>.
 * This is accomplished by an additional module IBJnls_PGM.c created by the
 * NLS server:
 *
 *   IBJnls -defaults PGM M0 M1 M2 ...
 *
 * The created source file contains only one function
 *   char **NlsGetStrings(char *name)
 * with statements like
 *   if (!strcmp(name, "Mxxx")) return Mxxx_get_strings();
 * This source file has to be compiled and also linked to PGM.
 *
 * If a translation is required by the user the main function of PGM should
 * call as early as possible the function
 *   NlsRead(char *path, char *pgm, char *lan)
 * of the NLS client. The arguments are
 *   path: the directory where the running program is located
 *   pgm : the name PGM used for identifying the translated strings
 *   lan : the language LL
 * When the client reading the file <path>/PGM_LL.nls finds a new module
 * name M (identified by the leading "****"), it asks the function
 * NlsGetStrings("M") for the string array of this module, reads the following
 * lines and replaces at the indicated position the default string by the
 * translated string.
 *
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "IBJnls.h"                                               //-2008-12-11

static char path[256] = "../source";
static char cset[16];                                             //-2008-07-22
static char vers[64];                                             //-2008-07-22
static char *_tbuf;
static int ltbuf;
static char **tkeys;
static char **tvals;
static int ntkeys;
static char *_dbuf;
static char **dkeys;
static char **dvals;
static int ndkeys;

char *NlsGetCset(void) {   // return character set                //-2008-07-22
  return (*cset) ? cset : "utf-8";
}

static int file_exists(    // check, if a file exists (and can be read)
char *path )               // file name
{                          // RETURN : 1 if exists
  FILE *pfile;
  pfile = fopen(path, "r");
  if (pfile)  fclose(pfile);
  return (pfile != NULL);
}

static void remove_escape(char *p) {                              //-2008-07-22
  char *p1, *p2;
  p1 = p;
  p2 = p;
  while (1) {
    char c;
    c = *p2;
    if (c == '\\') {
      c = *++p2;
      switch (c) {
      case 't': c = '\t'; break;
      case 'r': c = '\r'; break;
      case 'n': c = '\n'; break;
      case 'b': c = '\b'; break;
      case '"': c = '"'; break;
      default:;
      }
    }
    *p1++ = c;
    if (c == 0)
      break;
    p2++;
  }
}

static char *next_line(char *pc) {
  char *pn;
  if (!pc) return NULL;
  while(*pc < ' ') {
    if (!*pc) return NULL;
    pc++;
  }
  pn = pc;
  while (*pn >= ' ')  pn++;
  *pn = 0;
  return pc;
}

static int read_mapping(char *fn) {                               //-2008-07-22
  FILE *f;
  int n, i, l;
  int is_nls;
  char *pc, *pe, *pa;
  char sep;
  is_nls = !strcmp(fn+strlen(fn)-4, ".nls");
  sep = (is_nls) ? ' ' : '=';
  if (!fn)
    return -1;
  f = fopen(fn, "rb");
  if (!f)
    return -2;
  fseek(f, 0, SEEK_END);
  ltbuf = ftell(f);
  fseek(f, 0, SEEK_SET);
  _tbuf = malloc(ltbuf+2);
  if (!_tbuf)
    return -3;
  n = fread(_tbuf, 1, ltbuf, f);
  if (n != ltbuf)
    return -4;
  fclose(f);
  _tbuf[ltbuf] = 0;
  _tbuf[ltbuf+1] = 0;
  for (i=0, pc=_tbuf; i<ltbuf; i++, pc++) {                       //-2008-10-03
    if (*pc < ' ' && *pc != '\r' && *pc != '\n')
      *pc = ' ';
  }
  pc = _tbuf;
  n = 0;
  for (pc=_tbuf; pc<_tbuf+ltbuf; ) {
    pc = next_line(pc);
    if (!pc)  break;
    l = strlen(pc);
    if (*pc != '#')  n++;
    pc += l + 1;
  }
  tkeys = malloc(n*sizeof(char*));
  tvals = malloc(n*sizeof(char*));
  if (!tkeys || !tvals)
    return -5;
  i = 0;
  for (pc=_tbuf; pc<_tbuf+ltbuf; ) {
    pc = next_line(pc);
    if (!pc)  break;
    l = strlen(pc);
    if (*pc == '#') {                                             //-2008-07-22
      if (i == 0 && is_nls) {
        pa = strchr(pc, '@');
        if (pa != NULL) {
          pa++;
          pe = strchr(pa, '"');
          if (pe != NULL)  *pe = 0;
          strcpy(cset, pa);
        }
        else if (strlen(pc) > 2 && strlen(pc) < 66) {
          strcpy(vers, pc+2);
        }
      }
    }
    else if (*pc <= ' ')                                          //-2008-07-22
      continue;
    else {
      tkeys[i] = pc;
      for (pe=pc; (*pe); pe++) {
        if (*pe == sep)  break;
      }
      if (!*pe)
        return -6;
      for (pa=pe-1; pa>pc; pa--) {                                //-2008-07-22
        if (*pa > ' ')  break;
        *pa = 0;
      }
      *pe++ = 0;
      if (is_nls) remove_escape(pe);                              //-2008-07-22
      tvals[i] = pe;
      i++;
      if (i >= n) break;
    }
    pc += l + 1;
  }
  ntkeys = n;
  return n;
}

#ifdef MAIN //======================================================= server

static int write_nls(char *pgm) {
  FILE *f;
  char fn[256];
  int i;
  if (!pgm) return -1;
  sprintf(fn, "%s/%s.nls", path, pgm);
  f = fopen(fn, "w");
  if (!f) return -2;
  fprintf(f, "// default text definitions for %s\n", pgm);
  fprintf(f, "extern char *%s_NLS[];\n", pgm);
  for (i=0; i<ntkeys; i++) {
    fprintf(f, "#define %s %s_NLS[%d]\n", tkeys[i], pgm, i);
  }
  fprintf(f, "\n");
  fprintf(f, "#ifndef NLS_DEFINE_ONLY\n");
  fprintf(f, "char *%s_NLS[%d] = {\n", pgm, ntkeys+1);
  for (i=0; i<ntkeys; i++) {
    fprintf(f, "  \"%s\",\n", tvals[i]);
  }
  fprintf(f, "  NULL };\n");
  fprintf(f, "#endif\n");
  fclose(f);
  free(tkeys);
  free(tvals);
  free(_tbuf);
  return 0;
}

/*
 * Convert a *.text file to a *.nls file
 */
static int prepare(char *pgm) {
  char fn[256];
  int n;
  if (!pgm)  return -1;
  sprintf(fn, "%s/nls/%s.text", path, pgm);
  printf("IBJnls: preparing %s\n", fn);
  n = read_mapping(fn);
  if (n <= 0) {
    printf("IBJnls: *** read_mapping = %d\n", n);
    exit(1);
  }
  n = write_nls(pgm);
  if (n < 0) {
    printf("IBJnls: *** write_nls(%s)=%d\n", pgm, n);
    exit(2);
  }
  return 0;
}

static int collect(int argc, char *argv[]) {
  char out[256], lan[64], *pc;
  char fn_def[256], fn_lan[256], *key, *vrs;
  FILE *g;
  int i, n, it, id;
  vrs = argv[1] + strlen("-collect");
  strcpy(out, argv[2]);
  pc = strchr(out, '_');
  if (!pc) {
    printf("IBJnls: *** no language specified in \"%s\"\n", out);
    return -10;
  }
  strcpy(lan, pc+1);
  strcat(out, ".nls");
  g = fopen(out, "w");
  if (!g) {
    printf("IBJnls: *** can't open file \"%s\"\n", out);
    return -11;
  }
  fprintf(g, "# language definitions \"%s\"\n", argv[2]);
  fprintf(g, "# %s\n", vrs);
  for (i=3; i<argc; i++) {
    sprintf(fn_def, "%s/nls/%s.text", path, argv[i]);
    sprintf(fn_lan, "%s/nls/%s/%s_%s.text", path, lan, argv[i], lan);
    if (!file_exists(fn_lan)) {
      printf("IBJnls: no mapping file \"%s\"\n", fn_lan);
      continue;
    }
    n = read_mapping(fn_def);
    if (n <= 0) {
      printf("IBJnls: *** can't read mapping \"%s\"\n", fn_def);
      return -12;
    }
    dkeys = tkeys;
    dvals = tvals;
    _dbuf = _tbuf;
    ndkeys = ntkeys;
    n = read_mapping(fn_lan);
    if (n <= 0) {
      printf("IBJnls: *** can't read mapping \"%s\"\n", fn_lan);
      return -13;
    }
    fprintf(g, "**** %s\n", argv[i]);
    for (it=0; it<ntkeys; it++) {
      key = tkeys[it];
      for (id=0; id<ndkeys; id++) {
        if (!strcmp(key, dkeys[id])) break;
      }
      if (id >= ndkeys) {
        printf("IBLnls: key \"%s\" not found\n", key);
      }
      else {
        fprintf(g, "%04d %s\n", id, tvals[it]);
      }
    }
    free(tkeys);
    free(tvals);
    free(_tbuf);
    free(dkeys);
    free(dvals);
    free(_dbuf);
  }
  fclose(g);
  printf("IBJnls: file %s written\n", out);
  return 0;
}

static int defaults(int argc, char *argv[]) {
  char fn[256];
  FILE *f;
  int n;
  sprintf(fn, "%s/IBJnls_%s.c", path, argv[2]);
  f = fopen(fn, "w");
  if (!f) {
    printf("IBJnls: *** can't write default string access\n");
    return -1;
  }
  fprintf(f, "// default string access for \"%s\"\n", argv[2]);
  fprintf(f, "#include <string.h>\n");
  fprintf(f, "extern char**NlsGetStrings(char *pgm);\n");         //-2008-12-11
  for (n=3; n<argc; n++) {
    fprintf(f, "extern char*%s_NLS[];\n", argv[n]);
  }
  fprintf(f, "//\n");
  fprintf(f, "char**NlsGetStrings(char *pgm) {\n");
  fprintf(f, "  if (!pgm) return NULL;\n");
  for (n=3; n<argc; n++) {
    fprintf(f, "  else if (!strcmp(pgm, \"%s\")) return %s_NLS;\n",
               argv[n], argv[n]);
  }
  fprintf(f, "  return NULL; }\n");
  fclose(f);
  return 0;
}

int main(int argc, char *argv[]) {
  int i, n;
  if (argc < 3) {
    printf("usage: IBJnls [-prepare] <Name> ...\n");
    printf("       IBJnls [-defaults | -collect] <Outfile> <Name> ...\n");
    exit(0);
  }
  if (!strcmp(argv[1], "-prepare")) {
    for (i=2; i<argc; i++) {
      n = prepare(argv[i]);
    }
  }
  else if (!strncmp(argv[1], "-collect", strlen("-collect"))) {
    n = collect(argc, argv);
  }
  else if (!strcmp(argv[1], "-defaults")) {
    n = defaults(argc, argv);
  }
  return n;
}

#else //============================================================== client

#include "IBJnls.h"

#if defined __linux__         // gcc, icc
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
  #define  utimbuf    _utimbuf
  #define  TUT_DIREC  int
  #define  TUT_FINFO  struct _finddata_t
  #define  TUTVALID(a)  ((a) > 0)
  #define  TUTFNAME(a)  (a.name)
  #define  TUTFATTR(a)  ((unsigned char) (a.attrib))
  #define  TUTMKDIR(a)  mkdir(a)
#endif

extern char**NlsGetStrings(char *pgm);
int NlsCheck = 0;
char NlsLanguage[256];                                            //-2008-08-27
char NlsLanOption[256];                                           //-2008-12-19

static int starts_with(char *name, char *head) {
  int l = 0;
  if (!name || !head)
    return 0;
  l = strlen(head);
  if (l > strlen(name))
    return 0;
  return (0 == strncmp(name, head, l));
}

static int ends_with(char *name, char *head) {
  int l = 0;
  if (!name || !head)
    return 0;
  l = strlen(head);
  if (l > strlen(name))
    return 0;
  return (0 == strncmp(name + strlen(name) - l, head, l));
}

static int FindClose( TUT_DIREC d ) {
#if defined __linux__
  if (d) closedir(d);
#else
  if (d > 0) _findclose(d);
#endif
  return 0;
}

static int FindNext( TUT_DIREC d, TUT_FINFO *fip ) {
#if defined __linux__
  TUT_FINFO *direntp;
  direntp = readdir(d);
  if (!direntp) {
    FindClose(d);
    return -1;
  }
  *fip = *direntp;
  return 0;
#else
  if (_findnext(d, fip)) {
    FindClose(d);
    return -1;
  }
  return 0;
#endif
}

static TUT_DIREC FindFirst( char *path, TUT_FINFO *fip ) {
  TUT_DIREC d;
#if defined __linux__
  int n;
  d = opendir(path);
  if (!d)  return NULL;
  n = FindNext(d, fip);
  return (n) ? NULL : d;
#else
  char fn[256];
  strcpy(fn, path);
  strcat(fn, "/*");
  d = _findfirst(fn, fip);
  return d;
#endif
}

static int IsDir( int a ) {
  #ifdef __linux__
    return (a & DT_DIR) != 0;
  #else
    return (a & _A_SUBDIR) != 0;
  #endif
}

static char *GetFileName( char *path, char *head, char *tail ) {
  static char fn[256];
  int r, i;
  TUT_DIREC d;
  TUT_FINFO fi;
  d = FindFirst(path, &fi);
  r = TUTVALID(d);
  if (!r)
    return NULL;
  for (i=0; ; ) {
    if (!IsDir(TUTFATTR(fi))) {
      strcpy(fn, TUTFNAME(fi));
      if (starts_with(fn, head) && ends_with(fn, tail))
        return fn;
      i++;
    }
    if (FindNext(d, &fi))  break;
  }
  return NULL;
}

int NlsRead(char *name, char *pgm, char *lan, char *vrs)
{
  char fn[256], path[256], module[64], *key, *val, *pc;
  char **strings = NULL;
  int nstrings = 0;
  int i, j, n, k, m=0, drop=1;
  if (lan == NULL)                                                //-2008-07-22
    lan = "";
  strcpy(NlsLanOption, lan);                                      //-2008-12-19
  if (*lan == '-')                                                //-2008-09-26
    return 0;
  else if (*lan == '*') {                                         //-2008-09-30
    lan++;
    NlsCheck = 1;
  }
  if (NlsCheck) printf("NLS: NlsRead(%s, %s, %s, %s)\n", name, pgm, lan, vrs);
  strcpy(path, name);
  for (pc=path; (*pc); pc++)
    if (*pc == '\\')  *pc = '/';
  pc = strrchr(path, '/');
  if (pc) *pc = 0;
  if (*path)  strcat(path, "/");
  *NlsLanguage = 0;
  if (!*lan) {
    sprintf(fn, "%s_", pgm);
    pc = GetFileName(path, fn, ".nls");
    if (pc == NULL) {
      if (NlsCheck) printf("NLS: file \"%s\", \"%s\",\"%s\" not found\n", path, name, ".nls");
      return 0;
    }
    sprintf(fn, "%s%s", path, pc);
    n = strlen(pgm) + 1;                                          //-2008-08-27
    strncat(NlsLanguage, pc+n, strlen(pc)-n-4);                   //-2008-08-27
  }
  else {
    sprintf(fn, "%s%s_%s.nls", path, pgm, lan);
    if (!file_exists(fn)) {
      if (NlsCheck) printf("NLS: file \"%s\" not found, continue\n", fn);
      sprintf(fn, "%snls/%s/%s_%s.nls", path, lan, pgm, lan);     //-2008-09-26
      if (!file_exists(fn)) {
        if (NlsCheck) printf("NLS: file \"%s\" not found\n", fn);
        return -1;
      }
    }
    strcpy(NlsLanguage, lan);                                     //-2008-08-27
  }
  if (NlsCheck) printf("NLS: reading file \"%s\"\n", fn);
  n = read_mapping(fn);
  if (NlsCheck) printf("NLS: read_mapping(%s) = %d\n", fn, n);
  if (n < 0)
    return -1;
//
//  printf("found vers=%s in file %s, requested vrs=%s\n", vers, fn, vrs);
//
  if (vrs != NULL && strcmp(vrs, vers))                           //-2008-07-22
    return -2;
  for (i=0; i<ntkeys; i++) {
    key = tkeys[i];
    val = tvals[i];
    if (NlsCheck > 1) printf("NlsRead: %3d %s=%s<<<\n", i, key, val);
    if (!strcmp(key, "****")) {
      strcpy(module, val);
      if (NlsCheck) printf("NLS: found definitions for \"%s\"\n", module);
      strings = NlsGetStrings(module);
      drop = (strings == NULL);
      if (drop) {
        m++;
        if (NlsCheck) printf("NLS: dropping these definitions\n");
        nstrings = 0;
        continue;
      }
      for (j=0; strings[j]!=NULL; j++);
      nstrings = j;
    }
    else {
      if (drop) continue;
      if (*key == '#') {
        continue;
      }
      if (1 != sscanf(key, "%d", &k)) {
        m++;
        if (NlsCheck) printf("NLS: invalid key \"%s\" in line %d\n", key, i+1);
        continue;
      }
      if (k >= nstrings) {
        m++;
        if (NlsCheck) printf("NLS: key %d out of range in line %d\n", k, i+1);
        continue;
      }
      strings[k] = val;
    }
  }
  free(tkeys);
  free(tvals);
  return m;
}

#endif

/*
 * History:
 *
 * 2006-10-16 lj 2.2.16 created
 * 2008-07-22 lj 2.4.2  converting escape sequences
 * 2008-08-27 lj 2.4.3  filter language file, save selected language
 * 2008-09-26 lj        NlsRead(): read from nls/LAN/; drop lan=="-"
 * 2008-09-30 lj        NlsRead(): set NlsCheck if *lan=='*'
 * 2008-10-03 lj        NlsRead(): replace HT by SP
 * 2008-12-11 lj 2.4.5  header "IBJnls.h" included
 * 2008-12-19 lj 2.4.6  NlsLanOption saves the language option given
 *
 */
