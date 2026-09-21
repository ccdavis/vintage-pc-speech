This repo is a set of disk images you can create talking boot disks from, and collections of files you can put on to utility disks.  The main goal was to make vintage PCs as accessible to blind users.

In particular, I wanted to make the software synthesizers as functional as possible. These days it's hard to come by a hardware synthesizer like DEC Talk Express or Double Talk (I was surprised how pricey they are on Ebay.)   I tried to cover most common cases: PC-speaker only; generic Soundblaster; or 21st Century sound hardware that can work through a Soundblaster emulation layer. I only tested on my 386-DX 25 and a PS/2 Model 30. The 386 has an AWE32 card but I only employed basic SB32 features and those should work on SB16 as well. I need to test with a vintage original Soundblaster still.

The software here is intended to run on existing DOS installations and also offer Free DOS boot disk images to get up and running. If your main drive isn't a FAT filesystem you won't be able to read any hard drive files though. 

I included the open source Provox screen reader. The speech gets  sent to a com port and Provox thinks it has a Double Talk hardware synth attached; really we have made a virtual Double Talk that uses the Soundblaster hardware or PC speaker as its output. In theory you could use other screen readers that work with Double Talk which is most of them.

In the 1980s you could get speech out of an XT or AT system speaker but mostly it was a party trick since it was synchronous and not too useful  for screen readers. Later, Soundblaster improved default hardware speech a little but generating speech was sluggish at best -- still synchronous usually.  You could get SAM or (different I think? Dr. Sbaitso speech to work through a driver program as a TSR.

This release is English only due to space limitations. It would be possible to make a version supporting Spanish for DECTalk and Espeak (Espeak has lots of supported languages.) I don't know about the others. Basically you would include just the language support you need as there isn't room for everything in RAM.

Speech available: 

Speaker:

"Speech.com": Works on original PC through the speaker. It's meant as a bootstrap feature: It's slow but you can at least use it to recover files from an old PC.

In theory we could send sound to the speaker from any of the other synthesizers but it would be slower and tie up the CPU more than using Soundblaster.

Soundblaster 16/32:

SAM:Requires at least a 386 SX or very fast 286. 16MHz 386 is barely capable.

Custom Clat synthesizer built from Rsynth libraries: Decently intelligable, but requires more CPU, at least a 386-40 MHz.

Espeak NG: Requires a 386-25 at least, faster than the custom Clat. Requires more memory though. You need at least 2M of RAM. Only supports English currently.

DECTalk 5.0: Surprisingly this one is about as fast as the custom Clat synth. Realistically you need a 486-25 or better but a 386-40 should just about be able to keep up.

The DECTalk is the best of all the synths. 

There is a boot disk for newer systems using integrated Intel sound and you will want to use only DECTalk or Espeak on those.

This whole project is AI-generated. There's no way I'd have been able to finish the work in less than a year otherwise. The AI could drive Qemu to test out many, many configurations and bug fixes all automatically, plus port from 16 bit C to  with Watcom to DJGPP 32 bit and back. 

All Claude generated from this point 
--------------------------------------------------

# Vintage PC speech: a talking DOS disk for old PCs

Screen reader plus software speech synthesis for DOS machines from an 8088 XT up to a 386 and
beyond, playing through a Sound Blaster or the PC speaker. Tested on a 386 DX-25 with a Sound
Blaster 32 running DR-DOS 7.02 and for an IBM PS/2 Model 30 (8086), tested under FreeDOS, and
meant to work on MS-DOS as well. Also tested with QEMU as a 486 DX-100 and Pentium II.

**Downloads** are on the [Releases](../../releases) page:

- `talkdisk.img`: a bootable 1.44 MB FreeDOS floppy. It speaks a menu on the PC speaker at the
  first boot, asks what sound hardware you have, saves the choice, and from then on boots straight
  into a talking DOS (the Provox screen reader reading everything through the synthesizer).
- `talkpc.img` (and the same files as `talkpc.zip`): the talking disk for 2005-2010 PCs that have
  no Sound Blaster (Intel HD Audio, AC97, PCI sound): FreeDOS + JEMMEX + HDPMI32i + SBEMU + DECtalk
  + Provox, with a spoken first-boot menu that finds the working output. Pentium or better.
- `a11y386.zip` and `espk386.zip`: the test floppies (blind-runnable batch files: `A:\GO`,
  `TALK`, `TALKD`, `DTALK`, `BENCH`, `DOSTEST`), see `dist/README.TXT`. Disk 2 carries the
  eSpeak and DECtalk voices and the SBEMU stack; `DTALK` uses a real Sound Blaster when there is
  one and loads SBEMU otherwise.
- `a11y-src.zip`: this source tree as a bundle (`dist/SOURCES.md` lists every component).

## What is inside

| Piece | Role |
|---|---|
| `src/sbtalk` | SBTALK: resident synthesizer that looks like a serial DoubleTalk on a COM port (so Provox, JAWS for DOS, ASAP and others drive it unmodified), with Sound Blaster DMA output or PC speaker output, and three voices: SAM (1982 formant engine), a fixed-point Klatt, and the 1983 SPEECH.COM one-bit voice. 8086, 386 and XT builds. |
| `src/klatt` | Fixed-point Klatt synthesizer with 386 assembly hot paths, rsynth-derived English rules, parity and intelligibility tools. |
| `src/espeak` | eSpeak NG's English front end ported to DOS (DJGPP) driving the Klatt engine: `ESPK.EXE` (one shot, card or speaker) and `ESPKD.EXE`, the resident form that hosts a DOS shell and drives the screen reader. The most intelligible voice; needs a 386 and about 2 MB. |
| `src/dectalk` | DECtalk built for DOS (DJGPP, the engine's single-threaded embedded configuration, dictionary compiled in): `DTSAY.EXE` and the resident `DTALKD.EXE`, the best voice of the set (Whisper WER 12 % against 30 % for the eSpeak port at 8 kHz). Only the glue, build files and patches are here; `run/get-dectalk.sh` fetches the engine source from github.com/dectalk/dectalk. |
| `share/talkpc` | The boot files of the talking disk for newer PCs; `research/04`, `research/05` cover the hardware, SBEMU and the DECtalk source history. |
| `src/retro`, `src/horndrv` | The 1983 voice (Andy McGuire's SPEECH.COM phonemes via Jon Hornstein's horndrv source). Practical on an XT. |
| `src/provox7` | Provox 7 screen reader (GPL, 1985-1999, Kansys / Charles E. Hallenbeck), rebuilt from source with one fix. |
| `share/talkdisk` | The talking disk's boot files. |
| `run/` | QEMU and DOSBox-X harness, regression tests, package and disk builders. |
| `PLAN.md`, `research/` | Findings, measurements and the plan; surveys of DOS screen readers and synthesizers. |

Measured on the 386 DX-25: SAM 2.1x real time, Klatt 0.5x (386) / real time on a 486, eSpeak 3x real
time, DECtalk 0.95x at 11 kHz and 1.3x at 8 kHz (DOSBox-X estimate); resident memory 62 to 103 KB for the 16-bit builds, about 25 KB conventional for the resident
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
1982 abandonware with no licence and is kept only as a prototype voice. DECtalk is Fonix proprietary
code (the source has circulated since 2015; the rights holder is defunct); its source is not in this
repository, only our glue and patches, but the release binaries contain the engine, as the community's
NVDA add-ons and web builds do. See `dist/SOURCES.md`.
