#define TRACE 1
/*
	physical I/O of blocks and cache maintenance
	11-04-88 keep track of free space
	02-17-89 multi-device filesystems
	05-18-90 hashed block lookup
$Log$
 */
#if !defined(lint) && !defined(__MSDOS__)
static char what[] = "%W%";
#endif

#define MAXFLUSH 60

#ifdef TRACE
#include <stdio.h>
#endif

#include <fcntl.h>
#include <time.h>
#include <errno.h>
#include <assert.h>
#include "btbuf.h"
#include <io.h>
#include "node.h"
#include "bterr.h"
#include "hash.h"

typedef struct btfhdr DEV;

static int writehdr(/**/ DEV * /**/);

static t_block root;		/* current root node */
static unsigned blksize;	/* max block size */
static char *pool;		/* buffer storage */
static int poolsize;		/* number of buffers */
static unsigned nsize;		/* sizeof a buffer */
static int sortout(/**/ BLOCK **, int /**/);

int newcnt = 0;		/* accumulates new blocks added/deleted */
long curtime;		/* BTAS time */
int maxrec;		/* maximum record size */
int safe_eof = 1;		/* safe OS eof processing */
char *workrec;
DEV devtbl[MAXDEV];	/* headers by mid */
struct btpstat stat;

static struct extent {
  int fd;		/* OS file descriptor */
  short mid;		/* index into fstbl */
  struct btdev d;	/* device info from header */
} extbl[MAXDEV];	/* extent table */

static int extcnt = 0;	/* extent table entries in use */

static struct btfhdr *dev;	/* current file system */

int btroot(r,mid)
  t_block r;		/* set current root node */
  short mid;		/* mount id */
{
  if (mid < 0 || mid >= MAXDEV) return -1;
  dev = &devtbl[mid];	/* base device for current root node */
  if (!dev->blksize) return -1;
  root = r;
  return 0;		/* mount id OK */
}

void btspace() {	/* check available space */
  int needed;		/* max number of blocks required */
  int cnt;
  needed = sp - stack + 2;	/* maybe we should pass needed . . . */
  if (needed <= dev->space) return;	/* enough known space */
  for (cnt = 0; cnt < dev->dcnt; ++cnt) {
    struct extent *p = &extbl[cnt + dev->baseid];
    errno = BTERFULL;
    if (p->d.eof == 0) {
      if (safe_eof) {
	needed -= dev->space;	/* additional blocks to reserve */
	assert((unsigned)needed <= MAXBUF);
	btget(needed);
	while (needed) {
	  t_block savfree = dev->free;
	  BLOCK *bp;
	  int rc;
	  dev->free = 0L;
	  bp = btnew(0);
	  dev->free = savfree;
	  if (rc = writebuf(bp)) {
	    if (rc == ENOSPC) {
	      p->d.eof = --p->d.eod;	/* don't try this extent ever again */
	      --newcnt;
	      break;
	    }
	    btpost(rc);
	  }
	  btfree(bp);
	  --needed;
	}
	if (needed) continue;		/* try another extent */
      }
      return;	/* expandable */
    }
  }
  btpost(errno);
}

/* We need to read/write SECT_SIZE bytes for raw devices.  We should have
   an additional verification field in sector 1. */

short btmount(name)	/* mount filesystem & return mount id */
  char *name;           /* OS file name */
{
  register int i;
  int fd, rc;
  union {
    char buf[SECT_SIZE];
    struct btfs d;	/* file system header */
  } u;

  fd = _open(name,O_RDWR+O_BINARY);	/* can we open the file ? */
  if (fd == -1) btpost(errno);
  rc = _read(fd,u.buf,sizeof u.buf);	/* can we read the superblock ? */
  if (rc == -1)
    rc = errno;

  /* does the super block seem reasonable ? */
  else if (rc != sizeof u.buf || u.d.hdr.blksize > blksize
	|| u.d.hdr.blksize < SECT_SIZE || (int)u.d.hdr.baseid < 0
	|| u.d.hdr.magic != BTMAGIC)
    rc = BTERMBLK;	/* invalid filesystem */

  /* is it already mounted ? */
  else if (u.d.hdr.baseid < extcnt
	&& (i = extbl[u.d.hdr.baseid].mid,devtbl[i].blksize)
	&& u.d.hdr.mount_time == devtbl[i].mount_time) {
    (void)_close(fd);		/* don't need new OS file */
    rc = 0;
  }

  /* is it possibly corrupted (dirty flag set) ? */
  else if (u.d.hdr.flag)
    rc = BTERBMNT;

  /* everything seems OK, mount the filesystem */
  else {
    for (i = 0; i < MAXDEV && devtbl[i].blksize; ++i); /* find free mid */
    if (i >= MAXDEV || extcnt + u.d.hdr.dcnt > MAXDEV)
      rc = BTERMTBL;		/* mount table full */
    else {			/* build table entries */
      u.d.hdr.baseid = extcnt;	/* base extent table entry */
      u.d.hdr.mcnt = 0;			/* reset mount count */
      if (strlen(name) > sizeof u.d.dtbl->name) {
	char *p;
	for (p = name; *p; ++p) {
#ifdef __MSDOS__
	  if (*p == '/' || *p == '\\')
#else
	  if (*p == '/')
#endif
	    name = p + 1;
	}
      }
      strncpy(u.d.dtbl->name,name,sizeof u.d.dtbl->name);
      curtime = time(&u.d.hdr.mount_time);	/* mark mount time */
      if (lseek(fd,0L,0) || _write(fd,u.buf,sizeof u.buf) != sizeof u.buf)
	rc = errno;			/* couldn't update file */
      else {
	int j;
	for (j = 0; j < u.d.hdr.dcnt; ++j) {
	  struct extent *p = &extbl[extcnt+j];
	  p->d = u.d.dtbl[j];
	  p->mid = i;
	  if (j) {			/* open extension files */
	    char dname[sizeof p->d.name + 1];
	    /* FIXME: prepend primary extent directory if no leading '/' */
	    strncpy(dname,p->d.name,sizeof p->d.name)[sizeof p->d.name] = 0;
	    fd = _open(p->d.name,O_RDWR+O_BINARY);
	    if (fd == -1) {
	      rc = errno;
	      while (--j) (void)_close((*--p).fd);
	      btpost(rc);
	    }
	    /* might want to read header & verify */
	  }
	  p->fd = fd;
	}
	devtbl[i] = u.d.hdr;		/* install new table entry */
	extcnt += u.d.hdr.dcnt;
	rc = 0;
      }
    }
  }
  if (rc) {
    (void)_close(fd);
    btpost(rc);
  }
  dev = devtbl + i;
  ++dev->mcnt;		/* increment mount count */
  return i;
}

int btumount(m)		/* unmount filesystem */
  short m;
{
  register int i;
  int rc = 0;
  register BLOCK *bp;
  register struct btfhdr *d = &devtbl[m];
  if (d->blksize == 0) btpost(BTERMID);
  if (--d->mcnt > 0) return 0;	/* still in use */
  for (i = 0; i < poolsize; ++i) {
    bp = (BLOCK *)(pool + i * nsize);
    if (bp->mid == m) {
      if (bp->flags & BLK_MOD)
	if (writebuf(bp))	/* flush this filesys buffers */
	  rc = errno;
      bp->blk = 0L;		/* mark this filesys buffers unused */
    }
  }
  if (d->flag) {
    d->flag = (rc != 0);
    if (writehdr(d))
      rc = errno;
  }
  d->blksize = 0;			/* free mount table entry */
  for (i = 0; i < d->dcnt; ++i) {	/* close file & extents */
    struct extent *p = &extbl[d->baseid + i];
    if (_close(p->fd)) rc = errno;
  }
  for (i = d->baseid + d->dcnt; i < extcnt; ++i) {
    extbl[d->baseid] = extbl[i];        /* compress extent table */
    if (devtbl[extbl[i].mid].baseid == i)
      devtbl[extbl[i].mid].baseid = d->baseid;
    ++d->baseid;
  }
  extcnt = d->baseid;
  return rc;
}

static int sortout(a,n)
  BLOCK **a;		/* block pointers to output */
  int n;
{
  int gap,i,rc;
  for (gap = n/2; gap > 0; gap /= 2) {
    for (i = gap; i < n; ++i) {
      int j;
      for (j = i - gap; j >= 0; j -= gap) {
	BLOCK *temp;
	int k = j + gap;
	rc = a[j]->mid - a[k]->mid;
	if (rc < 0) break;
	if (rc == 0 && a[j]->blk <= a[k]->blk) break;
	temp = a[j];
	a[j] = a[k];
	a[k] = temp;
      }
    }
  }
  rc = 0;
  for (i = 0; i < n; ++i) {
    int wc;
    if (wc = writebuf(a[i]))
      rc = wc;
  }
  stat.fbufs += n;
  return rc;
}

int btflush()			/* flush buffers to disk */
{
  register int i;
  int n = 0;
  int rc = 0,wc;
  BLOCK *a[MAXFLUSH];
  for (i = 0; i < poolsize; ++i) {
    register BLOCK *bp;
    bp = (BLOCK *)(pool + i * nsize);
    if (bp->flags & BLK_MOD) {
      a[n++] = bp;
#ifndef __MSDOS__
      if (n == MAXFLUSH) {
	if (rc = sortout(a,n))
	  return rc;
	return BTERBUSY;	/* incomplete flush */
      }
#endif
    }
  }
  if (wc = sortout(a,n))
    rc = wc;
  ++stat.fcnt;
  for (i = 0; i < MAXDEV; ++i)	/* update all super blocks */
    if (devtbl[i].blksize && devtbl[i].flag) {
      devtbl[i].flag = 0;
      if (writehdr(&devtbl[i]))
	rc = errno;
    }
#ifndef __MSDOS__
  sync();			/* flush host file system */
#endif
  return rc;
}

int btbegin(size,psize)		/* initialize buffer pool */
  int size;			/* maximum buffer size */
  unsigned psize;		/* total buffer pool size */
{
  register int i;
  register BLOCK *p;
  blksize = size;		/* save max block size */
  maxrec = (size - 18)/2 - 1;
  nsize = sizeof *p - sizeof p->buf + size;
  poolsize = psize / nsize;
  pool = malloc(nsize * poolsize + maxrec);
  if (pool == 0) return -1;
  for (i = 0; i < poolsize; ++i) {
    p = (BLOCK *)(pool + i * nsize);
    p->flags = 0;
    p->blk = 0;
    p->mid = 0;
    p->np = (union node *)p->buf.r.data;
  }
  workrec = pool + poolsize * nsize;
  if (begbuf(pool,nsize,poolsize)) {
    free(pool);
    return -1;
  }
  return 0;
}

void btend()
{
  register int i;
  for (i = 0; i < MAXDEV; ++i) {	/* unmount all filesystems */
    if (devtbl[i].blksize) {
      devtbl[i].mcnt = 1;
      (void)btumount(i);
    }
  }
  endbuf();
  (void)free(pool);		/* release buffer pool */
}

BLOCK *btbuf(blk)
  t_block blk;
{
  register BLOCK *bp;
  if (blk == 0L)
    btpost(BTERROOT);	/* invalid root error */
  if ((bp = btread(blk))->buf.r.root != root) {
#if TRACE > 0
    fprintf(stderr,"201: btbuf(%08lX), root = %08lX\n", blk, root);
    fprintf(stderr,"blk = %08lX, mid = %d, cnt = %d, %s %s %s %s\n",
      bp->blk, bp->mid, bp->count,
      (bp->flags & BLK_MOD) ? "BLK_MOD" : "",
      (bp->flags & BLK_ROOT) ? "BLK_ROOT" : "",
      (bp->flags & BLK_STEM) ? "BLK_STEM" : "",
      (bp->flags & BLK_NEW) ? "BLK_NEW" : ""
    );
    fprintf(stderr,"root = %08lX, son/rbro = %08lX\n",
      bp->buf.r.root, bp->buf.r.son);
#endif
    if (!(bp->flags & BLK_MOD)) {
      /* OS may have screwed up and given us the wrong block.
	 Don't laugh, it happens on several systems that I know of . . . */
      bp->blk = 0L;	/* say we don't know where this block came from */
      btflush();		/* before it's too late . . . */
    }
    btpost(BTERROOT);	/* invalid root error */
  }
  return bp;
}

BLOCK *btnew(flag)
  short flag;		/* type of node to create */
{
  register BLOCK *bp;

  if (dev->free == 0L) {
    int i;		/* search extents for space */
    t_block blk = 0;
    short mid = dev - devtbl;
    for (i = 0; i < dev->dcnt; ++i) {
      register struct extent *p = &extbl[dev->baseid + i];
      if (p->d.eof == 0 || p->d.eod < p->d.eof) {
	blk = mk_blk(i,++p->d.eod);
	if (p->d.eof) --dev->space;
	break;
      }
    }
    if (blk == 0) btpost(BTERFULL);
    bp = getbuf(blk,mid);
    bp->blk = blk;
  }
  else {		/* remove block from free list */
    bp = btread(dev->free);
    if (bp->buf.s.root == 0L)
      dev->free = bp->buf.s.son;
    else {
      if (bp->buf.s.root != dev->droot) {
	BLOCK *dp;
	++curcnt;
	dp = btread(bp->buf.s.root);	/* get root node */
	if (dp->buf.r.root)		/* verify that it's deleted */
	  btpost(BTERFREE);
	dev->droot = bp->buf.s.root;	/* new droot */
      }
      dev->free = bp->buf.s.lbro;
    }
    --dev->space;
  }
  ++newcnt;

  /* BLK_NEW below might be incorrect */
  bp->flags = flag | BLK_NEW | BLK_MOD | BLK_ACC;
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
  *bp->np->tab = bp->buf.data + dev->blksize - (char *)bp->np->dat;
  return bp;
}

void btfree(bp)		/* delete node */
  BLOCK *bp;
{
  t_block left, right;
  left = bp->buf.r.son;
  bp->buf.r.root = 0;	/* mark deleted */
  bp->buf.r.son  = dev->free;
  bp->flags |= BLK_MOD;
  dev->free = bp->blk;
  if (bp->flags & BLK_ROOT) {	/* if root node, do "instant delete" */
    long droot = bp->blk, bcnt = bp->buf.r.stat.bcnt;
    while (bp->flags & BLK_STEM) {
      node_ldptr(bp,node_count(bp),&right);
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

int gethdr(dp,h,len)
  const DEV *dp;
  char *h;
  int len;		/* maximum buffer size */
{
  struct btfs f;
  register int i;
  f.hdr = *dp;
  for (i = 0; i < dp->dcnt; ++i)
    f.dtbl[i] = extbl[dp->baseid + i].d;
  i = (char *)&f.dtbl[i] - (char *)&f;
  if (i > len) i = len;
  (void)memcpy(h,(char *)&f,i);
  return i;
}

static int writehdr(dp)
  DEV *dp;
{
  union {
    char buf[SECT_SIZE];
    struct btfs d;
  } u;
  register int i;
  (void)memset(u.buf,0,sizeof u.buf);
  u.d.hdr = *dp;
  for (i = 0; i < dp->dcnt; ++i)
    u.d.dtbl[i] = extbl[dp->baseid + i].d;
  if (dp->flag == 0) {
    for (i = 0; i < dp->dcnt; ++i)
      (void)_close(dup(extbl[dp->baseid + i].fd));	/* update DOS dir */
  }
  i = extbl[dp->baseid].fd;	/* primary file */
  return (lseek(i,0L,0) || _write(i,u.buf,sizeof u.buf) != sizeof u.buf);
}

int writebuf(bp)
  BLOCK *bp;
{
  int fd; t_block blk;
  DEV *dp;
  int rc = 0;
  ++stat.pwrites;
  dp = devtbl + bp->mid;
  fd = extbl[dp->baseid + blk_dev(bp->blk)].fd;
  blk = blk_off(bp->blk);
  bp->flags &= ~(BLK_MOD|BLK_NEW);	/* mark unmodifed oldtimer */
#if TRACE > 1
  fprintf(stderr,"fd=%d ",fd);
#endif
  if (!dp->flag) {
    dp->flag = ~0;
    if (writehdr(dp))			/* flag update in progress */
      rc = errno;
  }
  *bp->np->tab = bp->count | (bp->flags & BLK_STEM);
#ifdef MARKNODE
  if (bp->flags & BLK_ROOT)
    bp->buf.r.root |= MARKNODE;
  if (bp->flags & BLK_DIR)
    bp->buf.r.son |= MARKNODE;
#endif
#if TRACE > 1
  fprintf(stderr,"pos=%ld blksize=%u ",blk_pos(blk,dp->blksize),dp->blksize);
#endif
  if ( lseek(fd,blk_pos(blk,dp->blksize),0) == -1
    || _write(fd,bp->buf.data,dp->blksize) != dp->blksize)
    rc = errno;
  *bp->np->tab = bp->buf.data + dp->blksize - (char *)bp->np->dat;
  if (rc) {
    fprintf(stderr,"writebuf(mid=%d,blk=%08lX) returns %d\n",
	bp->mid,bp->blk,rc);
    ++dp->errcnt;
  }
  return rc;
}

BLOCK *btread(blk)
  t_block blk;
{
  register BLOCK *bp;
  t_block dblk;
  int fd, rc, retry;
  unsigned short i;
  short mid = dev - devtbl;
  ++stat.lreads;

  /* search for matching block */
  bp = getbuf(blk,mid);
  if (bp->blk == blk)
    return bp;

  ++stat.preads;
#if TRACE > 1
  fprintf(stderr,"readbuf(%d,%lx)\n",mid,blk);
#endif
  i = blk_dev(blk);			/* extent index */
  dblk = blk_off(blk);			/* block offset */

  if (i >= dev->dcnt || dblk > extbl[dev->baseid + i].d.eod) {
#if TRACE > 0
    fprintf(stderr,"209: btread(%08lx), eod=%ld\n",
      blk, extbl[dev->baseid + i].d.eod);
#endif
    btpost(BTERBBLK);
  }
  fd = extbl[dev->baseid + i].fd;	/* OS file descriptor */

  errno = BTERBBLK;		/* bad block if eof or other problem */
  rc = -2; retry = 2;
  while (lseek(fd,blk_pos(dblk,dev->blksize),0) == -1
    || (rc = _read(fd,bp->buf.data,dev->blksize)) != dev->blksize) {
#if TRACE > 0
    fprintf(stderr,"%d: btread(%08lx), fd=%d, rc=%d\n", errno, blk, fd, rc);
#endif
    if (!retry--)
      btpost(errno);	/* post I/O error */
    /* OS may have screwed up and given us a bogus error.
       Don't laugh, it happens on several systems that I know of . . . */
    btflush();
  }

  bp->blk = blk;
#ifdef MARKNODE
  bp->buf.r.root &= ~MARKNODE;
  bp->buf.r.son &= ~MARKNODE;
#endif
  if (blk == bp->buf.r.root) {
    bp->np = (union node *)bp->buf.r.data;
    bp->flags = BLK_ROOT | BLK_NEW;
  }
  else {
    bp->np = (union node *)bp->buf.s.data;
    bp->flags = BLK_NEW;
  }
  bp->count = *bp->np->tab;
  bp->flags |= (bp->count&BLK_STEM);
  bp->count &= ~BLK_STEM;
  *bp->np->tab = bp->buf.data + dev->blksize - (char *)bp->np->dat;
  return bp;
}
