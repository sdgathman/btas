#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <btflds.h>
#include <stdlib.h>
#include <assert.h>
#include <signal.h>
#include <errenv.h>
#include <isamx.h>
#include "obmem.h"
#include "config.h"
#include "cursor.h"
#include "object.h"

static void sqlexec(/**/ const struct sql_stmt * /**/);

//static __main() {}

int debug = 0;
int mapupper = 0;
volatile int cancel = 0;

static char verbose = 1;
static char errorch = 0;
static char formatch[] = " '";
static char echostmt = 0;
static volatile int inexec = 1;

static void cdecl trap(int sig) {
  signal(sig,trap);
  if (!inexec)
    exit(3);
  cancel = sig;
}

static struct obstack h;

int main(int argc,char **argv) {
  char *p;
  const char *prompt = "SQL> ";
  int lineno = 0;
  int interactive = isatty(0);
  int i;

  for (i = 1; i < argc; ++i) {
    if (*argv[i] == '-') {
      int j = 0;
      while (argv[i][++j]) {
      switch (argv[i][j]) {
      case 'V':
	puts(version);
	return 0;
      case 'v':
	debug = 1;
	continue;
      case 'x':
	echostmt = 1;
	continue;
      case 'i':
	interactive = 1;
	continue;
      case 'u':
	mapupper = 1;
	continue;
      case 'b':		/* uniplex 'batch' mode */
	verbose = 0;
	formatch[1] = 0;
	formatch[0] = '\t';
	continue;
      case 'q':		/* 'quiet' mode */
	verbose = 0;
	interactive = 0;
	continue;
      case 'p':
	if (argv[i][++j])
	  prompt = argv[i] + j;
	else if (argv[i + 1])
	  prompt = argv[++i];
	break;
      case 'f':		/* format & field separator */
	verbose = 0;
	if (argv[i][++j]) {
	  formatch[0] = argv[i][j];
	  if (isdigit(formatch[0]))
	    formatch[0] = atoi(argv[i]+j);
	  break;
	}
	else if (argv[i + 1]) {
	  formatch[0] = atoi(argv[++i]);
	  break;
	}
	break;
      case 'e':		/* escape (quote) character */
	verbose = 0;
	if (argv[i][++j]) {
	  formatch[1] = argv[i][j];
	  if (isdigit(formatch[1]))
	    formatch[1] = atoi(argv[i]+j);
	  break;
	}
	else if (argv[i + 1]) {
	  formatch[1] = atoi(argv[++i]);
	  break;
	}
	break;
      case 'E':		/* error prefix character */
	if (argv[i][++j]) {
	  errorch = atoi(argv[i]+j);
	  break;
	}
	else if (argv[i + 1]) {
	  errorch = atoi(argv[++i]);
	  break;
	}
      default:
fputs("\
Usage:	sql [options] [script]\n\
	-V	display version and exit\n\
	-i	force interactive mode\n\
	-v	debugging output\n\
	-u	map unquoted identifiers to upper case\n\
	-b	Uniplex 'batch' mode: quiet + tab seperated\n\
	-q	quiet mode - suppress banners & prompts\n\
	-x	echo statements before executing\n\
	-f n	set format & field separator\n\
	-e n	set escape (quote) character\n\
	-E n	write error lines prefixed by ascii(n) to stdout\n\
	-p 's'	set prompt to s\n\
	script	script file for batch processing\n\
",stderr);
#if 0
	if (freopen("ERROR.log","w+",stderr))
	  for (i = 0; i < argc; ++i)
	    fputs(argv[i],stderr), fputc((i+1<argc)?' ':'\n',stderr);
#endif
	
	return 1;
      }
      break;
      }
      continue;		/* continue *for* loop (not while) */
    }
    /* replace stdin with script file if specified */
    if (!freopen(argv[i],"r",stdin)) {
      perror(argv[i]);
      return 1;
    }
    interactive = 0;
  }

  obstack_init(&h);
  trap(SIGINT);
  trap(SIGTERM);
#ifndef __MSDOS__
  trap(SIGHUP);
#endif
  inexec = 0;
  if (verbose && interactive)
    puts(version);
  if (interactive)
    printf(prompt);
  fflush(stdout);
  for (;;) {
    int c;
    char squote = 0, dquote = 0, comment = 0, begline = 1;
    do {
      switch (c = getchar()) {
      case '#':
	if (begline) {
	  comment = 2;
	  c = '/';
	  obstack_1grow(&h,c);
	}
	break;
      case EOF:
	return 0;
      case 0:
	continue;
      case '\'':
	if (comment == 2) break;
	squote ^= 1; break;
      case '"':
	if (comment == 2) break;
	dquote ^= 1; break;
      case '/':
	if (comment == 2) break;	/* already in comment */
	if (squote || dquote) break;	/* in quote */
	++comment;
	break;
      case '\n':
	comment = 0;
	if (interactive) {
	  if (++lineno)
	    printf("%3d> ",lineno);
	  else
	    printf(prompt);
	  fflush(stdout);
	}
      }
      begline = 0;
      obstack_1grow(&h,c);
    } while (c != ';' || squote || dquote || comment == 2);

    obstack_1grow(&h,0);
    p = obstack_finish(&h);
    if (echostmt) puts(p);
    free_sql(&h);		/* init sql heap */
    sqlexec(parse_sql(p));
    obstack_free(&h,p);
    if (cancel) {
      if (cancel != SIGINT) return 1;
      puts("\n** CANCELED **");
    }
    lineno = -1;
  }
}

PTR xmalloc(unsigned size) {
  PTR p = malloc(size);
  if (!p && size) {
    fputs("Out of memory!\n",stderr);
    /* abort();	/* let debugger show us how we used it up */
    exit(1);	/* tricky to close files, etc. with objects */
		/* need catch, throw */
  }
  return p;
}

static void report(long cnt,const char *verb) {
  if (verbose)
    fprintf(stderr,"%ld record%s %s\n",cnt,(cnt == 1) ? "" : "s",verb);
}

static void uniplex_print(Cursor *c, enum Column_type type, int sep) {
  int i = 0;
  for (;;) {
    sql y = tostring(do0(c->col[i],load));
    const char *p = (y->op == EXSTRING) ? y->u.name[0] : "";
    fputs(p,stdout);
    if (++i < c->ncol)
      putchar(sep);
    else {
      putchar('\n');
      break;
    }
  }
}

static void sqlexec(const struct sql_stmt *cmd) {
  Cursor *t = 0, *s = 0;
  sql x;
  long cnt = 0L;
  if (!cmd) return;
  inexec = 1;
  cancel = 0;
  envelope
  switch (cmd->cmd) {
  case SQL_USE:
    if (btchdir(cmd->val->u.name[0]))
      fprintf(stderr,"Can't change directory to %s\n",cmd->val->u.name[0]);
    else if (verbose)
      fprintf(stderr,"Default directory is now %s\n",cmd->val->u.name[0]);
    break;
  case SQL_SELECT:
    t = Select_init(cmd->src,1,0);
    if (t) {
      if (verbose) {
	do2(t,print,TITLE,formatch);
	do2(t,print,SCORE,formatch);
      }
      if (!formatch[1])
        t->_class->print = uniplex_print;
      if (do0(t,first) == 0) {
	do
	  do2(t,print,formatch[0] == ' ' ? VALUE : DATA,formatch);
	while (do0(t,next) == 0 && !cancel);
      }
    }
    break;
  case SQL_DROP:
    for (x = cmd->dst->table_list; (x = x->u.opd[1]) != 0;) {
      const char *path = x->u.opd[0]->u.name[0];
      if (iserase(path) == 0)
	fprintf(stderr,"Table %s deleted\n",path);
      else
	fprintf(stderr,"Table %s not deleted\n",path);
    }
    break;
  case SQL_DELETE:
    if (!cmd->dst->where_sql) {		/* clear table */
      for (x = cmd->dst->table_list; (x = x->u.opd[1]) != 0;) {
	const char *path = x->u.opd[0]->u.name[0];
	if (btclear(path) == 0)
	  fprintf(stderr,"Table %s cleared\n",path);
	else
	  fprintf(stderr,"Table %s not cleared\n",path);
      }
      break;
    }
    t = Select_init(cmd->dst,0,0);
    if (t) {
      if (do0(t,first) == 0) {
	do {
	  if (do0(t,del) == 0) ++cnt;
	} while (do0(t,next) == 0 && !cancel);
      }
      report(cnt,"deleted");
    }
    break;
  case SQL_UPDATE:
    t = Select_init(cmd->dst,0,0);
    if (t) {
      if (do0(t,first) == 0) {
	do {
	  if (debug)
	    do2(t,print,formatch[0] == ' ' ? VALUE : DATA,formatch);
	  for (x = cmd->dst->select_list; (x = x->u.opd[1]) != 0; ) {
	    sql e = x->u.opd[0],c,res;
	    int rc;
	    assert(e->op == EXASSIGN);
	    c = e->u.opd[0];
	    assert(c->op == EXCOL);
	    res = sql_eval(e->u.opd[1],MAXTABLE);
	    if (debug) {
	      fprintf(stderr,"%s = ",c->u.col->name);
	      sql_print(res,1);
	    }
	    rc = do2(c->u.col,store,res,c->u.col->buf);
	    rmexp(res);
	    if (rc) {
	      fprintf(stderr,"Can't convert %s\n",c->u.col->name);
	      break;
	    }
	  }
	  if (!x && do0(t,update) == 0) ++cnt;
	} while (do0(t,next) == 0 && !cancel);
      }
      report(cnt,"updated");
    }
    break;
  case SQL_INSERT:
    t = Select_init(cmd->dst,0,0);
    if (cmd->src) {		/* INSERT ... SELECT */
      s = Select_init(cmd->src,1,0);
      if (s) {
	if (!t) {
	  struct btflds f;
	  int i,pos = 0;
	  x = cmd->dst->table_list->u.opd[1];
	  assert(x != 0);
	  if (x->u.opd[1]) break;	/* can create only one table now */
	  x = x->u.opd[0];
	  assert(x->op == EXNAME);
	  f.klen = s->klen;
	  f.rlen = s->ncol;
	  if (!f.klen) f.klen = f.rlen;
	  for (i = 0; i < f.rlen; ++i) {
	    f.f[i].pos = pos;
	    f.f[i].type = s->col[i]->type;
	    f.f[i].len  = s->col[i]->len;
	    pos += f.f[i].len;
	  }
	  f.f[i].pos = pos;
	  f.f[i].type = 0;
	  fprintf(stderr,"Creating new table %s\n",x->u.name[0]);
	  f.f[i].len = 0;
	  if (btcreate(x->u.name[0],&f,0666)) break;
	  fputs("New table created\n",stderr);
	  t = Table_init(x->u.name[0],0);
	  if (!t) break;
	}
	if (s->ncol != t->ncol)
	  fprintf(stderr,"INSERT must have same number of columns, %d != %d\n",
	    s->ncol,t->ncol);
	else if (do0(s,first) == 0) {
	  do {
	    int i;
	    for (i = 0; i < s->ncol; ++i) {
	      Column *x = s->col[i], *y = t->col[i];
	      if (x->_class == y->_class && x->len == y->len)
		/* copy directly */
		do1(s->col[i],copy,t->col[i]->buf);
	      else if (do2(y,store,do0(x,load),y->buf)) {
		fprintf(stderr,"Can't convert %s to %s\n",x->name,y->name);
		break;
	      }
	    }
	    if (i == s->ncol && do0(t,insert) == 0) ++cnt;
	  } while (do0(s,next) == 0 && !cancel);
	}
      }
    }
    report(cnt,"inserted");
    break;
  case SQL_CREATE:
    {
      struct btflds f;
      int i,pos = 0;
      const char *tblname;
      x = cmd->dst->table_list;
      assert(x != 0);
      assert(x->op == EXNAME);
      tblname = x->u.name[0];
      x = cmd->val->u.opd[1];
      for (i = 0; x && i < MAXFLDS; ++i) {
        sql typ = x->u.opd[0];
	assert(typ->op == EXTYPE);
	f.f[i].pos = pos;
	f.f[i].type = typ->u.t.type;
	f.f[i].len  = typ->u.t.len;
	pos += typ->u.t.len;
	x = x->u.opd[1];
      }
      f.f[i].pos = pos;
      f.f[i].type = 0;
      f.f[i].len = 0;
      f.rlen = i;
      f.klen = f.rlen;	/* FIXME */
      fprintf(stderr,"Creating new table %s, cols = %d, rlen = %d\n",
        tblname,i,pos);
      if (btcreate(tblname,&f,0666)) break;
    }
    break;
  }
  enverr
    fprintf(stderr,"Fatal error %d\n",errno);
  envend
  inexec = 0;
  if (t) do0(t,free);
  if (s) do0(s,free);
}

void sql_syntax(const char *s,const char *p,const char *lexptr) {
  int pos = 5, lineno = 0;
  const char *q = p;
  printf("*** %s ***\n",s);
  while (p < lexptr) {
    if (*p++ == '\n') {
      q = p;
      pos = 5;
      ++lineno;
    }
    else
      ++pos;
  }
  printf("%3d> ",lineno);
  while (*q && *q != '\n')
    putchar(*q++);
  putchar('\n');
  while (pos--)
    putchar(' ');
  puts("^");
}
