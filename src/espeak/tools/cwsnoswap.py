# /// script
# requires-python = ">=3.9"
# ///
"""cwsnoswap.py CWSDPMI.EXE OUT.EXE [pagetables]: copy of CWSDPMI with virtual memory disabled
(the swap file name in its CWSPBLK parameter block set to "", what CWSPARAM's "swapfile" prompt
does with two double quotes).  ESPKD's handlers run from interrupt context and lock all their
memory; a host that can never page out means that a memory shortage fails at start-up instead
of as a page fault inside a real-mode callback.  The optional third argument is CWSPARAM's
"number of page tables to initially allocate" (0 = auto, which reserves enough for 20 MB: 32 KB
of DOS memory booked to ESPKD's PSP; 2 covers 8 MB and costs 20 KB, CWSDPMI allocates more at
run time if a nested DPMI program needs them).  See CONTROL.H in the CWSDPMI source."""
import sys, struct
src, dst = sys.argv[1], sys.argv[2]
pagetables = int(sys.argv[3]) if len(sys.argv) > 3 else None
d = bytearray(open(src, "rb").read())
i = d.find(b"CWSPBLK\0")
if i < 0: sys.exit("no CWSPBLK parameter block in " + src)
name = d[i + 8:i + 56].split(b"\0")[0].decode()
d[i + 8:i + 56] = b"\0" * 48
msg = f"{dst}: swap file '{name}' -> disabled (no paging)"
if pagetables is not None:
    old = struct.unpack_from("<H", d, i + 58)[0]      # CWSDPMI_pblk.pagedir, after magic[8], swapname[48], flags
    struct.pack_into("<H", d, i + 58, pagetables)
    msg += f", page tables {old} -> {pagetables}"
open(dst, "wb").write(d)
print(msg)
