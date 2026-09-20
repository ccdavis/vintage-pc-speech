/* Minimal config.h for building upstream rsynth's `say` on a modern Linux
   host without autotools (see Makefile target rsynth_say). */
#define STDC_HEADERS 1
#define HAVE_UNISTD_H 1
#define HAVE_FCNTL_H 1
#define HAVE_LIMITS_H 1
#define HAVE_SYS_TYPES_H 1
#define HAVE_FTRUNCATE 1
#define RETSIGTYPE void
