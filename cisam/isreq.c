#include <isamx.h>
#include <btas.h>
#include "isreq.h"

/** Execute an encapsulated C-isam request.  Returns the C-isam result code
 * of the operation plus p1len, p1buf, and C-isam globals (iserrno, etc.).
 * This simplifies wrapping the library for RPC like protocols (isserve.c),
 * and Virtual Machines like Java.
 */

int isreq(int rfd,enum isreqOp fxn, int *p1lenp, int p2len,
    char *p1buf, char *p2buf, int mode,int len) {
    int i;
    int p1len = *p1lenp;
    *p1lenp = 0;
    switch (fxn) {
      union {
	struct keydesc desc;
	struct dictinfo dict;
      } d;
    case ISBUILD:
	ldkeydesc(&d.desc,p2buf);
	if (len == 0)
	  i = isbuildx(p1buf,len,&d.desc,mode,0);
	else
	  i = isbuild(p1buf,len,&d.desc,mode);
	break;
    case ISOPEN:
	i = isopen(p1buf,mode);
	if (i >= 0) {
	  isindexinfo(i,&d.desc,0);
	  iserrno = d.dict.di_recsize;
	}
	break;
    case ISCLOSE:
	i = isclose(rfd);
	break;
    case ISADDINDEX:
	ldkeydesc(&d.desc,p1buf);
	i = isaddindexn(rfd,&d.desc,(p2len == 0) ? 0 : p2buf);
	break;
    case ISDELINDEX:
	ldkeydesc(&d.desc,p1buf);
	i = isdelindex(rfd,&d.desc);
	break;
    case ISSTART:
	ldkeydesc(&d.desc,p2buf);
	isrange(rfd,0,0);
	i = isstart(rfd,&d.desc,len,p1buf,mode);
	break;
    case ISSTARTN:
	isrange(rfd,0,0);
	i = isstartn(rfd,p2buf,len,p1buf,mode);
	break;
    case ISREAD: case ISREADREC:
	if (p1len == 0)	/* Allow client to skip key bytes for FIRST,NEXT,etc. */
	  p1len = isreclen(rfd);
	else if (p1len >= isreclen(rfd) + 4) {
	  p1len -= 4;
	  isrecnum = ldlong(p1buf + p1len);
	}
	else	/* Allow client to send bytes needed for key info only. */
	  p1len = isreclen(rfd);
	switch (mode) {
	case ISLESS:
	  mode = ISPREV;
	  i = isread(rfd,p1buf,ISGTEQ);
	  if (i) {
	    if (iserrno != ENOREC) break;
	    mode = ISLAST;
	  }
	  break;
	case ISLTEQ:
	  mode = ISPREV;
	  i = isread(rfd,p1buf,ISGREAT);
	  if (i) {
	    if (iserrno != ENOREC) break;
	    mode = ISLAST;
	  }
	  break;
	}
	i = isread(rfd,p1buf,mode);
	if (fxn == ISREADREC) {
	  stlong(isrecnum,p1buf + p1len);
	  p1len += 4;
	}
	*p1lenp = p1len;
	break;
    case ISWRITE:
	i = iswrite(rfd,p1buf);
	break;
    case ISREWRITE:
	if (p1len >= isreclen(rfd) + 4) {
	  p1len -= 4;
	  isrecnum = ldlong(p1buf + p1len);
	}
	i = isrewrite(rfd,p1buf);
	break;
    case ISWRCURR:
	i = iswrcurr(rfd,p1buf);
	break;
    case ISREWCURR:
	i = isrewcurr(rfd,p1buf);
	break;
    case ISREWREC:
	i = isrewrec(rfd,isrecnum,p1buf);
	break;
    case ISDELREC:
	i = isdelrec(rfd,isrecnum);
	break;
    case ISDELETE:
	if (p1len >= isreclen(rfd) + 4) {
	  p1len -= 4;
	  isrecnum = ldlong(p1buf + p1len);
	}
	i = isdelete(rfd,p1buf);
	break;
    case ISDELCURR:
	i = isdelcurr(rfd);
	break;
    case ISUNIQUEID: {
	long id;
	i = isuniqueid(rfd,&id);
	stlong(id,p1buf);
	*p1lenp = 4;
	break;
      }
    case ISINDEXINFO:
	i = isindexinfo(rfd,&d.desc,mode);
	if (mode)
	  *p1lenp = stkeydesc(&d.desc,p1buf);
	else
	  *p1lenp = stdictinfo(&d.dict,p1buf);
	break;
    case ISINDEXNAME:
	i = isindexname(rfd,p1buf,mode);
	*p1lenp = strlen(p1buf);
	break;
    case ISAUDIT:
	i = isaudit(rfd,p1buf,mode);
	break;
    case ISERASE:
	i = iserase(p1buf);
	break;
    case ISLOCKOP:
	i = islock(rfd);
	break;
    case ISUNLOCK:
	i = isunlock(rfd);
	break;
    case ISRELEASE:
	i = isrelease(rfd);
	break;
    case ISRENAME:
	i = isrename(p1buf,p2buf);
	break;
    case ISSETRANGE:
	i = isrange(rfd,p1len ? p1buf : 0,p2len ? p2buf : 0);
	break;
    case ISADDFLDS:
	i = addflds(rfd,p1buf,p1len);
	break;
    case ISGETFLDS:
	i = getflds(rfd,p1buf);
	*p1lenp = i;
	i = 0;
	break;
    case ISMKDIR:
      if (mode < 0)
        i = btrmdir(p1buf);
      else
        i = btmkdir(p1buf,mode);
      if (i) {
        iserrno = i;
	i = -1;
      }
      break;
    default:
	i = -1;
	iserrno = 118;
    }
    return i;
}
