#include <stdio.h>
#include <string.h>
#include "btutil.h"

char *readtext(const char *s) {
  char *p;
  static char buf[256], *wsp = " \t\n", *last = 0;
  if (!s) last = 0;
  p = gettoken((char *)0,wsp);		/* next command word */
  while (!p && s) {
    int len = 0;
    char *t;
    if (last)
      len = last - buf;
    t = buf + len;
    (void)fputs(s,stdout);
    if (fgets(t,sizeof buf - len,stdin) == 0) return 0;
    last = t + strlen(t) + 1;
    p = gettoken(t,wsp);
  }
  return p;
}

long getval(const char *s) {
  char *p;
  long val;
  for (;;) {
    p = readtext(s);
    if (!p) return 0;
    val = atol(p);
    if (!s || val > 0) return val;
  }
}

int question(const char *s) {
  int c;
  for (;;) {
    (void)fputs(s,stdout);
    c = getchar();
    while (getchar() != '\n');
    if (c == 'Y' || c == 'y') return 1;
    if (c == 'N' || c == 'n') return 0;
  }
}

char *gettoken(char *s,const char *wsp) {
  static char *p = "";
  char *t;

  if (s) p = s;
  while (*p && strchr(wsp,*p)) ++p;
  t = p;
  if (*p) {
    if (*p == '"' || *p == '\'') {
      char sep = *p;
      t = ++p;
      while (*p && *p != sep) ++p;	/* quoted string */
    }
    else {
      while (*p && !strchr(wsp,*p)) ++p;	/* unquoted string */
    }
    if (*p) *p++ = 0;
  }
  if (!*t) return 0;
  return t;
}
