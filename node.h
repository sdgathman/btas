/*  Node level operations.
    Find and insert are in their own implementation files.
  
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
 * $Log$
 * Revision 2.4  2009/09/25 16:27:43  stuart
 * Make BLOCK a POD.
 *
 * Revision 2.3  2001/02/28 21:58:11  stuart
 * add dump method
 *
 * Revision 2.2  2000/05/03 19:13:10  stuart
 * handle configurable block size
 *
 * Revision 2.1  1996/12/17  16:55:34  stuart
 * C++ node interface
 *
 */
#include <stddef.h>
#include "btree.h"

extern "C" {
  long ldlong(const char *);
  void stlong(long,char *);
  void blkmovl(char *, const char *, int);
  short blkcmp(const unsigned char *,const unsigned char *,int);
}

struct BLOCK;

/* A union node is an array of variable length records.  A table of offsets
   grows from the beginning and the records grow from the end.  A node
   can be in a memory or disk format.  The first offset table entry has
   the number of records in the disk format.  It has the size of the node
   in the memory format.  Clients must save the record count elsewhere when
   converting to memory format.
 */

union node {
private:
  friend struct BLOCK;
  /* The size of a node is variable, these maximums should keep
     memory bound checkers happy. */
  short tab[BLKSIZE/sizeof (short)];		/* offset table */
  unsigned char dat[BLKSIZE];			/* data area */

public:
  // return a pointer to a given record
  unsigned char *rptr(short idx) { return dat + tab[idx]; }
  const unsigned char *rptr(short idx) const { return dat + tab[idx]; }

  // set record count to make disk resident form
  void setsize(short cnt) { tab[0] = cnt; }	// set count

  // return record count of disk node or size in bytes of memory node
  short size() const { return tab[0]; }

  // set end of buffer to make memory resident form
  void setsize(void *end) { tab[0] = (unsigned char *)end - dat; }

  // NOTE: all remaining operations are only valid for memory format nodes

  // return size of a given record
  short size(int i) const { return tab[i-1] - tab[i]; }

  // Load and store the block pointer for a given stem node record.
  // Pointers are stored as the last 4 bytes of a record.
  enum { PTRLEN = 4 };
  void stptr(int i,t_block p) { stlong(p,(char *)rptr(i-1)-PTRLEN); }
  t_block ldptr(int i) const { return ldlong((char *)rptr(i-1)-PTRLEN); }

  // return free bytes in node with n records
  int free(int n) const { return tab[n] - (n+1) * sizeof tab[0]; }
};

inline int stptr(t_block n,char *p) { stlong(n,p); return node::PTRLEN; }
inline t_block ldptr(const char *p) { return ldlong(p); }

typedef union node NODE;

/* flag values for BLOCK */
enum {
  BLK_MOD =	001,	/* block modified */
  BLK_LOW =	002,	/* block is low priority */
  BLK_CHK =	004,	/* block has been checkpointed */
  BLK_ROOT =	010,	/* root node */
  BLK_KEY =	020,	/* block id has been modified */
  BLK_DIR =	040,	/* block belongs to a directory */
  BLK_TOUCH =   0100,	/* block is in touched list */
  BLK_STEM =	0x8000u	/* key data */
};

/** A BLOCK stores the memory resident form of a NODE.  It has a place to
   save the record count and various fields used by BufferPool.  It considers
   each record of a NODE to have a leading duplicate count as the first byte.  
   It provides methods to insert/delete/copy records with leading duplicate
   compression.  

   Block must be a POD (no ctor/dtor/virtual, only POD members) because
   it is variable size (depending on max block size) which requires
   the use of offsetof().

   FIXME: Convert buf member to pointer or reference.  A BLOCK can then
   point into a multi-block disk buffer and be a C++ object with 
   ctor, private members, etc.
 */

struct BLOCK {
  BLOCK *lru;
#ifndef NOHASH
  BLOCK *mru;		/* lru buffer chain */
#endif
  t_block blk;		/* block address */
  short mid;		/* mount id */
  short flags;
  short count;		/* holds count while NODE is in memory format */
  BLOCK *init();
  union node *np;	/* pointer to data area of node */
  union btree buf;
  int cnt() const { return count; }	// return record count
  short size(int i) const;		// return record size
  void setblksize(unsigned blksize);	// set block size
  unsigned getblksize() const {
    return (char *)np->dat + np->size() - buf.data; }
  static short maxrec(unsigned blksz) {	// return max record size
    return (blksz - offsetof(leaf,data[3])) / 2 - 1;
  }
  short maxrec() const { return maxrec(getblksize()); }

  // extract a record.  dup byte are already in buffer (from sequential
  // records).  Use 0 if in doubt.
  int copy(int idx,char *rec,int len,int dup = 0) const;

  /** Retrieve block number from key record. */
  t_block ldptr(int i) const { return np->ldptr(i); }

  /** Store block number in key record. */
  void stptr(int i,t_block p) { np->stptr(i,p); }

  /** Insert record.  Idx for insert must be returned by a previous call to
   * find with the same record.
   */
  bool insert(int idx,const char *rec,int len);	// return true if no room

  /** The number of bytes known to be the same in the user's key and
     the current record slot.  This is used for comparing keys, but
     can also be used to optimize node_copy().  If you are at all unsure
     whether it is still valid, use 0 instead for BLOCK::copy(). */
  static short dup;

  /** Return idx of record matching key.  Set dup to leading dup count.
    Dir matters only with partial (i.e. "duplicate") keys.
   */
  int find(const char *key,int len,int dir) const;

  void clear();			// delete all records from node
  void del(int idx,int n = 1);	// delete n records from node

  /** Replace record.  The caller guarrantees no key change for replace.
     The record is deleted on failure. */
  int replace(int idx,const char *rec, int len);

  /** Return number of chars identical in key and record */
  short comp(int idx,const char *rec,int rlen) const;

  /** Move records between nodes, return actual number moved.
     dir ==  1	end of source to start of destination
     dir == -1    start of source to end of destination */
  short move(BLOCK *dst,int from,int to,int cnt) const;
  /** dump header fields to stderr for debugging. */
  void dump() const;
};
