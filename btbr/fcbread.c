/*
	read fcb file and return pointer to FLD array
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <btflds.h>
#include "fld.h"
#include <obstack.h>
#include <errenv.h>

static struct obstack ob;

#define obstack_chunk_free free

char *obstack_chunk_alloc(unsigned size) {
  register char *p = malloc(size);
  if (!p) errpost(1);
  return p;
}

char *strread(FILE *fp,const char *term) {
  register int c;
  char *p;
  for (;;) {
    c = getc(fp);
    if (c == EOF || strchr(term,c)) {
      ungetc(c,fp);
      c = 0;
    }
    obstack_1grow(&ob,c);
    if (c == 0) {
      p = obstack_finish(&ob);
      if (!*p) {
	obstack_free(&ob,p);
	return "";
      }
      return p;
    }
  }
}

int numread(FILE *fp,const char *term) {
  register int c;
  int val = -1;
  for (;;) {
    c = getc(fp);
    if (c == EOF) return val;
    if (strchr(term,c)) {
      ungetc(c,fp);
      return val;
    }
    if (c < '0' || c > '9') continue;
    if (val < 0) val = 0;
    val = val * 10 + c - '0';
  }
}
    
int fldread(FILE *fp,FLD *fld,char *buf,int rlen) {
  register int c;
  int pos, step = 0;
  static char sep[] = " \t\n";
  for (;;) {
    switch (c = getc(fp)) {
    case EOF:
      if (step == 0) return -1;
    case '\n':
      if (step == 0) continue;	/* skip empty lines */
      if (strpbrk(fld->format,"#zZ")) {
	char *mask;
	fld->put = BT_NUM;
	if (mask = strchr(fld->format,'.'))	/* is there a decimal pt.? */
	  while (*++mask && strchr("zZ#",*mask)) ++fld->put;/* count places */
	fld->dlen = strlen(fld->format);
	fld->type = BT_NUM;
      }
      else {
	switch (*fld->format) {
	case 'T':
	  fld->put = fld->type = BT_TIME;
	  fld->dlen = 19;
	  fld->format = 0;
	  break;
	case 'D':
	case 'J':
	  fld->put = fld->type = BT_DATE;
	  fld->dlen = 8;
	  fld->format = 0;
	  break;
	case 'X':
	  fld->put = fld->type = BT_BIN;
	  fld->dlen = fld->len * 2;
	  fld->format = 0;
	  break;
	case 'C':
	  fld->format = ++fld->format;
	case 'R':
	  fld->put = fld->type = BT_CHAR;
	  fld->dlen = fld->len;
	  break;
	default:
	  fld->put = fld->type = BT_BIN;
	  fld->dlen = fld->len * 2;
	}
      }
      return 0;
    default:
      ungetc(c,fp);
      switch (step++) {
      case 0:
	fld->name = strread(fp,sep);
	fld->desc = fld->name;
	fld->len = 0;
	fld->data = buf;
	fld->format = "Incomplete Field";
	break;
      case 1:
	pos = numread(fp,sep);
	if (pos < 0) {
	  fld->format = "Offset Error";
	  step = 4;
	}
	break;
      case 2:
	fld->len = numread(fp,sep);
	if (fld->len < 0 || pos+fld->len > rlen) {
	  fld->format = "Length Error";
	  step = 4;
	}
	else
	  fld->data += pos;
	break;
      case 3:
	fld->format = strread(fp,sep);
	break;
      case 4:
	fld->desc = strread(fp,sep+2);
      }
    case ' ': case '\t':
      ;
    }
  }
}

FLD *fcbread(const char *name,char *buf,int rlen) {
  register FLD *fld;
  FLD *prev, *f1;
  static char init = 0;
  static char ext[] = ".fcb";
  char *tmp, *base;
  FILE *fp = 0;
  int rc;

  if (!init) {
    obstack_init(&ob);
    init = 1;
  }
  base = obstack_base(&ob);
  envelope
    tmp = getenv("FCB");
    if (tmp && *tmp) {
      obstack_grow(&ob,tmp,strlen(tmp));
      obstack_1grow(&ob,'/');
    }
    obstack_grow(&ob,name,strlen(name));
    tmp = obstack_copy(&ob,ext,sizeof ext);
    tmp[strlen(tmp)-4] = 0;	/* check for file without ".fcb" */
    fp = fopen(tmp,"r");
    if (!fp) {
      tmp[strlen(tmp)] = '.';	/* check for file with ."fcb" */
      fp = fopen(tmp,"r");
    }
    rc = errno;
    obstack_free(&ob,tmp);	/* don't need name any more */
    if (!fp) errpost(rc);		/* couldn't open file */
    f1 = fld = (FLD *)obstack_alloc(&ob,sizeof *fld);
    while (fldread(fp,fld,buf,rlen) == 0) {
      fld->next = (FLD *)obstack_alloc(&ob,sizeof *fld);
      prev = fld;
      fld = fld->next;
    }
    obstack_free(&ob,fld);
    prev->next = 0;
  enverr
    obstack_free(&ob,base);	/* delete any storage used */
    f1 = 0;
  envend
    if (fp != 0) fclose(fp);
  return f1;
}

void fcbfree(fld)
  FLD *fld;
{
  FLD *nfld;
  while (fld) {
    nfld = fld->next;
    obstack_free(&ob,fld);
    fld = nfld;
  }
}
