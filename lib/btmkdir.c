/*
	Copyright 1994,2001,2009,2011 Business Management Systems, Inc.
	Author: Stuart D. Gathman

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
#include <stdlib.h>
#include <errenv.h>
#include <btas.h>

/* Create a new directory with standard links.
   Returns BTERDUP if file already exists, and 0 on success.
   Returns BTERKEY if a parent directory doesn't exist.
   @param path pathname to create
   @param mode posix-like mode and permissions
   @return 0 on success
*/
int btmkdir(const char *path,int mode) {
  BTCB * volatile b = 0;
  BTCB * volatile c = 0;
  BTCB *s = btasdir;
  int rc;

  catch(rc)
    b = btopendir(path,BTWRONLY+BTDIROK);
    /* b->lbuf[24] = 0;	/* enforce max filename length */
    /* copy fld to lbuf, compute rlen */
    b->rlen = b->klen;
    b->u.id.user = geteuid();
    b->u.id.group = getegid();
    b->u.id.mode  = getperm(mode) | BT_DIR;
    btas(b,BTCREATE);
    btasdir = b;
    c = btopen(b->lbuf,BTWRONLY+BTDIROK,3);
    strcpy(c->lbuf,".");
    c->rlen = c->klen = 2;
    c->u.cache.node = c->root;
    btas(c,BTLINK);
    strcpy(c->lbuf,"..");
    c->rlen = c->klen = 3;
    c->u.cache.node = b->root;
    btas(c,BTLINK);
  envend
  btasdir = s;
  btclose(c);
  btclose(b);
  return rc;
}
