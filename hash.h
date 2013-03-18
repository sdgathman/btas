#ifndef HASH_H
#define HASH_H
#pragma interface
#include "btree.h"
#define FAILSAFE
/*  Block cache manager.
 
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

class BLOCK;
class BlockCache;

class BufferPool {
  enum {
    MAXFLUSH = 400,	// try not to flush more than this at a time
    MAXBUF = MAXLEV * 2	// maximum buffers reserved (worst case insert)
  };
  int hash(unsigned long v) const {
    return int(v * 781316125L >> hshift) & hmask;
  }
  BLOCK *mru, **hashtbl;
  BLOCK **inuse;
  BLOCK **touched;	// All buffers with BLK_TOUCH or BLK_CHK set
  int numtouch;		// touched (BLK_TOUCH | BLK_CHK) buffer cnt
  int modcnt;		// modified (BLK_TOUCH) buffer cnt
  int lastcnt;
  int hshift, hmask;
  int curcnt;
  const char *curfile;
  int curline;
  int maxtouch;
  void rehash(BLOCK *bp,int oldhash,int newhash);
  int indexOf(BLOCK *bp) const;
protected:
  virtual int writebuf(BLOCK *) = 0;
public:
  static int maxflush;
  btpstat serverstats;
  BufferPool(int ncnt = 0);
  void put(BLOCK *);
  void get(int cnt);	// reserve cnt buffers
  void get(int cnt,const char *file,int line);	// reserve cnt buffers w/ debug
  // write up to lim modified buffers, if all written, start writer sync
  int sync(short mid,int lim = 0);
  int wait(short mid);	// wait for sync, and release touched buffers
  void clear(BLOCK *);	// clear cache entry for block
  void onemore() { ++curcnt; }
  BLOCK *find(t_block blk,short mid);	// get a block in a reserved buffer
  void swap(BLOCK *,BLOCK *);	// swap block id's without copying contents
  static bool ok(int size);	// true iff storage is adequate
  virtual ~BufferPool();
};
#endif
