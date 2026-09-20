#!/usr/bin/env bash
# Fetch FreeDOS 1.4 (LiveCD ISO for the QEMU harness, boot floppy image as the talking disk's base) into images/.
set -e
D="$(cd "$(dirname "$0")/.." && pwd)"; mkdir -p "$D/images"; cd "$D/images"
[ -f FD14LIVE.iso ] && [ -f FD14BOOT.img ] && { echo "already present"; exit 0; }
curl -fsSL -o FD14-LiveCD.zip https://www.ibiblio.org/pub/micro/pc-stuff/freedos/files/distributions/1.4/FD14-LiveCD.zip
echo "2020ff6bb681967fd6eff8f51ad2e5cd5ab4421165948cef4246e4f7fcaf6339  FD14-LiveCD.zip" | sha256sum -c -
unzip -o -q FD14-LiveCD.zip FD14LIVE.iso FD14BOOT.img && ls -la FD14LIVE.iso FD14BOOT.img
