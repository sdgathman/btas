/*	operations using a workfile
	Copyright 1993 Business Management Systems, Inc.
	Author: Stuart D. Gathman
$Log$
Revision 1.1  2001/02/28 23:00:04  stuart
Old C version of sql recovered as best we can.

 * Revision 1.4.1.1  1993/05/21  20:45:43  stuart
 * give Order objects their own obstack point their sql results to it
 *
 * Revision 1.4  1993/02/24  15:47:05  stuart
 * make 'dup' work with the special Sum sub-classing
 *
 * Revision 1.3  1993/02/19  04:27:35  stuart
 * Sum_init not handling enough types - should call dup when MIN/MAX
 *
 * Revision 1.2  1993/02/17  01:00:57  stuart
 * note when o == s and avoid screwing things up
 *
*/

#include <stdio.h>
#include <assert.h>
#include <string.h>
#include "object.h"
#include "cursor.h"
#include "coldate.h"
#include <ftype.h>
#include <btflds.h>
#include <bterr.h>

void inc(char *,int);
int blkcmpr(const char *,const char *,int);

typedef struct Order {
  struct Cursor_class *_class;
/* public: */
  short ncol;	/* number of columns */
  short klen;	/* number of key columns */
  Column **col;	/* columns pointing to sorted relation */
  long rows;	/* estimated number of rows */
  long cnt;	/* actual returned so far */
/* private: */
  struct obstack h;
  long desc;	/* which key fields are descending */
  Cursor *rel;	/* unsorted relation */
  Column **rcol;/* columns pointing to source relation */
  BTCB *w;	/* work file */
  char name[32];/* name of work file */
  char *buf;	/* work record */
  struct btflds *f;	/* field table for work record */
  short common;	/* key columns in common with source Cursor */
  short comlen;	/* uncompressed bytes in common with source Cursor */
  short keylen;	/* uncompressed unique key length */
  char eof;	/* EOF on source relation */
  char agg;
} Order;

static void Order_free(Cursor *);
static int Order_first(Cursor *);
static int Order_next(Cursor *);
static int Order_find(Cursor *);
static void sum_frec(sql x,struct btfrec *f);
static Column *Sum_init(sql x,char *buf,const struct btfrec *,char *name);

/* Determine whether two sql expressions have the same value.  If we can't
   tell, say no. */

/* FIXME: this should probably go in sql.c */
int sql_same(sql x,sql y) {
  if (x == y) return 1;
  if (!x || !y) return 0;
  if (x->op == EXDESC) x = x->u.opd[0];
  if (y->op == EXDESC) y = x->u.opd[0];
  if (x->op == y->op) {
    const char *p;
    int i;
    if (x->op == EXCOL)
      return (x->u.col == y->u.col);
    if (x->op == EXAGG && x->u.g.op != y->u.g.op)
      return 0;
    p = sql_form[(int)x->op].fmt;
    for (i = 0; p[i]; ++i) {
      switch (p[i]) {
      case 'x':
	if (!sql_same(x->u.opd[i],y->u.opd[i])) return 0;
	break;
      case 'f':
	if (x->u.f.func != y->u.f.func) return 0;
	break;
      case 'i':
	if (x->u.ival != y->u.ival) return 0;
	break;
      case 's':
	if (strcmp(x->u.name[i],y->u.name[i])) return 0;
	break;
      case 'd':
	if (x->u.val != y->u.val) return 0;
	break;
      case 'n':
	if (cmpM(&x->u.num.val,&y->u.num.val)) return 0;
	break;
      }
    }
    return 1;
  }
  /* FIXME: should we handle Sqlfld Columns? */
  return 0;
}

/* allocate a grouped/ordered field.
   update *e to reference the new field,
   add references to original table to r[rlen++],
   return rlen */

static int allocFld(sql *e,sql *r,int rlen) {
  sql x = *e;
  int i;
  if (!x) return rlen;
  /* move any aggregates to the top level of r[] */
  if (x->op != EXAGG && x->op != EXCOL) {
    const char *p = sql_form[(int)x->op].fmt;
    for (i = 0; p[i]; ++i) if (p[i] == 'x')
      rlen = allocFld(&x->u.opd[i],r,rlen);
    return rlen;
  }
  for (i = 0; i < rlen; ++i)
    if (sql_same(x,r[i])) break;
  if (i == rlen)
    r[rlen++] = x;
  /* make a temporary reference to our new table -
     we replace all EXCOL nodes above, so EXCOL makes a suitable marker
     for updateFld/undoFld to find them later */
  x = mksql(EXCOLIDX);
  x->u.ival = i;	/* save column index (Admittedly Hokey) */
  *e = x;	/* modify select list to point to new columns */
  return rlen;
}

#if 0	/* needed if noop test improved below */
/* undo field allocation done by the above */
static void undoFld(sql *e,sql *r) {
  sql x = *e;
  if (x->op != EXCOL) {
    int i;
    const char *p = sql_form[(int)x->op].fmt;
    for (i = 0; p[i]; ++i) if (p[i] == 'x' && x->u.opd[i])
      undoFld(&x->u.opd[i],r);
  }
  else {
    *e = r[x->u.ival];
    rmsql(x);
  }
}
#endif

/* update all temporary references to our new table
   to point to the actual column */
static void updateFld(sql x,Column **c) {
  const char *p;
  int i;
  if (x->op == EXCOLIDX) {
    x->op = EXCOL;
    x->u.col = c[x->u.ival];
    return;
  }
  p = sql_form[(int)x->op].fmt;
  for (i = 0; p[i]; ++i) if (p[i] == 'x' && x->u.opd[i])
    updateFld(x->u.opd[i],c);
}

/* distinct = 0, add sequence if necessary to keep unique
   distinct = 1, ignore duplicates
   distinct = 2, group */

Cursor *Order_init(Cursor *c,sql o,sql s,int distinct,int idx) {
  Order *this;
  int klen,rlen,common;
  int i,pos;
  long desc = 0;
  sql savstk;
  sql e,r[MAXFLDS];
  static struct Cursor_class Order_class = {
    sizeof *this, Order_free, Order_first, Order_next, Order_find, 
    Cursor_print, Cursor_fail, Cursor_fail, Cursor_fail, Cursor_optim
  };

  /* FIXME: need to check for rlen > MAXFLDS or klen > 32 */

  /* get user specified key fields */
  common = 0;
  klen = 0;
  if (o) for (e = o; e = e->u.opd[1];) {
    sql x = e->u.opd[0];
    if (x->op == EXDESC)
      x = x->u.opd[0];
    for (i = 0; i < klen; ++i)
      if (sql_same(x,r[i])) break;
    if (i == klen) {
      if (klen <= common && klen < c->klen
	&& x->op == EXCOL && x->u.col == c->col[klen])
	  ++common;
      if (e->u.opd[0]->op == EXDESC)
	desc |= 1L << klen;
      r[klen++] = x;
    }
  }

  if (!distinct) {
    if (common == klen) {
      /* Even if distinct, no sort is required if all selected fields are 
	 source key fields.  We don't detect this at present. */
      /* FIXME: check this after SELECT fields are processed.
		treat SELECT fields as key fields if DISTINCT.
		Need to cleanup somehow if sort not required. . . */
      return c;	/* no sort required */
    }

    if (common < c->klen) {
      r[klen++] = 0;	/* add sequence no. to preserve dups */
    }
  }

  this = xmalloc(sizeof *this);
  this->_class = &Order_class;
  this->rows = c->rows;
  this->cnt  = 0;
  this->rel  = c;
  this->agg  = (distinct == 2);
  this->common = common;
  this->desc = desc;
  obstack_init(&this->h);
  savstk = free_sql(&this->h);

  /* now include SELECT fields not already in the key */
  rlen = klen;
  if (o != s) {
    if (s) for (e = s; e = e->u.opd[1];)
      rlen = allocFld(&e->u.opd[0],r,rlen);
    else for (i = 0; i < c->ncol; ++i) {
      sql x = mksql(EXCOL);
      int j;
      x->u.col = c->col[i];
      for (j = 0; j < rlen; ++j)
	if (sql_same(x,r[j])) break;
      if (j == rlen)
	r[rlen++] = x;
      else
	rmsql(x);
    }
  }

  if (common < c->klen && distinct == 1)
    klen = rlen;	/* make all columns key to eliminate duplicates */

  /* feedback key to source relation for optimization - recompute common */

  this->klen = klen;
  this->ncol = rlen;
  this->col  = obstack_alloc(&this->h,rlen * sizeof *this->col);
  this->rcol = obstack_alloc(&this->h,rlen * sizeof *this->rcol);

  /* compute field table */
  {
    struct btfrec *p;
    this->f  = obstack_alloc(&this->h,
      sizeof *this->f - sizeof this->f->f + sizeof *this->f->f * (rlen + 1)
    );
    this->f->klen = klen;
    this->f->rlen = rlen;
    pos = 0;
    for (p = this->f->f,i = 0; i < rlen; ++i,++p) {
      sql x = r[i];
      if (debug) {
	printf("Order#%d:",i);
	sql_print(x,1);
      }
      if (x) {
	if (x->op == EXAGG) {
	  this->rcol[i] = Column_sql(x->u.g.exp,"");
	  sum_frec(x,p);
	}
	else {
	  this->rcol[i] = Column_sql(x,"");
	  p->type = this->rcol[i]->type;
	  p->len = this->rcol[i]->len;
	}
      }
      else {
	this->rcol[i] = 0;
	p->type = BT_NUM;
	p->len = 4;
      }
      p->pos = pos;
      pos += p->len;
    }
    p->type = 0;
    p->len  = 0;
    p->pos  = pos;
  }

  this->comlen = this->f->f[this->common].pos;
  this->keylen = this->f->f[this->klen].pos;
  this->buf = obstack_alloc(&this->h,btrlen(this->f));
  for (i = 0; i < rlen; ++i) {
    struct btfrec *p = this->f->f + i;
    sql x = r[i];
    if (x) {
      if (x->op == EXAGG)
	this->col[i] = Sum_init(x,this->buf,p,this->rcol[i]->name);
      else
	this->col[i] = do1(this->rcol[i],dup,this->buf + p->pos);
    }
    else
      this->col[i] = Column_gen(this->buf,p,"ROWNO");
    this->col[i]->tabidx = idx;
    this->col[i]->colidx = i;
  }

  /* Finally, plug real Column pointers into modified SELECT list */
  if (s && o != s) for (e = s; e = e->u.opd[1];)
    updateFld(e->u.opd[0],this->col);

  /* And, open temp file to hold sorted/grouped records,
     but only if a workfile is needed */
  if (this->klen == 0)
    this->w = 0;
  else {
    this->w = btopentmp("OrderBy",this->f,this->name);
    /* printf("Creating workfile '%s'\n",this->name); */
  }
  this->eof = 0;
  pop_sql(savstk);
  return (Cursor *)this;
}

static void Order_free(Cursor *c) {
  Order *this = (Order *)c;
  int i;
  if (this->w) {
    if (debug)
      btclose(this->w);	/* leave workfile for inspection */
    else
      btclosetmp(this->w,this->name);
  }
  for (i = 0; i < this->ncol; ++i) {
    do0(this->col[i],free);
    if (this->rcol[i]) do0(this->rcol[i],free);
  }
  do0(this->rel,free);
  obstack_free(&this->h,0);
  free((PTR)this);
}

/* read next group of records with a common key from source.
   return partial key needed to go back to begin of group after sorting. */

static int Order_read(Order *this) {
  int rlen = btrlen(this->f);
  int klen = 0;
  int common = 0;
  long cnt = 0;
  do {
    int i, rc = BTERKEY;
    long desc = this->desc;
    /* Copy source record to work buffer & check common fields */
    for (i = 0; i < this->ncol; ++i,desc >>= 1) {
      Column *x = this->col[i], *y = this->rcol[i];
      char buf[256], *p;
      if (y == 0) {
	stlong(this->cnt + cnt,x->buf);
	continue;
      }
      if (this->agg && i >= this->klen) {
	sql d;
	if (i == this->klen) {
	  if (i) {
	    u2brec(this->f->f,this->buf,this->keylen,this->w,this->keylen);
	    this->w->rlen = rlen;
	    rc = btas(this->w,BTREADEQ + NOKEY);
	    if (rc == 0)
	      b2urec(this->f->f,this->buf,rlen,this->w->lbuf,this->w->rlen);
	  }
	  else if (cnt)
	    rc = 0;
	  else
	    cnt = 1;
	  if (rc) {	/* initialize aggregates */
	    int j;
	    for (j = i; j < this->ncol; ++j)
	      do2(this->col[j],store,sql_nul,this->col[j]->buf);
	  }
	}
	d = do0(y,load);
	if (d->op != EXNULL)
	  do2(x,store,d,p = buf); /* accumulate aggregates */
      }
      else if (y->_class == x->_class)
	p = y->buf;
      else
	do1(y,copy,p = buf);
      if (desc & 1) {
	int j;
	for (j = 0; j < x->len; ++j)
	  buf[j] = ~p[j];
	p = buf;
      }
      if (i < common) {
	if (memcmp(x->buf,p,x->len))
	  goto endgroup;
      }
      else
	memcpy(x->buf,p,x->len);
    }
    if (this->klen) {
      u2brec(this->f->f,this->buf,rlen,this->w,this->keylen);
      klen = this->w->klen;
      if (rc) {
	rc = btas(this->w,BTWRITE+DUPKEY); /* ignore dups to make DISTINCT */
	if (rc == 0)
	  ++cnt;
	else if (klen > MAXKEY) {
	  /* %% save potentially lost records */
	  puts("*** some records may be missing ***");	/* cop out for now */
	}
      }
      else
	btas(this->w,BTREPLACE);	/* update aggregates */
    }
    common = this->common;
  } while (do0(this->rel,next) == 0);
  this->eof = 1;
endgroup:
  if (cnt > 1) {
    /* compute compressed common key length if multiple records,
       is there a better way to do this? */
    u2brec(this->f->f,this->buf,rlen,this->w,this->comlen);
    klen = this->w->klen;
  }
  return klen;		/* return compressed common key length */
}

int Order_first(Cursor *c) {
  Order *this = (Order *)c;
  int rlen = btrlen(this->f);
  if (this->cnt == 0) {	/* if no work file yet */
    if (do0(this->rel,first)) return -1;
    Order_read(this);
  }
  /* go back to first record of work file */
  if (this->w) {
    this->w->klen = 0;
    this->w->rlen = rlen;
    if (btas(this->w,BTREADGE+NOKEY)) return -1;
    b2urec(this->f->f,this->buf,rlen,this->w->lbuf,this->w->rlen);
  }
  if (this->desc) {
    int i;
    long desc;
    for (i = 0,desc = this->desc; i < this->klen; ++i,desc >>= 1) {
      if (desc & 1) {
	Column *x = this->col[i];
	int j;
	for (j = 0; j < x->len; ++j)
	  x->buf[j] ^= ~0;
      }
    }
  }
  this->cnt = 1;
  return 0;
}

int Order_next(Cursor *c) {
  Order *this = (Order *)c;
  int rlen;
  if (this->klen == 0) return -1;
  rlen = btrlen(this->f);
  this->w->klen = this->w->rlen;
  this->w->rlen = rlen;
  if (btas(this->w,BTREADGT+NOKEY)) {
    if (this->eof) return -1;
    /* get more records from source */
    this->w->klen = Order_read(this);
    this->w->rlen = rlen;
    /* go back to start of common key */
    if (btas(this->w,BTREADGE+NOKEY)) return -1;
  }
  b2urec(this->f->f,this->buf,rlen,this->w->lbuf,this->w->rlen);
  if (this->desc) {
    int i;
    long desc;
    for (i = 0,desc = this->desc; i < this->klen; ++i,desc >>= 1) {
      if (desc & 1) {
	Column *x = this->col[i];
	int j;
	for (j = 0; j < x->len; ++j)
	  x->buf[j] ^= ~0;
      }
    }
  }
  ++this->cnt;
  return 0;
}

int Order_find(Cursor *c) {
  Order *this = (Order *)c;
  char savkey[MAXKEY];
  int klen;
  u2brec(this->f->f,this->buf,btrlen(this->f),this->w,this->keylen);
  /* Save compressed key and call Order_read() until we pass it. */
  memcpy(savkey,this->w->lbuf,klen = this->w->klen);
  if (this->cnt == 0 && do0(this->rel,first)) return -1;
  while (!this->eof) {
    int rc;
    Order_read(this);
    rc = blkcmpr(savkey,this->w->lbuf,klen);
    if (rc <= 0) break;
  }
  /* Try to find in workfile */
  memcpy(this->w->lbuf,savkey,this->w->klen = klen);
  this->w->rlen = btrlen(this->f);
  if (debug) printf("Order_find('%s')\n",this->w->lbuf);
  if (btas(this->w,BTREADGE+NOKEY) == 0) {
    b2urec(
      this->f->f,this->buf,btrlen(this->f),this->w->lbuf,this->w->rlen);
    return 0;
  }
  return -1;
}

static void sum_frec(sql x,struct btfrec *f) {
  switch (x->u.g.op) {
  case AGG_CNT:
    f->type = BT_NUM;
    f->len = 4;
    break;
  case AGG_SUM:
    sql_frec(x->u.g.exp,f);
    if ((f->type & ~15) == BT_NUM)
      f->len = 6;
    else if (f->type != BT_FLT) {
      f->type = BT_FLT;
      f->len = 8;
    }
    break;
  case AGG_PROD:
    f->type = BT_FLT;
    f->len = 8;
    break;
  case AGG_MAX: case AGG_MIN:
    sql_frec(x->u.g.exp,f);
    break;
  }
}

/* extensions to Standard column objects for aggregate types */

typedef struct Sum_class {
  struct Column_class _class;
  struct Column_class *super;
  char first;
  char op;	/* e.g. EXADD/EXMULT/EXLT/EXGT */
} Sum_class;

static void Sum_free(Column *c) {
  Sum_class *class = (Sum_class *)c->_class;
  c->_class = class->super;
  free(class);
  do0(c,free);
}

static Column *Sum_dup(Column *c,char *buf) {
  Sum_class *class = (Sum_class *)c->_class;
  Column *r;
  c->_class = class->super;
  r = do1(c,dup,buf);	/* dup might copy _class ptr :-( */
  c->_class = (struct Column_class *)class;
  return r;
}

/* NOTE, aggregate store function initialize their data when
   storing a NULL */

static int Sum_store(Column *this, sql x, char *buf) {
  Sum_class *class = (Sum_class *)this->_class;
  if (x->op != EXNULL) {
    if (class->first)
      class->first = 0;
    else
      x = mkbinop(x,class->op,do0(this,load));
  }
  else
    class->first = 1;
  return (*class->super->store)(this,x,buf);
}

static int Cnt_store(Column *c, sql x, char *buf) {
  if (x->op != EXNULL) {
    memcpy(buf,c->buf,4);
    inc(buf,4);
  }
  else
    memset(buf,0,4);
  return 0;
}

static int MinMax_store(Column *this, sql x, char *buf) {
  char tmp[256];
  Sum_class *class = (Sum_class *)this->_class;
  if (x->op != EXNULL) {
    if (class->first)
      class->first = 0;
    else {
      sql y = mkbinop(do0(this,load),class->op,sql_eval(x,0));
      if (y->op == EXINT && y->u.ival) {
	memcpy(buf,this->buf,this->len);
	rmsql(y);
	return 0;
      }
      rmsql(y);
    }
  }
  else
    class->first = 1;
  return (*class->super->store)(this,x,buf);
}

static Column *Sum_init(sql x,char *buf,const struct btfrec *f,char *name) {
  Column *this;
  Sum_class *class;
  this = Column_gen(buf,f,name);
  /* the tricky part */
  class = xmalloc(sizeof *class);
  class->_class = *this->_class;
  class->super = this->_class;
  this->_class = (struct Column_class *)class;
  switch (x->u.g.op) {
  case AGG_SUM: class->_class.store = Sum_store;
		class->op = EXADD; break;

  case AGG_PROD:class->_class.store = Sum_store;
		class->op = EXMUL; break;

  case AGG_CNT: class->_class.store = Cnt_store; break;

  case AGG_MAX: class->_class.store = MinMax_store;
		class->op = EXGT; break;

  case AGG_MIN: class->_class.store = MinMax_store;
		class->op = EXLT; break;
  }
  class->_class.free = Sum_free;
  class->_class.dup = Sum_dup;
  return this;
}

/* does expression contain aggregate references? */
static int isAgg(sql x) {
  if (!x) return 0;
  if (x->op != EXAGG) {
    int i;
    const char *p = sql_form[(int)x->op].fmt;
    for (i = 0; p[i]; ++i) if (p[i] == 'x')
      if (isAgg(x->u.opd[i])) return 1;
    return 0;
  }
  return 1;
}

Cursor *Group_init(Cursor *c,sql g,sql s,sql h,int idx) {
  if (!h && !g && !isAgg(s))
    return c;		/* no summarizing required */
  if (h)		/* add having to select so that Order_init will */
    addlist(s,h);	/* point EXAGG & EXCOL references to the work file */
  c = Order_init(c,g,s,2,idx);
  if (h) {
    (void)rmlast(s);
    c = Where_init(c,h,idx,0);
  }
  return c;
}
