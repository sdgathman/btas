#ifndef SQL_H
#define SQL_H

#define MAXTABLE 100
#include "obmem.h"
#include <money.h>

#define DEF_SQL(x,y,z)	x,
enum sqlop {
#include "sql.def"
};
#undef DEF_SQL

/* scaled integer */
typedef struct {
  MONEY val;
  short fix;
} sconst;

typedef struct sqlnode *sql;
struct Column;

struct select {
  sql select_list;	/* SELECT list */
  sql table_list;	/* FROM list */
  sql where_sql;	/* WHERE exp */
  sql group_list;	/* GROUP BY list */
  sql having_sql;	/* HAVING exp */
  sql order_list;	/* ORDER BY list */
  int distinct;		/* DISTINCT */
};

enum aggop { AGG_SUM, AGG_CNT, AGG_MAX, AGG_MIN, AGG_PROD, AGG_NONE };

struct sqlnode {
  enum sqlop op;
  union {
    int ival;
    sql opd[2];
    double val;
    sconst num;
    const char *name[2];
    struct Column *col;	/* should be struct perminfo */
    struct {
      sql exp;
      const char *name;
    } a;
    struct { 
      sql arg;
      sql (*func)(sql);
    } f;
    struct {
      sql exp;
      enum aggop op;
    } g;
    struct {
      struct select *qry;
      const char *name;
    } q;
    struct {
      struct Cursor *qry;
      int lev;
    } s;
    struct {
      struct obstack *stk;
      sql freelist;
    } p;
    struct type_node {
      sql def;
      unsigned char type, len;
    } t;   
  } u;
};

enum sqlcmd { SQL_SELECT, SQL_INSERT, SQL_UPDATE, SQL_DELETE,
		SQL_USE, SQL_DROP, SQL_CREATE, SQL_INDEX, SQL_ERROR };

struct sql_stmt {
  enum sqlcmd cmd;
  struct select *src;
  struct select *dst;
  sql val;
};

/* parse.y */
struct sql_stmt *parse_sql(const char *);
	/* report syntax error */
/* sqlexec.c */
void sqlexec(const struct sql_stmt *cmd,const char *formatch,int verbose);
void sql_syntax(const char *,const char *,const char *);
extern int yydebug;
/* func.c */
sql sql_upper(sql);
sql sql_lower(sql);
int func_width(sql);
/* like.c */
extern int like(const char *, const char *);
/* sql.c */
sql mksql(enum sqlop);
void rmsql(sql), rmexp(/**/ sql /**/);
sql mkfunc(const char *, sql);
const char *nmfunc(sql (*)(sql));
sql mkbinop(sql, enum sqlop, sql);
sql mklist(void);
void addlist(sql, sql);
int cntlist(sql);
sql rmlast(sql);	/* remove last element from list */
sql mkalias(const char *, sql);
sql mkunop(enum sqlop, sql);
sql mkname(const char *, const char *);
sql mkconst(const sconst *);
sql mkbool(int);
#define istrue(x) (x->op == EXBOOL && x->u.ival)
#define isfalse(x) (x->op == EXBOOL && !x->u.ival)
sql mktype(const char *,const char *,int,int);
sql mkstring(const char *);
sql mklit(const char *,sql);
sql tostring(sql);
sql toconst(sql,int);
int getconst(sconst *p,const char **lexptr);
const char *dump_num(const sconst *n);
sql free_sql(struct obstack *);
void pop_sql(sql);
sql sql_eval(sql, int);
sql sql_if(sql);
sql sql_substr(sql);
int sql_width(sql);
struct btfrec;
void sql_frec(sql, struct btfrec *);
int sql_same(sql,sql);
void sql_print(sql, int);
double const2dbl(const sconst *num);
extern struct sqlform { const char *name, *fmt, *op; } sql_form[];
extern struct obstack *sqltree;
void yyerror(struct sql_stmt *,const char *);
extern int debug, mapupper;
extern sql sql_nul;
#endif
