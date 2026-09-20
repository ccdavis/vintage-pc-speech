# FreeDOS on a 386 DX-25: screen reader plus a software synthesizer through a Sound Blaster 32

Date: 2026-09-19. Status: feasibility verified under emulation and on the real machine (Phase 1 done).

## 0. Results from the real machine (2026-09-19)

The Phase 1 package (`dist/a11y386.zip`, run blind with `A:\GO`) was run on the target. The
machine turned out to run **Caldera DR-DOS 7.02 (1998)**, not FreeDOS, with 8 MB RAM (598 KB
conventional free, 7 MB XMS), EMM386-style memory management (EMMQXXX0, DPMSXXX0) and
**JAWS for DOS installed at C:\JAWS**. The Sound Blaster reports DSP 4.16 (SB32/AWE32 family),
BLASTER=A220 I5 D1 H5 P330 E620 T6. Full log: `research/evidence/386dx25-results-2026-09-19.txt`.

| Test | Result |
|---|---|
| 440 Hz tone, 84 KB WAV via DMA | played, clean |
| SAM, 4.39 s of audio | 1.80 s, **2.4x real time** (turbo on) |
| SAM, 2.86 s of audio, fast rate | 1.20 s, 2.4x real time |
| SAM, long sentence | no output: the batch line exceeded DR-DOS's 127-character command limit (fixed) |

So SAM-class synthesis uses about 42 % of the 386 DX-25 at 22.05 kHz, and DOSBox-X at 5000 fixed
cycles (2.4x) is a faithful stand-in for this machine with the turbo button pressed (a first run
with turbo off was noticeably slower). Everything works under DR-DOS as well as FreeDOS; the 32-bit
DJGPP programs ran fine under its DPMI setup.

Calibration warning for the engine choice: the research report estimated SAM at under 10 % of
the CPU, but it measured 42 %. Its estimate for a fixed-point Klatt (25-55 %) rests on the same
arithmetic and should be read as roughly 100-200 %, i.e. a full Klatt at 11 kHz is likely **not**
real time on this machine. The Klatt step must start with a cost measurement (host callgrind
instruction counts scaled by SAM's measured 1.9 M instructions per audio second ≈ 42 %), and
plan on 8 kHz, fewer resonators, and a cheaper voicing source.

Target: the user's 386 DX-25 (assume no FPU, 4-16 MB RAM), FreeDOS 1.4, Sound Blaster 32 already
configured (SB16-compatible DSP at the usual 220/IRQ 5/DMA 1; the EMU8000 wavetable is not used).
Goal: a DOS screen reader and a software speech synthesizer that plays through the SB32, more modern
than the First Byte SmoothTalker engine behind Dr. Sbaitso.

## 1. Recommendation

- **Screen reader: Provox 7** (Kansys / Charles E. Hallenbeck). It is the only DOS screen reader with
  published source under a free license (GPL v2 or later, stated in provox7.doc). It is ~146 KB of
  A86 assembly in ten files, loads as a device driver or TSR, hooks INT 05/08/09/10/16/17/2F, polls
  text video memory from the timer tick, and sends plain text plus DoubleTalk-style control codes to
  a synthesizer through BIOS INT 14h (serial) or INT 17h (parallel). It has been rebuilt from source
  today and runs on FreeDOS 1.4 (details below). ASAP and JAWS for DOS are binary-only; Vocal-Eyes
  "made free" is unconfirmed; Tinytalk is redistributable shareware without source. Full survey:
  `research/01-dos-screen-readers.md`.
- **Synthesizer: a new resident "virtual DoubleTalk"**. A small TSR that emulates a serial
  synthesizer on a chosen COM port by hooking INT 14h, parses the DoubleTalk command set (`^A<n>P`
  pitch, `^A<n>S` speed, `^A<n>V` volume, `^X` flush, etc., exactly what Provox emits today), and
  renders speech into a Sound Blaster auto-init DMA double buffer from the SB interrupt. Because it
  looks like a DoubleTalk/LiteTalk on a COM port it works unmodified with Provox, ASAP, JAWS,
  Tinytalk and Vocal-Eyes, and it is what SvarDOS users already do with a real Braille 'n Speak.
- **Engine, two steps.** Step 1: SAM (the 1982 formant engine, 3.4 K lines of integer C, no FPU,
  fits easily in a 386 budget) as the proof-of-concept engine and as the source of an English
  text-to-phoneme "reciter". Its licence is unclear (reverse-engineered abandonware), so it is a
  stepping stone, not the product. Step 2: **a fixed-point Klatt** formant synthesizer, ported from
  klatt 3.04 `parwave.c` (GPL, 1.2 K lines, float today) to 16.16/Q30 integer arithmetic using the
  386's 32-bit IMUL, at 8-11 kHz, driven by rsynth's Holmes phoneme rules or a table. This is the
  "more modern" voice the user asked for and the research estimates 25-55 % of a 386 DX-25 at 8-11
  kHz. eSpeak NG is integer-core but measured at ~9 M instructions per second of audio on the host,
  2-3x more than a 386-25 can give; it is the voice for a 486 upgrade path (as a DJGPP process,
  not a TSR). Full survey: `research/02-dos-software-speech.md`.

## 2. What was verified hands-on today (QEMU 8.2, KVM, and DOSBox-X for cycle-limited timing)

Harness in `run/` (see README.md). All artefacts under `research/evidence/`.

1. FreeDOS 1.4 LiveCD (sha256 verified) boots headless in QEMU with an emulated SB16; the DOS console
   is driven from the host over COM1 (`CTTY COM1`), files reach the guest through QEMU's virtual FAT
   drive, and the card's output is captured to a WAV, which is the automated "did it speak" oracle.
2. A DJGPP program (`src/sbsynth/sb16.c`) resets and detects the DSP (v4.05 under QEMU), programs
   the 8237 DMA controller and plays 8-bit PCM. A 440 Hz test tone came out at 441 Hz by FFT of the
   capture, and an 84 KB SAM WAV played correctly.
3. `SAMSB.EXE` synthesizes text with SAM and plays it straight to the card. Whisper transcribes the
   captured audio as recognisable English ("Hello, Mrs. Sam's speaking on a three-eighty-six...").
4. CPU budget for SAM, measured: 9.0 M instructions for 4.83 s of audio on the host (callgrind,
   x86-64 build), i.e. ~1.9 M instructions per second of audio. Under DOSBox-X with a 386 core at
   fixed cycles, `SAMSB` produced 4.83 s of audio in 3.35 s at 3000 cycles, 2.00 s at 5000 cycles
   (about a 386 DX-25) and 1.25 s at 8000 cycles. SAM is therefore real time with headroom on the
   target. eSpeak NG on the same sentence: 38.8 M instructions for 4.15 s plus 13 M start-up, so
   ~9.3 M per second of audio: not real time on a 386-25.
5. Provox 7.03 source (provox7.zip from the Historical Access Preservation Project) assembles with the
   bundled A86 3.22 under DOSBox-X after one fix: `2I8CODE.A` line 212 jumped to an undefined label
   `i8mov1`; it now jumps to the cursor-routing code at `l2`. The object links with Open Watcom's
   `wlink` (`system dos`), giving a 23.6 KB PROVOX7T.EXE. The shipped binary is 7.05, newer than the
   7.03 source, so byte comparison is not possible; both behave the same in the tests below.
6. Both the shipped and the rebuilt Provox load as a TSR on FreeDOS 1.4 in QEMU, are activated with
   `PV7 LITETALK COM2`, and stream speech to COM2: the DoubleTalk init sequence
   `^A@ ^A50P ^A5F ^A1X ^AM ^AT ^A7S ^A5V ^A6B`, then the screen backlog, then every new line written
   to the screen ("Volume in drive C is QEMU VVFAT ... Provox reads the screen on Free DOS"), with
   punctuation spoken and `^X` flushes on key presses. Captured in
   `research/evidence/provox-com2-stream.log`. Provox reading is unaffected by the console being
   redirected to COM1 as long as output is sent to the screen (`command > CON`).

Caveats: QEMU under KVM says nothing about speed, which is why DOSBox-X's fixed cycles were used;
neither is cycle accurate (86Box would be closer). The real 386 is the only true test, and the
test package below is designed to be run on it first.

## 2b. Phase 2 status (2026-09-19)

`src/sbtalk/` is the resident synthesizer: Open Watcom 16-bit small model, 29 KB EXE, ~50 KB
resident (20 KB code, 8 KB tables, 19 KB data of which 4 KB sample ring, 4 KB text ring, 6 KB synth
stack) plus an 8 KB DOS block for the DMA buffer. Structure: `dos.c` (SB16/SB Pro auto-init DMA
driver, IRQ handler, INT 14h emulation, TSR install, `/TEST` diagnostics), `coro.asm` (a 15-line
stack switch: the synthesizer runs as a coroutine resumed from the Sound Blaster interrupt and
yields whenever the ring is full), `ring.c`, `synth.c` (DoubleTalk command parser and utterance
segmentation), `sam/` (generated from the vendored SAM by `mksam.py`: streaming output, 32-bit
positions, 16-bit-safe phase math, no stdio). The host build `sbtalk_host` runs the same parser
and engine to a WAV; its output is byte-identical to upstream SAM for the first utterance.

Verified in QEMU: `SBTALK COM3` speaks "S B talk ready" on load; with `PROVOX7` + `PV7 LITETALK
COM3` Provox reads the screen through it (capture in `research/evidence/sbtalk-provox-qemu.wav`,
Whisper recognises the boot messages). Under DOSBox-X at 5000 and 3000 fixed cycles the ring stays
full while an utterance renders (no underruns), and the pre-SB16 DSP path works on emulated SB 2.0
(DSP 2.01) and SB Pro (DSP 3.02) cards (`run/dosbox/sbtalk-*.conf`). Lessons: Watcom's stack-overflow check must be off (`-s`)
for code on a private stack, `int` is 16-bit so SAM's `phase*256` math had to be made unsigned,
and the coroutine pragma must declare the caller-saved registers as clobbered.

**Talking disk (2026-09-19).** `run/mktalkdisk.sh` builds `dist/talkdisk.img`, a bootable FreeDOS 1.4
floppy (FreeDOS kernel, COMMAND.COM, HIMEMX, EDIT, MEM, SYS, FORMAT, SBTALK, SAY, Provox, SBPLAY,
634 KB free) whose FDAUTO.BAT loads SBTALK and Provox. Booted in QEMU it says "S B talk ready.
The talking disk is ready. Type help and press enter for a list of commands", then Provox speaks
everything that appears on screen; `HELP` gives a spoken command list; `SAY` speaks from batch
files (`research/evidence/talkdisk-boot-qemu.wav`). `SAY /X` and any keypress in Provox flush
speech (verified: a 7 s utterance cut at ~2 s on the host harness, ~3 s through DOS).

**Footprint and TSR review (2026-09-20).** Resident size measured with `MEM /C` under DOSBox-X
(`src/sbtalk/memtest/`): SAM build 99 KB -> **81 KB**, Klatt (SBTALK3) 123 KB -> **103 KB**. `dos.c`
now avoids the C runtime's stdio/heap (own `tprintf` over INT 21h/40h, `main(void)` with a PSP
command-tail parser, BLASTER read straight from the environment block) and frees the environment
block before `_dos_keep`; the DMA block is right-sized to `2*HALF` (4 KB SAM, 2 KB Klatt, was a
fixed 8 KB) and is force-allocated in conventional memory so `LH SBTALK` loads the image into a UMB
while DMA stays below 1 MB. Synth stack cut 6 KB -> 4 KB (painted; host and DOSBox high-water < 200 B).
New `SBTALKXT.EXE` (`-DNO_SAM`, 1983 voice only) is **62 KB** resident for the XT/games branch.
Bugs fixed in the resident half: `spk_calibrate` 32-bit overflow on an XT (el*838095 wrapped, giving a
wrong bit delay on the very machine the path targets); PIT-timer EOI/BIOS-tick chaining reworked so
exactly one EOI is sent and the 18.2 Hz tick is chained on every 64K wrap; torn 32-bit reads of
`ring_consumed`/`ring_committed` closed (double-read + cli/sti); `text_get` vs `text_clear` race; RTC
NMI left enabled on teardown; PIT channel-0 restored to BIOS mode 3; DoubleTalk parser `^A^A` and
`^X`-inside-command handled; `/SAY` while another SBTALK is resident now speaks through it (INT 14h
AH=F7h identity probe) instead of double-installing. `_dos_keep` size now derived from `_STACKLOW`
(end of BSS) rather than the live SP. Host WAV output is byte-identical to before for SAM, Klatt,
1983 and the flush test. Blocking XT-speaker bit-bang still only verifiable on real hardware
(DOSBox executes port-61h writes at ~1 ms, far too slow to finish an utterance).

## 3. Architecture

```
 keyboard ──INT 09/16h──┐                      ┌── INT 14h hook (COMn) ──┐
 screen  ──B800:0000────┤  PROVOX7 (TSR, asm)  │  SBTALK2 (TSR, C+asm)  │── 8237 DMA ──► SB DSP ──► speaker
 timer   ──INT 08h──────┘   text + ^A cmds ───►│  parse ▸ text→phoneme   │
                                               │  ▸ synth into ring buf  │◄── SB IRQ 5: refill next half
                                               └─────────────────────────┘
```

SBTALK2 (working name) components:

1. **INT 14h emulation** for one COM number: AH=00 init (accept), AH=01 send byte (queue, return
   status with THRE set), AH=02 receive (nothing), AH=03 status (CTS/DSR/THRE set; report "busy"
   by clearing THRE only if the queue is full so Provox's XON/XOFF and CTS logic is satisfied).
   Optionally also serve the DoubleTalk PC INT 2Fh interface later.
2. **Command parser**: DoubleTalk `^A<digits><letter>` commands, `^X` flush (stop audio, clear
   queue), text buffered until a phrase delimiter or timeout.
3. **Text to phonemes**: start with SAM's reciter rules (English letter-to-sound rules, ~1 K lines);
   later the rsynth/Holmes rule set or a compact dictionary. Number, punctuation and symbol
   expansion is done partly by Provox already ("colen", "period").
4. **Synthesizer**: SAM render first; then fixed-point Klatt (Q16/Q30, 32-bit IMUL, 8 or 11 kHz).
   Synthesis runs in the SB IRQ handler for the next buffer half (with interrupts re-enabled after
   EOI so the timer and keyboard keep working), so the foreground DOS program keeps whatever CPU
   is left. A float-vs-fixed parity test on the host guards the Klatt port.
5. **Output**: SB16 auto-init 8-bit DMA (DSP 41h rate, C6h+mode+len-1), buffer in conventional
   memory not crossing a 64 KB page, IRQ acknowledged at 22Eh. Detection from the BLASTER variable.
6. **Footprint**: resident target under 64 KB, loadable high with EMM386/JEMM; SmoothTalker's 180 KB
   is why blind users rejected the Sound Blaster route in the 1990s.

Toolchain: Open Watcom 2.0 (Linux cross build in `../tools/ow`) for 16-bit real-mode C with `-3`
for 386 instructions, `wasm`/A86 for the hooks, DJGPP (`../tools/djgpp`) for non-resident test
programs like the ones already written.

## 4. Phases

- **Phase 0 (done today)**: harness, SB16 output path, SAM real-time proof, Provox rebuilt and running
  on FreeDOS in QEMU with its speech stream captured.
- **Phase 1 (done 2026-09-19, see section 0): hardware truth.** Package `SBPLAY.EXE`, `SAMSB.EXE`, `CWSDPMI.EXE`, `PROVOX7.EXE`,
  `PV7.EXE` for the real 386 (a floppy image or a zip to copy over). Run `SBPLAY -tone`, then
  `SAMSB some text` and read the printed synthesis time; run Provox with `PV7 LITETALK COM2` and a
  null-modem cable to a laptop running espeak-ng if available. This settles the CPU budget and the
  SB32 DSP behaviour before any TSR work.
- **Phase 2: SBTALK2 with SAM.** INT 14h virtual COM port, DoubleTalk parser, SAM reciter and
  render in a Watcom 16-bit TSR, SB auto-init DMA from the IRQ. Test in QEMU: load SBTALK2, load
  Provox with `PV7 LITETALK COM3`, write lines to the screen, transcribe the captured WAV with
  Whisper. Then the real machine.
- **Phase 2b: hardware.** TALK.BAT on the test floppy, then the talking disk itself on the 386 and on
  any other DOS machine with a Sound Blaster; JAWS for DOS on the 386 configured for a DoubleTalk LT on
  COM3 as a second reader. Tune: utterance segmentation and latency, Provox speed/pitch mapping,
  SB Pro/2.0 path (DSP < 4) on real hardware, memory (load SBTALK high with JEMM386).
- **Phase 3: fixed-point Klatt engine.** Status 2026-09-19: engine and frontend done on the host
  (`src/klatt/`, see NOTES.md there: klatt 3.04 parwave ported to Q12/Q30 integers, 37.7 dB SNR
  against the float original at 8 kHz, rsynth NRL letter-to-sound rules and Holmes element tables
  ported to integers, Whisper word error rate on par with SAM at 8 kHz). Integrated into SBTALK3
  (`/KLATT`, 8000 Hz, 62 KB EXE, ~85 KB resident) and verified speaking through DOS in QEMU.
  Speed: host callgrind 2.4 M instructions per audio second (fits the SAM-calibrated budget), but
  Watcom's 16-bit code for 32-bit arithmetic is 5-6x worse than the host compiler: DOSBox-X at the
  386 DX-25 cycle count gave 0.32x real time in plain C, 0.60x with the resonators and multiplies
  in 386 assembly (`src/klatt/res386.asm`, `imul32.asm`; SAM benches 1.9x on the same run). The
  rest of the per-sample path (glottal source, parallel branch, output) is still C and is the next
  assembly target; Measured on the real 386 DX-25 (`research/evidence/386dx25-bench-2026-09-19.txt`): SAM 2.1x,
  Klatt **0.50x** real time, and both voices judged barely intelligible by ear through Provox, Klatt
  slightly worse. Intelligibility investigation (host, Whisper word error rate on 24 sentences, see
  `src/klatt/NOTES.md` "Intelligibility"): the port is faithful (frame-exact frontend, upstream
  rsynth's own float synth scores *worse*, 74 % WER); the 8 kHz / lite-source / 8-bit cuts cost
  nothing measurable; the real limiter is rsynth's 2004 element table and NRL rules. Reference
  points: eSpeak NG's Klatt voice 22 % WER, SAM 33 %, our DOS voice 58 % before and 51 % after the
  fixes (white-noise fricative path with corrected level offsets, first-order pre-emphasis in the
  output stage instead of the 2-pole low-pass, x4 gain with a limiter for the 8-bit path, primary
  stress on every vowel). Cost unchanged (0.59x at 386 cycles). Conclusion: with this frontend the
  Klatt voice will stay in SAM's intelligibility class; a clearly better voice needs eSpeak NG's
  phoneme data and rules driving the synthesizer (frame-rate work, so CPU-cheap, but ~800 KB of data
  and a large port), which is the 486/DJGPP path in section 5, or a hand-improved element table.
  Remaining 386 work in order: (a) decide the frontend question above, (b) 2-3x speedup of the
  per-sample path in assembly, or 6 kHz. Fallbacks if the hardware says no: 6 kHz output,
  drop F3/parallel for a "386 voice", or the EMU8000 path. At 12000 DOSBox-X cycles (a 486DX-33 class machine) the current
  build benches at 1.44x real time (SAM 4.5x), so the 486 path already fits. Port klatt 3.04 parwave to integers on the host with a
  parity oracle, then drop it into SBTALK2 behind the same phoneme interface. Compare
  intelligibility (Whisper word error rate on a fixed sentence list) and CPU (DOSBox-X cycles and
  the real machine).
- **Phase 4 (started 2026-09-19, user decision): eSpeak NG frontend.** Port eSpeak NG's English
  text-to-phoneme and phoneme-to-parameter pipeline (GPL v3+, ~800 KB of data, fine with the 386's
  8 MB) to DOS as a DJGPP build driving the integer Klatt engine; milestone 1 is a non-resident
  `ESPK.EXE` that speaks through the Sound Blaster and reports speed, scored with the Whisper WER
  set against `espeak-ng -v en+klatt` (22 %). Work in `src/espeak/`. Milestone 1 done 2026-09-19 (`src/espeak/NOTES.md`): `ESPK.EXE` (DJGPP, 375 KB stripped +
  378 KB ESPK.DAT, < 2 MB RAM) speaks through the Sound Blaster; 18 of libespeak-ng's files with a
  96-line diff, wavegen replaced by an integer mapping of espeak's frames onto klatt_fx; Whisper
  WER 10 % (small.en) at 8 kHz, matching espeak-ng's own Klatt voice at 22 kHz and 3.5x better
  than the rsynth voice; DOSBox-X at 386 DX-25 cycles: **3.07x real time** (frontend 5 %, synth
  27 % of real time), 486 class 7.4x. 32-bit code makes the same Klatt engine ~3x faster than the
  16-bit Watcom build. ESPK.EXE is on the test floppy and the talking disk (`ESPK words`; `ESPK -k words` plays
  it on the PC speaker: PWM paced by polling the PIT channel 0 counter at the native 8000 Hz,
  no RTC or interrupt needed, so it works on any PC; the disk's first-boot menu is spoken by it).
  **Resident form done 2026-09-20** (`src/espeak/espkd/`, `share/espk/ESPKD.EXE`, NOTES.md "Resident
  ESPKD"): a DPMI shell wrapper that spawns COMMAND.COM; INT 14h is answered by a 256-byte real-mode
  stub (status polls never leave real mode, only data bytes cross into protected mode); SB DMA and RTC
  speaker outputs run in protected mode with hand-written IRQ wrappers (DJGPP's own drop nested
  invocations, which the coroutine design produces); all memory locked, CWSDPMI shipped with paging
  off. Provox works through it in QEMU; DOSBox-X at 386 cycles: 32 % CPU on the card, 45 % on the
  speaker; 461 KB conventional memory left in the child. On the 386 test floppy 2 as TALKD.BAT; too
  big (568 KB) for the single talking disk, so the 386 talking set will become two disks or an
  installed C: drive. Untested: real ISA IRQ/DMA latency, EMM386/VCPI reflection cost, JAWS, DR-DOS
  COMMAND.COM as the child.
  Residency question (answered above):
  a DJGPP engine cannot be a TSR; it would be a DPMI shell-wrapper process hosting COMMAND.COM with
  a real-mode callback for INT 14h.
- **Research (user request, done 2026-09-19): old DOS synthesizers.** Report
  `research/03-old-dos-synths.md`. TRAN (Stephen Neely 1988/1990): binary only; NRL rules (which we
  have) plus SPEECH.COM's phonemes. SPEECH.COM (Andy McGuire 1983): 36 one-bit phonemes clocked by
  the refresh timer at 33 kbit/s; binary, but Jon Hornstein's horndrv/TALK.SYS (1992-99, freeware +
  GPL, full Turbo C source, `src/horndrv/`) carries the phoneme tables. Monologue/SmoothTalker
  (First Byte): no source ever; method known from its now-expired patents (stored phoneme-portion
  and transition waveforms, period-stretch pitch, 4-bit delta packing), a possible low-CPU design
  to borrow later. Outcome: the 1983 voice is now engine 3 in SBTALK3 (`/RETRO`, `src/retro/`,
  `src/sbtalk/engine_retro.c`: NRL phones mapped to McGuire's set, bits resampled to 22 kHz with a
  box average; +22 KB of far data). Bonus find: the 1992 Simtel HANDICAP directory (ASAP 3/92
  demo, JAWS 2.11 docs, Flipper demo, DECtalk/Echo/Artic reader programs) in
  `research/simtel-handicap/`.
- **Phase 5: Provox improvements** (GPL, so upstreamable to HAPP or a new home): 386-aware speed
  tweaks, a proper `LITETALK`-like "SBTALK2" device entry, fix the PV7 control tool's dependency on
  RC Systems' proprietary DTC.LIB (rewrite PV7 in Watcom C), FreeDOS packaging (an `APPINFO` LSM
  and zip) so it can be offered to the FreeDOS project.
- **Later**: eSpeak NG under DJGPP as a non-resident "shell wrapper" for 486-class machines; a
  Speakup-style FreeDOS kernel hook is not needed since Provox already polls video memory.

## 5. Engine strategy: one talking disk, several engines, several classes of hardware

The disk should carry more than one engine and pick by hardware. All engines sit behind the same
two interfaces, so screen readers never notice: the DoubleTalk command set on a virtual COM port
(INT 14h) on the input side, and the Sound Blaster DSP/DMA path on the output side. Engine selection
is a load-time switch (`SBTALK /ENGINE=SAM|KLATT`) or separate EXEs where the code size differs.

1. **SAM (done).** 1982 formant engine, integer, ~28 KB of code+tables, 42 % of a 386 DX-25 at
   22 kHz, runs on an 8086 build. Baseline for every machine from XT to Pentium; licence is the
   open question (reverse-engineered abandonware), so it must be replaceable.
2. **Fixed-point Klatt, CPU only (Phase 3, in progress in `src/klatt/`).** Port of klatt 3.04
   parwave (GPL) to Q16/Q30 integers, driven by Holmes-style phoneme rules (rsynth). Budget on the
   386 DX-25 calibrated by SAM: ~4.5 M host instructions per audio second is 100 % of the machine.
   The inner loop is second-order resonators: ~3 multiplies each, 6-8 resonators plus source and
   output filters, so at 8 kHz about 200 K multiplies per second. A 386 `IMUL r32,r32` costs 9-38
   cycles, `IMUL r16` 9-22, so the C version will be marginal and the hot paths (resonator bank,
   voicing source, noise) get hand-written 386 assembly with 16x16->32 multiplies and Q15
   coefficients kept in registers; measure with DOSBox-X at 5000 cycles, then the real machine.
   Fallbacks in order: 8 kHz instead of 11, drop the parallel branch, fewer cascade resonators.
   This is the 486-and-up default voice and the 386 "if it fits" voice.
3. **Klatt with EMU8000 hardware assistance (SB32/AWE32/AWE64 only, optional, parked 2026-09-19).**
   Decision: recorded as an optional path to revisit after the software Klatt is intelligible and
   fast enough. Prerequisite to check: on-card DRAM (many SB32 boards ship with empty SIMM
   sockets; `AWEUTIL /S` reports it). First step when resumed: a host prototype of the EMU8000
   signal path (pulse loop through 2-pole resonant low-pass filters, Q <= 24 dB, no band-pass)
   driven by the existing Klatt parameter tracks, scored with Whisper against the software
   Klatt; then a register driver against 86Box (emulates the AWE32 with DRAM), then the SB32. The
   EMU8000 has 32 voices, each a sample-loop oscillator with pitch control, a 2-pole resonant
   low-pass filter (cutoff and Q per voice) and envelopes, all set by register writes. A formant
   synthesizer maps onto it: one small looping glottal-pulse wavetable in the card's DRAM (the SB32
   has 512 KB) played by 3-4 voices at F0, each voice's filter tuned to a formant (cutoff = F1,
   F2, F3 with high Q, mixed at the amplitudes A1-A3), plus a noise-loop voice through a filter
   for fricatives and a burst voice for stops. The CPU then only writes a handful of registers
   per 5-10 ms frame, exactly the Klatt parameter track, and spends nothing per sample; the 386
   is left free for the screen reader and the application. Limits: the filter is low-pass, not
   band-pass, so formant shapes are approximations (usable: the resonant peak dominates), Q tops
   out around 24 dB, register programming needs the documented EMU8000 init sequence (Linux
   `awe_wave.c`/ALSA `emu8000.c` and the AWE32 Developer's Information Pack). Test path: 86Box or
   PCem emulate the EMU8000 (QEMU and DOSBox-X do not), then the real SB32. Output can be mixed
   with the DSP path, so SAM/CPU-Klatt remain for consonant bursts if the card path is weak.
   OPL3 FM (present on every SB) is a second, cruder hardware-assist option for machines
   without an EMU8000.
4. **Modern DOS machines (486, Pentium, later).** No ISA Sound Blaster, but SBEMU and VSBHDA
   present an SB16-compatible DSP/DMA interface on top of HDA/AC97, so the same output path works.
   With 10-100x the CPU, the disk can run heavier engines: eSpeak NG (integer core, ~9 M
   instructions per audio second, 815 KB of English data) as a 32-bit DJGPP process hosting the
   DoubleTalk emulation itself, later Flite/Pico-class voices. These are separate binaries on the
   disk chosen by a CPU check in FDAUTO.BAT (`CPUID.EXE` is on the disk).

Build variants: `-0` (8086) builds for the XT/286 + SB 2.0/Pro class, `-3` builds with 386
instructions for the Klatt engine, DJGPP builds for the 32-bit engines.

**PC speaker output (2026-09-19, user request).** `SBTALK /SPK` plays any engine on the PC speaker:
PWM on PIT channel 2 (mode 0, one reload per sample, 145 levels) clocked by the RTC periodic
interrupt at 8192 Hz (IRQ 8, untouched by DOS and by Provox's timer hook), engines above that rate
are stepped down on the fly. Hardware-timed, so nothing depends on CPU speed (SPEECH.COM's delay
loops did, which is why AT owners had to slow their machines). Needs an AT-class RTC; XT-class
machines would need a timer-0 variant. `SBTALK /SPK /SAY text` speaks once without staying
resident and `SBTALK /ASK` returns a key as the errorlevel; the talking disk uses both for a
first-boot bootstrap menu on the speaker that asks which hardware and voice to use and saves the
answer in A:\VOICE.BAT (`RESETVOI` clears it). QEMU cannot capture PWM speaker audio (its speaker
model only renders square waves), so the speaker path is verified for timing and control flow in
QEMU and for sound on real hardware.

**XT class (2026-09-20, user request: PS/2 Model 30, 8086).** Findings from DOSBox-X with an
8086 core at 4.77 MHz (`run/dosbox/xt*.conf`, audio captured through SDL's disk driver):
- SAM is ~15x too slow (80 s of stuttering output for 5 s of speech), so only the 1983 voice is
  practical. Even that voice at 22 kHz through the sample ring ran at half real time, because
  per-sample C code costs more than an 8088 has.
- Fix: `/BITS` mode. The 1983 records are already 1-bit waveforms, so the ring carries the packed
  bytes and the Sound Blaster interrupt expands each byte into 8 DMA samples through a 2 KB table,
  played at the 33 kbit/s bit rate in the card's high-speed mode (DSP 2.01+, command 90h). On the
  emulated XT the synthesizer then keeps ahead of playback (11 s of continuous speech, non-blocking).
- XT speaker: no interrupt scheme can toggle port 61h 33,000 times a second on an 8088 (the
  RTC path needs an AT anyway), so `/SPK /RETRO /BITS` bit-bangs the bits in the caller's INT 14h
  call, blocking, like SPEECH.COM did, with the per-bit LOOP delay calibrated against the BIOS clock
  at start-up instead of SPEECH.COM's fixed loop (that is why AT owners had to slow their machines).
  Emulators execute port-61h writes far too slowly to judge this path (QEMU: ~1 ms per write), so
  it is verified for control flow only; the Model 30 is the real test.
- Tools: `SBTALK /CPU` returns the CPU class as the errorlevel (0 = 8086/88, 2 = 286, 3 = 386+),
  the talking disk boots the 8086 FreeDOS kernel (KERNL86.SYS works on every CPU) and its
  autoexec branches: 386+ get the full menu (eSpeak voice on the speaker, SAM/Klatt/1983 on the
  card), smaller machines get a 1983-voice menu (card, or speaker). A 286 has an RTC, so
  `/PIT` (timer-0 PWM at 5.5 kHz, BIOS tick chained) or `/SPK` would work there for SAM if it were
  fast enough; it is not, so the 286 takes the XT branch too.
- 32-bit programs (ESPK, SBPLAY, SBTALK3) are simply absent from the XT branch.

## 5b. Code review (2026-09-20)

Three reviews (resident SBTALK, the ESPKD DPMI wrapper, the engines) with fixes applied and
verified (host WAVs byte-identical where behaviour was meant to be unchanged; `run/test-sbtalk.sh`
is the QEMU regression over every SBTALK mode; ESPKD re-verified with Provox and Ctrl-C).

Bugs that mattered: SAM (upstream) looped forever or overran its 60-entry output list on long
stop-consonant-heavy text, exactly what a screen reader sends (fixed in the generated sources, fuzzed
under sanitizers); the Klatt frontend silently dropped the rest of a clause past 400 elements (now
resumable); the 1983 voice never played its word gap; ESPKD crashed the machine on Ctrl-C in the
child shell (DJGPP's SIGINT hook) and left PIC/DMA/RTC state behind on exit; SBTALK's XT speaker
calibration overflowed 32 bits at 8088 speeds, its timer-0 path could lose or duplicate BIOS
ticks, 32-bit ring counters could tear on a 16-bit CPU, ^X inside a DoubleTalk command was
swallowed, and loading SBTALK twice re-hooked everything (now "already resident"; SAY and /SAY
route through the resident copy).

Memory: SBTALK resident 99 -> 81 KB (SAM), SBTALK3 123 -> 103 KB (Klatt), new SBTALKXT 62 KB for
the XT branch; no stdio in the resident half, environment freed, DMA block right-sized, engine
tables moved to far data (SBTALK3's DGROUP was 5.6 KB from the 64 KB limit, now 36 KB used).
ESPKD conventional 45 -> 25 KB, locked extended 1.4 -> 1.0 MB. Not changed (behaviour):
text-ring overflow drops bytes rather than de-asserting THRE; CWSDPMI takes all XMS from the child.
Bench re-baseline at 386 cycles after the reviews: SAM 1.6x (was 1.9x; the bench now also
checksums every sample, and SAM carries bounds checks), Klatt 0.58x (unchanged); the resident
hot path was verified to keep the ring full in /TEST.

**DR-DOS and the universal disk (2026-09-20).** DR-DOS 7.02's EMM386 is a DPMI host
(`/DPMI=ON`); with it ESPKD runs without CWSDPMI (43 KB conventional saved, no XMS grab), at the
cost of V86-mode interrupt reflection, to be measured on the 386. Under MS-DOS EMM386 and FreeDOS
JEMM386 (V86 + VCPI) CWSDPMI is used: verified in QEMU with JEMM386 loaded (ESPK, ESPKD, SAY,
unload all fine) once enough XMS is free; with the LiveCD's RAM disk eating XMS, CWSDPMI died with
a page fault, so ESPKD now refuses to start below 2 MB free. ESPKD prints the DPMI host and puts
its real-mode block in a UMB when one exists; the child shell shows an `[E speak]` prompt. TASKMGR:
SBTALK and Provox are global TSRs (load them before TASKMGR); ESPKD is a DPMI client inside one
session and should not be combined with it. `dist/DOSTEST.BAT` is the per-DOS test sheet.

## 6. Open questions and risks

- Licences: Provox is GPL (good). SAM is unlicensed reverse-engineered abandonware: prototype only.
  klatt 3.04 is GPL; rsynth's licence needs reading before use. DECtalk's leaked source is not usable.
- Real-mode C vs. 32-bit: SAM and Klatt fit in 16-bit segments; the Klatt port must be careful with
  32-bit intermediates (Watcom `long` and `-3`).
- INT 14h handshake: Provox uses CTS and XON/XOFF; the emulation must never report "not ready" for
  long or Provox stalls the foreground. Provox source (`6I16CODE.A`, `9PROVOX7.A`) documents the
  exact status bits it checks.
- The console-on-COM1 test trick drops characters if sent faster than DOS polls; `run/serial.sh`
  paces input. Occasional lost commands have been seen; rerun if a test line does not appear.
- The 386 has no cache and a 16-bit ISA bus; DMA and IRQ latency on the real board may differ from
  emulation. Phase 1 exists to find out early.
