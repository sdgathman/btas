/*
	btkill.c - Deletes a btas file by pathname
*/

#include <string.h>
#include <errenv.h>
#include <btflds.h>

int btkill(path)
  const char *path;		/* name of file to delete */
{
  BTCB * volatile b = 0;
  int rc;

  catch(rc)
    b = btopendir(path,BTWRONLY);
    b->rlen = b->klen;		/* filename is key */
    rc = btas(b,BTDELETE+NOKEY);
  envend
  (void)btclose(b);
  return rc;
}
