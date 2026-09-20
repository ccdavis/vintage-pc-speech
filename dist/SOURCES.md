# Accessible DOS talking disk: source distribution

This bundle holds every source we wrote or modified for the talking disk, plus the build harness.
Binaries are distributed separately (a11y386.zip, espk386.zip, talkdisk.img, talkpc.img, talkpc.zip). Build hosts: Linux
with the cross toolchains listed below; DOS builds were tested under QEMU, DOSBox-X, a 386 DX-25
(DR-DOS 7.02) and are aimed at any PC from an XT up.

| Directory | What | Origin and licence |
|---|---|---|
| `src/sbtalk/` | SBTALK, the resident synthesizer (virtual DoubleTalk on INT 14h, Sound Blaster DMA and PC speaker outputs, engines SAM / Klatt / 1983 voice), SAY.EXE, host harness | ours (GPL v3+ as a whole, because of the engines below) |
| `src/sbtalk/sam/`, `src/sbtalk/mksam.py` | SAM engine, generated from the vendored `src/sam-upstream` with streaming output, 16-bit fixes and the bug fixes of the review | SAM: reverse-engineered 1982 abandonware, no licence (github.com/s-macke/SAM). Prototype and fallback voice only; replaceable |
| `src/klatt/` | Fixed-point Klatt synthesizer (`klatt_fx.c`), rsynth-derived English rules and Holmes tables, 386 assembly hot paths, parity and WER tools | klatt 3.04 port: GPL v3+ (Jon Iles / Nick Ing-Simmons); rsynth parts: LGPL v2+; ours: GPL v3+ |
| `src/dectalk/` | DECtalk built for DOS: build files, `glue/` (host and DOS front ends), `dtalkd/` (the resident wrapper, engine glue, ASK.EXE), `patches/upstream.diff` (3 hunks); `run/get-dectalk.sh` fetches the engine | engine: github.com/dectalk/dectalk (develop 69ebb459), Fonix proprietary LICENCE, not included here; our glue GPL v3+ |
| `share/talkpc/` | Boot files of the talking disk for newer PCs (FreeDOS + JEMMEX 5.84 + HDPMI32i + SBEMU 1.0.0-beta.6 + DECtalk + Provox) | JEMMEX/HDPMI: Japheth (Artistic/GPL), SBEMU: GPL v2 (github.com/crazii/SBEMU); binaries only in the release zips |
| `src/espeak/lib/`, `espk/`, `espkd/`, `tools/`, `Makefile` | eSpeak NG subset (18 files, small diff in `results/lib-vs-upstream.diff`), the integer frame-to-Klatt mapping, ESPK.EXE and the resident ESPKD.EXE shell wrapper | eSpeak NG: GPL v3+ (commit noted in NOTES.md); ours: GPL v3+ |
| `src/retro/` | The 1983 SPEECH.COM voice as an engine: table generator from horndrv's PHONEME.C | data: horndrv (Jon Hornstein, "public domain as freeware" + GPL); ours: GPL v3+ |
| `src/horndrv/` | horndrv / TALK.SYS 1992-99 source as retrieved (reference for the 1983 voice) | Jon Hornstein, freeware + GPL text included |
| `src/sbsynth/` | DJGPP Sound Blaster test programs (SBPLAY, SAMSB) | ours, GPL v3+ (SAMSB embeds SAM, see above) |
| `src/provox7/` | Provox 7.03 screen reader source with one fix (`2I8CODE.A`: undefined label), 7.05 binaries, A86 assembler and its manual as shipped in provox7.zip | Charles E. Hallenbeck / Kansys, GPL v2 or later; A86 is Eric Isaacson's shareware, redistributed as in the original package |
| `share/talkdisk/` | Talking disk files: FDAUTO.BAT (first-boot menu, CPU branch), HELP, BLASTER, RESETVOI, README | ours |
| `dist/*.BAT`, `dist/README.TXT` | Test floppy batches (GO, TALK, TALKD, BENCH, ESPK, DOSTEST, PROVOX) | ours |
| `run/` | QEMU/DOSBox-X harness, disk and package builders, regression tests | ours |
| `PLAN.md`, `README.md`, `research/*.md` | Findings, measurements, plan | ours |

Not included: FreeDOS (get FD14-LiveCD.zip from freedos.org; the talking disk uses its 8086 kernel,
COMMAND.COM, HIMEMX, EDIT, MEM), the DJGPP and Open Watcom toolchains, CWSDPMI (FreeDOS package;
ESPKD ships a copy with paging disabled, see `src/espeak/tools/cwsnoswap.py`), the eSpeak NG git
clone and its host build, TRAN and SPEECH.COM binaries (research only).

## Building

- Toolchains: DJGPP cross compiler (build-djgpp v3.4, gcc 12.2) at `tools/djgpp`, Open Watcom 2.0
  Linux snapshot at `tools/ow`, `nasm`/`uv`/`gcc` on the host, DOSBox-X for the A86 step.
- `make -C src/sbtalk install` builds SBTALK.EXE (8086), SBTALK3.EXE (386, adds Klatt),
  SBTALKXT.EXE (XT, 1983 voice only), SAY.EXE; `make host` builds the host harness.
- `make -C src/espeak dos && make -C src/espeak ESPKD.EXE && make -C src/espeak install` builds
  ESPK.EXE and ESPKD.EXE; `tools/mkdata_en.sh` compiles ESPK.DAT from an eSpeak NG checkout.
- `make -C src/klatt check` runs the fixed-point parity audit; `src/klatt/tools/wer_eval.py` scores
  intelligibility with Whisper.
- Provox: assemble with the bundled A86 under DOSBox-X (`src/provox7/build/pv.conf` pattern) and link
  with Open Watcom's `wlink system dos`.
- `run/mkdist.sh` makes the two floppy zips, `run/mktalkdisk.sh` the bootable talking disk (needs the
  FreeDOS boot floppy image), `run/mksrcdist.sh` this bundle.
