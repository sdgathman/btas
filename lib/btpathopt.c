/*
	Optimize path (find shorter equivalent path)
	return open BTCB just like btopen().
	We try to be smart and efficient, but we aren't perfect.
	We check our work by opening the optimized and unoptimized
	pathnames and comparing root,mid.  In the worst case, we
	leave the path unchanged (but correct).  We will never make the
	path longer.
*/
#include <btas.h>
#include <string.h>

int btpathopt(BTCB *b, char *name) {
  BTCB *t;
  char path[MAXKEY];
  int rc = -1;
  pathopt(path,name);
  /* printf("name=%s\npath=%s\n",name,path); */
  /* now check result */
  t = btopen(path,BTNONE+BTDIROK+NOKEY,0);
  if (t) {
    if (b->root == t->root && b->mid == t->mid)
      strcpy(name,path), rc = 0;
    btclose(t);
  }
  return rc;
}
