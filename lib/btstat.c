#include <btas.h>
#include <errenv.h>
#include <mem.h>

int btstat(s,p)
  const char *s;		/* file name to stat */
  struct btstat *p;
{
  BTCB *dir = 0;
  int rc;
  catch(rc)
    dir = btopen(s,BTNONE+LOCK+BTDIROK,sizeof *p);
    if (!dir) return -1;
    dir->rlen = sizeof *p;
    dir->klen = 0;
    (void)btas(dir,BTSTAT);
    (void)memcpy((char *)p,dir->lbuf,sizeof *p);
  envend
  (void)btclose(dir);
  return rc;
}

int btfstat(btcb,p)
  BTCB *btcb;
  struct btstat *p;
{
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
