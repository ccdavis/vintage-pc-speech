#!/usr/bin/env python3
"""Derive sbtalk/sam/*.c from ../sam-upstream/src: same engine, but output streams through ring_write()
instead of a 220 KB malloc'd buffer, bufferpos is 32-bit, debug printing is removed, the text is not copied, the reciter rules and the sample
table are far data on 16-bit Watcom, and one upstream out-of-bounds read (AdjustLengths) is fixed."""
import re, shutil, pathlib
src = pathlib.Path('../sam-upstream/src'); dst = pathlib.Path('sam')
def sub(s, old, new, count=1):
    assert old in s, old
    return s.replace(old, new, count)
# --- render.c
s = (src/'render.c').read_text()
s = sub(s, 'extern int bufferpos;', 'extern long bufferpos;\nextern void ring_write5(unsigned long pos, const unsigned char *ary5);')
s = sub(s, '''    bufferpos += timetable[oldtimetableindex][index];
    oldtimetableindex = index;
    // write a little bit in advance
    for(k=0; k<5; k++)
        buffer[bufferpos/50 + k] = ary[k];''',
'''    bufferpos += timetable[oldtimetableindex][index];
    oldtimetableindex = index;
    (void)k;
    ring_write5((unsigned long)bufferpos / 50, ary);   /* 5 samples, written a little in advance */''')
s = sub(s, '''if (debug)
{
    PrintOutput(sampledConsonantFlag, frequency1, frequency2, frequency3, amplitude1, amplitude2, amplitude3, pitches);
}''', '')
# dead code after the render loop references buffer[]; drop it (it is unreachable in upstream too)
i = s.index('    // The following code is never reached.'); j = s.index('// Create a rising or falling inflection')
s = s[:i] + '}\n\n\n' + s[j:]
s = sub(s, 'extern unsigned char *buffer;', '') if 'extern unsigned char *buffer;' in s else s
s = re.sub(r'extern char \*buffer;\n', '', s)
s = re.sub(r'\n[ \t]*printf\("Error reading to tables"\);', '\n', s)
# Watcom C: no non-constant aggregate initializers, no label right after a mid-block declaration
s = sub(s, '    unsigned char ary[5] = {A,A,A,A,A};', '    unsigned char ary[5]; ary[0] = ary[1] = ary[2] = ary[3] = ary[4] = A;')
s = sub(s, 'void RenderSample(unsigned char *mem66)\n{\n    int tempA;', 'void RenderSample(unsigned char *mem66)\n{\n    int tempA; unsigned char phase1;')
s = sub(s, '\n\n    unsigned char phase1;\n\npos48315:', '\n\npos48315:')
# 16-bit int: phase1*256 and frequency*256 overflow a 16-bit int; keep the math unsigned
for f in ('1','2','3'):
    s = sub(s, 'unsigned int p%s = phase%s * 256;' % (f,f), 'unsigned int p%s = (unsigned)phase%s * 256u;' % (f,f))
    s = sub(s, 'p%s += frequency%s[Y] * 256 / 4;' % (f,f), 'p%s += (unsigned)frequency%s[Y] * 64u;' % (f,f))
(dst/'render.c').write_text(s)
# --- sam.c
s = (src/'sam.c').read_text()
s = sub(s, '#include <stdio.h>\n#include <string.h>\n#include <stdlib.h>\n#include "debug.h"', '#include <string.h>\n#include "debug.h"')
s = sub(s, 'int bufferpos=0;\nchar *buffer = NULL;', 'long bufferpos=0;')
# no private copy of the text: the caller's buffer (engine_sam.c utt[256], NUL-terminated after the 155 end marker)
# is the reciter's output and stays valid through SAMMain(); saves 256 bytes of BSS
s = sub(s, 'char input[256]; //tab39445', 'char *input;   /* the caller\'s buffer (>= 256 bytes, reciter output, 155-terminated) */')
s = sub(s, '''    int i, l;
    l = strlen(_input);
    if (l > 254) l = 254;
    for(i=0; i<l; i++)
        input[i] = _input[i];
    input[l] = 0;''', '    input = _input;')
# AdjustLengths: after 'mem56 = 65 (end marker) else flags[index]' the next test read flags[255], one byte past the
# 81-entry table (upstream bug, found with ASan); use the value just computed
s = sub(s, '''            mem56 = flags[index];

            // not a consonant
            if ((flags[index] & 64) == 0)''', '''            mem56 = flags[index];

            // not a consonant
            if ((mem56 & 64) == 0)   /* was flags[index]: reads past the table when index == 255 */''')
s = sub(s, 'char* GetBuffer(){return buffer;}\nint GetBufferLength(){return bufferpos;}', 'long GetBufferLength(){return bufferpos;}')
s = re.sub(r'\n\s*// TODO, check for free the memory[^\n]*\n\s*buffer = malloc\(22050\*10\);', '', s)
s = re.sub(r'\n[ \t]*if \(debug\)\s*\n\s*\{[^}]*\}', '\n', s)
s = re.sub(r'\n[ \t]*if \(debug\)\s*\n[ \t]*Print[^\n]*;', '\n', s)
s = re.sub(r'\n\s*if \(debug\) [^\n]*;', '\n', s)                       # one-line debug prints
s = re.sub(r'\n\s*if\(debug\) [^\n]*;', '\n', s)
s = s.replace('#include <stdio.h>', '')
s = re.sub(r'\n[ \t]*if \(debug && A != 255\) printf[^\n]*;', '\n', s)
# PrepareOutput: the 60-entry output lists overflow when more than 59 phonemes fit in InsertBreath's 232-frame
# chunk (runs of short stops, e.g. "t t t t ..."), writing past phonemeIndexOutput[] (upstream bug, found with
# ASan); render the chunk when the list is full, the way a 254 breath marker does
s = sub(s, '''        phonemeIndexOutput[Y] = A;
        phonemeLengthOutput[Y] = phonemeLength[X];
        stressOutput[Y] = stress[X];
        X++;
        Y++;''', '''        phonemeIndexOutput[Y] = A;
        phonemeLengthOutput[Y] = phonemeLength[X];
        stressOutput[Y] = stress[X];
        X++;
        Y++;
        if (Y == 59)   /* output lists full (60 entries): render this chunk now instead of overrunning them */
        {
            int temp = X;
            phonemeIndexOutput[Y] = 255;
            Render();
            X = temp;
            Y = 0;
        }''')
# Insert(): shifting the list up drops entry 254; when that was the 255 end marker the list is unterminated and
# InsertBreath()/Code41240() loop forever (about 200 characters of stop-heavy text: T, P, B become 3 entries each
# and breaths are added; found by fuzzing).  Keep the marker: the last phoneme is dropped instead.
s = sub(s, '''    int i;
    for(i=253; i >= position; i--) // ML : always keep last safe-guarding 255
    {
        phonemeindex[i+1] = phonemeindex[i];
        phonemeLength[i+1] = phonemeLength[i];
        stress[i+1] = stress[i];
    }''', '''    int i;
    unsigned char last = phonemeindex[254];
    for(i=253; i >= position; i--) // ML : always keep last safe-guarding 255
    {
        phonemeindex[i+1] = phonemeindex[i];
        phonemeLength[i+1] = phonemeLength[i];
        stress[i+1] = stress[i];
    }
    if (last == 255) phonemeindex[254] = 255;   /* list full: keep the end marker, lose the last phoneme */''')
# Loop termination on a full list (fuzzing found three more infinite loops, all upstream): keep entry 255 a
# permanent end marker (upstream set it to 32), never let Insert() write past 254, and stop the walks in
# Code41240 / InsertBreath / Parser2 instead of letting their 8-bit positions wrap around to 0.
s = sub(s, "    phonemeindex[255] = 32; //to prevent buffer overflow", "    phonemeindex[255] = 255; /* permanent end marker: every 8-bit walk stops here at the latest (upstream: 32) */")
s = sub(s, '''    int i;
    unsigned char last = phonemeindex[254];''', '''    int i;
    unsigned char last = phonemeindex[254];
    if (position > 254) return;                  /* list full: entry 255 stays the end marker */''')
s = sub(s, "    unsigned char pos=0;\n\n    while(phonemeindex[pos] != 255)", "    unsigned pos=0;                                /* not 8-bit: pos += 3 must not wrap to 0 */\n\n    while(pos < 255 && phonemeindex[pos] != 255)")
s = sub(s, "    unsigned char mem66 = 0;\n    while(1)\n    {\n        //pos48440:\n        X = mem66;", "    unsigned mem66 = 0;                            /* not 8-bit: must not wrap to 0 */\n    while(1)\n    {\n        //pos48440:\n        if (mem66 > 254) return;\n        X = mem66;")
s = sub(s, '''        X = mem54;
        phonemeindex[X] = 31;   // 'Q*' glottal stop''', '''        if (mem54 == 255) mem54 = X;               /* no word boundary in 232 frames: break here (upstream wrote entry 255) */
        X = mem54;
        phonemeindex[X] = 31;   // 'Q*' glottal stop''')
s = sub(s, '''        Insert(X, 254, mem59, 0);
        X++;
        mem66 = X;
    }''', '''        Insert(X, 254, mem59, 0);
        if (X == 255) return;
        X++;
        mem66 = X;
    }''')
# (Parser2 only ever does pos++, so its 8-bit pos stops at the permanent marker in entry 255, and its
#  phonemeindex[pos-1] look-backs at pos 0 read that marker, whose padded flags are 0)
# AdjustLengths, <VOWEL> RX|LX <CONSONANT> rule: the phoneme after RX/LX may be the end marker (255), one past flags[]
s = sub(s, "                    if ((flags[index] & 64) != 0) {\n                        // RULE: <VOWEL> RX | LX <CONSONANT>",
           "                    if (index != 255 && (flags[index] & 64) != 0) {   /* end marker: not a consonant */\n                        // RULE: <VOWEL> RX | LX <CONSONANT>")
# Parser1: a stress digit before the first phoneme wrote stress[-1]
s = sub(s, "        stress[position-1] = Y;", "        if (position) stress[position-1] = Y;   /* a stress digit before the first phoneme wrote stress[-1] */")
# AdjustLengths, <LIQUID> <DIPHTHONG> rule: a liquid as the very first phoneme read phonemeindex[-1]
s = sub(s, "            index = phonemeindex[X-1];\n", "            index = X ? phonemeindex[X-1] : 255;   /* no prior phoneme (read phonemeindex[-1] upstream) */\n")
# Parser2 / AdjustLengths look-backs: phonemeindex[pos-1] and [X-1] are int -1 at position 0 (8-bit pos does not
# wrap in "pos-1"); read entry 255, the permanent end marker, instead (its padded flags are 0 = no feature)
s = s.replace('phonemeindex[pos-1]', 'phonemeindex[(unsigned char)(pos-1)]').replace('phonemeindex[X-1]', 'phonemeindex[(unsigned char)(X-1)]')
(dst/'sam.c').write_text(s)
# --- reciter.c
s = (src/'reciter.c').read_text()
s = s.replace('#include <stdio.h>', '')
s = re.sub(r'\n[ \t]*if \(debug\)\s*\n\s*\{[^}]*\}', '\n', s)
s = re.sub(r'\n[ \t]*if \(debug\)\s*\n[ \t]*Print[^\n]*;', '\n', s)
s = re.sub(r'\n\s*if ?\(debug\) [^\n]*;', '\n', s)
s = sub(s, 'unsigned char A, X, Y;', 'extern unsigned char A, X, Y;')
(dst/'reciter.c').write_text(s)
FAR = '/* 16-bit Watcom: this table lives in a far data segment, outside DGROUP */\n#ifdef __WATCOMC__\n#define SAM_FAR __far\n#else\n#define SAM_FAR\n#endif\n'
for h in ('RenderTabs.h', 'SamTabs.h', 'ReciterTabs.h', 'render.h', 'reciter.h'):
    shutil.copy(src/h, dst/h)
s = (dst/'ReciterTabs.h').read_text()
s = sub(s, 'const unsigned char rules[] =', 'const unsigned char SAM_FAR rules[] =')
s = sub(s, 'const unsigned char rules2[] =', 'const unsigned char SAM_FAR rules2[] =')
(dst/'ReciterTabs.h').write_text(FAR + s)
s = (dst/'RenderTabs.h').read_text()
s = sub(s, 'const unsigned char sampleTable[0x500] =', 'const unsigned char SAM_FAR sampleTable[0x500] =')
(dst/'RenderTabs.h').write_text(FAR + s)
# the parsers probe the phoneme after the current one and index these tables with it even when it is the
# 255 end marker (or the 254 breath marker): pad them to 256 entries (zero = no feature, length 0) instead of
# reading whatever follows them in memory (+700 bytes of const data)
s = (dst/'SamTabs.h').read_text()
for t in ('flags[]', 'flags2[]', 'phonemeStressedLengthTable[]', 'phonemeLengthTable[]'):
    s = sub(s, 'const unsigned char ' + t, 'const unsigned char ' + t.replace('[]', '[256]'))
(dst/'SamTabs.h').write_text(s)
(dst/'sam.h').write_text((src/'sam.h').read_text().replace('int GetBufferLength();', 'long GetBufferLength();').replace('char* GetBuffer();\n', ''))
(dst/'debug.h').write_text('/* debug output removed for the resident build */\n')
print('ok')
