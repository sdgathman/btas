/*
    Translate block IO to filesystem extents.

    Copyright (C) 1985-2013 Business Management Systems, Inc

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/
#pragma implementation
#ifndef __MSDOS__
const char what[] = "$Revision$";
#endif
#define _LARGEFILE64_SOURCE
#include <stdio.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <unistd.h>
#include <malloc.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <time.h>
#include <alloca.h>
#include <bterr.h>
#include <assert.h>
#include "btserve.h"
#include "btdev.h"
#include "btbuf.h"

#if _LFS64_LARGEFILE
#define OPEN open64
#else
#warn "No LARGEFILE support"
#define O_LARGEFILE 0
#define OPEN _open
#endif

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

  if (blk < 0 || i >= dcnt || blk_off(blk) > ext(i).d.eod) {
#if TRACE > 0
    fprintf(stderr,"209: btread(%08lx), eod=%ld\n",
      blk, ext(i).d.eod);
#endif
    return BTERBBLK;
  }
#if TRACE > 1
  fprintf(stderr,"readbuf(%lx) pos=%lld\n",blk,blk_pos(blk));
#endif
  int fd = ext(i).fd;	/* OS file descriptor */

  errno = BTERBBLK;		/* bad block if eof or other problem */
  int rc = -2;
  int retry = 2;
  while (lseek64(fd,blk_pos(blk),0) == -1
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
  fprintf(stderr,"pos=%lld blksize=%u ",blk_pos(blk),blksize);
#endif
  if (lseek64(fd,blk_pos(blk),0) == -1 || ::_write(fd,buf,blksize) != blksize) {
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
      if (dcnt > 1) {
	if (p->d.eod + needed - space > MAXBLK) continue;
      }
      else
	if (p->d.eod + needed - space > maxblk) continue;
      if (!safe_eof) return 0;	// expandable with no checking
      needed -= space + newspace;	/* additional blocks to reserve */
      for (t_block blk = p->d.eod + newspace; needed; --needed) {
	if (dcnt > 1 && blk >= MAXBLK || blk >= maxblk) {
	  p->d.eof = blk;
	  space += blk - p->d.eod;
	  return BTERFULL;
	}
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
  int mode = (rdonly?O_RDONLY:O_RDWR)+O_BINARY+O_LARGEFILE;
  struct rlimit rlim;
  int rc = getrlimit(RLIMIT_FSIZE,&rlim);
  if (rc == -1) return errno;

  int fd = ::OPEN(name,mode);	/* can we open the file ? */
  if (fd == -1) return errno;
  rc = ::_read(fd,u.buf,sizeof u.buf);/* can we read the superblock ? */

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
  fprintf(stderr,
    "superoffset = %d, blkoffset = %ld, extoffset = %ld, blksize=%d\n",
    superoffset,blkoffset,extoffset,blksize
  );
#endif
  int64_t maxsects = 0xFFFFFFFFLL;	// max sector in 4k ext2/3
  if (rlim.rlim_cur != RLIM_INFINITY)
    maxsects = rlim.rlim_cur / SECT_SIZE;

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
#if !_LFS64_LARGEFILE
  else if (blk_pos(u.d.dtbl->eof)-1 > 0x7FFFFFFFL) {
    rc = BTERFULL;
  }
#endif
  else {
    
    /* everything seems OK, mount the filesystem */
    (btfhdr &)*this = u.d.hdr;	/* install new table entry */
    server = index;		/* set server ID */
    mcnt = 0;			/* reset mount count */
    mount_time = time(&btserve::curtime);		/* mark mount time */
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
    maxblk = blk_sects(0,maxsects);
    baseid = extcnt;
    for (int j = 0; j < dcnt; ++j) {
      extent *p = &ext(j);
      p->d = u.d.dtbl[j];
      p->dev = this;
      if (j) {			/* open extension files */
	char dname[sizeof p->d.name + 1];
	/* FIXME: prepend primary extent directory if no leading '/' */
	strncpy(dname,p->d.name,sizeof p->d.name)[sizeof p->d.name] = 0;
	fd = ::OPEN(p->d.name,mode);
	if (fd == -1) {
	  rc = errno ? errno : BTERMBLK;
	  --j;
	  while (j-- >= 0) ::_close((*p--).fd);
	  blksize = 0;
	  return rc;
	}
	/* might want to read header & verify */
      }
      /* check ulimit */
      if (rlim.rlim_cur != RLIM_INFINITY) {
	struct stat64 st;
	int rc = fstat64(fd,&st);		/* can we stat the file ? */
	if (rc == -1) {
	  rc = errno;
	  while (j-- >= 0) ::_close((*p--).fd);
	  blksize = 0;
	  return rc;
	}
	/* Check ulimit for a regular file. */
	if (S_ISREG(st.st_mode)) {
	  t_block ulim = blk_sects(j,maxsects);
	  if (ulim < p->d.eod) {
	    rc = BTERFULL;
	    while (j-- >= 0) ::_close((*p--).fd);
	    blksize = 0;
	    return rc;
	  }
	  if (ulim < p->d.eof || p->d.eof == 0) {
	    if (p->d.eof > 0)
	      space -= p->d.eof - ulim;
	    else
	      space += ulim - p->d.eod;
	    p->d.eof = ulim;
	  }
	}
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
    if ((p->d.eof == 0 && (dcnt == 1 || p->d.eod < MAXBLK))
		|| p->d.eod < p->d.eof) {
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

int DEV::sync_all() const {
  int rc = 0;
#ifdef m88k
  if (sync())
    rc = errno;
#else
  for (int i = 0; i < dcnt; ++i)
#ifdef __MSDOS__
    if (::_close(dup(ext(i).fd)))	/* update DOS dir */
      rc = errno;	
#else
    if (fsync(ext(i).fd))	
      rc = errno;
#endif
#endif
  return rc;
}

int DEV::writehdr() const {
  char buf[SECT_SIZE];
  gethdr(buf,sizeof buf);
  if (flag == 0) sync_all(); /* ensure updates written before clean flag */
  int i = ext(0).fd;	/* primary file */
  if (lseek(i,(long)superoffset,0) != superoffset
	|| ::_write(i,buf,sizeof buf) != sizeof buf)
    return errno;
#ifdef m88k
  if (flag != 0) sync(); /* ensure dirty flag written before updates */
#else
  if (flag != 0) fsync(i); /* ensure dirty flag written before updates */
#endif
  return 0;
}

int DEV::sync(int32_t &) {
  if (blksize && flag) {
    flag = 0;
    return writehdr();
  }
  return 0;
}

int DEV::wait() {
  return 0;
}


/** Return byte offset of block. */
DEV::t_off64 DEV::blk_pos(t_block b) const {
  long offset = blkoffset;
  if (blk_dev(b)) {
    b = blk_off(b);
    offset = extoffset;
  }
  return (b - 1LL) * blksize + offset;
}

/** Return the most blocks that will fit in sectors for an extent. */
t_block DEV::blk_sects(int ext,int64_t sects) const {
  long offset = ext ? extoffset : blkoffset;
  int sectsperblk = blksize/SECT_SIZE;
#if 0
  fprintf(stderr,"sects = %ld, offset = %ld, sectsperblk = %d\n",
     sects,offset/SECT_SIZE,sectsperblk);
#endif
  int64_t maxblks = (sects - offset/SECT_SIZE) / sectsperblk;
  return (maxblks > 0x7FFFFFFFL) ? 0x7FFFFFFFL : (int32_t)maxblks;
}
