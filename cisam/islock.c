/*	islock.c

	Copyright 1990 Business Management Systems, Inc.
	Author: Stuart D. Gathman

	Note that redundant calls to btlock nest.  I don't know what
	islock does in that respect, so beware...
*/

#include "cisam.h"

int islockx(fd) {	/* islock without (ridiculous, stupid) busy waiting */
  struct cisam *r = ischkfd(fd);
  if (r == 0) return iserr(ENOTOPEN);
  btlock(r->key.btcb,0);
  return iserr(0);
}

int islock(fd) {
  struct cisam *r = ischkfd(fd);
  if (r == 0) return iserr(ENOTOPEN);
  return iserr(btlock(r->key.btcb,LOCK) ? ELOCKED : 0);
}

int isunlock(fd) {
  struct cisam *r = ischkfd(fd);
  if (r == 0) return iserr(ENOTOPEN);
  btunlock(r->key.btcb);
  return iserr(0);
}
