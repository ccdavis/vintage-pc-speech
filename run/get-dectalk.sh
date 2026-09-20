#!/usr/bin/env bash
# Fetch the DECtalk source tree that src/dectalk builds against (git-ignored, Fonix proprietary licence:
# it is not part of this repository) and apply our patches.  usage: run/get-dectalk.sh
set -e
D="$(cd "$(dirname "$0")/.." && pwd)"; U="$D/src/dectalk-upstream/dectalk"; COMMIT=69ebb459137a7a8d92ed41da8362233eaa173efc
if [ ! -d "$U/.git" ]; then mkdir -p "$D/src/dectalk-upstream"; git clone https://github.com/dectalk/dectalk.git "$U"; fi
git -C "$U" fetch -q origin "$COMMIT" 2>/dev/null || true
git -C "$U" checkout -q "$COMMIT"
git -C "$U" apply --check "$D/src/dectalk/patches/upstream.diff" 2>/dev/null && git -C "$U" apply "$D/src/dectalk/patches/upstream.diff" && echo "patches applied" || echo "patches already applied (or do not apply cleanly: check git -C $U status)"
echo "DECtalk upstream at $COMMIT in $U; now: make -C src/dectalk dos resident"
