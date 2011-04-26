/*
 * $Log$
 * Revision 1.3  2007/09/26 17:43:03  stuart
 * Support DATE +/- INT
 *
 * Revision 1.2  2001/11/14 19:16:45  stuart
 * Implement INSERT INTO ... VALUES
 * Fix assignment bug.
 *
 * Revision 1.1  2001/02/28 23:00:03  stuart
 * Old C version of sql recovered as best we can.
 *
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include "cursor.h"
#include "obmem.h"
#include "object.h"

/* Build optimized query

	Copyright 1990 Business Management Systems, Inc.
	Author: Stuart D. Gathman

   We build a reasonably optimized query that reads a minimum number of tables.
Since there are no system maintained summary files, GROUP BY and HAVING
must be implemented as post processors at present.  We recognize and optimize 
comparisons of "bare" column references to any expression (which we will
call "basic queries").  This is done by the Vwsel module.

   Input:
	s - list of select expressions (SELECT)
	t - list of tables (FROM)
	w - select condition (WHERE)
	o - list of sort expressions (ORDER BY,GROUP BY)

   Output:
	a Join Object with Range front ends and Sort backends
	
	for each Table, a preprocessor (Where) object with:
	  an sql expression dependent only on this or previous tables.

	The sort expressions are used to select appropriate physical
	tables (i.e. keys).  A post processor is tacked on to do
	the actual sort if required.

	The order of physical tables is tricky, it will depend on:
	a) reducing the number of records processed
	b) reducing the amount of sorting required
*/

struct tab {
  const char *name;
  Cursor *tab;
  int ref;
  char outer;
};

/* Since C doesn't support nested functions, we collect the
   things that our recursive helper functions need in this structure.
   Gives you some respect for Pascal.
*/

struct tabinfo {
  sql t;		/* table list */
  Cursor **tab;		/* array of tables */
  sql *tcnd;		/* where conditions by table */
  int *ref;		/* reference count for each table */
  char *outer;
  short base;		/* number of tables in outer nesting levels */
  short ntab;		/* number of source tables */
  struct tabinfo *nest;	/* outer nesting levels */
#if 0		/* this is probably a better solution later on */
  colperm *col;		/* permutation table */
  colperm **ccol;	/* permutation tables by colidx */
  colperm ***tcol;	/* permutation table entries by tabidx,colidx */
#endif
};

static Cursor *systabcsr[1] = { &System_table };
static int systabref[1];
static struct tabinfo systabinfo = {
  0,systabcsr,0,systabref,"",0,1,0
};

static int resolve(sql, struct tabinfo *,sql);
static void renumber(struct tabinfo *);
static int maxlevel(sql);
static void split(sql, struct tabinfo *);
static void col_equiv(sql, struct tabinfo *);
static sql distrib(sql, sql);

Cursor *Select_init(const struct select *sel,int rdonly,struct tabinfo *nest) {
  Cursor *q;
  sql x;		/* scratch expression */
  int ncols = 0;	/* total columns in source tables */
  int i;
  struct tabinfo ti;	/* helps you appreciate Pascal, doesn't it? */
  int errs;

  if (nest == 0) {
    systabinfo.ref[0] = 0;
    nest = &systabinfo;
  }

  if (sel->table_list == 0)
    return Values_init(sel->select_list);

  /* step 1,	get the list of fully qualified column names for
		the list of tables from the data dictionary */

  /* how many tables are there? */
  ti.nest = nest;
  ti.base = nest->base + nest->ntab;
  ti.ntab = cntlist(sel->table_list);
  ti.tab = (Cursor **)xmalloc(ti.ntab * sizeof *ti.tab);
  ti.t   = sel->table_list;
  ti.ref = (int *)xmalloc(ti.ntab * sizeof *ti.ref);
  for (i = 0; i < ti.ntab; ti.ref[i++] = 0);
  ti.outer = (char *)xmalloc(ti.ntab);
  memset(ti.outer,0,ti.ntab);

  errs = 0;

  /* open the primary tables */
  for (i = 0,x = ti.t; x = x->u.opd[1]; ++i) {
    sql y = x->u.opd[0];
    if (y->op == EXISNUL) {
      ti.outer[i] = 1;
      y = y->u.opd[0];
    }
    if (y->op == EXQUERY)
      ti.tab[ti.ntab = i] = Select_init(y->u.q.qry,1,&ti);
    else {
      assert(y->op == EXNAME);
      if (strcasecmp(y->u.name[0],"system") == 0)
	ti.tab[i] = &System_table;
      else
	ti.tab[i] = Table_init(y->u.name[0],rdonly || ti.outer[i]);
    }
    if (!ti.tab[i]) {
      ++errs;
      continue;
    }
    if (!sel->select_list)
      ++ti.ref[i];	/* select * references user tables */
    ncols += ti.tab[i]->ncol;
  }
  ti.ntab = i;	/* restore number of tables (possibly set by EXQUERY above) */

  /* step 2,	resolve column references in s,w,o by searching 
		the list of columns from step 1.  Mark referenced
		columns - this will eventually be done by adding
		them to the permutation array, for now we just
		want to see something run and don't bother . . .
  */

  if (sel->select_list)
    errs += resolve(sel->select_list,&ti,0);

  if (sel->where_sql)
    errs += resolve(sel->where_sql,&ti,0);	/* WHERE condition */

  if (sel->order_list)
    errs += resolve(sel->order_list,&ti,0);

  if (sel->group_list)
      errs += resolve(sel->group_list,&ti,0);

  if (sel->having_sql)
    errs += resolve(sel->having_sql,&ti,0);	/* HAVING condition */

  if (errs) {
    fprintf(stderr,"%d errors in SELECT\n",errs);
    for (i = 0; i < ti.ntab; ++i)
      if (ti.tab[i])
	do0(ti.tab[i],free);		/* close the tables */
    free((PTR)ti.tab);
    free((PTR)ti.ref);
    free((PTR)ti.outer);
    return 0;
  }

  renumber(&ti);			/* renumber tables */
  col_equiv(sel->where_sql,&ti);	/* optimize equivalent columns */
  renumber(&ti);			/* renumber again */

  /* distribute ANDed expressions to tables by dependency */

  ti.tcnd = (sql *)xmalloc(ti.ntab * sizeof *ti.tcnd);
  for (i = 0; i < ti.ntab; ++i)
    ti.tcnd[i] = 0;	/* Remember, a null sql is always true */

  split(sel->where_sql,&ti);	/* distribute WHERE among tables */
  
  /* contruct Where objects */

  for (i = 0; i < ti.ntab; ++i) {
    if (debug) {
      printf("T#%d: %s",i+ti.base,ti.outer[i] ? "OUT" : "   ");
      sql_print(ti.tcnd[i],1);
    }
    if (ti.tcnd[i])
      ti.tab[i] = Where_init(ti.tab[i],ti.tcnd[i],i+ti.base,ti.outer[i]);
  }

  free((PTR)ti.tcnd);
  free((PTR)ti.outer);
  free((PTR)ti.ref);

  if (ti.ntab > 1)
    q = Join_init(ti.ntab,ti.tab);
  else if (ti.ntab)
    q = ti.tab[0];
  else
    q = &System_table;

  /* FIXME: move sql->where_sql sub-expressions
	involving EXAGG to sel->having_sql */

  i = ti.ntab + ti.base;
  /* create grouping if required */
  q = Group_init(q,sel->group_list,sel->select_list,sel->having_sql,i);

  /* sort output & select columns */
  if (sel->order_list) {
    q = Order_init(q,sel->order_list,sel->select_list,sel->distinct,i);
    if (sel->select_list)
      q = Selcol_init(q,sel->select_list);
  }
  else if (sel->select_list && sel->distinct)
    q = Order_init(q,sel->select_list,sel->select_list,1,i);
  else if (sel->select_list && rdonly)
    q = Selcol_init(q,sel->select_list);
  if (debug && sel->select_list)
    sql_print(sel->select_list,1);
  return q;
}

/* Resolve column references in an expression, return number of errors */

static int resolve(sql x,struct tabinfo *ti,sql parent) {
  const char *tname, *fname;
  sql y;
  int i,j;
  if (!x) return 0;	/* nothing to resolve */
  if (x->op == EXQUERY) {
    Cursor *t = Select_init(x->u.q.qry,1,ti);
    if (!t) return 1;
    x->op = EXCURSOR;
    x->u.s.qry = t;
    x->u.s.lev = ti->base + ti->ntab;
    return 0;
  }
  if (x->op != EXNAME) {
    const char *p = sql_form[(int)x->op].fmt;
    //sql_print(x,1);
    for (j = 0,i = 0; p[i]; ++i) if (p[i] == 'x')
      j += resolve(x->u.opd[i],ti,x);
    return j;
  }
  tname = x->u.name[0];		/* table name */
  if (!tname) tname = "";
  fname = x->u.name[1];		/* field name */
  for (i = 0,y = ti->t; y; ++i) {
    const char *name;
    if (!y->u.opd[1]) {
      if (!(ti = ti->nest)) break;
      y = ti->t;
      i = 0;
    }
    if (y && (y = y->u.opd[1])) {
      sql z = y->u.opd[0];
      if (z->op == EXISNUL)
	z = z->u.opd[0];
      name = z->u.name[1];
    }
    else
      name = "system";
    if (!*tname || !strcmp(tname,name)) {
      Cursor *t = ti->tab[i];
      if (!t) continue;
      if (fname == 0) {
	/* expand to all table columns */
	if (parent && parent->op == EXLIST) {
	  x->op = EXCOL;
	  x->u.col = t->col[0];
	  ++ti->ref[i];
	  for (j = 1; j < t->ncol; ++j) {
	    sql l = mksql(EXLIST);
	    x = mksql(EXCOL);
	    x->u.col = t->col[j];
	    ++ti->ref[i];
	    l->u.opd[1] = parent->u.opd[1];
	    l->u.opd[0] = x;
	    parent->u.opd[1] = l;
	    parent = l;
	  }
	}
	else {
	  x->op = EXHEAD;
	  x->u.opd[0] = x;
	  x->u.opd[1] = 0;
	  for (j = 0; j < t->ncol; ++j) {
	    sql c = mksql(EXCOL);
	    c->u.col = t->col[j];
	    ++ti->ref[i];
	    addlist(x,c);
	  }
	}
	return 0;
      }
      if (fname[0] == '#') {
	j = atoi(fname+1) - 1;
	if (j >= 0 && j < t->ncol) {
	  x->op = EXCOL;		/* column matches */
	  x->u.col = t->col[j];
	  ++ti->ref[i];
	  return 0;
	}
	break;
      }
      for (j = 0; j < t->ncol; ++j) {
	int rc;
	if (i)
	  rc = strcmp(fname,t->col[j]->name);
	else
	  rc = strcasecmp(fname,t->col[j]->name);
	if (!rc) {
	  x->op = EXCOL;		/* column matches */
	  x->u.col = t->col[j];
	  ++ti->ref[i];
	  return 0;
	}
      }
    }
  }
  /* invalid field or table name */
  if (fname == 0)
    fprintf(stderr,"resolve: table '%s' not found\n",tname);
  else if (tname && *tname)
    fprintf(stderr,"resolve: column '%s.%s' not found\n",tname,fname);
  else
    fprintf(stderr,"resolve: column '%s' not found\n",fname);
  return 1;
}

/* distribute ORed parts of an expression over ANDed parts */
static sql distrib(sql a,sql b) {
  if (b->op == EXAND)
    return mkbinop(distrib(a,b->u.opd[0]),EXAND,distrib(a,b->u.opd[1]));
  return mkbinop(a,EXOR,b);
}

/* distribute ANDed parts of an expression to the appropriate tables */

static void split(sql w,struct tabinfo *ti) {
  int lev;
  sql x,y;
  if (!w) return;
  switch (w->op) {
  case EXAND:
    x = w->u.opd[0];
    y = w->u.opd[1];
    rmsql(w);
    split(x,ti);
    split(y,ti);
    break;
  case EXOR:
    x = w->u.opd[0];
    y = w->u.opd[1];
    if (maxlevel(x) < maxlevel(y)) {
      if (y->op == EXAND) {
	rmsql(w);
	split(distrib(x,y),ti);
	break;
      }
    }
    else {
      if (x->op == EXAND) {
	rmsql(w);
	split(distrib(y,x),ti);
	break;
      }
    }
  default:
    lev = maxlevel(w);
    if (lev < ti->base)
      lev = ti->ntab - 1;
    else
      lev -= ti->base;
    if (ti->tcnd[lev])
      ti->tcnd[lev] = mkbinop(ti->tcnd[lev],EXAND,w);
    else
      ti->tcnd[lev] = w;
  }
}

/* find maximum table level referenced by an expression */

static int maxlevel(sql x) {
  int lev1, i;
  const char *p;
  if (!x) return 0;
  if (x->op == EXCOL)
    return x->u.col->tabidx;
  lev1 = 0;
  p = sql_form[(int)x->op].fmt;
  for (i = 0; p[i]; ++i) if (p[i] == 'x') {
    int lev2 = maxlevel(x->u.opd[i]);
    if (lev2 > lev1) lev1 = lev2;
  }
  return lev1;
}

/* find equivalent columns where (t1.fld = t2.fld) */

#define MAXEQUIV 50

struct equiv {
  int cnt, *ref, base;
  struct {
    Column *x, *y;
  } t[MAXEQUIV];
};

static void add_equiv(struct equiv *e,Column *x, Column *y) {
  Column *t;
  int i;
  if (x->tabidx < y->tabidx) {
    t = x;
    x = y;
    y = t;
  }
  for (i = 0; i < e->cnt; ++i) {
    if (e->t[i].x == x) break;
  }
  if (i == e->cnt) {
    /* table overflow just means lost optimizations */
    if (i >= MAXEQUIV) return;
    ++e->cnt;
  }
  else {
    int j;
    t = e->t[i].y;
    if (t->tabidx > y->tabidx) {	/* new minimum table idx */
      for (j = 0; j < e->cnt; ++j) {
	if (e->t[j].y == t) e->t[j].y = y;
      }
    }
    else
      y = t;
  }
  e->t[i].x = x;
  e->t[i].y = y;
}

static void build_equiv(struct equiv *e,sql w) {
  sql x,y;
  if (!w) return;
  switch (w->op) {
  case EXEQ:
    x = w->u.opd[0];
    y = w->u.opd[1];
    if (x->op == EXCOL && y->op == EXCOL)
      if (x->u.col->type == y->u.col->type)
	add_equiv(e,x->u.col,y->u.col);	/* add to equivalence table */
    break;
  case EXAND:
    build_equiv(e,w->u.opd[0]);
    build_equiv(e,w->u.opd[1]);
  }
}

/* make all equivalent columns refer to the lowest index table */

static void opt_equiv(struct equiv *e,sql w) {
  const char *p;
  int i;
  if (!w) return;
  switch (w->op) {
  case EXCOL:
    for (i = 0; i < e->cnt; ++i) {
      if (e->t[i].x == w->u.col) {
	--e->ref[w->u.col->tabidx - e->base];
	w->u.col = e->t[i].y;
	++e->ref[w->u.col->tabidx - e->base];
	break;
      }
    }
    break;
  case EXEQ:
    if (w->u.opd[0]->op == EXCOL && w->u.opd[1]->op == EXCOL) {
      break;
    }
  default:
    p = sql_form[(int)w->op].fmt;
    for (i = 0; p[i]; ++i) if (p[i] == 'x')
      opt_equiv(e,w->u.opd[i]);
  }
}

static void col_equiv(sql w,struct tabinfo *ti) {
  struct equiv etbl;
  etbl.cnt = 0;
  etbl.ref = ti->ref;
  etbl.base = ti->base;
  build_equiv(&etbl,w);
  opt_equiv(&etbl,w);
}

/* remove unused tables and set tabidx & colidx */

static void renumber(struct tabinfo *ti) {
  int i,j,n;
  for (i = 0, n = 0; i < ti->ntab; ++i) {
    Cursor *t = ti->tab[i];
    if (t && ti->ref[i]) {
      for (j = 0; j < t->ncol; ++j) {
	t->col[j]->tabidx = (short)n + ti->base;
	t->col[j]->colidx = (short)j;
      }
      ti->tab[n] = t;
      ti->outer[n] = ti->outer[i];
      ti->ref[n++] = ti->ref[i];
    }
    else if (t)
      do0(t,free);
  }
  ti->ntab = n;
}
