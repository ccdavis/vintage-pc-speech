.AUTODEPEND

MODEL=s
CC = tcc
TASM = TASM
TLIB = tlib
TLINK = tlink
LIBPATH = C:\TC\LIB
LIBS=c$(MODEL).lib
INCLUDEPATH = C:\TC\INCLUDE


#       *Implicit Rules*
.c.obj:
  $(CC) -mt -f- -1 -b- -a- -Fs -f- -G- -O -tDc  -w -c -d -Z {$< }

.asm.obj:
        $(TASM) /Zn /m5 /mx /w+ /c /l /v $*

#       *List Macros*


EXE_dependencies =  \
 c0.obj \
 execdrv.obj \
 english.obj \
 phoneme.obj \
 parser.obj \
 speak.obj \
 ring.obj

all: talk.sys

#       *Explicit Rules*
talk.exe: $(EXE_dependencies)
  $(TLINK) /m /s /L$(LIBPATH) @&&!
c0.obj+
execdrv.obj+
speak.obj+
english.obj+
phoneme.obj+
parser.obj+
ring.obj
talk
talk
$(LIBS)
!

#       *Individual File Dependencies*
c0.obj: c0.asm

execdrv.obj: execdrv.c

phoneme.obj: phoneme.c

parser.obj: parser.c

speak.obj: speak.c

ring.obj: ring.c

english.obj: english.c

talk.sys: talk.exe
  exe2bin talk.exe talk.sys
