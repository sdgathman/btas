#include <stdlib.h>
#include <stdio.h>
#include <errenv.h>
#include <btas.h>
#include <string.h>

#ifndef __MSDOS__
#include <pwd.h>
#include <grp.h>
struct passwd *getpwnam(/**/ const char * /**/);
struct group *getgrnam(/**/ const char * /**/);
#endif

#include "btutil.h"

static void getnum(/**/ const char *, short * /**/);

int change()
{
  char *u = readtext("User id: ");
  char *g = readtext("Group id: ");
  char *m = readtext("File mode: ");
  char *s = readtext("Filename: ");
  struct btff ff;
  int rc;
  do {
    rc = findfirst(s,&ff);
    if (rc) {
      (void)fprintf(stderr,"%s: not found\n",s);
      continue;
    }
    while (rc == 0) {
      struct btstat st;
      BTCB *b = 0;
      envelope
	b = btopen(ff.b->lbuf,BTNONE+4,sizeof st);
	b->rlen = sizeof st;
	b->klen = 0;
	if (btas(b,BTSTAT) == 0) {
	  (void)memcpy((char *)&st,b->lbuf,sizeof st);
	  if (u) {
#ifndef __MSDOS__
	    struct passwd *pw = getpwnam(u);
	    if (pw)
	      st.id.user = pw->pw_uid;
	    else
#endif
	      getnum(u,&st.id.user);
	  }
	  if (g) {
#ifndef __MSDOS__
	    struct group *gr = getgrnam(g);
	    if (gr)
	      st.id.group = gr->gr_gid;
	    else
#endif
	      getnum(g,&st.id.group);
	  }
	  getnum(m,&st.id.mode);
	  (void)memcpy(b->lbuf,(char *)&st,sizeof st);
	  b->u.id.user = geteuid();
	  b->u.id.group = getegid();
	  b->u.id.mode = 0666;
	  b->klen = sizeof st;
	  (void)btas(b,BTTOUCH);
	}
      enverr
	(void)fprintf(stderr,"%s: fatal error %d\n",ff.b->lbuf,errno);
      envend
      btclose(b);
      rc = findnext(&ff);
    }
  } while (s = readtext((char *)0));
  return 0;
}

static void getnum(s,sp)
  const char *s;
  short *sp;
{
  if (s) {
    if (*s == '0') {
      if (s[1] == 'x' || s[1] == 'X')
	*sp = strtol(s,(char **)0,16);
      else
	*sp = strtol(s,(char **)0,8);
    }
    else if (*s >= '1' && *s <= '9')
      *sp = strtol(s,(char **)0,10);
  }
}
