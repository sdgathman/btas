/*	BMS super btree
	variable length records up to 1/2 node size
	minimum unique keys
 * $Log$
 * Revision 2.1  1996/12/17  16:38:05  stuart
 * C++ bufpool interface
 *
 * Revision 1.23  1993/08/25  22:57:29  stuart
 * new bttrace interface
 *
 */
#if !defined(lint) && !defined(__MSDOS__)
static char what[] = "$Id$";
#endif
#include <stdio.h>
#include <assert.h>
#include "btbuf.h"
#include "btas.h"
#include "bterr.h"
#include "btfile.h"

inline BLOCK *bttrace::btnew(short flags) { return bufpool->btnew(flags); }
BLOCK *bttrace::btbuf(t_block node) { return bufpool->btbuf(node); }
inline void bttrace::btfree(BLOCK *blk) { bufpool->btfree(blk); }

/*	the level stack records the path followed when
	locating a record.  bttrace() follows the tree
	and builds the stack.  btdel() and btadd() use
	the stack information to rotate, split and merge
	nodes.
*/

/*
	add() - may be called from btwrite or btrewrite when more space
		is needed in a data node.

	bttrace() must have been called first.
	A record may be replaced and/or inserted at each iteration.
*/

void bttrace::add(char *urec,int ulen) {
  BLOCK *bp,*np,*dp,*ap;
  struct btlevel *savstk;
  short idx,i,limit,acnt,cnt;
  int rc;
  char insrec[MAXKEY+sizeof (t_block)];

  /* assume caller has already attempted BLOCK::insert */

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
      acnt = np->cnt();
      switch (acnt) {
      case 1:
	i = idx;
	break;
      case 2:
	i = 1;
	break;
      default:
	limit = np->np->free(0) / 2;
	for (i = cnt = 0; np->np->free(i) > limit && ++i < acnt;);
      }
      if (i > idx) --i;
      np->move(bp,1,0,i);
      np->move(ap,i+1,0,acnt - i);
      sp->node = ap->blk;	/* default to right node */
      rc = insert(bp,ap,idx -= i,urec,ulen,sp);
      assert (rc == 0);
      np->clear();
      np->buf.r.son = bp->blk;
      ulen = getkey(bp,ap,insrec,&ap->blk);
      np->insert(0,insrec,ulen);
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
      cnt = bp->cnt();
      if (bp->flags & BLK_STEM) {
	if (dp->move(bp,sp->slot,cnt,1))
	  bp->stptr(++cnt,np->buf.s.son);
	else
	  rc = -1;		/* need dadkey left later */
      }

      /* if key left ok, try to rotate with brother first */

      if (rc == 0) {
	if (idx && (acnt = np->move(bp,1,cnt,idx)) != 0) {
	  np->del(1,acnt);
	  idx -= acnt;
	}
	rc = insert(bp,np,idx,urec,ulen,savstk);
	if (rc == 0) {
	  ulen = getkey(bp,np,insrec,&sp[1].node);
	  if (dp->replace(sp->slot,insrec,ulen) == 0) return;
	  urec = insrec;
	  --sp->slot;		/* replace failed */
	  continue;		/* insert on another iteration */
	}
	rc = 0;			/* dadkey already moved left */
      }

      assert(idx <= np->cnt());

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

      cnt = bp->cnt();
      limit = bp->np->free(0)/2 - ulen + bp->np->free(cnt);
      for (i = cnt; bp->np->free(i) < limit && i > 0; --i);
      /* i is number of records to keep in bp */
      if (++i < cnt) {
	cnt = bp->move(ap,i+1,0,bp->cnt()-i);
	bp->del(i+1,cnt);
      }
      else cnt = 0;
      /* cnt is now number of records in node ap */
      if (rc) {		/* if we still need to move dadkey left */
	rc = dp->move(ap,sp->slot,cnt,1);
	assert(rc == 1);
	ap->stptr(++cnt,np->buf.s.son);
      }
      if (idx && (acnt = np->move(ap,1,cnt,idx)) != 0) {
	np->del(1,acnt);
	idx -= acnt;
      }
      rc = insert(ap,np,idx,urec,ulen,savstk);
      assert(rc == 0);
      --savstk;
      if (savstk->slot)
	ulen = getkey(bp,ap,insrec,&ap->blk);
      else {
	t_block chk = dp->ldptr(sp->slot);	/* get current dad ptr */
	assert(chk == sp[1].node);
	ulen = getkey(bp,ap,insrec,&chk);
      }
      if (dp->replace(sp->slot,insrec,ulen)) {
	t_block apblk = ap->blk, npblk = np->blk;
	--sp->slot;
	add(insrec,ulen);	/* nasty recursive insert */
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
      limit = ap->np->free(0) / 2;
      i = (np->flags & BLK_STEM) ? 1 : 0;
      while (i < idx && np->np->free(i) > limit) ++i;
      cnt = np->move(ap,1,0,i);
      np->del(1,cnt);
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
    assert(sp->slot <= dp->cnt());
    if (dp->insert(sp->slot,insrec,ulen) == 0) {
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

bttrace::bttrace(BlockCache *c) {
  bufpool = c;
}

BLOCK *bttrace::trace(BTCB *b,int klen,int dir) {
  char *key = b->lbuf;
  BLOCK *leaf = 0;
  sp = stack;
  sp->node = b->root;
  for (;;) {
    btget(1);
    leaf = btbuf(sp->node);
    short pos = leaf->find(key,klen,dir);
    sp->slot = pos;
    if ((leaf->flags&BLK_STEM) == 0) break;
    if (++sp >= stack + MAXLEV)
      btpost(BTERSTK);
    if (pos == 0)
      sp->node = leaf->buf.s.son;		/* store node */
    else
      sp->node = leaf->ldptr(pos);	/* store node */
  }
  b->u.cache = *sp;
  return leaf;
}

/*
	btdel()	- may be called from btdelete() or btrewrit() when
		the space in a node decreases to less than half.

	bttrace() must have been called first.
	A record may be replaced and/or deleted at each iteration.
*/

void bttrace::del() {
  BLOCK *np, *bp, *dp;
  int i;
  int nfree,bfree,totfree,cnt,ulen,bcnt;
  char keyrec[MAXKEY+sizeof (t_block)];

  for (;;) {
    if (sp == stack) return;	/* short root node OK */
    btget(4);
    --sp;
    dp = btbuf(sp->node);
    if (sp->slot < dp->cnt()) {	/* if right brother */
      bp = btbuf(sp[1].node);
      sp[1].node = dp->ldptr(++sp->slot);
      np = btbuf(sp[1].node);
    }
    else {
      np = btbuf(sp[1].node);
      bp = btbuf(np->buf.l.lbro);
    }
    bcnt = bp->cnt();
    bfree = bp->np->free(bcnt);	/* before adding dad key */
    if (bp->flags & BLK_STEM) {
      if (dp->move(bp,sp->slot,bcnt,1))
	bp->stptr(++bcnt,np->buf.s.son);
      else {
	dp->move(np,sp->slot,0,1);
	np->stptr(1,np->buf.s.son);
      }
    }
    nfree = np->np->free(np->cnt());
    totfree = nfree + bp->np->free(bcnt);	/* includes dad key */
    if (totfree >= 2*np->np->free(0) - dp->np->free(0)) {
      dp->del(sp->slot);
      if (dp->cnt() == 0 && dp->flags & BLK_ROOT) {
	if (np->flags & BLK_STEM)
	  dp->buf.r.son = bp->buf.s.son;
	else {
	  dp->buf.r.son = 0;
	  dp->flags &= ~BLK_STEM;	/* root is now the (only) leaf node */
	}
	bp->move(dp,1,0,bcnt);
	np->move(dp,1,bcnt,np->cnt());
	btfree(np);
	btfree(bp);
	return;
      }
      cnt = np->move(bp,1,bcnt,np->cnt());
      assert (cnt == np->cnt());
      /* This is messy.  We want to move data from np to bp, but we want
	 to delete node bp since there is no rbro for stem nodes.  Our
	 solution is to switch block ids. */
      bufpool->swap(np,bp);
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
        dp->stptr(sp->slot,bp->blk);
      else
	dp->buf.s.son = bp->blk;
    }
    else {		/* even up data in brothers */
      totfree = (nfree + bfree)/2;	
      if (nfree < bfree || bcnt < 2) {
	totfree += np->np->free(0) - bfree;
	for (i = 0; np->np->free(i) > totfree; ++i);
	cnt = np->move(bp,1,bcnt,i);
	np->del(1,cnt);
      }
      else {
	for (i = bcnt - 1; bp->np->free(i) < totfree; --i); ++i;
	cnt = bp->move(np,i+1,0,bcnt - i);
	assert(cnt == bcnt - i);
	bp->del(i+1,cnt);
      }
      assert(bp->cnt() && np->cnt());
      ulen = getkey(bp,np,keyrec,&np->blk);
      if (dp->replace(sp->slot,keyrec,ulen)) {
	--sp->slot;
	add(keyrec,ulen);
	return;
      }
    }
    if (dp->np->free(dp->cnt()) < dp->np->free(0)/2) return;
  }
}

/* Insert a record after position idx in node np.  If idx < 0, insert
   in node bp instead.  If idx == 0, insert in whichever node it will
   fit.  If the insert succeeds, update lp to point to the new record
   location, otherwise point lp to the record after which the new record
   still needs to be inserted.  Return 0 for success. */

int
bttrace::insert(BLOCK *bp,BLOCK *np,int idx,char *urec,int ulen,btlevel *lp) {
  int rc;
  if (idx <= 0) {
    rc = bp->insert(idx + bp->cnt(),urec,ulen);
    if (idx < 0) {
      lp->node = bp->blk;
      lp->slot = idx + bp->cnt();
      return rc;
    }
    /* if idx == 0, it is still 0 after a successful insert in bp */
    if (rc == 0) {
      lp->slot = idx;
      return rc;
    }
  }
  rc = np->insert(idx,urec,ulen);
  if (rc == 0) ++idx;
  lp->slot = idx;
  return rc;
}

/* compute the unique key record to distinquish two nodes */

int bttrace::getkey(BLOCK *bp,BLOCK *np,char *urec,t_block *blkp) {
  register int ulen,i;
  i = bp->cnt();
  if (np->flags & BLK_STEM) {
    assert(i>1);		/* use & remove implied key for stem nodes */
    ulen = bp->size(i) - sizeof *blkp;
    np->buf.s.son = bp->ldptr(i);
    np->flags |= BLK_MOD;
    bp->copy(i,urec,ulen);
    bp->del(i);
  }
  else {			/* construct unique key for leaf nodes */
    short size; unsigned char *p;
    size = np->size(1);
    ulen = bp->size(i);
    if (ulen > size) ulen = size;
    if (ulen > MAXKEY) ulen = MAXKEY;
    np->copy(1,urec,ulen);
    p = bp->np->rptr(i) + 1;
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
