#include <isamx.h>
#include <btas.h>
#include <btflds.h>
#include "isreq.h"

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
  return n * 2;
}

/** Execute an encapsulated C-isam request.  Returns the C-isam result code
 * of the operation plus p1len, p1buf, and C-isam globals (iserrno, etc.).
 * This simplifies wrapping the library for RPC like protocols (isserve.c),
 * and Virtual Machines like Java.  P1 and p2 and variable length parameters,
 * for example a filename or a record buffer.
 * @param rfd	file descriptor 
 * @param fxn	function code, defined in isreq.h
 * @param p1lenp ptr to size of p1, replaced with variable result size.
 * @param p2len	size of p2
 * @param p1buf	buffer to hold p1 and variable result
 * @param p2buf	buffer to hold p2
 * @param mode	function modifier (for example, ISREAD uses ISFIRST)
 * @param len	length parameter (used by ISSTART for example)
 */

int isreq(int rfd,int fxn, int *p1lenp, int p2len,
    char *p1buf, const char *p2buf, int mode,int len) {
    int i;
    int p1len = *p1lenp;
    *p1lenp = 0;	// default to no variable result
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
	if (i) {
	  *p1lenp = 0;
	  break;
	}
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
