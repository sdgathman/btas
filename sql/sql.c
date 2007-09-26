#include <stdio.h>
#include <errenv.h>
#include <assert.h>
#include <string.h>
#include <date.h>
#include "config.h"
#include "sql.h"
#include "obmem.h"
#include "object.h"
#include "cursor.h"

static struct sqlnode SQLFIXED[] = {
  { EXNULL },
  { EXBOOL, 1 },
  { EXBOOL, 0 },
  { EXCONST },
  { EXSTRING }
};
enum { SQLNULL, SQLTRUE, SQLFALSE, SQLZERO, SQLNULSTR, NFIXED };
#define ISFIXED(x) (x >= SQLFIXED && x < SQLFIXED+NFIXED)

sql sql_nul = &SQLFIXED[SQLNULL];

struct obstack *sqltree;
static sql freeptr = 0;

sql mksql(enum sqlop op) {
  sql x;
  if (freeptr) {
    x = freeptr;
    freeptr = x->u.opd[0];
  }
  else {
    x = (sql)obstack_alloc(sqltree,sizeof *x);
  }
  x->op = op;
  return x;
}

/* delete a single sql node */

void rmsql(sql x) {
  if (ISFIXED(x)) return;
  x->u.opd[0] = freeptr;
  freeptr = x;
}

/* recursively delete an sql expression */

void rmexp(sql x) {
  const char *p;
  int i;
  if (!x) return;
  p = sql_form[(int)x->op].fmt;
  for (i = 0; p[i]; ++i) if (p[i] == 'x')
    rmexp(x->u.opd[i]);
  rmsql(x);
}

sql mkname(const char *a,const char *b) {
  sql x = mksql(EXNAME);
  x->u.name[0] = a;
  x->u.name[1] = b;
  return x;
}

sql mkconst(const sconst *m) {
  sql x;
  if (sgnM(&m->val) == 0)
    return &SQLFIXED[SQLZERO];
  x = mksql(EXCONST);
  x->u.num = *m;
  return x;
}

sql mkbool(int f) {
  return f ? &SQLFIXED[SQLTRUE] : &SQLFIXED[SQLFALSE];
}

sql mkstring(const char *s) {
  sql x;
  if (*s == 0) {
    x = &SQLFIXED[SQLNULSTR];
    x->u.name[0] = "";
  }
  else {
    x = mksql(EXSTRING);
    x->u.name[0] = s;
  }
  return x;
}

sql mkunop(enum sqlop op,sql x) {
  sql a;
  switch (op) {
  case EXISNUL:
    switch (x->op) {
    case EXNULL:
      return mkbool(1);
    case EXCONST: case EXDBL: case EXBOOL: case EXSTRING: case EXDATE:
      return mkbool(0);
    }
    break;
  case EXNOT:
    switch (x->op) {
    case EXNULL:
      return x;
    case EXBOOL:
      return mkbool(!x->u.ival);
    case EXAND:
      x->u.opd[0] = mkunop(EXNOT,x->u.opd[0]);
      x->u.opd[1] = mkunop(EXNOT,x->u.opd[1]);
      x->op = EXOR;
      return x;
    case EXOR:
      x->u.opd[0] = mkunop(EXNOT,x->u.opd[0]);
      x->u.opd[1] = mkunop(EXNOT,x->u.opd[1]);
      x->op = EXAND;
      return x;
    case EXGT:
      x->op = EXLE;
      return x;
    case EXGE:
      x->op = EXLT;
      return x;
    case EXEQ:
      x->op = EXNE;
      return x;
    case EXNE:
      x->op = EXEQ;
      return x;
    case EXLT:
      x->op = EXGE;
      return x;
    case EXLE:
      x->op = EXGT;
      return x;
    case EXNOT:
      a = x->u.opd[0];
      rmsql(x);
      return a;
    }
    break;
  case EXNEG:
    switch (x->op) {
    case EXDATE:
      x->op = EXCONST;	// no longer a date
    case EXCONST:
      mulM(&x->u.num.val,-1,0);
      return x;
    case EXDBL:
      x->u.val = -x->u.val;
      return x;
    case EXSUB:
      a = x->u.opd[0];
      x->u.opd[0] = x->u.opd[1];
      x->u.opd[1] = a;
      return x;
    }
    break;
  }
  if (x->op == EXNULL)
    return x;
  a = mksql(op);
  a->u.opd[0] = x;
  return a;
}

sql mkalias(const char *n,sql a) {
  sql x = mksql(EXALIAS);
  x->u.a.exp = a;
  x->u.a.name = n;
  return x;
}

double const2dbl(const sconst *num) {
  static long scale[] = {
    1, 10, 100, 1000, 10000, 100000L, 1000000L,
    10000000L, 100000000L, 1000000000L
  };
  double a = Mtof(&num->val);
  if (num->fix > 0)		/* FIXME: can |fix| be > 9? */
    a /= scale[num->fix];
  else if (num->fix < 0)
    a *= scale[-num->fix];
  return a;
}

sql mkbinop(sql x,enum sqlop op,sql y) {
  sql z;

  if (x->op == EXNULL) {
    z = x;
    x = y;
    y = z;
  }
  if (y->op == EXNULL) {
    switch (op) {
    case EXAND: case EXOR:
      if (op == EXAND && isfalse(x)) return x;
      if (op == EXOR && istrue(x)) return x;
      z = mksql(op);
      z->u.opd[0] = x;
      z->u.opd[1] = y;
      return z;
    }
    rmexp(x); return y;
  }

  switch (op) {
  case EXCAT:
    x = tostring(x);
    y = tostring(y);
    break;
  }

  if (op == EXADD || op == EXSUB) {
    if (y->op == EXDATE && op == EXADD) {
      sql t = x; x = y; y = t;
    }
    if (x->op == EXDATE || x->op == EXCONST) {
      if (y->op == EXCONST || y->op == EXDBL && x->op == EXDATE) {
	y = toconst(y,x->u.num.fix);
	if (op == EXSUB)
	  subM(&x->u.num.val,&y->u.num.val);
	else
	  addM(&x->u.num.val,&y->u.num.val);
	rmsql(y);
	return x;
      }
    }
  }

  if (x->op == EXCONST || x->op == EXDATE) {
    x->u.val = const2dbl(&x->u.num);
    x->op = EXDBL;
  }
  if (y->op == EXCONST || y->op == EXDATE) {
    y->u.val = const2dbl(&y->u.num);
    y->op = EXDBL;
  }
  if (x->op == EXDBL && y->op == EXDBL) {
    double d;
    static double eps = .355271E-14;
    switch (op) {
    case EXADD:
      x->u.val += y->u.val;
      break;
    case EXSUB:
      x->u.val -= y->u.val;
      break;
    case EXMUL:
      x->u.val *= y->u.val;
      break;
    case EXDIV:
      x->u.val /= y->u.val;
      break;
    case EXEQ:
      d = x->u.val - y->u.val;
      if (d < 0.0) d = -d;
      rmsql(x);
      x = mkbool(d < eps);
      break;
    case EXNE:
      d = x->u.val - y->u.val;
      if (d < 0.0) d = -d;
      rmsql(x);
      x = mkbool(d >= eps);
      break;
    case EXGT:
      d = x->u.val - y->u.val;
      rmsql(x);
      x = mkbool(d >= eps);
      break;
    case EXLT:
      d = y->u.val - x->u.val;
      rmsql(x);
      x = mkbool(d >= eps);
      break;
    case EXGE:
      d = y->u.val - x->u.val;
      rmsql(x);
      x = mkbool(d < eps);
      break;
    case EXLE:
      d = x->u.val - y->u.val;
      rmsql(x);
      x = mkbool(d < eps);
      break;
    default:
      z = mksql(op);
      z->u.opd[0] = x;
      z->u.opd[1] = y;
      return z;
    }
    rmsql(y);
    return x;
  }
  if (x->op == EXBOOL) {
    switch (op) {
    case EXAND:
      if (x->u.ival) {
	rmsql(x);
	return y;
      }
      else {
	rmexp(y);
	return x;
      }
    case EXOR:
      if (x->u.ival) {
	rmexp(y);
	return x;
      }
      else {
	rmsql(x);
	return y;
      }
    }
  }
  if (y->op == EXBOOL) {
    switch (op) {
    case EXAND:
      if (y->u.ival) {
	rmsql(y);
	return x;
      }
      else {
	rmexp(x);
	return y;
      }
    }
  }
  if (x->op == EXSTRING && y->op == EXSTRING) {
    switch (op) {
    case EXEQ:
      z = mkbool(strcmp(x->u.name[0],y->u.name[0]) == 0);
      rmsql(x); x = z;
      break;
    case EXNE:
      z = mkbool(strcmp(x->u.name[0],y->u.name[0]) != 0);
      rmsql(x); x = z;
      break;
    case EXGT:
      z = mkbool(strcmp(x->u.name[0],y->u.name[0]) > 0);
      rmsql(x); x = z;
      break;
    case EXGE:
      z = mkbool(strcmp(x->u.name[0],y->u.name[0]) >= 0);
      rmsql(x); x = z;
      break;
    case EXLT:
      z = mkbool(strcmp(x->u.name[0],y->u.name[0]) < 0);
      rmsql(x); x = z;
      break;
    case EXLE:
      z = mkbool(strcmp(x->u.name[0],y->u.name[0]) <= 0);
      rmsql(x); x = z;
      break;
    case EXLIKE:
      z = mkbool(like(x->u.name[0],y->u.name[0]));
      rmsql(x); x = z;
      break;
    case EXCAT:
      { const char *p;
	for (p = x->u.name[0]; *p; obstack_1grow(sqltree,*p++));
	for (p = y->u.name[0]; *p; obstack_1grow(sqltree,*p++));
	obstack_1grow(sqltree,0);
	x->u.name[0] = (char *)obstack_finish(sqltree);
      }
      break;
    default:
      z = mksql(op);
      z->u.opd[0] = x;
      z->u.opd[1] = y;
      return z;
    }
    rmsql(y);
    return x;
  }

  z = mksql(op);
  z->u.opd[0] = x;
  z->u.opd[1] = y;
  return z;
}

sql mklist() {		/* create empty list */
  sql x = mksql(EXHEAD);
  x->u.opd[0] = x;	/* pointer to last list node */
  x->u.opd[1] = 0;	/* pointer to next list node */
  return x;
}

void addlist(sql a,sql b) {	/* add to end of list */
  sql x;
  assert(a->op == EXHEAD);
  x = a->u.opd[0];			/* last element */
  assert(x == a || x->op == EXLIST);
  assert(b->op != EXLIST);		/* appending another list */
#if 0	/* create applist() if we need this */
  assert(b->op == EXHEAD);		/* appending another list */
  a->u.opd[0] = b->u.opd[0];		/* new last element */
  x->u.opd[1] = b->u.opd[1];
  rmsql(b);				/* discard second header */
  return;
#endif
  a->u.opd[0] = x->u.opd[1] = mksql(EXLIST);	/* add new EXLIST node */
  x = x->u.opd[1];
  x->u.opd[0] = b;
  x->u.opd[1] = 0;
}

sql rmlast(sql a) {
  sql x;
  assert(a->op == EXHEAD);
  if (a->u.opd[1] == 0) return 0;	/* list is empty */
  for (x = a; x->u.opd[1] != a->u.opd[0]; x = x->u.opd[1]);
  a->u.opd[0] = x;		/* new last header */
  a = x->u.opd[1];		/* original last header */
  x->u.opd[1] = 0;		/* terminate list */
  x = a->u.opd[0];		/* element removed */
  rmsql(a);			/* discard header */
  return x;
}

int cntlist(sql x) {	/* count elements in list */
  int cnt;
  assert(x->op == EXHEAD);
  for (cnt = 0; x = x->u.opd[1]; ++cnt);
  return cnt;
}

/* evaluate columns with tabidx < idx */
sql sql_eval(sql x,int idx) {
  const char *p;
  sql op1,a;
  if (x == 0) return 0;
  if (ISFIXED(x)) return x;
  switch (x->op) {
  case EXHEAD:
    /* evaluate list elements and duplicate headers */
    a = mklist();
    for (op1 = x->u.opd[1]; op1; op1 = op1->u.opd[1])
      addlist(a,sql_eval(op1->u.opd[0],idx));
    return a;
  case EXEXIST: case EXUNIQ:
    op1 = x->u.opd[0];
    if (op1->op == EXCURSOR && op1->u.s.lev < idx) {
      sql p;
      p = free_sql(sqltree);
      a = mkbool(do0(op1->u.s.qry,first) == 0);
      if (a->u.ival && x->op == EXUNIQ)
	a = mkbool(do0(op1->u.s.qry,next) != 0);
      pop_sql(p);
      return a;
    }
    break;
  }
  p = sql_form[(int)x->op].fmt;
  switch (p[0]) {
  case 'x':
    op1 = sql_eval(x->u.opd[0],idx);
    switch (p[1]) {
    case 0:
      return mkunop(x->op,op1);
    case 'x':
      return mkbinop(op1,x->op,sql_eval(x->u.opd[1],idx));
    case 'f':
      return (op1->op == EXNULL) ? op1 : (*x->u.f.func)(op1);
    case 's':
      return mkalias(x->u.a.name,op1);
    }
    return op1;
  case 'c':
    if (x->u.col->tabidx < (short)idx)
      return do0(x->u.col,load);
    break;
  }
  op1 = mksql(x->op);
  memcpy(&op1->u,&x->u,sizeof op1->u);	/* AIX compiler bug */
  /* op1->u = x->u; */
  return op1;
}

/* compute worst case print width of an expression */

int sql_width(sql x) {
  int w,w1;
  switch (x->op) {
  case EXNULL:
    return 4;
  case EXCOL:
    return x->u.col->width;
  case EXFUNC:
    return func_width(x);
  case EXDATE:
  case EXCONST:
  case EXDBL:
    return sql_width(tostring(x));
  default:
    return 18;		/* lazy for now */
  case EXSTRING:
    return strlen(x->u.name[0]);
  case EXCAT:
    return sql_width(x->u.opd[0]) + sql_width(x->u.opd[1]);
  case EXMUL: case EXDIV:
    w = sql_width(x->u.opd[0]) + sql_width(x->u.opd[1]);
    if (w > 18) return 18;
    return w;
  case EXADD: case EXSUB:
    w = sql_width(x->u.opd[0]);
    w1 = sql_width(x->u.opd[1]);
    if (w1 > w) w = w1;
    if (++w > 18) return 18;
    return w;
  }
}

/* init sql expression memory to use an obstack */

sql free_sql(struct obstack *o) {
  struct obstack *old = sqltree;
  sql oldptr = freeptr;
  sql p;
  sqltree = o;
  freeptr = 0;
  p = mksql(EXNOP);
  p->u.p.stk = old;
  p->u.p.freelist = oldptr;
  return p;
}

void pop_sql(sql p) {
  assert(p->op == EXNOP);
  sqltree = p->u.p.stk;
  freeptr = p->u.p.freelist;
}

/* generate table of opcode descriptions */
#ifdef __STDC__
#define DEF_SQL(x,y,z)	{ #x, y, z },
#else
#define DEF_SQL(x,y,z)	{ "x", y, z },
#endif
struct sqlform sql_form[] = {
#include "sql.def"
};

static char buf[32];

static const char *dump_date(const sconst *n) {
  struct mmddyy mdy;
  julmdy(Mtol(&n->val),&mdy);
  sprintf(buf,"%2d/%02d/%04d",mdy.mm,mdy.dd,mdy.yy);
  return buf;
}

static const char *dump_num(const sconst *n) {
  int fix = n->fix;
  char *p = buf, *q = buf;
  char sign = 0;
  MONEY m = n->val;
  if (sgnM(&m) < 0) {
    mulM(&m,-1,0);
    sign = '-';
  }
  if (fix > 0) {
    while (fix)
      *p++ = (char)divM(&m,10) + '0', --fix;
    *p++ = '.';
  }
  while (fix++ < 0)
    *p++ = '0';
  while (sgnM(&m))
    *p++ = (char)divM(&m,10) + '0';
  if (sign) *p++ = sign;
  *p = 0;
  while (p > q) {
    char c = *--p;
    *p = *q;
    *q++ = c;
  }
  return buf;
}

sql tostring(sql x) {
  const char *p;
  switch (x->op) {
  char buf[32];
  case EXDATE:
    p = dump_date(&x->u.num);
    break;
  case EXCONST:
    p = dump_num(&x->u.num);
    break;
  case EXDBL:
    sprintf(buf,"%g",x->u.val);
    p = buf;
    break;
  case EXBOOL:
    if (x->u.ival)
      p = "TRUE";
    else
      p = "FALSE";
    break;
  default:
    return x;
  }
  return mkstring((char *)obstack_copy0(sqltree,p,strlen(p)));
}

void sql_print(sql x,int nl) {
  const char *p, *o;
  int i;
  char binop;
  if (!x) {
    printf("NONE\n");
    return;
  }
  switch (x->op) {
  case EXNULL:
    printf("NULL");
    break;
  case EXHEAD:
    putchar('(');
    while (x = x->u.opd[1]) {
      sql_print(x->u.opd[0],0);
      if (x->u.opd[1])
	putchar(',');
    }
    putchar(')');
    break;
  case EXSTRING:
    printf("'%s'",x->u.name[0]);
    break;
  case EXNAME:
    printf(
      "%s.%s",
      x->u.name[0]?x->u.name[0]:"*",
      x->u.name[1]?x->u.name[1]:"*"
    );
    break;
  case EXDATE:
    printf("J'%s'",dump_date(&x->u.num));
    break;
  case EXCONST:
    fputs(dump_num(&x->u.num),stdout);
    break;
  case EXDBL:
    printf("%f",x->u.val);
    break;
  case EXFUNC:
    printf(x->u.f.arg->op != EXHEAD ? "%s(" : "%s",nmfunc(x->u.f.func));
    sql_print(x->u.f.arg,0);
    if (x->u.f.arg->op != EXHEAD) putchar(')');
    break;
  case EXCOL:
    if (x->u.ival < 200)
      printf("<%d>",x->u.ival);
    else
      printf("%d.%s",x->u.col->tabidx,x->u.col->name);
    break;
  case EXBOOL:
    printf("%s",x->u.ival ? "TRUE" : "FALSE");
    break;
  case EXAGG:
    switch (x->u.g.op) {
    case AGG_SUM: p = "SUM"; break;
    case AGG_CNT: p = "CNT"; break;
    case AGG_MAX: p = "MAX"; break;
    case AGG_MIN: p = "MIN"; break;
    }
    printf("%s(",p);
    sql_print(x->u.f.arg,0);
    putchar(')');
    break;
  default:
    p = sql_form[(int)x->op].fmt;
    o = sql_form[(int)x->op].op;
    binop = 1;
    if (o[0]) {
      if (strcmp(p,"xx") == 0)
	putchar('(');
      else {
	if (p[1] == 0)
	  printf(o);
	binop = 0;
      }
    }
    else
      printf("%s(",sql_form[(int)x->op].name);
    for (i = 0; p[i]; ++i) {
      if (i)
	if (o[0])
	  printf(o);
	else
	  printf(",");
      switch (p[i]) {
      case 'x':
	sql_print(x->u.opd[i],0);
	break;
      case 's':
	printf(x->u.name[i]);
	break;
      }
    }
    if (binop) putchar(')');
  }
  if (nl) putchar('\n');
}

sql toconst(sql x,int fix) {
  int i;
  double val;
  switch (x->op) {
    const char *p;
  case EXSTRING:
    p = x->u.name[0];
    if (getconst(&x->u.num,&p) == 0) {
      x->op = EXNULL;
      break;
    }
  case EXDATE:
    x->op = EXCONST;
    /* now match decimals */
  case EXCONST:
    i = x->u.num.fix;
    /* match decimal points */
    for (i = x->u.num.fix; i < fix; ++i)
      mulM(&x->u.num.val,10,0);
    if (i > fix)  {
      MONEY a;
      a = ltoM(5L);
      while (i-- > fix) {
	addM(&x->u.num.val,&a);
	divM(&x->u.num.val,10);
      }
    }
    break;
  case EXDBL:
    i = fix;
    val = x->u.val;
    for (i = fix; i > 0; --i)
      val *= 10;
    while (i++ < 0)
      val /= 10;
    x->u.num.val = ftoM(val + .5);
    x->u.num.fix = fix;
    x->op = EXCONST;
    break;
  }
  return x;
}

int getconst(sconst *p,const char **lexptr) {
  char decpnt = 0, seen = 0, sign = 0;
  p->val = zeroM;
  p->fix = 0;
  while (**lexptr == ' ' || **lexptr == '\t' || **lexptr == '\n')
    ++*lexptr;
  while (**lexptr) {
    switch (**lexptr) {
    case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9':
      mulM(&p->val,10,*(*lexptr)++ - '0');
      p->fix += decpnt;
      seen = 1;
      continue;
    case '.':
      decpnt = 1;
      ++*lexptr;
      continue;
    case '-':
      sign = 1;
      ++*lexptr;
      continue;
    }
    break;
  }
  if (sign)
    mulM(&p->val,-1,0);
  return seen;
}
