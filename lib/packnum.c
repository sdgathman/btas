/*	Pack numbers
	Copyright 2001 Business Management Systems, Inc. 
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

    Experimental variable length packed number support.  Not used.

	Input	src/dst	compress buffer
		ubuf	uncompressed buffer
		len	uncompressed length
		return:	total compressed length
*/

#include <block.h>

static const char plead[] = { 0x00,0x20,0x30,0x38,0x3C,0x3E,0x3F };
static const char plim[] = { 0x20,0x10,0x08,0x04,0x02,0x01,0x00 };

int packnum(char *dst,const char *ubuf,int flen) {
  int b = *ubuf & 0xFF;
  char sign = b & 0x80;
  int len = flen;
  char fill,lb;
  if (b & 0x40) {
    b |= 0x80;
    fill = 0xFF;
    while (b == 0xFF && len > 1) {
      b = *++ubuf & 0xFF;
      --len;
    }
    lb = ~b;
  }
  else {
    fill = 0;
    b &= 0x7F;
    while (b == 0 && len > 1) {
      b = *++ubuf & 0xFF;
      --len;
    }
    lb = b;
  }
  switch (len) {
  case 1:
    if (lb < 32) break;
    --ubuf; ++len; b = 0;
  case 2:
    if (lb < 16) { b ^= 0x20; break; }
    --ubuf; ++len; b = 0;
  case 3:
    if (lb < 8) { b ^= 0x30; break; }
    --ubuf; ++len; b = 0;
  case 4:
    if (lb < 4) { b ^= 0x38; break; }
    --ubuf; ++len; b = 0;
  case 5:
    if (lb < 2) { b ^= 0x3C; break; }
    --ubuf; ++len; b = 0;
  case 6:
    --ubuf; ++len; b = 0x3E;	// 3E is followed by 6 bytes
    break;
  case 7:
    --ubuf; ++len;
  default:
    --ubuf; ++len;
    b = 0x3F;
    len = 9;
  }
  *dst++ = sign | b;
  while (--len > flen)
    *dst++ = fill;
  while (--len > 0)
    *dst++ = *ubuf++;
}

int unpacknum(const char *src,char *ubuf,int len) {
}
