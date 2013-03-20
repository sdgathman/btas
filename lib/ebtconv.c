/*
	convert BTAS/1 - BTAS/X data records

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
 * Revision 1.7  2003/05/19 18:24:49  stuart
 * Typo defining away farblkfntl
 *
 * Revision 1.6  2001/02/28 22:25:02  stuart
 * define away x86 abominations
 *
 * Revision 1.5  1994/02/10  19:51:13  stuart
 * replace control chars when compressing BT_CHAR fields
 *
 */

#include <stdlib.h>
#include <string.h>
#include <btflds.h>
#include <block.h>
#include <ebcdic.h>

#define BIGREC		/* for ues with btasedx.h */

/* abominations from x86, should probably define in block.h, but hopefully
 * segmented x86 WIN16 is dead now.  We need mixed model for HBX
 * implementation of BTAS/1 emulator. */
#define _fmemset memset
#define _fmemcpy memcpy
#define farblkfntr blkfntr
#define farblkfntl blkfntl

static unsigned short e2brec_rlen = ~0;
BYTE e2brec_skip = 0;

void e2brec_begin(unsigned rlen) {	/* large record length */
  e2brec_rlen = rlen;
  e2brec_skip = 0;
}

/** Convert BTAS/X record to EDL.  Unpacks records packed b7 e2brec.
 * @param p	field table as stored in directory record
 * @param urec	unpacked EDX record
 * @param ulen	size of EDX record
 * @param buf	BTAS/X record buffer
 * @param len	size of BTAS/X record
*/
void b2erec(const BYTE *p,BYTE far *urec,int ulen,const char *buf,int len) {
  register int pos = 0;
  for (++p; *p; p += 2) {
    register int flen;

    if (pos >= ulen) {
      e2brec_rlen = ~0;	/* disable large record code until next e2brec_begin */
      e2brec_skip = 0;
      break;	/* field outside user buffer */
    }
    flen = p[1];		/* uncompressed field length */

    if (*p == BT_CHAR) {
      if (pos + flen > ulen)
	flen = ulen - pos;	/* truncate short field */
      pos  += flen;
      if (len == 0 || *buf == 0) {		/* blank field */
	if (len) { ++buf; --len; }
      }
      else {
	register int rc,i;
	if (*buf < ' ') {	/* if leading blanks */
	  int blen = ' ' - *buf++ + 1;
	  --len;
	  if (blen > flen) blen = flen;
	  _fmemset(urec,0x40,blen); /* add leading blanks */
	  urec += blen;
	  flen -= blen;
	}
	rc = (flen > len) ? len : flen;

	for (i = 0; i < rc; ++i) {
	  if (*buf == 0) {
	    ++buf; --len;
	    break;
	  }
	  *urec++ = ascii(*buf++);
	}
	len -= i;
	flen -= i;
      }
      _fmemset(urec,0x40,flen);	/* add trailing blanks */
    }
#ifdef BIGREC
    else if (*p == BT_SEQ) {
      int skip;		/* number of fields to skip */
      buf += flen;
      len -= flen;
      /* use low order byte only */
      e2brec_skip = skip = (unsigned char)buf[-1];	
      /* add up uncompressed bytes to skip */
      for (flen = 0; skip-- && p[2]; p += 2,flen += p[1]);
      pos += flen;
    }
#endif
#if 0
    else if (*p == BT_RLOCK) {
      if (buf[len - 1] == 0)
	*urec = 0xff;
      else
	*urec = 0x00;
      ++pos;
    }
#endif
    else {
      int mlen;
      /* optimize by combining adjacent binary fields */
      while (p[2] && p[2] != BT_CHAR) {
#ifdef BIGREC
	if (p[2] == BT_SEQ) break;
#endif
	p += 2; flen += p[1];
      }
      if (pos + flen > ulen)
	flen = ulen - pos;	/* truncate short field */
      pos += flen;
      mlen = flen;
      if (mlen > len) {
	_fmemset(urec + len, 0, mlen - len);
	mlen = len;
      }
      _fmemcpy(urec,buf,mlen);
      buf += mlen;
      len -= mlen;
    }
    urec += flen;
  }
#if 0	/* BTAS/1 did not do this */
  if (pos < ulen)
    _fmemset(urec, 0, ulen - pos);
#endif
}

/** Pack EDX BTAS/1 record into BTCB.  The BTCB will contained
   the packed record length and key length.
<p>
	This is similar to pack.c used by the C-isam emulator,
	but character fields are translated EBCDIC/ASCII and
	scatter/gather is not required.  Accordingly, the btflds
	structure is not used and the field tables are stored
	as they appear in the directory records to save memory.
<p>
	pack.c removes trailing nulls when packing records.  We indicate
	BTAS/1 locked records by appending a trailing null.  This uses
	one additional byte only for locked records and the records are
	still read correctly by pack.c.  It follows that the BTAS/1
	lock status is not detected by the C library routines.  (But
	maybe we'll add the feature.)  Anyway, we aren't using locked
	records in EDX anymore either!
 @param p field table as stored in directory record
 @param urec	unpacked user record
 @param ulen	size of user record
 @param btcb	BTCB with lbuf to hold packed record
 @param klen	logical key length
*/
unsigned e2brec(
  const BYTE *p,	/* field table */
  const BYTE far *urec,	/* unpacked user record */
  unsigned ulen,	/* size of user record */
  BTCB *btcb,		/* btcb->klen, btcb->rlen, btcb->lbuf modified */
  int klen		/* unpacked key length */
) {
  const BYTE *sav = p;
  char *buf = btcb->lbuf;	/* compressed buffer pointer */
  unsigned pos = 0;	/* position in user buffer */
  int skip = 0;		/* adjustment to pos to give position in segment */
  btcb->klen = 0;	/* compress key length */
  for (++p;; p += 2) {
    int flen;		/* uncompressed size of current field */
    if (*p == 0) {
      e2brec_rlen = ~0;	/* reset until e2brec_begin */
      e2brec_skip = 0;
      break;
    }
    flen = p[1];
    pos += flen;
    if ((unsigned)(pos - skip) > e2brec_rlen) {
      e2brec_skip = (p - sav) / 2 - *sav;
      pos -= flen;
      break;
    }
    if (pos > ulen)
      flen -= pos - ulen;
    if (flen <= 0) continue;
    if (*p == BT_CHAR) {
      register int blen;		/* leading blank count */
      register int nblen;		/* non-blank count */
      blen = farblkfntr((char far *)urec,0x40,flen);
      if (blen == flen || *urec == 0) {
	*buf++ = 0;
	nblen = 0;
      }
      else {
	register int i;
	if (blen) {	/* leading blank compression */
	  if (blen > ' ')
	    blen = ' ';	/* max leading blank count */
	  *buf++ = (char)(' ' - blen + 1);
	  if (klen) {
	    if (klen <= blen) {
	      btcb->klen = (short)(buf - btcb->lbuf);
	      klen = 0;
	    }
	    else
	      klen -= blen;
	  }
	  urec += blen;
	  flen -= blen;
	}

	i = farblkfntl((char far *)urec+flen,0x40,flen);
	for (nblen = 0; *urec && nblen < i; ++nblen) {
	  int c = ebcdic(*urec++);
	  if (c < ' ')
	    c = ':';
	  *buf++ = c;
	}
	if (nblen < p[1] - blen)
	  *buf++ = 0;
      }
      if (klen) {
	if (klen <= flen) {
	  /* entire field including null */
	  btcb->klen = (short)(buf - btcb->lbuf);
	  if (klen <= nblen) {
	    btcb->klen -= (short)(nblen - klen);	/* partial field */
	    if (nblen < p[1] - blen) --btcb->klen;
	  }
	  klen = 0;
	}
	else
	  klen -= flen;
      }
      flen -= nblen;
    }
    else {
      if (klen) {
	if (klen <= flen) {
	  btcb->klen = (short)(buf - btcb->lbuf + klen);
	  klen = 0;
	}
	else
	  klen -= flen;
      }
#ifdef BIGREC
      if (*p == BT_SEQ) {
	skip -= flen;	/* BT_SEQ counts in the segment length, */
	pos  -= flen;	/* but not the user record position */
	while (flen-- > 1)
	  *buf++ = 0;
	*buf++ = e2brec_skip;	/* use low order byte only */
	/* add up uncompressed bytes to skip */
	for (flen = 0; e2brec_skip-- && p[2]; p += 2,flen += p[1]);
	e2brec_skip = 0;
	pos += flen;
	skip += flen;
      }
      else		/* no compression for non-character types */
#endif
      {
	_fmemcpy(buf,urec,flen);
	buf += flen;
      }
    }
    urec += flen;
  }
  /* final record length */
  btcb->rlen = (short)farblkfntl((char far *)buf,0,buf - btcb->lbuf);
  if (btcb->rlen < btcb->klen)
    btcb->rlen = btcb->klen;
  return pos;
}

#if 0 // LOCKING
int islocked(const BTCB *b) {
  return (b->rlen > b->klen && b->lbuf[b->rlen - 1] == 0);
}

void lockrec(BTCB *b) {
  if (!islocked(b))
    b->lbuf[b->rlen++] = 0;
}

void unlockrec(BTCB *b) {
  if (islocked(b)) --b->rlen;
}
#endif

/** Convert EDX record segment to stand alone field table */
int e2brec_conv(BYTE *dst,const BYTE *src,int maxlen,int skip) {
  int rlen = 0;
  int pos = 0;
  int klen = *src++;
  for (*dst++ = klen; *src; src += 2) {
    rlen += src[1];
    if (rlen > maxlen) break;
    *dst++ = *src;
    *dst++ = src[1];
    if (klen) {
      pos += src[1];
      --klen;
    }
    if (*src == BT_SEQ) {
      dst[-2] = BT_BIN;
      pos = rlen - src[1];
      while (skip-- && src[2])
	src += 2, pos += src[1];
    }
  }
  *dst = 0;	/* terminate output table */
  return pos;	/* return user position of non-key portion */
}
