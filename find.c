/*	find a record by key, on exit return idx such that:
	(1)	0 <= idx <= np->cnt()
	(2)	k[idx] >= key > k[idx+1]
	(3)	k[0] > all x > k[np->cnt()+1]
	dir > 0		return greatest idx satisfying (2)
	dir < 0		return least idx satisfying (2)
	BLOCK::dup = leading duplicate count
 
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
 * Revision 2.1  1996/12/17 16:41:50  stuart
 * C++ node interface
 *
 */

#include "node.h"

short BLOCK::dup = 0;	/* same count for last find */

int BLOCK::find(const char *key,int len,int dir) const {
  /* find first (dir<0) or last (dir>0) match */
  int i,j;		/* unchanged key bytes */
  short *tp;		/* table pointer */
  int ptrsize;		/* overhead len */

  ptrsize = (flags&BLK_STEM) ? sizeof (t_block) + 1 : 1;

  j = 0;		/* init first character differed */
  for (tp = np->tab + count; tp > np->tab; --tp) {
    unsigned char *kp;	/* current record pointer */
    int rlen;
    kp = np->dat + *tp;
    i = *kp++;
    if (i > j) continue;		/* no change in result! */
    j = blkcmp(kp,(unsigned char *)key+i,len-i) + i;
    rlen = tp[-1] - tp[0] + i - ptrsize;	/* tp->size(0) */
    if (j >= len) {	/* compared equal */
      if (j >= rlen) {
	j = rlen;
	if (flags & BLK_STEM) continue;
      }
      if (dir < 0) continue;
      break;
    }
    if (j < rlen && kp[j-i] > (unsigned char)key[j])
      break; 		/* compared lt */
	     		/* compared gt */
  }
  dup = j;
  return tp - np->tab;	/* key less than all records seen */
}
