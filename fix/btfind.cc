#include <string.h>
extern "C" {
#include <btas.h>
#include <errenv.h>
}
#include "btfind.h"

btFind::btFind(const char *path) {
  b = btopendir(path,BTRDONLY+NOKEY);
  dir = path;
  pat = basename(path);
  slen = strcspn(pat,"*[?");
}

void btFind::path(char *buf) const {
  int len = pat - dir;
  memcpy(buf,dir,len);
  strcpy(buf+len,b->lbuf);
}

int btFind::first() {
  int rc;
  memcpy(b->lbuf,pat,slen);
  b->klen = slen;
  b->rlen = MAXREC;
  catch(rc)
    btas(b,BTREADGE);
  enverr
    return rc;
  envend
  return match(b->lbuf,pat) ? 0 : next();
}

int btFind::next() {
  int rc;
  catch(rc)
    for (;;) {
      b->klen = b->rlen;
      b->rlen = MAXREC;
      btas(b,BTREADGT);
      if (strncmp(b->lbuf,pat,slen)) {
	rc = 1;
	break;
      }
      if (match(b->lbuf,pat)) {
	rc = 0;
	break;
      }
    }
  envend
  return rc;
}

