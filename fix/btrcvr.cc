/*
	Copyright 1990,1991,1992 Business Management Systems, Inc.
	Author: Stuart D. Gathman
	Recover a list of files while BTAS/X is running.

$Log$
Revision 2.6  2003/03/05 04:45:02  stuart
Minor fixes.

Revision 2.5  2003/03/04 16:03:16  stuart
Convert btrcvr to use STL.

Revision 2.4  2001/02/28 22:46:38  stuart
use new.h

 * Revision 2.3  1999/05/10  22:41:19  stuart
 * *** empty log message ***
 *
# Revision 2.2  1998/01/14  20:13:09  stuart
# Use btasXFS class
#
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
#include <unistd.h>
#include <string.h>
#include <new>
#include <time.h>
#include <map>
using namespace std;
extern "C" {
#include <btas.h>
#include "../util/util.h"
}
#include <bterr.h>
#include <errenv.h>
#include "fs.h"
#include "rcvr.h"
#include "../node.h"
#include "btfind.h"

static char *fsname(int);

// files to be recovered from one filesystem

template class map<t_block,rcvr *>;
typedef map<t_block,rcvr *>::iterator Pix;

class rcvrlist {
  rcvrlist *next;
  map<t_block,rcvr *> list;
  const char *imagefile;// image backup file or NULL for in place
  bool test,interactive;
  friend class rcvrall;
public:
  rcvrlist(bool = false,bool = true,const char * = 0);
  const char *name() ;
  bool add(rcvr *,const char *image = 0); // add a file to recover
  rcvr *get(t_block root);
  void display();			// display files to recover
  void recover();			// recover files in list
  void summary();			// summarize recovery results
  ~rcvrlist();
};

class rcvrall {
  rcvrlist *list;
  bool test,interactive;
public:
  rcvrall(bool t = false,bool i = true): test(t), interactive(i), list(0) {}
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
  while ((p = list) != 0) {
    p->recover();
    list = p->next;
    delete p;
  }
}

rcvrall::~rcvrall() {
  rcvrlist *p;
  while ((p = list) != 0) {
    list = p->next;
    delete p;
  }
}

rcvrlist::rcvrlist(bool t,bool interact,const char *name):
  test(t), interactive(interact)
{
  next = 0;
  imagefile = name;
}

const char *rcvrlist::name() {
  if (imagefile) return imagefile;
  Pix i = list.begin();
  if (i == list.end()) return 0;
  return fsname(i->second->mid());
}

rcvrlist::~rcvrlist() {
  for (Pix i = list.begin(); i != list.end(); ++i)
    delete i->second;
}

bool rcvrlist::add(rcvr *p,const char *image) {
  if (imagefile && strcmp(image,imagefile))
    return true;	// can't recover from different source images
  if (p) {
    Pix i = list.begin();
    if (!imagefile && i != list.end() && p->mid() != i->second->mid())
      return true;	// can't recover in place on different fs
    list[p->rootid()] = p;
    return false;
  }
  return true;
}

// display what we are about to do for confirmation

void rcvrlist::display() {
  printf("%16s %8s %8s\n","OS file","Old root","Path");
  const char *devname;
  Pix i = list.begin();
  if (i == list.end()) return;
  if (imagefile)
    devname = imagefile;
  else
    devname = fsname(i->second->mid());
  while (i != list.end()) {
    const rcvr *p = i->second;
    printf("%16s%9lx %s\n",devname,p->rootid(),p->path());
    devname="";		/* list each fs only once */
    ++i;
  }
}

rcvr *rcvrlist::get(t_block root) {
  Pix i = list.find(root);
  if (i != list.end()) return i->second;
  return 0;
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
  while ((bt = fs.get()) != 0) {
    t_block blk = fs.lastblk();
    rcvr *p = get(bt->r.root&0x7FFFFFFFL);
    if (p) {
      int t = fs.lasttype();
      /* directories are recovered, but links to them need to be patched.
       */
      //if (t & BLKDIR) continue;
      switch (t) {
      case BLKLEAF + BLKDATA:
	p->donode((NODE *)bt->l.data,maxrec,blksize + bt->data);
	break;
      case BLKROOT + BLKDATA:
	p->donode((NODE *)bt->r.data,maxrec,blksize + bt->data);
	break;
      }
      if (!imagefile)
	p->free(blk);
    }
  }
  summary();
}

void rcvrlist::summary() {
  Pix i = list.begin();
  if (i == list.end()) return;
  printf("\nSummary for filesystem %s:\n",name());
  rcvr::summaryHeader();
  while (i != list.end()) {
    i->second->summary();
    ++i;
  }
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

static GOTO usage() {
  puts("\
Usage:	btrcvr [-t] [file]\n\
	-t	test mode, writes will be suppressed\n\
	file	the OS name of an image backup\n\
");
  exit(1);
}

void nomem() {
  puts("Out of memory!");
  set_new_handler(0);
}

int main(int argc,char **argv) {
  bool interactive = !!isatty(0);
  bool test = false;		/* true if test mode (no writes) */
  const char *imagefile = 0;	/* true if restoring files from image backup */
  puts("Btas/X file recovery program\n\
	$Id$");
  while (argc > 1 && *argv[1] == '-') {
    --argc; ++argv;
    switch (*++*argv) {
    case 't': case 'T':
      test = true;
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
  int i;
  for (i = 0; ; ++i) {
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
