/* Need to check pathnames when renaming a directory.
   Also need to fix the '..' file. */
#include <stdio.h>
#include <string.h>
#include <errenv.h>
#include <bterr.h>
#include "btutil.h"

static void copy1(/**/ BTCB *, BTCB * /**/);
static int copymove(/**/ int /**/);

int copy() {
  return copymove(0);
}

int move() {
  return copymove(1);
}

int linkbtfile() {
  if (strchr(switch_char,'s')) {
    char *src = readtext("Source: ");
    char *dst = readtext("Destination: ");
    btslink(src,dst);
    return 0;
  }
  return copymove(2);
}

static int copymove(int move) {
  char *src = readtext("Source: ");
  char *dst = readtext("Destination: ");
  BTCB *srcf = 0, *dstf = 0, *dstff;
  int rc;
  dstf = btopen(dst,BTWRONLY+4+NOKEY,MAXREC);
  envelope
  if (dstf->flags) {			/* target exists */
    if (dstf->flags & BT_DIR) {		/* target is a directory */
      struct btff f;
      rc = findfirst(src,&f);		/* copy matching files */
      if (rc) {
	(void)fprintf(stderr,"%s: not found\n",src);
        btclose(dstf);
	return 0;
      }
      envelope
      for (;!cancel && rc == 0; btclose(srcf), srcf = 0, rc = findnext(&f)) {
	BTCB *savdir;
	/* dstf is "scratch" for the next few steps */
	memcpy(dstf->lbuf,f.b->lbuf,f.b->rlen);
	dstf->rlen = f.b->rlen;
	dstf->klen = strlen(dstf->lbuf) + 1;
	srcf = btopen(dstf->lbuf,BTRDONLY+4+LOCK,MAXREC);
	if (!srcf) {
	  fprintf(stderr,"%s: locked, not copied\n",dstf->lbuf);
	  continue;
	}
	if (move) {
	  dstf->u.cache.node = srcf->root;
	  dstf->op = (int)BTLINK;
	}
	else {
	  if (srcf->flags & BT_DIR) {
	    (void)fprintf(stderr,"%s: directory not copied\n",dstf->lbuf);
	    continue;
	  }
	  dstf->u.id.user = getuid();
	  dstf->u.id.group = getgid();
	  dstf->u.id.mode = getperm(0666);
	  dstf->op = (int)BTCREATE;
	}
	/* create either a link or new file */
	if (btas(dstf,dstf->op + DUPKEY)) {
	  if (move) {
	    /* FIXME: should we btkill(dstf->lbuf)? */
	    (void)fprintf(stderr,"%s: already exists\n",dstf->lbuf);
	    continue;
	  }
	}
	if (move) {	 /* link or move the file to a different directory */
	  if (move == 1) btkill(f.b->lbuf);
	  continue;
	}
	else {		/* copy the file to a different directory */
	  dstff = 0;
	  savdir = btasdir;
	envelope
	  btasdir = dstf;
	  dstff  = btopen(dstf->lbuf,BTWRONLY,MAXREC);
	  copy1(srcf,dstff);
	enverr
	  (void)fprintf(stderr,"errno = %d\n",errno);
	envend
	  btclose(dstff);
	  btasdir = savdir;
        }
      }
      enverr
	(void)fprintf(stderr,"errno = %d\n",errno);
      envend
      finddone(&f);
    }
    else if (move) {
      (void)fprintf(stderr,"%s: already exists\n",dstf->lbuf);
    }
    else {			/* copy to different structure */
      /* no explicit conversion now - this will still expand char fields! */
      srcf = btopen(src,BTRDONLY,MAXREC);
      copy1(srcf,dstf);
      btclose(srcf);
      srcf = 0;
    }
  }
  else {			/* copy 1 file to different name */
    srcf = btopen(src,BTRDONLY+4,MAXREC);
    dstf = btopendir(dst,BTWRONLY);
    srcf->klen = strlen(srcf->lbuf) + 1;
    if (srcf->rlen > srcf->klen)
      memcpy(dstf->lbuf + dstf->klen,
		 srcf->lbuf + srcf->klen,
		 srcf->rlen - srcf->klen);
    else
      srcf->rlen = srcf->klen;
    dstf->rlen = dstf->klen + srcf->rlen - srcf->klen;
    if (move) {
      dstf->u.cache.node = srcf->root;
      dstf->op = (int)BTLINK;
    }
    else {
      if (srcf->flags & BT_DIR)
	errpost(BTERDIR);
      dstf->u.id.user = getuid();
      dstf->u.id.group = getgid();
      dstf->u.id.mode = getperm(0666);
      dstf->op = (int)BTCREATE;
    }
    if (btas(dstf,dstf->op + DUPKEY)) {
      (void)fprintf(stderr,"%s: already exists\n",dstf->lbuf);
    }
    else if (move) {
      if (move == 1) btkill(src);
    }
    else {
      BTCB *savdir = btasdir;
      dstff = 0;
    envelope
      btasdir = dstf;
      dstff = btopen(dstf->lbuf,BTWRONLY,MAXREC);
      copy1(srcf,dstff);
    enverr
      (void)fprintf(stderr,"errno = %d\n",errno);
    envend
      btclose(dstff);
      btasdir = savdir;
    }
  }
  enverr
    switch (errno) {
    case BTERDIR:
      puts("Can't copy a directory");
      break;
    default:
      printf("BTAS/X errno %d\n",errno);
    }
  envend
  btclose(dstf);
  btclose(srcf);
  return 0;
}

static char *ftbldup(const char *buf,int rlen) {
  int slen = strlen(buf);
  int flen = rlen - slen;
  char *ftbl = malloc(flen);
  if (ftbl == 0) return 0;
  memcpy(ftbl,buf + slen + 1,flen - 1);
  ftbl[flen] = 0;
  return ftbl;
}

static void copy1(BTCB *srcf,BTCB *dstf) {
  int rc;
  char *srctbl = 0, *dsttbl = 0;
  char *buf = 0;
  int rlen, klen;
  char econvert = !!strchr(switch_char,'e');	/* ebcdic conversion flag */
  char aconvert = !!strchr(switch_char,'a');	/* ascii conversion flag */
  char replace = !!strchr(switch_char,'r');	/* replace records */
  long recs = 0L, dups = 0L;
  (void)fprintf(stderr,"%s: ",dstf->lbuf);
  (void)fflush(stderr);

  if (econvert) {
    srctbl = ftbldup(srcf->lbuf,srcf->rlen);
    rlen = bt_rlen(srcf->lbuf,srcf->rlen);
    dsttbl = ftbldup(dstf->lbuf,dstf->rlen);
    klen = bt_klen(dstf->lbuf);
    buf = malloc(rlen);
    if (!buf || !dsttbl || !srctbl) {
      puts("*** out of memory ***\n");
      cancel = 1;
    }
  }

  srcf->klen = 0;
  srcf->rlen = MAXREC;
  catch(rc)
  if (strchr(switch_char,'d'))	/* if copy directory only */
    rc = 212;		/* pretend file is empty */
  else
    rc = btas(srcf,BTREADGE+NOKEY);
  while (rc == 0 && !cancel) {
    if (econvert) {
      b2erec(srctbl,buf,rlen,srcf->lbuf,srcf->rlen);
      e2brec(dsttbl,buf,rlen,dstf,klen);
    }
    else {
      (void)memcpy(dstf->lbuf,srcf->lbuf,srcf->rlen);
      dstf->klen = dstf->rlen = srcf->rlen;
    }
    rc = btas(dstf,BTWRITE+DUPKEY);
    if (rc) {
      if (replace)
	(void)btas(dstf,BTREPLACE);
      ++dups;
    }
    ++recs;
    srcf->klen = srcf->rlen;
    srcf->rlen = MAXREC;
    rc = btas(srcf,BTREADGT+NOKEY);
  }
  rc = 0;
  envend
  if (srctbl) free(srctbl);
  if (dsttbl) free(dsttbl);
  if (buf) free(buf);
  (void)fprintf(stderr,"%ld records copied, %ld dups\n",recs,dups);
  if (rc) errpost(rc);
}
