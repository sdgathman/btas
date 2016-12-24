/** Attempt to encapsulate messages between BTAS server and clients.
   This is the POSIX version, and isn't used yet.  See server.c
   Must work for 
     Windows
     Unix msg
     socket datagram
     DOS soft int
  
    Copyright (C) 1985-2013 Business Management Systems, Inc

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */
class btmsg {
  int btasreq, btasres;
  static void cleanq(int qid,int size);
public:
  send(const void *res,int size);
  recv(void *req,int size);
  static bool valid(int);
  bool OK() const { return btasreq >= 0 && btasres >= 0; }
};

/* remove result messages whose receiver has died 
*/

bool btmsg::valid(int pid) {
  return kill(pid,0) == 0 || errno != ESRCH;
}

void btmsg::cleanq(int qid,int size) {
  struct {
    long mtype;
    char mtext[1024];
  } msg;
  struct msqid_ds d;
  /* throw away stale messages until there is room */
  do {
    int rc = msgrcv(qid,&msg,sizeof msg.mtext,0L,IPC_NOWAIT+MSG_NOERROR);
    if (rc == -1) break;	/* all gone now . . . */
    if (valid(msg.mtype)) {	/* put it back if task still around */
      msgsnd(qid,&msg,rc,IPC_NOWAIT);
      nap(50L);			/* let applications run awhile */
    }
    else
      fprintf(stderr,"Removed dead message for pid %ld\n",msg.mtype);
    if (msgctl(qid,IPC_STAT,&d) == -1) break;
  } while (d.msg_cbytes + size > d.msg_qbytes);
}

btmsg::btmsg(long key) {
  btasres = -1;
  btasreq = msgget(key,0620+IPC_CREAT+IPC_EXCL);
  if (btasreq == -1) {
    if (errno == EEXIST)
      fputs("BTAS/X Server already loaded\n",stderr);
    else
      perror(*argv);
  }
  else {
    btasres = msgget(key+1,0640+IPC_CREAT+IPC_EXCL);
    if (btasres == -1)
      perror(*argv);
  }
}

btmsg::~btmsg() {
  struct msqid_ds buf;
  if (btasreq >= 0)
    msgctl(btasreq,IPC_RMID,&buf);
  if (btasres >= 0)
    msgctl(btasres,IPC_RMID,&buf);
}

int btmsg::recv(void *reqp,int reqsiz) {
  int rc = msgrcv(btasreq,(struct msgbuf *)reqp,
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
}

int btmsg::send(const void *reqp,int len) {
  rc = msgsnd(btasres,(struct msgbuf *)reqp,len,IPC_NOWAIT);
  ticks = 1;	/* don't flush during activity */
  if (rc == -1) {
    if (errno != EAGAIN) break;
    cleanq(btasres,len + 4);	/* remove dead messages from queue */
    if (msgsnd(btasres,(struct msgbuf *)reqp,len,0) == -1) break;
  }
}
