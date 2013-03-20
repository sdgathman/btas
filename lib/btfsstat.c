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

/** Retrieve the status of a BTAS filesystem.  
 * @param drive the mount index of the filesystem
 * @param f pointer to btfs (output)
 * @return 0 on success
 */
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
