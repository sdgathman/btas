/*
	Create "archives" of BTAS/2 files on a sequential file.
	These archives are designed to be portable to any computer.

	FIXME: load() should continue reading archive after some (but
	not all) errors.
*/

#include <stdio.h>
#include <fcntl.h>
#include <errenv.h>
#include <string.h>
#include "btutil.h"
#include <ftype.h>	/* portable file formats */
#include <block.h>
#include <io.h>
#include <mem.h>
#include <bterr.h>

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

int archive() {
  int rc;
  char *s = readtext("Filename: ");
  char *t = readtext("Output file: ");
  /* char buf[8192];		/* big I/O buffer */
  struct btff ff;
  f = fopen(t,"wb");
  if (f == 0 /* || setvbuf(f,buf,_IOFBF,sizeof buf) */) {
    perror(t);
    return 0;
  }
  seekable = !strchr(switch_char,'t');	/* tape device flag */
  dironly = !!strchr(switch_char,'d');	/* no data records */

  /* this test does not really work on System V.  How can we test for
     a seekable device? */
  if (seekable)	/* verify that devices seems to really support seeks */
    seekable = !lseek(fileno(f),0L,0);
  if (!seekable)
    printf(
      "Media does not support seeks.  Archive will be in sequential format.\n"
    );

  if (strchr(switch_char,'s') && !strcmp(s,"*")) {
    s = "[!.]*";
  }
  lastpath.len = 0;
  lastpath.pos = -1L;
  catch(rc)
    rc = findfirst(s,&ff);
    while (rc == 0 && cancel == 0) {
      if (strchr(switch_char,'s'))
	btwalk(ff.b->lbuf,arcone);
      else
	arcone(ff.b->lbuf,0);
      rc = findnext(&ff);
    }
    rc = 0;
  enverr
    finddone(&ff);
  envend
  puteof();
  (void)fclose(f);
  if (rc) 
    (void)printf("Fatal error %d\n",rc);
  return 0;
}

static int arcone(const char *name,const struct btstat *stp) {
  int rc,len;
  BTCB * volatile dtf = 0;
  long recs = 0L;
  long pos, newpos;
  struct ar_hdr hdr;
  struct fstat st;
  struct btstat s;

  if (cancel) return -1;
  fprintf(stderr,"%s: ",name);
  catch(rc)
  dtf = btopen(name,BTRDONLY+4,MAXREC);
  len = dtf->rlen;
  memcpy(st.rec,dtf->lbuf,len);	/* save dir record */

  /* get file stats */
  dtf->klen = 0;
  dtf->rlen = sizeof s;
  if (stp) {
    const char *p = strrchr(name,'/');
    int plen = p ? p - name : 0;
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
	  (void)fseek(f,newpos,0);
	}
      }
      stlong(0L,hdr.next);
      memcpy(lastpath.name,name,plen);
      lastpath.len = plen;
      lastpath.pos = newpos;		/* save current position */
      puthdr(&hdr,name,plen);		/* output chdir */
    }
    s = *stp;
  }
  else {
    rc = btas(dtf,BTSTAT);
    if (rc) {
      (void)fputs("couldn't stat, ",stderr);
      (void)btclose(dtf);
      dtf = 0;
      errpost(rc);
    }
    (void)memcpy((char *)&s,dtf->lbuf,sizeof s);
    if (s.id.mode & BT_DIR) {
      (void)btclose(dtf);
      dtf = 0;
      errpost(BTERDIR);
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
    dtf->klen = 0;
    dtf->rlen = MAXREC;
    rc = btas(dtf,BTREADGE+NOKEY);
    while (rc == 0 && cancel == 0) {
      putdata(dtf->lbuf,dtf->rlen,dtf->klen);
      ++recs;
      dtf->klen = dtf->rlen;
      dtf->rlen = MAXREC;
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
  (void)btclose(dtf);

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
  (void)fprintf(stderr,"%ld records archived\n",recs);
  return 0;		/* successful */
}

int load() {
  char *s = readtext("Filename: ");
  char *t = readtext("Archive: ");
  char *d = readtext((char *)0);	/* dir option */
  char dirflag = 0;
  /* char buf[8192]; */
  struct ar_hdr hdr;
  union {
    struct fstat st;
    char dir[MAXREC];
  } u;
  int rc, rlen;
  BTCB *savdir = btasdir;
  f = fopen(t,"rb");
  if (!f /* || setvbuf(f,buf,_IOFBF,sizeof buf) */) {
    perror(t);
    return 0;
  }
  if (d)
    dirflag = 1;

  catch(rc)
  while ((rlen = gethdr(&hdr,u.dir,sizeof u)) >= 0) {
    int magic = ldshort(hdr.magic);
    if (magic == chdir_MAGIC) {
      u.dir[rlen] = 0;
      (void)fprintf(stderr,"Chdir: %s\n",u.dir);
      if (!dirflag) {
	btclose(b);
	btasdir = savdir;
	b = btopen(u.dir,BTWRONLY+BTDIROK,MAXREC);
	btasdir = b;
      }
    }
    else if (magic == fstat_MAGIC) {
      rlen -= sizeof u.st - sizeof u.st.rec;
      (void)fprintf(stderr,"%s: ",u.st.rec); (void)fflush(stderr);
      if (!dirflag && match(u.st.rec,s)) {	/* if it matches pattern */
	BTCB *dtf;
	struct btstat bst;
	/* save original permissions */
	bst.id.user = ldshort(u.st.user);
	bst.id.group = ldshort(u.st.group);
	bst.id.mode = ldshort(u.st.mode);
	bst.atime = bst.mtime = ldlong(u.st.mtime);
	/* make sure we can write to it */
	b->u.id.user = geteuid();
	b->u.id.group = getegid();
	if (bst.id.mode & BT_DIR)
	  b->u.id.mode = 0700 + BT_DIR;
	else
	  b->u.id.mode = 0600;
	memcpy(b->lbuf,u.st.rec,(unsigned)rlen);
	b->rlen = rlen;
	b->klen = strlen(u.st.rec) + 1;
	rc = btas(b,BTCREATE+DUPKEY);
	if (rc)
	  fprintf(stderr," rewriting, ");
	else
	  fprintf(stderr," creating,  ");
	dtf = btopen(b->lbuf,BTWRONLY+BTDIROK,MAXREC);
	if (dtf) {
	  long recs = 0L, dups = 0L;
	  volatile char done = 0;
	  envelope
	    if ((bst.id.mode & BT_DIR) && rc == 0) {
	      strcpy(dtf->lbuf,".");
	      dtf->klen = dtf->rlen = 2;
	      dtf->u.cache.node = dtf->root;
	      btas(dtf,BTLINK);
	      strcpy(dtf->lbuf,"..");
	      dtf->klen = dtf->rlen = 3;
	      dtf->u.cache.node = b->root;
	      btas(dtf,BTLINK);
	    }
	    while ( (dtf->rlen = getdata(dtf->lbuf,MAXREC)) > 0) {
	      dtf->klen = dtf->rlen;
	      if (btas(dtf,BTWRITE+DUPKEY))
		++dups;
	      ++recs;
	    }
	    done = 1;
	    (void)memcpy(dtf->lbuf,(char *)&bst,sizeof bst);
	    dtf->klen = dtf->rlen = sizeof bst;
	    dtf->u.id.user = geteuid();
	    rc = btas(dtf,BTTOUCH);
	  enverr
	    fprintf(stderr,"Error code %d, permissions not restored.\n",errno);
	    if (!done) {	// skip 
	      long skipped = 0L;
	      while (getdata(dtf->lbuf,MAXREC))
		++skipped;
	      fprintf(stderr,"	%ld records skipped.\n",skipped);
	    }
	  envend
	  btclose(dtf);
	  fprintf(stderr,"%ld records loaded, %ld duplicates\n",recs,dups);
	}
      }
      else {
	long pos = ldlong(hdr.next);
	long recs;
	fprintf(stderr," skipping, ");
	if (!pos || fseek(f,pos,0)) {		/* if no seek possible */
	  recs = 0;
	  while (getdata(b->lbuf,MAXREC) >= 0)
	    ++recs;
	}
	else
	  recs = ldlong(u.st.rcnt);
	fprintf(stderr,"%ld records\n",recs);
      }
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
  btasdir = savdir;
  if (fclose(f) == 0)
    fprintf(stderr,"Archive file closed.\n");
  return 0;
}
