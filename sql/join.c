#include "object.h"
#include "cursor.h"

/* The first and find methods are not really correct.  The conditions
   that caused a first/find failure on a table may be related to the
   contents of records already read.  We must then go back and do 
   a next operation on the previous table and try again.  This could
   be a lengthy process.  It is not necessary if the WHERE conditions
   for a table depend only on constants and not on previous records.  
   There is currently no way for us to know this.

   The thing that makes even a slow version non-trivial is that we must
   remember the key we started with when backtracking.  This is an argument
   for passing the key for find as an argument.
*/

typedef struct Join/*:public Cursor*/ {
  struct Cursor_class *_class;
/* from Cursor */
  short ncol;	/* number of columns */
  short klen;	/* number of key columns */
  Column **col;
  long rows;
  long cnt;
/* private: */
  int n;
  Cursor *rel[1];
} Join;

static void Join_free(/**/ Cursor * /**/);
static int Join_first(/**/ Cursor * /**/);
static int Join_next(/**/ Cursor * /**/);
static int Join_find(/**/ Cursor * /**/);
static int Join_insert(/**/ Cursor * /**/);
static int Join_update(/**/ Cursor * /**/);
static int Join_delete(/**/ Cursor * /**/);

Cursor *Join_init(int n,Cursor **rel) {
  Join *j;
  static struct Cursor_class Join_class = {
    sizeof *j, Join_free, Join_first, Join_next, Join_find, Cursor_print,
    Join_insert,Join_update,Join_delete,Cursor_optim
  };
  int i;
  j = (Join *)xmalloc(sizeof *j - sizeof j->rel + n*sizeof *j->rel);
  j->_class = &Join_class;
  j->ncol = 0;
  j->rows = 1;
  for (i = 0; i < n; ++i) {
    j->ncol += rel[i]->ncol;
    j->rows *= rel[i]->rows;
  }
  j->col = (Column **)xmalloc(sizeof *j->col * j->ncol);
  j->n = n;
  j->klen = 0;
  for (n = 0; n < j->n; ++n) {
    j->rel[n] = rel[n];
    for (i = 0; i < rel[n]->klen; ++i)
      j->col[j->klen++] = rel[n]->col[i];	/* install key columns */
  }
  j->ncol = j->klen;
  for (n = 0; n < j->n; ++n) {
    for (i = rel[n]->klen; i < rel[n]->ncol; ++i)
      j->col[j->ncol++] = rel[n]->col[i];	/* install non-key columns */
  }
  return (Cursor *)j;
}

static void Join_free(Cursor *c) {
  Join *j = (Join *)c;
  int i;
  for (i = 0; i < j->n; ++i)
    do0(j->rel[i],free);
  free((PTR)j->col);
  free((PTR)j);
}

static int Join_first(Cursor *c) {
  Join *j = (Join *)c;
  int i;
  for (i = 0; i < j->n; ++i) {
    if (do0(j->rel[i],first)) {
      /* if outer join, generate null record here */
      do {
	if (--i < 0) return -1;
      } while (do0(j->rel[i],next));
    }
  }
  j->cnt = 1;
  return 0;
}

static int Join_next(Cursor *c) {
  Join *j = (Join *)c;
  int i = j->n;
  while (i--) {
    if (do0(j->rel[i],next)) continue;
    do {
      if (++i >= j->n) {
	++j->cnt;
	return 0;
      }
    } while (do0(j->rel[i],first) == 0);
    /* generate null record here if outer join */
  }
  return -1;
}

static int Join_insert(Cursor *c) {
  Join *j = (Join *)c;
  int i = j->n;
  while (i--) {
    if (do0(j->rel[i],insert)) {	/* if DUPKEY */
      while (++i < j->n)
	do0(j->rel[i],del);	/* undo what we already did */
      return -1;
    }
  }
  return 0;
}

static int Join_update(Cursor *c) {
  Join *j = (Join *)c;
  int rc = 0;
  int i = j->n;
  while (i--) {
    if (do0(j->rel[i],update))
      rc = -1;
  }
  return rc;
}

static int Join_delete(Cursor *c) {
  Join *j = (Join *)c;
  int rc = -1;
  int i = j->n;
  while (i--) {
    if (do0(j->rel[i],del) == 0)
      rc = 0;
  }
  return rc;
}

static int Join_find(Cursor *c) {
  /* for this to really work we need to pass the key as an argument */
  Join *j = (Join *)c;
  int i;
  for (i = 0; i < j->n; ++i) {
    if (do0(j->rel[i],find)) {
      /* if outer join, generate null record here */
      do {
	if (--i < 0) return -1;
      } while (do0(j->rel[i],next));
    }
  }
  return 0;
}
