/*
	Create "archives" of BTAS/2 files on a sequential file.
	These archives are designed to be portable to any computer.

	FIXME: load() should continue reading archive after some (but
	not all) errors.
 *
 * $Log$
 * Revision 1.5  1998/05/13  04:19:16  stuart
 * compute maxdat with btfsstat
 *
 * Revision 1.4  1998/04/08  22:03:24  stuart
 * Extended btar_extract options
 *
 * Revision 1.3  1997/09/10  21:39:20  stuart
 * make intermediate directories
 *
 * Revision 1.2  1996/01/05  01:37:22  stuart
 * fix many bugs
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errenv.h>
#include <string.h>
#include <btflds.h>
#include <bttype.h>
#include "btar.h"
#include <ftype.h>	/* portable file formats */
#include <block.h>
#include <bterr.h>

extern volatile int cancel;

static int maxdat = btmaxrec(1024);

struct ar_hdr;

static int arcone(const char *, const struct btstat *);
static unsigned chksum(const char *, int);
static void getbuf(PTR, int);
static int gethdr(struct ar_hdr *, PTR, int);
static int getdata(char *, int);

static void putbuf(const PTR, int);
static void puthdr(struct ar_hdr *, const PTR, int);
static void putdata(const char *, int, int);
static void puteof(void);

/* archive structure:

	optional chdir record
	one or more file structures
	repeat as needed . . .

   file structure:

	file status record
	data records { FSHORT rlen; uchar duplen; char rec[]; }
	eof record { FSHORT -1; }
*/

struct fstat {		/* portable status structure */
  FLONG rcnt;		/* record count / file size - unreliable if next == 0 */
  FLONG mtime;		/* time modified */
  FSHORT link;		/* link id - zero if none or unknown */
  FSHORT user;
  FSHORT group;
  FSHORT mode;
  char rec[MAXREC];	/* variable length directory record */
};

struct ar_hdr {		/* archive header */
  FSHORT rlen;		/* total length of header */
  FSHORT chksum;	/* makes 16-bit chksum of header zero */
  FSHORT magic;		/* type of header */
  FLONG next;		/* offset to next header of same type - or zero */
};

struct rec_hdr {	/* data record header */
  FSHORT rlen;
  unsigned char dup;
};

#define fstat_MAGIC	0157
#define chdir_MAGIC	0357

/* The archive file needs to be buffered since tapes require a physical
	block size. */

static FILE *f;		/* the current archive file */
static int seekable, dironly;

/* The checksum in the header records helps detect corrupted archives
   and to locate the next valid header */

static unsigned chksum(const char *buf,int len) {
  unsigned sum = 0;
  while (len--) sum += (*buf++ & 0xff) << (len & 1) * 8;
  return sum;
}

/* Reading archive objects */

/* read raw bytes with error checking */

static void getbuf(PTR buf,int len) {
  int rc;
  rc = fread(buf,1,len,f);	/* get fixed part */
  if (rc != len) {
    rc = errno;
    perror("Archive input");
    errpost(200);
  }
}
  
/* read a header record */

static int gethdr(struct ar_hdr *hdr,PTR dat,int len) {
  int rlen;
  getbuf(hdr->rlen,2);
  rlen = ldshort(hdr->rlen);
  if (rlen < 10) return -1;
  rlen -= 10;
  if (rlen > len) return -1;
  getbuf(hdr->rlen + 2, 8);	/* read remainder of header */
  getbuf(dat,rlen);		/* read extension */
  if (0xffff & (chksum((PTR)hdr,10) + chksum(dat,rlen))) return -1;
  return rlen;			/* return size of extension */
}

/* read a data record */

static int getdata(char *buf,int len) {
  int rlen;
  struct rec_hdr rechdr;
  getbuf((PTR)&rechdr,3);
  rlen = ldshort(rechdr.rlen);
  if (rlen < 3) {
    ungetc(rechdr.dup,f);
    return -1;
  }
  rlen = rlen + rechdr.dup - 3;	/* logical record size */
  if (len < rlen) return -1;	/* huge archive record */
  getbuf(buf + rechdr.dup,rlen - rechdr.dup);
  return rlen;	/* return logical record size */
}

/* Output archive objects */

/* output raw bytes with error checking */

static void putbuf(const PTR buf,int len) {
  int rc;
  rc = fwrite(buf,1,len,f);
  if (rc != len) {
    rc = errno;
    perror("Archive output");
    errpost(rc);
  }
}

/* output archive header */
static void puthdr(struct ar_hdr *hdr,const PTR dat,int len) {
  stshort(len + 10,hdr->rlen);
  stshort(0,hdr->chksum);
  stshort(-(chksum((PTR)hdr,10) + chksum(dat,len)),hdr->chksum);
  putbuf((PTR)hdr,10);
  putbuf(dat,len);
}

/* output data record */
static void putdata(const char *buf,int len,int prevlen) {
  static unsigned char prevkey[MAXKEY];
  struct rec_hdr rechdr;
  rechdr.dup = blkcmp((PTR)buf,prevkey,(len > prevlen) ? prevlen : len);
  stshort(len - rechdr.dup + 3,rechdr.rlen);
  putbuf((PTR)&rechdr,3);
  putbuf(buf+rechdr.dup,len - rechdr.dup);
  if (len > MAXKEY) len = MAXKEY;
  memcpy(prevkey+rechdr.dup,buf+rechdr.dup,len-rechdr.dup);
}

static void puteof() {
  char buf[2];
  stshort(-1,buf);
  putbuf(buf,2);
}

/* OK, the high level logic at last! */

static struct {
  long pos;
  short len;
  char name[MAXKEY];
} lastpath;

int btar_opennew(const char *t,int seekflag) {
  /* char buf[8192];		/* big I/O buffer */
  f = t ? fopen(t,"wb") : stdout;
  seekable = seekflag;
  if (f == 0 /* || setvbuf(f,buf,_IOFBF,sizeof buf) */) {
    perror(t);
    return -1;
  }
  /* this test does not really work on System V.  How can we test for
     a seekable device? */
  if (seekable)	/* verify that device seems to really support seeks */
    seekable = !lseek(fileno(f),0L,0);
  if (!seekable)
    fprintf(stderr,
      "Media does not support seeks.  Archive will be in sequential format.\n"
    );
  lastpath.len = 0;
  lastpath.pos = -1L;
  return 0;
}

int btar_close() {
  puteof();
  return fclose(f);
}

static const char *ffdirname;

int btar_add(const char *s,int dirflag,int subdirs) {
  int rc;
  struct btff ff;
  dironly = dirflag;
  catch(rc)
    rc = findfirst(s,&ff);
    if (rc)
      fprintf(stderr,"%s: not found\n",s);
    while (rc == 0 && cancel == 0) {
      if (subdirs) {
	if (strcmp(ff.b->lbuf,"..") && strcmp(ff.b->lbuf,"."))
	  btwalk(ff.b->lbuf,arcone);
      }
      else {
	ffdirname = ff.dir;
	arcone(ff.b->lbuf,0);
      }
      rc = findnext(&ff);
    }
    rc = 0;
  enverr
    finddone(&ff);
  envend
  return rc;
}

static BTCB *checkMaxdat(BTCB *dtf) {
  struct btfs f;
  if (btfsstat(dtf->mid,&f) == 0) {
    int nmax = btmaxrec(f.hdr.blksize);
    if (nmax > maxdat) {
      maxdat = nmax;
      dtf = realloc(dtf,btsize(nmax));
      if (!dtf)
	errpost(ENOMEM);
    }
  }
  return dtf;
}

static int arcone(const char *name,const struct btstat *stp) {
  int rc,len;
  BTCB * volatile dtf = 0;
  long recs = 0L;
  long pos, newpos;
  struct ar_hdr hdr;
  struct fstat st;
  struct btstat s;
  int plen;

  if (cancel) return -1;
  fprintf(stderr,"%s: ",name);
  catch(rc)
  dtf = btopen(name,BTRDONLY+4,maxdat);
  len = dtf->rlen;
  memcpy(st.rec,dtf->lbuf,len);	/* save dir record */

  /* get file stats */
  dtf->klen = 0;
  dtf->rlen = sizeof s;
  if (stp) {
    const char *p = strrchr(name,'/');
    plen = p ? p - name : 0;
    s = *stp;
  }
  else {
    name = ffdirname;
    plen = name ? strlen(name) : 0;
    rc = btas(dtf,BTSTAT);
    if (rc) {
      fputs("couldn't stat, ",stderr);
      btclose(dtf);
      dtf = 0;
      errpost(rc);
    }
    memcpy((char *)&s,dtf->lbuf,sizeof s);
    if (s.id.mode & BT_DIR) {
      btclose(dtf);
      dtf = 0;
      errpost(BTERDIR);
    }
  }

  {
    /* check if chdir needed */
    if (plen != lastpath.len || memcmp(lastpath.name,name,plen)) {
      stshort(chdir_MAGIC,hdr.magic);
      newpos = ftell(f);
      if (lastpath.pos >= 0L && seekable) {
	if (fseek(f,lastpath.pos,0))	/* file is seekable */
	  perror("fseek");
	else {
	  stlong(newpos,hdr.next);		/* link to next header */
	  puthdr(&hdr,lastpath.name,lastpath.len);
	  fseek(f,newpos,0);
	}
      }
      stlong(0L,hdr.next);
      memcpy(lastpath.name,name,plen);
      lastpath.len = plen;
      lastpath.pos = newpos;		/* save current position */
      puthdr(&hdr,name,plen);		/* output chdir */
    }
  }
  /* build portable stat structure */

  stlong(s.rcnt,st.rcnt);
  stlong(s.mtime,st.mtime);
  stshort(0,st.link);		/* no links yet */
  stshort(s.id.user,st.user);
  stshort(s.id.group,st.group);
  stshort(s.id.mode,st.mode);

  stshort(fstat_MAGIC,hdr.magic);
  stlong(0L,hdr.next);

  /* output directory record */
  pos = ftell(f);		/* save current position */
  puthdr(&hdr,(PTR)&st,sizeof st - sizeof st.rec + len);

  /* output data records */
  if (!(s.id.mode & BT_DIR) && !dironly) {
    dtf = checkMaxdat(dtf);
    dtf->klen = 0;
    dtf->rlen = maxdat;
    rc = btas(dtf,BTREADGE+NOKEY);
    while (rc == 0 && cancel == 0) {
      putdata(dtf->lbuf,dtf->rlen,dtf->klen);
      ++recs;
      dtf->klen = dtf->rlen;
      dtf->rlen = maxdat;
      rc = btas(dtf,BTREADGT+NOKEY);
    }
  }
  enverr
    switch (rc) {
    case BTERPERM:
      fputs("permission denied",stderr);
      break;
    case BTERDIR:
      fputs("directory",stderr);
      break;
    default:
      fprintf(stderr,"error code %d",rc);
    }
    if (!dtf) {	/* we never even started this file! */
      fputc('\n',stderr);
      return 0;	/* but we still want to continue with the next file */
    }
    fputs(", ",stderr);
  envend
  btclose(dtf);

  puteof();
  newpos = ftell(f);
  if (seekable)
    if (fseek(f,pos,0))		/* file is seekable */
      perror("fseek");
    else {
      stlong(recs,st.rcnt);		/* accurate record count */
      stlong(newpos,hdr.next);		/* link to next header */
      puthdr(&hdr,(PTR)&st,sizeof st - sizeof st.rec + len);
      (void)fseek(f,newpos,0);
    }
  fprintf(stderr,"%ld records archived\n",recs);
  return 0;		/* successful */
}

/* open directory, recursively creating parents if required */
static int btmkdirp(char *dir,int mode) {
  int rc = btmkdir(dir,mode);
  char *p;
  if (rc != BTERKEY) return rc;
  p = strrchr(dir,'/');
  if (!p || p == dir) return rc;
  *p = 0;
  btmkdirp(dir,mode);
  *p = '/';
  return btmkdir(dir,mode);
}

static BTCB *b = 0;

int btar_extract(
  const char *dir,const char *drec,int rlen,const struct btstat *bst) {
  return btar_extractx(dir,drec,rlen,bst,0);
}

int btar_extractx(
  const char *dir,const char *drec,int rlen,const struct btstat *bst,int flg) {
  BTCB *savdir = btasdir;
  BTCB *dtf = 0;
  struct btflds *srctbl = 0;
  struct btflds *dsttbl = 0;
  int urlen = 0;
  int uklen = 0;
  char *buf = 0;
  int dirlen = strlen(dir);
  int rc;
  long recs = 0L, dups = 0L;
  volatile int done = 0;
  catch (rc)
  if (dirlen != lastpath.len || memcmp(dir,lastpath.name,dirlen)) {
    btclose(b);
    b = 0;
  }
  if (!b) {
    b = btopen(dir,BTWRONLY+BTDIROK+NOKEY,MAXREC);
    if (!b->flags) {
      char pdir[MAXKEY + 1];
      strcpy(pdir,dir);
      btmkdirp(pdir,0777);
      btopenf(b,dir,BTWRONLY+BTDIROK,MAXREC);
    }
    memcpy(lastpath.name,dir,dirlen);
    lastpath.len = dirlen;
  }
  btasdir = b;
  if ((flg&FLAG_REPLACE) && !(flg&FLAG_FIELDS) && !(bst->id.mode&BT_DIR)) {
    memcpy(b->lbuf,drec,(unsigned)rlen);
    b->klen = strlen(drec) + 1;
    b->rlen = rlen;
    rc = btas(b,BTDELETE+NOKEY);
  }
  /* make sure we can write to it */
  b->u.id.user = geteuid();
  b->u.id.group = getegid();
  if (bst->id.mode & BT_DIR)
    b->u.id.mode = 0700 + BT_DIR;
  else
    b->u.id.mode = 0600;
  memcpy(b->lbuf,drec,(unsigned)rlen);
  b->rlen = rlen;
  b->klen = strlen(drec) + 1;
  rc = btas(b,BTCREATE+DUPKEY);
  if (rc) {
    if (flg & FLAG_FIELDS) {
      if (flg & FLAG_REPLACE)
	fprintf(stderr," rewriting, ");
      else
	fprintf(stderr," converting, ");
    }
    else
      fprintf(stderr," merging, ");
  }
  else {
    flg &= ~FLAG_FIELDS;
    if (flg & FLAG_REPLACE)
      fprintf(stderr," replacing,  ");
    else
      fprintf(stderr," creating,  ");
    flg &= ~FLAG_REPLACE;
  }
  dtf = btopen(b->lbuf,BTWRONLY+BTDIROK,maxdat);
  dtf = checkMaxdat(dtf);
  if (dtf) {
    if ((bst->id.mode & BT_DIR) && rc == 0) {
      strcpy(dtf->lbuf,".");
      dtf->klen = dtf->rlen = 2;
      dtf->u.cache.node = dtf->root;
      btas(dtf,BTLINK);
      strcpy(dtf->lbuf,"..");
      dtf->klen = dtf->rlen = 3;
      dtf->u.cache.node = b->root;
      btas(dtf,BTLINK);
    }
    if (flg&FLAG_FIELDS) {
      srctbl = ldflds(0,drec,rlen);
      dsttbl = ldflds(0,dtf->lbuf,dtf->rlen);
      urlen = btrlen(srctbl);
      uklen = dsttbl->f[dsttbl->klen].pos;
    }
    buf = alloca(urlen);
    while ( (dtf->rlen = getdata(dtf->lbuf,maxdat)) > 0) {
      if (flg&FLAG_FIELDS) {
	b2urec(srctbl->f,buf,urlen,dtf->lbuf,dtf->rlen);
	u2brec(dsttbl->f,buf,urlen,dtf,uklen);
      }
      else
	dtf->klen = dtf->rlen;
      if (btas(dtf,BTWRITE+DUPKEY)) {
	++dups;
	if (flg&FLAG_REPLACE)
	  btas(dtf,BTREPLACE+NOKEY);
      }
      ++recs;
    }
    done = 1;
    memcpy(dtf->lbuf,bst,sizeof *bst);
    dtf->klen = dtf->rlen = sizeof *bst;
    dtf->u.id.user = geteuid();
    rc = btas(dtf,BTTOUCH);
  }
  enverr
    if (!done) {	// skip 
      long skipped = 0L;
      while (getdata(dtf->lbuf,maxdat) > 0)
	++skipped;
      fprintf(stderr,"	%ld records skipped.\n",skipped);
    }
    switch (rc) {
    case 202:
      fprintf(stderr,"out of BTAS space");
      cancel = 1;
      break;
    default:
      fprintf(stderr,"error %d",rc);
    }
    fprintf(stderr,", permissions not restored.\n");
  envend
  btasdir = savdir;
  btclose(dtf);
  if (srctbl) free(srctbl);
  if (dsttbl) free(dsttbl);
  fprintf(stderr,"%ld records loaded, %ld duplicates\n",recs,dups);
  return rc;
}


static struct ar_hdr hdr;

long btar_skip(long recs) {
  long pos = ldlong(hdr.next);
  if (!pos || fseek(f,pos,0)) {		/* if no seek possible */
    char buf[maxdat];
    recs = 0;
    while (getdata(buf,maxdat) >= 0)
      ++recs;
  }
  return recs;
}

int btar_load(const char *t,loadf *userf) {
  /* char buf[8192]; */
  union {
    struct fstat st;
    char dir[MAXREC];
  } u;
  static char prefix[MAXREC];
  int rc, rlen;
  f = t ? fopen(t,"rb") : stdin;
  if (!f /* || setvbuf(f,buf,_IOFBF,sizeof buf) */) {
    perror(t);
    return -1;
  }
  catch(rc)
  while ((rlen = gethdr(&hdr,u.dir,sizeof u)) >= 0) {
    int magic = ldshort(hdr.magic);
    if (magic == chdir_MAGIC) {
      u.dir[rlen] = 0;
      strcpy(prefix,u.dir);
    }
    else if (magic == fstat_MAGIC) {
      struct btstat bst;
      rlen -= sizeof u.st - sizeof u.st.rec;
      /* save original permissions */
      bst.id.user = ldshort(u.st.user);
      bst.id.group = ldshort(u.st.group);
      bst.id.mode = ldshort(u.st.mode);
      bst.atime = bst.mtime = ldlong(u.st.mtime);
      bst.rcnt = ldlong(u.st.rcnt);
      bst.bcnt = 0L;
      if (userf(prefix,u.st.rec,rlen,&bst)) break;
    }
    else {
      fprintf(stderr,"bad magic\n");
      break;
    }
  }
  enverr
    switch (rc) {
    case BTERPERM:
      fputs("permission denied\n",stderr);
      break;
    default:
      fprintf(stderr,"error code %d\n",rc);
      break;
    }
  envend
  btclose(b);
  b = 0;
  return fclose(f);
}
