/*	execute a Vwsel list
	Copyright 1990 Business Management Systems, Inc.
	Author: Stuart D. Gathman
 * $Log$
 */

#include <stdio.h>
#include <block.h>
#include "obmem.h"
#include "object.h"
#include "vwopt.h"
#include "cursor.h"

#if !defined(lint) && !defined(__MSDOS__)
static const char what[] = "$Id$";
#endif

typedef struct where/*:Cursor*/ {
  struct Cursor_class *_class;
  short ncol;
  short klen;
  Column **col;		/* array of column objects */
  long rows;		/* estimated number of rows */
  long cnt;
/* private: */
  Vwsel_ctx *ctx;
  Cursor *rel;		/* source relation */
  sql exp;		/* where condition */
  Vwsel *p ,**a;	/* optimized where condition, current pointers */
  short idx;		/* table index */
  char eof;
} Where;

static void Where_free(Cursor *);
static int Where_first(Cursor *);
static int Where_next(Cursor *);
static int Where_find(Cursor *);
static int Where_read(Where *);
static int Where_insert(Cursor *);
static int Where_update(Cursor *);
static int Where_delete(Cursor *);

static int Where_first_outer(Cursor *c) {
  int rc = Where_first(c);
  if (rc == -1) {
    int i;
    /* return null record */
    for (i = 0; i < c->ncol; ++i)
      do2(c->col[i],store,sql_nul,c->col[i]->buf);
    if (debug)
      puts("NULL record supplied");
    return 0;
  }
  return rc;
}

struct optim_rec {
  int tabidx;
  int colidx;
  Column *col[8];
};

static void Where_optim(sql x,struct optim_rec *p) {
  switch (x->op) {
    int i;
  case EXAND: case EXOR:
  case EXEQ: case EXLT: case EXGT: case EXLE: case EXGE: case EXNE:
    if (p->colidx >= 8) break;
    Where_optim(x->u.opd[0],p);
    Where_optim(x->u.opd[1],p);
    break;
  case EXCOL:
    if (p->colidx >= 8 || x->u.col->tabidx != p->tabidx) break;
    for (i = 0; i < p->colidx; ++i) {
      if (p->col[i] == x->u.col) break;
    }
    if (i == p->colidx) {
      p->col[p->colidx++] = x->u.col;
      if (debug) printf(" %s",x->u.col->name);
    }
    break;
  }
}


Cursor *Where_init(Cursor *rel,sql exp,int idx,int outer) {
  Where *this;
  struct optim_rec r;
  static struct Cursor_class Where_class = {
    sizeof *this, Where_free, Where_first, Where_next, Where_find,
    Cursor_print, Where_insert, Where_update, Where_delete, Cursor_optim
  };
  static struct Cursor_class Where_class_outer = {
    sizeof *this, Where_free, Where_first_outer, Where_next, Where_find,
    Cursor_print, Where_insert, Where_update, Where_delete, Cursor_optim
  };
  this = xmalloc(sizeof *this);
  this->_class = outer ? &Where_class_outer : &Where_class;

  r.tabidx = idx;
  r.colidx = 0;
  if (debug) printf("Where_optim:");
  Where_optim(exp,&r);
  if (debug) puts("");
  do3(rel,optim,r.col,r.colidx,r.colidx);

  this->ncol = rel->ncol;
  this->klen = rel->klen;
  this->col  = rel->col;
  this->cnt  = 0;
  this->rel  = rel;
  this->exp  = exp;
  this->a    = xmalloc(sizeof *this->a * this->klen);
  this->idx  = (short)idx;
  this->eof  = 1;
  this->ctx = Vwsel_push(0,this->col,this->klen,idx);
  this->p = Vwsel_init(this->exp);
  this->rows = Vwsel_estim(this->p,rel->rows);
  return (Cursor *)this;
}

static void Where_free(Cursor *c) {
  Where *this = (Where *)c;
  do0(this->rel,free);
  Vwsel_free_ctx(this->ctx);
  free((PTR)this->a);
  free((PTR)this);
}

static int Where_first(Cursor *c) {
  int i, pos, len;
  Where *this = (Where *)c;
  this->cnt = 0;
  this->eof = 0;
  this->ctx = Vwsel_push(this->ctx,this->col,this->klen,this->idx);
  this->p = Vwsel_init(this->exp);
  if (debug) {
    printf("Where_first: T#%d\n",this->idx);
    Vwsel_print(this->p);
  }
  if (this->p == 0)
    return -1;
  for (pos = 0,i = 0; i < this->klen; pos += len,this->a[i++] = this->p) {
    len = this->col[i]->len;
    memcpy(this->col[i]->buf,this->p->min + pos,len);
  }
  if (do0(this->rel,find)) return -1;
  return Where_read(this);
}

static int Where_find(Cursor *c) {
  Where *this = (Where *)c;
  if (!this->p) return -1;
  if (do0(this->rel,find)) return -1;
  this->eof = 0;
  return Where_read(this);
}

static int Where_insert(Cursor *c) {
  Where *this = (Where *)c;
  return do0(this->rel,insert);
}

static int Where_update(Cursor *c) {
  Where *this = (Where *)c;
  return do0(this->rel,update);
}

static int Where_delete(Cursor *c) {
  Where *this = (Where *)c;
  return do0(this->rel,del);
}

static int Where_next(Cursor *c) {
  Where *this = (Where *)c;
  if (!this->p || this->eof) return -1;
  if (do0(this->rel,next)) {
    this->rows = this->cnt;
    return -1;
  }
  return Where_read(this);
}

static int Where_read(Where *this) {
  int rc;
  char scan = 0;
  do {
    int pos = 0, i;
    /* check vwsel limits */
    for (i = 0; i < this->klen; ++i) {
      int len = this->col[i]->len;
      int j;
      if (blkcmpr(this->p->max + pos,this->col[i]->buf,len) < 0) {
	if (!this->p->next || blkcmpr(this->p->min,this->p->next->min,pos)) {
	  if (i == 0) goto eof;
	  for (j = i; j < this->klen; ++j)
	    memset(this->col[j]->buf,0,this->col[j]->len);
	  while (i-- > 0) {	/* increment prior key */
	    len = this->col[i]->len;
	    while (len-- && !++this->col[i]->buf[len]);
	    if (len >= 0) break;
	  }
	  rc = do0(this->rel,find);
	  break;
	}
	this->p = this->p->next;
	for (j = i--; ++j < this->klen; this->a[j] = this->p);
	continue;
      }
      if (blkcmpr(this->p->min + pos,this->col[i]->buf,len) > 0) {
	int j;
	if (this->p != this->a[i] && !scan) {
	  scan = 1;
	  this->p = this->a[i];
	  rc = 0;
	  break;
	}
	scan = 0;
	for (j = i; j < this->klen; ++j) {
	  len = this->col[j]->len;
	  memcpy(this->col[j]->buf,this->p->min + pos,len);
	  pos += len;
	}
	this->p = this->a[i];
	rc = do0(this->rel,find);
	break;
      }
      pos += len;
    }
    /* if record passed vwsel, check expression */
    if (i >= this->klen) {
      sql x;
      /* in range, check expression if present */
      if (x = this->p->exp)
	x = sql_eval(x,MAXTABLE);	/* return zero if TRUE */
      if (x) {
	rc = (x->op == EXINT && x->u.ival);
	rmexp(x);
      }
      else
	rc = 1;
      if (rc) {
	++this->cnt;
	return 0;		/* got a matching record */
      }
      /* record failed, try next one */
      rc = do0(this->rel,next);
    }
  } while (rc == 0 && !cancel);
eof:
  this->rows = this->cnt;
  this->eof  = 1;
  return -1;
}
