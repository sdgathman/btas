#include <stdio.h>
#include <fcntl.h>
#include <String.h>
#include <string.h>
#include "fix.h"
#include "s1fs.h"

struct s1name {
  ECHAR<8> name;
  ECHAR<6> vol;
};

s1FS::s1FS(const char *name,int flag) {
  c = 0;
  if (name)
    fd = _open(name,O_RDONLY);
  else
    fd = 0;
  fs = 0;
  if (fd == -1) return;
  fs = blkopen(new unixio(MAXDEV),flag);
  if (!fs) return;
  char buf[15*256];
  int rc = _read(fd,buf,512);
  // FIXME: detect CAMBEX tape header
  if (rc == 512) {
    errno = 294;
    if (strncmp(buf,"AD",2)) goto fail;
    rc = _read(fd,buf+512,13*256);
    if (rc != 13 * 256) goto fail;
    errno = 294;
    s1fs *p = (s1fs *)(buf + 256);
    s1name *ntbl = (s1name *)(buf + 8 * 256);
    if (p->ver != "BTREE") goto fail;
    fs->typex = fstypex;
    blksize = p->blk;
    rlen = p->rlen;
    klen = p->klen;
    fs->u.f.hdr.free = p->free;
    fs->u.f.hdr.droot = 0;
    fs->u.f.hdr.root = p->root;
    fs->u.f.hdr.space = p->spa;
    fs->u.f.hdr.mount_time = 0;
    fs->u.f.hdr.errcnt = p->lock;
    fs->u.f.hdr.blksize = blksize * 256;
    fs->u.f.hdr.dcnt = p->cnt;
    fs->u.f.hdr.magic = BTMAGIC;
    fs->u.f.hdr.baseid = 0;
    fs->u.f.hdr.flag = (p->lock < 0) ? ~0 : 0;
    for (int i = 0; i < fs->u.f.hdr.dcnt; ++i) {
      btdev &d = fs->u.f.dtbl[i];
      d.eod = (p->ds[i].eod - 2) / blksize;
      d.eof = (p->ds[i].eof - 1) / blksize;
      String name = xname(ntbl[i].name + ',' + ntbl[i].vol);
      if (fs->io->open(name,fs->flags) != i) {
	perror(name);
	goto fail;
      }
      if (name.length() > sizeof d.name)
	strncpy(d.name,basename(name),sizeof d.name);
      else
	strncpy(d.name,name,sizeof d.name);
    }
    fs->nblks = 102400 / fs->u.f.hdr.blksize;
    fs->buf = new char[fs->nblks * fs->u.f.hdr.blksize];
    fs->seek = 1;
    return;
  }
fail:
  _close(fd); fd = -1;
  blkclose(fs); fs = 0;
}

// convert BTAS/X blkno to BTAS/1 blkno
t_block s1FS::xblk(t_block blk) const {
  return blk ? mk_blk(blk_dev(blk),(blk_off(blk)-1) * blksize + 2) : 0;
}

// convert BTAS/1 blkno to BTAS/X blkno
t_block s1FS::oblk(t_block blk) const {
  return blk ? mk_blk(blk_dev(blk),(blk_off(blk)-2) / blksize + 1) : 0;
}

t_block s1FS::lastblk() const {
  return xblk(fs->last_blk);
}

s1btree *s1FS::get(t_block blk) {
  s1btree *b = (s1btree *)btFS::get(oblk(blk));
}

/* this typex expects an xblock */
int s1FS::typex(void *buf,t_block blk) {
  return fstypex(buf,xblk(blk));
}

/* this typex expects an s1 block */
int s1FS::fstypex(const void *buf,t_block blk) {
  if (blk & 1)
    fprintf(stderr,"typeoblk=%08X\n",blk);
  int rc;
  const s1btree *b = (s1btree *)buf;
  if (b == 0 || blk <= 0) return BLKERR;
  long root = b->root;
  if (root == 0L) return BLKDEL;
  if (root < 0L || root == blk) {
    rc = BLKROOT;
    if (b->r.type == 0)
      rc |= BLKDIR;
    if (b->son <= 0L)
      rc |= BLKDATA;
    return rc;
  }
  return (b->son > 0L) ? BLKSTEM : BLKLEAF | BLKDATA;
}

s1FS::~s1FS() {
  _close(fd);	// close the control dataset
  if (fs) {
    delete [] fs->buf;
    blkclose(fs); fs = 0;
  }
}

// convert EDX NAME,VOL to pathname
String s1FS::xname(const String &s) {
  String a[2];
  int n = split(downcase(s),a,2,',');
  if (n == 2) 
    return a[1] + '/' + a[0] + ".edx";
  return a[0] + ".edx";
}
