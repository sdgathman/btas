/*	BTAS/X dump directory
	Author: Stuart D. Gathman
	Copyright 1991 Business Management Systems, Inc.

	Displays a directory of a BTAS/X filesystem.  Usually this is an
image backup on tape, but could also be a filesystem on disk.  This program
can be used to identify the root id's of files on an image backup, or to find
"lost" files and directories.
*/

#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <alloc.h>
#include <errno.h>
extern "C" {
#include <btas.h>
int ldedir(BTCB *,const unsigned char far *,int,int);
}
#include "s1tar.h"
/* #define TRACE */
static int donode(t_block, s1btree *, unsigned, int);

static s1tar *tar = 0;

main(int argc,char **argv) {
  const char *name;
  int ofd = -1;
  if (strncmp(argv[1],"-f",2) == 0) {
    ofd = open(argv[1]+2,O_WRONLY+O_CREAT+O_TRUNC,0644);
    if (ofd < 0)
      perror(argv[1]+2);
    --argc; ++argv;
  }
  if (argc != 2 && argc != 1) {
    fputs("Usage:\tbtddir1 [-ftarfile] [osfile]\n",stderr);
    return 1;
  }
  name = argv[1];
  s1FS fs(name,isatty(0) ? FS_RDONLY : FS_RDONLY + FS_BGND);
  if (!name) name = "(stdin)";
  if (!fs) {
    if (errno == 294)
      fprintf(stderr,"%s: not a BTAS/1 filesystem\n",name);
    else
      perror(name);
    return 1;
  }
  s1btree *b;
  while (b = fs.get()) {
    int t = fs.lasttype();
    int cnt = 0;
#if 0
    printf("root=%08X,node=%08X,blksize=%d,bp=%08X\n",
      b->r.root,lastblock(fs),fs->u.f.hdr.blksize,b->data);
#endif
    if (t == BLKDEL) continue;
    if (t & BLKROOT) {
      btstat st;
      st.bcnt = (b->root & 0x7fffffffL) + 1;
      st.rcnt = b->r.size;
      st.atime = st.ctime = st.mtime = 0;
      st.links = -1;
      st.opens = b->r.lock;
      // id filled in by directory record for BTAS/1
      st.id.user = 0;
      st.id.group = 0;
      st.id.mode = (b->r.type ? 0 : BT_DIR) | 0666;
      t_block root = fs.lastblk();
      logroot(root,&st);
      if (t & BLKDATA) {
	short rlen = 0;
	if (b->info.klen < 0)
	  rlen = b->info.rlen;
	else if (t & BLKDIR)
	  rlen = fs.rlen;
	cnt = donode(root,b,rlen,t & BLKDIR);
      }
      logrecs(root,cnt);
#ifdef TRACE
      //fprintf(stderr,"blktypex = %02X, cnt = %d\n",t,cnt);
#endif
    }
    else {
      if (t & BLKDATA) {
#ifdef TRACE
	fprintf(stderr,"blk=%08X root=%08X typ=%d klen=%d rlen=%d\n",
	  fs.lastblk(),long(b->root),t,(int)b->info.klen,(int)b->info.rlen);
#endif
	t = fs.type(b->root);
	if (b->root == 2)
	  fprintf(stderr,"blk=%08X root=%08X typ=%d\n",
	    fs.lastblk(),long(b->root),t);
	if (!(t & BLKROOT)) continue;
	short rlen = 0;
	if (b->info.klen < 0)
	  rlen = b->info.rlen;
	else if (t & BLKDIR)
	  rlen = fs.rlen;
	cnt = donode(b->root,b,rlen,t & BLKDIR);
      }
      logrecs(b->root,cnt);
    }
  }
  fprintf(stderr,"%ld cache hits, %ld cache misses\n",fs.hits,fs.misses);
  s1tar tarfile(ofd,&fs);
  if (ofd > 1)
    tar = &tarfile;	// ptr for doroot()
  logprint(2L);
  return 0;
}

static int donode(t_block root,s1btree *np,unsigned rlen,int dirflag) {
  int i;
  int cnt;
  cnt = np->count;
  if (cnt <= 0) return cnt;
  if (rlen <= 0) return -1;
  if (cnt * rlen > 512 - 18) return -2;
  if (dirflag) for (i = 0; i < cnt; ++i) {
    s1dir *p = (s1dir *)(np->data + i * rlen);
    BTCB b;
    btstat st;
    if ((unsigned)p->rlen > 494u) return -3;
    if (ldedir(&b,p->name,p->klen & 0x7fff,p->rlen))
      return -4;
#ifdef TRACE
    fprintf(stderr,"%8lX:%s (%d,%d) -> %8lX\n",
	root,b.lbuf,(int)p->rlen,(int)p->klen,(long)p->root);
#endif
    logdir(root,b.lbuf,b.rlen,p->root);
  }
  return cnt;
}

/* hook to output files in #TAR format */

void doroot(struct root_n *r) {
  if (tar) {
    long recs = tar->addfile(r->root,r->path);
    if (recs >= 0) {
      fprintf(stderr,"	%ld recs archived\n",recs);
    }
  }
}

extern "C" void perror(const char *s) {
  fprintf(stderr,"%s: (%d) %s\n",s,errno,
    ((unsigned)errno < (unsigned)sys_nerr) ? sys_errlist[errno] : 
	"unknown error"
  );
}
