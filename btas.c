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

  if (b->klen > b->rlen
    || (unsigned)b->klen > MAXKEY || (unsigned)b->rlen > MAXREC)
    return BTERKLEN;	/* invalid key or record length */
  if (rc = setjmp(btjmp)) {
    return rc;
  }
  if (b->root && btroot(b->root,b->mid)) return BTERMID;
  b->op = opcode;
  opcode &= 31;
  if ( (btopflags[opcode] & b->flags) != btopflags[opcode]) return BTEROP;
  switch (opcode) {
  case BTLINK:
    if ( (b->flags & BT_DIR) == 0) return BTERDIR;
    root = linkfile(b);		/* add root ptr to record */
    if (rc = addrec(b))
      (void)delfile(root);		/* unlink if can't write record */
    return rc;
  case BTCREATE:
    if ( (b->flags & BT_DIR) == 0) return BTERDIR;
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
    newcnt = 0;
    if (b->flags & BT_DIR) {
      if (b->rlen > MAXREC - sizeof (t_block))
        b->rlen = MAXREC - sizeof (t_block);
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
      if (rlen > b->rlen) {
	node_dup = b->rlen;
	node_copy(bp,sp->slot,b->lbuf,rlen);
	(void)node_replace(bp,sp->slot,b->lbuf,b->rlen);
      }
      else if (node_replace(bp,sp->slot,b->lbuf,b->rlen)) {
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
    if (sp->slot == 0
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
	rc = blkcmp(rptr(bp->np,b->u.cache.slot)+1,b->lbuf,b->klen);
      }
      else
	rc = *rptr(bp->np,b->u.cache.slot);
      if (rc >= b->klen) {
	bp = bttrace(b->root,b->lbuf,b->klen,-1);
	b->u.cache = *sp;
      }
    }
    if (b->u.cache.slot <= 0) return BTEREOF;
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
      node_dup = node_comp(bp,++b->u.cache.slot,b->lbuf,b->klen);
      if (node_dup < b->klen) break;
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
      node_dup = 0;
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
    (void)closefile(b);
    return 0;
  case BTUMOUNT:
    return btunjoin(b);	/* unmount filesystem */
  case BTMOUNT:
    return btjoin(b);
  case BTSTAT:		/* extract status information */
    if (b->root) {
      btget(1);
      bp = btbuf(b->root);
      b->rlen = sizeof bp->buf.r.stat;
      (void)memcpy(b->lbuf,(char *)&bp->buf.r.stat,sizeof bp->buf.r.stat);
      return 0;
    }
    if (btroot(b->root,b->mid)) return BTERMID;
    b->rlen = gethdr(devtbl + b->mid, b->lbuf, MAXREC);
    return 0;
  case BTFLUSH:
    btflush();
    return 0;
  /* system maintenance functions */
  case BTFREE:		/* add block to free space */
    btget(1);
    bp = btread((t_block)0);
    bp->blk = b->u.cache.node;
    btfree(bp);
    return 0;
  /* BTUNLINK is done by setting b->flags at present */
  default:
    return BTEROP;	/* invalid operation */
  }

  /* Successful read operations end up here */

  b->rlen = node_size(bp->np,b->u.cache.slot);
  if ((b->flags & BT_DIR) && b->rlen >= sizeof (t_block))
    b->rlen -= sizeof (t_block);	/* hidden root */
  node_copy(bp,b->u.cache.slot,b->lbuf,b->rlen);
  return 0;
}
