#include "cisam.h"
/* #define DEB */

#ifdef DEB
#include <stdio.h>

static int dump_keydesc(const struct keydesc *k) {
  int i,len,x,max;
  fprintf(stderr,"keydesc: parts=%d",k->k_nparts);
  for (i=0,len=0,max=0; i < k->k_nparts ; i++) {
    fprintf(stderr," %3d-%-2d-%d",
      k->k_part[i].kp_start,k->k_part[i].kp_leng,k->k_part[i].kp_type);
    len += k->k_part[i].kp_leng;
    x = k->k_part[i].kp_start + k->k_part[i].kp_leng;
    if (x > max) max = x;
  }
  fprintf(stderr," len=%d,max=%d\n",len,max);
  return len > max ? len : max;
}
#endif

int iskeycomp(const struct keydesc *p,const struct keydesc *q) {
  int i;
#ifdef DEB
  fprintf(stderr,"iskeycomp:\n");
  dump_keydesc(p);
  dump_keydesc(q);
#endif
/*  if (p->k_flags != q->k_flags) return 1; */
  if (p->k_nparts != q->k_nparts) return 1;
  for (i = 0; i < q->k_nparts; ++i)
    if ( p->k_part[i].kp_start != q->k_part[i].kp_start
      || p->k_part[i].kp_leng  != q->k_part[i].kp_leng
      || p->k_part[i].kp_type  != q->k_part[i].kp_type) return 1;
  return 0;	/* everything matches */
}

/* isstart by key name */
int isstartn(int fd,const char *k,int len,const PTR buf,int mode) {
  struct cisam_key *p;
  struct cisam *r = ischkfd(fd);
  if (r == 0) return iserr(ENOTOPEN);
  /* search for a matching keydesc */

  for (p = &r->key; p; p = p->next)
    if (strcmp(p->name,k) == 0) break;
  if (p == 0) return iserr(EBADKEY);	/* keydesc not found */
  return isstart(fd,&p->k,len,buf,mode);
}

int isstart(int fd,const struct keydesc *k,int len,const PTR buf,int mode) {
  register int i;
  enum btop op;
  struct cisam *r;
  struct keydesc kn;	/* normalized keydesc */
  struct cisam_key *p;
  BTCB *b;
  r = ischkfd(fd);
  if (r == 0) return iserr(ENOTOPEN);

  /* make kn a normalized copy of *k */

  kn = *k; iskeynorm(&kn);

  /* search for a matching keydesc */

  for (p = &r->key; p; p = p->next) {
    struct keydesc km;
    km = p->k; iskeynorm(&km);
    if (iskeycomp(&kn,&km) == 0) break;
  }
  if (p == 0) return iserr(EBADKEY);	/* keydesc not found */

  if (len < 0  || len > kn.k_len) return iserr(EBADARG);
  if (len == 0) len = kn.k_len;

  b = p->btcb;
  switch (mode) {
  case ISGTEQ:
    op = BTREADGE;
    break;
  case ISGREAT:
    op = BTREADGT;
    break;
  case ISEQUAL:
    op = BTREADF;
    break;
  default:
    return iserr(EBADARG);
  case ISFIRST: case ISLAST:
    b->rlen = 0;
  }
  r->klen = len;
  r->start = mode;
  r->curidx = p;

  if (mode != ISFIRST && mode != ISLAST) {
    u2brec(p->f->f,buf,r->rlen,b,len);
    b->rlen = btrlen(p->f);
    i = btas(b,(int)op + NOKEY);	/* read key record */
    if (i != 0)
      return ismaperr(i);
    /* FIXME: set isrecnum here if recno master */
  }
  return iserr(0);
}
