/* check errors encountered by cisam calls */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <isamx.h>
#include <errenv.h>
#include <ischkerr.h>

static char errtbl[] = {
  1,	/* duplicate record */
  0,0,0,0,0,0,	/* various consistency checks */
  2,	/* locked record */
  1,	/* duplicate index */
  0,	/* delete primary */
  1,	/* EOF */
  1,	/* NOKEY */
  4,	/* no current record */
  2	/* locked file */
};

int ischkerr(int mask,int lineno,const char *module) {
  register int err = iserrno - 100;
  char buf[80];
  if (err < 0 || err >= sizeof errtbl || (errtbl[err] & mask) == 0 ) {
    sprintf(buf,"%s line %d",module,lineno);
    errdesc(buf,iserrno);
  }
  return iserrno;
}
