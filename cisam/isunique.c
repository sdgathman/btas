#include "cisam.h"
#include <btas.h>

int isuniqueid(int fd,long *lp) {
  register struct cisam *r;
  register BTCB *b;
  r = ischkfd(fd);
  if (r == 0) return iserr(ENOTOPEN);
  b = r->idx;
  if (b == 0) return iserr(EBADFILE);

  do {
    b->lbuf[0] = 0; b->klen = 1; b->rlen = 5;
    *lp = btas(b,BTREADL + NOKEY) ? 0L : ldlong(b->lbuf + 1);
    stlong(++*lp,b->lbuf+1);
    b->klen = 5;
  } while (btas(b,BTWRITE + DUPKEY));

  b->klen = 5;
  while (btas(b,BTREADLT + NOKEY) == 0 && btas(b,BTDELETE + NOKEY) == 0);

  return iserr(0);
}

int issetunique(int fd,long id) {
  register struct cisam *r;
  register BTCB *b;
  r = ischkfd(fd);
  if (r == 0) return iserr(ENOTOPEN);
  b = r->idx;
  if (b == 0) return iserr(EBADFILE);
  b->lbuf[0] = 0; b->klen = b->rlen = 5;
  stlong(id,b->lbuf+1);
  (void)btas(b,BTWRITE + DUPKEY);
  return iserr(0);		/* delete duplicates */
}
