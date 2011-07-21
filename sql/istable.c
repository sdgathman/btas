/*
 * $Log$
 * Revision 1.5  2009/01/15 19:02:07  stuart
 * Add isupdate call to update key fields and save key position.
 *
 * Revision 1.4  2005/02/08 15:17:42  stuart
 * Release 2.10.8
 *
 */
#include <stdio.h>
#include <string.h>
#include <btflds.h>
#include <isamx.h>
#include "cursor.h"
#include "object.h"
#include "coldate.h"

typedef struct Isam/*:public Cursor*/ {
  struct Cursor_class *_class;
/* from Cursor */
  short ncol;	/* number of columns */
  short klen;	/* number of key columns */
  Column **col;
  long rows;
  long cnt;
/* common to Tables */
  char *ubuf;
/* private: */
  int fd;	/* C-isam file descriptor */
  int rlen;	/* size of ubuf (for debugging) */
  struct keydesc kd;
  char rdonly;
} Isam;

static void Isam_free(Cursor *);
static int Isam_first(Cursor *);
static int Isam_next(Cursor *);
static int Isam_find(Cursor *);
static int Isam_insert(Cursor *);
static int Isam_update(Cursor *);
static int Isam_delete(Cursor *);
static void Isam_optim(Cursor *, Column **, int , int);
static struct Cursor_class Isam_class = {
  sizeof (Isam), Isam_free, Isam_first, Isam_next,
  Isam_find, Cursor_print, Isam_insert, Isam_update,
  Isam_delete, Isam_optim
};

Cursor *Table_init(const char *name,int flag) {
  Isam *t;
  struct dictinfo d;
  struct btflds *f;
  int fd;
  int mode;
  if (*name == 0)
    name = btgetcwd();
  if (flag == 1)
    mode = ISINPUT;
  else if (flag == 2)
    mode = ISINOUT + ISEXCLLOCK;
  else
    mode = ISINOUT + ISMANULOCK;
  fd = isopen(name,mode);
  if (fd == -1) {
    struct Cursor *dir = Directory_init(name,flag == 1);
    if (dir) return dir;
    fprintf(stderr,"istable: Cannot open \"%s\".\n",name);
    return 0;
  }
  if (debug)
    printf("Open table \"%s\"\n",name);
  t = (Isam *)xmalloc(sizeof *t);
  t->_class = &Isam_class;
  t->col = 0;
  t->ncol = 0;
  t->klen = 0;
  t->cnt = 0;
  t->fd = fd;
  t->ubuf = 0;
  t->rdonly = flag == 1;
  f = isflds(fd);
  if (isindexinfo(t->fd,&t->kd,1))	/* save primary keydesc */
    t->kd.k_nparts = 0;
  isindexinfo(fd,(struct keydesc *)&d,0);
  t->rows = d.di_nrecords;
  /* t->ubuf = xmalloc(d.di_recsize); */
  t->rlen = btrlen(f);
  t->ubuf = (char *)xmalloc(t->rlen);
  getdict((Cursor *)t,name,t->ubuf,f);
  return (Cursor *)t;
}

int Isam_getfd(Cursor *t) {
  if (t->_class == &Isam_class)
    return ((Isam *)t)->fd;
  return -1;
}

static void Isam_optim(Cursor *c,Column **col,int ncol,int klen) {
  Isam *thistab = (Isam *)c;
  int i;
  int bestscore;
  int bestused;		/* fields used in optimal keydesc */
  struct keydesc bestkey;	/* optimal keydesc */

  /* Select optimal keydesc based on:
     1) all fields used or master			+20 if true
     2) most key fields used				+10 each field
     3) most initial key fields in common (same order)	+1 each field
  */
  struct keydesc kd;
  if (isindexinfo(thistab->fd,&kd,i = 1))
    return;	/* give up if we can't get primary keydesc */

  bestkey = kd;
  bestscore = bestused = 0;

  do {
    int kp;	/* current keypart */
    int cp;	/* current user column idx */
    int common, used;
    int score;
    struct keydesc k;	/* scratch copy of keydesc which we monkey with */
    k = kd;

    /* count initial fields in common */
    for (cp = 0,kp = 0; cp < ncol && kp < k.k_nparts; ++cp) {
      struct keypart *p = k.k_part + kp;
      Column *q = col[cp];
      if (thistab->ubuf + p->kp_start != q->buf) break;
      if (p->kp_leng > q->len) {
	p->kp_leng -= q->len;
	p->kp_start += q->len;
      }
      else
	++kp;
    }
    common = cp;

    /* count additional fields used */ 
    for (used = common; cp < ncol; ++cp) {
      int pos = col[cp]->buf - thistab->ubuf;
      int j;
      for (j = kp; j < k.k_nparts; ++j) {
	struct keypart *p = k.k_part + j;
	if (pos >= p->kp_start && pos < p->kp_start + p->kp_leng)
	  ++used;
      }
    }

    /* compute score */
    score = used * 10 + common;
    if (used == ncol)
      score += 20;

    if (score > bestscore) {
      bestscore = score;
      bestkey = kd;
      bestused = used;
    }

  } while (!isindexinfo(thistab->fd,&kd,++i));

  /* move new key columns to the front and adjust klen */
  {
    struct keydesc k;
    int kp = 0;
    k = bestkey;
    thistab->klen = 0;
    while (kp < k.k_nparts) {
      struct keypart *p = k.k_part + kp;
      Column *q;
      int cp;
      for (cp = 0; cp < thistab->ncol; ++cp) {
	q = thistab->col[cp];
	if (thistab->ubuf + p->kp_start == q->buf) break;
      }
      if (cp >= thistab->ncol) break;
      if (cp != thistab->klen) {	/* swap columns */
	thistab->col[cp] = thistab->col[thistab->klen];
	thistab->col[cp]->colidx = cp;
	thistab->col[thistab->klen] = q;
	q->colidx = thistab->klen;
      }
      ++thistab->klen;
      if (p->kp_leng > q->len) {
	p->kp_leng -= q->len;
	p->kp_start += q->len;
      }
      else
	++kp;
    }
  }
  if (debug)
    printf("optim: klen = %d, ncol = %d\n",thistab->klen,thistab->ncol);

  thistab->kd = bestkey;
  isstart(thistab->fd,&bestkey,0,(PTR)0,ISFIRST);
  /* FIXME: if used == ncol, split into read & update fd's
	read fd has key record only - extract name of
	key file, do another isopen(), and remap the columns. */
}

static void Isam_free(Cursor *c) {
  int i;
  Isam *thistab = (Isam *)c;
  isclose(thistab->fd);
  if (thistab->col) {
    for (i = 0; i < thistab->ncol; ++i)
      do0(thistab->col[i],free);
    free((PTR)thistab->col);
  }
  if (thistab->ubuf)
    free(thistab->ubuf);
  free((PTR)thistab);
}

static int Isam_first(Cursor *c) {
  Isam *thistab = (Isam *)c;
  if (isread(thistab->fd,thistab->ubuf,ISFIRST)) {
    thistab->rows = thistab->cnt = 0;
    return -1;
  }
  thistab->cnt = 1;
  return 0;
}

static int Isam_next(Cursor *c) {
  Isam *thistab = (Isam *)c;
  if (cancel) return -1;
  if (isread(thistab->fd,thistab->ubuf,ISNEXT)) {
    thistab->rows = thistab->cnt;
    return -1;
  }
  ++thistab->cnt;
  return 0;
}

static int Isam_find(Cursor *c) {
  Isam *thistab = (Isam *)c;
  return isread(thistab->fd,thistab->ubuf,thistab->klen ? ISGTEQ : ISFIRST);
}

static int Isam_insert(Cursor *c) {
  Isam *thistab = (Isam *)c;
  return iswrcurr(thistab->fd,thistab->ubuf);
}

#if 0
static int iskeylen(const struct keydesc *kp) {
  int i,len = 0;
  for (i = 0; i < kp->k_nparts; ++i)
    len += kp->k_part[i].kp_leng;
  return len;
}

static void issavkey(const struct keydesc *kp,const char *src,char *dst) {
  int i, pos = 0;
  for (i = 0; i < kp->k_nparts; ++i) {
    const struct keypart *p = kp->k_part + i;
    memcpy(dst+pos,src+p->kp_start,p->kp_leng);
    pos += p->kp_leng;
  }
}

static void isrstkey(const struct keydesc *kp,char *dst,const char *src) {
  int i, pos = 0;
  for (i = 0; i < kp->k_nparts; ++i) {
    const struct keypart *p = kp->k_part + i;
    memcpy(dst+p->kp_start,src+pos,p->kp_leng);
    pos += p->kp_leng;
  }
}
#endif

static int Isam_update(Cursor *c) {
  Isam *thistab = (Isam *)c;
  if (!thistab->rdonly)
    //return isrewcurr(thistab->fd,thistab->ubuf);
    return isupdate(thistab->fd,thistab->ubuf);
  return -1;
}

static int Isam_delete(Cursor *c) {
  Isam *thistab = (Isam *)c;
  if (!thistab->rdonly)
    return isdelcurr(thistab->fd);
  return -1;
}
