#include <btas.h>
#include <string.h>
#include <stdlib.h>

int btchdir(const char *name) {
  BTCB *b;
  if (!name || !*name) return 0;	/* no change */
  if (strcmp(name,"/") == 0)
    b = 0;				/* root dir */
  else {
    int len = strlen(name) + 1;
    if (name[0] != '/')
      len += strlen(btgetcwd());
    len += 9;		/* room for "BTASDIR=" */
    b = btopen(name,BTERR+BTNONE+BTDIROK+8,len);
    if (b->flags == 0) {
      btclose(b);
      return -1;
    }
    b->lbuf[0] = 0;
    if (name[0] != '/' && btasdir) {
      strcat(b->lbuf,btasdir->lbuf);
      strcat(b->lbuf,"/");
    }
    else {
      strcat(b->lbuf,"BTASDIR=/");
      if (*name == '/') ++name;
    }
    strcat(b->lbuf,name);
    btpathopt(b,b->lbuf + 8);
    b->rlen = strlen(b->lbuf);
  }
  btputdir(b);
  return 0;
}

BTCB *btgetdir() {
  BTCB *b;
  if (!btasdir) return 0;
  b = btopen("",BTNONE+BTDIROK+8,btasdir->rlen);/* open current directory */
  strcpy(b->lbuf,btasdir->lbuf); /* copy directory environment */
  b->rlen = btasdir->rlen;
  return b;
}

char *btgetcwd() {
  char *p;
  if (btasdir) return strchr(btasdir->lbuf,'/');
  p = getenv("BTASDIR");
  if (!p || *p != '/') return "/";
  return p;
}
