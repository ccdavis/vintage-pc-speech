#!/usr/bin/env bash
# Build dist/talkdisk.img: a bootable 1.44 MB FreeDOS floppy that talks (SBTALK + Provox from FDAUTO.BAT).
# FreeDOS itself (LiveCD in QEMU) trims the installer floppy and copies our files, driven by share/MKTALK.BAT.
set -u
D="$(cd "$(dirname "$0")/.." && pwd)"; cd "$D"
STRIP="${TOOLS:-$D/../tools}/djgpp/bin/i586-pc-msdosdjgpp-strip"; [ -f share/espk/ESPK.EXE ] && "$STRIP" share/espk/ESPK.EXE 2>/dev/null
make -s -C src/sbtalk install >/dev/null || { echo "sbtalk build failed"; exit 1; }
make -s -C src/sbsynth install >/dev/null
cp run/mktalk.bat share/MKTALK.BAT
cp images/FD14BOOT.img run/talkdisk.img
killq() { for p in $(ps -eo pid,args | grep "[q]emu-system-i386" | awk '{print $1}'); do kill -9 "$p" 2>/dev/null; done; }
killq; sleep 1
echo "booting the LiveCD with the floppy image attached..."
FLOPPYIMG="$D/run/talkdisk.img" timeout 150 run/boot.sh >/dev/null || { echo "boot failed"; killq; exit 1; }
echo "running MKTALK.BAT in the guest..."
out=$(timeout 60 run/serial.sh freedos "c:\\mktalk" 35)
grep -q "MKTALK DONE" <<<"$out" || { echo "MKTALK did not finish:"; echo "$out" | tail -8; killq; exit 1; }
grep "bytes free" <<<"$out" | tail -1
killq; sleep 1
mkdir -p dist && cp run/talkdisk.img dist/talkdisk.img && ls -la dist/talkdisk.img
echo "boot test: BOOT=a FLOPPYIMG=\$PWD/dist/talkdisk.img run/freedos.sh"
