/*	BTAS/X Image file system restore
	Author: Stuart D. Gathman
	Copyright 1991 Business Management Systems, Inc.
*/
// $Log$
// Revision 1.3  1995/11/08  19:36:46  stuart
// oops
//
// Revision 1.2  1995/11/02  15:33:59  stuart
// rearrange RCS id
//
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <io.h>
#include "fs.h"

// FIXME: can only restore one extent

enum { SECT_SIZE = 512 };

int main(int argc,char **argv) {
  bool convert68k = false;	// convert from 68k format
  unsigned chk = 0;		// checkpoint size
  int i;
  for (i = 1; i < argc; ++i) {
    if (strcmp(argv[i],"-6") == 0)
      convert68k = true;
    else if (strncmp(argv[i],"-c",2) == 0) {
      if (argv[i][2])
	chk = atoi(argv[i] + 2);
      else
	chk = atoi(argv[++i]);
    }
    else
      break;
  }
  if (i + 1 != argc) {
    fputs("$Id$\n\
Usage:	btrest [-6] [-cchksize] osfile <archive\n\
	-6	convert root nodes from 68k to RISC format (add padding)\n\
	-c	specify checkpoint size to reserve\n\
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
      char buf[SECT_SIZE];
    };
    bool seekable = false;
    if (chk) {
      long superoffset = SECT_SIZE;
      seekable = true;
      if (fseek(outf,superoffset,0)) {
	memset(buf,0,sizeof buf);
	if (fwrite(buf,sizeof buf,1,outf) != 1) {
	  perror(outputname);
	  return 1;
	}
      }
      f = *fs.hdr();
      f.hdr.root = (superoffset + SECT_SIZE) / SECT_SIZE + chk;
      int blksects = f.hdr.blksize / SECT_SIZE;
      if (f.hdr.root % blksects)
	f.hdr.root += blksects - f.hdr.root % blksects;
    }
    else
      f = *fs.hdr();
    long rootpos = f.hdr.root * SECT_SIZE;
    f.hdr.magic = BTMAGIC;
    if (fwrite(buf,sizeof buf,1,outf) != 1) {
      perror(outputname);
      return 1;
    }
    if (seekable) {
      if (fseek(outf,rootpos,0)) {
	perror(outputname);
	return 1;
      }
    }
    else {
      memset(buf,0,sizeof buf);
      while (rootpos > SECT_SIZE) {
	if (fwrite(buf,sizeof buf,1,outf) != 1) {
	  perror(outputname);
	  return 1;
	}
	rootpos -= SECT_SIZE;
      }
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
