#include <stdio.h>
#include <string.h>
#include "btutil.h"
#include <btas.h>
#include <bttype.h>
#include <bterr.h>
#include <time.h>
#include <errenv.h>
#include <math.h>

static void stat1(int);

int mountfs() {
  BTCB t;
  char *s = readtext("Filesystem to mount: ");
  char *dir = readtext(0);
  int rc;
  int len = strlen(s);
  if (dir) {
    if (btopenf(&t,dir,BTWRONLY + 4,sizeof t.lbuf)) {
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
    t.u.id.user = geteuid();
    t.u.id.group = getegid();
    t.u.id.mode = BT_DIR + BT_UPDATE;
    rc = btas(&t,BTOPEN + NOKEY);
  }
  else
    rc = btopenf(&t,s,BTWRONLY + BTDIROK + NOKEY,sizeof t.lbuf);
  if (rc) {
    puts("Can't open directory.");
    return 0;
  }
  catch(rc)
    btas(&t,BTUMOUNT);
    puts("Unmounted");
  enverr
    printf("Unmount failed: %d\n",rc);
  envend
  btas(&t,BTCLOSE);
  return 0;
}

static void stat1(int drive) {
  struct btfs f;
  register int i;
  if (btfsstat(drive,&f) == 0) {
    printf("%c: %8ld %.16s %6d %6d %s\n", drive + 'A',
	f.hdr.space, ctime(&f.hdr.mount_time),
	f.hdr.blksize, f.hdr.mcnt,
	(f.hdr.flag == 0xFF) ? "Dirty" : (f.hdr.flag ? "ChkPnt" : "Clean")
    );
    for (i = 0; i < f.hdr.dcnt; ++i) {
      struct btdev *d = f.dtbl + i;
      printf("\t%.16s %ld",d->name,d->eod);
      if (d->eof) printf(" of %ld",d->eof);
      puts(" used");
    }
  }
}

int fsstat() {		/* report status of mounted file systems */
  int d;
  char *s = readtext((char *)0);
  (void)printf("   %8s %16s %6s %6s %s\n",
	"Free","Mounted","Blksiz","Opens","Status");
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
  {
    double iocnt = stat.preads + stat.pwrites;
    double avg = iocnt / stat.trans;
    double avg2 = (double)stat.sum2/stat.trans;
    double stddev = sqrt(avg2 - (avg * avg));
    printf("\nMessages = %ld, Avg = %f, StdDev = %f, StdDev/Avg = %f\n",
	  stat.trans,avg,stddev,stddev/avg);
    printf("Update bytes = %ld\n",stat.lwriteb);
  }
  if (stat.uptime) {
    long uptime = time(0) - stat.uptime;
    long hours = uptime / 3600;
    int mins = uptime % 3600;
    int secs = mins % 60;
    mins /= 60;
    printf("Uptime = %ld hour%s, %d minute%s, %d second%s",
      hours, "s" + (hours == 1),
      mins, "s" + (mins == 1),
      secs, "s" + (secs == 1)
    );
    if (stat.checkpoints)
      printf(", Checkpoints = %ld",stat.checkpoints);
    puts("");
    if (stat.version) {
      printf("Dirty buffers = %d, Server Version = %ld.%02ld\n",
	stat.dirty,stat.version/100,stat.version%100);
    }
  }
  return 0;
}
