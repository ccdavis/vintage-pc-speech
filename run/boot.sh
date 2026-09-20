#!/usr/bin/env bash
# (Re)boot FreeDOS under QEMU into Live mode with the console on COM1; waits until the shell answers.
# usage: run/boot.sh [extra qemu args]
D="$(cd "$(dirname "$0")" && pwd)"
pkill -f '^qemu-system-i386' 2>/dev/null; sleep 1
rm -f "$D/freedos-serial.log" "$D/freedos-audio.wav"
(cd "$D" && nohup ./freedos.sh "$@" > "$D/freedos-qemu.log" 2>&1 &)
sleep 8; "$D/mon.sh" freedos "sendkey ret" >/dev/null
for t in $(seq 1 30); do
  sleep 3
  "$D/type.sh" freedos "ctty com1"
  sleep 1
  out=$("$D/serial.sh" freedos "ver" 2)
  if grep -q FreeCom <<<"$out"; then echo "freedos ready after ~$((8+t*4))s"; exit 0; fi
done
echo "freedos did not come up"; cat "$D/freedos-qemu.log"; exit 1
