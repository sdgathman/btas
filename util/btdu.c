#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <btas.h>

long totstk[256];
long *total = totstk;

volatile int cancel = 0;

void sig(s) {
  signal(s,sig);
  cancel = 1;
}

char dir[MAXKEY];

int show(const char *name,const struct btstat *st) {
  if (st) {
    if (st->id.mode & BT_DIR) {
      /* puts(""); */
      *++total = st->bcnt;
    }
    else {
      printf("%-8ld %s/%s\n",st->bcnt,dir,name);
      *total += st->bcnt;
    }
  }
  else {
    /* printf("%8s %8s\n","--------","--------"); */
    printf("%-8ld %s/%s\n",*total--,dir,name);
    *total += total[1];
  }
  return cancel;
}

main(int argc,char **argv) {
  int i;
  long gtot = 0L;
  signal(SIGTERM,sig);
  signal(SIGINT,sig);
  signal(SIGHUP,sig);
  for (i = 1; i < argc; ++i) {
    struct btff ff;
    int rc = findfirst(argv[i],&ff);
    strcpy(dir,btgetcwd());
    if (strcmp(dir,"/") == 0)
      dir[0] = 0;
    *total = 0L;
    while (rc == 0) {
      switch (rc = btwalk(ff.b->lbuf,show)) {
      case 0:
	break;
      case 1:
	finddone(&ff);
	fprintf(stderr,"Canceled\n");
	return 1;
      default:
	fprintf(stderr,"btwalk failed, rc = %d\n",rc);
      }
      rc = findnext(&ff);
    }
    gtot += *total;
  }
  if (gtot > *total)
    printf("%-8ld GRAND TOTAL\n",gtot);
  return 0;
}
