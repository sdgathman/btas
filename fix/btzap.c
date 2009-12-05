/*
	Q&D program to unlink bogus hardlinks
*/
#include <stdio.h>
#include <string.h>
#include <errenv.h>
#include <btas.h>

main(argc,argv)
  char **argv;
{
  BTCB *dirf = 0;
  char *p, *dir;
  if (argc != 2) {
    fputs("Usage:\tbtzap pathname\n",stderr);
    return 1;
  }
  if (p = strrchr(argv[1],'/')) {
    *p++ = 0;
    dir = argv[1];
  }
  else {
    p = argv[1];
    dir = "";
  }
  envelope
    dirf = btopen(dir,BTWRONLY+4,MAXREC);
    dirf->klen = strlen(p) + 1;
    dirf->rlen = dirf->klen;
    strncpy(dirf->lbuf,p,MAXREC);
    printf("unlink %s on %s\n",dirf->lbuf,dir);
    dirf->flags ^= BT_DIR;
    (void)btas(dirf,BTDELETE + NOKEY);
    dirf->flags ^= BT_DIR;
  enverr
    fprintf(stderr,"BTAS errno = %d\n",errno);
  envend
  btclose(dirf);
  return 0;
}
