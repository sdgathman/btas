/*
 * $Log$
 * Revision 2.6  2007/06/26 19:16:59  stuart
 * Check cache size at startup.
 *
 * Revision 2.5  2005/02/08 16:16:17  stuart
 * Port to ISO C++.
 *
 * Revision 2.4  2001/02/28 21:32:31  stuart
 * flush LRU buffer if we run out
 *
 * Revision 2.3  1999/10/03  04:49:36  stuart
 * mark buffers removed from touched
 *
 * Revision 2.2  1997/06/23  15:40:24  stuart
 * incremental flushing
 *
 * Revision 2.1  1996/12/17  16:44:12  stuart
 * C++ bufpool interface
 *
 * Revision 1.4  1995/05/31  20:42:24  stuart
 * prioritize buffer with BLK_LOW
 *
 * Revision 1.3  1994/03/28  20:08:59  stuart
 * log where btget errors occur
 *
 * Revision 1.2  1993/12/09  19:20:50  stuart
 * failsafe/improved modified buffer handling
 *
 */
#pragma implementation
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include "hash.h"
#include "node.h"
#include "btdev.h"

#ifndef ASSERTFCN
#define ASSERTFCN _assert
extern "C" void _assert(const char *,const char *,int);
#endif

int BufferPool::maxflush = MAXFLUSH;

/* allocate hash table, and initialize chain */

BufferPool::BufferPool(int size) {
  memset(&serverstats,0,sizeof serverstats);
  numtouch = 0;
  modcnt = 0;
  lastcnt = 0;
  curcnt = 0;
  mru = 0;
  maxtouch = size / 3;
  if (maxtouch > maxflush)
    maxtouch = maxflush;
  int hsize = size * 3 / 2;
  for (hmask = 16, hshift = 28; hmask < hsize; hmask <<= 1, --hshift);
  hashtbl = new BLOCK *[hmask];
  inuse = new BLOCK *[MAXBUF];
  touched = new BLOCK *[maxtouch];
  for (int i = 0; i < hmask; ++i)
    hashtbl[i] = 0;
  serverstats.hsize = hmask--;
  assert(maxtouch > MAXBUF);
}

bool BufferPool::ok(int size) {
  return (size / 3) > MAXBUF;
}

void BufferPool::put(BLOCK *bp) {
  if (mru) {
    bp->mru = mru->mru;
    mru->mru->lru = bp;
    mru->mru = bp;
    bp->lru = mru;
    mru = bp;
  }
  else {
    mru = bp;
    mru->mru = mru->lru = mru;
  }
}

BufferPool::~BufferPool() {
  delete [] hashtbl;
  delete [] inuse;
  delete [] touched;
}

void BufferPool::get(int cnt,const char *file,int line) {
  curfile = file;
  curline = line;
  get(cnt);
}

/* reserve cnt buffers */
void BufferPool::get(int cnt) {
  while (lastcnt > 0) {
    BLOCK *bp = inuse[--lastcnt];
#ifdef FAILSAFE
#if 0  // using TOUCH was unreliable, don't know why yet
    /* block is modified but not already touched */
    switch (bp->flags & (BLK_MOD | BLK_TOUCH | BLK_CHK)) {
    case BLK_MOD:
      int i;
      for (i = 0; i < numtouch; ++i)
	if (touched[i] == bp) {
	  bp->dump();
	  bp->flags |= BLK_TOUCH;
	  break;
	}
      if (i < numtouch) break;
      assert(numtouch < maxtouch);
      touched[numtouch++] = bp;
      // fall through since also newly modified
    case BLK_MOD | BLK_CHK:
      bp->flags |= BLK_TOUCH;
      ++modcnt;
      assert(modcnt <= numtouch);
      break;
    }
#else	// add to touched array if not already there
    if (bp->flags & BLK_MOD) {
      int mcnt = 0;
      int i;
      bp->flags |= BLK_TOUCH;
      for (i = 0; i < numtouch; ++i) {
	BLOCK *p = touched[i];
	if (p == bp) break;
	if (p->flags & BLK_MOD)
	  ++mcnt;
      }
      if (i == numtouch) {
	assert(numtouch < maxtouch);
	touched[numtouch++] = bp;
	modcnt = mcnt;
      }
      ++modcnt;
    }
#endif
#endif
    if (bp->flags & BLK_LOW)
      mru = mru->lru->lru;
    /* insert block in circular chain just after mru block in mru direction
       assume chain is never empty */
    /* FIXME: an assert for chain not empty should be simple, but
	      I can't think of it right now. */
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
#ifdef FAILSAFE
  if (cnt == 0) {
    /* Flush enough touched buffers to leave room for a worst
       case insert.  By incrmementally flushing the first fs in the
       touched list, we effectively round-robin the updates.
     */
    while (numtouch >= maxtouch - MAXBUF) {
      assert(numtouch > 0);
      sync(touched[0]->mid,MAXBUF);
    }
  }
#endif
}

BLOCK *BufferPool::find(t_block blk,short mid) {
  BLOCK *bp;
  int hval;
  int h;
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

  bp = mru->mru;	// least recently used buffer
#ifdef FAILSAFE
  while (bp->flags & (BLK_MOD | BLK_CHK)) {
    if (bp == mru) {	// shouldn't happen, but
      bp = bp->mru;	// ran out of unencumbered buffers, grab LRU
      bp->dump();	// log the encumbered buffer we grab and keep going
      writebuf(bp);
      break;
    }
    bp = bp->mru;
    //assert(bp != mru);
  }
#else
  if (bp->flags & BLK_MOD)
    writebuf(bp);
#endif
  /* remove from chain and install new hash entry */
  mru = bp->mru->lru = bp->lru;
  bp->lru->mru = bp->mru;

  h = hash(bp->blk+bp->mid);		/* compute old hash value */
  hashtbl[hval] = bp;
  bp->blk = blk;	/* set new key so hash delete works */
  bp->mid = mid;
  rehash(bp,h,hval);
  return inuse[lastcnt++] = bp;
}

void BufferPool::clear(BLOCK *bp) {
  assert((bp->flags & BLK_MOD) == 0);
  int h = hash(bp->blk+bp->mid);		/* compute old hash value */
  rehash(bp,h,-1);
}

void BufferPool::rehash(BLOCK *bp,int h,int hval) {
  while (hashtbl[h]) {
    if (hashtbl[h] == bp && h != hval) {	/* find old entry */
      int hval2;
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
}

/* effectively swap contents of two blocks by just swapping pointers */
/* NOTE: mid must be same! */

void BufferPool::swap(BLOCK *np,BLOCK *bp) {
  assert(np->mid == bp->mid);
  int hval = indexOf(np);
  int hval2 = indexOf(bp);
  hashtbl[hval] = bp;
  hashtbl[hval2] = np;
  long blk = np->blk;
  np->blk = bp->blk;
  bp->blk = blk;
}

/* return index in hashtbl of a BLOCK *.  WARNING: infinite loop results
 * if the BLOCK is not in fact in the hashtbl. */
int BufferPool::indexOf(BLOCK *np) const {
  int hval = hash(np->blk+np->mid);
  while (hashtbl[hval] && hashtbl[hval] != np)
    if (--hval < 0) hval = hmask;
  return hval;
}

// purge fully checkpointed buffers from touched list
int BufferPool::wait(short mid) {
  assert(mid >= 0);
  int rc = devtbl[mid].wait();
  assert(lastcnt == 0);
  /* if (lastcnt != 0) abort(); */
  if (rc == 0) {
    int newcnt = 0;
    int mcnt = 0;
    for (int i = 0; i < numtouch; ++i,++newcnt) {
      BLOCK *bp = touched[i];
      if (bp->flags & BLK_MOD)
	++mcnt;
      if (i > newcnt)
	touched[newcnt] = bp;
      if (bp->mid == mid) {
	bp->flags &= ~BLK_CHK;
	if ((bp->flags & BLK_MOD) == 0) {
	  --newcnt;			// remove from touched array
	  bp->flags &= ~BLK_TOUCH;
	}
      }
    }
    modcnt = mcnt;
    /* if (numtouch != newcnt) 
      fprintf(stderr,"%d buffers released\n",numtouch - newcnt);  */
    numtouch = newcnt;
  }
  return rc;
}

int BufferPool::sync(short mid,int lim) {
  int rc = wait(mid);
  if (rc) {
    fprintf(stderr,"wait failed, rc = %d\n",rc);
    return rc;
  }
  FDEV *dev = devtbl + mid;
  if (lim == 0)
    lim = numtouch;
  else if (dev->max() > 0)
    lim = dev->max();	// limit does not apply to failsafe
  int cnt = 0;
  for (int i = 0; i < numtouch; ++i) {
    BLOCK *bp = touched[i];
    if (bp->mid == mid && (bp->flags & BLK_MOD)) {
      int res = writebuf(bp);
      if (res == 0)
	++cnt;
      else
	rc = res;
      if (cnt > lim) {
	modcnt -= cnt;
	return rc;
      }
    }
  }
  modcnt -= cnt;
  int res = dev->sync(serverstats.checkpoints);
  if (res)
    rc = res;
  return rc;
}
