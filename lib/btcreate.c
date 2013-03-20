/*	btcreate.c - Creates a btas file with standard field table.
 
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
#include <errenv.h>
#include <btflds.h>

#ifndef __MSDOS__
#include <unistd.h>
int getperm(int mask) {
  register int rc = umask(mask);
  return umask(rc) & ~rc;
}
#else
#define	getperm(mask)	mask
#endif

/** Store btflds in a compact portable form.  By convention, BTAS
 * directory records have a null terminated filename, and the field
 * table follows the null terminator.
 * @param fld field table as btflds
 * @param directory record buffer
 * @return new directory record length
 */
int stflds(const struct btflds *fld,char *buf) {
  register char *bf = buf;
  register int i;

  bf += strlen(bf) + 1;		/* skip over filename */
  if (fld) {
    *bf++ = fld->klen;
    for (i = 0; i < fld->rlen; i++) {
      *bf++ = fld->f[i].type;
      *bf++ = fld->f[i].len;
    }
  }
  return bf - buf;	/* return new record length */
}

/** Create a file in the current directory.  The file is owned by
 * the effective user and group.  However, nothing prevents a malicious
 * client from lying when using the SysV IPC implementation (which
 * doesn't inform the server of the clients user and group).
 * @param path	filename
 * @param fld field table
 * @param mode	file mode and permissions (posix-like)
 * @return BTERDUP if file already exists, and 0 on success.
*/
int btcreate(const char *path,const struct btflds *fld,int mode) {
  BTCB * volatile b = 0;
  int rc;

  catch(rc)
    b = btopendir(path,BTWRONLY);
    /* b->lbuf[24] = 0;	/* enforce max filename length */
    /* copy fld to lbuf, compute rlen */
    b->rlen = stflds(fld,b->lbuf);
    b->u.id.user = geteuid();
    b->u.id.group = getegid();
    b->u.id.mode  = getperm(mode);
    rc = btas(b,BTCREATE+DUPKEY);
  envend
  (void)btclose(b);
  return rc;
}
