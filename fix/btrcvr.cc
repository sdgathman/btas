/*
	Copyright 1990,1991,1992 Business Management Systems, Inc.
	Author: Stuart D. Gathman
	Recover a list of files while BTAS/X is running.

$Log$
# Revision 2.1  1995/12/20  23:48:57  stuart
# *** empty log message ***
#
# Revision 1.7  1993/08/02  19:22:30  stuart
# Fixed: not finding btsaved root nodes
#
# Revision 1.6  1993/07/31  00:49:14  stuart
# fix bug where it undoes things it never did
#
# Revision 1.5  1993/07/29  22:09:19  stuart
# some bugs with hash version
#
# Revision 1.4  1993/07/29  18:54:36  stuart
# use hash table to look up root nodes
#
# Revision 1.3  1993/07/29  18:05:10  stuart
# fix bug restoring from image backup
#

2.0	undo changes if user aborts
	handle wildcards
	use C++

1.4	recover directories
	add recovered records to existing files

*/
#include <stdio.h>
#include <string.h>
#include <time.h>

#include <VHMap.cc>
#include <Map.cc>

extern "C" {
#include <btas.h>
#include "../util/util.h"
}
#include <ftype.h>
#include <bterr.h>
#include <errenv.h>
#include "fs.h"
#include "../node.h"
#include "btfind.h"

static char *fsname(int);
static t_block dirstat(BTCB *, const char *, struct btstat *);

// info about one file to be recovered
// Our constructor disconnects the original file and creates a new
// empty file.
// Our destructor restores the original file if no blocks have been
// written, otherwise the new file has the recovered data.
// If we were interrupted, some data may still be under the old root
// and may be recovered by appending from the old root to the already
// recovered data.

class rcvr {
  char *name;		/* pathname of file being recovered */
  t_block root;		/* old root id */
  long blks;		/* blocks processed */
  long recs;		/* records processed */
  long dups;		/* duplicates */
  struct bttag tag;	/* BT_DIR is turned off */
  char undo;
  friend class rcvrlist;
public:
  int test() const { return !(tag.flags & BT_UPDATE); }
  rcvr(const char *name,long rt,char test = 0,char interactive = 1);
  int donode(BTCB *,NODE *,int,unsigned);
  static void summaryHeader() {
    printf("%8s %8s %8s %s\n","BLKS","RECS","DUPS","PATHNAME");
  }
  void summary() const {
    printf("%8ld %8ld %8ld %s\n",blks,recs,dups,name);
  }
  short mid() const { return tag.mid; }
  ~rcvr();
};

rcvr::rcvr(const char *n,long rt,char test,char interactive) {
  tag.flags = 0;
  blks = 0L;
  recs = 0L;
  dups = 0L;
  root = 0L;
  undo = 0;
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
  BTCB *b = (BTCB *)alloca(btsize(0));
  tag2btcb(b,&tag);
  b->klen = b->rlen = 0;
  btas(b,BTCLOSE);
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

// files to be recovered from one filesystem

inline unsigned hash(t_block t) { return t; }

template class VHMap<t_block,rcvr *>;
template class Map<t_block,rcvr *>;

class rcvrlist {
  rcvrlist *next;
  VHMap<t_block,rcvr *> list;
  const char *imagefile;// image backup file or NULL for in place
  char test,interactive;
  friend class rcvrall;
public:
  rcvrlist(char = 0,char = 1,const char * = 0);
  const char *name() ;
  int add(rcvr *,const char *image = 0); // add a file to recover
  void display();			// display files to recover
  void recover();			// recover files in list
  void summary();			// summarize recovery results
  ~rcvrlist();
};

class rcvrall {
  rcvrlist *list;
  char test,interactive;
public:
  rcvrall(char t = 0,char i = 1): test(t), interactive(i), list(0) {}
  void add(const char *,long,const char * = 0);
  void display();
  void recover();
  ~rcvrall();
};

void rcvrall::add(const char *name,long rt,const char *image) {
#ifdef __MSDOS__
  while (coreleft() < 20480) {
    rcvrlist *l = list;
    puts("Not enough memory");
    if (!l) return;
    puts("  - need to recover some files now.");
    l->recover();
    list = l->next;
    delete l;
  }
#endif
  rcvr *p = new rcvr(name,rt,test,interactive);
  if (p) for (rcvrlist **l = &list; ; l = &(*l)->next) {
    if (!*l) {
      *l = new rcvrlist(test,interactive,image);
      if (!*l) {
	delete p;
	return;
      }
    }
    if (!(*l)->add(p,image)) break;
  }
}

void rcvrall::display() {
  for (rcvrlist *p = list; p; p = p->next)
    p->display();
}

void rcvrall::recover() {
  rcvrlist *p;
  while (p = list) {
    p->recover();
    list = p->next;
    delete p;
  }
}

rcvrall::~rcvrall() {
  rcvrlist *p;
  while (p = list) {
    list = p->next;
    delete p;
  }
}

rcvrlist::rcvrlist(char t,char interact,const char *name):
  list((rcvr *)0,100),
  test(t), interactive(interact)
{
  next = 0;
  imagefile = name;
}

const char *rcvrlist::name() {
  if (imagefile) return imagefile;
  Pix i = list.first();
  if (!i) return 0;
  return fsname(list.contents(i)->mid());
}

rcvrlist::~rcvrlist() {
  for (Pix i = list.first(); i; list.next(i))
    delete list.contents(i);
}

int rcvrlist::add(rcvr *p,const char *image) {
  if (imagefile && strcmp(image,imagefile)) return 1;	// not added
  if (p) {
    Pix i = list.first();
    if (!imagefile && i && p->mid() != list.contents(i)->mid()) return 1;
    list[p->root] = p;
    return 0;
  }
  return 1;
}

// display what we are about to do for confirmation

void rcvrlist::display() {
  printf("%16s %8s %8s %s\n","OS file","Old root","New root","Path");
  const char *devname;
  Pix i = list.first();
  if (!i) return;
  if (imagefile)
    devname = imagefile;
  else
    devname = fsname(list.contents(i)->mid());
  while (i) {
    const rcvr *p = list.contents(i);
    printf("%16s%9lx%9lx %s\n",devname,p->root,p->tag.root,p->name);
    devname="";		/* list each fs only once */
    list.next(i);
  }
}

void rcvrlist::recover() {
  const char *devname = name();
  if (!devname) return;	// nothing to do

  display();
  if (test)
    printf("test: ");
  if (interactive) {
    if (question("Continue with recover? ") == 0) return;
  }
  else
    puts("Continuing with recover.");

  while (btsync() == BTERBUSY);	// ensure our changes are on disk
  btasXFS fs(devname,
	isatty(0) ? fsio::FS_RDONLY : fsio::FS_RDONLY + fsio::FS_BGND);
  if (!fs) {
    fprintf(stderr,"%s: not a BTAS/X filesystem.\n",devname);
    return;
  }
  printf("Processing %s\n",devname);

  union btree *bt;
  unsigned blksize = fs.blksize();
  int maxrec = btmaxrec(blksize);
  BTCB *b = (BTCB *)alloca(btsize(maxrec));
  while (bt = fs.get()) {
    t_block blk = fs.lastblk();
    Pix i = list.seek(bt->r.root&0x7FFFFFFFL);
    if (i) {
      rcvr *p = list.contents(i);
      tag2btcb(b,&p->tag);
      switch (fs.lasttype()) {
      case BLKLEAF:
	p->donode(b,(NODE *)bt->l.data,maxrec,
	      blksize - ((char *)bt->l.data - bt->data));
	break;
      case BLKROOT:
	p->donode(b,(NODE *)bt->r.data,maxrec,
	      blksize - ((char *)bt->r.data - bt->data));
	break;
      }
      if (!imagefile) {
	b->u.cache.node = blk;
	if (test)
	  printf("test: FREE %08lX\n",blk);
	else {
	  b->klen = b->rlen = 0;
	  btas(b,BTFREE);
	}
      }
    }
  }
  summary();
}

void rcvrlist::summary() {
  Pix i = list.first();
  if (!i) return;
  printf("\nSummary for filesystem %s:\n",name());
  rcvr::summaryHeader();
  while (i) {
    list.contents(i)->summary();
    list.next(i);
  }
}

// write records in a node to recovered file.
int rcvr::donode(BTCB *b,NODE *np,int maxrec,unsigned blksize) {
  ++blks;
  int cnt = np->size();
  if (cnt <= 0) return 0;
  np->setsize(blksize);
  for (int i = 1; i <= cnt; ++i)	// offsets must be strictly descending
    if (np->free(i) >= np->free(i-1)) return 0;
  if (np->free(cnt) < 0) return 0;
  if (*np->rptr(cnt)) return 0;	/* first dup count must be zero */
  while (--i) {
    unsigned char *p = np->rptr(i);
    b->rlen = np->size(i);
    if (b->rlen > maxrec) break;
    memcpy(b->lbuf + *p,p + 1,b->rlen - *p);
    b->klen = b->rlen;
    if (test()) {
      if (btas(b,BTREADF + NOKEY) == 0)
	++dups;
    }
    else if (btas(b,BTWRITE + DUPKEY))
      ++dups;
  }
  recs += cnt;
  return cnt;
}

static char *fsname(int mid) {
  BTCB b;
  struct btfs f;
  static char name[18];
  b.root = 0;
  b.mid = mid;
  b.flags = 0;
  b.rlen = MAXREC;
  b.klen = 0;
  envelope
    btas(&b,BTSTAT);
    if (b.rlen > sizeof f) b.rlen = sizeof f;
    (void)memcpy((char *)&f,b.lbuf,b.rlen);
    strncpy(name,f.dtbl[0].name,sizeof f.dtbl[0].name);
  enverr
    return 0;
  envend
  return name;
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

static GOTO usage() {
  puts("\
Usage:	btrcvr [-t] [file]\n\
	-t	test mode, writes will be suppressed\n\
	file	the OS name of an image backup\n\
");
  exit(1);
}

extern "C" void (* cdecl set_new_handler(void (*new_handler)(void)))();

void nomem() {
  puts("Out of memory!");
  set_new_handler(0);
}

int main(int argc,char **argv) {
  char interactive = !!isatty(0);
  char test = 0;		/* true if test mode (no writes) */
  const char *imagefile = 0;	/* true if restoring files from image backup */
  puts("Btas/X file recovery program\n\
	$Id$");
  while (argc > 1 && *argv[1] == '-') {
    --argc; ++argv;
    switch (*++*argv) {
    case 't': case 'T':
      test = 1;
      break;
    default:
      usage();
    }
  }
  switch (argc) {
  case 2:
    printf("Recovering files from image backup %s\n",imagefile = argv[1]);
    break;
  default:
    usage();
  case 1: case 0:
    ;
  }
  if (test)
    puts("test: no actual writes will take place.");

  // get files to recover
  rcvrall list(test,interactive);
  for (int i = 0; ; ++i) {
    set_new_handler(nomem);
    if (interactive) {
      printf("Enter name of #%d file to recover.\n",i+1);
      puts("If the file already exists and you supply a root id,");
      puts("records will be appended to the existing file.");
      printf("Name (Ctrl-D or // to end): ");
    }
    const char *name = readtext("");
    if (!name) break;
    if (strcmp(name,"//") == 0) break;
    // can't use findfirst because rcvr objects can't undo changes
    // when BTASDIR changes.
    btFind ff(name);
    for (int rc = ff.first(); !rc; rc = ff.next()) {
      if (strcmp(ff.name(),basename(name)) == 0) {
	t_block rt = strtol(readtext("Enter root id (0 for same): "),0,16);
	list.add(name,rt,imagefile);
      }
      else {
	char buf[MAXKEY];
	ff.path(buf);
	list.add(buf,0,imagefile);
      }
    }
  }
  puts("");
  if (i == 0) {
    puts("Nothing to do.");
    return 0;
  }

  list.recover();
  return 0;
}
