#include "cursor.h"
#include "object.h"

typedef struct Selcol {
  struct Cursor_class *_class;
  short ncol, klen;
  Column **col;
  long rows,cnt;
  /* private: */
  Cursor *rel;
} Selcol;

static void Selcol_free(Cursor *c) {
  int i;
  Selcol *cur = (Selcol *)c;
  c = cur->rel;
  for (i = 0; i < cur->ncol; ++i) {
    if (cur->col[i] != c->col[i])
      do0(cur->col[i],free);
  }
  do0(c,free);
  free((PTR)cur->col);
  free((PTR)cur);
}

static int Selcol_first(Cursor *cur) {
  return do0(((Selcol *)cur)->rel,first);
}

static int Selcol_next(Cursor *cur) {
  return do0(((Selcol *)cur)->rel,next);
}

static int Selcol_find(Cursor *c) {
  Selcol *cur = (Selcol *)c;
  c = cur->rel;
  if (cur->klen) {
    int i = cur->klen;
    while (i < c->klen)
      memset(c->col[i]->buf,0,c->col[i]->len);
    return do0(((Selcol *)cur)->rel,find);
  }
  return do0(((Selcol *)cur)->rel,first);
}

static void Selcol_print(Cursor *c,enum Column_type type,int sel) {
  Selcol *cur = (Selcol *)c;
  if (type < VALUE && cur->rel == &System_table) return;
  Cursor_print(c,type,sel);
}

Cursor *Selcol_init(Cursor *c,sql s) {
  Selcol *cur;
  int i;
  sql x;
  static struct Cursor_class Selcol_class = {
    sizeof *cur, Selcol_free, Selcol_first, Selcol_next, Selcol_find,
    Selcol_print, Cursor_fail, Cursor_fail, Cursor_fail, Cursor_optim
  };
  cur = (Selcol *)xmalloc(sizeof *cur);
  cur->_class = &Selcol_class;
  cur->rel = c;
  cur->klen = 0;
  cur->rows = c->rows;
  cur->cnt = c->cnt;

  cur->ncol = cntlist(s);
  cur->col  = (Column **)xmalloc(cur->ncol * sizeof *cur->col);
  for (i = 0,x = s; x = x->u.opd[1]; ++i) {
    sql y = x->u.opd[0];
    if (y->op == EXCOL && y->u.col == c->col[i]) {
      cur->col[i] = c->col[i];
      if (i == cur->klen)
	++cur->klen;
    }
    else
      cur->col[i] = Column_sql(y,"");
  }
  return (Cursor *)cur;
}
