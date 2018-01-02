/*
    This file is part of the BTAS client library.

    The BTAS client library is free software: you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public License as
    published by the Free Software Foundation, either version 3 of the License,
    or (at your option) any later version.

    BTAS is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with BTAS.  If not, see <http://www.gnu.org/licenses/>.
 *
 * $Log$
 * Revision 1.4  2009/03/31 17:29:02  stuart
 * Never throw errenv exception from btclose.
 *
 * Revision 1.3  2001/02/28 22:31:11  stuart
 * debugging for following symlinks
 *
 * Revision 1.2  1999/01/22  23:50:52  stuart
 * Fix symlinks that are not the last part of the path.
 *
 */

#include <string.h>
#include <stdlib.h>
#include <errenv.h>
#include <unistd.h>

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

/** Current directory BTCB.  The path name of the directory is saved
 * in the buffer of the BTCB.
 */
BTCB *btasdir = 0;

static char btdirname[] = "BTASDIR";	/* environment name */

static void cdecl closedir(void) {
  (void)btclose(btasdir);
}

/** Allocate & open a BTAS/2 file with support for "current" directory.  
  The "current" directory is passed to programs through the environment,
  and is available within a process through the <code>btasdir</code> 
  global pointer to a BTCB.
 */
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

/** Open a BTAS file in an existing BTCB with support for "current" directory.  
  The "current" directory is passed to programs through the environment,
  and is available within a process through the <code>btasdir</code> 
  global pointer to a BTCB.
 */
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
    if (rc != BTERLINK) {
      break;
    }
    newpath_offset = strlen(b->lbuf) + 1;
    /* shift to beginning of buffer to ensure room for terminator */
    memcpy(b->lbuf,b->lbuf + newpath_offset,b->rlen -= newpath_offset);
    b->lbuf[b->rlen] = 0;	/* null terminate */
    if (b->klen > newpath_offset) {
      b->klen -= newpath_offset;
      name += strlen(name) - b->klen;
      dirname = b->lbuf;
#ifdef DEBUG_MAIN
      printf("newpath: %s/%s\n",b->lbuf,name);
#endif
      if (*dirname == '/') {
	b->mid = 0;	/* begin again at root directory */
	b->root = 0;
	++dirname;
      }
    }
    else {
      name = b->lbuf;
      dirname = 0;
#ifdef DEBUG_MAIN
      printf("newpath: %s\n",name);
#endif
      if (*name == '/') {
	b->mid = 0;	/* begin again at root directory */
	b->root = 0;
	++name;
      }
    }
  }
  return rc;
}

int btclose(BTCB *b) {
  int rc = 0;
  if (b) {
    b->rlen = b->klen = 0;
    catch(rc)
      rc = btas(b,BTCLOSE);
    envend
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

#ifdef DEBUG_MAIN
int main(int argc,char **argv) {
  int i;
  for (i = 1; i < argc; ++i) {
    BTCB *b = 0;
    envelope
      b = btopen(argv[i],BTRDONLY,512);
    enverr
      printf("Error %d opening %s\n",errno,argv[i]);
    envend
    btclose(b);
  }
}
#endif
