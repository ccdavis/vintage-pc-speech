#!/bin/sh
# usage: cg_engine.sh parity_binary track.par sr nfc lite  -> instructions per second of audio, engine only
S=/tmp/claude-1000/-home-ccd-accessible-os/f017725b-8796-4160-940f-c815cc0af146/scratchpad
bin=$1; par=$2; sr=$3; nfc=$4; lite=$5
valgrind --tool=callgrind --callgrind-out-file=$S/cg_e.out $bin -e -r $sr -n $nfc -l $lite $par $S/cg_tmp.wav >/dev/null 2>&1
frames=$(wc -l < $par)
secs=$(echo "$frames / 100" | bc -l)
ir=$(callgrind_annotate $S/cg_e.out 2>/dev/null | sed '/Auto-annotated/q' | grep 'klatt_fx.c:' | awk '{gsub(",","",$1); s+=$1} END{print s}')
printf "sr=%5d nfc=%d lite=%d : %8.0f instr/audio-second  (engine %s Ir over %.2f s)\n" $sr $nfc $lite $(echo "$ir / $secs" | bc -l) $ir $secs
