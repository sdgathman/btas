/*
 * $Log$
 * Revision 2.20  2003/07/29 18:21:16  stuart
 * Restrict file descriptors to fit 8 bit field.
 *
 * Revision 2.19  2003/04/05 04:40:59  stuart
 * Sanity check keydesc.  Initialize kp->k.k_len in isaddindex().
 *
 * Revision 2.18  2003/03/18 21:38:34  stuart
 * Make isreq a public interface.
 *
 * Revision 2.17  2003/03/12 22:41:53  stuart
 * Create isreq encapsulated C-isam call interface.
 *
 * Revision 2.16  2003/02/25 04:43:57  stuart
 * More functions and testing for cisam python module.
 *
 * Revision 2.15  2002/03/20 20:07:14  stuart
 * Support ISMKDIR/ISRMDIR in isserve.
 * Include istrace in package.
 *
 * Revision 2.14  2001/10/16 19:26:28  stuart
 * send key only on ISREAD in isserve.c, minor release
 *
 * Revision 2.13  2000/09/27 19:50:58  stuart
 * Add timing info to trace output
 * clear range on isstart
 *
 * Revision 2.12  2000/09/21 02:47:59  stuart
 * handle network byte order for loopback port/IP
 *
 * Revision 2.11  1998/10/05  17:57:07  stuart
 * bug with read mode
 *
 * Revision 2.10  1998/06/24  16:38:48  stuart
 * simplify socket setup
 *
 * Revision 2.9  1998/04/15  19:14:58  stuart
 * support ISINDEXNAME
 *
 * Revision 2.8  1997/11/25  20:53:15  stuart
 * support appending isrecnum to record to simplify browsing
 * files with no unique key
 *
 * Revision 2.7  1997/11/21  01:55:39  stuart
 * command line trace file
 * fix field table get/set bugs
 *
 * Revision 2.6  1997/08/13  15:23:07  stuart
 * SETRANGE and fieldtable
 *
 * Revision 2.5  1997/04/30  19:30:28  stuart
 * a few missing ops
 *
 * Revision 2.4  1997/02/17  21:26:02  stuart
 * TCPNODELAY
 * readn()
 * operation from inetd
 *
 * Revision 2.3  1996/09/23  21:51:42  stuart
 * run from inetd, readFully request headers
 *
 * Revision 2.2  1994/02/13  21:26:16  stuart
 * convert keydesc and dictinfo parms
 *
 * Revision 2.1  1994/02/13  20:36:47  stuart
 * works on same architecture with sockets
 *
 */
static const char id[] = "$Id$";

#define TRACE
#include <stdio.h>
#include <sys/times.h>
#include <ctype.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <isamx.h>
#include <fcntl.h>
#include <stdlib.h>
#include <btflds.h>
#include <arpa/inet.h>
#include "isreq.h"

static int readFully(int fd,char *buf,int len) {
  int cnt = 0;
  while (cnt < len) {
    int n = read(fd,buf + cnt,len - cnt);
    if (n <= 0) return cnt;
    cnt += n;
  }
  return cnt;
}

static volatile int stop = 0;

static void stopserver(int sig) {
  stop = 1;
}

static int writetrace(int fd,char *buf,size_t len) {
  if (fd > 0) {	/* FIXME: should this be GE ? */
    ssize_t n = write(fd,buf,len);
    if (n != len) {
      perror("write");
      close(fd);
      fd = -1;
    }
  }
  return fd;
}

/* Discard extra when pXlen > MAXRLEN. */
static ssize_t readlast(int fd,char *buf,size_t len) {
  if (len) {
    while (len > MAXRLEN) {
      ssize_t n = read(fd,buf,MAXRLEN);
      if (n < 0) return n;
      len -= n;
    }
    size_t rem = len;
    while (rem > 0) {
      ssize_t n = read(fd,buf,rem);
      if (n < 0) return n;
      rem -= n;
    }
    buf[len]=0;
  }
  return len; /* return total read chars not discarded */
}

static int server() {
  int i, trace = -1;
  char *auxbuf = 0;
  int auxmax = 0;
  struct ISREQ r;			/* request header */
  struct ISRES res;			/* result header */
  /* FIXME: need file formats for these also (and in isstub.c) */
  union { char buf[MAXRLEN+1];
	} p1;		/* parameter 1 */
  union { char buf[MAXRLEN+1];
	} p2;		/* parameter 2 */

  signal(SIGINT,stopserver);
  signal(SIGHUP,stopserver);
  signal(SIGKILL,stopserver);

  for (i=3;i<20;) close(i++);	/* close parent's files */
#ifdef TRACE
  {
    char name[64];
    char *p = getenv("ISTRACE");
    if (p) {
      //sprintf(name,"%s/islog.%d",p,getpid());
      //freopen(name,"w",stderr);
      sprintf(name,"%s/istrace.%d",p,getpid());
      trace = open(name,O_WRONLY+O_CREAT+O_TRUNC,0666);
    }
  }
#endif
  /* Limit file descriptors for this process.  We shortsightedly
   * allocated only 8 bits for fd. */
  isfdlimit(255);

  while (!stop && readFully(0,(char *)&r,sizeof r) == sizeof r) {
    int p1len = ldshort(r.p1);
    int p2len = ldshort(r.p2);
    int len = ldshort(r.len);
    int mode = ldshort(r.mode);
    int auxlen = 0;
    isrecnum = ldlong(r.recnum);
    if (trace > 0) {
      struct tms tbuf;
      clock_t ts = times(&tbuf);
      char tsbuf[12];
      stlong(ts,tsbuf);
      stlong(tbuf.tms_utime,tsbuf+4);
      stlong(tbuf.tms_stime,tsbuf+8);
      trace = writetrace(trace,tsbuf,sizeof tsbuf);
      trace = writetrace(trace,(char *)&r,sizeof r);
    }
    p1len = readlast(0,p1.buf,p1len);
    p2len = readlast(0,p2.buf,p2len);
    if (trace > 0) {
      if (p1len) trace = writetrace(trace,p1.buf,p1len);
      if (p2len) trace = writetrace(trace,p2.buf,p2len);
    }
      
    i = isreq(r.fd,r.fxn,&p1len,p2len,p1.buf,p2.buf,mode,len);
    stshort(p1len,res.p1);
    if (i == 0 && len > 0 && (r.fxn == ISREAD || r.fxn == ISREADREC)) {
      switch (mode & 255) {
      case ISLESS: case ISLTEQ: case ISPREV: case ISLAST:
	mode = ISPREV; break;
      case ISGREAT: case ISGTEQ: case ISNEXT: case ISFIRST:
	mode = ISNEXT; break;
      default:
	mode = ISNEXT;
      }
      auxlen = --len * p1len;
      if (auxlen > auxmax) {
	if (auxbuf) free(auxbuf);
	auxmax = auxlen;
	auxbuf = malloc(auxlen);
      }
      for (i = 0; i < len; ++i) {
	char *buf = auxbuf + i * p1len;
	if (isread(r.fd,auxbuf + i * p1len,mode)) break;
	if (r.fxn == ISREADREC)
	  stlong(isrecnum,buf + p1len - 4);
      }
      auxlen = i++ * p1len;
    }

    stlong(isrecnum,res.recnum);
    stshort(i,res.res);
    stshort(iserrno,res.iserrno);
    res.isstat1 = isstat1;
    res.isstat2 = isstat2;
    if (trace > 0) {
      struct tms tbuf;
      clock_t ts = times(&tbuf);
      char tsbuf[12];
      stlong(ts,tsbuf);
      stlong(tbuf.tms_utime,tsbuf+4);
      stlong(tbuf.tms_stime,tsbuf+8);
      trace = writetrace(trace,tsbuf,sizeof tsbuf);
      trace = writetrace(trace,(char *)&res,sizeof res);
      if (p1len) trace = writetrace(trace,p1.buf,p1len);
      if (auxlen > 0) trace = writetrace(trace,auxbuf,auxlen);
    }
    /* FIXME: should this be fd 0 ? May work because both are same socket. */
    write(0,(char *)&res,sizeof res);
    if (p1len) write(0,p1.buf,p1len);
    if (auxlen > 0) write(0,auxbuf,auxlen);
  }
  if (auxbuf)
    free(auxbuf);
  iscloseall();
  return 0;
}

#if 0
int main() {
  return server();
}
#else

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#ifndef m88k
#include <netinet/tcp.h>
#endif
#include <netdb.h>

int main(int argc,char **argv) {
  int port = -1;
  int fd;
  int nodelay = 1;
  struct sockaddr_in saddr;
  int i;
  int callback = 0;
  for (i = 1; i < argc; ++i) {
    if (strncmp(argv[i],"-f",2) == 0) {
      const char *trfile = argv[i][2] ? argv[i] + 2 : argv[++i];
      static const char trkey[] = "ISTRACE";
      char *ebuf = malloc(strlen(trfile) + sizeof trkey + 1);
      sprintf(ebuf,"%s=%s",trkey,trfile);
      putenv(ebuf);
    }
    if (strcmp(argv[i],"-a") == 0) {
      callback = 1;
      continue;
    }
    else if (isdigit(*argv[i]))
      port = atoi(argv[i]);
    else {
      fputs("$Id$\n\
Usage:	isserve [-a] [-ftracedir] [tcpport]\n",stderr);
      return 1;
    }
  }

  if (port < 1) {
  #ifdef TCP_NODELAY
    setsockopt(0,IPPROTO_TCP,TCP_NODELAY,&nodelay,sizeof nodelay);
  #endif
    return server();
  }
  fd = socket(AF_INET,SOCK_STREAM,0);
  if (fd == -1) {
    perror("socket");
    return 1;
  }
  saddr.sin_family = AF_INET;
  saddr.sin_addr.s_addr = INADDR_ANY;
  saddr.sin_port = htons(port);
  if (callback) {
    saddr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    if (connect(fd,(struct sockaddr *)&saddr,sizeof saddr) < 0) {
      perror("connect");
      return 1;
    }
  #ifdef TCP_NODELAY
    setsockopt(fd,IPPROTO_TCP,TCP_NODELAY,&nodelay,sizeof nodelay);
  #endif
    close(0); close(1); close(2);
    if (dup(fd) != 0 || dup(fd) != 1) {
      perror("dup returned unexpected value");
      return 1;
    }
    close(fd);
    return server();
    //return fork() ? 0 : server();
  }
  /* setup socket for listening */
  if (bind(fd,(struct sockaddr *)&saddr,sizeof saddr) < 0) {
    perror("bind");
    return 1;
  }
  if (listen(fd,5) < 0) {
    perror("listen");
    return 1;
  }

  /* accept incoming connections */
  for (;;) {
    socklen_t len = sizeof saddr;
    int s = accept(fd,(struct sockaddr *)&saddr,&len);
    if (s < 0) {
      perror("accept");
      continue;
    }
  #ifdef TCP_NODELAY
    setsockopt(s,IPPROTO_TCP,TCP_NODELAY,&nodelay,sizeof nodelay);
  #endif
    switch (fork()) {
    case -1:
      perror("fork");
      break;
    case 0:	/* child */
      fprintf(stderr,"Connected to %s:%d\n",
	inet_ntoa(saddr.sin_addr),
	saddr.sin_port
      );
      close(fd); close(0); close(1);
      if (dup(fd) != 0 || dup(fd) != 1) {
	perror("dup returned unexpected value");
	return 1;
      }
      close(s);
      return server();
    }
    close(s);
  }
  return 0;
}
#endif
