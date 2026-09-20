#!/usr/bin/env bash
# Build dist/a11y386.zip and dist/floppy/: the Phase 1 test package for the real 386.
D="$(cd "$(dirname "$0")/.." && pwd)"; cd "$D" && STRIP="${TOOLS:-$(dirname "$0")/../../tools}/djgpp/bin/i586-pc-msdosdjgpp-strip"; [ -f share/espk/ESPK.EXE ] && "$STRIP" share/espk/ESPK.EXE 2>/dev/null; [ -f share/espk/ESPKD.EXE ] && "$STRIP" share/espk/ESPKD.EXE 2>/dev/null
make -s -C src/sbsynth install && make -s -C src/sbtalk install >/dev/null; [ -f share/espk/ESPK.EXE ] || make -s -C src/espeak install >/dev/null
rm -rf dist/a11y386.zip dist/espk386.zip dist/floppy dist/floppy2 && mkdir -p dist/floppy dist/floppy2
cp dist/README.TXT dist/GO.BAT dist/PROVOX.BAT dist/TALK.BAT dist/BENCH.BAT dist/DOSTEST.BAT dist/DOSTEST2.BAT share/SBTALK.EXE share/SBTALK3.EXE share/SBTALKXT.EXE share/SAY.EXE share/SBPLAY.EXE share/SAMSB.EXE share/CWSDPMI.EXE share/HELLO.WAV \
   share/PROVOX7.EXE share/PROVOX7T.EXE share/PV7.EXE dist/floppy/ && cp src/provox7/provox7.doc dist/floppy/PROVOX7.DOC
(cd dist/floppy && zip -q ../a11y386.zip *) && ls -la dist/a11y386.zip && du -sh dist/floppy
# second floppy: the eSpeak voice (too big to share a 1.44 MB disk with the rest)
cp share/espk/ESPK.EXE share/espk/ESPKD.EXE share/espk/ESPK.DAT share/espk/CWSDPMI.EXE dist/ESPK.BAT dist/TALKD.BAT dist/TALKD2.BAT dist/floppy2/ && (cd dist/floppy2 && zip -q ../espk386.zip *) && ls -la dist/espk386.zip && du -sh dist/floppy2
