# QEMU environment. Prefers the system QEMU; falls back to the no-root unpack under tools/qemu.
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
if command -v qemu-system-x86_64 >/dev/null 2>&1 && [ "$(command -v qemu-system-x86_64)" = /usr/bin/qemu-system-x86_64 ]; then
  export QEMU_BIOS="/usr/share/qemu"
else
  export QEMU_PREFIX="$ROOT/tools/qemu"
  export QEMU_MODULE_DIR="$QEMU_PREFIX/usr/lib/x86_64-linux-gnu/qemu"
  export PATH="$QEMU_PREFIX/usr/bin:$PATH"
  export QEMU_BIOS="$QEMU_PREFIX/usr/share/qemu"
fi
# KVM only if we can open /dev/kvm (fix: sudo usermod -aG kvm $USER, then re-login)
if [ -r /dev/kvm ] && [ -w /dev/kvm ]; then export QEMU_ACCEL="-enable-kvm -cpu host"; else export QEMU_ACCEL="-accel tcg,thread=multi -cpu max"; fi
