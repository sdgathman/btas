#pragma interface
#ifndef _FS_H
#define _FS_H
#include "../btree.h"
#include "fsio.h"
extern "C" {
#include <btas.h>
}

/** fstbl efficiently reads a BTAS filesystem sequentially via
    a large buffer.  It also accomodates random reads of a single block.
    When a read error occurs on a multi-block read, it re-reads individual
    blocks to narrow down exactly which blocks cannot be read.

    This class has a C flavor because it was converted from C code.

    It can process BTAS/1 as well as BTAS/X filesystems, and can
    use unix style I/O or something else.  I/O is done through the
    fsio abstract class.

    C++ code will use the btFS wrapper which handles cacheing
    root node types.  
    
    The s1FS derived class handles BTAS/1 filesystems
    loaded into unix files from a Series/1 image backup.  
    
    The btasFS abstract class handles BTAS/X specific features, but requires
    an fsio mixin to implement fsio.

    The btasXFS derived class mixes btasFS with unixio to handle BTAS/X
    filesystems with unix style I/O.

 */

class fstbl {
  /* const struct fs_class *_class; */
  char *buf;		/* I/O buffer */
  /* int fd[MAXDEV];	/* OS file descriptors for extent(s) */
  fsio *io;
  short nblks;		/* max number of blocks in I/O buffer */
  short bcnt;		/* actual number of blocks in I/O buffer */
  /* sequential status */
  t_block base;		/* base block of current I/O buffer */
  short cur;		/* current block offset in I/O buffer */
  short curext;		/* current extent */
  bool seek;		/* true if seek required for next buffer */
  char flags;		/* flags passed in open */
  // FIXME: copied from DEV
  static int blk_dev(t_block b) { return (unsigned char)(b >> 24); }
  static long blk_off(t_block b) { return b & 0xFFFFFFL; }
  static t_block mk_blk(short i,long offset) { return (i<<24L)|offset; }
  unsigned long blk_pos(t_block b) const;
  int superoffset, blkoffset, extoffset;
public:
  unsigned long getSuperOffset() const { return superoffset; }
  bool validate(int superoffset);
  bool isSaveImage() const { return u.f.hdr.magic == BTSAVE_MAGIC; }
  enum { SECT_SIZE = 512 };
  /* last block read */
  char *last_buf;
  t_block last_blk;
  /* superblock */
  union {
    struct btfs f;		/* superblock structure */
    char buf[SECT_SIZE];	/* sector buffer for superblock */
  } u;
  int (*typex)(const void *, t_block);
  enum { FS_RDONLY = 1, FS_BGND = 2, FS_SWAP = 4 };
  fstbl(fsio *,int flag);
  void *get();
  void *get(t_block blk);
  void put();
  t_block lastblk() const { return last_blk; }
  ~fstbl();
};

enum blk_type { BLKERR, BLKDEL = 1, BLKROOT = 2, BLKSTEM = 4, BLKLEAF = 8,
/* enhanced block type flags apply to BLKROOT only */
    BLKCOMPAT =	0x0F,	/* mask for compatible types */
    BLKDIR	=	0x40,	/* block belongs to a directory */
    BLKDATA	=	0x80	/* block contains data (not index) */
};

/** C++ style wrapper for fstbl that handles root type cacheing.
 */
class btFS {
  struct cache *c;
protected:
  fstbl *fs;
public:
/* flags used by fsopen() */
  long hits, misses;
  btFS();
  int operator!() const { return fs == 0; }
  virtual t_block lastblk() const { return fs->last_blk; }
  int lasttype() { return typex(fs->last_buf,lastblk()); }
  const btfs *hdr() const { return fs ? &fs->u.f : 0; }
  virtual int typex(void *b,t_block k) { return (*fs->typex)(b,k); }
  virtual int type(t_block blk);
  virtual ~btFS();
};

/** An abstract class that handles BTAS/X filesystems, but I/O must 
    be specified by mixing in a class that implements fsio.
 */
struct btasFS: btFS, virtual private fsio {
  btree *get(t_block blk = 0L);
  void del();
  int maxrec() const { return fs ? fs->u.f.hdr.blksize / 2 - 10 : 0; }
  unsigned blksize() const { return fs ? fs->u.f.hdr.blksize : 0; }
  bool isSaveImage() const { return fs && fs->isSaveImage(); }
  void clear();
  void put() { if (fs) fs->put(); }
  ~btasFS();
protected:
  int fsopen(const char *s,int f);
  void fsclose();
private:
  static int buftypex(const void *,t_block);
};

/** Handles BTAS/X filesystems with unix style I/O.
 */
struct btasXFS: btasFS, unixio {
  btasXFS(const char *s,char flags = fstbl::FS_RDONLY);
  btasXFS(int mid);
  ~btasXFS();
};
#endif
