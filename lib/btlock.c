/* 
	Copyright 1990 Business Management Systems, Inc.
	Author: Stuart D. Gathman

    This file is part of the BTAS client library.

    The BTAS client library is free software: you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public License as
    published by the Free Software Foundation, either version 3 of the License,
    or (at your option) any later version.

    BTAS is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with BTAS.  If not, see <http://www.gnu.org/licenses/>.

*/
	
#include <btas.h>
#include <bterr.h>
/** Acquire a voluntary exclusive lock on a btas file.  
 <p>
	Although there is a server operation to do this, it is more efficient
in SysV to do one semop() than to do both msgsnd() and msgrcv() for a server
call.  We hash the root node to an array of binary semaphores.  Collisions
are OK, they just reduce parallelism.
<p>
	Note that btlock() counts locks up to a maximum.  This allows
library routines to contain lock/unlock pairs without knowing about
application usage.
<p>
	For single user systems, btlock and btunlock are \#defined away
in btas.h.
 */
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

/** Release a voluntary exclusive lock on a btas file.  */
void btunlock(b)
  BTCB *b;
{
  if (b->flags & BT_LOCK) {
    b->flags -= (~BT_LOCK+1 & BT_LOCK);
    if (b->flags & BT_LOCK) return;
    semlock((int)b->root,1);
  }
}
