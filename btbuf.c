#define TRACE 1
#pragma implementation
/*
	physical I/O of blocks and cache maintenance
	11-04-88 keep track of free space
	02-17-89 multi-device filesystems
	05-18-90 hashed block lookup
$Log$
Revision 2.7  2007/06/26 19:16:59  stuart
Check cache size at startup.

Revision 2.6  2005/02/08 16:16:17  stuart
Port to ISO C++.

Revision 2.5  2001/02/28 21:26:00  stuart
make dumpbuf into method

 * Revision 2.4  1999/01/25  18:20:21  stuart
 * use extern const for version info in serverstats
 *
 * Revision 2.3  1997/06/23  15:28:13  stuart
 * move static vars to a BlockCache object
 *
 * Revision 2.2  1997/01/27  15:50:56  stuart
 * release buffers before unmount
 *
 * Revision 2.1  1995/12/12  23:36:31  stuart
 * C++ version
 *
 * Revision 1.6  1995/07/31  18:06:16  stuart
 * Use bufpool object
 *
 * Revision 1.5  1995/05/31  20:48:58  stuart
 * rename serverstats to avoid conflict with stats (stat symbolic link)
 *
 * Revision 1.4  1993/12/09  19:42:31  stuart
 * log bad roots on 205, clear free space
 *
 * Revision 1.3  1993/12/09  19:30:56  stuart
 * ANSI formatting, rough in checkpoint logic
 *
 * Revision 1.2  1993/02/23  18:49:33  stuart
 * fix assertion failure following 201 error
 *
 */
#if !defined(lint) && !defined(__MSDOS__)
static const char what[] = "$Id$";
#endif

#ifdef TRACE
#include <stdio.h>
#endif
//#include <unistd.h>
#include <time.h>
#include <assert.h>
#include <new>
#include "btserve.h"
#include "btbuf.h"
#include "btdev.h"
//#include <io.h>
#include "bterr.h"

FDEV devtbl[MAXDEV];

int BlockCache::btroot(t_block r,short mid) {
  if (mid < 0 || mid >= MAXDEV) return -1;
  dev = &devtbl[mid];	/* base device for current root node */
  if (!dev->isopen()) return -1;
  root = r;
  return 0;		/* mount id OK */
}

void BlockCache::btspace(int needed) {	/* check available space */
  int rc = dev->chkspace(needed,safe_eof);
  if (rc) btpost(rc);
}

void BlockCache::btcheck() { get(0); }

/* mount filesystem & return mount id */
short BlockCache::mount(const char *name) {
  int i, rc;
  for (i = 0; i < MAXDEV && devtbl[i].isopen(); ++i); /* find free mid */
  if (i >= MAXDEV)
    rc = BTERMTBL;		/* mount table full */
  else
    rc = devtbl[i].open(name);
  if (rc)
    btpost(rc);
  dev = devtbl + i;
  return i;
}

int BlockCache::unmount(short m) {		/* unmount filesystem */
  DEV *d = &devtbl[m];
  if (!d->isopen()) btpost(BTERMID);
  // FIXME: should we force unmount despite opens?
  //   the problem is that apps with open files may make additional
  //   calls against another filesystem mounted at the same mid.
  if (d->mcnt > 1) return 0;	/* still in use */
  get(0);
  --d->mcnt;
  int rc = sync(m);
  int res = sync(m);	// untouch buffers
  if (!rc) rc = res;
  res = d->close();
  if (!rc) rc = res;
  for (int i = 0; i < poolsize; ++i) {
    BLOCK *bp = (BLOCK *)(pool + i * nsize);
    if (bp->mid == m)
      clear(bp);
  }
  return rc;
}

int BlockCache::flush() {		/* flush buffers to disk */
  int rc = 0;
  for (int i = 0; i < MAXDEV; ++i) {	/* update all super blocks */
    int j = sync(i);
    if (!rc)
      rc = (j || devtbl[i].isclean()) ? j : BTERBUSY;
  }
  return rc;
}

/** Initialize buffer pool.  BLOCK should be a POD (no ctor, dtor, no virtual
   methods, no member pointers).
   @param size	maximum block size
   @param psize	size in chars of buffer pool
 */
BlockCache::BlockCache(int size,unsigned psize):
  maxrec(BLOCK::maxrec(size)),
  BufferPool(poolsize = psize / (nsize = offsetof(BLOCK,buf) + size))
{
  DEV::maxblksize = size;	/* save max block size */
  newcnt = 0;
  safe_eof = true;
  pool = new char[nsize * poolsize];
  if (pool == 0) return;
  for (int i = 0; i < poolsize; ++i)
    put(getblock(i)->init());
  serverstats.bufs = poolsize;
  btserve::curtime = time(&serverstats.uptime);
  serverstats.version = version_num;
}

bool BlockCache::ok(int bsize,unsigned cachesize) {
  return BufferPool::ok(cachesize / (offsetof(BLOCK,buf) + bsize));
}

BlockCache::~BlockCache() {
  for (int i = 0; i < MAXDEV; ++i) {	/* unmount all filesystems */
    if (devtbl[i].isopen()) {
      devtbl[i].mcnt = 1;
      unmount(i);
    }
  }
  delete [] pool;
}

BLOCK *BlockCache::btbuf(t_block blk) {
  register BLOCK *bp;
  if (blk == 0L)
    btpost(BTERROOT);	/* invalid root error */
  if ((bp = btread(blk))->buf.r.root != root) {
#if TRACE > 0
    fprintf(stderr,"201: btbuf(%08lX), root = %08lX\n", blk, root);
    bp->dump();
#endif
#if 0
    if (!(bp->flags & BLK_MOD)) {
      /* OS may have screwed up and given us the wrong block.
	 Don't laugh, it happens on several systems that I know of . . . */
      bp->blk = 0L;	/* say we don't know where this block came from */
      btflush();		/* before it's too late . . . */
    }
#endif
    btpost(BTERROOT);	/* invalid root error */
  }
  return bp;
}

BLOCK *BlockCache::btnew(short flag) {
  register BLOCK *bp;

  if (dev->free == 0L) {
    t_block blk = dev->newblk();
    short mid = dev - devtbl;
    if (blk == 0) btpost(BTERFULL);
    bp = find(blk,mid);
    bp->blk = blk;
  }
  else {		/* remove block from free list */
    bp = btread(dev->free);
    if (bp->buf.s.root == 0L)
      dev->free = bp->buf.s.son;
    else {
      if (bp->buf.s.root != dev->droot) {
	BLOCK *dp;
	onemore();
	dp = btread(bp->buf.s.root);	/* get root node */
	if (dp->buf.r.root) {		/* verify that it's deleted */
	  dev->free = 0; dev->space = 0;
	  fprintf(stderr,"free root = %08lX, %08lX\n",
		bp->buf.s.root,dp->buf.r.root);
	  btpost(BTERFREE);
	}
	dev->droot = bp->buf.s.root;	/* new droot */
      }
      dev->free = bp->buf.s.lbro;
    }
    --dev->space;
  }
  ++newcnt;

  bp->flags = (flag & ~BLK_CHK) | BLK_MOD;
  if (bp->flags&BLK_ROOT) {
    bp->np = (union node *)bp->buf.r.data;
    bp->buf.r.root = bp->blk;	/* make root node */
    bp->buf.r.son = 0L;
    bp->buf.r.stat.bcnt = 1L;	/* 1 block used so far */
    bp->buf.r.stat.rcnt = 0L;	/* no records yet */
    bp->buf.r.stat.links = 0;	/* no links yet */
    bp->buf.r.stat.opens = 0;	/* no opens yet */
    bp->buf.r.stat.id.user = 0;	/* owned by current user */
    bp->buf.r.stat.id.group = 0;	/* current group */
    bp->buf.r.stat.id.mode = 0;	/* no permissions, data file */
    /* file create time */
    btserve::curtime = bp->buf.r.stat.mtime = bp->buf.r.stat.atime
	= time(&bp->buf.r.stat.ctime);
  }
  else {
    bp->np = (union node *)bp->buf.s.data;
    bp->buf.r.root = root;
  }
  bp->count = 0;
  bp->setblksize(dev->blksize);
  return bp;
}

void BlockCache::btfree(BLOCK *bp) {		/* delete node */
  t_block left, right;
  left = bp->buf.r.son;
  bp->buf.r.root = 0;	/* mark deleted */
  bp->buf.r.son  = dev->free;
  bp->flags |= BLK_MOD;
  dev->free = bp->blk;
  if (bp->flags & BLK_ROOT) {	/* if root node, do "instant delete" */
    long droot = bp->blk, bcnt = bp->buf.r.stat.bcnt;
    while (bp->flags & BLK_STEM) {
      right = bp->ldptr(bp->cnt());
      get(2);
      bp = btread(left);
      if (bp->buf.s.root != droot) btpost(BTERROOT);
      left = bp->buf.s.son;
      bp->buf.l.lbro = dev->free;
      bp->flags |= BLK_MOD;
      bp = btread(right);
      if (bp->buf.s.root != droot) btpost(BTERROOT);
      dev->free = right;
    }
    dev->space += bcnt;	/* adjust space if successful */
  }
  else {
    ++dev->space;
    --newcnt;
  }
}

int BlockCache::writebuf(BLOCK *bp) {
  DEV *dp = devtbl + bp->mid;
  bp->flags &= ~(BLK_MOD | BLK_TOUCH);	/* mark unmodifed */
  bp->flags |= BLK_CHK;
  bp->np->setsize(bp->cnt() | (bp->flags & BLK_STEM));
  int rc = dp->write(bp->blk,bp->buf.data);
  ++serverstats.pwrites;
  bp->np->setsize(bp->buf.data + dp->blksize);
#ifdef MARKNODE
  if (bp->flags & BLK_ROOT)
    bp->buf.r.root |= MARKNODE;
  if (bp->flags & BLK_DIR)
    bp->buf.r.son |= MARKNODE;
#endif
  if (rc) {
    fprintf(stderr,"writebuf(mid=%d,blk=%08lX) returns %d\n",
	bp->mid,bp->blk,rc);
  }
  return rc;
}

BLOCK *BlockCache::btread(t_block blk) {
  int mid = dev - devtbl;
  ++serverstats.lreads;

  /* search for matching block */
  BLOCK *bp = find(blk,mid);
  if (bp->blk == blk)
    return bp;

  int rc = dev->read(blk,bp->buf.data);
  ++serverstats.preads;
  if (rc) btpost(rc);

  bp->blk = blk;
#ifdef MARKNODE
  bp->buf.r.root &= ~MARKNODE;
  bp->buf.r.son &= ~MARKNODE;
#endif
  if (blk == bp->buf.r.root) {
    bp->np = (union node *)bp->buf.r.data;
    bp->flags = BLK_ROOT;
  }
  else {
    bp->np = (union node *)bp->buf.s.data;
    bp->flags = 0;
  }
  bp->count = bp->np->size();
  bp->flags |= (bp->count&BLK_STEM);
  bp->count &= ~BLK_STEM;
  bp->np->setsize(bp->buf.data + dev->blksize);
  return bp;
}
