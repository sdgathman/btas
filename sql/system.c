/*	The system table containing magic fields */
#include <stdio.h>
#include <time.h>
#ifndef __MSDOS__
#include <sys/utsname.h>
int getuid(void), getgid(void);
#else
#include <dos.h>
#endif
#include <string.h>
#include "cursor.h"
#include "coldate.h"
#include <date.h>
#include <btflds.h>
#include "config.h"

static void Cursor_noop(Cursor *);
static void Column_noop(Column *);
static int system_store(Column *,sql,char *);
static int system_first(Cursor *);

static struct Cursor_class system_class = {
  sizeof (Cursor),
  Cursor_noop,
  system_first,Cursor_fail,Cursor_fail,
  Cursor_print, Cursor_fail,Cursor_fail,Cursor_fail,Cursor_optim
};

static void sysdate_print(Column *this, enum Column_type type,char *buf) {
  struct mmddyy mdy;
  if (type < VALUE) {
    Column_print(this,type,buf);
    return;
  }
  mdy = doc_to_mdy(sysD());
#ifdef mmddyy
  sprintf(buf,"%2d/%02d/%02d",mdy.mo,mdy.dy,mdy.yr);
#else
  sprintf(buf,"%2d/%02d/%02d",mdy.mm,mdy.dd,mdy.yy);
#endif
}

static sql sysdate_load(Column *this) {
  sql x = mksql(EXDBL);
  x->u.val = sysD() + (double)DAYCENTORG;
  return x;
}

static struct Column_class sysdate_class = {
  sizeof (Column),Column_noop,sysdate_print,system_store,sysdate_load,
  Column_copy,Column_dup
};

static Column sysdate = {
  &sysdate_class,"sysdate","System date",0,2,BT_DATE,8,8,0,0
};


static void sysname_print(Column *this,enum Column_type type,char *buf) {
  if (type >= VALUE) {
    sprintf(buf,"%-*s",this->dlen,this->buf);
    return;
  }
  Column_print(this,type,buf);
}

static sql sysname_load(Column *this) {
  return mkstring(this->buf);
}

static struct Column_class sysname_class = {
  sizeof (Column),Column_noop,sysname_print,system_store,sysname_load,
  Column_copy,Column_dup
};

static char sysname_str[17];
static char sysnl_str[2] = "\n";

static Column sysname = {
  &sysname_class,"sysname","System node name",sysname_str,16,BT_CHAR,16,16
};

static Column sysnl = {
  &sysname_class,"nl","Newline",sysnl_str,1,BT_CHAR,1,1
};

static void systime_print(Column *this,enum Column_type type,char *buf) {
  time_t t;
  struct tm *p;
  if (type < VALUE) {
    Column_print(this,type,buf);
    return;
  }
  time(&t);
  p = localtime(&t);
  sprintf(buf,"%2d/%02d/%02d %2d:%02d:%02d",
    p->tm_mon + 1,p->tm_mday,p->tm_year,
    p->tm_hour,p->tm_min,p->tm_sec
  );
  /* cftime(buf,"%D %T",&t); */
}

static sql systime_load(Column *this) {
  sql x = mksql(EXDBL);
  x->u.val = time((time_t *)0);
  return x;
}

static struct Column_class systime_class = {
  sizeof (Column),Column_noop,systime_print,system_store,systime_load,
  Column_copy,Column_dup
};

static Column systime = {
  &systime_class,"systime","System time",0,4,BT_TIME,17,17,0,0
};

static void getcwd_print(Column *this,enum Column_type type,char *buf) {
  if (type < VALUE) {
    Column_print(this,type,buf);
    return;
  }
  sprintf(buf,"%-32s",btgetcwd());
}

static sql getcwd_load(Column *this) {
  return mkstring(btgetcwd());
}

static struct Column_class getcwd_class = {
  sizeof (Column),Column_noop,getcwd_print,system_store,getcwd_load,
  Column_copy,Column_dup
};

static Column sysdir = {
  &getcwd_class,"sysdir","Current directory",0,32,BT_CHAR,32,32,0,0
};

static void uid_print(Column *this, enum Column_type type,char *buf) {
  if (type < VALUE) {
    Column_print(this,type,buf);
    return;
  }
  sprintf(buf,"%5d", (this->name[3] == 'u') ? getuid() : getgid());
}

static sql uid_load(Column *this) {
  sconst num;
  num.val = ltoM((long)((this->name[3] == 'u') ? getuid() : getgid()));
  num.fix = 0;
  return mkconst(&num);
}

static struct Column_class uid_class = {
  sizeof (Column),Column_noop,uid_print,system_store,uid_load,
  Column_copy,Column_dup
};

static Column sysuid = {
  &uid_class,"sysuid","User id",0,2,BT_NUM,5,8,0,0
};

static Column sysgid = {
  &uid_class,"sysgid","Group id",0,2,BT_NUM,5,8,0,0
};

#ifdef MC_TIME
#include <mcdate.h>

static void sysmin_print(Column *this, enum Column_type type, char *buf) {
  struct mdy mdy;
  struct hms hms;
  if (type < VALUE) {
    Column_print(this,type,buf);
    return;
  }
  mins2mdy(mc_time(),&mdy,&hms);
  sprintf(buf,"%2d/%02d/%04d %2d:%02d",
      mdy.mo,mdy.dy,mdy.yr,hms.hour,hms.min,hms.sec);
}

static sql sysmin_load(this)
  Column *this;
{
  sql x = mksql(EXDBL);
  x->u.val = mc_time();
  return x;
}

static struct Column_class sysmin_class = {
  sizeof (Column),Column_noop,sysmin_print,Column_fail,sysmin_load,
  Column_copy,Column_dup
};

static Column sysmin = {
  &sysmin_class,"sysmin","System time in minutes",0,4,MC_TIME,16,16,0,0
};
#endif

static  Column *system_cols[] = {
#ifdef MC_TIME
  &sysmin,
#endif
  &sysdate,
  &systime,
  &sysname,
  &sysdir,
  &sysuid, &sysgid,
  &sysnl,
#if 0
  &sysver
#endif
};

#define count(a) sizeof a/sizeof a[0]

Cursor System_table = {
  &system_class, count(system_cols) ,0,system_cols,1,0
};

static void Cursor_noop(Cursor *this) {}

static void Column_noop(Column *this) {}

static int system_store(Column *this,sql x,char *buf) {
  return -1;	/* always fail */
}

static int system_first(Cursor *this) {
#ifndef __MSDOS__
    struct utsname u;
    if (uname(&u))
      strcpy(sysname_str,"N/A");
    else
      strncpy(sysname_str,u.nodename,16);
    sysname_str[16] = 0;
#else
  sprintf(sysname_str,"DOS%d.%d",_osmajor,_osminor);
#endif
  return 0;	/* always succeeds */
}
