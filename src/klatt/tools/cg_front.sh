#!/bin/sh
# usage: cg_front.sh klattsay_binary "opts" "text" -> Ir per audio second for engine, frontend, total
S=/tmp/claude-1000/-home-ccd-accessible-os/f017725b-8796-4160-940f-c815cc0af146/scratchpad
bin=$1; opts=$2; text=$3
valgrind --tool=callgrind --callgrind-out-file=$S/cg_f.out $bin $opts "$text" $S/cg_tmp2.wav 2>$S/cg_f.err >/dev/null
secs=$(sed -n 's/.*(\([0-9.]*\) s).*/\1/p' $S/cg_f.err)
ann=$(callgrind_annotate $S/cg_f.out 2>/dev/null | sed '/Auto-annotated/q')
eng=$(echo "$ann" | grep 'klatt_fx.c:' | awk '{gsub(",","",$1); s+=$1} END{print s+0}')
fr=$(echo "$ann" | grep -E '(holmes|nrl|phtoelm_tab|elements_tab)\.c:' | awk '{gsub(",","",$1); s+=$1} END{print s+0}')
printf "%-22s %.2f s : engine %8.0f  frontend %7.0f  total %8.0f instr/audio-second\n" "\"$opts\"" $secs $(echo "$eng/$secs" | bc -l) $(echo "$fr/$secs" | bc -l) $(echo "($eng+$fr)/$secs" | bc -l)
