#include <string.h>
#include <btas.h>
#include <errenv.h>

#define btread(b,cmd,len)	\
	((b)->klen = (len),(b)->rlen = MAXREC,btas((b),cmd))

struct nest {
  btwalk_fn userf;
  BTCB bt;
  char rpath[MAXKEY + 1];
};

struct stk {
  BTCB *b;
  long root;
  short mid;
  short flags;
  struct btlevel cache;
  struct stk *prev;
};

static int dodir(n,len,prev)
  struct nest *n;
  int len;
  struct stk *prev;
{
  /* FIXME: hate to waste MAXREC bytes/level on 16-bit machines */
  struct btstat st;
  int rc;
  struct stk stk;
  if (rc = btopenf(&n->bt,n->rpath,BTRDONLY+BTDIROK+NOKEY,MAXREC))
    return rc;
  stk.root = n->bt.root;
  stk.mid  = n->bt.mid;
  stk.flags= n->bt.flags;
  stk.cache= n->bt.u.cache;
  stk.prev = prev;

  /* check if this is a '.' or '..' directory and skip */
  while (prev) {
    if (n->bt.root == prev->root && n->bt.mid == prev->mid) {
      btas(&n->bt,BTCLOSE);
      return 0;
    }
    prev = prev->prev;
  }

  catch(rc)
    n->bt.klen = 0;
    n->bt.rlen = sizeof st;
    btas(&n->bt,BTSTAT);
    memcpy(&st,n->bt.lbuf,sizeof st);
    rc = (*n->userf)(n->rpath,&st);
    if (rc == 0 && (n->bt.flags & BT_DIR)) {
      n->bt.klen = 0;
      n->bt.rlen = MAXREC;
      n->rpath[len++] = '/';
      for (
	rc = btread(&n->bt,BTREADGE + NOKEY,0);
	rc == 0;
	rc = btread(&n->bt,BTREADGT + NOKEY,strlen(n->bt.lbuf) + 1)
      ) {
	int nlen = strlen(n->bt.lbuf) + len;
	if (nlen >= sizeof n->rpath) return -1;
	strcpy(n->rpath+len,n->bt.lbuf);
	if (rc = dodir(n,nlen,&stk))
	  errpost(rc);
	n->bt.root = stk.root;
	n->bt.mid  = stk.mid;
	n->bt.flags= stk.flags;
	n->bt.u.cache = stk.cache;
	n->rpath[nlen] = 0;
	strcpy(n->bt.lbuf,n->rpath + len);
      }
      rc = 0;
    }
  enverr
    n->bt.root = stk.root;
    n->bt.mid  = stk.mid;
    n->bt.flags= stk.flags;
  envend
  btas(&n->bt,BTCLOSE);
  return rc;
}

int btwalk(path,fn)
  const char *path;
  btwalk_fn fn;
{
  struct nest n;
  struct stk stk, *stkp = 0;
  BTCB * volatile savdir;
  char * volatile p;
  int rc;
  int len = strlen(path);
  if (len >= sizeof n.rpath)
    return -1;
  strcpy(n.rpath,path);
  n.userf = fn;
  catch(rc)
  if (p = strrchr(n.rpath,'/')) {
    *p++ = 0;
    savdir = btgetdir();
    btchdir(n.rpath);
    strcpy(n.rpath,p);
    len = strlen(p);
  }
  btopenf(&n.bt,"",BTRDONLY+BTDIROK,MAXREC);
  stk.root = n.bt.root;
  stk.mid  = n.bt.mid;
  stk.flags= n.bt.flags;
  stk.cache= n.bt.u.cache;
  stk.prev = 0;
  stkp = &stk;
  btas(&n.bt,BTCLOSE);
  rc = dodir(&n,len,stkp);
  envend
  if (p)
    btputdir(savdir);
  return rc;
}
