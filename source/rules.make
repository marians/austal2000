# rules for making austal2000
# last change: 2012-11-05 uj (TalBlm.h added to TalDtb.obj)
#              2014-01-30 uj (split A2K and A2KN)
#              2014-02-20 uj IBJnls call in verif section adopted to make-version 3.82
#
# language file: make NLS LAN=<language> VRS=<version>
#          e.g.: make NLS LAN=de VRS=2.5.0
#
vpath %.c $(SRC)
vpath %.h $(SRC)
vpath %.nls $(SRC)
vpath %.text $(SRC)/nls
#
OBJALL = TalDos$(OBJ) TalDtb$(OBJ) TalBlm$(OBJ) TalGrd$(OBJ) \
        TalHlp$(OBJ) TalIo1$(OBJ) \
        TalMat$(OBJ) TalMod$(OBJ) TalNms$(OBJ) TalPlm$(OBJ) \
        TalPpm$(OBJ) TalPrm$(OBJ) TalZet$(OBJ) TalPtl$(OBJ) TalSrc$(OBJ) \
        TalTmn$(OBJ) TalWrk$(OBJ) TalBds$(OBJ) \
        genutl$(OBJ) genio$(OBJ)  TalRnd$(OBJ) \
        IBJdmng$(OBJ) IBJary$(OBJ) IBJtxt$(OBJ) IBJmsg$(OBJ) IBJgk$(OBJ) \
        IBJntr$(OBJ) TalUtl$(OBJ) TALuti$(OBJ) TalAKT$(OBJ) TalDef$(OBJ) \
        TalAKS$(OBJ) TalInp$(OBJ) TalQtl$(OBJ) TalMtm$(OBJ) TalMon$(OBJ) \
        TalRgl$(OBJ) TalPrf$(OBJ) TalVsp$(OBJ) \
        TalDMK$(OBJ) TalDMKutil$(OBJ) TalSOR$(OBJ) \
        austal2000$(OBJ) TALdia$(OBJ) IBJstd$(OBJ) TalCfg$(OBJ) \
        TalStt$(OBJ) $(LIBS)

OBJA2K = TalDos$(OBJ) TalDtb$(OBJ) TalBlm$(OBJ) TalGrd$(OBJ) \
        TalHlp$(OBJ) TalIo1$(OBJ) \
        TalMat$(OBJ) TalMod$(OBJ) TalNms$(OBJ) TalPlm$(OBJ) \
        TalPpm$(OBJ) TalPrm$(OBJ) TalZet$(OBJ) TalPtl$(OBJ) TalSrc$(OBJ) \
        TalTmn$(OBJ) TalWrk$(OBJ) TalBds$(OBJ) \
        genutl$(OBJ) genio$(OBJ)  TalRnd$(OBJ) \
        IBJdmng$(OBJ) IBJary$(OBJ) IBJtxt$(OBJ) IBJmsg$(OBJ) IBJgk$(OBJ) \
        IBJntr$(OBJ) TalUtl$(OBJ) TALuti$(OBJ) TalAKT$(OBJ) TalDef$(OBJ) \
        TalAKS$(OBJ) TalInp$(OBJ) TalQtl$(OBJ) TalMtm$(OBJ) TalMon$(OBJ) \
        TalRgl$(OBJ) TalPrf$(OBJ) TalVsp$(OBJ) IBJnls$(OBJ) IBJstd$(OBJ) \
        TalCfg$(OBJ) TalStt$(OBJ) $(LIBS)

OBJTDM =    TalBlm$(OBJ)  TalIo1$(OBJ)  TalGrd$(OBJ)  \
            TalNms$(OBJ)  TalTmn$(OBJ)  TalMat$(OBJ)  TalAdj$(OBJ) \
            genutl$(OBJ)  genio$(OBJ) \
            IBJdmng$(OBJ) IBJmsg$(OBJ)  IBJtxt$(OBJ)  IBJary$(OBJ) \
            IBJgk$(OBJ)   TalInp$(OBJ)  TalDef$(OBJ)  TalAKT$(OBJ) \
            TalAKS$(OBJ)  TalUtl$(OBJ)  TalRgl$(OBJ)  IBJntr$(OBJ) \
            TalDMK$(OBJ)  TalBds$(OBJ)  TalSOR$(OBJ)  TalDMKutil$(OBJ) \
            IBJstd$(OBJ)  IBJnls$(OBJ)  TalCfg$(OBJ)  TalStt$(OBJ) $(LIBS)
            
EXEVRF = verif00$(EXE) verif01$(EXE)  verif02$(EXE)  \
         verif11$(EXE) verif13$(EXE)  verif14$(EXE) \
         verif21$(EXE) verif22a$(EXE) verif22b$(EXE) \
         verif23$(EXE) verif31$(EXE) \
         verif41$(EXE) verif51a$(EXE) verif51b$(EXE) verif51c$(EXE) verif61$(EXE) 

# programs with native language support
NLSPGM = A2K A2KN DIA V00 V01 V02 V11 V13 V14 V21 V22a V22b V23 V31 V41 V51a V51b V51c V61

# modules for A2KN with NLS
NLSA2KN =  austal2000n TalInp IBJstd TalHlp TalMtm TalQtl TalDef IBJary IBJdmn \
         IBJmsg IBJntr IBJtxt TalAKS TalAKT TalBlm TalDos TalDtb TalGrd \
         TalIo1 TalMod TalMon TalPpm TalPrf TalPrm TalPtl TalRgl TalSrc \
         TalTmn TALuti TalUtl TalVsp TalWrk TalZet TalCfg
         
# modules for A2K with NLS
NLSA2K =  austal2000 TalInp IBJstd TalHlp TalMtm TalQtl TalDef IBJary IBJdmn \
         IBJmsg IBJntr IBJtxt TalAKS TalAKT TalBlm TalDos TalDtb TalGrd \
         TalIo1 TalMod TalMon TalPpm TalPrf TalPrm TalPtl TalRgl TalSrc \
         TalTmn TALuti TalUtl TalVsp TalWrk TalZet TalCfg

# modules for DIA with NLS
NLSDIA = TalWnd TalInp TalDef IBJary IBJdmn IBJmsg IBJntr IBJtxt TalAdj \
         TalAKS TalAKT TalBds TalBlm TalDMK TalDMKutil TalGrd \
         TalIo1 TalRgl TalSOR TalTmn TalUtl TalCfg

#modules for verfication with NLS      
NLSVRF = verif00  verif01  verif02  verif11  verif13  verif14  verif21 \
         verif22a verif22b verif23  verif31  verif41  verif51a verif51b verif51c \
         verif61  TalCfg

NLSV00  = verif00  TalCfg  IBJmsg
NLSV01  = verif01  TalCfg  IBJmsg
NLSV02  = verif02  TalCfg  IBJmsg
NLSV11  = verif11  TalCfg  IBJmsg
NLSV13  = verif13  TalCfg  IBJmsg
NLSV14  = verif14  TalCfg  IBJmsg
NLSV21  = verif21  TalCfg  IBJmsg
NLSV22a = verif22a TalCfg  IBJmsg
NLSV22b = verif22b TalCfg  IBJmsg
NLSV23  = verif23  TalCfg  IBJmsg
NLSV31  = verif31  TalCfg  IBJmsg
NLSV41  = verif41  TalCfg  IBJmsg
NLSV51a = verif51a TalCfg  IBJmsg
NLSV51b = verif51b TalCfg  IBJmsg
NLSV51c = verif51c TalCfg  IBJmsg
NLSV61  = verif61  TalCfg  IBJmsg

######################################################## main targets

all austal2000-system : IBJnls$(EXE) austal2000$(EXE) austal2000n$(EXE) taldia$(EXE) verification 

NLS : A2K_$(LAN).nls  A2KN_$(LAN).nls  DIA_$(LAN).nls  VRF_$(LAN).nls 

######################################################## implicit rules

#
%$(OBJ) : %.c
	$(CC) -c $(COPT) $(SRC)$*.c $(OUTO)$*$(OBJ)

# support for language LAN
%_$(LAN).nls : $(SRC)nls/*.text $(SRC)nls/$(LAN)/*.text IBJnls$(EXE)
	IBJnls -collect$(VRS) nls/$(LAN)/$*_$(LAN) $(NLS$*) 

# the NLS source code included by the modules
%.nls : %.text IBJnls$(EXE)
	IBJnls -prepare $*

# the NLS client source code for usage with a program
IBJnls_%.c : IBJnls$(EXE)
	IBJnls -defaults $* $(NLS$*)

IBJnls_%$(OBJ) : IBJnls_%.c

######################################################### explicit rules

austal2000$(EXE): austal2000$(OBJ) $(OBJA2K) IBJnls_A2K$(OBJ)
	$(CC) $(COPT) -c $(SRC)IBJstamp.c
	$(CC) $(COPT) $(OUTE)austal2000$(EXE) $(LOPT) austal2000$(OBJ) IBJstamp$(OBJ) \
	              $(OBJA2K) IBJnls_A2K$(OBJ) $(SYSL)

austal2000n$(EXE): austal2000n$(OBJ) $(OBJA2K) IBJnls_A2KN$(OBJ)
	$(CC) $(COPT) -c $(SRC)IBJstamp.c
	$(CC) $(COPT) $(OUTE)austal2000n$(EXE) $(LOPT) austal2000n$(OBJ) IBJstamp$(OBJ) \
	              $(OBJA2K) IBJnls_A2KN$(OBJ) $(SYSL)

taldia$(EXE) : TALdia$(OBJ) $(OBJTDM) IBJnls_DIA$(OBJ)
	$(CC) $(COPT) -c $(SRC)IBJstamp.c
	$(CC) $(COPT) $(OUTE)taldia$(EXE) $(LOPT) TALdia$(OBJ) IBJstamp$(OBJ) \
	              IBJnls_DIA$(OBJ) $(OBJTDM) $(SYSL)	

# the NLS server
IBJnls$(EXE) : IBJnls_m$(OBJ)
	$(CC) $(COPT) $(OUTE)IBJnls$(EXE) $(LOPT) IBJnls_m$(OBJ) $(SYSL)

#
GENHDR   = genutl.h genio.h
IBJHDR   = IBJtxt.h IBJmsg.h IBJall.h IBJary.h IBJstamp.h

IBJstd$(OBJ) : IBJstd.c IBJstd.h IBJmsg.h IBJstd.nls
genutl$(OBJ) : genutl.c genutl.h $(IBJHDR)
genio$(OBJ)  : genio.c  genio.h  genutl.h $(IBJHDR)
gencnv$(OBJ) : gencnv.c gencnv.h $(IBJHDR) $(GENHDR)
IBJary$(OBJ) : IBJary.c IBJary.nls $(IBJHDR)
IBJdmng$(OBJ) : IBJdmn.c IBJdmn.nls IBJdmn.h $(IBJHDR) zlib.h zconf.h
	$(CC) -c $(COPT) $(OUTO)IBJdmng$(OBJ) $(SRC)IBJdmn.c

IBJgk$(OBJ)  : IBJgk.c  IBJgk.h
IBJmsg$(OBJ) : IBJmsg.c IBJmsg.h IBJmsg.nls IBJall.h
IBJnls$(OBJ) : IBJnls.c IBJnls.h
IBJntr$(OBJ) : IBJntr.c IBJntr.h IBJntr.nls IBJdmn.h $(IBJHDR) 
IBJstamp$(OBJ): IBJstamp.c IBJstamp.h
IBJtxt$(OBJ) : IBJtxt.c IBJtxt.h IBJtxt.nls IBJmsg.h IBJall.h
TalAdj$(OBJ) : TalAdj.c TalAdj.h TalAdj.nls $(IBJHDR) $(GENHDR)
TalAKS$(OBJ) : TalAKS.c TalAKS.h TalAKS.nls TalInp.h TalStt.h TalCfg.h $(IBJHDR)
TalAKT$(OBJ) : TalAKT.c TalAKT.h TalAKT.nls IBJdmn.h TalInp.h $(IBJHDR)
TalBds$(OBJ) : TalBds.c TalBds.h TalBds.nls TalNms.h TalMat.h TalIo1.h \
               IBJdmn.h $(GENHDR)
TalBlm$(OBJ) : TalBlm.c TalBlm.h TalBlm.nls TalGrd.h TalIo1.h TalTmn.h TalNms.h \
               IBJstd.h $(IBJHDR) $(GENHDR)
TalCfg$(OBJ) : TalCfg.c TalCfg.h TalCfg.nls
TalDef$(OBJ) : TalDef.c TalDef.h TalDef.nls TalAKT.h TalAKS.h TalInp.h \
               TalUtl.h TalMat.h IBJdmn.h TalStt.h TalCfg.h $(IBJHDR) 
TalDMK$(OBJ) : TalDMK.c TalSOR.h TalDMK.nls TalDMKutil.h TalDMK.h TalWnd.h \
               IBJdmn.h $(IBJHDR) 
TalDMKutil$(OBJ) : TalDMKutil.c TalDMKutil.h TalDMKutil.nls $(IBJHDR) TalWnd.h
TalDos$(OBJ) : TalDos.c TalDos.h TalDos.nls TalGrd.h TalBlm.h TalNms.h \
               TalPrm.h TalPtl.h \
               TalSrc.h TalTmn.h TalWrk.h IBJstd.h $(IBJHDR) $(GENHDR)
TalDtb$(OBJ) : TalDtb.c TalDtb.h TalDtb.nls TalDos.h TalGrd.h TalMod.h TalNms.h \
               TalInp.h TalPrm.h TalZet.c TalZet.h \
               TalTmn.h TalWrk.h TalBlm.h IBJstd.h IBJdmn.h $(IBJHDR) $(GENHDR)
TalGrd$(OBJ) : TalGrd.c TalGrd.h TalGrd.nls IBJstd.h TalNms.h TalTmn.h TalBds.h \
               IBJdmn.h $(IBJHDR) $(GENHDR)
TalHlp$(OBJ) : TalHlp.c TalHlp.h TalHlp.nls TalCfg.h IBJmsg.h
TalInp$(OBJ) : TalInp.c TalInp.h TalInp.nls IBJdmn.h TalCfg.h $(IBJHDR) \
               TalUtl.h TalAKT.h TalDef.h TalAKS.h IBJntr.h TalStt.h \
               TalMat.h TalDMK.h TalZet.h TalRgl.h
TalIo1$(OBJ) : TalIo1.c TalIo1.h TalIo1.nls IBJmsg.h IBJary.h $(GENHDR)
TalMat$(OBJ) : TalMat.c TalMat.h
TalMod$(OBJ) : TalMod.c TalMod.h TalMod.nls TalPrf.h TalGrd.h TalPrm.h TalNms.h \
               TalTmn.h IBJstd.h IBJdmn.h $(IBJHDR) $(GENHDR)
TalMon$(OBJ) : TalMon.c TalMon.h TalMon.nls TalInp.h TalCfg.h TALuti.h \
               IBJdmn.h $(IBJHDR)
TalMtm$(OBJ) : TalMtm.c TalMtm.h TalMtm.nls genutl.h IBJdmn.h $(IBJHDR)
TalNms$(OBJ) : TalNms.c TalNms.h
TalPlm$(OBJ) : TalPlm.c TalPlm.h TalVsp.h
TalPpm$(OBJ) : TalPpm.c TalPpm.h TalPpm.nls TalGrd.h TalNms.h TalMod.h \
               TalPrf.h TalPrm.h TalTmn.h IBJstd.h IBJdmn.h $(IBJHDR) $(GENHDR)
TalPrf$(OBJ) : TalPrf.c TalPrf.h TalPrf.nls TalBlm.h TalGrd.h TalIo1.h TalNms.h  \
               TalTmn.h IBJstd.h TalWnd.h TalUtl.h TalPrm.h IBJdmn.h $(GENHDR) $(IBJHDR)
TalPrm$(OBJ) : TalPrm.c TalPrm.h TalPrm.nls IBJstd.h TalNms.h TalTmn.h \
               TalIo1.h TalGrd.h $(IBJHDR) $(GENHDR)
TalPtl$(OBJ) : TalPtl.c TalPtl.h TalPtl.nls TalNms.h TalTmn.h IBJstd.h \
               TalInp.h $(IBJHDR) $(GENHDR)
TalQtl$(OBJ) : TalQtl.c TalQtl.h TalQtl.nls TalDtb.h TalUtl.h \
               IBJstamp.h IBJdmn.h $(IBJHDR)
TalRgl$(OBJ) : TalRgl.c TalRgl.h TalRgl.nls IBJgk.h IBJdmn.h $(IBJHDR)
TalRnd$(OBJ) : TalRnd.c TalRnd.h
TalSOR$(OBJ) : TalSOR.c TalSOR.h TalSOR.nls IBJmsg.h IBJary.h TalDMKutil.h \
               TalWnd.h
TalSrc$(OBJ) : TalSrc.c TalSrc.h TalSrc.nls TalGrd.h TalNms.h TalPrm.h \
               TalPtl.h IBJstd.h TalRnd.h TalTmn.h IBJdmn.h $(IBJHDR) $(GENHDR)
TalStt$(OBJ) : TalStt.c TalStt.h IBJdmn.h IBJtxt.h IBJary.h IBJmsg.h
TalTmn$(OBJ) : TalTmn.c TalTmn.h TalTmn.nls IBJdmn.h $(IBJHDR) $(GENHDR)
TALuti$(OBJ) : TALuti.c TALuti.h TALuti.nls TalCfg.h IBJdmn.h $(IBJHDR)
TalUtl$(OBJ) : TalUtl.c TalUtl.h TalUtl.nls $(IBJHDR)
TalVsp$(OBJ) : TalVsp.c TalVsp.h TalVsp.nls
TalWrk$(OBJ) : TalWrk.c TalWrk.h TalWrk.nls TalGrd.h TalMat.h TalMod.h \
               TalNms.h TalPlm.h TalPrf.h TalPrm.h TalPtl.h TalPpm.h \
               TalSrc.h TalTmn.h IBJstd.h IBJdmn.h $(IBJHDR) $(GENHDR)
TalZet$(OBJ) : TalZet.c TalZet.h TalZet.nls TalGrd.h IBJstd.h TalNms.h \
               TalTmn.h IBJdmn.h $(IBJHDR) $(GENHDR)

austal2000$(OBJ) : austal2000.c austal2000.h austal2000.nls IBJstd.nls\
           TalDos.h TalDtb.h \
           TalGrd.h TalIo1.h TalMod.h TalNms.h TalPlm.h \
           TalPrf.h TalPrm.h TalPtl.h TalSrc.h TalTmn.h TalWrk.h \
           TalVsp.h TalInp.h TalDef.h TalMtm.h TalQtl.h TalMon.h TALuti.h \
           TalUtl.h TalRnd.h $(IBJHDR) IBJstamp.h IBJstd.h $(GENHDR) \
           TalCfg.h TalStt.h
	$(CC)  -DMAIN $(COPT) -c $(SRC)austal2000.c

austal2000n$(OBJ) : austal2000.c austal2000.h austal2000n.nls IBJstd.nls\
           TalDos.h TalDtb.h \
           TalGrd.h TalIo1.h TalMod.h TalNms.h TalPlm.h \
           TalPrf.h TalPrm.h TalPtl.h TalSrc.h TalTmn.h TalWrk.h \
           TalVsp.h TalInp.h TalDef.h TalMtm.h TalQtl.h TalMon.h TALuti.h \
           TalUtl.h TalRnd.h $(IBJHDR) IBJstamp.h IBJstd.h $(GENHDR) \
           TalCfg.h TalStt.h
	$(CC)  -DMAIN  -DAUSTAL2000N $(COPT) -c $(SRC)austal2000.c $(OUTO)austal2000n$(OBJ)

TALdia$(OBJ) : TalWnd.c  TalWnd.h  TalWnd.nls TalGrd.h  TalBlm.h  TalNms.h  \
           TalTmn.h  TalMat.h  IBJstd.h  TalAdj.h  $(IBJHDR) TalBds.h \
           $(GENHDR) TalInp.h  TalDef.h TalDMK.h TalDMKutil.h IBJdmn.h \
           TalStt.h  IBJnls.h
	$(CC) -DMAIN -c $(COPT) $< $(OUTO)$@

IBJnls_m$(OBJ) : IBJnls.c
	$(CC)  -DMAIN $(COPT) -c $(SRC)IBJnls.c $(OUTO)IBJnls_m$(OBJ)

########################################################################

verification : $(EXEVRF)

verif%$(EXE) : verif%.c verif.h verif%.nls IBJnls_V%.c TalCfg.h IBJmsg.h \
               TalCfg$(OBJ) IBJmsg$(OBJ)
	$(CC) -c $(COPT) $(SRC)IBJnls_V$*.c $(OUTO)IBJnls_V$*$(OBJ)
	$(CC) $(COPT) $< $(LOPT) IBJnls_V$*$(OBJ) IBJnls$(OBJ) TalCfg$(OBJ) \
                         IBJmsg$(OBJ) $(SYSL) $(OUTE)$@

IBJnls_V%.c : $(SRC)nls/verif%.text
	IBJnls -defaults V$* $(NLSV$*)

###################### remove all object files #####################

.PHONY : clean clean_nls
clean :
	$(REMOVE) *$(OBJ) austal2000$(EXE) austal2000n$(EXE) taldia$(EXE) IBJnls$(EXE) $(EXEVRF) \
	$(SRCRM)*.nls $(SRCRM)IBJnls_*.c

clean_nls :
	$(REMOVE) IBJnls_*$(OBJ) $(SRCRM)IBJnls_*.c

##########################################################################
