#include <string.h>

char *pathopt(char *path,const char *p) {
  /* apply standard .,.. convention */
  char *op = path;
  while (p) {
    int len;
    const char *w = p;
    if (p = strchr(w,'/')) {
      len = p++ - w;
    }
    else
      len = strlen(w);
    memcpy(op,w,len);
    op[len] = 0;
    if (strcmp(op,"..") == 0) {
      if (op > path + 1) {
	char *q;
	*--op = 0;
	if (q = strrchr(path,'/'))
	  ++q;
	else
	  q = path;
	if (strcmp(q,"..")) {
	  op = q;
	  continue;
	}
	*op++ = '/';
      }
    }
    else if (strcmp(op,".") == 0) {	/* strip "." */
      continue;
    }
    op += len;
    *op++ = '/';
  }
  if (op > path + 1) --op;
  *op = 0;
  return path;
}
