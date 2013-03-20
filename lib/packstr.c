/*	Pack strings
	Copyright 1990 Business Management Systems, Inc. 
	Author: Stuart D. Gathman 

    This file is part of the BTAS client library.

    The BTAS client library is free software: you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public License as
    published by the Free Software Foundation, either version 3 of the License,
    or (at your option) any later version.

    BTAS is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with BTAS.  If not, see <http://www.gnu.org/licenses/>.

    Experimental string packing with blank compression, inspired by IBM.
    Not used.

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
