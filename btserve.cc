/*  Top level server object
 
    Copyright (C) 1985-2013 Business Management Systems, Inc

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/
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
  delete locktbl;
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
