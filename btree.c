/*	BMS super btree
	variable length records up to 1/2 node size
	minimum unique keys
 * $Log$
 */
#if !defined(lint) && !defined(__MSDOS__)
static char what[] = "$Id$";
#endif

#include "btbuf.h"
#include "node.h"
#include "btas.h"
#include "bterr.h"
#include <stdio.h>
#include <assert.h>

static int getkey(BLOCK *, BLOCK *, char *, t_block *);
static int insert(BLOCK *, BLOCK *, int, char *, int, struct btlevel *);

/*	the level stack records the path followed when
	locating a record.  bttrace() follows the tree
	and builds the stack.  btdel() and btadd() use
	the stack information to rotate, split and merge
	nodes.
*/

struct btlevel stack[MAXLEV];	/* level stack */
struct btlevel *sp;		/* stack pointer */

/*
	btadd() - may be called from btwrite or btrewrite when more space
		is needed in a data node.

	bttrace() must have been called first.
	A record may be replaced and/or inserted at each iteration.
*/

void btadd(char *urec,int ulen) {
  register BLOCK *bp,*np,*dp,*ap;
  struct btlevel *savstk;
  short idx,i,limit,acnt,cnt;
  int rc;
  char insrec[MAXKEY+sizeof (t_block)];

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
      acnt = node_count(np);
      switch (acnt) {
      case 1:
	i = idx;
	break;
      case 2:
	i = 1;
	break;
      default:
	limit = node_free(np->np,0) / 2;
	for (i = cnt = 0; node_free(np->np,i) > limit && ++i < acnt;);
      }
      if (i > idx) --i;
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
	  rc = -1;		/* need dadkey left later */
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
	  --sp->slot;		/* replace failed */
	  continue;		/* insert on another iteration */
	}
	rc = 0;			/* dadkey already moved left */
      }

      assert(idx <= node_count(np));

      /* have to split */

      ap = btnew(np->flags);		/* create new node */
      if ((ap->flags & BLK_STEM) == 0) {
	ap->buf.l.rbro = bp->buf.l.rbro;
	bp->buf.l.rbro = ap->blk;
	bp->flags |= BLK_MOD;	/* in case nothing moved for large ulen */
      }
      ap->buf.l.lbro = np->buf.l.lbro;
      np->buf.l.lbro = ap->blk;
      np->flags |= BLK_MOD;

      cnt = node_count(bp);
      limit = node_free(bp->np,0)/2 - ulen + node_free(bp->np,cnt);
      for (i = cnt; node_free(bp->np,i) < limit && i > 0; --i);
      /* i is number of records to keep in bp */
      if (++i < cnt) {
	cnt = node_move(bp,ap,i+1,0,node_count(bp)-i);
	node_delete(bp,i+1,cnt);
      }
      else cnt = 0;
      /* cnt is now number of records in node ap */
      if (rc) {		/* if we still need to move dadkey left */
	rc = node_move(dp,ap,sp->slot,cnt,1);
	assert(rc == 1);
	node_stptr(ap,++cnt,&np->buf.s.son);
      }
      if (idx && (acnt = node_move(np,ap,1,cnt,idx))) {
	node_delete(np,1,acnt);
	idx -= acnt;
      }
      rc = insert(ap,np,idx,urec,ulen,savstk);
      assert(rc == 0);
      --savstk;
      if (savstk->slot)
	ulen = getkey(bp,ap,insrec,&ap->blk);
      else {
	t_block chk;
	node_ldptr(dp,sp->slot,&chk);	/* get current dad ptr */
	assert(chk == sp[1].node);
	ulen = getkey(bp,ap,insrec,&chk);
      }
      if (node_replace(dp,sp->slot,insrec,ulen)) {
	t_block apblk = ap->blk, npblk = np->blk;
	--sp->slot;
	btadd(insrec,ulen);	/* nasty recursive insert */
	btget(3);
	/* continue where we left off, this depends on the node stack
	   being correctly maintained */
	assert(npblk == savstk[1].node);
	ap = btbuf(apblk);
	np = btbuf(npblk);
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

BLOCK *bttrace(BTCB *b,int klen,int dir) {
  t_block root = b->root;
  char *key = b->lbuf;
  BLOCK *bp;
  short pos;
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
  b->u.cache = *sp;
  return bp;
}

/*
	btdel()	- may be called from btdelete() or btrewrit() when
		the space in a node decreases to less than half.

	bttrace() must have been called first.
	A record may be replaced and/or deleted at each iteration.
*/

void btdel() {
  register BLOCK *np, *bp, *dp;
  register short i;
  short nfree,bfree,totfree,cnt,ulen,bcnt;
  char keyrec[MAXKEY+sizeof (t_block)];

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
      /* This is messy.  We want to move data from np to bp, but we want
	 to delete node bp since there is no rbro for stem nodes.  Our
	 solution is to switch block ids. */
      swapbuf(np,bp);
      if ((np->flags & BLK_STEM) == 0) {	/* connect right pointers */
	bp->buf.l.rbro = np->buf.l.rbro;
	if (bp->buf.l.lbro) {
	  BLOCK *ap;
	  ap = btbuf(bp->buf.l.lbro);
	  ap->buf.l.rbro = bp->blk;
	  ap->flags |= BLK_MOD;
	}
      }
      bp->flags |= BLK_MOD | BLK_KEY;
      btfree(np);
      if (--sp->slot > 0)
        node_stptr(dp,sp->slot,&bp->blk);
      else
	dp->buf.s.son = bp->blk;
    }
    else {		/* even up data in brothers */
      totfree = (nfree + bfree)/2;	
      if (nfree < bfree || bcnt < 2) {
	totfree += node_free(np->np,0) - bfree;
	for (i = 0; node_free(np->np,i) > totfree; ++i);
	cnt = node_move(np,bp,1,bcnt,i);
	node_delete(np,1,cnt);
      }
      else {
	for (i = bcnt - 1; node_free(bp->np,i) < totfree; --i); ++i;
	cnt = node_move(bp,np,i+1,0,bcnt - i);
	assert(cnt == bcnt - i);
	node_delete(bp,i+1,cnt);
      }
      assert(node_count(bp) && node_count(np));
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

/* Insert a record after position idx in node np.  If idx < 0, insert
   in node bp instead.  If idx == 0, insert in whichever node it will
   fit.  If the insert succeeds, update lp to point to the new record
   location, otherwise point lp to the record after which the new record
   still needs to be inserted.  Return 0 for success. */

static int
insert(BLOCK *bp,BLOCK *np,int idx,char *urec,int ulen,struct btlevel *lp) {
  int rc;
  if (idx <= 0) {
    rc = node_insert(bp,idx + node_count(bp),urec,ulen);
    if (idx < 0) {
      lp->node = bp->blk;
      lp->slot = idx + node_count(bp);
      return rc;
    }
    /* if idx == 0, it is still 0 after a successful insert in bp */
    if (rc == 0) {
      lp->slot = idx;
      return rc;
    }
  }
  rc = node_insert(np,idx,urec,ulen);
  if (rc == 0) ++idx;
  lp->slot = idx;
  return rc;
}

/* compute the unique key record to distinquish two nodes */

static int getkey(BLOCK *bp,BLOCK *np,char *urec,t_block *blkp) {
  register int ulen,i;
  i = node_count(bp);
  if (np->flags & BLK_STEM) {
    assert(i>1);		/* use & remove implied key for stem nodes */
    ulen = node_size(bp->np,i) - sizeof *blkp;
    node_ldptr(bp,i,&np->buf.s.son);
    np->flags |= BLK_MOD;
    node_copy(bp,i,urec,ulen,0);
    node_delete(bp,i,1);
  }
  else {			/* construct unique key for leaf nodes */
    short size; unsigned char *p;
    size = node_size(np->np,1);
    ulen = node_size(bp->np,i);
    if (ulen > size) ulen = size;
    if (ulen > MAXKEY) ulen = MAXKEY;
    node_copy(np,1,urec,ulen,0);
    p = rptr(bp->np,i) + 1;
    ulen = blkcmp(p,(unsigned char *)urec,ulen);
    if (ulen < size) {
      urec[ulen] = p[ulen];
      ++ulen;
    }
  }
  assert(ulen <= MAXKEY);
  ulen += stptr(*blkp,urec + ulen);
  return ulen;
}
