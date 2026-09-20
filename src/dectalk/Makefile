# DECtalk for DOS: the ARM7/Epson single-threaded engine configuration built with DJGPP.
#   make            host reference ./dtsay (text -> WAV)
#   make dos        DTSAY.EXE (non-resident, Sound Blaster)   [later]
#   make patch      refresh patches/upstream.diff from the git-ignored upstream clone
UP = ../dectalk-upstream/dectalk/src/dapi/src
DJ ?= $(abspath ../../../tools/djgpp/bin)
DCC = $(DJ)/i586-pc-msdosdjgpp-gcc
SRCS = $(shell cat files.txt)
UPSRCS = $(addprefix $(UP)/,$(SRCS))
GLUE = glue/stubs.c
vpath %.c $(sort $(dir $(UPSRCS))) glue
OBJS_H = $(patsubst %.c,build-host/%.o,$(notdir $(SRCS) $(GLUE)))
OBJS_D = $(patsubst %.c,build-dos/%.o,$(notdir $(SRCS) $(GLUE)))
DEFS = -DARM7 -DENGLISH_US -DACNA -DARM7_NOSWI -DENGLISH -Di386 -DWIN32_TEST -DBLD_DECTALK_DLL
INC = -I$(UP)/api -I$(UP)/cmd -I$(UP)/lts -I$(UP)/ph -I$(UP)/vtm -I$(UP)/kernel -I$(UP)/nt -I$(UP)/include -I$(UP)/../.. -I$(UP)/hlsyn -Iglue
WARN = -w
CFLAGS = -g -O2 -fno-strict-aliasing -U__linux__ -Ulinux -U__gnu_linux__ $(DEFS) $(INC) $(WARN)
DCFLAGS = -O2 -march=i386 -mtune=i386 -fno-strict-aliasing -UMSDOS -U__MSDOS__ $(DEFS) $(INC) $(WARN)

all: dtsay
dtsay: $(OBJS_H) glue/main_host.c
	$(CC) $(CFLAGS) -o $@ $(OBJS_H) glue/main_host.c -lm
build-host/%.o: %.c | build-host
	$(CC) $(CFLAGS) -c $< -o $@
build-dos/%.o: %.c | build-dos
	$(DCC) $(DCFLAGS) -c $< -o $@
build-host build-dos:
	mkdir -p $@
patch:
	cd $(UP)/../../.. && git diff > $(abspath patches/upstream.diff)
clean:
	rm -rf build-host build-dos dtsay *.EXE
.PHONY: all patch clean

# DOS, non-resident: DTSAY.EXE plays through the Sound Blaster (polling driver from ../sbsynth)
DTSAY.EXE: $(OBJS_D) glue/main_dos.c ../sbsynth/sb16.c
	$(DCC) $(DCFLAGS) -I../sbsynth -o $@ $(OBJS_D) glue/main_dos.c ../sbsynth/sb16.c -lemu
dos: DTSAY.EXE
install: DTSAY.EXE DTALKD.EXE
	mkdir -p ../../share/dectalk && cp DTSAY.EXE DTALKD.EXE ../../share/dectalk/ && cp ../../share/espk/CWSDPMI.EXE ../../share/dectalk/ 2>/dev/null || true
.PHONY: dos install

# DTALKD.EXE: the resident (shell wrapper) form, the ESPKD framework with DECtalk as the engine
DAS = $(DJ)/i586-pc-msdosdjgpp-as
DOBJCOPY = $(DJ)/i586-pc-msdosdjgpp-objcopy
DTALKD_SRC = dtalkd/dtalkd.c dtalkd/engine_dectalk.c dtalkd/ring.c dtalkd/synth.c dtalkd/coro.S
dtalkd/rmstub_bin.h: dtalkd/rmstub.S
	$(DAS) $< -o dtalkd/rmstub.o && $(DOBJCOPY) -O binary -j .text dtalkd/rmstub.o dtalkd/rmstub.bin
	( echo "/* generated from rmstub.S */"; echo "static const unsigned char rmstub_bin[] = {"; od -An -tx1 -v dtalkd/rmstub.bin | sed 's/ *\([0-9a-f][0-9a-f]\)/0x\1,/g'; echo "};" ) > $@
DTALKD.EXE: $(OBJS_D) $(DTALKD_SRC) dtalkd/rmstub_bin.h
	$(DCC) $(DCFLAGS) -Idtalkd -o $@ $(OBJS_D) $(DTALKD_SRC) -Wl,--wrap=malloc,--wrap=calloc,--wrap=realloc,--wrap=free -lemu
	uv run --script ../espeak/tools/stubsize.py $@ 4096
resident: DTALKD.EXE
.PHONY: resident

# ASK.EXE: timed key prompt for the disk's spoken menu (Open Watcom 16-bit)
WATCOM ?= $(abspath ../../../tools/ow)
dtalkd/ASK.EXE: dtalkd/ask.c
	cd dtalkd && export WATCOM=$(WATCOM) INCLUDE=$(WATCOM)/h PATH=$(WATCOM)/binl64:$$PATH && wcc -ms -0 -zq -w1 -bt=dos -os -fo=ask.obj ask.c && wlink system dos option quiet file ask.obj name ASK.EXE

# stage: the stripped + UPX-packed resident for the talking disk (share/talkpc/, see run/mktalkpc.sh)
UPX ?= $(abspath ../../../tools/upx/upx)
DSTRIP = $(DJ)/i586-pc-msdosdjgpp-strip
stage: DTALKD.EXE dtalkd/ASK.EXE
	cp DTALKD.EXE ../../share/talkpc/DTALKD.EXE && $(DSTRIP) ../../share/talkpc/DTALKD.EXE && $(UPX) -qq --best ../../share/talkpc/DTALKD.EXE
	cp dtalkd/ASK.EXE ../../share/talkpc/ASK.EXE
.PHONY: stage
