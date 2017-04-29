# Makefile to create austal2000 and taldia
#
# system dependent definitions for gcc on Linux
# 2011-07-18
#
########################################################################
BIN   =
OBJ   = .o
EXE   =
SRC   = ../source/
SRCRM = $(SRC)
COPT  = -m32 -DMSGALLOC -fpack-struct -funsigned-char \
        -DMAKE_LABEL=\"LG-0\" -O0 -ffloat-store
LOPT  = -m32
SYSL  = -lm
LIBS  = libz.a
OUTO  = -o #
OUTE  = -o #
CC    =$(BIN)gcc
REMOVE = rm -f
#
vpath %.c $(SRC)
vpath %.h $(SRC)
include $(SRC)rules.make
#########################################################################

