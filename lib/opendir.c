#include <string.h>
#include <btas.h>

BTCB *btopendir(const char *path,int mode) {
  char savbuf[MAXKEY + 1];
  BTCB *b;
  strncpy(savbuf,path,MAXKEY)[MAXKEY] = 0;
  b = btopen(dirname(savbuf),mode|BTDIROK,MAXREC);	/* open directory */
  strncpy(b->lbuf,basename(path),MAXKEY)[MAXKEY] = 0;
  b->klen = strlen(b->lbuf) + 1;
  return b;
}
