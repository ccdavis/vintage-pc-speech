# /// script
# requires-python = ">=3.9"
# ///
"""stubsize.py EXE bytes: set the DJGPP stub's transfer-buffer size (the 'minkeep' field of
the go32stub info block, normally 16 KB of conventional memory) so the resident ESPKD
leaves more DOS memory for the child shell.  Same effect as `stubedit EXE bufsize=4k`."""
import sys, struct
p, size = sys.argv[1], int(sys.argv[2])
d = bytearray(open(p, "rb").read())
i = d.find(b"go32stub, v 2.0")
if i < 0: sys.exit("no go32stub info block in " + p)
off = i + 16 + 4 + 4 + 4 + 4          # magic, size, minstack, memory_handle, initial_size
old = struct.unpack_from("<H", d, off)[0]
struct.pack_into("<H", d, off, size)
open(p, "wb").write(d)
print(f"{p}: transfer buffer {old} -> {size} bytes")
