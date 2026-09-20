#!/usr/bin/env bash
# Mirror the source distribution into the public repository working tree (default ~/vintage-pc-speech),
# from git HEAD of this project, same file list as mksrcdist.sh. Commit and push there afterwards.
set -e
D="$(cd "$(dirname "$0")/.." && pwd)"; P="${1:-$HOME/vintage-pc-speech}"
[ -d "$P/.git" ] || { echo "no git repo at $P"; exit 1; }
tmp=$(mktemp -d)
git -C "$D/.." archive --format=tar HEAD -- \
  freedos/src/sbtalk freedos/src/klatt freedos/src/espeak freedos/src/retro freedos/src/horndrv \
  freedos/src/sbsynth freedos/src/provox7 freedos/src/sam-upstream freedos/src/dectalk freedos/share/talkpc \
  freedos/share/talkdisk freedos/dist/README.TXT 'freedos/dist/*.BAT' freedos/dist/SOURCES.md \
  freedos/run freedos/PLAN.md freedos/README.md freedos/research/01-dos-screen-readers.md \
  freedos/research/02-dos-software-speech.md freedos/research/03-old-dos-synths.md \
  freedos/research/04-modern-dos-audio.md freedos/research/05-dectalk-sources.md \
  | tar -x -C "$tmp"
find "$tmp" -name '*.EXE' -o -name '*.exe' -o -name '*.obj' -o -name '*.o' -o -name '*.wav' -o -name '*.raw' -o -name '*.map' \
  | grep -v "provox7/\(PROVOX7\|PV7\|FAKE\|A86\|EXMAC\|XREF\)" | xargs rm -f
rm -rf "$tmp"/freedos/src/espeak/results/*.wav "$tmp"/freedos/src/klatt/results/*.wav
for d in src share dist run research PLAN.md HARNESS.md; do rm -rf "$P/$d"; done
mv "$tmp"/freedos/README.md "$tmp"/freedos/HARNESS.md      # the public repo keeps its own README.md
cp -r "$tmp"/freedos/. "$P"/
rm -rf "$tmp"
echo "synced to $P; now: cd $P && git add -A && git commit && git push"
