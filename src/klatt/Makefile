# Klatt fixed-point engine + rsynth-derived frontend, host build.
CC      ?= gcc
CFLAGS  ?= -O2 -std=gnu99 -Wall -Wextra -Wno-unused-parameter -Wno-implicit-fallthrough
UP      = upstream/klatt-3.04
INC     = -I. -Ihost -I$(UP)

ENGINE  = klatt_fx.c
FRONT   = holmes.c elements_tab.c nrl.c phtoelm_tab.c

all: klattsay parity

klattsay: host/klattsay.c $(ENGINE) $(FRONT) host/wavout.c
	$(CC) $(CFLAGS) $(INC) -o $@ $^ -lm

parity: host/parity.c $(ENGINE) host/wavout.c $(UP)/parwave.c
	$(CC) $(CFLAGS) $(INC) -o $@ $^ -lm

# overflow-audited build of the parity harness (64-bit check of every resonator sum)
parity_chk: host/parity.c $(ENGINE) host/wavout.c $(UP)/parwave.c
	$(CC) $(CFLAGS) -DKFX_CHECK $(INC) -o $@ $^ -lm

# float-vs-fixed parity on the committed tracks (results/*.par)
check: parity_chk
	./parity_chk -r 8000  -n 3 results/hello8k.par  results/parity_hello8k_ref.wav  results/parity_hello8k_fx.wav
	./parity_chk -r 11025 -n 4 results/hello11k.par results/parity_hello11k_ref.wav results/parity_hello11k_fx.wav
	./parity_chk -r 8000  -n 4 results/synth8k.par  results/parity_synth8k_ref.wav  results/parity_synth8k_fx.wav
	./parity_chk -r 11025 -n 5 results/synth11k.par results/parity_synth11k_ref.wav results/parity_synth11k_fx.wav

# the demo sentence
hello: klattsay
	./klattsay -l 1 "Hello. This is the Klatt synthesizer speaking on Free DOS." results/hello8k_src1x.wav

# tables are committed; regenerate with:
tables:
	uv run tools/gen_tables.py klatt_tab.h
	uv run tools/gen_elements.py upstream/rsynth/Elements.def elements_tab.c

clean:
	rm -f klattsay parity parity_chk *.o

.PHONY: all clean tables check hello

# upstream rsynth (float, its own synth) as the intelligibility reference:
#   ./rsynth_say -r 8000 -l out.raw "text"   (raw s16le; see tools/rsynth_wav.sh)
RS      = upstream/rsynth
RSSRC   = $(RS)/opsynth.c $(RS)/elements.c $(RS)/holmes.c $(RS)/trie.c \
          $(RS)/phtoelm.c $(RS)/darray.c $(RS)/say.c $(RS)/phones.c \
          $(RS)/text.c $(RS)/english.c $(RS)/deutsch.c $(RS)/charset.c \
          $(RS)/l2u.c $(RS)/aufile.c $(RS)/getarg.c $(RS)/Revision.c \
          $(RS)/dict.c $(RS)/saymain.c host/rsynth/play.c
rsynth_say: $(RSSRC)
	$(CC) -O2 -w -Ihost/rsynth -I$(RS) -DDICT_DIR=\"/nonexistent\" -DDEFAULT_DICT=\"\" -o $@ $(RSSRC) -lm
