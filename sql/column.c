#include <stdio.h>
#include <string.h>
#include <btflds.h>
#include <ftype.h>
#include <ebcdic.h>
#include "config.h"
#include "cursor.h"
#include "coldate.h"
#include "obmem.h"
#include "object.h"

struct Number/*:public Column*/ {
  struct Column_class *_class;
  char *name;
  char *desc;
  char *buf;
  unsigned char len;
  char type;
  short dlen, width;
  short tabidx, colidx;		/* quick version */
/* private: */
  char *fmt;
  short fix;
};

typedef struct Sqlfld/*:public Column*/ {
  struct Column_class *_class;
  char *name;
  char *desc;
  char *buf;
  unsigned char len;
  char type;
  short dlen, width;
  short tabidx, colidx;		/* quick version */
/* private: */
  sql exp;
} Sqlfld;

static int Column_store(Column *,sql,char *);
static sql Column_load(Column *);

static void Number_free(Column *);
static void Number_print(Column *,enum Column_type, char *);
static int Number_store(Column *,sql,char *);
static sql Number_load(Column *);
Column *Number_dup(Column *, char *);

static void Charfld_print(Column *,enum Column_type, char *);
static int Charfld_store(Column *,sql,char *);
static sql Charfld_load(Column *);

static void Sqlfld_print(Column *, enum Column_type, char *);
static int Sqlfld_store(Column *,sql,char *);
static sql Sqlfld_load(Column *);
static void Sqlfld_copy(Column *, char *);
static Column *Sqlfld_dup(Column *, char *);

static void Dblfld_print(Column *, enum Column_type, char *);
static int Dblfld_store(Column *,sql,char *);
static sql Dblfld_load(Column *);

Column *Column_init(Column *col,char *buf,int len) {
  static struct Column_class Column_class = {
    sizeof *col, Column_free, Column_print, Column_store, Column_load,
    Column_copy, Column_dup
  };
  if (col || (col = (Column *)malloc(sizeof *col))) {
    col->_class = &Column_class;
    col->name = 0;
    col->desc = 0;
    col->len  = (short)len;
    col->buf  = buf;
    col->type = BT_BIN;
    col->width = col->dlen = (short)len * 2;
  }
  return col;
}

void Column_free(Column *col) {
  Column_name(col,0,0);
  free((PTR)col);
}

#undef putchar
#define putchar(c) (*buf++ = (c))

void Column_print(Column *col,enum Column_type what, char *buf) {
  int i,len;
  switch (what) {
    const char *p;
  case TITLE:
    len = (col->desc) ? strlen(p = col->desc) : 0;
    if (len == 0 || len > col->width)
      len = (col->name) ? strlen(p = col->name) : 0;
    for (i = 0; i < len; ++i)
      putchar(p[i]);
    break;
  case SCORE:
    len = col->width;
    for (i = 0; i < len; ++i)
      putchar('-');
    break;
  default:
    for (i = 0; i < col->len; ++i) {
      static char tbl[] = "0123456789ABCDEF";
      putchar(tbl[col->buf[i]>>4&15]);
      putchar(tbl[col->buf[i]&15]);
    }
    i *= 2;
  }
  if (what < DATA) while (i++ < col->width)
    putchar(' ');
}

static int Column_store(Column *c,sql x,char *buf) {
  switch (x->op) {
  case EXDBL:
    if (c->len > 6) break;	/* too big */
    stnum(ftoM(x->u.val + 0.5),buf,c->len);
    return 0;
  case EXCONST: case EXDATE:
    if (c->len > 6) break;	/* too big */
    stnum(x->u.num.val,buf,c->len);
    return 0;
  case EXSTRING:
    stchar(x->u.name[0],buf,c->len);
    return 0;
  default: ;
  }
  return -1;	/* can't convert sql to binary columns */
}

static sql Column_load(Column *col) {
  sql x = mksql(EXCONST);
  if (col->len > 6) {
    printf("*** %s: display only ***\n",col->name);
    cancel = 1;
  }
  else {	/* kludge for DATE selection . . . */
    x->u.num.val = ldnum(col->buf,col->len);
    x->u.num.fix = 0;
  }
  return x;
}

void Column_copy(Column *col,char *buf) {
  memcpy(buf,col->buf,col->len);
}

Column *Column_dup(Column *col,char *buf) {
  Column *x;
  x = (Column *)xmalloc(col->_class->size);
  memcpy(x,col,col->_class->size);
  x->buf = buf;
  if (x->name)
    x->name = strdup(col->name);
  if (col->name != col->desc)
    x->desc = strdup(col->desc);
  else
    x->desc = x->name;
  return x;
}

/* BT_NUM */

static struct Column_class Number_class = {
  sizeof (Number), Number_free, Number_print, Number_store, Number_load,
  Column_copy, Number_dup
};

Column *Number_init(Number *num,char *buf,int len,int fix) {
  if (num || (num = (Number *)malloc(sizeof *num))) {
    static char stbl[8] = { 0, 4, 6, 9, 11, 13, 16, 0 };
    Column_init((Column *)num,buf,len);
    num->_class = &Number_class;
    num->dlen = stbl[len & 7] + !!fix;
    num->fix = (short)fix;
    num->fmt = (char *)xmalloc(num->dlen + 1);
    memset(num->fmt,'Z',num->dlen);
    num->fmt[0] = '-';
    num->fmt[num->dlen] = 0;
    num->fmt[num->dlen-1] = '#';
    if (num->fix)
      num->fmt[num->dlen - num->fix - 1] = '.';
    num->width = num->dlen;
    num->type = BT_NUM + (char)fix;
  }
  return (Column *)num;
}

Column *Number_init_mask(Number *num,char *buf,int len,const char *mask) {
  if (num || (num = (Number *)malloc(sizeof *num))) {
    int i, decpnt;
    Column_init((Column *)num,buf,len);
    num->_class = &Number_class;
    num->dlen = (short)strlen(mask);
    num->fix = 0;
    decpnt = 0;
    for (i = 0; i < num->dlen; ++i) {
      switch (mask[i]) {
      case 'Z': case 'z': case '#':
	num->fix += decpnt;
	break;
      case '.':
	decpnt = 1;
      }
    }
    num->fmt = strdup(mask);
    num->width = num->dlen;
    num->type = BT_NUM + (char)num->fix;
    /* fprintf(stderr,"numtype = %02X, mask='%s', fix=%d, decpnt=%d\n", */
	/* num->type,mask,num->fix,decpnt); */
  }
  return (Column *)num;
}

static void Number_free(Column *c) {
  Number *num = (Number *)c;
  free(num->fmt);
  Column_free(c);
}

Column *Number_dup(Column *c,char *buf) {
  Number *num = (Number *)Column_dup(c,buf);
  if (num->fmt)
    num->fmt = strdup(num->fmt);
  return (Column *)num;
}

static void Number_print(Column *c,enum Column_type type, char *buf) {
  Number *num = (Number *)c;
  int i,len;
  MONEY m;
  char fbuf[20], *p;
  switch (type) {
  case TITLE:
    len = num->desc ? strlen(p = num->desc) : 0;
    if (!len || len > num->width)
      len = num->name ? strlen(p = num->name) : 0;
    for (i = 0; i < num->width - len; ++i)
      putchar(' ');
    for (i = 0; i < len; ++i)
      putchar(p[i]);
    break;
  case SCORE:
    Column_print(c,type,buf);
    break;
  default:
    m = ldnum(num->buf,num->len);
    if (cmpM(&m,&nullM)) {
      pic(&m,num->fmt,fbuf);
      len = num->dlen;
    }
    else
      len = 0;
    if (type == DATA) {
      int lead;
      for (i = 0; i < len; ++i)
        if (fbuf[i] != ' ') break;
      lead = i;
      while (i < len)
	putchar(fbuf[i++]);
      if (lead > 0)
        putchar(0);
    }
    else {
      for (i = 0; i < num->width - len; ++i)
	putchar(' ');	/* leading spaces */
      for (i = 0; i < len; ++i)
	putchar(fbuf[i]);
    }
  }
}

#ifndef NOEBCDIC
static void Ebcdic_print(Column *,enum Column_type, char *);
static int Ebcdic_store(Column *,sql,char *);
static sql Ebcdic_load(Column *);
static void Ebcdic_copy(Column *, char *);
static Column *Ebcdic_dup(Column *, char *);

static void Pnum_print(Column *num,enum Column_type type, char *buf) {
  switch (type) {
  case VALUE: case DATA:
    if (*num->buf & 0x80) {
      int i;
      int len = num->len - 1;
      for (i = 0; i < num->width - len; ++i)
	putchar(' ');	/* leading spaces */
      for (i = 1; i <= len; ++i)
	putchar(ebcdic(num->buf[i]));
      return;
    }
    break;
  }
  Number_print(num,type,buf);
}

static struct Column_class Pnum_class = {
  sizeof (Number), Number_free, Pnum_print, Number_store, Number_load,
  Column_copy, Number_dup
};

Column *Pnum_init(Number *num,char *buf,int len,const char *fmt) {
  Column *p = Number_init_mask(num,buf,len,fmt);
  if (p) p->_class = &Pnum_class;
  return p;
}
#endif

/* store number in file format */

static int Number_store(Column *c,sql x,char *buf) {
  Number *num = (Number *)c;
  char tmp[6];
  x = toconst(x,num->fix);
  switch (x->op) {
  case EXNULL:
    memset(buf,0,num->len);
    buf[0] |= signmethod();
    return 0;
  case EXCONST:
    break;
  default:
    return -1;	/* invalid conversion */
  }
#if 1
  /* check for value out of range */
  if (num->len > 6) return -1;
  stnum(x->u.num.val,tmp,num->len);
  if (vcmpM(x->u.num.val,ldnum(tmp,num->len))) return -1;
  memcpy(buf,tmp,num->len);
#else
  stnum(m,buf,num->len);
#endif
  return 0;
}

static sql Number_load(Column *c) {
  Number *col = (Number *)c;
  sql x;
  MONEY num;
  num = ldnum(col->buf,col->len);
  if (!cmpM(&num,&nullM)) return sql_nul;
  x = mksql(EXCONST);
  x->u.num.val = num;
  x->u.num.fix = col->fix;
  return x;
}

/* BT_CHAR */

Column *Charfld_init(Column *cf,char *buf,int len) {
  static struct Column_class Charfld_class = {
    sizeof *cf,Column_free,Charfld_print,Charfld_store,Charfld_load,
    Column_copy, Column_dup
  };
  if ((cf = Column_init(cf,buf,len)) != 0) {
    cf->_class = &Charfld_class;
    cf->width = cf->dlen = (short)len;
    cf->type = BT_CHAR;
  }
  return cf;
}

static void Charfld_print(Column *cf,enum Column_type type,char *buf) {
  int i;
  if (type < VALUE)
    Column_print(cf,type,buf);
  else {
    int len = cf->len;
    if (type == DATA)
      while (len > 0 && cf->buf[len-1] == ' ') --len;
    for (i = 0; i < len; ++i)
      putchar(cf->buf[i]);
    if (type == DATA)
      putchar(0);
    else while (i++ < cf->width)
      putchar(' ');
  }
}

static int Charfld_store(Column *cf,sql x,char *buf) {
  const char *p;
  int i;
  /* FIXME: should we convert num to string? */
  if (!x) return -1;
  switch (x->op) {
  case EXSTRING:
    p = x->u.name[0];
    break;
  case EXNULL:
    p = "";
    break;
  default:
    x = tostring(sql_eval(x,0));
    if (x->op != EXSTRING)
      return -1;
    p = x->u.name[0];
    rmexp(x);
  }
  for (i = 0; i < cf->len && p[i]; ++i)
    buf[i] = p[i];
  while (i < cf->len)
    buf[i++] = ' ';
  return 0;
}

static sql Charfld_load(Column *cf) {
  sql x;
  int len = cf->len;
  while (len && cf->buf[len - 1] == ' ') --len;
  /* if (len == 0) return sql_nul; */
  x = mksql(EXSTRING);
  x->u.name[0] = (char *)obstack_copy0(sqltree,cf->buf,len);
  return x;
}

Column *Column_gen(char *buf,const struct btfrec *f,char *name) {
  Column *col;
  buf += f->pos;
  switch (f->type) {
  case BT_RECNO:
    /* FIXME: needs special Column type since isrecnum not file format.
       This kludge works on Big Endian machines only! */
    col = new3(Number,buf,4,0);
    break;
  case BT_CHAR:
    col = new2(Charfld,buf,f->len);
    break;
  case BT_DATE:
    if (f->len == 2)
      col = new1(Date,buf);	/* day of the century */
    else if (f->len == 4)
      col = new1(Julian,buf);	/* julian day */
    else
      col = new2(Column,buf,f->len);
    break;
  case BT_TIME:
    if (4 <= f->len && f->len <= 8)
      col = new2(Time,buf,f->len);	/* unix time stamp */
    else
      col = new2(Column,buf,f->len);
    break;
  case BT_FLT:
    if (f->len == 8)
      col = new1(Dblfld,buf); /* sortable IEEE floating point */
#if 0
    else if (f->len == 4)
      col = new1(Fltfld,buf);
#endif
    else
      col = new2(Column,buf,f->len);
    break;
#ifdef MC_TIME
  case MC_TIME:
    col = new1(Julmin,buf);
    break;
  case MC_TIMEP:
    col = new1(Julminp,buf);
    break;
#endif
  default:
    if (f->type >= BT_NUM && f->type < BT_NUM + 16)
      col = new3(Number,buf,f->len,f->type - BT_NUM);
#ifdef MC_TIME	/* wild guesses for testing */
    else if (f->len == 5)
      col = new1(Julminp,buf);
#endif
    else
      col = new2(Column,buf,f->len);
  }
  Column_name(col,name,name);
  return col;
}

/* set column name.  We copy the name to dynamic storage. */

void Column_name(Column *col,const char *name,const char *desc) {
  int len;
  /* delete old name & desc */
  if (col->name && *col->name)
    free(col->name);
  if (col->desc != col->name && col->desc && *col->desc)
    free(col->desc);
  col->name = "";
  if (name && *name)
    col->name = strdup(name);	/* short name */
  col->desc = col->name;
  if (desc && *desc && name != desc)
    col->desc = strdup(desc);	/* long name */
  col->width = col->dlen;
  len = name ? strlen(name) : 0;
  if (len > col->width)
    col->width = len;
}

Column *Column_sql(sql x,char *name) {
  Sqlfld *col;
  struct btfrec f;
  static struct Column_class Sqlfld_class = {
    sizeof *col, Column_free, Sqlfld_print, Sqlfld_store, Sqlfld_load,
    Sqlfld_copy, Sqlfld_dup
  };
  if (x->op == EXALIAS)
    return Column_sql(x->u.opd[0],(char *)x->u.name[1]);
  if (x->op == EXCOL) {
    Column *c = do1(x->u.col,dup,x->u.col->buf);
    if (name && *name)
      Column_name(c,name,name);
    return c;
  }
  sql_frec(x,&f);
  col = (Sqlfld *)xmalloc(sizeof *col);
  Column_init((Column *)col,0,f.len);
  col->_class = &Sqlfld_class;
  col->dlen = (short)sql_width(x);
  col->exp = x;
  col->type = f.type;
  Column_name((Column *)col,name,name);
  return (Column *)col;
}

static void Sqlfld_print(Column *c,enum Column_type type,char *buf) {
  Sqlfld *col;
  sql x;
  if (type < VALUE) {
    Column_print(c,type,buf);
    return;
  }
  col = (Sqlfld *)c;
  /* sql_print(col->exp,1); */
  x = tostring(sql_eval(col->exp,MAXTABLE));
  switch (x->op) {
  case EXSTRING:
    if (type == DATA)
      strcpy(buf,x->u.name[0]);
    else
      sprintf(buf,"%-*s",col->width,x->u.name[0]);
    break;
  default:
    sql_print(x,0);	/* let us see what happened for debugging */
  }
  rmexp(x);
}

static sql Sqlfld_load(Column *c) {
  return sql_eval(((Sqlfld *)c)->exp,MAXTABLE);
}

static int Sqlfld_store(Column *col,sql x,char *buf) {
  Sqlfld *this = (Sqlfld *)col;
  this->exp = x;
  if (!buf) return -1;
  switch (x->op) {
  case EXSTRING:
    stchar(x->u.name[0],buf,col->len);
    return 0;
  case EXDBL:
    if (col->len != 8) break;
    stdbl(x->u.val,buf);
    return 0;
  case EXCONST: case EXDATE:
    stnum(x->u.num.val,buf,col->len);
    return 0;
  default: ;
  }
  return -1;
}

static void Sqlfld_copy(Column *col,char *buf) {
  sql x = sql_eval(((Sqlfld *)col)->exp,MAXTABLE);
  Sqlfld_store(col,x,buf);
  /* FIXME: store NULL if store fails */
  rmexp(x);
}

static Column *Sqlfld_dup(Column *c,char *buf) {
  Sqlfld *col = (Sqlfld *)c;
  sql x = col->exp;
  if (col->type == BT_CHAR)
    c = Charfld_init((Column *)0,buf,col->len);
  else
    c = Dblfld_init((Column *)0,buf);
  Column_name(c,col->name,col->desc);
  return c;
}

/* FIXME: should be combined with sql_width */
void sql_frec(sql x,struct btfrec *f) {
  struct btfrec g[2];
  int len;
  switch (x->op) {
  case EXCOL: case EXAGG:
    f->len = x->u.col->len;
    f->type = x->u.col->type;
    return;
  case EXDATE:
    f->len = 4;
    f->type = BT_DATE;
    return;
  case EXCONST:	/* FIXME: could actually look at constant */
    f->len = 6;
    f->type = BT_NUM + (char)x->u.num.fix;
    return;
  case EXDBL:
    f->len = 8;
    f->type = BT_FLT;		/* store as floating point */
    return;
  case EXSTRING:
    len = strlen(x->u.name[0]);
    f->len = (len > 255) ? 255 : (unsigned char)len;
    f->type = BT_CHAR;
    return;
  case EXCAT:
    sql_frec(x->u.opd[0],&g[0]);
    sql_frec(x->u.opd[1],&g[1]);
    len = (short)g[0].len + (short)g[1].len;
    f->len = (len > 255) ? 255 : (unsigned char)len;
    f->type = BT_CHAR;
    return;
  default:
    f->len = 10;
    f->type = BT_BIN;		/* store as binary */
    return;
  }
}

#ifndef NOEBCDIC
Column *Ebcdic_init(Column *col,char *buf,int len) {
  static struct Column_class Ebcdic_class = {
    sizeof *col,Column_free,Ebcdic_print,Ebcdic_store,Ebcdic_load,
    Ebcdic_copy, Ebcdic_dup
  };
  if (col = Charfld_init(col,buf,len)) {
    col->_class = &Ebcdic_class;
  }
  return col;
}

static void Ebcdic_print(Column *col,enum Column_type type,char *buf) {
  int i;
  if (type < VALUE)
    Column_print(col,type,buf);
  else {
    for (i = 0; i < col->len; ++i)
      putchar(ebcdic(col->buf[i]));
    while (i++ < col->width)
      putchar(' ');
  }
}

static int Ebcdic_store(Column *col,sql x,char *buf) {
  const char *p;
  /* FIXME: should we convert num to string? */
  if (!x) return -1;
  switch (x->op) {
  case EXSTRING:
    p = x->u.name[0];
    break;
  case EXNULL:
    p = "";
    break;
  default:
    return -1;
  }
  stebstr(p,(unsigned char *)buf,col->len);
  return 0;
}

static sql Ebcdic_load(Column *col) {
  sql x;
  int len = col->len;
  while (len && col->buf[len - 1] == 0x40) --len;
  /* if (len == 0) return sql_nul; */
  x = mksql(EXSTRING);
  x->u.name[0] = (char *)obstack_alloc(sqltree,len);
  ldebstr((char *)x->u.name[0],(unsigned char *)col->buf,len);
  return x;
}

static void Ebcdic_copy(Column *col,char *buf) {
  int len;
  const char *src = col->buf;
  for (len = col->len; len; --len)
    *buf++ = ebcdic(*src++);
}

static Column *Ebcdic_dup(Column *col,char *buf) {
  Column *x = Ebcdic_init((Column *)0,buf,col->len);
  Column_name(x,col->name,col->desc);
  return x;
}
#endif

Column *Dblfld_init(Column *col,char *buf) {
  static struct Column_class Dblfld_class = {
    sizeof *col,Column_free,Dblfld_print,Dblfld_store,Dblfld_load,
    Column_copy, Column_dup
  };
  if (col = Column_init(col,buf,8)) {
    col->_class = &Dblfld_class;
    col->width = col->dlen = 18;
    col->type = BT_FLT;
  }
  return col;
}

static void Dblfld_print(Column *col,enum Column_type type,char *buf) {
  if (type < VALUE)
    Column_print(col,type,buf);
  else
    sprintf(buf,"%18f",lddbl(col->buf));
}

static char nulnum[8] = { 0x80 };

static int Dblfld_store(Column *c,sql x,char *buf) {
  if (!x) return -1;
  switch (x->op) {
  case EXCONST: case EXDATE:
    stdbl(const2dbl(&x->u.num),buf);
    break;
  case EXDBL:
    stdbl(x->u.val,buf);
    break;
  case EXNULL:
    memcpy(buf,nulnum,8);
    break;
  default:
    return -1;
  }
  return 0;
}

static sql Dblfld_load(Column *col) {
  sql x;
  if (memcmp(col->buf,nulnum,8) == 0)
    return sql_nul;
  x = mksql(EXDBL);
  x->u.val = lddbl(col->buf);
  return x;
}

void printColumn(Column *col,enum Column_type type) {
  char *buf = (char *)alloca(col->width+1);
  /* printf("w = %2d ",col->width); */
  buf[col->width] = 0;
  do2(col,print,type,buf);
  fputs(buf,stdout);
}
