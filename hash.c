/* $Log$
 * Revision 1.3  1994/03/28  20:08:59  stuart
 * log where btget errors occur
 *
 * Revision 1.2  1993/12/09  19:20:50  stuart
 * failsafe/improved modified buffer handling
 *
 */
#include <stdio.h>
#include <assert.h>
#include <alloc.h>
#include "btbuf.h"
#include "hash.h"

/* NOTE, hmask not required if we force an unsigned shift.  This is not
   safe, however, unless we know the exact size on an unsigned long. */

#define hash(v)	((v) * 781316125L >> hshift & hmask)

static BLOCK *mru, **hashtbl;
static BLOCK *inuse[MAXBUF];
#ifdef FAILSAFE
static BLOCK *touched[MAXBUF];
static int numtouch = 0;
#endif
static int lastcnt = 0;

static int hshift, hmask;
int curcnt = 0;

/* allocate hash table, and initialize chain */

int begbuf(char *pool,int nsize,int size) {
  int hsize;
  endbuf();
  hsize = size * 3 / 2;
  for (hmask = 16, hshift = 28; hmask < hsize; hmask <<= 1, --hshift);
  hashtbl = (BLOCK **)calloc(hmask,sizeof *hashtbl);
  if (hashtbl == 0) return -1;
  mru = (BLOCK *)pool;
  serverstats.bufs = size;
  serverstats.hsize = hmask--;
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
    free(hashtbl);
    hashtbl = 0;
  }
}

#ifdef btget
#undef btget
static const char *curfile;
static int curline;
void btget_deb(int cnt,const char *file,int line) {
  curfile = file;
  curline = line;
  btget(cnt);
}
#endif

/* reserve cnt buffers */
void btget(int cnt) {
  while (lastcnt > 0) {
    BLOCK *bp = inuse[--lastcnt];
#ifdef FAILSAFE
    /* block is modified but not checkpointed or already touched */
    if ((bp->flags & (BLK_MOD | BLK_CHK | BLK_TOUCH)) == BLK_MOD) {
      touched[numtouch++] = bp;
      bp->flags |= BLK_TOUCH;
    }
#endif
    if (bp->flags & BLK_LOW)
      mru = mru->lru->lru;
    /* insert block in circular chain just after mru block in mru direction
       assume chain is never empty */
    bp->mru = mru->mru;
    mru->mru->lru = bp;
    mru->mru = bp;
    bp->lru = mru;
    /* move low priority block to just before end of mru chain */
    if (bp->flags & BLK_LOW) {
      bp->flags &= ~BLK_LOW;
      mru = bp->mru->mru;
    }
    else	/* make block most recently used */
      mru = bp;
  }
  curcnt = cnt;
}

BLOCK *getbuf(t_block blk,short mid) {
  register BLOCK *bp;
  int hval;
  register int h;
  if (lastcnt >= curcnt) {
    char buf[64];
    sprintf(buf,"lastcnt(%d) < curcnt(%d)",lastcnt,curcnt);
    if (curcnt >= MAXBUF)
      ASSERTFCN(buf,curfile,curline);
    fprintf(stderr,"%s, %s, line %d\n",buf,curfile,curline);
    ++curcnt;
    assert(lastcnt < curcnt);
  }
  hval = hash(blk+mid);
  ++serverstats.searches;
  while (++serverstats.probes, bp = hashtbl[hval]) {
    if (bp->blk == blk && bp->mid == mid) {
      /* check if already in use */
      for (h = 0; h < lastcnt; ++h)
	if (inuse[h] == bp) return bp;
      /* remove from chain */
      if (bp == mru)
	mru = bp->lru;
      bp->mru->lru = bp->lru;
      bp->lru->mru = bp->mru;
      return inuse[lastcnt++] = bp;
    }
    if (--hval < 0) hval = hmask;
  }

  bp = mru->mru;
#ifdef FAILSAFE
  while (bp->flags & BLK_MOD) {
    if (bp->flags & BLK_CHK) {
      writebuf(bp);	/* OK to update if checkpointed */
      break;
    }
    assert(bp != mru);
    bp = bp->mru;
  }
#else
  if (bp->flags & BLK_MOD)
    writebuf(bp);
#endif
  /* remove from chain and install new hash entry */
  hashtbl[hval] = bp;
  mru = bp->mru->lru = bp->lru;
  bp->lru->mru = bp->mru;

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
/* NOTE: mid must be same! */

void swapbuf(BLOCK *np,BLOCK *bp) {
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
}
