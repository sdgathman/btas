#include <stdlib.h>
#include <btas.h>
#include <string.h>
#include <errenv.h>

int findfirst(const char *name,struct btff *ff) {
  const char *p;
  int rc;

  /* find filename portion */
  ff->s = name;

  if (p = strrchr(ff->s,'/')) {
    ff->savdir = btgetdir();
    if (p == ff->s)
      ff->dir = "/";
    else {
      int len = p - ff->s;
      ff->dir = malloc(len + 1);
      memcpy(ff->dir,ff->s,len);
      ff->dir[len] = 0;
    }
    ff->s = p+1;
    rc = btchdir(ff->dir);
  }
  else {
    ff->dir = "";
    ff->savdir = 0;
    rc = 0;
  }

  if (rc || (ff->b = btopen("",BTRDONLY + NOKEY + 4,MAXREC)) == 0) {
    finddone(ff);
    return 1;
  }
  if (*ff->s == 0) ff->s = "*";
  ff->slen = strcspn(ff->s,"*[?");
  (void)memcpy(ff->b->lbuf,ff->s,ff->slen);
  ff->b->klen = ff->slen;
  ff->b->rlen = MAXREC;
  catch(rc)
    rc = btas(ff->b,BTREADGE + NOKEY);
  envend
  if (rc) {
    finddone(ff);
    return rc;
  }
  return match(ff->b->lbuf,ff->s) ? 0 : findnext(ff);
}

int findnext(struct btff *ff) {
  int rc;
  catch(rc)
    for (;;) {
      ff->b->klen = (ff->b->rlen < MAXKEY) ? ff->b->rlen : MAXKEY;
      ff->b->rlen = MAXREC;
      if (btas(ff->b,BTREADGT+NOKEY) || strncmp(ff->b->lbuf,ff->s,ff->slen)) {
	rc = 1;
	break;
      }
      if (match(ff->b->lbuf + ff->slen,ff->s + ff->slen)) {
	rc = 0;
	break;
      }
    }
  envend
  if (rc) finddone(ff);
  return rc;
}

void finddone(struct btff *ff) {
  if (*ff->dir) {
    if (strcmp(ff->dir,"/"))
      free(ff->dir);
    ff->dir = "";		/* allow multiple finddone() */
    btputdir(ff->savdir);
  }
  btclose(ff->b);
  ff->b = 0;
}
