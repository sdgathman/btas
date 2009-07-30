static char what[] = "1.17";
/*
Program:   browse.c Revision:  1.17 as of 4/19/95
Author:	   Ed Bond
Function:  browse.c - Browse BTAS C-ISAM type files.
Copyright (c) 1990,1991 Business Management Systems, Inc.

TODO:
Add check to make sure ".fcb" length equals record length. WHY?
Add Change View logic.
*/

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <isamx.h>
#include <errenv.h>
#include <fs.h>
#include <keys.h>
#include <fileio.h>
#include <btflds.h>
#include <date.h>
#include <money.h>
#include <ftype.h>
#include <curses.h>	/* to get wclear */
#include "fld.h"
#include "etoa.h"

#define MAXFILES 1	/* should use obstack also */
#define MAXCOLS 80
/* debug bits */
#define DBFLD	1		/* print_fld */
#define DBIDX	2		/* print_idx */
static int debug = 0;		/* debug flag */
static int cflag = 0;		/* change flag */
static int eflag = 0;		/* ebcdic flag */
static int xflag = 0;		/* flag to force use of BTAS/X field table */
static int verbose = 0;		/* flag to expand physical field for desc */
static struct file ftbl[MAXFILES];	/* files used */
static int nfiles = 0;
static int maxlines = 16;	/* display this many lines */
#define MAXFLDTBL 30
static int tblindx[MAXFLDTBL];
static int tblflds[MAXFLDTBL];
static FLD *tblview[MAXFLDTBL];
static int curwin,winno;	/* current & number of field windows */

typedef int (*GETPUTFCN)(int,char *,char *,int);

static int open_file(char *,int);
static FIELD *makescrn(FLD *);
static void puttitle(int,FIELD *,FLD *,int);
static void initdata(FLD *);
static GETPUTFCN getfunc(int),putfunc(int);
static int getebcdic(int,char *,char *,int),putebcdic(int,char *,char *,int);
static int my_gettimef(int fld,char *data,const char *opt,int len);
static int my_puttimef(int fld,const char *data,const char *opt,int len);

#ifdef BTASX
#define FILESYS "BTAS/X"
#else
#ifdef CISAM
#define FILESYS "CISAM"
#else
#define FILESYS ""
#endif
#endif

static void usage()
{
  printf("%s Browse Ver. %s\n",FILESYS,"1.17");
  printf("Usage: %s [-c] [-dopt] [-iN] [-e] [-lN] [-x] [-vN] file\n",bmsprog);
  puts(" -c    Change. Allow data to be changed.");
  puts(" -dopt Debug. 'opt' is zero or more of the following:");
  puts("         f print field table (default)");
  puts("         i print idx descriptions");
  puts(" -iN   Index. N specfies index to use (default=1).");
  puts(" -e    Ebcdic. Character fields are in ebcdic.");
  puts(" -lN   Lines. N specfies number of lines to display at a time.");
  puts(" -x    Force use of BTAS/X field table.");
  puts(" -vN   Verbose. N specifies verbose level.");
  exit(1);
}

static char help_editon[] = "Turn Edit Mode On ";
static char help_editof[] = "Turn Edit Mode Off";

#define SLLEFTKEY F6
#define SLRGHTKEY F9
#define TGLEDITKEY 4003

static KEYLIST helplist[] = {
  "Exit Help",0,
  "Backward Page      F3",F3,
  "Foward Page        F4",F4,
  "Search             F5",F5,
  "Left Slide         F6",SLLEFTKEY,
  "Right Slide        F9",SLRGHTKEY,
  "Select New View",-1,	/* not implemented yet */
  "Insert Record      F7",-1,
  "Delete Record      F8",-1,
  help_editon,TGLEDITKEY,
  "Quit               F10",F10,
  0,
};

static void tglmode(f)
  FCB *f;
{
  static char readonly = 1;
  readonly = readonly ? 0 : 1;
  if (f) (void)fcbflags(f,FCBFLG_RDONLY,readonly);
  if (readonly) {
    helplist[7].key = -1;	/* disable insert help msg */
    helplist[8].key = -1;	/* disable delete help msg */
    helplist[9].desc = help_editon;
  }
  else {
    helplist[7].key = F7;	/* enable insert help msg */
    helplist[8].key = F8;	/* enable delete help msg */
    helplist[9].desc = help_editof;
  }
}

static ALTFKEY altfkey[] = {
  { TGLEDITKEY,tglmode },
  { F10, 0 },
  { 0, 0 }
};

static const char *typestr(type)
  int type;
{
  static char tstr[5];
  if (type&BT_NUM) {
    sprintf(tstr,"N%d",type&0x0F);
    return tstr;
  }
  switch (type) {
    case BT_CHAR: return "C";
    case BT_DATE: return "D";
    case BT_TIME: return "T";
    case BT_BIN: return "X";
  }
  /* we return the hex string for unknown types */
  sprintf(tstr,"%2X",type & 0xFF);
  return tstr;
}

static void print_fld(f)
  struct file *f;
{
    FLD *fp = f->flist;
    printf("%-10s %-3s %-3s %-16s %-10s %-4s %s\n",
        "Name","Off","Len","Format","Desc","Type","Dlen");
    while (fp) {
      printf("%-10.10s %-3d %-3d %-16s",fp->name,fp->data - f->buf,fp->len,
	(fp->format) ? fp->format : typestr(fp->put));
      printf(" %-10.10s %-4s %d\n",fp->desc,typestr(fp->type),fp->dlen);
      fp = fp->next;
    }
}

/* findfld - Returns the field whose offset is "start".
   Returns 0 if offset is not found.
   NOTE:  offset = fldtbl->data - databuf;	(* ptr subtraction *)
*/
static FLD *findfld(start,fldtbl,databuf)
  int start;		/* start of field (i.e. - offset) */
  FLD *fldtbl;		/* pointer to fcb table */
  char *databuf;	/* pointer to data buffer (beginning) */
{
  while (fldtbl) {
    if (fldtbl->data - databuf == start) return fldtbl;
    fldtbl = fldtbl->next;
  }
  return 0;
}

static void print_idx(f) 
  struct file *f;
{
    int i,p;
    struct keydesc k;
    putchar('\n');
    for (i=1; !isindexinfo(f->fd,&k,i) ; ++i) {
      FLD *fld;
      printf("Index #%d is %d bytes and contains %d parts:\n",
	      i,k.k_len,k.k_nparts);
      printf("%-10s %-3s %-3s %-14s %s\n",
	 "Name","Off","Len","Format","Desc");
      for (p=0; p < k.k_nparts; p++)
	if (fld = findfld(k.k_part[p].kp_start,f->flist,f->buf))
	  printf("%-10.10s %-3d %-3d %-14s %10.10s\n",
		  fld->name,fld->data - f->buf,fld->len,
	          (fld->format) ? fld->format : typestr(fld->put),fld->desc);
	else 
	  printf("%-10d %-3d %-3d %d\n",p,k.k_part[p].kp_start,
		  k.k_part[p].kp_leng,k.k_part[p].kp_type);
    }
}

int main(argc,argv)
  int argc;
  char **argv;
{
  struct file *f;
  int index = 1,er;

  bmsprog = argv[0];
  if (--argc < 1) usage();

  while (**++argv == '-') {
    switch (*++*argv) {
    case 'c':
      cflag = 1;	/* ebcdic flag */
      break;
    case 'd':
      if (argv[0][1]) {
	if (strchr(&argv[0][1],'f')) debug |= DBFLD;
	if (strchr(&argv[0][1],'i')) debug |= DBIDX;
      }
      else debug |= DBFLD;
      break;
    case 'e':
      eflag = 1;	/* ebcdic flag */
      break;
    case 'i':
      index = atoi(*argv +1);  /* isam file index. 1=primary */
      break;
    case 'l':
      maxlines = atoi(*argv +1);  /* max lines to show */
      break;
    case 'x':
      xflag = 1;	/* force to use BTAS/X field table */
      break;
    case 'v':
      verbose = 1;	/* expand physical field to desc size */
      if (argv[0][1] == '2') verbose = 2; 
      break;
    default:
      printf("Invalid option %s\n",*argv);
      usage();
    }
  }

  if (open_file(*argv,index) == -1) return 1;
catch(er)
  f = &ftbl[0];		/* ftbl is filled by open_file() */
  if (debug) {
    struct dictinfo d;
    printf("File:  %s\t",f->name);
    if (isindexinfo(f->fd,(struct keydesc *)&d,0) == -1)
      printf("isindexinfo got error %d\n",iserrno);
    else
      printf("%ld records, %d key%s, %d bytes/record\n",
             d.di_nrecords, d.di_nkeys,(d.di_nkeys==1)?"":"s",d.di_recsize);
    if (debug&DBFLD) print_fld(f);
    if (debug&DBIDX) print_idx(f);
  }
  else {
    FIELD *tbl;
    FCB *fcb[2];
    FILEIO *io;
    int pfkey,err,strtline;
    struct keydesc key1;

    /* get primary key desc (we will assume it is the first index) */
    isindexinfo(f->fd,&key1,1);	
    io = filestart(f->fd,f->buf,&key1);
    if (!io) {
      fprintf(stderr,"%s: filestart failed - index 1 is a DUPKEY index\n",
		bmsprog);
      errpost(-1);
    }
    fsinit();
  catch(err)
    tbl = makescrn(f->flist);	/* fsnflds-2 equals count(tbl) */
    if (!tbl) errdesc("Can't create display tables",errno);
    initdata(f->flist);		/* initialize data to null values */
    fcb[0] = fcbbeg(tbl,fsnflds-4,altfkey,fileio,(PTR)io,
	 	    FCBFLG_TOP+FCBFLG_RDONLY);
    fcb[1] = 0;

    puttime(0,0,"Y");				/* display time */
    putstrn(1,ftbl[0].name,(char *)0,60);	/* display filename */
    strtline = 0;
    curwin = 0;
    puttitle(2,&tbl[tblindx[curwin]],tblview[curwin],tblflds[curwin]);
    fcbsetdisp(fcb[0],tblflds[curwin],&tbl[tblindx[curwin]]);
    viewkey(io,f->klist,1);
    fcblfwd(fcb[0]);
    if (cflag) tglmode(fcb[0]);
    for (;;) {
      fsline = strtline;
      pfkey = fcbscrn(fcb,tblflds[curwin],&tbl[tblindx[curwin]],
		      (ALTFKEY *)0,helplist);
      strtline = fsline;
      if (pfkey == F10) break;
      switch (pfkey) {
      case SLRGHTKEY:
      case SLLEFTKEY:
	if (pfkey == SLRGHTKEY) {
	  if (++curwin > winno) curwin = 0;
	}
	else {		/* reverse */
	  if (--curwin < 0) curwin = winno;
	}
	wclear(fwin->w);	/* clear screen */
	puttime(0,0,"Y");	/* display time */
  	putstrn(1,ftbl[0].name,(char *)0,60);	/* display filename */
        puttitle(2,&tbl[tblindx[curwin]],tblview[curwin],tblflds[curwin]);
        fcbsetdisp(fcb[0],tblflds[curwin],&tbl[tblindx[curwin]]);
	fcblocate(fcb[0],0);
	fcblfwd(fcb[0]);
        fslfld = 0;
	break;
      }
    }
    fcbend(fcb);
  envend
    fsend();
    if (err) errpost(err);
  }
envend
  iscloseall();
  if (er) {
    if (er != -1) fprintf(stderr,"%s: error %d\n",bmsprog,er);
    return 1;
  }
  return 0;
}

static int open_file(name,index)
  char *name;
  int index;
{
  register struct file *f = &ftbl[nfiles];
  struct dictinfo d;
  struct keydesc k;
  char *filename;
  int err = 0;

  f->name = name;
  f->fd = -1;
catch(err)
  f->fd = isopen(name,ISMANULOCK+ISINOUT);
  if (f->fd == -1) {
    fprintf(stderr,"%s: %d Can't open \"%s\".\n",bmsprog,iserrno,name);
    errpost(-1);
  }
  if (isindexinfo(f->fd,(struct keydesc *)&d,0) == -1) {
    fprintf(stderr,"%s: %d Can't read index for \"%s\".\n",
		bmsprog,iserrno,name);
    errpost(-1);
  }
  /* if (index < 1 || index > d.di_nkeys) index = 1; */
  if (isindexinfo(f->fd,&k,index) == -1) {	 /* get key desc */
    fprintf(stderr,"%s: %d Index %d is invalid\n",bmsprog,iserrno,index);
    errpost(-1);
  }
  if (isstart(f->fd,&k,0,(char *)0,ISFIRST) == -1) {     /* set index */
    fprintf(stderr,"Index %d has no records",index);
    fprintf(stderr,"%s: %d Index %d has no records.\n",bmsprog,iserrno,index);
    errpost(-1);
  }
  f->buf = malloc(d.di_recsize);
  f->rlen = d.di_recsize;
  if (!f->buf) {
    fprintf(stderr,"%s: %d Malloc failed for buf size %d\n",
		bmsprog,errno,f->rlen);
    errpost(-1);
  }
  filename = strrchr(name,'/');		/* strip path for fcbread */
  if (filename) filename++;
  else filename = name;
  f->flist = 0;
  if (!xflag) {
    if (f->flist = ddread(filename,f->buf,d.di_recsize))
      puts("Using DATA.DICT.");
    else if (f->flist = fcbread(filename,f->buf,d.di_recsize))
      printf("Using %s.fcb.\n",filename);
  }
#ifdef BTASX
  if (!f->flist) {	/* .fcb not found or error processing */
    f->flist = btfread(name,f->buf,d.di_recsize);
    if (!f->flist) {
      free(f->buf);
      fprintf(stderr,"%s: %d Unable to process \"%s\" field table.\n",
		bmsprog,errno,filename);
      errpost(-1);
    }
    puts("Using BTAS/X field table.");
  }
#else
  if (!f->flist) {
    free(f->buf);
    fprintf(stderr,"%s: %d Unable to process \"%s.fcb\".\n",
		bmsprog,errno,filename);
    errpost(-1);
  }
#endif
  f->klist = (struct keydesc *)malloc(sizeof k);
  if (!f->klist) {
    fprintf(stderr,"%s: %d Malloc failed for key size %d\n",
		bmsprog,errno,sizeof k);
    free(f->buf);
    errpost(-1);
  }
  f->klist[0] = k;
  ++nfiles;
envend
  if (err) {
    if (err != -1)
      fprintf(stderr,"%s: error %d on file \"%s\".\n",bmsprog,err,name);
    if (f->fd != -1) isclose(f->fd);
    return -1;
  }
  return 0;
}

/* returns the logical field table for the file */
/* sets flds to point to new physical field table */
/* To make this work for sliding, we read in the whole field table */
static FIELD *makescrn(view)
  FLD *view;
{
  register struct fldtbl *f;	/* physical field table */
  register FIELD *l;		/* logical field table */
  register FLD *v;
  int curcol,n,i,lfld,numtflds,dlen;
  static struct fldtbl fixfld[4] = {
    /* time             filename        title           title2 */
      {0,60,16,0,1,0}, {0,0,50,0,1,0}, {2,0,80,2,1,0}, {3,0,80,3,1,0}
  };

  f = (struct fldtbl *)calloc(100,sizeof *f);
  if (!f) return 0;

  flds = ++f;

  /* initialize physical fields */
  f[0] = fixfld[0];	/* time field */
  f[1] = fixfld[1];	/* file field */
  f[2] = fixfld[2];	/* title field */
  f[3] = fixfld[3];	/* title field */
  n = 4;	/* physical field starts at 4 - file,time,title 0,1,2 */
  tblindx[winno] = 0;
  tblview[winno] = view;
  numtflds = 0;
  for (curcol=0,v=view,lfld=0; v; ++n,++lfld,++numtflds,v=v->next) {
    if (n == 100) { free(f); return 0; }	/* too many fields */
    dlen = v->dlen;
    if (verbose) {
      int nlen = strlen(v->name);
      if (nlen > dlen) dlen = nlen;
      if (verbose > 1) {
        nlen = strlen(v->desc);
        if (nlen > dlen) dlen = nlen;
      }
      if (dlen > COLS) dlen = COLS;
    }
    if (curcol + dlen > COLS) {
      /* make sure any one field is not larger than COLS */
      if (curcol == 0) { free(f); return 0; }
      curcol = 0;
      /* next logical fcb window */
      tblflds[winno] = numtflds;
      tblindx[++winno] = lfld;
      tblview[winno] = v;
      numtflds = 0;
    }
    f[n].row = 5;
    f[n].col = curcol;
    f[n].len = dlen;		/* max with strlen(name) ? */
    f[n].min = f[n].row;
    f[n].lcnt = maxlines;	/* display this many lines at a time */
    f[n].nul = 0;
    curcol += dlen;
    ++curcol;	/* leave a space between fields */
  }
  if (n == 4) { free(f); return 0; } /* no data fields */
  tblflds[winno] = numtflds;
  
  fsnflds = n;		/* number of fields + time & title field */
  flds = f;		/* set current physical field table */

  /* n is now number of data fields + file + time + title field */
  /* we don't want time & title field in logical table */
  l = (FIELD *)calloc(n-4,sizeof *l);	/* logical field table */
  if (!l) { free(f); return 0; }

  /* initialize logical field table */
  for (i=0,v=view,n=4; v ; ++i,v=v->next,++n) {
    l[i].fld = n;
    l[i].get = getfunc(v->put);	/* set to proper get function */
    l[i].put = putfunc(v->put);	/* set to proper put function */
    l[i].dat = v->data;
    l[i].opt = v->format;
    l[i].len = v->len;
    l[i].inq = 0;
    l[i].up = -1;
    l[i].dn = -1;
  }
  curwin = winno;
  return l;
}

static void puttitle(fld,t,v,size)
  int fld,size;
  FIELD *t;
  FLD *v;
{
  int i,dlen,tlen;
  char titlestr[MAXCOLS];
  char titlestr2[MAXCOLS];
  memset(titlestr,' ',sizeof titlestr);		/* clear title */
  memset(titlestr2,' ',sizeof titlestr2);	/* clear title */
  for (i=0; i < size;++i,v=v->next) {
    dlen = flds[t[i].fld].len;		/* physical length */
    tlen = strlen(v->name);
    if (tlen > dlen) tlen = dlen;
    memcpy(&titlestr[flds[t[i].fld].col],v->name,tlen);
    tlen = strlen(v->desc);
    if (tlen > dlen) tlen = dlen;
    memcpy(&titlestr2[flds[t[i].fld].col],v->desc,tlen);
  }
  putfield(fld,titlestr,sizeof titlestr);	/* display title */
  putfield(fld+1,titlestr2,sizeof titlestr2);	/* display title 2 */
}
    
static void initdata(v)
  FLD *v;
{
  while (v) {
    if (v->put&BT_NUM)
      stnum(zeroM,v->data,v->len);
    else switch (v->put) {
      case BT_CHAR:
	memset(v->data,' ',v->len);
	break;
      case BT_DATE:
	if (v->len < 4)
	  stD(zeroD,v->data);
	else
	  stJ(zeroJ,v->data);
	break;
      default:
	memset(v->data,0,v->len);
    }
    v = v->next;
  }
}

static int putunknown(fld,data,opt,len)
  int fld,len;
  char *data,*opt;
{
  putfield(fld,"?",1);
  return 0;
}
#if 0
static int mygetnumf(f,d,o,l)
  int f,l;
  char *d,*o;
{ return 0; }

static int myputnumf(f,d,o,l)
  int f,l;
  char *d,*o;
{ putstr(f,o,"",80);
  return 0;
}
#endif

static GETPUTFCN getfunc(type)
  int type;
{
  if (type&BT_NUM) return getnumf;
  switch (type) {
    case BT_CHAR: return eflag ? getebcdic : getstrn;
    case BT_DATE: return getdatef;
    case BT_BIN: return gethex;
    case BT_TIME: return my_gettimef;
  }
  return 0;
}

static GETPUTFCN putfunc(type)
  int type;
{
  if (type&BT_NUM) return putnumf;
  switch (type) {
    case BT_CHAR: return eflag ? putebcdic : putstrn;
    case BT_DATE: return putdatef;
    case BT_BIN: return puthex;
    case BT_TIME: return my_puttimef;
  }
  return putunknown;
}

static int getebcdic(fld,data,opt,len)
  int fld,len;
  char *data,*opt;
{
  getstrn(fld,data,opt,len);
  atoe(data,data,len);
  return 0;
}

static int putebcdic(fld,data,opt,len)
  int fld,len;
  char *data,*opt;
{
  char buf[132];
  if (len > sizeof buf) len = sizeof buf;
  etoa(buf,(unsigned char *)data,len);
  putstrn(fld,buf,opt,len);
  return 0;
}

/* FIXME: This solution depends on gettm and puttm which roll over in 2038.
   Gettm and puttm can be fixed to work until 2106. */

static int my_gettimef(int fld,char *data,const char *opt,int len) {
  long t;
  int rc;
  if (len == 4)
    t = ldlong(data);
  else {
    MONEY m = ldnum(data,len);
    if (len > 5)
      divM(&m,1000);
    t = Mtol(&m);
  }
  rc = gettm(fld,(char *)&t,opt,sizeof t);
  if (len == 4)
    stlong(t,data);
  else {
    MONEY m = ltoM(t);
    if (len > 5)
      mulM(&m,1000,0);
    stnum(m,data,len);
  }
  return rc;
}

static int my_puttimef(int fld,const char *data,const char *opt,int len) {
  long t;
  if (len == 4)
    t = ldlong(data);
  else {
    MONEY m = ldnum(data,len);
    if (len > 5)
      divM(&m,1000);
    t = Mtol(&m);
  }
  return puttm(fld,(char *)&t,opt,sizeof t);
}
