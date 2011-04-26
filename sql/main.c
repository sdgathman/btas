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

//static __main() {}

static char verbose = 1;
static char errorch = 0;
static char formatch[] = " '";
static char echostmt = 0;

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
	yydebug = 1;
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
    sqlexec(parse_sql(p),formatch,verbose);
    obstack_free(&h,p);
    if (cancel) {
      if (cancel != SIGINT) return 1;
      puts("\n** CANCELED **");
    }
    lineno = -1;
  }
}
