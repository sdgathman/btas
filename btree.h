#include <stdlib.h>
#include <setjmp.h>

#define STATIC		/* private functions made extern for debugging */

#define MAXLEV  8		/* maximum tree levels */
#define MAXBUF  (MAXLEV + 2)	/* maximum buffers reserved */

#include "bttype.h"

#define blk_dev(b)	((unsigned char)((b) >> 24))	/* device index */
#define blk_off(b)	((b) & 0xFFFFFFL)		/* block offset */
#define mk_blk(i,b)	(((long)(i)<<24)|(b))

/* byte position of block */
#define blk_pos(b,s)	(((b)-1) * (long)(s) + (long)SECT_SIZE)

extern struct btlevel stack[];
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

typedef struct block {
  struct block *lru;
#ifndef __MSDOS__
  struct block *mru;	/* lru buffer chain */
#endif
  t_block blk;		/* block address */
  short mid;		/* mount id */
  short flags;
  short count;		/* holds count while in memory */
  union node *np;	/* pointer to data area of node */
  union btree buf;
} BLOCK;

/* flag values */
#define BLK_MOD		001	/* block modified */
#define BLK_LOW		002	/* block is low priority */
#define BLK_CHK		004	/* block has been checkpointed */
#define BLK_ROOT	010	/* root node */
#define BLK_KEY		020	/* block id has been modified */
#define BLK_DIR		040	/* block belongs to a directory */
#define BLK_TOUCH      0100	/* block is in touched list */
#define BLK_STEM	(unsigned short)0x8000	/* key data */

struct BTCB;
BLOCK *bttrace(struct BTCB *, int, int);
void btadd(char *, int);
void btdel(void);
extern jmp_buf btjmp;
#define btpost(c)	longjmp(btjmp,c)
