/*
	btrmdir.c - Removes a btas directory with standard links.
*/
#include <string.h>
#include <errenv.h>
#include <btas.h>
#include <bterr.h>

int btrmdir(const char *path) {
  BTCB * volatile b = 0;
  BTCB * volatile c = 0;
  BTCB *s = btasdir;
  int rc,pass;

  catch(rc)
    b = btopendir(path,BTWRONLY);
    b->rlen = b->klen;
    btasdir = b;
    c = btopen(b->lbuf,BTRDWR+BTDIROK+BTEXCL,MAXREC+1);
    btasdir = c;
    /* remove self & parent links after checking for any other links */
    for (pass = 1; pass <= 2; ++pass) {
      c->rlen = MAXREC;
      c->klen = 0;
      rc = btas(c,BTREADGE+NOKEY);
      while (rc == 0) {
	struct btstat st;
	struct btlevel fa;
	c->lbuf[c->rlen] = 0;
	c->klen = strlen(c->lbuf);
	rc = btlstat(c,&st,&fa);
	if (rc) errpost(rc);
	if (fa.node == c->root && fa.slot == c->mid
	 || fa.node == b->root && fa.slot == b->mid) {
	  if (pass == 2)
	    btas(c,BTDELETE);
	}
	else
	  errpost(BTERDIR);
	c->klen = c->rlen;
	c->rlen = MAXREC;
	rc = btas(c,BTREADGT+NOKEY);
      }
    }
    btclose(c);
    c = 0;
    btas(b,BTDELETE);
    rc = 0;
  envend
  btasdir = s;
  (void)btclose(c);
  (void)btclose(b);
  return rc;
}
