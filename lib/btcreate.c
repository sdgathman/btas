/*
	btcreate.c - Creates a btas file with standard field table.
*/

#include <stdlib.h>
#include <string.h>
#include <errenv.h>
#include <btflds.h>

#ifndef __MSDOS__
#include <unistd.h>
int getperm(int mask) {
  register int rc = umask(mask);
  return umask(rc) & ~rc;
}
#else
#define	getperm(mask)	mask
#endif

/* stflds - Store function for btflds.
*/
int stflds(const struct btflds *fld,char *buf) {
  register char *bf = buf;
  register int i;

  bf += strlen(bf) + 1;		/* skip over filename */
  if (fld) {
    *bf++ = fld->klen;
    for (i = 0; i < fld->rlen; i++) {
      *bf++ = fld->f[i].type;
      *bf++ = fld->f[i].len;
    }
  }
  return bf - buf;	/* return new record length */
}

/* btcreate - creates a file in the current directory
   Returns BTERDUP if file already exists, and 0 on success.
*/
int btcreate(const char *path,const struct btflds *fld,int mode) {
  BTCB * volatile b = 0;
  int rc;

  catch(rc)
    b = btopendir(path,BTWRONLY);
    /* b->lbuf[24] = 0;	/* enforce max filename length */
    /* copy fld to lbuf, compute rlen */
    b->rlen = stflds(fld,b->lbuf);
    b->u.id.user = geteuid();
    b->u.id.group = getegid();
    b->u.id.mode  = getperm(mode);
    rc = btas(b,BTCREATE+DUPKEY);
  envend
  (void)btclose(b);
  return rc;
}
