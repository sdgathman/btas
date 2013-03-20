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

/** Clear a btas file by pathname.   All records are removed from the
 * file by the efficient expedient of deleting it, then creating it
 * again with the same directory record.
 */
int btclear(path)
  const char *path;		/* name of file to clear */
{
  BTCB * volatile b = 0;
  struct btstat st;
  int rc;

  /* FIXME: should use btlstat on later btservers that support it */
  btstat(path,&st);	/* get perm */
  catch(rc)
    int rlen;
    b = btopendir(path,BTRDWR);
    b->rlen = MAXREC;
    btas(b,BTREADEQ);		/* read filename & field table */
    rlen = b->rlen;		/* save rlen to work around btserve bug */
    btas(b,BTDELETE+NOKEY);	/* delete filename */
    b->u.id = st.id;		/* restore perm */
    b->rlen = rlen;
    btas(b,BTCREATE);		/* recreate filename */
    rc = 0;
  envend
  (void)btclose(b);
  return rc;
}
