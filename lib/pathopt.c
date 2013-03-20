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

/** Copy file path name with optimizations. Removes '.' and 'par/..'
 * path elements.  The output is never larger than the input.
 * @param path	output path name
 * @param p	input path name
 * @return pointer to output
 */
char *pathopt(char *path,const char *p) {
  /* apply standard .,.. convention */
  char *op = path;
  while (p) {
    int len;
    const char *w = p;
    if (p = strchr(w,'/')) {
      len = p++ - w;
    }
    else
      len = strlen(w);
    memcpy(op,w,len);
    op[len] = 0;
    if (strcmp(op,"..") == 0) {
      if (op > path + 1) {
	char *q;
	*--op = 0;
	if (q = strrchr(path,'/'))
	  ++q;
	else
	  q = path;
	if (strcmp(q,"..")) {
	  op = q;
	  continue;
	}
	*op++ = '/';
      }
    }
    else if (strcmp(op,".") == 0) {	/* strip "." */
      continue;
    }
    op += len;
    *op++ = '/';
  }
  if (op > path + 1) --op;
  *op = 0;
  return path;
}
