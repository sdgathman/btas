#define OLDMONEY 1	/* support old MONEY format */
#include <stdlib.h>
#include <money.h>
#include <memory.h>

#define SGNBIT	(((unsigned char)(~0) >> 1) + 1)

static char sbit = OLDMONEY;

MONEY ldnum(buf,len)
  const char *buf;
  int len;
{
  FMONEY m;
  register int i;
  i = sizeof m;
  while (i > 0 && len > 0) m[--i] = buf[--len];
#if OLDMONEY == 1
  if (sbit == 1) {
    const char *s = getenv("MONEY");
    sbit = (s && *s == 'O') ? SGNBIT : 0;
  }
  if (i < 2) m[i] ^= sbit;
#endif
  if (m[i] == (char)SGNBIT) {
    int j;
    for (j = i + 1; j < sizeof m && m[j] == 0; ++j)
      ;
    if (j == sizeof m)
      return nullM;
  }
  (void)memset(m,m[i] & SGNBIT ? ~0 : 0,i);
  m[0] ^= sbit;
  return ldM(m);
}

void stnum(v,buf,len)
  MONEY v;
  char *buf;
  int len;
{
  FMONEY m;
  (void)stM(v,m);
#if OLDMONEY == 1
  if (sbit == 1) {
    char *s = getenv("MONEY");
    sbit = (s && *s == 'O') ? SGNBIT : 0;
  }
  m[0] ^= sbit;
#endif
  if (len > sizeof m) {
    (void)memset(buf,*m & SGNBIT ? ~0 : 0,len - (int)sizeof m);
    buf += len - sizeof m;
    len = sizeof m;
  }
  (void)memcpy(buf,m + sizeof m - len,len);
  if (len > 4) buf[0] ^= sbit;
}
