# DECtalk for DOS (DTSAY / DTALKD)

Started 2026-09-20 for the "newer PC" leg (2005-2010 laptops such as the Panasonic Toughbook
CF-30/CF-31 with Intel HD Audio, used as talking FreeDOS terminals).  The engine is the
DECtalk 4.x/5.0 source tree from github.com/dectalk/dectalk (develop, 69ebb459, 2026-07-21,
includes the 2026 "restore classic DECtalk sound" retunes), built in its **ARM7 / Epson embedded
configuration**: single-threaded (`SINGLE_THREADED`), no pipes, no audio subsystem, static engine
state, US English dictionary compiled in (`lts/maindict_us.c`, 643 KB of C -> ~160 KB of data),
classic integer Klatt vocal tract (`vtm/`, HLsyn compiled out), 11025 Hz or 8000 Hz 16-bit output
delivered in 71/51-sample blocks through a callback (`api/epsonapi.c`: `TextToSpeechInit`,
`TextToSpeechStart`).  Research: `../../research/05-dectalk-sources.md` (source releases, legal
status), `../../research/04-modern-dos-audio.md` (Toughbook audio, SBEMU, libau).

Licence: the tree is Fonix proprietary (LICENCE); rights holder defunct, community ships builds.
The upstream clone lives in `../dectalk-upstream/` (git-ignored) and is **not** copied into the
public mirror; this directory holds only glue, build files and `patches/upstream.diff`.

## Layout

- `files.txt`          the 67 upstream files compiled (the armtest.dsp list + the armltsus LTS library)
- `glue/`              `stubs.c` (ARM7 maps printf to `error_func_printf`), `config.h` (empty, lsw_main.c wants it),
                       `main_host.c` (dtsay), `main_dos.c` (DTSAY.EXE)
- `dtalkd/`            the resident wrapper: `dtalkd.c` = `../espeak/espkd/espkd.c` with DECtalk as the engine,
                       `engine_dectalk.c` (new), `ring.c synth.c coro.S rmstub.S` copied unchanged from espkd
- `patches/upstream.diff`  3 hunks: `include/port.h` ARM7 typedefs `long`->`int` (LP64 host), `ph/ph_drwt01.c`
                       three log-file blocks under `#ifndef ARM7`.  Apply with `git apply` in the upstream clone.
- `results/`           WAVs and WER numbers

## Build

    make               # host reference ./dtsay [-r 8000] [-o out.wav] text
    make dos           # DTSAY.EXE  (DJGPP; synthesize into memory, print timing, play via ../sbsynth/sb16.c)
    make resident      # DTALKD.EXE (DJGPP; the shell-wrapper TSR, see ../espeak/NOTES.md for the design)
    make install       # copies both + CWSDPMI.EXE into ../../share/dectalk/
    make patch         # refresh patches/upstream.diff

Defines: `-DARM7 -DENGLISH_US -DACNA -DARM7_NOSWI -DENGLISH -Di386 -DWIN32_TEST -DBLD_DECTALK_DLL`
(the armtest.dsp release set; `WIN32_TEST` only drops the ARM `__align(8)` attributes).  The host
build adds `-U__linux__` so port.h takes its generic branch instead of the pthread/Linux one; the
DJGPP build adds `-UMSDOS -U__MSDOS__` so the 1990s 16-bit `#ifdef MSDOS` paths stay off.
Everything compiled first time; the DJGPP link needs `-lemu` only as a safety net (the VTM is
integer, `SamplePeriod` is the one double).

Sizes: DTSAY.EXE 807 KB, DTALKD.EXE 861 KB on disk; runtime heap 12 KB (all engine state is
static: `heap 0 KB locked`), synthesizer stack 10.4 KB of the 160 KB reserved, 0 heap
allocations from interrupt context over a 22-utterance Provox session.

## Results (2026-09-20)

Speed: host 3800x real time; QEMU/KVM 486 model 2300x.  Real 2010 hardware will be far above
real time; the 386 is not a target for this engine.

Intelligibility (Whisper base.en, `../espeak/results/sentences24.txt`, `--stable`):

| Voice | WER |
|---|---|
| **DECtalk (this port) 11025 Hz 16-bit** | **12.4 %** |
| **DECtalk (this port) 8000 Hz 16-bit** | **12.4 %** |
| espeak-ng en+klatt 22050 Hz (native) | 10.4 % |
| ESPK (our eSpeak NG port) 8 kHz 8-bit | 29.5 % |
| SAM 22 kHz | 33 % |

QEMU (`run/boot.sh`, FreeDOS 1.4 LiveCD):
- `DTSAY` speaks through the emulated SB16 (`results/dos.wav`).
- `DTALKD /TEST`: 3 utterances, 0 underruns, interrupts in both protected and real mode
  (`results/dtalkd-test-qemu.wav` -> "Deck talk ready. Test 123. 456 ... 910").
- `DTALKD /C PVTEST.BAT` (Provox 7 + `PV7 LITETALK COM3`): 22 utterances, 0 underruns, Provox
  reads the screen with punctuation names (`results/dtalkd-provox-qemu.wav`).
- **SBEMU on Intel HD Audio** (`SOUND=hda MEM=256 run/boot.sh`, QEMU ich9-intel-hda + hda-duplex):
  `VDPMI /DPMIMEM=32`, `SBEMUV /A220 /I5 /D1 /T6 /O0`, then `DTALKD /TEST` and the Provox batch
  both work: DSP 4.00 reported, DPMI host "0.90 32-bit V86 reflection", 129 virtual SB IRQs
  delivered to our protected-mode handler, 0 underruns (`results/dtalkd-test-vdpmi-hda.wav`,
  `results/dtalkd-provox-vdpmi-hda.wav`).  The classic JEMMEX + QPIEMU + HDPMI32i route also
  loads from the command line (JEMMEX ignores NOEMS there) but was not exercised end to end.
  SBEMU files: `images/sbemu/` (git-ignored), staged in `share/sbemu/` (SBEMU.EXE = beta.6 for
  JEMMEX/HDPMI, SBEMUV.EXE = the VDPMI build).

## DoubleTalk mapping (engine_dectalk.c)

Speed 0..9 -> `[:ra 100+20*s]` words per minute (5 = 200 wpm, DECtalk's default); pitch 0..99 ->
`[:dv ap 60+1.25*p]` Hz (50 = 122 Hz, Paul); volume through the wrapper's output LUT.  Each
utterance is prefixed with those two commands.  A flush (^X) makes the sample callback return
NULL: DECtalk raises `halting`, `TextToSpeechStart` returns ERR_RESET after re-initialising
the static engine state (cheap: init measured at 0 ms).

## Open items

- The ARM7 configuration's voice defaults may differ from the Linux `say` build (7.92 s vs 7.43 s for
  the same sentence; same spectrum); compare with `[:ra 200]` forced on both.
- Direct HD Audio output (libau / MPXPlay au_cards) instead of SBEMU: see research/04 addendum.
- 8-bit DMA only; SB16 16-bit DMA would remove the 8-bit quantisation (WER is already at the
  16-bit figure, so low priority).
- The bootable image for the newer PCs (HIMEMX + VDPMI + SBEMUV + DTALKD + Provox + packet driver).

## The talking disk for newer PCs (talkpc.img)

User decision 2026-09-20: the image only has to bring the machine up with speech; no installer,
no FDISK/FORMAT (the user switches to their own DOS disk once it talks).  Everything fits on one
1.44 MB floppy (also bootable from a USB stick on most BIOSes) with 405 KB to spare:

    run/mktalkpc.sh       # dist/talkpc.img from images/FD14BOOT.img via the LiveCD in QEMU (share/MKTALKPC.BAT)
    BOOT=a FLOPPYIMG=$PWD/dist/talkpc.img SOUND=hda MEM=256 run/freedos.sh    # boot it on emulated HD Audio

Contents (`share/talkpc/`): KERNEL.SYS + COMMAND.COM (FreeDOS 1.4), JEMMEX.EXE 5.84 (XMS + V86
monitor, `NOEMS X2MAX=8192` in FDCONFIG.SYS), JLOAD.EXE + QPIEMU.DLL (QEMM-style port trapping
for real-mode programs), HDPMI32i.EXE (`-r -x`, the DPMI host SBEMU and DTALKD run under; no
CWSDPMI on the disk), SBEMU.EXE (1.0.0-beta.6, UPX 549 -> 246 KB), DTALKD.EXE (stripped + UPX 861 -> 330 KB), ASK.EXE
(timed key prompt, Open Watcom, `dtalkd/ask.c`), PROVOX7.EXE, PV7.EXE, SAY.EXE, FDCONFIG.SYS
(JEMMEX, DOS=HIGH,UMB), FDAUTO.BAT (first boot: SBEMU /O1 speaker ->
`DTALKD COM3 /SAY question` -> `ASK 10`; then SBEMU /O0 headphone; then `DTALKD /SPK`; the
answer is saved as A:\VOICE.BAT), START.BAT (runs inside DTALKD's shell: PROVOX7, PV7 LITETALK
COM3, nested COMMAND.COM; EXIT unloads), HELP.BAT, README.TXT.  `DTALKD /SAY text` is new:
install, speak, wait, uninstall (one shot, for menus).

Debugging the first boots (2026-09-20, all in QEMU, `results/bisect.log`, `bisect2.log`,
`floppytests.log`): the shell went silent after a while and the guest ended halted (HLT with
IF=0 at ring 0 inside VDPMI) or spinning in ring 3 with interrupts off.  Three separate things:

1. **A flush during playback hung the engine.**  Provox sends ^X on every keystroke; the sample
   callback returned NULL, DECtalk's embedded "halting" path re-initialised the engine, and the
   *next* `TextToSpeechStart` looped forever in `ls_util_next_item_new` (reproduced on the host:
   `printf 'x\n' | ./dtsay -a 5 -m text` never returns).  Under HDPMI/VDPMI the client's `sti`
   is virtual, so the coroutine spinning inside the SB interrupt froze the keyboard as well and
   SBEMU kept replaying the last DMA buffer (a 93 ms loop, visible as a constant RMS and a 0.75
   autocorrelation at 4096 samples in the capture).  Fix in `engine_dectalk.c`: never return
   NULL; on a flush discard the rest of the utterance (synthesis runs ~2000x real time).
2. **DSP reset too early.**  `/SAY` and the unload path reset the DSP as soon as the ring was
   drained; SBEMU still had the audio buffered and dropped it (`/SAY` was silent in the LiveCD
   tests, V4/V7/V8).  Fix: wait 1.5 s after the drain before the reset (V9 speaks).
3. **VDPMI itself halts intermittently.**  With both fixes in, one boot in five still ended with
   VDPMI halted at ring 0 (HLT, IF=0, always EIP 50B41Fh, right after the menu choice while the
   second DTALKD instance started).  VDPMI is a closed-source pre-release, so the disk now uses
   JEMMEX 5.84 + QPIEMU.DLL + HDPMI32i (open source, SBEMU's standard stack), which passed the
   bisect (V6) and the idle test.  The VDPMI files stay in `share/sbemu/` (`VDPMI.EXE`,
   `SBEMUV.EXE`) as the alternative to try on hardware if the JEMMEX stack has trouble.  The UMB
   placement of the real-mode stub and the SBEMU re-runs were innocent (TPF1/TPF2/talkpc all
   passed); `/LOW` is kept anyway.

Final image (`dist/talkpc.img`, 15:15, 405 KB free), `results/talkpc-final-qemu.wav`: spoken
menu, key 1, Provox reads the screen, typed keys during speech flush correctly, `HELP` is read,
guest alive.  Level: 8-bit samples through SBEMU at /VOL100 peak around 5000/32767 on QEMU's
codec; the loud burst at DTALKD start (peak 28000, 1 s) is SBEMU replaying stale data when the
DSP restarts, harmless.  `DTVOL=0..9` overrides the start volume.  Harness: `run/testtalkpc.sh
[img]` (boot, answer, RMS per phase, CPU state), `run/mon.sh freedos "info registers"` tells a
dead guest (HLT=1, IF=0, CPL=0) from a live one; never `pkill -f` an unanchored pattern.

## On the 386 DX-25 (DOSBox-X, `run/dosbox/dtsay-*.conf`, 2026-09-20)

`DTSAY -q` on the 24-word test sentence (8.2 s of audio), 386 core at fixed cycles; 5000 cycles
is the setting that matched the real 386 DX-25 for SAM (research/evidence):

| cycles | rate | audio / synthesis | real time |
|---|---|---|---|
| 5000 (386 DX-25) | 11025 Hz | 8.27 s / 8.64 s | 0.95x |
| 5000 (386 DX-25) | 8000 Hz | 8.19 s / 6.27 s | 1.30x |
| 3000 | 11025 Hz | 8.27 s / 14.47 s | 0.57x |
| 12000 (486 class) | 11025 Hz | 8.27 s / 3.59 s | 2.30x |

So DECtalk is 2-3x cheaper than our fixed-point Klatt port (0.50x measured on the real 386) and
is borderline real time on the 386 DX-25: at 8 kHz it would keep up with a screen reader (the
resident only needs to stay ahead of the DMA, and text-rate work is small), at 11 kHz it would
fall behind on long text.  Unverified on the real machine: DOSBox's 386 model was calibrated with
16-bit real-mode code (SAM); 32-bit DJGPP code may scale differently.  Memory: DTALKD locks about
1.2 MB (the 8 MB machine is fine).  `DTALKD /8K` selects 8000 Hz.  VDPMI needs a Pentium, so on
the 386 the resident runs under CWSDPMI with a real Sound Blaster, exactly like ESPKD (DR-DOS:
EMM386 /DPMI=ON or CWSDPMI with 2 MB XMS free).  A `TALKD`-style batch on the 386 test floppy is
the next step there; the 861 KB EXE needs the second floppy or the hard disk.

## Disk 2 (espk386.zip) carries DECtalk too (2026-09-20)

`run/mkdist.sh` packs ESPK.EXE and ESPKD.EXE with UPX (49 %) so that DTALKD.EXE (packed), the
SBEMU stack (SBEMU.EXE packed, HDPMI32i, JLOAD, QPIEMU.DLL) and `DTALK.BAT` / `DTALK2.BAT` fit
next to the eSpeak voice: 1,415 KB of the 1,457 KB on a 1.44 MB floppy.  `DTALK` runs straight
from the disk's drive: DTALKD first (real Sound Blaster, CWSDPMI from the disk), and if it exits
with errorlevel 1 (no DSP) loads HDPMI32i and SBEMU and tries again; `DTALK SBEMU` forces that,
`DTALK SPK` is the PC speaker.  On an already booted DOS the SBEMU real-mode port trapping is
missing unless JEMMEX + QPIEMU are loaded, but DTALKD is a protected-mode client, so HDPMI32i
alone is enough for it.  `dist/talkpc.zip` = the files of talkpc.img.

Disk 2 QEMU tests (`results/disk2-dtalk-*.wav`): `DTALK` from the LiveCD with the SB16 loads
DTALKD directly; with the ICH9 HDA it reports "no Sound Blaster DSP", loads HDPMI32i + SBEMU and
comes up (DSP 4.00); with Provox in C:\SPEECH it reads the screen; the packed ESPKD passes /TEST.
Two hours were lost to a stale `share/speech/` (phase-1 copies of Provox with a different saved
configuration; Provox stores its settings in its own EXE): every batch variant that found that copy
"hung" the serial console.  Keep `share/speech/` in step with `share/PROVOX7.EXE`.
