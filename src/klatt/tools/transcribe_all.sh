#!/bin/sh
# usage: tools/transcribe_all.sh file.wav...  -> prints Whisper (base.en) transcript per file
S=/tmp/claude-1000/-home-ccd-accessible-os/f017725b-8796-4160-940f-c815cc0af146/scratchpad
for f in "$@"; do
  b=$(basename "$f" .wav)
  ffmpeg -loglevel error -y -i "$f" -ar 16000 -ac 1 "$S/$b.16k.wav"
  printf '%-28s ' "$b:"; (cd "$S" && uv run transcribe.py "$b.16k.wav" 2>/dev/null | sed 's/^\[[^]]*\] *//' | tr '\n' ' '); echo
done
