/*
	erase files comprising an emulated C-isam file
	tack .idx and .n to the end in turn and make BTAS calls
*/

#define _GNU_SOURCE
#include <string.h>
#include <libgen.h>
#include <errenv.h>
#include <btas.h>
#include <bterr.h>
#include "cisam.h"

int isrename(const char *from, const char *to) {
  BTCB * volatile b = 0;
  int rc;
  static char idx[] = CTL_EXT;
  struct btflds * volatile f = 0;
  char buf[MAXKEY + 1], tobuf[MAXKEY + 1];

  /* first, rename .idx */
  catch(rc)
    strcpy(tobuf,to);
    strcat(tobuf,idx);
    strcpy(buf,from);
    strcat(buf,idx);
    if (rc = btrename(buf,tobuf))
      rc = btrename(from,to);	/* no .idx, just rename file */
    else {
      /* .idx is renamed, now try to rename keys */
      struct fisam frec;
      char *tname, *fname;
      char const *newname, *oldname;
      int len;
      b = btopen(tobuf,BTRDWR + BTEXCL,sizeof frec);
      f = ldflds(0,b->lbuf,b->rlen);
      if (!f) errpost(ENOMEM);
      strcpy(tobuf,to);
      tname = (char *)basename(tobuf);
      strcpy(buf,from);
      fname = (char *)basename(buf);
      oldname = basename((char *)from);
      newname = basename((char *)to);
      len = strlen(oldname);
      b->lbuf[0] = 0;
      b->klen = 1;
      b->rlen = sizeof frec;
      while (btas(b,BTREADGT + NOKEY) == 0) {
	/* leave absolute names alone */
	if (b->lbuf[0] != '/') {
	  b2urec(f->f,(PTR)&frec,sizeof frec,b->lbuf,b->rlen);
	  /* it is not clear how to rename keys, but I think the
	     following rules are appropriate:
	     a) rename into target directory
	     b) if a prefix of basename matches source basename,
		change prefix to match target */
	  ldchar(frec.name,sizeof frec.name,fname);
	  if (strncmp(fname,oldname,len))
	    strcpy(tname,fname);
	  else {
	    strcpy(tname,newname);
	    strcat(tname,fname + len);
	  }
	  if (!btrename(buf,tobuf)) {
	    stchar(tname,frec.name,sizeof frec.name);
	    u2brec(f->f,(PTR)&frec,sizeof frec,b,sizeof frec.name);
	    rc = btas(b,BTWRITE + DUPKEY);
	    stchar(fname,frec.name,sizeof frec.name);
	    u2brec(f->f,(PTR)&frec,sizeof frec,b,sizeof frec.name);
	    if (rc == 0)
	      btas(b,BTDELETE);
	  }
	}
	b->klen = b->rlen;
	b->rlen = sizeof frec;
      }
      rc = 0;
    }
  envend
  btclose(b);
  if (f)
    free((PTR)f);
  return ismaperr(rc);
}
