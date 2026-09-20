# /// script
# requires-python = ">=3.9"
# dependencies = ["numpy", "matplotlib", "scipy"]
# ///
"""Plot waveform + spectrogram of one or more WAV files: spectro.py out.png a.wav [b.wav ...]"""
import sys, numpy as np, matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
from scipy.io import wavfile
from scipy.signal import spectrogram
files = sys.argv[2:]
fig, axes = plt.subplots(2 * len(files), 1, figsize=(16, 4 * len(files)))
for i, f in enumerate(files):
    sr, x = wavfile.read(f)
    if x.ndim > 1: x = x[:, 0]
    if x.dtype == np.uint8: x = (x.astype(np.float32) - 128) * 256
    x = x.astype(np.float32)
    t = np.arange(len(x)) / sr
    ax = axes[2 * i]; ax.plot(t, x, lw=0.3); ax.set_title(f"{f}  sr={sr} peak={np.abs(x).max():.0f} rms={np.sqrt((x**2).mean()):.0f}"); ax.set_xlim(0, t[-1])
    F, T, Sx = spectrogram(x, sr, nperseg=256, noverlap=192)
    ax = axes[2 * i + 1]; ax.pcolormesh(T, F, 10 * np.log10(Sx + 1e-3), shading="auto", cmap="magma", vmin=-20, vmax=80); ax.set_ylabel("Hz"); ax.set_xlim(0, t[-1])
plt.tight_layout(); plt.savefig(sys.argv[1], dpi=70)
print("wrote", sys.argv[1])
