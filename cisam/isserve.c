static char id[] = "@(#)isserve.c 1.3 2/9/94";

#include <isam.h>
#include "isreq.h"

main()
{
  REQ r;			/* request header */
  RES res;			/* result header */
  union { struct keydesc desc;
	  struct dictinfo dict;
	  char buf[MAXRLEN+1];
	  long id;
	} p1;		/* holds miscellany */
  union { struct keydesc desc;
	  char buf[MAXNAME+1];
	} p2;		/* holds miscellany */


  while (read(0,&r,sizeof r) == sizeof r) {
    if (r.p1) {
      while (r.p1 > MAXRLEN) { read(0,p1,MAXRLEN); r.p1 -= MAXRLEN; }
      read(0,p1.buf,r.p1);
      p1.buf[r.p1]=0;
    }
    if (r.p2) {
      while (r.p2 > MAXNAME) { read(0,p2,MAXNAME); r.p2 -= MAXNAME; }
      read(0,p2.buf,r.p2);
      p2.buf[r.p2]=0;
    }
      
    res.res = -1;
    res.p1  = 0;

    switch (r.fxn) {
    case ISBUILD:
	res.res = isbuild(p1.buf,r.len,&p2.desc,r.mode); break;
    case ISOPEN:
	res.res = isopen(p1.buf,r.mode);
	if (res.res >= 0) {
	  isindexinfo(res.res,&p1.dict,0);
	  iserrno = p1.dict.di_recsize;
	} break;
    case ISCLOSE:
	res.res = isclose(r.fd); break;
    case ISADDINDEX:
	res.res = isaddindex(r.fd,&p1.desc); break;
    case ISDELINDEX:
	res.res = isdelindex(r.fd,&p1.desc); break;
    case ISSTART:
	res.p1 = r.p1;
	res.res = isstart(r.fd,p2.desc,r.len,p1.buf,r.mode); break;
    case ISREAD:
	res.p1 = r.p1;
	res.res = isread(r.fd,p1.buf,r.mode); break;
    case ISWRITE:
	res.res = iswrite(r.fd,p1.buf); break;
    case ISREWRITE:
	res.res = isrewrite(r.fd,p1.buf); break;
    case ISWRCURR:
	res.res = iswrcurr(r.fd,p1.buf); break;
    case ISUNIQUEID:
	res.p1 = sizeof p1.id;
	res.res = isuniqueid(r.fd,&p1.id); break;
    case ISINDEXINFO:
	res.p1 = sizeof p1.dict;
	if (r.mode) {
	  res.p1 = sizeof p1.desc;
	}
	res.res = isindexinfo(r.fd,&p1.desc,r.mode); break;
    case ISERASE:
	res.res = iserase(p1.buf); break;
    case ISLOCK:
	res.res = islock(r.fd); break;
    case ISUNLOCK:
	res.res = isunlock(r.fd); break;
    case ISRELEASE:
	res.res = isrelease(r.fd); break;
    case ISRENAME:
	res.res = isrename(p1.buf,p2.buf); break;
    default:
	iserrno = 118;
    }

    res.iserrno = iserrno;
    res.isstat1 = isstat1;
    res.isstat2 = isstat2;
    write(1,&res,sizeof res);
    if (res.p1) write(1,p1.buf,res.p1);
  }
}
