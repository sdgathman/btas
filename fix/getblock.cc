/*
 * $Log$
 */

/*
	traverse blocks in a BTAS/X filesystem sequentially
	and perform other low level I/O

	Used by data recovery programs.
*/
#include <stdio.h>
#include <bterr.h>
#include <errno.h>
#include "fs.h"

#if defined(m68k) || defined(__MSDOS__)
enum { FIXNBLK = 30 };	/* blocks per read, larger numbers don't work on 68k */
#else
enum { FIXNBLK = 100 };
#endif

static void swap(unsigned short &w) {
  unsigned short t = w;
  unsigned char b0 = (t >> 8) & 255;
  unsigned char b1 = t & 255;
  w = (b1 << 8) | b0;
}

static void swap(short &w) {
  short t = w;
  unsigned char b0 = (t >> 8) & 255;
  unsigned char b1 = t & 255;
  w = (b1 << 8) | b0;
}

static void swap(t_block &w) {
  t_block t = w;
  unsigned char b0 = (t >> 24) & 255;
  unsigned char b1 = (t >> 16) & 255;
  unsigned char b2 = (t >> 8) & 255;
  unsigned char b3 = t & 255;
  w = (b3 << 24) | (b2 << 16) | (b1 << 8) | b0;
}

static void swap(struct btstat &st) {
  swap(st.bcnt);
  swap(st.rcnt);
  swap(st.atime);
  swap(st.ctime);
  swap(st.mtime);
  swap(st.links);
  swap(st.opens);
  swap(st.id.user);
  swap(st.id.group);
  swap(st.id.mode);
}

static void swap(short *tab) {
  swap(tab[0]);
  int cnt = tab[0] & 0x7FFF;
  for (int i = 1; i <= cnt; ++i)
    swap(tab[i]);
}

static void swap(void *buf) {
  union btree *b = (union btree *)buf;
  swap(b->r.root);
  swap(b->r.son);
  if (b->r.root < 0) {
    swap(b->r.stat);
    swap(b->r.data);
  }
  else {
    swap(b->s.lbro);
    swap(b->s.data);
  }
}

fstbl::fstbl(fsio *ioob,int flag) {
  bcnt = 0;
  base = 0;
  cur = 0;
  curext = 0;
  last_buf = 0;
  last_blk = 0;
  seek = false;
  buf = 0;
  io = ioob;
  flags = flag;
}

fstbl::~fstbl() {
  delete [] buf;
}

/*	return pointer to filesystem block
	return 0 if error or end, errno has reason
*/

void *fstbl::get(t_block blk) {
  int rc, fd;
  last_buf = 0;
  fd = blk_dev(blk);
  if (fd >= u.f.hdr.dcnt) return 0;	/* invalid extent */
  blk = blk_off(blk);

  /* check if block already in buffer */
  if (fd == curext && blk > base + 1 && blk <= base + bcnt)
    last_buf = buf + (int)(blk - base - 1) * u.f.hdr.blksize;
  else {	/* we really have to do it */
    if (io->seek(fd,blk_pos(blk)) == -1L)
	return 0;				/* seek error, give up */

    /* determine buffer to use */
    if (cur != 1) {
      last_buf = buf;		/* use beginning if not in use */
    }
    else {				/* use end of buffer */
      if (bcnt == nblks) {
	--bcnt;			/* steal room if needed */
	seek = true;
      }
      last_buf = buf + bcnt * u.f.hdr.blksize;
    }

    rc = io->read(fd,last_buf,u.f.hdr.blksize);
    if (fd == curext)
      seek = true;	/* sequential logic needs to reseek at end of buffer */
    if (rc != u.f.hdr.blksize)
      last_buf = 0;	/* read failed */
    else if (flags & FS_SWAP)
      swap(last_buf);
  }
  return last_buf;
}

/*
	return pointer to next sequential filesystem block
	if blk != 0, return specified block
	return 0 if error or end, errno has reason
*/

void *fstbl::get() {
  t_block eod = u.f.dtbl[curext].eod;
  last_buf = 0;
  do {
    int rc, fd;
    int cnt;
    if (base + cur >= eod) {
      errno = 0;
      if (curext+1 >= u.f.hdr.dcnt) return 0;	/* end of data */
      eod = u.f.dtbl[++curext].eod;
      cur = 0;
      base = 0;
      seek = true;
      bcnt = 0;
    }
    fd = curext;
    if (cur < bcnt) break;	/* already in buffer */

    /* read in a new buffer full */
    cnt = nblks;
    base += cur;	/* proposed new base */
    if (base + cnt >= eod) {
      cnt = (int)(eod - base);	/* short final buffer */
    }
    if (seek) {
      long pos = blk_pos(base+1);
      /* if ((flags & io->FS_BGND) == 0) */
	/* fprintf(stderr,"Dataset #%d, seek(%ld), size(%ld)\n", */
	  /* curext+1,pos,eod); */
      if (io->seek(fd,pos) == -1L) {
	perror("seek");
	return 0;		/* seek failed, give up */
      }
      seek = false;
    }
    if ((flags & io->FS_BGND) == 0)
      fprintf(stderr,"Block %d-%8ld, %d\r",curext,base,cnt);
    rc = io->read(fd,buf,u.f.hdr.blksize * cnt);

    /* This mangled logic is needed because most OSes won't tell
       us where the I/O error occurred (even though it knows).  We
       have to find out by trying each one.  We try to speed this
       up by doing a (sort of) binary search. */
    if (rc == -1) {
      perror("\nread");		/* read error */
      if (errno != EIO) return 0;	/* give up if weird error */
      fputs("Attempting recovery . . .\n",stderr);
      do {
	if (cnt == 1) {
	  fprintf(stderr,"Bad block %d-%ld\n",curext,base+1);
	  rc = u.f.hdr.blksize;
	  *(long *)buf = 0x80000000L;	/* Mark as I/O error */
	  break;
	}
	cnt >>= 1;
	if (
	  io->seek(fd,blk_pos(base+1)) == -1L) {
	  perror("seek");
	  return 0;		/* give up on seek failure */
	}
	rc = io->read(fd,buf,u.f.hdr.blksize * cnt); /* try some */
      } while (rc == -1);
    }

    bcnt = rc / u.f.hdr.blksize;	/* actual blocks read */
    cur = 0;
    while (bcnt < cnt && rc) {
      int more = io->read(fd,buf + rc,u.f.hdr.blksize * cnt - rc);
      if (more <= 0) break;
      rc += more;
      bcnt = rc / u.f.hdr.blksize;	/* actual blocks read */
    }
    if (bcnt < cnt) {
      fprintf(stderr,"\nPremature EOF at %d-%ld (%ld)\n",
	      curext,base + bcnt,eod);
      eod = base + bcnt;
    }
    if (flags & FS_SWAP)
      for (int i = 0; i < bcnt; ++i)
	swap(buf + i * u.f.hdr.blksize);
  } while (base >= eod);
  last_buf = buf + cur * u.f.hdr.blksize;
  last_blk = mk_blk(curext,base + ++cur);
  return last_buf;
}

unsigned long fstbl::blk_pos(t_block b) const {
  unsigned long pos = blk_off(b) - 1;
  int fd = blk_dev(b);
  int offset;
  if (isSaveImage()) {
    for (int i = 0; i < fd; ++i)
      pos += u.f.dtbl[i].eod;
    offset = SECT_SIZE;
  }
  else if (fd)
    offset = extoffset;
  else
    offset = blkoffset;
  return pos * u.f.hdr.blksize + offset;
}

static void swap(btfhdr &hdr) {
  swap(hdr.free);
  swap(hdr.droot);
  swap(hdr.root);
  swap(hdr.space);
  swap(hdr.mount_time);
  swap(hdr.blksize);
  swap(hdr.mcnt);
  swap(hdr.dcnt);
  swap(hdr.magic);
}

static void swap(btdev &hdr) {
  swap(hdr.eod);
  swap(hdr.eof);
}

static void swap(btfs &fs) {
  swap(fs.hdr);
  for (int i = 0; i < fs.hdr.dcnt; ++i)
    swap(fs.dtbl[i]);
}

bool fstbl::validate(int supoff) {
  short sm = u.f.hdr.magic;
  swap(sm);
  if (sm == BTSAVE_MAGIC && (flags & FS_RDONLY)) {
    swap(u.f);
    flags |= FS_SWAP;
  }
  if ((u.f.hdr.magic != BTMAGIC && (u.f.hdr.magic != BTSAVE_MAGIC || supoff))
    || u.f.hdr.blksize % SECT_SIZE || u.f.hdr.blksize == 0) {
    return false;	
  }
  nblks = FIXNBLK * 1024 / u.f.hdr.blksize;
  u.f.hdr.flag = 0;
  buf = new char[nblks * u.f.hdr.blksize];
  if (!buf) return false;	
  superoffset = supoff;
  blkoffset = (long)SECT_SIZE * u.f.hdr.root;
  extoffset = superoffset + SECT_SIZE;
  if (superoffset) {	// new format rounds to blksize
    extoffset = (extoffset + u.f.hdr.blksize - 1) & ~(u.f.hdr.blksize - 1);
    seek = true;
  }
  return true;
}
