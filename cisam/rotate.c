/* rotate.c

	Copyright 1990 Business Management Systems, Inc.
	Author: Stuart D. Gathman
*/

#include <block.h>

void swap(buf1, buf2, len)
  char *buf1, *buf2;
  int len;
{
  char tmp[128];		/* the bigger, the faster */
  int tmplen = sizeof tmp;
  while (len) {
    if (tmplen > len) tmplen = len;
    (void)memcpy(tmp,buf1,tmplen);
    (void)memcpy(buf1,buf2,tmplen);
    (void)memcpy(buf2,tmp,tmplen);
    buf1 += tmplen;
    buf2 += tmplen;
    len  -= tmplen;
  }
}

/*
  Method: 
	1) rotate left or right, whichever is smaller
	2) if amount fits in tmp buffer, use it and we're done
	3) else, swap bytes at ends and recursively rotate remainder
	4) tail recursion is optimized into a loop
*/

void rotate(buf,len,cnt)	/* rotate bytes */
  char *buf;			/* address of area to rotate */
  int len;			/* size of area to rotate */
  int cnt;			/* number of bytes to rotate right */
{
  while (cnt < 0) cnt += len;	/* negative count rotates left */
  while (len > cnt) {		/* tail recursion loop */
    register int nlen = len - cnt;
    char tmp[128];		/* the bigger, the faster */
    if (cnt > len/2) {
      if (nlen <= sizeof tmp) {		/* rotate left */
	(void)memcpy(tmp,buf,nlen);
	(void)memcpy(buf,buf+nlen,cnt);	/* use blkmovr?, memcpy not reliable */
	(void)memcpy(buf+cnt,tmp,nlen);
	return;
      }
      swap(buf, buf + cnt, nlen);
      len = cnt;
      cnt -= nlen;
    }
    else {
      if (cnt <= sizeof tmp) {		/* rotate right */
	(void)memcpy(tmp,buf+nlen,cnt);
	blkmovl(buf+len,buf+nlen,nlen);
	(void)memcpy(buf,tmp,cnt);
	return;
      }
      swap(buf, buf + nlen, cnt);
      len = nlen;
      buf += cnt;
    }
  }
}
