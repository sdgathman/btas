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
