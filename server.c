#define TRACE 1
/*#define DETERMINISTIC	/* define to test for OS bugs */
/*
	Server program to execute BTAS/2 requests
	Single thread execution for now.
 * $Log$
 * Revision 1.2  1993/05/14  16:21:08  stuart
 * return key information on NOKEY so that symlink works
 *
 * Flush buffer on timer - SDG 03-19-89
 */

#include "btbuf.h"
#include "node.def"
#include <btas.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <signal.h>
#include <errno.h>
#include <sys/lock.h>

#ifdef DETERMINISTIC
time_t time(time_t *p) {
  if (p)
    *p = 0x7FFFFFFFL;		/* the clock is frozen */
  return 0x7FFFFFFFL;
}
#else
extern time_t time(/**/ time_t * /**/);
#endif

extern const char version[];

static unsigned char btflags[32] = {
  0, 0, 8, 0, 0, 0, 0,0, 0, 0, 0,
  DUPKEY>>8, NOKEY>>8, NOKEY>>8, LOCK>>8, 8, 0, 0, 8,0,0,0,0,8
};

static void ignore(int), cleanq(int,int);

static int btasreq, btasres;
static int ticks = 0, fatal = 0;

static void ignore(sig) {	/* ignore signals (but terminate system call) */
  if (sig == SIGALRM) {
    if (ticks) {
      ++ticks;
      (void)signal(SIGALRM,ignore);
      (void)alarm(2);
      (void)time(&curtime);
    }
  }
  else fatal = sig;
}

/* remove result messages whose receiver has died 
*/

#define validpid(pid) (kill(pid,0) == 0 || errno != ESRCH)

static void cleanq(qid,size) {
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
      msgsnd(qid,&msg,rc,IPC_NOWAIT);
      nap(50L);			/* let applications run awhile */
    }
    else
      fprintf(stderr,"Removed dead message for pid %ld\n",msg.mtype);
    if (msgctl(qid,IPC_STAT,&d) == -1) break;
  } while (d.msg_cbytes + size > d.msg_qbytes);
}

char debug = 0;
char noflush = 0;

main(argc,argv)
  char **argv;
{
  register int rc;
  register BTCB *reqp;
  struct msqid_ds buf;
  unsigned cache = 50000U;	/* works on 16 bit systems */
  unsigned block = BLKSIZE;	/* good for most systems */
  int reqsiz,i;			/* max size of request block */
  long iocnt = 0;

  (void)fputs(version,stderr);
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
    case 'f':		/* noflush mode */
      noflush = 1;
      continue;
    case 'e':		/* fast (but unsafe) OS eof processing */
      safe_eof = 0;
      continue;
    case '-':		/* exit switch mode */
      ++i;
      break;

    default:
syntax:
      fputs(
"\
Usage:	btserve [-b blksize] [-s cachesize] [-d] [-e] [-f] [filesys ...]\n\
	-b ddd	maximum blocksize in bytes\n\
	-d	debugging flag, do not run in background, disable flush\n\
	-e	disable safe EOF processing for OS files\n\
	-f	disable auto-flush\n\
	-s ddd	cache memory size in bytes\n\
	--	stop interpreting flag arguments\n\
",
	stderr);
      return 1;
    }
    break;
  }

  rc = btbegin(block,cache);
  reqsiz = sizeof *reqp - sizeof reqp->lbuf + maxrec;
  if (rc || (reqp = (BTCB *)malloc(reqsiz)) == 0) {
    (void)fputs("No memory\n",stderr);
    return 1;
  }

  if (!debug)
    (void)setpgrp();			/* disconnect from interrupts */

  (void)signal(SIGTERM,ignore);	/* shutdown on terminate */
  (void)signal(SIGPWR,ignore);	/* attempt shutdown on powerfail */

  btasreq = msgget(BTASKEY,0620+IPC_CREAT+IPC_EXCL);
  if (btasreq == -1) {
    if (errno == EEXIST)
      (void)fputs("BTAS/X Server already loaded\n",stderr);
    else
      perror(*argv);
    return 1;
  }
  btasres = msgget(BTASKEY+1,0640+IPC_CREAT+IPC_EXCL);
  if (btasres == -1) {
    perror(*argv);
    (void)msgctl(btasreq,IPC_RMID,&buf);
    return 1;
  }
#ifdef DETERMINISTIC
  (void)fprintf(stderr,"DETERMINISTIC version for testing.\n");
#endif

  while (i < argc) {		/* mount listed filesystems */
    char *s = argv[i++];	/* setjmp may or may not increment i! */
    if (rc = setjmp(btjmp))
      (void)fprintf(stderr,"Error %d mounting %s\n",rc,s);
    else {
      (void)btmount(s);
      (void)fprintf(stderr,"%s mounted on /\n",s);
    }
  }

  if (!debug)
    if (fork()) return 0;		/* run in background */

  /* process requests */

  plock(DATLOCK);
  for (;;) {
    register int len, op;
    long cnt = iocnt;

    iocnt = stat.preads + stat.pwrites;
    cnt = iocnt - cnt;
    stat.sum2 += cnt * cnt;
    ++stat.trans;
    rc = msgrcv(btasreq,(struct msgbuf *)reqp,
		reqsiz - sizeof reqp->msgident, 0L, MSG_NOERROR);
    if (rc == -1) {
      if (errno != EINTR || fatal) break;
      if (ticks > 2) {
#if TRACE > 1
	fputs("Autoflush\n",stderr);
#endif
	if (btflush() == 0)
	  ticks = 0;		/* turn off timer if complete */
      }
      continue;
    }
#if TRACE > 1
    fprintf(stderr,"rmlen=%d,op=%x,klen=%d,rlen=%d,\n",
	rc,reqp->op,reqp->klen,reqp->rlen);
#endif
    if (ticks == 0 && !noflush) {
      ticks = 1;	/* enable alarm handler */
      (void)signal(SIGALRM,ignore);
      (void)alarm(2);
      (void)time(&curtime);
    }
    reqp->op = btas(reqp,op = reqp->op);	/* execute request */
    len = sizeof *reqp - sizeof reqp->msgident - sizeof reqp->lbuf;
    if (btopflags[op&31] != BT_UPDATE)
      len += reqp->rlen;
    else if (reqp->op & NOKEY)
      len += reqp->klen;	/* report why nokey failed */

    if (reqp->op >= 200 && reqp->op < sizeof btflags + 200) {
      reqp->op |= (btflags[reqp->op - 200] << 8) & op & BTERR;
#if TRACE > 0
      if ((reqp->op & BTERR) == 0) {	/* if fatal error */
	int i, n;
	switch (reqp->op) {
	case 214: case 212: case 217: case 210:	/* cases we don't care about */
	  break;
	default:
	  fprintf(stderr,
	    "%sBTCB:\tpid=%ld root=%08lX mid=%d flgs=%04X rc=%d op=%d\n",
	    ctime(&curtime),
	    reqp->msgident,reqp->root,reqp->mid,reqp->flags,reqp->op,op
	  );
	  fprintf(stderr,"\tklen = %d rlen = %d\n",reqp->klen,reqp->rlen);
	  n = len; if (n > MAXREC) n = MAXREC;
	  for (i = 0; i < n; ++i) {
	    fprintf(stderr,"%02X%c",
		  reqp->lbuf[i]&0xff, ((i&15) == 15) ? '\n' : ' ');
	  }
	  if (i & 15) putc('\n',stderr);
	}
      }
#endif
    }
#if TRACE > 1
    fprintf(stderr,"smlen=%d\n",len);
#endif
    rc = msgsnd(btasres,(struct msgbuf *)reqp,len,IPC_NOWAIT);
    ticks = 1;	/* don't flush during activity */
    if (rc == -1) {
      if (errno != EAGAIN) break;
      cleanq(btasres,len + 4);	/* remove dead messages from queue */
      if (msgsnd(btasres,(struct msgbuf *)reqp,len,0) == -1) break;
    }
  }
  /* shutdown */
  rc = errno;
  btend();
  (void)msgctl(btasreq,IPC_RMID,&buf);
  (void)msgctl(btasres,IPC_RMID,&buf);
  if (rc == EINTR) {
    (void)fputs("BTAS/X shutdown: SIGTERM\n",stderr);
    return 0;
  }
  if (rc == EIDRM) {
    (void)fputs("BTAS/X shutdown: btstop\n",stderr);
    return 0;
  }
  (void)fprintf(stderr,"BTAS/2 system error %d\n",rc);
  return 1;
}
