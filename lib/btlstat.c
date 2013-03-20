/* 
	Copyright 1994,2001,2009,2011 Business Management Systems, Inc.
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
