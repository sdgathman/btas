/*
	erase files comprising an emulated C-isam file
	tack .idx and .n to the end in turn and make BTAS calls
*/

#include <string.h>
#include <errenv.h>
#include <btas.h>
#include <bterr.h>
#include "cisam.h"

static struct elist {
  struct elist *next;
  BTCB *btcb;
  char name[1];
} *list;

/* Retry failed erases.  Return number remaining. */
int iscleanup(const char *fname) {
  int cnt = 0;
  if (list) {
    struct elist **p, *cur;
    BTCB *savdir = btasdir;
    for (p = &list; *p; ) {
      cur = *p;
      if (fname == 0 || strcmp(fname,cur->name) == 0) {
	*p = cur->next;	/* remove from list */
	btasdir = cur->btcb;
	iserase(cur->name);		/* retry failed iserase calls */
	btclose(btasdir);
	if (*p && strcmp((*p)->name,cur->name) == 0) {
	  /* retry didn't work */
	  if (fname == 0) {
	    /* cleanup all - give up completely */
	    free(cur);
	    cur = *p;
	    *p = cur->next;
	    btclose(cur->btcb);
	  }
	  else
	    p = &cur->next;	/* try again later */
	  ++cnt;
	}
	free(cur);
      }
      else {
	p = &cur->next;
	++cnt;
      }
    }
    btasdir = savdir;
  }
  return cnt;
}

static void addlist(BTCB *b,const char *name) {
  struct elist *p;
  p = (struct elist *)malloc(sizeof *p + strlen(name));
  if (p) {
    p->next = list;
    p->btcb = b;
    strcpy(p->name,name);
    list = p;
  }
}

static int endswith(const char *s,const char *e) {
  int lens = strlen(s);
  int lene = strlen(e);
  if (lene > lens) return 0;
  return strcmp(s + lens - lene,e) == 0;
}

int iserase(const char *name) {
  BTCB * volatile b = 0;
  int rc;
  char *fname;
  static char idx[] = CTL_EXT;
  char idxname[128];
  BTCB *savdir = btasdir;
  struct btflds * volatile f = 0;
  struct btstat st;

  if (strlen(name) + sizeof idx > sizeof idxname)
    return iserr(EFNAME);
  strcpy(idxname,name);
  fname = basename(idxname);
  name = dirname(idxname);
  strcat(fname,idx);
  catch(rc) {
    struct fisam frec;
    int dlen;
    dlen = strlen(fname);
    if (dlen < sizeof frec.name)
      dlen = sizeof frec.name;
    btasdir = btopen(name,BTRDWR+4,dlen + 1);	/* open directory */
    b = btopen(fname,BTRDONLY + BTEXCL + NOKEY + LOCK,sizeof frec);
    if (b->flags) {
      f = ldflds(0,b->lbuf,b->rlen);
      if (!f) errpost(ENOMEM);
      b->lbuf[0] = 0;
      b->klen = 1;
      b->rlen = sizeof frec;
      while (btas(b,BTREADGT + NOKEY) == 0) {
	b2urec(f->f,(PTR)&frec,sizeof frec,b->lbuf,b->rlen);
	ldchar(frec.name,sizeof frec.name,btasdir->lbuf);
	btasdir->klen = strlen(btasdir->lbuf) + 1;
	btasdir->rlen = btasdir->klen;
	btas(btasdir,BTDELETE + NOKEY);
	b->klen = b->rlen;
	b->rlen = sizeof frec;
      }
    }
    else if (b->op == BTERKEY)
      *strrchr(fname,'.') = 0;	/* strip .idx */
    else
      errpost(b->op);
    btclose(b);
    b = 0;
    free((PTR)f);
    if (btstat(fname,&st) == 0 && (st.id.mode & BT_DIR))
      rc = BTERDIR;
    else {
      strcpy(btasdir->lbuf,fname);
      btasdir->klen = strlen(btasdir->lbuf) + 1;
      btasdir->rlen = btasdir->klen;
      btas(btasdir,BTDELETE + NOKEY);
      rc = 0;
    }
  }
  enverr
    btclose(b);
    if (f) free((PTR)f);
    if (rc == BTERDEL) { 	/* file open, delete at end of program */
      if (endswith(fname,idx))
	fname[strlen(fname) - sizeof idx + 1] = 0;
      addlist(btasdir,fname);
      btasdir = savdir;
      return iserr(0);
    }
  envend
  btclose(btasdir);
  btasdir = savdir;
  return iserr(rc);
}
