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
*/

#include "btbuf.h"
#include "node.h"
#include "btas.h"
#include "btkey.h"
#include "bterr.h"
#include "btfile.h"

BLOCK *uniquekey(b)
  BTCB *b;
{
  BLOCK *bp, *dp;
  t_block rbro;

  /* We would like to use verify(), but we don't for now because
     our callers often need to call btadd() or btdel() and they
     would have to check sp and call bttrace() themselves when necessary.
  */
  bp = bttrace(b->root,b->lbuf,b->klen,1);

  if (sp->slot == 0 || node_dup < b->klen) btpost(BTERKEY); /* not found */
  b->u.cache = *sp;

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

/* Verify cached record location.  Call bttrace() if required.
   Set b->u.cache.node,b->u.cache.slot to the located record.
   We should probably guarrantee first or last record based on dir, but
   the caller does it currently.  The dir now supplied is just an optimization.
*/

BLOCK *verify(b,dir)
  BTCB *b;
  int dir;				/* direction for bttrace() */
{
  register BLOCK *bp;
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
  /* failed exam, retrace */
  bp = bttrace(b->root,b->lbuf,b->klen,dir);
  b->u.cache.node = sp->node;
  b->u.cache.slot = sp->slot;
  return bp;
}

int addrec(b)
  BTCB *b;
{
  BLOCK *bp;
  bp = bttrace(b->root,b->lbuf,b->klen,1);
  if (node_dup == b->klen) {
    return BTERDUP;	/* duplicate */
  }
  newcnt = 0;
  if (node_insert(bp,sp->slot,b->lbuf,b->rlen)) {
    btspace();		/* check for enough free space */
    btadd(b->lbuf,b->rlen);
    b->u.cache.slot = 0;		/* didn't keep track of slot */
  }
  else {
    b->u.cache.node = sp->node;
    b->u.cache.slot = sp->slot + 1;
  }
  btget(1);
  bp = btbuf(b->root);
  ++bp->buf.r.stat.rcnt;	/* increment record count */
  bp->buf.r.stat.bcnt += newcnt;
  bp->buf.r.stat.mtime = curtime;
  bp->flags |= BLK_MOD;
  return 0;
}

void delrec(b,bp)
  BTCB *b;
  register BLOCK *bp;
{
  if (b->flags & BT_DIR) {
    t_block root;
    int rc;
    node_ldptr(bp,b->u.cache.slot,&root);
    if (rc = delfile(b,root)) btpost(rc);
    btget(1);
    bp = btbuf(b->u.cache.node);
  }
  node_delete(bp,b->u.cache.slot,1);
  newcnt = 0;
  if (node_free(bp->np,node_count(bp)) >= node_free(bp->np,0)/2)
    btdel();
  btget(1);
  bp = btbuf(b->root);
  --bp->buf.r.stat.rcnt;	/* decrement record count */
  bp->buf.r.stat.bcnt += newcnt;
  bp->buf.r.stat.mtime = curtime;
  bp->flags |= BLK_MOD;
  b->u.cache.slot = 0;		/* slot no longer valid */
  return;
}
