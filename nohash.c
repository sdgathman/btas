#include <stdio.h>
#include "hash.h"

/* allocate hash table, and initialize chain */

int begbuf(char *pool,int nsize,int size) {
  stat.bufs = size;
  while (size-- > 0) {
    BLOCK *bp = (BLOCK *)pool;
    bp->lru = mru;
    mru = bp;
    pool += nsize;
  }
  return 0;
}

void endbuf() {
  mru = 0;
}

static BLOCK *inuse[MAXBUF];
static int lastcnt = 0;
int curcnt = 0;

void btget(int cnt) {
  while (lastcnt > 0) {
    register BLOCK *bp = inuse[--lastcnt];
    /* assume chain is never empty */
    bp->lru = mru;
    mru = bp;
  }
  curcnt = cnt;
}

BLOCK *getbuf(t_block blk,short mid) {
  register BLOCK *bp, **pp;
  ++stat.searches;
  for (pp = &mru;;pp = &bp->lru) {
    bp = *pp;
    ++stat.probes;
    if (bp->blk == blk && bp->mid == mid)
      break;
    if (bp->lru == 0) {
      if (bp->flags & BLK_MOD)
	writebuf(bp);
      bp->blk = 0;
      bp->mid = mid;
      break;
    }
  }
  *pp = bp->lru;
  return inuse[lastcnt++] = bp;
}

/* swap block contents - mid and node types must be the same! */

void swapbuf(BLOCK *np,BLOCK *bp) {
  short flag = np->flags;
  long blk = np->blk;
  np->blk = bp->blk;
  np->flags = bp->flags;
  bp->flags = flag;
  bp->blk = blk;
}
