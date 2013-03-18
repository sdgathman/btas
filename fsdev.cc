/*  Checkpointed physical block IO.
    FIXME: compute checksum over journal like ext4 to prevent corruption
    from applying journal in case of reordered writes.
 
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
#include <stdio.h>
#include <unistd.h>
#include <malloc.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <alloca.h>
#include <bterr.h>
#include "btserve.h"
#include "btdev.h"
#include "btbuf.h"
#pragma implementation "Sort.h"
#include <Sort.h>

FDEV::FDEV() {
  maxblks = 0;
  nblks = 0;
  buf = 0;
  blks = 0;
}

FDEV::~FDEV() {
  close();
  delete [] buf;
}

int FDEV::open(const char *osname,bool rdonly) {
  nblks = 0;
  int rc = DEV::open(osname,rdonly);
  if (rc == 0 && root > 32) {
    // compute maximum chkpoint size
    // checkpoint is old SB + blocklist (1 sector) + blocks + new SB
    long size = root * SECT_SIZE - superoffset;
    int max = (size - SECT_SIZE * 2) / blksize;
    if (max > blkmax)
      max = blkmax;
#if 0	// this is wrong - needs to be maxtouch / 2
    if (max > serverstats.bufs)
      max = serverstats.bufs;
#endif
    lastsize = chksize(max) + SECT_SIZE;
    if (max > maxblks) {
      delete [] buf;
      buf = new char [lastsize];
      blks = (long *)(buf + SECT_SIZE);
      maxblks = max;
    }
    else if (max <= 0) {
      delete [] buf;
      maxblks = 0;
    }
    if (flag) {		// load checkpoint if valid
      rc = load_chkpoint();
      if (rc)
	close();
    }
    else
      gethdr(buf,SECT_SIZE);
  }
  // FIXME: set maxblks for non-failsafe
  return rc;
}

int FDEV::write(t_block blk,const char *bf) {
  if (nblks < maxblks) {
    memcpy(buf + chksize(nblks),bf,blksize);
    blks[nblks++] = blk;
    return 0;
  }
  if (flag != 0xFF) flag = 0;	// flag dirty update
  return DEV::write(blk,bf);
}

/* detect a partial checkpoint
   An I/O error reading checkpoint will fail the mount.  If the drive "heals"
   itself, retrying the mount should detect a partial checkpoint.
   The weakness of this scheme is that since the checkpoint is variable
   length, a data block in a previous checkpoint might look like our super
   block (but very unlikely).  Worse, 256 short checkpoints followed by
   a long checkpoint could mistake an old CP for the latest.

   Other schemes:
     1) require checksums of old, new to be equal.  Modify magic in new
	to make them equal.  (Better, but still might conceivably fail.)
     2) use a fixed length checkpoint and write the new superblock to
	a fixed position at the end. (A little slower, but guarranteed.)
*/

static bool ispartial(const btfs *f, const btfs *g) {
  // using time field to record checkpoint time
  // if (f->hdr.mount_time != g->hdr.mount_time) return true;
  if (f->hdr.magic != g->hdr.magic) return true;
  if (f->hdr.blksize != g->hdr.blksize) return true;
  if (f->hdr.server != g->hdr.server) return true;
  if (f->hdr.flag != g->hdr.flag) return true;
  if (f->hdr.root != g->hdr.root) return true;
  for (int i = 0; i < f->hdr.dcnt; ++i) {
    if (f->dtbl[i].eof != g->dtbl[i].eof) return true;
    if (strncmp(f->dtbl[i].name,g->dtbl[i].name,16)) return true;
  }
  unsigned char chkseq = f->hdr.chkseq + 1;
  if (chkseq != g->hdr.chkseq) return true;
  return false;
}

int FDEV::load_chkpoint() {
  if (flag > maxblks) return BTERMBLK;
  int fd = ext(0).fd;
  unsigned size = chksize(flag) + SECT_SIZE;
  if (::lseek(fd,superoffset,0) != superoffset || ::read(fd,buf,size) != size)
    return errno ? errno : BTERBMNT;
  size -= SECT_SIZE;

  // validate last sector of checkpoint here, ignore if partial checkpoint
  const btfs *f = (const btfs *)(buf);		// old superblock
  const btfs *g = (const btfs *)(buf + size);	// new superblock

  if (ispartial(f,g)) {
    fprintf(stderr,"Partial checkpoint ignored\n");
  }
  else {
    // quickie block table validate

    if (flag < blkmax && blks[flag]) return BTERMBLK;
    if (blks[flag - 1] == 0L) return BTERMBLK;

    // update superblock
    free = g->hdr.free;		/* next free list block */
    droot = g->hdr.droot;	/* current deleted root */
    space = g->hdr.space;	/* number of free blocks */
    for (int i = 0; i < dcnt; ++i)
      ext(i).d.eod = g->dtbl[i].eod;
    ++chkseq;

    // update blocks
    nblks = flag;
    fprintf(stderr,"Checkpoint loaded: %d blocks from %s",
	  nblks,ctime(&g->hdr.mount_time));
  }
  return wait();
}

int FDEV::sync(long &chkpntCount) {
  if (!nblks) return DEV::sync(chkpntCount);
  ++chkpntCount;
  int fd = ext(0).fd;
  unsigned size = chksize(nblks);
  flag = nblks;
  ++chkseq;
  gethdr(buf + size,SECT_SIZE);	// get current header
  btfs *g = (btfs *)(buf + size);	// new SB
  btfs *f = (btfs *)(buf);		// backup SB in case of partial CP
  btserve::curtime = time(&g->hdr.mount_time);	// record time of checkpoint
  if (flag < blkmax) blks[flag] = 0L;	// redundant end of blks mark
  f->hdr.flag = flag;
  size += SECT_SIZE;
  if (size < lastsize) {
    unsigned pad = lastsize - size;
    memset(buf + size,0,pad);
    lastsize = size;
    size += pad;
  }
  else
    lastsize = size;
  fsync(fd);
  if (::lseek(fd,superoffset,0) != superoffset || ::write(fd,buf,size) != size)
    return errno;
  return 0;
}

struct blkptr {
  long blk;
  char *buf;
  operator long() const { return blk; }
};

#ifdef __GNUG__
template void sort(blkptr *,int);
#endif

int FDEV::wait() {
  if (!nblks) return 0;
  int rc = 0;
  blkptr *a = (blkptr *)alloca(nblks * sizeof *a);
  int i;
  for (i = 0; i < nblks; ++i) {
    a[i].blk = blks[i];
    a[i].buf = buf + chksize(i);
  }
  sort(a,nblks);
  for (i = 0; i < nblks; ++i) {
    int res = DEV::write(a[i].blk,a[i].buf);
    if (!rc) rc = res;
  }
  nblks = 0;
  gethdr(buf,SECT_SIZE);
  return rc;
}

int FDEV::read(t_block blk,char *buf) {
  int rc = DEV::read(blk,buf);
  if (flag && nblks > 0) {
    // start a write 
    --nblks;
    DEV::write(blks[nblks],this->buf + chksize(nblks));
  }
  return rc;
}
