#ifndef __MSDOS__
static const char what[] =
  "$Id$";
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
 * Revision 2.5  2001/02/28 21:26:35  stuart
 * support .. from mounted directory
 *
 * Revision 2.4  1999/01/25 18:01:50  stuart
 * return enough info for library to implement symlinks
 *
 * Revision 2.3  1999/01/22  22:40:09  stuart
 * fix opencnt updating
 *
 * Revision 2.2  1997/06/23  15:30:00  stuart
 * implement btfile object
 *
 * Revision 2.1  1996/12/17  16:46:23  stuart
 * C++ node interface
 *
 * Revision 1.5  1995/08/25  18:46:10  stuart
 * some prototypes
 *
 * Revision 1.4  1994/03/28  20:13:55  stuart
 * support BTSTAT with pathname
 *
 * Revision 1.3  1993/12/09  19:32:23  stuart
 * use RCS Id
 *
 * Revision 1.2  1993/05/14  16:19:23  stuart
 * make symbolic link really work after testing
 *
 */
#pragma implementation
#include <string.h>
#include <btas.h>
#include <bterr.h>
#include "btserve.h"
#include "btbuf.h"
#include "btfile.h"
#include "btdev.h"

#include <time.h>

struct btfile::joinrec {
  t_block root;
  /* We don't store roota, because we don't currently allow clients
   * general access to the join table.  Since the root directory is 
   * currently always node 1, roota is always 1. 
   */
  short mid, mida;
};

btfile::btfile(BlockCache *c): bttrace(c) {
  joincnt = 0;
  jointbl = new joinrec[MAXDEV];
}

int btfile::join(BTCB *b) {
  if (joincnt >= MAXDEV) return BTERJOIN;
  b->lbuf[b->klen] = 0;
  int mid = bufpool->mount(b->lbuf);
  joinrec *p = jointbl + joincnt++;
  p->root = b->root;
  p->mid  = b->mid;
  b->mid = p->mida = mid;
  b->root = 0;
  b->flags = 0;		/* keep original root open */
  return 0;
}

int btfile::unjoin(BTCB *b) {
  int mid = b->mid;
  closefile(b);
  for (int i = 0; i < joincnt; ++i)
    if (jointbl[i].mida == mid && b->root == 1L) {
      if (devtbl[mid].mcnt > 1) return BTERBUSY;
      b->root = jointbl[i].root;
      b->mid  = jointbl[i].mid;
      while (++i < joincnt) jointbl[i-1] = jointbl[i];
      --joincnt;
      b->flags = BT_OPEN;
      bufpool->btroot(b->root,b->mid);
      closefile(b);	/* close root mounted on */
      return bufpool->unmount(mid);
    }
  return BTERJOIN;	/* not a mounted filesystem */
}

static int check_perm(btperm *st,btperm *ut,short mode) {
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

int btfile::openfile(BTCB *b,int stat) {
  BLOCK *bp;
  int len, rlen = b->rlen;
  char *p;
  btperm id;

  char name[MAXKEY+1];		/* max path element */

  if (b->flags) return BTEROPEN;	/* sanity check */
  len = b->klen;
  memcpy(name,b->lbuf,len);
  name[len] = 0;		/* null terminate path */
  id = b->u.id;

#ifdef TRACE
    fprintf(stderr,"uid=%d,gid=%d,mode=%o,bgid=%d,bmode=%o\n",
	id.user,id.group,id.mode,b->u.id.group,b->u.id.mode);
#endif

  if (b->root == 0L) {		/* find root of filesystem */
    b->root = 1L;
    bufpool->btroot(b->root,b->mid);
  }

  /* follow path */

  t_block newroot = b->root;
  for (p = name; len > 0; len -= b->klen, p += b->klen) {
    // check for .. from join table target
    if (!strcmp(p,"..") && b->root == 1L) {	
      for (int i = 0; i < joincnt; ++i) {
	if (jointbl[i].mida == b->mid /* && jointbl[i].roota == b->root */) {
	  b->mid = jointbl[i].mid;
	  b->root = jointbl[i].root;
	  bufpool->btroot(b->root,b->mid);		/* new root */
	  break;
	}
      }
    }
    btget(1);
    bp = btbuf(b->root);
    if ((bp->buf.r.stat.id.mode & BT_DIR) == 0)
      return BTERNDIR;		/* not a directory */

    /* check execute permission for directory */
    if (check_perm(&bp->buf.r.stat.id,&id,BT_EXEC) == 0)
      return BTERPERM;

    b->rlen = b->klen = strlen(p) + 1;
    strcpy(b->lbuf,p);
    b->u.cache.slot = 0;		/* invalidate cache */
    bp = uniquekey(b);		/* lookup name, errors return failing name */
    newroot = bp->ldptr(sp->slot);

    /* check for null link (i.e. symbolic link) */
    if (newroot == 0L) break;
    b->root = newroot;

    /* check join table for (b->mid,b->root) - should btroot do it? */
    for (int i = 0; i < joincnt; ++i) {
      if (jointbl[i].mid == b->mid && jointbl[i].root == b->root) {
	b->mid = jointbl[i].mida;
	b->root = 1L;
	break;
      }
    }
    bufpool->btroot(b->root,b->mid);		/* new root */
  }

  b->rlen = 0;
  if (b->klen) {			/* if filename */
    b->rlen = bp->size(sp->slot) - sizeof b->root;
    /* give user dir record */
    bp->copy(sp->slot,b->lbuf,b->rlen,BLOCK::dup);
  }
  if (newroot == 0L) {
    b->klen = len;	// return how much of path still unused
    return BTERLINK;
  }

  btget(1);
  bp = btbuf(b->root);

  if (bp->buf.r.stat.atime < devtbl[b->mid].mount_time)
    bp->buf.r.stat.opens = 0;			/* reset stale lock */

  if (stat) {
    if (rlen < sizeof (struct btstat)) return BTERKLEN;
    memcpy(b->lbuf,(PTR)&bp->buf.r.stat,sizeof (struct btstat));
    b->rlen = sizeof (struct btstat);
    return 0;
  }
  /* check read/write security */
  if (check_perm(&bp->buf.r.stat.id,&id,id.mode) == 0)
    return BTERPERM;

  /* check for unexpected directory */
  if (bp->buf.r.stat.id.mode & ~id.mode & BT_DIR)
    return BTERDIR;
  id.mode &= ~(~bp->buf.r.stat.id.mode & BT_DIR);

  /* check file locking */

#ifndef SINGLE
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
  btserve::curtime = time(&bp->buf.r.stat.atime);	// new access time
  bp->flags |= BLK_MOD;
#endif
  b->flags = id.mode | BT_OPEN;
  ++devtbl[b->mid].mcnt;	/* increment opens to device */
  return 0;			/* open successful */
}

void btfile::closefile(BTCB *b) {
  if (b->flags) {
#ifndef SINGLE
    btget(1);
    BLOCK *bp = btbuf(b->root);
    if (bp->buf.r.stat.opens == -1) {
      bp->buf.r.stat.opens = 0;
      bp->flags |= BLK_MOD;
      --devtbl[b->mid].mcnt;
    }
    else if (bp->buf.r.stat.opens) {
      --bp->buf.r.stat.opens;
      bp->flags |= BLK_MOD;
      --devtbl[b->mid].mcnt;
    }
#else
    --devtbl[b->mid].mcnt;
#endif
    b->flags = 0;
  }
}

t_block btfile::creatfile(BTCB *b) {
  if (b->rlen + node::PTRLEN > bufpool->maxrec) btpost(BTERKLEN);
  btget(2);
  BLOCK *bp = bufpool->btnew(BLK_ROOT);
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

int btfile::delfile(BTCB *b,t_block root) {
  if (root == 0L) return 0;
  // check that target is not mounted
  for (int i = 0; i < joincnt; ++i) {
    if (jointbl[i].mid == b->mid && jointbl[i].root == root)
      return BTERDEL;
  }
  btget(2);
  BLOCK *bp = bufpool->btread(root);
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
      bufpool->btfree(bp);
    }
    else
      bp->flags |= BLK_MOD;
  }
  return 0;
}

t_block btfile::linkfile(BTCB *b) {
  if (b->rlen > bufpool->maxrec - sizeof b->root) btpost(BTERKLEN);
  if (b->u.cache.node == 1L) {
    // prevent linking a filesystem root
    btpost(BTERLINK);
  }
  // a link to a 0 root is a symlink
  if (b->u.cache.node) {
    btget(2);
    BLOCK *bp = bufpool->btread(b->u.cache.node);
    if (bp->buf.r.root != b->u.cache.node) btpost(BTERROOT);
#ifdef SAFELINK
    // SAFELINK was an attempt to prevent cycles when linking directories
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
