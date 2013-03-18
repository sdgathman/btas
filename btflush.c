/*  Command line utility to trigger BTFLUSH.
 
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
#include <btas.h>
#include <bterr.h>

main(int argc,char **argv) {
  int secs = 0;
  if (argc == 2)
    secs = atoi(argv[1]);
  if (!secs)
    secs = 30;
  for (;;) {
    int t = secs;
    int rc = btsync();
    if (rc == 298) break;
    if (rc == BTERBUSY)
      t = 2;
    sleep(t);
  }
  return 0;
}
