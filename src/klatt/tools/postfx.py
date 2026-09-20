# /// script
# dependencies = ["numpy", "scipy"]
# ///
"""Host-side post-processing experiments on a WAV: pre-emphasis and gain.
usage: postfx.py in.wav out.wav [-p k] [-g dB] [-n] [-h hz] [-s hz,dB]
   -p k: pre-emphasis y=x-k*x[-1]; -h: 1st-order high-pass; -s: high shelf +dB above hz; -n: normalize peak to -1 dBFS"""
import sys, numpy as np
from scipy.io import wavfile
from scipy.signal import lfilter
a = sys.argv; inp, out = a[1], a[2]; k = 0.0; g = 0.0; norm = False; hp = 0.0; shelf = None
i = 3
while i < len(a):
    if a[i] == "-p": k = float(a[i+1]); i += 2
    elif a[i] == "-g": g = float(a[i+1]); i += 2
    elif a[i] == "-n": norm = True; i += 1
    elif a[i] == "-h": hp = float(a[i+1]); i += 2
    elif a[i] == "-s": shelf = [float(v) for v in a[i+1].split(",")]; i += 2
    else: i += 1
sr, x = wavfile.read(inp)
x = ((x.astype(np.float64) - 128) * 256) if x.dtype == np.uint8 else x.astype(np.float64)
if k: x = np.concatenate([[x[0]], x[1:] - k * x[:-1]])
if hp:
    r = np.exp(-2 * np.pi * hp / sr); x = lfilter([1, -1], [1, -r], x)
if shelf:
    fc, db = shelf; r = np.exp(-2 * np.pi * fc / sr); gain = 10 ** (db / 20)
    # x + (gain-1) * highpassed(x)
    x = x + (gain - 1) * lfilter([1, -1], [1, -r], x)
x *= 10 ** (g / 20)
if norm: x *= 32768 * 10 ** (-1 / 20) / max(1, np.abs(x).max())
wavfile.write(out, sr, np.clip(x, -32767, 32767).astype(np.int16))
