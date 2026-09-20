WATCOM ?= $(abspath ../../../tools/ow)
export WATCOM
export INCLUDE = $(WATCOM)/h
BIN = $(WATCOM)/binl64
export PATH := $(BIN):$(PATH)
comma := ,
empty :=
space := $(empty) $(empty)
WCC = $(BIN)/wcc -ms -0 -j -zq -w1 -bt=dos -za99 -s -DHAVE_RETRO
WASM = $(BIN)/wasm -ms -zq
WLINK = $(BIN)/wlink
OBJS = dos.obj ring.obj synth.obj coro.obj engine_sam.obj sam_sam.obj sam_render.obj sam_reciter.obj engine_retro86.obj retro_tab86.obj nrl86.obj
KL = ../klatt
KOBJS = dos3.obj ring3.obj synth3.obj coro.obj engine_sam3.obj engine_klatt.obj sam_sam3.obj sam_render3.obj sam_reciter3.obj kl_fx.obj kl_holmes.obj kl_nrl.obj kl_elements.obj kl_phtoelm.obj kl_imul.obj kl_res.obj engine_retro.obj retro_tab.obj
WCC3 = $(BIN)/wcc -ms -3 -j -zq -w1 -bt=dos -za99 -s -DHAVE_KLATT -DHAVE_RETRO

XOBJS = dosxt.obj ring.obj synth.obj coro.obj engine_samxt.obj engine_retro86.obj retro_tab86.obj nrl86.obj
WCCX = $(WCC) -DNO_SAM

all: SBTALK.EXE SBTALK3.EXE SBTALKXT.EXE SAY.EXE
# XT build: 1983 voice only (SAM is 15x too slow on an 8088), same options otherwise
SBTALKXT.EXE: $(XOBJS)
	$(WLINK) system dos option quiet,map,stack=2048 file $(subst $(space),$(comma),$(strip $(XOBJS))) name $@
dosxt.obj: dos.c ring.h synth.h engine.h ; $(WCCX) -os -fo=$@ dos.c
engine_samxt.obj: engine_sam.c engine.h ; $(WCCX) -os -fo=$@ engine_sam.c
SBTALK3.EXE: $(KOBJS)
	$(WLINK) system dos option quiet,map,stack=2048 file $(subst $(space),$(comma),$(strip $(KOBJS))) name $@
dos3.obj: dos.c ring.h synth.h engine.h ; $(WCC3) -os -fo=$@ dos.c
ring3.obj: ring.c ring.h ; $(WCC3) -ox -fo=$@ ring.c
synth3.obj: synth.c ring.h synth.h engine.h ; $(WCC3) -os -fo=$@ synth.c
engine_sam3.obj: engine_sam.c engine.h ; $(WCC3) -os -fo=$@ engine_sam.c
engine_klatt.obj: engine_klatt.c engine.h $(KL)/klatt_fx.h $(KL)/holmes.h $(KL)/nrl.h ; $(WCC3) -ox -fo=$@ engine_klatt.c
sam_sam3.obj: sam/sam.c ; $(WCC3) -ox -fo=$@ sam/sam.c
sam_render3.obj: sam/render.c ; $(WCC3) -ox -fo=$@ sam/render.c
sam_reciter3.obj: sam/reciter.c ; $(WCC3) -ox -fo=$@ sam/reciter.c
kl_fx.obj: $(KL)/klatt_fx.c $(KL)/klatt_fx.h $(KL)/klatt_tab.h ; $(WCC3) -ox -fo=$@ $(KL)/klatt_fx.c
kl_holmes.obj: $(KL)/holmes.c $(KL)/holmes.h ; $(WCC3) -ox -fo=$@ $(KL)/holmes.c
kl_nrl.obj: $(KL)/nrl.c $(KL)/nrl.h $(KL)/nrl_rules.inc ; $(WCC3) -ox -fo=$@ $(KL)/nrl.c
engine_retro.obj: engine_retro.c engine.h ring.h ; $(WCC3) -os -fo=$@ engine_retro.c
retro_tab.obj: ../retro/retro_tab.c ; $(WCC3) -os -fo=$@ ../retro/retro_tab.c
../retro/retro_tab.c: ../retro/mkretro.py ../horndrv/src/PHONEME.C ; cd ../retro && uv run mkretro.py --dump retro_tab.c
kl_res.obj: $(KL)/res386.asm ; $(WASM) -fo=$@ $(KL)/res386.asm
kl_imul.obj: $(KL)/imul32.asm ; $(WASM) -fo=$@ $(KL)/imul32.asm
kl_elements.obj: $(KL)/elements_tab.c ; $(WCC3) -os -fo=$@ $(KL)/elements_tab.c
kl_phtoelm.obj: $(KL)/phtoelm_tab.c ; $(WCC3) -os -fo=$@ $(KL)/phtoelm_tab.c
SAY.EXE: say.c ; $(WCC) -os -fo=say.obj say.c && $(WLINK) system dos option quiet file say.obj name $@
SBTALK.EXE: $(OBJS)
	$(WLINK) system dos option quiet,map,stack=2048 file $(subst $(space),$(comma),$(strip $(OBJS))) name $@
engine_retro86.obj: engine_retro.c engine.h ring.h ; $(WCC) -os -fo=$@ engine_retro.c
retro_tab86.obj: ../retro/retro_tab.c ; $(WCC) -os -fo=$@ ../retro/retro_tab.c
nrl86.obj: $(KL)/nrl.c $(KL)/nrl.h $(KL)/nrl_rules.inc ; $(WCC) -os -fo=$@ $(KL)/nrl.c
dos.obj: dos.c ring.h synth.h engine.h ; $(WCC) -os -fo=$@ dos.c
ring.obj: ring.c ring.h ; $(WCC) -ox -fo=$@ ring.c
synth.obj: synth.c ring.h synth.h engine.h ; $(WCC) -os -fo=$@ synth.c
engine_sam.obj: engine_sam.c engine.h ; $(WCC) -os -fo=$@ engine_sam.c
coro.obj: coro.asm ; $(WASM) -fo=$@ coro.asm
sam_sam.obj: sam/sam.c ; $(WCC) -ox -fo=$@ sam/sam.c
sam_render.obj: sam/render.c ; $(WCC) -ox -fo=$@ sam/render.c
sam_reciter.obj: sam/reciter.c ; $(WCC) -ox -fo=$@ sam/reciter.c
sam/sam.c sam/render.c sam/reciter.c: mksam.py ../sam-upstream/src/*.c ; python3 mksam.py
.PHONY: all host install clean
host: sbtalk_host
sbtalk_host: host.c ring.c synth.c engine_sam.c engine_klatt.c sam/sam.c sam/render.c sam/reciter.c
	gcc -O2 -fcommon -w -DHAVE_KLATT -DHAVE_RETRO -o $@ host.c ring.c synth.c engine_sam.c engine_klatt.c engine_retro.c ../retro/retro_tab.c sam/sam.c sam/render.c sam/reciter.c $(KL)/klatt_fx.c $(KL)/holmes.c $(KL)/nrl.c $(KL)/elements_tab.c $(KL)/phtoelm_tab.c
install: SBTALK.EXE SBTALK3.EXE SBTALKXT.EXE SAY.EXE ; cp SBTALK.EXE SBTALK3.EXE SBTALKXT.EXE SAY.EXE ../../share/
clean: ; rm -f *.obj *.o *.lst SBTALK.EXE SBTALK3.EXE SBTALKXT.EXE SAY.EXE SBTALK.map SBTALK3.map SBTALKXT.map sbtalk_host
