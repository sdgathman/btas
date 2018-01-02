/*
    This file is part of the BTAS client library.

    The BTAS client library is free software: you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public License as
    published by the Free Software Foundation, either version 3 of the License,
    or (at your option) any later version.

    BTAS is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with BTAS.  If not, see <http://www.gnu.org/licenses/>.
*/
#include <stdlib.h>
#include <btas.h>
#include <string.h>
#include <errenv.h>

int findfirst(const char *name,struct btff *ff) {
  const char *p;
  int rc;

  /* find filename portion */
  ff->s = name;

  p = strrchr(ff->s,'/');
  if (p) {
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
