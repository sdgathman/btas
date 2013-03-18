/*  Command line server status query.
 
    Copyright (C) 1985-2013 Business Management Systems, Inc

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/
#include <stdio.h>
#include <stdlib.h>
#include <btas.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>

main() {
  int btasreq, btasres;
  struct msqid_ds buf;
  const char *s = getenv("BTSERVE");
  long key;
  if (!s) s = "";
  key = BTASKEY + *s * 2;
  btasreq = msgget(key,0620);
  btasres = msgget(key+1,0640);
  if (btasreq == -1 && btasres == -1) {
    printf("Server%s not loaded\n",s);
    return 1;
  }
  printf("Server%s loaded\n",s);
  return 0;
}
