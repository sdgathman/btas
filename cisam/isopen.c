/*
	Emulate C-isam(tm) functions using BTAS/2(tm) functions.

	Copyright 1990 Business Management Systems, Inc.
	Author: Stuart D. Gathman
 * $Log$
 */

#include <string.h>
#include <errno.h>
#include <errenv.h>
#include <bterr.h>
#include "cisam.h"

int iserrno, iserrio, issingleuser = 0;
char isstat1, isstat2;	/* not updated */
char *isversnumber = "$Revision$";
char *iscopyright  =
	"Copyright 1988,1989,1990 Business Management Systems, Inc.";
char *isserial	   = "000001";

struct cisam *isamfd[MAXFILES];		/* max cisam files open */

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
  if (ischkfd(fd) == 0) return iserr(ENOTOPEN);
  (void)btas(isamfd[fd]->key.btcb,BTFLUSH);
  return 0;
}

int isclose(int fd) {
  struct cisam *ip;
  int rc;
  register struct cisam_key *kp;
  ip = ischkfd(fd);
  if (ip == 0) return iserr(ENOTOPEN);
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
  free((char *)ip);
  isamfd[fd] = 0;
  return iserr(rc);
}

void cdecl iscloseall() {
  register int i;
  for (i = 0; i < MAXFILES; ++i)
    if (isamfd[i]) (void)isclose(i);
}

int isopen(const char *name,int mode) {
  return isopenx(name,mode,MAXCISAMREC);	/* no recsize protection */
}

int isopenx(const char *name,int mode,int rlen) {
  int fd;

  /* find free descriptor */
  for (fd = 0; fd < MAXFILES; ++fd) {
    if (isamfd[fd] == 0) {
      static char installed = 0;
      struct cisam *cp;
      char ctl_name[128];
      int len, rc;
      int bmode = mode & 3;	/* BTAS data file mode */
      int cmode = (bmode == BTWRONLY) ? BTRDWR : bmode;

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
      isamfd[fd] = cp;
      len = strlen(name);
      if (len > sizeof ctl_name - sizeof CTL_EXT)
	errpost(EFNAME);			/* name too long */
      strcpy(ctl_name,name);
      strcat(ctl_name+len,CTL_EXT);
      cp->idx = btopen(ctl_name,cmode + NOKEY,sizeof (struct fisam));
      if (cp->idx->flags) {	/* generate keydesc's from idx file */
	struct cisam_key *kp = 0;
	char *np;
	if (np = strrchr(ctl_name,'/'))
	  ++np;
	else
	  np = ctl_name;
	cp->f = ldflds((struct btflds *)0, cp->idx->lbuf, cp->idx->rlen);
	if (cp->f == 0) errpost(EBADMEM);
	cp->idx->klen = 1;
	cp->idx->rlen =  sizeof (struct fisam);
	cp->idx->lbuf[0] = 0;	/* skip isunique record */
	while (btas(cp->idx,BTREADGT + NOKEY) == 0) {
	  struct keydesc kn;
	  struct fisam f;		/* fisam record */
	  b2urec(cp->f->f,(char *)&f,sizeof f,cp->idx->lbuf,cp->idx->rlen);
	  isldkey(np,&kn,&f);
	  if (kp) {
	    struct cisam_key *p = (struct cisam_key *)malloc(sizeof *p);
	    if (p == 0) errpost(EBADMEM);
	    if (kp == cp->recidx)	/* make recidx last or first */
	      kp = &cp->key;
	    p->btcb = 0;
	    p->next = kp->next;	/* add to chain */
	    kp->next = p;
	    kp = p;
	  }
	  else kp = &cp->key;

	  kp->k = kn;
	  iskeynorm(&kn);
	  kp->k.k_len = kn.k_len;

	  if (kn.k_nparts == 0) {
	    cp->recidx = kp;
	    c_recno(cp) = 0L;
	  }

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
	    kp->btcb = btopen(ctl_name,bmode,btrlen(kp->f));
	  }
	  if (kn.k_flags&ISDUPS)
	    kp->klen = btrlen(kp->f);
	  else
	    kp->klen = kn.k_len;		/* cisam key length */
	  {
	    char *p = strrchr(cp->idx->lbuf,'.');
	    if (p && strcmp(p,".1") == 0)
	      cp->curidx = kp;			/* default to C-isam primary */
	  }
	  cp->idx->klen = cp->idx->rlen;	/* set to read next record */
	  cp->idx->rlen = sizeof f;
	}
	if (bmode&BTEXCL) {
	  np[0] = 0;
	  cp->dir = btopen(ctl_name,BTWRONLY+4,0);
	}
	else {
	  free((char *)cp->f);
	  cp->f = 0;
	}
      }
      else {		/* no idx - use master field table */
	(void)btclose(cp->idx);
	cp->idx = 0;	/* isunique not possible */
	cp->key.btcb = btopen(name,bmode+NOKEY,rlen);
	if (cp->key.btcb->flags == 0)
	  errpost(BTERKEY);	/* post here to avoid errdesc in btopen */
	cp->key.f = ldflds((struct btflds *)0,
			  cp->key.btcb->lbuf,cp->key.btcb->rlen);
	if (cp->key.f == 0) errpost(EBADMEM);
	/* should check for BT_RECNO . . . */
	cp->key.k.k_flags = 0;		/* no dups on primary */
	cp->key.k.k_nparts = 1;		/* primary key normalized */
	cp->key.k.k_start = 0;
	cp->key.k.k_leng = cp->key.f->f[cp->key.f->klen].pos;
	cp->key.k.k_len = cp->key.klen = cp->key.k.k_leng;
	cp->key.k.k_type = CHARTYPE;
      }
      enverr
	(void)isclose(fd);	/* close frees all control blocks */
	return ismaperr(rc);
      envend
      cp->rlen = rlen;		/* save users record size */
      cp->klen = cp->key.k.k_len;
      iserrno = 0;
      if (!installed) {
	atexit(iscloseall);
	installed = 1;
      }
      return fd;
    }
  }
  return iserr(ETOOMANY);
}

/* normalize keydesc */

void iskeynorm(k)
  struct keydesc *k;
{
  register struct keypart *kp;
  register int i;
  k->k_flags &= ISDUPS;		/* only ISDUPS flag used with BTAS */
  if (k->k_nparts == 0) {
    k->k_len = 4;
    return;
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
}
