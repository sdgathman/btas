#include <stdio.h>
#include <memory.h>
#include <btflds.h>

main(int argc,char **argv) {
  int i;
  for (i = 1; i < argc; ++i) {
    BTCB *b = btopen(argv[i],BTRDWR + NOKEY,MAXREC);
    if (b->flags) {
      struct btflds f;
      int rc, klen;
      long dups = 0L;
      long recs = 0L;
      char lbuf[MAXKEY];
      ldflds(&f,b->lbuf,b->rlen);
      klen = f.f[f.klen].pos;
      b->klen = 0;
      b->rlen = MAXREC;
      rc = btas(b,BTREADGE + NOKEY);
      if (rc == 0) {
	++recs;
	b2urec(f.f,lbuf,klen,b->lbuf,b->rlen);
	for (;;) {
	  char urec[MAXKEY];
	  b->klen = b->rlen;
	  b->rlen = MAXREC;
	  if (btas(b,BTREADGT + NOKEY)) break;
	  b2urec(f.f,urec,klen,b->lbuf,b->rlen);
	  if (memcmp(urec,lbuf,klen) == 0) {
	    b->klen = b->rlen;
	    btas(b,BTDELETE);
	    ++dups;
	  }
	  else {
	    memcpy(lbuf,urec,klen);
	    ++recs;
	  }
	}
	fprintf(stderr,"%8ld records kept in %s\n",recs,argv[i]);
	fprintf(stderr,"%8ld dups deleted in %s\n",dups,argv[i]);
      }
    }
    btclose(b);
  }
}
