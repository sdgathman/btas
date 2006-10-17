/* $Log$
/* Revision 2.4  2001/02/28 21:35:32  stuart
/* dump method
/*
 * Revision 2.3  1997/06/23 15:39:52  stuart
 * use local workrec
 *
 * Revision 2.2  1996/12/17  17:00:57  stuart
 * fix comment syntax
 *
 * Revision 2.1  1996/12/17  16:40:32  stuart
 * C++ node interface
 *
/*
	implement node functions

	We need to ensure that inserting in ascending key order is efficient.
	This means we must always start comparing with the highest keys when
	storing newer records toward the beginning of the node.  We store
	the table at the beginning and records from the end because this makes
	the table access the simplest.  We continue to use key records pointing
	to data gteq the key value.  This is only because we are used to it.
	There are no advantages to other key record conventions except for
	partial keys, but then it favors a particular direction.
*/

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "node.h"

BLOCK::BLOCK() {
  flags = 0;
  blk = 0;
  mid = 0;
  count = 0;
  np = (union node *)buf.l.data;
}

void BLOCK::setblksize(unsigned blksz) {
  np->setsize(buf.data + blksz);
}

short BLOCK::size(int i) const { return np->size(i) + *np->rptr(i) - 1; }

/* Extract record from node.  Caller should set sdup to compress count
   of prev (next lower) record in node.  If unknown, supply zero.  If reading
   in reverse, set sdup = len for best speed.  Return compress count of
   record extracted.  Caller can get actual record size
   with BLOCK::size(), this function does not check. */

int BLOCK::copy(int idx,char *rec,int len,int sdup) const {
  unsigned char *p;
  int i, dup;
  p = np->rptr(idx);
  dup = *p;
  for (;;) {
    assert(idx > 0 && idx <= count);
    i = *p;
    if (i < len) {
      memcpy(rec+i,++p,len-i);
      len = i;
    }
    if (i <= sdup) break;
    p = np->rptr(++idx);
  }
  return dup;
}

/* compare a record to a partial key */

short BLOCK::comp(int idx,const char *rec,int len) const {
  unsigned char buf[256];		/* largest compression */
  register int i,res;
  register unsigned char *p;
  p = np->rptr(idx);
  i = *p;
  if (i > len) {
    copy(idx,(char *)buf,len);/* extract compressed portion */
    return blkcmp(buf,(unsigned char *)rec,len);	/* and compare */
  }
  if (i > 0) {
    copy(idx,(char *)buf,i);/* extract compressed portion */
    res = blkcmp(buf,(unsigned char *)rec,i);	/* and compare */
    if (res < i) return res;		/* return if that sufficed */
  }
  res = np->tab[idx-1] - np->tab[idx] - 1;	/* remaining bytes */
  if ((len -= i) > res) len = res;			/* left to compare */
  return (i + blkcmp(p+1,(unsigned char *)rec+i,len));	/* compare rest */
}

void BLOCK::clear() {
  count = 0;
  flags |= BLK_MOD;
  memset(np->dat+sizeof *np->tab,0,*np->tab - sizeof *np->tab);
}

/*
  Move records from one node to another, return actual number moved.
  The records are *not* deleted from the source node.  The caller
  can easily do that when it is most efficient.

  This implementation is simple. FIXME: it should move the records
  as one block.  When the destination is the end of a node,
  even this version is reasonably efficient;
*/

short BLOCK::move(BLOCK *bp2,int from,int to,int cnt) const {
  short scnt = count;
  int i, dup;

  dup = 0;		/* init compress count for copy */
  char *workrec = (char *)alloca(maxrec());
  for (i = 0; i < cnt && from <= scnt; ++i,++to,++from) {
    int rlen = size(from);
    dup = copy(from,workrec,rlen,dup);
    if (bp2->insert(to,workrec,rlen)) break;
  }
  /* bp2->insert() sets bp2->flags |= BLK_MOD */
  return i;
}

/* this is where we use blkmovl() */

void BLOCK::del(int idx,int cnt) {
  if (cnt > 0) {
    unsigned char *tgt;
    int i,slen,n = count;
    assert(idx > 0 && idx <= n);

    /* recompute key compression */

    tgt = np->rptr(--idx);
    if (idx > 0) {
      int tlen = *tgt++;
      for (i = 0; ++i <= cnt;) {
	unsigned char *src = np->rptr(idx+i);
	slen = tlen - *src;
	if (slen > 0) {
	  tgt -= slen;
	  tlen -= slen;
	  np->tab[idx] -= slen;
	  blkmovl((char *)tgt+slen,(char *)++src+slen,slen);
	  if (tlen == 0) break;
	}
      }
      *--tgt = tlen;
    }
    i = idx + cnt;
    slen = np->tab[idx] - np->tab[i];	/* amount to shift */

    /* shift data */
    blkmovl((char *)tgt,(char *)np->rptr(i),np->tab[i] - np->tab[n]);

    /* recompute offsets */
    while (i < n) np->tab[++idx] = np->tab[++i] + slen;

    count -= cnt;
    flags |= BLK_MOD;		/* mark block modified */
    assert(count == 0 || *np->rptr(count) == 0);
  }
}

/* replace a record.  Delete it if the new record is too big.
   Although the current implementation is trivial, this is a separate
   function because it could be optimized.
*/

int BLOCK::replace(int idx,const char *rec,int len) {
  /* a really lazy implementation for now */
  del(idx);
  return insert(idx-1,rec,len);
}

void BLOCK::dump() const {
  fprintf(stderr,"blk = %08lX, mid = %d, cnt = %d,%s%s%s%s%s\n",
    blk, mid, cnt(),
    (flags & BLK_MOD) ? " BLK_MOD" : "",
    (flags & BLK_TOUCH) ? " BLK_TOUCH" : "",
    (flags & BLK_ROOT) ? " BLK_ROOT" : "",
    (flags & BLK_STEM) ? " BLK_STEM" : "",
    (flags & BLK_CHK) ? " BLK_CHK" : ""
  );
  fprintf(stderr,"root = %08lX, son/rbro = %08lX\n", buf.r.root, buf.r.son);
}
