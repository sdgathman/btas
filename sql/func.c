/*
	Built in functions.  These may be called at compile time 
(for constant reduction) or at run time.
They must act like the mkxxx function - they consume their input expression
and return the output expression.  The input expression is always an EXHEAD.
The input code may be checked for sanity or for recursive convenience.
(I.e. strip the EXHEAD then call yourself.)
 *$Log$
 *Revision 1.4  2007/09/26 20:21:41  stuart
 *Implement NULLIF
 *
 *Revision 1.3  2007/09/26 19:07:06  stuart
 *Create SQLTRUE, SQLFALSE, SQLZERO, SQLNULSTR fixed nodes.
 *
 *Revision 1.2  2007/09/26 17:04:38  stuart
 *Implement EXDATE type.
 *
 *Revision 1.1  2001/02/28 23:00:03  stuart
 *Old C version of sql recovered as best we can.
 *
 */

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <date.h>
#include "sql.h"
#include "obmem.h"

static sql applylist(sql x,sql (*f)(sql)) {
  if (x->op == EXHEAD) {
    sql z;
    for (z = x->u.opd[1]; z; z = z->u.opd[1])
      z->u.opd[0] = applylist(z->u.opd[0],f);
    z = x->u.opd[1];
    if (z && z->u.opd[1] == 0) {
      rmsql(x);
      x = z->u.opd[0];
      rmsql(z);
    }
    return x;
  }
  return (*f)(x);
}

static sql delayFunc(sql x,sql (*f)(sql)) {
  sql z = mksql(EXFUNC);
  z->u.f.arg = x;
  z->u.f.func = f;
  return z;
}

static sql sql_dayno(sql x) {
  sql z;
  switch (x->op) {
  case EXSTRING:
    z = mksql(EXDBL);
    z->u.val = stoJ(x->u.name[0]);
    rmsql(x);
    return z;
  case EXHEAD:
    return applylist(x,sql_dayno);
  case EXDATE:
    x->op = EXCONST;
  case EXNULL:
  case EXCONST: EXDBL:
    return x;
  default: ;
  }
  return delayFunc(x,sql_dayno);
}

/* convert string characters to upper case, concatenate multiple
   arguments */

static sql sql_case(sql x,sql (*f)(sql)) {
  sql z;
  switch (x->op) {
  const char *p;
  case EXNULL:
    return x;
  default:
    x = tostring(x);
    if (x->op != EXSTRING) break;
  case EXSTRING:
    p = x->u.name[0];
    if (f == sql_upper)
      do 
	obstack_1grow(sqltree,islower(*p) ? _toupper(*p) : *p);
      while (*p++);
    else
      do 
	obstack_1grow(sqltree,isupper(*p) ? _tolower(*p) : *p);
      while (*p++);
    rmsql(x);
    return mkstring((char *)obstack_finish(sqltree));
  case EXHEAD:
    return applylist(x,f);
  }
  return delayFunc(x,f);
}

sql sql_upper(sql x) {
  return sql_case(x,sql_upper);
}

sql sql_lower(sql x) {
  return sql_case(x,sql_lower);
}

/* convert constant expressions to logical value (EXBOOL)
 * if(cond,trueval,falseval)
 * if(cond,val,cond,val,elseval)
 */

sql sql_if(sql x) {
  sql z;
  switch (x->op) {
  case EXNULL:
    return x;
  case EXHEAD:
    for (;;) {
      sql cond,y,t;
      int val;
      z = x->u.opd[1];	/* list is not null */
      if (!z) {
	rmsql(x);
	return sql_nul;
      }
      y = z->u.opd[1];
      if (!y) {			/* last element */
	y = z->u.opd[0];
	rmsql(x);
	rmsql(z);
	return y;
      }
      cond = z->u.opd[0];	/* condition or else value */
      if (cond->op == EXNULL) {	/* condition unknown */
	rmexp(x);
	return sql_nul;
      }
      if (cond->op != EXBOOL) break;	/* delayed evaluation */
      val = cond->u.ival;
      t = y->u.opd[0];
      rmsql(cond);
      rmsql(z);		/* cond list element */
      z = y->u.opd[1];	/* false list element */
      rmsql(y);		/* true list element no longer needed */
      x->u.opd[1] = z;
      if (val) {
	rmexp(x);
	return t;
      }
      rmsql(t);
    }
  default: ;
  }
  return delayFunc(x,sql_if);
}

sql sql_substr(sql x) {
  sql z;
  switch (x->op) {
  case EXNULL:
    return x;
  case EXHEAD:
    if (z = x->u.opd[1]) {	/* list not null */
      sql y = tostring(z->u.opd[0]);	/* string */
      sql f,from,forl;
      int slen,len,pos;
      char *p;
      if (y->op != EXSTRING && y->op != EXNULL) {
	z->u.opd[0] = y;	/* delayed evaluation */
	break;
      }
      f = z->u.opd[1];		/* from list element */
      if (!f) {			/* no from, for */
	rmsql(x);
	rmsql(z);
	return y;
      }
      from = toconst(f->u.opd[0],0);
      if (y->op == EXNULL || from->op == EXNULL) {
	rmexp(x);
	return sql_nul;
      }
      if (from->op != EXCONST) {
	f->u.opd[0] = from;	/* delayed evaluation */
	break;
      }
      pos = (int)Mtol(&from->u.num.val) - 1;
      slen = strlen(y->u.name[0]);
      if (forl = f->u.opd[1]) {
	sql forval = toconst(forl->u.opd[0],0);
	if (forval->op == EXNULL) {
	  rmexp(x);
	  return sql_nul;
	}
	if (forval->op != EXCONST) {
	  forl->u.opd[0] = forval;	/* delayed evaluation */
	  break;
	}
	len = (int)Mtol(&forval->u.num.val);
	if (pos < 0)
	  len += pos, pos = 0;
      }
      else {
	if (pos < 0)
	  pos = 0;
	len = slen - pos;
      }
      /* pos >= 0 */
      if (pos + len > slen)
	len = slen - pos;
      if (len <= 0)
	p = "";
      else {
	p = (char *)obstack_alloc(sqltree,len + 1);
	memcpy(p,y->u.name[0]+pos,len);
	p[len] = 0;
      }
      return mkstring(p);
    }
    rmsql(x);
    return sql_nul;
  default: ;
  }
  return delayFunc(x,sql_substr);
}

static sql sql_elem(sql x,int elem) {
  if (x->op == EXHEAD)
    x = x->u.opd[1];
  if (x->op != EXLIST)
    return elem ? 0 : x;
  while (x && elem--)
    x = x->u.opd[1];
  return x ? x->u.opd[0] : 0;
}

static sql sql_fmt(sql x,sql (*f1)(sql),sql (*f2)(sql,sql)) {
  sql z;
  switch (x->op) {
  case EXNULL:
    return x;
  case EXHEAD:
    if (z = x->u.opd[1]) {	/* list not null */
      sql y = z->u.opd[0];
      sql f = z->u.opd[1], fmt;
      int fix = 0;
      /* conversion with no format argument */
      if (f == 0) {	
	y = tostring(y);
	if (y->op != EXSTRING && y->op != EXNULL) {
	  z->u.opd[0] = y;	/* delayed evaluation */
	  break;
	}
	rmsql(x);
	rmsql(z);
	return y;		/* conversion suceeded */
      }
      /* conversion with format argument */
      fmt = tostring(f->u.opd[0]);
      if (fmt->op == EXNULL) {
	rmsql(x); rmsql(z);
	return fmt;
      }
      if (fmt->op != EXSTRING) {
	f->u.opd[0] = fmt;
	break;			/* delayed evaluation */
      }
      f = (*f2)(y,fmt);
      if (f->op == EXNULL)
	f = mkstring("");
      else if (f->op != EXSTRING) {
	z->u.opd[0] = f;
	break;
      }
      rmsql(y);
      rmsql(fmt);
      return f;
    }
    rmsql(x);
    return sql_nul;
  default: ;
  }
  return delayFunc(x,f1);
}

static sql sql_fmtdate(sql y,sql fmt) {
  if (y->op == EXSTRING)
    y = sql_dayno(y);
  y = toconst(y,0);
  if (y->op == EXCONST) {
    int len = strlen(fmt->u.name[0]) + 16;
    char *p = (char *)obstack_alloc(sqltree,len);
    struct tm t;
    struct mmddyy mdy;
    julmdy(Mtol(&y->u.num.val),&mdy);
    t.tm_year = mdy.yy - 1900;
    t.tm_mon = mdy.mm - 1;
    t.tm_mday = mdy.dd;
    t.tm_hour = 0;
    t.tm_min = 0;
    t.tm_sec = 0;
    t.tm_isdst = 0;
    strftime(p,len,fmt->u.name[0],&t);
    return mkstring(p);
  }
  return y;
}

static sql sql_fdate(sql x) {
  return sql_fmt(x,sql_fdate,sql_fmtdate);
}

static sql sql_fmtpic(sql y,sql fmt) {
  char *p;
  int fix = 0, len;
  /* compute fix from format */
  {
    p = strrchr(fmt->u.name[0],'.');
    if (p != 0) while (*p) if (strchr("zZ#",*p++)) ++fix;
  }
  y = toconst(y,fix);
  if (y->op != EXCONST)
    return y;
  len = strlen(fmt->u.name[0]);
  p = (char *)obstack_alloc(sqltree,len + 1);
  pic(&y->u.num.val,fmt->u.name[0],p);
  p[len] = 0;
  return mkstring(p);
}

static sql sql_pic(sql x) {
  return sql_fmt(x,sql_pic,sql_fmtpic);
}

static sql sql_fmttime(sql y,sql fmt) {
  int len;
  long t;
  char *p;
  y = toconst(y,0);
  if (y->op != EXCONST)
    return y;
  t = Mtol(&y->u.num.val);
  if (!t) return mkstring("");
  len = strlen(fmt->u.name[0]); 
  p = (char *)obstack_alloc(sqltree,len + 1);
  timemask(t,fmt->u.name[0],p);
  p[len] = 0;
  return mkstring(p);
}

static sql sql_timemask(sql x) {
  return sql_fmt(x,sql_timemask,sql_fmttime);
}

static sql sql_isnull(sql x) {
  sql z;
  switch (x->op) {
  case EXNULL:
    return x;
  case EXHEAD:
    if (z = x->u.opd[1]) {	/* list not null */
      sql y = z->u.opd[0];
      sql n = z->u.opd[1];
      if (n == 0) {
	rmsql(x); rmsql(z);
	return y;
      }
      switch (y->op) {
      case EXNULL:
	y = n->u.opd[0];
	break;
      case EXCONST: case EXDBL: case EXBOOL: case EXSTRING: case EXDATE:
	rmexp(n->u.opd[0]);
	break;
      default:
	goto delay;
      }
      rmsql(n); rmsql(x); rmsql(z);
      return y;
    }
    rmsql(x);
    return sql_nul;
  default: ;
  }
delay:
  z = mksql(EXFUNC);
  z->u.f.arg = x;
  z->u.f.func = sql_isnull;
  return z;
}

static sql sql_length(sql x) {
  sql z;
  sconst m;
  switch (x->op) {
  case EXHEAD:
    return applylist(x,sql_length);
  case EXNULL:
    return x;
  default:
    x = tostring(x);
    if (x->op != EXSTRING) break;
  case EXSTRING:
    m.val = ltoM((long)strlen(x->u.name[0]));
    m.fix = 0;
    /* printf("length('%s') = %d\n",x->u.name[0],strlen(x->u.name[0])); */
    rmexp(x);
    return mkconst(&m);
  }
  z = mksql(EXFUNC);
  z->u.f.arg = x;
  z->u.f.func = sql_length;
  return z;
}

static sql sql_abs(sql x) {
  sql z;
  switch (x->op) {
  case EXHEAD:
    return applylist(x,sql_abs);
  case EXNULL:
  default:
    return x;
  case EXDATE:
    x->op = EXCONST;	// no longer a date
  case EXCONST:
    if (sgnM(&x->u.num.val) < 0)
      mulM(&x->u.num.val,-1,0);
    return x;
  case EXDBL:
    if (x->u.val < 0)
      x->u.val = - x->u.val;
    return x;
  }
  z = mksql(EXFUNC);
  z->u.f.arg = x;
  z->u.f.func = sql_abs;
  return z;
}

static sql sql_trim(sql x) {
  sql z;
  int len;
  char *p;
  switch (x->op) {
  case EXHEAD:
    return applylist(x,sql_trim);
  case EXNULL:
    return x;
  default:
    x = tostring(x);
    if (x->op != EXSTRING) break;
  case EXSTRING:
    p = (char *)x->u.name[0];
    len = strlen(p);
    while (p[len-1] == ' ') --len;
    p[len] = 0;
    return x;
  }
  z = mksql(EXFUNC);
  z->u.f.arg = x;
  z->u.f.func = sql_trim;
  return z;
}

static sql sql_agg(sql x,enum aggop op);

static sql sql_sum(sql x) {
  return sql_agg(x,AGG_SUM);
}

static sql sql_cnt(sql x) {
  return sql_agg(x,AGG_CNT);
}

static sql sql_max(sql x) {
  return sql_agg(x,AGG_MAX);
}

static sql sql_min(sql x) {
  return sql_agg(x,AGG_MIN);
}

static sql sql_prod(sql x) {
  return sql_agg(x,AGG_PROD);
}

static sql (*aggtbl[AGG_NONE])(sql) = {
  sql_sum, sql_cnt, sql_max, sql_min, sql_prod
};

static sql sql_agg(sql x,enum aggop op) {
  sql z;
  switch (x->op) {
  case EXHEAD:
    return applylist(x,aggtbl[op]);
  default: ;
  }
  z = mksql(EXAGG);
  z->u.g.op = op;
  z->u.g.exp = x;
  return z;
}

static sql sql_avg(sql x) {
  sql z;
  switch (x->op) {
  case EXHEAD:
    return applylist(x,sql_avg);
  default: ;
  }
  z = sql_eval(x,0);
  return mkbinop(sql_sum(x),EXDIV,sql_cnt(z));
}

static struct ftbl {
  const char *name;
  sql (*func)(/**/ sql /**/);
} functbl[] = {
  { "upper", sql_upper },
  { "lower", sql_lower },
  { "abs", sql_abs },
  { "if", sql_if },
  { "dayno", sql_dayno },
  { "pic", sql_pic },
  { "datefmt", sql_fdate },
  { "timemask", sql_timemask },
  { "isnull", sql_isnull },
  { "length", sql_length },
  { "substr", sql_substr },
  { "char_length", sql_length },	/* ANSI name */
  { "character_length", sql_length },	/* ANSI name */
  { "trim", sql_trim },
  { "sum", sql_sum },
  { "count", sql_cnt },
  { "cnt", sql_cnt },
  { "max", sql_max },
  { "min", sql_min },
  { "prod", sql_prod },
  { "avg", sql_avg },
  { 0, 0 }
};

sql mkfunc(const char *f,sql a) {
  struct ftbl *p;
  for (p = functbl; p->name && strcasecmp(p->name,f); ++p);
  if (p->name == 0) {
    char buf[80];
    sprintf(buf,"Function name '%s' not recognized.",f);
    yyerror(buf);
    return sql_nul;
  }
  return (*p->func)(a);
}

const char *nmfunc(sql (*func)(sql)) {
  struct ftbl *p;
  for (p = functbl; p->func && func != p->func; ++p);
  if (p->func == 0)
    return "?";
  return p->name; 
}

int func_width(sql x) {
  if (x->u.f.func == sql_substr) {
    /* return 'for' elem if constant, else sql_width of 'str' elem */
    sql y = sql_elem(x->u.f.arg,2);
    if (y && y->op == EXCONST)
      return (int)Mtol(&y->u.num.val);
    return sql_width(sql_elem(x->u.f.arg,0));
  }
  if (x->u.f.func == sql_timemask || x->u.f.func == sql_pic) {
    /* return width of mask */
    return sql_width(sql_elem(x->u.f.arg,1));
  }
  if (x->u.f.func == sql_length)
    return 5;
  return sql_width(x->u.f.arg);
}
