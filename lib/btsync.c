#include <btas.h>
#include <errenv.h>

int btsync() {
  BTCB bt;
  int rc;
  bt.root = 0;
  bt.mid = 0;
  bt.flags = 0;
  bt.klen = 0;
  bt.rlen = 0;
  catch (rc)
    rc = btas(&bt,BTFLUSH);
  envend
  return rc;
}
