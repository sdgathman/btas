#include <stdio.h>
#include <string.h>
#include "btutil.h"

#ifdef __MSDOS__
char helpfile[] = "/lib/btutil.hlp";
#else
static char helpfile[] = "/usr/share/btas/btutil.help";
#endif

static FILE *f;

int help(void)
{
  char *cmd = readtext((char *)0);
  char buf[256], chain[3];
  char searchmode;
  if (!f) {
    f = fopen(helpfile,"r");
    if (!f) {
      puts("No help available!");
      printf("(%s not found)\n",helpfile);
      return 0;
    }
  }
  else
    rewind(f);

  searchmode = (cmd && *cmd);
  if (searchmode) {
    strncpy(chain,cmd,2);
    chain[2] = 0;
    strupr(chain);
    cmd = chain;
  }

  while (fgets(buf,sizeof buf,f)) {
    if (searchmode) {
      if (buf[0] != '.') continue;
      if (strncmp(buf+1,cmd,2)) continue;
      searchmode = 0;
      continue;
    }
    if (buf[0] == '-') {
      buf[3] = 0;
      strcpy(chain,buf+1);
      cmd = chain;
      searchmode = 1;
      continue;
    }
    if (buf[0] == '.') {
      if (buf[1] == '.') {
	char quit = 0;
	printf("Press return to continue (q to exit help) . . . ");
	fflush(stdout);
	while (!cancel) {
	  switch (getchar()) {
	  case '\n':
	    break;
	  case 'q': case 'Q':
	    quit = 1;
	  default:
	    continue;
	  }
	  if (quit) return 0;
	  break;
	}
	continue;
      }
      break;
    }
    fputs(buf,stdout);
  }
  if (searchmode)
    printf("No help found for %s.\n",cmd);
  return 0;
}
