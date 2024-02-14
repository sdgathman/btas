/*
    This file is part of the BTAS client library.

    The BTAS client library is free software: you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public License as
    published by the Free Software Foundation, either version 3 of the License,
    or (at your option) any later version.

    BTAS is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with BTAS.  If not, see <http://www.gnu.org/licenses/>.
*/
#include <string.h>
#include <btas.h>
#include <errenv.h>

#define btread(b,cmd,len)	\
	((b)->klen = (len),(b)->rlen = MAXREC,btas((b),cmd))

struct nest {
  btwalk_fn userf;
  btwalk_err errf;
  BTCB bt;
  char rpath[MAXKEY + 1];
};

struct stk {
  BTCB *b;
  struct bttag tag;
  struct btlevel cache;
  struct stk *prev;
};

static int dodir(struct nest *n,int len,struct stk *prev) {
  /* FIXME: hate to waste MAXREC bytes/level on 16-bit machines */
  struct btstat st;
  int rc;
  struct stk stk;
  if ((rc = btopenf(&n->bt,n->rpath,BTRDONLY+BTDIROK+NOKEY,MAXREC))) {
    return rc;
  }
  stk.prev = prev;
  btcb2tag(&n->bt,&stk.tag);

  /* check if this is a '.' or '..' directory and skip */
  while (prev) {
    if (n->bt.root == prev->tag.root && n->bt.mid == prev->tag.mid) {
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
	stk.cache= n->bt.u.cache;
	rc = dodir(n,nlen,&stk);
	if (rc != 0) {
	  if (n->errf != 0)
	    rc = (*n->errf)(n->rpath,rc);
	  if (rc != 0)
	    errpost(rc);
	}
	n->bt.u.cache = stk.cache;
	tag2btcb(&n->bt,&stk.tag);
	n->rpath[nlen] = 0;
	strcpy(n->bt.lbuf,n->rpath + len);
      }
      n->rpath[--len] = 0;
      rc = (*n->userf)(n->rpath,0);
    }
  enverr
    tag2btcb(&n->bt,&stk.tag);
  envend
  btas(&n->bt,BTCLOSE);
  return rc;
}

int btwalk(const char *path,btwalk_fn fn) {
  return btwalkx(path,fn,0);
}

int btwalkx(const char *path,btwalk_fn fn,btwalk_err errf) {
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
  n.errf = errf;
  catch(rc)
  if ((p = strrchr(n.rpath,'/'))) {
    *p++ = 0;
    savdir = btgetdir();
    btchdir(n.rpath);
    strcpy(n.rpath,p);
    len = strlen(p);
  }
  btopenf(&n.bt,"",BTRDONLY+BTDIROK,MAXREC);
  btcb2tag(&n.bt,&stk.tag);
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
