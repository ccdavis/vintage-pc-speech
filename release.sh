#!/usr/bin/env bash
# Build all four artifacts into dist/ and (with --publish) create a GitHub release with them.
# Needs: toolchains (run/get-toolchains.sh), FreeDOS images (run/get-freedos.sh), QEMU with KVM, DOSBox-X, zip, python3, uv.
set -e
D="$(cd "$(dirname "$0")" && pwd)"; cd "$D"
export TOOLS="${TOOLS:-$D/../tools}"; export WATCOM="$TOOLS/ow"; export DJ="$TOOLS/djgpp/bin"
run/mkdist.sh
run/mktalkdisk.sh
run/mksrcdist.sh
ls -la dist/a11y386.zip dist/espk386.zip dist/talkdisk.img dist/a11y-src.zip
if [ "${1:-}" = "--publish" ]; then
  tag="${2:-v$(date +%Y.%m.%d)}"
  gh release create "$tag" dist/a11y386.zip dist/espk386.zip dist/talkdisk.img dist/a11y-src.zip \
    --title "Talking disk $tag" --notes "Built from $(git rev-parse --short HEAD). See dist/README.TXT for how to run the test floppies and the talking disk."
fi
