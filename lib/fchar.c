#include <ftype.h>
#include <string.h>

void ldchar(src,len,dst)
  const char *src;
  char *dst;
{
  while (len-- && src[len] == ' ');
  memcpy(dst,src,++len);
  dst[len] = 0;
}

int stchar(src,dst,len)
  const char *src;
  char *dst;
{
  if (src)
    while (len && *src) *dst++ = *src++,len--;
  memset(dst,' ',len);
  return (*src) ? -1 : 0;
}
