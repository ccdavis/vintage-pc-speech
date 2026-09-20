# usage: uv run wavcheck.py file.wav [byte_offset]  -> duration, peak, loud samples, dominant frequency
import sys, struct, array, math
p = sys.argv[1]; off = int(sys.argv[2]) if len(sys.argv) > 2 else 0
raw = open(p, 'rb').read()
ch = struct.unpack_from('<H', raw, 22)[0]; rate = struct.unpack_from('<I', raw, 24)[0]; bits = struct.unpack_from('<H', raw, 34)[0]
d = raw[max(off, 44):]; a = array.array('h'); a.frombytes(d[:len(d)//2*2]); a = a[::ch]
n = len(a); loud = sum(1 for x in a if abs(x) > 200)
print(f"rate={rate} ch={ch} bits={bits} samples={n} dur={n/rate:.2f}s peak={max(map(abs,a)) if n else 0} loud={loud}")
if n > 4096:
    N = 8192; seg = [x for x in a[n//2 - N//2: n//2 + N//2]]
    best = (0, 0)
    for k in range(1, N//2):
        f = k * rate / N
        if f > 4000: break
        re = sum(seg[i] * math.cos(2*math.pi*k*i/N) for i in range(0, N, 4)); im = sum(seg[i] * math.sin(2*math.pi*k*i/N) for i in range(0, N, 4))
        m = re*re + im*im
        if m > best[0]: best = (m, f)
    print(f"dominant≈{best[1]:.0f} Hz")
