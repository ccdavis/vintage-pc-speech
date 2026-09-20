#!/usr/bin/env bash
# Type a string into the guest via the QEMU monitor's sendkey (ends with Enter unless -n).
# usage: run/type.sh freedos "dir c:"        run/type.sh -n freedos "y"
D="$(cd "$(dirname "$0")" && pwd)"; NL=1; [ "$1" = "-n" ] && { NL=0; shift; }
VM=$1; STR=$2; S="$D/$VM-mon.sock"
key() { printf 'sendkey %s\n' "$1"; }
{
for ((i=0; i<${#STR}; i++)); do
  c="${STR:$i:1}"
  case "$c" in
    [a-z0-9]) key "$c";; [A-Z]) key "shift-$(tr A-Z a-z <<<"$c")";;
    ' ') key spc;; '-') key minus;; '=') key equal;; '.') key dot;; ',') key comma;;
    '/') key slash;; '\\') key backslash;; ':') key shift-semicolon;; ';') key semicolon;;
    "'") key apostrophe;; '"') key shift-apostrophe;; '_') key shift-minus;; '+') key shift-equal;;
    '*') key shift-8;; '?') key shift-slash;; '!') key shift-1;; '@') key shift-2;; '#') key shift-3;;
    '$') key shift-4;; '%') key shift-5;; '^') key shift-6;; '&') key shift-7;; '(') key shift-9;;
    ')') key shift-0;; '<') key shift-comma;; '>') key shift-dot;; '|') key shift-backslash;;
    '[') key bracket_left;; ']') key bracket_right;; '{') key shift-bracket_left;; '}') key shift-bracket_right;;
    '~') key shift-grave_accent;; '`') key grave_accent;;
    *) echo "type.sh: unmapped char '$c'" >&2;;
  esac
done
[ $NL = 1 ] && key ret
} | while read -r line; do printf '%s\n' "$line" | timeout 5 socat - UNIX-CONNECT:"$S" >/dev/null 2>&1; sleep 0.05; done
