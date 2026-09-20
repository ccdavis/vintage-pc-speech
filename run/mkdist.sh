#!/usr/bin/env bash
# Build dist/a11y386.zip and dist/floppy/: the Phase 1 test package for the real 386.
D="$(cd "$(dirname "$0")/.." && pwd)"; cd "$D" && STRIP="${TOOLS:-$(dirname "$0")/../../tools}/djgpp/bin/i586-pc-msdosdjgpp-strip"; [ -f share/espk/ESPK.EXE ] && "$STRIP" share/espk/ESPK.EXE 2>/dev/null; [ -f share/espk/ESPKD.EXE ] && "$STRIP" share/espk/ESPKD.EXE 2>/dev/null
make -s -C src/sbsynth install && make -s -C src/sbtalk install >/dev/null; [ -f share/espk/ESPK.EXE ] || make -s -C src/espeak install >/dev/null
rm -rf dist/a11y386.zip dist/espk386.zip dist/floppy dist/floppy2 && mkdir -p dist/floppy dist/floppy2
cp dist/README.TXT dist/GO.BAT dist/PROVOX.BAT dist/TALK.BAT dist/BENCH.BAT dist/DOSTEST.BAT dist/DOSTEST2.BAT share/SBTALK.EXE share/SBTALK3.EXE share/SBTALKXT.EXE share/SAY.EXE share/SBPLAY.EXE share/SAMSB.EXE share/CWSDPMI.EXE share/HELLO.WAV \
   share/PROVOX7.EXE share/PROVOX7T.EXE share/PV7.EXE dist/floppy/ && cp src/provox7/provox7.doc dist/floppy/PROVOX7.DOC
(cd dist/floppy && zip -q ../a11y386.zip *) && ls -la dist/a11y386.zip && du -sh dist/floppy
# second floppy: the eSpeak voice and DECtalk (both UPX-packed so that they share one 1.44 MB disk),
# with the SBEMU stack for machines without a Sound Blaster (see dist/DTALK.BAT)
UPX="${TOOLS:-$(dirname "$0")/../../tools}/upx/upx"
cp share/espk/ESPK.EXE share/espk/ESPKD.EXE share/espk/ESPK.DAT share/espk/CWSDPMI.EXE dist/ESPK.BAT dist/TALKD.BAT dist/TALKD2.BAT dist/floppy2/
"$UPX" -qq --best dist/floppy2/ESPK.EXE dist/floppy2/ESPKD.EXE >/dev/null 2>&1 || true
make -s -C src/dectalk stage >/dev/null && cp share/talkpc/DTALKD.EXE share/talkpc/SBEMU.EXE share/talkpc/HDPMI32i.EXE share/talkpc/JLOAD.EXE share/talkpc/QPIEMU.DLL share/SAY.EXE dist/DTALK.BAT dist/DTALK2.BAT dist/floppy2/
(cd dist/floppy2 && zip -q ../espk386.zip *) && ls -la dist/espk386.zip && du -sh dist/floppy2
# the talking disk for newer PCs as a plain zip (run/mktalkpc.sh builds the image and dist/talkpc/)
[ -d dist/talkpc ] && rm -f dist/talkpc.zip && (cd dist/talkpc && zip -q ../talkpc.zip *) && ls -la dist/talkpc.zip
