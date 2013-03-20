/*
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
 *
 * $Log$
 * Revision 1.4  2009/12/05 23:33:14  stuart
 * Handle BTSTAT with msgident check
 *
 * Revision 1.3  2001/02/28 22:27:53  stuart
 * use btopenf to avoid malloc
 *
 */
#include <btas.h>
#include <errenv.h>
#include <string.h>

/** Retrieve status (BTAS metadata) for a path name.
 * @param s	path name
 * @param p	btstat struct to store status
 * @return 0 on success
 */
int btstat(const char *s,struct btstat *p) {
  BTCB bt;
  int rc;
  catch(rc)
    if (btopenf(&bt,s,BTNONE+LOCK+NOKEY+BTDIROK,sizeof bt.lbuf))
      return -1;
    bt.rlen = sizeof *p;
    bt.klen = 0;
    (void)btas(&bt,BTSTAT);
    (void)memcpy(p,bt.lbuf,sizeof *p);
  envend
  btas(&bt,BTCLOSE);
  return rc;
}

/** Retrieve status (BTAS metadata) for an open BTCB.
 * @param btcb	pointer to BTCB of open file
 * @param p	btstat struct to store status
 * @return 0 on success
 */
int btfstat(BTCB *btcb,struct btstat *p) {
  BTCB bt;
  int rc;
  bt.msgident = btcb->msgident;
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
