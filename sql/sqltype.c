#include <stdio.h>
#include <btflds.h>
#include "sql.h"

enum sql_type {
  SQL_ERRTYPE, SQL_CHARACTER, SQL_VARCHAR, SQL_BIT, SQL_VARBIT,
  SQL_NUMERIC, SQL_DECIMAL, SQL_INTEGER, SQL_SMALLINT, SQL_FLOAT,
  SQL_DOUBLE, SQL_TIMESTAMP, SQL_DATE
};

static enum sql_type parse_type(const char *s1,const char *s2) {
  if (strcasecmp(s1,"CHARACTER") == 0 || strcasecmp(s1,"CHAR") == 0) {
    if (s2 == 0) return SQL_CHARACTER;
    if (strcasecmp(s2,"VARYING") == 0) return SQL_VARCHAR;
    return SQL_ERRTYPE;
  }
  if (strcasecmp(s1,"VARCHAR") == 0) {
    if (s2 == 0) return SQL_VARCHAR;
    return SQL_ERRTYPE;
  }
  if (strcasecmp(s1,"BIT") == 0) {
    if (s2 == 0) return SQL_BIT;
    if (strcasecmp(s2,"VARYING") == 0) return SQL_VARBIT;
    return SQL_ERRTYPE;
  }
  if (strcasecmp(s1,"NUMERIC") == 0 && s2 == 0) return SQL_NUMERIC;
  if (strcasecmp(s1,"DECIMAL") == 0 && s2 == 0) return SQL_DECIMAL;
  if (strcasecmp(s1,"DEC") == 0 && s2 == 0) return SQL_DECIMAL;
  if (strcasecmp(s1,"INTEGER") == 0 && s2 == 0) return SQL_INTEGER;
  if (strcasecmp(s1,"INT") == 0 && s2 == 0) return SQL_INTEGER;
  if (strcasecmp(s1,"SMALLINT") == 0 && s2 == 0) return SQL_SMALLINT;
  if (strcasecmp(s1,"FLOAT") == 0 && s2 == 0) return SQL_FLOAT;
  if (strcasecmp(s1,"REAL") == 0 && s2 == 0) return SQL_FLOAT;
  if (strcasecmp(s1,"DOUBLE") == 0 && strcasecmp(s2,"PRECISION") == 0)
    return SQL_DOUBLE;
  if (strcasecmp(s1,"DATE") == 0 && s2 == 0) return SQL_DATE;
  if (strcasecmp(s1,"TIMESTAMP") == 0 && s2 == 0) return SQL_TIMESTAMP;
  return SQL_ERRTYPE;
}

static const char lentbl[] = {
  1, 1, 1, 2, 2, 3, 3, 4, 4, 4, 5, 5
};

static int
sqltypefrec(enum sql_type type, int len,int prec,struct type_node *f) {
  switch (type) {
  case SQL_ERRTYPE: return -1;
  case SQL_NUMERIC: case SQL_DECIMAL:
    f->type = BT_NUM + prec;
    if (len >= sizeof lentbl)
      f->len = 6;
    else
      f->len = lentbl[len];
    break;
  case SQL_BIT: case SQL_VARBIT:
    f->type = BT_BIN;
    f->len = (len + 7) / 8;
    break;
  case SQL_CHARACTER: case SQL_VARCHAR:
    f->type = BT_CHAR;
    f->len = len;
    break;
  case SQL_INTEGER:
    f->type = BT_NUM;
    f->len = 4;
    break;
  case SQL_SMALLINT:
    f->type = BT_NUM;
    f->len = 2;
    break;
  case SQL_FLOAT:
    f->type = BT_FLT;
    f->len = 4;
    break;
  case SQL_DOUBLE:
    f->type = BT_FLT;
    f->len = 8;
    break;
  case SQL_DATE:
    f->type = BT_DATE;
    f->len = 4;
    break;
  case SQL_TIMESTAMP:
    f->type = BT_TIME;
    f->len = 4;
    break;
  }
  return 0;
}

sql mktype(const char *s1,const char *s2,int len,int prec) {
  sql x = mksql(EXTYPE);
  enum sql_type t = parse_type(s1,s2);
  x->u.t.def = sql_nul;
  if (sqltypefrec(parse_type(s1,s2),len,prec,&x->u.t) || debug) {
    if (s2 == 0) s2 = "";
    fprintf(stderr,"sqltype %s %s [%d] (%d,%d) -> %02x,%d\n",
      s1,s2,t,len,prec,x->u.t.type,x->u.t.len);
  }
  return x;
}
