/*
	Reset lock count on files.  Needed when kernel cannot detect
	application termination or does not track open files.
*/
#include <stdio.h>
#include <string.h>
#include <errenv.h>
#include <bterr.h>
#include "btutil.h"

int bunlock()
{
  char *s = readtext("File to unlock: ");
  struct btff ff;
  int count = atoi(switch_char);
  if (!count) count = 1;
  do {
    if (findfirst(s,&ff)) {
      (void)printf("%s: not found\n",s);
      continue;
    }
    envelope
      do {
	int n;
	for (n=count;n--;) {
	  BTCB *b = btopen(ff.b->lbuf,BTNONE+BTEXCL+LOCK+4,strlen(ff.b->lbuf));
	  if (b->op == BTERLOCK) b->flags = 1;
	  (void)btclose(b);
  	}
      } while (findnext(&ff) == 0);
    enverr
      finddone(&ff);
    envend
  } while (s = readtext((char *)0));
  return 0;
}
