#pragma implementation
#include <stdio.h>
#include "Obstack.h"
extern "C" {
#include <search.h>
#include <port.h>
}
#include <string.h>
#include "logdir.h"
/* #define TRACE /**/

enum { treeflag = false };

struct dir_n {
  Logdir::root_n *file;
  struct dir_n *next;
  short rlen;
  char buf[MAXREC];
};

static int cmp(const PTR a, const PTR b) {
  if (*(t_block *)a > *(t_block *)b) return 1;
  if (*(t_block *)a < *(t_block *)b) return -1;
  return 0;
}

Logdir::Logdir(): h(*new Obstack) {
  t = 0;
  lostcnt = 0L;
  path = (char *)h.alloc(MAXKEY + 2);
  memset(path,0,sizeof path);
}

Logdir::~Logdir() {
  delete &h;
}

void Logdir::doroot(root_n *) { }

Logdir::root_n *Logdir::addroot(long blk, const struct btstat *st) {
  root_n *r = (root_n *)h.alloc(sizeof *r);
  r->root = blk;
  root_n *p = *(root_n **)tsearch(r,&t,cmp);
  if (p == r) {
    if (!st)
      memset(&r->st,0,sizeof r->st);
    r->dir = 0;
    r->last = 0;
    r->path = 0;
    r->bcnt = 0L;
    r->rcnt = 0L;
    r->links = 0;
  }
  else {
    h.free(r);
    r = (root_n *)p;
  }
  if (st) {
    r->st = *st;
  }
  return r;
}

void Logdir::logroot(t_block blk,const struct btstat *st) {
  root_n *r = addroot(blk,st);
#ifdef TRACE
  printf("%ld %05o %4d %4d\n",
	r->root,r->st.id.mode,r->st.id.user,r->st.id.group);
#endif
}

void Logdir::logrecs(t_block blk,int cnt) {
  root_n *r = addroot(blk,0);
  ++r->bcnt;
  if (cnt > 0)
    r->rcnt += cnt;
}

void Logdir::logdir(t_block root,const char *buf,int len,t_block blk) {
  struct dir_n *n, **p;
  root_n *r = addroot(root,0);
  n = (dir_n *)h.alloc(sizeof *n - sizeof n->buf + len + 1);
  n->rlen = len;
  memcpy(n->buf,buf,len);
  n->buf[len] = 0;
  if (blk != 0) {
    n->file = addroot(blk,0);
    ++n->file->links;
  }
  else
    n->file = 0;
  if (r->last && strcmp(r->last->buf,n->buf) < 0)
    p = &r->last->next;
  else
    p = &r->dir;
  while (*p && strcmp((*p)->buf,n->buf) > 0)
    p = &(*p)->next;
  n->next = *p;
  r->last = *p = n;
#ifdef TRACE
  printf("%ld %s --> %ld\n",root,buf,blk);
#endif
}

long Logdir::printroot(struct root_n *r,int lev,const char *name) {
  struct dir_n *d;
  long cnt;
  if (treeflag)
    printf("%08lX %05o%5d%5d%*s%s\n",
      r->root, r->st.id.mode, r->st.id.user, r->st.id.group,++lev," ",name);
  else {
    if (lev > 1 || lev && *path != '/') path[lev++] = '/';
    strcpy(path+lev,name);
    lev += strlen(name);
    if (r->st.links == -1)
      r->st.links = r->links;
    printf("%08lX %05o%5d%5d%8ld%3d %s",
      r->root,r->st.id.mode,r->st.id.user,r->st.id.group,
      r->st.bcnt,r->st.links,path
    );
  }
  if (r->path) {
    printf(" --> %s\n",r->path);
    return 0L;
  }
  putchar('\n');

  if (
    r->links != r->st.links ||
    r->bcnt  != r->st.bcnt  ||
    r->rcnt  != r->st.rcnt
  ) {
    printf("%8s%8ld records%8ld%3d\n","********", r->rcnt,r->bcnt,r->links);
  }

  if (r->links > 1)
    r->path = strdup(path);
  else
    r->path = path;
  doroot(r);
  cnt = r->bcnt;
  for (d = r->dir; d; d = d->next) {
    if (d->file)
      cnt += printroot(d->file,lev,d->buf);
    else
      printf("%s --> %s\n",d->buf,d->buf + strlen(d->buf) + 1);
  }
  return cnt;
}

static void loghdr(const char *title) {
  printf("%8s %5s%5s%5s%8s%3s %s\n",
    "Root","Mode","Owner","Group","Blocks","Links",title);
}

static Logdir *log;

void Logdir::logchk(root_n *r) {
  if (r->path == 0) {
    puts("");
    loghdr("DISCONNECTED TREE");
    ++r->links;
    lostcnt += printroot(r,0,"?");
  }
}

void Logdir::logprint(t_block root) {
  struct root_n *r = (root_n *)tfind(&root,&t,cmp);
  if (r) {
    long cnt;
    loghdr("MAIN DIRECTORY TREE");
    ++r->links;
    cnt = printroot(r,0,"/");
    printf("\n%8ld blocks in accessible files and directories.\n",cnt);
  }
  struct local {
    static void logchk(const void *p, const VISIT w, const int depth) {
      if (w == preorder || w == endorder) return;
      Logdir::root_n *r = *(Logdir::root_n **)p;
      log->logchk(r);
    }
  };
  // FIXME: not thread safe
  log = this;
  twalk(t,local::logchk);
  log = 0;
  printf("\n%8ld blocks in lost files and directories.\n",lostcnt);
}
