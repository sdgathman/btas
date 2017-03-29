/*
	Basic BTAS/2 utility
 * $Log$
 * Revision 1.3  1993/05/26  18:07:34  stuart
 * shorten di/t format
 *
 * Revision 1.2  1993/05/18  15:02:29  stuart
 * make symbolic links work
 *
 */

#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <time.h>
#include <tsearch.h>
#include <errenv.h>
#include "btflds.h"
#include "bterr.h"
#include "btutil.h"

#ifdef __MSDOS__
#include <mem.h>
#define getpwuid(x)	0
#define getgrgid(x)	0
#define endpwent()
#define endgrent()
#pragma warn -par
#else
#include <pwd.h>
#include <grp.h>
extern unsigned short getuid(), getgid();

#include <memory.h>
#endif

static void docmd(char *);
int btsync(void);
static void cdecl handler(int);
char prompt[] = "COMMAND(?): ";
char *switch_char;		/* special command options */

int cdecl main(int argc,char **argv) {
  if (argc > 1) {		/* get commands from arguments */
    while (--argc) {
      docmd(*++argv);
    }
  }
  else {			/* get commands from stdin */
    char cmd[80];
    (void)puts(btas_version);
    for (;;) {
      (void)fputs(prompt,stdout);
      (void)fgets(cmd,sizeof cmd,stdin);
      docmd(cmd);
    }
  }
  return 0;
}

struct cmdlist {
  char *name;			/* command name */
  int (*func)(/**/ void /**/);	/* function to call */
  char flag;			/* nonzero if default dir needed */
} cmdlist[] = {
  "AR", archive, 0,
  "CD", setdir, 0,
  "CH", change, 0,
  "CL", empty, 0,
  "CO", copy, 0,
  "CP", copy, 0,
  "CR", create, 1,
  "DE", delete, 0,
  "DF", fsstat, 0,
  "DI", dir, 0,
  "DU", dump, 0,
  "HE", help, 0,
  "LC", dir, 0,
  "LN", linkbtfile, 0,
  "LO", load, 1,
  "LS", dir, 0,
  "MO", mountfs, 0,
  "MV", move, 0,
  "PA", patch, 0,
  "PW", pwdir, 0,
  "RE", move, 0,
  "RM", delete, 0,
  "SE", setdir, 0,
  "ST", pstat, 0,
  "SY",	btsync,	0,
  "UM", unmountfs, 0,
  "UN", bunlock, 0,
  0,0,0
};

BTCB *b;
static char wsp[] = " \t\n";

volatile int cancel = 0;

/*ARGSUSED*/
static void cdecl handler(int sig) {
  cancel = 1;
}

static void docmd(char *line) {
  char *cmd;
  struct cmdlist *p;
  cmd = gettoken(line,wsp);
  if (!cmd) cmd = "";
  if (switch_char = strchr(cmd,'/')) *switch_char++ = 0;
  else switch_char = "";
  if (strncasecmp(cmd,"EN",2) == 0) exit(0);
  strlwr(switch_char);
  for (p = cmdlist; p->name; ++p) {
    if (strncasecmp(cmd,p->name,strlen(p->name)) == 0) {
      void cdecl (* saveint)(/**/ int /**/) = signal(SIGINT,handler);
#ifndef __MSDOS__
      void cdecl (* savehup)(/**/ int /**/) = signal(SIGHUP,handler);
#endif
      cancel = 0;
      if (p->flag) {
	/* try to open current directory for update */
	envelope
	  b = btopen("",BTRDWR+4,MAXREC);
	enverr
	  b = 0;
	envend
	if (!b) {
	  (void)fprintf(stderr,"Error: %d Can't open current directory.\n",
			errno);
	  return;
	}
      }
      envelope
	if ((*p->func)()) {
	  printf("Usage error: for help type HE %s\n",cmd);
	}
      enverr
	(void)printf("Fatal error %d\n",errno);
      envend
      if (p->flag)
	(void)btclose(b);
      (void)signal(SIGINT,saveint);
#ifndef __MSDOS__
      (void)signal(SIGHUP,savehup);
#endif
      if (cancel)
	(void)puts("Cancelled by user.");
      return;
    }
  }
  puts("For help, type HELP.");
}

/* print a long directory entry */

struct ls {
  time_t mtime;
  char txt[128];
};

/* insert ls record in most recent mod time order */
static int cmpls(const void *a,const void *b) {
  struct ls *aa = (struct ls *)a;	/* item to insert */
  struct ls *bb = (struct ls *)b;	/* item already there */
  if (aa->mtime > bb->mtime) return -1;
  return 1;
}

static void prtls(PTR a) {
  fputs(((struct ls *)a)->txt,stdout);
  free(a);
}

int dir() {
  int lflag = 'l';	/* list type flag */
  int dflag = 0;	/* list only directories? */
  int vflag = 0;	/* verbose listing? */
  int sflag = 0;	/* sort by time? */
  long	totfile = 0, totblks = 0;
  struct btff ff;
  char *s;
  TREE troot = 0;

  s = strpbrk(switch_char,"t1f");
  if (s) lflag = *s;
  if (strchr(switch_char,'d')) dflag = 1;
  if (strchr(switch_char,'v')) vflag = 1;
  if (strchr(switch_char,'s')) sflag = 1;
  switch (lflag) {
  case 't':
    printf("%-10s %8s %8s %-12s %s\n",
      "Mode","User","Group","Modified","Name"
    );
    break;
  case '1':
    break;
  case 'f':
    printf("%8s %3s %4s%5s%5s%6s%6s %s\n",
      "Root","Mid","Rlen","Klen","Flds","Kflds","Links","Name"
    );
    break;
  default:
    printf("%-10s %8s %8s %4s %8s %8s %s\n",
      "Mode","User","Group","Lock","Records","Blocks","Name"
    );
  }
  s = readtext((char *)0);
  if (!s) s = "*";
  do {
    if (findfirst(s,&ff)) {
      fprintf(stderr,"%s: not found\n",s);
      continue;
    }
    do {
      int rc;
      struct btstat st;
      struct btlevel fa;
      if (lflag == '1')
	printf("%s\n",ff.b->lbuf);
      else if (ff.b->klen = strlen(ff.b->lbuf),rc = btlstat(ff.b,&st,&fa))
	if (rc == BTERLINK) {
	  ff.b->lbuf[ff.b->rlen] = 0;
	  printf("%s --> %s\n",ff.b->lbuf,ff.b->lbuf+strlen(ff.b->lbuf)+1);
	}
	else
	  printf("%s <no access>\n", ff.b->lbuf);
      else if (dflag && !(st.id.mode & BT_DIR))
	continue;
      else if (lflag == 'f') {
	struct btflds f;
	ldflds(&f,ff.b->lbuf,ff.b->rlen);
	if (f.rlen == 0)
	  printf("%8lX %3d %-20s%6d %s\n",
	    fa.node,fa.slot,
	    (st.id.mode & BT_DIR) ? " DIR  ---" : "      ---",
	    st.links,ff.b->lbuf);
	else
	  printf("%8lX %3d %4d%5d%5d%6d%6d %s\n",
	    fa.node,fa.slot,
	    btrlen(&f),f.f[f.klen].pos,f.rlen,f.klen,st.links,ff.b->lbuf);
      }
      else {
	const char *perm = permstr(st.id.mode);
	char buf1[18], buf2[18];
#ifndef __MSDOS__
	static struct passwd *pw;
	static struct group *gr;
	if (!pw || pw->pw_uid != st.id.user) pw = getpwuid(st.id.user);
	if (!gr || gr->gr_gid != st.id.group) gr = getgrgid(st.id.group);
	if (pw)
	  strcpy(buf1,pw->pw_name);
	else
	  sprintf(buf1,"%d",st.id.user);
	if (gr)
	  strcpy(buf2,gr->gr_name);
	else
	  sprintf(buf2,"%d",st.id.group);
#else
	itoa(st.id.user,buf1,10);
	itoa(st.id.group,buf2,10);
#endif
	totfile++;
	totblks += st.bcnt;
	switch (lflag) {
	  char buf3[14];
	case 't':
	  timemask(st.mtime,"Nnn DD CCCCC",buf3);
	  if (sflag) {
	    struct ls *ls = (struct ls *)malloc(sizeof *ls);
	    if (!ls) {
	      puts("*** out of memory ***\n");
	      cancel = 1;
	    }
	    sprintf(ls->txt,"%10.10s %8.8s %8.8s %12.12s %.24s\n",
	      perm,buf1,buf2,buf3,ff.b->lbuf);
	    ls->mtime = st.mtime;
	    tsearch((PTR)ls,&troot,cmpls);
	  }
	  else {
	    printf("%10.10s %8.8s %8.8s %12.12s %s\n",
	      perm,buf1,buf2,buf3,ff.b->lbuf);
	  }
	  break;
	default:
	  printf("%s %8s %8s %4d %8ld %8ld %s\n",
	    perm,buf1,buf2, st.opens, st.rcnt,st.bcnt, ff.b->lbuf);
	}
      }
    } while (cancel == 0 && findnext(&ff) == 0);
  } while (s = readtext((char *)0));
  if (sflag)
    tclear(&troot,prtls);
  if (vflag)
    printf("Total: %ld files, %ld blocks, %ld bytes\n",totfile,totblks,
	    totblks*1024);	/* FIXME: ought to get blksize from BTSTAT */
				/* see stat1() in mount.c */
  endgrent();
  endpwent();
  return 0;
}

int setdir() {
  char *s = readtext((char *)0);
  if (s == 0 || *s == 0)
    s = "/";
  if (btchdir(s))
    (void)puts("Can't change directory.");
  pwdir();
  return 0;
}

int pwdir() {
  (void)printf("Using %s\n",btgetcwd());
  return 0;
}

int btsync() {
  BTCB btcb;
  (void)memset((char *)&btcb,0,sizeof btcb);
  do {
    envelope
    (void)btas(&btcb,BTFLUSH);
    errno = 0;
    envend
  } while (errno == BTERBMNT-256 || errno == BTERBUSY);
  if (errno) errpost(errno);
  return 0;
}
