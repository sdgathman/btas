#include <stdio.h>
#include <assert.h>
#include "btree.h"
#include "node.h"

/* insert record in node after idx, 0 <= idx <= node_count(np) */

int node_insert(BLOCK *bp,int idx,char *rec,int len) {
  int i;
  NODE *np = bp->np;
  unsigned char *dst,*src,*tgt;
  int n;		/* number of records in node */
  int samenext;		/* compress length of following record */
  int sameinc;		/* change in samenext */
  int dup;		/* compress length of inserted record */

  n = bp->count;
  assert(0 <= idx && idx <= n);

  src = rptr(np,n);	/* save current base of data */

  /* find new compress length for following record.  Note that 'following'
	means 'following in compress sequence'.  Currently this is
	the index we are inserting after */

  sameinc = dup = 0;
  if (idx > 0) {
    tgt = rptr(np,idx);
    samenext = *tgt++;
    if (idx < n) dup = samenext;
    i = len;
    if (bp->flags & BLK_STEM) i -= sizeof bp->blk;
    sameinc = blkcmp(tgt,(unsigned char *)rec+samenext,i - samenext);
    samenext += sameinc;
    assert(samenext == i || tgt[sameinc] > (unsigned char)rec[samenext]);
  }

  /* compute compress length for inserted record using 'previous' record */

  if (idx < n) {
    unsigned char buf[MAXKEY];
    i = node_size(np,idx+1);
    if (bp->flags & BLK_STEM) i -= sizeof bp->blk;
    if (i > sizeof buf) i = sizeof buf;
    node_copy(bp,idx+1,(char *)buf,i,dup);
    dup += blkcmp(buf+dup,(unsigned char *)rec+dup,i-dup);
    assert(dup == i || (unsigned char)rec[dup] > buf[dup]);
  }

  len += 1 - dup;	/* what we actually need to store */
  sameinc -= len;	/* what we need to shift down by */

  /* does new record fit? */
  if (np->tab[n] + sameinc < (n+2) * (int)sizeof *np->tab)
    return -1;

  bp->count = ++n;
  for (i = n; i > idx; --i)	/* insert a table spot */
    np->tab[i] = np->tab[i-1] + sameinc;

  if (idx) {	/* first in node is a special case */
    np->tab[idx] += sameinc + len;
    *rptr(np,idx) = samenext;
  }

  tgt = rptr(np,++i);
  if (i < n) {
    dst = rptr(np,n);	/* shift existing data */
    memcpy(dst,src,tgt - dst);
  }
  *tgt++ = dup;				/* key compression for new record */
  memcpy(tgt,rec+dup,len - 1);	/* insert new record */
  bp->flags |= BLK_MOD;				/* mark block modified */
  assert(*rptr(np,bp->count) == 0);		/* did we screw up ? */
  return 0;
}

