#!/usr/bin/env bash
# Boot FreeDOS headless with Sound Blaster 16 emulation.
#   serial  -> run/freedos-serial.sock + run/freedos-serial.log   (guest: CTTY COM1 to drive it)
#   COM2    -> run/freedos-com2.sock + run/freedos-com2.log     (a serial "synthesizer" the screen reader talks to)
#   monitor -> run/freedos-mon.sock   (sendkey, screendump)      VNC :7
#   audio   -> run/freedos-audio.wav  (SB16 DSP output; the "did it speak" oracle)
#   D:      -> $SHARE (host directory exposed as a FAT drive via QEMU vvfat; avoid writing to it from the guest)
# Env: DISK=path to installed hard-disk image (default: boot the LiveCD), SHARE=dir, CPU=486|pentium,
#      FLOPPY=dir exposed as a vvfat floppy, or FLOPPYIMG=raw 1.44M image; BOOT=a boots from it
set -euo pipefail
source "$(dirname "$0")/qemu-env.sh"
ISO="$ROOT/images/FD14LIVE.iso"
SHARE="${SHARE:-$ROOT/share}"; mkdir -p "$SHARE"
CPU="${CPU:-486}"
AUDIO="${AUDIO:--audiodev wav,id=snd0,path=$ROOT/run/freedos-audio.wav}"
case "$QEMU_ACCEL" in *kvm*) ACCEL="-enable-kvm -cpu $CPU";; *) ACCEL="-accel tcg -cpu $CPU";; esac
FLOPPY_ARGS=(); [ -n "${FLOPPY:-}" ] && FLOPPY_ARGS=(-drive file=fat:floppy:rw:"$FLOPPY",if=floppy,format=raw)
[ -n "${FLOPPYIMG:-}" ] && FLOPPY_ARGS=(-drive file="$FLOPPYIMG",if=floppy,format=raw)
if [ "${BOOT:-}" = a ]; then BOOT_ARGS=(-boot a); else BOOT_ARGS=(); fi
if [ -n "${DISK:-}" ]; then
  BOOT=(-drive file="$DISK",format=qcow2,if=ide,index=0 -boot c)
else
  BOOT=(-cdrom "$ISO" -boot d)
  [ "${BOOT_ARGS[*]:-}" ] && BOOT=(-cdrom "$ISO" "${BOOT_ARGS[@]}")
fi
exec qemu-system-i386 -L "$QEMU_BIOS" $ACCEL -m ${MEM:-64} -machine pc,pcspk-audiodev=snd0 -no-reboot \
  "${BOOT[@]}" "${FLOPPY_ARGS[@]}" \
  -drive file=fat:rw:"$SHARE",format=raw,if=ide,index=1,media=disk \
  $AUDIO -device sb16,iobase=0x220,irq=5,dma=1,dma16=5,audiodev=snd0 \
  -chardev socket,id=ser0,path="$ROOT/run/freedos-serial.sock",server=on,wait=off,logfile="$ROOT/run/freedos-serial.log" -serial chardev:ser0 \
  -chardev socket,id=ser1,path="$ROOT/run/freedos-com2.sock",server=on,wait=off,logfile="$ROOT/run/freedos-com2.log" -serial chardev:ser1 \
  -monitor unix:"$ROOT/run/freedos-mon.sock",server,nowait \
  -display none -vnc :7 "$@"
