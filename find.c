/*	find a record by key, on exit return idx such that:
	(1)	0 <= idx <= node_count(np)
	(2)	k[idx] >= key > k[idx+1]
	(3)	k[0] > all x > k[node_count(np)+1]
	dir > 0		return greatest idx satisfying (2)
	dir < 0		return least idx satisfying (2)
	node_dup = leading duplicate count
*/

#include "btree.h"
#include "node.h"

short node_dup = 0;	/* same count for last find */

/*ARGSUSED*/
int node_find(bp,key,len,dir)
  BLOCK *bp;
  char *key;		/* user key */
  int len;		/* user key length */
  int dir;		/* find first (<0) or last (>0) match */
{
  register int i,j;		/* unchanged key bytes */
  register short *tp;		/* table pointer */
  int ptrsize;		/* overhead len */

  ptrsize = (bp->flags&BLK_STEM) ? sizeof (t_block) + 1 : 1;

  j = 0;		/* init first character differed */
  for (tp = bp->np->tab + bp->count; tp > bp->np->tab; --tp) {
    register unsigned char *kp;	/* current record pointer */
    register int rlen;
    kp = bp->np->dat + *tp;
    i = *kp++;
    if (i > j) continue;		/* no change in result! */
    j = blkcmp(kp,(unsigned char *)key+i,len-i) + i;
    rlen = tp[-1] - tp[0] + i - ptrsize;	/* node_size(tp,0) */
    if (j >= len) {	/* compared equal */
      if (j >= rlen) {
	j = rlen;
	if (bp->flags & BLK_STEM) continue;
      }
      if (dir < 0) continue;
      break;
    }
    if (j < rlen && kp[j-i] > (unsigned char)key[j])
      break; 		/* compared lt */
	     		/* compared gt */
  }
  node_dup = j;
  return tp - bp->np->tab;	/* key less than all records seen */
}
