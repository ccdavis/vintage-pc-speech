#!/usr/bin/env bash
# A private QEMU FreeDOS instance for the ESPKD tests (same layout as ../../../run/freedos.sh but
# with its own sockets, so it can run beside the shared "freedos" VM):
#   VM name "espkd": run/espkd-serial.sock/.log (console, CTTY COM1), run/espkd-mon.sock,
#   run/espkd-com2.log, audio capture run/espkd-audio.wav, VNC :8, 8 MB RAM, share/ as a FAT drive.
# usage: src/espeak/run/qemu-espkd.sh          # (re)boots and waits for the prompt (argv[0] is "qemu-espkd" so
#        run/boot.sh's pkill of ^qemu-system-i386 for the shared VM does not kill it)
#        run/serial.sh espkd "dir c:\espk"     # then drive it with the shared scripts
ROOT="$(cd "$(dirname "$0")/../../.." && pwd)"
source "$ROOT/run/qemu-env.sh"
R="$ROOT/run"; VM=espkd
pkill -f "path=$R/$VM-serial.sock" 2>/dev/null; sleep 1
rm -f "$R/$VM-serial.log" "$R/$VM-audio.wav"
# FLOPPYIMG=path boots that raw floppy image (src/espeak/run/mkbootimg.py) instead of the LiveCD
if [ -n "${FLOPPYIMG:-}" ]; then MEDIA=(-drive file="$FLOPPYIMG",if=floppy,format=raw -boot a); else MEDIA=(-cdrom "$ROOT/images/FD14LIVE.iso" -boot d); fi
case "$QEMU_ACCEL" in *kvm*) ACCEL="-enable-kvm -cpu 486";; *) ACCEL="-accel tcg -cpu 486";; esac
nohup bash -c 'exec -a qemu-espkd "$(command -v qemu-system-i386)" "$@"' _ -L "$QEMU_BIOS" $ACCEL -m ${MEM:-8} -machine pc,pcspk-audiodev=snd0 -no-reboot \
  "${MEDIA[@]}" \
  -drive file=fat:rw:"$ROOT/share",format=raw,if=ide,index=1,media=disk \
  -audiodev wav,id=snd0,path="$R/$VM-audio.wav" -device sb16,iobase=0x220,irq=5,dma=1,dma16=5,audiodev=snd0 \
  -chardev socket,id=ser0,path="$R/$VM-serial.sock",server=on,wait=off,logfile="$R/$VM-serial.log" -serial chardev:ser0 \
  -chardev socket,id=ser1,path="$R/$VM-com2.sock",server=on,wait=off,logfile="$R/$VM-com2.log" -serial chardev:ser1 \
  -monitor unix:"$R/$VM-mon.sock",server,nowait -display none -vnc :8 > "$R/$VM-qemu.log" 2>&1 &
sleep 8; "$R/mon.sh" $VM "sendkey ret" >/dev/null
for t in $(seq 1 30); do
  sleep 3; "$R/type.sh" $VM "ctty com1"; sleep 1
  out=$("$R/serial.sh" $VM "ver" 2)
  if grep -q FreeCom <<<"$out"; then echo "$VM ready after ~$((8+t*4))s"; exit 0; fi
done
echo "$VM did not come up"; cat "$R/$VM-qemu.log"; exit 1
