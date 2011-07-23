/*
 * $Log$
 * Revision 1.6  2003/07/29 18:41:41  stuart
 * Check for fd avail before creating file in isbuildx.
 *
 * Revision 1.5  2003/07/29 18:15:38  stuart
 * Auto expand file descriptor array.
 *
 * Revision 1.4  2001/02/28 23:14:39  stuart
 * better handling of recno key
 *
 * Revision 1.3  1998/12/17  23:56:12  stuart
 * build with rlen == 0 in any directory (bug fix)
 * try to make isopen get correct klen for rlen == 0,
 * probably better fixed in isaddflds()
 *
 * Revision 1.2  1998/12/04  22:30:39  stuart
 * Mark primary key in .idx
 *
 * Revision 1.1  1995/04/07  22:52:21  stuart
 * Initial revision
 *
 */
#include <errenv.h>
#include <string.h>
#include "cisam.h"

/* construct default field table from key description */

static struct btflds *makeflds(const struct keydesc *k,int rlen,int num) {
  static struct {
    unsigned char klen,rlen;
    struct btfrec f[18];
  } fcb;
  struct btfrec *f;
  int pos;
  f = fcb.f;
  for (pos = 0; pos < rlen; pos += f->len, ++f) {
    register int i;
    f->pos = pos;
    f->type = BT_BIN;		/* dump in hex */
    if (rlen - pos >= MAXKEY)
      f->len = MAXKEY - 1;
    else
      f->len = rlen - pos;
    for (i = 0; i < k->k_nparts; ++i)
      if (k->k_part[i].kp_start < pos + f->len
	&& k->k_part[i].kp_start >= pos) {
        f->len = k->k_part[i].kp_start - pos;
	if (f->len == 0) {
	  f->len = k->k_part[i].kp_leng;
	  break;
	}
      }
  }
  if (num) {
    f->type = BT_RECNO;
    f->len  = num;
    f->pos  = rlen;
    ++f;
    rlen += num;
  }
  f->type = f->len = 0;
  f->pos = rlen;
  fcb.klen = fcb.rlen = f - fcb.f;
  return (struct btflds *)&fcb;
}

int isbuildx(
  const char *name,     	/* filename to create */
  int rlen,			/* C-isam record length */
  const struct keydesc *k,	/* primary keydesc */
  int mode,			/* file mode */
  struct btflds *f)		/* field table for C-isam record */
{
  struct btflds *fcb;
  struct keydesc kd;
  char idxname[128];
  struct keydesc kr;
  char needrecnum;	// true if recnum key needed
  int rc, len = strlen(name);
  const char *fname;
  if (len + sizeof CTL_EXT > sizeof idxname) return iserr(EFNAME);
  if (k->k_flags & ISDUPS) return iserr(EBADKEY);
  if (isnewfd() < 0) return -1;
  kd = *k;
  iskeynorm(&kd);

  strcpy(idxname,name);
  fname = basename(idxname);

  // check if user field table includes a recno field
  needrecnum = (kd.k_nparts > 0 && f && isrlen(f) != btrlen(f));

  /* create .idx file if needed */
  if (needrecnum || kd.k_nparts != 1 || kd.k_start > 0 || mode == 0) {
    static const char idxf[] = { 0, 1,		/* idxname, klen */
      BT_CHAR, 32, BT_NUM, 1, BT_NUM,  1,	/* name, ISDUPS, k_nparts */
      BT_NUM, 2, BT_NUM, 2, BT_NUM, 2,		/* pos, len, type */
      BT_NUM, 2, BT_NUM, 2, BT_NUM, 2,
      BT_NUM, 2, BT_NUM, 2, BT_NUM, 2,
      BT_NUM, 2, BT_NUM, 2, BT_NUM, 2,
      BT_NUM, 2, BT_NUM, 2, BT_NUM, 2,
      BT_NUM, 2, BT_NUM, 2, BT_NUM, 2,
      BT_NUM, 2, BT_NUM, 2, BT_NUM, 2,
      BT_NUM, 2, BT_NUM, 2, BT_NUM, 2
    };
    BTCB * volatile ctlf = 0;
    struct fisam idxrec;
    fcb = ldflds((struct btflds *)0,idxf,sizeof idxf);
    if (fcb == 0) return iserr(EBADMEM);
    catch(rc)
      strcpy(idxname+len,CTL_EXT);
      rc = btcreate(idxname,fcb,0666);	/* create .idx file */
      if (rc) errpost(rc);
      ctlf = btopen(idxname,BTRDWR,sizeof idxrec);
      idxname[len] = 0;
      isstkey(fname,k,&idxrec);
      idxrec.flag |= ISCLUSTER;	/* mark as primary key */
      u2brec(fcb->f,(char *)&idxrec,sizeof idxrec,ctlf,sizeof idxrec.name);
      btas(ctlf,BTWRITE);	/* write primary key record */
      if (needrecnum) {
	/* create isrecnum key */
	strcpy(idxname + len,".num");
	kr.k_flags = kr.k_nparts = 0;
	isstkey(fname,&kr,&idxrec);
	u2brec(fcb->f,(char *)&idxrec,sizeof idxrec,ctlf,sizeof idxrec.name);
	btas(ctlf,BTWRITE);	/* write isrecnum key record */
      }
    envend
    free((char *)fcb);
    (void)btclose(ctlf);
    if (rc) return ismaperr(rc);
  }

  /* create default field table if needed.  Use logical key length when
     rlen == 0 so that isopen will get a reasonably correct primary key.
     A better solution might be to patch in the users original
     keydesc after the call to isopenx() at the end.
   */
  if (!f) f = makeflds(k,rlen ? rlen : kd.k_len,!k->k_nparts?4:0);

  /* create files */
  if (needrecnum) {		/* if recidx created */
    fcb = isconvkey(f,&kr,1);
    if (fcb == 0) return iserr(EBADMEM);
    rc = btcreate(idxname,fcb,0666);	/* create recnum file */
    free((PTR)fcb);
    /* we ought to delete ctlf on error */
    if (rc) return ismaperr(rc);
  }
  rc = f->klen;			/* save klen */
  f->klen = f->rlen;		/* convert all fields */
  fcb = isconvkey(f,&kd,1);	/* move key to front of record */
  f->klen = rc;			/* restore klen */
  if (fcb == 0) return iserr(EBADMEM);
  idxname[len] = 0;
  rc = btcreate(idxname,fcb,0666);	/* create file */
  free((PTR)fcb);
  /* FIXME: we ought to delete ctlf on error, but isaddindex depends
    on keeping it to auto create a .idx */
  if (rc) return ismaperr(rc);
  return isopenx(name,mode,rlen);
}

int isbuild(name,rlen,k,mode)
  const char *name;
  int rlen;
  const struct keydesc *k;
  int mode;
{
  return isbuildx(name,rlen,k,mode,makeflds(k,rlen,4));
}
