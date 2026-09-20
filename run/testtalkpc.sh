#!/usr/bin/env bash
# Boot dist/talkpc.img (or $1) on emulated HD Audio, answer the first spoken question with "1", then check that
# the talking shell speaks and the guest is still alive. Prints audio RMS per phase and the CPU state.
D="$(cd "$(dirname "$0")/.." && pwd)"; cd "$D"; S="${SCRATCH:-/tmp/claude-1000/-home-ccd-accessible-os/fa385b08-2dba-4932-a847-cb461e7b8ee9/scratchpad}"
# never boot the distributable image itself: the first boot writes VOICE.BAT into the image it runs from
IMG="${1:-$D/dist/talkpc.img}"; cp "$IMG" "$D/run/talkpc-test.img"; IMG="$D/run/talkpc-test.img"
pkill -f '^qemu-system-i386' 2>/dev/null; sleep 1; rm -f run/freedos-audio.wav
(BOOT=a FLOPPYIMG="$IMG" SOUND=hda MEM=256 nohup run/freedos.sh > run/freedos-qemu.log 2>&1 &)
sleep 20; o=$(stat -c %s run/freedos-audio.wav); run/mon.sh freedos "sendkey 1" >/dev/null; sleep 12
echo "menu+choice: $(uv run --with numpy python3 $D/run/segrms.py run/freedos-audio.wav 0)"
run/shot.sh freedos ttp1 >/dev/null 2>&1
o=$(stat -c %s run/freedos-audio.wav); sleep 12; echo "after choice (Provox + ready): $(uv run --with numpy python3 $D/run/segrms.py run/freedos-audio.wav $o)"
o=$(stat -c %s run/freedos-audio.wav); run/type.sh freedos "say Hello from the talking disk." >/dev/null 2>&1; sleep 8
echo "say command: $(uv run --with numpy python3 $D/run/segrms.py run/freedos-audio.wav $o)"
o=$(stat -c %s run/freedos-audio.wav); run/type.sh freedos "help" >/dev/null 2>&1; sleep 12
echo "help screen via Provox: $(uv run --with numpy python3 $D/run/segrms.py run/freedos-audio.wav $o)"
run/shot.sh freedos ttp2 >/dev/null 2>&1
run/mon.sh freedos "info registers" | grep -E "^EIP|^CS " | cut -c1-80
