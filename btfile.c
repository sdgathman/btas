#if !defined(lint) && !defined(__MSDOS__)
static char what[] = "%W%";
#endif
/*
	BTAS/2 directory operations

	Ultimately, we want to allow *any* record to be a link
	to another file.  BTWRITE creates a record without a link;
	BTCREATE creates a record with a link to a new empty file.  
	Although a bit per record is available (high order bit of record size),
	it is expensive to mask for normal operations (at least
	in this version).

	For now, we have two types of files.  A file must contain either
	*no* links, or *all* records must be links; I.e. only BTWRITE is
	allowed in an "ordinary" file and only BTCREATE is allowed in a
	"directory" file.  With this restriction, usage is the same as
	for the general case.

	Because it is currently too expensive to check the root node for
	every operation, we keep the file type in a bit of the flag
	word of the BTCB.  This also allows convenient directory maintenance 
	by utilities.  This will not be needed when the bit is maintained
	by record.

	A record with a link may be deleted only if its file contains no links.
	The recursive case can be implemented in the application.

	Rewriting a linked record does not change the link.

	BTLINK creates a record with a link to an existing file.  The
	node field of the BTCB is the root of the existing file.

	BTOPEN starts at the file currently in the BTCB (specified by the
	root node) and follows link names separated by null chars for klen
	chars.  The link names followed by a null character are used as
	keys to the directory records. The entire directory record is
	returned to the user and may contain any arbitrary information.  
	If a (mid,root) is encountered that is in the mount table,
	the path continues with the new filesystem.

	BTMOUNT mounts a filesystem and opens the root file.  The
	user buffer contains the OS file name.  This may be used instead
	of BTOPEN to use OS files for data.  If the OS file does not exist,
	an attempt is made to create it.  If BTAS is the default OS filesystem,
	then lbuf names a block device or partition.

	BTJOIN creates a temorary link by adding the (mid,root) of the
	currently open file to a mount table with the (mid,root) of the
	pathname specified as for BTOPEN.  Any permanent data reachable
	via the pathname is unreachable via BTOPEN while the join is in effect.

	BTUNJOIN removes a filesystem from the mount table and frees the mid.
 * $Log$
 */

#include <string.h>
#include "btbuf.h"
#include "node.h"
#include "btas.h"
#include "bterr.h"
#include "btfile.h"
#include "btkey.h"

#include <time.h>

struct btjoin {
  t_block root;
  short mid, mida;
};

static struct btjoin join[MAXDEV];	/* simple sequential search */
static int joincnt = 0;

int btjoin(BTCB *b) {
  struct btjoin *p;
  int mid;
  if (joincnt >= MAXDEV) return BTERJOIN;
  b->lbuf[b->klen] = 0;
  mid = btmount(b->lbuf);
  p = join + joincnt++;
  p->root = b->root;
  p->mid  = b->mid;
  b->mid = p->mida = mid;
  b->root = 0;
  b->flags = 0;		/* keep original root open */
  return 0;
}

int btunjoin(BTCB *b) {
  register int i,mid = b->mid;
  closefile(b);
  for (i = 0; i < joincnt; ++i)
    if (join[i].mida == mid && devtbl[mid].root == b->root) {
      if (devtbl[mid].mcnt > 1) return BTERBUSY;
      b->root = join[i].root;
      b->mid  = join[i].mid;
      while (++i < joincnt) join[i-1] = join[i];
      --joincnt;
      b->flags = BT_OPEN;
      btroot(b->root,b->mid);
      closefile(b);	/* close root mounted on */
      return btumount(mid);
    }
  return BTERJOIN;	/* not a mounted filesystem */
}

static int check_perm(struct btperm *st,struct btperm *ut,short mode) {
  mode &= 0777;
  if (ut->user == 0) return 1;		/* check for super user */
  if (ut->user == st->user) {		/* check owner perms */
    mode <<= 6;
  }
  else if (ut->group == st->group) {	/* check group perms */
    mode <<= 3;
  }
  return ((mode & st->mode) == mode);
}

#ifdef TRACE
#include <stdio.h>
#endif

int openfile(BTCB *b) {
  register BLOCK *bp;
  register int len;
  register char *p;
  struct btperm id;

  char name[MAXKEY+1];		/* max path element */

  if (b->flags) return BTEROPEN;	/* sanity check */
  len = b->klen;
  (void)memcpy(name,b->lbuf,len);
  name[len] = 0;		/* null terminate path */
  id = b->u.id;

#ifdef TRACE
    fprintf(stderr,"uid=%d,gid=%d,mode=%o,bgid=%d,bmode=%o\n",
	id.user,id.group,id.mode,b->u.id.group,b->u.id.mode);
#endif

  if (b->root == 0L) {		/* find root of filesystem */
    b->root = devtbl[b->mid].root;
    (void)btroot(b->root,b->mid);
  }

  /* follow path */

  for (p = name; len > 0; len -= b->klen, p += b->klen) {
    register int i;
    btget(1);
    bp = btbuf(b->root);
    if ((bp->buf.r.stat.id.mode & BT_DIR) == 0)
      return BTERNDIR;		/* not a directory */

    /* check execute permission for directory */
    if (check_perm(&bp->buf.r.stat.id,&id,BT_EXEC) == 0)
      return BTERPERM;

    b->rlen = b->klen = strlen(p) + 1;
    (void)strcpy(b->lbuf,p);
    b->u.cache.slot = 0;		/* invalidate cache */
    bp = uniquekey(b);		/* lookup name, errors return failing name */
    node_ldptr(bp,sp->slot,&b->root);

    /* check for null link (i.e. symbolic link) */
    if (b->root == 0L) break;

    /* check join table for (b->mid,b->root) - should btroot do it? */
    for (i = 0; i < joincnt; ++i) {
      if (join[i].mid == b->mid && join[i].root == b->root) {
	b->mid = join[i].mida;
	b->root = devtbl[b->mid].root;
	break;
      }
    }
    (void)btroot(b->root,b->mid);		/* new root */
  }

  b->rlen = 0;
  if (b->klen) {			/* if filename */
    b->rlen = node_size(bp->np,sp->slot) - sizeof b->root;
    /* give user dir record */
    node_copy(bp,sp->slot,b->lbuf,b->rlen,node_dup);
  }
  if (b->root == 0L)
    return BTERLINK;

  btget(1);
  bp = btbuf(b->root);

  /* check read/write security */
  if (check_perm(&bp->buf.r.stat.id,&id,id.mode) == 0)
    return BTERPERM;

  /* check for unexpected directory */
  if (bp->buf.r.stat.id.mode & ~id.mode & BT_DIR)
    return BTERDIR;
  id.mode &= ~(~bp->buf.r.stat.id.mode & BT_DIR);

  /* check file locking */

#ifndef SINGLE
  if (bp->buf.r.stat.atime < devtbl[b->mid].mount_time)
    bp->buf.r.stat.opens = 0;			/* reset stale lock */

  if (b->op & BTEXCL) {
    if (bp->buf.r.stat.opens)
      return BTERLOCK;
    bp->buf.r.stat.opens = -1;
    id.mode |= BTEXCL;
  }
  else {
    if (bp->buf.r.stat.opens == -1)
      return BTERLOCK;
    ++bp->buf.r.stat.opens;
  }
  curtime = time(&bp->buf.r.stat.atime);		/* new access time */
  bp->flags |= BLK_MOD;
#endif
  b->flags = id.mode | BT_OPEN;
  ++devtbl[b->mid].mcnt;	/* increment opens to device */
  return 0;			/* open successful */
}

void closefile(BTCB *b) {
  register BLOCK *bp;
  if (b->flags) {
#ifndef SINGLE
    btget(1);
    bp = btbuf(b->root);
    if (bp->buf.r.stat.opens == -1)
      bp->buf.r.stat.opens = 0;
    if (bp->buf.r.stat.opens)
      --bp->buf.r.stat.opens;
    bp->flags |= BLK_MOD;
#endif
    b->flags = 0;
    --devtbl[b->mid].mcnt;
  }
}

t_block creatfile(b)
  BTCB *b;
{
  BLOCK *bp;
  if (b->rlen + PTRLEN > maxrec) btpost(BTERKLEN);
  btget(2);
  bp = btnew(BLK_ROOT);
#ifdef TRACE
  fprintf(stderr,"uid=%d,gid=%d,mode=%o\n",
    b->u.id.user,b->u.id.group,b->u.id.mode);
#endif
#ifdef SAFELINK
  bp->buf.r.stat.level = btbuf(b->root)->buf.r.stat.level + 1;
#endif
  bp->buf.r.stat.id = b->u.id;
  ++bp->buf.r.stat.links;
  bp->flags |= BLK_MOD;
  b->rlen += stptr(bp->buf.r.root,b->lbuf + b->rlen);
  /* caller writes directory record */
  return bp->buf.r.root;
}

int delfile(BTCB *b,t_block root) {
  BLOCK *bp;
  if (root == 0L) return 0;
  btget(2);
  bp = btread(root);
  if (bp->buf.r.root == root) {
    /* check if directory unlink is legal */
    if (bp->buf.r.stat.id.mode & BT_DIR && bp->buf.r.stat.rcnt) {
      /* directory is not empty */
      if (root != b->root) {
	/* not self */
#ifdef SAFELINK
	if (btbuf(b->root)->buf.r.stat.level < bp->buf.r.stat.level)
	  return BTERLINK;
#else
	/* if (btbuf(b->root)->buf.r.stat.rcnt != 1L) return BTERLINK; */
	if (bp->buf.r.stat.links < 2) return BTERLINK;
#endif
      }
    }
    if (--bp->buf.r.stat.links <= 0) {
      if (bp->buf.r.stat.opens) {
	/* we can't delete files that are open */
	/* FIXME: when we have a reliable way to know when clients die,
	   we can delay removal until last close instead as in unix */
	++bp->buf.r.stat.links;
	return BTERDEL;
      }
      btfree(bp);
    }
    else
      bp->flags |= BLK_MOD;
  }
  return 0;
}

t_block linkfile(BTCB *b) {
  if (b->rlen > maxrec - sizeof b->root) btpost(BTERKLEN);
  if (b->u.cache.node) {
    BLOCK *bp;
    btget(2);
    bp = btread(b->u.cache.node);
    if (bp->buf.r.root != b->u.cache.node) btpost(BTERROOT);
#ifdef SAFELINK
    if (bp->buf.r.stat.level > btbuf(b->root)->buf.r.stat.level)
      btpost(BTERLINK);
#endif
    if (++bp->buf.r.stat.links <= 0) {
      --bp->buf.r.stat.links;
      btpost(BTERLINK);
    }
    bp->flags |= BLK_MOD;
  }
  b->rlen += stptr(b->u.cache.node,b->lbuf + b->rlen);
  return b->u.cache.node;
}
