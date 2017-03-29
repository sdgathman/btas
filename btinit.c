/*
	Initialize BTAS/2 file systems.  
	This version does not support extents.
 
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
 * $Log$
 * Revision 2.5  2007/06/27 21:17:25  stuart
 * Test suite and fix mounting >2G filesystem.
 *
 * Revision 2.4  2007/06/26 21:40:42  stuart
 * Provide option to eat blocks in btinit to test large file conditions.
 * Fix > 2G support.
 *
 * Revision 2.3  2007/06/21 23:32:51  stuart
 * Create . and .. in root dir.
 *
 * Revision 2.2  2001/02/28 21:28:08  stuart
 * align root with block size
 *
*/

#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include "btree.h"
#include <btas.h>

#include <time.h>

int btinit(const char *, int, long, unsigned, unsigned);

static void usage(void) {
    puts("Usage:\tbtinit [-cchksize] [-bblksize] [-eblks] [-f] osfile [size]");
    puts("	size is in 512 byte blocks");
    puts("-c	chksize (checkpoint size) is in 512 byte blocks");
    puts("-b	blksize is in bytes and defaults to 1024");
    puts("-e	blks is number of blocks to 'eat' for stress testing.");
    puts("-f	force - do not ask before initializing output file.");
}

static long eatblocks = 0L;

int main(int argc,char **argv) {
  long size;
  int rc;
  unsigned short blksize = 1024;
  unsigned chksize = 0;
  char force = 0;
  while (argc > 1 && *argv[1] == '-') {
    --argc, ++argv;
    switch (*++*argv) {
    case 'b':	// blksize
      if (*++*argv) {
	blksize = (int)atol(*argv);
	continue;
      }
      else if (argv[1]) {
	blksize = (int)atol(*++argv),--argc;
	continue;
      }
      break;
    case 'c':	// checkpoint size
      if (*++*argv) {
	chksize = (unsigned)atol(*argv);
	continue;
      }
      else if (argv[1]) {
	chksize = (unsigned)atol(*++argv),--argc;
	continue;
      }
      break;
    case 'e':	// blocks to eat for stress testing
      if (*++*argv) {
	eatblocks = (unsigned)atol(*argv);
	continue;
      }
      else if (argv[1]) {
	eatblocks = (unsigned)atol(*++argv),--argc;
	continue;
      }
      break;
    case 'f':	// do not ask before initializing
      force = 1;
      continue;
    }
    usage();
    return 1;
  }
  if (argc != 3 && argc != 2) {
    usage();
    return 1;
  }
  if (argc == 3)
    size = atol(argv[2]) * SECT_SIZE;
  else
    size = 0L;
  if (size)
    printf("Initializing %s as %ld %u byte blocks for BTAS/X",
	 argv[1], (size - SECT_SIZE) / blksize, blksize);
  else
    printf("Initializing %s as expandable %u byte blocks for BTAS/X",
	 argv[1], blksize);
  if (force)
    printf(".\n");
  else {
    printf(", continue?");
    rc = getchar();
    if (rc != 'y' && rc != 'Y') {
      printf("%s unchanged.\n",argv[1]);
      return 1;
    }
  }
  errno = btinit(argv[1],BT_DIR+0777,size,blksize,chksize);
  if (errno) {
    perror(argv[1]);
    return 1;
  }
  printf("%s initialized.\n",argv[1]);
  return 0;
}

int btinit(const char *s,int mode,long size,unsigned blksize,unsigned chk) {
  int fd,rc = 0;
  long superoffset = SECT_SIZE;
  union {
    struct btfs d;
    struct root_node r;
    char buf[SECT_SIZE];
  } u;
  fd = open(s,O_WRONLY+O_CREAT+O_BINARY,0666);
  if (fd < 0) return errno;
  if (size == 0L)
    size = lseek(fd,0L,2);	/* size of file */
  if (size < 0)
    rc = errno;
  else {
    int align = blksize / SECT_SIZE;	// sector alignment
    memset(u.buf,0,sizeof u.buf);
    u.d.dtbl->eod = 1L + eatblocks;	/* we will write a root node */
    (void)strncpy(u.d.dtbl->name,s,sizeof u.d.dtbl->name);
    u.d.hdr.root = (superoffset + SECT_SIZE) / SECT_SIZE + chk;
    u.d.hdr.root += align - 1;
    u.d.hdr.root -= u.d.hdr.root % align;
    long rootpos = u.d.hdr.root * SECT_SIZE;
    if (size <= rootpos) {
      u.d.hdr.space = 0;
      u.d.dtbl->eof = 0;
    }
    else {
      u.d.dtbl->eof = (size - rootpos) / blksize;
      u.d.hdr.space = u.d.dtbl->eof - 1;
    }
    u.d.hdr.blksize = blksize;
    u.d.hdr.dcnt = 1;
    u.d.hdr.magic = BTMAGIC;
    if (lseek(fd,superoffset,0) == -1L
      || write(fd,u.buf,SECT_SIZE) != SECT_SIZE)
      rc = errno;
    else {
      memset(u.buf,0,sizeof u.buf);
      u.r.root = 1L;
      u.r.stat.bcnt = 1L;
      u.r.stat.links = 1;
      u.r.stat.mtime = u.r.stat.atime = u.r.stat.ctime = time(0);
      u.r.stat.id.user = getuid();
      u.r.stat.id.group = getgid();
      u.r.stat.id.mode = mode;
      u.r.data[0] = 2;
      u.r.data[1] = u.buf + blksize - (char *)u.r.data - 7;
      u.r.data[2] = u.buf + blksize - (char *)u.r.data - 14;
      if (lseek(fd,rootpos,0) != rootpos ||
	write(fd,u.buf,sizeof u.buf) != sizeof u.buf)
	rc = errno;
      else {
	memset(u.buf,0,sizeof u.buf);
	for (unsigned rem = blksize - SECT_SIZE; rem && rem < blksize;
	  rem -= SECT_SIZE) {
	  if (rem == SECT_SIZE) {
	    // write . and .. to last sector of root
	    u.buf[SECT_SIZE-13] = '.';
	    u.buf[SECT_SIZE-8] = 1;
	    u.buf[SECT_SIZE-7] = 1;
	    u.buf[SECT_SIZE-6] = '.';
	    u.buf[SECT_SIZE-1] = 1;
	    long datpos = rootpos + blksize - SECT_SIZE;
	    if (lseek(fd,datpos,0) != datpos || 
	      write(fd,u.buf,sizeof u.buf) != sizeof u.buf)
	      rc = errno;
	  }
	  if (write(fd,u.buf,sizeof u.buf) != sizeof u.buf) {
	    rc = errno;
	    break;
	  }
	}
      }
    }
  }
  (void)close(fd);
  return rc;
}
