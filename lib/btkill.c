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
#include <btflds.h>

/** Delete btas file.  
 * @param path pathname of file to delete
 * @return 0 on success
 */
int btkill(path)
  const char *path;		/* name of file to delete */
{
  BTCB * volatile b = 0;
  int rc;

  catch(rc)
    b = btopendir(path,BTWRONLY);
    b->rlen = b->klen;		/* filename is key */
    rc = btas(b,BTDELETE+NOKEY);
  envend
  (void)btclose(b);
  return rc;
}
