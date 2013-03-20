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
#include <stdlib.h>
#include <string.h>
#include "btflds.h"

/** Load btfds from a directory record.  When using an existing btflds
 * array, there is no check for overrun.  For a directory record of length
 * n, there should be room for at least n/2 fields (there can be at most
 * n/2 - 1 fields for an empty string filename, but an extra field at the
 * end stores the logical record length).
 * @param fcb	pointer to btflds, or NULL to malloc one
 * @param directory record buffer
 * @param len	length of directory record buffer
 * @return pointer to btflds
 */
struct btflds *ldflds(fcb,buf,len)	/* cvt fld data */
  struct btflds *fcb;       /* fields structure, malloc if NULL */
  const char *buf;            /* directory record */
  int len;			/* size of directory record */
{
  register int rlen;
  rlen = strlen(buf) + 1;
  buf += rlen;			/* skip filename */
  rlen = (len - rlen - 1) / 2;	/* max number of fields */
  if (fcb == 0)
    fcb = (struct btflds *)malloc(
		sizeof *fcb - sizeof fcb->f + (rlen+1) * sizeof fcb->f[0]
	     			 );
  if (rlen <= 0) {
    fcb->klen = fcb->rlen = 0;
    return fcb;
  }
  if (fcb) {
    register struct btfrec *p;
    short pos = 0;
    fcb->klen = *buf++;
    fcb->rlen = 0;
    for (p = fcb->f; fcb->rlen < rlen && *buf; ++p,++fcb->rlen) {
      p->type = *buf++;
      p->len = *buf++;
      p->pos = pos;
      pos += p->len;
    }
    p->type = 0;
    p->len  = 0;
    p->pos  = pos;
  }
  return fcb;
}
