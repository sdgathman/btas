/*	Pack strings
	Copyright 1990 Business Management Systems, Inc. 
	Author: Stuart D. Gathman 

	Input	src	Count or null terminated
		dst	compressed string
		return:	total compressed length
*/

#include <block.h>

int packstr(dst,ubuf,len)
  char *dst;		/* output buffer */
  const char *ubuf;	/* uncompressed buffer */
  int len;		/* original uncompressed length */
{
  register int blen;		/* leading blank count */
  register int nblen;		/* non-blank count */
  int flen = len;		/* remaining uncompressed length */
  register char *buf = dst;
  char *cp;
  for (blen = 0; ubuf[blen] == ' '; ++blen);	/* blkfntr() ? */
  if (blen == flen || *ubuf == 0) {
    *buf++ = 0;
    nblen = 0;
  }
  else {
    if (blen) {	/* leading blank compression */
      if (blen > ' ')
	blen = ' ';	/* max leading blank count */
      *buf++ = ' ' - blen + 1;
      ubuf += blen;
      flen -= blen;
    }

    nblen = blkfntl(ubuf+flen,' ',flen);
    if (cp = memccpy(buf,ubuf,0,nblen)) {
      nblen = cp - buf - 1;
      buf = cp;	/* stopped at null terminator */
    }
    else {
      buf += nblen;	/* otherwise, all is copied */
      if (nblen < len - blen)
	*buf++ = 0;	/* add null terminator */
    }
  }
  return buf - dst;
}
