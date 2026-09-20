#!/usr/bin/env bash
# Regression test for the resident synthesizer under QEMU FreeDOS: run/test-sbtalk.sh  (prints ok/FAIL lines)
D="$(cd "$(dirname "$0")" && pwd)"; W="$D/freedos-audio.wav"; pass=0; fail=0
ok() { echo "ok   $1"; pass=$((pass+1)); }; bad() { echo "FAIL $1"; fail=$((fail+1)); }
sz() { stat -c %s "$W" 2>/dev/null || echo 0; }
loud() { python3 - "$W" "$1" <<'PY'
import sys,array
d=open(sys.argv[1],'rb').read()[max(int(sys.argv[2]),44):]; a=array.array('h'); a.frombytes(d[:len(d)//2*2]); a=a[::2]
print(sum(1 for x in a if abs(x)>300))
PY
}
say() { "$D/serial.sh" freedos "$1" "${2:-3}"; }
"$D/boot.sh" >/dev/null || { bad "boot"; exit 1; }
# --- SAM on the card, resident
b=$(sz); out=$(say "c:\\sbtalk com3" 5); grep -q "SBTALK" <<<"$out" && ok "SBTALK loads: $(grep SBTALK <<<"$out" | head -1 | cut -c1-60)" || bad "SBTALK load: $out"
sleep 2; [ "$(loud $b)" -gt 5000 ] && ok "ready message produced audio" || bad "no audio for the ready message"
b=$(sz); say "c:\\say one two three four five six seven eight nine ten" 1 >/dev/null; sleep 6; n1=$(loud $b)
[ "$n1" -gt 20000 ] && ok "SAY speaks ($n1 loud samples)" || bad "SAY produced little audio ($n1)"
b=$(sz); say "c:\\say one two three four five six seven eight nine ten eleven twelve thirteen fourteen" 1 >/dev/null; sleep 1; say "c:\\say /x" 1 >/dev/null; sleep 5; n2=$(loud $b)
[ "$n2" -lt "$n1" ] && ok "flush cut the utterance ($n2 < $n1)" || bad "flush did not shorten speech ($n2 vs $n1)"
out=$(say "c:\\sbtalk com3" 4); grep -qi "resident\|already" <<<"$out" && ok "second load refused: $(grep -i 'resident\|already' <<<"$out" | head -1 | cut -c1-60)" || bad "second load not refused: $out"
say "c:\\provox7" 2 >/dev/null; say "c:\\pv7 LITETALK COM3" 3 >/dev/null; sleep 20
b=$(sz); say "echo the quick brown fox jumps over the lazy dog > con" 2 >/dev/null; sleep 8; [ "$(loud $b)" -gt 10000 ] && ok "Provox speaks screen output through SBTALK" || bad "no audio from Provox"
# --- speaker paths on a fresh boot (RTC and timer 0 must keep the DOS clock)
"$D/boot.sh" >/dev/null
for mode in "/spk" "/pit"; do
  s0=$(date +%s); out=$(say "c:\\sbtalk com3 $mode /test" 14); e0=$(date +%s)
  grep -q "t=108" <<<"$out" && ok "$mode /TEST: BIOS clock reached 110 ticks (wall $((e0-s0))s incl. 14s wait)" || bad "$mode /TEST: BIOS ticks did not advance: $(grep -c '^t=' <<<"$out") lines"
  out=$(say "c:\\sbtalk com3 $mode" 5); grep -q "SBTALK" <<<"$out" && ok "SBTALK $mode loads resident" || bad "SBTALK $mode: $out"
  say "c:\\say one two three four five six seven" 1 >/dev/null; sleep 8
  say "ver" 2 | grep -q FreeCom && ok "$mode: DOS still answers after speech" || bad "$mode: DOS unresponsive"
  "$D/boot.sh" >/dev/null
done
# --- 1983 voice bit mode and Klatt on the card
b=$(sz); out=$(say "c:\\sbtalkxt com3 /retro /bits" 4); grep -q "SBTALK" <<<"$out" && ok "SBTALKXT loads: $(grep SBTALK <<<"$out" | head -1 | cut -c1-60)" || bad "SBTALKXT: $out"
say "c:\\say hello world one two three" 1 >/dev/null; sleep 7; [ "$(loud $b)" -gt 20000 ] && ok "1983 voice bit mode speaks" || bad "no audio in bit mode"
"$D/boot.sh" >/dev/null
b=$(sz); out=$(say "c:\\sbtalk3 com3 /klatt" 5); grep -q "Klatt" <<<"$out" && ok "SBTALK3 /KLATT loads" || bad "SBTALK3: $out"
say "c:\\say please open the file and read the first line" 1 >/dev/null; sleep 9; [ "$(loud $b)" -gt 20000 ] && ok "Klatt speaks" || bad "no Klatt audio"
pkill -f '^qemu-system-i386'
echo "passed $pass, failed $fail"
