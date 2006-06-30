#include <stdio.h>
#include <Date.h>
#include <textlist.h>
#include <Browse.h>
#include "fs.h"
extern "C" {
long ldlong(const char *);
}

MDIFrame *app;

union node {
  unsigned short tab[1];
  unsigned char dat[sizeof (short)];
};

class Btfs: public StringArray {
  btasFS *f;
  static t_block lastblk;
  static btree *b;
public:
  unsigned short blksize;
  Btfs(btasFS *p) { f = p; refresh(); blksize = f->blksize(); }
  btree *getblock(t_block blk);
  int type(t_block blk) { return f->type(blk); }
  t_block inq(int r,int c);
  void refresh();
};

t_block Btfs::lastblk;
btree *Btfs::b;

btree *Btfs::getblock(t_block blk) {
  if (blk == lastblk)
    return b;
  return b = f->get(lastblk = blk);
}

void Btfs::refresh() {
  char buf[80];
  clear();
  const struct btfhdr &hdr = f->hdr()->hdr;
  sprintf(buf,"Next free block:      %8lX", hdr.free);
  add(buf);
  sprintf(buf,"Current deleted root: %8lX", hdr.droot);
  add(buf);
  sprintf(buf,"Root of root dir:     %8lX", hdr.root);
  add(buf);
  sprintf(buf,"Free blocks:          %8ld", hdr.space);
  add(buf);
  char tbuf[16];
  timemask(hdr.mount_time,"DDNnn CCCCC",tbuf);
  sprintf(buf,"Mount time: %11.11s",tbuf);
  add(buf);
  sprintf(buf,"Block size:           %u", hdr.blksize);
  add(buf);
}

t_block Btfs::inq(int r,int) {
  const struct btfhdr &hdr = f->hdr()->hdr;
  switch (r) {
  case 0: return hdr.free;
  case 1: return hdr.droot;
  case 2: return hdr.root;
  }
  return 0;
}

class Btnode: public TextArray {
  btasFS *f;
  btree *b;
  node  *n;
  t_block blk;
  int roottype;
public:
  Btnode(btasFS *,t_block);
  const char *operator[](int);
  t_block inq(int r,int c);
  int size();
  int count() const { return n->tab[0] & ~BLKSTEM; }
  int hasdata() const { return !(n->tab[0] & BLKSTEM); }
  void refresh();
  int copy(int i,char *buf,int max,int dup = 0) const;
  unsigned char *rptr(int i) const {
    return n->tab[i] + n->dat;
  }
  int rlen(int i) const;
  t_block ldptr(int i) const {
    return ldlong((char *)rptr(i) + rlen(i) - 4);
  }
};

Btnode::Btnode(btasFS *fs,t_block block) {
  f = fs;
  blk = block;
}

int Btnode::rlen(int i) const {
    if (i == 1)
      return (unsigned char *)b + f->blksize() - n->dat - n->tab[1];
    return n->tab[i-1] - n->tab[i];
  }

void Btnode::refresh() {
  if (b = f->get(blk)) {
    switch (f->typex(b,blk)) {
    case BLKROOT:
      n = (node *)b->r.data;
      break;
    default:
      n = (node *)b->l.data;
    }
    roottype = f->type(b->r.root);
  }
  else {
    roottype = BLKERR;
    n = 0;
  }
}

int Btnode::size() {
  refresh();
  switch (f->typex(b,blk)) {
  case BLKROOT:
    return count() + 4;
  case BLKSTEM:
    return count() + 2;
  case BLKLEAF:
    return count() + 1;
  }
  return 1;
}

t_block Btnode::inq(int r,int c) {
  refresh();
  if (f->typex(b,blk) == BLKROOT) {
    switch (r) {
    case 0: case 1: case 2:
      return b->r.root;
    case 3: 
      return b->r.son;
    }
  }
  else if (r == 0) {
    if (c < 13) return b->r.son;
    if (c < 28) return b->s.lbro;
    return b->r.root;
  }
  r -= size() - count();
  if (!((roottype & BLKDIR) || !hasdata()) || r < 0 || r >= count()) return 0;
  return ldptr(r + 1);
}

const char *Btnode::operator[](int i) {
  if (i < 0) return 0;
  refresh();
  static char buf[4096]; 
  switch (f->typex(b,blk)) {
    char mbuf[16], abuf[16], cbuf[16];
  case BLKDEL:
    if (--i < 0) {
      sprintf(buf,"DELETED Next: %8X",b->l.rbro);
      return buf;
    }
    return 0;
  case BLKROOT:
    switch (i -= 4) {
    case -4:
      sprintf(buf,"ROOT Root: %8X",b->r.root);
      return buf;
    case -3:
      sprintf(buf,"     Bcnt: %8ld Rcnt: %8ld",b->r.stat.bcnt,b->r.stat.rcnt);
      return buf;
    case -2:
      timemask(b->r.stat.mtime,"DDNnn CCCCC",mbuf);
      timemask(b->r.stat.ctime,"DDNnn CCCCC",cbuf);
      timemask(b->r.stat.atime,"DDNnn CCCCC",abuf);
      sprintf(buf,"    Mtime: %11.11s Atime: %11.11s Ctime: %11.11s",
	mbuf,abuf,cbuf);
      return buf;
    case -1:
      sprintf(buf,"Son: %08X",b->r.son);
      return buf;
    }
    break;
  case BLKSTEM:
    if (--i < 0) {
      sprintf(buf,"Son: %08X  Lft: %08X  Root: %08X",
	b->s.son,b->s.lbro,b->s.root);
      return buf;
    }
    break;
  case BLKLEAF:
    if (--i < 0) {
      sprintf(buf,"Rgt: %08X  Lft: %08X  Root: %08X",
	b->l.rbro,b->l.lbro,b->l.root);
      return buf;
    }
    break;
  }
  if (i >= count()) return 0;
  i = i + 1;
  unsigned char *p = rptr(i);
  int len = rlen(i) + *p - 1;
  char *s = buf;
  s += sprintf(s,"%03d:",i);
  char rbuf[2048];
  t_block blk = 0;
  if (!hasdata() || (roottype & BLKDIR)) {
    s += sprintf(s, " %08X",blk = ldptr(i));
    len -= 4;
  }
  copy(i,rbuf,len);
  s += sprintf(s, " %3d",*p);	// dup count
  if (roottype & BLKDIR) {
    rbuf[len] = 0;
    p = (unsigned char *)rbuf + strlen(rbuf) + 1;
    if (blk == 0)
      s += sprintf(s, " %s --> %s",rbuf,(char *)p);
    else {
      s += sprintf(s, " %s %d",rbuf,*p++);
      while (p[0] && p[1]) {
	switch (p[0]) {
	case 0x01: s += sprintf(s, " c %d", p[1]); break;
	case 0x02: s += sprintf(s, " d %d", p[1]); break;
	case 0x03: s += sprintf(s, " t %d", p[1]); break;
	case 0x40: case 0x41: case 0x42: case 0x43: case 0x44:
	case 0x45: case 0x46: case 0x47: case 0x48: case 0x49:
	  s += sprintf(s, " n%d %d", p[0] - 0x40,p[1]); break;
	default: s += sprintf(s, " %02X %d", p[0], p[1]);
	}
	p += 2;
      }
    }
  }
  else {
    for (i = 0; i < len; ++i)
      s += sprintf(s, " %02X", (unsigned char)rbuf[i]);
  }
  return buf;
}

struct Nodewin: TextBrowser {
  Nodewin(btasFS *,t_block);
  void key(int);
private:
  btasFS *fs;
  Btnode txt;
  char desc[16];
};

Nodewin::Nodewin(btasFS *f,t_block blk): TextBrowser(txt), txt(f,blk)
{
  fs = f;
  sprintf(desc,"Block %08X",blk);
  frame->set(desc);
}

void Nodewin::key(int ky) {
  switch (ky) {
    int row, col;
  case 'o':
    getyx(row,col);
    t_block blk = txt.inq(row,col);
    if (blk) {
      Nodewin *w = new Nodewin(fs,blk);
      app->position(w,w->lines(),70);
      app->addwin(w);
    }
  }
  TextBrowser::key(ky);
}

struct FSwin: TextBrowser {
  FSwin(const char *s);
  void key(int);
  ~FSwin();
private:
  btasXFS fs;
  Btfs h;
};

FSwin::FSwin(const char *s):
  fs(s), h(&fs), TextBrowser(h)
{
  frame->set(s);
}

FSwin::~FSwin() { }

void FSwin::key(int ky) {
  switch (ky) {
    int row, col;
  case 'o':
    getyx(row,col);
    t_block blk = h.inq(row,col);
    if (blk) {
      Nodewin *w = new Nodewin(&fs,blk);
      app->position(w,w->lines(),70);
      app->addwin(w);
    }
  }
  TextBrowser::key(ky);
}

main(int argc,char **argv) {
  MDIFrame app;
  ::app = &app;
  FSwin *f = new FSwin(argv[1]);
  app.position(f,8,40);
  app.addwin(f);
  app.run();
  return 0;
}

int Btnode::copy(int idx,char *rec,int len,int s) const {
  unsigned char *p;
  int i, dup;
  p = rptr(idx);
  dup = *p;
  for (;;) {
    i = *p;
    if (i < len) {
      (void)memcpy(rec+i,(char *)++p,len-i);
      len = i;
    }
    if (i <= s) break;
    p = rptr(++idx);
  }
  return dup;
}
