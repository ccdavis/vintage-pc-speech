#!/bin/sh
# usage: tools/rsynth_wav.sh RATE "text" out.wav [say options]
# Runs upstream rsynth's say (./rsynth_say, float, its own synth) and writes a WAV.
here=$(dirname "$0")/..
rate=$1; text=$2; out=$3; shift 3
"$here/rsynth_say" -r "$rate" "$@" -l "$out.raw" "$text" 2>/dev/null
ffmpeg -loglevel error -y -f s16le -ar "$rate" -ac 1 -i "$out.raw" "$out" && rm -f "$out.raw"
