//==================================================================== TalTmn.c
//
//  Table Manager, uses array's from the gen-lib.
//  =============================================
//
// Copyright (C) Umweltbundesamt, Dessau-Roﬂlau, Germany, 2002-2005
// Copyright (C) Janicke Consulting, 88662 ‹berlingen, Germany, 2002-2012
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
// last change: 2012-09-04 uj
//
//==========================================================================
//  A table is an array with a header string, valid for a specified time
//  interval. The data are stored in memory or on disk or both. They can
//  be created by the user, read from disk or created automatically, if
//  a creator is specified by the user. Attributes are:
//  TMN_UNIQUE: The data exist only once, either on disk or in memory.
//  TMN_FIXED:  The data are not moved within in the memory, therefore
//              the addresses of the elements will not change during execution
//              of the program.
//  TMN_WAIT:   If the data are not available on disk, wait and try it again.
//  TMN_LOCKED: The data may not be removed from the work space (memory).
//  TMN_MODIFY: The data will be modified and hence only one routine can
//              get access at one time.
//============================================================================

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <time.h>
#include <limits.h>

#include "IBJmsg.h"
#include "IBJary.h"
#include "IBJtxt.h"
#include "IBJdmn.h"
static char *eMODn = "TalTmn";

#include "genio.h"
#include "genutl.h"

#include "IBJstamp.h"
#include "TalNms.h"
#include "TalTmn.h"
#include "TalTmn.nls"

#define  TMN_VERSION    2
#define  TMN_SUBVERSION 6
#define  TMN_PATCH      "1"
#define  TMN_STDXTN     ".dmna"                                  //-2011-06-29

#define  DISPLAY     (TmnFlags & TMN_DISPLAY)

typedef struct tmnrec *TMNRECPTR;
typedef struct tmnrec {
  TMNRECPTR next, last;
  long id, mask, usage, storage, count;
  long t1, t2;
  long *depends;
  TMNDATSRV datserver;
  TMNHDRSRV hdrserver;
  char servname[TMN_SNMLEN];
  TXTSTR header;                                                 //-2011-06-29
  char *tag;
  long lpos1, lpos2, lt1, lt2;
  ARYDSC dsc;
  } TMNREC;

static const long  TMN_DISK      = 0x0001;
static const long  TMN_MYWRKSPC  = 0x0002;

static TMNRECPTR TmnHead, TmnTail;
static TMNNAMSRV NameServer;
static TMNHDRSRV HeaderServer;
static char TmnWorkDir[256];
static long TmnFlags, TmnLogAction;
static FILE *TmnLog;
static long IdCounter = 0x80000000;

/*================ Prototypes of static functions ========================*/

  /*=================================================================== AddRec
  Add a record with id and return a pointer to the record. If a record
  with this id already exist, NULL is returned.
  */
static TMNRECPTR AddRec(        /* pointer to new record */
  long id )                     /* id of the table       */
  ;
  /*=================================================================== AddXtn
  */
static void AddXtn(             /* add standard extension to file name  */
  char *fname )
  ;
  /*=================================================================== DelRec
  Delete a record of the table chain and the data of the table.
  */
static TMNRECPTR DelRec(        /* pointer to the next record          */
  TMNRECPTR pthis )             /* pointer to the record to be deleted */
  ;
  /*================================================================== FindRec
  Find the record where the table id is defined.
  */
static TMNRECPTR FindRec(       /* pointer to the record or NULL */
  long id )                     /* table id                      */
  ;
  /*============================================================ FindDatServer
  Find the record where a server for the masked id is defined.
  */
static TMNRECPTR FindDatServer( /* pointer to the record or NULL */
  long id )                     /* table id                      */
  ;
  /*============================================================ FindHdrServer
  Find the record where a server for the masked id is defined.
  */
static TMNRECPTR FindHdrServer( /* pointer to the record or NULL */
  long id )                     /* table id                      */
  ;
  /*================================================================== PrnRec
  Print the most important data of a record in the form:
  <line number>: <name> u:<usage> c:<count> s:<storage> [<t1>-<t2>]
  */
static int PrnRec(      /* return -1, if not initialized */
  FILE *prn,            /* print file                    */
  int n,                /* line number                   */
  TMNRECPTR prec )      /* pointer to the record         */
  ;
  /*=================================================================== TmnGet
  Try to read a table from disk. Time series of arrays are scanned. If
  the wait flag is set for the table, the routine tries infinitely
  every 2 seconds to read the table.
  */
static TMNRECPTR TmnGet(        /* pointer to the table record  */
  long id,                      /* identification               */
  long *pt1,                    /* start of the requested time  */
  long *pt2 )                   /* not used                     */
  ;
  /*========================================================== TmnHeaderServer
  Creates a header for storage on disk, if the user did not
  supply a header server.
  */
static char *TmnHeaderServer(   /* the header (global storage)  */
  long id,                      /* identification               */
  long *pt1,                    /* start of the validity time   */
  long *pt2 )                   /* end of validity time         */
  ;
  /*================================================================== TmnMake
  Make an array by calling the user supplied creator or by a call
  to system() with the user supplied text. In addition the name of
  the working directory and the options -n<hexid> and -t<time> are
  supplied.
  */
static TMNRECPTR TmnMake(       /* pointer to the table record  */
  long id,                      /* identification               */
  long *pt1,                    /* start of requested time      */
  long *pt2 )                   /* end of requested time        */
  ;
  /*============================================================ TmnNameServer
  Internal name server, if none is supplied by the user.
  */
static char *TmnNameServer(     /* file name            */
  long id )                     /* identification       */
  ;

/*============================ internal functions ==========================*/

static int endswith( char *body, char *end ) {
  int lb, le;
  if (body && end) {
    lb = strlen(body);
    le = strlen(end);
    if ((lb >= le) && !strcmp(body+lb-le, end))  return 1;
  }
  return 0;
}

  /*=================================================================== AddXtn
  */
static void AddXtn(             /* add standard extension to file name  */
  char *fname )
  {
  char *ps;
  ps = strrchr(fname, MSG_PATHSEP);
  if (ps)  ps++;
  else  ps = fname;
  if (!strchr(ps, '.'))  strcat(ps, TMN_STDXTN);
  }

  /*=================================================================== AddRec
  Add a record with id and return a pointer to the record. If a record
  with this id already exist, NULL is returned.
  */
static TMNRECPTR AddRec(        /* pointer to new record */
  long id )                     /* id of the table       */
  {
  dP(AddRec);
  TMNRECPTR prec;
  id &= ~NMS_CMMND;                                               //-2011-06-29
  for (prec=TmnHead; prec!=NULL; prec=prec->next)
    if (prec->id == id)  return NULL;
  prec = ALLOC(sizeof(TMNREC));  if (!prec)           eX(1);
  prec->id = id;
  prec->mask = -1;
  prec->next = TmnHead;
  prec->last = NULL;
  if (TmnHead)  TmnHead->last = prec;
  if (!TmnTail)  TmnTail = prec;
  TmnHead = prec;
  return prec;
eX_1:
  nMSG(_no_memory_);
  return NULL;
  }

  /*================================================================== DelRec
  Delete a record of the table chain and the data of the table.
  */
static TMNRECPTR DelRec(        /* pointer to the next record          */
  TMNRECPTR pthis )             /* pointer to the record to be deleted */
  {
  dQ(DelRec);
  TMNRECPTR pnext, plast;
  if (!pthis)  return NULL;
  if (pthis->depends)    FREE(pthis->depends);
  TxtClr(&(pthis->header));
  if (pthis->tag)        FREE(pthis->tag);
  if (pthis->dsc.start)  FREE(pthis->dsc.start);
  pnext = pthis->next;
  plast = pthis->last;
  if (pnext)  pnext->last = plast;
  else  TmnTail = plast;
  if (plast)  plast->next = pnext;
  else  TmnHead = pnext;
  FREE(pthis);
  return pnext;
  }

  /*================================================================== FindRec
  Find the record where the table id is defined.
  */
static TMNRECPTR FindRec(       /* pointer to the record or NULL */
  long id )                     /* table id                      */
  {
  TMNRECPTR prec;
  id &= ~NMS_CMMND;                                               //-2011-06-29
  for (prec=TmnHead; prec!=NULL; prec=prec->next)
    if (prec->id == id)  break;
  return prec;
  }

  /*============================================================= FindDatServer
  Find the record where a server for the masked id is defined.
  */
static TMNRECPTR FindDatServer( /* pointer to the record or NULL */
  long id )                     /* table id                      */
  {
  TMNRECPTR prec;
  for (prec=TmnHead; prec!=NULL; prec=prec->next)
    if ((prec->id == id) && (prec->datserver))  return prec;
  for (prec=TmnHead; prec!=NULL; prec=prec->next) {
    if ((prec->id == (id & prec->mask)) && (prec->datserver))  return prec;
    }
  return NULL;
  }

  /*============================================================= FindHdrServer
  Find the record where a server for the masked id is defined.
  */
static TMNRECPTR FindHdrServer( /* pointer to the record or NULL */
  long id )                     /* table id                      */
  {
  TMNRECPTR prec;
  for (prec=TmnHead; prec!=NULL; prec=prec->next)
    if ((prec->id == id) && (prec->hdrserver))  return prec;
  for (prec=TmnHead; prec!=NULL; prec=prec->next) {
    if ((prec->id == (id & prec->mask)) && (prec->hdrserver))  return prec;
    }
  return NULL;
  }

  /*================================================================== PrnRec
  Print the most important data of a record in the form:
  <line number>: <name> u:<usage> c:<count> s:<storage> <bytes> [<t1>-<t2>]
  */
static int PrnRec(      /* return -1, if not initialized */
  FILE *prn,            /* print file                    */
  int n,                /* line number                   */
  TMNRECPTR prec )      /* pointer to the record         */
  {
  char name[256], usage[40], storage[40], t1s[40], t2s[40], t[40], srv[4];
  char *psep, mem[40];
  int i, k, na=-1;
  if (!prec)  return 0;
  if (!NameServer)  return -1;
  strcpy( name, NameServer(prec->id) );
  psep = strrchr(name, MSG_PATHSEP);
  if (psep)  psep++;
  else  psep = name;
  if (!strchr(psep, '.'))  strcat(psep, "    ");
  strcpy(usage, "u:");
  if (prec->usage & TMN_CREATED)  strcat(usage, "c");
  if (prec->usage & TMN_DEFINED)  strcat(usage, "d");
  if (prec->usage & TMN_FIXED)    strcat(usage, "f");
  if (prec->usage & TMN_LOCKED)   strcat(usage, "l");
  if (prec->usage & TMN_MODIFY)   strcat(usage, "m");
  if (prec->usage & TMN_MODIFIED) strcat(usage, "M");
  if (prec->usage & TMN_UNIQUE)   strcat(usage, "u");
  if (prec->usage & TMN_WAIT)     strcat(usage, "w");
  if (prec->usage & TMN_UPDATE)   strcat(usage, "+");
  if (prec->usage & TMN_DEFINED) {
    na = (prec->dsc.start)? prec->dsc.ttlsz : 0;
  }
  k = 1;
  strcpy(storage, "s:");
  for (i=0; i<31; i++) {
    if (prec->storage & k) {
      sprintf(t, " %d", i);
      strcat(storage, t); }
    k <<= 1;
    }
  if (prec->datserver) strcpy(srv, "S");
  else  strcpy(srv, "-");
  if (prec->hdrserver) strcat(srv, "H");
  else  strcat(srv, "-");
  strcpy(t1s, TmString(&(prec->t1)));
  strcpy(t2s, TmString(&(prec->t2)));
  if (na >= 0)  sprintf(mem, "%9d", na);
  else strcpy(mem, "        -");
  fprintf(prn, "%3d: %12s %-7s %2ld %s %-8s %s [%s,%s]\n", n, psep, usage,
                prec->count, srv, storage, mem, t1s, t2s);
  return 0;
  }

/*=================================================================== MakeFn
 *   Combine file name with path name
 */
static char *MakeFn( char *name )
  {
  static char fn[256];
  if (name[0] == '~') {           
    strcpy(fn, TmnWorkDir);
    if (strlen(fn)+strlen(name) > 255)  exit(99);
    else  strcat(fn, name+1);
    }
  else {
    if (strchr(name, MSG_PATHSEP) || strchr(name, ':'))  strcpy(fn, name);
    else {
      strcpy(fn, TmnWorkDir);
      if (strlen(fn)+strlen(name) > 255)  exit(99);
      else  strcat(fn, name);
      }
    }
  return fn;
  }

/*=========================== external functions ===========================*/

  /*============================================================== TmnArchive
  Store a table on disk. The name (id) may be changed and the data
  may be appended to an existing file. If the name is not changed,
  the data are registered as "saved on disk" (flag TMN_DISK).
  */
long TmnArchive(        /* returns 0 or error code      */
  long id,              /* old identification           */
  char *name,           /* new name                     */
  int options )         /* append or setundef or -pos   */
  {
  dP(TmnArchive);
  char iname[256], jname[256], fname[256], *hdr;
  long pos1, pos2, t1, t2, pos=0, rc=0;
  int changename=0, setundef=0, append=0;
  TXTSTR usrhdr = { NULL, 0 };
  TMNRECPTR pr, ps;
  TMNHDRSRV hdrsrv;
  if (!NameServer)  return -1;
  if (options > 0) {                                          //-2000-02-21 lj
    append = options & TMN_APPEND;
    setundef = options & TMN_SETUNDEF;
  }
  else  pos = -options;
  if ((!name) || (!*name))  *jname = 0;
  else  strcpy(jname, name);
  strcpy(iname, NameServer(id));
  AddXtn(iname);
  if (!*jname)  strcpy(jname, iname);
  changename = (NULL != name);          
  if (TmnLogAction) fprintf(TmnLog, "TmnArchive(%s, %s, %d)\n",
    iname, jname, append);
  pr = FindRec(id);  if (!pr)                                   eX(1);
  if (pr->usage & TMN_MODIFY)                                   eX(2);
  if (setundef) {
    t1 = TmMin();
    t2 = TmMax(); }
  else {
    t1 = pr->t1;
    t2 = pr->t2; }
  if ((!pr->header.s) || (!*pr->header.s)) {
    if (pr->hdrserver)  
      hdrsrv = pr->hdrserver;
    else {
      ps = FindHdrServer(id & ~NMS_CMMND);                        //-2011-06-29
      if (ps)  hdrsrv = ps->hdrserver;
      else  hdrsrv = TmnHeaderServer;
      }
    hdr = (*hdrsrv)(id, &t1, &t2);
    TxtCpy(&(pr->header), hdr);
    FREE(hdr);
    hdr = NULL;
  }
  else {
    TmnSetTimes( &(pr->header), &t1, &t2 );                     eG(7);
  }
  if (pr->usage & TMN_INVALID) 
    DmnRplValues(&pr->header, "valid", "0");                     //-2011-06-29
  strcpy(fname, MakeFn(jname));   
  //
  // write DMNA                                                  //-2011-06-29
  pos1 = 0;
  pos2 = 0;
  if (endswith(fname, ".dmna")) { 
    TxtCpy(&usrhdr, pr->header.s);
    DmnRplName(&usrhdr, "FORMAT", "form");
    TxtCat(&usrhdr, "mode binary\n");
    TxtCat(&usrhdr, "cmpr 7\n");
    rc = DmnWrite(fname, usrhdr.s, &pr->dsc);
    TxtClr(&usrhdr);
    if (rc < 0)                                                   eX(5);
  }
  else                                                            eX(30);
  //
  /*
  if (pos > 0) {                                        //-2000-02-21 lj
    datafile = fopen(fname, "rb+");
    if (datafile)  fseek(datafile, pos, SEEK_SET);
    else  datafile = fopen(fname, "wb+");
  }
  else if (append) {
    datafile = fopen(fname, "rb+");
    if (datafile)  fseek(datafile, 0, SEEK_END);
    else  datafile = fopen(fname, "wb+");
    }
  else  datafile = fopen(fname, "wb+");
  if (!datafile)                                                eX(4);
  pos1 = ftell(datafile);  if (pos1 < 0)                        eX(10);
  ArrWriteMul(jname, pr->header, &(pr->dsc), &datafile);        eG(5);
  pos2 = ftell(datafile);  if (pos2 < 0)                        eX(11);
  fclose(datafile);
  */
  if (TmnLogAction)
    fprintf(TmnLog, "%s archived at (%ld,%ld)\n", jname, pos1, pos2);
  if (!changename) {
    pr->storage |= TMN_DISK;
    pr->lpos1 = pos1;
    pr->lpos2 = pos2;
    pr->lt1 = pr->t1;
    pr->lt2 = pr->t2;
    }
  if ((!changename) && (pr->usage & TMN_UNIQUE)) {
    if (pr->count > 0)                                          eX(6);
    TxtClr(&(pr->header));
    if (pr->tag)        FREE(pr->tag);
    pr->tag = NULL;
    if (pr->dsc.start)  FREE(pr->dsc.start);
    pr->dsc.start = NULL;
    pr->storage &= ~TMN_MYWRKSPC;
    pr->usage &= ~TMN_LOCKED;
    }
  return pos2;                                                  //-2000-02-21 lj
eX_1:
  eMSG(_cant_find_$_, iname);
eX_2:
  eMSG(_$_used_, iname);
eX_5:
  eMSG(_$_not_written_, jname);
eX_6:
  eMSG(_$_used_, iname);
eX_7:
  eMSG(_cant_set_times_$_, iname);
//eX_10: eX_11:
//  eMSG(_error_position_$_, fname);
eX_30:
  eMSG("unsupported operation");
  }

  /*=============================================================== TmnAttach
  Get the array and the header of a table. If no data are defined,
  get and make are tried. A lock is removed during the first attach,
  but may be requested in mode.
  */
ARYDSC *TmnAttach(      /* pointer to the array                 */
  long id,              /* identification                       */
  long *pt1,            /* start of the requested time interval */
  long *pt2,            /* end of time, is set if *pt1==*pt2    */
  int mode,             /* 0 or TMN_MODIFY or TMN_LOCK          */
  TXTSTR *hdr )         /* copy of the header                   */
  {
  dP(TmnAttach);
  TMNRECPTR pr;
  char name[256], t1s[40], t2s[40], t1t[40], t2t[40];
  TXTSTR fname = { NULL, 0 };
  int r;
  if (!NameServer)  
    return NULL;
  pr = FindRec(id);
  if ((id < 0) && (pr) && (*(pr->servname)))  strcpy(name, pr->servname);
  else  strcpy(name, NameServer(id));
  if (TmnLogAction) {
    int usage;
    usage = (pr) ? pr->usage : 0;
    strcpy(t1s, TmString(pt1));
    strcpy(t2s, TmString(pt2));
    fprintf(TmnLog, "TmnAttach(%s [%s,%s] %08x(%08x))\n",
      name, t1s, t2s, mode, usage);
    }
  if (!pr) {
    pr = AddRec(id);
    if (!pr)                                            eX(20);
    pr->usage |= (mode & TMN_USRFLAGS);
    }
  if ((pr) && (pr->usage & TMN_DEFINED)) {
    r = TmRelation(pr->t1, pr->t2, pt1, pt2);
    if (r < 0)                                          eX(7);
    if (r > 0)  pr->usage |= TMN_UPDATE;
  }
  if ((!pr) || ((pr->storage&TMN_MYWRKSPC) == 0) || (pr->usage&TMN_UPDATE)) {
    pr = TmnGet(id, pt1, pt2);                          eG(2);
    if ((!pr) && (mode & TMN_READ))
      return NULL;
  }
  if ((!pr) || ((pr->usage & TMN_DEFINED) == 0) || (pr->usage & TMN_UPDATE)) {
    pr = TmnMake(id, pt1, pt2);                         eG(1);
    if (!pr)
      return NULL;
  }
  if (pr->usage & TMN_MODIFY)                           eX(3);
  if (mode & TMN_MODIFY) {
    if (pr->count > 0)                                  eX(4);
    pr->usage |= (mode & TMN_USRFLAGS) | TMN_MODIFY;
    if (pt1)  *pt1 = pr->t1;
    if (pt2)  *pt2 = pr->t2;
    if (pr->storage & TMN_DISK) {
      if (name[0] != MSG_PATHSEP)  TxtCpy(&fname, TmnWorkDir);
      TxtCat(&fname, name);
      if (strlen(fname.s) >= 256)                       eX(8);
      if (remove(fname.s))                              eX(10);
      pr->storage &= ~TMN_DISK;
      pr->lpos1 = 0;
      pr->lpos2 = 0;
      pr->lt1 = 0;
      pr->lt2 = 0;
    }
  }
  else {
    if (pt2) {
      if ((pt1) && (*pt2 > *pt1) && (*pt2 > pr->t2))    eX(6);
      if ((pt1) && (*pt2 == *pt1))  *pt2 = pr->t2;
    }
    if ((pt1) && (*pt1 < pr->t1)) {
      if (*pt1 == TmMin())  *pt1 = pr->t1;
      else                                              eX(5);
    }
  }
  if (!pr->count)  pr->usage &= ~TMN_LOCKED;
  if (mode & TMN_LOCKED)  pr->usage |= TMN_LOCKED;
  if (mode & TMN_FIXED)   pr->usage |= TMN_FIXED;
  pr->count++;
  if (hdr)  TxtCpy(hdr, pr->header.s);                            //-2011-06-29
  TxtClr(&fname);
  return &(pr->dsc);
eX_20:  eX_1:
  strcpy(t1s, TmString(pt1));
  strcpy(t2s, TmString(pt2));
  nMSG(_cant_attach_$$$_, name, t1s, t2s);
  return NULL;
eX_2:
  nMSG(_cant_get_$_, name);
  return NULL;
eX_3:  eX_4:
  nMSG(_$_used_, name);
  return NULL;
eX_5:
  strcpy(t1s, TmString(pt1));
  nMSG(_time_1_$$$_, t1s, TmString(&(pr->t1)), name);
  return NULL;
eX_6:
  strcpy(t2s, TmString(pt2));
  nMSG(_time_2_$$$_, t2s, TmString(&(pr->t2)), name);
  return NULL;
eX_7:
  strcpy(t1s, TmString(&pr->t1));
  strcpy(t2s, TmString(&pr->t2));
  strcpy(t1t, TmString(pt1));
  strcpy(t2t, TmString(pt2));
  nMSG(_cant_update_$$$_$$_, name, t1s, t2s, t1t, t2t);
  return NULL;
eX_8:
  nMSG(_name_too_long_$_, fname.s);
  return NULL;
eX_10:
  nMSG(_cant_remove_$_, fname.s);
  return NULL;
  }

  /*================================================================ TmnClear
  Clear tables from the workspace, that are not valid beyond
  time t. Tables valid only at time t are not cleared. Time values
  and information on disk storage are preserved, but the tables
  are regarded as "not defined".
  */
long TmnClear(          /* number of cleared tables     */
  long t,               /* validity time threshold      */
  long id, ... )        /* id's of tables, ends with 0  */
  {
  dP(TmnClear);
  va_list argptr;
  TMNRECPTR pr;
  char name[256];
  int n;
  if (!NameServer)  return -1;
  va_start( argptr, id );
  n = 0;
  while (id) {
    if (TmnLogAction) fprintf(TmnLog, "TmnClear(%08lx)\n", id);
    pr = FindRec(id);
    if ((pr) && ((pr->t2 < t) || ((pr->t2 == t) && (pr->t1 < pr->t2)))) {
      strcpy( name, NameServer(id) );
      if (pr->count > 0)                                eX(1);
      if (pr->usage & TMN_FIXED)                        eX(2);
      TxtClr(&(pr->header));
      if (pr->tag)        FREE(pr->tag);
      pr->tag = NULL;
      if (pr->dsc.start)  FREE(pr->dsc.start);
      pr->dsc.start = NULL;
      pr->storage &= ~TMN_MYWRKSPC;
      pr->usage   &= ~(TMN_DEFINED | TMN_MODIFIED | TMN_CREATED);
      n++;
      if (TmnLogAction) fprintf(TmnLog, "TmnClear(%s)\n", name);
      }
    id = va_arg( argptr, long );
    }
  va_end( argptr );
  return n;
eX_1:
  eMSG(_$_not_cleared_, name);
eX_2:
  eMSG(_cant_delete_fixed_$_, name);
  }

  /*============================================================== TmnClearAll
  Clear tables from the workspace using a name mask: all tables
  are cleared for which (id & mask) == id_parameter (cf. TmnClear).
  */
long TmnClearAll(       /* number of cleared tables     */
  long t,               /* validity time threshold      */
  long mask,            /* mask for name id             */
  long id, ... )        /* id's of tables, ends with 0  */
  {
  dP(TmnClearAll);
  va_list argptr;
  TMNRECPTR pr;
  char name[256];
  int n;
  if (!NameServer)  return -1;
  va_start(argptr, id);
  n = 0;
  while (id) {
    if (TmnLogAction) fprintf(TmnLog, "TmnClearAll(%08lx)\n", id);
    id &= mask;
    for (pr=TmnHead; pr!=NULL; pr=pr->next)
      if ((pr->id & mask) == id) {
        n += TmnClear(t, pr->id, TMN_NOID);                     eG(1);
      }
    id = va_arg(argptr, long);
    }
  va_end(argptr);
  return  n;
eX_1:
  strcpy(name, NameServer(pr->id));
  eMSG(_cant_clear_$_, name);
  }

  /*=============================================================== TmnCreate
  Create a table with identification id.
  */
ARYDSC *TmnCreate(      /* pointer to the array of the table    */
  long id,              /* identification of the table          */
  int elmsz,            /* element size in the array            */
  ... )                 /* <dims>, <i1>, <i2>, <j1>, <j2> ...   */
  {
  dP(TmnCreate);
  va_list argptr;
  ARYDSC *pa;
  TMNRECPTR pr;
  int numdm, g, i;
  char name[256];
  if (!NameServer) return NULL;
  strcpy( name, NameServer(id) );
  if (TmnLogAction) {
    fprintf(TmnLog, "TmnCreate(%s,%d", name, elmsz);
    if (elmsz > 0) {
      va_start( argptr, elmsz );
      numdm = va_arg( argptr, int );
      fprintf(TmnLog, ",%d", numdm);
      for (i=0; i<numdm; i++) {
        g = va_arg( argptr, int );
        fprintf(TmnLog, ",%d", g);
        g = va_arg( argptr, int );
        fprintf(TmnLog, ",%d", g);
        }
      va_end( argptr );
      }
    fprintf(TmnLog, ")\n");
    }
  pr = FindRec(id);
  if (!pr)  pr = AddRec(id);                    eG(1);
  if (!pr)                                      eX(10);
  if (pr->usage & TMN_DEFINED)                  eX(2);
  pr->usage = TMN_CREATED | TMN_DEFINED | TMN_MODIFY;
  pr->storage = TMN_MYWRKSPC;
  pr->count = 1;
  pr->t1 = TmMin();
  pr->t2 = TmMax();
  pa = &(pr->dsc);
  if (elmsz < 1 )  return pa;
  va_start( argptr, elmsz );
  numdm = va_arg( argptr, int );
  pa->elmsz = elmsz;
  pa->maxdm = ARYMAXDIM;
  pa->numdm = numdm;
  for (i=0; i<numdm; i++) {
    g = va_arg( argptr, int );
    pa->bound[i].low = g;
    g = va_arg( argptr, int );
    pa->bound[i].hgh = g; }
  va_end( argptr );
  AryDefine(pa, -1);                                            eG(4);
  pa->start = ALLOC(pa->ttlsz);  if (!pa->start)              eX(5);
  return pa;
eX_1:  eX_10:
  nMSG(_cant_create_entry_$_, name);
  return NULL;
eX_2:
  nMSG(_duplicate_entry_$_, name);
  return NULL;
eX_4:
  nMSG(_not_allocated_$_, name);
  return NULL;
eX_5:
  nMSG(_no_memory_);
  return NULL;
  }

  /*=============================================================== TmnCreator
  Specify, which routine can create a certain table. If a server
  is specified, no system() call will be tried.
  */
long TmnCreator(        /* returns 0 or error code              */
  long id,              /* identification of the table          */
  long mask,            /* mask for grouping entries            */
  long usage,           /* 0 or TMN_WAIT                        */
  char *sname,          /* name and options for system()        */
  TMNDATSRV datsrv,     /* address of data server routine       */
  TMNHDRSRV hdrsrv )    /* address of header server routine     */
  {
  dP(TmnCreator);
  TMNRECPTR pr;
  char name[256], *sn;
  if (!NameServer)  return -1;
  id &= mask;
  usage &= TMN_USRFLAGS;
  strcpy( name, NameServer(id) );
  pr = FindRec(id);
  if (!pr)  pr = AddRec(id);
  if (!pr)                                      eX(1);
  pr->usage |= usage;
  pr->mask  = mask;
  pr->datserver = datsrv;
  pr->hdrserver = hdrsrv;
  if (sname) {
    if (strlen(sname) > TMN_SNMLEN-1)           eX(2);
    strcpy(pr->servname, sname);
    }
  sn = (pr->servname) ? pr->servname : "(null)";
  if (TmnLogAction) fprintf(TmnLog, "TmnCreator(%s/%08lx, %08lx, %s)\n",
    name, id, mask, sn);
  return 0;
eX_1:
  eMSG(_cant_register_$_, name);
eX_2:
  eMSG(_string_too_long_$_, name);
  }

  /*=============================================================== TmnDelete
  Delete tables from workspace and from disk, that are not valid
  beyond time t. Tables valid only at time t are not deleted. Only
  user supplied usage information is retained.
  A file is deleted from disk, if the table is created and archived
  by TMN.
  */
long TmnDelete(         /* returns number of deleted tables     */
  long t,               /* validity time threshold              */
  long id, ... )        /* id's of the tables, ends with 0      */
  {
  dP(TmnDelete);
  va_list argptr;
  TMNRECPTR pr;
  char fname[256], name[256];
  int n;
  if (!NameServer)  return -1;
  va_start( argptr, id );
  n = 0;
  while (id) {
    if (TmnLogAction) fprintf(TmnLog, "TmnDelete(%s, %08lx)\n", TmString(&t), id);
    pr = FindRec(id);
    if ((pr) && ((pr->t2 < t) || ((pr->t2 == t) && (pr->t1 < pr->t2)))) {
      strcpy(name, NameServer(id));
      if (TmnLogAction)  fprintf(TmnLog, "TmnDelete: deleting %s\n", name);
      if ((id > 0) && (pr->count > 0))                  eX(1);
      if (pr->usage & TMN_FIXED)                        eX(4);
      TxtClr(&(pr->header));
      if (pr->tag)        FREE(pr->tag);
      pr->tag = NULL;
      if (pr->dsc.start)  FREE(pr->dsc.start);
      pr->dsc.start = NULL;
      pr->storage &= ~TMN_MYWRKSPC;
      if (pr->storage & TMN_DISK) {
        strcpy(fname, MakeFn(name));          /* 99-08-26 */
        if (TmnLogAction)  fprintf(TmnLog, "TmnDelete: removing %s\n", fname);
        if (remove(fname))                              eX(2);
        pr->storage &= ~TMN_DISK;
        }
      if (pr->storage)                                  eX(3);
      pr->usage = 0;
      pr->count = 0;
      pr->lpos1 = 0;
      pr->lpos2 = 0;
      pr->lt1 = 0;
      pr->lt2 = 0;
      pr->t1 = 0;
      pr->t2 = 0;
      if (id < 0)  DelRec(pr);
      n++;
      if (TmnLogAction) fprintf(TmnLog, "TmnDelete(%s)\n", name);
      }
    id = va_arg( argptr, long );
    }
  va_end( argptr );
  return n;
eX_1:
  eMSG(_$_not_deleted_, name);
eX_2:
  eMSG(_cant_remove_$_, fname);
eX_3:
  eMSG(_how_delete_$_, name);
eX_4:
  eMSG(_cant_delete_fixed_$_, name);
  }

  /*============================================================= TmnDeleteAll
  Delete tables from the workspace using a name mask: all tables
  are deleted for which (id & mask) == id_parameter (cf. TmnDelete).
  */
long TmnDeleteAll(      /* number of deleted tables     */
  long t,               /* validity time threshold      */
  long mask,            /* mask for name id             */
  long id, ... )        /* id's of tables, ends with 0  */
  {
  dP(TmnDeleteAll);
  va_list argptr;
  TMNRECPTR pr, pn;
  char name[256];
  int n;
  if (!NameServer)  return -1;
  va_start(argptr, id);
  n = 0;
  while (id) {
    if (TmnLogAction) fprintf(TmnLog, "TmnDeleteAll(%08lx)\n", id);
    id &= mask;
    for (pr=TmnHead; pr!=NULL; ) {
      pn = pr->next;
      if ((pr->id & mask) == id) {
        n += TmnDelete(t, pr->id, TMN_NOID);                    eG(1);
        }
      pr = pn;
      }
    id = va_arg(argptr, long);
    }
  va_end(argptr);
  return  n;
eX_1:
  strcpy(name, NameServer(pr->id));
  eMSG(_cant_delete_$_, name);
  }

  /*================================================================ TmnDetach
  Release an attached table. If the modify flag is set in mode, the
  times and the header are updated. A lock may be given in mode.
  */
long TmnDetach(         /* returns 0 or error code              */
  long id,              /* identification                       */
  long *pt1,            /* start of validity time interval      */
  long *pt2,            /* end of validity time interval        */
  long mode,            /* TMN_MODIFY, if attached in this mode */
  TXTSTR *phdr )        /* copy of the header                   */
  {
  dP(TmnDetach);
  TMNRECPTR pr;
  char name[256];
  if (!NameServer)  return -1;
  pr = FindRec(id);  if (!pr)                           eX(1);
  if ((id < 0) && (*(pr->servname)))  strcpy(name, pr->servname);
  else  strcpy(name, NameServer(id));
  if (TmnLogAction) {
    char t1s[40], t2s[40];
    int usage;
    usage = (pr) ? pr->usage : 0;
    strcpy(t1s, TmString(pt1));
    strcpy(t2s, TmString(pt2));
    fprintf(TmnLog, "TmnDetach(%s [%s,%s] %08lx(%08x))\n",
      name, t1s, t2s, mode, usage);
    }
  if ((pr->usage & TMN_DEFINED) == 0)                   eX(2);
  if (pr->count <= 0)                                   eX(3);
  if ((mode & TMN_MODIFY) != (pr->usage & TMN_MODIFY))  eX(4);
  if (mode & TMN_MODIFY) {
    if ((pr->usage & TMN_MODIFY) == 0)                  eX(5);
    if (pt1)  pr->t1 = *pt1;
    if (pt2)  pr->t2 = *pt2;
    if (phdr) {                                                   //-2011-06-29
      TxtClr(&pr->header);
      TxtCpy(&pr->header, phdr->s);
    }
    if (mode & TMN_SETALL)    pr->usage &= ~TMN_USRFLAGS;
    if (mode & TMN_SETVALID)  pr->usage &= ~TMN_INVALID;  //-2001-06-29
    pr->usage |= (mode & TMN_USRFLAGS);
    pr->usage &= ~TMN_MODIFY;
    pr->usage |= TMN_MODIFIED;
    }
  pr->count--;
  if (mode & TMN_LOCKED)  pr->usage |= TMN_LOCKED;
  if (mode & TMN_FIXED)   pr->usage |= TMN_FIXED;
  return 0;
eX_1:
  eMSG(_$_unknown_, NameServer(id));
eX_2:  eX_3:  eX_4:  eX_5:
  eMSG(_cant_detach_$_, name);
}

  /*================================================================ TmnFinish
  */
int TmnFinish(          /* delete all records and data */
  void )
  {
  int n;
  n = 0;
  while (TmnHead) {
    n++;
    DelRec( TmnHead );
    }
  return n;
  }

  /*=================================================================== TmnGet
  Try to read a table from disk. Time series of arrays are scanned. If
  the wait flag is set for the table, the routine tries infinitely
  every 2 seconds to read the table.
  */
static TMNRECPTR TmnGet(        /* pointer to the table record  */
  long id,                      /* identification               */
  long *pt1,                    /* start of the requested time  */
  long *pt2 )                   /* not used                     */
  {
  dP(TmnGet);
  TMNRECPTR pr;
  char name[256], t1s[40], t2s[40], fname[256], gname[256], s[256];
  char *ps, *pe, *pn;
  long t1, t2, rc, cntr;
  float valid;
  FILE *datafile;
  int done, has_extension, is_def_file, is_dmn_file; //-2011-06-29
  TXTSTR usrhdr = { NULL, 0 };
  TXTSTR syshdr = { NULL, 0 };
  char *_ss = NULL;
  int n = 0;
  if (!NameServer)  
    return NULL;
  cntr = 0;
  pr = FindRec(id);
  if ((id < 0) && (pr))  strcpy( name, pr->servname );
  else  strcpy( name, NameServer(id) );
  if (TmnLogAction)  {
    char t1s[40], t2s[40];
    int usage;
    usage = (pr) ? pr->usage : 0;
    strcpy(t1s, TmString(pt1));
    strcpy(t2s, TmString(pt2));
    fprintf(TmnLog, "TmnGet (%s [%s,%s], %08x)\n", name, t1s, t2s, usage);
    }
  if (!pr)  pr = AddRec(id);
  if (!pr)                                                              eX(1);
  pn = MakeFn(name);  if (!pn)                                          eX(3);
  strcpy(fname, pn);                  
  pe = strrchr(fname, '.');
  ps = strrchr(fname, MSG_PATHSEP);
  if ((pe) && ((!ps) || (pe > ps))) {
    has_extension = 1;
    is_def_file = (0 == strcmp(pe, ".def"));
    is_dmn_file = (0 == strcmp(pe, ".dmna"));
  }
  else {
    has_extension = 0;
    is_def_file = 0;
    is_dmn_file = 0;  
  }
  strcpy(gname, fname);
  datafile = NULL;
  done = 0;
  if (pr->usage & TMN_DEFINED) {
    if ((pr->usage & TMN_UPDATE) == 0)                                  eX(2);
    if (pr->storage & TMN_DISK) {
      if ((pt1) && (*pt1 >= pr->lt2) && (pr->lt2 > pr->lt1))            eX(20);
    }
  }
  if (!datafile && (!has_extension || is_dmn_file)) {
    strcpy(fname, gname);
    if (!has_extension) {  
      if (strlen(fname) >= 255-5)                                       eX(4);
      strcat(fname, ".dmna");
    }
    datafile = fopen(fname, "r");
    if (datafile)  is_dmn_file = 1;
  }
  if (!datafile) {
    if (pr->storage & TMN_DISK)                                         eX(22);
    return NULL;
  }  
  fclose(datafile);
  datafile = NULL;
  if (pr->dsc.start != NULL)                                            eX(201);
  DmnRead(fname, &usrhdr, &syshdr, &pr->dsc);                           eG(40);
  n = DmnGetString(syshdr.s, "form|format", &_ss, 0);  if (n < 0)       eX(43); //-2007-04-24
  TxtCat(&usrhdr, "form \"");
  TxtCat(&usrhdr, _ss);
  TxtCat(&usrhdr, "\"\n");
  if (0 < DmnGetString(usrhdr.s, "t1", &_ss, 1))
    t1 = TmValue(_ss);
  else  t1 = TmMin();
  if (0 < DmnGetString(usrhdr.s, "t2", &_ss, 1)) {
    t2 = TmValue(_ss);
    if (t2 == TmMin())  t2 = TmMax();
  }
  else  t2 = TmMax();
  if (_ss) FREE(_ss);
  _ss = NULL;
  strcpy(t1s, TmString(&t1));
  strcpy(t2s, TmString(&t2));
  if (TmnLogAction)
    fprintf(TmnLog, "DmnRead(%s) [%s,%s]\n", fname, t1s, t2s);
  if ((pt1) && (*pt1<t1 || *pt1>t2))                                   eX(41);
  pr->lpos1 = 0;
  pr->lpos2 = 0;
  pr->lt1 = 0;
  pr->lt2 = 0;
  TxtCpy(&(pr->header), usrhdr.s);                                  //-2011-06-29
  pr->t1 = t1;
  pr->t2 = t2;
  pr->storage |= TMN_MYWRKSPC;
  TxtClr(&usrhdr);
  TxtClr(&syshdr);
  if (pr->usage & TMN_UNIQUE) {
    if (remove(fname))                                                   eX(8);
    pe = strrchr(fname, '.');
    if (pe) {
      if (strlen(fname) >= 255-8)                                        eX(5);
      *pe = 0;
      sprintf(s, "%s.dmnt", fname); remove(s);
      sprintf(s, "%s.dmnt.gz", fname); remove(s);
      sprintf(s, "%s.dmnb", fname); remove(s);
      sprintf(s, "%s.dmnb.gz", fname); remove(s);
    }
    pr->storage &= ~TMN_DISK;
  }
  pr->usage &= ~TMN_UPDATE;
  pr->usage |= TMN_DEFINED|TMN_CREATED;
  rc = DmnGetFloat(pr->header.s, "valid", "%f", &valid, 1);
  if (rc && valid==0)
    pr->usage |= TMN_INVALID;
  pr->storage |= TMN_MYWRKSPC;
  return pr;
eX_1:
  nMSG(_cant_register_$_, name);
  return NULL;
eX_2:
  nMSG(_duplicate_name_$_, name);
  return NULL;
eX_3: eX_4: eX_5:
  nMSG(_name_too_long_);
  return NULL;
eX_8:
  nMSG(_cant_remove_file_$_, fname);
  return NULL;
eX_20:
  nMSG(_cant_restore_$_, name);
  return NULL;
eX_22:
  nMSG(_file_$_not_found_, fname);
  return NULL;
eX_41:
  nMSG(_file_$$$_invalid_$_, fname, t1s, t2s, TmString(pt1));
  return NULL;
eX_40:
  nMSG(_cant_read_$_, fname);
  return NULL;
eX_43:
  nMSG(_no_format_string_$_, fname);
  return NULL;
eX_201:
  nMSG(_descriptor_already_used_$_, fname);
  return NULL;
}

  /*========================================================== TmnHeaderServer
  Creates a header for storage on disk, if the user did not
  supply a header server.
  */
static char *TmnHeaderServer(   /* the header (global storage)  */
  long id,                      /* identification               */
  long *pt1,                    /* start of the validity time   */
  long *pt2 )                   /* end of validity time         */
  {
  char name[256], t1s[40], t2s[40], s[100+256];
  TXTSTR hdr = { NULL, 0 };
  if (!NameServer)  
    return NULL;
  strcpy( name, NameServer(id) );
  strcpy( t1s, TmString(pt1) );
  strcpy( t2s, TmString(pt2) );
  TxtCpy(&hdr, "\n");
  sprintf(s, "TMN_%d.%d.%s", TMN_VERSION, TMN_SUBVERSION, TMN_PATCH);
  sprintf(s, "prgm  \"%s\"\n", s);
  TxtCat(&hdr, s);
  sprintf(s, "name  \"%s\"\n", name);
  TxtCat(&hdr, s);
  sprintf(s, "t1    \"%s\"\n", t1s);
  TxtCat(&hdr, s);
  sprintf(s, "t2    \"%s\"\n", t2s);
  TxtCat(&hdr, s);
  return hdr.s;
}

  /*================================================================ TmnIdent
  Returns a new Id for private data.
  */
long TmnIdent(          /* returns new Id                       */
  void )
{
  long id;
  id = ++IdCounter;
  return id;
}

  /*================================================================= TmnInfo
  Gives information on selected table.
  */
int TmnInfo(            /* returns 1, if record exist           */
  long id,              /* identification                       */
  long *pt1,            /* pointer to begin of time interval    */
  long *pt2,            /* pointer to end of time interval      */
  long *pu,             /* pointer to usage flags               */
  long *ps,             /* pointer to storage flags             */
  long *pc )            /* pointer to user count                */
{
  TMNRECPTR pr;
  if (pu) *pu = 0;
  pr = FindRec(id);
  if (!pr)  return 0;
  if (pt1)  *pt1 = pr->t1;
  if (pt2)  *pt2 = pr->t2;
  if (pu)  *pu = pr->usage;
  if (ps)  *ps = pr->storage;
  if (pc)  *pc = pr->count;
  return 1;
}

  /*================================================================ TmnInit
  Initialisation of the table manager.
  */
int TmnInit(                    /* returns always 0                     */
  char *wdir,                   /* working directory                    */
  TMNNAMSRV nameserver,         /* user supplied name server            */
  TMNHDRSRV headerserver,       /* user supplied header server          */
  long m,                       /* 0 or TMN_LOGALLOC or TMN_LOGACTION   */
  FILE *f )                     /* log file                             */
{
  int l;
  TmnFlags = m;
  if (wdir) {
    strcpy(TmnWorkDir, wdir);
    l = strlen(TmnWorkDir);
    if ((TmnWorkDir[l-1] != MSG_PATHSEP) && (TmnWorkDir[l-1] != ':')) {
      TmnWorkDir[l] = MSG_PATHSEP;
      TmnWorkDir[l+1] = 0; }
    }
  else  *TmnWorkDir = 0;
  SetDataPath( TmnWorkDir );
  if (nameserver)  NameServer = nameserver;
  else  NameServer = TmnNameServer;
  HeaderServer = headerserver;
  TmnLog = f;
  TmnLogAction = ((f) && (m & TMN_LOGACTION));
  lMSG(3,5)("TMN_%d.%d.%s %s", 
    TMN_VERSION, TMN_SUBVERSION, TMN_PATCH, IBJstamp(__DATE__, __TIME__));
  return 0; 
}

  /*================================================================== TmnList
  Print a list of all records in the table chain, format:
  <line number>: <name> u:<usage> c:<count> s:<storage> [<t1>-<t2>]
  */
int TmnList(            /* return number of printed records */
  FILE *prn )           /* print file                       */
  {
  int n;
  TMNRECPTR prec;
  if (!NameServer)  return -1;
  n = 0;
  for (prec=TmnHead; prec!=NULL; prec=prec->next) {
    n++;
    if (prn != NULL)  PrnRec( prn, n, prec );                     //-2006-02-15
    }
  return n;
  }

  /*================================================================== TmnLock
  Lock tables in the workspace. If the table does not exist, a chain
  entry is created.
  */
long TmnLock(           /* number of created records            */
  long id, ... )        /* id's of the tables, ends with 0      */
  {
  dP(TmnLock);
  va_list argptr;
  TMNRECPTR pr;
  char name[256];
  int n;
  if (!NameServer)  return -1;
  va_start( argptr, id );
  n = 0;
  while (id) {
    strcpy( name, NameServer(id) );
    pr = FindRec(id);
    if (!pr) {
      pr = AddRec(id);  if (!pr)                eX(1);
      n++; }
    pr->usage |= TMN_LOCKED;
    if (TmnLogAction) fprintf(TmnLog, "TmnLock(%s)\n", name);
    id = va_arg(argptr, long);
    }
  va_end(argptr);
  return n;
eX_1:
  eMSG(_$_not_locked_, name);
  }

  /*================================================================== TmnMake
  Make an array by calling the user supplied creator or by a call
  to system() with the user supplied text. In addition the name of
  the working directory and the options -N<hexid> and -T<time> are
  supplied.
  */
static TMNRECPTR TmnMake(       /* pointer to the table record  */
  long id,                      /* identification               */
  long *pt1,                    /* start of requested time      */
  long *pt2 )                   /* end of requested time        */
  {
  dP(TmnMake);
  TMNRECPTR pr, ps;
  TMNDATSRV srv;
  char name[256], t1s[40], t2s[40], idhex[12], sname[256], argstr[256], *pc;
  if (!NameServer)  return NULL;
  strcpy( name, NameServer(id) );
  pr = FindRec(id);
  strcpy(t1s, TmString(pt1));
  strcpy(t2s, TmString(pt2));
  if (TmnLogAction) {
    int usage;
    usage = (pr) ? pr->usage : 0;
    fprintf(TmnLog, "TmnMake(%s [%s,%s], %08x)\n", name, t1s, t2s, usage);
    }
  sprintf(idhex, "%08lx", id);
  if (!pr)  pr = AddRec(id);
  if (!pr)                                              eX(1);
  ps = NULL;
  if (!pr->datserver)  ps = FindDatServer(id);
  if (!ps)  ps = pr;
  strcpy(sname, ps->servname);
  if (ps->datserver) {
    pr->usage &= ~TMN_UPDATE;
    srv = ps->datserver;
    if (0 > (*srv)(TmnWorkDir))                         eX(2);
    if (*sname) {
      pc = strtok(sname, " \t");
      if (pc)
        while ((pc=strtok(NULL," \t"))) {
          if (0 > ((*srv)(pc)))                         eX(4);
          }
      }
    if ((ps->usage & TMN_WAIT) && (0 > (*srv)("-W")))   eX(3);
    sprintf(argstr, "-N%s", idhex);
    if (0 > (*srv)(argstr))                             eX(3);
    if (pt1) {
      sprintf(argstr, "-T%s", TmString(pt1));
      if (0 > (*srv)(argstr))                           eX(5);
      }
    if (0 > (*srv)(""))                                 eX(6);
    if ((pr->usage & TMN_DEFINED) == 0)  return NULL;
    return pr;
    }
  if (*sname>' ' && id>0) {
    sprintf(argstr, "%s%s %s -N%s -T", TmnWorkDir, sname, TmnWorkDir, idhex);
    if (pt1)  strcat(argstr, TmString(pt1));
    if (TmnLogAction)
      fprintf(TmnLog, "TmnMake: system(%s)\n", argstr);
    system(argstr);
    pr = TmnGet(id, pt1, pt2);                          eG(8);
    return pr;
    }
  return NULL;                                         
eX_1:
  nMSG(_cant_register_$_, name);
  return NULL;
eX_2:  eX_3:  eX_4:  eX_5:  eX_6:
  nMSG(_error_server_$_, name);
  return NULL;
eX_8:
  nMSG(_system_$_failed_, argstr);
  return NULL;
  }

  /*=============================================================== TmnMemory
  Count the memory used by all records
  */
long TmnMemory(            // return number of used bytes     //-2006-02-09
  FILE *prn )
  {
  int n;
  long bytes, nh, nt, na;
  char name[256], *psep;
  TMNRECPTR p;
  if (!NameServer)  return -1;
  n = 0;
  bytes = 0;
  for (p=TmnHead; (p); p=p->next) {
    n++;
    nh = (p->header.s) ? strlen(p->header.s)+1 : 0;
    nt = (p->tag)      ? strlen(p->tag)+1 : 0;
    na = (p->dsc.start)? p->dsc.ttlsz : 0;
    if (prn) {
      strcpy(name, NameServer(p->id));
      psep = strrchr(name, MSG_PATHSEP);
      if (psep)  psep++;
      else  psep = name;
      if (!strchr(psep, '.'))  strcat(psep, "    ");
      fprintf(prn, "%3d: %14s %6ld %6ld %8ld\n", n, psep, nh, nt, na);
    }
    bytes += sizeof(TMNREC) + nh + nt + na;
    }
  return bytes;
  }

  /*=============================================================== TmnParticles
  Count the number of particles
  */
long TmnParticles(            // return number of particles     //-2006-02-09
  void )
  {
  long count;
  TMNRECPTR p;
  ARYDSC dsc;
  int dt;
  count = 0;
  for (p=TmnHead; (p); p=p->next) {
    dt = XTR_DTYPE(p->id);
    if (dt!=PTLarr && dt!=PTLtab && dt!=XTRarr)  continue;
    dsc = p->dsc;
    if (dsc.start == NULL)  continue;
    if (dsc.numdm != 1)     continue;
    count += dsc.bound[0].hgh - dsc.bound[0].low + 1;
  }
  return count;
}

  /*============================================================ TmnNameServer
  Internal name server, if none is supplied by the user.
  */
static char *TmnNameServer(     /* file name            */
  long id )                     /* identification       */
  {
  static char s[40];
  sprintf(s, "%08lx.dmna", id);                                  //-2011-06-29
  return s;
  }

  /*================================================================== TmnRead
  Try to read a table from disk. Time series of arrays are scanned. If
  the wait flag is set for the table, the routine tries infinitely
  every 2 seconds to read the table.
  */
ARYDSC *TmnRead(        /* pointer to array                     */
  char *iname,          /* file name (may be nonstandard)       */
  long *pid,            /* table id, set by TMN                 */
  long *pt1,            /* start of the requested time          */
  long *pt2,            /* end of time, is set if *pt1==*pt2    */
  int mode,             /* 0 or TMN_MODIFY or TMN_LOCK          */
  TXTSTR *hdr )         /* copy of the header                   */
  {
  dP(TmnRead);
  ARYDSC *pa;
  TMNRECPTR pr;
  long id;
  if (!NameServer)  return NULL;
  if (TmnLogAction) {
    char t1s[40], t2s[40];
    strcpy(t1s, TmString(pt1));
    strcpy(t2s, TmString(pt2));
    fprintf(TmnLog, "TmnRead(%s, %s, %s)\n", iname, t1s, t2s);
    }
  if (strlen(iname) >= TMN_SNMLEN)                              eX(1);
  id = ++IdCounter;
  pr = AddRec(id);  if (!pr)                                    eX(2);
  strcpy(pr->servname, iname);
  pa = TmnAttach(id, pt1, pt2, mode, hdr);                      eG(3);
  if (pa)  *pid = id;                                           //-2000-02-26
  else  DelRec(FindRec(id));                                    //-2000-02-26
  return pa;
eX_1:
  nMSG(_name_too_long_);
  return NULL;
eX_2:
  nMSG(_cant_register_$_, iname);
  return NULL;
eX_3:
  nMSG(_cant_read_$_, iname);
  return NULL;
  }

  /*================================================================ TmnRename
  Change the ID.
  */
long TmnRename(         /* returns 1, if table exist            */
  long oldid,           /* old identification                   */
  long newid )          /* new identification                   */
  {
  dP(TmnRename);
  TMNRECPTR prnew, prold;
  char oldname[256], newname[256];
  long rc;
  if (!NameServer)  return -1;
  strcpy(oldname, NameServer(oldid));
  strcpy(newname, NameServer(newid));
  if (TmnLogAction) fprintf(TmnLog, "TmnRename(%s, %s)\n", oldname, newname);
  rc = 0;
  prnew = FindRec(newid);
  if (prnew) {
    if (prnew->usage & TMN_DEFINED)             eX(1);
    rc = 1;
    }
  else {
    prnew = AddRec(newid);                      eG(2);
    }
  prold = FindRec(oldid);
  if (!prold)                                   eX(3);
  if (prold->count)                             eX(4);
  prnew->mask  = prold->mask;
  prnew->usage = prold->usage;
  prnew->t1 = prold->t1;
  prnew->t2 = prold->t2;
  prnew->storage = TMN_MYWRKSPC;
  prold->storage &= ~TMN_MYWRKSPC;
  if (prold->storage == 0) {
    prold->usage &= ~TMN_DEFINED;
    prold->t1 = 0;
    prold->t2 = 0;
    }
  TxtCpy(&(prnew->header), prold->header.s);                        //-2011-06-29
  TxtClr(&(prold->header));
  prnew->lpos1 = 0;
  prnew->lpos2 = 0;
  prnew->lt1 = 0;
  prnew->lt2 = 0;
  prnew->dsc = prold->dsc;
  prold->dsc.start = NULL;
  return rc;
eX_1:
  eMSG(_duplicate_name_$_, newname);
eX_2:
  eMSG(_cant_create_entry_$_, newname);
eX_3:
  eMSG(_$_not_found_, oldname);
eX_4:
  eMSG(_$_used_, oldname);
  }

  /*================================================================ TmnResize
  Change the size of the data array.
  */
long TmnResize(         /* returns the old upper bound          */
  long id,              /* identification                       */
  int newi2 )           /* new upper bound                      */
  {
  dP(TmnResize);
  TMNRECPTR pr;
  char name[256];
  ARYDSC *pa;
  void *pold, *pnew;
  int i1, i2, oldsize, newsize, cmpsize;
  if (!NameServer)  return -1;
  strcpy(name, NameServer(id));
  if (TmnLogAction) fprintf(TmnLog, "TmnResize(%s, %d)\n", name, newi2);
  pr = FindRec(id);  if (!pr)                           eX(1);
  if (!(pr->usage & TMN_MODIFY))                        eX(5);
  if (pr->usage & TMN_DISK)                             eX(2);
  pa = &(pr->dsc);
  pold = pa->start;
  if (pa->numdm < 1)                                    eX(3);
  i1 = pa->bound[0].low;
  i2 = pa->bound[0].hgh;
  if (i2 == newi2)  return i2;
  oldsize = pa->ttlsz;
  newsize = (newi2 - i1 + 1)*pa->bound[0].mul;
  pnew = ALLOC(newsize);  if (!pnew)                  eX(4);
  cmpsize = MIN(newsize, oldsize);
  memcpy(pnew, pold, cmpsize);
  if (newsize > oldsize)
    memset((char*)pnew+oldsize, 0, newsize-oldsize);
  FREE(pold);
  pa->start = pnew;
  pa->ttlsz = newsize;
  pa->bound[0].hgh = newi2;
  return i2;
eX_1:
  eMSG(_$_not_found_, name);
eX_2:
  eMSG(_resize_$_unique_only_, name);
eX_3:
  eMSG(_invalid_structure_$_, name);
eX_4:
  eMSG(_cant_realloc_$_$_, name, newsize);
eX_5:
  eMSG(_$_not_attached_, name);
  }

  /*=============================================================== TmnScratch
  Delete entries of tables that are "not defined".
  */
long TmnScratch(        /* number of deleted entries    */
  long id, ... )        /* id's of tables, ends with 0  */
  {
  dP(TmnScratch);
  va_list argptr;
  TMNRECPTR pr;
  char name[256];
  int n;
  if (!NameServer)  return -1;
  va_start(argptr, id);
  n = 0;
  while (id) {
    pr = FindRec(id);
    if (pr) {
      strcpy(name, NameServer(id));
      if (pr->count > 0)                                eX(1);
      if (pr->usage & TMN_DEFINED)                      eX(2);
      DelRec(pr);
      n++;
      if (TmnLogAction) fprintf(TmnLog, "TmnScratch(%s)\n", name);
      }
    id = va_arg( argptr, long );
    }
  va_end(argptr);
  return n;
eX_1:
  eMSG(_not_scatched_$_, name);
eX_2:
  eMSG(_not_scatched_$_, name);
  }

  /*============================================================ TmnScratchAll
  Delete entries of tables that are "not defined": all entries
  are deleted for which (id & mask) == id_parameter (cf. TmnScratch).
  */
long TmnScratchAll(     /* number of deleted entries    */
  long mask,            /* mask for name id             */
  long id, ... )        /* id's of tables, ends with 0  */
  {
  dP(TmnScratchAll);
  va_list argptr;
  TMNRECPTR pr, pn;
  char name[256];
  int n;
  if (!NameServer)  return -1;
  va_start(argptr, id);
  n = 0;
  while (id) {
    if (TmnLogAction) fprintf(TmnLog, "TmnScratchAll(%08lx)\n", id);
    id &= mask;
    for (pr=TmnHead; pr!=NULL; pr=pn) {
      pn = pr->next;
      if ((pr->id & mask) == id) {
        n += TmnScratch(pr->id, TMN_NOID);                     eG(1);
      }
    }
    id = va_arg(argptr, long);
  }
  va_end(argptr);
  return  n;
eX_1:
  strcpy(name, NameServer(pr->id));
  eMSG(_cant_scratch_$_, name);
  }

  /*============================================================== TmnSetTimes
  Set validity times in the header.
  */
long TmnSetTimes(       /* returns 1, if times are set  */
  TXTSTR *phdr,         /* pointer to the header        */
  long *pt1,            /* start of validity time       */
  long *pt2 )           /* end of validity time         */
  {
  dP(TmnSetTimes);
  char t1s[40], t2s[40];
  if (!phdr)  return 0;
  strcpy( t1s, TmString(pt1) );
  strcpy( t2s, TmString(pt2) );
  DmnRplValues(phdr, "T1", t1s);                                eG(1);
  DmnRplValues(phdr, "T2", t2s);                                eG(2);
  return 1;
eX_1:  eX_2:
  eMSG(_cant_change_time_);
  }

  /*================================================================= TmnStore
  Save tables on disk, use the same name, create the file.
  */
long TmnStore(          /* number of saved tables       */
  long id, ... )        /* identifications, ends with 0 */
  {
  dP(TmnStore);
  char iname[256];
  va_list argptr;
  int n;
  long rc;
  if (!NameServer)  return -1;
  va_start( argptr, id );
  n = 0;
  while (id) {
    strcpy(iname, NameServer(id));
    AddXtn(iname);
    rc = TmnArchive(id, iname, 0);  if (rc < 0)                   eX(1);
    n++;
    id = va_arg( argptr, long );
    }
  va_end( argptr );
  return n;
eX_1:
  eMSG(_not_stored_);
  }

  /*============================================================== TmnStoreAll
  Save tables on disk, use the same name, create the file.
  */
long TmnStoreAll(       /* number of saved tables       */
  long mask,            /* selection mask               */
  long id, ... )        /* identifications, ends with 0 */
  {
  dP(TmnStoreAll);
  va_list argptr;
  char iname[256];
  int n, cm;
  TMNRECPTR pr;
  long rc, rid;
  if (!NameServer)  return -1;
  va_start(argptr, id);
  n = 0;
  while (id) {
    id &= mask;
    cm = id & NMS_CMMND;                                          //-2011-06-29
    for (pr=TmnHead; pr!=NULL; pr=pr->next) {
      if ((pr->id & mask) == (id & ~NMS_CMMND)) {                 //-2011-06-29
        if (!(pr->usage & TMN_DEFINED))  continue;
        if (!(pr->usage & TMN_MODIFIED)) continue;
        rid = pr->id | cm;                                        //-2011-06-29
        strcpy(iname, NameServer(rid));                           //-2011-06-29
        AddXtn(iname);
        rc = TmnArchive(rid, iname, 0);  if (rc < 0)    eX(1);    //-2011-06-29
        n++;
        }
      }
    id = va_arg(argptr, long);
    }
  va_end(argptr);
  return n;
eX_1:
  eMSG(_$_not_stored_, NameServer(id));
  }

  /*=================================================================== TmnTag
  Sets and returns the tag of the selected table.
  */
char *TmnTag(           /* returns the tag or NULL  */
  long id,              /* identification           */
  char *pc )            /* pointer to tag           */
  {
  dQ(TmnTag);
  TMNRECPTR pr;
  char *pp;
  pr = FindRec(id);
  if (!pr)  return NULL;
  pp = pr->tag;
  if (pc) {
    pp = pr->tag;
    if (pp)  FREE(pp);
    pr->tag = NULL;
    pp = ALLOC(strlen(pc)+1);
    if (!pp)  return NULL;
    strcpy(pp, pc);
    pr->tag = pp;
    }
  return pp;
  }

  /*========================================================== TmnTaggedDelete
  Delete tables with specific tags.
  */
int TmnTaggedDelete(    /* returns number of deleted tables */
  char *field,          /* key field within tag             */
  char *selection,      /* kind of selection                */
  long mask,            /* selection mask                   */
  long id, ... )        /* table identification             */
  {
  dP(TmnTaggedDelete);
  TMNRECPTR pr;
  va_list argptr;
  char *pp, name[256];
  int n;
  if (!NameServer)  return -1;
  va_start(argptr, id);
  n = 0;
  while (id) {
    id &= mask;
    for (pr=TmnHead; pr!=NULL; pr=pr->next) {
      if ((pr->id & mask) != id)  continue;
      pp = pr->tag;
      if (!pp)  continue;
      if      (!strcmp(selection, "eq")) {
        if (!strstr(pp, field))  continue;
        }
      else if (!strcmp(selection, "ne")) {
        if (strstr(pp, field))  continue;
        }
      else  continue;
      strcpy(name, NameServer(id));
      if ((id > 0) && (pr->count > 0))                  eX(1);
      if (pr->usage & TMN_FIXED)                        eX(4);
      TxtClr(&(pr->header));                                        //-2011-06-29
      if (pr->tag)        FREE(pr->tag);
      pr->tag = NULL;
      if (pr->dsc.start)  FREE(pr->dsc.start);
      pr->dsc.start = NULL;
      pr->storage &= ~TMN_MYWRKSPC;
      pr->storage &= ~TMN_DISK;
      if (pr->storage)                                  eX(3);
      pr->usage = 0;
      pr->count = 0;
      pr->lpos1 = 0;
      pr->lpos2 = 0;
      pr->lt1 = 0;
      pr->lt2 = 0;
      pr->t1 = 0;
      pr->t2 = 0;
      if (id < 0)  DelRec(pr);
      n++;
      if (TmnLogAction) fprintf(TmnLog, "TmnTaggedDelete(%s)\n", name);
      }
    id = va_arg(argptr, long);
    }
  va_end(argptr);
  return n;
eX_1:
  eMSG(_$_not_deleted_, name);
eX_3:
  eMSG(_how_delete_$_, name);
eX_4:
  eMSG(_cant_delete_fixed_$_, name);
  }

  /*================================================================ TmnUnLock
  Unlock tables which are locked in the workspace. Id's of non
  existent tables are ignored.
  */
int TmnUnLock(          /* number of unlocked tables            */
  long id, ... )        /* id's of the tables, ends with 0      */
  {
  va_list argptr;
  TMNRECPTR pr;
  int n;
  char name[256];
  if (!NameServer)  return -1;
  va_start( argptr, id );
  n = 0;
  while (id) {
    strcpy( name, NameServer(id) );
    pr = FindRec(id);
    if (pr) {
      pr->usage &= ~TMN_LOCKED;
      n++;
      if (TmnLogAction) fprintf(TmnLog, "TmnUnLock(%s)\n", name);
      }
    id = va_arg( argptr, long );
    }
  va_end( argptr );
  return n;
  }

  /*=============================================================== TmnVerify
  Verify, that the data identified by id are available. After
  successfull return the data are locked.
  */
int TmnVerify(          /* returns 0 if success                 */
  long id,              /* identification                       */
  long *pt1,            /* start of the requested time interval */
  long *pt2,            /* end of time, is set if *pt1==*pt2    */
  int mode )            /* = TMN_LOCKED                         */
  {
  dP(TmnVerify);
  ARYDSC *pa;
  char name[256];
  strcpy(name, NameServer(id));
  if (TmnLogAction) fprintf(TmnLog, "TmnVerify(%s)\n", name);
  pa = TmnAttach(id, pt1, pt2, mode, NULL);                     eG(1);
  if (!pa)                                                      eX(2);
  TmnDetach(id, pt1, pt2, mode, NULL);                          eG(3);
  return 0;
eX_1:
  eMSG(_cant_attach_$_, name);
eX_2:
  eMSG(_$_not_available_, name);
eX_3:
  eMSG(_cant_detach_$_, name);
  }

//===========================================================================
//
// history:
//
// 2002-09-24 lj 1.0.0  final release candidate
// 2002-12-19 lj 1.0.4  reference to sleep() removed
// 2005-03-17 uj 2.2.0  version number upgrade
// 2006-02-09 lj 2.2.9  TmnList, TmnMemory, TmnParticles
// 2006-02-15 lj        TmnList
// 2006-10-26 lj 2.3.0  external strings
// 2011-06-29 uj 2.5.0  dmn output
//                      TmnAttach(), TmnDetach(), TmnRead with TXTSTR
//                      TmnChangeValue eliminated
// 2012-04-06 uj 2.6.0  version number upgrade
// 2012-09-04 uj 2.6.1  gencnv.h excluded (not used)
//
//===========================================================================

