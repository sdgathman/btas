#if !defined(lint) && !defined(__MSDOS__)
static char what[] = "@(#)btree.c	1.12";
#endif

#include "btbuf.h"
#include "node.h"
#include "bterr.h"
#include <stdio.h>
#include <assert.h>

int getkey(/**/ BLOCK *, BLOCK *, char *, t_block * /**/);
int insert(/**/ BLOCK *, BLOCK *, short, char *, short, struct btlevel * /**/);

/*	the level stack records the path followed when
	locating a record.  bttrace() follows the tree
	and builds the stack.  btdel() and btadd() use
	the stack information to rotate, split and merge
	nodes.
*/

static struct btlevel stack[MAXLEV];	/* level stack */
struct btlevel *sp;		/* stack pointer */

/*
	btadd() - may be called from btwrite or btrewrite when more space
		is needed in a data node.

	bttrace() must have been called first.
	A record may be replaced and/or inserted at each iteration.
*/

void btadd(urec,ulen)
  char *urec;
  int ulen;
{
  register BLOCK *bp,*np,*dp,*ap;
  struct btlevel *savstk;
  short idx,i,limit,acnt,cnt;
  int rc;
  char insrec[MAXREC];

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
      limit = node_free(bp->np,0) / 2;
      acnt = node_count(np);
      for (i = cnt = 0; node_free(np->np,i) > limit && ++i < acnt;);
      if (i > idx && (bp->flags & BLK_STEM)) --i;
      (void)node_move(np,bp,1,0,i);
      (void)node_move(np,ap,i+1,0,acnt - i);
      sp->node = ap->blk;	/* default to right node */
      rc = insert(bp,ap,idx -= i,urec,ulen,sp);
      assert (rc == 0);
      node_clear(np);
      np->buf.r.son = bp->blk;
      ulen = getkey(bp,ap,insrec,&ap->blk);
      (void)node_insert(np,0,insrec,ulen);
      return;
    }

    if (np->buf.l.lbro) {		/* if left brother */
      bp = btbuf(np->buf.l.lbro);
      assert((bp->flags & BLK_STEM) || bp->buf.l.rbro == np->blk);
      savstk = sp;
      do {
	assert(sp > stack);
	--sp;
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
	if (idx && (acnt = node_move(np,bp,1,cnt,idx))) {
	  node_delete(np,1,acnt);
	  idx -= acnt;
	}
	rc = insert(bp,np,idx,urec,ulen,savstk);
	if (rc == 0) {
	  ulen = getkey(bp,np,insrec,&sp[1].node);
	  if (node_replace(dp,sp->slot,insrec,ulen) == 0) return;
	  urec = insrec;
	  --sp->slot;
	  continue;		/* insert on another iteration */
	}
	rc = 0;
      }

      assert(idx <= node_count(np));

      /* have to split */

      ap = btnew(np->flags);		/* create new node */
      if ((ap->flags & BLK_STEM) == 0) {
	ap->buf.l.rbro = bp->buf.l.rbro;
	bp->buf.l.rbro = ap->blk;
      }
      ap->buf.l.lbro = np->buf.l.lbro;
      np->buf.l.lbro = ap->blk;
      np->flags |= BLK_MOD;

      cnt = node_count(bp);
      limit = node_free(bp->np,0)/2 - ulen + node_free(bp->np,cnt);
      for (i = cnt; node_free(bp->np,i) < limit && i > 0; --i);
      if (++i <= cnt) {
	cnt = node_move(bp,ap,i,0,node_count(bp)-i+1);
	node_delete(bp,i,cnt);
      }
      else cnt = 0;
      if (rc) {		/* if we still need to move key left */
	rc = node_move(dp,ap,sp->slot,cnt,1);
	assert(rc == 1);
	node_stptr(ap,++cnt,&np->buf.s.son);
      }
      /* cnt should have current number of records in ap node */
      if (idx && (acnt = node_move(np,ap,1,cnt,idx))) {
	node_delete(np,1,acnt);
	idx -= acnt;
      }
      rc = insert(ap,np,idx,urec,ulen,savstk);
      assert(rc == 0);
      --savstk;
      if (savstk->slot)
	ulen = getkey(bp,ap,insrec,&ap->blk);
      else
	ulen = getkey(bp,ap,insrec,&sp[1].node);
      if (node_replace(dp,sp->slot,insrec,ulen)) {
	t_block new;
	new = ap->blk;
	--sp->slot;
	btadd(insrec,ulen);
	btget(3);
	ap = btbuf(new);
	np = btbuf(savstk[1].node);
	if (savstk->slot)
	  dp = btbuf(savstk->node);
      }
      sp = savstk;
    }
    else {

    /* append sequential node */

      ap = btnew(np->flags);
      if (ap->flags & BLK_STEM)
	ap->buf.s.son = np->buf.s.son;
      else
	ap->buf.l.rbro = np->blk;
      np->buf.l.lbro = ap->blk;
      ap->buf.l.lbro = 0;
      np->flags |= BLK_MOD;
      limit = node_free(ap->np,0) / 2;
      i = (np->flags & BLK_STEM) ? 1 : 0;
      while (i < idx && node_free(np->np,i) > limit) ++i;
      cnt = node_move(np,ap,1,0,i);
      node_delete(np,1,cnt);
      rc = insert(ap,np,idx -= cnt,urec,ulen,sp);
      assert(rc == 0);
      --sp;
      assert(sp->slot == 0);
    }

    ulen = getkey(ap,np,insrec,&np->blk);
    if (sp->slot == 0) {
      dp = btbuf(sp->node);
      dp->buf.s.son = ap->blk;
    }
    assert(sp->slot <= node_count(dp));
    if (node_insert(dp,sp->slot,insrec,ulen) == 0) {
      ++sp->slot;
      return;
    }
    dp->flags |= BLK_MOD;
    urec = insrec;
  }
}

/*
	trace tree from root to data level
	build level stack for possible inserts later
*/

BLOCK *bttrace(root,key,klen,dir)
  t_block root;
  char *key;
  short klen,dir;
{
  register BLOCK *bp;
  register short pos;
  sp = stack;
  sp->node = root;
  for (;;) {
    btget(1);
    bp = btbuf(sp->node);
    pos = node_find(bp,key,klen,dir);
    sp->slot = pos;
    if ((bp->flags&BLK_STEM) == 0) break;
    if (++sp >= stack + MAXLEV)
      btpost(BTERSTK);
    if (pos == 0)
      sp->node = bp->buf.s.son;		/* store node */
    else
      node_ldptr(bp,pos,&sp->node);	/* store node */
  }
  return bp;
}

/*
	btdel()	- may be called from btdelete() or btrewrit() when
		the space in a node decreases to less than half.

	bttrace() must have been called first.
	A record may be replaced and/or deleted at each iteration.
*/

void btdel()
{
  register BLOCK *np, *bp, *dp;
  register short i;
  short nfree,bfree,totfree,cnt,ulen,bcnt;
  char keyrec[MAXREC];

  for (;;) {
    if (sp == stack) return;	/* short root node OK */
    btget(4);
    --sp;
    dp = btbuf(sp->node);
    if (sp->slot < node_count(dp)) {	/* if right brother */
      bp = btbuf(sp[1].node);
      node_ldptr(dp,++sp->slot,&sp[1].node);
      np = btbuf(sp[1].node);
    }
    else {
      np = btbuf(sp[1].node);
      bp = btbuf(np->buf.l.lbro);
    }
    bcnt = node_count(bp);
    bfree = node_free(bp->np,bcnt);	/* before adding dad key */
    if (bp->flags & BLK_STEM) {
      if (node_move(dp,bp,sp->slot,bcnt,1))
	node_stptr(bp,++bcnt,&np->buf.s.son);
      else {
	(void)node_move(dp,np,sp->slot,0,1);
	node_stptr(np,1,&np->buf.s.son);
      }
    }
    nfree = node_free(np->np,node_count(np));
    totfree = nfree + node_free(bp->np,bcnt);	/* includes dad key */
    if (totfree >= 2*node_free(np->np,0) - node_free(dp->np,0)) {
      t_block blk;
      node_delete(dp,sp->slot,1);
      if (node_count(dp) == 0 && dp->flags & BLK_ROOT) {
	if (np->flags & BLK_STEM)
	  dp->buf.r.son = bp->buf.s.son;
	else {
	  dp->buf.r.son = 0;
	  dp->flags &= ~BLK_STEM;	/* root is now the (only) leaf node */
	}
	(void)node_move(bp,dp,1,0,bcnt);
	(void)node_move(np,dp,1,bcnt,node_count(np));
	btfree(np);
	btfree(bp);
	return;
      }
      cnt = node_move(np,bp,1,bcnt,node_count(np));
      assert (cnt == node_count(np));
      blk = np->blk;		/* switch nodes to get efficient node_move */
      np->blk = bp->blk;
      bp->blk = blk;
      if ((np->flags & BLK_STEM) == 0) {	/* connect right pointers */
	bp->buf.l.rbro = np->buf.l.rbro;
	if (bp->buf.l.lbro) {
	  BLOCK *ap;
	  ap = btbuf(bp->buf.l.lbro);
	  ap->buf.l.rbro = bp->blk;
	  ap->flags |= BLK_MOD;
	}
      }
      btfree(np);
      if (--sp->slot > 0)
        node_stptr(dp,sp->slot,&bp->blk);
      else
	dp->buf.s.son = bp->blk;
    }
    else {
      totfree = (nfree + bfree)/2;	/* even up data in brothers */
      if (nfree > bfree || bcnt < 2) {
	totfree += node_free(np->np,0) - bfree;
	for (i = 0; node_free(np->np,i) > totfree; ++i);
	cnt = node_move(np,bp,1,bcnt,i+1);
	node_delete(np,1,cnt);
      }
      else {
	for (i = bcnt - 1; node_free(bp->np,i) < totfree; --i); ++i;
	cnt = node_move(bp,np,i+1,0,bcnt - i);
	node_delete(bp,i+1,cnt);
	assert(bcnt - cnt == node_count(bp));
      }
      ulen = getkey(bp,np,keyrec,&np->blk);
      if (node_replace(dp,sp->slot,keyrec,ulen)) {
	--sp->slot;
	btadd(keyrec,ulen);
	return;
      }
    }
    if (node_free(dp->np,node_count(dp)) < node_free(dp->np,0)/2) return;
  }
}

STATIC int insert(bp,np,idx,urec,ulen,lp)
  BLOCK *bp,*np;
  short idx;
  char *urec;
  short ulen;
  struct btlevel *lp;
{
  int rc;

  if (idx <= 0) {		/* insert new record in one of the nodes */
    rc = node_insert(bp,idx + node_count(bp),urec,ulen);
    if (idx < 0) {
      lp->node = bp->blk;
      idx += node_count(bp);
      if (rc) --idx;
    }
    else {
      idx = 0;
      if (rc) {
	rc = node_insert(np,idx,urec,ulen);
	if (rc == 0) ++idx;
      }
    }
  }
  else {
    rc = node_insert(np,idx,urec,ulen);
    if (rc == 0) ++idx;
  }

  lp->slot = idx;
  return rc;
}

static int getkey(bp,np,urec,blkp)
  BLOCK *bp, *np;
  char *urec;
  t_block *blkp;
{
  register int ulen,i;
  node_dup = 0;
  i = node_count(bp);
  if (np->flags & BLK_STEM) {
    assert(i>1);
    ulen = node_size(bp->np,i) - sizeof *blkp;
    node_ldptr(bp,i,&np->buf.s.son);
    np->flags |= BLK_MOD;
    node_copy(bp,i,urec,ulen);
    node_delete(bp,i,1);
  }
  else {
    short size; unsigned char *p;
    size = node_size(np->np,1);
    ulen = node_size(bp->np,i);
    if (ulen < size) ulen = size;
    node_copy(np,1,urec,ulen);
    p = rptr(bp->np,i) + 1;
    ulen = blkcmp(p,(unsigned char *)urec,ulen);
    if (ulen < size) {
      urec[ulen] = p[ulen];
      ++ulen;
    }
  }
  (void)memcpy(urec+ulen,(char *)blkp,sizeof *blkp);
  ulen += sizeof *blkp;
  return ulen;
}
