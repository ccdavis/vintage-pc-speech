#!/bin/sh
# reference at $1 Hz, 16 bit: espeak_ref16.sh rate "text" out.wav
D="$(cd "$(dirname "$0")/.." && pwd)/build-host"
ESPEAK_DATA_PATH="$D" "$D/src/espeak-ng" --path="$D" -v en+klatt --stdout "$2" | ffmpeg -loglevel error -y -i - -ar "$1" -ac 1 "$3"
