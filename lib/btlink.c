/*
	btlink.c - hard link 2 btas files together
*/

#include <string.h>
#include <errenv.h>
#include <btas.h>
#include <bterr.h>

int btlink(const char *src,const char *dst) {
  BTCB * volatile b = 0;
  BTCB bt;
  int rc;

  bt.flags = 0;
  catch(rc)
    btopenf(&bt,src,BTNONE,MAXREC);	/* no directory links */
    b = btopendir(dst,BTWRONLY);
    if (b->mid != bt.mid)
      rc = BTERLINK;
    else {
      bt.klen = strlen(bt.lbuf) + 1;
      memcpy(b->lbuf + b->klen,bt.lbuf + bt.klen,bt.rlen - bt.klen);
      b->rlen = bt.rlen - bt.klen + b->klen;
      b->u.cache.node = bt.root;
      rc = btas(b,BTLINK + DUPKEY);		/* add link */
    }
  envend
  if (bt.flags)
    btas(&bt,BTCLOSE);
  (void)btclose(b);
  return rc;
}

int btrename(src,dst)
  const char *src;		/* name of file to link */
  const char *dst;		/* name of file to link to */
{
  return btlink(src,dst) || btkill(src);
}
