#!/usr/bin/env bash
# Fetch the cross toolchains into $TOOLS (default: ../tools next to this project): DJGPP (build-djgpp v3.4,
# gcc 12.2, 386 protected mode) and Open Watcom 2.0 (16-bit real mode). Linux x86-64 hosts.
set -e
T="${TOOLS:-$(cd "$(dirname "$0")/.." && pwd)/../tools}"; mkdir -p "$T"; cd "$T"
[ -x djgpp/bin/i586-pc-msdosdjgpp-gcc ] || { curl -fsSL -o djgpp.tar.bz2 https://github.com/andrewwutw/build-djgpp/releases/download/v3.4/djgpp-linux64-gcc1220.tar.bz2 && tar xjf djgpp.tar.bz2 && rm djgpp.tar.bz2; }
[ -x ow/binl64/wcc ] || { curl -fsSL -o ow.tar.xz https://github.com/open-watcom/open-watcom-v2/releases/download/2026-09-01-Build/ow-snapshot.tar.xz && mkdir -p ow && tar xJf ow.tar.xz -C ow && rm ow.tar.xz; }
echo "toolchains in $T"; ls "$T"
