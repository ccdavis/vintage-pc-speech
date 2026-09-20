DJ ?= $(abspath ../../../tools/djgpp/bin)
CC = $(DJ)/i586-pc-msdosdjgpp-gcc
CFLAGS = -O2 -march=i386 -mtune=i386 -Wall -fcommon
SAM = ../sam-upstream/src
all: SBPLAY.EXE SAMSB.EXE
SBPLAY.EXE: sbplay.c sb16.c sb16.h
	$(CC) $(CFLAGS) -o $@ sbplay.c sb16.c -lm
SAMSB.EXE: samsb.c sb16.c sb16.h $(SAM)/sam.c $(SAM)/reciter.c $(SAM)/render.c $(SAM)/debug.c
	$(CC) $(CFLAGS) -o $@ samsb.c sb16.c $(SAM)/sam.c $(SAM)/reciter.c $(SAM)/render.c $(SAM)/debug.c
install: all
	cp SBPLAY.EXE SAMSB.EXE ../../share/
clean:
	rm -f *.EXE *.o
