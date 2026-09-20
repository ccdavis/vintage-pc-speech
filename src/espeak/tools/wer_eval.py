# /// script
# requires-python = ">=3.9"
# dependencies = ["faster-whisper"]
# ///
"""Intelligibility check: synthesize sentences, transcribe with Whisper
base.en (same settings as the transcribe.py reference script), report word
error rate per configuration.

usage: wer_eval.py [--model base.en] [--stable] sentences.txt CONFIG...
   --stable: temperature 0, no conditioning on previous text (much less run-to-run
             chaos than the transcribe.py defaults; use for A/B comparisons)
   CONFIG is one of
     "opts"                 klattsay option string, e.g. "-r 11025 -n 4" ("" = defaults)
     cmd:NAME:template      shell template with {text} and {wav} placeholders,
                            e.g. "cmd:rsynth8k:tools/rsynth_wav.sh 8000 {text} {wav}"
                            ({textq} = text as a quoted SAMPA string via klattsay -P,
                             {phones} = the phone string itself)
     wav:/path/file.wav     an existing WAV, compared against the first sentence
Transcripts are cached by WAV content hash in $TMPDIR/wer_cache.
"""
import sys, subprocess, re, os, pathlib, tempfile, hashlib, json, shlex
from faster_whisper import WhisperModel

args = sys.argv[1:]
model_name = "base.en"
stable = False
while args and args[0].startswith("--"):
    if args[0] == "--model":
        model_name = args[1]; args = args[2:]
    elif args[0] == "--stable":
        # deterministic decoding: no temperature fallback (which samples and
        # hallucinates on this kind of audio), no conditioning on previous text
        stable = True; args = args[1:]
here = pathlib.Path(__file__).resolve().parent.parent
sents = [l.strip() for l in open(args[0]) if l.strip()]
model = WhisperModel(model_name, device="cpu", compute_type="int8")
tmp = pathlib.Path(tempfile.mkdtemp(prefix="wer_"))
cache_dir = pathlib.Path(tempfile.gettempdir()) / ("wer_cache_" + model_name + ("_stable" if stable else ""))
cache_dir.mkdir(exist_ok=True)

DIGITS = {"0": "zero", "1": "one", "2": "two", "3": "three", "4": "four", "5": "five",
          "6": "six", "7": "seven", "8": "eight", "9": "nine", "10": "ten"}
def norm(t):
    t = t.lower().replace("-", " ")
    t = re.sub(r"[^a-z0-9 ]", "", t)
    return [DIGITS.get(w, w) for w in t.split()]

def wer(ref, hyp):
    r, h = norm(ref), norm(hyp)
    d = [[0] * (len(h) + 1) for _ in range(len(r) + 1)]
    for i in range(len(r) + 1): d[i][0] = i
    for j in range(len(h) + 1): d[0][j] = j
    for i in range(1, len(r) + 1):
        for j in range(1, len(h) + 1):
            d[i][j] = min(d[i-1][j] + 1, d[i][j-1] + 1, d[i-1][j-1] + (r[i-1] != h[j-1]))
    return d[len(r)][len(h)], len(r)

def transcribe(wav):
    h = hashlib.sha1(open(wav, "rb").read()).hexdigest()
    cf = cache_dir / (h + ".txt")
    if cf.exists():
        return cf.read_text()
    w16 = str(wav) + ".16k.wav"
    subprocess.run(["ffmpeg", "-loglevel", "error", "-y", "-i", str(wav), "-ar", "16000", "-ac", "1", w16], check=True)
    if stable:
        segs, _ = model.transcribe(w16, beam_size=5, temperature=0.0, condition_on_previous_text=False)
    else:
        segs, _ = model.transcribe(w16, beam_size=5)
    t = " ".join(s.text.strip() for s in segs)
    cf.write_text(t)
    return t

def phones_of(text):
    p = subprocess.run([str(here / "klattsay"), "-P", text, "/dev/null"], capture_output=True, text=True)
    return " ".join(l.strip("[]") for l in p.stderr.splitlines() if l.startswith("["))

results = {}
for cfg in args[1:]:
    errs = words = 0
    lines = []
    if cfg.startswith("wav:"):
        hyp = transcribe(cfg[4:])
        e, n = wer(sents[0], hyp)
        print("%-40s WER %5.1f%%  (%d/%d)" % (cfg, 100.0 * e / n, e, n))
        print("      %s" % hyp)
        continue
    if cfg.startswith("cmd:"):
        _, name, template = cfg.split(":", 2)
    else:
        name, template = '"' + cfg + '"', None
    for i, s in enumerate(sents):
        wav = tmp / ("%s_s%d.wav" % (re.sub(r'[^A-Za-z0-9]', '_', name), i))
        if template is None:
            subprocess.run([str(here / "klattsay")] + cfg.split() + [s, str(wav)], check=True, stderr=subprocess.DEVNULL)
        else:
            c = template
            if "{phones}" in c or "{textq}" in c:
                ph = phones_of(s)
                c = c.replace("{phones}", shlex.quote(ph)).replace("{textq}", shlex.quote("[" + ph + "]"))
            c = c.replace("{text}", shlex.quote(s)).replace("{wav}", shlex.quote(str(wav)))
            subprocess.run(c, shell=True, check=True, cwd=str(here), stderr=subprocess.DEVNULL)
        hyp = transcribe(wav)
        e, n = wer(s, hyp)
        e = min(e, n)           # cap: a hallucination cascade counts as one fully wrong sentence
        errs += e; words += n
        lines.append("      [%2d/%2d] %s" % (e, n, hyp))
    print("%-40s WER %5.1f%%  (%d/%d)" % (name, 100.0 * errs / words, errs, words), flush=True)
    for l in lines: print(l)
    results[name] = 100.0 * errs / words
print("\nSUMMARY (%s%s)" % (model_name, " stable" if stable else ""))
for k, v in results.items():
    print("  %-40s %5.1f%%" % (k, v))
