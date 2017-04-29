# Makefile to create austal2000 and taldia
#
# system dependent definitions for Linux Intel C (12.0)
# 2011-08-01
#
#########################################################################

BIN   =
OBJ   = .o
EXE   =
SRC   = ../source/
SRCRM = $(SRC)
CC    = icc
COPT  = -DMSGALLOC -fpack-struct -Wmissing-prototypes -funsigned-char \
        -no-multibyte-chars -DMAKE_LABEL=\"LI-x\" -O3 -msse2 -fp-model source
LIBS  = libz.a
LOPT  = -static -L/usr/lib
SYSL  = -lm
OUTO  = -o #
OUTE  = -o #
REMOVE = rm -f
#
vpath %.c $(SRC)
vpath %.h $(SRC)
include $(SRC)rules.make
#########################################################################

