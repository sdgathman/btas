/*
	Delete C-isam records
*/
#include <malloc.h>
#include "cisam.h"

int isdelcurr(fd)
  int fd;
{
  register struct cisam *r;
  register struct cisam_key *kp;
  BTCB *b;
  char *buf;
  int rlen;
  r = ischkfd(fd);
  if (r == 0) return iserr(ENOTOPEN);
  kp = &r->key;
  b = kp->btcb;
  if (b->op || r->start != ISCURR) return iserr(ENOCURR);
  rlen = isrlen(kp->f);			/* isam record length */
  buf = alloca(rlen);
  b2urec(kp->f->f,buf,rlen,b->lbuf,b->rlen);	/* load master data */
  b->klen = b->rlen;
  for (;;) {
    (void)btas(b,BTDELETE + NOKEY);		/* delete all records */
    kp = kp->next;
    if (kp == 0) break;
    b = kp->btcb;
    u2brec(kp->f->f,buf,rlen,b,kp->klen);
  }
  return iserr(0);
}

int isdelete(fd,rec)
  int fd;
  const PTR rec;
{
  register struct cisam *r;
  BTCB *b;
  int rc;
  r = ischkfd(fd);
  if (r == 0) return iserr(ENOTOPEN);
  b = r->key.btcb;
  u2brec(r->key.f->f,rec,r->rlen,b,r->key.klen);
  b->rlen = btrlen(r->key.f);
  rc = btas(b,BTREADEQ + NOKEY);
  if (rc) return ismaperr(rc);
  r->start = ISCURR;
  return isdelcurr(fd);
}

int isdelrec(fd,recno)
  int fd;
  long recno;
{
  register struct cisam *r;
  struct cisam_key *k;
  BTCB *b;
  r = ischkfd(fd);
  if (r == 0) return iserr(ENOTOPEN);
  k = r->recidx;
  if (k == 0) return iserr(ENOREC);	/* no recnum idx */
  b = k->btcb;
  stlong(recno,b->lbuf);
  b->klen = 4;
  b->rlen = btrlen(k->f);
  if (btas(b,BTREADEQ + NOKEY)) return iserr(ENOREC);
  r->start = ISCURR;
  return isdelcurr(fd);
}
