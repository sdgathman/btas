#include <stdio.h>
#include <time.h>
#include <date.h>
#include <money.h>
#include <ftype.h>
#include <btflds.h>
#include <string.h>
#include <malloc.h>
#include "config.h"
#include "column.h"
#include "coldate.h"
#include "obmem.h"
#include "object.h"

#define UNIX  25203  /* days after DAYCENT of unix origin */
#define SECDAY  86400  /* seconds per day */

static void Date_print(Column *,enum Column_type, char *);
static int Date_store(Column *,sql,char *);
static sql Date_load(Column *);

static void Julian_print(Column *,enum Column_type, char *);
static int Julian_store(Column *,sql,char *);
static sql Julian_load(Column *);

static void Time_print(Column *,enum Column_type, char *);
static int Time_store(Column *,sql,char *);
static sql Time_load(Column *);

Column *Date_init(Column *this,char *buf) {
  static struct Column_class Date_class = {
    sizeof *this, Column_free, Date_print, Date_store, Date_load,
    Column_copy, Column_dup
  };
  if (this || (this = malloc(sizeof *this))) {
    Column_init(this,buf,2);
    this->_class = &Date_class;
    this->width = this->dlen = 8;
    this->type = BT_DATE;
  }
  return (Column *)this;
}

static int Date_store(Column *this,sql x,char *buf) {
  double val;
  switch (x->op) {
  case EXCONST:
    if (sgnM(&x->u.num.val) == 0) {
      stD(zeroD,buf);
      return 0;
    }
    val = const2dbl(&x->u.num);
    break;
  case EXDBL:
    if (x->u.val == 0) {
      stD(zeroD,buf);
      return 0;
    }
    val = x->u.val;
    break;
  case EXSTRING:
    {
      DATE d = x->u.name ? stoD(x->u.name[0]) : 0;
      if (d != (DATE)~0) {
	stD(d,buf);
	return 0;
      }
    }
  default:
    return -1;
  }
  val -= DAYCENTORG;
  if (val < 1 || val >= 65536L)
    return -1;    /* out of range */
  stD((DATE)val,buf);
  return 0;
}

static sql Date_load(Column *this) {
  sql x;
  DATE dt = ldD(this->buf);
  if (!dt) return sql_nul;
  x = mksql(EXDBL);
  x->u.val = ldD(this->buf) + DAYCENTORG;
  return x;
}

static void Date_print(Column *this,enum Column_type type,char *buf) {
  DATE d;
  struct mmddyy mdy;
  if (type < VALUE) {
    Column_print(this,type,buf);
    return;
  }
  if (d = ldD(this->buf)) {
    mdy = doc_to_mdy(d);
#ifdef mmddyy
    sprintf(buf,"%2d/%02d/%02d",mdy.mo,mdy.dy,mdy.yr);
#else
    sprintf(buf,"%2d/%02d/%02d",mdy.mm,mdy.dd,mdy.yy);
#endif
  }
  else
    sprintf(buf,"        ");    /* NULL date */
}

Column *Julian_init(Column *this,char *buf) {
  static struct Column_class Julian_class = {
    sizeof *this, Column_free, Julian_print, Julian_store, Julian_load,
    Column_copy, Column_dup
  };
  if (this || (this = malloc(sizeof *this))) {
    Column_init(this,buf,4);
    this->_class = &Julian_class;
    this->width = this->dlen = 10;
    this->type = BT_DATE;
  }
  return (Column *)this;
}

static int Julian_store(Column *this,sql x,char *buf) {
  double val;
  switch (x->op) {
  case EXCONST:
    if (sgnM(&x->u.num.val) == 0) {
      stlong(0L,buf);
      return 0;
    }
    val = const2dbl(&x->u.num);
    break;
  case EXDBL:
    if (x->u.val == 0) {
      stlong(0L,buf);
      return 0;
    }
    val = x->u.val;
    break;
  case EXSTRING:
    {
      long jday = x->u.name[0] ? stoJ(x->u.name[0]) : 0L;
      if (jday != -1L) {
	stlong(jday,buf);
	return 0;
      }
    }
  default:
    return -1;
  }
  stlong((long)val,buf);
  return 0;
}

static sql Julian_load(Column *this) {
  sql x;
  long dt = ldlong(this->buf);
  if (!dt) return sql_nul;
  x = mksql(EXDBL);
  x->u.val = dt;
  return x;
}

static void Julian_print(Column *this,enum Column_type type,char *buf) {
  long d;
  struct mmddyy mdy;
  if (type < VALUE) {
    Column_print(this,type,buf);
    return;
  }
  d = ldlong(this->buf);
  if (d >= 14) {
    julmdy(d,&mdy);
#ifdef mmddyy
    sprintf(buf,"%2d/%02d/%04d",mdy.mo,mdy.dy,mdy.yr);
#else
    sprintf(buf,"%2d/%02d/%04d",mdy.mm,mdy.dd,mdy.yy);
#endif
  }
  else if (d) {
    static const char table[7][10] = {
      "MONDAY","TUESDAY","WEDNESDAY","THURSDAY","FRIDAY","SATURDAY","SUNDAY"
    };
    sprintf(buf,"%10s",table[(int)d%7]);
  }
  else
    sprintf(buf,"%10s","");
}

struct Time {
  struct Column_class *_class;
/* public: */
  char *name;		/* field name */
  char *desc;		/* description */
  char *buf;		/* buffer address */
  unsigned char len;	/* field length */
  char type;		/* type code */
  short dlen;		/* display length */
  short width;		/* column width (room for dlen & strlen(desc) */
  short tabidx, colidx;	/* hack for quick testing */
/* private: */
  char *fmt;
};

static void Time_free(Column *);

Column *Time_init(Time *this,char *buf) {
  extern Column *Number_dup(Column *,char *);
  static struct Column_class Time_class = {
    sizeof *this, Time_free, Time_print, Time_store, Time_load,
    Column_copy, Number_dup
  };
  if (this || (this = malloc(sizeof *this))) {
    Column_init((Column *)this,buf,4);
    this->_class = &Time_class;
    this->width = this->dlen = 24;
    this->type = BT_TIME;
    this->fmt = 0;	/* default format */
  }
  return (Column *)this;
}

void Time_free(Column *c) {
  Time *this = (Time *)c;
  if (this->fmt) free(this->fmt);
  Column_free(c);
}

Column *Time_init_mask(Time *this,char *buf,const char *mask) {
  if (this = (Time *)Time_init(this,buf)) {
    if (mask && *mask) {
      this->fmt = strdup(mask);
      this->width = this->dlen = strlen(mask);
    }
  }
  return (Column *)this;
}

static int Time_store(Column *this,sql x,char *buf) {
  double val;
  switch (x->op) {
  case EXCONST:
    val = const2dbl(&x->u.num);
    break;
  case EXDBL:
    val = x->u.val;
    break;
  case EXNULL:
    stlong(0L,buf);
  default:
    return -1;
  }
  stlong((long)val,buf);
  return 0;
}

static sql Time_load(Column *this) {
  sql x;
  long t = ldlong(this->buf);
  if (!t || t == -1L) return sql_nul;
  x = mksql(EXDBL);
  x->u.val = t;
  return x;
}

static void Time_print(Column *c,enum Column_type type,char *buf) {
  Time *this = (Time *)c;
  time_t t;
  if (type < VALUE) {
    Column_print(c,type,buf);
    return;
  }
  t = ldlong(this->buf);
  if (t && t != -1L) {
    const char *mask = this->fmt;
    int len;
    if (!mask) mask = "Www Nnn DD hh:mm:ss YYYY";
    len = strlen(mask);
    timemask(t,mask,buf);
    buf[len] = 0;
  }
  else
    sprintf(buf,"%*s",this->width,"");    /* NULL time */
}

#ifdef MC_TIME

static void Julmin_print(Column *,enum Column_type);
static void Julminp_print(Column *,enum Column_type);
static int Julmin_store(Column *,sql,char *);
static sql Julmin_load(Column *);

#include <mcdate.h>

Column *Julmin_init(Column *this,char *buf) {
  static struct Column_class Julmin_class = {
    sizeof *this, Column_free, Julmin_print, Julmin_store, Julmin_load,
    Column_copy, Column_dup
  };
  if (this || (this = malloc(sizeof *this))) {
    Column_init(this,buf,4);
    this->_class = &Julmin_class;
    this->width = this->dlen = 16;
    this->type = MC_TIME;
  }
  return (Column *)this;
}

static int Julmin_store(Column *this,sql x,char *buf) {
  double val;
  switch (x->op) {
  case EXCONST:
    if (sgnM(&x->u.num.val) == 0) {
      stjulmin((JULMIN)0,buf);
      return 0;
    }
    val = const2dbl(&x->u.num);
    break;
  case EXDBL:
    if (x->u.val == 0) {
      stjulmin((JULMIN)0,buf);
      return 0;
    }
    val = x->u.val;
    break;
  default:
    return -1;
  }
  stjulmin((JULMIN)val,buf);
  return 0;
}

static sql Julmin_load(Column *this) {
  sql x;
  long t = ldjulmin(this->buf);
  if (!t) return sql_nul;
  x = mksql(EXDBL);
  x->u.val = ldjulmin(this->buf);
  return x;
}

static void Julmin_print(Column *this,enum Column_type type,char *buf) {
  JULMIN d;
  struct mdy mdy;
  struct hms hms;
  if (type < VALUE) {
    Column_print(this,type,buf);
    return;
  }
  if (d = ldjulmin(this->buf)) {
    mins2mdy(d,&mdy,&hms);
    sprintf(buf,"%2d/%02d/%04d %2d:%02d",
        mdy.mo,mdy.dy,mdy.yr,hms.hour,hms.min,hms.sec);
  }
  else
    sprintf(buf,"                ");    /* NULL date */
}

Column *Julminp_init(Column *this,char *buf) {
  static struct Column_class Julminp_class = {
    sizeof *this, Column_free, Julminp_print, Julmin_store, Julmin_load,
    Column_copy, Column_dup
  };
  if (this || (this = malloc(sizeof *this))) {
    Column_init(this,buf,5);
    this->_class = &Julminp_class;
    this->width = this->dlen = 16;
    this->type = MC_TIME;
  }
  return (Column *)this;
}

static void Julminp_print(Column *this,enum Column_type type,char *buf) {
  struct julp d;
  (void)ldjulp(&d,this->buf);
  if (type < VALUE) {
    Column_print(this,type,buf);
    return;
  }
  if (d.julmins) {
    sprintf(buf,"%16s",julpstr(&d));
  }
  else
    sprintf(buf,"                ");    /* NULL date */
}
#endif
