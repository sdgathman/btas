/*
	Initialize BTAS/2 file systems
 * $Log$
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
    puts("Usage:\tbtinit [-cchksize] [-bblksize] osfile [size]");
    puts("	size is in 512 byte blocks");
    puts("	chksize (checkpoint size) is in 512 byte blocks");
    puts("	blksize is in bytes and defaults to 1024");
}

int main(int argc,char **argv) {
  long size;
  int rc;
  unsigned short blksize = 1024;
  unsigned chksize = 0;
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
    printf("Initialize %s as %ld %u byte blocks for BTAS/X, continue?",
	 argv[1], (size - SECT_SIZE) / blksize, blksize);
  else
    printf("Initialize %s as expandable %u byte blocks for BTAS/X, continue?",
	 argv[1], blksize);
  rc = getchar();
  if (rc != 'y' && rc != 'Y') {
    printf("%s unchanged.\n",argv[1]);
    return 1;
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
    u.d.dtbl->eod = 1L;			/* we will write a root node */
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
      u.r.stat.mtime = u.r.stat.atime = time(&u.r.stat.ctime);
      u.r.stat.id.user = getuid();
      u.r.stat.id.group = getgid();
      u.r.stat.id.mode = mode;
      if (lseek(fd,rootpos,0) != rootpos ||
	write(fd,u.buf,sizeof u.buf) != sizeof u.buf)
	rc = errno;
      else {
	memset(u.buf,0,sizeof u.buf);
	for (unsigned rem = blksize - SECT_SIZE; rem && rem < blksize;
	  rem -= SECT_SIZE)
	  write(fd,u.buf,sizeof u.buf);
      }
    }
  }
  (void)close(fd);
  return rc;
}
