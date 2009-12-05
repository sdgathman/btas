#include <btas.h>
#include <errenv.h>

int btlstat(const BTCB *btcb,struct btstat *p,struct btlevel *l) {
  BTCB bt;
  int rc;
  bt.msgident = btcb->msgident;
  bt.root = btcb->root;
  bt.mid = btcb->mid;
  bt.flags = 0;
  bt.u.id.user = getuid();
  bt.u.id.group = getgid();
  bt.u.id.mode = 0;
  catch(rc)
    memcpy(bt.lbuf,btcb->lbuf,bt.klen = btcb->klen);
    bt.rlen = sizeof bt.lbuf;
    (void)btas(&bt,BTSTAT);		/* get scoop */
    memcpy((char *)p,bt.lbuf,sizeof *p);
    l->node = bt.root;
    l->slot = bt.mid;
  envend
  return rc;
}
