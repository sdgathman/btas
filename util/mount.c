#include <stdio.h>
#include <string.h>
#include "btutil.h"
#include <btas.h>
#include <bttype.h>
#include <bterr.h>
#include <time.h>
#include <errenv.h>

static void stat1(int);

int mountfs() {
  BTCB t;
  char *s = readtext("Filesystem to mount: ");
  char *dir = readtext(0);
  int rc;
  int len = strlen(s);
  if (dir) {
    if (btopenf(&t,dir,BTWRONLY + 4,len+1)) {
      (void)puts("Can't open directory for update.");
      return 0;
    }
  }
  else {
    t.root = 0;
    t.mid = 0;
    t.flags = BT_UPDATE;
  }
  (void)strcpy(t.lbuf,s);
  t.klen = len;
  t.rlen = len+1;
  catch(rc)
    (void)btas(&t,BTMOUNT);	/* mount filesystem */
    if (dir)
      printf("%s mounted on %s\n",s,dir);
    else
      printf("%s mounted on %c:\n",s,t.mid + 'A');
  enverr
    switch (rc) {
    case BTERBMNT:
      printf("%s: possibly damaged filesystem\n",s);
      break;
    case BTERMBLK:
      printf("%s: block size too big\n",s);
      break;
    default:
      (void)printf("Mount failed, rc = %d\n",rc);
    }
  envend
  if (t.root)
    btas(&t,BTCLOSE);
  return 0;
}

int unmountfs() {
  BTCB t;
  char *s = readtext("Directory to unmount: ");
  int rc;
  if (strlen(s) == 2 && s[1] == ':') {
    t.root = 0;
    t.mid = s[0] - 'A';
    t.flags = 0;
    t.klen = t.rlen = 0;
    rc = btas(&t,BTOPEN + NOKEY + BT_UPDATE);
  }
  else
    rc = btopenf(&t,s,BTWRONLY + BTDIROK + NOKEY,0);
  if (rc) {
    puts("Can't open directory.");
    return 0;
  }
  envelope
    btas(&t,BTUMOUNT);
    puts("Unmounted");
  enverr
    puts("Unmount failed");
  envend
  btas(&t,BTCLOSE);
  return 0;
}

static void stat1(int drive) {
  BTCB t;
  struct btfs f;
  register int i;
  t.root = 0;
  t.flags = 0;
  t.mid = drive;
  t.rlen = MAXREC;
  t.klen = 0;
  envelope
    (void)btas(&t,BTSTAT);	/* try to stat drive */
    if (t.rlen > sizeof f) t.rlen = sizeof f;
    (void)memcpy((char *)&f,t.lbuf,t.rlen);
    (void)printf("%c: %8ld %.16s %6d %6d %s\n", t.mid + 'A',
	f.hdr.space, ctime(&f.hdr.mount_time),
	f.hdr.blksize, f.hdr.mcnt, f.hdr.flag ? "Dirty" : "Clean");
    for (i = 0; i < f.hdr.dcnt; ++i) {
      struct btdev *d = f.dtbl + i;
      printf("\t%.16s %ld",d->name,d->eod);
      if (d->eof) printf(" of %ld",d->eof);
      puts(" used");
    }
  envend
}

int fsstat() {		/* report status of mounted file systems */
  int d;
  char *s = readtext((char *)0);
  (void)printf("   %8s %16s %6s %6s %s\n",
	"Free","Mounted","Blksiz","Mounts","Status");
  if (s) {
    stat1(*s - 'A');
    return 0;
  }
  for (d = 0; d < MAXDEV; ++d)
    stat1(d);
  return 0;
}

int pstat() {		/* report performance statistics */
  struct btpstat stat;
  BTCB t;
  t.klen = 0;
  t.root = 0L;
  t.rlen = sizeof stat;
  btas(&t,BTPSTAT);
  memcpy(&stat,t.lbuf,sizeof stat);
  printf("Buffers = %d, hashsize = %d\n",
	stat.bufs,stat.hsize);
  printf("Hash:\tsearches = %ld, probes = %ld\n",
	stat.searches,stat.probes);
  printf("Cache:\tlreads = %ld, preads = %ld, pwrites = %ld\n",
	stat.lreads,stat.preads,stat.pwrites);
  printf("Flush:\tfbufs  = %ld, fcnt   = %ld", stat.fbufs, stat.fcnt);
  if (stat.fcnt > 0)
    printf(", average = %ld", stat.fbufs/stat.fcnt);
  printf("\nMessages = %ld, Sum(IO^2) = %ld\n",stat.trans,stat.sum2);
  return 0;
}
