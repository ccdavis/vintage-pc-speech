# Software speech synthesis for a 386DX-25 FreeDOS screen reader (Sound Blaster 32)

Status: research notes, 2026-09-19. Target: 386DX-25, no FPU, 8-16 MB RAM, Sound Blaster 32 (SB16-compatible DSP 4.xx; the EMU8000 is irrelevant here). Goal: a software synthesizer that is a clear step up from Dr. Sbaitso, streaming PCM through the SB DSP, coexisting with a DOS screen-reader TSR.

Conventions: **[verified]** = read in a primary source or in the code itself (several repos were shallow-cloned and grepped for this report); **[recall]** = from memory, not re-checked; **[estimate]** = my arithmetic.

---

## 1. Historical DOS software TTS on the Sound Blaster

### First Byte SmoothTalker / Monologue / SBTALKER (the Dr. Sbaitso engine)

- SmoothTalker (1984, Macintosh first; DOS v1.1 in 1988) was First Byte's rule-based engine: text -> "sound descriptors" (a phonetic code with pitch/duration/amplitude), then descriptors -> waveform. The whole Mac package was under 200 KB **[verified: comp.speech ProVoice page; Macintosh Repository]**. Sources disagree on the back end: it is usually described as concatenating small stored speech fragments (diphone/allophone-like) with pitch manipulation rather than as a Klatt formant synth **[recall; I found no primary technical description]**.
- Monologue for DOS (1990, $149) is a **TSR**; it used up to 220 KB of conventional memory, or ~45 KB conventional when EMS/XMS was available; it ran on XT/AT-class machines and drove the PC speaker, Covox Speech Thing, Street Echo PC, Sound Blaster, Hearsay, Tandy and IBM speech adapters. Reviewers described the voice as having "a vague Eastern European accent" and it only worked with text-mode programs **[verified: Baltimore Sun, 31 Dec 1990, https://baltimoresun.com/news/bs-xpm-1990-12-31-1990365184-story.html]**.
- Dr. Sbaitso (1991, bundled with Sound Blasters) is a chat front-end over a resident copy of the same engine: `SBTALKER.EXE` (TSR) + `BLASTER.DRV`. A ChatGPT-for-DOS project that reuses it states its requirements as "a Sound Blaster-compatible card and 8 MHz CPU" **[verified: https://github.com/yeokm1/doschgpt]**, so the CPU cost is tiny (XT class).
- The engine's private API has been reverse-engineered: `dosbtalk` (Apache-2.0, Turbo C 2.01) shows detection via **INT 2Fh with AX=FBFBh** (driver returns ES:BX to a data block beginning with the "FB" signature and a version word), then far calls into the TSR (V3 = "SmoothTalker 3.5", V4 = "Text-to-Speech Engine 4.1" with `SOUNDBST.EXE`, `SPEECH.EXE`, `KERNEL.DIC` and swappable sound drivers) **[verified: https://github.com/systoolz/dosbtalk]**. A Turbo Pascal wrapper ("SB Text to Speech Unit") exists in SWAG **[verified listing: http://www.retroarchive.org/swag/SOUND/0037.PAS.html; page did not load for me]**.
- Survivals: Smooth Talker v1.1 for DOS (https://archive.org/details/smooth_talker_v2), Dr. Sbaitso in Creative driver collections on VOGONS, Monologue '97 for Windows (https://archive.org/details/monologue-97) **[verified listings]**.

### Who used it with a screen reader

A 1995 alt.comp.blind-users thread ("Software that will use Soundblaster for speech", https://groups.google.com/g/alt.comp.blind-users/c/_08-9cUISpk) is the best single source **[verified]**:

- **Tinytalk Personal** (OMS Development) worked with the Sound Blaster via `BLASTER.DRV`, i.e. the First Byte engine, but was "memory-intensive" and "sluggish".
- **Provox** (Kansys) had supported the Sound Blaster with similar complaints.
- **JAWS for DOS** supported the Sound Blaster but "Henter-Joyce discourages the use of this card".
- Consensus (Mike Freeman): "Forget it! The SoundBlaster was not designed for production-quality speech synthesis for the blind"; under DOS it was "a dead loss", under Windows (TextAssist) it could give "DECtalk quality speech". A 486DX/33 or better was recommended for the Windows route.

So historically the pattern was: **synth as a separate real-mode TSR exposing an INT 2Fh API, screen reader as a client**. It was slow mostly because the engine did all synthesis synchronously in the caller's context and hogged conventional memory.

### Creative TextAssist (DECtalk) - not a DOS option

- TextAssist is DECtalk licensed by Creative and shipped with the SB16 ASP/CSP and AWE32; the vocal-tract work ran on the **CSP ("Advanced/Creative Signal Processor") chip**, and it is a **Windows 3.1 application** with a 16-bit API (TAAPI) **[verified: comp.speech page http://mi.eng.cam.ac.uk/comp.speech/Section5/Synth/creative.html; Wikipedia SB16; VOGONS t=31835]**. On cards without a CSP it fails with "Unable to open driver" **[verified: VOGONS]**. I found **no DOS TSR version** of TextAssist. The SB16 install CD on archive.org lists a "TextAssist Program Disk" but everything about it is Windows **[verified: https://archive.org/details/creative-sound-blaster-cd-software-voice-assist-text-assist-qsound-dos-windows-drivers-sb-16-wss-2.0]**.
- Screen-reader support was on Windows only: Henter-Joyce announced JAWS for Windows support for "Soundblaster 16 ASP or AWE32 ... with the Soundblaster's TextAssist software" in August 1995 **[verified: NFB newsletter http://www.nfbnet.org/files/newsletters/NEWS0895.TXT]**.
- Important for this project: the **Sound Blaster 32** (CT36xx) normally ships **without** the CSP chip (empty socket), so TextAssist would not run on it even under Windows **[recall]**.

### Others

- **Covox Speech Thing** (1987): parallel-port R-2R DAC; its bundled TTS was First Byte's SmoothTalker-family software **[verified: Wikipedia; archive.org ephemera]**. Not relevant to an SB machine except as a reminder that 8-bit 8 kHz output was the norm.
- **ProVoice Developer's Toolkit** (First Byte) existed for DOS; text -> "sound descriptors" -> speech **[verified: comp.speech page]**. Commercial, no source.
- **"Blaster Master"** is a sample editor, not a synthesizer **[recall]**. "SBTalker" is the Dr. Sbaitso TSR above.
- comp.speech FAQ Q5.5 also lists DOS synthesizers CSRE, Infovox, MBROLA, SENSYN, spchsyn.exe, Tinytalk, ZMD **[verified: http://mi.eng.cam.ac.uk/comp.speech/Section5/Q5.5.html]**; none is open source.
- **DECtalk on DOS**: only as hardware (DECtalk PC ISA card). The leaked DECtalk tree still contains `#ifdef MSDOS` in 53 core files and a 16-bit x86 `hardware/` subtree (`c0.asm`, `.mak` files), i.e. the core was once built with a 16-bit x86 toolchain for the card firmware **[verified in source; interpretation is mine]**.
- **MBROLA for DOS**: real. The 3.01 docs say it compiled with Visual C++ or Borland C++ for "PC486/DOS6" with `LITTLE_ENDIAN` and `DOS` defined; the DOS binary shipped as `mbr301d.zip` **[verified: MBROLA docs and FreeTTS forum]**. It is a diphone back end only (input is phonemes + durations + pitch), needs a ~5 MB voice database per language **[recall]**, and its own docs name a 486 as the platform.

---

## 2. Open-source synthesizers that could be ported

Repos below were cloned and inspected on 2026-09-19.

### SAM (Software Automatic Mouth), C port by s-macke

- https://github.com/s-macke/SAM. C, ~3,400 lines total (`sam.c` 1448, `render.c` 1105, `reciter.c` 551, `main.c` 263). **Zero uses of `float`/`double`** in `src/` **[verified by grep]**. Executable "less than 39 KB" **[verified README]**.
- Algorithm: formant "synthesis" by adding two sine-table lookups (F1, F2) and one rectangular wave (F3), each scaled by a 4-bit amplitude, restarted every glottal pulse; unvoiced consonants are 1-bit sample tables **[verified in render.c]**. Output is 8-bit unsigned; the C port writes at 22,050 Hz using a 6502-cycle time table to stretch samples.
- CPU: it ran in real time on a 1 MHz 6502 (about 0.43 MIPS) with the screen blanked on the C64 **[verified: Wikipedia; C64 blanks the screen so the CPU is not stolen]**. On a 386DX-25 this is well under 10% of the machine even with the reciter running.
- License: none. The README says it cannot be put under an open-source license and "might be used under the Fair Use act in the USA"; the original owner (SoftVoice, Inc.) still trades **[verified: README; https://www.text2speech.com/]**. Fine for a private prototype, not for redistribution.
- Quality: 1982; intelligible but clearly worse than Dr. Sbaitso. Not a "more modern" voice.
- DOS port needs: nothing hard. It would fit a 16-bit real-mode TSR (Open Watcom or Turbo C); the tables are a few tens of KB. Many MCU ports exist (ESP8266/ESP32/Arduino) **[verified listings]**.

### rsynth (Nick Ing-Simmons) and klatt 3.04 (Iles/Ing-Simmons; espeak-ng/klatt)

- rsynth: https://github.com/rhdunn/rsynth (modernized fork; original https://rsynth.sourceforge.net/). C, ~6,500 lines. Pipeline: text -> CMU dictionary / NRL letter-to-sound -> phonemes -> `holmes.c` (Holmes' phoneme-to-parameter rules) -> `opsynth.c` (Klatt-style synthesizer). **Float everywhere**: `opsynth.c` has 51 float/double uses, `holmes.c` 27 **[verified by grep]**. License: COPYING is GPLv2 and README says files are GPL or LGPL, but the Klatt-derived code carried Dennis Klatt's copyright, which Ing-Simmons said he intended to re-code **[verified: README; ROCK Linux package notes]**. I found no historical DOS build of rsynth.
- klatt 3.04 standalone: https://github.com/espeak-ng/klatt (GPLv3, "ported to C by John Iles and Nick Ing-Simmons up to version 3.04", cleaned up by Reece Dunn). Only 1,238 lines; `parwave.c` (the actual synthesizer) has 50 float/double uses; default 10 kHz output **[verified]**. This is the classic cascade/parallel Klatt 1980 design and the natural basis for a **fixed-point rewrite**: about 20 second-order resonators (cascade F1-F5, nasal pole/zero, parallel F1-F6, glottal and radiation filters).
- Existing fixed-point / MCU formant synths I could find:
  - **klattsch** (Tony Gies, MIT license): a 3-formant parallel synth (Klatt-1980-style biquads, Rosenberg pulse) with a **DOS port ("klattsch 386") built with a DJGPP cross toolchain (GCC 12.2, i586-pc-msdosdjgpp), Sound Blaster streaming, and a fixed-point conversion validated against the JavaScript engine** **[verified: https://crashunited.itch.io/klattsch/devlog/1579917/klattsch-386-making-dos-stuff-with-modern-tools; https://github.com/tgies/klattsch]**. It is a singing toy, not a TTS, and the DOS source's location/license was not stated on the devlog page; but it is proof that a fixed-point formant synth streams on a 386-class DOS target with modern tools. Worth contacting the author.
  - Klatt-Synth-STM (STM32F4): float **[verified]**. crispinprojects/klatt-synthesizer, chdh/klatt-syn, rhdunn/klatt: float/double **[verified or from search]**.
  - **Talkie** (Peter Knight, GPLv2): LPC-10 (TI Speak & Spell) decoder in integer math; 8 kHz sample interrupt costs ~50 us (~800 cycles) on a 16 MHz ATmega328 **[verified: https://github.com/ArminJo/Talkie]**. Not a TTS (fixed vocabulary), but a useful cost calibration: a 10-pole lattice per sample is cheap.
  - I found no ready-made "integer Klatt" or "TinyKlatt" for 8/16-bit CPUs; the fixed-point port is work we would do ourselves (it is small).

### eSpeak / eSpeak NG

- https://github.com/espeak-ng/espeak-ng, GPLv3, C, ~32,000 lines in `src/libespeak-ng`. History: "Speak" for Acorn RISC OS from 1995, rewritten as eSpeak in 2007 with "a relaxation of the original memory and processing power constraints" **[verified: espeak.sourceforge.net; Wikipedia]**.
- Integer core, mostly: the main harmonic synthesis loop in `wavegen.c` is integer (`pitch` as Hz<<12, `sin_tab`, integer AGC); `synthesize.c`, `synthdata.c`, `intonation.c` have zero float uses. `double` appears only in the optional breath-noise resonator (`resonator()`), the sonic speed-up glue, and one place each in `setlengths.c`/`numbers.c` **[verified by grep]**. The optional `klatt.c` back end is float.
- Data for English alone: `phondata` 550,424 B, `en_dict` 166,916 B, `phontab` 55,764 B, `phonindex` 39,062 B -> about 815 KB, which is loaded into RAM **[verified: pschatzmann Arduino port write-up, https://www.pschatzmann.ch/home/2022/11/10/espeak-ng-the-difficult-journey-to-an-arduino-library/]**. Total program + all languages ~2 MB **[verified: sourceforge page]**.
- Output rate is fixed at 22,050 Hz (phoneme data are compiled for it) **[recall]**. The additive synthesis sums every harmonic below ~4-5 kHz per sample, i.e. tens of table-multiply-adds per sample at 22 kHz.
- No DOS/DJGPP build found. A DJGPP build is plausible (plain C; strip pthreads, pcaudiolib, sonic, MBROLA) but needs DPMI for the ~1 MB data.

### Flite (CMU)

- https://github.com/festvox/flite, BSD-like. Core "approximately 50K", but an 8 kHz diphone voice is ~5 MB code+data **[verified: Flite paper summary]**; `lang/cmu_us_kal` source is 6.4 MB and `cmulex` 4.6 MB **[verified: du]**. Floats pervade the feature/CART code; only the wave synthesizer has `lpc_resynth_fixedpoint` **[verified]**. Not a 386 candidate.

### SVOX Pico

- Apache-2.0, HMM-based, <400 KB ROM per language, <250 KB RAM, "ARM9 or equivalent" **[verified: SVOX press release]**. An ARM9 at ~200 MHz is 30-50x a 386DX-25; hopeless.

### DECtalk source (2015/2022 releases)

- https://github.com/dectalk/dectalk (and https://github.com/dectalk/463 for 4.63). Files were shared by original developer Edward Bruckert on the DECtalk list in Oct/Nov 2015 and more code was added in 09/2022; Fonix DECtalk 5.1 was shared in 2023 **[verified: README; search]**.
- Legal status: **proprietary**. `LICENCE` is the Fonix (2002-03) / Force Computers (2000-01) / SMART Modular (1999) notice: confidential technology, use only under a written license, "all other rights reserved" **[verified]**. Source files carry Dennis Klatt 1984 and DEC 1984/1993/1995 copyrights. Fine to experiment with privately; not shippable.
- The technically interesting part: the vocal tract model exists in **integer** form. `src/dapi/src/vtm/vtm_i.c` (893 lines) has zero `float` uses (one `double` cast for the sample-rate variable); the default `vtm3.c` (2,479 lines) also has no float, only `exp()/cos()` at table-init time; the float build (`vtm_fa.c`) is selected only with `FP_VTM`, added "for alpha builds"; there are `ARM7` sections for the Epson ARM7 embedded target **[verified by grep]**. The rest of DAPI (`src/dapi/src`) is ~63,600 lines, plus a 397 KB US dictionary (`dtalk_us.dic`) **[verified]**. This is the 4.x software DECtalk that ran on Windows NT/Alpha and on Windows CE/ARM.

---

## 3. Sound Blaster side: streaming PCM from DOS

From the Creative "Sound Blaster Series Hardware Programming Guide" **[verified: https://pdos.csail.mit.edu/6.828/2018/readings/hardware/SoundBlaster.pdf, text-extracted]**:

- Ports: base 2x0h; DSP reset 2x6h (write 1, wait 3 us, write 0, poll 2xEh bit 7 then read 0AAh from 2xAh); write data/command 2xCh (poll bit 7 clear on 2xCh first); read data 2xAh; read-buffer status 2xEh. `E1h` returns the DSP version (SB32 = 4.xx).
- Sample rate: on DSP 4.xx use `41h` + rate high byte + low byte (e.g. 44100 -> ACh 44h); older cards use `40h` + time constant, `TC = 65536 - 256000000/(channels*rate)`, high byte only.
- SB16 auto-init playback: send `C6h` (8-bit) or `B6h` (16-bit), then mode byte (`00h` 8-bit mono unsigned, `10h` 16-bit mono signed, `20h`/`30h` stereo), then block size low, high, where **block size = samples - 1** and, for double buffering, the DSP block is half the DMA buffer (e.g. 8 KB DMA buffer, 4 K-sample DSP block). At the end of each block the card raises the IRQ; the ISR refills the half just played. Single-cycle versions are `C0h`/`B0h`; exit auto-init `DAh` (8-bit) / `D9h` (16-bit); pause/continue `D0h`/`D4h` (8-bit), `D5h`/`D6h` (16-bit); speaker on/off `D1h`/`D3h` (8-bit only on older DSPs).
- IRQ acknowledge: read 2xEh for 8-bit DMA (shared with SB-MIDI), 2xFh for 16-bit DMA; mixer register 82h tells which source raised the shared IRQ; then EOI to the PIC(s).
- DMA: 8-bit data on an 8-bit channel (1 or 3), 16-bit on channel 5/6/7; the DMA buffer must not cross a 64 KB physical page; program the 8237 (mask, clear flip-flop, mode 58h+channel = auto-init read-from-memory, address, page register, count-1, unmask). 16-bit channels count in words and use the shifted address.
- Speech at 8-11 kHz mono needs 8-11 KB/s; a 2 x 2048-byte buffer at 8 kHz gives 256 ms per half and 4 IRQs/s, so IRQ overhead is negligible even on a 386.

Working open-source examples:

- **sb16-wav** (TASM, 8086-compatible, 882 lines): 16-bit auto-init playback, ISR, DMA setup, tested in DOSBox **[verified: https://github.com/margaretbloom/sb16-wav; no license file in repo]**.
- **JUDAS** sound system (Cadaver/Yehar; GPLv2 per README): SB/SB16 auto-init DMA driver for DJGPP and Watcom, including `JUDASDMA.C` which allocates a DOS DMA buffer via `__dpmi_allocate_dos_memory` and locks it **[verified: https://github.com/volkertb/JUDAS]**.
- **Allegro 4 DOS driver `sb.c`** (Giftware license, https://liballeg.org/license.html) and **libmikmod `drivers/dos/dossb.c`** (LGPL) are the other well-known DJGPP SB16 drivers **[verified listings]**.
- Tutorials: OSDev wiki "Sound Blaster 16" (blocked me with 403 today), GameDev.net "Programming the SoundBlaster 16", shdon.com sound code (Pascal/C/DJGPP mixing with auto-init DMA).

Test rigs before the real machine:

- **QEMU** `-device sb16` defaults: iobase 0x220, IRQ 5, DMA 1, DMA16 5 **[verified: hw/audio/sb16.c]**. Good for correctness, useless for timing (host-speed CPU).
- **DOSBox-X**: `cputype=386`, `cycles=fixed 4595` is the documented guess for a "386DX-25" (386DX-33: 6075, 486DX-33: 12019), with the explicit caveat that DOSBox-X "is not cycle accurate" and one emulated instruction is one cycle **[verified: https://dosbox-x.com/wiki/Guide:CPU-settings-in-DOSBox%E2%80%90X]**. It also emulates an SB16. 86Box with a 386DX-25 model is the more timing-faithful option **[recall]**.

---

## 4. Real-time budget on a 386DX-25

Measured/quoted performance **[verified: VOGONS t=46350; tommesani.com; Wikipedia]**:

- Intel's rating for the 25 MHz 386: ~7 MIPS. Dhrystone ~8,000-9,000/s = ~4.6-5.1 DMIPS. On a real board, PC Doctor 1.7 reports 6.3 MIPS, PMIPS 4.5, an integer "MIPS test" 3.53. Same-clock 286 is nearly as fast in 16-bit code; the 386 only pulls ahead with 32-bit code. A 386 `IMUL r32,r32` takes 9-38 clocks depending on operand magnitude **[recall]**, so treat one multiply as ~4-6 simple instructions.

Budget at ~5 MIPS effective (32-bit code), instructions per output sample:

| Output rate | 100% of CPU | 50% (leave half for reader + app) |
|---|---|---|
| 8,000 Hz | ~625 | ~310 |
| 11,025 Hz | ~450 | ~225 |
| 16,000 Hz | ~310 | ~155 |
| 22,050 Hz | ~225 | ~110 |

Per-sample cost of the candidates **[estimate]**:

- **SAM**: 3 table lookups, 3 small multiplies, a few adds per internal sample, plus per-frame bookkeeping: ~30-50 instructions. Real time on a 1 MHz 6502 confirms it. **< 10% of a 386DX-25.**
- **Talkie-class LPC-10**: ~800 AVR cycles/sample at 8 kHz; on a 386 with 32-bit multiplies ~100-150 instructions/sample -> ~15-20% at 8 kHz. Cheap, but not a TTS.
- **Full Klatt 1980 (cascade + parallel, ~20 biquads) in fixed point**: ~50-60 multiplies and ~150 adds/moves -> ~350-500 instructions/sample -> **3-4 MIPS at 8 kHz, 4-5.5 MIPS at 11 kHz**: 60-100% of the machine. A reduced configuration (cascade F1-F4 + nasal pair, one parallel frication path, update coefficients every 5-10 ms instead of every sample) is ~150-250 instructions/sample -> **~25-40% at 8 kHz, ~35-55% at 11 kHz.** Feasible, but only in 32-bit integer code, and only at 8-11 kHz.
- **DECtalk integer VTM (vtm3.c/vtm_i.c)**: same family of filters as Klatt with extras; expect the same 30-60% at 11 kHz if the rest of the pipeline is cheap. The letter-to-sound, dictionary lookup and prosody are text-rate work and cost little per second of audio, but the 397 KB dictionary and 63 K lines of engine force a DPMI/DJGPP build.
- **eSpeak NG**: per sample it sums every harmonic below ~4.5 kHz; at 100 Hz pitch that is ~45 harmonics x (lookup, multiply, add) plus AGC -> ~300-600 instructions/sample at 22,050 Hz -> **7-13 MIPS**. That is 486DX2-66 territory; a 386DX-25 would run at roughly one-half to one-third of real time. It also allocates ~1 MB for data. Cutting the harmonic ceiling and running at 11 kHz would need changes in `wavegen.c` and recompiled phoneme data.
- **MBROLA** (486 per its own docs, 5 MB database), **Flite** (5 MB voice, floats), **Pico** (ARM9-class): hopeless on this machine.

Ranking (real-time speech on a 386DX-25): 1) SAM, easily. 2) Fixed-point reduced Klatt at 8-11 kHz, with care. 3) DECtalk integer VTM: plausible but unverified and legally dead-ended. 4) eSpeak NG: needs a 486DX2 or better as-is; a 486DX-33 might manage at 11 kHz after surgery. 5) MBROLA/Flite/Pico: no.

---

## 5. Architecture: reader TSR + synthesizer on DOS

Memory facts for this machine: 640 KB conventional, but with 8-16 MB the 386 can run `EMM386` and load TSRs into UMBs, and XMS holds tables. DJGPP programs run in 32-bit protected mode under CWSDPMI (or HDPMI32) and address all RAM directly; real-mode TSRs must copy from XMS via INT 15h/XMS block moves.

How the originals did it: First Byte's engine was a **real-mode TSR** with an INT 2Fh (AX=FBFBh) far-call API; screen readers (Tinytalk, Provox, JAWS DOS) called it and the engine synthesized synchronously, which is why users called it sluggish; Monologue needed 220 KB conventional (45 KB with EMS/XMS) **[verified above]**. TextAssist avoided the problem by running the synth on the CSP DSP chip, under Windows only.

Options:

**(a) One real-mode TSR: reader + synth, synthesis in the background, DMA double-buffered.** The reader hooks INT 9/10h/16h/1Ch as usual; the synth exposes a queue of phoneme/parameter frames; the SB16 IRQ handler renders the next half-buffer (auto-init DMA, 2 x 1-2 KB, 8 kHz 8-bit unsigned) directly. Rendering inside the ISR is safe because the synth never touches DOS/BIOS; the ISR only has to finish within one half-buffer period (128-256 ms). Text-to-phoneme runs in the caller's context (it is cheap and can be chunked) or in the INT 28h idle hook. Interrupting speech on a keystroke = flush the queue and zero the buffer; no DSP reprogramming needed. Compiler: Open Watcom 16-bit with `-3` (386 instructions and 32-bit registers in real-mode code) gives 32-bit `IMUL` inside a TSR **[recall on the `-3` switch]**; Turbo Pascal/C works for SAM-sized code. With SAM (~40 KB) or a reduced Klatt (~30 KB code + ~50 KB parameter tables) the whole thing fits in a UMB. This is the closest to what worked historically, minus the synchronous-call mistake.

**(b) DJGPP program hosting the synth, reader as a real-mode TSR.** Hard: DJGPP programs are not TSRs. DPMI 1.0 defines "resident service providers" (INT 31h 0C00h/0C01h) **[verified: http://www.delorie.com/djgpp/doc/dpmi/ch4.8.html]**, but CWSDPMI implements DPMI 0.9 and does not offer that **[recall]**, and the DJGPP FAQ notes that going resident means hooking and monitoring many DOS/BIOS interrupts **[verified: FAQ index note]**. The workable DJGPP shape is instead a **shell wrapper**: one DJGPP process = reader + synth, which `system()`s a `COMMAND.COM` and stays underneath, hooking the keyboard/video/SB interrupts from protected mode (`__dpmi_set_protected_mode_interrupt_vector`, real-mode callbacks) with the DMA buffer in DOS memory (`__dpmi_allocate_dos_memory`, locked, exactly as JUDAS does). Costs: CWSDPMI + stub steal 20-40 KB of conventional memory from every DOS program, every hardware interrupt causes a mode switch (fine at 4 IRQ/s), and DOS programs that bring their own DPMI/VCPI extender may conflict. Benefit: 32-bit gcc code, all RAM, and it is the only realistic way to run eSpeak NG or the DECtalk engine.

**(c) Second machine over serial**: the classic hardware-synthesizer topology, out of scope here.

Recommendation on architecture: prototype (a) for the small synth, keep (b) as the path for a big engine, and expose the same interface from both (a small INT 2Fh API modelled on First Byte's, or simply a shared command queue) so the reader does not care which one is loaded.

---

## 6. Recommendation

**First prototype: a fixed-point Klatt-style synthesizer, not SAM, driven first by SAM's reciter or rsynth's front end, output 8 kHz 8-bit via SB16 auto-init DMA, built with DJGPP for the bench and Open Watcom `-3` for the TSR.**

Why not SAM as the goal: it is trivially real-time and would be the quickest "it talks" milestone (an afternoon with the sb16-wav or JUDAS driver code), but it is 1982 quality and cannot be redistributed. Use it only as the first sound-out test and as the source of a decent English text-to-phoneme rule set.

Why an integer Klatt: the standalone klatt 3.04 code is 1,238 lines and GPL; converting `parwave.c` to Q16/Q30 fixed point with 32-bit `IMUL` is small, and the parallel/cascade parameter stream is exactly what Holmes' rules (rsynth `holmes.c`, also float, also small) or a hand-made phoneme table produce. The CPU estimate says a reduced Klatt at 8-11 kHz is 25-55% of a 386DX-25, which leaves room for the reader; klattsch-DOS shows a fixed-point formant synth already streaming to a Sound Blaster from a DJGPP 386 build. Quality lands between Dr. Sbaitso and DECtalk: Klatt/Holmes-class speech at 8-10 kHz, the same design lineage as DECtalk's own VTM.

Validation plan: build klatt 3.04 + rsynth on Linux, generate the parameter streams, write the fixed-point renderer with a "parity oracle" against the float version (the klattsch author's approach), then run it in DOSBox-X at `cputype=386`, `cycles=fixed 4595` and time it with the DOS timer before touching the real board; cross-check on QEMU for SB16 correctness.

**Second, higher-quality target: eSpeak NG under DJGPP as a shell-wrapper (option b).** It is GPL, its core is already integer, English data are ~815 KB, and it is the most intelligible open engine at high speaking rates, which is what a screen reader user actually wants. On the 386DX-25 it will not be real-time without reducing the harmonic ceiling and sample rate, so treat it as the target for the same software on a 486 (or as a stretch goal after profiling `wavegen.c` on the 386). The DECtalk integer VTM would give the best voice of all, but the license forbids anything beyond a private experiment, so it stays a curiosity.

---

## Sources

- alt.comp.blind-users 1995 thread: https://groups.google.com/g/alt.comp.blind-users/c/_08-9cUISpk
- NFB newsletter Aug 1995 (JAWS + TextAssist): http://www.nfbnet.org/files/newsletters/NEWS0895.TXT
- comp.speech: TextAssist http://mi.eng.cam.ac.uk/comp.speech/Section5/Synth/creative.html; ProVoice http://mi.eng.cam.ac.uk/comp.speech/Section5/Synth/provoice.html; Q5.5 list http://mi.eng.cam.ac.uk/comp.speech/Section5/Q5.5.html
- Baltimore Sun on Monologue (1990): https://baltimoresun.com/news/bs-xpm-1990-12-31-1990365184-story.html
- dosbtalk (First Byte API): https://github.com/systoolz/dosbtalk; doschgpt: https://github.com/yeokm1/doschgpt
- VOGONS: Dr. Sbaitso engine thread https://www.vogons.org/viewtopic.php?t=37742; TextAssist hardware thread https://www.vogons.org/viewtopic.php?t=31835; 386DX-25 benchmarks https://www.vogons.org/viewtopic.php?t=46350
- SB16 install CD: https://archive.org/details/creative-sound-blaster-cd-software-voice-assist-text-assist-qsound-dos-windows-drivers-sb-16-wss-2.0; Smooth Talker DOS: https://archive.org/details/smooth_talker_v2
- SAM: https://github.com/s-macke/SAM; Wikipedia https://en.wikipedia.org/wiki/Software_Automatic_Mouth; SoftVoice https://www.text2speech.com/
- rsynth: https://github.com/rhdunn/rsynth, https://rsynth.sourceforge.net/; klatt 3.04: https://github.com/espeak-ng/klatt
- klattsch DOS devlog: https://crashunited.itch.io/klattsch/devlog/1579917/klattsch-386-making-dos-stuff-with-modern-tools; https://github.com/tgies/klattsch
- Talkie: https://github.com/ArminJo/Talkie
- eSpeak: https://espeak.sourceforge.net/; https://github.com/espeak-ng/espeak-ng; Arduino port sizes https://www.pschatzmann.ch/home/2022/11/10/espeak-ng-the-difficult-journey-to-an-arduino-library/
- Flite: https://github.com/festvox/flite; Pico: https://www.pressebox.com/pressrelease/svox-ag/SVOX-releases-Pico-highest-quality-sub-1-MB-Text-to-Speech-system-available/boxid/217338
- MBROLA: https://github.com/numediart/MBROLA (AGPL-3.0), DOS notes in Documentation/documentation301.html
- DECtalk: https://github.com/dectalk/dectalk, LICENCE https://github.com/dectalk/dectalk/blob/develop/LICENCE, https://github.com/dectalk/463
- Creative SB Hardware Programming Guide: https://pdos.csail.mit.edu/6.828/2018/readings/hardware/SoundBlaster.pdf
- sb16-wav: https://github.com/margaretbloom/sb16-wav; JUDAS: https://github.com/volkertb/JUDAS; Allegro license https://liballeg.org/license.html; libmikmod DOS driver https://github.com/sezero/mikmod/blob/master/libmikmod/drivers/dos/dossb.c
- QEMU sb16: https://github.com/qemu/qemu/blob/master/hw/audio/sb16.c; DOSBox-X CPU guide: https://dosbox-x.com/wiki/Guide:CPU-settings-in-DOSBox%E2%80%90X
- DPMI resident service providers: http://www.delorie.com/djgpp/doc/dpmi/ch4.8.html; DJGPP FAQ index: https://www.delorie.com/djgpp/v2faq/faq25.html
- MIPS figures: https://en.wikipedia.org/wiki/Instructions_per_second; https://tommesani.com/product/intel-386-dx-25-mhz/
