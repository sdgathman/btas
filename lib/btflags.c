#include <btas.h>

char btopflags[32] = {	/* flags required by operation */
	/* read operations */
  BT_READ, BT_READ, BT_READ, BT_READ, BT_READ, BT_READ, BT_READ, BT_READ,
	/* write,rewrite,delete */
  BT_UPDATE, BT_UPDATE, BT_UPDATE, BT_UPDATE, BT_UPDATE, BT_UPDATE,
	/* open, create, seek, link, touch, stat, flush */
  0, BT_UPDATE, BT_READ|BT_UPDATE, BT_UPDATE, BT_OPEN, 0, 0,
	/* mount, join, unjoin, close, umount */
  BT_UPDATE, BT_UPDATE, BT_UPDATE, 0, BT_UPDATE
};

