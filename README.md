# Vintage PC speech: a talking DOS disk for old PCs

Screen reader plus software speech synthesis for DOS machines from an 8088 XT up to a 386 and
beyond, playing through a Sound Blaster or the PC speaker. Built for a 386 DX-25 with a Sound
Blaster 32 running DR-DOS 7.02 and for an IBM PS/2 Model 30 (8086), tested under FreeDOS, and
meant to work on MS-DOS as well.

**Downloads** are on the [Releases](../../releases) page:

- `talkdisk.img`: a bootable 1.44 MB FreeDOS floppy. It speaks a menu on the PC speaker at the
  first boot, asks what sound hardware you have, saves the choice, and from then on boots straight
  into a talking DOS (the Provox screen reader reading everything through the synthesizer).
- `a11y386.zip` and `espk386.zip`: the test floppies (blind-runnable batch files: `A:\GO`,
  `TALK`, `TALKD`, `BENCH`, `DOSTEST`), see `dist/README.TXT`.
- `a11y-src.zip`: this source tree as a bundle (`dist/SOURCES.md` lists every component).

## What is inside

| Piece | Role |
|---|---|
| `src/sbtalk` | SBTALK: resident synthesizer that looks like a serial DoubleTalk on a COM port (so Provox, JAWS for DOS, ASAP and others drive it unmodified), with Sound Blaster DMA output or PC speaker output, and three voices: SAM (1982 formant engine), a fixed-point Klatt, and the 1983 SPEECH.COM one-bit voice. 8086, 386 and XT builds. |
| `src/klatt` | Fixed-point Klatt synthesizer with 386 assembly hot paths, rsynth-derived English rules, parity and intelligibility tools. |
| `src/espeak` | eSpeak NG's English front end ported to DOS (DJGPP) driving the Klatt engine: `ESPK.EXE` (one shot, card or speaker) and `ESPKD.EXE`, the resident form that hosts a DOS shell and drives the screen reader. The most intelligible voice; needs a 386 and about 2 MB. |
| `src/retro`, `src/horndrv` | The 1983 voice (Andy McGuire's SPEECH.COM phonemes via Jon Hornstein's horndrv source). Practical on an XT. |
| `src/provox7` | Provox 7 screen reader (GPL, 1985-1999, Kansys / Charles E. Hallenbeck), rebuilt from source with one fix. |
| `share/talkdisk` | The talking disk's boot files. |
| `run/` | QEMU and DOSBox-X harness, regression tests, package and disk builders. |
| `PLAN.md`, `research/` | Findings, measurements and the plan; surveys of DOS screen readers and synthesizers. |

Measured on the 386 DX-25: SAM 2.1x real time, Klatt 0.5x (386) / real time on a 486, eSpeak 3x real
time; resident memory 62 to 103 KB for the 16-bit builds, about 25 KB conventional for the resident
eSpeak.

## Building

    run/get-toolchains.sh      # DJGPP and Open Watcom into ../tools (or set TOOLS)
    run/get-freedos.sh         # FreeDOS 1.4 LiveCD + boot floppy into images/
    ./release.sh               # builds everything, the floppies, the disk image and the source bundle into dist/

Individual builds: `make -C src/sbtalk install`, `make -C src/espeak dos ESPKD.EXE install`,
`make -C src/klatt check`. The disk image is assembled by booting FreeDOS in QEMU (`run/mktalkdisk.sh`).
`HARNESS.md` documents the emulator harness. A `Dockerfile` reproduces the build environment.

## Licences

GPL v3 or later for the whole, because it includes eSpeak NG and the Klatt code (GPL v3+); rsynth-derived
parts LGPL v2+; Provox GPL v2+; horndrv freeware + GPL. SAM (`src/sam-upstream`) is reverse-engineered
1982 abandonware with no licence and is kept only as a prototype voice. See `dist/SOURCES.md`.
