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
#include "hash.h"		/* buffer, btree operations */
#include <bterr.h>		/* user error codes */
extern "C" {
#include <btas.h>		/* user interface */
}
#include "btfile.h"		/* files and directories */
#include "btkey.h"		/* key lookup */
#include "btdev.h"

/* NOTE - for historical reasons, higher keys and lower indexes are
	  to the "left", lower keys and higher indexes to the "right".
	  This may change.  */

jmp_buf btjmp;		/* jump target for btpost */

#ifdef TRACE
#include <stdio.h>
#endif

extern "C" int btas(BTCB *b,int opcode) {
  register BLOCK *bp;
  int rlen,rc;
  t_block root;

  if ((unsigned)b->klen > b->rlen || (unsigned)b->rlen > maxrec)
    return BTERKLEN;	/* invalid key or record length */
  if (b->klen > MAXKEY)
    b->klen = MAXKEY;	/* truncate long keys */
  rc = setjmp(btjmp);
  if (rc != 0)
    return rc;
  if (b->root && btroot(b->root,b->mid)) return BTERMID;
  b->op = opcode;
  opcode &= 31;
  if (btopflags[opcode] && (btopflags[opcode] & b->flags) == 0)
    return BTERMOD;
  switch (opcode) {
  case BTLINK: case BTCREATE:
    if ( (b->flags & BT_DIR) == 0) return BTERDIR;
    if (opcode == (int)BTLINK)
      root = linkfile(b);		/* add root ptr to record */
    else
      root = creatfile(b);		/* add root ptr to record */
    rc = addrec(b);
    if (rc != 0)
      delfile(b,root);		/* unlink if can't write record */
    else
      b->rlen -= node::PTRLEN;
    return rc;
  case BTWRITE:				/* write unique key */
    if (b->flags & BT_DIR) return BTERDIR;
    return addrec(b);
  case BTDELETE:			/* delete unique key */
    delrec(b,uniquekey(b));
    return 0;
  case BTDELETEF:			/* delete first matching key */
    bp = btfirst(b);
    if (!bp) return BTEREOF;
    delrec(b,bp);
    return 0;
  case BTDELETEL:			/* delete last matching key */
    bp = btlast(b);
    if (!bp) return BTEREOF;
    delrec(b,bp);
    return 0;
  case BTREPLACE: case BTREWRITE:
    bp = uniquekey(b);
    /* check free space, physical size could change because of key
       compression even when logical size doesn't */
    btspace();
    newcnt = 0;
    /* convert user record to a directory record if required */
    if (b->flags & BT_DIR) {
      t_block root;
      if (b->rlen > maxrec - sizeof (t_block))
	b->rlen = maxrec - sizeof (t_block);
      root = bp->ldptr(sp->slot);
      b->rlen += stptr(root,b->lbuf + b->rlen);
    }
    switch (opcode) {
    case BTREWRITE:
      rlen = bp->size(sp->slot);
      /* extend user record with existing record if required */
      if (rlen > b->rlen) {
	int ptrlen = (b->flags & BT_DIR) ? node::PTRLEN : 0;
	bp->copy(sp->slot,b->lbuf,rlen,b->rlen - ptrlen);
	b->rlen = rlen;
      }
      /* OK, now replace the record */
    case BTREPLACE:
      if (bp->replace(sp->slot,b->lbuf,b->rlen)) {
	--sp->slot;
	btadd(b->lbuf,b->rlen);
      }
      else if (bp->np->free(bp->cnt()) >= bp->np->free(0)/2)
	btdel();
      break;
    }
    if (b->flags & BT_DIR)
      b->rlen -= node::PTRLEN;
    btget(1);
    bp = btbuf(b->root);
    if (newcnt || bp->buf.r.stat.mtime != curtime) {
      bp->buf.r.stat.bcnt += newcnt;
      bp->buf.r.stat.mtime = curtime;
      bp->flags |= BLK_MOD;
    }
    return 0;
  case BTREADEQ:		/* read unique key */
    bp = uniquekey(b);
    break;
  case BTREADF:			/* read first matching key */
    bp = btfirst(b);
    if (!bp) return BTERKEY;
    break;
  case BTREADL:			/* read last matching key */
    bp = btlast(b);
    if (!bp) return BTERKEY;
    break;
  case BTREADGT:	/* to the left */
    bp = verify(b);
    if (!bp)
      bp = bttrace(b,b->klen,-1);
    if (BLOCK::dup >= b->klen) {
      if (--b->u.cache.slot <= 0) {
	if (bp->flags & BLK_ROOT || bp->buf.l.lbro == 0L)
	  return BTEREOF;
	b->u.cache.node = bp->buf.l.lbro;
	bp->flags |= BLK_LOW;
	btget(1);
	bp = btbuf(b->u.cache.node);
	b->u.cache.slot = bp->cnt();
	rc = blkcmp(bp->np->rptr(b->u.cache.slot)+1,
		(unsigned char *)b->lbuf,b->klen);
      }
      else
	rc = *bp->np->rptr(b->u.cache.slot);
      if (rc >= b->klen)
	bp = bttrace(b,b->klen,-1);
    }
    if (b->u.cache.slot <= 0) {
      if (bp->flags & BLK_ROOT || bp->buf.l.lbro == 0L)
	return BTEREOF;
      b->u.cache.node = bp->buf.l.lbro;
      bp->flags |= BLK_LOW;
      btget(1);
      bp = btbuf(b->u.cache.node);
      b->u.cache.slot = bp->cnt();
    }
    break;
  case BTREADLT:
    bp = verify(b);
    if (!bp)
      bp = bttrace(b,b->klen,1);
    for (;;) {
      if (b->u.cache.slot == bp->cnt()) {
	if (bp->flags & BLK_ROOT || bp->buf.l.rbro == 0L)
	  return BTEREOF;
	b->u.cache.slot = 0;
	b->u.cache.node = bp->buf.l.rbro;
	bp->flags |= BLK_LOW;
	btget(1);
	bp = btbuf(b->u.cache.node);
      }
      rc = bp->comp(++b->u.cache.slot,b->lbuf,b->klen);
      if (rc < b->klen) break;
      bp = bttrace(b,b->klen,1);
    }
    break;
  case BTREADGE:
    bp = bttrace(b,b->klen,1);
    if (b->u.cache.slot == 0) {
      if (bp->flags&BLK_ROOT || bp->buf.l.lbro == 0) return BTEREOF;
      b->u.cache.node = bp->buf.l.lbro;
      btget(1);
      bp = btbuf(b->u.cache.node);
      b->u.cache.slot = bp->cnt();
    }
    break;
  case BTREADLE:
    bp = bttrace(b,b->klen,-1);
    if (b->u.cache.slot++ == bp->cnt()) {
      if (bp->flags&BLK_ROOT || bp->buf.l.rbro == 0) return BTEREOF;
      b->u.cache.slot = 1;
      b->u.cache.node = bp->buf.l.rbro;
      btget(1);
      bp = btbuf(b->u.cache.node);
    }
    break;
  case BTOPEN:
#ifdef TRACE
    fprintf(stderr,"uid=%d,gid=%d,mode=%o\n",
	b->u.id.user,b->u.id.group,b->u.id.mode);
#endif
    return openfile(b,0);		/* check permissions */
  case BTCLOSE:
    closefile(b);
#ifdef SINGLE
    return btflush();
#else
    return 0;
#endif
  case BTUMOUNT:
    if (b->root)
      return btunjoin(b);	/* unmount filesystem */
    if (devtbl[b->mid].mcnt > 1) return BTERBUSY;
    return btumount(b->mid);
  case BTMOUNT:
    if (b->root)
      return btjoin(b);
    b->lbuf[b->klen] = 0;
    b->mid = btmount(b->lbuf);
    return 0;
  case BTSTAT:		/* set/extract status information */
    if (b->klen)
      return openfile(b,1);
    if (b->root) {
      if (b->rlen >= sizeof (long)) {
	if (b->rlen > sizeof (struct btstat))
	  b->rlen = sizeof (struct btstat);
	btget(1);
	bp = btbuf(b->root);
	memcpy(b->lbuf,&bp->buf.r.stat,b->rlen);
	return 0;
      }
      return BTERKLEN;
    }
    if (btroot(b->root,b->mid)) return BTERMID;
    b->rlen = devtbl[b->mid].gethdr(b->lbuf,b->rlen);
    return 0;
  case BTTOUCH:
    if (b->rlen >= sizeof (struct btstat)) {
      struct btstat st;
      btget(1);
      bp = btbuf(b->root);
      st = bp->buf.r.stat;
      (void)memcpy((char *)&st,b->lbuf,b->klen);
      bp->buf.r.stat.atime = st.atime;
      bp->buf.r.stat.mtime = st.mtime;
      if (b->u.id.user == bp->buf.r.stat.id.user || b->u.id.user == 0) {
	st.id.mode ^= (st.id.mode ^ bp->buf.r.stat.id.mode) & ~07777;
	bp->buf.r.stat.id = st.id;
      }
      bp->flags |= BLK_MOD;
      (void)memcpy(b->lbuf,(char *)&bp->buf.r.stat,sizeof st);
      return 0;
    }
    return BTERKLEN;
  case BTFLUSH:
    do
      rc = btflush();
    while (rc == BTERBUSY && (b->op & BTEXCL));
    if (rc == 0 && (b->op & BTEXCL))
      return BTERLOCK;
    return rc;
  /* system maintenance functions */
  case BTFREE:		/* add block to free space */
    btget(1);
    bp = bufpool->find(b->u.cache.node,b->mid);
    bp->blk = b->u.cache.node;
    bp->flags = 0;
    btfree(bp);
    return 0;
  /* BTUNLINK is done by setting b->flags at present */
  case BTPSTAT:
    if (b->rlen >= sizeof serverstats)
      b->rlen = sizeof serverstats;
    memcpy(b->lbuf,&serverstats,b->rlen);
    return 0;
  default:
    return BTEROP;	/* invalid operation */
  }

  /* Successful read operations end up here */

  b->rlen = bp->size(b->u.cache.slot);
  assert(b->rlen >= 0 && b->rlen <= maxrec);
  /* FIXME: keep track of BLOCK::dup above where feasible. */
  /* too many places too check whether BLOCK::dup is accurate, use 0 for now */
  bp->copy(b->u.cache.slot,b->lbuf,b->rlen);
  if ((b->flags & BT_DIR) && b->rlen >= node::PTRLEN)
    b->rlen -= node::PTRLEN;	/* hidden root, but user can look if he wants */
  return 0;
}
