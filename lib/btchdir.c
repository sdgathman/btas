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
 *
 * $Log$
 * Revision 1.4  2012/03/14 22:03:26  stuart
 * Fix some warnings.  basename is a problem.
 *
 * Revision 1.3  2006/06/23 18:09:30  stuart
 * fix btgetdir() when BTASDIR not set
 *
 * Revision 1.2  2003/11/12 23:41:11  stuart
 * btchdir needs to save BTASDIR env even when btasdir is null.
 *
 */
#include <stdio.h>
#include <btas.h>
#include <string.h>
#include <stdlib.h>

int btchdir(const char *name) {
  BTCB *b;
  if (!name || !*name) return 0;	/* no change */
  if (strcmp(name,"/") == 0)
    b = 0;				/* root dir */
  else {
    int len = strlen(name) + 1;
    if (name[0] != '/')
      len += strlen(btgetcwd());
    len += 9;		/* room for "BTASDIR=" */
    b = btopen(name,BTERR+BTNONE+BTDIROK+8,len);
    if (b->flags == 0) {
      btclose(b);
      return -1;
    }
    b->lbuf[0] = 0;
    if (name[0] != '/' && btasdir) {
      strcat(b->lbuf,btasdir->lbuf);
      strcat(b->lbuf,"/");
    }
    else {
      strcat(b->lbuf,"BTASDIR=/");
      if (*name == '/') ++name;
    }
    strcat(b->lbuf,name);
    btpathopt(b,b->lbuf + 8);
    b->rlen = strlen(b->lbuf);
  }
  btputdir(b);
  return 0;
}

BTCB *btgetdir() {
  BTCB *b;
  if (btasdir) {
    b = btopen("",BTNONE+BTDIROK+8,btasdir->rlen);/* open current directory */
    strcpy(b->lbuf,btasdir->lbuf); /* copy directory environment */
    b->rlen = btasdir->rlen;
  }
  else {
    const char *s = getenv("BTASDIR");
    int len;
    if (s == 0) s = "";
    len = strlen(s) + 9;
    b = btopen("",BTNONE+BTDIROK+8,len);/* open current directory */
    sprintf(b->lbuf,"BTASDIR=%s",s);
    b->rlen = len;
  }
  return b;
}

char *btgetcwd() {
  char *p;
  if (btasdir) return strchr(btasdir->lbuf,'/');
  p = getenv("BTASDIR");
  if (!p || *p != '/') return "/";
  return p;
}
