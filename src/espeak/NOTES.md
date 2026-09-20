# ESPK: eSpeak NG's English frontend driving the integer Klatt synthesizer on DOS

First milestone (2026-09-19): `ESPK.EXE "text"` on FreeDOS speaks through the
Sound Blaster and prints the synthesis time.  Text goes through eSpeak NG's
translate / dictionary / numbers / phoneme list / lengths / intonation /
spectral frames, and the frames drive `src/klatt/klatt_fx` (the Q12 integer
parwave) instead of espeak's own float `klatt.c`.  Integer only; no
modification of `src/klatt`.

```
make            # host: ./espk_host [-r 8000|11025|16000] [-n 3..5] [-g dB] "text" out.wav
make data       # data/ESPK.DAT (English-only, 378 KB) from the host build in build-host/
make dos        # DJGPP: ESPK.EXE      make install -> share/espk/{ESPK.EXE,ESPK.DAT}
c:\espk\espk [-r 8000] [-n 3] [-s 3] [-g 6] [-S 175] [-w out.wav] [-q] text...   # BLASTER= honoured
```
Whisper WER: `uv run tools/wer_eval.py --stable [--model small.en] results/sentences24.txt
"cmd:x:./espk_host {text} {wav}" "cmd:ref:tools/espeak_ref16.sh 8000 {text} {wav}"`.
Levels: `uv run tools/levels.py a.wav b.wav`.  Timing: `run/dosbox/espk-*.conf`.

## Source, licence

`upstream/` = git clone of https://github.com/espeak-ng/espeak-ng, commit
`699e79690f23e2558d990a3a78b0050745b96932` (2026-09-11).  Licence GPL v3 or
later (COPYING; the ucd-tools part is Apache/BSD but is not used, see below).
klatt_fx is a GPL klatt 3.04 port, so the combined program is GPL v3+.
`build-host/` is a normal cmake build of that commit (`-DUSE_MBROLA=OFF
-DUSE_LIBSONIC=OFF -DUSE_LIBPCAUDIO=OFF -DUSE_SPEECHPLAYER=OFF -DUSE_ASYNC=OFF`),
used as the reference `espeak-ng -v en+klatt` binary and as the data compiler.

## What is in the subset

`lib/` = 18 files copied from `upstream/src/libespeak-ng/` (19.5 k lines,
607 KB of the upstream 32.4 k lines / 951 KB): translate, translateword,
dictionary, numbers, tr_languages, readclause, phonemelist, setlengths,
intonation, synthdata, synthesize, voices, common, mnemonics, langopts,
phoneme, error, encoding, plus the headers.  Changes against upstream
(`results/lib-vs-upstream.diff`, 96 lines): `synthdata.c` ReadPhFile and
`dictionary.c` LoadDictionary take their files from the data pack;
`voices.c` reads its two voice files from embedded strings (`espk/voices_txt.c`).
Slim replacement headers in `lib/` for the dropped modules: `speech.h`,
`wavegen.h`, `mbrola.h`, `soundicon.h`, `ssml.h`, `compiledict.h`, and
`config.h`.  Everything else is new, in `espk/` (1.2 k lines):

- `wgout.c` (580 lines): consumes espeak's wavegen command queue
  (`wcmdq`), the part of `wavegen.c` and `klatt.c` that the frontend needs.
  WCMD_KLATT frame pairs -> `kfx_frame_t` with the same parameter mapping as
  `klatt.c` SetSynth_Klatt/Wavegen_Klatt but in Q8 integers (formants F1-F5
  and F0 per 64-sample step, bandwidths F1-F3, nasal zero, AV/tilt/aspiration/
  skew, parallel amplitudes); WCMD_WAVE sampled consonants and WCMD_WAVE2
  mixed samples resampled from 22050 Hz by a Q16 phase accumulator with box
  averaging; pauses, pitch envelopes, amplitude, embedded commands.  All
  lengths stay in espeak's 22050 Hz units and are converted to the output
  rate by an exact remainder accumulator.  The per-phoneme amplitude is
  folded into Klatt Gain0 through an integer 20*log10 (`db_ratio`), so the
  vowel path is `kfx_render8` straight into the output buffer.
- `espk.c`: init (speech.c's espeak_ng_Initialize without files, devices,
  events, async) and the Synthesize loop, with separate timers for the
  frontend (SpeakNextClause/Generate) and the output stage (wgout_run).
- `espk_data.c` + `tools/mkdat.py`: one packed file ESPK.DAT (phontab,
  phonindex, phondata, intonations, en_dict; 16-byte aligned; read with one
  fread into malloc'd memory, so no 8.3 name problems and no directory tree).
- `ucdmini.c` + generated `ucdtab.h`: replaces the 740 KB ucd-tools with
  categories and case maps for U+0000-052F (Latin, Greek, Cyrillic), the
  espeak punctuation properties for ASCII, and the wide-character functions
  DJGPP 2.05's libc lacks (`wcompat.h`; DJGPP's wchar_t is 16 bit, fine for
  English).
- `stubs.c`: sound icons, SSML tags, markers, phoneme-trace rule decoding.
- `main_dos.c` / `main_host.c`.

Dropped: wavegen's formant synthesizer, klatt.c's float parwave, MBROLA,
libsonic, pcaudio, async/fifo/event, espeak_api/command, SSML/markup, sound
icons, the dictionary/phoneme compilers, speechPlayer, all non-English
dictionaries and phoneme tables.  From the Klatt path itself: F0 flutter
(float sines), the 64-sample fade in/out at segment ends, echo.

Data: `tools/mkdata_en.sh` compiles the phoneme master file cut down to the
tables `base1` and `en` and compiles `en_dict` against it with the upstream
compiler: phondata 186 KB (600 KB with every language), phontab 3.3 KB,
phonindex 3.8 KB, intonations 2.6 KB, en_dict 182 KB; ESPK.DAT 378 KB
(898 KB with the full phoneme data; output is byte-identical).

## Sizes

ESPK.EXE 534 KB on disk (DJGPP: text 340 KB including libc and libemu, data
32 KB, bss 109 KB).  Our own objects: 195 KB of code+const tables (dictionary
24 KB, translate 20 KB, numbers 19 KB, tr_languages 16 KB, synthesize 13 KB
+47 KB bss frame pool, translateword 12 KB, voices 12 KB, ucdmini 11 KB,
readclause 10 KB, intonation 8 KB, encoding 8 KB, wgout 5 KB, klatt_fx 6 KB).
Heap after loading: 564 KB (data pack 378 KB + dictionary copy 182 KB, the
rest is translator and phoneme structures), 644 KB after a 6 s utterance
(the output buffer; DOSBox-X figures, QEMU/CWSDPMI reports 1.1 MB because of
its allocation granularity).  Total well under 2 MB of the 8 MB.

## Intelligibility (Whisper WER, results/sentences24.txt, 241 words, --stable)

| Voice | base.en | small.en |
|---|---|---|
| espeak-ng en+klatt, native 22050 Hz 16-bit (this commit) | 10.4 % | 9.1 % |
| espeak-ng en+klatt, downsampled to 8 kHz 16-bit | 26.6 % | 17.8 % |
| espeak-ng en+klatt, 8 kHz 8-bit | 30.7 % | - |
| **ESPK 8 kHz 8-bit** (defaults: F1-F3 cascade, 8.7 ms frames, +6 dB) | 29.5 % | - |
| ESPK 8 kHz, level +3 dB (`-g 3`) | 26.1 % | **10.0 %** |
| ESPK 8 kHz, level +0 dB | 26.6 % | - |
| ESPK 11025 Hz, F1-F4 (`-r 11025 -n 4 -g 3`) | 22.0-23.7 % | 14.9 % |
| ESPK 16000 Hz, F1-F5 (`-r 16000 -n 5 -g 3`) | 18.7 % | 16.6 % |
| ESPK 8 kHz, 2.9 ms frames (`-s 1`) | 29.0 % | - |
| ESPK 8 kHz, Kopen 25 | 29.5 % | - |
| ESPK 8 kHz, F1-F4 cascade (`-n 4`) | 45.6 % | - |
| for reference (src/klatt/NOTES.md): rsynth-based DOS voice | 54-58 % | 34-35 % |
| SAM 22 kHz | 33 % | 30 % |

So the port scores the same as espeak-ng's own Klatt voice at the same rate
(base.en 26-30 % vs 27-31 %; small.en 10 % vs 18 %), and half the error of
the rsynth-based voice.  espeak itself loses most of its intelligibility
going from 22 kHz to 8 kHz (10 -> 27 % base.en), which is where the
remaining gap sits; on a 386 an 11 or 16 kHz output rate is affordable (below).
Whisper base.en single runs move by +-3 points; small.en is steadier.
Putting F4 into the 8 kHz cascade hurts, as it did for the rsynth voice.
The level calibration (`tools/levels.py`, `gain_ref` 270) makes the Klatt
path match espeak-ng's own vowel level so that the sampled consonants keep
espeak's balance; `-g` then raises everything for the 8-bit path (+6 dB peaks
at -5 dBFS).

Verified through DOS: QEMU FreeDOS, `c:\espk\espk Please open the file and
read the first line. Twenty five people came to the party on Friday night.`
captured from the Sound Blaster (`results/espk-qemu-2.wav`), Whisper: "Please
open the aisle and read the first line, 25 people came to the party on Friday
night."  (`results/espk-qemu-hello.wav` is the first test sentence.)

## Speed

DOSBox-X, 386 core at fixed cycles (5000 = the real 386 DX-25 for integer
code by the SAM calibration; 12000 = 486DX-33 class), two sentences, 6.15 s
of audio, `-q` (no playback):

| Cycles | Rate | Load data | Frontend | Synthesizer | Total | x real time |
|---|---|---|---|---|---|---|
| 5000 | 8000 Hz F1-F3 | 235 ms | 330 ms | 1676 ms | 2.0 s | **3.07x** |
| 5000 | 11025 Hz F1-F4 | 379 ms* | 330 ms | 2303 ms | 2.6 s | 2.34x |
| 5000 | 16000 Hz F1-F5 | 379 ms* | 330 ms | 3354 ms | 3.7 s | 1.67x |
| 12000 | 8000 Hz | 222 ms* | 137 ms | 696 ms | 0.83 s | 7.4x |
| 12000 | 16000 Hz F1-F5 | 221 ms* | 137 ms | 1392 ms | 1.5 s | 4.0x |

(* with the 898 KB full data pack.)  The frontend costs 5 % of real time on
the 386 (54 ms per second of speech) and the synthesizer 27 %.  Host callgrind
on four sentences: 1.55 M instructions per second of audio in total, of
which kfx_sample 0.92 M (60 %), the queue consumer with the sample resampler
0.14 M (9 %), kfx_set_frame+setabc 0.09 M (6 %), and the whole frontend
about 0.2 M (13 %; MatchRule in dictionary.c is the largest single item).
This is 3x faster than the Watcom 16-bit SBTALK3 Klatt (0.50x on the real
machine): DJGPP's 32-bit code for the same integer engine, F1-F3 in the
cascade, and espeak's frames leave the parallel branch idle for most vowels
(kfx skips idle sections).  There is no floating point on the synthesis
path (the DOS objects were audited with objdump: the only x87 code is the
`pitch` keyword branch of LoadVoice, never reached; `-lemu` is linked as a
safety net for printf).

## What a resident version would need

The resident SBTALK is a 16-bit real-mode TSR (Watcom, ~85 KB).  This
engine is 32-bit DJGPP code with ~0.5 MB of code and ~0.6 MB of heap, so it
cannot be a real-mode TSR.  The workable shape is the "shell wrapper" from
PLAN.md section 5.4: ESPK runs as a DPMI process (CWSDPMI, or an rsx/ HDPMI
that does not conflict with EMM386), installs the INT 14h emulation as a
real-mode callback (`__dpmi_allocate_real_mode_callback`, which switches
to protected mode for every serial port call; DoubleTalk traffic is a few
hundred bytes per second, fine), owns the Sound Blaster with auto-init DMA
and an IRQ handler (`_go32_dpmi_chain_protected_mode_interrupt_vector`,
locked memory), and then spawns COMMAND.COM (`system("COMMAND.COM")`) so the
user's session runs inside it; the synthesizer runs from the timer or IRQ
context, or better as the coroutine of the existing design but in
protected mode.  Points to verify: (1) CWSDPMI's real-mode callback and
protected-mode IRQ latency on a 386 with the screen reader polling INT 14h
(Provox checks status bits continuously: each check is a mode switch,
~tens of microseconds on a 386, so INT 14h status polls should be answered
from a small real-mode stub that reads a shared flag, with only data
transfers going to protected mode); (2) DOS re-entrancy: synthesis must not
call DOS (it does not: all data is in memory; only `malloc` growth must be
avoided while resident, so the output ring is preallocated); (3) memory: the
DPMI host, the wrapper and its heap take ~1 MB of extended memory, and the
conventional memory cost is the DPMI host's transfer buffer plus
COMMAND.COM, so the 386's 8 MB is fine; (4) Provox and JAWS under a DPMI
client: they are real-mode programs and do not care, but a second DPMI
program (a game) must be able to nest under CWSDPMI (it can, CWSDPMI
supports nested clients).  Simpler alternative for the first resident
attempt: keep SBTALK as the real-mode COM-port TSR and move only the
text-to-frames step into the wrapper, sending 30-byte Klatt frames to a
tiny real-mode player through a shared buffer.

## Resident ESPKD (2026-09-20)

`ESPKD.EXE [COMn] [/SPK] [/TEST] [/11K] [/PMALL] [/NOSTI] [/C command]` (DJGPP, 567 KB,
`CWSDPMI.EXE` beside it, sources in `espkd/`) is the resident form: a "shell wrapper" DPMI
process.  It loads ESPK.DAT, initialises eSpeak NG + klatt_fx, warms the pipeline up, locks
its memory, installs the output driver and the INT 14h DoubleTalk emulation for COMn
(default COM3), says "E speak ready", and spawns `COMSPEC` (or `COMSPEC /C command`).
Everything run from that shell, Provox and JAWS included, sees a DoubleTalk on COM3;
`EXIT` uninstalls everything.  `make dos install` puts ESPKD.EXE, ESPK.DAT, LOOPS.EXE,
INT14B.EXE, the test batches and a CWSDPMI with paging disabled into `share/espk/`.

Design: sbtalk/dos.c moved to protected mode, reusing sbtalk's `ring.c` and `synth.c`
unchanged (ring enlarged to 8192 samples = 1 s) plus `engine_espk.c` (the engine hook:
`espk_speak_stream` streams samples through `wgout_sink` into `ring_write_block`, which
yields when the ring is full; a flush raises `wgout_abort`, the render loops in wgout.c
skip their work and the frontend finishes the clause, no longjmp through espeak's state).

- **Coroutine.** The synthesizer (translate, phoneme list, Klatt frames, klatt_fx, the
  ring) runs on its own 192 KB stack (46.8 KB used, measured), resumed from the output
  interrupt with interrupts re-enabled and yielding when the ring is full or no text is
  pending; `coro.S` is the 32-bit twin of sbtalk's `co_switch` with SS in the context,
  because a hardware interrupt handler runs on the DPMI host's locked stack, not on DS.
  CWSDPMI handles the nesting this creates correctly (checked in its source, TABLES.ASM
  / DPMISIM.ASM): a nested IRQ arriving in protected mode continues on the interrupted
  stack, and one arriving in real mode (e.g. inside the timer handler / Provox that the
  host reflected to real mode while the synthesizer was running) uses the client stack
  saved at that reflection, i.e. our synth stack, never the top of the 4 KB locked stack.
- **What did not work: DJGPP's iret wrapper.**  `_go32_dpmi_allocate_iret_wrapper`
  (this libc) switches to a private stack and keeps an in-use flag; a nested call while
  the flag is set jumps straight to `popa; iret` without calling the handler.  With the
  coroutine that is every second SB interrupt (the handler is still "inside" the wrapper
  while the synthesizer runs): the second IRQ was taken, never acknowledged, the PIC
  stayed in service and the DSP stopped (`/TEST` showed `isr=20 sbint=21 dmapos=512`,
  one IRQ ever, on CWSDPMI and CWSDPR0 alike).  `coro.S` therefore carries its own
  20-byte wrappers for the SB and RTC vectors (save registers, load the data selector
  patched in at install time, call, iret).  The real-mode-callback wrapper has the same
  flag but still performs the real-mode iret fixup when it drops a call, which only ever
  affects a redundant `/SPK` wake, so it is used as is.
- **INT 14h.**  The real-mode vector points at a 256-byte real-mode stub in DOS memory
  (`rmstub.S`, assembled by the DJGPP `as` in 16-bit mode, no relocations).  For our port
  it answers AH=0/3 (status 60B0h) and AH=2 (timeout) itself and far-jumps into the DPMI
  real-mode callback (`_go32_dpmi_allocate_real_mode_callback_iret`) only for AH=1, whose
  iret return goes straight back to the caller; other ports chain by a far jump through
  the saved old vector, all in real mode.  Cost, measured with INT14B.EXE from the child
  shell (5000 calls each, DOSBox-X): at 386 DX-25 cycles a status poll costs 33 us
  (identical to the BIOS with no port, i.e. the stub itself is free) against 121 us when
  routed to protected mode (`/PMALL`), and a data byte (callback) 132 us; at 486 class
  11 / 55 / 55 us.  Provox polls status about once per byte (1469 polls for 1497 bytes
  in one run), so the stub halves its mode switches: LOOPS during Provox reading at
  386 speed 195 ticks with the stub, 201 with `/PMALL` (129 idle).
- **IRQs while the child runs in real mode** (the feasibility question): yes.  CWSDPMI
  reflects a hooked hardware interrupt that arrives in real mode through an internal
  real-mode callback into the protected-mode handler.  `/TEST` spends 3 s in protected
  mode and 3 s spinning in real mode (the `rmwait` stub) and both phases show the SB
  IRQs (15.6/s) or the RTC ticks and wakes flowing and the ring staying full; the child
  shell, batch files, LOOPS, SAY, Provox and `mem` all run under it in QEMU and DOSBox-X.
- **Outputs.**  SB: auto-init 8-bit DMA, 2 x 512 samples (64 ms halves) in DOS memory
  below 1 MB not crossing 64 KB, refilled from the SB IRQ (`_farnspokeb`), DSP >= 4 and
  pre-SB16 command paths as in sbtalk.  `/SPK`: RTC periodic interrupt at 8192 Hz, PWM on
  PIT channel 2, 8000 Hz samples stepped to 8192.  A protected-mode handler for 8192
  ticks/s would cost a mode switch per tick whenever the CPU is in real mode (the child),
  so the stub carries the IRQ 8 handler and an 8 KB PWM ring in DOS memory: ticks that
  arrive in real mode never leave real mode; ticks that arrive in protected mode go to an
  identical protected-mode handler (both hooked, RM vector set after the PM hook); every
  512 ticks (62.5 ms) the stub `pushf; lcall`s a wake callback that tops the DOS ring up
  from the sample ring and resumes the synthesizer.  Flush also drops the DOS ring.
- **Memory locking (DPMI host requirement).**  CWSDPMI aborts the program on any page
  fault inside a real-mode callback or hardware interrupt, and it commits pages lazily,
  so everything is locked: `_CRT0_FLAG_LOCK_MEMORY | _CRT0_FLAG_NONMOVE_SBRK`, then one
  `__dpmi_lock_linear_region` over [image page 1, sbrk(0)) after init (the heap blocks are
  contiguous after the image under CWSDPMI); a failed lock is fatal.  The shipped
  `share/espk/CWSDPMI.EXE` is the stock r7 with the swap file name cleared in its CWSPBLK
  parameter block (`tools/cwsnoswap.py`, what CWSPARAM's "" answer does): no virtual
  memory, so a memory shortage fails at start-up instead of as a page fault later.  That
  bit: under the FreeDOS LiveCD in QEMU the ramdisk leaves 65-430 KB of XMS, CWSDPMI
  paged to C:\CWSDPMI.SWP plus "640K paging", the lock quietly failed, and an 8 MB run
  died with `Page Fault cr2=004b5d00 in RMCB`; the QEMU tests therefore boot a floppy
  built by `run/mkbootimg.py` (talking disk with a bare FDAUTO.BAT, no ramdisk).
  CWSDPR0 (ring 0, no paging, lock is a no-op) also runs it.  Locked size: 780 KB image
  (text 362 KB, data 34 KB, bss 385 KB of which 192 KB synth stack, 64 KB malloc arena,
  8 KB ring) + 560 KB heap = ~1.4 MB; ESPK.EXE used 1.1-1.2 MB.
- **No DOS, no malloc from interrupt context.**  The warm-up speaks two texts with
  numbers, punctuation and abbreviations before installing; `malloc/calloc/realloc/free`
  are wrapped (`-Wl,--wrap`): on the synth stack they come from a static 64 KB arena and
  are counted; every run reports 0 such allocations.  The espeak paths that could
  allocate (`AddNameData` for SSML marks, the phoneme-string buffer, `DoVoiceChange`)
  are off or init-only; `InitText(espeakKEEP_NAMEDATA)` per utterance.

CPU cost, DOSBox-X, 386 core, `LOOPS 1500000` in the child shell (ticks, 55 ms; idle vs
while `SAY /F LONG.TXT` renders a 45 s passage; `run/dosbox/espkd-*.conf`):

| Cycles | Output | Idle | Speaking | CPU taken by ESPKD |
|---|---|---|---|---|
| 5000 (386 DX-25) | Sound Blaster | 129 | 185, 193 | **32 %** (ESPK.EXE measured 27 % synth + 5 % frontend) |
| 5000 | PC speaker | 139 (7 % for the 8192 Hz stub while idle) | 248, 259 | 45 % (PM-arriving ticks cost a ring transition each) |
| 12000 (486DX-33) | Sound Blaster | 53 | 65, 59 | 15 % |
| 12000 | PC speaker | 56 | 65, 64 | 13 % |

`/TEST` at both speeds: no mid-utterance underrun on either output (the counter includes
the partial first fill of an utterance and shows 0-1 there); in the Provox run at 5000
cycles one underrun in 9 utterances, where a short clause is followed by the frontend
burst of the next one (the ring holds 1 s).  The foreground is not blocked: LOOPS and
Provox run while it speaks, key presses flush.

Verified in QEMU (FreeDOS 1.4 booted from `run/mkbootimg.py`'s floppy with 8 MB and HIMEMX,
share/ as C:, private VM `run/qemu-espkd.sh`, captures downsampled in `results/espkd-qemu-*.wav`):
`ESPKD COM3` then, in the child shell, `SAY hello` -> Whisper "Hello"; `SAY Please open the
file and read the first line. Twenty five people came to the party on Friday night.` ->
small.en "Please open the pile and read the first line. 25 people came to the party on Friday
night." (the same as ESPK.EXE's result); a long `SAY` followed by `SAY /X` about 2 s later:
the RMS envelope shows 2.3 s of speech then silence, transcript "This is a very long
sentence, I should..."; `PROVOX7` + `PV7 LITETALK COM3` in the child: Provox reads the boot
screen backlog through ESPKD (45 s of it, Whisper recognises the FreeDOS kernel banner and
GPL notice with "semicolon", "period", "colon", "slash" spoken, 53 utterances, 1911 bytes
and 1647 status polls over INT 14h, no underrun); then `echo Provox reads the screen through
E speak D on Free DOS > con` -> small.en "Robot reads the screen through ECT on 3DLS", and
the fox sentence -> "The unique brown-pot stunts over the lazy dock" (Whisper's usual trouble
with short 8 kHz lines; the words are there by ear-shaped structure and the sentence set
scores 10 % WER).  `EXIT` unloads (the SB is reset, vectors, callbacks and DOS blocks
restored) and `dir`, `mem` and a second `ESPKD` run all work afterwards.  `/SPK /TEST` in
QEMU: 8192 ticks/s split between the real-mode stub and the protected-mode handler
according to where the CPU is (24379 pm / 25023 rm over 6 s), 95 wakes, ring full; QEMU
cannot render the PWM audio, so the speaker path is verified for flow only, as for sbtalk.

Conventional memory, `MEM /C` inside the child shell (8 MB FreeDOS, HIMEMX, no other
drivers; DOSBox-X figures are the same within 5 KB): SYSTEM 13 KB, HIMEMX 2 KB, **ESPKD 45 KB**
(real-mode stub + 8 KB PWM ring, 2 KB DMA buffer, PSP/environment, 4 KB DJGPP transfer
buffer (stub patched by `tools/stubsize.py`), CWSDPMI's page tables and heap that DOS
books to ESPKD's PSP), **CWSDPMI 43 KB**, child COMMAND.COM 71 KB + 3 KB environment,
**461 KB free for programs** (620 KB before ESPKD; with the LiveCD's FreeCom in XMS-swap
mode 425 KB).  Provox (29 KB) fits with room for ordinary DOS programs; a TSR loaded inside
the child stays resident after EXIT, as any TSR would.  XMS: CWSDPMI takes it all for its
pool while ESPKD runs (4.9 MB free inside the pool after ESPKD's 1.5 MB), nested DPMI
programs run from that pool.  Extended: ESPKD.EXE 567 KB on disk, heap 560 KB after init,
623 KB after the tests, the same as the non-resident ESPK.EXE.

Files: `espkd/espkd.c` (driver, ISRs, INT 14h, install/uninstall, /TEST), `espkd/coro.S`
(co_switch and the two IRQ wrappers), `espkd/rmstub.S` (real-mode stub, `rmstub_bin.h`
generated), `espkd/engine_espk.c`, `espkd/ring.[ch]` `synth.[ch]` `engine.h` (from sbtalk),
`espkd/loops.c` `int14b.c` (real-mode benchmarks), `espkd/dos/*.BAT` `LONG.TXT` (test batches
for the child shell), `run/qemu-espkd.sh` `run/mkbootimg.py` (private QEMU test rig),
`tools/cwsnoswap.py` `tools/stubsize.py`, `../../run/dosbox/espkd-{sb,spk}-{5000,12000}.conf`
(DOSBox-X: /TEST, BENCH.BAT, PVBENCH.BAT with and without /PMALL, I14BENCH.BAT; outputs in
`share/out/E*.TXT`).  Changes to the shared espk core: `wgout.c` gained `wgout_sink` and
`wgout_abort`, `espk.c` `espk_speak_stream`, `espk_set_rate/pitch`; ESPK.EXE is unchanged
in behaviour.

What remains for the 386 (DR-DOS 7.02, EMM386, SB DSP 4.16): (1) CWSDPMI runs under
EMM386 via VCPI there, so the reflection and callback costs measured above are for raw
mode; measure with LOOPS.EXE / INT14B.EXE / BENCH.BAT on the machine (the 32 % SB figure
should hold, the /SPK figure may not); (2) DPMI memory: with EMM386 present CWSDPMI takes
VCPI memory (the docs advise `JEMM386 LOAD MAX=0`-style limits for speed) and the lock must
succeed, ESPKD refuses to run if it does not; (3) the real SB IRQ 5 / DMA 1 latency on the
ISA bus with the coroutine holding the CPU for tens of ms at a time: `ESPKD COM3 /TEST` shows
underruns if any; (4) JAWS for DOS at C:\JAWS configured for a DoubleTalk LT on COM3, which
only exercises the same INT 14h path as Provox; (5) the speaker PWM sound itself, which no
emulator renders; (6) DR-DOS's COMMAND.COM as the child (`COMSPEC`), and its 127-character
command line for `ESPKD COM3 /C ...`.
