/* isindex.c	C-isam index maintenance

	Copyright 1990 Business Management Systems, Inc.
	Author: Stuart D. Gathman
 * $Log$
 * Revision 1.4  1995/04/07  18:46:23  stuart
 * repair DUPKEYs in isfixindex
 *
 * Revision 1.3  1995/04/05  14:32:50  stuart
 * support ISCLUSTER
 *
 * Revision 1.2  1994/02/24  22:13:18  stuart
 * isfixindex
 *
 * Revision 1.1  1994/02/24  20:06:49  stuart
 * Initial revision
 *
 */
#include <malloc.h>
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

static long isfix(struct cisam *r,struct cisam_key *p,const char *name,int e) {
  BTCB *c = btopen(name,BTRDWR + BTEXCL,btrlen(p->f));
  int rlen = btrlen(r->key.f);
  BTCB *b = r->key.btcb;
  char *buf;
  int rc;
  long dups = 0;
  p->btcb = c;
  b->klen = 0;
  b->rlen = rlen;
  rc = btas(b,BTREADGE + NOKEY);
  buf = alloca(rlen);
  while (rc == 0) {
    b2urec(r->key.f->f,buf,rlen,b->lbuf,b->rlen);
    u2brec(p->f->f,buf,rlen,c,p->klen);
    rc = btas(c,BTWRITE + e);
    b->klen = b->rlen;
    if (rc) {
      struct cisam_key *kp = &r->key;
      int i;
      for (i = 0; i < c->rlen; ++i)
	fprintf(stderr,"%02X%c",c->lbuf[i],i + 1 < c->rlen ? ' ' : '\n');
      ++dups;
      for (;;) {
	if (kp != p)
	  (void)btas(b,BTDELETE + NOKEY); /* delete all other key records */
	kp = kp->next;
	if (kp == 0) break;
	b = kp->btcb;
	u2brec(kp->f->f,buf,rlen,b,kp->klen);
      }
      b = r->key.btcb;
    }
    b->rlen = rlen;
    rc = btas(b,BTREADGT + NOKEY);
  }
  return dups;
}

int isaddindex(int fd,const struct keydesc *k) {
  int idx, rc;
  struct cisam *r;
  struct cisam_key *p;
  struct keydesc kn;
  struct btflds *fp;
  BTCB *c, *savdir = btasdir;
  char *name;
  r = ischkfd(fd);
  if (r == 0) return iserr(ENOTOPEN);
  if ((r->key.btcb->flags & BTEXCL) == 0)
    return iserr(ENOTEXCL);
  c = r->idx;
  if (k->k_flags & ISCLUSTER) return iserr(EPRIMKEY);
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
  btasdir = r->dir;
  if (!c) {	/* auto create .idx */
    char ctl_name[MAXKEYNAME + 1];
    strcpy(ctl_name,r->key.name);
    isbuildx(ctl_name,isrlen(r->key.f),&r->key.k,0,0);
    strcat(ctl_name,CTL_EXT);
    c = btopen(ctl_name,BTRDWR,sizeof (struct fisam));
    r->f = ldflds((struct btflds *)0, c->lbuf, c->rlen);
    r->idx = c;
  }
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
  ldchar(c->lbuf,MAXKEYNAME,p->name);
  rc = btcreate(p->name,p->f,0666);		/* create new index file */
  if (rc) errpost(rc);
  name = p->name;	/* tell enverr to delete key file on error */
  p->k = *k;
  if (kn.k_flags & ISDUPS)
    p->klen = btrlen(p->f);
  else
    p->klen = kn.k_len;
  isfix(r,p,name,0);	/* populate the new key file, abort on DUPKEY */
  rc = 0;
  enverr
    /* cleanup after error */
    if (p) {
      r->key.next = p->next;
      (void)btclose(p->btcb);
      free((char *)p->f);
      free((char *)p);
    }
    if (c && c->op == 0)
      (void)btas(c,BTDELETE);	/* delete rec in .idx */
    if (name) btkill(name);
  envend
  btasdir = savdir;
  if (r->f == 0 || c == 0)
    return iserr(ENOTEXCL);
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
      /* record number of dups in isrecnum for caller */
      isrecnum = isfix(r,p,p->name,DUPKEY);
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
