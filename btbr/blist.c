static char what[] = "%W%";
/*
	quick list of cisam data based on field info in .fcb file
	if no .fcb, compute reasonable defaults based on defined
	indexes.  use hex format as a last resort.
*/

#define MAXFILES 1	/* should use obstack also */

#include <stdio.h>
#include <isam.h>
#include <ischkerr.h>
#include <fs.h>
#include <fcntl.h>
#include "fcb.h"

struct afile {
  char *name;	/* basename of file */
  char *buf;	/* data buffer */
  int rlen;	/* record length */
  int  fd;	/* cisam file descriptor */
  FLD  *flist;	/* list of field descriptors */
};

static struct fldtbl fldtbl;
static struct fwindow rwin = { 0, 0, 0, 1, &fldtbl, 0, 0, 0 };
static char ver[6] = "%I%";
struct fwindow *fwin = &rwin;
static int debug = 0;	/* debug flag */
static struct afile ftbl[MAXFILES];	/* files used this report */
static int nfiles = 0, read_mode = ISNEXT, start_mode = ISFIRST;
static FLD *view;


static void usage()
{
  (void)printf("blist v%-6.6s  Copyright (c) 1990 BMS,Inc.\n",ver);
  /* puts("Usage: blist [-d][-iN][-r] file [key1,key2]"); */
  puts("Usage: blist [-d][-iN][-r] file [key1,key2]");
  puts("       blist [-k][-l] files");
  puts("Options:");
  puts(" -d  debug");
  puts(" -i  N is the index to use (1 is default)");
  puts(" -k  same as -l and includes a description of the keys");
  puts(" -l  print #recs,#indexes,reclen for each file");
  puts(" -r  list records in reverse order (key search is LTEQ)");
#if 0
  puts(" key1 and key2 are starting and ending keys. They are described");
  puts(" by using these codes:");
  puts("   C'string'		CHAR");
  puts("   Cnum'string'		CHAR (padded with blanks)");
  puts("   L'num'		FLONG");
  puts("   S'num'		FSHORT");
  puts("   M'num'		FMONEY");
  puts("   D'date'		FDATE");
  puts("   J'date'		FJUL");
  puts("   X'hexnum'		HEXNUM");
  puts("   e.g., C3'AP'D'060190'L'5'S'1'");
#endif
  exit(1);
}

int main(argc,argv)
  int argc;
  char **argv;
{
  struct afile *f;
  int index = 0;

  if (--argc < 1) usage();

  while (**++argv == '-') {
    switch (*++*argv) {
    case 'r':
      read_mode = ISPREV;
      start_mode = ISLAST;
      break;
    case 'd':
      debug = 1;
      break;
    case 'i':
      index = atoi(*argv +1);  /* isam file index. 1=primary */
      break;
    case 'k':
    case 'l':
  printf("Name        Numrecs  Indexes   Reclen    Bytes  Last-mod\n");
  printf("---------------------------------------------------------------------------\n");
  while (*++argv) l_format(*argv);      /* list in unix "l" format */
  if (totbytes != 0L) {
    printf("\nTotal bytes = %12-s",ltoa((long) totbytes));
    printf("   Total megs = %f\n",(float)((float)totbytes/(float)1048576));
  }
    default:
      usage();
    }
  }

  if (open_file(*argv,index) == -1) {
    printf("Error opening file %s.dat",*argv);
    syserr(iserrno);
    return 1;
  }
  f = &ftbl[0];
  view = f->flist;	/* single file, all fields for first run */

  if (debug) {
    FLD *fp = view;
    while (fp) {
      printf("%s\t%d\t%d\t%s\t%s\n",
		fp->name,fp->data - f->buf,fp->len,fp->format,fp->desc);
      fp = fp->next;
    }
  }
  else {
    while (chk(isread(f->fd,f->buf,read_mode),ISEOF) == 0) {
      FLD *fp = view;
      int fld = 0;
      while (fp) {
	flds->len = fp->dlen;
	(*fp->put)(fld,fp->data,fp->format,fp->len);
	putc(' ',stdout);
	fp = fp->next;
      }
      putc('\n',stdout);
    }
  }
  return 0;
}

int putfield(fld,data,len)
  char *data;
{
  register int i;
  for (i = 0; i < len && *data; ++i)
    putchar(*data++);
  while (i < flds[fld].len)
    putchar(' ');
  return 0;
}

int getfield()
{}

int open_file(name,index)
  char *name;
  int index;
{
  register struct afile *f = &ftbl[nfiles];
  struct dictinfo d;
  struct keydesc k;

  f->name = name;
  errno = 0;
envelope
  f->fd = isopen(name,ISMANULOCK+ISINPUT);
  if (f->fd == -1) syserr(iserrno);
  if (isindexinfo(f->fd,&d,0) == -1) syserr(iserrno);
  f->buf = malloc(d.di_recsize);
  f->rlen = d.di_recsize;
  if (!f->buf) return -1;
  if (isindexinfo(f->fd,&k,index) == -1) {	 /* make sure index is valid */
    printf("%d is an invalid index\n",index);
    return  -1; 
  }
  if (isstart(f->fd,&k,0,f->buf,start_mode) == -1) {     /* set index */
      printf("Index %d has no records",index);
      return  -1; 
  }
  f->flist = fcbread(name,f->buf,d.di_recsize);
  if (!f->flist) {
    free(f->buf);
    printf("%s.fcb: Can't open\n",name);
    return  -1;
  }
  ++nfiles;
enverr
  printf("Error opening %s.dat\n",name);
envend
  return errno;
}

int l_format(name)
  char *name;
{
  struct dictinfo d;
  struct keydesc k;
  time_t lastmod;
  off_t size;
  int fd;
  static char *c = ".idx";
  if (strncmp(c,&name[strlen(name)-4],4)) return; /* not isam idx file */
  name[strlen(name)-4] = 0;
  getstat(name,&size,&lastmod);
  totbytes += (long) size;
  printf("%-11s",name);

  fd = isopen(name,ISMANULOCK+ISINPUT);
  if (fd == -1) {
    printf("Can't open\n");
    return -1;
  }

  if (isindexinfo(fd,&d,0) == -1) {
    printf("Can't get index info\n");
    return -1;
  }
  printf("%8s ",ltoa(d.di_nrecords));
  printf("%8s ",ltoa((long)d.di_nkeys));
  printf("%8s ",ltoa((long)d.di_recsize));
  printf("%8s ",ltoa((long) size));
  printf(" %s",ctime(&lastmod));
  if (opt_key) {
    int knum;
    struct keydesc k;
    for (knum=1;knum <= d.di_nkeys;knum++) {
      if (isindexinfo(fd,&k,knum) == -1) {
        printf("Error %d: on index %d\n",iserrno,knum);
      }
      else {
	int i;
	printf("   key %d: flags = %d, nparts = %d\n",
	       knum,k.k_flags,k.k_nparts);
	for (i=0; i < k.k_nparts; i++) {
	  struct keypart *kp;
	  kp = &k.k_part[i];
	  printf("      start=%d,len=%d,type=%d\n",
	    kp->kp_start,kp->kp_leng, kp->kp_type);
	}
      }
    }
  }
  isclose(fd);
}

getstat(name,size,lastmod) 
  char *name;
  off_t *size;
  time_t *lastmod;
{
  int rc;
  struct stat buf;
  char path[30];
  path[0] = 0;
  *size = (off_t) 0;
  *lastmod = (time_t) 0;
  (void) strcat(path,"./");
  (void) strcat(path,name);
  (void) strcat(path,".idx");
  rc =  stat(path,&buf);
  if (rc) errdesc("err",errno);
  *size = buf.st_size;
  *lastmod = buf.st_mtime;

  path[0] = 0;
  (void) strcat(path,"./");
  (void) strcat(path,name);
  (void) strcat(path,".dat");
  rc =  stat(path,&buf);
  if (rc) errdesc("err",errno);
  *size += buf.st_size;
}

#if 0
int l_format(name)
  char *name;
{
  struct dictinfo d;
  struct keydesc k;
  int fd;
  static char *c = ".idx";
  if (strncmp(c,&name[strlen(name)-4],4)) return; /* not isam idx file */
  name[strlen(name)-4] = 0;
  printf("%-11s",name);

  fd = isopen(name,ISMANULOCK+ISINPUT);
  if (fd == -1) {
    printf("Can't open\n");
    return -1;
  }

  if (isindexinfo(fd,&d,0) == -1) {
    printf("Can't get index info\n");
    return -1;
  }
  printf("%8s ",ltoa(d.di_nrecords));
  printf("%8s ",ltoa((long)d.di_nkeys));
  printf("%8s \n",ltoa((long)d.di_recsize));
  isclose(fd);
}
#endif
