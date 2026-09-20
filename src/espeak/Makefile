# ESPK: eSpeak NG English frontend + integer Klatt (klatt_fx) for DOS.
#   make            host build ./espk_host (text -> 8-bit WAV)
#   make dos        DJGPP build ESPK.EXE
#   make data       data/ESPK.DAT from the host-built espeak-ng data (build-host/)
DJ ?= $(abspath ../../../tools/djgpp/bin)
DCC = $(DJ)/i586-pc-msdosdjgpp-gcc
KLATT = ../klatt
LIBSRC = $(wildcard lib/*.c)
ESPKSRC = espk/wgout.c espk/espk.c espk/stubs.c espk/voices_txt.c espk/espk_data.c espk/ucdmini.c
INC = -Ilib -Ilib/espeak-ng -Ilib/ucd/.. -I$(KLATT) -Iespk
CFLAGS = -g -O2 -Wall -Wno-format-truncation -Wno-format-overflow -Wno-stringop-truncation -Wno-stringop-overflow -Wno-unused-function -Wno-unused-but-set-variable -Wno-unused-variable -fno-strict-aliasing $(INC)
DCFLAGS = -O2 -march=i386 -mtune=i386 -Wall -Wno-unused-function -Wno-unused-but-set-variable -Wno-unused-variable -fno-strict-aliasing $(INC) -Iespk

all: espk_host

espk_host: $(LIBSRC) $(ESPKSRC) espk/main_host.c $(KLATT)/klatt_fx.c
	$(CC) $(CFLAGS) -o $@ $(LIBSRC) $(ESPKSRC) espk/main_host.c $(KLATT)/klatt_fx.c -lm

data: data/ESPK.DAT
data/ESPK.DAT: tools/mkdata_en.sh tools/mkdat.py
	tools/mkdata_en.sh $@

install: ESPK.EXE ESPKD.EXE espkd/LOOPS.EXE espkd/INT14B.EXE data/ESPK.DAT
	mkdir -p ../../share/espk && cp ESPK.EXE ESPKD.EXE espkd/LOOPS.EXE espkd/INT14B.EXE data/ESPK.DAT ../../share/espk/ && cp espkd/dos/*.BAT espkd/dos/*.TXT ../../share/espk/ && uv run --script tools/cwsnoswap.py ../../share/CWSDPMI.EXE ../../share/espk/CWSDPMI.EXE 2

# ESPKD.EXE: the resident (shell wrapper) form, see espkd/espkd.c and NOTES.md
ESPKD_SRC = espkd/espkd.c espkd/engine_espk.c espkd/ring.c espkd/synth.c espkd/coro.S
DAS = $(DJ)/i586-pc-msdosdjgpp-as
DOBJCOPY = $(DJ)/i586-pc-msdosdjgpp-objcopy
espkd/rmstub_bin.h: espkd/rmstub.S
	$(DAS) $< -o espkd/rmstub.o && $(DOBJCOPY) -O binary -j .text espkd/rmstub.o espkd/rmstub.bin
	( echo "/* generated from rmstub.S */"; echo "static const unsigned char rmstub_bin[] = {"; od -An -tx1 -v espkd/rmstub.bin | sed 's/ *\([0-9a-f][0-9a-f]\)/0x\1,/g'; echo "};" ) > $@
ESPKD.EXE: $(LIBSRC) $(ESPKSRC) $(ESPKD_SRC) espkd/rmstub_bin.h $(KLATT)/klatt_fx.c
	$(DCC) $(DCFLAGS) -Iespkd -o $@ $(LIBSRC) $(ESPKSRC) $(ESPKD_SRC) $(KLATT)/klatt_fx.c -Wl,--wrap=malloc,--wrap=calloc,--wrap=realloc,--wrap=free -lemu
	uv run --script tools/stubsize.py $@ 4096
# LOOPS.EXE: a real-mode CPU benchmark run inside the child shell (Open Watcom 16-bit)
WATCOM ?= $(abspath ../../../tools/ow)
espkd/INT14B.EXE: espkd/int14b.c
	cd espkd && export WATCOM=$(WATCOM) INCLUDE=$(WATCOM)/h PATH=$(WATCOM)/binl64:$$PATH && wcc -ms -0 -zq -w1 -bt=dos -os -fo=int14b.obj int14b.c && wlink system dos option quiet file int14b.obj name INT14B.EXE
espkd/LOOPS.EXE: espkd/loops.c
	cd espkd && export WATCOM=$(WATCOM) INCLUDE=$(WATCOM)/h PATH=$(WATCOM)/binl64:$$PATH && wcc -ms -0 -zq -w1 -bt=dos -os -fo=loops.obj loops.c && wlink system dos option quiet file loops.obj name LOOPS.EXE

ESPK.EXE: $(LIBSRC) $(ESPKSRC) espk/main_dos.c $(KLATT)/klatt_fx.c ../sbsynth/sb16.c
	$(DCC) $(DCFLAGS) -o $@ $(LIBSRC) $(ESPKSRC) espk/main_dos.c $(KLATT)/klatt_fx.c ../sbsynth/sb16.c -I../sbsynth -lemu
dos: ESPK.EXE

clean:
	rm -f espk_host ESPK.EXE ESPKD.EXE *.o espkd/rmstub.o espkd/rmstub.bin espkd/rmstub_bin.h espkd/loops.obj espkd/LOOPS.EXE espkd/int14b.obj espkd/INT14B.EXE
