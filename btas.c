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
*/
#if !defined(lint) && !defined(__MSDOS__)
static char what[] = "@(#)btas.c	1.12";
#endif

#include "btbuf.h"		/* buffer, btree operations */
#include "node.h"		/* node operations */
#include <bterr.h>		/* user error codes */
#include <btas.h>		/* user interface */
#include "btfile.h"		/* files and directories */
#include "btkey.h"		/* key lookup */

/* NOTE - for historical reasons, higher keys and lower indexes are
	  to the "left", lower keys and higher indexes to the "right".
	  This may change.  */

jmp_buf btjmp;		/* jump target for btpost */

#ifdef TRACE
#include <stdio.h>
#endif

int btas(b,opcode)
  BTCB *b;
{
  register BLOCK *bp;
  int rlen,rc;
  t_block root;

  if ((unsigned)b->klen > b->rlen || (unsigned)b->rlen > maxrec)
    return BTERKLEN;	/* invalid key or record length */
  if (b->klen > MAXKEY)
    b->klen = MAXKEY;	/* truncate long keys */
  if (rc = setjmp(btjmp)) {
    return rc;
  }
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
    if (rc = addrec(b))
      (void)delfile(root);		/* unlink if can't write record */
    return rc;
  case BTWRITE:				/* write unique key */
    if (b->flags & BT_DIR) return BTERDIR;
    return addrec(b);
  case BTDELETE:			/* delete unique key */
    delrec(b,uniquekey(b));
    return 0;
  case BTDELETEF:			/* delete first matching key */
    bp = bttrace(b->root,b->lbuf,b->klen,1);
    if (node_dup < b->klen || sp->slot == 0) return BTEREOF;
    b->u.cache = *sp;
    delrec(b,bp);
    return 0;
  case BTDELETEL:			/* delete last matching key */
    bp = bttrace(b->root,b->lbuf,b->klen,-1);
    if (sp->slot == node_count(bp)
      || node_comp(bp,++sp->slot,b->lbuf,b->klen) < b->klen) return BTEREOF;
    b->u.cache = *sp;
    delrec(b,bp);
    return 0;
  case BTREPLACE: case BTREWRITE:
    bp = uniquekey(b);
    btspace();		/* check for enough free space */
    newcnt = 0;
    if (b->flags & BT_DIR) {
      if (b->rlen > maxrec - sizeof (t_block))
        b->rlen = maxrec - sizeof (t_block);
      node_ldptr(bp,sp->slot,b->lbuf + b->rlen);
      b->rlen += sizeof (t_block);
    }
    switch (opcode) {
    case BTREPLACE:
      if (node_replace(bp,sp->slot,b->lbuf,b->rlen)) {
	--sp->slot;
	btadd(b->lbuf,b->rlen);
      }
      else if (node_free(bp->np,node_count(bp)) >= node_free(bp->np,0)/2)
	btdel();
      break;
    case BTREWRITE:
      rlen = node_size(bp->np,sp->slot);
      if (rlen > b->rlen)
	node_copy(bp,sp->slot,b->lbuf,rlen,b->rlen);
      if (node_replace(bp,sp->slot,b->lbuf,b->rlen)) {
	--sp->slot;
	btadd(b->lbuf,b->rlen);
      }
    }
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
    bp = bttrace(b->root,b->lbuf,b->klen,1);
    if (node_dup < b->klen || sp->slot == 0) return BTERKEY;
    b->u.cache = *sp;
    break;
  case BTREADL:			/* read last matching key */
    bp = bttrace(b->root,b->lbuf,b->klen,-1);
    if (sp->slot == node_count(bp)
      || node_comp(bp,++sp->slot,b->lbuf,b->klen) < b->klen) return BTERKEY;
    b->u.cache = *sp;
    break;
  case BTREADGT:	/* to the left */
    bp = verify(b,-1);
    if (node_dup >= b->klen) {
      if (--b->u.cache.slot <= 0) {
	if (bp->flags & BLK_ROOT || bp->buf.l.lbro == 0L) return BTEREOF;
	b->u.cache.node = bp->buf.l.lbro;
	btget(1);
	bp = btbuf(b->u.cache.node);
	b->u.cache.slot = node_count(bp);
	rc = blkcmp(rptr(bp->np,b->u.cache.slot)+1,
		(unsigned char *)b->lbuf,b->klen);
      }
      else
	rc = *rptr(bp->np,b->u.cache.slot);
      if (rc >= b->klen) {
	bp = bttrace(b->root,b->lbuf,b->klen,-1);
	b->u.cache = *sp;
      }
    }
    if (b->u.cache.slot <= 0) {
      if (bp->flags & BLK_ROOT || bp->buf.l.lbro == 0L) return BTEREOF;
      b->u.cache.node = bp->buf.l.lbro;
      btget(1);
      bp = btbuf(b->u.cache.node);
      b->u.cache.slot = node_count(bp);
    }
    break;
  case BTREADLT:
    bp = verify(b,1);
    for (;;) {
      if (b->u.cache.slot == node_count(bp)) {
	if (bp->flags & BLK_ROOT || bp->buf.l.rbro == 0L) return BTEREOF;
	b->u.cache.slot = 0;
	b->u.cache.node = bp->buf.l.rbro;
	btget(1);
	bp = btbuf(b->u.cache.node);
      }
      rc = node_comp(bp,++b->u.cache.slot,b->lbuf,b->klen);
      if (rc < b->klen) break;
      bp = bttrace(b->root,b->lbuf,b->klen,1);
      b->u.cache = *sp;
    }
    break;
  case BTREADGE:
    bp = bttrace(b->root,b->lbuf,b->klen,1);
    b->u.cache = *sp;
    if (b->u.cache.slot == 0) {
      if (bp->flags&BLK_ROOT || bp->buf.l.lbro == 0) return BTEREOF;
      b->u.cache.node = bp->buf.l.lbro;
      btget(1);
      bp = btbuf(b->u.cache.node);
      b->u.cache.slot = node_count(bp);
    }
    break;
  case BTREADLE:
    bp = bttrace(b->root,b->lbuf,b->klen,-1);
    b->u.cache = *sp;
    if (b->u.cache.slot++ == node_count(bp)) {
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
    return openfile(b);		/* check permissions */
  case BTCLOSE:
    closefile(b);
#ifdef __MSDOS__
    return btflush();
#else
    return 0;
#endif
  case BTUMOUNT:
    return btunjoin(b);	/* unmount filesystem */
  case BTMOUNT:
    return btjoin(b);
  case BTSTAT:		/* set/extract status information */
	/* FIXME: if klen set, stat a pathname */
    if (b->root) {
      if (b->rlen >= sizeof (struct btstat)) {
	struct btstat st;
	btget(1);
	bp = btbuf(b->root);
	(void)memcpy(b->lbuf,(PTR)&bp->buf.r.stat,sizeof st);
	b->rlen = sizeof st;
	return 0;
      }
      return BTERKLEN;
    }
    if (btroot(b->root,b->mid)) return BTERMID;
    b->rlen = gethdr(devtbl + b->mid, b->lbuf, b->rlen);
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
    return btflush();
  /* system maintenance functions */
  case BTFREE:		/* add block to free space */
    btget(1);
    bp = getbuf(b->u.cache.node,b->mid);
    bp->blk = b->u.cache.node;
    bp->flags = BLK_NEW;
    btfree(bp);
    return 0;
  /* BTUNLINK is done by setting b->flags at present */
  case BTPSTAT:
    if (b->rlen >= sizeof stat)
      b->rlen = sizeof stat;
    memcpy(b->lbuf,(PTR)&stat,b->rlen);
    return 0;
  default:
    return BTEROP;	/* invalid operation */
  }

  /* Successful read operations end up here */

  b->rlen = node_size(bp->np,b->u.cache.slot);
  /* FIXME: keep track of node_dup above where feasible. */
  /* too many places too check whether node_dup is accurate, use 0 for now */
  node_copy(bp,b->u.cache.slot,b->lbuf,b->rlen,0);
  if ((b->flags & BT_DIR) && b->rlen >= PTRLEN)
    b->rlen -= PTRLEN;	/* hidden root, but user can look if he wants */
  return 0;
}
