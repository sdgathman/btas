/*
	btmkdir.c - Creates a btas directory with standard links.
*/

#include <string.h>
#include <stdlib.h>
#include <errenv.h>
#include <btas.h>

/* btmkdir - creates a new directory
   Returns BTERDUP if file already exists, and 0 on success.
   Returns BTERKEY if a parent directory doesn't exist.
*/
int btmkdir(const char *path,int mode) {
  BTCB * volatile b = 0;
  BTCB * volatile c = 0;
  BTCB *s = btasdir;
  int rc;

  catch(rc)
    b = btopendir(path,BTWRONLY+BTDIROK);
    /* b->lbuf[24] = 0;	/* enforce max filename length */
    /* copy fld to lbuf, compute rlen */
    b->rlen = b->klen;
    b->u.id.user = geteuid();
    b->u.id.group = getegid();
    b->u.id.mode  = getperm(mode) | BT_DIR;
    btas(b,BTCREATE);
    btasdir = b;
    c = btopen(b->lbuf,BTWRONLY+BTDIROK,3);
    strcpy(c->lbuf,".");
    c->rlen = c->klen = 2;
    c->u.cache.node = c->root;
    btas(c,BTLINK);
    strcpy(c->lbuf,"..");
    c->rlen = c->klen = 3;
    c->u.cache.node = b->root;
    btas(c,BTLINK);
  envend
  btasdir = s;
  btclose(c);
  btclose(b);
  return rc;
}
