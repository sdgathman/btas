/*
 * $Log$
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
#include <unistd.h>
#include <isamx.h>
#include <fcntl.h>
#include <stdlib.h>
#include <btflds.h>
#include "isreq.h"

int readFully(int fd,char *buf,int len) {
  int cnt = 0;
  while (cnt < len) {
    int n = read(fd,buf + cnt,len - cnt);
    if (n <= 0) return cnt;
    cnt += n;
  }
  return cnt;
}

int addflds(int fd,const char *buf,int len) {
  int n = len/2;
  int i;
  struct btfrec *f = alloca(len / 2);
  for (i = 0; i < n; ++i) {
    f->type = *buf++;
    f->len = *buf++;
  }
  return isaddflds(fd,f,n);
}

int getflds(int fd,char *buf) {
  struct btflds *f = isflds(fd);
  if (!f) return 0;
  for (i = 0; i < f->rlen; ++i) {
    *buf++ = f->f[i].type;
    *buf++ = f->f[i].len;
  }
  return i * 2;
}

int server() {
  int i, trace = -1, trout = -1;
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

  for (i=3;i<20;) close(i++);	/* close parent's files */
#ifdef TRACE
  {
    char name[64];
    char *p = getenv("ISTRACE");
    if (p) {
      sprintf(name,"%s/isin.%d",p,getpid());
      trace = open(name,O_WRONLY+O_CREAT+O_TRUNC,0666);
      sprintf(name,"%s/isout.%d",p,getpid());
      trout = open(name,O_WRONLY+O_CREAT+O_TRUNC,0666);
    }
  }
#endif
  while (readFully(0,(char *)&r,sizeof r) == sizeof r) {
    int p1len = ldshort(r.p1);
    int p2len = ldshort(r.p2);
    int len = ldshort(r.len);
    int auxlen = 0;
    isrecnum = ldlong(r.recnum);
    if (trace > 0)
      write(trace,(char *)&r,sizeof r);
    if (p1len) {
      while (p1len > MAXRLEN) { read(0,p1.buf,MAXRLEN); p1len -= MAXRLEN; }
      read(0,p1.buf,p1len);
      p1.buf[p1len]=0;
    }
    if (p2len) {
      while (p2len > MAXNAME) { read(0,p2.buf,MAXNAME); p2len -= MAXNAME; }
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
	i = isbuild(p1.buf,len,&d.desc,ldshort(r.mode));
	break;
    case ISOPEN:
	i = isopen(p1.buf,ldshort(r.mode));
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
	i = isaddindex(r.fd,&d.desc);
	break;
    case ISDELINDEX:
	ldkeydesc(&d.desc,p1.buf);
	i = isdelindex(r.fd,&d.desc);
	break;
    case ISSTART:
	ldkeydesc(&d.desc,p2.buf);
	i = isstart(r.fd,&d.desc,len,p1.buf,ldshort(r.mode));
	break;
    case ISSTARTN:
	i = isstartn(r.fd,p2.buf,len,p1.buf,ldshort(r.mode));
	break;
    case ISREAD:
	stshort(p1len,res.p1);
	if (ldshort(r.mode) == ISLESS) {
	  stshort(ISPREV,r.mode);
	  i = isread(r.fd,p1.buf,ISGTEQ);
	  if (i) {
	    if (iserrno != ENOREC) break;
	    stshort(ISLAST,r.mode);
	  }
	}
	if (ldshort(r.mode) == ISLTEQ) {
	  stshort(ISPREV,r.mode);
	  i = isread(r.fd,p1.buf,ISGREAT);
	  if (i) {
	    if (iserrno != ENOREC) break;
	    stshort(ISLAST,r.mode);
	  }
	}
	i = isread(r.fd,p1.buf,ldshort(r.mode));
	if (i == 0 && len > 0) {
	  int mode = ISNEXT;
	  switch (ldshort(r.mode)) {
	  case ISLESS: case ISLTEQ: case ISPREV: case ISLAST:
	    mode = ISPREV; break;
	  case ISGREAT: case ISGTEQ: case ISNEXT: case ISFIRST:
	    mode = ISNEXT; break;
	  }
	  auxlen = --len * p1len;
	  if (auxlen > auxmax) {
	    if (auxbuf) free(auxbuf);
	    auxmax = auxlen;
	    auxbuf = malloc(auxlen);
	  }
	  for (i = 0; i < len; ++i)
	    if (isread(r.fd,auxbuf + i * p1len,mode)) break;
	  auxlen = i++ * p1len;
	}
	break;
    case ISWRITE:
	i = iswrite(r.fd,p1.buf);
	break;
    case ISREWRITE:
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
	i = isdelete(r.fd,p1.buf);
	break;
    case ISDELCURR:
	i = isdelcurr(r.fd);
	break;
    case ISUNIQUEID:
	stshort(sizeof p1.id,res.p1);
	i = isuniqueid(r.fd,&p1.id);
	break;
    case ISINDEXINFO:
	i = isindexinfo(r.fd,&d.desc,ldshort(r.mode));
	if (ldshort(r.mode))
	  stshort(stkeydesc(&d.desc,p1.buf),res.p1);
	else
	  stshort(stdictinfo(&d.dict,p1.buf),res.p1);
	break;
    case ISAUDIT:
	i = isaudit(r.fd,p1.buf,ldshort(r.mode));
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
    if (trout > 0) {
      write(trout,(char *)&res,sizeof res);
      if (ldshort(res.p1)) write(trout,p1.buf,ldshort(res.p1));
      if (auxlen > 0) write(trout,auxbuf,auxlen);
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
  if (argc > 1) {
    port = atoi(argv[1]);
    if (port < 1) {
      fputs("$Id$\nUsage:	isserve [tcpport]\n",stderr);
      return 1;
    }
  }

  if (port < 1) {
  #ifdef TCP_NODELAY
    setsockopt(0,IPPROTO_TCP,TCP_NODELAY,&nodelay,sizeof nodelay);
  #endif
    return server();
  }
  /* setup socket for listening */
  fd = socket(AF_INET,SOCK_STREAM,0);
  if (fd == -1) {
    perror("socket");
    return 1;
  }
  saddr.sin_family = AF_INET;
  saddr.sin_addr.s_addr = INADDR_ANY;
  saddr.sin_port = port;
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
