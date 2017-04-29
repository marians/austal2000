# Makefile to create austal2000 and taldia
#
# system dependent definitions for gcc on Windows
# 2014-01-21
#
#########################################################################
BIN   =
OBJ   = .o
EXE   = .exe
SRC   = ../source/
SRCRM = ..\\source\\
COPT  = -Wall -m32 -DMSGALLOC -fpack-struct -funsigned-char \
        -DMAKE_LABEL=\"WG-0\" -O0 -ffloat-store
LOPT  = -m32
SYSL  = -lm
LIBS  = libz.a
OUTO  = -o #
OUTE  = -o #
CC    =$(BIN)gcc
REMOVE = del
#
vpath %.c $(SRC)
vpath %.h $(SRC)
include $(SRC)rules.make
#########################################################################

