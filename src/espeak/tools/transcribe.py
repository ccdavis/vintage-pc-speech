# /// script
# dependencies = ["faster-whisper"]
# ///
import sys
from faster_whisper import WhisperModel
m = WhisperModel("base.en", device="cpu", compute_type="int8")
segs, info = m.transcribe(sys.argv[1], beam_size=5)
for s in segs: print(f"[{s.start:6.2f}-{s.end:6.2f}] {s.text}")
