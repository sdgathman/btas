#if !defined(lint) && !defined(__MSDOS__)
static char what[] = "@(#)node.c	1.15";
#endif

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

#include "btree.h"
#include "node.h"
#include <stdio.h>
#include <assert.h>

#ifndef node_size
short node_size(np,i)
  NODE *np;
  int i;
{
  return np->tab[i-1] - np->tab[i] + *rptr(np,i) - 1;
}
#endif

/* Extract record from node.  Caller should set node_dup to compress count
   of prev (next lower) record in node.  If unknown, supply zero.  If reading
   in reverse, set node_dup = len for best speed.  Return compress count of
   record extracted.  Caller can get actual record size
   with node_size(), this function does not check. */

int node_copy(bp,idx,rec,len,s)
  BLOCK *bp;
  int idx;
  char *rec;
  int len,s;
{
  register unsigned char *p;
  register NODE *np = bp->np;
  register int i, dup;
  p = rptr(np,idx);
  dup = *p;
  for (;;) {
    assert(idx <= node_count(bp));
    i = *p;
    if (i < len) {
      (void)memcpy(rec+i,(char *)++p,len-i);
      len = i;
    }
    if (i <= s) break;
    p = rptr(np,++idx);
  }
  return dup;
}

/* compare a record to a partial key */

short node_comp(bp,idx,rec,len)
  BLOCK *bp;
  int idx;
  char *rec;
  int len;
{
  unsigned char buf[256];		/* largest compression */
  register int i,res;
  register unsigned char *p;
  p = rptr(bp->np,idx);
  i = *p;
  if (i > 0) {
    if (i > len) i = len;
    node_copy(bp,idx,(char *)buf,i,0);/* extract compressed portion */
    res = blkcmp(buf,(unsigned char *)rec,i);	/* and compare */
    if (res < i) return res;		/* return if that sufficed */
  }
  res = bp->np->tab[idx-1] - bp->np->tab[idx] - 1;	/* remaining bytes */
  if ((len -= i) > res) len = res;			/* left to compare */
  return (i + blkcmp(p+1,(unsigned char *)rec+i,len));	/* compare rest */
}

void node_clear(bp)
  BLOCK *bp;
{
  bp->count = 0;
  bp->flags |= BLK_MOD;
  (void)memset((char *)bp->np->dat+sizeof *bp->np->tab,0,
	*bp->np->tab - (int)sizeof *bp->np->tab);
}

/*
  Move records from one node to another, return actual number moved.
  The records are *not* deleted from the source node.  The caller
  can easily do that when it is most efficient.

  This implementation is simple; it should move the records
  as one block.  When the destination is the end of a node,
  even this version is reasonably efficient;
*/

short node_move(bp1,bp2,from,to,cnt)
  BLOCK *bp1,*bp2;
  int from,to;	/* first record to stay */
  int cnt;		/* positive for np1[n] -> np2[0] */
			/* negative for np1[1] -> np2[n] */
{
  short scnt = node_count(bp1);
  short rlen;
  NODE *np = bp1->np;
  register int i, dup;

  dup = 0;		/* init compress count for node_copy */
  for (i = 0; i < cnt && from <= scnt; ++i,++to,++from) {
    rlen = node_size(np,from);
    dup = node_copy(bp1,from,workrec,rlen,dup);
    if (node_insert(bp2,to,workrec,rlen)) break;
  }
  /* node_insert() sets bp2->flags |= BLK_MOD */
  return i;
}

/* this is where we use blkmovl() */

void node_delete(bp,idx,cnt)
  BLOCK *bp;
  int idx,cnt;
{
  register short i;
  register NODE *np = bp->np;
  unsigned char *tgt;
  short slen,n;
  if (cnt > 0) {
    n = node_count(bp);
    assert(idx > 0 && idx <= n);

    /* recompute key compression */

    tgt = rptr(np,--idx);
    if (idx > 0) {
      short tlen;
      tlen = *tgt++;
      for (i = 0; ++i <= cnt;) {
	unsigned char *src;
	src = rptr(np,idx+i);
	slen = tlen - *src;
	if (slen > 0) {
	  tgt -= slen;
	  tlen -= slen;
	  np->tab[idx] -= slen;
	  (void)blkmovl((char *)tgt+slen,(char *)++src+slen,slen);
	  if (tlen == 0) break;
	}
      }
      *--tgt = tlen;
    }
    i = idx + cnt;
    slen = np->tab[idx] - np->tab[i];	/* amount to shift */

    /* shift data */
    blkmovl((char *)tgt,(char *)rptr(np,i),np->tab[i] - np->tab[n]);

    /* recompute offsets */
    while (i < n) np->tab[++idx] = np->tab[++i] + slen;

    bp->count -= cnt;
    bp->flags |= BLK_MOD;		/* mark block modified */
    assert(bp->count == 0 || *rptr(np,bp->count) == 0);
  }
}

/* replace a record.  Delete it if the new record is too big.
   Although the current implementation is trivial, this is a separate
   function because it could be optimized.
*/

int node_replace(bp,idx,rec,len)
  BLOCK *bp;
  int idx;
  char *rec;
  int len;
{
  /* a really lazy implementation for now */
  node_delete(bp,idx,1);
  return node_insert(bp,idx-1,rec,len);
}
