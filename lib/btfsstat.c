#include <btas.h>
#include <errenv.h>

int btfsstat(int drive,struct btfs *f) {
  BTCB t;
  int rc = 0;
  t.root = 0;
  t.flags = 0;
  t.mid = drive;
  t.rlen = MAXREC;
  t.klen = 0;
  catch(rc)
    btas(&t,BTSTAT);	/* try to stat drive */
    if (t.rlen > sizeof *f) t.rlen = sizeof *f;
    memcpy(f,t.lbuf,t.rlen);
  envend
  return rc;
}
