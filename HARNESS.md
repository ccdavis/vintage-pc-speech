# accessible_os / freedos

A DOS screen reader and a software speech synthesizer for FreeDOS on a 386 DX-25 with a Sound
Blaster 32. See PLAN.md for findings, architecture and phases; research/ holds the survey reports.

## Layout

- `research/01-dos-screen-readers.md`  survey of DOS screen readers (Provox has GPL source).
- `research/02-dos-software-speech.md` survey of software synthesizers and SB16 programming.
- `research/evidence/`                 captured Provox speech stream, screenshots, SAM audio capture.
- `src/provox7/`     Provox 7.03 source and 7.05 binaries (GPL v2+, from provox7.zip at HAPP), with one
                     fix in `2I8CODE.A`. `build/` (git-ignored) holds the A86 assembly output.
- `src/sam-upstream/` SAM, vendored from github.com/s-macke/SAM (see SAM-UPSTREAM-COMMIT.txt).
- `src/sbtalk/`      SBTALK, the resident synthesizer (Open Watcom 16-bit): virtual DoubleTalk on INT 14h,
                     SB auto-init DMA, SAM engine as a coroutine driven from the SB IRQ. `SAY.EXE` speaks
                     via INT 14h. `make host` builds `sbtalk_host` (same parser+engine to a WAV).
- `src/klatt/`       fixed-point Klatt engine + rsynth frontend (host build `klattsay`, `make check` parity);
                     `res386.asm`/`imul32.asm` are the 386 hot paths used by the DOS build.
- `src/sbsynth/`     DJGPP programs: `sb16.c` (DSP + DMA 8-bit playback), `SBPLAY.EXE` (WAV/tone
                     player), `SAMSB.EXE` (SAM text-to-speech straight to the card, prints synthesis time).
- `run/`             QEMU and DOSBox-X harness (below). `share/` is the directory the guest sees as C:.
- `images/`          FreeDOS 1.4 LiveCD and BonusCD (git-ignored).

Toolchains live in `../tools/`: `djgpp/` (i586-pc-msdosdjgpp-gcc 12.2) and `ow/` (Open Watcom 2.0,
use `binl64/`, set `WATCOM=$PWD/../tools/ow`). Both are git-ignored downloads.

## The talking disk

    run/mktalkdisk.sh        # builds dist/talkdisk.img from images/FD14BOOT.img inside QEMU (share/MKTALK.BAT)
    BOOT=a FLOPPYIMG=$PWD/run/talkdisk.img run/freedos.sh   # boot it; audio in run/freedos-audio.wav

The disk's files live in `share/talkdisk/` (FDCONFIG.SYS, FDAUTO.BAT, HELP.BAT, BLASTER.BAT, README.TXT).

## DECtalk for newer PCs (2005-2010 laptops, Intel HD Audio)

`src/dectalk/` builds DECtalk (github.com/dectalk/dectalk, ARM7 single-threaded configuration,
dictionary compiled in; the upstream clone in `src/dectalk-upstream/` is git-ignored and not
mirrored, its licence is Fonix proprietary) with DJGPP: `DTSAY.EXE` (one shot) and `DTALKD.EXE`
(resident, the ESPKD framework: DoubleTalk on COM3 via INT 14h, Sound Blaster DMA).  On Intel HD
Audio machines the Sound Blaster is SBEMU (`share/sbemu/`, VDPMI build).  See `src/dectalk/NOTES.md`.

    make -C src/dectalk dos resident stage     # DTSAY.EXE, DTALKD.EXE, packed copy in share/talkpc/
    run/mktalkpc.sh                            # dist/talkpc.img: bootable floppy for those PCs (spoken first-boot menu)
    run/testtalkpc.sh                          # boot a copy on emulated HD Audio (SOUND=hda), answer the menu, check audio
    dist/talkpc/                               # the same files as a directory (run/imgextract.py), see COPYING.TXT there
    SOUND=hda MEM=256 run/boot.sh              # LiveCD with an ICH9 HDA instead of the SB16 (guest: c:\sbemu\vdpmi, c:\sbemu\sbemuv)

## Running

    run/boot.sh                          # boot the LiveCD in Live mode, console on COM1, waits for the prompt
    run/serial.sh freedos "dir c:"       # run a DOS command, print its output
    run/serial.sh freedos "echo hi > con"  # write to the screen (what a screen reader sees)
    run/type.sh freedos "text"           # type on the guest keyboard via the QEMU monitor
    run/shot.sh freedos name             # screenshot -> run/freedos-name.png
    run/mon.sh freedos "sendkey ret"     # raw monitor command
    python3 run/wavcheck.py run/freedos-audio.wav [offset]   # duration, peak, dominant frequency of the SB capture

Sound Blaster output is captured to `run/freedos-audio.wav`. Provox's serial synthesizer port is COM2,
logged to `run/freedos-com2.log`. Guest RAM defaults to 64 MB (`MEM=`), CPU model `486` (`CPU=`).

    set BLASTER=A220 I5 D1 H5 P330 T6    # in the guest
    c:\sbplay -tone 440
    c:\samsb Hello from Free DOS
    c:\sbtalk com3                       # resident synthesizer (add /TEST for 6 s of diagnostics, not resident)
    c:\sbtalk3 com3 /klatt               # 386 build, Klatt engine; /BENCH [text] times an engine in the foreground
    src/sbtalk/bench.sh 5000             # both engines' speed under DOSBox-X at 386 DX-25 cycles
    c:\say hello there                   # speak through it
    c:\provox7                           # or c:\provox7t (rebuilt from source)
    c:\pv7 LITETALK COM3                 # Provox talks through SBTALK (COM2 = QEMU's host-logged serial port)

For cycle-limited timing, `run/dosbox/386-*.conf` run `SAMSB` under DOSBox-X with a 386 core at fixed
cycles and write the timing to `share/out/`:

    SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy dosbox-x -conf run/dosbox/386-5000.conf -nogui -nomenu

Note: the snap-packaged dosbox-x cannot see /tmp, so keep its mounts under the home directory.

## Building

    make -C src/sbsynth install          # DJGPP cross build, copies the EXEs into share/
    # Provox: assemble in DOSBox-X (A86 needs DOS), link on the host
    dosbox-x -conf src/provox7/build/pv.conf -nogui -nomenu     # after copying *.A, A86.COM, A86.LIB into build/
    WATCOM=../tools/ow ../tools/ow/binl64/wlink system dos file src/provox7/build/PROVOX7.OBJ name PROVOX7T.EXE

## Publishing

The public mirror is https://github.com/ccdavis/vintage-pc-speech (working tree `~/vintage-pc-speech`).
Sources are git content there; the four artifacts are release assets.

    run/sync-public.sh                 # copy the source subset (same list as mksrcdist.sh) into the mirror
    (cd ~/vintage-pc-speech && git add -A && git commit -m "..." && git push)
    (cd ~/vintage-pc-speech && ./release.sh --publish [vTAG])   # rebuild everything and upload a release
                                       # or, from here: run/mkdist.sh; run/mktalkdisk.sh; run/mksrcdist.sh; gh release create ... dist/*

The mirror's GitHub Actions workflow only compiles the DOS programs as a check (it fetches the toolchains);
the disk image needs QEMU with KVM and the FreeDOS images, so releases are built locally.
