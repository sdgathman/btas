#include <stdio.h>
#include "cursor.h"
#include "object.h"

typedef struct Values {
  struct Cursor_class *_class;
/* public: */
  short ncol;	/* number of columns */
  short klen;	/* number of key columns */
  Column **col;	/* columns pointing to sorted relation */
  long rows;	/* estimated number of rows */
  long cnt;	/* actual returned so far */
/* private: */
  sql first;
  sql cur;
} Values;

static void Values_free(Cursor *c) {
  Values *this = (Values *)c;
  int i;
  sql t = this->first;
  if (!t) return;
  for (i = 0; i < this->ncol; ++i) {
    do0(this->col[i],free);
  }
  free(this->col);
  rmexp(this->first);
  free(this);
}

static inline sql listnext(sql t) {
  return t->u.opd[1];
}

static inline sql listelem(sql t) {
  return t->u.opd[0];
}

static void setrow(Values *this) {
  int i;
  sql t = listelem(this->cur);
  t = listnext(t);
  for (i = 0; i < this->ncol && t; ++i) {
    do2(this->col[i],store,listelem(t),0);
    t = listnext(t);
  }
}

static int Values_first(Cursor *c) {
  Values *this = (Values *)c;
  this->cur = listnext(this->first);
  if (!this->cur) return -1;	/* empty values list */
  setrow(this);
  return 0;
}

static int Values_next(Cursor *c) {
  Values *this = (Values *)c;
  if (!this->cur) return -1;	/* empty values list */
  this->cur = listnext(this->cur);
  if (!this->cur) return -1;	/* EOF */
  setrow(this);
  return 0;
}

Cursor *Values_init(sql val) {
  Values *this = xmalloc(sizeof *this);
  static struct Cursor_class Values_class = {
    sizeof *this, Values_free, Values_first, Values_next, Cursor_fail, 
    Cursor_print, Cursor_fail, Cursor_fail, Cursor_fail, Cursor_optim
  };
  int i;
  sql t = sql_eval(val,MAXTABLE);
  this->_class = &Values_class;
  this->klen = 0;
  this->rows = 0;	/* number of value rows */
  this->cnt = 0;	/* number of value rows */
  this->cur = 0;
  this->first = t;
  this->col = 0;
  t = listnext(t);	/* first list node */
  if (!t) {
    this->ncol = 0;
    return (Cursor *)this;
  }
  this->ncol = cntlist(listelem(t));	/* number of columns in first row */

  ++this->rows;
  while (t = listnext(t)) {
    if (cntlist(listelem(t)) != this->ncol)
      fputs("VALUES must have same number of columns\n",stderr);
    ++this->rows;
  }

  if (debug) {
    sql_print(this->first,1);
    fprintf(stderr,"ncols = %d\n",this->ncol);
  }

  this->col = xmalloc(sizeof *this->col * this->ncol);
  t = listelem(listnext(this->first));
  t = listnext(t);
  for (i = 0; t && i < this->ncol; ++i) {
    this->col[i] = Column_sql(listelem(t),"");
    t = listnext(t);
  }
  return (Cursor *)this;
}
