#ifndef _BTDEV_H
#define _BTDEV_H
#pragma interface
#ifndef MORE
#include <port.h>
#endif
/*
    Translate block IO to BTAS/X filesystem extents.
    Handles superblock offset, checkpoints, background writes, etc.
    Handles filesystem header revisions.
    Maximum filesystem size is 2**31 * blocksize - or 16T for 4K blocks.

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
#include <bttype.h>

enum { MAXMNT = 26 };

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
  virtual int sync(int32_t &chkpntCnt);
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
  enum { SECT_SIZE = 512, MAXEXT = MAXDEV, MAXBLK = 0xFFFFFF };
protected:
  int superoffset;	// offset in bytes of super block
  extent &ext(int i) const;
private:
  typedef long long t_off64;
  int writehdr() const;
  int sync_all() const;
  static extent extbl[MAXEXT];
  static int extcnt;
  static bool valid(const btfs &);
  int blk_dev(t_block b) const { return dcnt > 1 ? (unsigned char)(b >> 24):0; }
  long blk_off(t_block b) const { return dcnt > 1 ? (b & 0xFFFFFFL) : b ; }
  static t_block mk_blk(short i,long offset) { return (long(i)<<24)|offset; }
  long blkoffset;	// offset in bytes of first data block
  long extoffset;	// offset of first data block in extents
  t_off64 blk_pos(t_block b) const;	// byte offset of block
  t_block blk_sects(int ext,int64_t sects) const;
  int newspace;	// blocks reallocated from OS
  long maxblk;
};

#if 0
struct PipeDEV: DEV {
  PipeDEV();
  int open(const char *osname,bool rdonly = false);
  int write(t_block blk,const char *buf);
  int sync(int32_t &);
  int wait();
  int close();
  ~PipeDEV();
private:
  int writerfd, resfd;
  class Program *prog;
};
#endif

/** Checkpointed physical IO. */
struct FDEV: DEV {
  FDEV();
  int open(const char *osname,bool rdonly = false);
  int write(t_block blk,const char *buf);
  int read(t_block blk,char *buf);
  int sync(int32_t &);
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

extern FDEV devtbl[MAXMNT];		/* mid table */
#endif
