/*
	rebuild free space on unmounted BTAS/X filesystems.

	It is possible to rebuild free space while mounted provided
	blocks freed before and after starting the rebuild can be
	distinguished.  (With a delete timestamp for instance.)  This
	version does not try to do that.
*/

#include <stdio.h>
#include <errno.h>
#include "fs.h"

struct {
  long dels;
  long instant;
  long files, dirs;
  long stems, leaves;
  long dirstems, dirleaves;
  long garbage;
  long ioerrs;
} count;

int main(int argc,char **argv) {
  union btree *b;
  if (argc != 2) {
    (void)puts("Usage:\tbtfree filesys_osname");
    return 1;
  }
  btasXFS fs(argv[1],isatty(0)?0:FS_BGND);
  if (fs.blksize() == 0) {
    perror(argv[1]);
    return 1;
  }
  fs.clear();
  while (b = fs.get()) {
    int t = fs.lasttype();
    switch (t & BLKCOMPAT) {
    case BLKDEL:		/* deleted node */
      fs.del();
      ++count.dels;
      break;
    case BLKROOT:		/* root node */
      if (t & BLKDIR) {
	if (t & BLKDATA)
	  ++count.dirleaves;
	else
	  ++count.dirstems;
	++count.dirs;
      }
      else {
	if (t & BLKDATA)
	  ++count.leaves;
	else
	  ++count.stems;
	++count.files;
      }
      break;
    case BLKSTEM: case BLKLEAF:	/* stem or leaf */
      switch (fs.type(b->r.root) & (BLKCOMPAT|BLKDIR)) {
      case BLKDEL:		/* instant deleted */
	fs.del();
	++count.instant;
	break;
      case BLKROOT | BLKDIR:
	if (t & BLKDATA)
	  ++count.dirleaves;
	else
	  ++count.dirstems;
	break;
      case BLKROOT:		/* valid stem or leaf */
	if (t & BLKDATA)
	  ++count.leaves;
	else
	  ++count.stems;
	break;
      default:		/* garbage node */
	/* delblock(fs); */
      case BLKERR:		/* couldn't read root */
	++count.garbage;
      }
      break;
    default:
      ++count.ioerrs;
    }
  }
  printf("Statistics:        \n");
  printf("  %ld deleted, %ld instant deleted nodes\n",
	count.dels,count.instant);
  printf("  %ld garbage, %ld unreadable nodes\n",
	count.garbage,count.ioerrs);
  printf("  %ld data, %ld index nodes in %ld files\n",
	count.leaves, count.stems, count.files);
  printf("  %ld data, %ld index nodes in %ld directories\n",
	count.dirleaves, count.dirstems, count.dirs);
  printf("  %ld cache hits, %ld cache misses\n",fs.hits,fs.misses);
  return 0;
}
