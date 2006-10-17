/* $Log$
/* Revision 1.4  2005/05/10 21:41:46  stuart
/* More stringent checks on records.
/*
/* Revision 1.3  2002/11/06 05:18:13  stuart
/* remove libg++
/*
/* Revision 1.2  2001/02/28 22:44:02  stuart
/* use new API
/*
 * Revision 1.1  1999/01/25  22:26:27  stuart
 * Initial revision
 *
 *	BTAS/X dump directory
 *	Author: Stuart D. Gathman
 *	Copyright 1991 Business Management Systems, Inc.

	Displays a directory of a BTAS/X filesystem.  Usually this is an
image backup on tape, but could also be a filesystem on disk.  This program
can be used to identify the root id's of files on an image backup, or to find
"lost" files and directories.
*/

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include "fs.h"
#include "../node.h"
#include "logdir.h"
/* #define TRACE */

static int donode(Logdir &,t_block, NODE *, void *, int);

static int maxrec;

int main(int argc,char **argv) {
  const char *name;
  if (argc != 2 && argc != 1) {
    fputs("Usage:\tbtddir [osfile]\n",stderr);
    return 1;
  }
  name = argv[1];
  btasXFS fs(name,
    isatty(0) ? fsio::FS_RDONLY : fsio::FS_RDONLY + fsio::FS_BGND);
  if (!name) name = "(stdin)";
  if (!fs) {
    if (errno == 294)
      fprintf(stderr,"%s: not a BTAS/X filesystem\n",name);
    else
      perror(name);
    return 1;
  }
  Logdir log;
  union btree *b;
  maxrec = btmaxrec(fs.blksize());
  while ((b = fs.get()) != 0) {
    int t = fs.lasttype();
    int cnt = 0;
#if 0
    printf("root=%08X,node=%08X,maxrec=%d,bp=%08X\n",
      b->r.root,fs.lastblk(),fs.maxrec(),b->data);
#endif
    if (t == BLKDEL) continue;
    if (fs.isSaveImage())
      b->r.root &= 0x7fffffffL;
    if (t & BLKROOT) {
      log.logroot(b->r.root,&b->r.stat);
      if (t & BLKDATA)
	cnt = donode(log,
	  b->r.root,(NODE *)b->r.data,b->data + fs.blksize(),(t & BLKDIR)
	);
#ifdef TRACE
      printf("blktypex = %02X, cnt = %d\n",t,cnt);
#endif
    }
    else {
      int roottype = t;
      if (!fs.isSaveImage()) {
	roottype = fs.type(b->l.root);
	if (!(roottype & BLKROOT)) continue;
      }
      if (t & BLKDATA) {
	cnt = donode(log,
	  b->l.root,
	  (NODE *)b->l.data, b->data + fs.blksize(), (roottype & BLKDIR)
	);
      }
    }
    if (cnt < 0 && (cnt & 0x7fff) > fs.blksize())
      printf("%d root=%08lX blk=%08lX\n",cnt,b->r.root,fs.lastblk());
    log.logrecs(b->r.root,cnt);
  }
  fprintf(stderr,"%ld cache hits, %ld cache misses\n",fs.hits,fs.misses);
  log.logprint(1L);
  return 0;
}

static int donode(Logdir &log,t_block root,NODE *np,void *blkend,int dirflag) {
  int i;
  int cnt;
  cnt = np->size();
  if (cnt <= 0) return cnt;
  np->setsize(blkend);
  for (i = 1; i <= cnt; ++i)	/* offsets must be strictly descending */
    if (np->free(i) >= np->free(i-1)) {
      return -2;
    }
  if (np->free(cnt) < 0) return -3;
  if (*np->rptr(cnt)) return -4;	/* first dup count must be zero */
  char *lbuf = (char *)alloca(np->size());
  int lastrlen = 0;
  if (dirflag) while (--i) {
    unsigned char *p = np->rptr(i);
    int rlen = np->size(i) + *p - np->PTRLEN;
    if (rlen + np->PTRLEN > maxrec) return -5;
    if (rlen >= 0) {
      if (i < cnt && *p < lastrlen && (unsigned char)lbuf[*p] >= p[1])
	return -6;
      memcpy(lbuf + *p,p + 1,rlen - *p);
      log.logdir(root,lbuf,rlen,np->ldptr(i));
    }
    lastrlen = rlen;
  }
  else while (--i) {
    unsigned char *p = np->rptr(i);
    int rlen = np->size(i) + *p - 1;
    if (rlen > maxrec) return -5;
    if (rlen >= 0) {
      if (i < cnt && *p < lastrlen && (unsigned char)lbuf[*p] >= p[1])
	return -6;
      memcpy(lbuf + *p,p + 1,rlen - *p);
    }
    lastrlen = rlen;
  }
  return cnt;
}
