#!/usr/bin/env python3
"""Extract every root-directory file of a FAT12 floppy image into a directory (no mtools needed).
usage: run/imgextract.py image.img outdir"""
import struct, os, sys
img = open(sys.argv[1], 'rb').read(); out = sys.argv[2]; os.makedirs(out, exist_ok=True)
bps, spc, rs, nf, re, ts = struct.unpack_from('<HBHBHH', img, 11); spf = struct.unpack_from('<H', img, 22)[0]
fat = img[rs * bps:(rs + spf) * bps]; root = (rs + nf * spf) * bps; data = root + re * 32
def nxt(c):
    o = c * 3 // 2; v = struct.unpack_from('<H', fat, o)[0]; return (v >> 4) if c & 1 else (v & 0xFFF)
n = 0
for i in range(re):
    e = img[root + i * 32:root + i * 32 + 32]
    if e[0] == 0: break
    if e[0] == 0xE5 or e[11] & 0x18: continue
    name = e[:8].decode().strip(); ext = e[8:11].decode().strip(); name = name + ('.' + ext if ext else '')
    size = struct.unpack_from('<I', e, 28)[0]; c = struct.unpack_from('<H', e, 26)[0]; buf = b''
    while 2 <= c < 0xFF8 and len(buf) < size: buf += img[data + (c - 2) * spc * bps:data + (c - 1) * spc * bps]; c = nxt(c)
    open(os.path.join(out, name), 'wb').write(buf[:size]); n += 1
print(n, "files extracted to", out)
