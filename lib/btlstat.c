#include <btas.h>
#include <errenv.h>
#include <mem.h>

int btlstat(const BTCB *btcb,struct btstat *p,struct btlevel *l) {
  BTCB bt;
  int rc;
  bt.root = btcb->root;
  bt.mid = btcb->mid;
  bt.flags = 0;
  bt.u.id.user = getuid();
  bt.u.id.group = getgid();
  bt.u.id.mode = BT_DIR;
  catch(rc)
    memcpy(bt.lbuf,btcb->lbuf,bt.klen = btcb->klen);
    bt.rlen = sizeof bt.lbuf;
    btas(&bt,BTOPEN+BT_DIR+LOCK);	/* open */
    bt.klen = 0;
    bt.rlen = sizeof *p;
    (void)btas(&bt,BTSTAT);		/* get scoop */
    memcpy((char *)p,bt.lbuf,sizeof *p);
    l->node = bt.root;
    l->slot = bt.mid;
  envend
  (void)btas(&bt,BTCLOSE);		/* close */
  return rc;
}
