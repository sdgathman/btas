#include <errenv.h>
#include "cisam.h"

int isaddflds(int fd,const struct btfrec *finfo,int fldcnt) {
  Cisam *r = ischkfd(fd);
  BTCB *b;
  int rc = 0;
  int i,n;
  int fcnt;	// current fldcnt
  int pos;
  struct btflds *fp;
  struct btfrec *f;
  if (r == 0) return iserr(ENOTOPEN);
  if ((r->key.btcb->flags & BTEXCL) == 0 || !r->dir)
    return iserr(ENOTEXCL);
  fp = r->key.f;
  if (r->rlen == 0) {
    struct btstat st;
    btfstat(r->key.btcb,&st);
    if (st.rcnt != 0)
      return iserr(ENOREC);
    /* replace entire field table when rlen == 0 && no records */
    fcnt = 0;
  }
  else
    fcnt = fp->rlen;
  n = fcnt + fldcnt + 1;
  /* reallocate master field table */
  fp = realloc(fp,n * sizeof *fp->f + sizeof *fp - sizeof fp->f);
  r->key.f = fp;
  if (!fp) return iserr(ENOMEM);
  /* append new fields (ignore positions and recompute */
  f = fp->f + fcnt;
  if (fcnt == 0)
    f->pos = 0;		/* restart pos when replacing table */
  pos = f->pos;
  for (i = 0; i < fldcnt; ++i) {
    *f = finfo[i];
    f->pos = pos;
    pos += f->len;
    ++f;
  }
  /* FIXME: the offset of any occurrence of BT_RECNO needs to be
     moved to the new isrlen.  This only handles creating a new file.
   */
  if (r->key.k.k_nparts == 0) {
    f->type = BT_RECNO;
    f->len = 4;
    f->pos = pos;
    pos += 4;
    ++f;
  }
  f->type = 0;
  f->len = 0;
  f->pos = pos;
  fcnt = f - fp->f;
  fp->rlen = fcnt;
  if (r->rlen == 0) {
    fp->klen = fcnt;
    r->key.f = isconvkey(fp,&r->key.k,1);
    free(fp);
    fp = r->key.f;
    r->klen = r->key.klen = r->key.k.k_len;
  }
  /* reallocate master BTCB */
  b = (BTCB *)realloc(r->key.btcb, sizeof *r->key.btcb
		      - sizeof r->key.btcb->lbuf + btrlen(fp));
  // FIXME: should close file to prevent using too small btcb!
  if (!b) return iserr(ENOMEM);
  r->key.btcb = b;
  /* rewrite dir record for master file */
  catch (rc) {
    BTCB bt;
    memcpy(&bt,r->dir,sizeof bt - sizeof bt.lbuf);
    if (r->key.name[0] == 0)
      strcpy(r->key.name,"uhoh");
    strcpy(bt.lbuf,r->key.name);
    bt.klen = strlen(bt.lbuf) + 1;
    bt.rlen = stflds(fp,bt.lbuf);
    btas(&bt,BTREPLACE);
  }
  envend
  r->rlen = isrlen(fp);		// reset user record length
  if (rc) return ismaperr(rc);
  return r->rlen;		// return new record length
}

int isreclen(int fd) {
  Cisam *r = ischkfd(fd);
  if (r == 0)
    return iserr(ENOTOPEN);
  return r->rlen;
}
