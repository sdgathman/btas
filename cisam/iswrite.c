/*	iswrite.c

	Copyright 1990 Business Management Systems, Inc.
	Author: Stuart D. Gathman
*/

#include <mem.h>
#include "cisam.h"
#include <btas.h>

static int iswr(/**/ struct cisam *, const PTR /**/);

int iswrite(fd,rec)
  int fd;
  const PTR rec;
{
  struct cisam *r;
  BTCB *b,bt;
  int rc;
  r = ischkfd(fd);
  if (r == 0) return iserr(ENOTOPEN);
  b = r->curidx->btcb;
  memcpy(&bt,b,sizeof bt - sizeof bt.lbuf);
  r->curidx->btcb = &bt;
  rc = iswr(r,rec);
  r->curidx->btcb = b;
  return rc;
}

int iswrcurr(fd,rec)
  int fd;
  const PTR rec;
{
  struct cisam *r;
  r = ischkfd(fd);
  if (r == 0) return iserr(ENOTOPEN);
  return iswr(r,rec);
}

static int iswr(r,rec)
  struct cisam *r;
  const PTR rec;
{
  register struct cisam_key *kp;
  register BTCB *b;
  int rc = 0;
  if (kp = r->recidx) {	/* assign new isrecnum */
    b = kp->btcb;
    for (;;) {
      /* this should work first time for the most part */
      isrecnum = ++c_recno(r);
      u2brec(kp->f->f,rec,r->rlen,b,kp->klen);
      if (btas(b,BTWRITE + DUPKEY) == 0) break;	/* got it */
      b->klen = 0;
      btas(b,BTREADLE + NOKEY);	/* read last record */
      c_recno(r) = ldlong(b->lbuf);
    }
  }
  for (kp = &r->key; kp; kp = kp->next) {
    if (kp == r->recidx) continue;
    b = kp->btcb;
    u2brec(kp->f->f,rec,r->rlen,b,kp->klen);
    rc = btas(b,BTWRITE + DUPKEY);
    if (rc) {
      if (r->recidx)		/* undo isrecnum assignment */
	(void)btas(r->recidx->btcb,BTDELETE + NOKEY);
      if (kp != &r->key) {	/* if DUP after master */
	struct cisam_key *up;
	/* undo records already written, key records first */
	for (up = r->key.next; up != kp; up = up->next)
	  (void)btas(up->btcb,BTDELETE + NOKEY);
	(void)btas(r->key.btcb,BTDELETE + NOKEY);
      }
      break;
    }
  }
  return ismaperr(rc);
}
