#define TRACE 1
/*
	physical I/O of blocks and cache maintenance
	11-04-88 keep track of free space
	02-17-89 multi-device filesystems
	05-18-90 hashed block lookup
$Log$
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
static char what[] = "$Id$";
#endif

#ifdef TRACE
#include <stdio.h>
#endif
//#include <unistd.h>
#include <time.h>
#include <assert.h>
#include "hash.h"
#include "btdev.h"
//#include <io.h>
#include "bterr.h"

static t_block root;		/* current root node */
static char *pool;		/* buffer storage */
static int poolsize;		/* number of buffers */
static unsigned nsize;		/* sizeof a buffer */

BufferPool *bufpool;
int newcnt = 0;		/* accumulates new blocks added/deleted */
long curtime;		/* BTAS time */
int maxrec;		/* maximum record size */
int safe_eof = 1;	/* safe OS eof processing */
char *workrec;
struct btpstat serverstats;

static FDEV *dev;	/* current file system */
FDEV devtbl[MAXDEV];

int btroot(t_block r,short mid) {
  if (mid < 0 || mid >= MAXDEV) return -1;
  dev = &devtbl[mid];	/* base device for current root node */
  if (!dev->isopen()) return -1;
  root = r;
  return 0;		/* mount id OK */
}

void btspace() {	/* check available space */
  int needed = sp - stack + 2;
  int rc = dev->chkspace(needed);
  if (rc) btpost(rc);
}

short btmount(const char *name) { /* mount filesystem & return mount id */
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

int btumount(short m) {		/* unmount filesystem */
  DEV *d = &devtbl[m];
  if (!d->isopen()) btpost(BTERMID);
  // FIXME: should we force unmount despite opens?
  //   the problem is that apps with open files may make additional
  //   calls against another filesystem mounted at the same mid.
  if (d->mcnt > 1) return 0;	/* still in use */
  --d->mcnt;
  int rc = bufpool->sync(m);
  bufpool->wait(m);	// untouch buffers
  int res = d->close();
  for (int i = 0; i < poolsize; ++i) {
    BLOCK *bp = (BLOCK *)(pool + i * nsize);
    if (bp->mid == m)
      bufpool->clear(bp);
  }
  if (!rc) rc = res;
  return rc;
}

int btflush() {			/* flush buffers to disk */
  int rc = 0;
  for (int i = 0; i < MAXDEV; ++i) {	/* update all super blocks */
    int j = bufpool->sync(i);
    if (!rc) rc = j;
  }
  return rc;
}

int btbegin(int size,unsigned psize) {		/* initialize buffer pool */
  BLOCK *p;
  DEV::maxblksize = size;	/* save max block size */
  maxrec = (size - 18)/2 - 1;
  nsize = sizeof *p - sizeof p->buf + size;
  poolsize = psize / nsize;
  pool = new char[nsize * poolsize + maxrec];
  if (pool == 0) return -1;
  bufpool = new BufferPool(poolsize);
  for (int i = 0; i < poolsize; ++i) {
    p = (BLOCK *)(pool + i * nsize);
    p->flags = 0;
    p->blk = 0;
    p->mid = 0;
    p->np = (union node *)p->buf.r.data;
    bufpool->put(p);
  }
  serverstats.bufs = poolsize;
  workrec = pool + poolsize * nsize;
  time(&serverstats.uptime);
  serverstats.version = 112;
  return 0;
}

void btend() {
  for (int i = 0; i < MAXDEV; ++i) {	/* unmount all filesystems */
    if (devtbl[i].isopen()) {
      devtbl[i].mcnt = 1;
      btumount(i);
    }
  }
  delete bufpool;
  delete [] pool;
}

void btdumpbuf(const BLOCK *bp) {
  fprintf(stderr,"blk = %08lX, mid = %d, cnt = %d,%s%s%s%s%s\n",
    bp->blk, bp->mid, bp->cnt(),
    (bp->flags & BLK_MOD) ? " BLK_MOD" : "",
    (bp->flags & BLK_TOUCH) ? " BLK_TOUCH" : "",
    (bp->flags & BLK_ROOT) ? " BLK_ROOT" : "",
    (bp->flags & BLK_STEM) ? " BLK_STEM" : "",
    (bp->flags & BLK_CHK) ? " BLK_CHK" : ""
  );
  fprintf(stderr,"root = %08lX, son/rbro = %08lX\n",
    bp->buf.r.root, bp->buf.r.son);
}

BLOCK *btbuf(t_block blk) {
  register BLOCK *bp;
  if (blk == 0L)
    btpost(BTERROOT);	/* invalid root error */
  if ((bp = btread(blk))->buf.r.root != root) {
#if TRACE > 0
    fprintf(stderr,"201: btbuf(%08lX), root = %08lX\n", blk, root);
    btdumpbuf(bp);
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

BLOCK *btnew(short flag) {
  register BLOCK *bp;

  if (dev->free == 0L) {
    t_block blk = dev->newblk();
    short mid = dev - devtbl;
    if (blk == 0) btpost(BTERFULL);
    bp = bufpool->find(blk,mid);
    bp->blk = blk;
  }
  else {		/* remove block from free list */
    bp = btread(dev->free);
    if (bp->buf.s.root == 0L)
      dev->free = bp->buf.s.son;
    else {
      if (bp->buf.s.root != dev->droot) {
	BLOCK *dp;
	bufpool->onemore();
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

  bp->flags = (flag & ~(BLK_CHK|BLK_TOUCH)) | BLK_MOD;
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
    curtime = time(&bp->buf.r.stat.ctime);	/* file create time */
    bp->buf.r.stat.mtime = bp->buf.r.stat.atime = curtime;
  }
  else {
    bp->np = (union node *)bp->buf.s.data;
    bp->buf.r.root = root;
  }
  bp->count = 0;
  bp->np->setsize(bp->buf.data + dev->blksize);
  return bp;
}

void btfree(BLOCK *bp) {		/* delete node */
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
      btget(2);
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

int writebuf(BLOCK *bp) {
  DEV *dp = devtbl + bp->mid;
  bp->flags &= ~(BLK_MOD | BLK_TOUCH);	/* mark unmodifed */
  bp->flags |= BLK_CHK;
  bp->np->setsize(bp->cnt() | (bp->flags & BLK_STEM));
  int rc = dp->write(bp->blk,bp->buf.data);
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

BLOCK *btread(t_block blk) {
  int mid = dev - devtbl;
  ++serverstats.lreads;

  /* search for matching block */
  BLOCK *bp = bufpool->find(blk,mid);
  if (bp->blk == blk)
    return bp;

  int rc = dev->read(blk,bp->buf.data);
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
