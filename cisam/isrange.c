#include <string.h>
#include <bterr.h>
#include <block.h>
#include "cisam.h"

static int
isvwsel(struct cisam_key *kp,const char *min,const char *max, char *nxt,int rl);
static int
isvwrev(struct cisam_key *kp,const char *min,const char *max, char *nxt,int rl);
static int
isvwcur(struct cisam_key *kp,const char *min,const char *max, char *nxt,int rl);

int isCheckRange(const struct cisam *r,char *nxt,int op) {
  int rc = 0;
  if (r->min == 0)
    return rc;
  switch (op) {
  case BTREADGT: case BTREADGE:
    return isvwsel(r->curidx,r->min,r->max,nxt,r->rlen);
  case BTREADLT: case BTREADLE:
    return isvwrev(r->curidx,r->min,r->max,nxt,r->rlen);
  }
  return isvwcur(r->curidx,r->min,r->max,nxt,r->rlen);
}

static int
isvwcur(struct cisam_key *kp,const char *min,const char *max,
	char *nxt,int rlen) {
  const struct keydesc *key = &kp->k;
  BTCB *b = kp->btcb;
  int i;
  b2urec(kp->f->f,nxt,rlen,b->lbuf,b->rlen);
  for (i = 0; i < key->k_nparts; ++i) {
    int offset = key->k_part[i].kp_start;
    int size = key->k_part[i].kp_leng;
    if (blkcmpr(max+offset,nxt+offset,size) < 0)
      return BTERKEY;
    if (blkcmpr(min+offset,nxt+offset,size) > 0)
      return BTERKEY;
  }
  return 0;
}

static int
isvwsel(struct cisam_key *kp,const char *min,const char *max,
	char *nxt,int rlen) {
  const struct keydesc *key = &kp->k;
  BTCB *b = kp->btcb;
  int rc = 0;
  int krlen = btrlen(kp->f);	/* key record length */
  while (rc == 0) {
    int i, offset, size;
    b2urec(kp->f->f,nxt,rlen,b->lbuf,b->rlen);	/* unpack into nxt buffer */
    for (i = 0;;i++) {		/* check each key part */
      if (i >= key->k_nparts) return rc;
      offset = key->k_part[i].kp_start;
      if (offset >= rlen) continue;
      size = key->k_part[i].kp_leng;
      if (blkcmpr(max+offset,nxt+offset,size) < 0) {	/* > max value */
	int j;
	if (i == 0) return BTEREOF;
	for (j = i; j < key->k_nparts; ++j)
	  memset(nxt+key->k_part[j].kp_start,~0,key->k_part[j].kp_leng);
	u2brec(kp->f->f,nxt,rlen,b,kp->klen);
	b->rlen = krlen;
	rc = btas(b,BTREADGT + NOKEY);
	if (rc) return rc;
	b2urec(kp->f->f,nxt,rlen,b->lbuf,b->rlen);
	break;
      }
      if (blkcmpr(min+offset,nxt+offset,size) > 0)	/* < min value */
	break;
    }
    while (i < key->k_nparts) {
      offset = key->k_part[i].kp_start;
      size = key->k_part[i++].kp_leng;
      memcpy(nxt+offset,min+offset,size);
    }
    u2brec(kp->f->f,nxt,rlen,b,kp->klen);
    b->rlen = krlen;
    rc = btas(b,BTREADGE + NOKEY);
  }
  return rc;
}

static int
isvwrev(struct cisam_key *kp,const char *min,const char *max,
	char *nxt,int rlen) {
  const struct keydesc *key = &kp->k;
  BTCB *b = kp->btcb;
  int rc = 0;
  int krlen = btrlen(kp->f);	/* key record length */
  while (rc == 0) {
    int i, offset, size;
    b2urec(kp->f->f,nxt,rlen,b->lbuf,b->rlen);
    for (i=0;;i++) {
      if (i >= key->k_nparts) return rc;
      offset = key->k_part[i].kp_start;
      if (offset >= rlen) continue;
      size = key->k_part[i].kp_leng;
      if (blkcmpr(nxt+offset,min+offset,size) < 0) {
	int j;
	if (i == 0) return BTEREOF;
	for (j = i; j < key->k_nparts; ++j)
	  memset(nxt+key->k_part[j].kp_start,0,key->k_part[j].kp_leng);
	u2brec(kp->f->f,nxt,rlen,b,kp->klen);
	b->rlen = krlen;
	rc = btas(b,BTREADLT + NOKEY);
	if (rc) return rc;
	b2urec(kp->f->f,nxt,rlen,b->lbuf,b->rlen);
	break;
      }
      if (blkcmpr(nxt+offset,max+offset,size) > 0) break;
    }
    while (i < key->k_nparts) {
      offset = key->k_part[i].kp_start;
      size = key->k_part[i++].kp_leng;
      memcpy(nxt+offset,max+offset,size);
    }
    u2brec(kp->f->f,nxt,rlen,b,kp->klen);
    b->rlen = krlen;
    rc = btas(b,BTREADLE + NOKEY);
  }
  return rc;
}

int isrange(int fd,const PTR min,const PTR max) {
  struct cisam *r = ischkfd(fd);
  if (r == 0)
    return iserr(ENOTOPEN);
  free(r->min);
  r->min = 0;
  r->max = 0;
  if (min || max) {
    int rlen = r->rlen;
    //fprintf(stderr,"rlen = %d\n",rlen);
    r->min = malloc(rlen * 2);
    if (!r->min)
      return iserr(EBADMEM);
    r->max = r->min + rlen;
    if (min)
      memcpy(r->min,min,rlen);
    else
      memset(r->min,0,rlen);
    if (max)
      memcpy(r->max,max,rlen);
    else
      memset(r->max,~0,rlen);
  }
  return iserr(0);
}
