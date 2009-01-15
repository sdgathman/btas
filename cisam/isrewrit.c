/* $Log$
/* Revision 1.3  2001/02/28 23:19:21  stuart
/* remove spurious error map
/* ANSIfy
/*
 * Revision 1.2  1995/04/06  21:03:09  stuart
 * backout partial rewrite on DUPKEY
 *
 * Revision 1.1  1995/04/06  20:17:26  stuart
 * Initial revision
 *
 */
#include <malloc.h>
#include <string.h>
#include <errenv.h>
#include "cisam.h"

static int
cmprec(const struct btflds *f,const char *rec1,const char *rec2,int len) {
  register const struct btfrec *k;
  register int i;
  for (i = 0, k = f->f; i < f->rlen; ++i, ++k) {
    register int rc;
    if (k->pos + k->len > len) {
      if (k->pos >= len) continue;
      rc = len - k->pos;
    }
    else
      rc = k->len;
    rc = memcmp(rec1 + k->pos, rec2 + k->pos, rc);
    if (rc) return (i < f->klen) ? 1 : 2;
  }
  return 0;
}

/* the basic BTAS rewrite operation, when we are finished, all buffers
   will be updated with the new values. */

static int isrew(struct cisam *r,const void *rec,int savekey) {
  struct cisam_key *kp;
  BTCB *b;
  int rlen;
  char *buf;
  kp = &r->key;
  b = kp->btcb;
  if (b->op || r->start != ISCURR) return ENOCURR;
  rlen = isrlen(kp->f);		/* actual record length */
  buf = alloca(rlen);
  b2urec(kp->f->f,buf,rlen,b->lbuf,b->rlen);	/* save master record */
  /* first do the writes, saving the cmpresults and checking for DUPKEY */
  /* FIXME: should write the master record last */
  do {
    kp->cmpresult = cmprec(kp->f,rec,buf,r->rlen);
    if (kp->cmpresult == 1) {
      int rc;
      b = kp->btcb;
      u2brec(kp->f->f,rec,r->rlen,b,kp->klen);	/* load new record */
      rc = btas(b,BTWRITE + DUPKEY);		/* write new record */
      if (rc) {
	struct cisam_key *bk;
	/* got a DUPKEY, backout new records */
	for (bk = &r->key; bk != kp; bk = bk->next) {
	  if (bk->cmpresult == 1) {
	    b = bk->btcb;
	    (void)btas(b,BTDELETE + NOKEY);	  /* delete new record */
	    u2brec(bk->f->f,buf,rlen,b,bk->klen); /* restore old record */
	  }
	}
	return rc;
      }
    }
  } while ((kp = kp->next) != 0);

  /* now delete the old and rewrite the new */

  for (kp = &r->key; kp; kp = kp->next) {
    b = kp->btcb;
    switch (kp->cmpresult) {
    case 1:
      u2brec(kp->f->f,buf,rlen,b,kp->klen);	/* load old record */
      (void)btas(b,BTDELETE + NOKEY);		/* delete old record */
      if (!savekey)
	u2brec(kp->f->f,rec,r->rlen,b,kp->klen);/* restore new record */
      break;
    case 2:
      u2brec(kp->f->f,rec,r->rlen,b,kp->klen);	/* load new record */
      btas(b,BTREPLACE);			/* replace record */
    }
  }
  return 0;
}

/* the Cisam calls */

int isrewcurr(int fd,const void *rec) {
  struct cisam *r;
  r = ischkfd(fd);
  if (r == 0) return iserr(ENOTOPEN);
  return ismaperr(isrew(r,rec,0));
}

/* save key, BTREWRIT KEY=, restore key */

int isrewrite(int fd,const void *rec) {
  register struct cisam *r;
  struct cisam_key *kp;
  char *sav;	/* save key position */
  BTCB *b;
  int rc;
  r = ischkfd(fd);
  if (r == 0) return iserr(ENOTOPEN);
  sav = alloca(r->rlen);
  /* save key position */
  kp = r->curidx;
  if (kp != &r->key)
    b2urec(kp->f->f,sav,r->rlen,kp->btcb->lbuf,kp->klen);
  /* load master record */
  b = r->key.btcb;
  u2brec(r->key.f->f,rec,r->rlen,b,r->key.klen);	/* load master key */
  b->rlen = btrlen(r->key.f);
  /* FIXME: is there a faster way to trap errors? */
  catch(rc)
    rc = btas(b,BTREADEQ + NOKEY);		/* read master record */
    if (rc == 0) {
      /* do the rewrite */
      r->start = ISCURR;
      rc = isrew(r,rec,0);
    }
  envend
  /* restore key position */
  if (kp != &r->key)
    u2brec(kp->f->f,sav,r->rlen,kp->btcb,kp->klen);
  return ismaperr(rc);
}

int isrewrec(int fd,long recno,const void *rec) {
  register struct cisam *r;
  struct cisam_key *k, *kp;
  BTCB *b;
  int rc;
  char *sav;
  r = ischkfd(fd);
  if (r == 0) return iserr(ENOTOPEN);
  sav = alloca(r->rlen);
  k = r->recidx;
  if (k == 0) return iserr(ENOREC);	/* no recnum idx */
  b = k->btcb;
  stlong(recno,b->lbuf);
  b->klen = 4;
  b->rlen = btrlen(k->f);
  if (btas(b,BTREADEQ + NOKEY)) return iserr(ENOREC);
  /* save key position. FIXME: can probably just pass savekey=1 to isrew() */
  kp = r->curidx;
  if (kp != k)
    b2urec(kp->f->f,sav,r->rlen,kp->btcb->lbuf,kp->klen);
  catch (rc)
    /* read master record */
    if (k != &r->key) {
      BTCB *p = r->key.btcb;
      int rlen = btrlen(r->key.f);
      char *buf = alloca(rlen);
      b2urec(k->f->f,buf,rlen,b->lbuf,b->rlen);
      u2brec(r->key.f->f,buf,rlen,p,r->key.klen);
      p->rlen = rlen;
      if (btas(p,BTREADEQ + NOKEY))
	errpost(ENOREC);
    }
    /* do the rewrite */
    r->start = ISCURR;
    rc = isrew(r,rec,0);
  envend
  /* restore key position */
  if (kp != k)
    u2brec(kp->f->f,sav,r->rlen,kp->btcb,kp->klen);
  return ismaperr(rc);
}

/** Rewrite current saving key position.  
 */
int isupdate(int fd,const void *rec) {
  struct cisam *r;
  struct cisam_key *kp;
  BTCB *b;
  char *sav;
  int rc;
  r = ischkfd(fd);
  if (r == 0) return iserr(ENOTOPEN);
  return ismaperr(isrew(r,rec,1));
}
