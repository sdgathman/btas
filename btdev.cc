#pragma implementation
#ifndef __MSDOS__
const char what[] = "$Revision$";
#endif
#include <stdio.h>
#include <unistd.h>
#include <malloc.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <time.h>
#include <bterr.h>
#include "btserve.h"
#include "btdev.h"
#include "btbuf.h"

unsigned short DEV::maxblksize = 1024;
char DEV::index = 0;
int DEV::extcnt = 0;
DEV::extent DEV::extbl[DEV::MAXEXT];

DEV::extent &DEV::ext(int i) const {
  return extbl[baseid + i];
}

DEV::DEV() {
  blksize = 0;		// mark unused
  dcnt = 0; baseid = 0;	// return BTERBLK on any access
}

DEV::~DEV() {
  close();
}

int DEV::read(t_block blk,char *buf) {
  //++serverstats.preads;
  int i = blk_dev(blk);			/* extent index */

  if (i >= dcnt || blk_off(blk) > ext(i).d.eod) {
#if TRACE > 0
    fprintf(stderr,"209: btread(%08lx), eod=%ld\n",
      blk, ext(i).d.eod);
#endif
    return BTERBBLK;
  }
#if TRACE > 1
  fprintf(stderr,"readbuf(%lx) pos=%ld\n",blk,blk_pos(blk));
#endif
  int fd = ext(i).fd;	/* OS file descriptor */

  errno = BTERBBLK;		/* bad block if eof or other problem */
  int rc = -2;
  int retry = 2;
  while (lseek(fd,blk_pos(blk),0) == -1
    || (rc = ::_read(fd,buf,blksize)) != blksize) {
#if 1 //TRACE > 0
    fprintf(stderr,"%d: btread(%08lx), fd=%d, rc=%d\n", errno, blk, fd, rc);
#endif
    if (!retry--)
      return errno;
    /* OS may have screwed up and given us a bogus error.
       Don't laugh, it happens on several systems that I know of . . . */
    // btflush();	// messes up pre-image failsafe
  }
  return 0;
}

int DEV::write(t_block blk,const char *buf) {
  //++serverstats.pwrites;
  int fd = ext(blk_dev(blk)).fd;
#if TRACE > 1
  fprintf(stderr,"fd=%d ",fd);
#endif
  int rc = 0;
  if (!flag) {
    flag = (unsigned char)~0u;
    if (writehdr())			/* flag update in progress */
      rc = errno;
  }
#if TRACE > 1
  fprintf(stderr,"pos=%ld blksize=%u ",blk_pos(blk),blksize);
#endif
  if (lseek(fd,blk_pos(blk),0) == -1 || ::_write(fd,buf,blksize) != blksize) {
    rc = errno;
    ++errcnt;
  }
  return rc;
}

/* FIXME: should ensure that 'needed' blocks are verified and kept
   on an in memory list so that corrupted free space cannot corrupt
   a file. */

int DEV::chkspace(int needed,bool safe_eof) {	/* check available space */
  if (needed <= space + newspace) return 0;	/* enough known space */
  char *zbuf = (char *)alloca(blksize);
  memset(zbuf,0,blksize);
  for (int cnt = 0; cnt < dcnt; ++cnt) {
    extent *p = &ext(cnt);
    if (p->d.eof == 0) {
      if (!safe_eof) return 0;	// expandable with no checking
      needed -= space + newspace;	/* additional blocks to reserve */
      for (t_block blk = p->d.eod + newspace; needed; --needed) {
	int rc = write(++blk,zbuf);
	if (rc) {
	  if (rc == ENOSPC || rc == EFBIG) {
	    p->d.eof = --blk;	/* don't try this extent ever again */
	    space += blk - p->d.eod;
	    break;
	  }
	  return rc;
	}
	++newspace;
      }
      if (needed == 0) return 0; // new space has been preallocated
    }
  }
  return BTERFULL;
}

bool DEV::valid(const btfs &d) {
  return !( d.hdr.blksize > maxblksize
	    || d.hdr.blksize < SECT_SIZE
	    || (signed char)d.hdr.baseid < 0
	    || d.hdr.magic != BTMAGIC
	  );
}

/* mount filesystem */

int DEV::open(const char *name,bool rdonly) {
  if (blksize) return BTEROPEN;
  newspace = 0;
  union {
    char buf[SECT_SIZE];
    struct btfs d;	/* file system header */
  } u;
  int mode = rdonly ? O_RDONLY + O_BINARY : O_RDWR + O_BINARY;

  int fd = ::_open(name,mode);	/* can we open the file ? */
  if (fd == -1) return errno;
  int rc = ::_read(fd,u.buf,sizeof u.buf);/* can we read the superblock ? */

  /* does the super block seem reasonable ? */
  superoffset = 0;
  if (rc == sizeof u.buf && !valid(u.d)) {
    rc = ::_read(fd,u.buf,sizeof u.buf);
    superoffset += SECT_SIZE;
  }
  blkoffset = (long)SECT_SIZE * u.d.hdr.root;
  extoffset = superoffset + SECT_SIZE;
  if (superoffset)	// new format rounds to blksize
    extoffset = (extoffset + blksize - 1) & ~(blksize - 1);
#if TRACE > 1
  fprintf(stderr,"superoffset = %d, blkoffset = %ld, extoffset = %ld\n",
	superoffset,blkoffset,extoffset);
#endif
  if (rc != sizeof u.buf)
    rc = errno;
  else if (!valid(u.d) || blkoffset < superoffset + SECT_SIZE)
    rc = BTERMBLK;
  /* is it already mounted ? */
  else if (u.d.hdr.mcnt && u.d.hdr.server != index)
    rc = BTERBUSY;
  else if (u.d.hdr.baseid < extcnt
	&& extbl[u.d.hdr.baseid].dev->isopen()
	&& u.d.hdr.mount_time == extbl[u.d.hdr.baseid].dev->mount_time) {
    rc = BTEROPEN;
  }

  /* is it possibly corrupted (dirty flag set) ? */
  else if (u.d.hdr.flag == (unsigned char)~0) {
    rc = BTERBMNT;
  }
  else if (extcnt + u.d.hdr.dcnt > MAXEXT)
    rc = BTERMTBL;		/* extent table full */

  else {
    /* everything seems OK, mount the filesystem */
    (btfhdr &)*this = u.d.hdr;	/* install new table entry */
    server = index;		/* set server ID */
    mcnt = 0;			/* reset mount count */
    btserve::curtime = time(&mount_time);		/* mark mount time */
    if (strlen(name) > sizeof u.d.dtbl->name) {
      for (const char *p = name; *p; ++p) {
  #ifdef __MSDOS__
	if (*p == '/' || *p == '\\')
  #else
	if (*p == '/')
  #endif
	  name = p + 1;
      }
    }
    strncpy(u.d.dtbl->name,name,sizeof u.d.dtbl->name);
    baseid = extcnt;
    for (int j = 0; j < dcnt; ++j) {
      extent *p = &ext(j);
      p->d = u.d.dtbl[j];
      p->dev = this;
      if (j) {			/* open extension files */
	char dname[sizeof p->d.name + 1];
	/* FIXME: prepend primary extent directory if no leading '/' */
	strncpy(dname,p->d.name,sizeof p->d.name)[sizeof p->d.name] = 0;
	fd = ::_open(p->d.name,mode);
	if (fd == -1) {
	  rc = errno ? errno : BTERMBLK;
	  while (--j) ::_close((*--p).fd);
	  blksize = 0;
	  return rc;
	}
	/* might want to read header & verify */
      }
      p->fd = fd;
    }
    extcnt += dcnt;
    ++mcnt;
    rc = 0;
    if (!rdonly)
      writehdr();
  }
  if (rc) {
  // FIXME: return error if checkpoint and we are not an FDEV
    ::_close(fd);		/* don't need new OS file */
    blksize = 0;
  }
  return rc;
}

int DEV::close() {
  if (blksize == 0) return 0;
  int rc = 0;
  // don't write super block - it'll mess it up if blocks haven't been
  // updated.
  blksize = 0;			/* free mount table entry */
  int i;
  for (i = 0; i < dcnt; ++i) {	/* close file & extents */
    extent *p = &ext(i);
    if (::_close(p->fd)) rc = errno;
  }
  for (i = baseid + dcnt; i < extcnt; ++i) {
    DEV *m = extbl[i].dev;
    extbl[baseid] = extbl[i];        /* compress extent table */
    if (m->baseid == i)
      m->baseid = baseid;
    ++baseid;
  }
  extcnt = baseid;
  return rc;
}

t_block DEV::newblk() {
  for (int i = 0; i < dcnt; ++i) {
    extent *p = &ext(i);
    if (p->d.eof == 0 || p->d.eod < p->d.eof) {
      t_block blk = mk_blk(i,++p->d.eod);
      if (p->d.eof) --space;
      return blk;
    }
  }
  return 0;
}

int DEV::gethdr(char *h,int len) const {
  struct btfs f;
  int i;
  memset(h,0,len);
  f.hdr = *this;
  for (i = 0; i < dcnt; ++i)
    f.dtbl[i] = ext(i).d;
  //i = offsetof(struct btfs,dtbl[i]);
  i = (char *)&f.dtbl[i] - (char *)&f; 
  if (i > len) i = len;
  memcpy(h,&f,i);
  return i;
}

int DEV::writehdr() const {
  char buf[SECT_SIZE];
  int i;
  gethdr(buf,sizeof buf);
#ifdef __MSDOS__
  if (flag == 0) {
    for (i = 0; i < dcnt; ++i)
      ::_close(dup(ext(i).fd));	/* update DOS dir */
  }
#endif
  i = ext(0).fd;	/* primary file */
  if (lseek(i,(long)superoffset,0) != superoffset
	|| ::_write(i,buf,sizeof buf) != sizeof buf)
    return errno;
  return 0;
}

int DEV::sync(long &) {
  if (blksize && flag) {
    flag = 0;
    return writehdr();
  }
  return 0;
}

int DEV::wait() {
  return 0;
}

long DEV::blk_pos(t_block b) const {
  long offset = blkoffset;
  if (blk_dev(b)) {
    b = blk_off(b);
    offset = extoffset;
  }
  return (b - 1) * blksize + offset;
}
