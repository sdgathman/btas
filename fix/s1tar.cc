#include <unistd.h>
extern "C" {
#include <btas.h>
}
#include "s1tar.h"

s1tar::s1tar(int o,s1FS *f) {
  fd = o;
  fs = f;
}

static int s1path(unsigned char *buf,String path) {
  int len = 0;
  while (path.length() > 1) {
    const char *p = basename(path);
    int blen = strlen(p);
    stebstr(p,buf + len,blen);
    len += blen;
    buf[len++] = s1tar::ECOMMA;
    String tmp(dirname(&path[0]));
    path = tmp;
  }
  return len ? len - 1 : 0;	// don't include trailing comma
}

struct archdr {
  s1_block next;
  s1_block recs;
  archdr(long n,long cnt = -1) { next = n; recs = cnt; }
};

long s1tar::addfile(t_block root,const char *path) {
  // find first data node
  s1btree *b = fs->get(root);
  if (!b || b->r.type == 0) return -1;	// must be datafile
  long node = root;
  while (b && b->son > 0) {
    b = fs->get(node = b->son);
    if (b && b->root != root) return -1;
  }
  if (!b || b->son >= 0) return -1;	// must have rlen/klen
  int rlen = b->info.rlen;
  String dirbuf(path);
  const char *dir = dirname(&dirbuf[0]);
  archdr hdr(0);
  seqout(&hdr,8);
  if (curdir != dir)
    chdir(dir);
  s1dir edir;
  stebstr(basename(path),edir.name,sizeof edir.name);
  edir.ver = 0;
  edir.date = 0;
  edir.time = 0;
  edir.root = root;
  edir.klen = b->info.klen & 0x7fff;
  edir.rlen = rlen;
  edir.priv = 0;
  edir.term = 0;
  edir.user = geteuid();
  edir.acct = getegid();
  edir.who = 0x33;
  edir.type = 1;
  seqout(&edir,52);
  long recs = 0;
  for (;;) {
    int cnt = b->count;
    seqout(b->data,rlen * cnt);
    recs += cnt;
    if (node == root) break;
    node = b->s.rbro;
    if (node <= 0) break;
    b = fs->get(node);
    if (!b || b->root != root) break;
  }
  eof(rlen);
  return recs;
}

void s1tar::chdir(const char *dir) {
  unsigned char epath[256];
  epath[0] = ECOMMA;
  epath[1] = s1path(epath+2,dir);
  seqout(epath,epath[1]+2);
  curdir = dir;
}

void s1tar::eof(int rlen) {
  char buf[rlen];
  memset(buf,0x55,rlen);
  seqout(buf,rlen);
}

void s1tar::seqout(void *buf,int len) {
  write(fd,buf,len);
}

s1tar::~s1tar() {
  archdr hdr(-1);
  seqout(&hdr,8);
  close(fd);
}

/** hook to output files in \#TAR format. */

void s1tar::doroot(root_n *r) {
  long recs = addfile(r->root,r->path);
  if (recs >= 0) {
    fprintf(stderr,"	%ld recs archived\n",recs);
  }
}
