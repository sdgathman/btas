/* $Log$
 */
#pragma implementation
#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include <errenv.h>
#include <bterr.h>
#include <ftype.h>
#include "fix.h"
#include "../node.h"
#include "rcvr.h"
#include <new.h>

static t_block dirstat(BTCB *, const char *, struct btstat *);

rcvr::rcvr() {
  tag.flags = 0;
  blks = 0L;
  recs = 0L;
  dups = 0L;
  root = 0L;
  dir  = 0;
  name = 0;
  undo = false;
}

void rcvr::setName(const char *n,long rt) {
  if (!name) {
    new(this) rcvr(n,rt,false,false);
  }
}

rcvr::rcvr(const char *n,long rt,bool test,bool interactive) {
  new(this) rcvr;
  name = new char[strlen(n) + 1];
  if (!name) return;
  strcpy(name,n);

  BTCB * volatile dirf = 0;

  envelope
    const char *p = basename(n);
    int mode = (test) ? BTRDONLY : BTRDWR;

    /* open directory */
    dirf = btopendir(n,mode);
    /* check filename and get root id if required */
    if (dirf->flags & BT_DIR) {
      struct btstat st;
      dirf->rlen = MAXREC;
      root = dirstat(dirf,p,&st);
      if (root == -1L) {
	printf("%s: different filesystem.\n",p);
	errpost(BTERMID);
      }
      int rc = btas(dirf,BTREADEQ + NOKEY);
      if (rc == 0) {
	if (interactive)
	  puts("Records will be added to the existing file.");
	if (!rt || !root) {
	/* unlink filename */
	  if (test)
	    printf("test: UNLINK %s\n",dirf->lbuf);
	  else {
	    dirf->flags ^= BT_DIR;
	    undo = (btas(dirf,BTDELETE + NOKEY) == 0);
	    dirf->flags ^= BT_DIR;
	  }
	}
	if (rt)
	  root = rt;
      }
      else
	dirf->rlen = dirf->klen;	/* no field table */
      if (root == 0) {
	printf("%s: not found.\n",p);
	if (interactive) {
	  puts("NOTE: the recovered file will have no field table,");
	  puts("      but the records will be correct.");
	}
	root = rt;
      }
      if (test) {
	if (rc)
	  printf("test: CREATE %s\n",dirf->lbuf);
      }
      else {
	dirf->u.id = st.id;
	(void)btas(dirf,BTCREATE + DUPKEY);	/* create new file */
      }
      BTCB b;
      btopenf(&b,name,mode + BTDIROK,sizeof b.lbuf);
      btcb2tag(&b,&tag);
      printf("%s: root %08lX from %08lX\n",p,tag.root,root);
      tag.flags &= ~BT_DIR;	/* treat directories as files */
    }
    else {
      printf("%s: invalid path\n",name);
    }
  enverr
    printf("%s: errno %d\n",name,errno);
  envend
  btclose(dirf);
}

rcvr::~rcvr() {
  if (tag.flags) {
    BTCB *b = (BTCB *)alloca(btsize(0));
    tag2btcb(b,&tag);
    b->klen = b->rlen = 0;
    btas(b,BTCLOSE);
  }
  // no recovered data yet, restore original root
  if (blks == 0L && !test() && undo) {
    BTCB * volatile dirf = 0;
    envelope
      dirf = btopendir(name,BTRDWR);
      dirf->rlen = MAXREC;
      int rc = btas(dirf,BTREADEQ + NOKEY);	// get dir rec
      if (rc == 0) {
	(void)btas(dirf,BTDELETE + NOKEY);	// delete new root
	stlong(root,dirf->lbuf + dirf->rlen);
	dirf->rlen += 4;
	dirf->flags ^= BT_DIR;
	(void)btas(dirf,BTWRITE + DUPKEY);	// restore old root
	dirf->flags ^= BT_DIR;
      }
    envend
    btclose(dirf);
  }
  delete [] name;
}

void rcvr::setDir(linkdir *dir) {
  if (dir == this->dir) return;
  this->dir = dir;
  if (dir)
    tag.flags |= BT_DIR;	/* treat as directory */
  else
    tag.flags &= ~BT_DIR;	/* treat as file */
}

void rcvr::write(BTCB *b) {
  if (dir) {
    int len = b->rlen;
    if (len <= 4) return;	// garbage directory record
    len -= 4;
    t_block root = ldlong(b->lbuf + len);
    dir->link(b->lbuf,len,root);
    return;
  }
  if (test()) {
    if (btas(b,BTREADF + NOKEY) == 0)
      ++dups;
    return;
  }
  if (btas(b,BTWRITE + DUPKEY))
    ++dups;
}

// write records in a node to recovered file.
int rcvr::donode(NODE *np,int maxrec,void *blkend) {
  ++blks;
  int cnt = np->size();
  if (cnt <= 0) return 0;
  np->setsize(blkend);
  int i;
  for (i = 1; i <= cnt; ++i)	// offsets must be strictly descending
    if (np->free(i) >= np->free(i-1)) return 0;
  if (np->free(cnt) < 0) return 0;
  if (*np->rptr(cnt)) return 0;	/* first dup count must be zero */
  BTCB *b = (BTCB *)alloca(btsize(maxrec));
  tag2btcb(b,&tag);
  while (--i) {
    unsigned char *p = np->rptr(i);
    b->rlen = np->size(i) + *p;
    if (b->rlen > maxrec) break;
    memcpy(b->lbuf + *p,p + 1,b->rlen - *p);
    b->klen = b->rlen;
    write(b);
  }
  recs += cnt;
  return cnt;
}

void rcvr::summaryHeader() {
  printf("%8s %8s %8s %s\n","BLKS","RECS","DUPS","PATHNAME");
}

void rcvr::summary() const {
  printf("%8ld %8ld %8ld %s\n",blks,recs,dups,name);
}

static t_block dirstat(BTCB *dirf,const char *name,struct btstat *st) {
  BTCB b;
  int rc;
  b.root = dirf->root;
  b.mid = dirf->mid;
  b.flags = 0;
  b.u.id.user = 0;
  b.u.id.group = 0;
  b.u.id.mode = BT_DIR + 04;
  b.rlen = b.klen = strlen(name);
  (void)memcpy(b.lbuf,name,b.klen);
  catch(rc)
    btas(&b,BTOPEN);
    b.rlen = sizeof *st;
    b.klen = 0;
    btas(&b,BTSTAT);
    (void)memcpy((char *)st,b.lbuf,sizeof *st);
    btas(&b,BTCLOSE);
  enverr
    if (rc == 209)
      b.root = 0L;
    printf("%s: STAT failed (rc == %d), using defaults\n",name,rc);
    st->bcnt = 0;	/* can't get status */
    st->rcnt = 0;
    st->mtime = st->atime = time(&st->ctime);
    st->links = 0;
    st->opens = 0;
    st->id.user = getuid();
    st->id.group = getgid();
    st->id.mode = 0666;
  envend
  if (b.mid == dirf->mid && b.root == dirf->root)
    return 0L;	/* doesn't exist */
  if (b.mid != dirf->mid)
    return -1L;	/* different file system */
  return b.root;
}

void rcvr::free(t_block blk) {
  BTCB *b = (BTCB *)alloca(btsize(0));
  tag2btcb(b,&tag);
  b->klen = b->rlen = 0;
  b->u.cache.node = blk;
  if (test())
    printf("test: FREE %08lX\n",blk);
  else
    btas(b,BTFREE);
}
