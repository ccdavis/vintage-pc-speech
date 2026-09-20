#!/usr/bin/env bash
# Send a line to a VM's serial console (paced, one char at a time: DOS polls the UART without a FIFO
# and drops bursts) and print the new serial output after a short wait.
# usage: run/serial.sh freedos "dir c:" [wait_seconds]
D="$(cd "$(dirname "$0")" && pwd)"; VM=$1; CMD=$2; W=${3:-2}
LOG="$D/$VM-serial.log"; before=$(stat -c %s "$LOG" 2>/dev/null || echo 0)
( for ((i=0; i<${#CMD}; i++)); do printf '%s' "${CMD:$i:1}"; sleep 0.02; done; printf '\r'; sleep "$W" ) \
  | timeout $((W+10)) socat - UNIX-CONNECT:"$D/$VM-serial.sock" >/dev/null 2>&1
tail -c +$((before+1)) "$LOG" | sed -e 's/\r//g' | grep -v '^\s*$'
