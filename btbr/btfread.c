#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <btflds.h>
#include "fld.h"

static void typeform(type,len,form,dlen)
  int *type,len;
  char **form;
  short *dlen;
{
  if (*type&BT_NUM) {
    static char mtbl[8] = { 0, 4, 6, 9, 11, 13, 16, 0 };
    char mask[20];
    int l;
    l = mtbl[len & 7] + (*type > BT_NUM);
    memset(mask,'Z',l);
    mask[l-1] = '#';
    mask[l] = 0;
    mask[0] = '-';
    if (*type > BT_NUM)
      mask[l - *type + BT_NUM - 1] = '.';
    *dlen = l;
    *form = strdup(mask);
  }
  else {
    *form = 0;
    switch (*type) {
    case BT_TIME: *dlen = 19;		break;
    case BT_DATE: *dlen = 8;		break;
    case BT_CHAR: *dlen = len;		break;
    default:
      /* unknown types get changed to BT_BIN */
      *type = BT_BIN;
      *dlen = len * 2;
      break;
    }
  }
}

static char *mkname(i)
  int i;
{
  static char name[10];
  sprintf(name,"#%d",i);
  return name;
}

static char *mkoname(off,len,type)
  int off,len,type;
{
  static char name[132+1];
  if (type != BT_BIN) {
    sprintf(name,"%d",off);
  }
  else {	/* Hex offsets for description */
    int l;
    char *s = name;
    while (len-- > 0) {
      if (off%10 == 0 || off%5 == 0 || s == name) {
	sprintf(s,"%d",off);
	l = strlen(s);
	s += l;
	if (l == 1) *s++ = '-';
	else if (l > 2) {
	  l -= 2;
	  while (l) {
	    *s++ = '-';
	    if (l%2) { --len; ++off; }
	    l--;
	  }
	}
      }
      else {
        *s++ = '-';
        *s++ = '-';
      }
      ++off;
    }
    *s = 0;
  }
  return name;
}

FLD *btfread(name,buf,rlen)
  char *name,*buf;
  int rlen;
{
  BTCB *b;
  char tbuf[MAXREC];
  struct btflds bf;
  int i, dlen = 0, len = 0, offset;
  FLD *retfld = 0,*fld = 0;

  b = btopen(name,BTRDONLY,sizeof tbuf);	/* open file */
  if (!b) return 0;
  ldflds(&bf,b->lbuf,b->rlen);
  for (i=0; i < bf.rlen;) {
    if (!fld) {
      fld = malloc(sizeof *fld);
      retfld = fld;
    }
    else {
      fld->next = malloc(sizeof *fld);
      fld = fld->next;
    }
    if (!fld) return 0;
    offset = bf.f[i].pos + len;
    fld->data = buf+offset;
    fld->type = bf.f[i].type;
    fld->put = bf.f[i].type;
    fld->len = bf.f[i].len;
    typeform(&fld->put,fld->len,&fld->format,&fld->dlen);
    /* split field into multipule fields if dlen > 80 */
    if (dlen != 0) {
      fld->dlen = dlen;
      fld->len = dlen;
      if (fld->type&BT_BIN) fld->len /= 2;
    }
    if (fld->dlen > 80) {
      dlen = fld->dlen - 80;
      fld->dlen = fld->len = 80;
      if (fld->type&BT_BIN) fld->len /= 2;
      len += fld->len;
    }
    else {
      ++i;
      dlen = len = 0;
    }
    fld->name = strdup(mkname(i));
    fld->desc = strdup(mkoname(offset,fld->len,fld->type));
  }
  if (fld) fld->next = 0;
  btclose(b);
  return retfld;
}
