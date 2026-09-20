# DECtalk source landscape (for the DOS/DJGPP port)

Researched 2026-09-20. Tags: `[verified: URL]` = fetched and read this session; `[local]` = read from our clone at
`freedos/src/dectalk-upstream/dectalk`; `[recall]` = from memory, not re-checked; `[unverified]` = search-engine
snippet or secondary summary only.

## 0. TL;DR

- Every known DECtalk source drop is in one directory on datajake's server. It moved: the URL in the upstream README
  (`/tts/DECtalk%20source%20code%20archive/`) now returns 404; the live path is
  `https://datajake.braillescreen.net/TTS/DECtalk/SourceCodeArchive/` [verified: that URL, 2026-09-20].
- `dectalk/dectalk` `develop` is what the community calls "DECtalk 4.99": the 2003-04 "advanced development" tree
  (`Ad 2.zip`, an early 5.0 beta) built with the classic Klatt vocal-tract (`vtm/`), HLSyn compiled out, plus 2022-2026
  portability fixes and, since July 2026, the PR #89/#90 "restore classic sound" patches. It is the right base.
- 4.63 (`dectalk/463`) has `#define HLSYN` on by default and is generally described as not sounding classic; 5.x
  (`ad-new.zip`) is a FonixTalk-lineage beta. Neither is a better base for a screen reader that should sound like
  a DECtalk PC/Express.
- No DOS/DJGPP port of any DECtalk source exists anywhere I could find. The closest things are the 1995 DECtalk-PC
  card firmware (16-bit Microsoft C, `/G1` = 80186 code) in `src/hardware/`, and DECtalkMini's GBA/Pico builds.

## 1. The datajake archive

Directory listing of `https://datajake.braillescreen.net/TTS/DECtalk/SourceCodeArchive/` [verified: that URL]:

| File | Date | Size | What it is (from `info.html`, same dir [verified]) |
|---|---|---|---|
| `Ad 2.zip` | 13-Nov-2015 | 88M | DECtalk 5.0 *beta*, "advanced development (ad) branch"; synth code late 2003, rest early 2004. The Bruckert 2015 release. Basis of `dectalk/dectalk` `master`. |
| `460r008.zip` | 15-Sep-2022 | 15M | DECtalk 4.60 Revision 8, the build shipped with GW Micro Window-Eyes, patched for GW Micro 13-Jun-2003. |
| `product.460.zip` | 15-Sep-2022 | 20M | DECtalk 4.60 Revision 11, "final revision of 4.60"; comments hint at HLsyn but it is not used or included. Used in Blaxxun Contact VRML plugin. |
| `462.zip` | 15-Sep-2022 | 34M | DECtalk 4.62 = 4.61 plus updates, Window-Eyes candidate; also contains "an old version of the vocal tract model from the mid-90s". |
| `463.zip` | 15-Sep-2022 | 555M | DECtalk 4.63, includes prebuilt demo and SAPI5 installation media. Mirrored as the `dectalk/463` repo. |
| `ad-new.zip` | 15-Sep-2022 | 1G | DECtalk 5.1 **and 4.64**. 5.1 "appears to be a beta of what ultimately became FonixTalk"; includes internal docs/papers for Chinese, Japanese etc. Basis of `RetroBunn/dt51`. |
| `hebrew_mac_2016_09_04_latest.zip` | 15-Sep-2022 | 300M | MacOS/iOS port of DECtalk 5.0 used in TropeTrainer (Hebrew cantillation product). |
| `microdectalk.zip` | 15-Sep-2022 | 715K | DECtalk **Express firmware** modified to run on Epson S1C33 class devices; no project files; code "most likely based on DECtalk 4.2". |
| `parser.zip` | 15-Sep-2022 | 13M | Text preprocessor circa 1997. |
| `tools.zip` | 15-Sep-2022 | 83M | LTS/phoneme compilers and data; also "a Unix port of an early revision of DECtalk 4.51". |
| `info.html` | 30-Jul-2024 | 3156 B | The descriptions above. |

Notes:
- There is **no 4.40 or 4.61 source zip**. 4.61 only exists as the base of 4.62/4.63 and as `VERSION.H`
  ("V4 6 1 - August 18, 2000") which is stale in every later tree, including `develop` and `463` [local; verified:
  https://raw.githubusercontent.com/dectalk/463/main/dapi/src/INCLUDE/VERSION.H].
- Who released what: Edward Bruckert (the original DEC/Fonix developer, died 2020) posted `Ad 2.zip` to the DECtalk
  list Oct/Nov 2015; "another developer" (a former Fonix developer) supplied the 15-Sep-2022 batch [local README;
  verified: https://raw.githubusercontent.com/RetroBunn/dt51/HEAD/README.md says "shared by a former Fonix Corp.
  developer"]. The Bluegrasspals Mailman archive that hosted those threads was decommissioned 31-Mar-2026 (list
  moved to groups.io) and the Wayback Machine has no copies of the 2016/2019/2022 threads [verified: fetch attempts].

### HLSyn vs classic VTM, per tree

`HLSYN` is a compile-time switch in `dectalkf.h`; when on, `hlsyn/` (Sensimetrics HLsyn, quasi-articulatory
model by Ken Stevens' group) replaces the Klatt formant vocal tract in `vtm/` [local: `src/dectalkf_klsyn.h`
lines 112-121; `src/dapi/src/hlsyn/`, `src/dapi/src/vtm/`].

| Tree | `HLSYN` default | Source |
|---|---|---|
| 4.60 R8 / R11 (`460r008`, `460` branches) | no HLSYN define at all | [verified: raw dectalkf.h of both branches] |
| 4.62 (`462` branch) | `//#define HLSYN` (off) | [verified: raw dectalkf.h] |
| 4.63 (`dectalk/463`) | `#define HLSYN` (on); `//#define NON_HLSYN` | [verified: https://raw.githubusercontent.com/dectalk/463/main/dectalkf.h lines 113-118] |
| "4.99" (`dectalk/dectalk` develop) | `dectalkf.h` includes `dectalkf_klsyn.h` (HLSYN off); `dectalkf_hlsyn.h` kept as alternative | [local: `src/dectalkf.h`] |
| 5.1 (`ad-new.zip`, `dt51`) | HLSyn ("High-Compute") and a "Low-Compute" build both exist | [verified: dt51 README mentions High-Compute/Low-Compute; dectalk.nu changelog "LowCompute/HighCompute builds from October 2006"] |

Bruckert himself on HLSyn: "While at Fonix I decided to incorporate Prof. Stevens ... it was a more detailed model of
the vocal tract" (DECtalk list, 16-Nov-2015) [verified: Wayback copy of
https://bluegrasspals.com/pipermail/dectalk/2015-November/004545.html]. A list member on the same thread called the
5.0 speak window "such a strange voice ... It's got the beginnings of FonixTalk in it. What a shame."

### Which one sounds like the DECtalk blind users remember

- dectalk.nu history: sound "remained mostly the same throughout the nineties" through 4.51 (Compaq 1998) and 4.60
  (SMART Modular 1999, the Window-Eyes version). "In 2000, DECtalk was sold to Force Computers ... Their first and only
  release, version 4.61, had a much thinner sound than previous versions, and altered the pitch and characteristics of
  most of the voices." Fonix's 5.0 "was available in many different sounding editions"; FonixTalk 6.x continued the
  5.0 sound [verified: https://dectalk.nu/].
- "Version 4.63 uses HLSyn, so it doesn't sound like classic DECtalk at all" [unverified: search snippet of the
  now-offline 2022-09 list thread "It's time to bust some DECtalk myths"].
- The PR #89/#90 author verified changes "directly against DECtalk 4.3 and 4.4 [Software] and 4.2CD for DECtalk PC and
  Express hardware", i.e. the community's reference for "classic" is 4.2 (hardware) / 4.3-4.4 (software) Paul
  [verified: https://api.github.com/repos/dectalk/dectalk/pulls/89 and /90].
- Issue #81 (Nov 2025) asks to revert parsing/emphasis to "4.51 or 4.60" behaviour and to use voice ROMs from
  "3.0, 4.0, 4.1, or 4.61, 4.62, or 4.64", with the manual's `[:dv ...]` voice definitions
  [verified: https://api.github.com/repos/dectalk/dectalk/issues/81].
- Net: the classic reference is the 4.2-4.4 Klatt VTM sound; 4.60 is the last "old sound" software; 4.61+ was
  retuned; 4.63 HLSyn and 5.x are the "smooth/FonixTalk" direction. The develop branch after #89/#90 is the only
  source tree that has been deliberately tuned back toward 4.2/4.4.

## 2. GitHub org `dectalk`

Repos [verified: https://api.github.com/orgs/dectalk/repos]:

| Repo | Pushed | Size | Content |
|---|---|---|---|
| `dectalk` | 2026-07-21 | 174 MB | The main tree ("4.99"). Branches: `master` (raw `Ad 2.zip` dump, first commit 2020-08-28, `src/` move 2021-08-22, tip `0530ba77`), `develop` (tip `69ebb459`), plus **verbatim archive dumps**: `460` (`915a9869`), `460r008` (`5b2f8b15`), `462` (`2b5bf029`), `microdectalk` (`06e35437`), `parser` (`b2a5d028`), `tools` (`d6fc78df`), all "Initial commit" 2022-09-16/17 [verified: branches API]. Also `cmake`, `cmake-openal`, `emscripten`, `macos-compile`, `test-for-gnome`. |
| `463` | 2023-12-22 | 30 MB | DECtalk 4.63 source, single "Initial commit." `e6bfa51e`, no README/license [verified]. 3179 blobs; has `dapi/src/hlsyn` and `dapi/src/VTM`, Windows CE installers, SAPI5 installer project. |
| `DECtalkMini` | 2026-08-10 | 42 MB | "portable DECtalk": premake build; platforms `android, gba, java, minecraft, node, pico, uefi, wasm`; nix flake; commits 2026-07-14/22 "port patches from dectalk/dectalk to make it sound like 4.3/4.4"; 2026-08-08 "add floating point vtm support (default disabled), and fix dictionary on big endian" [verified: repo API + README]. |
| `lintalker` | 2026-04-22 | 2.5 MB | "a speech synthesizer", README is one line; unclear relation. |
| `DTPad` | 2024-04 | | MFC editor using DECtalk. |
| `archlinux-dectalk`, `tw-dectalk` (TurboWarp ext), `SharpVoxTTS`, `paul` (bot), `homepage`, `dectalk-de`, `list`, `.github` | | | not engine sources. |

Tags/releases on `dectalk/dectalk` [verified: tags + releases API]: `2022-09-15` (`5e937e1c`, linux-amd64 + win32-ia32),
`2023-02-24-cmake-rc1/rc2` (`95cd27bf`/`8c8f0931`, adds emscripten, vs6-ia32, windows-amd64 assets), `2023-10-30`
(`cde003e8`, "Allow manually running build"; assets macOS, ubuntu, vs2022, vs6, win32-ia32), `v5-beta` (`699af9f8`,
2021-04-18 "Copy LICENCE back outside" - an early re-layout of the Ad 2 tree, not 5.x code). No tag after 2023-10-30;
current builds come from GitHub Actions on `develop`.

Recent merged PRs (2025-2026) [verified: pulls API]:
- #70 (2025-06-10) buffer overflow crash in `f_fr_ru1.c`; #73 NVDA add-on for 2025.1; #75 version-speak text; #76 pages workflow.
- #83 (2026-03-16, C0atRack) Linux audio stall when text is queued right after previous text ends (`nt/linux_audio.c`, +2 lines).
- #87 (2026-07-16) fix Docker build.
- #89 (2026-07-14, akse0435, `19c02c34` -> merge `223aeaa9`, 6 files): sentence-final pitch fall (`ph_inton1.c`, `ph_defs.h`),
  phoneme transition behaviour in `ph_setar.c` "verified against 4.6 source", `ph_sort2.c` fixes (last sung note 4 dB
  too loud; pitch rise before comma/colon; exclamation intonation), and `vtm/vtm1.c`: fix pitch-period scaling from
  the hardware's 10 kHz to 11.025 kHz that made high sung notes out of tune. Author states changes were made "with
  heavy help from AI" but built/tested on VS6.
- #90 (2026-07-21, akse0435, `a819192c` -> merge `69ebb459` = our HEAD, 14 files): reverts the #89 `ph_inton1.c` change,
  phrase-final lengthening like 4.4/4.2CD (`p_us_tim0.c`, `ph_drwt01.c`), removes the 4.4-era intonation reset between
  clauses (`ph_inton0.c`), voice-ROM tweaks in `p_us_rom_dectalk_1996m_43f.c` ("rr" clearer, "now" longer before comma),
  `Dic_us.txt` +7, `vtm/vtm_fa.c`, `dectalkf_klsyn.h`.
- Open, unmerged: #85/#86 (lllucius) "preservation-first modernization scaffolding"/planning docs; #35 CMake.

Recommended base for classic 4.x sound: `develop` at `69ebb459` (2026-07-21). If the #89/#90 retuning is ever
judged wrong, the pre-#89 develop is `f2ef8f6a` (base of #89) [verified: PR 89 base sha]. There is no tagged
"classic" release.

## 3. Prior ports to small / non-standard targets

- **Emscripten**: `ports/emscripten/` (README "libdectalk, an emscripten/web port of Fonix DECtalk"; `yarn build:all`,
  `yarn compile` rebuilds `dapi`/`say.c`) [local]. `ports/emscripten/src/say.c` is a 2-argument program:
  `argv[1]` output wav, `argv[2]` text; it calls `TextToSpeechStartup`, `TextToSpeechOpenWaveOutFile`,
  `TextToSpeechSpeak(h, text, TTS_FORCE)`, `TextToSpeechSync` [local]. Merged as PR #28 (2022-11-15, 7coil); the
  public demo is https://webspeak.terminal.ink/ [local README]. Separate: `echogarden-project/dectalk-wasm` [unverified].
- **Epson ARM7** (Fonix era, ~2002): `src/dapi/src/dtarm7epson.def`, `epsonarm7/epsonarm7.mcp`, `epsonarm7lts/`,
  `epsonltsdummy/`, `espontest/`, `api/epsonapi.[ch]` (Metrowerks CodeWarrior `.mcp` projects); `#ifdef EPSON_ARM7`
  in `dectalkf_*.h` forces `HLSYN` on and includes `ltsnames.h` [local]. This is the lineage of the Epson S1V30120
  "Fonix DECtalk v5" TTS IC (11.025 kHz output, SPI-controlled) used in the Parallax Emic 2 [verified: search results
  for Epson S1V30120; core/clock not confirmed - Epson PDF returned 403].
- **Epson S1C33**: `microdectalk` branch / `microdectalk.zip` = DECtalk *Express* firmware (4.2-era) with `186.h`,
  `S1C33Audio.h`, `S1C33Memory.h`, `FonixCore.h` [verified: branch contents API].
- **Windows CE**: `src/dapi/src/ce/` (`wince.c`, `cemm.h`), `CE.DSP/CE.MAK/CE.VCP`, `Cebuild.bat`, `samplece/`;
  4.61 added palm-size/Pocket PC support and "reduced footprint" [local; verified:
  https://dectalk.github.io/dectalk/what_s_new_in_dectalk_v4_61.htm]. CE builds excluded HLSYN in 2001, included it in 2002 [local header history].
- **GBA / RP2040 / UEFI**: DECtalkMini `platforms/gba/main.c` (`TARGET_RATE 11025`, 19 s static buffer in EWRAM,
  generate-then-play, i.e. not necessarily real time on the 16.78 MHz ARM7TDMI) and `platforms/pico/main.c`
  (11025 Hz I2S, UART text input, `set_sys_clock_48mhz()` commented out) [verified: raw files]. No benchmark numbers
  published.
- **DOS/DJGPP**: nothing. Searches for "dectalk djgpp", "dectalk dos port", "dectalk freedos", "dectalk open watcom"
  return only our own `ccdavis/vintage-pc-speech` and generic DJGPP pages [verified: WebSearch, 2026-09-20].
- **Linux `say`** (from `src/docsosf/man/man1/say.1`, the Tru64/Linux sample) [local]:
  `say [-h] [-s #] [-r #] [-d #] [-e #] [-fi file] [-fo file] [-a text]`; `-a` quoted text, `-fi` input text file,
  `-fo` output wav, `-e` 1 = PCM 16-bit mono 11 kHz, 2 = PCM 8-bit mono 11 kHz, 3 = mu-law 8-bit 8 kHz, `-s` speaker 1-9,
  `-r` rate 75-650, `-d` audio device; with no args it reads stdin. `-pre "[:phoneme on]"` is not in this man page
  [unverified whether the develop-branch `say` adds it; check `src/samplosf/src/dtsamples/say.c` args before relying on it].

## 4. DECtalk PC (DTC07) / DECtalk Express and what is in `src/hardware`

- **DECtalk-PC card**: "three quarter size XT/AT-bus option card", DTC07-BA, $1195; TSR uses 20 KB RAM; interface via
  DOS `COPY`/`PRINT`, BIOS COM/LPT calls, or direct TSR calls; switch-selectable BIOS address, I/O, IRQ
  [local: `src/Txt16bit/dt_specs.txt`]. "The board is completely dependent on the software, requiring the PC to upload
  its sound data before it will speak"; DOS TSR creates pseudo-COM and pseudo-LPT ports; 80186 CPU ("an 80186 powered
  every DECtalk device up to and including the DECtalk-PC") [verified: oldvcr.blogspot.com/2023/05/... ]. Processor
  clock 10 MHz (DECtalk PC) vs 20 MHz (DECtalk PC2), PC2 8.75 in vs 13.3 in card [verified:
  https://rmdir.de/~michael/DECtalk_Express/rongemma/faqs.htm]. Whether the card also carries a TMS320 DSP is
  [unverified]; the firmware makefiles build the vocal tract as `vtm_i.exe` (integer) / `vtm_f.exe` (float) with
  `cl /Ox /G1 /Gs` (`/G1` = 80186 instructions), which suggests the integer VTM ran on the 80186 itself [local:
  `src/hardware/src/dtpcmacr.mak`, `dtpccjl1.mak` lines 115, 240]. MAME has a WIP DTC07 driver; the DOS loader
  (`dt_conf`, `dt_driv`, `dt_load`) uploads `kernel.sys`, `dtpc.dic`, `lts.exe`, `ph.exe`, `cmd.exe`
  [verified: https://github.com/mamedev/mame/issues/12501].
- **DECtalk Express (DTC08, 1994)**: AMD Am386SXLV-25 at 25 MHz, 1 MB DRAM (2 x 256Kx16), two ROMs, 2 x 1 Mbit flash,
  16C550 UART, **TMS320C25FNL-J DSP at 20 MHz**, NiCad battery, MMJ RS-423 at 9600 baud only; the only 386-based
  DECtalk and "the most computationally powerful" [verified: oldvcr]. Boot banner: "ROM checksum ... Copyright
  (C)1993-1996 Digital Equipment Corporation ... ss:esp" then "[DECtalk Express is running.]" after ~15 s.
- **In our clone, `src/hardware/src/`** (266 files) [local]:
  - `misc/`: DECtalk-PC DOS side - TSR (`tsr_main.c`, `tsr_com.c`, `tsr_lpt.c`, `dttsr.h`), loader (`ldr_*.c`,
    `pcboot.c`, `mkboot.c`), card boot ROM (`bootup.asm` "PC-DECtalk boot and diagnostic rom"), `MKPDROM.C`,
    code pages, user dictionary tools, `wsr.*` (Windows 3.x speak program), `dectalk.ini`, `!DTC07BA.CFG`.
  - `dll/`: `DTPC.DLL` Windows 3.x driver (`drv_main.c` "DECtalk-PC window Driver process", 1995).
  - `console/`: DECtalk **Express** "microPOST" monitor/console in 386 protected-mode MASM (`c0.asm`, `destart.asm`
    "DECtalk Express preload init", `.386P`, `82c315a.inc`, `MON.HEX`, `DTEXP.BAT`, `dtex.zip`).
  - `lib/`: card-side runtime (`printf.c`, `putc.c`, `gpio.c`, `serialu.c`, `kdisable.c`...).
  - `nwsnoaa/`: NOAA Weather Radio voice text. `dtpc*.mak`, `dtpckits.mak`: 1994-95 NMAKE kit builders (Carl Leeber).
  - The engine modules themselves (`cmd`, `lts`, `ph`, `vtm`) are the shared `dapi/src` sources compiled 16-bit; so the
    repo does contain a **16-bit real-mode x86 build recipe of the engine** (for the card's 80186, Microsoft C 8/MASM
    6.11), but not a DOS-hosted synthesizer. `microdectalk` is the Express firmware re-targeted to S1C33.
  - `datajake1999/dtpcnt`: Win32 port of the 16-bit `DTPC.DLL` using InpOut32; loads only DECtalk PC software 4.2;
    tested with a PC2 on a Pentium II / Windows 2000 [verified: https://github.com/datajake1999/dtpcnt].

## 5. Legal status (five lines)

1. `LICENCE` is Fonix's 2002-03 proprietary notice ("authorized only pursuant to a valid written license from FONIX");
   the older headers say Force Computers 2000-01 and DEC 1995-98 [local: `LICENCE`, `src/dectalkf_klsyn.h`].
2. Chain of title: DEC -> Compaq (1998) -> SMART Modular (1999) -> Force Computers (2000) -> Fonix Speech (Dec 2001) ->
   renamed SpeechFX, Inc. (by 2011, SEC filing) [verified: Wikipedia DECtalk wikitext; Wikipedia SpeechFX].
3. Fonix "went out of business in 2014 and Speech FX in 2020" [verified: https://dectalk.nu/, a fan archive; no primary
   corporate record found]; speechfxinc.com today is an unaffiliated reference site [verified].
4. No statement from any rights-holder about the GitHub repo was found (no takedown, no licence-related issue in the
   repo's issue search [verified: GitHub issues search]); every fork repeats "not under any open-source license".
5. The community ships builds anyway: GitHub Actions/Releases binaries, webspeak.terminal.ink, the in-tree NVDA add-on
   (`src/nvda/manifest.ini` v1.4, tested NVDA 2025.1), serrebidev/DECTalkNVDA, DECtalkMini, dectalk.nu downloads
   [local; verified: releases API, search results].

## 6. Performance data points

- DECtalk Software 4.6x official requirement text is deliberately vague: "an Intel-based Windows host ... performance
  is dependent on your system having enough computational power ... in a heavily loaded multi-tasking environment
  DECtalk could become starved ... and will not speak continuously"; audio 8/16-bit at 11.025 kHz; 31 MB disk
  [local: `src/docs/DTK_IG_4_6_2.pdf`, "Hardware Requirements"]. Supported OS list 95/98/ME/NT/2000/XP.
- Third-party FAQ for 4.x: "software-only ... only a sound card supported by the operating system and at least a
  **486/50** CPU" [verified: https://rmdir.de/~michael/DECtalk_Express/rongemma/faqs.htm].
- Hardware that ran the same algorithm family: DTC01 (1984) 68000 + TMS32010 at 20 MHz; DTC03 onward 80186 at 20 MHz;
  DECtalk-PC 80186 at 10 MHz (PC2: 20 MHz) running integer `vtm_i`; Express Am386SX-25 + TMS320C25 at 20 MHz
  [verified: oldvcr; rmdir FAQ; local makefiles]. So the classic 4.2-era engine ran in real time on a 10-25 MHz
  16-bit/386SX class CPU when the VTM was integer (and, on the Express, DSP-assisted).
- The develop tree's VTM exists in both integer (`vtm_i.c`, `decvoc_i.c`, `vtm_iman.c`) and float (`vtm_f.c`,
  `vtm_fa.c`, `FP_VTM`) forms; `FP_VTM` was "added for alpha builds" (1998) and is `#define`d in `dectalkf_klsyn.h`
  line 162 [local]. DECtalkMini made float VTM optional and "default disabled" (2026-08-08), implying the integer path
  is what they run on GBA/Pico [verified: DECtalkMini commits]. For a 386/486 without FPU, building with the integer
  VTM is the obvious first experiment.
- No published MIPS/CPU-% figures for DECtalk Software on Pentium-class machines were found [verified: searches].

## 7. Recommendation for the DOS port

Use `dectalk/dectalk` `develop` @ `69ebb459` (already cloned). Keep `dectalkf.h` -> `dectalkf_klsyn.h`, undefine
`FP_VTM` to get the integer vocal tract, drop `hlsyn/` from the build (it is still in `Makefile.sub.in`), and start
from `src/samplosf/src/dtsamples/say.c` + `src/dapi/src/nt/` audio glue as the shape of a DJGPP `say`. The `463` repo
and `ad-new.zip` are only worth mining for dictionaries/LTS data, not for the voice.
