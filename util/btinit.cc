/*
	Initialize BTAS/2 file systems
	Support multiple extents	8-13-92 sdg
*/

#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <mem.h>
#include <time.h>
extern "C" {
#include "../btree.h"
#include <btas.h>
#include "util.h"
}

class filesys {
  union {
    struct btfs d;
    struct root r;
    char buf[SECT_SIZE];
  } u;
  btperm id;
  int fd;
  const char *name;
  bool mod;
public:
  filesys(const char *name);
  operator const char *() const { return name; }
  unsigned blksize() const { return u.d.hdr.blksize; }
  const char *init(unsigned blksize,long size = 0,int mode = BT_DIR + 0777);
  void list() const;	// list extents
  void revert();        // revert to disk copy
  const char *add(const char *name,long size = 0);
  void del();
  const char *size(int,long);	// change size of extent
  int ismount() const;	// true if currently mounted
  ~filesys();
};

filesys::filesys(const char *s): name(s) {
  fd = open(s,O_RDWR+O_CREAT+O_BINARY,0666);
  revert();
  if (fd < 0) {
    perror(s);
    exit(1);
  }
}

void filesys::revert() {
  memset(u.buf,0,sizeof u.buf);
  mod = false;	// mark unmodified
  if (fd < 0) return;
  if (lseek(fd,0L,0)) return;
  int rc = _read(fd,u.buf,SECT_SIZE);
  if (rc != SECT_SIZE || u.d.hdr.magic != BTMAGIC
    || u.d.hdr.blksize & 0x81ff || u.d.hdr.dcnt <= 0) {
    memset(u.buf,0,sizeof u.buf);
  }
}

const char *filesys::init(unsigned bsz,long sz,int mode) {
  if (bsz % SECT_SIZE)
    return "Block size must be a multiple of 512 bytes";
  memset(u.buf,0,sizeof u.buf);
  if (sz <= 0)
    sz = filelength(fd);
  // initialize filesystem header
  u.d.hdr.blksize = bsz;
  u.d.hdr.magic = BTMAGIC;
  u.d.dtbl->eod = 0L;
  (void)strncpy(u.d.dtbl->name,name,sizeof u.d.dtbl->name);
  if (sz < SECT_SIZE + bsz) {
    u.d.dtbl->eof = 0;
    u.d.hdr.space = 0;
  }
  else {
    u.d.dtbl->eof = (sz - SECT_SIZE) / bsz;
    u.d.hdr.space = u.d.dtbl->eof;
  }
  u.d.hdr.root = 0L;
  u.d.hdr.dcnt = 1;
  id.user = getuid();
  id.group = getgid();
  id.mode = mode;
  mod = true;
  return 0;
}

filesys::~filesys() {
  if (!mod) {
    close(fd);
    printf("%s: unchanged\n",name);
    return;
  }
  unsigned bsz = blksize();
  if (fd < 0 || !bsz) return;
  long root = u.d.hdr.root;
  if (!root) {	// allocate initial root node
    u.d.hdr.root = ++u.d.dtbl->eod;
    if (u.d.dtbl->eof) --u.d.hdr.space;
  }
  if (lseek(fd,0L,0) || _write(fd,u.buf,SECT_SIZE) != SECT_SIZE) {
    perror(name);
    return;
  }
  if (root) return;
  root = u.d.dtbl->eod;
  // initialize root directory
  memset(u.buf,0,sizeof u.buf);
  u.r.root = root;
  u.r.stat.bcnt = 1L;
  u.r.stat.links = 1;
  u.r.stat.mtime = u.r.stat.atime = time(&u.r.stat.ctime);
  u.r.stat.id = id;
  for (unsigned i = 0; i < bsz; i += sizeof u.buf) {
    if (_write(fd,u.buf,sizeof u.buf) != sizeof u.buf)
      perror(name);
    memset(u.buf,0,sizeof u.buf);
  }
  close(fd);
}

class Extent {
  int fd;
  long sz;
public:
  union {
    btfs fs;
    char buf[SECT_SIZE];
  };
  Extent(const char *s,int f,int m = 0666);
  operator int() { return fd; }
  const char *add(btfs &primary,int i,long sz);
  ~Extent() { close(fd); }
};

Extent::Extent(const char *s,int f,int m) {
  fd = open(s,f,m);
  memset(buf,0,sizeof buf);
  sz = filelength(fd);
  if (sz < 0 || lseek(fd,0L,0) || sz > 0 && _read(fd,buf,sizeof buf) != sizeof buf) {
    close(fd);
    fd = -1;
    return;
  }
}

const char *Extent::add(btfs &prim,int i,long size) {
  if (fd < 0) return "I/O error";
  if (size)
    sz = size;
  if (sz < SECT_SIZE + prim.hdr.blksize)
    sz = 0;
  else
    sz = (sz - SECT_SIZE) / prim.hdr.blksize;
  if (i) {
    fs.hdr = prim.hdr;
    fs.hdr.dcnt = 0;			// mark as extent
    fs.hdr.baseid = i;
    fs.dtbl[0] = prim.dtbl[i];
    fs.dtbl[0].eof = sz;
    fs.dtbl[1] = prim.dtbl[0];	// primary extent we belong to
  }

  if (prim.dtbl[i].eof)
    prim.hdr.space -= prim.dtbl[i].eof - prim.dtbl[i].eod;
  if (sz)
    prim.hdr.space += sz - prim.dtbl[i].eod;
  prim.dtbl[i].eof = sz;
  if (i && _write(fd,buf,sizeof buf) != sizeof buf)
    return "I/O error";
  return 0;
}

const char *filesys::add(const char *s,long sz) {
  if (!blksize())
    return "Not initialized";
  if (u.d.hdr.dcnt >= MAXDEV) return "Too many extents";
  Extent efd(s,O_RDWR+O_CREAT+O_BINARY,0666);
  if (efd < 0) return "Can't open extent";
  strncpy(u.d.dtbl[u.d.hdr.dcnt].name,s,sizeof u.d.dtbl[0].name);
  u.d.dtbl[u.d.hdr.dcnt].eod = 0;
  const char *msg = efd.add(u.d,u.d.hdr.dcnt,sz);
  if (msg) return msg;
  ++u.d.hdr.dcnt;
  mod = true;
  return 0;
}

const char *filesys::size(int i,long newsize) {
  if (i < 0 || i >= u.d.hdr.dcnt)
    return "Invalid extent index";
  btdev &d = u.d.dtbl[i];
  char name[sizeof d.name + 1];
  strncpy(name,d.name,sizeof d.name)[sizeof d.name] = 0;
  Extent efd(name,O_RDWR+O_BINARY);
  if (efd < 0) return "Can't open extent";
  if (newsize && newsize < d.eod) return "New size too small";
  const char *msg = efd.add(u.d,i,newsize);
  if (msg) return msg;
  mod = true;
  return 0;
}

void filesys::list() const {
  if (blksize()) {
    long alloc = 0, used = 0, eod = 0;
    printf("Filesystem: %s\tBlocksize: %u\n",name,blksize());
    printf("%-16s %8s %8s\n","Dataset","Alloc","Used");
    for (int i = 0; i < u.d.hdr.dcnt; ++i) {
      printf("%-16.16s %8ld %8ld\n",
	  u.d.dtbl[i].name,u.d.dtbl[i].eof,u.d.dtbl[i].eod);
      alloc += u.d.dtbl[i].eof;
      used += u.d.dtbl[i].eod;
      if (u.d.dtbl[i].eof)
	eod += u.d.dtbl[i].eof - u.d.dtbl[i].eod;
    }
    printf("%-16s %8s %8s\n","_______","_______","_______");
    printf("%-16.16s %8ld %8ld\t Freelist: %8ld\n",
	"Total: ",alloc,used,u.d.hdr.space - eod);
  }
  else
    printf("Filesystem: %s\tnot initialized\n",name);
}

void filesys::del() {	// delete unused extents
  int i;
  for (i = u.d.hdr.dcnt; i > 1 && u.d.dtbl[--i].eod == 0;) {
    memset(&u.d.dtbl[i],0,sizeof u.d.dtbl[i]);
    mod = true;
  }
  u.d.hdr.dcnt = i;
}

int main(int argc,char **argv) {
  if (argc != 2) {
    puts("Usage:\tbtinit osfile");
    return 1;
  }
  filesys fs(argv[1]);

  puts("BTINIT version 1.1");
  fs.list();

  for (;;) {
    char *cmd = strupr(readtext("COMMAND(?): "));
    if (strncmp(cmd,"EN",2) == 0) break;
    if (strncmp(cmd,"IN",2) == 0) {
      unsigned blksize = getval("Block size: ");
      long size = getval(0) * SECT_SIZE;
      if (fs.blksize()) {
	printf("%s already initialized, ",(const char *)fs);
	if (!question("Continue? ")) continue;
      }
      const char *msg = fs.init(blksize,size);
      if (msg)
	printf("%s: %s.\n",(const char *)fs,msg);
      else
	fs.list();
    }
    else if (strncmp(cmd,"AD",2) == 0) {
      const char *name = readtext("Dataset: ");
      long size = getval(0) * SECT_SIZE;
      const char *msg = fs.add(name,size);
      if (msg)
	printf("%s: %s.\n",name,msg);
      else
	fs.list();
    }
    else if (strncmp(cmd,"DE",2) == 0) {
      fs.del();
      fs.list();
    }
    else if (strncmp(cmd,"LI",2) == 0) {
      fs.list();
    }
    else if (strncmp(cmd,"RE",2) == 0) {
      fs.revert();
      fs.list();
    }
    else if (strncmp(cmd,"SI",2) == 0) {
      int idx = getval(0);
      long size = getval(0) * SECT_SIZE;
      const char *msg = fs.size(idx,size);
      if (msg)
	printf("size(%d): %s.\n",idx,msg);
      else
	fs.list();
    }
    else {
      printf("\
BTINIT commands:\n\
  IN bs [size]		Initialize filesystem, bs in bytes, size in sectors\n\
  AD file [size]	Add extent\n\
  DE			Delete unused extents\n\
  SI idx size		Change size of extent\n\
  LI			List extents\n\
  RE			Undo changes\n\
  EN			End utility\n\
"     );
    }
  }
  return 0;
}
