#include <btas.h>
#include <errenv.h>

/** Return information about file pointed to by symlink.
  @param btcb the BTCB to report
  @param p place to store status of file
  @param l place to store root and mid of file
 */
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
    if (btcb->klen > sizeof bt.lbuf) errpost(BTERKLEN);
    memcpy(bt.lbuf,btcb->lbuf,bt.klen = btcb->klen);
    bt.rlen = sizeof bt.lbuf;
    (void)btas(&bt,BTSTAT);		/* get scoop */
    memcpy((char *)p,bt.lbuf,sizeof *p);
    l->node = bt.root;
    l->slot = bt.mid;
  envend
  return rc;
}
