/*	BTAS/X Image file system backup
	Author: Stuart D. Gathman
	Copyright 1991 Business Management Systems, Inc.
*/

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errenv.h>
#include "fs.h"

#define zerodel 1

main(int argc,char **argv) {
  int i = 1;
  if (i < argc && strcmp(argv[i],"-f") == 0) {
    BTCB b;
    memset(&b,0,sizeof b);
    ++i;
    envelope
      if (btas(&b,BTFLUSH + BTEXCL + LOCK) != 0)
	fputs("BTAS/X frozen for the duration of this process\n",stderr);
    envend
  }
  if (argc < 2) {
    fputs("$Id$\n\
Usage:	btsave [-f] osfile >archive\n\
	-f	freeze btserve during backup\n\
	osfile	Btas filesystem to backup\n\
",stderr);
    return 1;
  }
  if (i >= argc) return 0;

  btasXFS fs(argv[i],
	isatty(0) ? fsio::FS_RDONLY : fsio::FS_RDONLY + fsio::FS_BGND);
  if (!fs) {
    fprintf(stderr,"%s: not a BTAS/X filesystem.\n",argv[i]);
    return 1;
  }
  { // write BTSAVE header
    union {
      btfs f;
      char buf[SECT_SIZE];
    };
    f = *fs.hdr();
    f.hdr.magic = BTSAVE_MAGIC;
    if (write(1,buf,sizeof buf) != sizeof buf) {
      perror(argv[0]);
      return 1;
    }
  }
  int blksize = fs.blksize();
  union btree *b;
  while (b = fs.get()) {
    int t = fs.lasttype();
    if (t & BLKROOT)
      b->r.root |= 0x80000000L;
    if ((t != BLKDEL) && !(t & BLKROOT))
      t = fs.type(b->r.root);
    if (t & BLKDIR)
      b->r.son |= 0x80000000L;	/* mark directory nodes */
    else if (t == BLKDEL && zerodel) {	/* zero deleted nodes */
      t_block next;
      if (b->l.root)
	next = b->s.lbro;	/* keep chain intact */
      else
	next = b->s.son;
      memset(b->data,0,blksize);
      b->s.son = next;
    }
    if (write(1,b->data,blksize) != blksize) {
      perror(argv[0]);
      return 1;
    }
  }
  if (close(1)) {
    perror(argv[0]);
    return 1;
  }
  fprintf(stderr,"%s: image backup complete\n",argv[i]);
  return 0;
}
