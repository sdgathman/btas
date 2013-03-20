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
*/
#include <btas.h>
#include <errenv.h>

/** Write all dirty buffers for the current btserve to disk.  On successful
 * return, the sync is complete, and btserve has
 * done an fsync as well for a BTAS filesystem stored in a unix file.
 * FIXME: btserve should use root,mid to selectively flush a specific
 * BTAS file when specified.
 * @return 0 on success
 */
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
