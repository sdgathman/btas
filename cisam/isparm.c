#include <isamx.h>
#include <bttype.h>
#include "isreq.h"

int stkeydesc(const struct keydesc *k,char *buf) {
  int i;
  char *p = buf;
  int n = k->k_nparts;
  if (n < 0) n = 0;
  if (n > NPARTS) n = NPARTS;
  stshort(k->k_flags,p),p += 2;
  stshort(n,p), p += 2;
  for (i = 0; i < n; ++i) {
    stshort(k->k_part[i].kp_start,p), p += 2;
    stshort(k->k_part[i].kp_leng,p), p += 2;
    stshort(k->k_part[i].kp_type,p), p += 2;
  }
  return p - buf;
}

void ldkeydesc(struct keydesc *k,const char *p) {
  int i;
  int n;
  k->k_flags = ldshort(p), p += 2;
  n = ldshort(p), p += 2;
  if (n < 0) n = 0;
  if (n > NPARTS) n = NPARTS;
  k->k_nparts = n;
  for (i = 0; i < n; ++i) {
    k->k_part[i].kp_start = ldshort(p), p += 2;
    k->k_part[i].kp_leng = ldshort(p), p += 2;
    k->k_part[i].kp_type = ldshort(p), p += 2;
  }
}

void lddictinfo(struct dictinfo *d,const char *p) {
  d->di_nkeys = ldshort(p), p += 2;
  d->di_recsize = ldshort(p), p += 2;
  d->di_idxsize = ldshort(p), p += 2;
  d->di_nrecords = ldlong(p);
}

int stdictinfo(const struct dictinfo *d,char *buf) {
  char *p = buf;
  stshort(d->di_nkeys,p), p += 2;
  stshort(d->di_recsize,p), p += 2;
  stshort(d->di_idxsize,p), p += 2;
  stlong(d->di_nrecords,p), p += 4;
  return p - buf;
}

int stbtstat(const struct btstat *d,char *buf) {
  char *p = buf;
  stlong(d->bcnt,p), p += 4;
  stlong(d->rcnt,p), p += 4;
  stlong(d->atime,p), p += 4;
  stlong(d->ctime,p), p += 4;
  stlong(d->mtime,p), p += 4;
  stshort(d->links,p), p += 2;
  stshort(d->opens,p), p += 2;
  stshort(d->id.user,p), p += 2;
  stshort(d->id.group,p), p += 2;
  stshort(d->id.mode,p), p += 2;
  return p - buf;
}
