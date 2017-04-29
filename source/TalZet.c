// ======================================================== TalZet.c
//
// Calculate time series for AUSTAL2000
// ====================================
//
// Copyright (C) Umweltbundesamt, Dessau-Roﬂlau, Germany, 2002-2007
// Copyright (C) Janicke Consulting, 88662 ‹berlingen, Germany, 2002-2014
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
// last change:  2014-06-26 uj
//
// ==================================================================

#include <math.h>

#define  STDMYMAIN  ZetMain
#include "IBJmsg.h"
#include "IBJary.h"
#include "IBJtxt.h"
#include "IBJdmn.h"
#include "IBJstd.h"
static char *eMODn = "TalZet";

//=========================================================================

STDPGM(lopzet, ZetServer, 2, 6, 11);

//=========================================================================

#include "genutl.h"
#include "genio.h"

#include "TalGrd.h"
#include "TalTmn.h"
#include "TalNms.h"
#include "TalZet.h"
#include "TalZet.nls"

#define  ZET_CONVAL    0x00000001L
#define  ZET_CONSUM    0x00000002L
#define  ZET_CONANY    0x00000003L
#define  ZET_DRYVAL    0x00000004L
#define  ZET_DRYSUM    0x00000008L
#define  ZET_DRYANY    0x0000000cL
#define  ZET_DOSANY    0x000006ffL

#define  ZET_ONESTEP   0x00001000

#define  NUMREC  2000
#define  BUFLEN  4000

#define  MAXSUMCMP  60
#define  MAXMON    100
#define  BUFLEN   4000

/**************************************************************************/

static char KompNamen[MAXSUMCMP][40], *ListNamen[MAXSUMCMP];
static char KompEinheiten[MAXSUMCMP][40], *ListEinheiten[MAXSUMCMP];
static int KompNum;
static int NumItem, NumJtem, MonRecLen;
static long Ta, Te, Sequence, Imon, Idos, Idat;
static char DefName[120] = "monitor.def";
static char AltName[120] = "param.def";
static char ZtrName[120] = "monitor.ztr";
static char Fname[120];
static char Kennung[256], DosName[120];                           //-2006-08-22
static FILE *ZtrFile;

static ARYDSC *Pmon, *Pdos, *Pdat, *Pvol;
static char Nform[40], ComName[MAXSUMCMP][40], *NameList[MAXSUMCMP];
static char Cform[40], ConUnit[MAXSUMCMP][40], *ConList[MAXSUMCMP];
static char Dform[40], DryUnit[MAXSUMCMP][40], *DryList[MAXSUMCMP];
static char StdUnit[40], StdFormat[40];

int NumMon;

static int NumCom, ComInd[MAXSUMCMP];
static char Buf[BUFLEN+4];
static char Kform[40];
static float Faktor;

static float ConFac[MAXSUMCMP];
static float DryFac[MAXSUMCMP];
static char TagChar;

  /*=============================================================== ReadMonDef
  */
#define  GET(a,b,c,d)  GetData((a),(b),(c),(d)); strcpy(nn,(a)); eG(99)
#define  GETF(a,d,e)  {strcpy(nn,(a));if(0>(rc=DmnGetFloat(hdr,(a),"%f",(d),(e)))) eX(111);}
#define  GETI(a,d,e)  {strcpy(nn,(a));if(0>(rc=DmnGetInt(hdr,(a),"%d",(d),(e)))) eX(111);}
#define  GETS(a,d,e)  {strcpy(nn,(a));rc=DmnCpyString(hdr,(a),(d),(e));}
#define  GETL(a,d,e)  {strcpy(nn,(a));rc=DmnGetString(hdr,(a),(d),(e));}

static long ReadMonDef(      /* Definition der Monitor-Punkte einlesen.   */
  void )
  {
  dP(ReadMonDef);
  int i;
  long rc;
  MONREC *pm;
  char fname[120], nn[80], sform[40], fform[40];
  strcpy(fname, DefName);
  if (0 > OpenInput(fname, AltName))                                    eX(1);
  if (0 > GetLine('.', Buf, BUFLEN))                                    eX(2);
  for (i=0; i<MAXSUMCMP; i++) {
    NameList[i] = ComName[i];
    ConList[i] = ConUnit[i];
    DryList[i] = DryUnit[i];
    ListNamen[i] = KompNamen[i];
    ListEinheiten[i] = KompEinheiten[i];
    }
  strcpy( StdUnit, "?" );
  sprintf(sform, "%%[%d]s", MAXSUMCMP);
  sprintf(fform, "%%[%d]f", MAXSUMCMP);
  Faktor = 1.0;
  GET( "FAKTOR", Buf, "%f", &Faktor );
  strcpy( Kform, "%10.2f" );
  GET( "KFORMAT", Buf, "%s", Kform );
  strcpy( Nform, " %9.9s" );
  GET( "NFORMAT", Buf, "%s", Nform );
  strcpy( StdFormat, "%10.2e" );
  GET( "WFORMAT", Buf, "%s", StdFormat );
  strcpy( DosName, "c%02ld" );
  GET( "DOSNAME", Buf, "%s", DosName );
  rc = GET( "KOMPONENTEN|COMPONENTS", Buf, sform, NameList );           eG(3);
  vLOG(3)(_$_components_, rc);

  if (rc > 0)  NumCom = rc;
  else  NumCom = MAXSUMCMP;
  NumItem = 0;
  if (StdStatus & ZET_CONANY | -1) {
    rc = GET( "ConFac", Buf, fform, ConFac );
    if ((rc > 0) && (rc != NumCom))                                     eX(4);
    if (rc == 0)  for (i=0; i<NumCom; i++)  ConFac[i] = Faktor;
    rc = GET( "ConDim", Buf, sform, ConList );
    if (rc == 0)  strcpy( ConList[0], StdUnit );
    if (rc < NumCom)
      for (i=1; i<NumCom; i++)  strcpy( ConList[i], ConList[0] );
    rc = GET( "ConFormat", Buf, "%s", Cform );
    if (rc == 0)  strcpy( Cform, StdFormat );
    if (StdStatus & ZET_CONVAL)  NumJtem += 1;
    if (StdStatus & ZET_CONSUM)  NumItem += 1;
    }
  if (StdStatus & (ZET_DRYANY)) {                  /*-04feb98-*/
    rc = GET( "DryFac", Buf, fform, DryFac );
    if ((rc > 0) && (rc != NumCom))                                     eX(5);
    if (rc == 0)  for (i=0; i<NumCom; i++)  DryFac[i] = Faktor;
    rc = GET( "DryDim", Buf, sform, DryList );
    if (rc == 0)  strcpy( DryList[0], StdUnit );
    if (rc < NumCom)
      for (i=1; i<NumCom; i++)  strcpy( DryList[i], DryList[0] );
    rc = GET( "DryFormat", Buf, "%s", Dform );
    if (rc == 0)  strcpy( Dform, StdFormat );
    if (StdStatus & ZET_DRYVAL)  NumJtem += 1;
    if (StdStatus & ZET_DRYSUM)  NumItem += 1;
    }
  MonRecLen = sizeof(MONREC);
  if (0 > GetLine('!', Buf, BUFLEN))                                    eX(9);
  rc = CntLines('.', Buf, BUFLEN);  if (rc < 0)                         eX(10);
  NumMon = strspn(Buf, "M");  if (NumMon < 1)                           eX(11);
  Pmon = TmnCreate(Imon, MonRecLen, 1, 1, NumMon);                      eG(12);
  for (i=1; i<=NumMon; i++) {
    pm = AryPtr(Pmon, i);  if (!pm)                                     eX(13);
    if (0 > GetLine('M', Buf, BUFLEN))                                  eX(14);
    rc = GetList(Buf, pm->np, 39, &(pm->xp), 3);  if (rc < 0)           eX(91);
    if (3 != rc)                                                        eX(15);
    }
  CloseInput();
  vLOG(3)(_$_points_, NumMon);
  TmnDetach(Imon, NULL, NULL, TMN_MODIFY, NULL);                        eG(16);
  Pmon = NULL;
  if (NumCom < MAXSUMCMP)  NumItem += NumCom*NumJtem;
  return 0;
eX_99:
  eMSG(_error_value_$_$_, nn, fname);
eX_1:
  eMSG(_cant_read_$_, fname);
eX_2:
  eMSG(_no_assignment_$_, fname);
eX_3:
  eMSG(_no_components_$_, fname);
eX_4:
  eMSG(_inconsistent_count_factors_cnc_);
eX_5:
  eMSG(_inconsistent_count_factors_dry_);
eX_9:
  eMSG(_missing_header_$_, fname);
eX_10: eX_11:
  eMSG(_no_points_$_, fname);
eX_12:
  eMSG(_cant_create_table_);
eX_13:
  eMSG(_index_$_, i);
eX_14:
  eMSG(_invalid_tag_$_$_, Buf[0], fname);
eX_15:
  vMsg(_input_$_, Buf);
  eMSG(_few_values_);
eX_16:
  eMSG(_cant_detach_$_, NmsName(Imon));
eX_91:
  eMSG(_cant_read_name_);
  }

/*================================================================ WriteMonZtr
*/
static long WriteMonZtr(   /* Monitor-Werte als Zeitreihe ausschreiben.   */
void )
  {
  dP(WriteMonZtr);
  char fname[80], t1s[20], t2s[20], knm[400], enm[400], leer[60], name[80];
  char dims[40];
  FILE *out;
  long pos;
  int i, j, l1, l2, ll, n, m, l;
  MONREC *pm;
  float *pf;
  Pdat = TmnAttach(Idat, NULL, NULL, 0, NULL);                  eG(30);
  if (ZtrFile)  out = ZtrFile;
  else {
    strcpy(fname, StdPath);
    strcpy(name, ZtrName);
    if (!strchr(name, '.'))  strcat(name, ".ztr");
    strcat(fname, name);
    out = fopen(fname, "a");  if (!out)                         eX(1);
    ZtrFile = out;
    }
  fseek(out, 0, SEEK_END);
  pos = ftell(out);
  if (pos == 0) {
    fprintf( out, "-  TALZET %d.%d.%s: Konzentrationswerte an vorgegebenen "
      "Monitor-Punkten.\n-\n", StdVersion, StdRelease, StdPatch);
    fprintf( out, ".  Kennung = \"%s\"\n", Kennung );
    strcpy( leer, "                                        " );
    strcpy( knm,  "   Komponenten   = {" );
    strcpy( enm,  "   Stoff-Einheit = {" );
    for (i=0; i<NumCom; i++) {
      j = ComInd[i];
      l1 = strlen( ComName[i] );
      l2 = strlen( KompEinheiten[j] );
      ll = 1 + MAX( l1, l2 );
      strncat( knm, leer, ll-l1 );   strcat( knm, ComName[i] );
      strncat( enm, leer, ll-l2 );   strcat( enm, KompEinheiten[j] );
      }
    strcat( knm, " }" );
    strcat( enm, " }" );
    fprintf( out, "%s\n", knm );
    fprintf( out, "%s\n-\n", enm );
    if (Faktor != 1.0)  fprintf( out, "   Faktor = %10.3e\n", Faktor );
    fprintf( out, "   X-Koordinaten =      {  " );
    for (i=1; i<=NumMon; i++) {
      pm = AryPtrX(Pmon, i);
      fprintf( out, Kform, pm->xp ); }
    fprintf( out, " }\n" );
    fprintf( out, "   Y-Koordinaten =      {  " );
    for (i=1; i<=NumMon; i++) {
      pm = AryPtrX(Pmon, i);
      fprintf( out, Kform, pm->yp ); }
    fprintf( out, " }\n" );
    fprintf( out, "   Z-Koordinaten =      {  " );
    for (i=1; i<=NumMon; i++) {
      pm = AryPtrX(Pmon, i);
      fprintf( out, Kform, pm->zp ); }
    fprintf( out, " }\n-\n" );
    fprintf( out, "!            T1            T2" );
    for (i=1; i<=NumMon; i++) {
      pm = AryPtrX(Pmon,i );
      fprintf( out, Nform, pm->np ); }
    fprintf( out, "\n" );
    }
  TimeStr( Ta, t1s );
  TimeStr( Te, t2s );
  n = 0;
  if (StdStatus & ZET_CONVAL) {
    for (m=0; m<NumCom; m++) {
      if (n == 0)  fprintf( out, "Z%14s%14s", t1s, t2s );
      else         fprintf( out, "%29s", " " );
      l = ComInd[m];
      for (i=1; i<=NumMon; i++) {
        pf = AryPtr(Pdat, i, n);  if (!pf)                         eX(10); //-2014-06-26
        fprintf( out, Cform, *pf ); }
      if (*ConList[m] == '?')  *dims = 0;
      else  sprintf(dims, "[%s]", ConList[m]);
      fprintf( out, " \' C.%s %s\n", ComName[m], dims );
      n++; }
    }
  if (StdStatus & ZET_CONSUM) {
    if (n == 0)  fprintf( out, "Z%14s%14s", t1s, t2s );
    else         fprintf( out, "%29s", " " );
    for (i=1; i<=NumMon; i++) {
      pf = AryPtr(Pdat, i, n);  if (!pf)                           eX(11); //-2014-06-26
      fprintf( out, Cform, *pf ); }
    if (*ConList[0] == '?')  *dims = 0;
    else  sprintf(dims, "[%s]", ConList[0]);
    fprintf( out, " \' C.Summe %s\n", dims );
    n++; }
  if (StdStatus & ZET_DRYVAL) {
    for (m=0; m<NumCom; m++) {
      if (n == 0)  fprintf( out, "Z%14s%14s", t1s, t2s );
      else         fprintf( out, "%29s", " " );
      l = ComInd[m];
      for (i=1; i<=NumMon; i++) {
        pf = AryPtr(Pdat, i, n);  if (!pf)                         eX(12); //-2014-06-26
        fprintf( out, Dform, *pf ); }
      if (*DryList[m] == '?')  *dims = 0;
      else  sprintf(dims, "[%s]", DryList[m]);
      fprintf( out, " \' D.%s %s\n", ComName[m], dims );
      n++; }
    }
  if (StdStatus & ZET_DRYSUM) {
    if (n == 0)  fprintf( out, "Z%14s%14s", t1s, t2s );
    else         fprintf( out, "%29s", " " );
    for (i=1; i<=NumMon; i++) {
      pf = AryPtr(Pdat, i, n);  if (!pf)                           eX(13); //-2014-06-26
      fprintf( out, Dform, *pf ); }
    if (*DryList[0] == '?')  *dims = 0;
    else  sprintf(dims, "[%s]", DryList[0]);
    fprintf( out, " \' D.Summe %s\n", dims );
    n++; }
  if (NumItem > 1)  fprintf( out, "-\n" );
  TmnDetach(Idat, NULL, NULL, 0, NULL);                            eG(2);
  Pdat = NULL;
  return 0;
eX_30:  eX_2:
  eMSG(_no_data_$_, NmsName(Idat));
eX_1:
  eMSG(_cant_open_$_, fname);
eX_10: eX_11: eX_12: eX_13:
  eMSG(_index_$$_, i, n);
  }

/*================================================================== ReadArray
*/
static ARYDSC *ReadArray(      /* Array einlesen */
long id,
long *pid )
  {
  dP(ReadArray);
  char t1s[40], t2s[40], *hdr;
  char nn[256], name[256];				//-2011-06-29
  long rc, t1, t2, usage;
  int i, j, mc;
  enum DATA_TYPE dt;
  ARYDSC *pa;
  TXTSTR header = { NULL, 0 };                                    //-2011-06-29
  char **_vec;
  dt = XTR_DTYPE(id);
  mc = MAXSUMCMP;
  _vec = ALLOC(mc*sizeof(char*));

  strcpy(name, NmsName(id));
  TmnInfo(id, &t1, &t2, &usage, NULL, NULL);
  if (usage & TMN_DEFINED) {
    pa = TmnAttach(id, NULL, NULL, 0, &header);                 eG(20);
    if (!pa)  return NULL;
    *pid = id;
    }
  else {
    pa = TmnRead(name, pid, NULL, NULL, TMN_READ, &header);     eG(4);
    if (!pa)  return NULL;
    }
  vLOG(3)("working with array %s", name);
  hdr = header.s;
  GETS("T1", t1s, 40);
  GETS("T2", t2s, 40);
  GETS("IDNT|IDENT|KENNUNG", Kennung, 256);
  Ta = TmValue(t1s);
  Te = TmValue(t2s);
  if (dt == DOStab) {
    GETL("NAME", _vec, mc);  if (rc <= 0)                       eX(1);
    for (i=0; i<rc; i++) {
      if (strlen(_vec[i]) >= 256)                          eX(12);
      strcpy(ListNamen[i], _vec[i]);
    }
    KompNum = rc;
    if (KompNum > MAXSUMCMP)                                    eX(10);
    if (NumCom >= MAXSUMCMP) {
      NumCom = KompNum;
      NumItem += NumCom*NumJtem;
      NumJtem = 0;
      for (i=0; i<NumCom; i++)  strcpy(ComName[i], KompNamen[i]);
      }
    GETL("UNIT|EINHEIT", _vec, mc);
    if (rc != KompNum)                                          eX(2);
    for (i=0; i<rc; i++) {
      if (strlen(_vec[i]) >= 256)                          eX(13);
      strcpy(ListEinheiten[i], _vec[i]);
    }
    for (i=0; i<NumCom; i++) {
      for (j=0; j<KompNum; j++)
        if (0 == CisCmp(KompNamen[j], ComName[i])) break;
      if (j >= KompNum)                                         eX(3);
      ComInd[i] = j;
    }
  }
  for (i=0; i<mc; i++) {
    if (_vec[i] != NULL)  FREE(_vec[i]);
    _vec[i] = NULL;
  }
  FREE(_vec);
  _vec = NULL;
  if (!Pdat) {
    TmnCreate(Idat, sizeof(float), 2, 1, NumMon, 0, NumItem-1);   eG(5);
    TmnDetach(Idat, &Ta, &Te, TMN_MODIFY, NULL);                  eG(6);
    Pdat = TmnAttach(Idat, NULL, NULL, 0, NULL);  if (!Pdat)      eX(7); //-2014-06-26
  }
  TxtClr(&header);
  return pa;
eX_20:  eX_4:
  nMSG(_cant_get_$_, NmsName(id));
  return NULL;
eX_1:
  nMSG(_name_missing_$_, NmsName(id));
  return NULL;
eX_2:
  nMSG(_unit_inconsistent_$_, NmsName(id));
  return NULL;
eX_3:
  nMSG(_no_component_$_$_, ComName[i], NmsName(id));
  return NULL;
eX_10:
  nMSG(_component_count_$_, MAXSUMCMP);
  return NULL;
eX_5:  eX_6:  eX_7:
  nMSG(_cant_create_$_, NmsName(Idat));
  return NULL;
eX_12: eX_13:
  nMSG(_name_too_long_$_, _vec[i]);
  return NULL;
  }

/*================================================================== InsertVal
*/
static long InsertVal(     /* Konzentrationswerte √ºbernehmen */
int im )               /* Pointer auf Monitor-Punkt      */
  {
  dP(InsertVal);
  float *pf, *pdry, *pcon;
  int i, j, k, l, m, n;
  float sumcon, sumdry;
  float vol, fl;
  long dt;
  MONREC *pm;
  pm = AryPtr(Pmon, im);  if (!pm)                                 eX(10);
  i = pm->i;
  j = pm->j;
  k = pm->k;
  pf = GrdParr->start;
  dt = Te - Ta;
  fl = GrdPprm->dd*GrdPprm->dd*dt;
  vol = (GrdPprm->zscl > 0) ? (*(float*)AryPtrX(Pvol,i,j,k))*dt
      : (pf[k] - pf[k-1])*fl;                 //-98-08-22
  pf = AryPtr(Pdat, im, 0);  if (!pf)                              eX(11);
  n = 0;
  sumcon = 0;
  sumdry = 0;
  if (StdStatus & ZET_CONANY) {   /* Konzentration */
    for (m=0; m<NumCom; m++) {
      l = ComInd[m];
      pcon = AryPtr(Pdos, i, j, k, l);  if (!pcon)                 eX(1);
      if (StdStatus & ZET_CONVAL)  pf[n++] = ConFac[m]*(*pcon)/vol;
      if (StdStatus & ZET_CONSUM)  sumcon += ConFac[m]*(*pcon)/vol;
      }
    if (StdStatus & ZET_CONSUM)  pf[n++] = sumcon;
    }
  if (StdStatus & ZET_DRYANY) {   /* Massenstromdichte trockene Deposition */
    for (m=0; m<NumCom; m++) {
      l = ComInd[m];
      k = 0;
      pdry = AryPtr(Pdos, i, j, k, l);  if (!pdry)                 eX(2);
      if (StdStatus & ZET_DRYVAL)  pf[n++] = DryFac[m]*(*pdry)/fl;
      if (StdStatus & ZET_DRYSUM)  sumdry += DryFac[m]*(*pdry)/fl;
      }
    if (StdStatus & ZET_DRYSUM)  pf[n++] = sumdry;
    }
  return 0;
eX_1:  eX_2: 
  eMSG(_index_$$$$_, i, j, k, l);
eX_10: eX_11:
  eMSG(_index_$_, im);
  }

  /*================================================================ ClcMonVal
  */
static long ClcMonVal(       /* Werte an den Monitorpunkten berechnen.   */
  void )
  {
  dP(ClcMonVal);
  long idos, ivol;
  int i, net, reading, gl, gi;
  MONREC *pm;
  net = GrdPprm->numnet;
  do {
    if (net > 0) {
      GrdSetNet(net);                                   eG(1);
      }
    reading = 0;
    gl = GrdPprm->level;
    gi = GrdPprm->index;
    ivol = IDENT(GRDarr, 4, gl, gi);
    if (GrdPprm->zscl > 0) {                                    //-98-08-22
      Pvol = TmnAttach(ivol, NULL, NULL, 0, NULL);  if (!Pvol)  eX(10);
    }
    idos = IDENT(DOStab, 0, gl, gi);
    for (i=1; i<=NumMon; i++) {
      pm = AryPtr(Pmon, i);  if (!pm)                      eX(2);
      if (pm->nn == net) {
        if ((StdStatus & ZET_DOSANY) && (!Pdos)) {
          Pdos = ReadArray(idos, &Idos);                eG(3);
          if (!Pdos)  return -1;
          reading = 1; 
        }
        InsertVal(i);                                   eG(5);
      }
    }
    if (Pdos) {
      TmnDetach(Idos, NULL, NULL, 0, NULL);             eG(6);
      Pdos = NULL;
    }
    if (Pvol) {
      TmnDetach(ivol, NULL, NULL, 0, NULL);             eG(10);
      Pvol = NULL;
    }
    net--;
    } while (net > 0);
  TmnDetach(Idat, NULL, NULL, 0, NULL);                 eG(8);
  Pdat = NULL;
  return 0;
eX_1:
  eMSG(_cant_set_grid_$_, net);
eX_2:
  eMSG(_index_$_, i);
eX_3:
  eMSG(_error_reading_$_, NmsName(idos));
eX_5:
  eMSG(_error_calculating_values_);
eX_6: eX_8:
  eMSG(_cant_detach_);
eX_10:
  eMSG(_no_volume_values_);
  }

  /*================================================================ SearchNet
  */
static long SearchNet(
  void )
  {
  dP(SearchNet);
  int i, net, k;
  MONREC *pm;
  float *pf, d;
  pf = GrdParr->start;
  net = GrdPprm->numnet;
  Pmon = TmnAttach(Imon, NULL, NULL, 0, NULL);  if (!Pmon)        eX(10);
  do {
    if (net > 0) {
      GrdSetNet(net);                                             eG(1);
      }
    d = (net <= 1) ? 0 : 2*GrdPprm->dd;                     //-2005-03-11
    for (i=1; i<=NumMon; i++) {
      pm = AryPtr(Pmon, i);  if (!pm)                             eX(2);
      if ( (pm->nl == 0)
        && (pm->xp >= GrdPprm->xmin+d) && (pm->xp < GrdPprm->xmax-d)      //-2011-07-29
        && (pm->yp >= GrdPprm->ymin+d) && (pm->yp < GrdPprm->ymax-d) ) {  //-2011-07-29
        for (k=1; k<=GrdPprm->nzmap; k++) {
           if (pm->zp < pf[k])  break;
        }
        if (k <= GrdPprm->nzdos && k<=GrdPprm->nz) {               //-2011-06-09
          pm->nn = net;
          pm->nl = GrdPprm->level;
          pm->ni = GrdPprm->index;
          pm->i = MIN(1 + (pm->xp - GrdPprm->xmin)/GrdPprm->dd, GrdPprm->nx);
          pm->j = MIN(1 + (pm->yp - GrdPprm->ymin)/GrdPprm->dd, GrdPprm->ny);
          pm->k = k;
        }
      }
    }
    net--;
  } while (net > 0);
  for (i=1; i<=NumMon; i++) {
    pm = AryPtr(Pmon, i);  if (!pm)                             eX(3);
    if (pm->k < 1)                                              eX(4);
  }
  TmnDetach(Imon, NULL, NULL, 0, NULL);                         eG(5);
  return 0;
eX_10:  eX_5:
  eMSG(_missing_points_);
eX_1:
  eMSG(_cant_set_grid_$_, net);
eX_2:  eX_3:
  eMSG(_index_$_, i);
eX_4:
  eMSG(_no_position_$_$$$_, i, pm->xp, pm->yp, pm->zp);
}

  /*================================================================== ZetInit
  */
long ZetInit(           /* initialize server    */
  long flags,           /* action flags         */
  char *istr )          /* server options       */
  {
  dP(ZetInit);
  int vrb, dsp;
  char *jstr, *ps, s[200];
  if (StdStatus & STD_INIT)  return 0;
  if (istr) {
    jstr = istr;
    ps = strstr(istr, "-v");
    if (ps) sscanf(ps+2, "%d", &StdLogLevel);
    ps = strstr(istr, "-y");
    if (ps) sscanf(ps+2, "%d", &StdDspLevel);
    ps = strstr(istr, "-d");
    if (ps)  strcpy(AltName, ps+2);
    }
  else  jstr = "";
  StdStatus |= flags;
  vLOG(3)("TALZET %d.%d.%s (%08lx,%s)", StdVersion, StdRelease, StdPatch, flags, jstr);
  Imon = IDENT(MONarr, 0, 0, 0);
  Idat = IDENT(MONarr, 1, 0, 0);
  vrb = StdLogLevel;
  dsp = StdDspLevel;
  sprintf(s, " GRD -v%d -y%d -d%s", vrb, dsp, AltName);
  GrdInit(flags, s);                                                    eG(2);
  TmnVerify(IDENT(GRDpar,0,0,0), NULL, NULL, 0);                        eG(3);
  ReadMonDef();                                                         eG(4);
  SearchNet();                                                          eG(5);
  StdStatus |= STD_INIT;
  return 0;
eX_2:  eX_3:
  eMSG(_cant_init_servers_);
eX_4:
  eMSG(_cant_read_definition_);
eX_5:
  eMSG(_cant_analyse_definition_);
  }

//================================================================ ZetServer
//
long ZetServer(         /* server routine for ZET       */
  char *s )             /* calling option               */
  {
  dP(ZetServer);
  int ztr_defined;
  char *t, *ps;
  long rc;
  if (StdArg(s))  return 0;
  if (*s) {
    t = s+1;
    switch ( *t ) {
        case 'b':  sscanf( t+1, "%ld", &Sequence );
                   break;
        case 'B':  sscanf( t+1, "%ld", &Sequence );
                   StdStatus |= ZET_ONESTEP;
                   break;
        case 'c':  StdStatus |= ZET_CONVAL;
                   if (t[1] == 's')  StdStatus |= ZET_CONSUM;
                   if (t[1] == 'S') {
                     StdStatus |= ZET_CONSUM;
                     StdStatus &= ~ZET_CONVAL;
                     }
                   break;
        case 'd':  StdStatus |= ZET_DRYVAL;
                   if (t[1] == 's')  StdStatus |= ZET_DRYSUM;
                   if (t[1] == 'S') {
                     StdStatus |= ZET_DRYSUM;
                     StdStatus &= ~ZET_DRYVAL;
                     }
                   break;
        case 'f':  ps = strrchr( t+1, MSG_PATHSEP );
                   if (ps)  strcpy( Fname, ps+1 );
                   else  strcpy( Fname, t+1 );
                   break;
        case 'k':  TagChar = t[1];
                   break;
        case 'm':  strcpy( DefName, t+1 );
                   break;
        case 'z':  strcpy(ZtrName, t+1);
                   break;
        default:   ;
        }
    return 0;
    }
  if (MsgFile == stderr)                                eX(20);

  if (0 == (StdStatus & STD_INIT)) {
    ZetInit(StdStatus&STD_GLOBAL, " ZET");              eG(1);
    }

  if ((StdStatus & (ZET_DOSANY)) == 0) {
    if (StdStatus & STD_NOLOG)  return 0;
    vLOG(1)(_no_output_type_);
    StdStatus |= STD_NOLOG;
    return -1;
    }

  ztr_defined = (ZtrFile != NULL);
  if (Sequence < 1)  Sequence = 1;
  if (MsgDspLevel >= 5) {
    Printf("ZET: sequence %d   \r", Sequence);
    fflush(stdout);
    }
  NmsSeq("sequence", Sequence);
  Pmon = TmnAttach(Imon, NULL, NULL, 0, NULL);          eG(10);
  rc = ClcMonVal();                                     eG(2);
  if (rc < 0) {
    return -1;
    }
  WriteMonZtr();                                        eG(3);
  if (!ztr_defined) {
    fclose(ZtrFile);
    ZtrFile = NULL;
    }
  TmnDelete(Te, Idat, TMN_NOID);                        eG(12);
  TmnDetach(Imon, NULL, NULL, 0, NULL);                 eG(11);
  vLOG(1)(_sequence_$_, Sequence);
  return 0;
eX_20:
  eMSG(_no_log_);
eX_1:
  eMSG(_cant_init_);
eX_2:
  eMSG(_cant_calculate_$_, Sequence);
eX_3:
  eMSG(_cant_write_$_, Sequence);
eX_10: eX_11:
  eMSG(_no_definition_);
eX_12:
  eMSG(_cant_delete_$_, NmsName(Idat));
}

// =======================================================================
//
// history:
//
// 2002-09-24 lj 1.0.0  final release candidate
// 2004-10-25 lj 2.1.4  2 outermost intervals not used in inner grids
// 2004-11-26 lj 2.1.7  string length for names = 256
// 2005-03-11 lj 2.1.16 relation of monitor points to grids
// 2005-03-17 uj 2.2.0  version number upgrade
// 2006-08-22 lj 2.2.12 string length for "Kennung" increased
// 2006-10-26 lj 2.3.0  external strings
// 2011-06-09 uj 2.4.10 grid assignment for monitor points corrected
// 2011-06-29 uj 2.5.0  DMNA header
// 2011-07-29 uj        positioning of monitor points improved
// 2012-04-06 uj 2.6.0  version number upgrade
// 2014-06-26 uj 2.6.11 eX/eG adjusted
//
//========================================================================

