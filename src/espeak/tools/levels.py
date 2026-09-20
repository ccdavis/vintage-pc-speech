# /// script
# requires-python = ">=3.9"
# dependencies = ["numpy"]
# ///
"""Compare levels of two WAVs (any rate/bits): overall RMS, RMS of the loudest
20 % of 20 ms windows (vowels), 99th percentile of |x|, in dBFS (16-bit scale).
usage: levels.py a.wav b.wav"""
import sys, wave, numpy as np
def load(p):
    w = wave.open(p); n = w.getnframes(); sw = w.getsampwidth(); sr = w.getframerate()
    raw = w.readframes(n)
    if sw == 1: x = (np.frombuffer(raw, np.uint8).astype(np.float64) - 128) * 256
    else: x = np.frombuffer(raw, np.int16).astype(np.float64)
    return x, sr
def db(v): return 20 * np.log10(max(v, 1e-9) / 32768)
for p in sys.argv[1:]:
    x, sr = load(p)
    win = sr // 50
    nw = len(x) // win
    r = np.sqrt((x[:nw*win].reshape(nw, win) ** 2).mean(axis=1))
    top = np.sort(r)[-max(1, nw // 5):]
    print("%-40s %6.2f s  rms %6.1f dB  loud20 %6.1f dB  p99 %6.1f dB  max %6.1f dB" % (
        p.split('/')[-1], len(x) / sr, db(np.sqrt((x**2).mean())), db(top.mean()), db(np.percentile(np.abs(x), 99)), db(np.abs(x).max())))
