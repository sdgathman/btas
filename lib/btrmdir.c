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

/** Removes a btas directory with standard links.  Checks that only self
 * and parent links (identified by matching root nodes, not by name)
 * are in the directory, and if so, removes them and the directory.
 */
int btrmdir(const char *path) {
  BTCB * volatile b = 0;
  BTCB * volatile c = 0;
  BTCB *s = btasdir;
  int rc,pass;

  catch(rc)
    b = btopendir(path,BTWRONLY);
    b->rlen = b->klen;
    btasdir = b;
    c = btopen(b->lbuf,BTRDWR+BTDIROK+BTEXCL,MAXREC+1);
    btasdir = c;
    /* remove self & parent links after checking for any other links */
    for (pass = 1; pass <= 2; ++pass) {
      c->rlen = MAXREC;
      c->klen = 0;
      rc = btas(c,BTREADGE+NOKEY);
      while (rc == 0) {
	struct btstat st;
	struct btlevel fa;
	c->lbuf[c->rlen] = 0;
	c->klen = strlen(c->lbuf) + 1;
	rc = btlstat(c,&st,&fa);
	if (rc) errpost(rc);
	if ((fa.node == c->root && fa.slot == c->mid)
	 || (fa.node == b->root && fa.slot == b->mid)) {
	  if (pass == 2)
	    btas(c,BTDELETE);
	}
	else
	  errpost(BTERDIR);
	c->klen = c->rlen;
	c->rlen = MAXREC;
	rc = btas(c,BTREADGT+NOKEY);
      }
    }
    btclose(c);
    c = 0;
    btas(b,BTDELETE);
    rc = 0;
  envend
  btasdir = s;
  (void)btclose(c);
  (void)btclose(b);
  return rc;
}
