/*
	Intermediate level operations on BTAS files.

	A partial list of calls to lower levels:

btree:	bttrace()		find record in tree
	btadd()			insert record when node is full
	btdel()			call when node is less than half full

node:	node_insert()		insert record in node
	node_delete()		delete record from node
	node_copy()		extract record from node
	node_free()		return free bytes in node
	node_size()		return size of record
	node_comp()		compare user record to record in node
	node_count()		return number of records in node
	rptr()			return pointer to record in node

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
 * Revision 1.3  1993/08/25  22:53:06  stuart
 * fix partial key bug, new bttrace interface
 *
 * Revision 1.2  1993/05/28  19:40:28  stuart
 * isolated partial key searches to btfirst(), btlast()
 *
 */

static const char what[] = "$Id$";

#include "btbuf.h"
#include "node.h"
#include "btas.h"
#include "btkey.h"
#include "bterr.h"
#include "btfile.h"

BLOCK *btfirst(BTCB *b) {
  BLOCK *bp = bttrace(b,b->klen,1);
  if (sp->slot == 0) {
    if ((bp->flags && BLK_ROOT) || bp->buf.l.lbro == 0L) return 0;
    b->u.cache.node = bp->buf.l.lbro;
    btget(1);
    bp = btbuf(b->u.cache.node);
    b->u.cache.slot = node_count(bp);
    node_dup = node_comp(bp,b->u.cache.slot,b->lbuf,b->klen);
  }
  if (node_dup < b->klen) return 0;
  return bp;
}

BLOCK *btlast(BTCB *b) {
  BLOCK *bp = bttrace(b,b->klen,-1);
  if (sp->slot == node_count(bp)) {
    if ((bp->flags && BLK_ROOT) || bp->buf.l.rbro == 0L) return 0;
    b->u.cache.node = bp->buf.l.rbro;
    btget(1);
    bp = btbuf(b->u.cache.node);
    sp->slot = b->u.cache.slot = 0;
  }
  else
    ++sp->slot;
  if (node_comp(bp,++b->u.cache.slot,b->lbuf,b->klen) < b->klen) return 0;
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
  if (sp->slot == 0 || node_dup < b->klen) btpost(BTERKEY); /* not found */

  /* check for unique key */

  if (b->u.cache.slot < node_count(bp)) { 	/* check following record */
    if (*rptr(bp->np,b->u.cache.slot) >= b->klen) btpost(BTERDUP);
    return bp;
  }
  if (bp->flags & BLK_ROOT) return bp;

  /* have to check dad and perhaps right brother */

  rbro = bp->buf.l.rbro;	/* save right brother */
  if (rbro == 0) return bp;
  btget(3);
  --sp;
  dp = btbuf(sp->node);
  if (sp->slot >= node_count(dp)	/* if last record in dad */
    || node_comp(dp,sp->slot + 1,b->lbuf,b->klen) >= b->klen) {
    bp = btbuf(rbro);
    if (node_comp(bp,1,b->lbuf,b->klen) >= b->klen)
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
      && b->u.cache.slot <= node_count(bp)		   /* slot OK */
      && node_comp(bp,b->u.cache.slot,b->lbuf,b->klen) == b->klen)
    {
      node_dup = b->klen;
      return bp;
    }
  }
  return 0;
}

int addrec(BTCB *b) {
  BLOCK *bp;
  bp = bttrace(b,b->klen,1);
  if (node_dup == b->klen) {
    return BTERDUP;	/* duplicate */
  }
  newcnt = 0;
  if (node_insert(bp,sp->slot,b->lbuf,b->rlen)) {
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
  if (b->flags & BT_DIR) {
    t_block root;
    int rc;
    node_ldptr(bp,b->u.cache.slot,&root);
    if (rc = delfile(b,root)) btpost(rc);
    btget(1);
    bp = btbuf(b->u.cache.node);
  }
  /* save the record we are about to delete */
  b->rlen = node_size(bp->np,b->u.cache.slot);
  node_copy(bp,b->u.cache.slot,b->lbuf,b->rlen,b->klen);
  /* delete the record */
  node_delete(bp,b->u.cache.slot,1);
  newcnt = 0;
  if (node_free(bp->np,node_count(bp)) >= node_free(bp->np,0)/2) {
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
