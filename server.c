#define TRACE 1
/*#define DETERMINISTIC	/* define to test for OS bugs */
/*
	Server program to execute BTAS/2 requests
	Single thread execution for now.
 * $Log$
 * Revision 1.13  2005/01/21 23:25:51  stuart
 * Fix error reporting on startup and when hitting ulimit.
 *
 * Revision 1.12  2001/12/13 16:37:24  stuart
 * Track total update bytes in server stats
 *
 * Revision 1.11  2001/02/28 21:46:59  stuart
 * local nap() prototype
 *
 * Revision 1.10  2000/04/27 12:42:36  stuart
 * more time stamps in log
 * make BTERDIR trappable with 0xF00
 *
 * Revision 1.9  1999/05/17  21:41:07  stuart
 * Support startup script
 * don't log symlink notifications
 *
 * Revision 1.8  1997/06/23  15:30:35  stuart
 * use btserve object
 *
 * Revision 1.7  1996/09/12  22:17:16  stuart
 * improved auto-flush logic
 *
 * Revision 1.6  1995/08/18  20:15:53  stuart
 * a few prototypes, terminate on SIGDANGER
 *
 * Revision 1.5  1995/05/31  20:57:22  stuart
 * error reporting bug
 *
 * Revision 1.3  1994/03/28  20:14:55  stuart
 * improve error logging
 *
 * Revision 1.2  1993/05/14  16:21:08  stuart
 * return key information on NOKEY so that symlink works
 *
 * Flush buffer on timer - SDG 03-19-89
 */

#include <stdlib.h>
extern "C" {
#include <btas.h>
}
#include <stdio.h>
#include <ctype.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <errno.h>
#ifdef _AIX
#include <sys/lock.h>
#endif
#include "btserve.h"
#include "btbuf.h"
#include "alarm.h"

#ifdef DETERMINISTIC	/* make certain problems reproducible for debugging */
time_t time(time_t *p) {
  if (p)
    *p = 0x7FFFFFFFL;		/* the clock is frozen */
  return 0x7FFFFFFFL;
}
#else
extern time_t time(time_t *);
#endif

extern const char version[];
extern "C" int nap(long);

static unsigned char btflags[32] = {
  0, 0, 8, 0, 0, 0, 0,0, 0, 0, 8,
  DUPKEY>>8, NOKEY>>8, NOKEY>>8, LOCK>>8, 8, 0, 0, 8,0,0,0,0,8, NOKEY>>8
};

static int btasreq, btasres;

static void cleanq(int,int);

/* remove result messages whose receiver has died 
*/

static inline bool validpid(int pid) {
  return kill(pid,0) == 0 || errno != ESRCH;
}

static void cleanq(int qid,int size) {
  struct {
    long mtype;
    char mtext[1024];
  } msg;
  struct msqid_ds d;
  /* throw away stale messages until there is room */
  do {
    int rc = msgrcv(qid,&msg,sizeof msg.mtext,0L,IPC_NOWAIT+MSG_NOERROR);
    if (rc == -1) break;	/* all gone now . . . */
    if (validpid(msg.mtype)) {	/* put it back if task still around */
      // FIXME: this can fail on SCO
      msgsnd(qid,&msg,rc,IPC_NOWAIT);
      nap(50L);			/* let applications run awhile */
    }
    else
      fprintf(stderr,"Removed dead message for pid %ld\n",msg.mtype);
    if (msgctl(qid,IPC_STAT,&d) == -1) break;
  } while (d.msg_cbytes + size > d.msg_qbytes);
}

char debug = 0;

int main(int argc,char **argv) {
  int rc;
  BTCB *reqp;
  struct msqid_ds buf;
  unsigned cache = 50000U;	/* works on 16 bit systems */
  unsigned block = BLKSIZE;	/* good for most systems */
  int reqsiz,i;			/* max size of request block */
  int flushtime = 30;
  bool safe_eof = true;
  const char *startup = 0;

  time(&btserve::curtime);
  fprintf(stderr,"%s%s",ctime(&btserve::curtime),version);
  if (argc < 2) goto syntax;
  for (i = 1; i < argc && *argv[i] == '-'; ++i) {
    switch (argv[i][1]) {
    case 0: break;	/* terminate parameters */
    case 'c':		/* set cache size */
      if (argv[i][2])
	cache = (unsigned)atol(argv[i]+2);
      if (++i < argc)
	cache = (unsigned)atol(argv[i]);
      continue;
    case 'b':		/* set block size */
      if (argv[i][2])
	block = (unsigned)atol(argv[i]+2);
      else if (++i < argc)
	block = (unsigned)atol(argv[i]);
      continue;
    case 'd':		/* debugging mode */
      debug = 1;
      continue;
    case 'f':		/* noflush mode */
      if (argv[i][2])
	flushtime = (int)atol(argv[i]+2);
      else if (isdigit(argv[i+1][0]))
	flushtime = (int)atol(argv[++i]);
      else
	flushtime = 0;
      continue;
    case 's':		/* startup script */
      if (argv[i][2])
	startup = argv[i]+2;
      else
	startup = argv[++i];
      continue;
    case 'e':		/* fast (but unsafe) OS eof processing */
      safe_eof = false;
      continue;
    case '-':		/* exit switch mode */
      ++i;
      break;

    default:
syntax:
      fputs(
"\
Usage:	btserve [-b blksize] [-c cachesize] [-d] [-e] [-f] [filesys ...]\n\
	-b ddd	maximum blocksize in bytes\n\
	-d	daemon/debugging flag, do not run in background\n\
	-e	disable safe EOF processing for OS files\n\
	-f	disable auto-flush\n\
	-f ddd	auto-flush every ddd seconds while active (default 30)\n\
		auto-flush every 3 seconds while no activity\n\
	-c ddd	cache memory size in bytes\n\
	-s fnm	run startup script/program after mounting drives\n\
	--	stop interpreting flag arguments\n\
",
	stderr);
      return 1;
    }
    break;
  }

  if (!BlockCache::ok(block,cache)) {
    do
        cache += 50000L;
    while (!BlockCache::ok(block,cache));
    fprintf(stderr,"Cache size increased to %ld.\n",cache);
  }
  {
  const char *s = getenv("BTSERVE");
  char serverid = s ? *s : 0;
  btserve server(block,cache,serverid);
  reqsiz = sizeof *reqp - sizeof reqp->lbuf + server.getmaxrec();
  if ((reqp = (BTCB *)malloc(reqsiz)) == 0) {
    fputs("No memory\n",stderr);
    return 1;
  }
  server.setSafeEof(safe_eof);

  if (!debug)
    setpgrp();			/* disconnect from interrupts */

  long btaskey = BTASKEY + serverid * 2L;
  btasreq = msgget(btaskey,0620+IPC_CREAT+IPC_EXCL);
  if (btasreq == -1) {
    if (errno == EEXIST)
      fputs("BTAS/X Server already loaded\n",stderr);
    else
      perror(*argv);
    return 1;
  }
  btasres = msgget(btaskey+1,0640+IPC_CREAT+IPC_EXCL);
  if (btasres == -1) {
    perror(*argv);
    msgctl(btasreq,IPC_RMID,&buf);
    return 1;
  }
#ifdef DETERMINISTIC
  fprintf(stderr,"DETERMINISTIC version for testing.\n");
#endif

  while (i < argc) {		/* mount listed filesystems */
    const char *s = argv[i++];
    rc = server.mount(s);
    if (rc)
      fprintf(stderr,"Error %d mounting %s\n",rc,s);
    else
      fprintf(stderr,"%s mounted on /\n",s);
  }

  if (!debug)
    if (fork()) return 0;		/* run in background */

  if (startup) {
    switch (fork()) {
    case 0:
      execl(startup,startup,(char *)0);
    case -1:
      perror(startup);
    default:
      break;
    }
  }

  /* process requests */

#ifdef DATLOCK
  plock(DATLOCK);	/* lock data segment in memory */
#endif

  Alarm alarm;

  for (;;) {
    rc = msgrcv(btasreq,(struct msgbuf *)reqp,
		reqsiz - sizeof reqp->msgident, 0L, MSG_NOERROR);
    if (rc == -1) {
      if (alarm.check(&server)) break;
      continue;
    }
    int op = reqp->op;			// request op code
    int opflags = btopflags[op&31];	// type of op code
    server.incTrans( (opflags & BT_UPDATE) ? rc : 0 );
#if TRACE > 1
    fprintf(stderr,"rmlen=%d,op=%x,klen=%d,rlen=%d,\n",
	rc,op,reqp->klen,reqp->rlen);
#endif
    rc = reqp->op = server.btas(reqp,op);	/* execute request */

    /* add allowed trap flags to error code */
    if (reqp->op >= 200 && reqp->op < sizeof btflags + 200)
      reqp->op |= btflags[reqp->op - 200] << 8;

    int len = sizeof *reqp - sizeof reqp->msgident - sizeof reqp->lbuf;
    if (opflags != BT_UPDATE)
      len += reqp->rlen;
    else if (reqp->op & NOKEY)
      len += reqp->klen;	/* report why nokey failed */

#if TRACE > 0
    if (rc && (reqp->op & op & BTERR) == 0) {	/* if fatal error */
      int i, n;
      switch (rc) {
      /* cases we don't care about */
      case 223: case 214: case 212: case 217: case 210: case 221:
#if TRACE < 2
	break;
#endif
      default:
	fprintf(stderr,
	  "%sBTCB:\tpid=%ld root=%08lX mid=%d flgs=%04X rc=%d op=%d\n",
	  ctime(&btserve::curtime),
	  reqp->msgident,reqp->root,reqp->mid,reqp->flags,reqp->op,op
	);
	n = server.getmaxrec();
	fprintf(stderr,"\tklen = %d rlen = %d, maxrec=%d\n",
		reqp->klen,reqp->rlen,n);
	if (len < n) n = len;
	for (i = 0; i < n; ++i) {
	  fprintf(stderr,"%02X%c",
		reqp->lbuf[i]&0xff, ((i&15) == 15) ? '\n' : ' ');
	}
	if (i & 15) putc('\n',stderr);
      }
    }
#endif
#if TRACE > 1
    fprintf(stderr,"smlen=%d\n",len);
#endif
    rc = msgsnd(btasres,(struct msgbuf *)reqp,len,IPC_NOWAIT);
    if (rc == -1) {
      if (errno != EAGAIN) break;
      cleanq(btasres,len + 4);	/* remove dead messages from queue */
      if (msgsnd(btasres,(struct msgbuf *)reqp,len,0) == -1) break;
    }
    server.sync();
    /* suspend operations */
    if ((reqp->op & 255) == 214 && (op & 31) == BTFLUSH) {
      fprintf(stderr,"BTAS/X frozen at %s", ctime(&btserve::curtime));
      while (validpid(reqp->msgident))
	sleep(10);
      time(&btserve::curtime);
      fprintf(stderr,"BTAS/X unfrozen at %s", ctime(&btserve::curtime));
    }
    if (flushtime)
      alarm.enable(flushtime);
  }
  /* shutdown */
  rc = errno;
  }
  msgctl(btasreq,IPC_RMID,&buf);
  msgctl(btasres,IPC_RMID,&buf);
  time(&btserve::curtime);
  const char *s = ctime(&btserve::curtime);
  if (rc == EIDRM) {
    fprintf(stderr,"BTAS/X shutdown: btstop at %s",s);
    return 0;
  }
  if (rc == EINTR) return 0;
  fprintf(stderr,"BTAS/2 system error %d at %s\n",rc,s);
  return 1;
}
