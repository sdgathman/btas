/*
 * $Log$
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

static int addflds(int fd,const char *buf,int len) {
  int n = len/2;
  int i;
  struct btfrec *f = alloca(sizeof *f * n);
  for (i = 0; i < n; ++i) {
    f[i].type = *buf++;
    f[i].len = *buf++;
  }
  return isaddflds(fd,f,n);
}

static int cmprec(const void *a,const void *b) {
  const struct btfrec *fa = a;
  const struct btfrec *fb = b;
  return fa->pos - fb->pos;
}

static int getflds(int fd,char *buf) {
  struct btflds *f = isflds(fd);
  int i, n;
  struct btfrec *fa;
  if (!f) return 0;
  // sort by position, first copy 
  n = f->rlen;
  fa = alloca(sizeof *fa * n);
  for (i = 0; i < n; ++i)
    fa[i] = f->f[i];
  qsort(fa,n,sizeof *fa,cmprec);
  for (i = 0; i < n; ++i) {
    *buf++ = fa[i].type;
    *buf++ = fa[i].len;
  }
  return i * 2;
}

static volatile int stop = 0;

static void stopserver(int sig) {
  stop = 1;
}

static int server() {
  int i, trace = -1;
  char *auxbuf = 0;
  int auxmax = 0;
  struct ISREQ r;			/* request header */
  struct ISRES res;			/* result header */
  /* FIXME: need file formats for these also (and in isstub.c) */
  union { char buf[MAXRLEN+1];
	  long id;
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
      write(trace,tsbuf,sizeof tsbuf);
      write(trace,(char *)&r,sizeof r);
    }
    if (p1len) {
      while (p1len > MAXRLEN) { read(0,p1.buf,MAXRLEN); p1len -= MAXRLEN; }
      read(0,p1.buf,p1len);
      p1.buf[p1len]=0;
    }
    if (p2len) {
      while (p2len > MAXRLEN) { read(0,p2.buf,MAXRLEN); p2len -= MAXRLEN; }
      read(0,p2.buf,p2len);
      p2.buf[p2len]=0;
    }
    if (trace > 0) {
      if (p1len) write(trace,p1.buf,p1len);
      if (p2len) write(trace,p2.buf,p2len);
    }
      
    stshort(0,res.p1);
    switch (r.fxn) {
      union {
	struct keydesc desc;
	struct dictinfo dict;
      } d;
    case ISBUILD:
	ldkeydesc(&d.desc,p2.buf);
	if (len == 0)
	  i = isbuildx(p1.buf,len,&d.desc,mode,0);
	else
	  i = isbuild(p1.buf,len,&d.desc,mode);
	break;
    case ISOPEN:
	i = isopen(p1.buf,mode);
	if (i >= 0) {
	  isindexinfo(i,&d.desc,0);
	  iserrno = d.dict.di_recsize;
	}
	break;
    case ISCLOSE:
	i = isclose(r.fd);
	break;
    case ISADDINDEX:
	ldkeydesc(&d.desc,p1.buf);
	i = isaddindexn(r.fd,&d.desc,(p2len == 0) ? 0 : p2.buf);
	break;
    case ISDELINDEX:
	ldkeydesc(&d.desc,p1.buf);
	i = isdelindex(r.fd,&d.desc);
	break;
    case ISSTART:
	ldkeydesc(&d.desc,p2.buf);
	isrange(r.fd,0,0);
	i = isstart(r.fd,&d.desc,len,p1.buf,mode);
	break;
    case ISSTARTN:
	isrange(r.fd,0,0);
	i = isstartn(r.fd,p2.buf,len,p1.buf,mode);
	break;
    case ISREAD: case ISREADREC:
	if (p1len == 0)	/* Allow client to skip key bytes for FIRST,NEXT,etc. */
	  p1len = isreclen(r.fd);
	else if (p1len >= isreclen(r.fd) + 4) {
	  p1len -= 4;
	  isrecnum = ldlong(p1.buf + p1len);
	}
	else	/* Allow client to send bytes needed for key info only. */
	  p1len = isreclen(r.fd);
	switch (mode) {
	case ISLESS:
	  mode = ISPREV;
	  i = isread(r.fd,p1.buf,ISGTEQ);
	  if (i) {
	    if (iserrno != ENOREC) break;
	    mode = ISLAST;
	  }
	  break;
	case ISLTEQ:
	  mode = ISPREV;
	  i = isread(r.fd,p1.buf,ISGREAT);
	  if (i) {
	    if (iserrno != ENOREC) break;
	    mode = ISLAST;
	  }
	  break;
	}
	i = isread(r.fd,p1.buf,mode);
	if (r.fxn == ISREADREC) {
	  stlong(isrecnum,p1.buf + p1len);
	  p1len += 4;
	}
	stshort(p1len,res.p1);
	if (i == 0 && len > 0) {
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
	break;
    case ISWRITE:
	i = iswrite(r.fd,p1.buf);
	break;
    case ISREWRITE:
	if (p1len >= isreclen(r.fd) + 4) {
	  p1len -= 4;
	  isrecnum = ldlong(p1.buf + p1len);
	}
	i = isrewrite(r.fd,p1.buf);
	break;
    case ISWRCURR:
	i = iswrcurr(r.fd,p1.buf);
	break;
    case ISREWCURR:
	i = isrewcurr(r.fd,p1.buf);
	break;
    case ISREWREC:
	i = isrewrec(r.fd,isrecnum,p1.buf);
	break;
    case ISDELREC:
	i = isdelrec(r.fd,isrecnum);
	break;
    case ISDELETE:
	if (p1len >= isreclen(r.fd) + 4) {
	  p1len -= 4;
	  isrecnum = ldlong(p1.buf + p1len);
	}
	i = isdelete(r.fd,p1.buf);
	break;
    case ISDELCURR:
	i = isdelcurr(r.fd);
	break;
    case ISUNIQUEID: {
	long id;
	i = isuniqueid(r.fd,&id);
	stlong(id,p1.buf);
	stshort(4,res.p1);
	break;
      }
    case ISINDEXINFO:
	i = isindexinfo(r.fd,&d.desc,mode);
	if (mode)
	  stshort(stkeydesc(&d.desc,p1.buf),res.p1);
	else
	  stshort(stdictinfo(&d.dict,p1.buf),res.p1);
	break;
    case ISINDEXNAME:
	i = isindexname(r.fd,p1.buf,mode);
	stshort(strlen(p1.buf),res.p1);
	break;
    case ISAUDIT:
	i = isaudit(r.fd,p1.buf,mode);
	break;
    case ISERASE:
	i = iserase(p1.buf);
	break;
    case ISLOCKOP:
	i = islock(r.fd);
	break;
    case ISUNLOCK:
	i = isunlock(r.fd);
	break;
    case ISRELEASE:
	i = isrelease(r.fd);
	break;
    case ISRENAME:
	i = isrename(p1.buf,p2.buf);
	break;
    case ISSETRANGE:
	i = isrange(r.fd,p1len ? p1.buf : 0,p2len ? p2.buf : 0);
	break;
    case ISADDFLDS:
	i = addflds(r.fd,p1.buf,p1len);
	break;
    case ISGETFLDS:
	i = getflds(r.fd,p1.buf);
	stshort(i,res.p1);
	i = 0;
	break;
    default:
	i = -1;
	iserrno = 118;
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
      write(trace,tsbuf,sizeof tsbuf);
      write(trace,(char *)&res,sizeof res);
      if (ldshort(res.p1)) write(trace,p1.buf,ldshort(res.p1));
      if (auxlen > 0) write(trace,auxbuf,auxlen);
    }
    write(0,(char *)&res,sizeof res);
    if (ldshort(res.p1)) write(0,p1.buf,ldshort(res.p1));
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
    dup(fd); dup(fd); close(fd);
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
    int s, len = sizeof saddr;
    s = accept(fd,(struct sockaddr *)&saddr,&len);
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
      dup(s); dup(s); close(s);
      return server();
    }
    close(s);
  }
  return 0;
}
#endif
