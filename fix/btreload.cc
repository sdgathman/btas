/* $Log$
/* Revision 1.4  2002/11/06 05:13:32  stuart
/* Convert to STL.
/*
/* Revision 1.3  2001/10/17 00:49:32  stuart
/* Minor AIX build bugs, help message for btreload.
/*
/* Revision 1.2  1999/12/14 04:33:03  stuart
/* handle symlinks correctly
/*
 */
#include <stdio.h>
#include <string>
#include <map>
#include "fs.h"
#include "rcvr.h"
using namespace std;
/** A file or directory.  When we see a link to a file, we don't always know
    yet whether it is a file or directory.
 */
class Xrcvr {
  rcvr *parent;	// directory to create file in
  string rec;	// directory record to create
  //t_block root;	// old root of file to link to
  Xrcvr *next;
  friend class FileTable;
  Xrcvr(rcvr *p,const char *buf,int len);
  ~Xrcvr();
};

Xrcvr::Xrcvr(rcvr *p,const char *buf,int len): rec(buf,len) {
  parent = p;
}

Xrcvr::~Xrcvr() { }

class FileTable: rcvr::linkdir {
  typedef map<t_block,rcvr *> RcvrMap;
  typedef map<t_block,Xrcvr *> LinkMap;
  RcvrMap tbl;
  LinkMap linktbl;
  unsigned blksize;
  int maxrec;
  btasFS &fs;
  void link(rcvr *,const char *buf,int len,t_block root);
  void link(rcvr *parent,const char *buf,int len,rcvr *p);
  rcvr *rootdir;
  string lostdir;
public:
  FileTable(btasFS &fs,const char *dir);
  void load();
  ~FileTable();
};

FileTable::FileTable(btasFS &f,const char *dir): lostdir(dir), fs(f) {
  blksize = fs.blksize();
  maxrec = btmaxrec(blksize);
  rootdir = new rcvr;
  t_block rootnode = 1L;
  rootdir->setDir(this);
  rootdir->setName("",rootnode);
  tbl[rootnode] = rootdir;
}

FileTable::~FileTable() {
  for (RcvrMap::iterator i = tbl.begin(); i != tbl.end(); ++i)
    delete i->second;
}

void FileTable::link(rcvr *parent,const char *buf,int len,rcvr *p) {
  t_block root = p->newroot();
  //printf("xlink: %s/%s -> %08lX\n",parent->path(),buf,root);
  if (parent->link(buf,len,root) && p != rootdir)
    btkill(p->path());	// remove path from lost+found
}

void FileTable::link(rcvr *p,const char *buf,int len,t_block root) {
  if (root == 0L) {	// symlink
    p->link(buf,len,root);
    return;
  }
  rcvr *f = tbl[root];
  if (!f) {
    Xrcvr *t = new Xrcvr(p,buf,len);
    t->next = linktbl[root];
    linktbl[root] = t;
    //printf("deflink: %08lX,%s -> %08lX\n",p->rootid(),buf,root);
  }
  else
    link(p,buf,len,f);
}

void FileTable::load() {
  for (;;) {
    union btree *bt = fs.get();
    if (bt == 0) break;
    t_block root = bt->r.root & 0x7FFFFFFFL;
    int t = fs.lasttype();
    rcvr *p = tbl[root];
    if (!p) {
      char buf[32];
      sprintf(buf,"%s/%08lX",lostdir.c_str(),root);
      p = new rcvr;
      if (t & BLKDIR) {
	p->setDir(this);
	//printf("%s is a directory\n",p->path());
      }
      Xrcvr *q = linktbl[root];
      p->setName(buf,root);
      tbl[root] = p;
      // apply queued links to new file
      linktbl.erase(root);
      while (q) {
        link(q->parent,q->rec.c_str(),q->rec.length(),p);
	Xrcvr *n = q->next;
	delete q;
	q = n;
      }
    }
    switch (t & ~BLKDIR) {
    case BLKLEAF + BLKDATA:
      p->donode((union node *)bt->l.data,maxrec,bt->data + blksize);
      break;
    case BLKROOT + BLKDATA:
      p->donode((union node *)bt->r.data,maxrec,bt->data + blksize);
    case BLKROOT:
      //printf("touching %08lX\n",bt->r.root);
      p->touch(&bt->r.stat);
    }
  }
}

int main(int argc,char **argv) {
  if (argc < 2) {
    fputs("\
Usage:	btreload btasimg|-\n\
	Restores all files and directories from a Btas image backup\n\
	relative to the current directory.  All records are extracted\n\
	and reindexed, so this can be used to change block size of\n\
	a filesystem.\n",stderr);
    return 2;
  }
  string imagefile = argv[1];
  const char *lostdir = "lost+found";
  if (btmkdir(lostdir,0755)) {
    fprintf(stderr,"\
Can't create %s directory.  You may wish to delete or rename\n\
any existing file or directory.\n",lostdir);
    return 1;
  }
  btasXFS *fsp = 0;
  if (imagefile == "-")
    fsp = new btasXFS(0,fsio::FS_RDONLY);
  else
    fsp = new btasXFS(imagefile.c_str(),fsio::FS_RDONLY);
  btasXFS &fs = *fsp;
  FileTable tbl(fs,lostdir);
  tbl.load();

  return 0;
}
