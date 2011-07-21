#include <isamx.h>
#include <errenv.h>
#include <ftype.h>
#include <btflds.h>
#include "isdict.h"

static struct keydesc KDict = {
  0, 2, { { 0, FNAMESIZE, CHARTYPE }, { FNAMESIZE, 2, CHARTYPE },  }
};
static const char dictflds[] = { 0,2,BT_CHAR,FNAMESIZE,BT_NUM,2,
	BT_NUM,2,BT_CHAR,8,BT_CHAR,2,BT_NUM,2,BT_CHAR,15,BT_CHAR,24
};

int isdictdel(const char *tblname) {
  struct dictrec drec;
  char fname[sizeof drec.file];
  int rc;
  int fd = isopenx("DATA.DICT",ISINOUT + ISMANULOCK,sizeof drec);
  if (fd < 0) return fd;
  stchar(tblname,fname,sizeof fname);
  envelope
    stchar(tblname,drec.file,sizeof drec.file);
    stshort(0,drec.seq);
    rc = isread(fd,&drec,ISGTEQ);
    while (rc >= 0 && memcmp(drec.file,fname,sizeof fname) == 0) {
      isdelcurr(fd);
      rc = isread(fd,&drec,ISNEXT);
    }
  enverr
    isclose(fd);
    return -1;
  envend
  return isclose(fd);
}

int isdictadd(const char *tblname,const char *colname[],struct btflds *f) {
  struct dictrec drec;
  struct btflds *df = ldflds(0,dictflds,sizeof dictflds);
  int fd = isopenx("DATA.DICT",ISINOUT + ISMANULOCK,sizeof drec);
  int i;
  int rc;
  if (fd < 0) {
    fd = isbuildx("DATA.DICT",sizeof drec,&KDict,ISINOUT+ISMANULOCK,df);
    free(df);
    if (fd < 0) return fd;
  }
  for (i = 0; i < f->rlen; ++i) {
    const char *type = "X";
    char fmt[sizeof drec.fmt + 1];
    int len = f->f[i].len;
    fmt[0] = 0;
    int t = f->f[i].type;
    switch (t) {
    case BT_CHAR:
      type = "C"; break;
    case BT_EBCDIC:
      type = "E"; break;
    case BT_DATE:
      type = "J"; break;
    case BT_TIME:
      type = "T"; break;
    case BT_JULMIN:
      type = "M"; break;
    default:
      if (t >= BT_NUM && t < BT_NUM+13) {
        int prec = t - BT_NUM;
	static char sztbl[] = { 0, 3, 5, 7, 10, 12 };
	int digits = (len < sizeof sztbl) ? sztbl[f->f[i].len] : 14;
        type = "N";
	fmt[0] = '-';
	if (prec > 0 && digits < 14)
	  ++digits;
	memset(fmt+1,'Z',digits);
	fmt[digits+1] = 0;
	if (prec > 0)
	  fmt[digits + 1 - prec] = '.';
      }
    }
    stchar(tblname,drec.file,sizeof drec.file);
    stshort(i,drec.seq);
    stshort(f->f[i].pos,drec.pos);
    stshort(len,drec.len);
    stchar(type,drec.type,sizeof drec.type);
    stchar(colname[i],drec.name,sizeof drec.name);
    stchar(fmt,drec.fmt,sizeof drec.fmt);
    stchar("",drec.desc,sizeof drec.desc);
    iswrite(fd,&drec);
  }
  return 0;
}
