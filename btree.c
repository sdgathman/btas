static char what[] = "@(#)btree.c	1.2";

#include "node.h"
#include "btbuf.h"
#include <assert.h>

#define MAXLEV	16	/* maximum tree levels */

/*	the level stack records the path followed when
	locating a record.  bttrace() follows the tree
	and builds the stack.  btdel() and btadd() use
	the stack information to rotate, split and merge
	nodes.
*/

struct level {		/* level stack info */
  t_block node;		/* traced node */
  t_block bro;		/* left brother, rbro if slot == 0 */
  short slot;		/* traced slot */
  short same;		/* leading duplicate count */
};

static struct level stack[MAXLEV];	/* level stack */
static struct level *sp;		/* stack pointer */

/*
	btadd() - may be called from btwrite or btrewrite when more space
		is needed in a data node.

	bttrace() must have been called first.
	A record may be replaced and/or inserted at each iteration.
*/

void btadd(urec,ulen)
  char *urec;
  short ulen;
{
  register BLOCK *bp,*np,*dp;
  struct stack *savstk;

  /* assume caller has already attempted node_insert */

  for (;;) {
    btget(5);
    np = btbuf(sp->node);
    idx = sp->slot;

    /* root node gets special split so that it never moves */

    if (sp == stack) {		/* if root node */
      bp = btnew(np->flags & ~BLK_ROOT);
      ap = btnew(bp->flags);
      if ((ap->flags & BLK_STEM) == 0) {
	ap->buf.l.rbro = 0;
	bp->buf.l.rbro = ap->blk;
	np->flags |= BLK_STEM;		/* root just turned to stem */
      }
      else {
	bp->buf.s.son = np->buf.r.son;
      }
      ap->buf.l.lbro = bp->blk;
      bp->buf.l.lbro = 0;
      limit = node_free(bp,0) / 2;
      acnt = node_count(np);
      for (i = cnt = 0; cnt < limit && ++i < acnt;) {
	cnt += node_size(np,i);
      }
      if (i > idx && bp->flags & BLK_STEM) --i;
      node_move(np,bp,1,0,i);
      node_move(np,ap,i+1,0,acnt - i);
      if (i >= idx) {		/* insert new record in one of the nodes */
	rc = node_insert(bp,idx,urec,ulen);
	sp->node = bp->blk;
	++i;
      }
      else {
	rc = node_insert(ap,idx -= i,urec,ulen);
	sp->node = ap->blk;
      }
      assert (rc == 0);
      node_clear(np);
      np->buf.r.son = bp->blk;
      ulen = getkey(bp,ap,insrec,&ap->blk);
      (void)node_insert(np,0,insrec,ulen,-1);
      return;
    }

    savstk = sp;
    if (np->buf.l.lbro) {		/* if left brother */
      bp = btbuf(np->buf.l.lbro);
      do {
	--sp;
	assert(sp > stack);
      } while (sp->slot == 0);		/* find dad node */
      dp = btbuf(sp->node);
      rc = 0;			/* try to move dadkey left if needed */
      cnt = node_count(bp);
      if (bp->flags & BLK_STEM) {
	if (node_move(dp,bp,sp->slot,cnt,1))
	  node_stptr(bp,++cnt,&np->buf.s.son);
	else
	  rc = -1;
      }

      /* if key left ok, try to rotate with brother first */

      if (rc == 0) {
	if (idx && acnt = node_move(np,bp,1,cnt,idx)) {
	  node_delete(np,1,acnt);
	  cnt += acnt;
	  idx -= acnt;
	}
	if (idx || rc = node_insert(bp,cnt,urec,ulen,-1))
	  rc = node_insert(np,idx,urec,ulen,-1);
	if (rc == 0) {
	  savstk->slot = idx + 1; /* update stack in case of recursive call */
	  ulen = getkey(bp,np,insrec,&sp[1].node);
	  if (node_replace(dp,sp->slot,insrec,ulen) == 0) return;
	  urec = insrec;
	  --sp->slot;
	  continue;		/* insert on another iteration */
	}
	rc = 0;
      }

      /* have to split */

      ap = btnew(np->flags);		/* create new node */
      if ((ap->flags & BLK_STEM) == 0) {
	ap->buf.l.rbro = bp->buf.l.rbro;
	bp->buf.l.rbro = ap->blk;
      }
      ap->buf.l.lbro = np->buf.l.lbro;
      np->buf.l.lbro = ap->blk;

      i = rcount(bp,0,0,node_free(bp,0)/2 - ulen);
      limit = node_free(bp,0)/2 - ulen;
      cnt = node_count(bp);
      for (i = cnt; limit > 0 && i > 0; --i) {
	limit -= node_size(bp,i);
      }
      if (++i <= cnt) {
	cnt = node_move(bp,ap,i,0,node_count(bp)-i+1);
	node_delete(bp,i,cnt);
      }
      else cnt = 0;
      if (rc) {		/* if we still need to move key left */
	rc = node_move(dp,ap,sp->slot,cnt,1);
	assert(rc == 0);
	node_stptr(ap,++cnt,&np->buf.s.son);
      }
      /* cnt should have current number of records in ap node */
      if (idx && acnt = node_move(np,ap,1,cnt,idx)) {
	node_delete(np,1,acnt);
	cnt += acnt;
	idx -= acnt;
      }
      if (idx || rc = node_insert(ap,cnt,urec,ulen,-1))
	rc = node_insert(np,idx++,urec,ulen,-1);
      assert(rc == 0);
      savstk->slot = idx;	/* update stack in case of recursive call */
      --savstk;
      if (savstk->slot)
	ulen = getkey(bp,ap,insrec,&ap->blk);
      else
	ulen = getkey(bp,ap,insrec,&sp[1].node);
      if (node_replace(dp,sp->slot,insrec,ulen)) {
	t_node new;
	new = ap->blk;
	btadd(insrec,ulen);
	btget(3);
	ap = btbuf(new);
	np = btbuf(savstk[1].node);
      }
      sp = savstk;
    }
    else {

    /* append sequential node */

      ap = btnew(np->flags);
      (void)node_insert(ap,0,urec,ulen);
      --sp;
    }

    ulen = getkey(ap,np,insrec,&np->blk);
    dp = btbuf(sp->node);
    if (sp->slot == 0)
      dp->buf.s.son = ap->blk;
    dp->buf.s.son = ap->blk;
    if (node_insert(dp,sp->slot,insrec,ulen) == 0) return;
    dp->flags |= BLK_MOD;
    urec = insrec;
  }
}

/*
	trace tree from root to data level
	build level stack for possible inserts later
*/

BLOCK *bttrace(root,key,klen)
  t_node root;
  char *key;
  int klen;
{
  register BLOCK *bp;
  sp = stack;
  sp->node = root;
  sp->bro  = 0;
  btget(1);
  bp = btbuf(sp->node);
  while (bp->flags&BLK_STEM) {
    short pos;
    pos = node_find(bp,key,klen);
    sp->slot = pos;
    if (++sp >= stack + MAXLEV)
      btpost(BTERSTK);
    switch (pos) {
    case 0:
      sp->node = bp->buf.s.son;		/* store node */
      node_ldptr(bp,1,&sp->bro);	/* store its right brother */
      sp[-1].same = node_dup;
      break;
    case 1:
      node_ldptr(bp,pos,&sp->node);	/* store node */
      sp->bro = bp->buf.s.son;
      sp[-1].same = node_dup;
      break;
    default:
      node_ldptr(bp,pos,&sp->node);	/* store node */
      node_ldptr(bp,--pos,&sp->bro);	/* store its left brother */
      sp[-1].same = *rptr(bp,pos);
    }
    btget(1);
    bp = btbuf(sp->node);
  }
  sp->slot = node_find(bp,key,klen);
  sp->same = node_dup;
}

/*
	btdel()	- may be called from btdelete() or btrewrit() when
		the space in a node decreases to less than half.

	bttrace() must have been called first.
	A record may be replaced and/or deleted at each iteration.
*/

void btdel()
{
}

int getkey(bp,np,urec,blkp)
  BLOCK *bp, *np;
  char *urec;
  t_node *blkp;
{
  register int ulen,i;
  node_dup = 0;
  if (np->flags & BLK_STEM) {
    i = node_count(bp);
    if (node_free(bp,i) > node_free(np,node_count(np))) {
      bp = np;
      i = 1;
    }
    ulen = node_size(bp,i) - sizeof *blkp;
    node_ldptr(bp,i,&np->buf.s.son);
    node_copy(bp,i,urec,ulen);
    node_delete(bp,i,1);
  }
  else {
    assert(node_count(np) > 1);
    ulen = *rptr(np,1);
    node_copy(np,1,urec,++ulen);
  }
  (void)memcpy(urec+ulen,(char *)blkp,sizeof *blkp);
  ulen += sizeof *blkp;
  return ulen;
}
