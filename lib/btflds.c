#include <stdlib.h>
#include <string.h>
#include "btflds.h"

struct btflds *ldflds(fcb,buf,len)	/* cvt fld data */
  struct btflds *fcb;       /* fields structure, malloc if NULL */
  const char *buf;            /* directory record */
  int len;			/* size of directory record */
{
  register int rlen;
  rlen = strlen(buf) + 1;
  buf += rlen;			/* skip filename */
  rlen = (len - rlen - 1) / 2;	/* max number of fields */
  if (fcb == 0)
    fcb = (struct btflds *)malloc(
		sizeof *fcb - sizeof fcb->f + (rlen+1) * sizeof fcb->f[0]
	     			 );
  if (rlen <= 0) {
    fcb->klen = fcb->rlen = 0;
    return fcb;
  }
  if (fcb) {
    register struct btfrec *p;
    short pos = 0;
    fcb->klen = *buf++;
    fcb->rlen = 0;
    for (p = fcb->f; fcb->rlen < rlen && *buf; ++p,++fcb->rlen) {
      p->type = *buf++;
      p->len = *buf++;
      p->pos = pos;
      pos += p->len;
    }
    p->type = 0;
    p->len  = 0;
    p->pos  = pos;
  }
  return fcb;
}
