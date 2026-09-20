#!/usr/bin/env bash
# Screenshot the guest: run/shot.sh freedos [name]  -> run/freedos-<name>.png (and prints OCR-free text via VGA text dump if available)
D="$(cd "$(dirname "$0")" && pwd)"; VM=$1; N=${2:-shot}
"$D/mon.sh" "$VM" "screendump $D/$VM-$N.ppm" >/dev/null; sleep 0.5
python3 -c "
from PIL import Image; import sys; Image.open('$D/$VM-$N.ppm').save('$D/$VM-$N.png')" 2>/dev/null || ffmpeg -loglevel error -y -i "$D/$VM-$N.ppm" "$D/$VM-$N.png"
echo "$D/$VM-$N.png"
