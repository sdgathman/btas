/*
	btslink.c - symbolically link 2 btas files together
*/

#include <string.h>
#include <errenv.h>
#include <btas.h>
#include <bterr.h>

int btslink(const char *src,const char *dst) {
  BTCB * volatile b = 0;
  int rc;

  catch(rc)
    b = btopendir(dst,BTWRONLY);
    b->klen = strlen(b->lbuf) + 1;
    strncpy(b->lbuf + b->klen,src,MAXREC - b->klen);
    b->rlen = b->klen + strlen(b->lbuf + b->klen);
    b->u.cache.node = 0;
    rc = btas(b,BTLINK + DUPKEY);		/* add link */
  envend
  (void)btclose(b);
  return rc;
}
