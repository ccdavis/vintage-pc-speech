/* Engine abstraction: SAM (22050 Hz) and, in the 386 build, fixed-point Klatt (8000 Hz). */
#ifndef ENGINE_H
#define ENGINE_H
#define ENGINE_SAM 0
#define ENGINE_KLATT 1
#define ENGINE_RETRO 2   /* SPEECH.COM 1983 one-bit voice (386 build) */
extern int engine_id;
void engine_init(int which);                      /* before anything else */
unsigned engine_rate(void);                       /* output sample rate */
void engine_set_params(unsigned dt_speed, unsigned dt_pitch);   /* DoubleTalk 0..9, 0..99 */
void engine_speak(char *utt, unsigned len);       /* utt has room for 256 bytes; may be modified */
#endif
