/* btlock.c	temporary file locking

	Copyright 1990 Business Management Systems, Inc.
	Author: Stuart D. Gathman

	Although there is a server operation to do this, it is more efficient
in SysV to do one semop() than to do both msgsnd() and msgrcv() for a server
call.  We hash the root node to an array of binary semaphores.  Collisions
are OK, they just reduce parallelism.

	Note that btlock() counts locks up to a maximum.  This allows
library routines to contain lock/unlock pairs without knowing about
application usage.

	For single user systems, btlock and btunlock are #defined away
in btas.h.
*/
	
#include <btas.h>
#include <bterr.h>

int btlock(b,op)
  BTCB *b;
{
  if (b->flags & BT_LOCK) {	/* already locked */
    if ((b->flags & BT_LOCK) == BT_LOCK) {
      if (op) return BTERLOCK;
      errpost(BTERLOCK);
    }
    b->flags += (~BT_LOCK+1 & BT_LOCK);
    return;
  }
  b->flags += (~BT_LOCK+1 & BT_LOCK);
  /* I can't think of a legitimate reason to "busy wait"
     If you really need it, the semlock interface will
     need a "test and set" option.  For now, we always succeed. */
  semlock((int)b->root,-1);
  return 0;
}

void btunlock(b)
  BTCB *b;
{
  if (b->flags & BT_LOCK) {
    b->flags -= (~BT_LOCK+1 & BT_LOCK);
    if (b->flags & BT_LOCK) return;
    semlock((int)b->root,1);
  }
}
