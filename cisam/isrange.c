#include <string.h>
#include <bterr.h>
#include "cisam.h"

int isCheckRange(const struct cisam *r,char *nxt) {
  struct cisam_key *kp;
  struct keydesc *key;
  const char *min, *max;
  int rc = 0;
  BTCB *b;
  kp = r->curidx;
  b = kp->btcb;
  if (r->min == 0) {
    //b2urec(kp->f->f,nxt,r->rlen,b->lbuf,b->rlen);
    return rc;
  }
  key = &kp->k;
  min = r->min;
  max = r->max;
  while (rc == 0) {
    int i, offset, size;
    b2urec(kp->f->f,nxt,r->rlen,b->lbuf,b->rlen);
    for (i = 0;;i++) {
      if (i >= key->k_nparts) {
	return rc;
      }
      offset = key->k_part[i].kp_start;
      size = key->k_part[i].kp_leng;
      if (blkcmpr(max+offset,nxt+offset,size) < 0) {
	int j;
	if (i == 0) return BTEREOF;
	for (j = i; j < key->k_nparts; ++j)
	  memset(nxt+key->k_part[j].kp_start,~0,key->k_part[j].kp_leng);
	u2brec(kp->f->f,nxt,r->rlen,b,kp->klen);
	rc = btas(b,BTREADGT + NOKEY);
	if (rc) return rc;
	b2urec(kp->f->f,nxt,r->rlen,b->lbuf,b->rlen);
	break;
      }
      if (blkcmpr(min+offset,nxt+offset,size) > 0) break;
    }
    while (i < key->k_nparts) {
      offset = key->k_part[i].kp_start;
      size = key->k_part[i++].kp_leng;
      memcpy(nxt+offset,min+offset,size);
    }
    u2brec(kp->f->f,nxt,r->rlen,b,kp->klen);
    rc = btas(b,BTREADGE + NOKEY);
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
