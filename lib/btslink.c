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

#include <string.h>
#include <errenv.h>
#include <btas.h>
#include <bterr.h>

/** Symbolically link 2 btas files together.
 * @param src	BTAS path name
 * @param dst	name to be created with the symlink
 * @return 0 on success
 */
int btslink(const char *src,const char *dst) {
  BTCB * volatile b = 0;
  int rc;

  catch(rc)
    b = btopendir(dst,BTWRONLY);
    b->klen = strlen(b->lbuf) + 1;
    strncpy(b->lbuf + b->klen,src,MAXREC - b->klen);
    b->rlen = b->klen + strlen(b->lbuf + b->klen);
    b->u.cache.node = 0;
    rc = btas(b,BTLINK + DUPKEY);		/* add link */
  envend
  (void)btclose(b);
  return rc;
}
