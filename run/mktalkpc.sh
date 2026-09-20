#!/usr/bin/env bash
# Build dist/talkpc.img: a bootable 1.44 MB FreeDOS floppy for 2005-2010 PCs (VDPMI + SBEMU + DTALKD + Provox).
# FreeDOS itself (LiveCD in QEMU) trims the installer floppy and copies share/talkpc/*, driven by share/MKTALKPC.BAT.
set -u
D="$(cd "$(dirname "$0")/.." && pwd)"; cd "$D"
SRC="${TALKPC_DIR:-talkpc}"; sed "s/c:\\\\talkpc\\\\/c:\\\\$SRC\\\\/" run/mktalkpc.bat > share/MKTALKPC.BAT
cp images/FD14BOOT.img run/talkpc.img
killq() { for p in $(ps -eo pid,args | grep "[q]emu-system-i386" | awk '{print $1}'); do kill -9 "$p" 2>/dev/null; done; }
killq; sleep 1
echo "booting the LiveCD with the floppy image attached..."
FLOPPYIMG="$D/run/talkpc.img" timeout 150 run/boot.sh >/dev/null || { echo "boot failed"; killq; exit 1; }
echo "running MKTALKPC.BAT in the guest..."
out=$(timeout 60 run/serial.sh freedos "c:\\mktalkpc" 35)
grep -q "MKTALKPC DONE" <<<"$out" || { echo "MKTALKPC did not finish:"; echo "$out" | tail -8; killq; exit 1; }
grep "bytes free" <<<"$out" | tail -1
killq; sleep 1
mkdir -p dist && cp run/talkpc.img dist/talkpc.img && ls -la dist/talkpc.img
rm -rf dist/talkpc && run/imgextract.py dist/talkpc.img dist/talkpc && cp share/talkpc/COPYING.TXT dist/talkpc/   # the same files as a plain directory
echo "boot test: run/testtalkpc.sh   (boots a copy: the first boot writes VOICE.BAT into the image it runs from)"
