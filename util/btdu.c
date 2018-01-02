#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <btas.h>
#include <port.h>

long totstk[256], *total;

volatile int cancel = 0;

void sig(s) {
  signal(s,sig);
  cancel = 1;
}

struct {
  int files:1;	/* show files */
  int sum:1;	/* show summary only */
  int mnts:1;	/* include mounted filesystems */
} opt = { 0,0,1 };

char dir[MAXKEY];

int show(const char *name,const struct btstat *st) {
  if (st) {
    if (st->id.mode & BT_DIR) {
      /* puts(""); */
      *++total = st->bcnt;
    }
    else {
      if (opt.files)
	printf("%-8ld %s/%s\n",st->bcnt,dir,name);
      *total += st->bcnt;
    }
  }
  else {
    /* printf("%8s %8s\n","--------","--------"); */
    if (!opt.sum)
      printf("%-8ld %s/%s\n",*total,dir,name);
    --total;
    *total += total[1];
  }
  return cancel;
}

static GOTO usage() NORETURN;

static GOTO usage() {
  fputs("\
Usage:	btdu [-asm] files $Revision$\n\
	-a	show files also\n\
	-s	show summary of listed files only\n\
	-m	stay within filesystem (not working)\n",
  stderr);
  exit(1);
}

static int error(const char *path,int code) {
  switch (code) {
  case 222: fprintf(stderr,"%s: permission denied\n",path); return 0;
  }
  return code;
}

main(int argc,char **argv) {
  int i;
  long gtot = 0L;
  if (argc < 2)
    usage();
  signal(SIGTERM,sig);
  signal(SIGINT,sig);
  signal(SIGHUP,sig);
  for (i = 1; i < argc; ++i) {
    struct btff ff;
    int rc;
    if (argv[i][0] == '-') {
      while (*++argv[i]) {
	switch (*argv[i]) {
	case 'a':
	  opt.files = 1;
	  break;
	case 's':
	  opt.sum = 1;
	  break;
	case 'm':
	  opt.mnts = 0;
	  break;
	default:
	  usage();
	}
      }
      continue;
    }
    rc = findfirst(argv[i],&ff);
    strcpy(dir,btgetcwd());
    if (strcmp(dir,"/") == 0)
      dir[0] = 0;
    while (rc == 0) {
      total = totstk;
      *total = 0L;
      switch (rc = btwalkx(ff.b->lbuf,show,error)) {
      case 0:
	if (opt.sum)
	  printf("%-8ld %s/%s\n",totstk[0],dir,ff.b->lbuf);
	break;
      case 1:
	finddone(&ff);
	fprintf(stderr,"Canceled\n");
	return 1;
      default:
	fprintf(stderr,"btwalk failed, rc = %d\n",rc);
      }
      gtot += totstk[0];
      rc = findnext(&ff);
    }
  }
  if (gtot > totstk[0])
    printf("%-8ld GRAND TOTAL\n",gtot);
  return 0;
}
