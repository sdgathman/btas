#include <string.h>
#include <ftype.h>
#include "config.h"

union dbl {
  double f;	/* FIXME: not exactly portable :-) */
  long i[2];	/* we store only this much */
  char c[sizeof (double)];
};

void stdbl(double x,char *buf) {
  union dbl u;
  u.f = x;
#ifdef SWAPDBL		/* IEEE doubles byte swapped */
  {
    int i,j;
    for (i = 0,j = sizeof u.c - 1; i < j; ++i,--j) {
      char t = u.c[i];
      u.c[i] = u.c[j];
      u.c[j] = t;
    }
  }
#endif
  if (x < 0) {
    u.i[0] ^= ~0L;
    u.i[1] ^= ~0L;
  }
  else
    u.c[0] ^= 0x80;
  memcpy(buf,u.c,8);
}

double lddbl(const char *buf) {
  union dbl u;
  memcpy(u.c,buf,8);
  if (u.c[0] & 0x80)
    u.c[0] ^= 0x80;
  else {
    u.i[0] ^= ~0L;
    u.i[1] ^= ~0L;
  }
#ifdef SWAPDBL		/* IEEE doubles byte swapped */
  {
    int i,j;
    for (i = 0,j = sizeof u.c - 1; i < j; ++i,--j) {
      char t = u.c[i];
      u.c[i] = u.c[j];
      u.c[j] = t;
    }
  }
#endif
  return u.f;
}
