#ifndef ANSI
#define const
#endif

/* implement in assembler ASAP */

int blkcmp(s1,s2,len)	/* return index of first differing char */
  const unsigned char *s1, *s2;
  int len;
{
  register int i = len;
  while (i && *s1++ == *s2++) --i;
  return len - i;
}

void blkmovl(dst,src,len)
  char *dst;
  const char *src;
  int len;
{
  while (len--) *--dst = *--src;
}
