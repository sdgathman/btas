static char id[] = "@(#)isserve.c 1.9 2/9/94";

#include <isam.h>
#include "isreq.h"

main()
{
  register int i;
  REQ r;			/* request header */
  RES res;			/* result header */
  union { struct keydesc desc;
	  struct dictinfo dict;
	  char buf[MAXRLEN+1];
	  long id;
	} p1;		/* parameter 1 */
  union { struct keydesc desc;
	  char buf[MAXNAME+1];
	} p2;		/* parameter 2 */

  for (i=2;i<20;) close(i++);	/* close parent's files */
  while (read(0,(char *)&r,sizeof r) == sizeof r) {
    if (r.p1) {
      while (r.p1 > MAXRLEN) { read(0,p1.buf,MAXRLEN); r.p1 -= MAXRLEN; }
      read(0,p1.buf,r.p1);
      p1.buf[r.p1]=0;
    }
    if (r.p2) {
      while (r.p2 > MAXNAME) { read(0,p2.buf,MAXNAME); r.p2 -= MAXNAME; }
      read(0,p2.buf,r.p2);
      p2.buf[r.p2]=0;
    }
      
    res.p1  = 0;
    switch (r.fxn) {
    case ISBUILD:
	i = isbuild(p1.buf,r.len,&p2.desc,r.mode); break;
    case ISOPEN:
	i = isopen(p1.buf,r.mode);
	if (i >= 0) {
	  isindexinfo(i,&p1.desc,0);
	  iserrno = p1.dict.di_recsize;
	} break;
    case ISCLOSE:
	i = isclose(r.fd); break;
    case ISADDINDEX:
	i = isaddindex(r.fd,&p1.desc); break;
    case ISDELINDEX:
	i = isdelindex(r.fd,&p1.desc); break;
    case ISSTART:
	res.p1 = r.p1;
	i = isstart(r.fd,&p2.desc,r.len,p1.buf,r.mode); break;
    case ISREAD:
	res.p1 = r.p1;
	if (r.mode == ISLESS) {
	  r.mode = ISPREV;
	  if (i = isread(r.fd,p1.buf,ISGTEQ)) {
	    if (iserrno != ENOREC) break;
	    r.mode = ISLAST;
	  }
	}
	if (r.mode == ISLTEQ) {
	  r.mode = ISPREV;
	  if (i = isread(r.fd,p1.buf,ISGREAT)) {
	    if (iserrno != ENOREC) break;
	    r.mode = ISLAST;
	  }
	}
	i = isread(r.fd,p1.buf,r.mode); break;
    case ISWRITE:
	i = iswrite(r.fd,p1.buf); break;
    case ISREWRITE:
	i = isrewrite(r.fd,p1.buf); break;
    case ISWRCURR:
	i = iswrcurr(r.fd,p1.buf); break;
    case ISREWCURR:
	i = isrewcurr(r.fd,p1.buf); break;
    case ISDELETE:
	i = isdelete(r.fd,p1.buf); break;
    case ISDELCURR:
	i = isdelcurr(r.fd); break;
    case ISUNIQUEID:
	res.p1 = sizeof p1.id;
	i = isuniqueid(r.fd,&p1.id); break;
    case ISINDEXINFO:
	res.p1 = sizeof p1.dict;
	if (r.mode) {
	  res.p1 = sizeof p1.desc;
	}
	i = isindexinfo(r.fd,&p1.desc,r.mode); break;
    case ISAUDIT:
	i = isaudit(r.fd,p1.buf,r.mode); break;
    case ISERASE:
	i = iserase(p1.buf); break;
    case ISLOCK:
	i = islock(r.fd); break;
    case ISUNLOCK:
	i = isunlock(r.fd); break;
    case ISRELEASE:
	i = isrelease(r.fd); break;
    case ISRENAME:
	i = isrename(p1.buf,p2.buf); break;
    default:
	i = -1;
	iserrno = 118;
    }

    res.res = i;
    res.iserrno = iserrno;
    res.isstat1 = isstat1;
    res.isstat2 = isstat2;
    write(1,(char *)&res,sizeof res);
    if (res.p1) write(1,p1.buf,res.p1);
  }
  for (i=0;i<9;) isclose(i++);	/* close all files */
  return 0;
}
