#include <stdio.h>
#include <io.h>
extern "C" {
#include <obstack.h>
#include <tsearch.h>
}
#include <malloc.h>
#include <string.h>
#include "fix.h"
/* #define TRACE */

void *cdecl obstack_chunk_alloc(unsigned s) {
  void *p = malloc(s);
  if (!p) {
    write(2,"\r\nOut of memory!\r\n",18);
    abort();
  }
  return p;
}

/* #define obstack_chunk_alloc mymalloc */
#define obstack_chunk_free  free

#define treeflag 0
/* #define TRACE */

struct dir_n {
  struct root_n *file;
  struct dir_n *next;
  short rlen;
  char buf[MAXREC];
};

static TREE t;
static struct obstack h;

static int cmp(const PTR a, const PTR b) {
  if (*(t_block *)a > *(t_block *)b) return 1;
  if (*(t_block *)a < *(t_block *)b) return -1;
  return 0;
}

static struct root_n *addroot(long blk, const struct btstat *st) {
  register struct root_n *r;
  PTR *p;
  if (!t) obstack_init(&h);
  p = tsearch(&blk,&t,cmp);
  if (*p == &blk) {
    *p = r = (root_n *)obstack_alloc(&h,sizeof *r);
    if (!st)
      memset(&r->st,0,sizeof r->st);
    r->dir = 0;
    r->last = 0;
    r->root = blk;
    r->path = 0;
    r->bcnt = 0L;
    r->rcnt = 0L;
    r->links = 0;
  }
  else
    r = (root_n *)*p;
  if (st) {
    r->st = *st;
  }
  return r;
}

void logroot(t_block blk,const struct btstat *st) {
  struct root_n *r;
  r = addroot(blk,st);
#ifdef TRACE
  printf("%ld %05o %4d %4d\n",
	r->root,r->st.id.mode,r->st.id.user,r->st.id.group);
#endif
}

void logrecs(t_block blk,int cnt) {
  struct root_n *r;
  r = addroot(blk,0);
  ++r->bcnt;
  if (cnt > 0)
    r->rcnt += cnt;
}

void logdir(t_block root,const char *buf,int len,t_block blk) {
  struct root_n *r;
  struct dir_n *n, **p;
  r = addroot(root,0);
  n = (dir_n *)obstack_alloc(&h,sizeof *n - sizeof n->buf + len);
  n->rlen = len;
  memcpy(n->buf,buf,len);
  n->file = addroot(blk,0);
  ++n->file->links;
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

static long printroot(struct root_n *, int, const char *);

static char path[MAXKEY + 2];

static long printroot(struct root_n *r,int lev,const char *name) {
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
    cnt += printroot(d->file,lev,d->buf);
  }
  return cnt;
}

static void loghdr(const char *title) {
  printf("%8s %5s%5s%5s%8s%3s %s\n",
    "Root","Mode","Owner","Group","Blocks","Links",title);
}

static long lostcnt = 0L;

static void logchk(PTR p) {
  struct root_n *r = (root_n *)p;
  if (r->path == 0) {
    puts("");
    loghdr("DISCONNECTED TREE");
    ++r->links;
    lostcnt += printroot(r,0,"?");
  }
}

void logprint(t_block root) {
  struct root_n *r = (root_n *)tfind(&root,&t,cmp);
  if (r) {
    long cnt;
    loghdr("MAIN DIRECTORY TREE");
    ++r->links;
    cnt = printroot(r,0,"/");
    printf("\n%8ld blocks in accessible files and directories.\n",cnt);
  }
  tsort(t,logchk);
  printf("\n%8ld blocks in lost files and directories.\n",lostcnt);
}
