/*
	physical I/O of blocks and cache maintenance
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
#pragma interface
#include "node.h"
//#include "btdev.h"
#include "hash.h"

class BlockCache: public BufferPool {
  t_block root;			/* current root node */
  class FDEV *dev;		/* current file system */
  char *pool;			/* buffer storage */
  int poolsize;			/* number of buffers */
  unsigned nsize;		/* sizeof a buffer */
  BLOCK *getblock(int i) { return (BLOCK *)(pool + i * nsize); }
public:
  bool safe_eof;
  const int maxrec;
  BlockCache(int bsize,unsigned cachesize);
  void btcheck();		/* write pre-image checkpoint if required */
  void btspace(int needed);	/* check free space */
  int flush();		/* flush all buffers */
  int btroot(t_block root, short mid);	/* declare current root */
  BLOCK *btbuf(t_block blk);		/* read block */
  BLOCK *btread(t_block blk);		/* read block no root check */
  int writebuf(BLOCK *);		/* write block */
  BLOCK *btnew(short flags);		/* allocate block */
  void btfree(BLOCK *);			/* free block */
  short mount(const char *);		/* mount fs, return mid */
  int unmount(short mid);		/* unmount fs */
  int newcnt;				/* counts new/deleted blocks */
  static bool ok(int bsize,unsigned cachesize);
  ~BlockCache();
};

extern const char version[];
extern const int version_num;

#ifdef __MSDOS__
#define btget(i) bufpool->get(i,__FILE__,__LINE__)
#else
#define btget(i) bufpool->get(i,what,__LINE__)
#endif
