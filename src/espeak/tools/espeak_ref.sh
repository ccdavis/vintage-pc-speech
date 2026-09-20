#!/bin/sh
# reference: the same espeak-ng commit, en+klatt, downsampled to $1 Hz 8-bit: espeak_ref.sh rate "text" out.wav
D="$(cd "$(dirname "$0")/.." && pwd)/build-host"
ESPEAK_DATA_PATH="$D" "$D/src/espeak-ng" --path="$D" -v en+klatt --stdout "$2" | ffmpeg -loglevel error -y -i - -ar "$1" -ac 1 -acodec pcm_u8 "$3"
