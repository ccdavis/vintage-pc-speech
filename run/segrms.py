import sys, numpy as np
p, off = sys.argv[1], int(sys.argv[2]); d = np.fromfile(p, dtype='<i2', offset=max(off, 44))[::2].astype(float)
print("new audio %.1f s, rms %d, peak %d" % (len(d) / 44100, np.sqrt((d ** 2).mean()) if len(d) else 0, np.abs(d).max() if len(d) else 0))
