#pragma implementation
#include "btserve.h"
#include "btbuf.h"
#include "btfile.h"
#include "btdev.h"
#include "LockTable.h"

// FIXME: should be part of bufpool
jmp_buf btjmp;		// jump target for btpost

btserve::btserve(int maxblk,unsigned cachesize,char id) {
  iocnt = 0L;
  bufpool = new BlockCache(maxblk,cachesize);
  engine = new btfile(bufpool);
  locktbl = new LockTable;
  DEV::index = id;
}

btserve::~btserve() {
  delete engine;
  delete bufpool;
}

long btserve::curtime = 0L;

int btserve::getmaxrec() const {
  return bufpool->maxrec;
}

void btserve::sync() {
  bufpool->get(0);
}

int btserve::mount(const char *s) {
  int rc = setjmp(btjmp);
  if (rc == 0)
    bufpool->mount(s);
  return rc;
}

void btserve::setSafeEof(bool f) {
  bufpool->safe_eof = f;
}

void btserve::incTrans(int msglen) {
  long cnt = iocnt;
  iocnt = bufpool->serverstats.preads + bufpool->serverstats.pwrites;
  cnt = iocnt - cnt;
  bufpool->serverstats.sum2 += cnt * cnt;
  bufpool->serverstats.lwriteb += msglen;
  ++bufpool->serverstats.trans;
}

int btserve::flush() {
  return bufpool->flush();
}
