#include <btas.h>
#include "fs.h"
#include "../node.h"

/*
	return type of block in a buffer
*/

int btasFS::buftypex(const void *buf,t_block blk) {
  const union btree *b = (const union btree *)buf;
  int rc;
  if (b == 0 || blk == 0) return BLKERR;
  t_block root = b->r.root;
  if (root == 0x80000000L) return BLKERR;
  if (root == 0) return BLKDEL;
  if (root < 0 || root == blk) {
    rc = BLKROOT;
    if (b->r.stat.id.mode & BT_DIR)
      rc |= BLKDIR;
    return (b->s.data[0] & BLK_STEM) ? rc : rc | BLKDATA;
  }
  rc =  (b->s.data[0] & BLK_STEM) ? BLKSTEM : BLKLEAF + BLKDATA;
  if (b->s.son & 0x80000000L)
    rc |= BLKDIR;
  return rc;
}

