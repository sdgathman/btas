/*
	Emulate C-isam(tm) functions using BTAS/2(tm) functions.

	Copyright 1990 Business Management Systems, Inc.
	Author: Stuart D. Gathman
 *
 * $Log$
 * Revision 1.11  2003/07/29 18:41:41  stuart
 * Check for fd avail before creating file in isbuildx.
 *
 * Revision 1.10  2003/07/29 18:15:39  stuart
 * Auto expand file descriptor array.
 *
 * Revision 1.9  2003/07/29 16:46:35  stuart
 * isfdlimit entry point.  Always use ischkfd.
 *
 * Revision 1.8  1999/06/03 18:47:57  stuart
 * When no .idx, try to make master keydesc match btas fields.
 *
 * Revision 1.7  1998/12/17  23:34:05  stuart
 * return user key length from iskeynorm
 * set unique klen when no idx
 *
 * Revision 1.6  1998/12/09  22:05:25  stuart
 * support directory open
 *
 * Revision 1.5  1998/02/02  20:01:55  stuart
 * Auto create key files
 * support min/max records
 *
 * Revision 1.4  1997/05/01  19:13:39  stuart
 * save index names for use by isstartn
 *
 * Revision 1.3  1994/02/24  20:05:23  stuart
 * Use last ISCLUSTER file, if any, as master
 *
 * Revision 1.2  1994/02/23  22:07:18  stuart
 * isflds entry
 *
 * Revision 1.1  1993/09/14  17:22:09  stuart
 * Initial revision
 *
 */

#include <string.h>
#include <errno.h>
#include <errenv.h>
#include <bterr.h>
#include "cisam.h"

struct btflds *isflds(int fd) {
  struct cisam *r = ischkfd(fd);
  if (!r) return 0;
  return r->key.f;
}

int iserrno, iserrio, issingleuser = 0;
char isstat1, isstat2;	/* not updated */
char *isversnumber = "$Revision$";
char *iscopyright  =
	"Copyright 1988,1989,1990 Business Management Systems, Inc.";
char *isserial	   = "000001";

static int fdlimit = 0;		/* limit max fd to this */
static int startsearch = 0;	/* start looking for free fd here. */
struct cisam **isamfdptr = 0;
int isamfdsize = 0;

int isfdlimit(int maxfds) {
  int old = fdlimit;
  if (maxfds <= MAXFILES && maxfds >= 0) {
    fdlimit = maxfds;
    return old;
  }
  return iserr(EBADARG);
}

int ismaperr(int code) {
  switch (code) {
  case BTERKEY: code = ENOREC; break;
  case BTEREOF: code = EENDFILE; break;
  case BTERDUP: code = EDUPL; break;
  case BTERLOCK: code = ELOCKED; break;
  }
  return iserr(code);	/* no mapping yet */
}

int isflush(int fd) {
  struct cisam *r = ischkfd(fd);
  if (r == 0) return iserr(ENOTOPEN);
  (void)btas(r->key.btcb,BTFLUSH);
  return 0;
}

int isclose(int fd) {
  struct cisam *ip;
  int rc;
  register struct cisam_key *kp;
  ip = ischkfd(fd);
  if (ip == 0) return iserr(ENOTOPEN);
  isamfdptr[fd] = 0;
  if (fd < startsearch)
    startsearch = fd;
  rc = btclose(ip->idx);
  rc = btclose(ip->dir);
  for (kp = &ip->key; kp; ) {
    struct cisam_key *cur = kp;
    rc = btclose(kp->btcb);
    if (kp->f)
      free((char *)kp->f);
    kp = kp->next;
    if (cur != &ip->key)
      free((char *)cur);
  }
  free(ip->min);
  free((char *)ip);
  return iserr(rc);
}

void cdecl iscloseall() {
  register int i;
  for (i = 0; i < isamfdsize; ++i)
    if (isamfdptr[i]) (void)isclose(i);
}

int isopen(const char *name,int mode) {
  return isopenx(name,mode,MAXCISAMREC);	/* no recsize protection */
}

int isnewfd() {
  for (;;) {
    int fd;
    struct cisam **newptr;
    struct cisam **oldptr;
    int newsize;
    int maxfd = isamfdsize;
    if (fdlimit && fdlimit < isamfdsize)
       maxfd = fdlimit;
    /* find free descriptor */
    for (fd = startsearch; fd < maxfd; ++fd) {
      if (isamfdptr[fd] == 0) {
	startsearch = fd;
	return fd;
      }
    }
    startsearch = maxfd;
    if (fdlimit && maxfd) return iserr(ETOOMANY);
    newsize = isamfdsize + MAXFILES;
    newptr = (struct cisam **)malloc(newsize * sizeof *newptr);
    if (newptr == 0) return iserr(ETOOMANY);
    oldptr = isamfdptr;
    for (fd = 0; fd < isamfdsize; ++fd)
      newptr[fd] = oldptr[fd];
    for (fd = isamfdsize; fd < newsize; ++fd)
      newptr[fd] = 0;
    isamfdptr = newptr;
    isamfdsize = newsize;
    free(oldptr);
  }
}

int isopenx(const char *name,int mode,int rlen) {
  static char installed = 0;
  struct cisam *cp;
  char ctl_name[128];
  char *np;
  int len, rc;
  int bmode = mode & 3;	/* BTAS data file mode */
  int cmode = (bmode == BTWRONLY) ? BTRDWR : bmode;
  int fd = isnewfd();

  if (fd < 0) return fd;

  if (mode & ISEXCLLOCK) bmode |= BTEXCL;
  cp = (struct cisam *)malloc(sizeof *cp);
  if (!cp) {
    return iserr(EBADMEM);
  }
  catch(rc)

  /* init malloced pointers to 0 so that isclose() can clean up
     after any errors.  This lets us errpost() at any time. */

  cp->key.next = 0;
  cp->key.f = 0;
  cp->key.btcb = 0;
  cp->recidx = 0;
  cp->idx = 0;
  cp->dir = 0;
  cp->f = 0;
  cp->start = ISFIRST;
  cp->curidx = &cp->key;	/* default to primary key */
  cp->min = 0;
  cp->max = 0;
  isamfdptr[fd] = cp;
  len = strlen(name);
  if (len > sizeof ctl_name - sizeof CTL_EXT)
    errpost(EFNAME);			/* name too long */
  strcpy(ctl_name,name);
  strcat(ctl_name+len,CTL_EXT);
  cp->idx = btopen(ctl_name,cmode + NOKEY,sizeof (struct fisam));
  np = strrchr(ctl_name,'/');
  if (np)
    ++np;
  else
    np = ctl_name;
  if (cp->idx->flags) {	/* generate keydesc's from idx file */
    struct cisam_key *kp = 0;
    cp->f = ldflds((struct btflds *)0, cp->idx->lbuf, cp->idx->rlen);
    if (cp->f == 0) errpost(EBADMEM);
    cp->idx->klen = 1;
    cp->idx->rlen =  sizeof (struct fisam);
    cp->idx->lbuf[0] = 0;	/* skip isunique record */
    while (btas(cp->idx,BTREADGT + NOKEY) == 0) {
      struct fisam f;		/* fisam record */
      b2urec(cp->f->f,(char *)&f,sizeof f,cp->idx->lbuf,cp->idx->rlen);
      if (kp) {
	struct cisam_key *p = (struct cisam_key *)malloc(sizeof *p);
	if (p == 0) errpost(EBADMEM);
	if (kp == cp->recidx)	/* make recidx last or first */
	  kp = &cp->key;
	if (f.flag & ISCLUSTER) {
	  *p = cp->key;
	  kp = &cp->key;
	  kp->next = p;
	}
	else {
	  p->next = kp->next;	/* add to chain */
	  kp->next = p;
	  kp = p;
	}
      }
      else
	kp = &cp->key;
      kp->btcb = 0;
      kp->f = 0;
      isldkey(kp->name,&kp->k,&f);

      if (kp->k.k_nparts == 0) {
	cp->recidx = kp;
	c_recno(cp) = 0L;
      }

      cp->idx->klen = cp->idx->rlen;	/* set to read next record */
      cp->idx->rlen = sizeof f;
    }

    for (kp = &cp->key; kp; kp = kp->next) {
      struct keydesc kn;
      kn = kp->k;
      kp->k.k_len = iskeynorm(&kn);
      strcpy(np,kp->name);

      if (kp == &cp->key) {
	/* the master records may be permuted to match the 
	   C-isam structure */
	struct btflds *fp;
	struct btfrec *p;
	short pos;
	kp->btcb = btopen(ctl_name,bmode,MAXREC);
	kp->f = ldflds((struct btflds *)0,kp->btcb->lbuf,kp->btcb->rlen);
	if (kp->f == 0) errpost(EBADMEM);
	/* permute field table to match user record */
	kp->f->klen = kp->f->rlen;		/* convert entire table */
	fp = isconvkey(kp->f,&kn,-1);
	if (fp == 0) errpost(EBADMEM);
	/* compute positions in user record */
	for (pos = 0,p = fp->f; p->pos = pos,p->type; pos += p->len,++p);
	free((char *)kp->f);
	fp->klen = fp->rlen;
	/* permute field table back to btas record order */
	kp->f = isconvkey(fp,&kn,1);
	free((char *)fp);			/* with user positions */
	kp->btcb = (BTCB *)realloc(kp->btcb,
			      sizeof *kp->btcb
			    - sizeof kp->btcb->lbuf
			    + btrlen(kp->f));
      }
      else {
	kp->f = isconvkey(cp->key.f,&kn,1);
	if (kp->f == 0) errpost(EBADMEM);
	kp->btcb = btopen(ctl_name, bmode | NOKEY, btrlen(kp->f));
	/* create any missing key fiels as empty */
	if (kp->btcb->flags == 0) {
	  btclose(kp->btcb);
	  kp->btcb = 0;
	  (void)btcreate(ctl_name,kp->f,0666);
	  kp->btcb = btopen(ctl_name, bmode, btrlen(kp->f));
	}
      }
      if (kn.k_flags&ISDUPS)
	kp->klen = btrlen(kp->f);
      else
	kp->klen = kn.k_len;		/* cisam key length */
      {
	char *p = strrchr(kp->name,'.');
	if (p && strcmp(p,".1") == 0)
	  cp->curidx = kp;			/* default to C-isam primary */
      }
    }

    if (!(bmode&BTEXCL)) {
      free((char *)cp->f);
      cp->f = 0;
    }
  }
  else {		/* no idx - use master field table */
    int klen;
    int kflds;
    struct keydesc *k = &cp->key.k;
    struct btflds *f;
    (void)btclose(cp->idx);
    cp->idx = 0;	/* isunique not possible */
    if (mode & ISDIROK) bmode |= BTDIROK;
    cp->key.btcb = btopen(name,bmode+NOKEY,rlen);
    if (cp->key.btcb->flags == 0)
      errpost(BTERKEY);	/* post here to avoid errdesc in btopen */
    if (cp->key.btcb->flags & BT_DIR) {
      const static char dirflds[] =
	{ 0, 1, BT_CHAR, 40, BT_NUM, 1, BT_BIN, 255 };
      cp->key.f = ldflds((struct btflds *)0,dirflds,sizeof dirflds);
    }
    else
      cp->key.f = ldflds((struct btflds *)0,
		      cp->key.btcb->lbuf,cp->key.btcb->rlen);
    if (cp->key.f == 0) errpost(EBADMEM);
    f = cp->key.f;
    /* should check for BT_RECNO . . . */
    /* Construct keydesc from btas fields. */
    k->k_flags = 0;		/* no dups on primary */
    kflds = f->klen;
    klen = f->f[kflds].pos;
    if (kflds <= NPARTS) {
      int fidx;
      k->k_nparts = kflds;
      for (fidx = 0; fidx < kflds; ++fidx) {
        struct keypart *kp = &k->k_part[fidx];
        kp->kp_start = f->f[fidx].pos;
        kp->kp_leng = f->f[fidx].len;
	kp->kp_type = CHARTYPE;
      }
    }
    else {	/* primary key normalized if too many key fields. */	
      k->k_nparts = 1;		
      k->k_start = 0;
      k->k_leng = klen;
      k->k_type = CHARTYPE;
    }
    k->k_len = klen;
    cp->key.klen = klen;
    ctl_name[len] = 0;
    strncpy(cp->key.name,np,MAXKEYNAME)[MAXKEYNAME] = 0;
  }
  enverr
    (void)isclose(fd);	/* close frees all control blocks */
    return ismaperr(rc);
  envend
  if (bmode&BTEXCL) {
    np[0] = 0;
    cp->dir = btopen(ctl_name,BTWRONLY+4,0);
  }
  cp->rlen = isrlen(cp->key.f);
  if (rlen < cp->rlen)
    cp->rlen = rlen;		/* check users record size */
  cp->klen = cp->key.k.k_len;
  iserrno = 0;
  if (!installed) {
    atexit(iscloseall);
    installed = 1;
  }
  return fd;
}

/* normalize keydesc */

int iskeynorm(struct keydesc *k) {
  register struct keypart *kp;
  register int i;
  k->k_flags &= ISDUPS;		/* only ISDUPS flag used with BTAS */
  if (k->k_nparts == 0) {
    k->k_len = 4;
    return 4;
  }
  kp = k->k_part;
  k->k_len = 0;
  for (i = 0; i < k->k_nparts; ++i) {
    k->k_len += k->k_part[i].kp_leng;
    if (i > 0 && kp[-1].kp_start + kp[-1].kp_leng == k->k_part[i].kp_start
	      && kp[-1].kp_type == k->k_part[i].kp_type)
      kp[-1].kp_leng += k->k_part[i].kp_leng;
    else
      *kp++ = k->k_part[i];
  }
  k->k_nparts = kp - k->k_part;
  return k->k_len;
}
