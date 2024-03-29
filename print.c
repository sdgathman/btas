/* Route printf calls to the message() routine that is specialized
   for a specific environment, e.g. MSDOS or HBX.  Not needed for POSIX.
  
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
#include <stdarg.h>
#include "btserve.h"

int printf(const char *fmt,...) {
  char buf[1024];
  int rc;
  va_list ap;
  va_start(ap,fmt);
  rc = vsprintf(buf,fmt,ap);
  va_end(ap);
  message(buf);
  return rc;
}

int fprintf(FILE *f,const char *fmt,...) {
  char buf[1024];
  int rc;
  va_list ap;
  &f;
  va_start(ap,fmt);
  rc = vsprintf(buf,fmt,ap);
  va_end(ap);
  message(buf);
  return rc;
}
