/* isindex.c	C-isam index maintenance

	Copyright 1990 Business Management Systems, Inc.
	Author: Stuart D. Gathman
 * $Log$
 * Revision 1.1  1994/02/24  20:06:49  stuart
 * Initial revision
 *
 */

#include "cisam.h"
#include <stdio.h>
#include <errenv.h>
#include <string.h>

int isindexinfo(int fd,struct keydesc *k,int idx) {
  struct cisam *r;
  struct cisam_key *kp;
  r = ischkfd(fd);
  if (r == 0) return iserr(ENOTOPEN);
  kp = &r->key;
  if (idx == 0) {		/* dictinfo */
    struct dictinfo *d = (struct dictinfo *)k;
    struct btstat st;
    btfstat(kp->btcb,&st);
    d->di_recsize = isrlen(kp->f);
    d->di_idxsize = BLKSIZE;
    for (d->di_nkeys = 1; kp = kp->next; ++d->di_nkeys);
    d->di_nrecords = st.rcnt;
  }
  else {
    while (--idx > 0) {
      kp = kp->next;
      if (kp == 0) return iserr(EBADARG);
    }
    *k = kp->k;
  }
  return iserr(0);
}

static int isfix(struct cisam *r,struct cisam_key *p,const char *name) {
  BTCB *c = btopen(name,BTRDWR + BTEXCL,btrlen(p->f));
  int rlen = btrlen(r->key.f);
  BTCB *b = r->key.btcb;
  char *buf;
  int rc;
  p->btcb = c;
  b->klen = 0;
  b->rlen = rlen;
  rc = btas(b,BTREADGE + NOKEY);
  buf = alloca(rlen);
  while (rc == 0) {
    b2urec(r->key.f->f,buf,rlen,b->lbuf,b->rlen);
    u2brec(p->f->f,buf,rlen,c,p->klen);
    btas(c,BTWRITE);		/* should be no dupkeys */
    b->klen = b->rlen;
    b->rlen = rlen;
    rc = btas(b,BTREADGT + NOKEY);
  }
  return 0;
}

int isaddindex(int fd,const struct keydesc *k) {
  int idx, rc, rlen;
  struct cisam *r;
  struct cisam_key *p;
  struct keydesc kn;
  struct btflds *fp;
  BTCB *c, *savdir = btasdir;
  char *name;
  r = ischkfd(fd);
  if (r == 0) return iserr(ENOTOPEN);
  c = r->idx;		/* we don't auto-create .idx yet */
  if ( r->f == 0 || c == 0) return iserr(ENOTEXCL);
  kn = *k;
  fp = isconvkey(r->key.f,&kn,1);	/* compute field table */
  if (fp == 0) return iserr(EBADMEM);
  iskeynorm(&kn);
  for (p = &r->key; p; p = p->next) {
    struct keydesc km;
    km = p->k; iskeynorm(&km);
    if (iskeycomp(&kn,&km) == 0) break;
  }
  if (p) {
    free((char *)fp);
    return iserr(EKEXISTS);
  }

  catch(rc)
  name = 0;
  c->klen = 0; c->rlen = sizeof (struct fisam);
  (void)btas(c,BTREADLE);		/* get last name */
  idx = 0;
  if (name = strrchr(c->lbuf,'.')) {
    *name++ = 0;
    idx = atoi(name);
  }
  name = 0;
  do {
    struct fisam f;
    (void)sprintf(f.name,"%s.%02d",c->lbuf,++idx);	/* new index name */
    isstkey(f.name,k,&f);
    u2brec(r->f->f,(char *)&f,sizeof f,c,sizeof f.name);
  } while (btas(c,BTWRITE + DUPKEY));	/* create idx record */

  /* add cisam_key to list */
  p = (struct cisam_key *)malloc(sizeof *p);
  if (p == 0) errpost(EBADMEM);
  p->btcb = 0;
  p->next = r->key.next;
  r->key.next = p;
  p->f = fp;
  ldchar(c->lbuf,MAXKEYNAME,name = p->name);
  btasdir = r->dir;
  rc = btcreate(name,p->f,0666);		/* create new index file */
  if (rc) {
    (void)btas(c,BTDELETE);			/* couldn't create */
    errpost(rc);
  }
  p->k = *k;
  if (kn.k_flags & ISDUPS)
    p->klen = btrlen(p->f);
  else
    p->klen = kn.k_len;
  rc = isfix(r,p,name);
  enverr
    /* cleanup after error */
    if (p) {
      (void)btclose(p->btcb);
      free((char *)p->f);
      free((char *)p);
    }
    if (name) btkill(name);
  envend
  btasdir = savdir;
  return ismaperr(rc);
}

int isfixindex(int fd,const struct keydesc *k) {
  struct cisam *r;
  struct cisam_key *p;
  struct keydesc kn;
  BTCB *savdir;
  int rc;
  r = ischkfd(fd);
  if (r == 0) return iserr(ENOTOPEN);
  if (r->f == 0 || r->idx == 0) return iserr(ENOTEXCL);
  kn = *k; iskeynorm(&kn);
  for (p = &r->key; p; p = p->next) {
    struct keydesc km;
    km = p->k; iskeynorm(&km);
    if (iskeycomp(&kn,&km) == 0) break;
  }
  if (!p) return iserr(EBADKEY);
  if (p == &r->key) return iserr(EPRIMKEY);
  (void)btclose(p->btcb);
  savdir = btasdir;
  btasdir = r->dir;
  rc = btclear(p->name);
  if (rc == 0) {
    catch(rc)
      rc = isfix(r,p,p->name);
    envend
  }
  btasdir = savdir;
  return ismaperr(rc);
}

int isdelindex(int fd,const struct keydesc *k) {
  struct cisam *r;
  struct cisam_key *p, *q;
  struct keydesc kn;
  BTCB *c;
  r = ischkfd(fd);
  if (r == 0) return iserr(ENOTOPEN);
  c = r->idx;
  if (r->f == 0 || c == 0) return iserr(ENOTEXCL);
  kn = *k; iskeynorm(&kn);
  for (q = 0, p = &r->key; p; q = p, p = p->next) {
    struct keydesc km;
    km = p->k; iskeynorm(&km);
    if (iskeycomp(&kn,&km) == 0) break;
  }
  if (!p) return iserr(EBADKEY);
  if (!q) return iserr(EPRIMKEY);
  q->next = p->next;		/* delete from list */
  (void)btclose(p->btcb);
  free((char *)p->f);
  c->klen = 0; c->rlen = sizeof (struct fisam);
  if (btas(c,BTREADLE + NOKEY) == 0)
    while (c->lbuf[0]) {
      struct fisam f;
      struct keydesc km;
      b2urec(r->f->f,(char *)&f,sizeof f,c->lbuf,c->rlen);
      isldkey(c->lbuf,&km,&f);
      iskeynorm(&km);
      if (iskeycomp(&kn,&km) == 0) {
	BTCB *savdir;
	c->klen = c->rlen;
	btas(c,BTDELETE + NOKEY);	/* delete .idx record */
	savdir = btasdir;
	btasdir = r->dir;
	(void)btkill(c->lbuf);		/* delete file */
	btasdir = savdir;
	break;
      }
      c->klen = c->rlen;
      c->rlen = sizeof f;
      if (btas(c,BTREADLT + NOKEY)) break;
    }
  free((char *)p);
  return iserr(0);
}
