/*
    This file is part of the BTAS client library.

    The BTAS client library is free software: you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public License as
    published by the Free Software Foundation, either version 3 of the License,
    or (at your option) any later version.

    BTAS is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with BTAS.  If not, see <http://www.gnu.org/licenses/>.
*/
#include <stdlib.h>
#include <btas.h>
#include <bterr.h>
#include <errenv.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <errno.h>
#include <unistd.h>

#define HDRSIZE(b)	(sizeof *b - sizeof b->lbuf - sizeof b->msgident)
static int btasreq = -1;	/* BTAS/2 request queue */
static int btasres;		/* BTAS/2 result queue */
static int pid;
static int server_cnt = 0;	/* number of times server was restarted. */

/** True if the opcode doesn't rely on existing BTCB state. */
static int isStateless(BTCB *b,int op) {
  switch (op) {
  case BTOPEN: case BTFLUSH: case BTPSTAT:
    return 1;
  case BTSTAT: case BTMOUNT: case BTUMOUNT:
    return b->root == 0;
  case BTCLOSE:
    return b->flags == 0;
  }
  return 0;
}
  
/** Low level BTAS client operation.  Normally, all errors are
 * fatal and invoke errpost (lightweight C exceptions).  Certain
 * errors, like BTERKEY, can be trapped (returned as error codes)
 * with the DUPKEY, NOKEY, and LOCK op code flags.
 * @param b	pointer to BTCB
 * @param op	op code logically ored with flags
 * @return 0 on success
 */
int btas(BTCB *b,int op) {
  register int rc, size;
  // this doesn't detect stale bttag from old btas.h
  // FIXME: are there other ops that need an exception?
  if (b->msgident != server_cnt && !isStateless(b,op & 31))
    errpost(EIDRM);
  for (;;) {
    if (btasreq == -1) {
      const char *s = getenv("BTSERVE");
      int server = s ? *s * 2 : 0;
      //if ( (btasreq = msgget(BTASKEY + server,0420)) == -1
      //  || (btasres = msgget(BTASKEY+server+1,0240)) == -1) 
      if ( (btasreq = msgget(BTASKEY + server,0020)) == -1
	|| (btasres = msgget(BTASKEY+server+1,0040)) == -1) {
	btasreq = -1;
	errpost((errno == ENOENT) ? BTERSERV : errno);
      }
      ++server_cnt;
      pid = getpid();
    }
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
    if (rc != EIDRM) break;
    btasreq = -1;
  }
  if (rc == -1) {
    b->msgident = server_cnt;
    errpost(errno);		/* fatal error */
  }
  size = HDRSIZE(b) + b->rlen;		/* max received size */
  do {
    rc = msgrcv(btasres,(struct msgbuf *)b,size,b->msgident,MSG_NOERROR);
  } while (rc == -1 && errno == EINTR);
  b->msgident = server_cnt;
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

/** Save a passive form of an open BTCB.  The file will be kept
 * open, but the tag form is much smaller, and can be stored and
 * retrieved from files.  (Unless some process comes along and forcibly
 * closes it - as it likely to happen with temp files.)
 */
void btcb2tag(const BTCB *b, struct bttag *t) {
  t->ident = b->msgident;
  t->root = b->root; t->mid = b->mid; t->flags = b->flags;
}

/** Restore a BTCB from a saved tag.
 */
void tag2btcb(BTCB *b, const struct bttag *t) {
  b->msgident = t->ident;
  b->root = t->root; b->mid = t->mid; b->flags = t->flags;
}
