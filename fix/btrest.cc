/*	BTAS/X Image file system restore
	Author: Stuart D. Gathman
	Copyright 1991 Business Management Systems, Inc.
*/
// $Log$
// Revision 1.2  1995/11/02  15:33:59  stuart
// rearrange RCS id
//
#include <stdio.h>
#include <fcntl.h>
#include <io.h>
#include "fs.h"

// FIXME: can only restore one extent

int main(int argc,char **argv) {
  bool convert68k = false;
  int i;
  for (i = 1; i < argc; ++i) {
    if (strcmp(argv[i],"-6") == 0)
      convert68k = true;
    else
      break;
  }
  if (i + 1 != argc) {
    fputs("$Id$\n\
Usage:	btrest [-6] osfile <archive\n\
	-6	convert root nodes from 68k to RISC format (add padding)\n\
	osfile	filesystem to overwrite, - for stdout\n",
	stderr);
    return 1;
  }
  btasXFS fs(0,isatty(2) ? fsio::FS_RDONLY : fsio::FS_RDONLY + fsio::FS_BGND);
  btree *b;
  if (!fs || (b = fs.get()) == 0) {
    fprintf(stderr,"[stdin]: not a BTAS/X filesystem.\n");
    return 1;
  }
  const char *outputname = argv[i];
  unsigned blksize = fs.blksize();
  if (convert68k) {
    if (b->r.data[-1] == 0) {
      fputs(
	"btrest: Root node is empty.  Perhaps you specified -6 in error?\n",
	stderr
      );
      return 1;
    }
  }
  else {
    int cnt1 = b->r.data[0] & 0x7fff;
    if (cnt1 > blksize/2) {	// convert from unaligned btperm
      fputs(
	"btrest: Invalid root node.  Perhaps you need -6 ?\n",
	stderr
      );
      return 1;
    }
  }

  FILE *outf;
  if (strcmp(outputname,"-"))
    outf = fopen(outputname,"w");
  else {
    outf = stdout;
    outputname = "(stdout)";
  }
  if (!outf) {
    perror(outputname);
    return 1;
  }
  setvbuf(outf,0,_IOFBF,8192);
  {
    union {
      btfs f;
      char buf[512];
    };
    f = *fs.hdr();
    f.hdr.magic = BTMAGIC;
    if (fwrite(buf,sizeof buf,1,outf) != 1) {
      perror(outputname);
      return 1;
    }
  }
  do {
    b->r.root &= ~0x80000000L;	/* unmark root nodes */
    b->r.son &= ~0x80000000L;	/* unmark directory nodes */
    if (b->r.root == fs.lastblk()) {	// if root node
      if (convert68k) {
	int cnt = b->r.data[-1] & 0x7fff;
	if (cnt && b->r.data[cnt - 1] < 2 * cnt + 4)
	  fprintf(stderr,"Cannot convert root %08lX\n",b->r.root);
	else {
	  while (cnt-- > 0)
	    b->r.data[cnt + 1] = b->r.data[cnt] - 2;
	  b->r.data[0] = b->r.data[-1];
	  b->r.data[-1] = 0;
	}
      }
    }
    if (fwrite(b->data,blksize,1,outf) != 1) {
      perror(outputname);
      fclose(outf);
      return 1;
    }
    b = fs.get();
  } while (b);
  if (fclose(outf)) {
    perror(outputname);
    return 1;
  }
  fprintf(stderr,"%s: image restore complete\n",outputname);
  return 0;
}
