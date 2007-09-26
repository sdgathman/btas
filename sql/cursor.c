#include <stdio.h>
#include <string.h>
#include "config.h"
#include <btas.h>
#include <btflds.h>
#include <ftype.h>
#include "object.h"
#include "cursor.h"
#include "coldate.h"

void Cursor_print(Cursor *c,enum Column_type type,const char *s) {
  char sep = s[0];
  char quote = s[1];
  int i = 0;
  if (type < VALUE && sep != ' ') return;
  if (type == TITLE)
    puts("");
  for (;;) {
    Column *col = c->col[i];
    if (type == DATA) {
      char *p = (char *)alloca(col->width+1);
      /* printf("w = %2d ",col->width); */
      p[col->width] = 0;
      do2(col,print,type,p);
      if (strchr(p,quote) || sep && strchr(p,sep)) {
	putchar(quote);
	while (*p) {
	  if (*p == quote)
	    putchar(quote);
	  putchar(*p++);
	}
	putchar(quote);
      }
      else
	fputs(p,stdout);
    }
    else
      printColumn(col,type);
    if (++i < c->ncol) {
      if (sep) putchar(sep);
    }
    else {
      putchar('\n');
      break;
    }
  }
}

void Cursor_optim(Cursor *c,Column **cc,int k,int n) {
  /* trivial table optimizer */
}

int Cursor_fail(Cursor *c) {
  return -1;
}

/*
	Lookup data dictionary info
*/

#define FNAMESIZE	18
struct dictrec {
  char file[FNAMESIZE];
  FSHORT seq;
  FSHORT pos;
  char name[8];
  char type[2];
  FSHORT len;
  char fmt[15];
  char desc[24];
};

static BTCB *tryname(const char *,struct btflds *,const char *);

static BTCB *tryname(const char *name,struct btflds *f,const char *dictname) {
  BTCB *b;
  char path[MAXKEY];

  if (strlen(name) + strlen(dictname) >= MAXKEY)
    return 0;
  strcpy(path,name);
  strcpy((char *)basename(path),dictname);

  b = btopen(path,BTRDONLY+NOKEY,sizeof (struct dictrec));
  if (b->flags == 0) {
    btclose(b);
    return 0;
  }
  ldflds(f,b->lbuf,b->rlen);
  name = basename((char *)name);
  strncpy(b->lbuf,name,FNAMESIZE);
  b->klen = (short)strlen(name) + 1;
  if (b->klen > FNAMESIZE) b->klen = FNAMESIZE;
  b->rlen = sizeof (struct dictrec);
  if (btas(b,BTREADF+NOKEY)) {
    btclose(b);
    return 0;
  }
  return b;
}

/* temporary version of a field for massaging */

struct dict {
  short pos,len;
  char type[2];
  char name[9];	/* field name */
  char fmt[16];
  char desc[25];
};

void getdict(Cursor *cur,const char *name,char *ubuf,const struct btflds *f) {
  BTCB *b;
  struct dict d[MAXFLDS];
  int i;
  short maxpos;
  struct btflds flds;

  /* first try to find DATA.DICT in the same directory as the file */

  b = tryname(name,&flds,"DATA.DICT");
  name = basename((char *)name);
  if (!b)
    b = tryname(name,&flds,"/DATA.DICT");
  if (!b) {
    if (!f) return;
    /* no data dictionary entry found - generate numbered fields */
    cur->ncol = f->rlen;
    cur->klen = f->klen;
    cur->col = (Column **)xmalloc(sizeof *cur->col * cur->ncol);
    for (i = 0; i < cur->ncol; ++i) {
      char sbuf[8];
      sprintf(sbuf,"#%d",i+1);
      cur->col[i] = Column_gen(ubuf,f->f + i,sbuf);
    }
    return;
  }

  /* read all fields into memory.  They are not necessarily in order by
     position and we need to have key fields in order at the front. */

  for (cur->ncol = 0; cur->ncol < MAXFLDS;) {
    struct dictrec drec;
    struct dict *dp = &d[cur->ncol];
    b2urec(flds.f,(char *)&drec,sizeof drec,b->lbuf,b->rlen);
    dp->pos = ldshort(drec.pos);
    dp->len = ldshort(drec.len);
    if (dp->pos + dp->len <= btrlen(f) && dp->len < 256) {
      memcpy(dp->type,drec.type,sizeof dp->type);
      ldchar(drec.name,sizeof drec.name,dp->name);
      ldchar(drec.fmt,sizeof drec.fmt,dp->fmt);
      ldchar(drec.desc,sizeof drec.desc,dp->desc);
      ++cur->ncol;
    }
    b->klen = b->rlen;
    b->rlen = sizeof drec;
    if (btas(b,BTREADGT+NOKEY) || strncmp(b->lbuf,name,sizeof drec.file))
      break;
  }
  btclose(b);
#if 0
  /* sort by pos to move key fields to beginning.
     This determines the order of dictionary fields contained
     within physical fields. */

  for (gap = cur->ncol/2; gap > 0; gap /= 2) {
    for (i = gap; i < cur->ncol; ++i) {
      int j;
      for (j = i - gap; j >= 0; j -= gap) {
	struct dict temp;
	temp = d[j];
	if (temp.pos < d[j+gap].pos) break;
	if (temp.pos == d[j+gap].pos && temp.len >= d[j+gap].len) break;
	d[j] = d[j+gap];
	d[j+gap] = temp;
      }
    }
  }
#endif
  /* FIXME: there should be some way to remember the user's field
     order and use it with "SELECT * ...". */

  /* move dictionary fields that match btflds entries to beginning.
     if all fields match, this causes field numbers to match physical fields
     so that Rick's existing queries won't break.  This also helps us
     handle the fact that C-isam keys are not necessarily contiguous. */

  maxpos = 0;
  cur->klen = cur->ncol;
  for (i = 0; i < f->rlen; ++i) {
    short pos = f->f[i].pos;
    short len = f->f[i].len;
    if (i == f->klen)
      cur->klen = maxpos;
    while (len) {
      short j;
      for (j = maxpos; j < cur->ncol; ++j) {
	if (d[j].pos == pos && d[j].len <= len) {
	  pos += d[j].len;
	  len -= d[j].len;
	  if (maxpos != j) {
	    struct dict temp;
	    temp = d[j];
	    d[j] = d[maxpos];
	    d[maxpos] = temp;
	  }
	  ++maxpos;
	  break;
	}
      }
      if (j >= cur->ncol) break;
    }
#if 0	/* FIXME: insert missing key fields */
    if (len) {
    }
#endif
    /* any union fields will be left in the data portion! */
  }

#ifndef NOEBCDIC
  /* For the benefit of the BTAS/1 emulator, mark any character fields
     that overlap binary fields as EBCDIC */

  for (i = 0; i < cur->ncol; ++i) {
    struct dict *dp = &d[i];
    if (dp->type[0] == 'C') {
      int j;
      for (j = 0; j < f->rlen; ++j) {
	const struct btfrec *fp = &f->f[j];
	if (
	  fp->type != BT_CHAR &&
	  fp->pos < dp->pos + dp->len &&
	  fp->pos + fp->len > dp->pos
	) {
	  dp->type[0] = 'E';
	  break;
	}
      }
    }
  }
#endif
  /* Now translate fields to appropriate Column types. */

  cur->col = (Column **)malloc(cur->ncol * sizeof *cur->col);
  for (i = 0; i < cur->ncol; ++i) {
    struct dict *dp = &d[i];
    int len = dp->len;
    char *buf = ubuf + dp->pos;
    Column *c;
    switch (dp->type[0]) {
    case 'C':
      c = Charfld_init(0,buf,len);
      break;
    case 'T':
      if (len == 4) {
	c = Time_init_mask(0,buf,dp->fmt);
	break;
      }
    case 'N':
      c = Number_init_mask(0,buf,len,dp->fmt);
      break;
#ifndef NOEBCDIC
    case 'E':
      c = Ebcdic_init(0,buf,len);
      break;
    case 'P':
      c = Pnum_init(0,buf,len,dp->fmt);
      break;
#endif
    case 'D':
#ifdef USEDWORD	/* turn on if 'D' stands for DWORD */
      c = Number_init(0,buf,len,0);
      break;
#endif
    case 'J':		/* Julian day number */
      if (len == 2)
	c = new1(Date,buf);	/* day of century */
      else if (len == 4)
	c = new1(Julian,buf);	/* Julian date */
      else
	c = new2(Column,buf,len);
      break;
#ifdef MC_TIME
    case 'M':		/* Minute of the 400 years */
      if (len == 5)
	c = new1(Julminp,buf);
      else
	c = new1(Julmin,buf);
      break;
#endif
    default:
      c = Column_init(0,buf,len);
    }
    Column_name(c,dp->name,dp->desc);
    cur->col[i] = c;
  }
}
