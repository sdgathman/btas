#include <stdio.h>
#include "btree.h"
#include "hash.h"

static BLOCK *mru;

/* allocate hash table, and initialize chain */

int begbuf(pool,nsize,size)
  char *pool;
{
  mru = (BLOCK *)pool;
  fprintf(stderr,"Buffers = %d\n",size);
  while (--size > 0)
    putbuf((BLOCK *)(pool += nsize));
  return 0;
}

static long searches, probes;

void endbuf() {
  fprintf(stderr,"Linear search:\tsearches = %ld, probes = %ld\n",
	      searches,probes);
}

/* perhaps btget should be the interface here */

void putbuf(bp)
  BLOCK *bp;
{
  bp->lru = mru;
  mru = bp;
}

BLOCK *getbuf(blk,mid)
  t_block blk;
  int mid;
{
  register BLOCK *bp, **pp;
  ++searches;
  for (pp = &mru;;pp = &bp->lru) {
    bp = *pp;
    ++probes;
    if (bp->lru == 0 || bp->blk == blk && bp->mid == mid) {
      *pp = bp->lru;	/* remove from chain */
      return bp;
    }
  }
}

void swapbuf(np,bp)
  BLOCK *np, *bp;
{
  long blk;
  blk = np->blk;
  np->blk = bp->blk;
  bp->blk = blk;
  /* mid must be same! */
}
