/* Audio stub for the host build of upstream rsynth: no device, just the
   -r sample-rate option (the real drivers live in upstream/rsynth/config). */
#include <config.h>
#include <stdio.h>
#include "getargs.h"
#include "hplay.h"

long samp_rate = 8000;

int audio_init(int argc, char **argv)
{
    int rate = 0;
    argc = getargs("Sound driver", argc, argv, "r", "%d", &rate, "Sample rate", NULL);
    if (rate > 0)
        samp_rate = rate;
    return argc;
}
void audio_term(void) {}
void audio_play(int n, short *data) { (void)n; (void)data; }
