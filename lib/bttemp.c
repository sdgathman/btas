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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <btas.h>
#include <btflds.h>
#include <bterr.h>
#include <unistd.h>

/** Open a temporary BTAS file.
	Using SysV IPC, BTAS can't close files when processes end because
it doesn't know when they end.  This routine, therefore, unlocks and
deletes the temp files if they already exist.  Since the temp name includes
the process id, this shouldn't trash temp files of a running program.
However, this is not strictly
kosher and will go away for more capable message passing APIs.
 @param prefix	file name prefix - helpful when debugging
 @param flds	logical field table for temporary file
 @param name	buffer to store temporary file name
 @return open BTCB pointer for the temporary file
 */
BTCB *btopentmp(prefix,flds,name)
  const char *prefix;		/* make temp name mean something */
  const struct btflds *flds;
  char *name;
{
  static short ctr;
  BTCB *b;
  sprintf(name,"/tmp/%s%ld",prefix,getpid()*1000L+ctr++);
  b = btopen(name,BTRDONLY + BTEXCL + LOCK + NOKEY,strlen(name));
  if (b->op == BTERLOCK) b->flags = BTEXCL;
  btclose(b);		/* unlock if already open */
  btkill(name);		/* delete if already there */
  if (btcreate(name,flds,0644)) return 0;
  b = btopen(name,BTRDWR,btrlen(flds));
  return b;
}

/** Close and remove a temporary file.  
 * @param b	the BTCB returned from btopentmp
 * @param name	the temporary name returned from btopentmp
 */
void btclosetmp(b,name)
  BTCB *b;
  const char *name;	/* name of file when created */
{
  btclose(b);
  (void)btkill(name);
}
