/* SAY.EXE: send text to the serial synthesizer through BIOS INT 14h, the way a screen reader does.
 * usage: SAY [/COMn] text...     SAY /X  (flush)     SAY /F file  (speak a text file)     default COM3 */
#include <stdio.h>
#include <string.h>
#include <dos.h>
static unsigned port = 2;
static void out(unsigned char c) { union REGS r; r.h.ah = 1; r.h.al = c; r.x.dx = port; int86(0x14, &r, &r); }
static void outs(const char *s) { while (*s) out((unsigned char)*s++); }
int main(int argc, char **argv)
{
    int i;
    for (i = 1; i < argc; i++) {
        if (strnicmp(argv[i], "/COM", 4) == 0) port = argv[i][4] - '1';
        else if (stricmp(argv[i], "/X") == 0) out(24);
        else if (stricmp(argv[i], "/F") == 0 && i + 1 < argc) {
            FILE *f = fopen(argv[++i], "r"); char line[256];
            if (!f) { printf("cannot open %s\n", argv[i]); return 1; }
            while (fgets(line, sizeof line, f)) outs(line);
            fclose(f); out('\r');
        } else { outs(argv[i]); out(' '); }
    }
    out('\r');
    return 0;
}
