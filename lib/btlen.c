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
#include <btflds.h>
#include <string.h>

/* OK, so you hate the names.
	Well I asked, and no-one had any better ideas. */

/** Return logical key length of a directory record. Directory records
 * consist of a file name followed by a field table.  Return the logical
 * key length described by the field table.
 */
int bt_klen(const char *f) {
  register int n, len = 0;
  f += strlen(f) + 1;
  for (n = *f; n && *++f; --n) {
     if (*f++ != BT_SEQ)
       len += (unsigned char)*f;
  }
  return len;
}

/** Return logical record length of a directory record. Directory records
 * consist of a file name followed by a field table.  Return the logical
 * record length described by the field table.
 */
int bt_rlen(const char *f,int rlen) {
  register int len;
  len = strlen(f) + 1;
  f += len;
  rlen -= len + 1;
  len = 0;
  while (--rlen > 0 && *++f) {
    if (*f++ != BT_SEQ)
      len += (unsigned char)*f;
    if (--rlen == 0) break;
  }
  return len;
}
