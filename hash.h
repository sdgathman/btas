#ifndef HASH_H
#define HASH_H
#pragma interface
#include "btbuf.h"
/* interface between btbuf.c and {no}hash.c */
/* hash.c || nohash.c */

#define FAILSAFE

class BufferPool {
  int hash(unsigned long v) { return int(v * 781316125L >> hshift) & hmask; }
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
public:
  BufferPool(int ncnt);
  void put(BLOCK *);
  void get(int cnt);	// reserve cnt buffers
  void get(int cnt,const char *file,int line);	// reserve cnt buffers w/ debug
  int sync(short mid);	// write all modified buffers & start writer sync
  int wait(short mid);	// wait for sync, and release touched buffers
  void clear(BLOCK *);	// clear cache entry for block
  void onemore() { ++curcnt; }
  BLOCK *find(t_block blk,short mid);	// get a block in a reserved buffer
  void swap(BLOCK *,BLOCK *);	// swap block id's without copying contents
  ~BufferPool();
};
#endif
