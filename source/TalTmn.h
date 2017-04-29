//==================================================================== TalTmn.h
//
//  Table Manager, uses array's from the gen-lib.
//  =============================================
//
// Copyright (C) Umweltbundesamt, Dessau-Roﬂlau, Germany, 2002
// Copyright (C) Janicke Consulting, 88662 ‹berlingen, Germany, 2002-2011
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
// last change: 2011-06-29 uj
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

#ifndef  TALTMN_INCLUDE
#define  TALTMN_INCLUDE

#ifndef  IBJARY_INCLUDE
  #include "IBJary.h"
#endif

#ifndef  IBJTXT_INCLUDE
  #include  "IBJtxt.h"
#endif

#define  TMN_SNMLEN  256

#define  TMN_UNIQUE   (long) 0x0004
#define  TMN_FIXED    (long) 0x0008
#define  TMN_READ     (long) 0x0010
#define  TMN_WAIT     (long) 0x0400
#define  TMN_INVALID  (long) 0x8000
#define  TMN_USRFLAGS (long) 0xffff
#define  TMN_DEFINED  (long) 0x00010000
#define  TMN_CREATED  (long) 0x00020000
#define  TMN_UPDATE   (long) 0x00040000
#define  TMN_LOCKED   (long) 0x00100000
#define  TMN_MODIFY   (long) 0x00200000
#define  TMN_MODIFIED (long) 0x00400000
#define  TMN_SETVALID (long) 0x01000000
#define  TMN_SETALL   (long) 0x08000000

#define  TMN_DISPLAY   (long) 0x0001
#define  TMN_LOGALLOC  (long) 0x0002
#define  TMN_LOGACTION (long) 0x0004
#define  TMN_DONTZERO  (long) 0x0008

#define  TMN_APPEND   0x0001
#define  TMN_SETUNDEF 0x0002
#define  TMN_NOID     0L

typedef long  (*TMNDATSRV) (char*);
typedef char* (*TMNNAMSRV) (long);
typedef char* (*TMNHDRSRV) (long, long*, long*);

/*====================== table manager functions ==========================*/

  /*============================================================== TmnArchive
  Store a table on disk. The name (id) may be changed and the data
  may be appended to an existing file. If the name is not changed,
  the data are registered as "saved on disk" (flag TMN_DISK).
  */
long TmnArchive(        /* returns 0 or error code      */
  long id,              /* old identification           */
  char *name,           /* new name                     */
  int options )         /* append or setundef           */
  ;
  /*=============================================================== TmnAttach
  Get the array and the header of a table. If no data are defined,
  get and make are tried. A lock is removed int the first attach,
  but may be requested in mode.
  */
ARYDSC *TmnAttach(      /* pointer the array                    */
  long id,              /* identification                       */
  long *pt1,            /* start of the requested time interval */
  long *pt2,            /* end of time, is set if *pt1==*pt2    */
  int mode,             /* 0 or TMN_MODIFY or TMN_LOCK          */
  TXTSTR *hdr )         /* copy of the header                   */
  ;
  /*================================================================ TmnClear
  Clear tables from the workspace, that are not valid beyond
  time t. Tables valid only at time t are not cleared. Time values
  and information on disk storage are preserved, but the tables
  are regarded as "not defined".
  */
long TmnClear(          /* number of cleared tables     */
  long t,               /* validity time threshold      */
  long id, ... )        /* id's of tables, ends with 0  */
  ;
  /*============================================================== TmnClearAll
  Clear tables from the workspace using a name mask: all tables
  are cleared for which (id & mask) == id_parameter (cf. TmnClear).
  */
long TmnClearAll(       /* number of cleared tables     */
  long t,               /* validity time threshold      */
  long mask,            /* mask for name id             */
  long id, ... )        /* id's of tables, ends with 0  */
  ;
  /*=============================================================== TmnCreate
  Create a table with identification id.
  */
ARYDSC *TmnCreate(      /* pointer to the array of the table    */
  long id,              /* identification of the table          */
  int elmsz,            /* element size in the array            */
  ... )                 /* <dims>, <i1>, <i2>, <j1>, <j2> ...   */
  ;
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
  ;
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
  ;
  /*============================================================= TmnDeleteAll
  Delete tables from the workspace using a name mask: all tables
  are deleted for which (id & mask) == id_parameter (cf. TmnDelete).
  */
long TmnDeleteAll(      /* number of deleted tables     */
  long t,               /* validity time threshold      */
  long mask,            /* mask for name id             */
  long id, ... )        /* id's of tables, ends with 0  */
  ;
  /*================================================================ TmnDetach
  Release an attached table. If the modify flag is set in mode, the
  times and the header are updated. A lock may be given in mode.
  */
long TmnDetach(         /* returns 0 or error code              */
  long id,              /* identification                       */
  long *pt1,            /* start of validity time interval      */
  long *pt2,            /* end of validity time interval        */
  long mode,            /* TMN_MODIFY, if attached in this mode */
  TXTSTR *hdr )         /* copy of the header                   */
  ;
  /*================================================================ TmnFinish
  */
int TmnFinish(          /* delete all records and data */
  void )
  ;
  /*================================================================ TmnIdent
  Returns a new Id for private data.
  */
long TmnIdent(          /* returns new Id                       */
  void )
  ;
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
  ;
  /*================================================================ TmnInit
  Initialisation of the table manager.
  */
int TmnInit(                    /* returns always 0                     */
  char *wdir,                   /* working directory                    */
  TMNNAMSRV nameserver,         /* user supplied name server            */
  TMNHDRSRV headerserver,       /* user supplied header server          */
  long m,                       /* 0 or TMN_LOGALLOC or TMN_LOGACTION   */
  FILE *f )                     /* log file                             */
  ;
  /*================================================================== TmnList
  Print a list of all records in the table chain, format:
  <line number>: <name> u:<usage> c:<count> s:<storage> [<t1>-<t2>]
  */
int TmnList(            /* return number of printed records */
  FILE *prn )           /* print file                       */
  ;
  /*================================================================== TmnLock
  Lock tables in the workspace. If the table does not exist, a chain
  entry is created.
  */
long TmnLock(           /* number of created records            */
  long id, ... )        /* id's of the tables, ends with 0      */
  ;
  /*=============================================================== TmnMemory
  Count the memory used by all records
  */
long TmnMemory(            /* return number of used bytes */
  FILE *prn )
  ;
long TmnParticles(            // return number of particles
  void )
  ;
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
  ;
  /*================================================================ TmnRename
  Change the ID.
  */
long TmnRename(         /* returns 1, if table exist            */
  long oldid,           /* old identification                   */
  long newid )          /* new identification                   */
  ;
  /*================================================================ TmnResize
  Change the size of the data array.
  */
long TmnResize(         /* returns the old upper bound          */
  long id,              /* identification                       */
  int newi2 )           /* new upper bound                      */
  ;
  /*=============================================================== TmnScratch
  Delete entries of tables that are "not defined".
  */
long TmnScratch(        /* number of deleted entries    */
  long id, ... )        /* id's of tables, ends with 0  */
  ;
  /*============================================================ TmnScratchAll
  Delete entries of tables that are "not defined": all entries
  are deleted for which (id & mask) == id_parameter (cf. TmnScratch).
  */
long TmnScratchAll(     /* number of deleted entries    */
  long mask,            /* mask for name id             */
  long id, ... )        /* id's of tables, ends with 0  */
  ;
  /*============================================================== TmnSetTimes
  Set validity times in the header.
  */
long TmnSetTimes(       /* returns 1, if times are set  */
  TXTSTR *phdr,         /* pointer to the header        */
  long *pt1,            /* start of validity time       */
  long *pt2 )           /* end of validity time         */
  ;
  /*================================================================= TmnStore
  Save tables on disk, use the same name, create the file.
  */
long TmnStore(          /* number of saved tables       */
  long id, ... )        /* identifications, ends with 0 */
  ;
  /*============================================================== TmnStoreAll
  Save tables on disk, use the same name, create the file.
  */
long TmnStoreAll(       /* number of saved tables       */
  long mask,            /* selection mask               */
  long id, ... )        /* identifications, ends with 0 */
  ;
  /*=================================================================== TmnTag
  Sets and returns the tag of the selected table.
  */
char *TmnTag(           /* returns the tag or NULL  */
  long id,              /* identification           */
  char *pc )            /* pointer to tag           */
  ;
  /*========================================================== TmnTaggedDelete
  Delete tables with specific tags.
  */
int TmnTaggedDelete(    /* returns number of deleted tables */
  char *field,          /* key field within tag             */
  char *selection,      /* kind of selection                */
  long mask,            /* selection mask                   */
  long id, ... )        /* table identification             */
  ;
  /*================================================================ TmnUnLock
  Unlock tables which are locked in the workspace. Id's of non
  existent tables are ignored.
  */
int TmnUnLock(          /* number of unlocked tables            */
  long id, ... )        /* id's of the tables, ends with 0      */
  ;
  /*=============================================================== TmnVerify
  Verify, that the data identified by id are available. After
  successful return the data are locked.
  */
int TmnVerify(          /* returns 0 if success                 */
  long id,              /* identification                       */
  long *pt1,            /* start of the requested time interval */
  long *pt2,            /* end of time, is set if *pt1==*pt2    */
  int mode )            /* = TMN_LOCKED                         */
  ;
/*==========================================================================*/
#endif
