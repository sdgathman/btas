#ifndef HASH_H
#define HASH_H
#pragma interface
#include "btree.h"

#define FAILSAFE

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
  ~BufferPool();
};
#endif
