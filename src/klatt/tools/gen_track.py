# /// script
# requires-python = ">=3.9"
# ///
"""Write a synthetic klatt 3.04 parameter track (40 columns, 10 ms frames):
a vowel sequence with an F0 glide, a nasal, a fricative and a stop burst.
usage: gen_track.py out.par [sr]
"""
import sys
sr = int(sys.argv[2]) if len(sys.argv) > 2 else 8000
F4, B4 = (3300, 250) if sr <= 8000 else (3500, 300)
F5, B5 = 4700, 200
F6, B6 = 4990, 500
def frame(f0=0, av=0, f1=500, b1=60, f2=1500, b2=90, f3=2500, b3=150,
          fnz=270, bnz=100, fnp=270, bnp=100, asp=0, kopen=None, aturb=0, tlt=0,
          af=0, kskew=0, a1=0, a2=0, a3=0, a4=0, a5=0, a6=0, anp=0, ab=0, avp=0, g0=60):
    if kopen is None:
        kopen = int(0.4 * sr / (f0 / 10)) if f0 > 0 else 30
        kopen = max(10, min(kopen, 65))
    return [f0, av, f1, b1, f2, b2, f3, b3, F4, B4, F5, B5, F6, B6, fnz, bnz, fnp, bnp,
            asp, kopen, aturb, tlt, af, kskew, a1, b1, a2, b2, a3, b3, a4, B4, a5, B5, a6, B6, anp, ab, avp, g0]
rows = []
vowels = [(730, 1090, 2440), (270, 2290, 3010), (300, 870, 2240), (530, 1840, 2480), (570, 840, 2410)]
f0 = 1300
for i in range(10): rows.append(frame())
for (f1, f2, f3) in vowels:
    for t in range(30):
        rows.append(frame(f0=f0, av=60, f1=f1, f2=f2, f3=f3))
        f0 -= 3
# nasal: pole/zero apart
for t in range(20):
    rows.append(frame(f0=f0, av=55, f1=280, b1=60, f2=1000, b2=150, f3=2300, fnz=450, bnz=100, fnp=270, bnp=100))
# voiced fricative with tilt + breathiness
for t in range(20):
    rows.append(frame(f0=f0, av=50, f1=280, f2=1600, f3=2560, af=45, a2=25, a3=20, a4=20, a5=15, a6=10, ab=20, avp=45, a1=40, tlt=6, aturb=30))
# unvoiced fricative /s/
for t in range(25):
    rows.append(frame(f0=0, av=0, f1=400, b1=200, f2=1720, b2=100, f3=2620, b3=220, af=60, a2=26, a3=18, a4=26, a5=30, a6=30))
# stop burst + aspiration + vowel
for t in range(3):
    rows.append(frame(f0=0, af=60, a2=45, a3=33, a4=25))
for t in range(6):
    rows.append(frame(f0=0, asp=40, f1=500, f2=1500, f3=2500))
for t in range(30):
    rows.append(frame(f0=f0, av=60, f1=660, f2=1720, f3=2410, kskew=4))
    f0 -= 2
for i in range(10): rows.append(frame())
with open(sys.argv[1], "w") as f:
    for r in rows:
        f.write(" ".join(str(int(v)) for v in r) + "\n")
print("frames", len(rows), "->", sys.argv[1])
