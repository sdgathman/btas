#include <stdio.h>
#include <string.h>
#include "btutil.h"
#include "btar.h"

int archive() {
  int rc;
  char *s = readtext("Filename: ");
  char *t = readtext("Output file: ");
  if (btar_opennew(t,!strchr(switch_char,'t')))
    return 0;
  rc = btar_add(s,!!strchr(switch_char,'d'),!!strchr(switch_char,'s'));
  btar_close();
  if (rc) 
    fprintf(stderr,"Fatal error %d\n",rc);
  return 0;
}

static char *s;
static char dirflag;

static int
filter(const char *prefix,const char *dir,int ln,const struct btstat *bst) {
  fprintf(stderr,"%s: ",dir);
  if (match(dir,s))
    btar_extract(prefix,dir,ln,bst);
  else {
    fprintf(stderr," skipping, ");
    btar_skip(bst->rcnt);
    fprintf(stderr,"%ld records\n",btar_skip(bst->rcnt));
  }
  return cancel;
}

int load() {
  char *t, *d;
  s = readtext("Filename: ");
  t = readtext("Archive: ");
  d = readtext((char *)0);	/* dir option */
  dirflag = (d != 0);
  if (btar_load(t,filter) == 0)
    fprintf(stderr,"Archive file closed.\n");
  return 0;
}
