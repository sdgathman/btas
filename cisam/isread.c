/* $Log$
 * Revision 1.3  1998/05/26  18:20:09  stuart
 * record locking implemented
 *
 * Revision 1.2  1998/04/23  03:13:08  stuart
 * support key range checking
 *
 */
#include <malloc.h>
#include <errenv.h>
#include "cisam.h"
#include <bterr.h>

int isread(int fd,void *rec,int mode) {
  register struct cisam_key *kp;
  BTCB *b;
  enum btop op;
  int rc;
  int btmode;
  char *buf;
  int flags = mode;
  struct cisam *r = ischkfd(fd);
  if (r == 0) return iserr(ENOTOPEN);
  buf = alloca(r->rlen);
  kp = r->curidx;
  b = kp->btcb;
  mode &= 0xFF;

  /* interpret special isstart() braindamage */

  switch (r->start) {
  case ISFIRST:
    switch (mode) {
    case ISPREV: return iserr(EENDFILE);
    case ISCURR:
      b->op = 0;
      break;
    case ISNEXT: mode = ISFIRST;
    }
    break;
  case ISLAST:
    switch (mode) {
    case ISNEXT: return iserr(EENDFILE);
    case ISCURR:
      b->op = 0;
      break;
    case ISPREV: mode = ISLAST;
    }
  case ISCURR:
    break;
  default:
    if (mode == ISNEXT) mode = ISCURR;
  }
  r->start = ISCURR;

  switch (mode) {
  case ISNEXT:
    if (b->op) return iserr(ENOCURR);	/* previous read failed */
    /* apparently, setting logical key length on ISSTART affects ISNEXT
       on the real C-isam.  This is stupid, it should only affect ISGREAT,
       etc., but some applications may depend on it. */
#if 1	/* the way I think it ought to work */
    b->klen = b->rlen;	/* just get the next record */
#else	/* the way C-isam works - this breaks if any records are garbaged */
    b2urec(kp->f->f,buf,r->rlen,b->lbuf,kp->klen);
    u2brec(kp->f->f,buf,r->rlen,b,kp->klen);
#endif
    op = BTREADGT;
    break;
  case ISFIRST:
    b->klen = 0;
    op = BTREADGE;
    break;
  case ISLAST:
    b->klen = 0;
    op = BTREADLE;
    break;
  case ISPREV:
    if (b->op) return iserr(ENOCURR);
    b->klen = b->rlen;
    op = BTREADLT;
    break;
  case ISCURR:
    /* what is the "current record"?  Our present interpretation is to
       reread the record with the last primary key used.  (Rereading
       the last record read seems silly to me.) */
    if (kp != r->recidx) {
      if (b->op) return iserr(ENOCURR);
      if (kp != &r->key || (flags & ISLOCK)) {
	b2urec(kp->f->f,buf,r->rlen,b->lbuf,b->rlen);
	kp = &r->key;
	b = kp->btcb;
	u2brec(kp->f->f,buf,r->rlen,b,kp->klen);
      }
      op = BTREADEQ;	/* reread record */
      break;
    }
    /* fall through ISCURR == ISEQUAL for recidx */
  case ISEQUAL:
    /* FIXME: does ISEQUAL use partial key? */
    u2brec(kp->f->f,rec,r->rlen,b,kp->klen);
    op = BTREADEQ;
    break;
  case ISGREAT:
    u2brec(kp->f->f,rec,r->rlen,b,r->klen);
    op = BTREADGT;
    break;
  case ISGTEQ:
    u2brec(kp->f->f,rec,r->rlen,b,r->klen);
    op = BTREADGE;
    break;
  case ISLESS:
    u2brec(kp->f->f,rec,r->rlen,b,r->klen);
    op = BTREADLT;
    break;
  case ISLTEQ:
    u2brec(kp->f->f,rec,r->rlen,b,r->klen);
    op = BTREADLE;
    break;
  }
  b->rlen = btrlen(kp->f);	/* max record length */
  if (b->klen > b->rlen)
    b->klen = b->rlen;
  btmode = NOKEY;
  if ((flags & ISLOCK) && kp == &r->key)
    btmode |= LOCK;
  catch(rc)
  rc = btas(b,(int)op + btmode);	/* read btas (possibly key) record */
  if (rc == 0)
    rc = isCheckRange(r,buf,op);
  if (rc == 0) {
    if (kp != &r->key) {
      b2urec(kp->f->f,buf,r->rlen,b->lbuf,b->rlen);
      kp = &r->key;
      b = kp->btcb;
      u2brec(kp->f->f,buf,r->rlen,b,kp->klen);
      b->rlen = btrlen(kp->f);
      if (flags & ISLOCK)
	btmode |= LOCK;
      rc = btas(b,BTREADEQ + btmode);
    }
    if (rc == 0)
      b2urec(kp->f->f,rec,r->rlen,b->lbuf,b->rlen);
  }
  else if (rc == BTEREOF) {
    switch (mode) {
    case ISLESS: case ISLTEQ:
      rc = BTERKEY;	/* isread returns ENOREC for EOF ?! */
    case ISPREV:
      r->start = ISFIRST;
      break;
    case ISGREAT: case ISGTEQ:
      rc = BTERKEY;	/* isread returns ENOREC for EOF ?! */
    case ISNEXT:
      r->start = ISLAST;
      break;
    }
  }
  envend
  return ismaperr(rc);
}

int isrelease(int fd) {
  BTCB bt;
  struct cisam *r = ischkfd(fd);
  if (r == 0) return iserr(ENOTOPEN);
  memcpy(&bt,r->key.btcb,sizeof bt - sizeof bt.lbuf);
  bt.klen = 0;
  bt.rlen = 0;
  return ismaperr(btas(&bt,BTRELEASE));
}
