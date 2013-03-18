/*  Buffer management with no hash table.  On memory constrained systems
    (16 bit processor), this gives better performance.
  
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
#include <stdio.h>
#include "hash.h"

/* allocate hash table, and initialize chain */

int begbuf(char *pool,int nsize,int size) {
  stat.bufs = size;
  while (size-- > 0) {
    BLOCK *bp = (BLOCK *)pool;
    bp->lru = mru;
    mru = bp;
    pool += nsize;
  }
  return 0;
}

void endbuf() {
  mru = 0;
}

static BLOCK *inuse[MAXBUF];
static int lastcnt = 0;
int curcnt = 0;

void btget(int cnt) {
  while (lastcnt > 0) {
    register BLOCK *bp = inuse[--lastcnt];
    /* assume chain is never empty */
    bp->lru = mru;
    mru = bp;
  }
  curcnt = cnt;
}

BLOCK *getbuf(t_block blk,short mid) {
  register BLOCK *bp, **pp;
  ++stat.searches;
  for (pp = &mru;;pp = &bp->lru) {
    bp = *pp;
    ++stat.probes;
    if (bp->blk == blk && bp->mid == mid)
      break;
    if (bp->lru == 0) {
      if (bp->flags & BLK_MOD)
	writebuf(bp);
      bp->blk = 0;
      bp->mid = mid;
      break;
    }
  }
  *pp = bp->lru;
  return inuse[lastcnt++] = bp;
}

/* swap block contents - mid and node types must be the same! */

void swapbuf(BLOCK *np,BLOCK *bp) {
  short flag = np->flags;
  long blk = np->blk;
  np->blk = bp->blk;
  np->flags = bp->flags;
  bp->flags = flag;
  bp->blk = blk;
}
