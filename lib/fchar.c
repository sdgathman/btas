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
#include <ftype.h>
#include <string.h>

void ldchar(src,len,dst)
  const char *src;
  char *dst;
{
  while (len-- && src[len] == ' ');
  memcpy(dst,src,++len);
  dst[len] = 0;
}

int stchar(src,dst,len)
  const char *src;
  char *dst;
{
  if (src)
    while (len && *src) *dst++ = *src++,len--;
  memset(dst,' ',len);
  return (*src) ? -1 : 0;
}
