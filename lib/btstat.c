/* $Log$
 */
#include <btas.h>
#include <errenv.h>
#include <string.h>

int btstat(const char *s,struct btstat *p) {
  BTCB bt;
  int rc;
  catch(rc)
    if (btopenf(&bt,s,BTNONE+LOCK+NOKEY+BTDIROK,sizeof bt.lbuf))
      return -1;
    bt.rlen = sizeof *p;
    bt.klen = 0;
    (void)btas(&bt,BTSTAT);
    (void)memcpy(p,bt.lbuf,sizeof *p);
  envend
  btas(&bt,BTCLOSE);
  return rc;
}

int btfstat(BTCB *btcb,struct btstat *p) {
  BTCB bt;
  int rc;
  bt.root = btcb->root;
  bt.mid = btcb->mid;
  bt.flags = btcb->flags;
  bt.u.cache.node = 0;
  bt.klen = 0;
  bt.rlen = sizeof *p;
  catch(rc)
    (void)btas(&bt,BTSTAT);	/* get scoop */
    (void)memcpy((char *)p,bt.lbuf,sizeof *p);
  envend
  return rc;
}
