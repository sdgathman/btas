/*
	Under SysV, BTAS can't close files when processes end because
it doesn't know when they end.  This routine, therefore, unlocks and
deletes the temp files if they already exist.  This is not strictly
kosher and will go away on real OS's.
*/

#include <stdlib.h>
#include <string.h>
#include <btas.h>
#include <btflds.h>
#include <bterr.h>
#include <unistd.h>

BTCB *btopentmp(prefix,flds,name)
  const char *prefix;		/* make temp name mean something */
  const struct btflds *flds;
  char *name;			/* buffer to store file name */
{
  static short ctr;
  char buf[18];
  BTCB *b;
  (void)strcpy(name,"/tmp/");
  (void)strcat(name,prefix);
  (void)strcat(name,ltoa(((long)getpid()*1000L)+ctr++,buf,10));
  b = btopen(name,BTRDONLY + BTEXCL + LOCK + NOKEY,strlen(name));
  if (b->op == BTERLOCK) b->flags = BTEXCL;
  btclose(b);		/* unlock if already open */
  btkill(name);		/* delete if already there */
  if (btcreate(name,flds,0644)) return 0;
  b = btopen(name,BTRDWR,btrlen(flds));
  return b;
}

void btclosetmp(b,name)
  BTCB *b;
  const char *name;	/* name of file when created */
{
  btclose(b);
  (void)btkill(name);
}
