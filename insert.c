/*  Insert variable length key compressed record in node.
 
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
 *
 * $Log$
 * Revision 2.2  2012/03/14 22:02:25  stuart
 * Undefined call to memcpy
 *
 * Revision 2.1  1996/12/17 16:42:46  stuart
 * C++ node interface
 *
 */
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "node.h"

/* insert record in node after idx, 0 <= idx <= np->cnt() */

bool BLOCK::insert(int idx,const char *rec,int len) {
  int i;
  unsigned char *dst,*src,*tgt;
  int n;		/* number of records in node */
  int samenext;		/* compress length of following record */
  int sameinc;		/* change in samenext */
  int dup;		/* compress length of inserted record */

  n = count;
  assert(0 <= idx && idx <= n);

  src = np->rptr(n);	/* save current base of data */

  /* find new compress length for following record.  Note that 'following'
	means 'following in compress sequence'.  Currently this is
	the index we are inserting after */

  sameinc = dup = 0;
  if (idx > 0) {
    tgt = np->rptr(idx);
    samenext = *tgt++;
    if (idx < n) dup = samenext;
    i = len;
    if (flags & BLK_STEM) i -= sizeof blk;
    sameinc = blkcmp(tgt,(unsigned char *)rec+samenext,i - samenext);
    samenext += sameinc;
    assert(samenext == i || tgt[sameinc] > (unsigned char)rec[samenext]);
  }

  /* compute compress length for inserted record using 'previous' record */

  if (idx < n) {
    unsigned char buf[MAXKEY];
    i = size(idx+1);
    if (flags & BLK_STEM) i -= sizeof blk;
    if (i > (int)sizeof buf) i = sizeof buf;
    copy(idx+1,(char *)buf,i,dup);
    dup += blkcmp(buf+dup,(unsigned char *)rec+dup,i-dup);
    assert(dup == i || (unsigned char)rec[dup] > buf[dup]);
  }

  len += 1 - dup;	/* what we actually need to store */
  sameinc -= len;	/* what we need to shift down by */

  /* does new record fit? */
  if (np->tab[n] + sameinc < (n+2) * (int)sizeof *np->tab)
    return true;

  count = ++n;
  for (i = n; i > idx; --i)	/* insert a table spot */
    np->tab[i] = np->tab[i-1] + sameinc;

  if (idx) {	/* first in node is a special case */
    np->tab[idx] += sameinc + len;
    *np->rptr(idx) = samenext;
  }

  tgt = np->rptr(++i);
  if (i < n) {
    dst = np->rptr(n);	/* shift existing data */
    memmove(dst,src,tgt - dst);
  }
  *tgt++ = dup;				/* key compression for new record */
  memcpy(tgt,rec+dup,len - 1);	/* insert new record */
  flags |= BLK_MOD;				/* mark block modified */
  assert(*np->rptr(count) == 0);		/* did we screw up ? */
  return false;
}
