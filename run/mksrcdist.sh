#!/usr/bin/env bash
# Build dist/a11y-src.zip: the source distribution (everything we wrote or modified, no binaries,
# no upstream clones). Contents are taken from git so nothing untracked slips in.
set -e
D="$(cd "$(dirname "$0")/.." && pwd)"; cd "$D"
OUT=dist/a11y-src.zip; rm -f "$OUT"; rm -rf dist/src-tmp; mkdir -p dist/src-tmp
git -C "$D/.." archive --format=tar HEAD -- \
  freedos/src/sbtalk freedos/src/klatt freedos/src/espeak freedos/src/retro freedos/src/horndrv \
  freedos/src/sbsynth freedos/src/provox7 freedos/src/sam-upstream \
  freedos/share/talkdisk freedos/dist/README.TXT 'freedos/dist/*.BAT' freedos/dist/SOURCES.md \
  freedos/run freedos/PLAN.md freedos/README.md freedos/research/01-dos-screen-readers.md \
  freedos/research/02-dos-software-speech.md freedos/research/03-old-dos-synths.md \
  | tar -x -C dist/src-tmp
# drop what does not belong in a source bundle
find dist/src-tmp -name '*.EXE' -o -name '*.exe' -o -name '*.obj' -o -name '*.o' -o -name '*.wav' -o -name '*.raw' -o -name '*.map' \
  | grep -v "provox7/\(PROVOX7\|PV7\|FAKE\|A86\|EXMAC\|XREF\)" | xargs rm -f
rm -rf dist/src-tmp/freedos/src/espeak/results/*.wav dist/src-tmp/freedos/src/klatt/results/*.wav
mv dist/src-tmp/freedos dist/src-tmp/a11y-src && cp dist/SOURCES.md dist/src-tmp/a11y-src/SOURCES.md
(cd dist/src-tmp && zip -q -r "../$(basename "$OUT")" a11y-src)
rm -rf dist/src-tmp
ls -la "$OUT"; unzip -l "$OUT" | tail -1
