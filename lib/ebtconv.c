/*
	convert BTAS/1 - BTAS/X data records

	This is similar to pack.c used by the C-isam emulator,
	but character fields are translated EBCDIC/ASCII and
	scatter/gather is not required.  Accordingly, the btflds
	structure is not used and the field tables are stored
	as they appear in the directory records to save memory.

	pack.c removes trailing nulls when packing records.  We indicate
	BTAS/1 locked records by appending a trailing null.  This uses
	one additional byte only for locked records and the records are
	still read correctly by pack.c.  It follows that the BTAS/1
	lock status is not detected by the C library routines.  (But
	maybe we'll add the feature.)  Anyway, we aren't using locked
	records in EDX anymore either!
*/

#include "btasedx.h"

#include <stdlib.h>
#include <string.h>
#include <btflds.h>
#include <block.h>
#include "ebcdic.h"

/*
	convert BTAS/X record to EDL
*/
void b2erec(p,urec,ulen,buf,len)
  const BYTE *p;		/* field table */
  BYTE far *urec;		/* uncompressed user buffer */
  int ulen;			/* user record length: max bytes to load */
  const char *buf;		/* compressed buffer */
  int len;			/* size of compressed buffer */
{
  register int pos = 0;
  for (++p; *p; p += 2) {
    register int flen;

    if (pos >= ulen) break;	/* field outside user buffer */
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
	  farmemset((char far *)urec,0x40,blen); /* add leading blanks */
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
      farmemset((char far *)urec,0x40,flen);	/* add trailing blanks */
    }
#if 0
    else if (*p == BT_RLOCK) {
      if (buf[len - 1] == 0)
	*urec = 0x40;
      else
	*urec = 0x00;
      ++pos;
    }
#endif
    else {
      int mlen;
      /* optimize by combining adjacent binary fields */
      while (p[2] && p[2] != BT_CHAR) {
	p += 2; flen += p[1];
      }
      if (pos + flen > ulen)
	flen = ulen - pos;	/* truncate short field */
      pos += flen;
      mlen = flen;
      if (mlen > len) {
	farmemset((char far *)urec + len, 0, mlen - len);
	mlen = len;
      }
      farmove((char far *)urec,buf,mlen);
      buf += mlen;
      len -= mlen;
    }
    urec += flen;
  }
  if (pos < ulen)
    farmemset((char far *)urec, 0, ulen - pos);
}

void e2brec(p,urec,ulen,btcb,klen)
  const BYTE *p;	/* field table */
  const BYTE far *urec;	/* unpacked user record */
  int ulen;		/* size of user record */
  BTCB *btcb;		/* btcb->klen, btcb->rlen, btcb->lbuf modified */
  int klen;		/* unpacked key length */
{
  register char *buf = btcb->lbuf;
  register int pos = 0;

  btcb->klen = 0;
  for (++p; *p; ++p, pos += *p++) {
    register int flen = p[1];
    if (pos >= ulen)
      flen = 0;
    else if (pos + flen > ulen)
      flen = ulen - pos;
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
	  *buf++ = ebcdic(*urec++);
	}
	if (nblen < p[1] - blen)
	  *buf++ = 0;
      }
      urec += flen - nblen;
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
    }
    else {		/* no compression for non-character types */
      farmove((char far *)buf,(char far *)urec,flen);
      if (klen) {
	if (klen <= flen) {
	  btcb->klen = (short)(buf - btcb->lbuf + klen);
	  klen = 0;
	}
	else
	  klen -= flen;
      }
      buf += flen;
      urec += flen;
    }
  }
  /* final record length */
  btcb->rlen = (short)farblkfntl((char far *)buf,0,buf-btcb->lbuf);
  if (btcb->rlen < btcb->klen) btcb->rlen = btcb->klen;
}

#ifdef LOCKING
int islocked(const BTCB *b)
{
  return (b->rlen > b->klen && b->lbuf[b->rlen - 1] == 0);
}

void lockrec(BTCB *b)
{
  if (!islocked(b))
    b->lbuf[b->rlen++] = 0;
}

void unlockrec(BTCB *b)
{
  if (islocked(b)) --b->rlen;
}
#endif
