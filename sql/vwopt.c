/*
	NOTE - a null sql expression is considered always true
	       a null vwsel selects no records (i.e. a false sql expression)

	       Free chains for sql, Vwsel, etc. are for speed only.
	       The obstack "mark/release" (push/pop) feature is used to make
	       sure storage is reclaimed.

	NOTE - memcmp() is unreliable, it doesn't work for example on
		the motorolas.
*/

#include <stdio.h>
#include <string.h>
#include <btflds.h>
#include <block.h>
#include "obmem.h"
#include "vwopt.h"
#include "column.h"
#include "object.h"

static struct vwsel_ctx {
  Column **col;	/* key columns */
  short kcol;	/* number of key columns */
  short klen;	/* total key length in bytes */
  short lev;
  Vwsel *free;	/* minimize memory use per level */
  struct vwsel_ctx *prev;
  struct obstack *h;
  sql savstk;
} *sp;

static Vwsel *Vwsel_or_set_1(Vwsel *a, const Vwsel *q);
static Vwsel *Vwsel_and_1(const Vwsel *a, const Vwsel *q);
static Vwsel *Vwsel_copy_1(const Vwsel *);
static int colid(sql x);

void Vwsel_free_ctx(Vwsel_ctx *p) {
  if (p) {
    struct obstack *h = p->h;
    obstack_free(h,0);
    free(h);
  }
}

Vwsel_ctx *Vwsel_push(Vwsel_ctx *cp,Column **col,int klen,int idx) {
  struct vwsel_ctx *p;
  struct obstack *h;
  int i;
  if (!cp) {
    h = xmalloc(sizeof *h);
    obstack_init(h);
  }
  else {
    h = cp->h;
  }
  while (cp && cp->lev >= (short)idx) {
    p = cp->prev;
    pop_sql(cp->savstk);
    obstack_free(h,cp);	/* clear higher levels */
    cp = p;
  }
    
  p = obstack_alloc(h,sizeof *p);
  p->savstk = free_sql(h);	/* don't get our sql get mixed up elsewhere */
  p->prev = cp;
  p->h = h;
  p->col = col;
  p->kcol = (short)klen;
  for (p->klen = 0,i = 0; i < p->kcol; p->klen += p->col[i++]->len);
  p->free = 0;
  p->lev = (short)idx;
  return sp = p;
}

Vwsel *Vwsel_new() {
  Vwsel *x = sp->free;
  if (x)
    sp->free = x->next;
  else {
    x = obstack_alloc(sp->h,sizeof *x);
    x->min = obstack_alloc(sp->h,sp->klen);
    x->max = obstack_alloc(sp->h,sp->klen);
  }
  x->next = 0;
  x->exp = 0;
  return x;
}

void Vwsel_free(Vwsel *x) {
  Vwsel *p;
  while (p = x) {
    x = p->next;
    p->next = sp->free;
    sp->free = p;
    /* rmexp(p->exp); /* may be shared with other vwsel's */
  }
}

#define Vwsel_free_1(x)	(x->next = 0,Vwsel_free(x))

Vwsel *Vwsel_copy_1(const Vwsel *a) {
  Vwsel *x;
  if (a == 0) return 0;
  x = Vwsel_new();
  memcpy(x->min,a->min,sp->klen);
  memcpy(x->max,a->max,sp->klen);
  x->exp = a->exp;
  return x;
}

Vwsel *Vwsel_copy(const Vwsel *a) {
  Vwsel *p = 0, **last = &p;
  while (a) {
    last = &(*last = Vwsel_copy_1(a))->next;
    a = a->next;
  }
  *last = 0;
  return p;
}

#define iscolumn(x)	(	\
	x->op == EXCOL && x->u.col->tabidx == sp->lev	\
	&& x->u.col->colidx < sp->kcol )
#define isconstant(x)	(x->op == EXSTRING || x->op == EXCONST	\
			|| x->op == EXDBL			\
			|| x->op == EXCOL && x->u.col->tabidx < sp->lev)

/* test whether an expression is a basic query and compute column id */

static int colid(sql x) {
  switch (x->op) {
  case EXGT: case EXGE: case EXEQ: case EXNE: case EXLT: case EXLE:
    /* both columns should not be constant after evaluation */
    if (iscolumn(x->u.opd[0]) && isconstant(x->u.opd[1]))
      return x->u.opd[0]->u.col->colidx;
    if (iscolumn(x->u.opd[1]) && isconstant(x->u.opd[0])) {
      sql t;
      t = x->u.opd[0];
      x->u.opd[0] = x->u.opd[1];
      x->u.opd[1] = t;
      switch (x->op) {
      case EXGT: x->op = EXLT; break;
      case EXGE: x->op = EXLE; break;
      case EXLT: x->op = EXGT; break;
      case EXLE: x->op = EXGE; break;
      }
      return x->u.opd[0]->u.col->colidx;
    }
  case EXLIKE:
    if (iscolumn(x->u.opd[0]) && isconstant(x->u.opd[1]))
      return x->u.opd[0]->u.col->colidx;
    /* LIKE not symmetrical - pattern is always on the right */
  }
  return -1;
}

/* transform an sql expression into a vwsel list.  Any basic queries
   we recognize are turned into a vwsel box.  Any expressions we don't
   recognize become the expression that goes with a box.  We know how
   to combine boxes along with their expressions using AND and OR logic.
*/

Vwsel *Vwsel_init(sql x) {
  Vwsel *op0, *op1;
  int i;
  switch (x->op) {
  case EXAND:
    op0 = Vwsel_init(x->u.opd[0]);
    op1 = Vwsel_init(x->u.opd[1]);
    op0 = Vwsel_and_set(op0,op1);
    Vwsel_free(op1);
    return op0;
  case EXOR:
    op0 = Vwsel_init(x->u.opd[0]);
    op1 = Vwsel_init(x->u.opd[1]);
    op0 = Vwsel_or_set(op0,op1);
    Vwsel_free(op1);
    return op0;
  }
  x = sql_eval(x,sp->lev);	/* copy and reduce constants */
  if (x && x->op == EXINT && !x->u.ival) {	/* the constant FALSE */
    rmsql(x);
    return 0;
  }
  op0 = Vwsel_new();
  memset(op0->min,0,sp->klen);
  memset(op0->max,~0,sp->klen);
  if (x) {
    if ((i = colid(x)) >= 0) {
      int j;
      int pos = 0;
      int len = sp->col[i]->len;
      for (j = 0, pos = 0; j < i; pos += sp->col[j++]->len);
      op0->exp = 0;
      switch (x->op) {
      /* FIXME: need better action when store fails . . . */
      case EXGT:
	if (do2(sp->col[i],store,x->u.opd[1],op0->min+pos)) break;
	rmexp(x);
	if (blkcmpr(op0->min+pos,op0->max+pos,len) >= 0) {
	  Vwsel_free(op0);
	  return 0;
	}
	inc(op0->min+pos,len);
	return op0;
      case EXGE:
	if (do2(sp->col[i],store,x->u.opd[1],op0->min+pos)) break;
	rmexp(x);
	return op0;
      case EXLT:
	if (do2(sp->col[i],store,x->u.opd[1],op0->max+pos)) break;
	rmexp(x);
	if (blkcmpr(op0->max+pos,op0->min+pos,len) <= 0) {
	  Vwsel_free(op0);
	  return 0;
	}
	dec(op0->max+pos,len);
	return op0;
      case EXLE:
	if (do2(sp->col[i],store,x->u.opd[1],op0->max+pos)) break;
	rmexp(x);
	return op0;
      case EXEQ:
	if (do2(sp->col[i],store,x->u.opd[1],op0->min+pos)) break;
	if (do2(sp->col[i],store,x->u.opd[1],op0->max+pos)) break;
	rmexp(x);
	return op0;
      case EXNE:
	if (do2(sp->col[i],store,x->u.opd[1],op0->min+pos)) break;
	if (blkcmpr(op0->min+pos,op0->max+pos,len) < 0)
	  inc(op0->min+pos,len);
	else {
	  Vwsel_free(op0);
	  op0 = 0;
	}
	op1 = Vwsel_new();
	memset(op1->min,0,sp->klen);
	memset(op1->max,~0,sp->klen);
	if (do2(sp->col[i],store,x->u.opd[1],op1->max+pos)) break;
	rmexp(x);
	if (blkcmpr(op1->min+pos,op1->max+pos,len) >= 0) {
	  Vwsel_free(op1);
	  return op0;
	}
	dec(op1->max+pos,len);
	op0 = Vwsel_or_set(op0,op1);
	Vwsel_free(op1);
	return op0;
      case EXLIKE:	/* make range but keep expression */
	op0->exp = x;
	if (do2(sp->col[i],store,x->u.opd[1],op0->min+pos)) break;
	memcpy(op0->max+pos,op0->min+pos,len);
	for (i = 0; i < len && !strchr("%_[",op0->min[pos+i]); ++i);
	while (i < len) {
	  op0->min[pos+i] = 0;
	  op0->max[pos+i] = ~0;
	  ++i;
	}
	return op0;
      }
    }
    if (x->op == EXINT && x->u.ival) {	/* the constant TRUE */
      rmsql(x);
      x = 0;
    }
  }
  op0->exp = x;
  return op0;
}

/* set dest BYTE array to src array minus 1 */

void dec(d,len)
  char *d;
{
  while (len-- && !d[len]--);
}

/* set dest BYTE array to src array plus 1 */

void inc(d,len)
  char *d;
{
  while (len-- && !++d[len]);
}

/* your basic distributive law at work . . . */

static Vwsel *Vwsel_and_1(a,b)
  const Vwsel *a, *b;
{
  Vwsel *x, *p, **prev;
  int i;
	/* trivial cases */
  if (b == 0 || a == 0)	/* if no records */
    return 0;	/* then no records in result */

  x = Vwsel_copy(a);
  prev = &x;
  while (p = *prev) {
    int pos, len;
    for (pos = 0,i = 0; i < sp->kcol; pos += len,++i) {
      len = sp->col[i]->len;
      if (blkcmpr(p->min + pos,b->min + pos,len) < 0)
	memcpy(p->min + pos,b->min + pos,len);
      if (blkcmpr(p->max + pos,b->max + pos,len) > 0)
	memcpy(p->max + pos,b->max + pos,len);
      if (blkcmpr(p->max + pos,p->min + pos,len) < 0) {
	*prev = p->next;	/* delete null box */
	Vwsel_free_1(p);
	goto next;
      }
    }
    if (!p->exp)
      p->exp = b->exp;
    else if (b->exp)
      p->exp = mkbinop(p->exp,EXAND,b->exp);
    prev = &p->next;
  next:;
  }
  return x;
}

/* We insert a vwsel entry into a.  The list
   is kept sorted and the boxes disjoint in such a way that they
   traverse the selected records once and only once in the
   most efficient manner for vwsel.  Each entry can be visualized as a box.

   We accomplish all this very simply by splitting each dimension recursively
   so that we need only merge identical boxes.  When a dimension overlaps,
   we split three ways.

	     q-----q			a-----a
	     |	   |			|     |
	     |     |			a-----a
	p----|--p  |		   p----q--p--q
	|    |	|  |	===>       |	|  |  |
	p----|--p  |		   p----q--p--q
	     |     |			b-----b
	     |	   |			|     |
	     q-----q			b-----b

   The top and bottom pieces are up to spec, but the two middle pieces may still
   overlap.  What to do?  We just call ourselves recursively to insert
   the middle pieces.  They are guarranteed not to overlap on the dimension
   we just split, and the recursive call will fix any overlap on succeding
   dimensions.  If all dimensions are equal, the boxes are identical and
   we simply OR the expressions.
*/

static Vwsel *Vwsel_or_set_1(a,q)
  Vwsel *a;
  const Vwsel *q;
{
  Vwsel **last = &a, *p;
  int i;

	/* trivial cases */
  if (q == 0)		/* if no records in q */
    return a;		/* then a is unchanged */
  if (a == 0) {		/* if no records in a */
    a = Vwsel_copy_1(q);
    a->next = 0;
    return a;		/* result is just q */
  }

  /* find first partial overlap along any dimension */
  while (p = *last) {
    int pos, len, res;
    for (pos = 0,i = 0; i < sp->kcol; pos += len,++i) {
      int rmin, rmax;
      len = sp->col[i]->len;
      res = blkcmpr(p->max + pos, q->min + pos,len);
      if (res < 0) break;
      res = blkcmpr(q->max + pos, p->min + pos,len);
      if (res < 0) goto append;
      rmin = blkcmpr(p->min + pos, q->min + pos,len);
      rmax = blkcmpr(p->max + pos, q->max + pos,len);
      if (rmin || rmax) {	/* must split */
	Vwsel *s, *t;
	*last = p->next;	/* delete p from target */
	t = Vwsel_copy_1(q);	/* don't modify source */
	if (rmin) {	/* split off top part */
	  if (rmin < 0) {		/* p is first */
	    s = Vwsel_copy_1(p);
	    memcpy(s->max + pos, t->min + pos, len);
	    dec(s->max + pos, len);
	    memcpy(p->min + pos, t->min + pos, len);
	  }
	  else {			/* q is first */
	    s = Vwsel_copy_1(t);
	    memcpy(s->max + pos, p->min + pos, len);
	    dec(s->max + pos, len);
	    memcpy(t->min + pos, p->min + pos, len);
	  }
	  *last = Vwsel_or_set_1(*last,s);
	  Vwsel_free_1(s);
	}
	if (rmax) {	/* split off bottom part */
	  if (rmax < 0) {
	    s = Vwsel_copy_1(t);
	    memcpy(s->min + pos, p->max + pos, len);
	    inc(s->min + pos,len);
	    memcpy(t->max + pos, p->max + pos, len);
	  }
	  else if (rmax > 0) {
	    s = Vwsel_copy_1(p);
	    memcpy(s->min + pos, t->max + pos, len);
	    inc(s->min + pos, len);
	    memcpy(p->max + pos, t->max + pos, len);
	  }
	  *last = Vwsel_or_set_1(*last,s);
	  Vwsel_free_1(s);
	}
	*last = Vwsel_or_set_1(*last,p);
	Vwsel_free_1(p);
	*last = Vwsel_or_set_1(*last,t);
	Vwsel_free_1(t);
	if (debug) {
	  puts("Vwsel_or_set_1");
	  Vwsel_print(a);
	}
	return a;
      }
    }
    if (res >= 0) {	/* all dimensions equal, merge expressions */
      if (p->exp && q->exp)
	p->exp = mkbinop(p->exp,EXOR,q->exp);
      else {
	/* rmexp(p->exp);	/* expressions may be shared */
	p->exp = 0;
      }
      return a;
    }
    last = &p->next;
  }
  /* no overlap, insert in chain */
append:
  *last = Vwsel_copy_1(q);
  (*last)->next = p;
  return a;
}

/* basic associative law . . . */

Vwsel *Vwsel_or_set(a,b)
  Vwsel *a;
  const Vwsel *b;
{
  if (b == 0) return a;
  if (a == 0) return Vwsel_copy(b);
  while (b) {
    a = Vwsel_or_set_1(a,b);
    b = b->next;
  }
  return a;
}

/* basic distributive law . . . */

Vwsel *Vwsel_and_set(a,b)
  Vwsel *a;
  const Vwsel *b;
{
  Vwsel *x;
  if (b == 0) { Vwsel_free(a); return 0; }
  if (a == 0) return 0;
  x = Vwsel_and_1(a,b);
  while (b = b->next) {
    Vwsel *t = Vwsel_and_1(a,b);
    x = Vwsel_or_set(x,t);
    Vwsel_free(t);
  }
  Vwsel_free(a);
  return x;
}

/* Estimate how many records will be traversed by the vwsel in
   a file of cnt records.  We interpolate using the key ranges
   of each entry and use a value of 1 when the entire key must match.

   This estimate in useful for deciding in what order to read the tables.
*/

long Vwsel_estim(a,cnt)
  const Vwsel *a;
  long cnt;
{
  long total = 0L;
#ifndef BYTE
  typedef unsigned char BYTE;
#endif
  while (a) {
    int i = blkcmp((BYTE *)a->min,(BYTE *)a->max,sp->klen);
    if (i == sp->klen)
      ++total;
    else {
      long e =  cnt * ((BYTE)a->max[i] - (BYTE)a->min[i]) / 256;
      if (i > sp->klen / 3)
	e /= 256;
      if (i > sp->klen * 2 / 3)
	e /= 256;
      total += e;
    }
    a = a->next;
  }
  return total;
}

#include <stdio.h>

void Vwsel_print(const Vwsel *a) {
  int i, pos;
  if (!a) {
    puts("** Empty Vwsel **");
    return;
  }
  fputs("     ",stdout);
  for (i = 0; i < sp->kcol; putchar((++i == sp->kcol) ? '\n' : ' '))
    printColumn(sp->col[i],TITLE);
  do {
    if (sp->kcol)
      fputs("MIN: ",stdout), pos = 0;
    for (i = 0; i < sp->kcol; putchar((++i == sp->kcol) ? '\n' : ' ')) {
      char *sav = sp->col[i]->buf;
      sp->col[i]->buf = a->min + pos;	/* HACK for C++ optional arg */
      printColumn(sp->col[i],VALUE);
      sp->col[i]->buf = sav;
      pos += sp->col[i]->len;
    }
    if (sp->kcol)
      fputs("MAX: ",stdout), pos = 0;
    for (i = 0; i < sp->kcol; putchar((++i == sp->kcol) ? '\n' : ' ')) {
      char *sav = sp->col[i]->buf;
      sp->col[i]->buf = a->max + pos;	/* HACK for C++ optional arg */
      printColumn(sp->col[i],VALUE);
      sp->col[i]->buf = sav;
      pos += sp->col[i]->len;
    }
    if (a->exp) {
      fputs("EXP: ",stdout);
      sql_print(a->exp,1);
    }
    putchar('\n');
  } while (a = a->next);
}
