#pragma implementation
#include <errno.h>
#include <stdio.h>
#include <bterr.h>
#include "fs.h"

enum { SET_SIZE = 3, CACHE_SIZE = 1023 };

struct cache {
  t_block blk[SET_SIZE];
  char type[SET_SIZE];
  char ptr;
}; 

int btFS::type(t_block blk) {
  if (blk <= 0 || !fs) return BLKERR;
  cache *p = &c[(int)(blk % CACHE_SIZE)];
  int i;
  for (i = 0; i < SET_SIZE; ++i)
    if (p->blk[i] == blk) {
      p->ptr = i;
      ++hits;
      i = p->type[i];
      return i;
    }
  if (++p->ptr >= SET_SIZE) p->ptr = 0;
  p->blk[p->ptr] = blk;
  i = typex(fs->get(blk),blk);
  p->type[p->ptr] = i;
  ++misses;
  return i;
}

btFS::btFS() {
  fs = 0; hits = 0; misses = 0;
  c = new cache[CACHE_SIZE];
  for (int i = 0; i < CACHE_SIZE; ++i) {
    for (int j = 0; j < SET_SIZE; ++j)
      c[i].blk[j] = 0;
    c[i].ptr = 0;
  }
}

btFS::~btFS() {
  delete [] c;
  delete fs;
}

btasFS::~btasFS() {
  fsclose();
}

btasXFS::btasXFS(const char *s,char f): unixio(MAXDEV) {
  err = fsopen(s,f);
}

btasXFS::~btasXFS() {
  fsclose();
}

const char *btasXFS::getStatus() const {
  if (fs) return 0;
  if (err < BTERROOT) {
    return strerror(err);
  }
  static char errmsg[16];
  snprintf(errmsg,sizeof errmsg,"errno=%d",err);
  return errmsg;
}

int btasFS::fsopen(const char *name,int flags) {
  fsclose();
  if (open(name,flags) < 0)
    return errno;
  fs = new fstbl(this,flags);

  fs->typex = buftypex;

  int rc = read(0,fs->u.buf,sizeof fs->u.buf);	/* read superblock */

  if (rc == sizeof fs->u.buf && !fs->validate(0)) {
    rc = read(0,fs->u.buf,sizeof fs->u.buf);
    if (rc == sizeof fs->u.buf && !fs->validate(sizeof fs->u.buf)) {
      delete fs; fs = 0;
      return BTERBMNT;
    }
  }
  if (rc != sizeof fs->u.buf) {
    delete fs; fs = 0;
    return errno;
  }

  for (int i = 1; i < fs->u.f.hdr.dcnt; ++i) {
    int fd = open(fs->u.f.dtbl[i].name,flags);
    if (fd != i) {
      fsclose();
      return errno;
    }
  }
  return 0;
}

void btasFS::fsclose() {
  if (!fs) return;
  /* see if header needs updating */
  unsigned long soff = fs->getSuperOffset();
  if (fs->u.f.hdr.flag && seek(0,soff) == soff) {
    fs->u.f.hdr.flag = 0;
    write(0,fs->u.buf,sizeof fs->u.buf);
  }
  delete fs;
  fs = 0;
}

btree *btasFS::get(t_block blk) {
  if (!fs) return 0;
  return (btree *)(blk ? fs->get(blk) : fs->get());
}
