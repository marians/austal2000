      PROGRAM VDISP
C  *******************************************************************
C  *  VDI-VERSION OF S/P-MODEL (VDISP)                               *
C  *  VERSION APRIL 88 (DOUBLE PRECISION, NO PLOT)
C  *******************************************************************
C  *                                                                 *
C  *  COMPILER:  FORTRAN 5 OR FORTRAN 77                             *
C  *                                                                 *
C  *  PROGRAM VDISP CALCULATES THE DISPERSION OF SINGLE WET AND DRY  *
C  *  COOLING TOWER PLUMES.IT IS BASED ON THE INTEGRAL-TYPE S/P-MO-  *
C  *  DEL. FOR DETAILS OF THE MODEL DERIVATION SEE :                 *
C  *  SCHATZMANN,M., AND POLICASTRO,A.J.(1984):AN ADVANCED INTEGRAL  *
C  *  MODEL FOR COOLING TOWER PLUME DISPERSION. ATMOSPHERIC ENVIRON- *
C  *  MENT, VOL.18,PP. 663-674.                                      *
C  *  PROGRAM VDISP CALLS THE FOLLOWING SUBROUTINES :                *
C  *  COMMV,READV,NONDV,GPWV,PWAV,SATV,INIV,DEFV,                    *
C  *  PREDV,CORRV,NUMIV,PRNV.                                        *
C  *  DIRECTIONS TO USE THE COMPUTER CODE ARE GIVEN IN SUBROUTINE    *
C  *  COMMV (TO BE FOUND BEHIND THE MAIN PROGRAM).                   *
C  *                                                                 *
C  *******************************************************************
C
C     Extended by JANICKE CONSULTING (IBJ) 2011-12-21
C
C     Working directory is given by first argument of program call.
C
C=====================================================================
C
      IMPLICIT REAL*8(A-H,O-Z)
C
      COMMON /DTDIM/  DM,UJMS,TJC,PHIJP,SIGJ,CONJ,Z01M,NPTS,ZM(101),
     &                TIC(101),PHIIP(101),UIMS(101),PIMB(101),TIK(101),
     &                ZLIMM
C
      COMMON /COUNT/  N,IDIST,IFORM,PRINT,PRINT1,ISEITE,ZAEHL,
     &                H,KK,KLIST,KOUNT,M,IEND
C
      COMMON /CONST/  A1,A2,A3,A4,CD,CDW,CLAMDA
C
      COMMON /DIMLNO/ AGRAV,CLAM,DBETA,DDELTA,DGAMMA,ENR,PI,
     &                VPRJ,DAL,RE
C
      COMMON /LAYER/  ACON,APRVN,QSC01,RNDIN,SATEMV,OMQA(101),TEMVA
     &                (101),PHIA(101),AUINA(101),APRVA(101),OMQA01,NK,
     &                NPT,STGC(101),RHGC(101),AVGC(101),PRG(101),
     &                OMQLIM
C
      COMMON /EXIT/   ATHBJ1,ATHJ1D,ATHBJ2,ATHJ2D,ALGVJ,ALTEVJ,ALCOVJ,
     &                ALXIVJ,ALDIVJ,FRDWT,FRDDR,AUIN01
C
      COMMON /ZFE/    ATHB01,ATHB02,ALGV0,ALBV0,ALTEV0,ALCOV0,ALXIV0,
     &                XIQ0,OMQ0,ETAQ0,PSIQ0,AST01,AST02,ACT01,ACT02
C
      COMMON /ENVIRO/ AUIN,ATEMV,PHI,RHG,SHG,SHR,STG
C
      COMMON /POSIT/  PSIQ,ETAQ,OMQ,XIQ
C
      COMMON /VARIAB/ ALBV,ALDIV,ALGV,ATHB1,ATHB2,ALTEV,ALCOV,ALXIV,
     &                EPS,FRNEU,U,VPR,WLAP,ETAS,DBR
C
      COMMON /ABBREV/ AW1,AW2,AW15,AW16,AW17,AW18,AW19,AW20,AST1,AST2,
     &                ACT1,ACT2
C
      COMMON /NUMINT/ XI,PSI,ETA,OMEGA
C
      COMMON /ADIF/   ADIF1,ADIF2,ADIF3,ADIF4,ADIF5,ADIF6,ADIF7
C
      DIMENSION       Y(7),YNEU(7),ADIFF1(7),ADIFF2(7),ADIFF3(7),
     &                ADIFF(7)
C
C     Added by IBJ  2011-12-21
C
      INTEGER pathlen
      CHARACTER*512 path
C
      PRINT*,'VDISP EXECUTING'
C
C     INPUT AND OUTPUT FILES, using working directory:
C
      path = '.'
      if (IARGC() .GT. 0)  CALL GETARG(1, path)
      pathlen = LEN_TRIM(path)
      OPEN(5,FILE=path(1:pathlen) // '/VDIIN.DAT',STATUS='OLD')
      OPEN(6,FILE=path(1:pathlen) // '/VDIOUT.DAT',STATUS='UNKNOWN')
C
      IF (KLIST .LT. 1) GOTO 22
      CALL COMMV
   22 CONTINUE
C
      IP=0
      N = 100
C
  100 CALL READV
      IF(IEND.EQ.999) GO TO 999
C
      PRINT = PRINT1
C
      CALL NONDV
C
      OMQ=0.0
      ETAQ=0.0
      XIQ=0.0
      PSIQ=0.0
      VPR=VPRJ
      ALXIV=ALXIVJ
      ALTEV=ALTEVJ
C
      CALL GPWV
C
      OMQAT=OMQA01+OMQ
      IF(OMQAT.GE.OMQA(1)) GO TO 44
      WRITE(6,1334)
      GO TO 324
C
   44 CALL PWAV
C
      CALL SATV
C
C
      CALL INIV
C
      IF (FRDWT .GE. 0.35) GO TO 40
      WRITE (6,1338)
      GO TO 324
   40 IF (KLIST .LT. 3) GO TO 119
      WRITE(6,13) A1,CD,FRDWT,SIGJ,A2,CLAMDA,AUIN01,ALXIVJ,
     &            A3,H,ALTEVJ,QSC01,A4,AGRAV,ALDIVJ,RE
   13 FORMAT(1H ,4X,3HA1=,F7.3,6X,7HCD    =,F5.2,8X,8HFRDWT = ,E10.4,2X,
     & 7HSIGJ  =,F7.5,/,5X,3HA2=,F7.3,6X,7HCLAMDA=,F6.3,7X,8HAUIN01= ,
     & E10.4,2X,7HALXIVJ=,E10.4,/,5X,
     & 3HA3=,F7.3,6X,7HH     =,F6.3,7X,8HALTEVJ= ,E10.4,2X,
     & 7HQSC01 =,E10.4,/,5X,3HA4=,F7.3,6X,7HAGRAV =,E11.4,2X,
     & 8HALDIVJ= ,E10.4,2X,7HRE    =,E10.4)
C
  119 DO 120 K=1,7
      ADIFF(K)=0.0
  120 CONTINUE
      XI=0.0
      ETA=0.0
      OMEGA=0.0
      PSI=0.0
      ISEITE=12
C     ISEITE: COUNTS THE LINES
      IP=IP+1
C     PRINT*,'IP=',IP		removed for g77 3.2
      ATHB1=ATHB01
      ATHB2=ATHB02
      ALGV=ALGV0
      ALBV=ALBV0
      ALTEV=ALTEV0
      ALCOV=ALCOV0
      ALXIV=ALXIV0
      XIQ=XIQ0
      PSIQ=PSIQ0
      ETAQ=ETAQ0
      OMQ=OMQ0
      ZAEHL=0.0
      M=0
C *********** VARIATION OF STEP LENGTH *****************************
C     SLP=STEP LENGTH PARAMETER
C     H1= NEW STEP LENGTH
      SLP=10.0
      H1=SLP*H
      IZ1=0
  150 IF (XI  .LT. .999999E1) GO TO 151
      IF (IZ1.EQ.1) GO TO 151
      ZAEHL=ZAEHL/SLP
      H=H1
      IZ1=1
C *********************************************************************
  151 ZAEHL=ZAEHL+1.
C     ZAEHL : COUNTS THE STEPS
C
      OMQAT=OMQA01+OMQ
      IF(OMQAT.GE.OMQA(1)) GO TO 45
      IF(KLIST.GT.2) WRITE(6,1334)
      GO TO 324
C
   45 CALL PWAV
C
  170 M=M+1
C
      CALL SATV
C
      CALL DEFV
C
      ADIFF(1)=AW1/ALBV
      ADIFF(2)=AW2/ALBV
      ADIFF(3)=AW15*ALGV/ALBV
      ADIFF(4)=AW16
      ADIFF(5)=-AW19*WLAP*AST2-AW20/ALBV
      ADIFF(6)=-AW17*ACON*AST2-ALCOV*AW18/ALBV
      ADIFF(7)=-AW17*(RHG+PHI*SHG)*AST2-ALXIV*AW18/ALBV
      ADIF1 = ADIFF(1)
      ADIF2 = ADIFF(2)
      ADIF3 = ADIFF(3)
      ADIF4 = ADIFF(4)
      ADIF5 = ADIFF(5)
      ADIF6 = ADIFF(6)
      ADIF7 = ADIFF(7)
C
      IF(ABS(ZAEHL-1.).LT.1.0E-6.AND.M.EQ.1) CALL PRNV
C
      IF((M/2*2).EQ.M) GO TO 190
C
      Y(1)=ATHB1
      Y(2)=ATHB2
      Y(3)=ALGV
      Y(4)=ALBV
      Y(5)=ALTEV
      Y(6)=ALCOV
      Y(7)=ALXIV
C
      CALL PREDV(Y,YNEU,M,H,ADIFF,ADIFF1,ADIFF2,ADIFF3)
C
      ATHB1=YNEU(1)
      ATHB2=YNEU(2)
      ALGV=YNEU(3)
      ALBV=YNEU(4)
      ALTEV=YNEU(5)
      ALCOV=YNEU(6)
      ALXIV=YNEU(7)
C
      GO TO 170
C
  190 CALL CORRV(Y,H,ADIFF,ADIFF1,ADIFF2,ADIFF3)
C
      CALL NUMIV
C
      XIQ=XIQ0+XI
      PSIQ=PSIQ0+PSI
      OMQ=OMQ0+OMEGA
      ETAQ=ETAQ0+ETA
C
      IF (OMQLIM.GT.OMQ ) GO TO 213
      CALL PRNV
      IF (KLIST.GT.2 ) WRITE(6,1337)
      GO TO 324
  213 IF(ATHB2.GE.0.0) GO TO 212
      CALL PRNV
      IF(KLIST.GT.2) WRITE(6,1336)
      GO TO 324
  212 ALCDIF=0.0
      IF(XIQ.LT.5.0) GO TO 211
      ALCDIF=ALCOV -ALCOVA
      IF(ALCDIF.LE.0.0) GO TO 211
      CALL PRNV
      IF(KLIST.GT.2)  WRITE(6,1335)
      GO TO 324
  211 ALCOVA=ALCOV
      IF (ALGV.GE.0.) GO TO 323
      CALL PRNV
      IF(KLIST.GT.2)  WRITE(6,1333)
      GO TO 324
  323 SUB=ABS((ZAEHL*H)-PRINT)
      IF(SUB.GT.0.0001) GO TO 210
C
      CALL PRNV
C
  324 CONTINUE
C
  210 IF (ALGV .LT. 0.) GO TO 220
      IF(OMQAT.LT.OMQA(1)) GO TO 220
      IF (FRDWT.LT.0.35) GO TO 220
      IF (ATHB2.LT.0.0) GO TO 220
      IF(ALCDIF.GT.0.0)GO TO 220
      IF(OMQLIM.LE.OMQ) GO TO 220
      IF(XIQ .LT.IDIST)GO TO 150
  220 IF (IP .LT. N ) GO TO 100
 1333 FORMAT(4X,'***** ALGV IS NEGATIVE *****')
 1334 FORMAT(4X,'***** NO AMBIENT DATA *****')
 1335 FORMAT(4X,'***** ALCOV IS INCREASING *****')
 1336 FORMAT(4X,'***** ATHB2 IS NEGATIVE *****')
 1337 FORMAT(4X,'***** OMQLIM SMALLER THAN OMQ *****')
 1338 FORMAT(4X,'***** FRD IS SMALLER THAN 0.35 *****')
C
  999 CONTINUE
C 	  PRINT*,'RESULTS WRITTEN ON FILE VDIOUT.DAT'  removed for g77 3.2
      STOP
      END
C
      SUBROUTINE COMMV
C
C   ******************************************************************
C   *                                                                *
C   * SUBROUTINE COMMV CONTAINS COMMENTS AND DIRECTIONS TO USE THE   *
C   * VDISP COMPUTER-CODE.                                           *
C   *                                                                *
C   ******************************************************************
C
C   1. INPUT
C   ========
C
C   THE COMPUTER CODE REQUIRES THE FOLLOWING INPUT DATA:
C
C   ALABEL  = NAME OF DATA SET (MAX. 20 LETTERS OR DIGITS)
C
C   DM      = DIAMETER OF EXITING PLUME  IN M
C   HM      = HEIGHT OF COOLING TOWER IN M
C   UJMS    = PLUME EXIT VELOCITY IN M/S
C   TJC     = PLUME EXIT TEMPERATURE IN DEGREE CENTIGRADE (CELSIUS).
C             IF TJC .LT. AMBIENT TEMPERATURE AT TOWER TOP LEVEL,
C             THE PROGRAM ASSUMES ZERO EXCESS PLUME EXIT TEMPERATURE.
C             THE AMBIENT TEMPERATURE AT 2M HEIGHT ABOVE GROUND IS
C             ASSUMED TO BE 10 DEGREE CENTIGRADE.
C   PHIJP   = PLUME EXIT RELATIVE HUMIDITY (PER CENT)
C             IF PHIJP=0.0, THE PROGRAM ASSUMES A DRY PLUME IN A DRY
C             ATMOSPHERE.
C             IF PHIJP .GT. 0.0, THE PROGRAM ASSUMES AN AMBIENT
C             RELATIVE HUMIDITY OF 77 PER CENT.
C   SIGJ    = PLUME EXIT LIQUID WATER CONTENT IN KG/KG
C   URMS    = AMBIENT WIND VELOCITY IN M/S, 'RECHENWERT' ACCORDING
C             TO TA-LUFT,APPENDIX C, CHAPTER 11
C   AK      = 'AUSBREITUNGSKLASSE' ACCORDING TO TA-LUFT, APPENDIX C,
C             CHAPTER 9 (REAL NUMBERS: AK=1.0, 2.0, 3.1, 3.2, 4.0, 5.0)
C   H       = STEP-LENGTH PARAMETER (FRACTION OF EXIT DIAMETER DM).
C             RECOMMENDED VALUE : H = 0.01
C   PRINT1  = CONTROL VARIABLE. IF, E.G., PRINT = 0.5 => RESULTS
C             ARE LISTED IN 0.5 EXIT DIAMETER DISTANCES (WITH RESPECT
C             TO X-AXIS ).RECOMMENDED VALUE : PRINT = 0.3
C   IDIST   = CONTROL VARIABLE. IF, E.G., IDIST = 20 => CALCULATION
C             STOPS AFTER A DISTANCE OF 20 EXIT DIAMETERS FROM TOWER.
C             RECOMMENDED VALUE : IDIST = 30
C
C   EXAMPLE:
C   -----------------
C   :    ALABEL     :
C   : VDI PLUME 1 A :
C   -----------------
C   --------------------------------------------------
C   : DM   :    HM  : UJMS :  TJC   : PHIJP :  SIGJ  :
C   : 65.0 :  130.0 : 4.1  :  36.0  : 100.0 : 0.003  :
C   --------------------------------------------------
C
C   -----------------------------------------
C   : URMS :    AK  :   H  : PRINT1 : IDIST :
C   : 9.0  :   3.2  : 0.01 :  0.3   :   30  :
C   -----------------------------------------
C
C   THE DATA INPUT MUST BE WRITTEN INTO A SEPARATE FILE NAMED
C   'VDIIN.DAT' (SEE OPEN STATEMENT IN LINE 73, MAIN PROGRAM).
C   UP TO 100 SINGLE DATA SETS CAN BE COMBINED WITHIN 1 VDIIN.DAT-
C   FILE. DURING COMPUTATION THE DATA SET JUST BEING PROCESSED IS
C   INDICATED ON THE TERMINAL DISPLAY (IP=..).
C
C
C   2. OUTPUT
C   =========
C
C   THE MODEL OUTPUT CONSISTS (1) OF THE NAME OF THE DATA SET (ALABEL),
C   (2) OF THE INPUT VARIABLES , AND FINALLY (3) A LISTING OF THE MODEL
C   RESULTS.
C   THE DENSIMETRIC FROUDE NUMBER, FRD, IS ALSO GIVEN. FRD IS DEFINED AS
C       FRD=UJMS/SQRT(DDR*GMSS*DM)
C       WITH
C       DDR= DENSITY DEFECT RATIO OF EXITING PLUME,
C       DDR= ABS((RHOPLUME-RHOAIR)/RHOAIR)
C       RHOPLUME=DENSITY OF EXITING PLUME IN KG/(M**3)
C       RHOAIR=DENSITY OF AMBIENT AIR AT TOWER TOP LEVEL IN KG/(M**3)
C       GMSS= GRAVITY ACCELERATION IN M/(S**2)
C       DM = DIAMETER OF EXITING PLUME IN M
C
C   THE MODEL RESULTS ARE
C   X   = DISTANCE FROM TOWER TOP IN M
C   UE  = PLUME RISE ABOVE TOWER TOP IN M
C
C
C   EXAMPLE:
C   -----------------
C   : VDI PLUME 1 A :
C   -----------------
C
C   ---------------------------------------------------------------
C   : DM    =  65.00   UJMS =  4.1    TJC  =  36.0   PHIJP = 100.0:
C   : SIGJ  =  0.003   URMS =  9.0    HM   = 130.0   H     = 0.010:
C   : PRINT1=  0.300   IDIST=  30     KLIST=   2     AK    = 3.200:
C   : FRD   =  0.494
C   ---------------------------------------------------------------
C
C   ------------------------------------------------------------------
C   DISTANCE X(M)                 PLUME RISE UE(M)
C          0.0                             0.0
C         18.8                             5.1
C         37.6                            10.5
C         56.3                            15.8
C           .                               .
C           .                               .
C        844.6                           137.5
C        864.1                           137.6
C        870.6                           137.6
C   -----------------------------------------------------------------
C
C   THE PROGRAM OUTPUT IS WRITTEN INTO A SEPARATE FILE NAMED VDIOUT.DAT
C   ( SEE OPEN STATEMENT IN LINE 74, MAIN PROGRAM ).
C
C
      RETURN
      END
C
      SUBROUTINE READV
C
C   ******************************************************************
C   *                                                                *
C   *    SUBROUTINE READV  (READ, VDI-VERSION ) READS TOWER          *
C   *    EXIT DATA AND AMBIENT DATA IN DIMENSIONAL FORM.             *
C   *                                                                *
C   ******************************************************************
C
      IMPLICIT REAL*8(A-H,O-Z)
      CHARACTER *30 ALABEL
C
C
      COMMON/DTDIM/ DM,UJMS,TJC,PHIJP,SIGJ,CONJ,Z01M,NPTS,ZM(101),
     &           TIC(101),PHIIP(101),UIMS(101),PIMB(101),TIK(101),
     &           ZLIMM
C
      COMMON/COUNT/ N,IDIST,IFORM,PRINT,PRINT1,ISEITE,ZAEHL,H,KK,
     &              KLIST,KOUNT,M,IEND
C
      COMMON/CONST/ A1,A2,A3,A4,CD,CDW,CLAMDA
C
      READ(5,101,END=999) ALABEL
  101 FORMAT (A30)
      READ(5,*)  DM,HM,UJMS,TJC,PHIJP,SIGJ
      READ(5,*)  URMS,AK,H,PRINT1,IDIST
C
      A1=0.057
      A2=0.011
      A3=10.0
      A4=9.0
      CD=2.5
      CLAMDA=0.92
C
C     VDI-VERSION: KLIST=2,  IFORM=2
C
      KLIST=2
      IFORM=2
C
      CONJ=0.0
      Z01M=HM
      IF(AK.GE.4.5) GO TO 41
      IF(AK.GE.3.5.AND.AK.LT.4.5) GO TO 42
      IF(AK.GE.3.15.AND.AK.LT.3.5) GO TO 43
      IF(AK.GE.2.5.AND.AK.LT.3.15) GO TO 44
      IF(AK.GE.1.5.AND.AK.LT.2.5) GO TO 45
      IF(AK.LT.1.5) GO TO 46
   41 EM= 0.09
      GAMCM=-0.011
      ZLIMM=1100.0
      GO TO 47
   42 EM=0.20
      GAMCM=-0.008
      ZLIMM=1100.0
      GO TO 47
   43 EM=0.22
      GAMCM=-0.007
      ZLIMM=800.0
      GO TO 47
   44 EM=0.28
      GAMCM=-0.007
      ZLIMM=800.0
      GO TO 47
   45 EM=0.37
      GAMCM=0.005
      ZLIMM=800.0
      GO TO 47
   46 EM=0.42
      GAMCM=0.015
      ZLIMM=800.0
C
   47 NPT=100
      ZM(1)=0.0
      DZM=2.0
      ZAUM=10.0
      ZATM=2.0
      TZATC=10.0
C
      DO 22 J=1,NPT
      ZM(J)=ZM(J-1)+DZM
      ZZZAU=ZM(J)/ZAUM
      UIMS(J)=URMS*ZZZAU**EM
      TIC(J)=TZATC+GAMCM*(ZM(J)-ZATM)
      PHIIP(J)=77.0
      IF (PHIJP.LT. 1.0E-09) PHIIP(J)=0.0
      IF (PHIJP.LT. 1.0E-09) SIGJ = 0.0
      PIMB(J)=0.0
   22 CONTINUE
C
      NPTS=NPT+1
      ZM(NPTS)=2000.
      UIMS(NPTS)=UIMS(NPT)
      TIC(NPTS)=TZATC+GAMCM*(ZM(NPTS)-ZATM)
      PHIIP(NPTS)=PHIIP(NPT)
      PIMB(NPTS)=PIMB(NPT)
C
   33 CONTINUE
      IF (SIGJ .LT. 1.0E-09)SIGJ = 0.00
      IF (CONJ .LT. 1.0E-09)CONJ = 1.0
      WRITE(6,03) ALABEL
   03 FORMAT (///,20X,A30,/)
      IF (KLIST .LT. 2) GOTO 100
      WRITE(6,04)DM,UJMS,TJC,PHIJP,SIGJ,URMS,HM,H,
     &           PRINT1,IDIST,KLIST,AK
   04 FORMAT(1X,'DM    = ',F10.5,3X,'UJMS = ',F10.5,3X,'TJC  = ',
     & F10.5,3X,'PHIJP= ',F8.3,/,1X,'SIGJ  = ',F10.5,3X,'URMS = ',
     & F10.5,3X,'HM   = ',F7.2,6X,'H    =  ',F7.5,
     & /,1X,'PRINT1= ',F10.5,3X,'IDIST= ',I5,7X,' KLIST= ',I4,9X,
     & 'AK   = ',F5.2)
      IF (KLIST.LT.4) GO TO 100
      WRITE(6,05)
   05 FORMAT(1X,///,9X,'AMBIENT DATA (DIMENSIONAL FORM)',/,
     &       1X,127('*'),//,10X,'ZM(J)',9X,'UIMS(J)',10X,'TIC(J)',
     &       9X,'PIMB(J)',9X,'PHIIP(J)')
      WRITE(6,06) (ZM(J),UIMS(J),TIC(J),PIMB(J),PHIIP(J),J=1,NPTS)
   06 FORMAT (1H,/,5(6X,F10.5))
      GO TO 100
  999 IEND=999
  100 CONTINUE
C
      RETURN
      END
C
      SUBROUTINE NONDV
C
C   ******************************************************************
C   *                                                                *
C   *        SUBROUTINE NONDV  NONDIMENSIONALIZES THE DATA           *
C   *        INPUT SUPPLIED BY READV.                                *
C   *                                                                *
C   ******************************************************************
C
      IMPLICIT REAL*8(A-H,O-Z)
C
      COMMON /DTDIM/ DM,UJMS,TJC,PHIJP,SIGJ,CONJ,Z01M,NPTS,ZM(101),
     &               TIC(101),PHIIP(101),UIMS(101),PIMB(101),TIK(101),
     &               ZLIMM
C
      COMMON /CONST/ A1,A2,A3,A4,CD,CDW,CLAMDA
C
      COMMON /DIMLNO/ AGRAV,CLAM,DBETA,DDELTA,DGAMMA,ENR,PI,
     &                VPRJ,DAL,RE
C
      COMMON /LAYER/ ACON,APRVN,QSC01,RNDIN,SATEMV,OMQA(101),TEMVA
     &              (101),PHIA(101),AUINA(101),APRVA(101),OMQA01,NK,
     &               NPT,STGC(101),RHGC(101),AVGC(101),PRG(101),
     &               OMQLIM
C
      COMMON /EXIT/ ATHBJ1,ATHJ1D,ATHBJ2,ATHJ2D,ALGVJ,ALTEVJ,ALCOVJ,
     &              ALXIVJ,ALDIVJ,FRDWT,FRDDR,AUIN01
C
      NPT=NPTS
      ATHJ1D=90.
      ATHJ2D=90.
C
      CLAM=CLAMDA*CLAMDA
      PI=4.*ATAN(1.)
      PNMB=1013.25
      TSK=373.15
      RDAIR=287.05
      RVAP=461.51
      HLVJK=2.501E06
      CPDAIR=1005.
      GMSS=9.80665
      VIMMS=15.0E-6
      GAMMA=0.61
      DELTA=-1.
      ACON=0.
      ALCOVJ=1.
      ALGVJ=1.
C
C     DETERMINATION OF AMBIENT PROPERTIES AT TOWER TOP LEVEL.
C
      K=1
      IF(ZM(K).LT.0.) Z01M = 0.
  102 IF(ZM(K)-Z01M) 103,104,105
  103 K=K+1
      GO TO 102
  105 K=K-1
      TG01CM=(TIC(K+1)-TIC(K))/(ZM(K+1)-ZM(K))
      PHIG01=(PHIIP(K+1)-PHIIP(K))/(ZM(K+1)-ZM(K))
      UG01MS=(UIMS(K+1)-UIMS(K))/(ZM(K+1)-ZM(K))
      PG01MB=(PIMB(K+1)-PIMB(K))/(ZM(K+1)-ZM(K))
      TI01C=TIC(K)+TG01CM*(Z01M-ZM(K))
      PHI01P=PHIIP(K)+PHIG01*(Z01M-ZM(K))
      UI01MS=UIMS(K)+UG01MS*(Z01M-ZM(K))
      PI01MB=PIMB(K)+PG01MB*(Z01M-ZM(K))
C
      GO TO 106
C
  104 TI01C=TIC(K)
      PHI01P=PHIIP(K)
      UI01MS=UIMS(K)
      PI01MB=PIMB(K)
C
  106 NK=K
C
      ATHBJ1=ATHJ1D*PI/180.
      ATHBJ2=ATHJ2D*PI/180.
      ASTJ1=SIN(ATHBJ1)
      ACTJ2=COS(ATHBJ2)
      IF(ABS(ASTJ1).LT.1.0E-5) ASTJ1=0.
      IF(ABS(ACTJ2).LT.1.0E-5) ACTJ2=0.
      UJSTMS=UJMS-UI01MS*ASTJ1*ACTJ2
C
      TJK=TJC+273.15
      TI01K=TI01C+273.15
      PHI01F=PHI01P/100.
C
C     NONDIMENSIONALIZATION. NK, IS THE NUMBER OF INTERFACE
C     JUST BELOW OR DIRECTLY AT TOWER TOP.
C
      DO 107 K=1,NPT
      OMQA(K)=ZM(K)/DM
      TIK(K)=TIC(K)+273.15
      TEMVA(K)=TIK(K)/TI01K
      PHIA(K)=PHIIP(K)/100.
      AUINA(K)=UIMS(K)/UJSTMS
      IF(PI01MB.GT.1.) APRVA(K)=PIMB(K)/PI01MB
      IF(PI01MB.LE.1.) APRVA(K)=0.
C
C     IF PI01MB .LE. 1MB, AMBIENT PRESSURE PROFILES ARE NOT INPUT.
C     THEY WILL BE CALCULATED IN SUBROUTINE GPWCT.
C
  107 CONTINUE
C
      OMQLIM=ZLIMM/DM
      RE=(UI01MS*DM)/VIMMS
      AUIN01=UI01MS/UJSTMS
      ALTEVJ=(TJK-TI01K)/TI01K
      IF ( ALTEVJ .LT. 0.0 ) ALTEVJ=0.0
      PHIJF=PHIJP/100.
      OMQA01=Z01M/DM
      AGRAV=DM*GMSS/(UJSTMS*UJSTMS)
      RNDIN=GMSS*DM/(RDAIR*TI01K)
      VPRJ=HLVJK/(RVAP*TI01K)
      IF(PI01MB.GT.1.) APRVN=PNMB/PI01MB
      IF(PI01MB.LE.1.) APRVN=1.
      SATEMV=TSK/TI01K
      TRK=273.15
      QSR=0.379038E-2
      QSC01=QSR*EXP(VPRJ*(TI01K-TRK)/TRK)
      DBETA=TI01K/TJK
      DGAMMA=GAMMA*QSC01
      DDELTA=DELTA*QSC01
      ENR=HLVJK*QSC01/(CPDAIR*TI01K)
      DAL=DM*GMSS/(TI01K*CPDAIR)
      QJQINR=EXP(ALTEVJ*VPRJ)
      QSCJ=QJQINR*QSC01
      QCJ=PHIJF*QSCJ
      QC01=PHI01F*QSC01
      SIG01=0.
      IF (PHIJF.LT.1.0E-09) SIGJ=0.
      ALDIVJ=-(DBETA*ALTEVJ+DGAMMA*(QCJ-QC01)/QSC01+DDELTA*(SIGJ-
     &        SIG01)/QSC01)
      ALXIVJ=((QCJ-QC01)+(SIGJ-SIG01))/QSC01
      IF(ALXIVJ.LT.1.0E-09) ALXIVJ=0.
C
      RETURN
      END
C
      SUBROUTINE GPWV
C
C   ******************************************************************
C   *                                                                *
C   * SUBROUTINE GPWV (GRADIENTS, POINTWISE AMBIENT DATA,            *
C   * VDI-VERSION ) CALCULATES THE GRADIENTS OF ALL AMBIENT          *
C   * VARIABLES WITHIN EACH LAYER AND, IF NECESSARY, ALSO THE        *
C   * AMBIENT PRESSURE RATIOS AT THE INTERFACE BETWEEN THE LAYERS.   *
C   *                                                                *
C   ******************************************************************
C
      IMPLICIT REAL*8(A-H,O-Z)
C
      COMMON /COUNT/ N,IDIST,IFORM,PRINT,PRINT1,ISEITE,ZAEHL,H,
     &               KK,KLIST,KOUNT,M,IEND
C
      COMMON /LAYER/ ACON,APRVN,QSC01,RNDIN,SATEMV,OMQA(101),
     & TEMVA(101),PHIA(101),AUINA(101),APRVA(101),OMQA01,NK,NPT,
     &               STGC(101),RHGC(101),AVGC(101),PRG(101),
     &               OMQLIM
C
      IF (APRVA(1).LT.0.01) APRVA(1) =1.
      NEND=NPT-1
      DO 200 K=1,NEND
      STGC(K)=(TEMVA(K+1)-TEMVA(K))/(OMQA(K+1)-OMQA(K))
      RHGC(K)=(PHIA(K+1)-PHIA(K))/(OMQA(K+1)-OMQA(K))
      AVGC(K)=(AUINA(K+1)-AUINA(K))/(OMQA(K+1)-OMQA(K))
      IF(STGC(K).EQ.0.) APRVA(K+1)=APRVA(K)*EXP(-RNDIN*(OMQA(K+1)-
     & OMQA(K))/TEMVA(K))
      IF(STGC(K).NE.0.) APRVA(K+1)=APRVA(K)*(TEMVA(K+1)/TEMVA(K))
     & **(-RNDIN/STGC(K))
C
  200 CONTINUE
C
      STGC(NPT)=STGC(NEND)
      RHGC(NPT)=RHGC(NEND)
      AVGC(NPT)=AVGC(NEND)
      IF (KLIST .LT. 4) GO TO 300
      WRITE(6,111)
  111 FORMAT(1H ,///,9X,'NONDIMENSIONALIZED AMBIENT PROFILES '
     &        ,/,1X,100('*'),//,13X,'OMQA(K)',12X,'TEMVA(K)',13X,
     &       'PHIA(K)',12X,'AUINA(K)',11X,'APRVA(K)')
      WRITE(6,120) (OMQA(K),TEMVA(K),PHIA(K),AUINA(K),APRVA(K),K=1,NPT)
  120 FORMAT(1H ,5(10X,F10.5))
C
  300 RETURN
      END
C
      SUBROUTINE PWAV
C
C   ******************************************************************
C   *                                                                *
C   * SUBROUTINE PWAV (POINTWISE AMBIENT DATA SUPPLY, VDI-VERSION)   *
C   * PROVIDES FOR EACH LOCAL POSITION UNDER CONSIDERATION           *
C   * THE APPROPRIATE SET OF AMBIENT PROPERTIES.                     *
C   *                                                                *
C   ******************************************************************
C
      IMPLICIT REAL*8(A-H,O-Z)
C
      COMMON /LAYER/ ACON,APRVN,QSC01,RNDIN,SATEMV,OMQA(101),TEMVA
     &              (101),PHIA(101),AUINA(101),APRVA(101),OMQA01,NK,
     &               NPT,STGC(101),RHGC(101),AVGC(101),PRG(101),
     &               OMQLIM
C
      COMMON /DIMLNO/ AGRAV,CLAM,DBETA,DDELTA,DGAMMA,ENR,PI,
     &                VPRJ,DAL,RE
C
      COMMON /ENVIRO/ AUIN,ATEMV,PHI,RHG,SHG,SHR,STG
C
      COMMON /POSIT/ PSIQ,ETAQ,OMQ,XIQ
C
      OMQAT=OMQA01+OMQ
      K=1
  300 IF(OMQA(K)-OMQAT) 301,302,303
  301 K=K+1
      IF (K.EQ.NPT) GO TO 302
      GO TO 300
  303 K=K-1
  302 AVG=AVGC(K)
      STG=STGC(K)
      RHG=RHGC(K)
      IF(ABS(STG).LT.1.0E-09) STG=0.
      AUIN=AUINA(K)+AVG*(OMQAT-OMQA(K))
      ATEMV=TEMVA(K)+STG*(OMQAT-OMQA(K))
      PHI=PHIA(K)+RHG*(OMQAT-OMQA(K))
      IF(STG.EQ.0.) APRV=APRVA(K)*EXP(-RNDIN*(OMQAT-OMQA(K))/
     & TEMVA(K))
      IF(STG.NE.0.) APRV=APRVA(K)*((ATEMV/TEMVA(K))**(-RNDIN/STG))
C
      FRAC=VPRJ*(ATEMV-1.)/ATEMV
      AFRAC=ABS(FRAC)
      IF(AFRAC.GT.100.) FRAC=100.
      SHR=(1./APRV)*EXP(FRAC)
      SHG=(1./APRV)*SHR*VPRJ*STG/(ATEMV*ATEMV)
C
      RETURN
      END
C
      SUBROUTINE SATV
C
C   ******************************************************************
C   *                                                                *
C   *    SUBROUTINE SATV COMPUTES THE VISIBLE PLUME RADIUS           *
C   *    ETAS = RS/B BY USING NEWTON'S APPROXIMATION PROCEDURE.      *
C   *                                                                *
C   ******************************************************************
C
      IMPLICIT REAL*8(A-H,O-Z)
C
      COMMON/DIMLNO/ AGRAV,CLAM,DBETA,DDELTA,DGAMMA,ENR,PI,
     &               VPRJ,DAL,RE
C
      COMMON/ENVIRO/ AUIN,ATEMV,PHI,RHG,SHG,SHR,STG
C
      COMMON/VARIAB/ ALBV,ALDIV,ALGV,ATHB1,ATHB2,ALTEV,ALCOV,ALXIV,
     &              EPS,FRNEU,U,VPR,WLAP,ETAS,DBR
C
      COMMON /COUNT/ N,IDIST,IFORM,PRINT,PRINT1,ISEITE,ZAEHL,H,
     &               KK,KLIST,KOUNT,M,IEND
C
      EX4SL = 0.
      EX4SR = 0.
      DBR = 0.
      KQ=0
      KOUNT=99
      KK = 1
      IF (ALXIV .LT. 1.0E-7) GO TO 10
      AUX1=VPR*ALTEV
      IF (KK.EQ.4) GO TO 50
      IF (KK.EQ.5) GO TO 50
      AUX2=ALTEV/ATEMV
      AUX10=ALXIV-SHR*(1.-PHI+AUX1/(1.+AUX2))
      IF(AUX10.LE.0.) GO TO 10
      IF (PHI .GT. 1.) GO TO 20
   30 IF (KQ .EQ. 0 ) EX4S = 1.
      IF (KQ .GT. 0 ) EX4S = 0.
C
C     KQ DECIDES WETHER THE LEFT (KQ > 0) OR THE RIGHT (KQ = 0)
C     OF THE TWO POSSIBLE ROOTS IN THE INTERVAL IS DETERMINED.
C
      KOUNT=0
   40 KOUNT=KOUNT+1
      AUX3=AUX1*EX4S
      AUX4=1.+AUX2*EX4S
      AUX5=AUX3/AUX4
C
C     KK DECIDES WETHER THE SPECIFIC HUMIDITY IS CALCULATED WITH
C     THE EXPONENTIAL FUNCTION( KK.NE.2 OR 3), WITH THE 8TH APPROXI-
C     MATION (KK=2) OR WITH THE THIRD APPROXIMATION (KK=3)
C
      IF(KK.EQ.2) GO TO 41
      IF(KK.EQ.3) GO TO 42
      IF (AUX5 .GE. 50) GO TO 21
      AUX5E= EXP(AUX5)
      FEX4S = ALXIV *EX4S -SHR*(-PHI+AUX5E)
      IF (KQ .GT. 0) GO TO 31
      IF (KOUNT .GT. 1) GO TO 31
      IF (FEX4S .GT. 0.) GO TO 45
   31 DFEX4S = ALXIV -SHR*AUX1*AUX5E/(AUX4*AUX4)
      DIFFER = ABS(FEX4S)
      IF (DIFFER .LT. 1.0E-7) GO TO 45
      IF (KOUNT .GT. 12) GO TO 45
      EX4S =EX4S -(FEX4S/DFEX4S)
      GO TO 40
   41 AUX6=1.0+AUX5+(1./2.)*AUX5**2+(1./6.)*AUX5**3
     &     +(1./24.)*AUX5**4+(1./120.)*AUX5**5
     &     +(1./720.)*AUX5**6+(1./5040.)*AUX5**7
      AUX7=AUX6+(1./40320.)*AUX5**8
      GO TO 43
   42 AUX6=1.+AUX5+(1./2.)*AUX5**2
      AUX7=AUX6+(1./6.)*AUX5**3
   43 FEX4S=ALXIV*EX4S-SHR*(-PHI+AUX7)
      IF (KQ .GT. 0) GO TO 32
      IF (KOUNT .GT. 1) GO TO 32
      IF (FEX4S .GT. 0.) GO TO 45
   32 DIFFER=ABS(FEX4S)
      IF(DIFFER.LT.1.0E-7) GO TO 45
      IF(KOUNT.GT.12) GO TO 45
      DFEX4S=ALXIV-SHR*AUX6*(AUX1*AUX4-AUX2*AUX3)/(AUX4*AUX4)
      EX4S=EX4S-(FEX4S/DFEX4S)
      GO TO 40
   45 IF(KQ .GT. 0) GO TO 46
      IF (KOUNT.GT. 12) GO TO 22
      IF (EX4S .LT. 0.) GO TO 23
      IF (KOUNT .EQ. 1) EX4S = 1.
      EX4SR = EX4S
      KQ=KQ+1
      GO TO 30
   46 IF (KOUNT.GT. 12) GO TO 24
      IF (EX4S .LT. 0.) GO TO 25
   47 EX4SL=EX4S
      DBR = EX4SR-EX4SL
      IF (DBR .LT. 1.0E-7) DBR = 0.
      EDGE=EXP(-2./CLAM)
      IF(EX4S.LT.EDGE) GO TO 20
      IF (EX4S .GT. 1.) EX4S= 1.
      ETASQ= -CLAM *LOG(EX4S)
      ETAS = SQRT(ETASQ)
      GO TO 70
   10 ETAS= 0.
      GO TO 70
   20 ETAS=SQRT(2.)
      GO TO 70
   21 KOUNT =21
      ETAS = 0.
      GO TO 70
   22 KOUNT = 22
      ETAS= 0.
      GO TO 70
   23 KOUNT = 23
      ETAS =0.
      GO TO 70
   24 KOUNT = 24
      ETAS = 0.
      GO TO 70
   25 KOUNT = 25
      ETAS = 0.
      GO TO 70
   26 KOUNT=26
      ETAS=0.
      GO TO 70
   50 AU10=ALXIV-SHR*(1.-PHI)-SHR*AUX1
      IF(AU10.LT.0.) GO TO 10
      IF(AUX1.LT.0.) KK=5
      EX4SR=1.
      EX4S=SHR*(1.-PHI)/(ALXIV-SHR*AUX1)
      KOUNT=77
      IF(KK.EQ.5) GO TO 47
      KOUNT=88
      AU1=0.5*SHR*AUX1*AUX1
      AU2=SHR*AUX1-ALXIV
      AU3=SHR*(1.-PHI)
      AU4=AU2*AU2-4.*AU1*AU3
      IF(AU4.LT.0.) GO TO 26
      AU5=SQRT(AU4)
      EX4SR=(0.5/AU1)*(-AU2+AU5)
      IF(EX4SR.GT.1.) EX4SR=1.
      EX4S=(0.5/AU1)*(-AU2-AU5)
      GO TO 47
C
   70 RETURN
      END
C
      SUBROUTINE INIV
C
C   ******************************************************************
C   *                                                                *
C   * SUBROUTINE INIV CALCULATES THE FIELD QUANTITIES AT THE END     *
C   * OF THE ZONE OF FLOW ESTABLISHMENT.                             *
C   *                                                                *
C   ******************************************************************
C
      IMPLICIT REAL*8(A-H,O-Z)
C
      COMMON /DIMLNO/ AGRAV,CLAM,DBETA,DDELTA,DGAMMA,ENR,PI,
     &                VPRJ,DAL,RE
C
      COMMON /ENVIRO/ AUIN,ATEMV,PHI,RHG,SHG,SHR,STG
C
      COMMON /EXIT/ ATHBJ1,ATHJ1D,ATHBJ2,ATHJ2D,ALGVJ,ALTEVJ,ALCOVJ,
     &              ALXIVJ,ALDIVJ,FRDWT,FRDDR,AUIN01
C
      COMMON /ZFE/ ATHB01,ATHB02,ALGV0,ALBV0,ALTEV0,ALCOV0,ALXIV0,
     &             XIQ0,OMQ0,ETAQ0,PSIQ0,AST01,AST02,ACT01,ACT02
C
      ALDIDR=-(DBETA*ALTEVJ)
C
      IF (ABS(ALDIVJ).LT.1.0E-5) FRDWT=1.0E+8
      IF (ABS(ALDIDR).LT.1.0E-5) FRDDR=1.0E+8
      IF (ABS(ALDIVJ).GE.1.0E-5) FRDWT=1./SQRT(ABS(AGRAV*ALDIVJ))
      IF (ABS(ALDIDR).GE.1.0E-5) FRDDR=1./SQRT(ABS(AGRAV*ALDIDR))
      FRD=FRDWT
      WRITE (6,03) FRD
   03 FORMAT (1X,'FRD   = ',E10.4,/)
C
      IF(ATHBJ1.GE.-1.0E-5.AND.ATHBJ1.LE.(0.99E0*PI/2.)) ATHB01=
     & PI/2.-(PI/2.-ATHBJ1)*EXP(-2.*AUIN)
C
      IF(ATHBJ1.LT.-1.0E-5.OR.ATHBJ1.GT.(0.99*PI/2.)) ATHB01=ATHBJ1
C
      IF(ATHBJ2.GE.1.0E-5.AND.ATHBJ2.LE.(1.01*PI/2.)) ATHB02=
     & ATHBJ2*EXP(-2./(1.+1.5/FRD)*AUIN)
C
      IF(ATHBJ2.LT.1.0E-5.OR.ATHBJ2.GT.(1.01*PI/2.)) ATHB02=ATHBJ2
C
      AST01=SIN(ATHB01)
      AST02=SIN(ATHB02)
      ACT01=COS(ATHB01)
      ACT02=COS(ATHB02)
C
      IF(ABS(AST01).LT.1.0E-5) AST01=0.
      IF(ABS(AST02).LT.1.0E-5) AST02=0.
      IF(ABS(ACT01).LT.1.0E-5) ACT01=0.
      IF(ABS(ACT02).LT.1.0E-5) ACT02=0.
C
      ALBV0=SQRT(0.5)
C
      ALTEV0=ALTEVJ
      ALXIV0=ALXIVJ
      ALCOV0=ALCOVJ
      ALGV0=ALGVJ
C
      IF (FRD.GE.100.0) EFRD=0.0
      IF (FRD.LT.100.0) EFRD=EXP(-0.1*FRD)
      XIQ0=6.2*(1.-EFRD)*EXP(-6.*AUIN)
      OMQ0=XIQ0*AST02
      ETAQ0=XIQ0*AST01*ACT02
      PSIQ0=XIQ0*ACT01*ACT02
C
      RETURN
      END
C
      SUBROUTINE DEFV
C
C   ******************************************************************
C   *                                                                *
C   *     SUBROUTINE DEFV CALCULATES THE ABBREVIATIONS USED TO       *
C   *     EXPRESS THE DERIVATIVES OF THE FIELD QUANTITIES.           *
C   *                                                                *
C   ******************************************************************
C
      IMPLICIT REAL*8(A-H,O-Z)
C
      COMMON /DIMLNO/ AGRAV,CLAM,DBETA,DDELTA,DGAMMA,ENR,PI,
     &                VPRJ,DAL,RE
C
      COMMON /CONST/A1,A2,A3,A4,CD,CDW,CLAMDA
C
      COMMON /ENVIRO/ AUIN,ATEMV,PHI,RHG,SHG,SHR,STG
C
      COMMON /VARIAB/ ALBV,ALDIV,ALGV,ATHB1,ATHB2,ALTEV,ALCOV,ALXIV,
     &                EPS,FRNEU,U,VPR,WLAP,ETAS,DBR
C
      COMMON /ABBREV/ AW1,AW2,AW15,AW16,AW17,AW18,AW19,AW20,AST1,
     &                AST2,ACT1,ACT2
C
      COMMON/EXIT/ ATHBJ1,ATHJ1D,ATHBJ2,ATHJ2D,ALGVJ,ALTEVJ,ALCOVJ,
     &             ALXIVJ,ALDIVJ,FRDWT,FRDDR,AUIN01
C
      ES=ETAS*ETAS
      EX1=EXP(-ES*(CLAM+1.)/CLAM)
      EX2=EXP(-ES*(CLAM+2.)/CLAM)
      EX3=EXP(-ES*(CLAM+3.)/CLAM)
      EX4=EXP(-ES/CLAM)
      EX5=EXP(-ES)
C
      AST1=SIN(ATHB1)
      ACT1=COS(ATHB1)
      AST2=SIN(ATHB2)
      ACT2=COS(ATHB2)
C
      IF(ABS(AST1).LT.1.0E-5) AST1=0.
      IF(ABS(AST2).LT.1.0E-5) AST2=0.
      IF(ABS(ACT1).LT.1.0E-5) ACT1=0.
      IF(ABS(ACT2).LT.1.0E-5) ACT2=0.
      U=AUIN/ALGV
      AVN1=1.+2.*U*AST1*ACT2
      AVN2=1.+(CLAM+1.)*U*AST1*ACT2
      AVN3=1.+U*AST1*ACT2
      AVN4=U*ACT1*ACT2
      AVN5=U*AST1*AST2
      AVN6=(CLAM+1.)*AVN4
      AVN7=(CLAM+1.)*AVN5
      AVN8=((CLAM+1.)/CLAM)*AVN1
      AVN9=AVN3-1.
C
      VPR=VPRJ/(ATEMV*ATEMV)
      EXPR=ALTEV*VPR
      WA1=(1.-PHI)*ES/CLAM+(1.-EX4)*VPR*ALTEV+0.25*(1.0-EX4
     &*EX4)*(EXPR*EXPR)+(1.-EX4*EX4*EX4)*(EXPR*EXPR*EXPR
     &)/18.
C
      WA2=((CLAM+1.)/CLAM)*(1.-EX5)*(1.0-PHI)+(1.-EX1)
     &*VPR*ALTEV+(CLAM+1.)/(CLAM+2.)*(1.0-EX2)*(EXPR*EXPR
     &)/2.+(CLAM+1.)/(CLAM+3.)*(1.0-EX3)*(EXPR*EXPR*EXPR
     &)/6.
C
      WA3=1.-EX1+(CLAM+1.)/(CLAM+2.)*(1.0-EX2)*VPR*ALTEV
     &+(CLAM+1.)/(CLAM+3.)*(1.0-EX3)*(EXPR*EXPR)/
     &2.+(1.-EX4+(1.-EX4*EX4)*VPR*ALTEV/2.+(1./6.)
     &*(1.-EX4*EX4*EX4)*(EXPR*EXPR))*(AVN2-1.)
C
      WA4=(1.-EX5+ES*AVN9)*(CLAM+1.)/CLAM
      WA5=EX1+EX4*(AVN2-1.)
      WA6=EX1*ALXIV
      WA7=EX4*ALXIV
C
C     ******  DOWNWASH CORRECTION  ******************
      CDW=1.0*LOG(1.+4.0*AUIN01/(1.+1./SQRT(FRDWT)))
      IF(RE.GE.2.0E05)CDW=0.33*CDW
      A1E=(A1+A2/(FRDWT*FRDWT))*(1.+1.2*CDW)
      CDC=CD+6.5*CDW
C     ***********************************************
C
      ALDIV=-(DBETA*ALTEV+(DGAMMA-DDELTA)*SHR*WA1+ALXIV*(DGAMMA*EX4
     &-DDELTA*(1.-EX4)))
      FRNEU=ALBV*AGRAV*ALDIV/(ALGV*ALGV)
      EPS= A1E/(1.+0.5*A3*ABS(U*AST1*ACT2))
     &           *(1.+A4*ABS(U)*SQRT(1.-AST1*AST1*ACT2*ACT2))
      WLAP=STG+DAL+ENR*(RHG+PHI*SHG)
C
      AW1=2.*(EPS*U*ACT1+SQRT(2.)/PI*CDC*U*ACT1*U*ACT1)/AVN1
      AW2=-2.*(CLAM*FRNEU*ACT2+EPS*U*AST1*AST2+SQRT(2.)/PI
     &             *CDC*(U*AST1*AST2*U*AST1*ABS(AST2)))/AVN1
      AW4=ENR*SHR*VPR*WA3
      AW5=ENR*(WA2+WA1*(AVN2-1.))
      AW6=ENR*WA4*SHR
      AW7=ENR*WA5
      AW8=ALTEV+ENR*(WA6+WA2*SHR)
      AW9=ALTEV+ENR*(WA7+WA1*SHR)
      AW10=2.*(AVN2*ALTEV+ENR*(WA6+WA2*SHR)+ENR*(WA7+WA1*SHR)*
     &(AVN2-1.))
      AW11=AW5*ALBV*SHG*AST2-2.*AW4*ALBV*ALTEV/ATEMV*STG*AST2-
     &     AW6*ALBV*RHG*AST2
      AW12=AVN2+AW4
      AW13=AW9*AVN6
      AW14=AW9*AVN7
      AW15=-2.*(CLAM*FRNEU*AST2+AVN1*EPS+AVN4*AW1-AVN5*AW2)
      AW16=CLAM*FRNEU*AST2+(1.+AVN1)*EPS+AVN4*AW1-AVN5*AW2
      AW17=AVN8/AVN2
      AW18=(AW15+2.*AVN2*AW16+AVN6*AW1-AVN7*AW2)/AVN2
      AW19=AVN8/AW12
      AW20=(AW8*AW15+AW1*AW13-AW2*AW14+AW10*AW16+AW11-AW7*(AW17*ALBV
     &    *AST2*(RHG+PHI*SHG)+ALXIV*AW18))/AW12
C
      RETURN
      END
C
      SUBROUTINE PREDV(Y,YNEU,M,H,DIFF,DIFFM1,DIFFM2,DIFFM3)
C
C   ******************************************************************
C   *                                                                *
C   * SUBROUTINE PREDV CONTAINS THE PREDICTOR PART                   *
C   * OF THE ADAMS-TYPE PREDICTOR-CORRECTOR PROCEDURE                *
C   * EMPLOYED TO SOLVE SIMULTANEOUSLY THE 7 DIFFERENTIAL            *
C   * EQUATIONS ADIFF(1) TO ADIFF(7).                                *
C   *                                                                *
C   ******************************************************************
C
      IMPLICIT REAL*8(A-H,O-Z)
C
      DIMENSION DIFF(7),DIFFM1(7),DIFFM2(7),DIFFM3(7),Y(7),YNEU(7)
C
      IF(M.EQ.1) GO TO 520
  185 DO 11 I=1,7
      YNEU(I)=Y(I)+1./24.*H*(55.*DIFF(I)-59.*DIFFM1(I)
     & +37.*DIFFM2(I)-9.*DIFFM3(I))
C
      DIFFM3(I)=DIFFM2(I)
      DIFFM2(I)=DIFFM1(I)
      DIFFM1(I)=DIFF(I)
C
   11 CONTINUE
C
      GO TO 1111
C
  520 DO 12 L =1,7
      DIFFM3(L)=DIFF(L)
      DIFFM2(L)=DIFF(L)
      DIFFM1(L)=DIFF(L)
C
   12 CONTINUE
C
      GO TO 185
C
 1111 RETURN
      END
C
      SUBROUTINE CORRV(Y,H,DIFF,DIFFM1,DIFFM2,DIFFM3)
C
C   ******************************************************************
C   *                                                                *
C   * SUBROUTINE CORRV CONTAINS THE CORRECTOR PART OF THE ADAMS-     *
C   * TYPE PREDICTOR-CORRECTOR PROCEDURE EMPLOYED TO SOLVE SIMUL-    *
C   * TANEOUSLY THE 7 DIFFERENTIAL EQUATIONS ADIFF(1) TO ADIFF(7).   *
C   *                                                                *
C   ******************************************************************
C
      IMPLICIT REAL*8(A-H,O-Z)
C
      COMMON /VARIAB/ ALBV,ALDIV,ALGV,ATHB1,ATHB2,ALTEV,ALCOV,ALXIV,
     & EPS,FRNEU,U,VPR,WLAP,ETAS,DBR
C
      DIMENSION Y(7),YNEU(7),DIFF(7),DIFFM1(7),DIFFM2(7),DIFFM3(7)
C
      DO 12 K=1,7
      YNEU(K)=Y(K)+1./24.*H*(9.*DIFF(K)+19.*DIFFM1(K)-
     & 5.*DIFFM2(K)+DIFFM3(K))
C
   12 CONTINUE
C
      ATHB1=YNEU(1)
      ATHB2=YNEU(2)
      ALGV=YNEU(3)
      ALBV=YNEU(4)
      ALTEV=YNEU(5)
      ALCOV=YNEU(6)
      ALXIV=YNEU(7)
C
      RETURN
      END
C
      SUBROUTINE NUMIV
C
C   ******************************************************************
C   *                                                                *
C   * SUBROUTINE NUMIV CARRIES OUT THE NUMERICAL INTEGRATION AND     *
C   * DETERMINES THE ACTUAL LENGTH OF THE TRAJECTORY AFTER EACH      *
C   * FULL INTERNAL STEP.  IT THEREBY CORRELATES THE VALUES OF THE   *
C   * COMPUTED FIELD QUANTITIES WITH ITS SPATIAL POSITION.           *
C   *                                                                *
C   ******************************************************************
C
      IMPLICIT REAL*8(A-H,O-Z)
C
      COMMON /VARIAB/ ALBV,ALDIV,ALGV,ATHB1,ATHB2,ALTEV,ALCOV,ALXIV,
     &                EPS,FRNEU,U,VPR,WLAP,ETAS,DBR
C
      COMMON /NUMINT/ XI,PSI,ETA,OMEGA
C
      COMMON /COUNT/ N,IDIST,IFORM,PRINT,PRINT1,ISEITE,ZAEHL,H,
     &               KK,KLIST,KOUNT,M,IEND
C
      AST1=SIN(ATHB1)
      AST2=SIN(ATHB2)
      ACT1=COS(ATHB1)
      ACT2=COS(ATHB2)
      IF(ABS(AST1).LT.1.0E-5) AST1=0.
      IF(ABS(AST2).LT.1.0E-5) AST2=0.
      IF(ABS(ACT1).LT.1.0E-5) ACT1=0.
      IF(ABS(ACT2).LT.1.0E-5) ACT2=0.
      XI=XI+H
      PSI=PSI+H*ACT1*ACT2
      ETA=ETA+H*AST1*ACT2
      OMEGA=OMEGA+H*AST2
C
      RETURN
      END
C
C
      SUBROUTINE PRNV
C
C   *****************************************************************
C   *                                                               *
C   *       SUBROUTINE PRNV PRINTS OUT THE FINAL RESULTS.           *
C   *                                                               *
C   *****************************************************************
C
      IMPLICIT REAL*8(A-H,O-Z)
C
      COMMON /DTDIM/ DM,UJMS,TJC,PHIJP,SIGJ,CONJ,Z01M,NPTS,ZM(101),
     &              TIC(101),PHIIP(101),UIMS(101),PIMB(101),TIK(101),
     &              ZLIMM
C
      COMMON /COUNT/  N,IDIST,IFORM,PRINT,PRINT1,ISEITE,ZAEHL,H,
     &                KK,KLIST,KOUNT,M,IEND
C
      COMMON /ENVIRO/ AUIN,ATEMV,PHI,RHG,SHG,SHR,STG
C
      COMMON /VARIAB/ ALBV,ALDIV,ALGV,ATHB1,ATHB2,ALTEV,ALCOV,ALXIV,
     &                EPS,FRNEU,U,VPR,WLAP,ETAS,DBR
C
      COMMON /ZFE/ ATHB01,ATHB02,ALGV0,ALBV0,ALTEV0,ALCOV0,ALXIV0,
     &             XIQ0,OMQ0,ETAQ0,PSIQ0,AST01,AST02,ACT01,ACT02
C
      COMMON /POSIT/ PSIQ,ETAQ,OMQ,XIQ
C
      COMMON /NUMINT/ XI,PSI,ETA,OMEGA
C
      COMMON /ADIF/ ADIF1,ADIF2,ADIF3,ADIF4,ADIF5,ADIF6,ADIF7
C
      PI=4.*ATAN(1.)
      IF(ABS(ZAEHL-1.).LT.1.0E-6.AND.M.EQ.1) GO TO 333
      PRINT=PRINT+PRINT1
      IF(ISEITE.EQ.72) GO TO 444
  111 ISEITE=ISEITE+1
  222 BRZD=1.*SQRT(2.)*ALBV
      VPD=2.*ETAS*ALBV
      IF(VPD.GT.BRZD) VPD=BRZD
      ATH1=180.*ATHB1/PI
      ATH2=180.*ATHB2/PI
      IF(ABS(ATH1).LT.1.0E-5) ATH1=0.
      IF(ABS(ATH2).LT.1.0E-5) ATH2=0.
      X=ETAQ*DM
      UE=OMQ*DM
      IF(IFORM.EQ.2) GO TO 555
C
      WRITE(6,11) XIQ,ETAQ,OMQ,ALCOV,ALXIV,VPD,BRZD,ATH2,ALTEV,ALDIV,
     & ALGV,U,EPS,FRNEU,DBR,KOUNT
C
      GO TO 999
  333 IF(IFORM.EQ.2) GO TO 777
      WRITE(6,12)
      GO TO 222
  444 IF(IFORM.EQ.2) GO TO 666
      WRITE(6,12)
      ISEITE=2
      GO TO 111
  666 WRITE(6,14)
      ISEITE=2
      GO TO 111
  777 WRITE(6,14)
      GO TO 222
  555 WRITE(6,13) X,UE
C
   11 FORMAT(1H ,F5.1,1X,F5.1,1X,F6.1,1X,4(E9.3,1X),F6.1,
     & 1X,E9.3,1X,E9.3,1X,3(E8.3,1X),E9.3,1X,E8.2,1X,I2)
C
   12 FORMAT(1H ,//,2X,3HXIQ,4X,4HETAQ,3X,3HOMQ,4X,5HALCOV,
     & 6X,5HALIXV,4X,3HVPD,7X,4HBRZD,4X,4HATH2,4X,5HALTEV,4X,
     & 5HALDIV,5X,4HALGV,6X,1HU,7X,3HEPS,5X,5HFRNEU,4X,
     & 3HDBR,3X,5HKOUNT)
C
   13 FORMAT(6H      ,F6.1,31X,F6.1)
C
   14 FORMAT(1H ,//,2X,13HDISTANCE X(M),22X,16HPLUME RISE UE(M))
C
  999 RETURN
      END
C
