#ifndef _BTDEV_H
#define _BTDEV_H
#pragma interface
#ifndef MORE
#include <port.h>
#endif
// physical I/O to a BTAS/X filesystem.
// Handles superblock offset, checkpoints, background writes, etc.
// Handles filesystem header revisions.
// FIXME: align 1st block to block boundary to improve filesystems
// 	  using regular (not-raw) os files.
// FIXME: put super block on second sector.  1st sector is boot sector
//	  and is reserved for AIX logical volumes (raw I/O) and when
//	  BTAS/X is the primary filesystem.
// FIXME: checkpoint should follow superblock, so both can be written
//	  in one operation.

#include <bttype.h>

struct DEV: btfhdr {
  DEV();
  struct extent {
    int fd;		/* OS file descriptor */
    DEV *dev;
    struct btdev d;	/* device info from header */
  };
  short mid;		/* index into fstbl */
  virtual int open(const char *osname,bool rdonly = false);
  virtual int write(t_block blk,const char *buf);
  virtual int read(t_block blk,char *buf);
  virtual int sync(long &chkpntCnt);
  virtual int wait();	// wait for sync
  int chkspace(int needed,bool safe_eof = true);
  int gethdr(char *buf,int len) const;
  t_block newblk();
  bool isopen() const { return blksize > 0; }
  bool isclean() const { return flag == 0; }
  virtual int close();
  static unsigned short maxblksize;
  static char index;	// server index
  virtual ~DEV();
  enum { SECT_SIZE = 512, MAXEXT = MAXDEV };
protected:
  int superoffset;	// offset in bytes of super block
  extent &ext(int i) const;
private:
  int writehdr() const;
  static extent extbl[MAXEXT];
  static int extcnt;
  static bool valid(const btfs &);
  static int blk_dev(t_block b) { return (unsigned char)(b >> 24); }
  static long blk_off(t_block b) { return b & 0xFFFFFFL; }
  static t_block mk_blk(short i,long offset) { return (long(i)<<24)|offset; }
  long blkoffset;	// offset in bytes of first data block
  long extoffset;	// offset of first data block in extents
  long blk_pos(t_block b) const;
  int newspace;	// blocks reallocated from OS
};

#if 0
struct PipeDEV: DEV {
  PipeDEV();
  int open(const char *osname,bool rdonly = false);
  int write(t_block blk,const char *buf);
  int sync(long &);
  int wait();
  int close();
  ~PipeDEV();
private:
  int writerfd, resfd;
  class Program *prog;
};
#endif

struct FDEV: DEV {
  FDEV();
  int open(const char *osname,bool rdonly = false);
  int write(t_block blk,const char *buf);
  int read(t_block blk,char *buf);
  int sync(long &);
  int wait();
  int max() const { return maxblks; }
  ~FDEV();
private:
  enum { blkmax = SECT_SIZE / sizeof (long) };
  // checkpoint must fit in 64K for 16-bit systems!
  unsigned chksize(int n) { return SECT_SIZE * 2 + n * blksize; }
  int load_chkpoint();
  unsigned lastsize;	// size of previous chkpoint
  int maxblks;	// maximum blks in a chkpoint
  int nblks;
  char *buf;
  long *blks;
};

extern FDEV devtbl[MAXDEV];		/* mid table */
#endif
