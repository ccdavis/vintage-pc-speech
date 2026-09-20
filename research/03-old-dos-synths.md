# Old DOS software synthesizers: what they were, what survives, what is worth porting

Status: research notes, 2026-09-19. Question from the user: can the code of "Tran" (PC-speaker TTS he
remembers) or of Monologue/SmoothTalker (the Dr. Sbaitso engine) be obtained and ported to play
through Sound Blaster DMA? Context: `PLAN.md` (SAM works; fixed-point Klatt plus eSpeak NG frontend in
progress). Conventions: **[verified]** = read in the primary source or in files downloaded and
inspected for this report; **[recall]** = memory, not re-checked; **[inference]** = my reasoning.

## 1. TRAN (Stephen T. Neely, Omaha NE, 1988-1990)

The user's recollection is right in every detail. TRAN is a shareware text-to-speech program for the
PC speaker. Two releases exist **[verified: both downloaded and unpacked into `src/tran/`]**:

- **TRAN 1.01, September 1988** (`TRAN.EXE` 48,163 B, Microsoft C 5 runtime, plus `TRAN.DOC`). The
  doc: "translate normal English spelling to phonemes, and sound out each phoneme through the speaker
  of the IBM-PC"; letter-to-sound rules from Elovitz, Johnson, McHugh and Shore (NRL, 1976); "35
  phonemes, each encoded as a sequence of bits controlling the position (in or out) of the PC speaker.
  The phoneme codes come from the public domain program SPEECH by Andy McGuire" (McGuire's 1983
  copyright string is embedded in the EXE). Timing is by software delay loops tuned per machine
  (`d1`, `d2`). Copies: archive.org `msdos_TRAN_shareware` and `msdos_shareware_fb_TRAN-101`, the
  Sound Sensations CD (`cd.textfiles.com/soundsensations/VOICE/ZIPFILES/TRAN.ZIP`, CRC32 CED1F453),
  Garbo ("TRAN.ZIP 32457 Sep 10 1988"), Vetusware. All contain the same 1988-09-09 EXE; the zips
  differ only in packing. A 2009 VCFed thread ("Looking for an old program called Tran") shows others
  remember it the same way.
- **TRAN version 0.4, September-November 1990** (`TRAN04.ZIP`, 154 KB EXE, "Copyright (c) 1989-90
  Stephen T. Neely", shareware $20 to "STN software"). Despite the lower number it is the later,
  bigger program: 47 phones in two sets, the 1-bit set at "about 16 kilohertz" for the speaker and an
  **8-bit set recorded from the author's own voice with a Covox Voice Master, played at 10 kHz** to any
  DAC on an I/O port (LPT, Covox VMK, hex port) or to a file (`-f`; `-Z` dumps every phone as a
  waveform file). Rules and phone tables ship as text (`RULES.TXT`, `PHONES.TXT`, `PHNM_DUR.TXT`).

Method: rule-based text-to-phoneme, then **concatenation of fixed sampled phones** (1-bit or 8-bit)
with no coarticulation and no prosody beyond word pauses. CPU: XT class. Licence: shareware, no
source ever distributed; no trace of source on Simtel, Garbo, PC-SIG, textfiles, GitHub. Stephen
Neely is, by name and city, the Boys Town National Research Hospital hearing scientist
**[inference from the address; not confirmed]**. **Porting assessment: nothing to port.** The rules
are the NRL set we already have in `src/klatt/nrl.c`; the voice is fixed samples below SAM's class.

## 2. The SPEECH.COM family (every PC-speaker TTS of the era descends from it)

- **SPEECH.COM, Andy C. McGuire, December 1983** (21,376 B, file date 1985-08-19; embedded string
  "Speech by Andy C. McGuire Copyright 1983, All Rights Reserved", and TALKDEMO.BAS says "Copyright
  December 2, 1983", so it is copyrighted freeware that the BBS world called public domain)
  **[verified: four identical copies (md5 8cceb99a...) unpacked into `src/speechcom/`]**. Companion
  files, all in `src/speechcom/`: `TALKDEMO.BAS` (McGuire, 1984, the demo that speaks "the rain in
  Spain"), `SAY.COM` by Thom Henderson (of ARC), `SAY.ASM`/`READ.ASM` by Douglas Sisco (1985,
  source), `T-SPEECH.COM` (a 4-byte patch of SPEECH.COM that takes a Turbo Pascal string instead of a
  BASIC descriptor), `SPEECH.PAS`/`SPEECH.BIN` (Turbo Pascal external binding), `TXT2SPCH.EXE` by
  Teddy Maningo (C, text-to-phoneme front end, no source), `SAYTIME` (Laith Farjo, 1986, talking
  clock TSR), `TQUERY.BAS` (1986, a talking phone book "for the visually impaired individual", the
  only assistive use found in print). Sources: the Sound Sensations CD (SPEECH, SPEECH2, SPEECHVI,
  TSPEECH, TXT2SPCH) and **PC-SIG disk #1668 "Speech"** (rebuilt from the pcjs JSON sector dump into
  `src/speechcom/pcsig1668/`, image and files) **[verified]**. The PC-SIG disk is a cautionary
  tale: in 1988 one Vincent Poy of San Francisco repackaged McGuire's binary (identical except two
  bit-rotted bytes in the TH waveform) as "$19.95 shareware, Copyright 1988 Vincent Poy, NOT a public
  domain or free program", with a Max Headroom demo and no credit to McGuire; PC-SIG's blurb adds
  "AT and 386 users will need to slow their computer down to about 7.5 MHz". **No source for
  SPEECH.COM itself exists**; it is small enough that the disassembly below is the documentation.

  **How SPEECH.COM works, from disassembly** **[verified: ndisasm of the binary]**:
  - Install: checks INT F1's vector for its own code, then sets **INT F1h** (handler 0x15E: DS:BX ->
    `$`-terminated phoneme string, the API RBIL lists) and **INT F2h** (handler 0x192: stack holds a
    pointer to a BASIC string descriptor {len, offset}; the BASIC path also clears the Ctrl-Break
    flag at 40:71h and aborts the utterance on Break), writes a 5-byte stub `INT F2h; RETF 2` over
    the INT F3h vector at 0000:03CCh so BASIC's `CALL` has a fixed far address (this is what SAY.ASM
    tests for), and goes resident with INT 27h keeping 0x543C bytes (21.5 KB).
  - Text: spaces are word pauses (a `LOOP` of 0x7000 iterations, roughly 0.1 s on a 4.77 MHz PC),
    `-` separates phonemes, letters are upper-cased, codes are matched first against the 15
    two-letter names at 0x304 (`AW AH UH AE OH EH OO IH EE WH CH SH TZ TH ZH`) and then the 21
    single letters at 0x2EF (`U A I B D G J P T K W Y R L M N S V F H Z`). `I` is special-cased and
    played as `AH` then `EE` (its own record is all zeros), so there are 36 codes but 35 waveforms,
    exactly TRAN.DOC's "35 phonemes".
  - Data: a flat table at 0x322 (file offset 0x222) of **36 fixed-size records of 0x23F = 575 bytes
    = 4,600 bits each** (20,700 bytes, ending at the copyright string); record = index for single
    letters, 21 + index for two-letter codes. Each record is **1-bit PCM, MSB first, about 48 %
    ones**, no length field, no envelope, so every phoneme has the same duration; horndrv's
    `PHONEME.C` arrays are these records byte for byte (`achPhonemeU` = record 0).
  - Playback: the handler saves port 61h, clears bit 0 (timer gate) and self-patches two `MOV AL,imm`
    with the speaker-data-on and -off values; the loop `SHL AH,1 / JC / MOV AL / OUT 61h / DEC DL /
    JNZ` drives the cone directly from each bit with only instruction timing as the clock (DX =
    0808h: 8 bits per byte, no calibration). On a 4.77 MHz 8088 that is roughly 100-120 cycles per
    bit, i.e. about 40 kbit/s and 0.1 s per phoneme; on anything faster it is unintelligible, which
    is why TRAN re-timed the same bits with `d1`/`d2` (its doc calls the rate "about 16 kHz") and
    horndrv retimed them from the DRAM-refresh timer. The exact original rate needs a cycle-accurate
    8088 run (86Box/PCjs) to measure.

  Reuse: slice SPEECH.COM at 0x222 + 575*n, unpack bits MSB first, emit 0/255 samples at the chosen
  rate (16 kHz per TRAN, or measure) and the 1983 voice plays through our DMA path unchanged. The
  data is McGuire's copyrighted freeware; horndrv redistributes the same bytes under "public domain
  as FREEWARE"/GPL.
- **horndrv / TALK.SYS, Jon Hornstein (Melbourne), written 1992, released 1995, source 1999**
  **[verified: `horndrv2.zip` (108,936 B) from ftp.sunet.se Simtel mirror `msdos/sound/`,
  unpacked]**. A DOS device driver (`$talk$` text and `$phonem$` phoneme devices) in Turbo C++ 3.0
  with a 141-line assembler stub; 5.3 K lines of C: `ENGLISH.C` (NRL-style rules "adapted from the
  TRAN program"), `PHONEME.C` (120 KB of 1-bit phoneme arrays hex-dumped out of SPEECH.COM, one
  `achPhoneme*[]` per phoneme, played in `PlayPhoneme` via port 61h), `RING.C`, `PARSER.C`,
  `EXECDRV.C`. Needs a 286-8. Licence: TALK.DOC says "placed in the public domain as FREEWARE",
  readme.txt says "a licence based upon the GNU concept", gpl.txt enclosed. **This is the only
  complete-source DOS PC-speaker TTS found.** Not copied into the repo (scratchpad only).
- **freebasic-dos-speech (angros47, 2013/2017)**: the same phoneme data and TRAN's rules, output
  through a Sound Blaster, in FreeBASIC. The SourceForge download returned HTML for me three times;
  unverified beyond the forum description.
- Also on the same CD, none with source or of interest: `SPEECH98` (SPEAK.COM by "W. Pound", 98
  phonemes, 1991), PCSPEAK/AUTOTALK/PC-Talk/SW-Talk (sample players, not synthesis), TALKFUN.BAS
  (spells letters), and hardware-driver packs (Votrax, Artic, CallText, DECtalk, Echo).
- **PC-TALKER / READSPF / SPEAKER (Király József, Hungary, 1990-91)**: PC speaker (PWM) and Sound
  Blaster, 5 s of the author's clipped voice at 9,178 Hz concatenated; binaries only, Hungarian
  only; the author rewrote them in Python in 2026 (`github.com/tgeczy/pctalker-nvda`) **[verified]**.
- **klattsch for MS-DOS (Tony Gies / Crash United, 2026)** **[verified: zip downloaded]**: a
  fixed-point parallel-formant synth, DJGPP, Sound Blaster streaming, phoneme input only (no text
  front end), "386 or better, no FPU needed". Its README's timing is a useful external calibration for
  our own Klatt: "A 386DX-33 is smooth up to about 8 kHz (11 kHz is borderline), any 486 should do
  about 11-16 kHz". The JS engine is MIT on GitHub; the DOS C port ships as binaries only.

Assessment for the family: the 1-bit phoneme tables are extractable (GPL/freeware via horndrv) and
would make a 2 KB "1983 voice" novelty engine at negligible CPU, played as 0/255 samples through DMA.
Not a step up from SAM.

## 3. First Byte SmoothTalker / Monologue / SBTALKER (the Dr. Sbaitso engine)

**Ownership.** First Byte (Santa Ana, later Long Beach CA; a Parsippany NJ office sold ProVoice to the
telephony market in 1993) became a subsidiary of Davidson & Associates (Torrance); Davidson was bought
by CUC International in 1996, which became Cendant Software, then Havas Interactive (1998-2001)
**[verified: Wikipedia, Telecompaper]**, then Vivendi Universal Games and Activision Blizzard (2008),
now Microsoft **[recall/inference]**. Consistent with this, Google Patents shows the First Byte
patents currently assigned to **Sierra Entertainment, Inc.** (a Vivendi/Activision label)
**[verified]**. Creative Labs holds copyright on SBTALKER's packaging (`READ.EXE`, 1989-1991).

**Method, from the patents printed in the engine's banner** **[verified: US 4,692,941; 4,617,645;
4,805,220 on Google Patents]**. It is neither a formant synthesizer nor LPC:

- 4,692,941 (Jacks and Sprague, filed 1984, "Real-time text-to-speech conversion system"): text to
  phonemes by an exception dictionary plus letter rules; speech by **concatenating very small stored
  waveform segments for phoneme portions and the transitions between phonemes**; pitch changed by
  **truncating or extending each voiced period** (sample-count per period); voiced transitions
  interpolated between adjacent phoneme waveforms; the whole engine in "40-50K".
- 4,617,645 (Sprague, 1984): 4-bit delta compaction of the waveform store, S(n) = 2S(c) - S(p) + delta
  from a 16-entry table, about 2:1.
- 4,805,220 (1986): PWM speaker output, 22.2 kHz carrier, 11.1 kHz sample rate, 32 levels.

So it is a compressed time-domain phoneme/transition concatenation synth with pitch-synchronous
period stretching, a PSOLA precursor. That explains the "8 MHz CPU" requirement, the ~220 KB (or 45
KB plus EMS) memory footprint of Monologue, and the reviewers' "sing-song ... not a native speaker"
voice (Baltimore Sun, 1991: "pronouncing them according to an elaborate set of rules", no digitized
recordings, 1,200 rules). V3 = "SmoothTalker 3.5" (1990, SBTALKER.EXE + BLASTER.DRV); V4 = "Text-to-
Speech Engine 4.1" (1991, SOUNDxxx.EXE drivers for Sound Blaster, Adlib, Covox, Disney, IBM, Tandy,
PC speaker, plus SPEECH.EXE, KERNEL.DIC, V4ENG*). All patents expired by 2007 **[inference from
filing dates]**.

**Source and API.** No source or internal documentation has ever leaked or been released; VOGONS has
nothing beyond version archaeology. `github.com/systoolz/dosbtalk` (Apache-2.0) is a cleaned
disassembly of the **client** side only: INT 2Fh AX=FBFBh detection, then far calls `DetectSpeech`,
`SetGlobals(gender, tone, volume, pitch, speed)`, `Parser(english, phonemes)`, `Say`,
`SpeakM`/`SpeakF(phonemes, ...)`, `ResetSpeech`, `SpeechVersion` **[verified: speech.h]**. Binaries:
WinWorld (Dr. Sbaitso 2.x; Monologue for DOS 3.1, 1992), archive.org (`smooth_talker_v2` 1988), the
Sound Sensations CD (`MONOLOG1.ZIP`). Status: abandonware; no licence to redistribute.

**Assessment.** Cannot be ported (no source). It could be *used* unmodified as a third engine behind
our INT 14h TSR through dosbtalk's calls, and the CPU cost would be trivial, but it eats conventional
memory, cannot be shipped on the disk legally, and its voice is what the project set out to beat.
Worth keeping only as a benchmark voice for WER comparison.

## 4. Other DOS synths asked about

- **DECtalk PC** (1993): an ISA card with its own processor; the DOS side is a driver TSR and firmware
  download **[verified: WinWorld, Linux dectalk_pc README]**. **DECtalk Software** (Win32, 1996+)
  source was shared by its developer Edward Bruckert in 2015 and is on `github.com/dectalk/dectalk`
  (LICENCE non-standard, no rights-holder release) **[verified]**; C, floating point, far heavier
  than a 386 and legally unclear. Not a DOS or 386 option.
- **Speech Plus Prose 2000 / CallText** (1982): 8086 board, KlattTalk descendant; hardware.
- **Klatt on DOS**: no "klsyn86" found. klsyn (Klatt's own C, "not for commercial products"), klatt
  3.04 (GPL, our base), klattsch (above) are the real ones.
- **rsynth** (Ing-Simmons, 1994): Unix, float; DJGPP-buildable; we already use its Holmes/NRL parts.
- **Texas Instruments TTS for DOS**, **Tom Jennings SPEAK.EXE**, **"Say It"**, **Roland Berg TTS**,
  **Blabber**, **Talking Head**, **Reading Machine**: nothing found. **ORATOR** (Bellcore) was Unix
  only. **Tinytalk** and **Vocal-Eyes** are screen readers, not synths. **SAM** had no native PC port
  (Wikipedia); the C reconstruction we use is the only one. **MBROLA** had a DOS binary (diphone,
  needs a phoneme front end, 486 class) **[verified listing in the 1997 comp.speech FAQ]**.

## 5. Table

| Program | Author / vendor | Years | Method | CPU | Licence / status | Source | Port? |
|---|---|---|---|---|---|---|---|
| TRAN 1.01 / 0.4 | Stephen T. Neely | 1988 / 1990 | NRL rules + fixed sampled phones (1-bit 16 kHz; 8-bit 10 kHz in 0.4) | XT | shareware, no licence text | none; binaries in `src/tran/` | No: rules we have, voice below SAM |
| SPEECH.COM (+SAY, READ, SAYTIME, TXT2SPCH) | Andy C. McGuire; Henderson, Sisco, Farjo, Maningo | 1983-88 | 35 fixed-length 1-bit PCM phonemes (575 B each), speaker driven per bit from port 61h, INT F1/F2 API | 4.77 MHz PC only (untimed loop) | copyright 1983, freeware; PC-SIG #1668 | none for SPEECH.COM; wrappers in asm; binaries and docs in `src/speechcom/` | No as a voice; data trivially reusable for a 1983 novelty voice |
| horndrv TALK.SYS | Jon Hornstein | 1992/95/99 | TRAN-style rules + SPEECH.COM 1-bit data | 286 | "public domain as FREEWARE" + GPL text | full C source (`horndrv2.zip`, Simtel) | Only as a novelty 1983 voice |
| freebasic-dos-speech | angros47 | 2013-17 | same data, SB output | any | open source (unverified) | SourceForge (download failed) | No |
| PC-TALKER family | Király József | 1990-91 | clipped-voice concatenation, PWM/SB | XT | binaries; 2026 Python rewrite | Python only, Hungarian | No |
| klattsch DOS | Tony Gies | 2026 | fixed-point parallel formant, phoneme input | 386, no FPU | engine MIT; DOS build binary | JS engine only | No (no text front end); calibration value |
| SmoothTalker 3.5 / TTS Engine 4.1 / Monologue / SBTALKER | First Byte (now Activision/Microsoft; patents at Sierra) | 1984-92 | compressed stored phoneme/transition waveforms, period stretching for pitch (patented) | 8 MHz | abandonware, not redistributable | none; client API in `dosbtalk` (Apache-2.0) | No: usable as benchmark voice only |
| DECtalk PC / DECtalk Software | DEC / Fonix | 1993 / 1996+ | hardware card / Klatt-class TTS in C | card / Pentium | driver only / source of unclear standing on GitHub | Win32 source (dectalk/dectalk) | No on a 386 |
| Prose 2000, CallText, Votrax, Artic, Echo | various | 1982-90 | hardware | - | - | none | No |

## 6. Recommendation

Nothing here beats what we have, and nothing here is portable in the sense the user hoped: TRAN and
SmoothTalker were never released as source, and the one full-source program (horndrv) is a 1983
one-bit voice. Keep the current path: SAM as the working engine, the fixed-point Klatt with the eSpeak
NG front end as the "modern" voice.

Three small things are worth taking from this survey:

1. **A design, not a port.** First Byte's patented scheme (stored phoneme-transition waveforms, 4-bit
   delta packing, pitch by truncating voiced periods) is exactly the low-CPU way to get a
   natural-sounding voice on a 386, and the patents are expired. If the Klatt voice stays in SAM's
   intelligibility class, a small diphone/transition concatenation engine driven by the eSpeak NG
   front end (which already produces phonemes, durations and pitch) is the cheaper route to a "more
   modern than Dr. Sbaitso" voice than more resonators. MBROLA-style data would have to be recorded or
   generated ourselves.
2. **Calibration.** klattsch's DOS README independently puts a fixed-point parallel-formant synth at
   "smooth to 8 kHz on a 386DX-33, 11 kHz borderline", which matches our measurements and supports the
   8 kHz decision.
3. **Benchmark voice.** SBTALKER through dosbtalk's API, run privately under DOSBox, gives a WER
   reference for "the voice we are trying to beat". Do not put its binaries on the distributed disk.

Optional novelty: the 36 one-bit phonemes in horndrv's `PHONEME.C` would make a "1983 voice" engine of
a few KB for the talking disk's amusement value; zero priority.

## Addendum (2026-09-20): the Simtel HANDICAP "DECtalk" and "Artic" programs

Checked `research/simtel-handicap/DCTKPRGM.ZIP` and `ARTCPRGM.ZIP` (extracted from the September 1992
Simtel CD): both are the 1986 **Enable Reader** screen reader (`ENABLE_D.EXE` / `ARTIC.EXE`, 35 KB
binaries) with configuration tables for a serial DECtalk box (`9600COM1.DEC` etc.) and for the Artic
SynPhonix card. They contain no synthesis at all; they drive hardware synthesizers. `ETSMANUL.ZIP` is
the Enable Reader 4.0 manual, `ACCENT.ZIP` an Accent card demo. Nothing here is usable as a software
voice; the DECtalk software that exists (the 2022 GitHub release) is Win32 and legally unclear (see
section 4).
