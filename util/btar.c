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

typedef enum {false, true} bool;

int verbose;
volatile int cancel;

static void handler(int sig) {
  cancel = true;
}

static void *xmalloc(unsigned size) {
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
};

static struct obstack h;
static struct namelist *filelist = 0, *dirlist = 0;

static struct namelist *add_list(struct namelist *p,const char *n) {
  struct namelist *q = obstack_alloc(&h,sizeof *p);
  q->name = n;
  if (!p) {
    p = q;
    p->next = q;
  }
  q->next = p->next;
  p->next = q;
  return p;
}

static struct namelist *read_listfile(struct namelist *p,const char *n) {
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
	p = add_list(p,obstack_finish(&h));
      }
      break;
    }
    if (c == '\n') {
      if (obstack_object_size(&h)) {
	obstack_1grow(&h,0);
	p = add_list(p,obstack_finish(&h));
      }
    }
    else
      obstack_1grow(&h,c);
  }
  fclose(f);
  return p;
}

static bool matchname(const char *dir,const char *name) {
  struct namelist *s;
  if (!filelist && !dirlist) return true;	// always match if no list
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
    if (match(name,s->name)) return true;
    if (s == filelist) break;
  }
  for (s = dirlist; s; ) {
    s = s->next;
    if (match(name,s->name)) return true;
    if (s == dirlist) break;
  }
  return false;
}

static int ldf(const char *dir,const char *r,int ln,const struct btstat *st) {
  if (matchname(dir,r)) {
    fprintf(stderr,"%s: ",r);
    btar_extract(dir,r,ln,st);
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
  char buf1[18], buf2[18], buf3[18];
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
	-f	name archive file.  Default is stdin/stdout\n\
	-T	file contains BTAS pathnames, one per line\n\
	-s	archive is on sequential media (i.e. tape),\n\
		do not use random access links.\n\
",stderr);
  exit(1);
}

enum btarCmd { BTAR_NONE, BTAR_EXTRACT, BTAR_LIST, BTAR_CREATE };

int main(int argc,char **argv) {
  enum btarCmd cmd = BTAR_NONE;
  const char *arfile = 0;
  bool dironly = false;
  bool seekable = true;
  bool expand_dir = true;
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
	  dironly = true;
	  continue;
	case 'E':
	  dironly = false;
	  continue;
	case 'd':
	  expand_dir = false;
	  continue;
	case 'D':
	  expand_dir = true;
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
	    if (dironly)
	      dirlist = read_listfile(dirlist,listfile);
	    else
	      filelist = read_listfile(filelist,listfile);
	  }
	  break;
	default:
	  usage();
	}
	break;
      }
      continue;
    }
    if (dironly)
      dirlist = add_list(dirlist,argv[i]);
    else
      filelist = add_list(filelist,argv[i]);
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
    for (s = dirlist; s && !cancel; ) {
      s = s->next;
      if (btar_add(s->name,true,expand_dir))
	rc = 1;
      if (s == dirlist) break;
    }
    for (s = filelist; s && !cancel; ) {
      s = s->next;
      if (btar_add(s->name,false,expand_dir))
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
