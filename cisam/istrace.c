#include <stdio.h>
#include <sys/times.h>
#include <fcntl.h>
#include <isamx.h>
#include "isreq.h"

#ifndef O_BINARY
#define O_BINARY 0
#endif

static int readFully(int fd,char *buf,int len) {
  int cnt = 0;
  while (cnt < len) {
    int n = read(fd,buf + cnt,len - cnt);
    if (n < 0) {
      perror("read");
      return -1;
    }
    if (n == 0) {
      //fprintf(stderr,"EOF on input\n");
      return 0;
    }
    cnt += n;
  }
  return cnt;
}

static void hexDump(const char *buf,int len) {
  int i;
  for (i = 0; i < len; i += 16) {
    int j;
    printf(" ");
    for (j = 0; j < 16 && i + j < len - 1; j += 2)
      printf(" %02X%02X",buf[i + j] & 255, buf[i + j + 1] & 255);
    if (j < 16 && i + j < len) {
      printf(" %02X  ",buf[i + j] & 255);
      j += 2;
    }
    while (j < 16) {
      printf("     ");
      j += 2;
    }
    printf("|");
    for (j = 0; j < 16 && i + j < len; ++j) {
      int c = buf[i + j] & 255;
      printf("%c",(c < ' ' || c == 127) ? '.' : c);
    }
    puts("|");
  }
}

static int fxn;

static const char * const fxtbl[] = {
  "ISBUILD",
  "ISOPEN",
  "ISCLOSE",
  "ISADDINDEX",
  "ISDELINDEX",
  "ISSTART",
  "ISREAD",
  "ISWRITE",
  "ISREWRITE",
  "ISWRCURR",
  "ISREWCURR",
  "ISDELETE",
  "ISDELCURR",
  "ISUNIQUEID",
  "ISINDEXINFO",
  "ISAUDIT",
  "ISERASE",
  "ISLOCKOP",
  "ISUNLOCK",
  "ISRELEASE",
  "ISRENAME",
  "ISREWREC",
  "ISSTARTN",
  "ISDELREC",
  "ISSETRANGE",
  "ISADDFLDS",
  "ISGETFLDS",
  "ISREADREC",
  "ISINDEXNAME"
};

static const char *fxname(int fxn) {
  if (fxn < 0 || fxn >= sizeof fxtbl / sizeof fxtbl[0]) {
    static char buf[16];
    sprintf(buf,"%02X",fxn & 255);
    return buf;
  }
  return fxtbl[fxn];
}

static const char * const rmtbl[] = {
  "FIRST",	/* position to first record	*/
  "LAST",	/* position to last record	*/
  "NEXT",	/* position to next record	*/
  "PREV",	/* position to previous record	*/
  "CURR",	/* position to current record	*/
  "EQUAL",	/* position to equal value	*/
  "GREAT",	/* position to greater value	*/
  "GTEQ",	/* position to >= value		*/
  "LESS",	/* position to < value		*/
  "LTEQ"	/* position to >= value 	*/
};

static const char *modename(int m) {
  static char buf[16];
  if (fxn == ISREAD && m >= 0 && m < sizeof rmtbl / sizeof rmtbl[0]) {
    return rmtbl[m];
  }
  sprintf(buf,"%4X",m & 0xFFFF);
  return buf;
}

static clock_t startclock;
static clock_t elapsed;
static clock_t user;
static clock_t system;

/** Trace timestamp info at start of request and result. */
static void traceTS(int fd) {
  char tbuf[12];
  readFully(fd,tbuf,sizeof tbuf);
  elapsed = ldlong(tbuf);
  user = ldlong(tbuf+4);
  system = ldlong(tbuf+8);
  if (startclock == 0)
    startclock = elapsed;
}

static int traceReq(int fd) {
  struct ISREQ req;
  int p1,p2;
  traceTS(fd);
  if (readFully(fd,(char *)&req,sizeof req) <= 0) return -1;
  p1 = ldshort(req.p1);
  p2 = ldshort(req.p2);
  fxn = req.fxn;
  printf("<%5ld %3d %5s %3d %3d fd=%d %s\n",
    ldlong(req.recnum),ldshort(req.len),modename(ldshort(req.mode)),
    p1,p2,req.fd, fxname(fxn));
  if (p1) {
    char *buf = alloca(p1 + 1);
    if (readFully(fd,buf,p1) <= 0) return -1;
    switch (req.fxn) {
    case ISBUILD: case ISOPEN: case ISRENAME:
      buf[p1] = 0;
      printf("  %s\n",buf);
      break;
    default:
      hexDump(buf,p1);
    }
  }
  if (p2) {
    char *buf = alloca(p2 + 1);
    if (readFully(fd,buf,p2) <= 0) return -1;
    switch (req.fxn) {
    case ISRENAME: case ISSTARTN:
      buf[p2] = 0;
      printf("  %s\n",buf);
      break;
    default:
      hexDump(buf,p2);
    }
  }
  return 0;
}

static int traceRes(int fd) {
  struct ISRES res;
  int p1, rc;
  clock_t rwall = elapsed;
  clock_t ruser = user;
  clock_t rsys = system;
  traceTS(fd);
  printf("Wall: %ld Real: %ld User: %ld Sys: %ld\n",
    rwall - startclock, elapsed - rwall, user - ruser, system - rsys);
  if (readFully(fd,(char *)&res,sizeof res) <= 0) return -1;
  p1 = ldshort(res.p1);
  rc = ldshort(res.res);
  printf(">%5ld %3d %3d %3d %3d %3d\n",
    ldlong(res.recnum),rc,ldshort(res.iserrno),p1,res.isstat1,res.isstat2);
  if (p1) {
    char *buf = alloca(p1);
    if (readFully(fd,buf,p1) <= 0) return -1;
    hexDump(buf,p1);
    if (fxn == ISREAD && rc-- > 0) {
      while (rc-- > 0) {
	readFully(fd,buf,p1);
	hexDump(buf,p1);
      }
    }
  }
  return 0;
}

static void traceFd(int fd) {
  for (;;) {
    if (traceReq(fd)) break;
    if (traceRes(fd)) break;
  }
}

int main(int argc,char **argv) {
  int i;
  if (argc < 2) {
    traceFd(0);
    return 0;
  }
  for (i = 1; i < argc; ++i) {
    int fd = open(argv[i],O_RDONLY + O_BINARY);
    if (fd < 0)
      perror(argv[i]);
    else {
      traceFd(fd);
      close(fd);
    }
  }
  return 0;
}
