#include "fs.h"

void btasFS::clear() {		/* clear free space */
  struct btfs *f = &fs->u.f;
  f->hdr.free = 0;
  f->hdr.server = 0;
  f->hdr.mount_time = 0;
  f->hdr.mcnt = 0;
  f->hdr.space = 0;
  f->hdr.droot = 0;
  f->hdr.errcnt = 0;
  f->hdr.flag = 1;
  for (int i = 0; i < f->hdr.dcnt; ++i) {
    if (f->dtbl[i].eof)
      f->hdr.space += f->dtbl[i].eof - f->dtbl[i].eod;
  }
}

/*
	rewrite the block just read
	This does not update the server cache and is used for
	standalone utilities such as btfree
*/

void fstbl::put() {
  if (last_buf) {
    int fd = blk_dev(last_blk);
    if (io->seek(fd,blk_pos(last_blk)) != -1L)
      io->write(fd,last_buf,u.f.hdr.blksize);
    if (fd == curext)	/* restore position */
      seek = true;			/* reseek required */
  }
}

void btasFS::del() {
  union btree *b = (union btree *)fs->last_buf;
  if (b) {
    b->r.root = 0;
    b->r.son = fs->u.f.hdr.free;
    fs->u.f.hdr.free = fs->last_blk;
    ++fs->u.f.hdr.space;
    fs->put();
  }
}
