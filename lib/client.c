#include <stdlib.h>
#include <btas.h>
#include <bterr.h>
#include <errenv.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <errno.h>

#define HDRSIZE(b)	(sizeof *b - sizeof b->lbuf - sizeof b->msgident)
static int btasreq = -1;	/* BTAS/2 request queue */
static int btasres;		/* BTAS/2 result queue */
static int pid;
  
int btas(BTCB *b,int op) {
  register int rc, size;
  if (btasreq == -1) {
    const char *s = getenv("BTSERVE");
    int server = s ? *s * 2 : 0;
    //if ( (btasreq = msgget(BTASKEY + server,0420)) == -1
    //  || (btasres = msgget(BTASKEY+server+1,0240)) == -1) {
    if ( (btasreq = msgget(BTASKEY + server,0020)) == -1
      || (btasres = msgget(BTASKEY+server+1,0040)) == -1) {
      btasreq = -1;
      errpost((errno == ENOENT) ? BTERSERV : errno);
    }
  }
  if (!pid) pid = getpid();
  b->msgident = pid;
  b->op = op;
  size = HDRSIZE(b);			/* BTCB header */
  if (btopflags[op&31]&BT_UPDATE)
    size += b->rlen;			/* plus record for updates */
  else
    size += b->klen;			/* or just key otherwise */
  do {
    rc = msgsnd(btasreq,(struct msgbuf *)b,size,0);
  } while (rc == -1 && errno == EINTR);
  if (rc == -1) errpost(errno);		/* fatal error */
  size = HDRSIZE(b) + b->rlen;		/* max received size */
  do {
    rc = msgrcv(btasres,(struct msgbuf *)b,size,b->msgident,MSG_NOERROR);
  } while (rc == -1 && errno == EINTR);
  if (rc == -1) errpost(errno);
  /* These codes overflow 8 bits.  Stupid, but I was trying
     to keep codes the same as the Series/1. */
  if (b->op == 294 || b->op == 283)
    errpost(b->op);
  /* check for fatal (not trapped) error */
  if (b->op) {
    op &= b->op & BTERR;
    b->op &= ~BTERR;
    if (op == 0)
      errpost(b->op);
  }
  return b->op;
}
