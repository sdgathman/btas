/* $Log$
  Allocate & open a BTAS/2 file with support for "current" directory.  
  The "current" directory is passed to programs through the environment.
*/

#include <string.h>
#include <stdlib.h>
#include <errenv.h>

#ifdef __MSDOS__
#define	geteuid()	0
#define	getegid()	0
#else
#ifndef lint
static char what[] = "%W%";
#endif
#endif

#include <btas.h>
#include <bterr.h>

static short perms[16] = {
	04, 02, 06, 00,
	BT_DIR+04, BT_DIR+02, BT_DIR+06, BT_DIR+00,
	05, 03, 07, 01,
	BT_DIR+05, BT_DIR+03, BT_DIR+07, BT_DIR+01,
};

BTCB *btasdir = 0;			/* current directory */

static char btdirname[] = "BTASDIR";	/* environment name */

static void cdecl closedir(void) {
  (void)btclose(btasdir);
}

BTCB *btopen(const char *name,int mode,int rlen) {
  BTCB *b, bt;
  (void)btopenf(&bt,name,mode,sizeof bt.lbuf);
  if (rlen < bt.rlen) rlen = bt.rlen;	/* keep directory record */
  rlen += sizeof bt - sizeof bt.lbuf;
  b = (BTCB *)malloc(rlen);
  if (!b) {
    btas(&bt,BTCLOSE);
    errpost(ENOMEM);
  }
  memcpy((PTR)b,(PTR)&bt,bt.rlen + sizeof bt - sizeof bt.lbuf);
  return b;
}

int btopenf(BTCB *b,const char *name,int mode,int rlen) {
  int rc,i;
  char *dirname;
  if (name[0] != '/' && btasdir == 0) {
    dirname = getenv(btdirname);
    if (dirname && dirname[0] == '/' && dirname[1]) {
      int dlen;
      BTCB *dir;
      dlen = strlen(dirname) + sizeof btdirname + 1;
      dir = btopen(dirname,BTNONE+BTDIROK+8,dlen);
      (void)strcpy(dir->lbuf,btdirname);
      (void)strcat(dir->lbuf,"=");
      (void)strcat(dir->lbuf,dirname);
      dir->rlen = dlen;
      btputdir(dir);
    }
  }
  b->flags = 0;
  b->root = 0L;
  b->mid = 0;
  dirname = 0;
  if (*name == '/') ++name;
  else if (btasdir) {
    if (btasdir->flags) {
      b->root = btasdir->root;
      b->mid  = btasdir->mid;
    }
    else
      dirname = btasdir->lbuf + sizeof btdirname;
  }
  /* open file following symbolic links */
  for (i = 0; i < 10; ++i) {
    int len;
    int newpath_offset;
    b->u.id.user = geteuid();
    b->u.id.group = getegid();
    b->u.id.mode = perms[mode&15];	/* desired permissions */
    /* FIXME: check for overflow constructing path */
    len = dirname ? strlen(dirname) : 0;
    if (len + strlen(name) + 2 > rlen)
      errdesc(name,BTERKLEN);		/* buffer too small */
    if (dirname) {
      strcpy(b->lbuf,dirname);
      b->lbuf[len++] = '/';
    }
    strcpy(b->lbuf + len,name);
    b->klen = len = strlen(b->lbuf);
    /* set path seperator to null for kernel */
    for (dirname = b->lbuf; dirname = strchr(dirname,'/'); *dirname++ = 0);
    b->rlen = rlen;
    catch(rc)
      rc = btas(b,BTOPEN+(mode&(BTERR|BTEXCL)));
    enverr
      if (rc != BTERLINK)
	errdesc(name,rc);	/* add filename to error log */
    envend
    if (rc != BTERLINK) break;
    newpath_offset = strlen(b->lbuf) + 1;
    b->klen -= newpath_offset;
    name += strlen(name) - b->klen;
    /* shift to beginning of buffer to ensure room for terminator */
    memcpy(b->lbuf,b->lbuf + newpath_offset,b->rlen -= newpath_offset);
    b->lbuf[b->rlen] = 0;	/* null terminate */
    dirname = b->lbuf;
    //printf("newpath: %s/%s\n",b->lbuf,name);
    if (*dirname == '/') {
      b->mid = 0;	/* begin again at root directory */
      b->root = 0;
      ++dirname;
    }
  }
  return rc;
}

int btclose(BTCB *b) {
  int rc = 0;
  if (b) {
    b->rlen = b->klen = 0;
    rc = btas(b,BTCLOSE);
    free((char *)b);
  }
  return rc;
}

void btputdir(BTCB *b) {
  static char didatexit;
  if (b)
    putenv(b->lbuf);
  else
    putenv("BTASDIR=/");
  if (btasdir) {
    if (b && btasdir->flags == 0)	/* if atexit failed */
      (void)btas(b,BTCLOSE);
    (void)btclose(btasdir);
  }
  else if (!didatexit && b) {
    if (atexit(closedir))
      (void)btas(b,BTCLOSE);	/* atexit failed */
    else
      didatexit = 1;
  }
  btasdir = b;
}

#if 0
int main(int argc,char **argv) {
  int i;
  for (i = 1; i < argc; ++i) {
    BTCB *b = btopen(argv[i],BTRDONLY,512);
    btclose(b);
  }
}
#endif
