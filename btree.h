#ifndef __BTREE_H
#define __BTREE_H
#include <port.h>
#include <stdlib.h>
#include <setjmp.h>

enum {
  MAXLEV = 8,		/* maximum tree levels */
  MAXBUF = MAXLEV + 2,	/* maximum buffers reserved */
  SECT_SIZE = 512
};

#include "bttype.h"

extern struct btlevel stack[MAXLEV];
extern struct btlevel *sp;

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

struct root {
  t_block root;		/* root == self */
  t_block son;		/* son == 0 for leaf root */
  struct btstat stat;
  short data[3];	/* data area */
};

union btree {
    struct root r;	/* root node */
    struct stem s;	/* or stem node */
    struct leaf l;	/* or leaf node */
    char data[BLKSIZE];	/* but write to disk as char array */
};

struct BTCB;
struct BLOCK *bttrace(struct BTCB *, int, int);
void btadd(char *, int);
void btdel(void);
extern jmp_buf btjmp;
extern int maxrec;	/* maximum record size */
#define btpost(c)	longjmp(btjmp,c)
#endif
