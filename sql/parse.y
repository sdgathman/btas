/*
 * $Log$
 * Revision 1.7  2011/07/01 21:45:16  stuart
 * Use bison and allocate sql_stmt per parse.
 * Implement PRIMARY KEY for CREATE TABLE.
 *
 * Revision 1.6  2011/04/26 18:14:45  stuart
 * Move sqlexec to its own file.
 *
 * Revision 1.5  2007/09/26 20:38:36  stuart
 * Implemented COALESCE
 *
 * Revision 1.4  2007/09/26 20:21:41  stuart
 * Implement NULLIF
 *
 * Revision 1.3  2001/11/14 23:07:35  stuart
 * Support CREATE TABLE, but ignores field names for now.
 *
 * Revision 1.2  2001/11/14 19:16:46  stuart
 * Implement INSERT INTO ... VALUES
 * Fix assignment bug.
 *
 * Revision 1.1  2001/02/28 23:00:05  stuart
 * Old C version of sql recovered as best we can.
 *
 * Revision 1.5.1.1  1993/05/24  13:59:06  stuart
 * allow subqueries in DELETE,UPDATE - make them read-only
 *
 */
%{
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "sql.h"

static int yyparse(struct sql_stmt *);
static int yylex(void);
static char *quotes(int);
static struct select *select_init(void);
static struct sql_stmt s;	/* result of parsing */
#define YYDEBUG 0
#if !YYDEBUG
int yydebug;
#endif
%}

%union {
  double val;
  sconst num;
  const char *name;
  struct select *qry;
  sql exp;
}

%token	<name> IDENT STRING
%token	<num>  CONST
%token	IS NUL SELECT ORDER BY WHERE FROM DISTINCT GROUP ERROR DESC ASC OUTER
%token	ALL ANY CONNECT INSERT VALUES INTO AS UPDATE SET DELETE ALTER MODIFY
%token	ADD UNIQUE INDEX ON DROP RENAME TO PRIOR TABLE VIEW START WITH CLUSTER
%token	UNION INTERSECT MINUS CREATE HAVING USE JOIN TABLE CROSS PRIMARY KEY
%token	CASE WHEN THEN ELSE END SUBSTRING FOR EXISTS NULLIF COALESCE
%left	<exp> OR
%left	<exp> AND
%right	NOT
%nonassoc <exp> '=' '>' '<' NE LE GE BETWEEN IN LIKE
%left	'+' '-'
%left	'*' '/'
%left	CONCAT
%nonassoc '.'
%type	<exp> logical expr exprlist file fldlist identlist arglist arg table
%type	<exp> alist alias order olist ident assignlist assign type typelist
%type	<exp> collist atom atomlist rowval rowlist whenlist whenxlist string
%type	<qry> filelist join where joinlist tabexp joinexp
%type	<qry> nonjoin values
%parse-param { struct sql_stmt *s }

%%

start	: tabexp ';'
		{ s->cmd = SQL_SELECT;
		  s->src = $1; }
	| tabexp ORDER BY olist ';'
		{ s->cmd = SQL_SELECT;
		  s->src = $1;
		  $1->order_list = $4; }
	| DELETE FROM joinlist WHERE logical ';'
		{ s->cmd = SQL_DELETE;
		  s->dst = $3;
		  s->dst->where_sql = $5; }
	| DELETE FROM joinlist ';'
		{ s->cmd = SQL_DELETE;
		  s->dst = $3; }
	| INSERT INTO filelist tabexp ';'
		{ s->cmd = SQL_INSERT;
		  s->dst = $3;
		  s->src = $4; }
	| INSERT INTO filelist '(' identlist ')' tabexp ';'
		{ s->cmd = SQL_INSERT;
		  s->dst = $3;
		  s->dst->select_list = $5;
		  s->src = $7; }
	| UPDATE joinlist SET assignlist ';'
		{ s->cmd = SQL_UPDATE;
		  s->dst = $2;
		  s->dst->select_list = $4; }
	| UPDATE joinlist SET assignlist WHERE logical ';'
		{ s->cmd = SQL_UPDATE;
		  s->dst = $2;
		  s->dst->where_sql = $6;
		  s->dst->select_list = $4; }
	| CREATE TABLE IDENT '(' typelist ')' ';'
		{ s->cmd = SQL_CREATE;
		  s->dst = select_init();
		  s->dst->table_list = mkname($3,$3);
		  s->val = $5;
		  assert(s->val->op == EXHEAD);
		  assert(s->val->u.opd[1]->op == EXLIST);
		  assert(s->val->u.opd[1]->u.opd[0]->op == EXTYPE);
		}
	| CREATE INDEX IDENT ON IDENT '(' identlist ')' ';'
		{ s->cmd = SQL_INDEX;
		  s->dst = select_init();
		  s->dst->table_list = mkname($3,$3);
		  s->src = select_init();
		  s->src->table_list = mkname($5,$5);
		  s->val = $7;
		}
	| CREATE UNIQUE INDEX IDENT ON IDENT '(' identlist ')' ';'
		{ s->cmd = SQL_INDEX;
		  s->dst = select_init();
		  s->dst->table_list = mkname($4,$4);
		  s->dst->distinct = 1;
		  s->src = select_init();
		  s->src->table_list = mkname($6,$6);
		  s->val = $8;
		}
	| USE IDENT ';'
		{ s->cmd = SQL_USE;
		  s->val = mkname($2,$2);
		}
	| DROP TABLE filelist ';'
		{ s->cmd = SQL_DROP;
		  s->dst = $3; }
	| alist ';'
		{ s->cmd = SQL_SELECT;
		  s->src = select_init();
		  addlist(s->src->table_list = mklist(),
			mkname("system","system"));
		  s->src->select_list = $1; }
	| error
		{ s->cmd = SQL_ERROR; }
	;

values	: VALUES rowlist
		{ $$ = select_init();
		  $$->select_list = $2; }
	;

rowlist	: rowval
		{ addlist($$ = mklist(),$1); }
	| rowlist ',' rowval
		{ addlist($$,$3); }
	;

rowval	: '(' atomlist ')'
		{ $$ = $2; }
	| '(' tabexp ')'
		{ $$ = mksql(EXQUERY);
		  $$->u.q.qry = $2;
		  $$->u.q.name = 0;
		}
	;

atomlist: atom
		{ addlist($$ = mklist(),$1); }
	| atomlist ',' atom
		{ addlist($$,$3); }
	;

order	: expr
	| expr ASC
		{ $$ = $1; }
	| expr DESC
		{ $$ = mkunop(EXDESC,$1); }
	;

olist	: order
		{ addlist($$ = mklist(),$1); }
	| olist ',' order
		{ addlist($$ = $1,$3); }
	;

alist	: alias
		{ addlist($$ = mklist(),$1); }
	| alist ',' alias
		{ addlist($$ = $1,$3); }
	;

alias	: expr
	| expr maybeAS IDENT
		{ $$ = mkalias($3,$1); }
	;

fldlist	: alist
	| '*'
		{ $$ = 0; }
	;

typelist: IDENT type
		{ addlist($$ = mklist(),$2); assert($2->op == EXTYPE);
		  $2->u.t.def = mkname($1,$1); }
	| IDENT type NOT NUL
		{ addlist($$ = mklist(),$2); $2->u.t.def = mkname($1,$1); }
	| typelist ',' IDENT type
		{ addlist($$ = $1,$4); $4->u.t.def = mkname($3,$3); }
	| typelist ',' IDENT type NOT NUL
		{ addlist($$ = $1,$4); $4->u.t.def = mkname($3,$3); }
	| typelist ',' UNIQUE '(' identlist ')'
		{ addlist($$ = $1,$5); }
	| typelist ',' PRIMARY KEY '(' identlist ')'
		{ addlist($$ = $1,$6); }
	;

type	: IDENT IDENT
		{ $$ = mktype($1,$2,0,0); }
	| IDENT
		{ $$ = mktype($1,0,0,0); }
	| IDENT '(' CONST ')'
		{ if ($3.fix) YYERROR;
		  $$ = mktype($1,0,(int)Mtol(&$3.val),0); }
	| IDENT IDENT '(' CONST ')'
		{ if ($4.fix) YYERROR;
		  $$ = mktype($1,$2,(int)Mtol(&$4.val),0); }
	| IDENT '(' CONST ',' CONST ')'
		{ if ($3.fix || $5.fix) YYERROR;
		  $$ = mktype($1,0,(int)Mtol(&$3.val),(int)Mtol(&$5.val)); }
	;

join	: SELECT fldlist FROM joinlist
		{ $$ = $4;
		  $$->select_list = $2; }
	| SELECT DISTINCT fldlist FROM joinlist
		{ $$ = $5;
		  $$->select_list = $3;
		  $$->distinct = 1; }
	;

where	: join
	| join WHERE logical
		{ $$ = $1; $$->where_sql = $3; }
	;

joinlist: table
		{ $$ = select_init();
		  addlist($$->table_list = mklist(),$1); }
	| joinlist ',' table
		{ $$ = $1;
		  addlist($$->table_list,$3); }
	;

filelist: file
		{ $$ = select_init();
		  addlist($$->table_list = mklist(),$1); }
	| filelist ',' file
		{ $$ = $1;
		  addlist($$->table_list,$3); }
	;

file	: IDENT
		{ $$ = mkname($1,$1); }
	| IDENT AS IDENT
		{ $$ = mkname($1,$3); }
	| IDENT IDENT
		{ $$ = mkname($1,$2); }
	;

table	: file
	| OUTER file
		{ $$ = mkunop(EXISNUL,$2); }
	| '(' tabexp ')' maybeAS IDENT collist
		{ $$ = mksql(EXQUERY);
		  $$->u.q.qry = $2;
		  $$->u.q.name = $5;
		  if ($6)
		    $$ = mkbinop($$,EXEQ,$6);	/* assign new column names */
		}
	| joinexp
		{ $$ = mksql(EXQUERY);
		  $$->u.q.qry = $1;
		  $$->u.q.name = 0;
		}
	;

collist	: /* empty */
		{ $$ = 0; }
	| '(' identlist ')'
		{ $$ = $2; }
	;

tabexp	: joinexp
	| nonjoin
	;

nonjoin	: TABLE IDENT
		{ $$ = select_init();
		  addlist($$->table_list = mklist(),mkname($2,$2)); }
	| values
	| where
	| where GROUP BY exprlist
		{ $$ = $1; $$->group_list = $4; }
	| where GROUP BY exprlist HAVING logical
		{ $$ = $1; $$->group_list = $4; $$->having_sql = $6; }
	| '(' nonjoin ')'
		{ $$ = $2; }
	;

joinexp	: table JOIN table ON logical
		{ $$ = select_init();
		  $$->table_list = mklist();
		  addlist($$->table_list,$1);
		  addlist($$->table_list,$3);
		  $$->where_sql = $5;
		}
	| table CROSS JOIN table
		{ $$ = select_init();
		  $$->table_list = mklist();
		  addlist($$->table_list,$1);
		  addlist($$->table_list,$4);
		}
	| '(' joinexp ')'
		{ $$ = $2; }
	;

assignlist	: assign
			{ addlist($$ = mklist(),$1); }
		| assignlist ',' assign
			{ addlist($$ = $1,$3); }
		;

assign	: ident '=' expr
		{ $$ = mkbinop($1,EXASSIGN,$3); }
	;

identlist	: ident
			{ addlist($$ = mklist(),$1); }
		| identlist ',' ident
			{ addlist($$ = $1,$3); }
		;

maybeis	: /* empty */
	| IS
	;

maybeAS : /* empty */
	| AS
	;

logical	: expr '=' expr
		{ $$ = mkbinop($1,EXEQ,$3); }
	| expr '<' expr
		{ $$ = mkbinop($1,EXLT,$3); }
	| expr '>' expr
		{ $$ = mkbinop($1,EXGT,$3); }
	| expr LE expr
		{ $$ = mkbinop($1,EXLE,$3); }
	| expr GE expr
		{ $$ = mkbinop($1,EXGE,$3); }
	| expr NE expr
		{ $$ = mkbinop($1,EXNE,$3); }
	| expr maybeis BETWEEN expr AND expr
		{ $$ = mkbinop(mkbinop(sql_eval($1,0),EXGE,$4),EXAND,
			 mkbinop($6,EXGE,sql_eval($1,0))); }
	| expr maybeis NOT BETWEEN expr AND expr
		{ $$ = mkbinop(mkbinop($5,EXGT,sql_eval($1,0)),EXOR,
			 mkbinop(sql_eval($1,0),EXGT,$7)); }
	| expr maybeis IN '(' exprlist ')'
		{
		  sql x = $5->u.opd[1];
		  $$ = mkbinop(sql_eval($1,0),EXEQ,x->u.opd[0]);
		  while (x = x->u.opd[1])
		    $$ = mkbinop($$,EXOR,
			mkbinop(sql_eval($1,0),EXEQ,x->u.opd[0]));
		}
	| expr maybeis IN '(' tabexp ')'
		{ sql q = mksql(EXQUERY), c = mklist();
		  q->u.q.qry = $5; q->u.q.name = "t";
		  $$ = q;
		  q = mksql(EXQUERY);
		  q->u.q.qry = select_init();
		  c = mklist();
		  addlist(c,$$);
		  q->u.q.qry->table_list = c;
		  c = mkname((const char *)0,"#1");
		  q->u.q.qry->where_sql = mkbinop(c,EXEQ,$1);
		  // FIXME: need operator to search query, EXISTS is wrong
		  $$ = mkunop(EXEXIST,q); }
	| expr maybeis NOT IN '(' exprlist ')'
		{
		  sql x = $6->u.opd[1];
		  $$ = mkbinop(sql_eval($1,0),EXNE,x->u.opd[0]);
		  while (x = x->u.opd[1])
		    $$ = mkbinop($$,EXAND,
			mkbinop(sql_eval($1,0),EXNE,x->u.opd[0]));
		}
	| expr maybeis LIKE STRING
		{ $$ = mkbinop($1,EXLIKE,mkstring($4)); }
	| expr maybeis NOT LIKE STRING
		{ $$ = mkunop(EXNOT,mkbinop($1,EXLIKE,mkstring($5))); }
	| expr IS NUL
		{ $$ = mkunop(EXISNUL,$1); }
	| expr IS NOT NUL
		{ $$ = mkunop(EXNOT,mkunop(EXISNUL,$1)); }
	| '(' logical ')'
		{ $$ = $2; }
	| logical AND logical
		{ $$ = mkbinop($1,EXAND,$3); }
	| logical OR logical
		{ $$ = mkbinop($1,EXOR,$3); }
	| NOT logical
		{ $$ = mkunop(EXNOT,$2); }
	| EXISTS '(' tabexp ')'
		{ sql q = mksql(EXQUERY);
		  q->u.q.qry = $3; q->u.q.name = 0;
		  $$ = mkunop(EXEXIST,q); }
	| UNIQUE '(' tabexp ')'
		{ sql q = mksql(EXQUERY);
		  q->u.q.qry = $3; q->u.q.name = 0;
		  $$ = mkunop(EXUNIQ,q); }
	;

exprlist: expr
		{ addlist($$ = mklist(),$1); }
	| exprlist ',' expr
		{ addlist($$ = $1,$3); }
	;

arg	: expr
	| logical
	;

arglist	: arg
		{ addlist($$ = mklist(),$1); }
	| arglist ',' arg
		{ addlist($$ = $1,$3); }
	;

ident	: IDENT
		{ $$ = mkname((const char *)0,$1); }
	| IDENT '.' IDENT
		{ $$ = mkname($1,$3); }
	| IDENT '.' '*'
		{ $$ = mkname($1,(const char *)0); }
	;

string	: STRING
		{ $$ = mkstring($1); }
	| string STRING
		{ $$ = mkbinop($1,EXCAT,mkstring($2)); }
	;

atom	: CONST
		{ $$ = mkconst(&$1); }
	| '-' CONST
		{ $$ = mkunop(EXNEG,mkconst(&$2)); }
	| string
		{ $$ = $1; }
	| IDENT string
		{ $$ = mklit($1,$2); }
	;

whenlist: WHEN logical THEN expr
		{ addlist($$ = mklist(),$2); addlist($$,$4); }
	| whenlist WHEN logical THEN expr
		{ addlist($$ = $1,$3); addlist($$,$5); }
	;

whenxlist: WHEN expr THEN expr
		{ addlist($$ = mklist(),$2); addlist($$,$4); }
	| whenxlist WHEN expr THEN expr
		{ addlist($$ = $1,$3); addlist($$,$5); }
	;

expr	: atom
	| IDENT '(' arglist ')'
		{ $$ = mkfunc($1,$3); }
	| ident
		{ $$ = $1; }
	| CASE whenlist ELSE expr END
		{ addlist($2,$4);
		  $$ = sql_if($2); }
	| CASE expr whenxlist ELSE expr END
		{ sql x = $3;
		  while (x = x->u.opd[1]) {
		    x->u.opd[0] = mkbinop(sql_eval($2,0),EXEQ,x->u.opd[0]);
		    x = x->u.opd[1];
		  }
		  addlist($3,$5);
		  $$ = sql_if($3); }
	| NULLIF '(' expr ',' expr ')'
		{ sql a = mklist();
		  addlist(a,mkbinop(sql_eval($3,0),EXEQ,$5));
		  addlist(a,sql_nul);
		  addlist(a,$3);
		  $$ = sql_if(a); }
	| COALESCE '(' expr ',' expr ')'
		{ sql a = mklist();
		  addlist(a, mkunop(EXNOT,mkunop(EXISNUL,sql_eval($3,0))));
		  addlist(a, $3);
		  addlist(a, $5);
		  $$ = sql_if(a); }
	| SUBSTRING '(' expr FROM expr FOR expr ')'
		{ addlist($$ = mklist(),$3); addlist($$,$5); addlist($$,$7);
		  $$ = sql_substr($$); }
	| SUBSTRING '(' expr FOR expr ')'
		{ static sconst one = { { 0, 1 } };
		  addlist($$ = mklist(),$3); addlist($$,mkconst(&one));
		  addlist($$,$5); $$ = sql_substr($$); }
	| SUBSTRING '(' expr FROM expr ')'
		{ addlist($$ = mklist(),$3); addlist($$,$5);
		  $$ = sql_substr($$); }
	/* | NUL */
		/* { $$ = sql_nul; } */
	| '(' tabexp ')'
		{ $$ = mksql(EXQUERY); $$->u.q.qry = $2; $$->u.q.name = 0; }
	| '(' expr ')'
		{ $$ = $2; }
	| '(' exprlist ')'
		{ $$ = $2; }
	| '-' expr %prec '*'
		{ $$ = mkunop(EXNEG,$2); }
	| expr CONCAT expr
		{ $$ = mkbinop($1,EXCAT,$3); }
	| expr '*' expr
		{ $$ = mkbinop($1,EXMUL,$3); }
	| expr '/' expr
		{ $$ = mkbinop($1,EXDIV,$3); }
	| expr '+' expr
		{ $$ = mkbinop($1,EXADD,$3); }
	| expr '-' expr
		{ $$ = mkbinop($1,EXSUB,$3); }
	;
%%
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "obmem.h"

static const char *lexptr, *expptr;

static char *quotes(q)
  int q;	/* quote char */
{
  while (*lexptr != (char)q || *++lexptr == (char)q)
    obstack_1grow(sqltree,*lexptr++);
  obstack_1grow(sqltree,0);
  return obstack_finish(sqltree);
}

static int yylex() {
  /* table of reserved words, must be sorted alphabetically */
  static const struct resword {
    char name[10];
    short rid;
  } rtbl[] = {
    { "ADD", ADD }, { "ALL", ALL }, { "ALTER", ALTER }, { "AND", AND },
    { "ANY", ANY }, { "AS", AS }, { "ASC", ASC },
    { "BETWEEN", BETWEEN }, { "BY", BY }, { "CASE", CASE },
    { "CLUSTER", CLUSTER }, { "COALESCE", COALESCE }, { "CONNECT", CONNECT },
    { "CREATE", CREATE }, { "CROSS", CROSS },
    { "DELETE", DELETE }, { "DESC", DESC }, { "DISTINCT", DISTINCT },
    { "DROP", DROP }, { "ELSE", ELSE }, { "END", END }, { "EXISTS", EXISTS },
    { "FOR", FOR }, { "FROM", FROM }, { "GROUP", GROUP },
    { "HAVING", HAVING }, { "IN", IN }, { "INDEX", INDEX },
    { "INSERT", INSERT }, {"INTERSECT", INTERSECT },
    { "INTO", INTO }, { "IS", IS }, { "JOIN", JOIN }, { "KEY", KEY },
    { "LIKE", LIKE }, { "MINUS", MINUS }, { "MODIFY", MODIFY },
    { "NOT", NOT }, { "NULL", NUL }, { "NULLIF", NULLIF },
    { "ON", ON }, { "OR", OR }, { "ORDER", ORDER }, { "OUTER", OUTER },
    { "PRIMARY", PRIMARY }, { "PRIOR", PRIOR }, { "RENAME", RENAME },
    { "SELECT", SELECT }, { "SET", SET }, { "START", START },
    { "SUBSTRING", SUBSTRING },
    { "TABLE", TABLE }, { "THEN", THEN }, { "TO", TO },
    { "UNION", UNION }, { "UNIQUE", UNIQUE }, { "UPDATE", UPDATE },
    { "USE", USE }, { "VALUES", VALUES }, { "VIEW", VIEW },
    { "WHEN", WHEN }, { "WHERE", WHERE }, { "WITH", WITH },
  };
  static int thistok;
  int lasttok = thistok;
  thistok = 0;
doline:
  while (*lexptr && strchr(" \t\n",*lexptr)) ++lexptr;
  switch (*lexptr) {
  case 0:
    return 0;
  case '"':
    yylval.name = quotes(*lexptr++);
    return IDENT;
  case '\'':
    yylval.name = quotes(*lexptr++);
    return STRING;
  case '/':
    if (lexptr[1] == '/') {
      while (*++lexptr && *lexptr != '\n');
      goto doline;
    }
  case '*': case '+': case '-': case '=':
  case '(': case ')': case ',': case ';':
    return *lexptr++;
  case '.':
    return getconst(&yylval.num,&lexptr) ? CONST : (thistok = '.');
  case '>':
    return (*++lexptr == '=') ? ++lexptr,GE : '>';
  case '<':
    if (*++lexptr == '>') return ++lexptr,NE;
    return (*lexptr == '=') ? ++lexptr,LE : '<';
  case '!':
    return (*++lexptr == '=') ? ++lexptr,NE : ERROR;
  case '^':
    return (*++lexptr == '=') ? ++lexptr,NE : ERROR;
  case '|':
    return (*++lexptr == '|') ? ++lexptr,CONCAT : ERROR;
  }
  if (isalpha(*lexptr) || *lexptr == '#') {
    struct resword *p;
    char *name;
    if (*lexptr == '#') {
      do
	obstack_1grow(sqltree,*lexptr++);
      while (isdigit(*lexptr));
      obstack_1grow(sqltree,0);
      yylval.name = obstack_finish(sqltree);
      return IDENT;
    }
    do
      obstack_1grow(sqltree,toupper(*lexptr++));
    while (isalnum(*lexptr) || *lexptr == '#' || *lexptr == '_');
    obstack_1grow(sqltree,0);
    name = obstack_finish(sqltree);
    if (lasttok != '.') {
      p = bsearch(name,
	    rtbl,sizeof rtbl/sizeof rtbl[0],sizeof rtbl[0],
	    (int cdecl (*)())strcmp);
      if (p) {
	obstack_free(sqltree,name);
	return p->rid;
      }
    }
    if (!mapupper) strlwr(name);
    yylval.name = name;
    return IDENT;
  }
  return (getconst(&yylval.num,&lexptr)) ? CONST : ERROR;
}

void yyerror(struct sql_stmt *s,const char *msg) {
  sql_syntax(msg,expptr,lexptr);
}

static struct select *select_init() {
  struct select *s = obstack_alloc(sqltree,sizeof *s);
  s->select_list = 0;
  s->group_list = 0;
  s->having_sql = 0;
  s->order_list = 0;
  s->where_sql = 0;
  s->table_list = 0;
  s->distinct = 0;
  return s;
}

struct sql_stmt *parse_sql(const char *txt) {
  expptr = lexptr = txt;
  struct sql_stmt *s = obstack_alloc(sqltree,sizeof *s);
  s->src = 0;
  s->dst = 0;
  s->val = 0;
  if (yyparse(s)) return 0;
  return s;
}
