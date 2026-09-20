#!/usr/bin/env bash
# Send a monitor command: run/mon.sh redox "screendump /path.ppm"
S="$(dirname "$0")/$1-mon.sock"; shift
printf '%s\n' "$*" | timeout 5 socat - UNIX-CONNECT:"$S" 2>/dev/null || { printf '%s\n' "$*" | timeout 5 nc -U "$S"; }
