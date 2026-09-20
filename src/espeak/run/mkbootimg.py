# /// script
# requires-python = ">=3.9"
# ///
"""mkbootimg.py talkdisk.img out.img: a copy of the bootable talking-disk floppy whose FDAUTO.BAT
only sets PATH and BLASTER and does CTTY COM1 (no SBTALK, no Provox, no first-boot menu), for the
ESPKD tests in QEMU: booting from it leaves the XMS free (the LiveCD's ramdisk takes nearly all of
it, which makes CWSDPMI page), and share/ is C:.  Pure python: rewrites the file in place in the
FAT12 root directory (the new text is shorter than the old file, so its first cluster is reused)."""
import sys, shutil, struct
src, dst = sys.argv[1], sys.argv[2]
shutil.copyfile(src, dst)
d = bytearray(open(dst, "rb").read())
bps, spc, res, nfat, nroot, _, _, spf = struct.unpack_from("<HBHBHHBH", d, 11)
root = (res + nfat * spf) * bps
data = root + nroot * 32
new = b"@echo off\r\nset PATH=A:\;A:\\FREEDOS\\BIN\r\ncall A:\\BLASTER.BAT\r\nCTTY COM1\r\n"
for e in range(root, data, 32):
    if d[e:e + 11] == b"FDAUTO  BAT":
        cl = struct.unpack_from("<H", d, e + 26)[0]
        off = data + (cl - 2) * spc * bps
        d[off:off + len(new)] = new
        struct.pack_into("<I", d, e + 28, len(new))
        open(dst, "wb").write(d)
        print(f"{dst}: FDAUTO.BAT replaced ({len(new)} bytes at cluster {cl})")
        sys.exit(0)
sys.exit("FDAUTO.BAT not found in the root directory")
