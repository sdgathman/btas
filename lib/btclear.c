/*
	btclear.c - Clear a btas file by pathname
*/

#include <string.h>
#include <errenv.h>
#include <btas.h>

int btclear(path)
  const char *path;		/* name of file to clear */
{
  BTCB * volatile b = 0;
  struct btstat st;
  int rc;

  /* FIXME: should use btlstat on later btservers that support it */
  btstat(path,&st);	/* get perm */
  catch(rc)
    int rlen;
    b = btopendir(path,BTRDWR);
    b->rlen = MAXREC;
    btas(b,BTREADEQ);		/* read filename & field table */
    rlen = b->rlen;		/* save rlen to work around btserve bug */
    btas(b,BTDELETE+NOKEY);	/* delete filename */
    b->u.id = st.id;		/* restore perm */
    b->rlen = rlen;
    btas(b,BTCREATE);		/* recreate filename */
    rc = 0;
  envend
  (void)btclose(b);
  return rc;
}
