/*
	Intermediate level operations on BTAS files.

	A partial list of calls to lower levels:

btree:	bttrace()		find record in tree
	btadd()			insert record when node is full
	btdel()			call when node is less than half full

btbuf:	btget()			reserve buffers
	btbuf()			get node in buffer
	btread()		get node from another file (no root check)

	What we do:

btkey:	verify()		locate record with a given key.
				Use cache.node,cache.slot if possible.
	uniquekey()		locate record with a given key, post
				DUPKEY if more than one.
	addrec()		add user record, check for DUPKEY
	delrec()		delete user record, caller locates first
 * $Log$
 * Revision 1.6  1995/01/13  23:48:31  stuart
 * stupid typo
 *
 * Revision 1.5  1994/07/06  15:52:13  stuart
 * don't return old node with record
 *
 * Revision 1.4  1993/12/09  19:33:53  stuart
 * RCS Id
 *
 * Revision 1.3  1993/08/25  22:53:06  stuart
 * fix partial key bug, new bttrace interface
 *
 * Revision 1.2  1993/05/28  19:40:28  stuart
 * isolated partial key searches to btfirst(), btlast()
 *
 */
#ifndef __MSDOS__
static const char what[] =
  "$Id$";
#endif

#include "hash.h"
#include "btas.h"
#include "btkey.h"
#include "bterr.h"
#include "btfile.h"

BLOCK *btfirst(BTCB *b) {
  BLOCK *bp = bttrace(b,b->klen,1);
  if (sp->slot == 0) {
    if ((bp->flags & BLK_ROOT) || bp->buf.l.lbro == 0L) return 0;
    b->u.cache.node = bp->buf.l.lbro;
    btget(1);
    bp = btbuf(b->u.cache.node);
    b->u.cache.slot = bp->cnt();
    BLOCK::dup = bp->comp(b->u.cache.slot,b->lbuf,b->klen);
  }
  if (BLOCK::dup < b->klen) return 0;
  return bp;
}

BLOCK *btlast(BTCB *b) {
  BLOCK *bp = bttrace(b,b->klen,-1);
  if (sp->slot == bp->cnt()) {
    if ((bp->flags & BLK_ROOT) || bp->buf.l.rbro == 0L) return 0;
    b->u.cache.node = bp->buf.l.rbro;
    btget(1);
    bp = btbuf(b->u.cache.node);
    sp->slot = b->u.cache.slot = 0;
  }
  else
    ++sp->slot;
  if (bp->comp(++b->u.cache.slot,b->lbuf,b->klen) < b->klen) return 0;
  return bp;
}

BLOCK *uniquekey(BTCB *b) {
  t_block rbro;

  /* We would like to use verify(), but we don't for now because
     our callers often need to call btadd() or btdel() and they
     would have to check sp and call bttrace() themselves when necessary.
  */
  BLOCK *dp;
  BLOCK *bp = bttrace(b,b->klen,1);
  if (sp->slot == 0 || BLOCK::dup < b->klen) btpost(BTERKEY); /* not found */

  /* check for unique key */

  if (b->u.cache.slot < bp->cnt()) { 	/* check following record */
    if (*bp->np->rptr(b->u.cache.slot) >= b->klen) btpost(BTERDUP);
    return bp;
  }
  if (bp->flags & BLK_ROOT) return bp;

  /* have to check dad and perhaps right brother */

  rbro = bp->buf.l.rbro;	/* save right brother */
  if (rbro == 0) return bp;
  btget(3);
  --sp;
  dp = btbuf(sp->node);
  if (sp->slot >= dp->cnt()	/* if last record in dad */
    || dp->comp(sp->slot + 1,b->lbuf,b->klen) >= b->klen) {
    bp = btbuf(rbro);
    if (bp->comp(1,b->lbuf,b->klen) >= b->klen)
      btpost(BTERDUP);
  }
  ++sp;
  return btbuf(b->u.cache.node);	/* get back our node */
}

/* Verify cached record location. */

BLOCK *verify(BTCB *b) {
  BLOCK *bp;
  if (b->u.cache.slot > 0) {		/* if cached location */
    btget(1);
    bp = btread(b->u.cache.node);
    if (bp->buf.r.root == b->root			   /* root node OK */
      && (bp->flags & BLK_STEM) == 0			   /* leaf node */
      && b->u.cache.slot <= bp->cnt()		   /* slot OK */
      && bp->comp(b->u.cache.slot,b->lbuf,b->klen) == b->klen)
    {
      BLOCK::dup = b->klen;
      return bp;
    }
  }
  return 0;
}

int addrec(BTCB *b) {
  BLOCK *bp;
  bp = bttrace(b,b->klen,1);
  if (BLOCK::dup == b->klen) {
    return BTERDUP;	/* duplicate */
  }
  newcnt = 0;
  if (bp->insert(sp->slot,b->lbuf,b->rlen)) {
    btspace();		/* check for enough free space */
    btadd(b->lbuf,b->rlen);
    b->u.cache.slot = 0;		/* didn't keep track of slot */
  }
  else
    ++b->u.cache.slot;
  btget(1);
  bp = btbuf(b->root);
  ++bp->buf.r.stat.rcnt;	/* increment record count */
  bp->buf.r.stat.bcnt += newcnt;
  bp->buf.r.stat.mtime = curtime;
  bp->flags |= BLK_MOD;
  return 0;
}

void delrec(BTCB *b,BLOCK *bp) {
  int rlen = bp->size(b->u.cache.slot);
  if ((b->flags & BT_DIR) && rlen >= node::PTRLEN) {
    t_block root;
    root = bp->ldptr(b->u.cache.slot);
    int rc = delfile(b,root);
    if (rc) btpost(rc);
    btget(1);
    bp = btbuf(b->u.cache.node);
    rlen -= node::PTRLEN;
  }
  /* save the record we are about to delete */
  bp->copy(b->u.cache.slot,b->lbuf,b->rlen = rlen,b->klen);
  /* delete the record */
  bp->del(b->u.cache.slot);
  newcnt = 0;
  if (bp->np->free(bp->cnt()) >= bp->np->free(0)/2) {
    if (sp->slot == 0)	/* need to trace location */
      bttrace(b,b->rlen,1);
    btdel();			/* adjust tree */
  }
  btget(1);
  bp = btbuf(b->root);
  --bp->buf.r.stat.rcnt;	/* decrement record count */
  bp->buf.r.stat.bcnt += newcnt;
  bp->buf.r.stat.mtime = curtime;
  bp->flags |= BLK_MOD;
  b->u.cache.slot = 0;		/* slot no longer valid */
  return;
}
