/*	parse sql quoted literals
	Copyright 1993 Business Management Systems, Inc.
	Author: Stuart D. Gathman
*/

#include <assert.h>
#include <string.h>
#include <ctype.h>
#include <date.h>
#include "sql.h"

static sql basenum(const char *p,int base) {
  sconst d;
  int c;
  d.val = zeroM;
  d.fix = 0;
  while (c = *p++) {
    if (isalnum(c)) {
      if (islower(c))
	c = _toupper(c);
      if (isdigit(c))
	mulM(&d.val,base,c - '0');
      else
	mulM(&d.val,base,c - 'A' + 10);
    }
  }
  return mkconst(&d);
}

sql mklit(const char *s,sql x) {
  sql z = sql_nul;
  assert(x->op == EXSTRING);
  if (strcasecmp(s,"J") == 0) {		/* Series/1 style julian date */
    sconst d;
    d.val = ltoM(stoJ(x->u.name[0]));
    d.fix = 0;
    z = mkconst(&d);
    z->op = EXDATE;
  }
  else if (strcasecmp(s,"X") == 0) {	/* hex number */
    z = basenum(x->u.name[0],16);
  }
  else if (strcasecmp(s,"O") == 0) {
    z = basenum(x->u.name[0],8);
  }
  else if (strcasecmp(s,"B") == 0) {
    z = basenum(x->u.name[0],2);
  }
  else if (strnicmp(s,"CL",2) == 0) {
    int len = atoi(s+2);
    char *p = obstack_alloc(sqltree,len+1);
    stchar(x->u.name[0],p,len);
    p[len] = 0;
    z = mkstring(p);
  }
  else if (strcasecmp(s,"DATE") == 0) {	/* SQL2 style date */
    sconst d;
    char buf[16], *p;
    struct mmddyy mdy;
    strncpy(buf,x->u.name[0],10)[10] = 0;
    mdy.yy = mdy.mm = mdy.dd = 0;
    if (p = strtok(buf,"-/")) {
      mdy.yy = atoi(p);
      if (p = strtok(0,"-/")) {
	mdy.mm = atoi(p);
	if (p = strtok(0,"-/"))
	  mdy.dd = atoi(p);
      }
    }
    d.val = ltoM(julian(&mdy));
    d.fix = 0;
    z = mkconst(&d);
    z->op = EXDATE;
  }
  else if (strcasecmp(s,"TIMESTAMP") == 0) {
    sconst d;
    char buf[128];
    char *date, *time, *p;
    struct tm t;
    static const char datesep[] = "-/";
    strncpy(buf,x->u.name[0],127)[127] = 0;
    date = strtok(buf," \t");
    time = strtok(0," \t");
    if (p = strtok(date,datesep)) {
      t.tm_year = atoi(p) - 1900;
      if (p = strtok(0,datesep)) {
	t.tm_mon = atoi(p) - 1;
	if (p = strtok(0,datesep)) {
	  t.tm_mday = atoi(p);
	}
      }
    }
    t.tm_hour = 0;
    t.tm_min = 0;
    t.tm_sec = 0;
    if (p = strtok(time,":")) {
      t.tm_hour = atoi(p);
      if (p = strtok(0,":")) {
	t.tm_min = atoi(p);
	if (p = strtok(0,":")) {
	  t.tm_sec = atoi(p);
	}
      }
    }
    d.val = ltoM(mktime(&t));
    d.fix = 0;
    z = mkconst(&d);
  }
  rmsql(x);
  return z;
}
