/*  Record insert and delete operations.
 
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
#ifndef __BTREE_H
#define __BTREE_H
#include <port.h>
#include <stdlib.h>
#include <setjmp.h>

enum {
  MAXLEV = 8,		/* maximum tree levels */
  SECT_SIZE = 512
};

#include "bttype.h"

/* make sure son is same offset for stem & root
   make sure lbro is same offset for stem & leaf */

struct stem {
  t_block root, son, lbro;	/* root == 0 => deleted node */
  short data[3];		/* data area for two records */
};

struct leaf {
  t_block root, rbro, lbro;
  short data[3];		/* data[0] & 0x8000 marks stem */
};

struct root_node {
  t_block root;		/* root == self */
  t_block son;		/* son == 0 for leaf root */
  struct btstat stat;
  short data[3];	/* data area */
};

union btree {
    struct root_node r;	/* root node */
    struct stem s;	/* or stem node */
    struct leaf l;	/* or leaf node */
    char data[BLKSIZE];	/* but write to disk as char array */
};

extern jmp_buf btjmp;
#define btpost(c)	longjmp(btjmp,c)
#endif
