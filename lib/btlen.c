#include <btflds.h>
#include <string.h>

/* OK, so you hate the names.
	Well I asked, and no-one had any better ideas. */

int bt_klen(const char *f) {
  register int n, len = 0;
  f += strlen(f) + 1;
  for (n = *f; n && *++f; --n) {
     if (*f++ != BT_SEQ)
       len += (unsigned char)*f;
  }
  return len;
}

int bt_rlen(const char *f,int rlen) {
  register int len;
  len = strlen(f) + 1;
  f += len;
  rlen -= len + 1;
  len = 0;
  while (--rlen > 0 && *++f) {
    if (*f++ != BT_SEQ)
      len += (unsigned char)*f;
    if (--rlen == 0) break;
  }
  return len;
}
