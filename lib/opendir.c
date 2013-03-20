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
#define _GNU_SOURCE
#include <string.h>
#include <libgen.h>
#include <btas.h>

/** Open the directory containing a path name.  This is handy when
 * preparing to rename or delete the file in question.
 * @param path	pathname of file, dirname(path) is opened
 * @param mode	flags for btopen
 */
BTCB *btopendir(const char *path,int mode) {
  char savbuf[MAXKEY + 1];
  BTCB *b;
  strncpy(savbuf,path,MAXKEY)[MAXKEY] = 0;
  b = btopen(dirname(savbuf),mode|BTDIROK,MAXREC);	/* open directory */
  strncpy(b->lbuf,basename(path),MAXKEY)[MAXKEY] = 0;
  b->klen = strlen(b->lbuf) + 1;
  return b;
}
