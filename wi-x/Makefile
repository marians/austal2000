# Makefile to create austal2000 and taldia
#
# system dependent definitions: Windows Intel C (13.0 with VS8)
# 2014-01-21
#
#########################################################################
BIN   =
OBJ   = .obj
EXE   = .exe
SRC   = ../source/
SRCRM = ..\\source\\
COPT  = /nologo /DMSGALLOC /Zp1 /J /D_CRT_SECURE_NO_DEPRECATE \
        /Qmultibyte-chars- /DMAKE_LABEL=\"WI-x\" /O3 /fp:source /arch:SSE2
LOPT  = /nologo
SYSL  = 
LIBS  = zlib.lib
OUTO  = /Fo
OUTE  = /Fe
CC    = $(BIN)icl
REMOVE = del
#
vpath %.c $(SRC)
vpath %.h $(SRC)
include $(SRC)rules.make
#########################################################################
