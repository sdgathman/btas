/*
	BTAS/2 high level interface

	request blocks are defined in "btas.h".  Note these features:

	  a) a variable length buffer is appended to the request.  This
	     is the users record or key.
	  b) the "u.cache" structure is used to cache the current
	     record location to enhance performance.  It is verified
	     before use and need not be correct.
	  c) the "u.id" structure contains security information for
	     BTOPEN and BTCREATE
 * $Log$
 * Revision 2.4  2001/02/28 21:25:24  stuart
 * support record locking
 *
 * Revision 2.3  1998/04/22  03:33:06  stuart
 * prevent instant delete of root
 *
 * Revision 2.2  1997/06/23  15:29:15  stuart
 * implement btserve object, use btkey object
 *
 * Revision 2.1  1996/12/17  16:43:28  stuart
 * C++ node and bufpool interface
 *
 * Revision 1.19  1995/08/03  20:04:07  stuart
 * hashtbl object
 *
 * Revision 1.18  1995/05/31  20:49:51  stuart
 * rename serverstats
 *
 * Revision 1.17  1994/10/17  19:03:41  stuart
 * support mounting on a bare drive letter (no mount directory)
 *
 * Revision 1.16  1994/03/28  20:13:20  stuart
 * BTSTAT with pathname
 *
 * Revision 1.15  1993/12/09  19:27:34  stuart
 * allow mount/unmount of a filesystem not on a directory
 *
 * Revision 1.14  1993/08/25  22:56:24  stuart
 * use btfirst, new bttrace
 *
 * Revision 1.13  1993/05/27  21:58:24  stuart
 * symbolic link support, beginning failsfae support
 *
 */
#ifndef __MSDOS__
static const char what[] =
"$Id$";
#endif
#include <string.h>
#include <assert.h>
#include <bterr.h>		/* user error codes */
extern "C" {
#include <btas.h>		/* user interface */
}
#include "btserve.h"
#include "btbuf.h"		/* buffer, btree operations */
#include "btfile.h"		/* files and directories */
#include "btdev.h"
#include "LockTable.h"

/* NOTE - for historical reasons, higher keys and lower indexes are
	  to the "left", lower keys and higher indexes to the "right".
	  This may change.  */

#ifdef TRACE
#include <stdio.h>
#endif

int btserve::btas(BTCB *b,int opcode) {
  BLOCK *bp;

  if ((unsigned)b->klen > b->rlen || (unsigned)b->rlen > bufpool->maxrec)
    return BTERKLEN;	/* invalid key or record length */
  if (b->klen > MAXKEY)
    b->klen = MAXKEY;	/* truncate long keys */
  int rc = setjmp(btjmp);
  if (rc != 0)
    return rc;
  if (b->root && bufpool->btroot(b->root,b->mid)) return BTERMID;
  b->op = opcode;
  opcode &= 31;
  if (btopflags[opcode] && (btopflags[opcode] & b->flags) == 0)
    return BTERMOD;
  switch (opcode) {
  case BTLINK: case BTCREATE:
    if ( (b->flags & BT_DIR) == 0) return BTERDIR;
    // append root ptr to record
    t_block root;
    if (opcode == (int)BTLINK)
      root = engine->linkfile(b);	
    else
      root = engine->creatfile(b);
    rc = engine->addrec(b);		// add record to dir
    if (rc != 0)
      engine->delfile(b,root);		// unlink if can't add record
    else
      b->rlen -= node::PTRLEN;
    return rc;
  case BTWRITE:				/* write unique key */
    if (b->flags & BT_DIR) return BTERDIR;
    if (b->op & LOCK) {
      if (!locktbl->addLock(b))
	return BTERLOCK;
    }
    return engine->addrec(b);
  case BTDELETE:			/* delete unique key */
    engine->delrec(b,engine->uniquekey(b));
    return 0;
  case BTDELETEF:			/* delete first matching key */
    bp = engine->btfirst(b);
    if (!bp) return BTEREOF;
    engine->delrec(b,bp);
    return 0;
  case BTDELETEL:			/* delete last matching key */
    bp = engine->btlast(b);
    if (!bp) return BTEREOF;
    engine->delrec(b,bp);
    return 0;
  case BTREPLACE:
    engine->replace(b);
    return 0;
  case BTREWRITE:
    engine->replace(b,true);
    return 0;
  case BTREADEQ:		/* read unique key */
    bp = engine->uniquekey(b);
    break;
  case BTREADF:			/* read first matching key */
    bp = engine->btfirst(b);
    if (!bp) return BTERKEY;
    break;
  case BTREADL:			/* read last matching key */
    bp = engine->btlast(b);
    if (!bp) return BTERKEY;
    break;
  case BTREADGT:	/* to the left */
    bp = engine->verify(b);
    if (!bp)
      bp = engine->trace(b,b->klen,-1);
    if (BLOCK::dup >= b->klen) {
      if (--b->u.cache.slot <= 0) {
	if (bp->flags & BLK_ROOT || bp->buf.l.lbro == 0L)
	  return BTEREOF;
	b->u.cache.node = bp->buf.l.lbro;
	bp->flags |= BLK_LOW;
	btget(1);
	bp = bufpool->btbuf(b->u.cache.node);
	b->u.cache.slot = bp->cnt();
	rc = blkcmp(bp->np->rptr(b->u.cache.slot)+1,
		(unsigned char *)b->lbuf,b->klen);
      }
      else
	rc = *bp->np->rptr(b->u.cache.slot);
      if (rc >= b->klen)
	bp = engine->trace(b,b->klen,-1);
    }
    if (b->u.cache.slot <= 0) {
      if (bp->flags & BLK_ROOT || bp->buf.l.lbro == 0L)
	return BTEREOF;
      b->u.cache.node = bp->buf.l.lbro;
      bp->flags |= BLK_LOW;
      btget(1);
      bp = bufpool->btbuf(b->u.cache.node);
      b->u.cache.slot = bp->cnt();
    }
    break;
  case BTREADLT:
    bp = engine->verify(b);
    if (!bp)
      bp = engine->trace(b,b->klen,1);
    for (;;) {
      if (b->u.cache.slot == bp->cnt()) {
	if (bp->flags & BLK_ROOT || bp->buf.l.rbro == 0L)
	  return BTEREOF;
	b->u.cache.slot = 0;
	b->u.cache.node = bp->buf.l.rbro;
	bp->flags |= BLK_LOW;
	btget(1);
	bp = bufpool->btbuf(b->u.cache.node);
      }
      rc = bp->comp(++b->u.cache.slot,b->lbuf,b->klen);
      if (rc < b->klen) break;
      bp = engine->trace(b,b->klen,1);
    }
    break;
  case BTREADGE:
    bp = engine->trace(b,b->klen,1);
    if (b->u.cache.slot == 0) {
      if (bp->flags&BLK_ROOT || bp->buf.l.lbro == 0) return BTEREOF;
      b->u.cache.node = bp->buf.l.lbro;
      btget(1);
      bp = bufpool->btbuf(b->u.cache.node);
      b->u.cache.slot = bp->cnt();
    }
    break;
  case BTREADLE:
    bp = engine->trace(b,b->klen,-1);
    if (b->u.cache.slot++ == bp->cnt()) {
      if (bp->flags&BLK_ROOT || bp->buf.l.rbro == 0) return BTEREOF;
      b->u.cache.slot = 1;
      b->u.cache.node = bp->buf.l.rbro;
      btget(1);
      bp = bufpool->btbuf(b->u.cache.node);
    }
    break;
  case BTOPEN:
#ifdef TRACE
    fprintf(stderr,"uid=%d,gid=%d,mode=%o\n",
	b->u.id.user,b->u.id.group,b->u.id.mode);
#endif
    return engine->openfile(b,0);		/* check permissions */
  case BTCLOSE:
    engine->closefile(b);
#ifdef SINGLE
    return bufpool->flush();
#else
    /* FIXME: release all locks for file only */
    //locktbl->delLock(b->msgident); 
    return 0;
#endif
  case BTUMOUNT:
    if (b->root)
      return engine->unjoin(b);	/* unmount filesystem */
    if (devtbl[b->mid].mcnt > 1) return BTERBUSY;
    return bufpool->unmount(b->mid);
  case BTMOUNT:
    if (b->root)
      return engine->join(b);
    b->lbuf[b->klen] = 0;
    b->mid = bufpool->mount(b->lbuf);
    return 0;
  case BTSTAT:		/* set/extract status information */
    if (b->klen)
      return engine->openfile(b,1);
    if (b->root) {
      if (b->rlen >= sizeof (long)) {
	if (b->rlen > sizeof (struct btstat))
	  b->rlen = sizeof (struct btstat);
	btget(1);
	bp = bufpool->btbuf(b->root);
	memcpy(b->lbuf,&bp->buf.r.stat,b->rlen);
	return 0;
      }
      return BTERKLEN;
    }
    if (bufpool->btroot(b->root,b->mid)) return BTERMID;
    b->rlen = devtbl[b->mid].gethdr(b->lbuf,b->rlen);
    return 0;
  case BTTOUCH:
    if (b->rlen >= sizeof (struct btstat)) {
      struct btstat st;
      btget(1);
      bp = bufpool->btbuf(b->root);
      st = bp->buf.r.stat;
      memcpy((char *)&st,b->lbuf,b->klen);
      bp->buf.r.stat.atime = st.atime;
      bp->buf.r.stat.mtime = st.mtime;
      if (b->u.id.user == bp->buf.r.stat.id.user || b->u.id.user == 0) {
	st.id.mode ^= (st.id.mode ^ bp->buf.r.stat.id.mode) & ~07777;
	bp->buf.r.stat.id = st.id;
      }
      bp->flags |= BLK_MOD;
      memcpy(b->lbuf,(char *)&bp->buf.r.stat,sizeof st);
      return 0;
    }
    return BTERKLEN;
  case BTFLUSH:
    do
      rc = bufpool->flush();
    while (rc == BTERBUSY && (b->op & BTEXCL));
    if (rc == 0 && (b->op & BTEXCL))
      return BTERLOCK;
    return rc;
  /* system maintenance functions */
  case BTFREE:		/* add block to free space */
    btget(1);
    bp = bufpool->find(b->u.cache.node,b->mid);
    bp->blk = b->u.cache.node;
    bp->flags &= ~BLK_ROOT;	// prevent "instant delete" of root
    bufpool->btfree(bp);
    return 0;
  /* BTUNLINK is done by setting b->flags at present */
  case BTPSTAT:
    if (b->rlen >= sizeof bufpool->serverstats)
      b->rlen = sizeof bufpool->serverstats;
    memcpy(b->lbuf,&bufpool->serverstats,b->rlen);
    return 0;
  case BTRELEASE:
    locktbl->delLock(b->msgident);
    return 0;
  default:
    return BTEROP;	/* invalid operation */
  }

  /* Successful read operations end up here */

  b->rlen = bp->size(b->u.cache.slot);
  assert(b->rlen >= 0 && b->rlen <= bufpool->maxrec);
  /* FIXME: keep track of BLOCK::dup above where feasible. */
  /* too many places too check whether BLOCK::dup is accurate, use 0 for now */
  bp->copy(b->u.cache.slot,b->lbuf,b->rlen);
  if ((b->flags & BT_DIR) && b->rlen >= node::PTRLEN)
    b->rlen -= node::PTRLEN;	/* hidden root, but user can look if he wants */

  if (b->op & LOCK) {
    if (!locktbl->addLock(b))
      return BTERLOCK;
  }

  return 0;
}
