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
#include <string.h>

/** Optimize path (find shorter equivalent path).
	We try to be smart and efficient, but we aren't perfect.
	We check our work by opening the optimized and unoptimized
	pathnames and comparing root,mid.  In the worst case, we
	leave the path unchanged (but correct).  We will never make the
	path longer.
  @param b An open BTCB for the input path name
  @param name path name to optimize, may be overwritten
  @return open BTCB just like #btopen()
 */
int btpathopt(BTCB *b, char *name) {
  BTCB *t;
  char path[MAXKEY], *p;
  int rc = -1;
  if (strlen(name) >= sizeof path)
    return rc;
  pathopt(path,name);
  /* printf("name=%s\npath=%s\n",name,path); */
  /* now check result */
  t = btopen(path,BTNONE+BTDIROK+NOKEY,0);
  if (t) {
    if (b->root == t->root && b->mid == t->mid)
      strcpy(name,path), rc = 0;
    btclose(t);
  }
  return rc;
}
