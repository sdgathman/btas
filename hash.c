#include <stdio.h>
#include <assert.h>
#include "btbuf.h"
#include "hash.h"

/* NOTE, hmask not required if we force an unsigned shift.  This is not
   safe, however, unless we know the exact size on an unsigned long. */

#define hash(v)	((v) * 781316125L >> hshift & hmask)

static BLOCK *mru, **hashtbl;
static int hshift, hmask;
static void hdel(int);

/* allocate hash table, and initialize chain */

int begbuf(pool,nsize,size)
  char *pool;
{
  int hsize;
  endbuf();
  hsize = size * 3 / 2;
  for (hmask = 16, hshift = 28; hmask < hsize; hmask <<= 1, --hshift);
  hashtbl = (BLOCK **)calloc(hmask,sizeof *hashtbl);
  if (hashtbl == 0) return -1;
  mru = (BLOCK *)pool;
  stat.bufs = size;
  stat.hsize = hmask--;
  mru->mru = mru->lru = mru;
  while (--size > 0) {
    BLOCK *bp = (BLOCK *)(pool += nsize);
    bp->mru = mru->mru;
    mru->mru->lru = bp;
    mru->mru = bp;
    bp->lru = mru;
    mru = bp;
  }
  return 0;
}

void endbuf() {
  if (hashtbl) {
    cfree((PTR)hashtbl);
    hashtbl = 0;
  }
}

static BLOCK *inuse[MAXBUF];
static int lastcnt = 0;
int curcnt = 0;

void btget(cnt)
  int cnt;		/* reserve cnt buffers */
{
  while (lastcnt > 0) {
    register BLOCK *bp = inuse[--lastcnt];
    /* assume chain is never empty */
    bp->mru = mru->mru;
    mru->mru->lru = bp;
    mru->mru = bp;
    bp->lru = mru;
    mru = bp;
  }
  curcnt = cnt;
}

BLOCK *getbuf(blk,mid)
  t_block blk;
  int mid;
{
  register BLOCK *bp;
  int hval;
  register int h;
  if (lastcnt >= curcnt) {
    char buf[64];
    sprintf(buf,"lastcnt(%d) < curcnt(%d)",lastcnt,curcnt);
    ASSERTFCN(buf,__FILE__,__LINE__);
  }

  assert(lastcnt < curcnt);
  hval = hash(blk+mid);
  ++stat.searches;
  while (++stat.probes, bp = hashtbl[hval]) {
    if (bp->blk == blk && bp->mid == mid) {
      /* remove from chain */
      if (bp == mru)
	mru = bp->lru;
      bp->mru->lru = bp->lru;
      bp->lru->mru = bp->mru;
      return inuse[lastcnt++] = bp;
    }
    if (--hval < 0) hval = hmask;
  }

  /* remove from chain and install new hash entry */
  hashtbl[hval] = bp = mru->mru;
  mru = bp->mru->lru = bp->lru;
  bp->lru->mru = bp->mru;

  if (bp->flags & BLK_MOD)
    writebuf(bp);

  h = hash(bp->blk+bp->mid);		/* compute old hash value */
  bp->blk = blk;	/* set new key so hash delete works */
  bp->mid = mid;

  /* remove old value from hash table */
  while (hashtbl[h]) {
    if (h != hval && hashtbl[h] == bp) {	/* find old entry */
      register int hval2;
      hashtbl[hval2 = h] = 0;
      for (;;) {
	BLOCK *np;
	if (--h < 0) h = hmask;
	np = hashtbl[h];
	if (np == 0) break;
	hval = hash(np->blk+np->mid);
	if (h > hval2 ? hval2 <= hval && hval < h : hval < h || hval >= hval2) {
	  hashtbl[hval2] = hashtbl[h];
	  hashtbl[hval2 = h] = 0;
	}
      }
      break;
    }
    if (--h < 0) h = hmask;
  }
  bp->blk = 0;		/* flag no match */
  return inuse[lastcnt++] = bp;
}

/* effectively swap contents of two blocks by just swapping pointers */

void swapbuf(np,bp)
  BLOCK *np, *bp;
{
  int hval;
  long blk;
  hval = hash(np->blk+np->mid);
  while (hashtbl[hval] && hashtbl[hval] != np)
    if (--hval < 0) hval = hmask;
  hashtbl[hval] = bp;
  hval = hash(bp->blk+bp->mid);
  while (hashtbl[hval] && hashtbl[hval] != bp)
    if (--hval < 0) hval = hmask;
  hashtbl[hval] = np;
  blk = np->blk;
  np->blk = bp->blk;
  bp->blk = blk;
  /* mid must be same! */
}
