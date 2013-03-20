/*
	Copyright 1989,1990 Business Management Systems, Inc.
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

 * $Log$
 * Revision 1.4  2001/10/16 19:26:28  stuart
 * send key only on ISREAD in isserve.c, minor release
 *
 * Revision 1.3  2001/02/28 22:17:33  stuart
 * handle complemented strings
 *
 * Revision 1.2  1994/02/10  19:50:35  stuart
 * replace control characters when compressing char fields
 *
 */

#include <stdlib.h>
#include <string.h>
#include <block.h>
#include <ftype.h>
#include "btflds.h"

/** Record number used by C-isam emulation.  C-isam uses a synthetic 
 * 32-bit key called a "record number".  Although record numbers are
 * stored physically in packed records (when there is a key by record number),
 * they are packed/unpacked from/to this global variable instead of the
 * unpacked record.
 */
long isrecnum;   /* klunky C-isam interface */

/* Unpack a packed record into a fixed length record.
   This restores a record packed by u2brec.
  @param p	field table
  @param urec	fixed length record buffer
  @param ulen	fixed length record size
  @param buf	packed record buffer
  @param len	size of packed buffer
 */
void b2urec(p,urec,ulen,buf,len)
  const struct btfrec *p;	/* field table */
  char *urec;		/* uncompressed user buffer */
  int ulen;		/* user record length: max bytes to load */
  const char *buf;	/* compressed buffer */
  int len;		/* size of compressed buffer */
{
  for (;p->type; ++p) {
    register int flen;
    register char *ubuf;

    if (p->pos >= ulen) {	/* field outside user buffer */
      if (p->type == BT_RECNO) {
        char recno[4];
        for (flen = 0; flen < len && flen < 4; recno[flen++] = *buf++);
        len -= flen;
        while (flen < 4) recno[flen++] = 0;
        isrecnum = ldlong(recno);
      }
      continue;	/* field outside user buffer */
    }
    flen = p->len;			/* uncompressed field length */
    ubuf = urec + p->pos;
    if (p->pos + flen > ulen)
      flen = ulen - p->pos;		/* truncate short field */

    if (p->type == BT_CHAR) {
      /*	flen	hars to store in user field
	      	len	remaining bytes in btas buffer
	      	slen	logical field length in btas buffer
	      	buf	current pointer in btas buffer
	      	ubuf	current pointer in user buffer
      */
      int slen = p->len;
      while (len && slen--) {     /* consume all of source field */
        int c = (unsigned char)*buf++;
        --len;
	if (c == 0) break;
        while (c < ' ' && flen) {
          *ubuf++ = ' ';
          --flen; --slen;
	  ++c;
	}
        if (flen) {
          *ubuf++ = c;
          --flen;
        }
      }
      memset(ubuf,' ',flen);    /* pad with trailing blanks */
      ubuf += flen;
    }
    else {
      if (flen > len) {
	(void)memset(ubuf + len, 0, flen - len);
	flen = len;
      }
      (void)memcpy(ubuf,buf,flen);
      if (p->type & 0x80) {
	int i;
	for (i = flen; i-- > 0;)
	  ubuf[i] ^= 0xFF;
      }
      if (flen < p->len && p->len <= len)
	flen = p->len;
      buf += flen;
      len -= flen;
    }
  }
}

/* copy characters, stop at 0, return number copied */

static int fldccpy(char *tbuf,const char *ubuf,int flen) {
  char *buf = tbuf;
  while (flen--) {
    int c = (unsigned char)*ubuf++;
    if (c == 0) break;
    /* replace illegal chars in user data */
    if (c == 0xff)
      c = 0x7f;		/* highest legal char */
    else if (c < ' ')
      c = ' ';		/* lowest legal char */
    *buf++ = c;
  }
  return buf - tbuf;
}

/* Pack a fixed length record into BTCB buffer.
	Since records need to be copied in and out of the BTCB, we
take advantage of this opportunity to compress the record while
copying with minimal CPU overhead.  Compression is based on the standard field
table.  Characters fields have leading and trailing blanks trimmed.  Others
are left alone.  The resulting compressed record will never be larger
than the original, but non-printing bytes in character fields may be lost.

Character field compression is designed to preserve sort order.
 @param p	field table
 @param urec	unpacked fixed length record
 @param ulen	size of unpacked record
 @param b	BTCB to hold packed record with new record and key length
 @param klen	logical key length of unpacked record
 */
void u2brec(p,urec,ulen,b,klen)
  const struct btfrec *p;	/* field table */
  const char *urec;		/* unpacked user record */
  int ulen;		/* size of user record */
  BTCB *b;		/* b->klen, b->rlen, b->lbuf modified */
  int klen;		/* unpacked key length */
{
  register char *buf = b->lbuf;

  for (; p->type; ++p) {
    register const char *ubuf;		/* address of unpacked data */
    register int flen = p->len;
    if (p->pos >= ulen) {
      if (p->type == BT_RECNO) {
	stlong(isrecnum,buf);
	if (klen) {
	  if (klen <= flen) {
	    b->klen = buf - b->lbuf + klen;
	    klen = 0;
	  }
	  else
	    klen -= flen;
	}
	buf += flen;
      }
      flen = 0;
    }
    else if (p->pos + flen > ulen)
      flen = ulen - p->pos;
    ubuf = urec + p->pos;
    if (p->type == BT_CHAR) {
	/*	flen	chars in user field
		buf	current pointer in btas buffer
		ubuf	current pointer in user buffer
	*/
      register int blen;		/* leading blank count */
      register int nblen;		/* non-blank count */
      if ((blen = blkfntr(ubuf,' ',flen)) == flen || *ubuf == 0) {
	*buf++ = 0;
	nblen = 0;
      }
      else {
	if (blen) {	/* leading blank compression */
	  if (blen > ' ')
	    blen = ' ';	/* max leading blank count */
	  *buf++ = ' ' - blen + 1;
	  if (klen) {
	    if (klen <= blen) {
	      b->klen = buf - b->lbuf;
	      klen = 0;
	    }
	    else
	      klen -= blen;
	  }
	  ubuf += blen;
	  flen -= blen;
	}

	nblen = fldccpy(buf,ubuf,flen);
	nblen = blkfntl(buf+nblen,' ',nblen);
	buf += nblen;	/* otherwise, all is copied */
	if (nblen < p->len - blen)
	  *buf++ = 0;	/* add null terminator */
      }
      if (klen) {
	if (klen <= flen) {
	  b->klen = buf - b->lbuf;	/* entire field including null */
	  if (klen <= nblen) {
	    b->klen -= nblen - klen;	/* partial field */
	    if (nblen < p->len - blen) --b->klen;
	  }
	  klen = 0;
	}
	else
	  klen -= flen;
      }
    }
    else {		/* no compression for non-character types */
      (void)memcpy(buf,ubuf,flen);
      if (p->type & 0x80) {
	int i;
	for (i = flen; i-- > 0;)
	  buf[i] ^= 0xFF;
      }
      if (klen) {
	if (klen <= flen) {
	  b->klen = buf - b->lbuf + klen;
	  klen = 0;
	}
	else
	  klen -= flen;
      }
      buf += flen;
    }
  }
  while (buf[-1] == 0) --buf;	/* strip trailing zeroes */
  b->rlen = buf - b->lbuf;
  if (b->rlen < b->klen) b->rlen = b->klen;
}
