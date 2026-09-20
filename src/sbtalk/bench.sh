#!/usr/bin/env bash
# Foreground synthesis speed of both engines under DOSBox-X with a 386 core at fixed cycles.
# usage: src/sbtalk/bench.sh [cycles]   (builds first; prints "bench SAM/Klatt: ... x real time")
A="$(cd "$(dirname "$0")/../.." && pwd)"; C=${1:-5000}
export WATCOM="$A/../tools/ow"
make -s -C "$A/src/sbtalk" install >/dev/null 2>&1 || { echo build failed; exit 1; }
run() { sed "s|sbtalk com3 /test|$1|; s|sbtalk-5000.txt|bench.txt|; s|cycles=fixed 5000|cycles=fixed $C|" "$A/run/dosbox/sbtalk-5000.conf" > "$A/run/dosbox/bench.conf"
  rm -f "$A/share/out/BENCH.TXT"; SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy timeout 300 dosbox-x -conf "$A/run/dosbox/bench.conf" -nogui -nomenu >/dev/null 2>&1
  tr -d '\r' < "$A/share/out/BENCH.TXT" 2>/dev/null | grep bench || echo "no result for: $1"; }
run "sbtalk com3 /bench"
run "sbtalk3 com3 /bench"
run "sbtalk3 com3 /klatt /bench"
