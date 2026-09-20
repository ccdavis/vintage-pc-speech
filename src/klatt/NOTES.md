# Klatt formant synthesizer for the FreeDOS speech project

Integer-only Klatt cascade/parallel synthesizer (`klatt_fx.c`) plus an
integer port of rsynth's Holmes phoneme-to-parameter frontend and NRL
letter-to-sound rules, so that `./klattsay "text" out.wav` produces speech on
the host with code that is meant to be recompiled by Open Watcom for a 386
real-mode TSR.

```
make            # builds ./klattsay and ./parity
make check      # float parwave vs klatt_fx on the committed tracks (+ overflow audit)
./klattsay -l 19 -8 -S 84 "Hello. This is the Klatt synthesizer speaking on Free DOS." out.wav
                # -l 19 = KFX_FLAGS_DOS (1x source, white noise, pre-emphasised output),
                # -S 84 = the DOS default rate: what SBTALK3 should produce
./klattsay -l 1 ...   # the pre-2026-09 DOS voice (exact parwave noise and output stage)
```
(`klattsay` prints its option list on a bad option; see the header of
`host/klattsay.c`.)

## Sources, licences

| What | Where | Version | Licence |
|---|---|---|---|
| klatt 3.04 `parwave.c` (Klatt 1980 Fortran; C by Jon Iles and Nick Ing-Simmons 1993/94; cleaned up by Reece H. Dunn) | `upstream/klatt-3.04/` = https://github.com/rhdunn/klatt | commit `fc01d4978b3f1f7b80a328c83cc18ee82075ede9` (2015-12-27) | GPL v3 or later (file headers and `COPYING`) |
| espeak-ng's copy of the same code (`src/libespeak-ng/klatt.c`, reworked for espeak's frame format) | `upstream/espeak-ng/klatt.c,.h` = raw files from https://github.com/espeak-ng/espeak-ng master | last touched 2024-12-15, commit `938fea0a` | GPL v3 or later |
| rsynth 2.x (Nick Ing-Simmons) | `upstream/rsynth/` = https://github.com/rhdunn/rsynth ("fork of the rsynth project to clean it up"; the original SourceForge/CPAN tarballs are no longer on GitHub) | commit `6111065823fbf2e51ccb9c8642765cfe0b5c4a84` (2014-01-12) | see below |

rsynth's licence (checked file by file): `README` says "The files in this
distribution are either under GPL or LGPL"; `COPYING` is GPL v2, `COPYING.LIB`
is LGPL v2.  Every file this port uses carries the **GNU Library General
Public License v2 or later** header: `holmes.c`, `elements.c`, `Elements.def`,
`phtoelm.c`, `phtoelm.def`, `english.c`, `text.c`, `say.c`, `trie.c`,
`rsynth.h`, `opsynth.c`.  Only `guess.c` and `saymain.c` (test/main programs,
not used) are GPL v2+.  The NRL rules in `english.c` derive from NRL Report
7948 (1976, US Naval Research Laboratory, a public-domain US government
work); Ing-Simmons' changes are LGPL.  `phones.def`/`phtoelm.def` have no
header of their own but are part of the LGPL library.

Resulting licences here: `klatt_fx.c/h` is GPL v3+ (derived from parwave.c);
`holmes.c/h`, `nrl.c/h`, `nrl_rules.inc`, `elements_tab.c`, `phtoelm_tab.c`
are LGPL v2+; the combined program is GPL v3.  The espeak-ng copy was only
used for comparison (it uses a sampled glottal source and espeak's own
frame layout; nothing was taken from it).

## Files

| File | Role |
|---|---|
| `klatt_fx.h`, `klatt_fx.c` | the engine: fixed-point parwave, frame API, `kfx_render16/8` |
| `klatt_tab.h` | generated tables: exp(-pi x), cos(2 pi x) (514 x int16 each), Klatt dB table (88), B0 pulse table (224): 2680 bytes |
| `holmes.h`, `holmes.c` | phones -> elements -> per-frame parameters -> `kfx_frame_t` (rsynth `phtoelm.c` + `holmes.c`, integer) |
| `elements_tab.c` | 84 Holmes elements x 18 parameters, generated from `Elements.def` |
| `phtoelm_tab.c` | 78 SAMPA phone -> element mappings, generated from `phtoelm.def` |
| `nrl.h`, `nrl.c`, `nrl_rules.inc` | text -> SAMPA: NRL rules (439 rules, verbatim from `english.c`), numbers, spelling, clause splitting |
| `host/klattsay.c` | host frontend, writes WAV, dumps parameter tracks |
| `host/parity.c` | runs upstream float `parwave.c` and `klatt_fx` on the same track |
| `host/wavout.c` | WAV writer |
| `tools/gen_tables.py`, `gen_elements.py`, `gen_phtoelm.py`, `gen_track.py` | generators (run with `uv run`) |
| `host/rsynth/config.h`, `play.c` | build upstream rsynth's `say` without autotools (`make rsynth_say`) |
| `tools/wer_eval.py`, `transcribe_all.sh`, `spectro.py`, `cg_engine.sh`, `cg_front.sh` | Whisper WER evaluation, spectrograms, callgrind accounting |
| `tools/rsynth_wav.sh`, `tools/postfx.py` | rsynth reference WAVs; host-side EQ/gain experiments |
| `results/` | parameter tracks (`*.par`, klatt 3.04 40-column format), WAVs, `transcripts.txt`, `sentences*.txt` |

Engine + frontend are ~2000 lines of C99 with no float, no malloc, no stdio,
no recursion (the number-to-words routine was made iterative), all 32-bit
quantities spelled `int32_t`/`uint32_t`, tables `const`.  `klatt_fx.c` also
compiles as C89 (`inline` is hidden behind `KFX_INLINE`).

## What was ported (klatt_fx.c)

From `parwave.c`, kept and made integer:

* NATURAL glottal source (`natural_source`: a n - b n(n+1)/2 pulse with the
  B0 table) run at 4x the sample rate through the `rlp` downsampling
  resonator, pitch-synchronous reset (`T0 = 40 sr / F0hz10`, `nopen = 4 Kopen`
  clamped to 40..T0-2), period skew (`Kskew`), spectral tilt (`TLTdb`),
  breathiness (`Aturb`), aspiration, noise generator with the 0.75 low-pass
  and half-amplitude modulation in the second half of the period.
* Cascade branch: nasal antiresonator + nasal pole + 1..6 formants.
* Parallel branch: F1, nasal pole, F2..F6 with alternating signs, bypass,
  excited by frication plus the first difference of the parallel voicing.
* Output resonator (`rout`), `Gain0`, clipping, `AVdb - 7`, `Gain0 - 3`.
* The `DBtoLIN` table and every fixed factor (0.4, 0.6, 0.15, 0.06, 0.04,
  0.022, 0.03, 0.05, 0.25, 0.1).

Cut: IMPULSIVE and SAMPLED sources, F0 flutter (needs sin at frame rate),
the ALL_PARALLEL switch, cascade formants 7/8 (only valid at >= 16 kHz),
stderr warnings.  Everything else behaves as parwave with `-c -n N -v 2`.

Number formats:

| Quantity | Format |
|---|---|
| resonator coefficients a, b, c | Q12 int32 (antiresonator Q10) |
| audio signals, delay lines | integer, parwave's 16-bit output scale divided by 4 (`KFX_HEADROOM`) |
| linear amplitudes (`amp_voice` ...) | Q12 |
| glottal pulse a, b, vwave | Q12, with the 0.028 output factor folded in |
| noise low-pass state | Q4 |
| tilt filter | Q14 |
| Hz -> fraction of sr | `(hz * (2^32 / sr)) >> 16`, then two 514-entry Q15 tables with linear interpolation (exp, cos); no division in `setabc` |

The resonator sum `a x + b p1 + c p2` is evaluated with 32 x 32 -> 32
multiplies modulo 2^32 (`MUL32` uses unsigned arithmetic so it is defined
behaviour).  That is exact whenever the true result fits, i.e. |y| < 2^19.
parwave's internal levels are much higher than its output: the cascade
sections near Nyquist (F3/F4 at 8 kHz have a = 2.5..3.4) boost the glottal
closure step, and the nasal antiresonator has coefficients up to +-42.  The
64-bit audit (`make check`, `-DKFX_CHECK`) found peaks of 1.2 M on parwave's
scale on the synthetic track, so the internal scale was divided by 4 (2 bits
taken from the source amplitudes, given back in the output gain).  After
that: 0 overflows, peak |y| = 186773 of 524287 on all four tracks.  The
documented safe ranges are AV, AF, ASP <= 70 dB, Gain0 <= 62.

### Parity (float parwave.c vs klatt_fx, same noise sequence, `make check`)

The harness feeds parwave's `rand()` from the engine's LCG so both see the
same noise; the remaining difference is rounding (Q12 coefficients, Q12
pulse, integer delay lines).

| Track | sr | cascade | max diff (of 32767) | rms diff | rms ref | SNR |
|---|---|---|---|---|---|---|
| `results/hello8k.par` (the sentence, 4.2 s) | 8000 | 3 | 157 | 25.8 | 1977 | 37.7 dB |
| `results/hello11k.par` | 11025 | 4 | 375 | 59.9 | 2011 | 30.5 dB |
| `results/synth8k.par` (vowels, nasal, fricatives, burst, tilt, breath, skew) | 8000 | 4 | 202 | 37.2 | 3678 | 39.9 dB |
| `results/synth11k.par` | 11025 | 5 | 557 | 69.9 | 3866 | 34.9 dB |

WAVs: `results/parity_*_ref.wav` (float) and `results/parity_*_fx.wav`
(fixed).  The residual is 30-40 dB below the signal, i.e. below the 8-bit
output's own quantization; the SRC1X option (below) is not a parity
configuration (SNR 11 dB against parwave, by design).

## Frontend

* `nrl.c`: rsynth's NRL rules and matcher verbatim (Latin-1 rules dropped),
  cardinal numbers (1100..1999 as "eleven hundred"), digit spelling after a
  decimal point, `[sampa]` pass-through, spelling of odd tokens and
  punctuation names, clause splitting at `. ! ? ; , ( )`.  All-caps words of
  3+ letters with a vowel are said as words ("DOS"), shorter ones spelled.
* Stress: rsynth gets stress from a dictionary we do not have; without it
  every vowel takes its short "unstressed" duration and the digits test went
  from 100 % to ~10 % words correct.  `nrl.c` therefore marks the first
  vowel of every word with primary stress and later vowels with tertiary
  (`nrl_stress_mode`, `-U` disables).
* `holmes.c`: `phone_to_elm` (durations `ud + (du-ud)*stress/3`, F0 contour
  starting at 1.1 F0, declining 0.12 Hz per frame, +2 % per stress level,
  floor 0.7 F0) and `rsynth_interpolate` (rank-dominated transitions,
  internal/external durations, the 0.5 smoothing filter) in Q8 with integer
  divisions only at frame rate.  Pull model: `klt_holmes_next()` gives one
  10 ms `kfx_frame_t`.
* Element -> Klatt mapping (`klt_map_frame`): F1-F3/B1-B3/fn/av/asp/af/
  a2-a6/ab straight across; F4-F6 fixed per sample rate (8 kHz: 3300/3800/
  3950 Hz, cascade uses only F1-F3; 11025: 3500/4500/5200, cascade F1-F4);
  nasal pole fixed at 270 Hz; `Kopen` = 40 % of the period; `AVpdb`, `A1`,
  `ANP`, `Aturb`, `Kskew` = 0 (`-a` maps rsynth's `avc` onto A1/AVpdb, it
  did not help; `-c dB` maps it onto AVdb, within noise).  rsynth's own
  synth applies its amplitude parameters without parwave's fixed factors
  and with a 6 dB stronger, white noise source, so the mapping adds
  +23/+30/+34/+39/+37 dB to a2..a6, +32 to ab and asp, +12 to af and
  expects `KFX_LITE_NOISE_WHITE` (see "Intelligibility" for the
  derivation; klattsay subtracts 6 dB again when the flag is off).
* Tuning by Whisper WER (below): nasal bandwidth 100 Hz (Klatt's value)
  instead of rsynth's 500 helped most; slower speech (`-S 85`) helps a bit;
  cascade F4 at 8 kHz, tilt, open quotient 50 %, no smoothing: no clear gain.

## Cost (callgrind, x86-64, gcc -O2, "Hello. This is the Klatt synthesizer speaking on Free DOS.", 4.23 s)

Instructions per second of audio; budget 2.5 M (= ~55 % of a 386 DX-25 by
the SAM calibration, 4.5 M = 100 %).

| Configuration | engine | frontend | total | vs budget |
|---|---|---|---|---|
| 8000 Hz, cascade F1-F3, exact 4x source | 2 749 k | 124 k | 2 873 k | +15 % |
| **8000 Hz, cascade F1-F3, SRC1X (`-l 1`)** | **2 260 k** | **124 k** | **2 384 k** | **-5 %** |
| 8000 Hz, SRC1X, 8-bit output | 2 276 k | 124 k | 2 400 k | -4 % |
| 8000 Hz, SRC1X, cascade F1-F4 | 2 370 k | 124 k | 2 494 k | 0 % |
| 11025 Hz, cascade F1-F4, exact | 3 893 k | 117 k | 4 010 k | +60 % |
| 11025 Hz, cascade F1-F4, SRC1X | 3 216 k | 117 k | 3 333 k | +33 % |
| 11025 Hz, cascade F1-F3, SRC1X | 3 067 k | 117 k | 3 184 k | +27 % |

Per sample at 8 kHz: 344 instructions exact, 283 with SRC1X, of which the
resonators are about half (~15 instructions each, 3 IMULs).  The frontend
(rules + interpolation + coefficient updates) is 4-5 % of the total.
`kfx_set_frame` itself (18 `setabc`, one `setzeroabc` with three divisions,
eleven `db_gain`) is ~1100 instructions per 10 ms frame, included in the
engine figure.

Option flags added later for intelligibility (see "Intelligibility"), all
at zero per-sample cost: `KFX_LITE_NOISE_WHITE` (no 0.75 noise low-pass),
`KFX_LITE_OUT_FIR` (output stage is the FIR `g (x - k x[-1])` evaluated in
the antiresonator form instead of parwave's 2-pole output low-pass),
`KFX_LITE_RLP_F4` (1x source: the rlp slot becomes the cascade F4 section;
not used), `KFX_LITE_SRC_DIFF` (first-differenced pulse, +2 %; not used).
`KFX_FLAGS_DOS` = SRC1X | NOISE_WHITE | OUT_FIR is what the DOS build
should pass to `kfx_init` (2.336 M instr/s vs 2.341 M for SRC1X alone).

**What was cut to meet the budget, and its effect:**

1. `KFX_LITE_SRC1X`: the glottal source is stepped once per sample instead
   of 4x oversampled; the pulse polynomial is advanced by 4 sub-samples at a
   time (exact values, boundaries handled with the slow loop), so F0 keeps
   its 1/4-sample period resolution.  Only the anti-aliasing resonator
   changes: parwave computes it for sr but runs it at 4 sr (a ~3 kHz
   low-pass), the 1x version uses the same pole (f = 0.38 sr, bw =
   0.252 sr).  Saves 3 resonator evaluations and the loop: -18 %.
   Spectrograms are near-identical (slightly more energy above 2.5 kHz);
   Whisper WER over the six test sentences 42.9 % -> 53.6 % at 8 kHz, which
   is inside the run-to-run noise of this test (see below), and the digits
   sentence stays at 0-10 % errors either way.
2. Cascade limited to F1-F3 at 8 kHz (F4 = 3300 Hz sits 700 Hz under
   Nyquist; parwave's F4/F5 sections there mostly add a transient boost).
   -4 %.  No measurable WER effect.
3. Idle parallel sections are skipped exactly (gain 0 and delay line 0 give
   output 0); the branch runs only during frication/aspiration.  This is
   bit-exact and always on, and is what makes the average cost depend on
   the text (about 42 IMULs per sample in fricatives, 26 in vowels).
4. Two multiplies removed from the sample path (noise mapping done with a
   compare; `par_amp_voice` multiply skipped when zero): exact.

11025 Hz does not fit: even with SRC1X and F1-F3 it is 27 % over.  It would
need dropping the nasal pole/zero pair (-2 resonators, ~-0.35 M) and the
parallel F5/F6 sections and would still be ~+10 %; so 8 kHz is the target.

A 386 cycle estimate, since the host-instruction calibration comes from
SAM whose code is mostly byte arithmetic: per 8 kHz sample with SRC1X, 26
IMUL r32 (12-38 cycles on a 386, ~25 with Q12 multipliers) + ~250 simple
instructions at ~2.5 cycles = ~1 300 cycles in vowels, ~1 700 in
fricatives -> 10-14 M cycles/s = 40-55 % of 25 MHz, consistent with the
instruction-count figure (2.38 M / 4.5 M = 53 %).

## Intelligibility (Whisper WER)

`tools/wer_eval.py [--stable] [--model small.en] sentences.txt CONFIG...`
synthesizes each sentence, converts to 16 kHz, transcribes with
faster-whisper and reports the word error rate.  `results/sentences24.txt`
(24 everyday sentences + two digit strings, 241 words) is the reference set;
`--stable` (temperature 0, no conditioning on previous text) removes most of
Whisper's run-to-run chaos, and a hallucination cascade is capped at one
fully wrong sentence.  Even so single-sentence results are noise; only
differences of ~10 points over the 24 sentences mean anything.

CONFIG can be a klattsay option string, `cmd:name:template` with `{text}`,
`{wav}`, `{textq}` (the NRL phone string in `[...]`, for rsynth), or
`wav:file`.  `tools/rsynth_wav.sh` runs upstream rsynth (`make rsynth_say`,
float, its own synth, no dictionary) as the reference.

### Second pass: what was wrong

A blind listener on the real 386 found the voice barely intelligible and a
little worse than SAM.  Measured on the host with the reference set, the
same order held (SAM 22 kHz ~30 %, this voice ~50-60 %), so the loss is in
the audio, not on the DOS side.  Findings:

1. **Upstream rsynth is not a good reference.**  Its own `say` (float,
   opsynth.c, its NRL rules, no dictionary) scores worse than this port on
   the same phone strings (table below).  The 2004 element table and rules
   are the ceiling of what this frontend can do; espeak-ng's Klatt voice
   (a far better frontend on a similar synth) scores ~16 % at 8 kHz, so
   8 kHz itself is not the limit.
2. **The frontend port is exact.**  Element sequences, durations, F0 and
   the 18 interpolated tracks agree frame by frame with rsynth's `-p`
   dump (`hello8k.par` vs rsynth's parameter file: same 350 frames, same
   values to the rounding).  Rate and F0 contour are rsynth's; the DOS
   default speed (`dt_speed` 5 = 84 %) is already the "15 % slower" that
   scored best.
3. **rsynth's synth is not parwave, and its amplitude table assumes its
   own source levels.**  opsynth.c uses white noise (sum of 16 uniforms,
   rms 9460) fed straight to the parallel resonators; parwave low-passes a
   uniform +-8191 source with a 0.75 pole (DC gain 4, -17 dB at 4 kHz)
   and scales AF/A2..A6/AB by 0.25/0.15/0.06/0.04/0.022/0.03/0.05.  The
   port's `+17..33 dB` offsets undid the factors but not the noise: its
   fricatives came out 6 dB too strong below 300 Hz and 5-11 dB too weak
   at 3-4 kHz.  A synthesized [s] was flat low-frequency hiss (band energy
   0-300 Hz -8 dB, 3.5-4 kHz -11 dB rel. total; rsynth: -28 / -1 dB).
   Fix: `KFX_LITE_NOISE_WHITE` (no low-pass, same cost) and offsets
   recomputed as -20 log10(factor) + 6 dB: a2..a6/ab 23/30/34/39/37/32,
   asp 32, af 12 with AF allowed up to 80 (overflow audit: peak 126 k of
   524 k).  With that [f] matches rsynth's spectrum within 2 dB per band
   and [s] is within 6 dB.
4. **Spectral balance of the voiced path.**  rsynth's cascade carries two
   fixed high sections (a 3500/1800 Hz "shaping" pole and F4 3900/400,
   both folded towards Nyquist at 8 kHz) that lift 3-4 kHz by 30-45 dB
   relative to DC; parwave with cascade F1-F3 has none, so vowels were
   10-35 dB darker above 2.5 kHz.  Putting F4 in the cascade (`-n 4`, or
   the free `KFX_LITE_RLP_F4` slot) did not help Whisper (a narrow
   resonance at 3.3 kHz made it worse); a plain first-order pre-emphasis
   did (host post-processing `x - 0.95 x[-1]`: 60 -> 49 %, and combined
   with the DOS rate 48 -> 32 %).  A high-pass alone or a shelf alone did
   nothing, so it is the tilt as a whole.  `KFX_LITE_OUT_FIR` puts that
   FIR into the output stage in place of parwave's 2-pole output low-pass
   (same antiresonator evaluation, same cost: 2.336 M vs 2.341 M
   instr/s), coefficient 0.95 and gain x4 (`kfx_t.out_k/out_gain`, Q10).
5. **Level in the 8-bit path.**  The old output peaked at -7 dBFS with a
   17-20 dB crest factor; after `>> 8` and the DOS volume LUT (80 % at the
   default 5) fricatives were 1-2 LSB rms.  The pre-emphasis gain of 4
   with parwave's hard limit at +-32767 acts as a crude limiter: 1.5 % of
   samples clip, the rms rises 12 dB and the 8-bit output uses the full
   +-127 (rms ~30 LSB).  Gain x2 (0.1 % clipped) scored the same within
   noise, so x4 is the default for the sake of the 8-bit path.
6. `KFX_LITE_SRC_DIFF` (first difference of the glottal pulse, voice-only
   pre-emphasis, +2 % cost) was tried too; it is weaker than OUT_FIR and
   is left in as an option.  Mapping rsynth's `avc` (voice bar) onto AVdb
   (`-c 8`) is within noise.  Nasal bandwidth 100 Hz, stress heuristic and
   the 1x source are unchanged (the 1x source is bit-identical to before).

### WER table

`results/sentences24.txt`, `--stable`, all this-port rows at 8 kHz, 8-bit,
`-S 84` (the DOS rate) unless noted.  "old" = the shipped voice (`-l 1`,
stress mode 1); "new" = `KFX_FLAGS_DOS` (`-l 19`, pre-emphasis 0.95 x4,
white noise, new offsets), stress mode 2 (every vowel primary) = the new
defaults.  base.en is what the reference transcribe.py uses; small.en is
closer to a human listener and much less erratic.

| Configuration | base.en | small.en |
|---|---|---|
| upstream rsynth `say`, float, its own synth, our phone string, 8 kHz | 74.3 % | 58.1 % |
| upstream rsynth, 11025 Hz (12 sentences) | 82.1 % | - |
| espeak-ng `en+klatt` downsampled to 8 kHz | 22.0 % | 19.1 % |
| SAM (22 kHz, native) | 33.2 % | 30.3 % |
| old, exact engine, 11025 Hz, F1-F5, 16-bit, `-S 100` | 59.8 % | - |
| old, exact engine, 8 kHz, F1-F3, 16-bit, `-S 100` | 58.1 % | - |
| old, `-l 1`, 16-bit, `-S 100` | 58.1 % | - |
| **old DOS voice** (`-l 1 -8 -S 84`) | **57.7 %** | **34.9 %** |
| old DOS voice, stress mode 2 | 57.7 % | - |
| new, LP noise + old offsets (`-l 17`), stress 1 | 50.2 % | - |
| new, white noise (`-l 19`), stress 1 | 54.4 % | 34.0 % |
| new, white noise, stress 1, 16-bit | 54.4 % | - |
| new, white noise, stress 1, gain x2 (`-o 973,2048`) | 54.4 % | - |
| new, white noise, stress 1, `-S 75` | 47.7 % | - |
| new, white noise, stress 1, `-c 8` (voice bar on AVdb) | 54.4 % | 32.0 % |
| new, LP noise + old offsets, stress 2 | 44.0 % | 35.3 % |
| new, LP noise + old offsets, stress 2, `-c 8` | 48.1 % | - |
| new, LP noise + old offsets, stress 3 | 47.7 % | - |
| new, LP noise + old offsets, stress 2, nasal bw 500 (`-B 500`) | 68.0 % | - |
| **new DOS voice** (`-l 19 -8 -S 84`, stress 2) | **51.0 %** | **32.8 %** |
| new DOS voice, `-c 8` | 51.5 % | 33.2 % |
| new DOS voice, `-S 75` | 51.9 % | - |

What the table says: the engine/frontend cuts (11 kHz vs 8 kHz, exact vs
1x source, 16 vs 8 bit) cost nothing measurable, and neither does the
sample rate; the whole voice sits 25 points behind SAM with base.en, and
with small.en the old voice is already close to SAM.  The audio fixes
(pre-emphasis + level, noise spectrum, offsets) gain ~7 points with
base.en, the stress change ~5 more, nothing beyond noise with small.en.
The nasal bandwidth (100 vs rsynth's 500) remains the single biggest
knob (+17 points).  The white/low-passed noise choice is inside the noise
of both models (base.en prefers low-passed by 3-6, small.en white by 2-3);
white is kept because the fricative spectra then match rsynth's, which
is what the element table was tuned for.  The remaining gap to espeak-ng
(~30 points, 12 with small.en) is the 2004 rsynth element table and NRL
rules themselves: upstream rsynth on the same phones is 20-25 points
*worse* than this port with either model.

Shipped WAVs: `results/hello8k_dos.wav`, `digits8k_dos.wav` (new DOS
voice; base.en hears "Hello, this is that in the later speed film on 3.0"
and the digits 1-10 correct).

### Old numbers (six sentences, `results/sentences.txt`, transcribe.py
defaults, kept for reference)

| Configuration | WER |
|---|---|
| 8 kHz defaults (exact source) | 42.9 % |
| 8 kHz `-l 1` (SRC1X, the DOS configuration) | 53.6 % |
| 8 kHz `-l 1 -S 85` (15 % slower) | 46.4 % |
| 8 kHz `-l 1 -8` (8-bit output) | 58.9 % |
| 11025 Hz | 57.1 % |
| 8 kHz, before the nasal-bandwidth change (`-B 500`) | 67.9 - 83.9 % |
| 8 kHz without the stress heuristic (`-U`) | ~80 % (digits 90 % wrong) |

Known frontend weaknesses: NRL mispronounces "nineteenth" (n'In+it+inT),
"Saturday" (s'eIt+3d+eI), the stress heuristic is crude, F0 is a plain
declination, word gaps are a fixed 60 ms, and the rsynth element table was
tuned for rsynth's own synth rather than Klatt's.

## DOS / Open Watcom integration notes

Memory (const data unless noted):

| Item | Size |
|---|---|
| `klatt_tab.h` tables | 2 680 bytes (far data on 16-bit Watcom, `KFX_TAB_FAR`) |
| `klt_elements` | 84 x 120 bytes = 10 080 (host; 84 x 114 = 9 576 on DOS, far data, `KLT_FAR`) |
| `klt_phtab` | 78 x 16 = 1 248 (+ strings) |
| NRL rules | 439 x 4 pointers (3.5 KB near, 7 KB far) + ~4 KB of strings |
| `kfx_t` (RAM) | 524 bytes |
| `klt_seq_t` (RAM, one chunk) | 1 032 bytes (256 elements, 256 int16 F0 entries; a long clause is rendered in several chunks, `seq->used`) |
| `klt_holmes_t` (RAM) | 480 bytes |
| phone buffer (RAM) | 1 KB in the host tool; 256 bytes is enough per clause |
| code | ~20 KB on x86-64; expect 10-15 KB of 16-bit code |

Points that need 32-bit arithmetic (all spelled `int32_t`/`uint32_t`; with
`-3` Watcom emits IMUL/SHRD on 32-bit longs in real mode):

* `resonator()`/`antiresonator()`: three 32 x 32 -> 32 multiplies, the sum
  wraps modulo 2^32 on purpose (`MUL32`, `SUM3` are unsigned); the
  `>> 12` needs an arithmetic shift of a signed 32-bit value (Watcom `SAR`,
  as gcc).
* `db_gain`: 16-bit table x 15-bit factor.
* `setabc`: `hz * inv_sr` is an unsigned 32-bit product that may reach
  2^32 - 1 for f near sr (unsigned wrap is intended); `rr * rr`, `rr * cc`
  are 30-bit.
* `setzeroabc`: three 32-bit divisions per frame; `pitch_synch_par_reset`:
  one division (`40 * sr / F0hz10`) per pitch period and a `/ 1000`.
* `kfx_init`: `0xFFFFFFFF / sr` once.
* `holmes.c`: `linear()` and `interpolate()` do `(b - a) * t / d` in Q8
  (values up to ~3e8) - 18 x 2-3 divisions per frame; `set_trans` one
  division per parameter per element.  All at frame rate (~1 % of the
  engine).
* The noise LCG: `seed * 1664525 + 1013904223` in `uint32_t`.

16-bit care:

* `int` is used only for small loop counters and booleans; every value that
  can exceed 16 bits is `int32_t`.  gcc `-Wconversion` only complains about
  the `L` constants being 64-bit on the host.
* No stack array is larger than 67 bytes (`nrl_word`); `klt_seq_t`,
  `klt_holmes_t`, the phone buffer are meant to be static/TSR-resident.
* `kfx_frame_t` is 40 x int16 = 80 bytes; the host tools cast it to an
  `int16_t[40]` for the `.par` dump, which relies on no padding (true for
  Watcom with default packing).
* `KFX_INLINE` expands to nothing outside C99; the hot loop is one function
  call per sample otherwise (~10 instructions).
* `nrl_stress_mode` is the only global; everything else is in caller-owned
  structs, so the engine is re-entrant apart from that flag.
* Timing: 10 ms frames = 80 samples at 8 kHz.  The natural DOS loop is:
  `klt_holmes_next()` -> `kfx_set_frame()` -> `kfx_render8(buf, 80)` into
  the DMA double buffer, one frame per SB interrupt; `kfx_set_frame` costs
  ~1100 instructions, the render ~23 000, so the per-frame work is even.
  `kfx_render8` writes unsigned 8-bit centred on 128.
* Text handling for a TSR: `nrl_translate(&text, buf, n)` consumes one
  clause at a time; `klt_phones_to_seq` caps at 400 elements / 200 F0
  points (long clauses are truncated at that point, then the next call
  continues from the text cursor).
* Watcom flags suggested: `-3` (32-bit IMUL/SAR in real mode), `-ms` or
  `-mc`, `-ot`; keep the default structure packing so `kfx_frame_t` stays
  80 bytes.  The code has no `long long`, no float, and needs nothing from
  `<stdint.h>` beyond `int8_t..uint32_t` (Open Watcom 1.9+ ships it).
* The parity and WER scripts are host-only; on DOS the only external
  dependency of the engine and frontend is `<stdint.h>`.
