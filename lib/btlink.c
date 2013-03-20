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

/** Hard link 2 btas files together.  Return BTERDUP is destination
 * name already exists, BTERLINK if source and destination are not
 * on the same filesystem.
 * @param src	source path name
 * @param dst	destination path name
 * @return 0 on success
 */
int btlink(const char *src,const char *dst) {
  BTCB * volatile b = 0;
  BTCB bt;
  int rc;

  bt.flags = 0;
  catch(rc)
    btopenf(&bt,src,BTNONE,MAXREC);	/* no directory links */
    b = btopendir(dst,BTWRONLY);
    if (b->mid != bt.mid)
      rc = BTERLINK;
    else {
      bt.klen = strlen(bt.lbuf) + 1;
      memcpy(b->lbuf + b->klen,bt.lbuf + bt.klen,bt.rlen - bt.klen);
      b->rlen = bt.rlen - bt.klen + b->klen;
      b->u.cache.node = bt.root;
      rc = btas(b,BTLINK + DUPKEY);		/* add link */
    }
  envend
  if (bt.flags)
    btas(&bt,BTCLOSE);
  (void)btclose(b);
  return rc;
}

int btrename(src,dst)
  const char *src;		/* name of file to link */
  const char *dst;		/* name of file to link to */
{
  return btlink(src,dst) || btkill(src);
}
