/*
	Intermediate level operations on BTAS files.

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

 * $Log$
 * Revision 2.4  2007/06/21 20:06:09  stuart
 * Fix uniquekey.
 *
 * Revision 2.2  1997/06/23 15:31:46  stuart
 * implement btfile object
 *
 * Revision 2.1  1996/12/17  16:46:59  stuart
 * C++ node interface
 *
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

#include "btserve.h"
#include "btbuf.h"
#include "btas.h"
#include "btfile.h"
#include "bterr.h"

BLOCK *btfile::btfirst(BTCB *b) {
  BLOCK *bp = trace(b,b->klen,1);
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

BLOCK *btfile::btlast(BTCB *b) {
  BLOCK *bp = trace(b,b->klen,-1);
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

BLOCK *btfile::uniquekey(BTCB *b) {
  /* We would like to use verify(), but we don't for now because
     our callers often need to call btadd() or btdel() and they
     would have to check sp and call bttrace() themselves when necessary.
  */
  BLOCK *bp = trace(b,b->klen,1);
  if (sp->slot == 0 || BLOCK::dup < b->klen) btpost(BTERKEY); /* not found */

  /* check for unique key */

  if (b->u.cache.slot > 1) {
    if (*bp->np->rptr(b->u.cache.slot - 1) >= b->klen) btpost(BTERDUP);
  }
  if (b->u.cache.slot < bp->cnt()) { 	/* check following record */
    if (*bp->np->rptr(b->u.cache.slot) >= b->klen) btpost(BTERDUP);
    if (b->u.cache.slot > 1) return bp;
  }
  if (bp->flags & BLK_ROOT) return bp;

  /* have to check dad and perhaps right brother */

  t_block rbro = bp->buf.l.rbro;	/* save right brother */
  if (rbro == 0) return bp;
  btget(3);
  --sp;
  BLOCK *dp = btbuf(sp->node);
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

BLOCK *btfile::verify(BTCB *b) {
  BLOCK *bp;
  if (b->u.cache.slot > 0) {		/* if cached location */
    btget(1);
    bp = bufpool->btread(b->u.cache.node);
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

int btfile::addrec(BTCB *b) {
  BLOCK *bp;
  bp = trace(b,b->klen,1);
  if (BLOCK::dup == b->klen) {
    return BTERDUP;	/* duplicate */
  }
  bufpool->newcnt = 0;
  if (bp->insert(sp->slot,b->lbuf,b->rlen)) {
    bufpool->btspace(needed());		/* check for enough free space */
    add(b->lbuf,b->rlen);
    b->u.cache.slot = 0;		/* didn't keep track of slot */
  }
  else
    ++b->u.cache.slot;
  btget(1);
  bp = btbuf(b->root);
  ++bp->buf.r.stat.rcnt;	/* increment record count */
  bp->buf.r.stat.bcnt += bufpool->newcnt;
  bp->buf.r.stat.mtime = btserve::curtime;
  bp->flags |= BLK_MOD;
  return 0;
}

void btfile::delrec(BTCB *b,BLOCK *bp) {
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
  bufpool->newcnt = 0;
  if (bp->np->free(bp->cnt()) >= bp->np->free(0)/2) {
    if (sp->slot == 0)	/* need to trace location */
      trace(b,b->rlen,1);
    del();			/* adjust tree */
  }
  btget(1);
  bp = btbuf(b->root);
  --bp->buf.r.stat.rcnt;	/* decrement record count */
  bp->buf.r.stat.bcnt += bufpool->newcnt;
  bp->buf.r.stat.mtime = btserve::curtime;
  bp->flags |= BLK_MOD;
  b->u.cache.slot = 0;		/* slot no longer valid */
  return;
}

void btfile::replace(BTCB *b,bool rewrite) {
  BLOCK *bp = uniquekey(b);
  /* check free space, physical size could change because of key
     compression even when logical size doesn't */
  bufpool->btspace(needed());
  bufpool->newcnt = 0;
  /* convert user record to a directory record if required */
  if (b->flags & BT_DIR) {
    if (b->rlen > bufpool->maxrec - (int)sizeof (t_block))
      b->rlen = bufpool->maxrec - sizeof (t_block);
    t_block root = bp->ldptr(sp->slot);
    b->rlen += stptr(root,b->lbuf + b->rlen);
  }
  if (rewrite) {
    int rlen = bp->size(sp->slot);
    /* extend user record with existing record if required */
    if (rlen > b->rlen) {
      int ptrlen = (b->flags & BT_DIR) ? node::PTRLEN : 0;
      bp->copy(sp->slot,b->lbuf,rlen,b->rlen - ptrlen);
      b->rlen = rlen;
    }
  }
    /* OK, now replace the record */
  if (bp->replace(sp->slot,b->lbuf,b->rlen)) {
    --sp->slot;
    add(b->lbuf,b->rlen);
  }
  else if (bp->np->free(bp->cnt()) >= bp->np->free(0)/2)
    del();

  if (b->flags & BT_DIR)
    b->rlen -= node::PTRLEN;
  btget(1);
  bp = btbuf(b->root);
  if (bufpool->newcnt || bp->buf.r.stat.mtime != btserve::curtime) {
    bp->buf.r.stat.bcnt += bufpool->newcnt;
    bp->buf.r.stat.mtime = btserve::curtime;
    bp->flags |= BLK_MOD;
  }
}
