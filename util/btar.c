/*
To: asipmax!darin
Subject: btar

I have hacked up the archive/load module in btutil to be independently
callable.  I kept it in C with static variables & all so that you might
be able to compile it yourself.  The archive module supports only one
concurrently open archive due to the static variables, but this is all
that is needed by btar and btutil.  Please review the proposed command
line options for btar and send your comments.
 *
 * $Log$
 * Revision 1.4  1998/04/08  21:58:26  stuart
 * Support replace/rewrite and dupkey
 *
 * Revision 1.3  1997/09/10  21:41:40  stuart
 * reduce non-verbose output
 *
 * Revision 1.2  1996/01/05  01:37:40  stuart
 * fix many bugs
 *
 * Revision 1.1  1996/01/04  21:32:26  stuart
 * Initial revision
 *
 */

#include <stdio.h>
#include <port.h>
#include <stdlib.h>
#include <signal.h>
#include <pwd.h>
#include <grp.h>
#include <obstack.h>
#include <date.h>
#include <btas.h>
#include <bttype.h>
#include "btar.h"

#define obstack_chunk_alloc xmalloc
#define obstack_chunk_free free

typedef enum {false, true} bool;

int verbose;
volatile int cancel;

static void handler(int sig) {
  cancel = true;
}

static void *xmalloc(size_t size) {
  void *p = malloc(size);
  if (!p && size) {
    fputs("btar: Out of memory!\n",stderr);
    exit(1);
  }
  return p;
}

struct namelist {
  struct namelist *next;
  const char *name;
  int flags;
};

static struct obstack h;
static struct namelist *filelist = 0;
static int flags = FLAG_EXPAND|FLAG_FIELDS;

static struct namelist *add_list(struct namelist *p,const char *n,int flags) {
  struct namelist *q = obstack_alloc(&h,sizeof *p);
  q->name = n;
  q->flags = flags;
  if (!p)
    p = q;
  else
    q->next = p->next;
  p->next = q;
  return q;
}

static struct namelist *
read_listfile(struct namelist *p,const char *n,int flags) {
  FILE *f = fopen(n,"r");
  if (!f) {
    perror(n);
    return p;
  }
  for (;;) {
    int c = getc(f);
    if (c == EOF) {
      if (obstack_object_size(&h)) {
	obstack_1grow(&h,0);
	p = add_list(p,obstack_finish(&h),flags);
      }
      break;
    }
    if (c == '\n') {
      if (obstack_object_size(&h)) {
	obstack_1grow(&h,0);
	p = add_list(p,obstack_finish(&h),flags);
      }
    }
    else
      obstack_1grow(&h,c);
  }
  fclose(f);
  return p;
}

static int matchname(const char *dir,const char *name) {
  struct namelist *s;
  if (!filelist) return flags|FLAG_FOUND;	// always match if no list
  if (*dir) {
    int len = strlen(dir);
    char *buf = alloca(len + strlen(name) + 2);
    strcpy(buf,dir);
    buf[len++] = '/';
    strcpy(buf+len,name);
    name = buf;
  }
  for (s = filelist; s; ) {
    s = s->next;
    if (match(name,s->name)) return s->flags|FLAG_FOUND;
    if (s == filelist) break;
  }
  return 0;
}

static int ldf(const char *dir,const char *r,int ln,const struct btstat *st) {
  int f = matchname(dir,r);
  if (f) {
    fprintf(stderr,"%s: ",r);
    btar_extractx(dir,r,ln,st,f);
  }
  else {
    long recs;
    if (verbose) { fprintf(stderr,"skipping"); fflush(stderr); }
    recs = btar_skip(st->rcnt);
    if (verbose) fprintf(stderr," %ld recs\n",recs);
  }
  return cancel;
}

static int dirf(const char *dir,const char *r,int ln,const struct btstat *st) {
  char buf1[18], buf2[18], buf3[13];
#ifndef __MSDOS__
  static struct passwd *pw;
  static struct group *gr;
  if (!pw || pw->pw_uid != st->id.user) pw = getpwuid(st->id.user);
  if (!gr || gr->gr_gid != st->id.group) gr = getgrgid(st->id.group);
  if (pw)
    strcpy(buf1,pw->pw_name);
  else
    sprintf(buf1,"%d",st->id.user);
  if (gr)
    strcpy(buf2,gr->gr_name);
  else
    sprintf(buf2,"%d",st->id.group);
#else
  itoa(st->id.user,buf1,10);
  itoa(st->id.group,buf2,10);
#endif
  timemask(st->mtime,"Nnn DD CCCCC",buf3);
  buf3[12] = 0;
  printf("%s %8s %8s %8ld %s %s%s%s\n",
    permstr(st->id.mode),buf1,buf2,st->rcnt,buf3,
    dir,*dir ? "/" : "",r
  );
  btar_skip(st->rcnt);
  return cancel;
}

static void usage(void) NORETURN;
static void usage(void) {
  fputs("$Id$\n\
Usage:	btar {-x,-c,-t} [-vsd] [-f archive] [ [-e] [pattern] [-T file] ... ]\n\
	-x	extract files with pathnames matching patterns, or all files\n\
	-c	archive files matching patterns,\n\
		recursively expand directories.\n\
	-t	list files with pathnames matching patterns, or all files\n\
	-e	archive following patterns as empty files\n\
	-E	archive following patterns will all records (default)\n\
	-d	do not expand subdirectories\n\
	-D	expand subdirectories (default)\n\
	-v	show more info in listings, tell what we are doing\n\
	-f file	name archive file.  Default is stdin/stdout\n\
	-T file	file contains BTAS pathnames, one per line\n\
	-s	archive is on sequential media (i.e. tape),\n\
		do not use random access links.\n\
	-r	replace files/records when extracting\n\
	-R	do not replace files when extracting (default)\n\
	-k	use fieldtable when extracting records (default)\n\
	-K	do not use fieldtable when extracting records\n\
	Examples:\n\
	-rk	rewrite records\n\
	-rK	replace files (careful!)\n\
	-Rk	ignore dupkeys (default)\n\
	-RK	merge records\n\
",stderr);
  exit(1);
}

enum btarCmd { BTAR_NONE, BTAR_EXTRACT, BTAR_LIST, BTAR_CREATE };

int main(int argc,char **argv) {
  enum btarCmd cmd = BTAR_NONE;
  const char *arfile = 0;
  bool seekable = true;
  int i,j;
  int rc = 0;
  obstack_specify_allocation(&h,0,4,xmalloc,free);
  signal(SIGINT,handler);
  for (i = 1; i < argc; ++i) {
    if (*argv[i] == '-' || i == 1) {
      for (j = (*argv[i] == '-'); argv[i][j]; ++j) {
	switch (argv[i][j]) {
	case 'x':
	  if (cmd != BTAR_NONE) usage();
	  cmd = BTAR_EXTRACT;
	  continue;
	case 't':
	  if (cmd != BTAR_NONE) usage();
	  cmd = BTAR_LIST;
	  continue;
	case 'c':
	  if (cmd != BTAR_NONE) usage();
	  cmd = BTAR_CREATE;
	  continue;
	case 'v':
	  ++verbose;
	  continue;
	case 's':
	  seekable = false;
	  continue;
	case 'e':
	  flags |= FLAG_DIRONLY;
	  continue;
	case 'E':
	  flags &= ~FLAG_DIRONLY;
	  continue;
	case 'd':
	  flags &= ~FLAG_EXPAND;
	  continue;
	case 'D':
	  flags |= FLAG_EXPAND;
	  continue;
	case 'k':
	  flags |= FLAG_FIELDS;
	  continue;
	case 'K':
	  flags &= ~FLAG_FIELDS;
	  continue;
	case 'r':
	  flags |= FLAG_REPLACE;
	  continue;
	case 'R':
	  flags &= ~FLAG_REPLACE;
	  continue;
	case 'f':
	  if (arfile) usage();
	  if (argv[i][++j])
	    arfile = argv[i] + j;
	  else
	    arfile = argv[++i];
	  break;
	case 'T':
	  {
	    const char *listfile;
	    if (argv[i][++j])
	      listfile = argv[i] + j;
	    else
	      listfile = argv[++i];
	    filelist = read_listfile(filelist,listfile,flags);
	  }
	  break;
	default:
	  usage();
	}
	break;
      }
      continue;
    }
    filelist = add_list(filelist,argv[i],flags);
  }
  switch (cmd) {
    struct namelist *s;
  case BTAR_EXTRACT:
    if (btar_load(arfile,ldf)) return 1;
    break;
  case BTAR_LIST:
    if (btar_load(arfile,dirf)) return 1;
    break;
  case BTAR_CREATE:
    if (btar_opennew(arfile,seekable)) return 1;
    for (s = filelist; s && !cancel; ) {
      s = s->next;
      if (verbose) {
	if (s->flags&FLAG_DIRONLY)
	  fprintf(stderr,"%s%s%s\n",
		  "EMPTY ",(s->flags&FLAG_EXPAND) ? "EXPAND " : "",s->name);
	else
	  fprintf(stderr,"%s%s\n",
		  (s->flags&FLAG_EXPAND) ? "EXPAND " : "",s->name);
      }
      if (btar_add(s->name,s->flags&FLAG_DIRONLY,s->flags&FLAG_EXPAND))
	rc = 1;
      if (s == filelist) break;
    }
    if (btar_close()) return 1;
    break;
  default:
    usage();
  }
  return rc || cancel;
}
